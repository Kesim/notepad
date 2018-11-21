/*
 *  Notepad (settings.c)
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

#include <winreg.h>
 /*
  * _T�� �����ڵ� ȯ���� �� ���ڿ��� �����ڵ�� ��ȯ��.
  * s_szRegistryKey ���� Microsoft�� Notepad������Ʈ�� �ּҸ� ������
 */
static LPCTSTR s_szRegistryKey = _T("Software\\Microsoft\\Notepad");


static LONG HeightFromPointSize(DWORD dwPointSize)
{	//DWORD�� 32bit cpu�� �ѹ��� ó���� �� �ִ� ����(unsigned long)
    LONG lHeight;
    HDC hDC;

    hDC = GetDC(NULL);//GetDC()�� device context(DC)�� �������� �Լ�. NULL�� �Է��ϸ� ȭ�� ��ü���� DC�� �˻���
	/*
	 * DC�� ��¿� �ʿ��� ������ ������ ������ ����ü. ��ǥ,�� ,����� ��¿� �ʿ��� ��� ������ ����ִ�
	 * HDC�� DC�� �ٷ�� handle��, DC�� ������ �����ϴ� ������ ����ü�� ��ġ�� ����Ŵ. �����Ͱ� �ƴ� ���� ��ü�� �޸� �ּҸ� ����Ŵ
	 * lHeight ���� ����� ���� ����, ������ ������ ���̸� ���밪���� �����Ѵ�.
	*/
	//32bit�� ù��°�� �ι�°�� ���� 64bit�� ���� 32bit�� 3��° ������ ����. ������ �ݿø���
    lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
	//GetDeviceCaps()�� ������ ��ġ�� Ư�� ������ �˻�. LOGPIXELSY�� ȭ���� ���� ��ġ �� �ȼ� ����
	ReleaseDC(NULL, hDC); //ReleaseDC()�� HWND�� DC�� ������
	//HWND�� ������ â�� �����ϴ� handle. �� ������ â���� ���������� ������

    return lHeight;
}

static DWORD PointSizeFromHeight(LONG lHeight)
{
    DWORD dwPointSize;
    HDC hDC;

    hDC = GetDC(NULL);
    dwPointSize = -MulDiv(lHeight, 720, GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

    /* round to nearest multiple of 10 */
    dwPointSize += 5;
    dwPointSize -= dwPointSize % 10;

    return dwPointSize;
}

static BOOL
QueryGeneric(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwExpectedType,
             LPVOID pvResult, DWORD dwResultSize)
{	//hkey�� ������ ������Ʈ�� ã�� �������ִ� �Լ�
    DWORD dwType, cbData;
    LPVOID *pTemp = _alloca(dwResultSize);

	//������ pTemp�� dwResultSize ũ�⸸ŭ 0x00���� ä���
    ZeroMemory(pTemp, dwResultSize);

    cbData = dwResultSize;
	/*
	 * hkey���� pszValueNameT�� ������Ʈ�� ã�� Ÿ���� dwType��, �׸��� pTemp��,
	 * �׸��� �����ϴµ� ����Ѹ޸� ���� ũ��� cbData�� �����Ѵ�.
	 * �����ϸ� ERROR_SUCCESS, �����ϸ� �������� ��ȯ�Ѵ�.
	*/
    if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
        return FALSE;

	//�Լ� ���� ������ Ÿ���� �ƴϸ� false
	if (dwType != dwExpectedType)
		return FALSE;

	//pvResult �޸� ������ pTemp�� ������ cbData����Ʈ��ŭ �����Ѵ�
    memcpy(pvResult, pTemp, cbData);
    return TRUE;
}

static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult)
{	//DWORD Ÿ���� ������Ʈ�� ����
    return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult)
{	//DWORD Ÿ���� ������Ʈ�� ����. ���� ������Ʈ���� �߸��� ũ�� �Ǻ�
    DWORD dwResult;
    if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
        return FALSE;
    if (dwResult >= 0x100) //0x100 �̻��� ���� �߸��� ��
        return FALSE;
    *pbResult = (BYTE) dwResult;
    return TRUE;
}

