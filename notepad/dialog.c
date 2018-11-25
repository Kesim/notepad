/*
*  Notepad (dialog.c)
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

/* Dialog.c 분석사항
-분석자: 심규림
-분석날짜: 2018.10.25~ 2018.10.30
-분석 내용: Dialog.h는 메모장에서 각 메뉴 안에 항목들을 구현해 놓은 소스파일입니다.
-구현 기능: -파일 열기 -파일 저장 -프린트하기 -메모장닫기
-실행취소 -잘라내기 -붙이기 -삭제 -모두선택 -시간날짜삽입
-하단 상태창띄우기 -폰트설정 -찾기/바꾸기
-도움말 -페이지 설정
*/
#include "notepad.h"
#include <assert.h>
#include <commctrl.h>
#include <strsafe.h>

//zy
#pragma comment(lib,"comctl32.lib") // Can call function in library

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // main.c 에 구현되어 있음. DoCreateEditWindow 내부에서 사용.

static const TCHAR helpfile[] = _T("notepad.hlp"); // 도움말 파일 // help filename
static const TCHAR empty_str[] = _T(""); // 빈 스트링. 에딧 컨트롤의 내용을 비울 때 사용. // empty string 
static const TCHAR szDefaultExt[] = _T("txt"); // 기본 확장자 // default extension
static const TCHAR txt_files[] = _T("*.txt"); // 모든 텍스트 파일 // All text files

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// 가장 최근의 에러를 보여줌 // show recent error
VOID ShowLastError(VOID) 
{
	DWORD error = GetLastError(); // 마지막 에러를 할당 // return recent error
	if (error != NO_ERROR)
	{
		LPTSTR lpMsgBuf = NULL;
		TCHAR szTitle[MAX_STRING_LEN];

		LoadString(Globals.hInstance, STRING_ERROR, szTitle, ARRAY_SIZE(szTitle)); // 에러 값에 해당하는 리소스를 로드 // load resource that maching with error

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, //error에 해당하는 문구가 lpMsgBuf에 들어감 // transfer data from error to lpMsgBuf
			NULL,
			error,
			0,
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);

		MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR); // 오류 내용과 타이틀 제목이 다이얼로그로 출력 // print Dialog about error 
		LocalFree(lpMsgBuf); // 로컬메시지 객체를 해제하고 핸들 무효화 // invalidate handle for localmsg object
	}
}

/**
* Sets the caption of the main window according to Globals.szFileTitle:
 파일명에 따른 메인 창의 캡션을 설정
*    (untitled) - Notepad      if no file is open
*    [filename] - Notepad      if a file is given
*/
void UpdateWindowCaption(BOOL clearModifyAlert) // 메모장 윈도우의 타이틀을 최신화 // update title
{
	TCHAR szCaption[MAX_STRING_LEN]; 
	TCHAR szNotepad[MAX_STRING_LEN]; // 앱 이름
	TCHAR szFilename[MAX_STRING_LEN];

	LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad)); //앱 이름 로드 // Load the name of the application

	if (Globals.szFileTitle[0] != 0) // 기존에 파일이 있으면 -> szFilename에 기존 파일 이름 할당 // allocate existing filename if there is 
		StringCchCopy(szFilename, ARRAY_SIZE(szFilename), Globals.szFileTitle);
	else // 새 파일이면 untitled에 해당하는 리소스 문자열 불러옴 // unless, allocate "untitled"
		LoadString(Globals.hInstance, STRING_UNTITLED, szFilename, ARRAY_SIZE(szFilename));

	/* When a file is being opened or created, there is no need to have the edited flag shown
	when the new or opened file has not been edited yet 
	파일이 열려 있거나 만들어질 때, 아직 편집 안되었으면, 편집되었다는 플래그가 보일 필요 없음
	*/
	if (clearModifyAlert) // TRUE 이면  "파일이름-메모장"의 형식으로 출력
		StringCbPrintf(szCaption, ARRAY_SIZE(szCaption), _T("%s - %s"), szFilename, szNotepad);
	else
	{	// 편집창 내용의 변경 여부 // whether letters of edit window changed
		BOOL isModified = (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0) ? TRUE : FALSE); 

		// Update the caption based upon if the user has modified the contents of the file or not 
		StringCbPrintf(szCaption, ARRAY_SIZE(szCaption), _T("%s%s - %s"), // 변경된 파일이면 *가 붙여서 출력, 아니면 파일이름만
			(isModified ? _T("*") : _T("")), szFilename, szNotepad);
	}

	SetWindowText(Globals.hMainWnd, szCaption); // 타이틀 제목을 할당
}

/* 특정 id의 메시지를 다이얼로그로 띄워주는 역할 : print dialog contains specifed id's msg
	formatId : 특정 메시지에 매칭되는 id  : id that matched specified msg
*/
int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCTSTR szString, DWORD dwFlags) 
{
	TCHAR szMessage[MAX_STRING_LEN];
	TCHAR szResource[MAX_STRING_LEN];

	// formatId에 해당하는 메시지를 리소스로 부터 로드 // Load and format szMessage 
	LoadString(Globals.hInstance, formatId, szResource, ARRAY_SIZE(szResource)); 
	_sntprintf(szMessage, ARRAY_SIZE(szMessage), szResource, szString);

	if ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION) // 느낌표(exclamation)가 있는 다이얼로그는 에러 표시시 // Load szCaption
		LoadString(Globals.hInstance, STRING_ERROR, szResource, ARRAY_SIZE(szResource));
	else
		LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, ARRAY_SIZE(szResource));

	//todo
	// Display Modal Dialog  
	// if (hParent == NULL)
	// hParent = Globals.hMainWnd;
	return MessageBox(hParent, szMessage, szResource, dwFlags); // 다이얼로그 표시
}

// 파일 못 찾았을 때
static void AlertFileNotFound(LPCTSTR szFileName) 
{
	DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION | MB_OK);
}

