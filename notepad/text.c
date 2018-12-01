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

#include "notepad.h"

static BOOL Append(LPWSTR *ppszText, DWORD *pdwTextLen, LPCWSTR pszAppendText, DWORD dwAppendLen);
static BOOL WriteEncodedText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int encFile);
static VOID freeBuffer(LPBYTE pAllocBuffer);

static BOOL Append(LPWSTR *ppszText, DWORD *pdwTextLen, LPCWSTR pszAppendText, DWORD dwAppendLen)
{	//기존에 새로운 내용 추가
    LPWSTR pszNewText;

    if (dwAppendLen > 0)
    {
        if (*ppszText) //원래 내용이 있으면
        {
            pszNewText = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, *ppszText, (*pdwTextLen + dwAppendLen) * sizeof(WCHAR));
        }	//힙 메모리 영역 다시 설정(원래 내용의 길이 + 추가 내용의 길이 만큼의 메모리 할당)
        else
        {
            pszNewText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwAppendLen * sizeof(WCHAR));
        }	//원래 내용이 없으므로 추가할 내용의 길이만큼만 메모리 할당

        if (!pszNewText) //할당 실패
            return FALSE;

		//위에서 새로 할당한 메모리의 원래 내용의 메모리주소 이후에 추가 내용의 길이만큼 추가 내용을 복사
        memcpy(pszNewText + *pdwTextLen, pszAppendText, dwAppendLen * sizeof(WCHAR)); //=> 원래내용 + 추가내용
        *ppszText = pszNewText; //합한 내용을 가지는 메모리를 가리킴
        *pdwTextLen += dwAppendLen; //원래의 길이에 추가된 길이만큼 합함
    }
    return TRUE;
}

