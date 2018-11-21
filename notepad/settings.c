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
  * _T는 유니코드 환경일 때 문자열을 유니코드로 변환함.
  * s_szRegistryKey 값은 Microsoft의 Notepad레지스트리 주소를 저장함
 */
static LPCTSTR s_szRegistryKey = _T("Software\\Microsoft\\Notepad");


static LONG HeightFromPointSize(DWORD dwPointSize)
{	//DWORD는 32bit cpu가 한번에 처리할 수 있는 단위(unsigned long)
    LONG lHeight;
    HDC hDC;

    hDC = GetDC(NULL);//GetDC()는 device context(DC)를 가져오는 함수. NULL을 입력하면 화면 전체에서 DC를 검색함
	/*
	 * DC는 출력에 필요한 정보를 가지는 데이터 구조체. 좌표,색 ,굵기등 출력에 필요한 모든 정보를 담고있다
	 * HDC는 DC를 다루는 handle로, DC의 정보를 저장하는 데이터 구조체의 위치를 가리킴. 포인터가 아닌 실제 객체의 메모리 주소를 가리킴
	 * lHeight 값이 양수면 셀의 높이, 음수면 글자의 높이를 절대값으로 설정한다.
	*/
	//32bit인 첫번째와 두번째를 곱한 64bit의 값을 32bit인 3번째 값으로 나눔. 정수로 반올림됨
    lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
	//GetDeviceCaps()는 지정된 장치의 특정 정보를 검색. LOGPIXELSY은 화면의 논리적 인치 당 픽셀 갯수
	ReleaseDC(NULL, hDC); //ReleaseDC()는 HWND의 DC를 해제함
	//HWND는 윈도우 창을 관리하는 handle. 각 윈도우 창마다 정수값으로 설정됨

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
{	//hkey의 지정한 레지스트를 찾아 복사해주는 함수
    DWORD dwType, cbData;
    LPVOID *pTemp = _alloca(dwResultSize);

	//포인터 pTemp에 dwResultSize 크기만큼 0x00으로 채운다
    ZeroMemory(pTemp, dwResultSize);

    cbData = dwResultSize;
	/*
	 * hkey에서 pszValueNameT의 레지스트를 찾아 타입은 dwType에, 항목은 pTemp에,
	 * 항목을 저장하는데 사용한메모리 공간 크기는 cbData에 저장한다.
	 * 성공하면 ERROR_SUCCESS, 실패하면 에러값을 반환한다.
	*/
    if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
        return FALSE;

	//함수 사용시 예측한 타입이 아니면 false
	if (dwType != dwExpectedType)
		return FALSE;

	//pvResult 메모리 공간에 pTemp의 내용을 cbData바이트만큼 복사한다
    memcpy(pvResult, pTemp, cbData);
    return TRUE;
}

static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult)
{	//DWORD 타입의 레지스트리 복사
    return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult)
{	//DWORD 타입의 레지스트리 복사. 얻은 레지스트리의 잘못된 크기 판별
    DWORD dwResult;
    if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
        return FALSE;
    if (dwResult >= 0x100) //0x100 이상의 값은 잘못된 값
        return FALSE;
    *pbResult = (BYTE) dwResult;
    return TRUE;
}

static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult)
{	//레지스트리를 복사해옴. 가져온 레지스트리의 값이 0이 아니면 true 반환
    DWORD dwResult;
    if (!QueryDword(hKey, pszValueName, &dwResult))
        return FALSE;
    *pbResult = dwResult ? TRUE : FALSE;
    return TRUE;
}

static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{	//문자열 값의 레지스트리를 복사해옴
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
	* SM_CXSCREEN 값은 주 모니터 화면의 너비(픽셀), SM_CYSCREEN 값은 높이
	* 두 값 중 길이가 더 큰쪽을 골라 base_length의 값으로 설정
	*/
    base_length = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN)) ?
                  GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

	//base_length값에서 각 비율을 사용해서 dx, dy 값 설정
    dx = (INT)(base_length * .95);
    dy = dx * 3 / 4;
	//사각형 포인터에 x왼쪽, y위, x오른쪽, y밑 좌표 설정
    SetRect(&Globals.main_rect, 0, 0, dx, dy);

	/*
	 * HKEY_CURRENT_USER의 레지스트리 키에서 s_szRegistryKey 을 찾아 hKey에 반환
	 * 성공 시 ERROR_SUCCESS 반환, 실패 시 에러코드 반환
	*/
    if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) == ERROR_SUCCESS)
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

		//bShowStatusBar 열려있는 것을 닫음??******************************
        /* invert value because DIALOG_ViewStatusBar will be called to show it */
        Globals.bShowStatusBar = !Globals.bShowStatusBar;

		/*
		 * 가져온 dwPointSize의 값으로 폰트의 lfHeight값을 설정.
		 * 지정한 0이 아닌 값이 있으면 HeightFromPointSize()로 논리값으로 변환하여 저장.
		 * 값이 없으면 100을 논리값으로 변환하여 사용.
		 * 100 값을 매직넘버로 변환 가능***********************************************
		*/
        if (dwPointSize != 0)
            Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
        else
            Globals.lfFont.lfHeight = HeightFromPointSize(100);

        RegCloseKey(hKey); //사용한 레지스터키를 닫음
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

static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{	/*
	 * hKey 레지스트리 키의 pszValueNameT에 DWORD타입의 dwValue을 
	 * sizeof(dwValue)만큼 쓴다. 성공하면 ERROR_SUCCESS반환
    */
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
{	//문자열 값의 레지스트리를 저장함
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad.
 */
void NOTEPAD_SaveSettingsToRegistry(void)
{	//현재 Globals가 가지고 있는 설정들을 레지스트리에 저장함
    HKEY hKey;
    DWORD dwDisposition;

	//윈도우 창의 좌표를 main_rect에 저장
    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);
	/*
	 * 레지스트리를 새로 만들며, 이미 존재하는 키일경우 해당 키를 오픈한다.
	 * 루트키 HKEY_CURRENT_USER에서 s_szRegistryKey 서브키를 생성하고
	 * 생성된 키를 hKey가 가리킨다. dwDisposition는 키를 새로 생성했는지, 단순히
	 * 있던 것을 연 것인지 저장함.
	 * 성공시 ERROR_SUCCESS를 반환.
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
		//너비와 높이 저장
        SaveDword(hKey, _T("iWindowPosDX"), Globals.main_rect.right - Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Globals.main_rect.bottom - Globals.main_rect.top);

        RegCloseKey(hKey);
    }
}