// 파일 미 저장시 알림
static int AlertFileNotSaved(LPCTSTR szFileName) 
{
	TCHAR szDialogTitle[MAX_STRING_LEN]; 
	LPCTSTR filename;
	int dialogResult;

	if (szFileName[0] == '\0') {
		filename = szFileName;
	}
	else {
		filename = szDialogTitle;
	}
	LoadString(Globals.hInstance, STRING_UNTITLED, szDialogTitle, ARRAY_SIZE(szDialogTitle));

	dialogResult = DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTSAVED, filename, MB_ICONQUESTION | MB_YESNOCANCEL);
	return dialogResult;
}

// 프린트 에러 시
static void AlertPrintError(void) 
{
	TCHAR szUntitled[MAX_STRING_LEN];
	TCHAR filename;

	if (Globals.szFileName[0] == '\0') {
		filename = Globals.szFileName;
	}
	else {
		filename = szUntitled;
	}
	LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, ARRAY_SIZE(szUntitled));

	DIALOG_StringMsgBox(Globals.hMainWnd, STRING_PRINTERROR, filename, MB_ICONEXCLAMATION | MB_OK);
}

/**
* Returns:
*   TRUE  - if file exists
*   FALSE - if file does not exist
  파일이 존재한다면
*/
BOOL FileExists(LPCTSTR szFilename)
{
	WIN32_FIND_DATA entry;
	HANDLE hFile;

	hFile = FindFirstFile(szFilename, &entry); // 파일에 대한 핸들을 할당을 시도
	FindClose(hFile);

	return (hFile != INVALID_HANDLE_VALUE); // 할당이 성공하면 TRUE 리턴
}

// 확장자를 가지고 있는지 (인수: 파일이름)
BOOL HasFileExtension(LPCTSTR szFilename) 
{
	LPCTSTR s;

	// 경로에서 파일이름만 추출(뒤에서부터 해당 문자 만날때까지 parsing  // only parse filename
	s = _tcsrchr(szFilename, _T('\\')); 
	if (s)
		szFilename = s;

	// 파일이름에서 확장자만 추출 // only extract extension // todo : 변수만들어서 해결
	return _tcsrchr(szFilename, _T('.')) != NULL; 
}

/* 선택된 윈도우의 글자 길이 리턴 : letter length of selected window
(인수 hwnd: 윈도우 핸들) : parameter : hwnd : window handle
(리턴: 글자의 길이) return : length of letter
*/
int GetSelectionTextLength(HWND hWnd) 
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	DWORD letterLength;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

	letterLength = dwEnd - dwStart;
	return letterLength;
}

/*선택된 윈도우의 글자 및 길이 반환 
인수 : parameter
  hwnd:윈도우 핸들, : window handle
  LPTSTR: 스트링변수 : string 
(리턴: 글자 수) return: letter length
*/
int GetSelectionText(HWND hWnd, LPTSTR lpString, int nMaxCount) 
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	DWORD dwSize; // 글자 수
	HRESULT hResult; // HRESULT 는 정수타입의 에러를 나타내는 코드 // HRESULT represents error type 
	LPTSTR lpLetterBuf;

	if (!lpString)
	{
		return 0;
	}

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

	if (dwStart == dwEnd) 
	{
		return 0;
	}

	dwSize = GetWindowTextLength(hWnd) + 1; // 글자 수 할당 // allocate letter length
	lpLetterBuf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(TCHAR)); // 있는 글자 수 만큼 버퍼 동적할당 // dynamic allocation
	if (lpLetterBuf == NULL)
	{
		return 0;
	}

	dwSize = GetWindowText(hWnd, lpLetterBuf, dwSize); // lpLetterBuf 에 글자가 들어감 // letter allocated to lpLetterBuf

	if (dwSize == 0) // 글자를 못 받아 오면 error // unless success => error
	{
		HeapFree(GetProcessHeap(), 0, lpLetterBuf);
		return 0;
	}

	hResult = StringCchCopyN(lpString, nMaxCount, lpLetterBuf + dwStart, dwEnd - dwStart);
	HeapFree(GetProcessHeap(), 0, lpLetterBuf);

	switch (hResult)
	{
		case S_OK: // no error
		{
			DWORD letterLength = dwEnd - dwStart;
			return letterLength;
		}

		case STRSAFE_E_INSUFFICIENT_BUFFER: // buffer not enough
		{
			DWORD maxBufSize = nMaxCount - 1;
			return maxBufSize;
		}

		default:
		{
			return 0;
		}
	}
}

/* 프린트 영역을 가리키는 구조체 구하기 
 input : printerHandle
 output: RECT structure
 기능; 물리오프셋, 물리너비/높이, 논리픽셀, 해상도를 윈도에서 가져오기
*/
static RECT
GetPrintingRect(HDC hdcPrintHandle, RECT margins) 
{
	int iLogPixelsX, iLogPixelsY;
	int iHorzResolution, iVertResolution;
	int iPhysPageX, iPhysPageY, iPhysPageW, iPhysPageH;
	RECT rcPrintRect;

	iPhysPageX = GetDeviceCaps(hdcPrintHandle, PHYSICALOFFSETX);
	iPhysPageY = GetDeviceCaps(hdcPrintHandle, PHYSICALOFFSETY);
	iPhysPageW = GetDeviceCaps(hdcPrintHandle, PHYSICALWIDTH);
	iPhysPageH = GetDeviceCaps(hdcPrintHandle, PHYSICALHEIGHT);
	iLogPixelsX = GetDeviceCaps(hdcPrintHandle, LOGPIXELSX);
	iLogPixelsY = GetDeviceCaps(hdcPrintHandle, LOGPIXELSY);
	iHorzResolution = GetDeviceCaps(hdcPrintHandle, HORZRES);
	iVertResolution = GetDeviceCaps(hdcPrintHandle, VERTRES);

	rcPrintRect.left = (margins.left * iLogPixelsX / 2540) - iPhysPageX;
	rcPrintRect.top = (margins.top * iLogPixelsY / 2540) - iPhysPageY;
	rcPrintRect.right = iHorzResolution - (((margins.left * iLogPixelsX / 2540) - iPhysPageX) + ((margins.right * iLogPixelsX / 2540) - (iPhysPageW - iPhysPageX - iHorzResolution)));
	rcPrintRect.bottom = iVertResolution - (((margins.top * iLogPixelsY / 2540) - iPhysPageY) + ((margins.bottom * iLogPixelsY / 2540) - (iPhysPageH - iPhysPageY - iVertResolution)));

	return rcPrintRect;
}

