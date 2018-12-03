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

//폰트 크기를 입력받아 화면 한 글자에 출력할 픽셀 수 반환
static LONG HeightFromPointSize(DWORD dwPointSize)
{	
    LONG lHeight;
    HDC hDC;

    hDC = GetDC(NULL);
	/*
	 * lHeight 값이 양수면 셀의 높이, 음수면 글자의 높이를 절대값으로 설정.
	 * 폰트 크기 설정값과 사용자 화면의 인치당 픽셀 수를 이용하여
	 * 화면 한 글자에 출력할 픽셀 수를 계산
	 * 소수점 첫째자리에서 반올림 하기 위해 폰크 크기값은 실제 크기값의 10배 크기로 사용
	*/
    lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
	ReleaseDC(NULL, hDC);

    return lHeight;
}

//화면 한 글자에 출력된 픽셀 수를 입력받아 폰트 크기 반환
static DWORD PointSizeFromHeight(LONG lHeight)
{
    DWORD dwPointSize;
    HDC hDC;

    hDC = GetDC(NULL);
    dwPointSize = -MulDiv(lHeight, 720, GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

	//계산한 폰트 크기값의 소수점 첫째자리에서 반올림
    /* round to nearest multiple of 10 */
    dwPointSize += 5;
    dwPointSize -= dwPointSize % 10;

    return dwPointSize;
}

/*
 * 레지스트리에서 저장된 값을 읽어옴
 * 밑의 각 타입의 함수에서 호출되어 사용
 * -매개변수
 * hKey : 레지스트리 핸들
 * pszValueNameT : 레지스트리 변수명
 * dwExpectedType : 저장된 값의 타입(DWORD, BYTE, BOOL, STRING)
 * pvResult : 저장된 값을 입력받을 변수
 * dwResultSize : 입력받을 값의 크기
 * -반환 : 읽기 성공 여부
 */
static BOOL QueryGeneric(HKEY hKey, LPCTSTR pszValueNameT,
	DWORD dwExpectedType, LPVOID pvResult, DWORD dwResultSize)
{	
    DWORD dwType, cbData;
    LPVOID *pTemp = _alloca(dwResultSize); //읽어온 값을 먼저 저장할 변수

    ZeroMemory(pTemp, dwResultSize); //할당된 메모리에 입력한 크기만큼 0x00로 채움

    cbData = dwResultSize;
	/*
	 * hkey에서 pszValueNameT의 레지스트를 찾아 타입은 dwType에, 항목은 pTemp에,
	 * 항목을 저장하는데 사용한메모리 공간 크기는 cbData에 저장.
	 * 성공하면 ERROR_SUCCESS, 실패하면 에러값을 반환.
	*/
    if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
        return FALSE;

	if (dwType != dwExpectedType) //잘못된 타입의 값을 읽음
		return FALSE;

	//읽어온 값을 매개변수에 복사
    memcpy(pvResult, pTemp, cbData);

    return TRUE;
}

/*
 * DWORD 타입의 레지스트리값 읽기
 * -매개변수
 * hKey : 레지스트리 핸들
 * pszValueName : 읽어올 레지스트리 변수명
 * pdwResult : 저장된 값을 입력받을 변수
 * -반환 : 읽기 성공 여부
 */
static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult)
{
    return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

/*
 * BYTE 타입의 레지스트리값 읽기
 * 먼저 DWORD타입으로 읽어온 후 BYTE로 형변환
 */
static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult)
{	
    DWORD dwResult;
    if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
        return FALSE;
    if (dwResult >= 0x100) //0x100 이상의 값은 잘못된 값
        return FALSE;
    *pbResult = (BYTE) dwResult;

    return TRUE;
}

//BOOL 타입의 레지스트리값 읽기
static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult)
{
    DWORD dwResult;
    if (!QueryDword(hKey, pszValueName, &dwResult))
        return FALSE;
    *pbResult = dwResult ? TRUE : FALSE;

    return TRUE;
}

