/*
 *  Notepad (text.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* 
 * text.c modified details
 *
 * -modifier : Kang SungMin
 * -modify period : 18.10.25 ~ 18.10.31
 * -analyzed details : As connect given file, read or write all text.
 *  This program can consider encoding type and newline type of text.
 * -implemented functionality : 'write text of program to file',
 *  'read text of file to program'
 */

#include "notepad.h"

static BOOL Append(LPWSTR *ppszText, DWORD *pdwTextLen, LPCWSTR pszAppendText, DWORD dwAppendLen);
BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, DWORD *pdwTextLen, int *pencFile, int *piEoln);
static BOOL WriteEncodedText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int encFile);
BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int encFile, int iEoln);
static BOOL freeLPBYTEBuffer(LPBYTE pAllocBuffer);
static VOID freeLPBYTEBufferSetNull(LPBYTE pAllocBuffer);
static BOOL freeLPWSTRBuffer(LPWSTR pAllocBuffer);
static VOID freeLPWSTRBufferSetNull(LPWSTR pAllocBuffer);

/*
 * ������ �ؽ�Ʈ�� �Է¹��� ������ �߰�
 * -�Ű�����
 * ppszText : ���� �ؽ�Ʈ
 * pdwTextLen : ���� �ؽ�Ʈ ������ ����
 * pszAppendText : �߰��� �ؽ�Ʈ
 * dwAppendLen : �߰��� �ؽ�Ʈ ������ ����
 * -��ȯ : ���� �߰� ���� ����
 */
static BOOL Append(LPWSTR *ppszText, DWORD *pdwTextLen, LPCWSTR pszAppendText, DWORD dwAppendLen)
{
    LPWSTR pszNewText;

    if (dwAppendLen > 0) //�߰��� ������ ���� ���� ����
    {
        if (*ppszText) //���� ������ ������
        {
			//�� �޸� ���� �� ����(���� ������ ���� + �߰� ������ ���� ��ŭ�� �޸� �Ҵ�)
            pszNewText = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, *ppszText, (*pdwTextLen + dwAppendLen) * sizeof(WCHAR));
        }	
        else //���� ������ ���� ���. �߰��� ������ ���̸�ŭ�� �޸� �Ҵ�
        {
            pszNewText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwAppendLen * sizeof(WCHAR));
        }	

        if (!pszNewText) //�޸� �Ҵ� ����
            return FALSE;

		//������ ���� �Ҵ��� �޸��� ���� ������ �޸��ּ� ���Ŀ� �߰� ������ ���̸�ŭ �߰� ������ ����
        memcpy(pszNewText + *pdwTextLen, pszAppendText, dwAppendLen * sizeof(WCHAR)); //=> �������� + �߰�����
        *ppszText = pszNewText; //���� ������ ������ �޸𸮸� ����Ŵ
        *pdwTextLen += dwAppendLen; //������ ���̿� �߰��� ���̸�ŭ ����
    }

    return TRUE;
}

/*
 * ������ ���Ͽ� ����� ���� �б�
 * -�Ű�����
 * hFile : ������ ���� �ڵ鷯
 * ppszText : �о�� �ؽ�Ʈ�� ������ ����
 * pdwTextLen : �о�� �ؽ�Ʈ�� ���̸� ������ ����
 * pencFile : ���� ���ڵ� ������ �Է¹��� ����
 * piEoln : ������ ���๮�� Ÿ���� �Է¹��� ����
 */
BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, DWORD *pdwTextLen, int *pencFile, int *piEoln)
{
    DWORD dwSize;
    LPBYTE pBytes = NULL;
    LPWSTR pszText;
    LPWSTR pszAllocText = NULL;
    DWORD dwPos, i;
    DWORD dwCharCount;
    BYTE byteTemp = 0;
    int encFile = ENCODING_ANSI; //�⺻ �����ڵ� ���� ������ ���
    int iCodePage = 0;
    WCHAR szCrlf[2] = {'\r', '\n'};
    DWORD adwEolnCount[3] = {0, 0, 0}; //�� ���๮���� ����

    *ppszText = NULL;
    *pdwTextLen = 0;

    dwSize = GetFileSize(hFile, NULL); //�ش� ������ ����� ����
	if (dwSize == INVALID_FILE_SIZE) //����� �� ������ ����
		return FALSE;

	//���� ũ�� + 2 ��ŭ �� �޸� �Ҵ�(������ �� ĭ�� ���߿� ���� �Է��� ��)
    pBytes = HeapAlloc(GetProcessHeap(), 0, dwSize + 2);
	if (!pBytes) //�Ҵ� ���� �� ����
	{
		freeLPBYTEBuffer(pBytes);

		return FALSE;
	}

	//������ ������ ������ �Ҵ��� �ڵ鷯�� �ִ� ������ ũ�⸸ŭ �д´�
	if (!ReadFile(hFile, pBytes, dwSize, &dwSize, NULL))
	{
		freeLPBYTEBuffer(pBytes); //�б� ���� �� ����

		return FALSE;
	}

    dwPos = 0;

    /* Make sure that there is a NULL character at the end, in any encoding */
    pBytes[dwSize + 0] = '\0'; //�о�� ������ �������� �ΰ��� �ι� �־� Ȯ���� ���δ´�
    pBytes[dwSize + 1] = '\0';

    /* Look for Byte Order Marks */
	//���� �� ���� ��� ����Ʈ�� �о� �����ڵ� �Ǻ�
    if ((dwSize >= 2) && (pBytes[0] == 0xFF) && (pBytes[1] == 0xFE))
    {
        encFile = ENCODING_UNICODE;
        dwPos += 2;
    }
    else if ((dwSize >= 2) && (pBytes[0] == 0xFE) && (pBytes[1] == 0xFF))
    {
        encFile = ENCODING_UNICODE_BE; //���ڰ� �ٸ�ȯ��(��-�޵��)
        dwPos += 2;
    }
    else if ((dwSize >= 3) && (pBytes[0] == 0xEF) && (pBytes[1] == 0xBB) && (pBytes[2] == 0xBF))
    {
        encFile = ENCODING_UTF8;
        dwPos += 3;
    }

    switch(encFile)
    {
		case ENCODING_UNICODE_BE:
			//����Ʈ ������ 2���� �յ� ������ �ٲ�. ��������� dwPos�� ���� ���� ��ġ�� ����Ŵ
			for (i = dwPos; i < dwSize-1; i += 2)
			{
				byteTemp = pBytes[i+0];
				pBytes[i+0] = pBytes[i+1];
				pBytes[i+1] = byteTemp;
			}
			/* fall through */

		case ENCODING_UNICODE:
			pszText = (LPWSTR) &pBytes[dwPos]; //���� ������ �����ͷ� ����Ŵ
			dwCharCount = (dwSize - dwPos) / sizeof(WCHAR); //������ ���� ����
			break;

		case ENCODING_ANSI:
		case ENCODING_UTF8:
		default :
			if (encFile == ENCODING_ANSI)
				iCodePage = CP_ACP;
			else if (encFile == ENCODING_UTF8)
				iCodePage = CP_UTF8;

			if ((dwSize - dwPos) > 0)
			{	//��Ƽ ����Ʈ�� �����ڵ� ���ڿ��� �ٲپ� ���� ������ �� 
				dwCharCount = MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, NULL, 0);
				if (dwCharCount == 0) //���ڰ� ������ ����
				{
					freeLPBYTEBuffer(pBytes);

					return FALSE;
				}
			}
			else //todo : �� ���Ϸ� �ؼ��Ǵµ� goto done; �� ������� ����
			{
				/* special case for files with no characters (other than BOMs) */
				dwCharCount = 0;
			}

			//���� ���� + 1 ��ŭ�� �޸𸮸� �Ҵ�
			pszAllocText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (dwCharCount + 1) * sizeof(WCHAR));
			if (!pszAllocText) //�Ҵ� ���� �� ����
			{
				freeLPBYTEBuffer(pBytes);
				freeLPWSTRBuffer(pszAllocText);

				return FALSE;
			}

			if ((dwSize - dwPos) > 0)
			{	//���Ͽ� ���ڰ� ������, ��Ƽ ����Ʈ���� �����ڵ� ���ڿ��� �ٲ�. pszAllocText�� dwCharCount����ŭ �����
				if (!MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, pszAllocText, dwCharCount))
				{
					freeLPBYTEBuffer(pBytes);
					freeLPWSTRBuffer(pszAllocText);

					return FALSE;
				}
			}

			pszAllocText[dwCharCount] = '\0'; //�Ҵ�� ���ڿ� �������� �� �߰�
			pszText = pszAllocText; //���� ������ �����ͷ� ����Ŵ
			break;
    }

    dwPos = 0; //���� ���� ���뿡�� �о�� �κ��� ������
    for (i = 0; i < dwCharCount; i++) //���๮���� ������ ��
    {
        switch(pszText[i])
        {
			case '\r':
				if ((i < dwCharCount-1) && (pszText[i+1] == '\n'))
				{
					i++; //��ĭ �� ����
					adwEolnCount[EOLN_CRLF]++;
					break;
				}
				/* fall through */

			case '\n':
				//\n�� \n������ ������ ���� ���뿡 ��ħ
				if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos))
				{
					freeLPBYTEBuffer(pBytes);
					freeLPWSTRBuffer(pszText);
					freeLPWSTRBufferSetNull(*ppszText);
					pdwTextLen = 0;

					return FALSE;
				}
				//���� ���� �������� \r\n�� ��ħ
				if (!Append(ppszText, pdwTextLen, szCrlf, ARRAY_SIZE(szCrlf)))
				{
					freeLPBYTEBuffer(pBytes);
					freeLPWSTRBuffer(pszText);
					freeLPWSTRBufferSetNull(*ppszText);
					pdwTextLen = 0;

					return FALSE;
				}
				dwPos = i + 1; //�о�� ���� ������ ������ ����

				if (pszText[i] == '\r')
					adwEolnCount[EOLN_CR]++; //\n\r
				else
					adwEolnCount[EOLN_LF]++; //�ܼ� ���� \n
				break;

			case '\0': //�� -> ' '
				pszText[i] = ' ';
				break;
			}
    }

    if (!*ppszText && (pszText == pszAllocText))
    {	//���� ������ ������, �߰��� ���뿡 ������ ���� ���
        /* special case; don't need to reallocate */
        *ppszText = pszAllocText; //�ܼ��� �߰��� ����� ���� ������ ���� ����� ���� ������ ��
        *pdwTextLen = dwCharCount;
        pszAllocText = NULL;
    }
    else
    {	//���� \n�� \n������ ���ڿ� ���� ���� �������� ������ ���� ���뿡 ��ħ
        /* append last remaining text */
		if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos + 1))
		{
			freeLPBYTEBuffer(pBytes);
			freeLPWSTRBuffer(pszText);

			return FALSE;
		}
    }

    /* chose which eoln to use */
    *piEoln = EOLN_CRLF; //���� ���� ����� ���๮�� ������ �Ѱ��� �⺻��(CRLF)
    if (adwEolnCount[EOLN_LF] > adwEolnCount[*piEoln])
        *piEoln = EOLN_LF;
    if (adwEolnCount[EOLN_CR] > adwEolnCount[*piEoln])
        *piEoln = EOLN_CR;
    *pencFile = encFile; //������ �����ڵ� ���� �Ѱ���

    return TRUE;
}