/* 파일 저장을 수행 save files
(리턴:성공여부) return: whether success
*/
static BOOL DoSaveFile(VOID) 
{
	BOOL isSaveSuccess = TRUE;
	HANDLE handleOfFile;
	LPTSTR lpLetterBuf;
	DWORD lettersize;

	handleOfFile = CreateFile(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, // 파일을 새로 생성
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handleOfFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError();
		return FALSE;
	}

	lettersize = GetWindowTextLength(Globals.hEdit) + 1;
	lpLetterBuf = HeapAlloc(GetProcessHeap(), 0, lettersize * sizeof(*lpLetterBuf));
	if (lpLetterBuf == NULL)
	{
		CloseHandle(handleOfFile);
		ShowLastError();
		return FALSE;
	}
	lettersize = GetWindowText(Globals.hEdit, lpLetterBuf, lettersize); // 현재 윈도우 창의 글자를 lpLetterBuf에 담음

	if (lettersize)
	{
		if (WriteText(handleOfFile, (LPWSTR)lpLetterBuf, lettersize, Globals.encFile, Globals.iEoln) == FALSE) // 메모장 파일에 씀(text.c의 함수인 WriteText 이용)
		{
			ShowLastError();
			isSaveSuccess = FALSE;
		}
		else
		{
			SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0); // 저장을 해서 수정이 안 된 상태로 만듬 // make it unmodified mode
			isSaveSuccess = TRUE;
		}
	}

	CloseHandle(handleOfFile);
	HeapFree(GetProcessHeap(), 0, lpLetterBuf);
	return isSaveSuccess;
}

/*
 파일 닫기 수행 :
 Returns:
   TRUE  - User agreed to close (both save/don't save)
   FALSE - User cancelled close by selecting "Cancel"
*/
BOOL DoCloseFile(VOID) 
{
	int dialogResult;

	if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0)) // 파일이 변경되었는지 여부 
	{
		// prompt user to save changes 
		dialogResult = AlertFileNotSaved(Globals.szFileName);
		switch (dialogResult)
		{
		case IDYES:
			if (!DIALOG_FileSave())
				return FALSE;
			break;

		case IDNO:
			break;

		case IDCANCEL:
			return FALSE;

		default:
			return FALSE;
		}
	}

	SetFileName(empty_str);
	UpdateWindowCaption(TRUE); 

	return TRUE;
}

/* 파일 열기 수행 
(인수: 파일이름)
*/
VOID DoOpenFile(LPCTSTR szFileName) 
{
	static const TCHAR logExtension[] = _T(".LOG");
	HANDLE hFile;
	LPTSTR lpTextBuf = NULL;
	DWORD dwTextLen;
	TCHAR log[5];

	/* Close any files and prompt to save changes */
	if (!DoCloseFile()) // 기존파일을 닫는다
		return;

	hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, // 파일을 열기
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ShowLastError();
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		return;
	}

	if (ReadText(hFile, (LPWSTR *)&lpTextBuf, &dwTextLen, &Globals.encFile, &Globals.iEoln) == FALSE) { // 파일에서 pszText로 내용을 읽어오기
		ShowLastError();
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		if (lpTextBuf)
			HeapFree(GetProcessHeap(), 0, lpTextBuf);
		return;
	}
	SetWindowText(Globals.hEdit, lpTextBuf); // 메모장의 에딧 컨트롤에 파일 내용을 적용 // set letters to editControl

	SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0); // 파일 수정여부는 다시 미 수정 상태로 // make it unmodified mode
	SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0); // 실행취소(ctrl+z) 버퍼 초기화 // claer undo buffer
	SetFocus(Globals.hEdit); // 키보드 입력 인식하도록

/*  If the file starts with .LOG, add a time/date at the end and set cursor after
*  See http://support.microsoft.com/?kbid=260563
*/					
	if (_tcscmp(log, logExtension) == 0) { // 로그 파일 일 때 : when open log files
		if (GetWindowText(Globals.hEdit, log, ARRAY_SIZE(log))) {
			static const TCHAR linefeed[] = _T("\r\n");
			static const DWORD endOfText = -1;

			SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), endOfText); // 편집창의 끝으로 포커스를 이동
			SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)linefeed); // 라인피드 삽입
			DIALOG_EditTimeDate(); // 현재 시간을 삽입하기 // insert current time
			SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)linefeed);
		}
	}

	SetFileName(szFileName);
	UpdateWindowCaption(TRUE);
	NOTEPAD_EnableSearchMenu(); // 찾기 메뉴 활성화
	
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (lpTextBuf)
		HeapFree(GetProcessHeap(), 0, lpTextBuf);
}

// 새 파일 만들기 선택 시 호출
VOID DIALOG_FileNew(VOID) 
{
	if (DoCloseFile()) { // 기존 파일 닫기
		SetWindowText(Globals.hEdit, empty_str);
		SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
		SetFocus(Globals.hEdit);
		NOTEPAD_EnableSearchMenu();
	}
}

// 파일 열기 다이얼로그 열기 // open fileopn dialog
VOID DIALOG_FileOpen(VOID) 
{
	OPENFILENAME openfilename; // 파일 정보 담고 있는 구조체 // contains file info
	TCHAR szPath[MAX_PATH];
	BOOL isNewFile = Globals.szFileName[0] == 0 ? TRUE : FALSE;

	ZeroMemory(&openfilename, sizeof(openfilename));

	if (isNewFile)
		_tcscpy(szPath, txt_files);
	else
		_tcscpy(szPath, Globals.szFileName);

	openfilename.lStructSize = sizeof(openfilename);
	openfilename.hwndOwner = Globals.hMainWnd;
	openfilename.hInstance = Globals.hInstance;
	openfilename.lpstrFilter = Globals.szFilter;
	openfilename.lpstrFile = szPath;
	openfilename.nMaxFile = ARRAY_SIZE(szPath);
	openfilename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	openfilename.lpstrDefExt = szDefaultExt; // set default extension

	if (GetOpenFileName(&openfilename)) { // 파일열기 다이얼로그 오픈 // dialog print
		if (FileExists(openfilename.lpstrFile))
			DoOpenFile(openfilename.lpstrFile);
		else
			AlertFileNotFound(openfilename.lpstrFile);
	}
}