BOOL
ReadText(HANDLE hFile, LPWSTR *ppszText, DWORD *pdwTextLen, int *pencFile, int *piEoln)
{
    DWORD dwSize;
    LPBYTE pBytes = NULL;
    LPWSTR pszText; //유니코드 문자열 포인터 타입
    LPWSTR pszAllocText = NULL;
    DWORD dwPos, i;
    DWORD dwCharCount;
    BOOL bSuccess = FALSE;
    BYTE b = 0;
    int encFile = ENCODING_ANSI; //기본 유니코드 종류 값으로 사용
    int iCodePage = 0;
    WCHAR szCrlf[2] = {'\r', '\n'};
    DWORD adwEolnCount[3] = {0, 0, 0};

    *ppszText = NULL;
    *pdwTextLen = 0;

    dwSize = GetFileSize(hFile, NULL); //해당 파일의 사이즈를 구함
    if (dwSize == INVALID_FILE_SIZE) //사용할 수 없으면 종료
        goto done;

	//파일 크기 + 2 만큼 힙 메모리 할당(마지막 두 칸엔 나중에 널을 입력할 것)
    pBytes = HeapAlloc(GetProcessHeap(), 0, dwSize + 2);
    if (!pBytes) //할당 실패 시 종료
        goto done;

	//파일의 내용을 위에서 할당한 핸들러에 최대 파일의 크기만큼 읽는다
    if (!ReadFile(hFile, pBytes, dwSize, &dwSize, NULL))
        goto done; //읽기 실패 시 종료
    dwPos = 0;

    /* Make sure that there is a NUL character at the end, in any encoding */
    pBytes[dwSize + 0] = '\0'; //읽어온 파일의 마지막에 널값을 두번 넣어 확실히 끝맺는다
    pBytes[dwSize + 1] = '\0';

    /* Look for Byte Order Marks */
	//파일 맨 앞의 몇개의 바이트를 읽어 유니코드 판별
    if ((dwSize >= 2) && (pBytes[0] == 0xFF) && (pBytes[1] == 0xFE))
    {
        encFile = ENCODING_UNICODE;
        dwPos += 2;
    }
    else if ((dwSize >= 2) && (pBytes[0] == 0xFE) && (pBytes[1] == 0xFF))
    {
        encFile = ENCODING_UNICODE_BE; //인텔과 다른환경(빅-메디안)
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
		//바이트 단위로 2개씩 앞뒤 순서를 바꿈. 결과적으로 dwPos는 내용 끝의 위치를 가리킴
        for (i = dwPos; i < dwSize-1; i += 2)
        {
            b = pBytes[i+0];
            pBytes[i+0] = pBytes[i+1];
            pBytes[i+1] = b;
        }
        /* fall through */

    case ENCODING_UNICODE:
        pszText = (LPWSTR) &pBytes[dwPos]; //파일 내용을 포인터로 가리킴
        dwCharCount = (dwSize - dwPos) / sizeof(WCHAR); //파일의 문자 갯수
        break;

    case ENCODING_ANSI:
    case ENCODING_UTF8:
        if (encFile == ENCODING_ANSI)
            iCodePage = CP_ACP;
        else if (encFile == ENCODING_UTF8)
            iCodePage = CP_UTF8;

        if ((dwSize - dwPos) > 0)
        {	//멀티 바이트를 유니코드 문자열로 바꾸어 문자 갯수를 셈 
            dwCharCount = MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, NULL, 0);
            if (dwCharCount == 0) //문자가 없으면 종료
                goto done;
        }
        else //todo : 빈 파일로 해석되는데 goto done; 을 사용하지 않음
        {
            /* special case for files with no characters (other than BOMs) */
            dwCharCount = 0;
        }

		//문자 갯수 + 1 만큼의 메모리를 할당
        pszAllocText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (dwCharCount + 1) * sizeof(WCHAR));
        if (!pszAllocText) //할당 실패 시 종료
            goto done;

        if ((dwSize - dwPos) > 0)
        {	//파일에 문자가 있으면, 멀티 바이트에서 유니코드 문자열로 바꿈. pszAllocText에 dwCharCount수만큼 저장됨
            if (!MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, pszAllocText, dwCharCount))
                goto done; //실패시 종료
        }

        pszAllocText[dwCharCount] = '\0'; //할당된 문자열 마지막에 널 추가
        pszText = pszAllocText; //파일 내용을 포인터로 가리킴
        break;
    }

    dwPos = 0; //현재 파일 내용에서 읽어올 부분의 시작점
    for (i = 0; i < dwCharCount; i++) //개행문자의 갯수를 셈
    {
        switch(pszText[i])
        {
        case '\r':
            if ((i < dwCharCount-1) && (pszText[i+1] == '\n'))
            {
                i++; //한칸 더 전진
                adwEolnCount[EOLN_CRLF]++;
                break;
            }
            /* fall through */

        case '\n':
			//\n와 \n사이의 내용을 원래 내용에 합침
            if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos))
                return FALSE;
			//원래 내용 마지막에 \r\n을 합침
            if (!Append(ppszText, pdwTextLen, szCrlf, ARRAY_SIZE(szCrlf)))
                return FALSE;
            dwPos = i + 1; //읽어올 파일 내용의 시작점 전진

            if (pszText[i] == '\r')
                adwEolnCount[EOLN_CR]++; //\n\r
            else
                adwEolnCount[EOLN_LF]++; //단순 개행 \n
            break;

        case '\0': //널 -> ' '
            pszText[i] = ' ';
            break;
        }
    }

    if (!*ppszText && (pszText == pszAllocText))
    {	//원래 내용이 없었고, 추가할 내용에 개행이 없는 경우
        /* special case; don't need to reallocate */
        *ppszText = pszAllocText; //단순히 추가할 내용과 문자 갯수를 현재 내용과 문자 갯수로 함
        *pdwTextLen = dwCharCount;
        pszAllocText = NULL;
    }
    else
    {	//위의 \n과 \n사이의 문자열 다음 파일 끝까지의 내용을 원래 내용에 합침
        /* append last remaining text */
        if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos + 1))
            return FALSE;
    }

    /* chose which eoln to use */
    *piEoln = EOLN_CRLF; //가장 많이 사용한 개행문자 형식을 넘겨줌 기본값(CRLF)
    if (adwEolnCount[EOLN_LF] > adwEolnCount[*piEoln])
        *piEoln = EOLN_LF;
    if (adwEolnCount[EOLN_CR] > adwEolnCount[*piEoln])
        *piEoln = EOLN_CR;
    *pencFile = encFile; //파일의 유니코드 종류 넘겨줌

    bSuccess = TRUE; //파일의 내용을 성공적으로 추가함