//STRING 타입의 레지스트리값 읽기
static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{
    return QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultSize * sizeof(TCHAR));
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad.
 * 레지스트리에 저장된 설정값 읽어오기. 저장된 값이 없으면 초기값으로 설정
 */
void NOTEPAD_LoadSettingsFromRegistry(void)
{
    HKEY hKey = NULL;
    HFONT hFont;
    DWORD dwPointSize = 0;
    INT baseLength, dx, dy;

	/*
	 * SM_CXSCREEN 값은 주 모니터 화면의 너비(픽셀), SM_CYSCREEN 값은 높이
	  * 두 값 중 길이가 더 큰쪽을 골라 baseLength의 값으로 설정
	 */
    baseLength = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN)) ?
                  GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

	//baseLength값에서 각 비율을 사용해서 dx, dy 값 설정
    dx = (INT)(baseLength * .95);
    dy = dx * 3 / 4;
	//사각형 포인터에 x왼쪽, y위, x오른쪽, y밑 좌표 설정
    SetRect(&Globals.main_rect, 0, 0, dx, dy);

	/*
	 * HKEY_CURRENT_USER의 레지스트리 키에서 sSzRegistryKey 을 찾아 hKey에 반환
	 * 성공 시 ERROR_SUCCESS 반환, 실패 시 에러코드 반환
	*/
    if (RegOpenKey(HKEY_CURRENT_USER, sSzRegistryKey, &hKey) == ERROR_SUCCESS)
    {	//레지스트리가 존재할 경우 저장된 값을 가져옴
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
		//윈도우 창 위치 정보
        QueryDword(hKey, _T("iMarginLeft"), (DWORD*)&Globals.lMargins.left);
        QueryDword(hKey, _T("iMarginTop"), (DWORD*)&Globals.lMargins.top);
        QueryDword(hKey, _T("iMarginRight"), (DWORD*)&Globals.lMargins.right);
        QueryDword(hKey, _T("iMarginBottom"), (DWORD*)&Globals.lMargins.bottom);

        QueryDword(hKey, _T("iWindowPosX"), (DWORD*)&Globals.main_rect.left);
        QueryDword(hKey, _T("iWindowPosY"), (DWORD*)&Globals.main_rect.top);
        QueryDword(hKey, _T("iWindowPosDX"), (DWORD*)&dx);
        QueryDword(hKey, _T("iWindowPosDY"), (DWORD*)&dy);

		//main 윈도우 창 크기 설정
        Globals.main_rect.right = Globals.main_rect.left + dx;
        Globals.main_rect.bottom = Globals.main_rect.top + dy;

        /* invert value because DIALOG_ViewStatusBar will be called to show it */
        Globals.bShowStatusBar = !Globals.bShowStatusBar;

        if (dwPointSize != 0)
            Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
        else
            Globals.lfFont.lfHeight = HeightFromPointSize(100); //잘못된 값이 저장되있을 경우 초기값으로 설정

        RegCloseKey(hKey); //사용한 레지스터키를 닫음
    }
    else
    {
		//지정한 레지스트리가 없으면 초기값으로 설정
        /* If no settings are found in the registry, then use default values */
        Globals.bShowStatusBar = FALSE;
        Globals.bWrapLongLines = FALSE;
        SetRect(&Globals.lMargins, 750, 1000, 750, 1000);

        /* FIXME: Globals.fSaveWindowPositions = FALSE; */
        /* FIXME: Globals.fMLE_is_broken = FALSE; */

		/*
		 * Globals.hInstance에서 STRING_PAGESETUP_HEADERVALUE을 찾아
		 * 문자배열 Globals.szHeader에 ARRAY_SIZE(Globals.szHeader)크기만큼 가져온다.
		*/
        LoadString(Globals.hInstance, STRING_PAGESETUP_HEADERVALUE, Globals.szHeader,
                   ARRAY_SIZE(Globals.szHeader));
        LoadString(Globals.hInstance, STRING_PAGESETUP_FOOTERVALUE, Globals.szFooter,
                   ARRAY_SIZE(Globals.szFooter));

		//lfFont는 14가지 구조체 정보를 가지고 있는 폰트
        ZeroMemory(&Globals.lfFont, sizeof(Globals.lfFont));
        Globals.lfFont.lfCharSet = ANSI_CHARSET; //ANSI_CHARSET 윈도우즈에서 사용하는 문자셋
		//클리핑  정확도 설정. 영역을 벗어난 부분에 어떻게 처리할지 지정
        Globals.lfFont.lfClipPrecision = CLIP_STROKE_PRECIS;
        Globals.lfFont.lfEscapement = 0; //폰트의 각도 (문자열과 x축의 각도)
        _tcscpy(Globals.lfFont.lfFaceName, _T("Lucida Console")); //폰트명
        Globals.lfFont.lfItalic = FALSE; //이텔릭체
        Globals.lfFont.lfOrientation = 0;  //글자 하나와 x축의 각도
        Globals.lfFont.lfOutPrecision = OUT_STRING_PRECIS; //출력 정밀도
		/*
		 * PitchAndFamily는 폰트의 피치와 그룹을 설정. 피치는 폰트의 폭이 글자마다
		 * 다른지 일정한지 지정. 그룹은 획의 굵기와 세리프 특성이 같은 폰트의 모임.
		 * 피치와 그룹을 | 을 사용해서 설정한다.
		 * 고정피치와 고정 폭이며 세리프는 있을 수도, 없을 수도 있다.
		*/
        Globals.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		//논리적 폰트를 물리적 폰트에 얼마나 근접시킬 것인가 설정.
        Globals.lfFont.lfQuality = PROOF_QUALITY;
        Globals.lfFont.lfStrikeOut = FALSE; //글꼴을 통과하는 가로선의 유무.
        Globals.lfFont.lfUnderline = FALSE; //밑줄선 유무
        Globals.lfFont.lfWeight = FW_NORMAL; //글꼴의 무게 설정
        Globals.lfFont.lfHeight = HeightFromPointSize(100); //글자의 높이 또는 폰트 셀의 높이
    }

    hFont = CreateFontIndirect(&Globals.lfFont); //위에서 설정한 기본값으로 폰트를 생성

	//Globals.hEdit의 WM_SETFONT에 hFont를 정수형 포인터로 넘겨준다
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
    if (hFont)
    {
        if (Globals.hFont) //이미 hFont가 있으면 개체 삭제
            DeleteObject(Globals.hFont);
        Globals.hFont = hFont; //위에서 만든 폰트로 설정
    }
}

