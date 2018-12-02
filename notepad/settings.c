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

/*
 * settings.c modifined details
 *
 * -modifier : Kang SungMin
 * -analyzed period : 18.10.19 ~ 18.10.26
 * -analyzed details : As connect registry of notepad, set or save settings value.
 *  Specially 'NOTEPAD_LoadSettingsFromRegistry()', 'NOTEPAD_SaveSettingsToRegistrly()'
 *  funcs are declared in 'main.h'.
 *  Original writer leaved 2 FIXME comment.
 *  FIXME: Globals.fSaveWindowPositions = FALSE;
 *  FIXME: Globals.fMLE_is_broken = FALSE;
 *  I don't understand of 'fMLE_is_broken' means.
 * -implemented functionality : 'save setting value to registry',
 *  'get setting value from registry and set to this program'
 */

#include "notepad.h"
#include <winreg.h>

#pragma warning(disable:4996)

static LPCTSTR sSzRegistryKey = _T("Software\\Microsoft\\Notepad");

static LONG HeightFromPointSize(DWORD dwPointSize);
static DWORD PointSizeFromHeight(LONG lHeight);
static BOOL QueryGeneric(HKEY hKey, LPCTSTR pszValueNameT,
	DWORD dwExpectedType, LPVOID pvResult, DWORD dwResultSize);
static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult);
static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult);
static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult);
static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize);
static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue);
static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue);

//��Ʈ ũ�⸦ �Է¹޾� ȭ�� �� ���ڿ� ����� �ȼ� �� ��ȯ
static LONG HeightFromPointSize(DWORD dwPointSize)
{	
    LONG lHeight;
    HDC hDC;

    hDC = GetDC(NULL);
	/*
	 * lHeight ���� ����� ���� ����, ������ ������ ���̸� ���밪���� ����.
	 * ��Ʈ ũ�� �������� ����� ȭ���� ��ġ�� �ȼ� ���� �̿��Ͽ�
	 * ȭ�� �� ���ڿ� ����� �ȼ� ���� ���
	 * �Ҽ��� ù°�ڸ����� �ݿø� �ϱ� ���� ��ũ ũ�Ⱚ�� ���� ũ�Ⱚ�� 10�� ũ��� ���
	*/
    lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
	ReleaseDC(NULL, hDC);

    return lHeight;
}

//ȭ�� �� ���ڿ� ��µ� �ȼ� ���� �Է¹޾� ��Ʈ ũ�� ��ȯ
static DWORD PointSizeFromHeight(LONG lHeight)
{
    DWORD dwPointSize;
    HDC hDC;

    hDC = GetDC(NULL);
    dwPointSize = -MulDiv(lHeight, 720, GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

	//����� ��Ʈ ũ�Ⱚ�� �Ҽ��� ù°�ڸ����� �ݿø�
    /* round to nearest multiple of 10 */
    dwPointSize += 5;
    dwPointSize -= dwPointSize % 10;

    return dwPointSize;
}

/*
 * ������Ʈ������ ����� ���� �о��
 * ���� �� Ÿ���� �Լ����� ȣ��Ǿ� ���
 * -�Ű�����
 * hKey : ������Ʈ�� �ڵ�
 * pszValueNameT : ������Ʈ�� ������
 * dwExpectedType : ����� ���� Ÿ��(DWORD, BYTE, BOOL, STRING)
 * pvResult : ����� ���� �Է¹��� ����
 * dwResultSize : �Է¹��� ���� ũ��
 * -��ȯ : �б� ���� ����
 */
static BOOL QueryGeneric(HKEY hKey, LPCTSTR pszValueNameT,
	DWORD dwExpectedType, LPVOID pvResult, DWORD dwResultSize)
{	
    DWORD dwType, cbData;
    LPVOID *pTemp = _alloca(dwResultSize); //�о�� ���� ���� ������ ����

    ZeroMemory(pTemp, dwResultSize); //�Ҵ�� �޸𸮿� �Է��� ũ�⸸ŭ 0x00�� ä��

    cbData = dwResultSize;
	/*
	 * hkey���� pszValueNameT�� ������Ʈ�� ã�� Ÿ���� dwType��, �׸��� pTemp��,
	 * �׸��� �����ϴµ� ����Ѹ޸� ���� ũ��� cbData�� ����.
	 * �����ϸ� ERROR_SUCCESS, �����ϸ� �������� ��ȯ.
	*/
    if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
        return FALSE;

	if (dwType != dwExpectedType) //�߸��� Ÿ���� ���� ����
		return FALSE;

	//�о�� ���� �Ű������� ����
    memcpy(pvResult, pTemp, cbData);

    return TRUE;
}

/*
 * DWORD Ÿ���� ������Ʈ���� �б�
 * -�Ű�����
 * hKey : ������Ʈ�� �ڵ�
 * pszValueName : �о�� ������Ʈ�� ������
 * pdwResult : ����� ���� �Է¹��� ����
 * -��ȯ : �б� ���� ����
 */
static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult)
{
    return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

/*
 * BYTE Ÿ���� ������Ʈ���� �б�
 * ���� DWORDŸ������ �о�� �� BYTE�� ����ȯ
 */
static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult)
{	
    DWORD dwResult;
    if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
        return FALSE;
    if (dwResult >= 0x100) //0x100 �̻��� ���� �߸��� ��
        return FALSE;
    *pbResult = (BYTE) dwResult;

    return TRUE;
}