done:
    if (pBytes) //파일을 읽어온 두 변수에 메모리를 할당하여 사용했으면 메모리 해제
        HeapFree(GetProcessHeap(), 0, pBytes);
    if (pszAllocText)
        HeapFree(GetProcessHeap(), 0, pszAllocText);

	//파일의 내용 추가에 실패하고, 원래의 내용이 존재하면 해당 내용의 메모리 해제 후 초기화
    if (!bSuccess && *ppszText)
    {
        HeapFree(GetProcessHeap(), 0, *ppszText);
        *ppszText = NULL;
        *pdwTextLen = 0;
    }
    return bSuccess;
}

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
    BYTE b; //UNICODE_BE에서 바이트를 교환할 때 사용할 공간
	//todo : BYTE b; 의 용도가 temp이므로 의미있는 변수명으로 바꿀 수 있음

    while(dwPos < dwTextLen)
    {
        switch(encFile) //유니코드 형식에 따라
        {
            case ENCODING_UNICODE:
                pBytes = (LPBYTE) &pszText[dwPos]; //현재 위치의 위치의 내용 이후를 가리킴
                dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR); //현재 위치에서 내용 끝까지의 바이트 갯수
                dwPos = dwTextLen; //현재 위치는 내용의 끝을 가리킴 -> while문 종료
                break;

            case ENCODING_UNICODE_BE:
                dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR); //현재 위치에서 내용 끝까지의 바이트 갯수
                if (dwByteCount > sizeof(buffer)) //바이트 갯수가 버퍼보다 크면 버퍼의 크기로 지정(최대값이 버퍼의 크기)
                    dwByteCount = sizeof(buffer);

                memcpy(buffer, &pszText[dwPos], dwByteCount); //버퍼에 현재 위치의 내용 부터 바이트 갯수만큼 복사
                for (i = 0; i < dwByteCount; i += 2) //2개씩 앞뒤로 자리를 바꿈
                {
                    b = buffer[i+0];
                    buffer[i+0] = buffer[i+1];
                    buffer[i+1] = b;
                }
                pBytes = (LPBYTE) &buffer[dwPos]; //유니코드에 맞게 변경된 내용을 가리킴
                dwPos += dwByteCount / sizeof(WCHAR); //현재 위치 + 현재위치에서부터 내용의 끝까지 = 내용의 끝 위치
                break;

            case ENCODING_ANSI:
            case ENCODING_UTF8:
                if (encFile == ENCODING_ANSI)
                    iCodePage = CP_ACP;
                else if (encFile == ENCODING_UTF8)
                    iCodePage = CP_UTF8;

				//유니코드 문자열을 바이트로 바꾸어 갯수를 셈(필요한 바이트의 크기)
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
                else //필요 바이트의 크기가 버퍼보다 같거나 클 때
                {
                    pAllocBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, iRequiredBytes);
                    if (!pAllocBuffer)
                        return FALSE;

                    pBytes = pAllocBuffer;
                    iBufferSize = iRequiredBytes;
                }

				//현재 위치에서부터 끝까지의 유니코드 문자열 내용을 pBytes에 iBufferSize의 크기만큼 멀티 바이트로 변환. 반환은 바이트의 갯수
                dwByteCount = WideCharToMultiByte(iCodePage, 0, &pszText[dwPos], dwTextLen - dwPos, (LPSTR) pBytes, iBufferSize, NULL, NULL);
				if (!dwByteCount)
				{
					freeLPBYTEBuffer(pAllocBuffer);

					return FALSE;
				}

                dwPos = dwTextLen; //while문 종료
                break;

            default: //위의 유니코드와 맞지 않으면 종료
				freeLPBYTEBuffer(pAllocBuffer);

				return FALSE;
        }

		//파일에 pBytes내용을 dwByteCount수만큼 씀. dwDummy는 파일에 쓰여진 바이트 갯수
		if (!WriteFile(hFile, pBytes, dwByteCount, &dwDummy, NULL))
		{
			freeBuffer(pAllocBuffer);

			return FALSE;
		}

        /* free the buffer, if we have allocated one */
		freeLPBYTEBufferSetNull(pAllocBuffer);
    }

    return TRUE;
}

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
        while(dwNext < dwTextLen) //개행문자를 찾음
        {
            if (pszText[dwNext] == '\r' && pszText[dwNext + 1] == '\n')
                break;
            dwNext++;
        }

        if (dwNext != dwTextLen) //찾은 개행문자가 내용의 중간
        {
            switch (iEoln)
            {
            case EOLN_LF:
                /* Write text (without eoln) */
				//현재 위치에서 개행문자 이전까지의 내용 합침
                if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos, encFile))
                    return FALSE;
                /* Write eoln */
				//이후 \n추가
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
        {	//현재 위치에서 파일의 마지막까지 내용을 추가함
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

BOOL freeLPBYTEBuffer(LPBYTE pAllocBuffer)
{
	if (pAllocBuffer)
	{
		HeapFree(GetProcessHeap(), 0, pAllocBuffer);

		return TRUE;
	}

	return FALSE;
}

VOID freeLPBYTEBufferSetNull(LPBYTE pAllocBuffer)
{
	if(freeLPBYTEBuffer(pAllocBuffer))
		pAllocBuffer = NULL;

	return;
}

BOOL freeLPWSTRBuffer(LPWSTR pAllocBuffer)
{
	if (pAllocBuffer)
	{
		HeapFree(GetprocessHeap(), 0, pAllocBuffer);

		return TRUE;
	}

	return FALSE;
}

VOID freeLPWSTRBufferSetNull(LPWSTR pAllocBuffer)
{
	if (freeLPWSTRBuffer(pAllocBuffer))
		pAllocBuffer = NULL;

	return;
}