/*
 * ������ ���Ͽ� �Է¹��� ���ڵ� �������� �ؽ�Ʈ ����
 * WriteText()���� ȣ��Ǿ� ���Ǹ� ������ ���� ����
*/
static BOOL WriteEncodedText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int encFile)
{
    LPBYTE pBytes = NULL;
    LPBYTE pAllocBuffer = NULL;
    DWORD dwPos = 0;
    DWORD dwByteCount;
    BYTE buffer[1024];
    UINT iCodePage = 0;
    DWORD dwDummy, i;
    int iBufferSize, iRequiredBytes;
    BYTE byteTemp; //UNICODE_BE���� ����Ʈ�� ��ȯ�� �� ����� ����

    while(dwPos < dwTextLen)
    {
        switch(encFile) //�����ڵ� ���Ŀ� ����
        {
            case ENCODING_UNICODE:
                pBytes = (LPBYTE) &pszText[dwPos]; //���� ��ġ�� ��ġ�� ���� ���ĸ� ����Ŵ
                dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR); //���� ��ġ���� ���� �������� ����Ʈ ����
                dwPos = dwTextLen; //���� ��ġ�� ������ ���� ����Ŵ -> while�� ����
                break;

            case ENCODING_UNICODE_BE:
                dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR); //���� ��ġ���� ���� �������� ����Ʈ ����
                if (dwByteCount > sizeof(buffer)) //����Ʈ ������ ���ۺ��� ũ�� ������ ũ��� ����(�ִ밪�� ������ ũ��)
                    dwByteCount = sizeof(buffer);

                memcpy(buffer, &pszText[dwPos], dwByteCount); //���ۿ� ���� ��ġ�� ���� ���� ����Ʈ ������ŭ ����
                for (i = 0; i < dwByteCount; i += 2) //2���� �յڷ� �ڸ��� �ٲ�
                {
					byteTemp = buffer[i+0];
                    buffer[i+0] = buffer[i+1];
                    buffer[i+1] = byteTemp;
                }
                pBytes = (LPBYTE) &buffer[dwPos]; //�����ڵ忡 �°� ����� ������ ����Ŵ
                dwPos += dwByteCount / sizeof(WCHAR); //���� ��ġ + ������ġ�������� ������ ������ = ������ �� ��ġ
                break;

            case ENCODING_ANSI:
            case ENCODING_UTF8:
                if (encFile == ENCODING_ANSI)
                    iCodePage = CP_ACP;
                else if (encFile == ENCODING_UTF8)
                    iCodePage = CP_UTF8;

				//�����ڵ� ���ڿ��� ����Ʈ�� �ٲپ� ������ ��(�ʿ��� ����Ʈ�� ũ��)
                iRequiredBytes = WideCharToMultiByte(iCodePage, 0, &pszText[dwPos], dwTextLen - dwPos, NULL, 0, NULL, NULL);
                if (iRequiredBytes <= 0)
                {
					return FALSE;
                }
                else if (iRequiredBytes < sizeof(buffer))
                {
                    pBytes = buffer;
                    iBufferSize = sizeof(buffer);
                }
                else //�ʿ� ����Ʈ�� ũ�Ⱑ ���ۺ��� ���ų� Ŭ ��
                {
                    pAllocBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, iRequiredBytes);
                    if (!pAllocBuffer)
                        return FALSE;

                    pBytes = pAllocBuffer;
                    iBufferSize = iRequiredBytes;
                }

				//���� ��ġ�������� �������� �����ڵ� ���ڿ� ������ pBytes�� iBufferSize�� ũ�⸸ŭ ��Ƽ ����Ʈ�� ��ȯ. ��ȯ�� ����Ʈ�� ����
                dwByteCount = WideCharToMultiByte(iCodePage, 0, &pszText[dwPos], dwTextLen - dwPos, (LPSTR) pBytes, iBufferSize, NULL, NULL);
				if (!dwByteCount)
				{
					freeLPBYTEBuffer(pAllocBuffer);

					return FALSE;
				}

                dwPos = dwTextLen; //while�� ����
                break;

            default: //���� �����ڵ�� ���� ������ ����
				freeLPBYTEBuffer(pAllocBuffer);

				return FALSE;
        }

		//���Ͽ� pBytes������ dwByteCount����ŭ ��. dwDummy�� ���Ͽ� ������ ����Ʈ ����
		if (!WriteFile(hFile, pBytes, dwByteCount, &dwDummy, NULL))
		{
			freeLPBYTEBuffer(pAllocBuffer);

			return FALSE;
		}

        /* free the buffer, if we have allocated one */
		freeLPBYTEBufferSetNull(pAllocBuffer);
    }

    return TRUE;
}