BOOL DIALOG_FileSave(VOID) // 파일저장 클릭 시 호출 => 이후 DoSaveFile로 저장 처리
{
	BOOL isNewFile = Globals.szFileName[0] == 0 ? TRUE : FALSE;

	if (isNewFile) { // 새 파일이면 => 다른이름으로 저장과 같은 동작 // if newFile => act like SaveAS
		return DIALOG_FileSaveAs();
	}
	else if (DoSaveFile()) { // 기존파일이면 // if there is current file
		UpdateWindowCaption(TRUE);
		return TRUE;
	}
	return FALSE;
}

// 다른이름으로 저장 수행(콜백함수) // SaveAS Dialog (callback function)
static UINT_PTR
CALLBACK
DIALOG_FileSaveAs_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	TCHAR szTextBuf[128];
	HWND hComboEncode, hComboEOLN;

	UNREFERENCED_PARAMETER(wParam);

	switch (msg)
	{
	case WM_INITDIALOG: // 다이얼로그 오픈 시 초기화
		hComboEncode = GetDlgItem(hDlg, ID_ENCODING); // 인코딩 종류에 관한 핸들포인터 얻기

		LoadString(Globals.hInstance, STRING_ANSI, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf); // 콤보박스에 항목들 더하기 // add items of combobox

		LoadString(Globals.hInstance, STRING_UNICODE, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_UNICODE_BE, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_UTF8, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		SendMessage(hComboEncode, CB_SETCURSEL, Globals.encFile, 0);

		hComboEOLN = GetDlgItem(hDlg, ID_EOLN); // 라인엔드 관한 핸들포인터 얻기

		LoadString(Globals.hInstance, STRING_CRLF, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_LF, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_CR, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		SendMessage(hComboEOLN, CB_SETCURSEL, Globals.iEoln, 0);
		break;

	case WM_NOTIFY: // 다이얼로그 작동 중 메시지
		if (((NMHDR *)lParam)->code == CDN_FILEOK) // OK 버튼 클릭시 설정 사항 반영 // todo: NMHDR 로 오는 구조체가 뭔지 알기
		{
			hComboEncode = GetDlgItem(hDlg, ID_ENCODING);
			if (hComboEncode)
				Globals.encFile = (int)SendMessage(hComboEncode, CB_GETCURSEL, 0, 0);

			hComboEOLN = GetDlgItem(hDlg, ID_EOLN);
			if (hComboEOLN)
				Globals.iEoln = (int)SendMessage(hComboEOLN, CB_GETCURSEL, 0, 0);
		}
		break;
	}
	return 0;
}

//다른이름으로 저장 클릭 시
BOOL DIALOG_FileSaveAs(VOID) 
{
	OPENFILENAME saveAsFileInfo;
	TCHAR szPath[MAX_PATH];
	BOOL isNewFile = Globals.szFileName[0] == 0 ? TRUE: FALSE;

	ZeroMemory(&saveAsFileInfo, sizeof(saveAsFileInfo));

	if (isNewFile)
		_tcscpy(szPath, txt_files);
	else
		_tcscpy(szPath, Globals.szFileName);

	saveAsFileInfo.lStructSize = sizeof(OPENFILENAME);
	saveAsFileInfo.hwndOwner = Globals.hMainWnd;
	saveAsFileInfo.hInstance = Globals.hInstance;
	saveAsFileInfo.lpstrFilter = Globals.szFilter;
	saveAsFileInfo.lpstrFile = szPath;
	saveAsFileInfo.nMaxFile = ARRAY_SIZE(szPath);
	saveAsFileInfo.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
		OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	saveAsFileInfo.lpstrDefExt = szDefaultExt;
	saveAsFileInfo.lpTemplateName = MAKEINTRESOURCE(DIALOG_ENCODING);
	saveAsFileInfo.lpfnHook = DIALOG_FileSaveAs_Hook; // DIALOG_FileSaveAs_Hook 콜백함수 등록

	if (GetSaveFileName(&saveAsFileInfo)) { // 저장 다이얼로그 로드 열기 // dialog Print
		/* HACK: Because in ROS, Save-As boxes don't check the validity
		* of file names and thus, here, szPath can be invalid !! We only
		* see its validity when we call DoSaveFile()... */
		SetFileName(szPath);
		if (DoSaveFile()) {
			UpdateWindowCaption(TRUE);
			return TRUE;
		}
		else {
			SetFileName(_T(""));
			return FALSE;
		}
	}
	else {
		return FALSE;
	}
}

// 파일 프린트
VOID DIALOG_FilePrint(VOID) 
{
	static const TCHAR times_new_roman[] = _T("Times New Roman");
	DOCINFO di;
	TEXTMETRIC tm;
	PRINTDLG printer;
	SIZE szMetric; // rectangle의 너비, 높이 설정
	LOGFONT hdrFont; // 폰트 정보 담은 구조체
	HFONT font, old_font = 0;
	DWORD size;
	LPTSTR pTemp;
	RECT rcPrintRect;
	int border;
	int xLeft, yTop, pagecount, dopage, copycount;
	unsigned int i;

	// Get a small font and print some header info on each page 
	ZeroMemory(&hdrFont, sizeof(hdrFont));
	hdrFont.lfHeight = 100; 
	hdrFont.lfWeight = FW_BOLD; // 글꼴의 가중치를 지정함
	hdrFont.lfCharSet = ANSI_CHARSET; // 문자 집합 지정
	hdrFont.lfOutPrecision = OUT_DEFAULT_PRECIS; // 출력 정밀도 지정
	hdrFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	hdrFont.lfQuality = PROOF_QUALITY; // 출력 품질 지정
	hdrFont.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN; // 글꼴 패밀리를 지정합니다
	_tcscpy(hdrFont.lfFaceName, times_new_roman); // 서체 이름 지정

	font = CreateFontIndirect(&hdrFont); // LOGFONT 구조체를 바탕으로 폰트 생성

	// 기존 세팅을 가져옴 // get current settings							
	ZeroMemory(&printer, sizeof(printer));
	printer.lStructSize = sizeof(printer); // 구조체 크기
	printer.hwndOwner = Globals.hMainWnd; // 대화상자 소유한 윈도우 핸들
	printer.hInstance = Globals.hInstance; // 별도의 대화상자 템플릿 사용시 리소스 가진 인스턴스 핸들

	printer.Flags = PD_RETURNDC | PD_SELECTION; // 대화상자 초기화에 사용 플래그 // Set some default flags 

	// 텍스트 없으면 선택 버튼 없앰 // Disable the selection radio button if there is no text selected
	if (GetSelectionTextLength(Globals.hEdit) == 0)
	{
		printer.Flags = printer.Flags | PD_NOSELECTION;
	}

	printer.nFromPage = 0;
	printer.nMinPage = 1;
	// FIXME : 최대 출력 장수 계산 못함
	printer.nToPage = (WORD)-1;
	printer.nMaxPage = (WORD)-1;

	printer.nCopies = (WORD)PD_USEDEVMODECOPIES;

	printer.hDevMode = Globals.hDevMode; // devmode 구조체를 가지는 전역 메모리 핸들
	printer.hDevNames = Globals.hDevNames; // denames 구조체를 가지는 전역 메모리 핸들

	if (PrintDlg(&printer) == FALSE) // 프린트 다이얼로그를 열기 // open print dialog
	{
		DeleteObject(font);
		return;
	}
	// 다이얼로그에서 설정한 대로 해당 구조체 최신화 // update structures
	Globals.hDevMode = printer.hDevMode;
	Globals.hDevNames = printer.hDevNames;

	assert(printer.hDC != 0); // 디바이스 컨텍스트(프린터) 핸들이 널이면 에러 // if not error occur (debug mode only)

	di.cbSize = sizeof(DOCINFO); // 구조체 크기
	di.lpszDocName = Globals.szFileTitle; // 문서 이름 지정 
	di.lpszOutput = NULL; // 출력 파일 이름 지정 
	di.lpszDatatype = NULL; // 인쇄 작업을 기록하는 데 사용되는 데이터 유형 지정하는 null로 끝나는 문자열의 포인터
	di.fwType = 0; // 인쇄 작업에 대한 추가 정보 

	if (StartDoc(printer.hDC, &di) <= 0) { // 프린트 작업 수행
		DeleteObject(font);
		return;
	}

	if (printer.Flags & PD_SELECTION) { // 편집창의 선택된 글 내용만 가져오기 // Get the file text 
		size = GetSelectionTextLength(Globals.hEdit) + 1; // todo : size => textLength
	}
	else { // 전체 글 내용을 가져오기
		size = GetWindowTextLength(Globals.hEdit) + 1;
	}

	pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(TCHAR)); // 버퍼 할당 // todo : pTemp => pTextBuf
	if (pTemp == NULL) {
		EndDoc(printer.hDC);
		DeleteObject(font);
		ShowLastError();
		return;
	}

	if (printer.Flags & PD_SELECTION) {
		size = GetSelectionText(Globals.hEdit, pTemp, size);
	}
	else {
		size = GetWindowText(Globals.hEdit, pTemp, size);
	}

	// 현재 인쇄 영역을 가져옴 // Get the current printing area 
	rcPrintRect = GetPrintingRect(printer.hDC, Globals.lMargins); // 프린팅 영역 (RECT)
															  
	SetMapMode(printer.hDC, MM_TEXT); // 각 논리 유닛 하나가 한 픽셀에 매치되게 함 // Ensure that each logical unit maps to one pixel 

	GetTextMetrics(printer.hDC, &tm); // 물리적인 폰트의 크기를 알아냄 // Needed to get the correct height of a text line 

	border = 15;
	for (copycount = 1; copycount <= printer.nCopies; copycount++) { // 반복문을 돌며 여러 페이지 출력
		i = 0; // todo: 위치 조정할 필요 있는지 검토
		pagecount = 1;
		do {
			dopage = 0;

			if (printer.Flags & PD_SELECTION) { // The user wants to print the current selection
				dopage = 1;
			}
			if (!(printer.Flags & PD_PAGENUMS) && !(printer.Flags & PD_SELECTION)) { // The user wants to print the entire document 
				dopage = 1;
			}
			if ((pagecount >= printer.nFromPage && pagecount <= printer.nToPage)) { // The user wants to print a specified range of pages 
				dopage = 1;
			}

			old_font = SelectObject(printer.hDC, font); // 인쇄에 설정할 폰트 설정

			if (dopage) {
				if (StartPage(printer.hDC) <= 0) { // 페이지 시작을 알림
					SelectObject(printer.hDC, old_font);
					EndDoc(printer.hDC);
					DeleteDC(printer.hDC); // 메모리에 할당되고 메모리상에서 사용하는 DC를 해제하기 위한 용도
					HeapFree(GetProcessHeap(), 0, pTemp);
					DeleteObject(font); // 메모리에서 해당 오브젝트 삭제
					AlertPrintError();
					return;
				}

				// 프린트 시작의 원점을 2,3번째 인수로 설정 // set 2,3rd parameters as start point 
				SetViewportOrgEx(printer.hDC, rcPrintRect.left, rcPrintRect.top, NULL); 

				// Write a rectangle and header at the top of each page // 사각형을 그림
				// 인수: 핸들 / 좌측 좌표 / 상단 좌표 / 우측 좌표 / 하단 좌표
				Rectangle(printer.hDC, border, border, rcPrintRect.right - border, border + tm.tmHeight * 2);
				
				// todo : I don't know what's up with this TextOut command. This comes out kind of mangled.
				TextOut(printer.hDC,
					border * 2, // x 좌표 
					border + tm.tmHeight / 2, // y 좌표
					Globals.szFileTitle, // 출력 내용(파일 제목)
					lstrlen(Globals.szFileTitle)); // 파일 제목의 길이
			}

			// 머릿말이 끝나고 메인 텍스트 부분이 나타남 // The starting point for the main text 
			xLeft = 0;
			yTop = border + tm.tmHeight * 4; 

			SelectObject(printer.hDC, old_font);

			// Since outputting strings is giving me problems, output the main text one character at a time. 
			do {
				if (pTemp[i] == '\n') { // 줄 넘기기
					xLeft = 0; // todo : 일정한 마진 값을 설정
					yTop += tm.tmHeight;
				}
				else if (pTemp[i] != '\r') { // \r 무시하기
					if (dopage)
						TextOut(printer.hDC, xLeft, yTop, &pTemp[i], 1);

					// We need to get the width for each individual char, since a proportional font may be used 
					GetTextExtentPoint32(printer.hDC, &pTemp[i], 1, &szMetric); // 문자열의 크기 조사 -> szMetric에 저장
					xLeft += szMetric.cx;

					// Insert a line break if the current line does not fit into the printing area 
					if (xLeft > rcPrintRect.right) { // 여백을 초과 시 다음줄로
						xLeft = 0;
						yTop = yTop + tm.tmHeight;
					}
				}
			} while (i++ < size && yTop < rcPrintRect.bottom);

			if (dopage)
				EndPage(printer.hDC); // 페이지 종료
			pagecount++; 
		} while (i < size); // 모든 문자 출력할 때까지 // until all the letters printed
	}

	if (old_font != 0)
		SelectObject(printer.hDC, old_font);
	EndDoc(printer.hDC); // 프린트 종료 
	DeleteDC(printer.hDC);
	HeapFree(GetProcessHeap(), 0, pTemp);
	DeleteObject(font);
}