//레지스트리에 DWORD 타입으로 설정값 쓰기
static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{	/*
	 * hKey 레지스트리 키의 pszValueNameT에 DWORD타입의 dwValue을 
	 * sizeof(dwValue)만큼 쓴다. 성공하면 ERROR_SUCCESS반환
    */
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

//레지스트리에 STRING 타입으로 설정값 쓰기
static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad.
 * 현재의 설정값들을 레지스트리에 쓰기
 */
void NOTEPAD_SaveSettingsToRegistry(void)
{	//현재 Globals가 가지고 있는 설정들을 레지스트리에 저장함
    HKEY hKey;
    DWORD dwDisposition;

	//윈도우 창의 좌표를 main_rect에 저장
    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);

	/*
	 * 레지스트리를 새로 만들며, 이미 존재하는 키일경우 해당 키를 오픈한다.
	 * 루트키 HKEY_CURRENT_USER에서 sSzRegistryKey 서브키를 생성하고
	 * 생성된 키를 hKey가 가리킨다. dwDisposition는 키를 새로 생성했는지, 단순히
	 * 있던 것을 연 것인지 저장함.
	 * 성공시 ERROR_SUCCESS를 반환.
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
		//너비와 높이 저장
        SaveDword(hKey, _T("iWindowPosDX"), Globals.main_rect.right - Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Globals.main_rect.bottom - Globals.main_rect.top);

        RegCloseKey(hKey);
    }
}