//������ ���Ͽ� �Է¹��� ���ڵ� ����, ���๮�� Ÿ������ �ؽ�Ʈ ����
BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int encFile, int iEoln)
{
    WCHAR wcBom;
    LPCWSTR pszLF = L"\n";
    DWORD dwPos, dwNext;

    /* Write the proper byte order marks if not ANSI */
    if (encFile != ENCODING_ANSI)
    {
        wcBom = 0xFEFF;
        if (!WriteEncodedText(hFile, &wcBom, 1, encFile))
            return FALSE;
    }

    dwPos = 0;

    /* pszText eoln are always \r\n */

    do
    {
        /* Find the next eoln */
        dwNext = dwPos;
        while(dwNext < dwTextLen) //���๮�ڸ� ã��
        {
            if (pszText[dwNext] == '\r' && pszText[dwNext + 1] == '\n')
                break;
            dwNext++;
        }

        if (dwNext != dwTextLen) //ã�� ���๮�ڰ� ������ �߰�
        {
            switch (iEoln)
            {
            case EOLN_LF:
                /* Write text (without eoln) */
				//���� ��ġ���� ���๮�� ���������� ���� ��ħ
                if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos, encFile))
                    return FALSE;
                /* Write eoln */
				//���� \n�߰�
                if (!WriteEncodedText(hFile, pszLF, 1, encFile))
                    return FALSE;
                break;
            case EOLN_CR:
                /* Write text (including \r as eoln) */
                if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos + 1, encFile))
                    return FALSE;
                break;
            case EOLN_CRLF:
                /* Write text (including \r\n as eoln) */
                if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos + 2, encFile))
                    return FALSE;
                break;
            default:
                return FALSE;
            }
        }
        else
        {	//���� ��ġ���� ������ ���������� ������ �߰���
            /* Write text (without eoln, since this is the end of the file) */
            if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos, encFile))
                return FALSE;
        }

        /* Skip \r\n */
        dwPos = dwNext + 2;
    }
    while (dwPos < dwTextLen);

    return TRUE;
}

static BOOL freeLPBYTEBuffer(LPBYTE pAllocBuffer)
{
	if (pAllocBuffer)
	{
		HeapFree(GetProcessHeap(), 0, pAllocBuffer);

		return TRUE;
	}

	return FALSE;
}

static VOID freeLPBYTEBufferSetNull(LPBYTE pAllocBuffer)
{
	if(freeLPBYTEBuffer(pAllocBuffer))
		pAllocBuffer = NULL;

	return;
}

static BOOL freeLPWSTRBuffer(LPWSTR pAllocBuffer)
{
	if (pAllocBuffer)
	{
		HeapFree(GetProcessHeap(), 0, pAllocBuffer);

		return TRUE;
	}

	return FALSE;
}

static VOID freeLPWSTRBufferSetNull(LPWSTR pAllocBuffer)
{
	if (freeLPWSTRBuffer(pAllocBuffer))
		pAllocBuffer = NULL;

	return;
}