// 나가기 선택 // execute exit
VOID DIALOG_FileExit(VOID) 
{
	PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

// 실행취소 // select undo
VOID DIALOG_EditUndo(VOID) 
{
	SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

// 잘라내기 // execute cut
VOID DIALOG_EditCut(VOID) 
{
	SendMessage(Globals.hEdit, WM_CUT, 0, 0);
}

//복사하기 // execute copy
VOID DIALOG_EditCopy(VOID) 
{
	SendMessage(Globals.hEdit, WM_COPY, 0, 0);
}

//붙여넣기 // execute paste
VOID DIALOG_EditPaste(VOID) 
{
	SendMessage(Globals.hEdit, WM_PASTE, 0, 0);
}

//지우기 // execute delete
VOID DIALOG_EditDelete(VOID) 
{
	SendMessage(Globals.hEdit, WM_CLEAR, 0, 0);
}

// 모두 선택 // execute "select all"
VOID DIALOG_EditSelectAll(VOID) 
{
	SendMessage(Globals.hEdit, EM_SETSEL, 0, (LPARAM)-1);
}

// 현재 날짜 및 시간 삽입 // insert current date & time
VOID DIALOG_EditTimeDate(VOID) 
{
	SYSTEMTIME st;
	TCHAR szDate[MAX_STRING_LEN];
	TCHAR szText[MAX_STRING_LEN * 2 + 2];

	GetLocalTime(&st);

	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, MAX_STRING_LEN);
	_tcscpy(szText, szDate);
	_tcscat(szText, _T(" "));
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, MAX_STRING_LEN);
	_tcscat(szText, szDate);
	SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText); //날짜와 시간을 붙여 삽입
}