static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult)
{	//������Ʈ���� �����ؿ�. ������ ������Ʈ���� ���� 0�� �ƴϸ� true ��ȯ
    DWORD dwResult;
    if (!QueryDword(hKey, pszValueName, &dwResult))
        return FALSE;
    *pbResult = dwResult ? TRUE : FALSE;
    return TRUE;
}

static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{	//���ڿ� ���� ������Ʈ���� �����ؿ�
    return QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultSize * sizeof(TCHAR));
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad.
 */
void NOTEPAD_LoadSettingsFromRegistry(void)
{
    HKEY hKey = NULL;
    HFONT hFont;
    DWORD dwPointSize = 0;
    INT base_length, dx, dy;

	/*
	* SM_CXSCREEN ���� �� ����� ȭ���� �ʺ�(�ȼ�), SM_CYSCREEN ���� ����
	* �� �� �� ���̰� �� ū���� ��� base_length�� ������ ����
	*/
    base_length = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN)) ?
                  GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

	//base_length������ �� ������ ����ؼ� dx, dy �� ����
    dx = (INT)(base_length * .95);
    dy = dx * 3 / 4;
	//�簢�� �����Ϳ� x����, y��, x������, y�� ��ǥ ����
    SetRect(&Globals.main_rect, 0, 0, dx, dy);

	/*
	 * HKEY_CURRENT_USER�� ������Ʈ�� Ű���� s_szRegistryKey �� ã�� hKey�� ��ȯ
	 * ���� �� ERROR_SUCCESS ��ȯ, ���� �� �����ڵ� ��ȯ
	*/
    if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) == ERROR_SUCCESS)
    {	//������Ʈ���� ������ ��� ����� ���� ������
        QueryByte(hKey, _T("lfCharSet"), &Globals.lfFont.lfCharSet);
        QueryByte(hKey, _T("lfClipPrecision"), &Globals.lfFont.lfClipPrecision);
        QueryDword(hKey, _T("lfEscapement"), (DWORD*)&Globals.lfFont.lfEscapement);
        QueryString(hKey, _T("lfFaceName"), Globals.lfFont.lfFaceName, ARRAY_SIZE(Globals.lfFont.lfFaceName));
        QueryByte(hKey, _T("lfItalic"), &Globals.lfFont.lfItalic);
        QueryDword(hKey, _T("lfOrientation"), (DWORD*)&Globals.lfFont.lfOrientation);
        QueryByte(hKey, _T("lfOutPrecision"), &Globals.lfFont.lfOutPrecision);
        QueryByte(hKey, _T("lfPitchAndFamily"), &Globals.lfFont.lfPitchAndFamily);
        QueryByte(hKey, _T("lfQuality"), &Globals.lfFont.lfQuality);
        QueryByte(hKey, _T("lfStrikeOut"), &Globals.lfFont.lfStrikeOut);
        QueryByte(hKey, _T("lfUnderline"), &Globals.lfFont.lfUnderline);
        QueryDword(hKey, _T("lfWeight"), (DWORD*)&Globals.lfFont.lfWeight);
        QueryDword(hKey, _T("iPointSize"), &dwPointSize);
        QueryBool(hKey, _T("fWrap"), &Globals.bWrapLongLines);
        QueryBool(hKey, _T("fStatusBar"), &Globals.bShowStatusBar);
        QueryString(hKey, _T("szHeader"), Globals.szHeader, ARRAY_SIZE(Globals.szHeader));
        QueryString(hKey, _T("szTrailer"), Globals.szFooter, ARRAY_SIZE(Globals.szFooter));
		//������ â ��ġ ����
        QueryDword(hKey, _T("iMarginLeft"), (DWORD*)&Globals.lMargins.left);
        QueryDword(hKey, _T("iMarginTop"), (DWORD*)&Globals.lMargins.top);
        QueryDword(hKey, _T("iMarginRight"), (DWORD*)&Globals.lMargins.right);
        QueryDword(hKey, _T("iMarginBottom"), (DWORD*)&Globals.lMargins.bottom);

        QueryDword(hKey, _T("iWindowPosX"), (DWORD*)&Globals.main_rect.left);
        QueryDword(hKey, _T("iWindowPosY"), (DWORD*)&Globals.main_rect.top);
        QueryDword(hKey, _T("iWindowPosDX"), (DWORD*)&dx);
        QueryDword(hKey, _T("iWindowPosDY"), (DWORD*)&dy);

		//main ������ â ũ�� ����
        Globals.main_rect.right = Globals.main_rect.left + dx;
        Globals.main_rect.bottom = Globals.main_rect.top + dy;

		//bShowStatusBar �����ִ� ���� ����??******************************
        /* invert value because DIALOG_ViewStatusBar will be called to show it */
        Globals.bShowStatusBar = !Globals.bShowStatusBar;

		/*
		 * ������ dwPointSize�� ������ ��Ʈ�� lfHeight���� ����.
		 * ������ 0�� �ƴ� ���� ������ HeightFromPointSize()�� �������� ��ȯ�Ͽ� ����.
		 * ���� ������ 100�� �������� ��ȯ�Ͽ� ���.
		 * 100 ���� �����ѹ��� ��ȯ ����***********************************************
		*/
        if (dwPointSize != 0)
            Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
        else
            Globals.lfFont.lfHeight = HeightFromPointSize(100);

        RegCloseKey(hKey); //����� ��������Ű�� ����
    }
    else
    {
        /* If no settings are found in the registry, then use default values */
        Globals.bShowStatusBar = FALSE;
        Globals.bWrapLongLines = FALSE;
        SetRect(&Globals.lMargins, 750, 1000, 750, 1000);

        /* FIXME: Globals.fSaveWindowPositions = FALSE; */
        /* FIXME: Globals.fMLE_is_broken = FALSE; */

		/*
		 * Globals.hInstance���� STRING_PAGESETUP_HEADERVALUE�� ã��
		 * ���ڹ迭 Globals.szHeader�� ARRAY_SIZE(Globals.szHeader)ũ�⸸ŭ �����´�.
		*/

        LoadString(Globals.hInstance, STRING_PAGESETUP_HEADERVALUE, Globals.szHeader,
                   ARRAY_SIZE(Globals.szHeader));
        LoadString(Globals.hInstance, STRING_PAGESETUP_FOOTERVALUE, Globals.szFooter,
                   ARRAY_SIZE(Globals.szFooter));

		//lfFont�� 14���� ����ü ������ ������ �ִ� ��Ʈ
        ZeroMemory(&Globals.lfFont, sizeof(Globals.lfFont));
        Globals.lfFont.lfCharSet = ANSI_CHARSET; //ANSI_CHARSET ��������� ����ϴ� ���ڼ�
		//Ŭ����  ��Ȯ�� ����. ������ ��� �κп� ��� ó������ ����
        Globals.lfFont.lfClipPrecision = CLIP_STROKE_PRECIS;
        Globals.lfFont.lfEscapement = 0; //��Ʈ�� ���� (���ڿ��� x���� ����)
        _tcscpy(Globals.lfFont.lfFaceName, _T("Lucida Console")); //��Ʈ��
        Globals.lfFont.lfItalic = FALSE; //���ڸ�ü
        Globals.lfFont.lfOrientation = 0;  //���� �ϳ��� x���� ����
        Globals.lfFont.lfOutPrecision = OUT_STRING_PRECIS; //��� ���е�
		/*
		 * PitchAndFamily�� ��Ʈ�� ��ġ�� �׷��� ����. ��ġ�� ��Ʈ�� ���� ���ڸ���
		 * �ٸ��� �������� ����. �׷��� ȹ�� ����� ������ Ư���� ���� ��Ʈ�� ����.
		 * ��ġ�� �׷��� | �� ����ؼ� �����Ѵ�.
		 * ������ġ�� ���� ���̸� �������� ���� ����, ���� ���� �ִ�.
		*/
        Globals.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		//���� ��Ʈ�� ������ ��Ʈ�� �󸶳� ������ų ���ΰ� ����.
        Globals.lfFont.lfQuality = PROOF_QUALITY;
        Globals.lfFont.lfStrikeOut = FALSE; //�۲��� ����ϴ� ���μ��� ����.
        Globals.lfFont.lfUnderline = FALSE; //���ټ� ����
        Globals.lfFont.lfWeight = FW_NORMAL; //�۲��� ���� ����
        Globals.lfFont.lfHeight = HeightFromPointSize(100); //������ ���� �Ǵ� ��Ʈ ���� ����
    }

    hFont = CreateFontIndirect(&Globals.lfFont); //������ ������ �⺻������ ��Ʈ�� ����
	//Globals.hEdit�� WM_SETFONT�� hFont�� ������ �����ͷ� �Ѱ��ش�
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
    if (hFont)
    {
        if (Globals.hFont) //�̹� hFont�� ������ ��ü ����
            DeleteObject(Globals.hFont);
        Globals.hFont = hFont; //������ ���� ��Ʈ�� ����
    }
}