//BOOL Ÿ���� ������Ʈ���� �б�
static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult)
{
    DWORD dwResult;
    if (!QueryDword(hKey, pszValueName, &dwResult))
        return FALSE;
    *pbResult = dwResult ? TRUE : FALSE;

    return TRUE;
}

//STRING Ÿ���� ������Ʈ���� �б�
static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{
    return QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultSize * sizeof(TCHAR));
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad.
 * ������Ʈ���� ����� ������ �о����. ����� ���� ������ �ʱⰪ���� ����
 */
void NOTEPAD_LoadSettingsFromRegistry(void)
{
    HKEY hKey = NULL;
    HFONT hFont;
    DWORD dwPointSize = 0;
    INT baseLength, dx, dy;

	/*
	 * SM_CXSCREEN ���� �� ����� ȭ���� �ʺ�(�ȼ�), SM_CYSCREEN ���� ����
	  * �� �� �� ���̰� �� ū���� ��� baseLength�� ������ ����
	 */
    baseLength = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN)) ?
                  GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

	//baseLength������ �� ������ ����ؼ� dx, dy �� ����
    dx = (INT)(baseLength * .95);
    dy = dx * 3 / 4;
	//�簢�� �����Ϳ� x����, y��, x������, y�� ��ǥ ����
    SetRect(&Globals.main_rect, 0, 0, dx, dy);

	/*
	 * HKEY_CURRENT_USER�� ������Ʈ�� Ű���� sSzRegistryKey �� ã�� hKey�� ��ȯ
	 * ���� �� ERROR_SUCCESS ��ȯ, ���� �� �����ڵ� ��ȯ
	*/
    if (RegOpenKey(HKEY_CURRENT_USER, sSzRegistryKey, &hKey) == ERROR_SUCCESS)
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

        /* invert value because DIALOG_ViewStatusBar will be called to show it */
        Globals.bShowStatusBar = !Globals.bShowStatusBar;

        if (dwPointSize != 0)
            Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
        else
            Globals.lfFont.lfHeight = HeightFromPointSize(100); //�߸��� ���� ��������� ��� �ʱⰪ���� ����

        RegCloseKey(hKey); //����� ��������Ű�� ����
    }
    else
    {
		//������ ������Ʈ���� ������ �ʱⰪ���� ����
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

//������Ʈ���� DWORD Ÿ������ ������ ����
static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{	/*
	 * hKey ������Ʈ�� Ű�� pszValueNameT�� DWORDŸ���� dwValue�� 
	 * sizeof(dwValue)��ŭ ����. �����ϸ� ERROR_SUCCESS��ȯ
    */
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

//������Ʈ���� STRING Ÿ������ ������ ����
static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad.
 * ������ ���������� ������Ʈ���� ����
 */
void NOTEPAD_SaveSettingsToRegistry(void)
{	//���� Globals�� ������ �ִ� �������� ������Ʈ���� ������
    HKEY hKey;
    DWORD dwDisposition;

	//������ â�� ��ǥ�� main_rect�� ����
    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);

	/*
	 * ������Ʈ���� ���� �����, �̹� �����ϴ� Ű�ϰ�� �ش� Ű�� �����Ѵ�.
	 * ��ƮŰ HKEY_CURRENT_USER���� sSzRegistryKey ����Ű�� �����ϰ�
	 * ������ Ű�� hKey�� ����Ų��. dwDisposition�� Ű�� ���� �����ߴ���, �ܼ���
	 * �ִ� ���� �� ������ ������.
	 * ������ ERROR_SUCCESS�� ��ȯ.
	 */
    if (RegCreateKeyEx(HKEY_CURRENT_USER, sSzRegistryKey,
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