VOID DoCreateStatusBar(VOID) // 상태 바 만들기
{
	RECT rc;
	RECT rcstatus;
	BOOL bStatusBarVisible;

	/* Check if status bar object already exists. */
	if (Globals.hStatusBar == NULL)
	{
		/* Try to create the status bar */
		Globals.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_EX_STATICEDGE, // 상태바 윈도우 만들기
			NULL,
			Globals.hMainWnd,
			CMD_STATUSBAR_WND_ID);

		if (Globals.hStatusBar == NULL)
		{
			ShowLastError();
			return;
		}

		/* Load the string for formatting column/row text output */
		LoadString(Globals.hInstance, STRING_LINE_COLUMN, Globals.szStatusBarLineCol, MAX_PATH - 1);

		/* Set the status bar for single-text output */
		SendMessage(Globals.hStatusBar, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0);
	}

	/* Set status bar visiblity according to the settings. */
	if (Globals.bWrapLongLines == TRUE || Globals.bShowStatusBar == FALSE)
	{
		bStatusBarVisible = FALSE;
		ShowWindow(Globals.hStatusBar, SW_HIDE);
	}
	else
	{
		bStatusBarVisible = TRUE;
		ShowWindow(Globals.hStatusBar, SW_SHOW);
		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
	}

	/* Set check state in show status bar item. */
	if (bStatusBarVisible)
	{
		CheckMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		CheckMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
	}

	/* Update menu mar with the previous changes */
	DrawMenuBar(Globals.hMainWnd); // 메뉴 바 그리기

								   /* Sefety test is edit control exists */
	if (Globals.hEdit != NULL)
	{
		/* Retrieve the sizes of the controls */
		GetClientRect(Globals.hMainWnd, &rc);
		GetClientRect(Globals.hStatusBar, &rcstatus);

		/* If status bar is currently visible, update dimensions of edit control */
		if (bStatusBarVisible)
			rc.bottom -= (rcstatus.bottom - rcstatus.top);

		/* Resize edit control to right size. */
		MoveWindow(Globals.hEdit,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			TRUE);
	}

	/* Update content with current row/column text */
	DIALOG_StatusBarUpdateCaretPos(); // 상태바 위치값 최신화
}