static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{	/*
	 * hKey ������Ʈ�� Ű�� pszValueNameT�� DWORDŸ���� dwValue�� 
	 * sizeof(dwValue)��ŭ ����. �����ϸ� ERROR_SUCCESS��ȯ
    */
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
{	//���ڿ� ���� ������Ʈ���� ������
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad.
 */
void NOTEPAD_SaveSettingsToRegistry(void)
{	//���� Globals�� ������ �ִ� �������� ������Ʈ���� ������
    HKEY hKey;
    DWORD dwDisposition;

	//������ â�� ��ǥ�� main_rect�� ����
    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);
	/*
	 * ������Ʈ���� ���� �����, �̹� �����ϴ� Ű�ϰ�� �ش� Ű�� �����Ѵ�.
	 * ��ƮŰ HKEY_CURRENT_USER���� s_szRegistryKey ����Ű�� �����ϰ�
	 * ������ Ű�� hKey�� ����Ų��. dwDisposition�� Ű�� ���� �����ߴ���, �ܼ���
	 * �ִ� ���� �� ������ ������.
	 * ������ ERROR_SUCCESS�� ��ȯ.
	*/

    if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey,
                       0, NULL, 0, KEY_SET_VALUE, NULL,
                       &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        SaveDword(hKey, _T("lfCharSet"), Globals.lfFont.lfCharSet);
        SaveDword(hKey, _T("lfClipPrecision"), Globals.lfFont.lfClipPrecision);
        SaveDword(hKey, _T("lfEscapement"), Globals.lfFont.lfEscapement);
        SaveString(hKey, _T("lfFaceName"), Globals.lfFont.lfFaceName);
        SaveDword(hKey, _T("lfItalic"), Globals.lfFont.lfItalic);
        SaveDword(hKey, _T("lfOrientation"), Globals.lfFont.lfOrientation);
        SaveDword(hKey, _T("lfOutPrecision"), Globals.lfFont.lfOutPrecision);
        SaveDword(hKey, _T("lfPitchAndFamily"), Globals.lfFont.lfPitchAndFamily);
        SaveDword(hKey, _T("lfQuality"), Globals.lfFont.lfQuality);
        SaveDword(hKey, _T("lfStrikeOut"), Globals.lfFont.lfStrikeOut);
        SaveDword(hKey, _T("lfUnderline"), Globals.lfFont.lfUnderline);
        SaveDword(hKey, _T("lfWeight"), Globals.lfFont.lfWeight);
        SaveDword(hKey, _T("iPointSize"), PointSizeFromHeight(Globals.lfFont.lfHeight));
        SaveDword(hKey, _T("fWrap"), Globals.bWrapLongLines ? 1 : 0);
        SaveDword(hKey, _T("fStatusBar"), Globals.bShowStatusBar ? 1 : 0);
        SaveString(hKey, _T("szHeader"), Globals.szHeader);
        SaveString(hKey, _T("szTrailer"), Globals.szFooter);
        SaveDword(hKey, _T("iMarginLeft"), Globals.lMargins.left);
        SaveDword(hKey, _T("iMarginTop"), Globals.lMargins.top);
        SaveDword(hKey, _T("iMarginRight"), Globals.lMargins.right);
        SaveDword(hKey, _T("iMarginBottom"), Globals.lMargins.bottom);
        SaveDword(hKey, _T("iWindowPosX"), Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosY"), Globals.main_rect.top);
		//�ʺ�� ���� ����
        SaveDword(hKey, _T("iWindowPosDX"), Globals.main_rect.right - Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Globals.main_rect.bottom - Globals.main_rect.top);

        RegCloseKey(hKey);
    }
}