VOID DoCreateEditWindow(VOID) // 편집 창 만들기
{
	DWORD dwStyle;
	int iSize;
	LPTSTR pTemp = NULL;
	BOOL bModified = FALSE;

	iSize = 0;

	/* If the edit control already exists, try to save its content */
	if (Globals.hEdit != NULL)
	{
		/* number of chars currently written into the editor. */
		iSize = GetWindowTextLength(Globals.hEdit);
		if (iSize)
		{
			/* Allocates temporary buffer. */
			pTemp = HeapAlloc(GetProcessHeap(), 0, (iSize + 1) * sizeof(TCHAR));
			if (!pTemp)
			{
				ShowLastError();
				return;
			}

			/* Recover the text into the control. */
			GetWindowText(Globals.hEdit, pTemp, iSize + 1); // 기존에 있던 내용 잠시 백업

			if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
				bModified = TRUE;
		}

		/* Restore original window procedure */
		SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);

		/* Destroy the edit control */
		DestroyWindow(Globals.hEdit); // 현재 에딧 컨트롤 제거
	}

	/* Update wrap status into the main menu and recover style flags */
	if (Globals.bWrapLongLines) // 자동 줄 바꿈 설정 시
	{
		dwStyle = EDIT_STYLE_WRAP;
		EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED); // 상태 바 메뉴 비활성화
	}
	else {
		dwStyle = EDIT_STYLE;
		EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_ENABLED);
	}

	/* Update previous changes */
	DrawMenuBar(Globals.hMainWnd); // 메뉴 바 그리기

								   /* Create the new edit control */
	Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, // 새 에딧 컨트롤 할당
		EDIT_CLASS,
		NULL,
		dwStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		Globals.hMainWnd,
		NULL,
		Globals.hInstance,
		NULL);

	if (Globals.hEdit == NULL)
	{
		if (pTemp)
		{
			HeapFree(GetProcessHeap(), 0, pTemp);
		}

		ShowLastError();
		return;
	}

	SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
	SendMessage(Globals.hEdit, EM_LIMITTEXT, 0, 0);

	/* If some text was previously saved, restore it. */
	if (iSize != 0)
	{
		SetWindowText(Globals.hEdit, pTemp); // 임시저장한 텍스트 내용을 다시 만든 에딧창에 적용
		HeapFree(GetProcessHeap(), 0, pTemp);

		if (bModified)
			SendMessage(Globals.hEdit, EM_SETMODIFY, TRUE, 0);
	}

	/* Sub-class a new window callback for row/column detection. */
	Globals.EditProc = (WNDPROC)SetWindowLongPtr(Globals.hEdit,
		GWLP_WNDPROC,
		(LONG_PTR)EDIT_WndProc);


	DoCreateStatusBar(); // 상태바 만들기

						 /* Finally shows new edit control and set focus into it. */
	ShowWindow(Globals.hEdit, SW_SHOW);
	SetFocus(Globals.hEdit);
}

VOID DIALOG_EditWrap(VOID) // 자동 줄 바꿈 설정 및 해제
{
	Globals.bWrapLongLines = !Globals.bWrapLongLines; // 자동 줄 바꿈 toggle 

	if (Globals.bWrapLongLines)
	{
		EnableMenuItem(Globals.hMenu, CMD_GOTO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED); // "이동" 버튼을 비활성화
	}
	else
	{
		EnableMenuItem(Globals.hMenu, CMD_GOTO, MF_BYCOMMAND | MF_ENABLED);
	}

	DoCreateEditWindow(); // 에딧 창을 다시 만듬(상태창을 만들고 없애기 위해서 에딧창도 다시 만듬)
}

VOID DIALOG_SelectFont(VOID) // 폰트 선택하기
{
	CHOOSEFONT cf;
	LOGFONT lf = Globals.lfFont;

	ZeroMemory(&cf, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = Globals.hMainWnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;

	if (ChooseFont(&cf)) // 폰트 선택 다이얼로그
	{
		HFONT currfont = Globals.hFont;

		Globals.hFont = CreateFontIndirect(&lf);
		Globals.lfFont = lf;
		SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)TRUE); // 폰트 설정
		if (currfont != NULL)
			DeleteObject(currfont);
	}
}

typedef HWND(WINAPI *FINDPROC)(LPFINDREPLACE lpfr);

static VOID DIALOG_SearchDialog(FINDPROC pfnProc) // 검색 다이얼로그
{
	ZeroMemory(&Globals.find, sizeof(Globals.find));
	Globals.find.lStructSize = sizeof(Globals.find);
	Globals.find.hwndOwner = Globals.hMainWnd;
	Globals.find.hInstance = Globals.hInstance;
	Globals.find.lpstrFindWhat = Globals.szFindText;
	Globals.find.wFindWhatLen = ARRAY_SIZE(Globals.szFindText);
	Globals.find.lpstrReplaceWith = Globals.szReplaceText;
	Globals.find.wReplaceWithLen = ARRAY_SIZE(Globals.szReplaceText);
	Globals.find.Flags = FR_DOWN;

	/* We only need to create the modal FindReplace dialog which will */
	/* notify us of incoming events using hMainWnd Window Messages    */

	Globals.hFindReplaceDlg = pfnProc(&Globals.find);
	assert(Globals.hFindReplaceDlg != 0);
}

VOID DIALOG_Search(VOID) // 검색
{
	DIALOG_SearchDialog(FindText);
}

VOID DIALOG_SearchNext(VOID) // 다음 찾기
{
	if (Globals.find.lpstrFindWhat != NULL)
		NOTEPAD_FindNext(&Globals.find, FALSE, TRUE);
	else
		DIALOG_Search();
}

VOID DIALOG_Replace(VOID) // 바꾸기
{
	DIALOG_SearchDialog(ReplaceText);
}

static INT_PTR
CALLBACK
DIALOG_GoTo_DialogProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam) // "이동" 다이얼로그 처리(콜백함수)
{
	BOOL bResult = FALSE;
	HWND hTextBox;
	TCHAR szText[32];

	switch (uMsg) {
	case WM_INITDIALOG:
		hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER);
		_sntprintf(szText, ARRAY_SIZE(szText), _T("%ld"), lParam);
		SetWindowText(hTextBox, szText);
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) // 마우스 버튼 클릭 시
		{
			if (LOWORD(wParam) == IDOK) // OK 버튼 누를 때
			{
				hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER); // 이동할 라인 입력창에 대한 핸들을 받음
				GetWindowText(hTextBox, szText, ARRAY_SIZE(szText));
				EndDialog(hwndDialog, _ttoi(szText)); // 다이얼로그 종료 (szText값을 정수로 변환)
				bResult = TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL) // 취소 버튼 누를 때
			{
				EndDialog(hwndDialog, 0);
				bResult = TRUE;
			}
		}
		break;
	}

	return bResult;
}

VOID DIALOG_GoTo(VOID) // "이동" 다이얼로그
{
	INT_PTR nLine;
	LPTSTR pszText;
	int nLength, i;
	DWORD dwStart, dwEnd;

	nLength = GetWindowTextLength(Globals.hEdit);
	pszText = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(*pszText));
	if (!pszText)
		return;

	/* Retrieve current text */
	GetWindowText(Globals.hEdit, pszText, nLength + 1);
	SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

	nLine = 1;
	for (i = 0; (i < (int)dwStart) && pszText[i]; i++)
	{
		if (pszText[i] == '\n')
			nLine++;
	}

	nLine = DialogBoxParam(Globals.hInstance,
		MAKEINTRESOURCE(DIALOG_GOTO),
		Globals.hMainWnd,
		DIALOG_GoTo_DialogProc, // 이동 다이얼로그
		nLine);

	if (nLine >= 1)
	{
		for (i = 0; pszText[i] && (nLine > 1) && (i < nLength - 1); i++)
		{
			if (pszText[i] == '\n')
				nLine--;
		}
		SendMessage(Globals.hEdit, EM_SETSEL, i, i);
		SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0); // 스크롤을 위치에 맞게 이동
	}
	HeapFree(GetProcessHeap(), 0, pszText);
}

VOID DIALOG_StatusBarUpdateCaretPos(VOID) // 상태바 위치 최신화
{
	int line, col;
	TCHAR buff[MAX_PATH];
	DWORD dwStart, dwSize;

	SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwSize);
	line = SendMessage(Globals.hEdit, EM_LINEFROMCHAR, (WPARAM)dwStart, 0);
	col = dwStart - SendMessage(Globals.hEdit, EM_LINEINDEX, (WPARAM)line, 0); // 현재 칼럼 수

	_stprintf(buff, Globals.szStatusBarLineCol, line + 1, col + 1);
	SendMessage(Globals.hStatusBar, SB_SETTEXT, SB_SIMPLEID, (LPARAM)buff);
}

VOID DIALOG_ViewStatusBar(VOID) // 상태바 보기 및 숨기기
{
	Globals.bShowStatusBar = !Globals.bShowStatusBar;
	DoCreateStatusBar();
}

VOID DIALOG_HelpContents(VOID) // 도움말
{
	WinHelp(Globals.hMainWnd, helpfile, HELP_INDEX, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID) // 메모장 정보
{
	TCHAR szNotepad[MAX_STRING_LEN];
	HICON notepadIcon = LoadIcon(Globals.hInstance, MAKEINTRESOURCE(IDI_NPICON));

	LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));
	ShellAbout(Globals.hMainWnd, szNotepad, 0, notepadIcon);
	DeleteObject(notepadIcon);
}

INT_PTR
CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) // 개발자 정보(작동 안됨)
{
	HWND hLicenseEditWnd;
	TCHAR *strLicense;

	switch (message)
	{
	case WM_INITDIALOG:

		hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE);

		/* 0x1000 should be enough */
		strLicense = (TCHAR *)_alloca(0x1000);
		LoadString(GetModuleHandle(NULL), STRING_LICENSE, strLicense, 0x1000);

		SetWindowText(hLicenseEditWnd, strLicense); // 라이선스 정보 할당

		return TRUE;

	case WM_COMMAND:

		if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
		{
			EndDialog(hDlg, LOWORD(wParam)); // 다이얼로그 종료
			return TRUE;
		}

		break;
	}

	return 0;
}

/***********************************************************************
*
*           DIALOG_FilePageSetup
*/
VOID DIALOG_FilePageSetup(void) // 페이지 설정
{
	PAGESETUPDLG page; // 페이지 정보를 담은 구조체

	ZeroMemory(&page, sizeof(page));
	page.lStructSize = sizeof(page);
	page.hwndOwner = Globals.hMainWnd;
	page.Flags = PSD_ENABLEPAGESETUPTEMPLATE | PSD_ENABLEPAGESETUPHOOK | PSD_MARGINS;
	page.hInstance = Globals.hInstance;
	page.rtMargin = Globals.lMargins;
	page.hDevMode = Globals.hDevMode;
	page.hDevNames = Globals.hDevNames;
	page.lpPageSetupTemplateName = MAKEINTRESOURCE(DIALOG_PAGESETUP);
	page.lpfnPageSetupHook = DIALOG_PAGESETUP_Hook;

	PageSetupDlg(&page);

	Globals.hDevMode = page.hDevMode;
	Globals.hDevNames = page.hDevNames;
	Globals.lMargins = page.rtMargin; // 여백 정보 할당
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*           DIALOG_PAGESETUP_Hook
*/

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) // 페이지 설정 다이얼로그 (콜백함수)
{
	switch (msg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) // 마우스 버튼이 클릭되면
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				/* save user input and close dialog */
				GetDlgItemText(hDlg, 0x141, Globals.szHeader, ARRAY_SIZE(Globals.szHeader)); // 푸터와, 헤더값 설정
				GetDlgItemText(hDlg, 0x143, Globals.szFooter, ARRAY_SIZE(Globals.szFooter));
				return FALSE;

			case IDCANCEL:
				/* discard user input and close dialog */
				return FALSE;

			case IDHELP:
			{
				/* FIXME: Bring this to work */
				static const TCHAR sorry[] = _T("Sorry, no help available");
				static const TCHAR help[] = _T("Help");
				MessageBox(Globals.hMainWnd, sorry, help, MB_ICONEXCLAMATION);
				return TRUE;
			}

			default:
				break;
			}
		}
		break;

	case WM_INITDIALOG:
		/* fetch last user input prior to display dialog */
		SetDlgItemText(hDlg, 0x141, Globals.szHeader); // 푸터와 헤더값 받아오기
		SetDlgItemText(hDlg, 0x143, Globals.szFooter);
		break;
	}

	return FALSE;
}
