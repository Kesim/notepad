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

/* Dialog.c �м�����
-�м���: �ɱԸ�
-�м���¥: 2018.10.25~ 2018.10.30
-�м� ����: Dialog.h�� �޸��忡�� �� �޴� �ȿ� �׸���� ������ ���� �ҽ������Դϴ�.
-���� ���: -���� ���� -���� ���� -����Ʈ�ϱ� -�޸���ݱ�
-������� -�߶󳻱� -���̱� -���� -��μ��� -�ð���¥����
-�ϴ� ����â���� -��Ʈ���� -ã��/�ٲٱ�
-���� -������ ����
*/
#include "notepad.h"
#include <assert.h>
#include <commctrl.h>
#include <strsafe.h>

//zy
#pragma comment(lib,"comctl32.lib") // Can call function in library

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // main.c �� �����Ǿ� ����. DoCreateEditWindow ���ο��� ���.

static const TCHAR helpfile[] = _T("notepad.hlp"); // ���� ���� // help filename
static const TCHAR empty_str[] = _T(""); // �� ��Ʈ��. ���� ��Ʈ���� ������ ��� �� ���. // empty string 
static const TCHAR szDefaultExt[] = _T("txt"); // �⺻ Ȯ���� // default extension
static const TCHAR txt_files[] = _T("*.txt"); // ��� �ؽ�Ʈ ���� // All text files

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// ���� �ֱ��� ������ ������ // show recent error
VOID ShowLastError(VOID) 
{
	DWORD error = GetLastError(); // ������ ������ �Ҵ� // return recent error
	if (error != NO_ERROR)
	{
		LPTSTR lpMsgBuf = NULL;
		TCHAR szTitle[MAX_STRING_LEN];

		LoadString(Globals.hInstance, STRING_ERROR, szTitle, ARRAY_SIZE(szTitle)); // ���� ���� �ش��ϴ� ���ҽ��� �ε� // load resource that maching with error

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, //error�� �ش��ϴ� ������ lpMsgBuf�� �� // transfer data from error to lpMsgBuf
			NULL,
			error,
			0,
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);

		MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR); // ���� ����� Ÿ��Ʋ ������ ���̾�α׷� ��� // print Dialog about error 
		LocalFree(lpMsgBuf); // ���ø޽��� ��ü�� �����ϰ� �ڵ� ��ȿȭ // invalidate handle for localmsg object
	}
}

/**
* Sets the caption of the main window according to Globals.szFileTitle:
 ���ϸ� ���� ���� â�� ĸ���� ����
*    (untitled) - Notepad      if no file is open
*    [filename] - Notepad      if a file is given
*/
void UpdateWindowCaption(BOOL clearModifyAlert) // �޸��� �������� Ÿ��Ʋ�� �ֽ�ȭ // update title
{
	TCHAR szCaption[MAX_STRING_LEN]; 
	TCHAR szNotepad[MAX_STRING_LEN]; // �� �̸�
	TCHAR szFilename[MAX_STRING_LEN];

	LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad)); //�� �̸� �ε� // Load the name of the application

	if (Globals.szFileTitle[0] != 0) // ������ ������ ������ -> szFilename�� ���� ���� �̸� �Ҵ� // allocate existing filename if there is 
		StringCchCopy(szFilename, ARRAY_SIZE(szFilename), Globals.szFileTitle);
	else // �� �����̸� untitled�� �ش��ϴ� ���ҽ� ���ڿ� �ҷ��� // unless, allocate "untitled"
		LoadString(Globals.hInstance, STRING_UNTITLED, szFilename, ARRAY_SIZE(szFilename));

	/* When a file is being opened or created, there is no need to have the edited flag shown
	when the new or opened file has not been edited yet 
	������ ���� �ְų� ������� ��, ���� ���� �ȵǾ�����, �����Ǿ��ٴ� �÷��װ� ���� �ʿ� ����
	*/
	if (clearModifyAlert) // TRUE �̸�  "�����̸�-�޸���"�� �������� ���
		StringCbPrintf(szCaption, ARRAY_SIZE(szCaption), _T("%s - %s"), szFilename, szNotepad);
	else
	{	// ����â ������ ���� ���� // whether letters of edit window changed
		BOOL isModified = (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0) ? TRUE : FALSE); 

		// Update the caption based upon if the user has modified the contents of the file or not 
		StringCbPrintf(szCaption, ARRAY_SIZE(szCaption), _T("%s%s - %s"), // ����� �����̸� *�� �ٿ��� ���, �ƴϸ� �����̸���
			(isModified ? _T("*") : _T("")), szFilename, szNotepad);
	}

	SetWindowText(Globals.hMainWnd, szCaption); // Ÿ��Ʋ ������ �Ҵ�
}

/* Ư�� id�� �޽����� ���̾�α׷� ����ִ� ���� : print dialog contains specifed id's msg
	formatId : Ư�� �޽����� ��Ī�Ǵ� id  : id that matched specified msg
*/
int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCTSTR szString, DWORD dwFlags) 
{
	TCHAR szMessage[MAX_STRING_LEN];
	TCHAR szResource[MAX_STRING_LEN];

	// formatId�� �ش��ϴ� �޽����� ���ҽ��� ���� �ε� // Load and format szMessage 
	LoadString(Globals.hInstance, formatId, szResource, ARRAY_SIZE(szResource)); 
	_sntprintf(szMessage, ARRAY_SIZE(szMessage), szResource, szString);

	if ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION) // ����ǥ(exclamation)�� �ִ� ���̾�α״� ���� ǥ�ý� // Load szCaption
		LoadString(Globals.hInstance, STRING_ERROR, szResource, ARRAY_SIZE(szResource));
	else
		LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, ARRAY_SIZE(szResource));

	//todo
	// Display Modal Dialog  
	// if (hParent == NULL)
	// hParent = Globals.hMainWnd;
	return MessageBox(hParent, szMessage, szResource, dwFlags); // ���̾�α� ǥ��
}

// ���� �� ã���� ��
static void AlertFileNotFound(LPCTSTR szFileName) 
{
	DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION | MB_OK);
}

// ���� �� ����� �˸�
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

// ����Ʈ ���� ��
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
  ������ �����Ѵٸ�
*/
BOOL FileExists(LPCTSTR szFilename)
{
	WIN32_FIND_DATA entry;
	HANDLE hFile;

	hFile = FindFirstFile(szFilename, &entry); // ���Ͽ� ���� �ڵ��� �Ҵ��� �õ�
	FindClose(hFile);

	return (hFile != INVALID_HANDLE_VALUE); // �Ҵ��� �����ϸ� TRUE ����
}

// Ȯ���ڸ� ������ �ִ��� (�μ�: �����̸�)
BOOL HasFileExtension(LPCTSTR szFilename) 
{
	LPCTSTR s;

	// ��ο��� �����̸��� ����(�ڿ������� �ش� ���� ���������� parsing  // only parse filename
	s = _tcsrchr(szFilename, _T('\\')); 
	if (s)
		szFilename = s;

	// �����̸����� Ȯ���ڸ� ���� // only extract extension // todo : �������� �ذ�
	return _tcsrchr(szFilename, _T('.')) != NULL; 
}

/* ���õ� �������� ���� ���� ���� : letter length of selected window
(�μ� hwnd: ������ �ڵ�) : parameter : hwnd : window handle
(����: ������ ����) return : length of letter
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

/*���õ� �������� ���� �� ���� ��ȯ 
�μ� : parameter
  hwnd:������ �ڵ�, : window handle
  LPTSTR: ��Ʈ������ : string 
(����: ���� ��) return: letter length
*/
int GetSelectionText(HWND hWnd, LPTSTR lpString, int nMaxCount) 
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	DWORD dwSize; // ���� ��
	HRESULT hResult; // HRESULT �� ����Ÿ���� ������ ��Ÿ���� �ڵ� // HRESULT represents error type 
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

	dwSize = GetWindowTextLength(hWnd) + 1; // ���� �� �Ҵ� // allocate letter length
	lpLetterBuf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(TCHAR)); // �ִ� ���� �� ��ŭ ���� �����Ҵ� // dynamic allocation
	if (lpLetterBuf == NULL)
	{
		return 0;
	}

	dwSize = GetWindowText(hWnd, lpLetterBuf, dwSize); // lpLetterBuf �� ���ڰ� �� // letter allocated to lpLetterBuf

	if (dwSize == 0) // ���ڸ� �� �޾� ���� error // unless success => error
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

/* ����Ʈ ������ ����Ű�� ����ü ���ϱ� 
 input : printerHandle
 output: RECT structure
 ���; ����������, �����ʺ�/����, ���ȼ�, �ػ󵵸� �������� ��������
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

/* ���� ������ ���� save files
(����:��������) return: whether success
*/
static BOOL DoSaveFile(VOID) 
{
	BOOL isSaveSuccess = TRUE;
	HANDLE handleOfFile;
	LPTSTR lpLetterBuf;
	DWORD lettersize;

	handleOfFile = CreateFile(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, // ������ ���� ����
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
	lettersize = GetWindowText(Globals.hEdit, lpLetterBuf, lettersize); // ���� ������ â�� ���ڸ� lpLetterBuf�� ����

	if (lettersize)
	{
		if (WriteText(handleOfFile, (LPWSTR)lpLetterBuf, lettersize, Globals.encFile, Globals.iEoln) == FALSE) // �޸��� ���Ͽ� ��(text.c�� �Լ��� WriteText �̿�)
		{
			ShowLastError();
			isSaveSuccess = FALSE;
		}
		else
		{
			SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0); // ������ �ؼ� ������ �� �� ���·� ���� // make it unmodified mode
			isSaveSuccess = TRUE;
		}
	}

	CloseHandle(handleOfFile);
	HeapFree(GetProcessHeap(), 0, lpLetterBuf);
	return isSaveSuccess;
}

/*
 ���� �ݱ� ���� :
 Returns:
   TRUE  - User agreed to close (both save/don't save)
   FALSE - User cancelled close by selecting "Cancel"
*/
BOOL DoCloseFile(VOID) 
{
	int dialogResult;

	if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0)) // ������ ����Ǿ����� ���� 
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

/* ���� ���� ���� 
(�μ�: �����̸�)
*/
VOID DoOpenFile(LPCTSTR szFileName) 
{
	static const TCHAR logExtension[] = _T(".LOG");
	HANDLE hFile;
	LPTSTR lpTextBuf = NULL;
	DWORD dwTextLen;
	TCHAR log[5];

	/* Close any files and prompt to save changes */
	if (!DoCloseFile()) // ���������� �ݴ´�
		return;

	hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, // ������ ����
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ShowLastError();
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		return;
	}

	if (ReadText(hFile, (LPWSTR *)&lpTextBuf, &dwTextLen, &Globals.encFile, &Globals.iEoln) == FALSE) { // ���Ͽ��� pszText�� ������ �о����
		ShowLastError();
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		if (lpTextBuf)
			HeapFree(GetProcessHeap(), 0, lpTextBuf);
		return;
	}
	SetWindowText(Globals.hEdit, lpTextBuf); // �޸����� ���� ��Ʈ�ѿ� ���� ������ ���� // set letters to editControl

	SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0); // ���� �������δ� �ٽ� �� ���� ���·� // make it unmodified mode
	SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0); // �������(ctrl+z) ���� �ʱ�ȭ // claer undo buffer
	SetFocus(Globals.hEdit); // Ű���� �Է� �ν��ϵ���

/*  If the file starts with .LOG, add a time/date at the end and set cursor after
*  See http://support.microsoft.com/?kbid=260563
*/					
	if (_tcscmp(log, logExtension) == 0) { // �α� ���� �� �� : when open log files
		if (GetWindowText(Globals.hEdit, log, ARRAY_SIZE(log))) {
			static const TCHAR linefeed[] = _T("\r\n");
			static const DWORD endOfText = -1;

			SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), endOfText); // ����â�� ������ ��Ŀ���� �̵�
			SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)linefeed); // �����ǵ� ����
			DIALOG_EditTimeDate(); // ���� �ð��� �����ϱ� // insert current time
			SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)linefeed);
		}
	}

	SetFileName(szFileName);
	UpdateWindowCaption(TRUE);
	NOTEPAD_EnableSearchMenu(); // ã�� �޴� Ȱ��ȭ
	
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (lpTextBuf)
		HeapFree(GetProcessHeap(), 0, lpTextBuf);
}

// �� ���� ����� ���� �� ȣ��
VOID DIALOG_FileNew(VOID) 
{
	if (DoCloseFile()) { // ���� ���� �ݱ�
		SetWindowText(Globals.hEdit, empty_str);
		SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
		SetFocus(Globals.hEdit);
		NOTEPAD_EnableSearchMenu();
	}
}

// ���� ���� ���̾�α� ���� // open fileopn dialog
VOID DIALOG_FileOpen(VOID) 
{
	OPENFILENAME openfilename; // ���� ���� ��� �ִ� ����ü // contains file info
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

	if (GetOpenFileName(&openfilename)) { // ���Ͽ��� ���̾�α� ���� // dialog print
		if (FileExists(openfilename.lpstrFile))
			DoOpenFile(openfilename.lpstrFile);
		else
			AlertFileNotFound(openfilename.lpstrFile);
	}
}

BOOL DIALOG_FileSave(VOID) // �������� Ŭ�� �� ȣ�� => ���� DoSaveFile�� ���� ó��
{
	BOOL isNewFile = Globals.szFileName[0] == 0 ? TRUE : FALSE;

	if (isNewFile) { // �� �����̸� => �ٸ��̸����� ����� ���� ���� // if newFile => act like SaveAS
		return DIALOG_FileSaveAs();
	}
	else if (DoSaveFile()) { // ���������̸� // if there is current file
		UpdateWindowCaption(TRUE);
		return TRUE;
	}
	return FALSE;
}

// �ٸ��̸����� ���� ����(�ݹ��Լ�) // SaveAS Dialog (callback function)
static UINT_PTR
CALLBACK
DIALOG_FileSaveAs_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	TCHAR szTextBuf[128];
	HWND hComboEncode, hComboEOLN;

	UNREFERENCED_PARAMETER(wParam);

	switch (msg)
	{
	case WM_INITDIALOG: // ���̾�α� ���� �� �ʱ�ȭ
		hComboEncode = GetDlgItem(hDlg, ID_ENCODING); // ���ڵ� ������ ���� �ڵ������� ���

		LoadString(Globals.hInstance, STRING_ANSI, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf); // �޺��ڽ��� �׸�� ���ϱ� // add items of combobox

		LoadString(Globals.hInstance, STRING_UNICODE, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_UNICODE_BE, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_UTF8, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEncode, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		SendMessage(hComboEncode, CB_SETCURSEL, Globals.encFile, 0);

		hComboEOLN = GetDlgItem(hDlg, ID_EOLN); // ���ο��� ���� �ڵ������� ���

		LoadString(Globals.hInstance, STRING_CRLF, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_LF, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		LoadString(Globals.hInstance, STRING_CR, szTextBuf, ARRAY_SIZE(szTextBuf));
		SendMessage(hComboEOLN, CB_ADDSTRING, 0, (LPARAM)szTextBuf);

		SendMessage(hComboEOLN, CB_SETCURSEL, Globals.iEoln, 0);
		break;

	case WM_NOTIFY: // ���̾�α� �۵� �� �޽���
		if (((NMHDR *)lParam)->code == CDN_FILEOK) // OK ��ư Ŭ���� ���� ���� �ݿ� // todo: NMHDR �� ���� ����ü�� ���� �˱�
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

//�ٸ��̸����� ���� Ŭ�� ��
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
	saveAsFileInfo.lpfnHook = DIALOG_FileSaveAs_Hook; // DIALOG_FileSaveAs_Hook �ݹ��Լ� ���

	if (GetSaveFileName(&saveAsFileInfo)) { // ���� ���̾�α� �ε� ���� // dialog Print
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

// ���� ����Ʈ
VOID DIALOG_FilePrint(VOID) 
{
	static const TCHAR times_new_roman[] = _T("Times New Roman");
	DOCINFO di;
	TEXTMETRIC tm;
	PRINTDLG printer;
	SIZE szMetric; // rectangle�� �ʺ�, ���� ����
	LOGFONT hdrFont; // ��Ʈ ���� ���� ����ü
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
	hdrFont.lfWeight = FW_BOLD; // �۲��� ����ġ�� ������
	hdrFont.lfCharSet = ANSI_CHARSET; // ���� ���� ����
	hdrFont.lfOutPrecision = OUT_DEFAULT_PRECIS; // ��� ���е� ����
	hdrFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	hdrFont.lfQuality = PROOF_QUALITY; // ��� ǰ�� ����
	hdrFont.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN; // �۲� �йи��� �����մϴ�
	_tcscpy(hdrFont.lfFaceName, times_new_roman); // ��ü �̸� ����

	font = CreateFontIndirect(&hdrFont); // LOGFONT ����ü�� �������� ��Ʈ ����

	// ���� ������ ������ // get current settings							
	ZeroMemory(&printer, sizeof(printer));
	printer.lStructSize = sizeof(printer); // ����ü ũ��
	printer.hwndOwner = Globals.hMainWnd; // ��ȭ���� ������ ������ �ڵ�
	printer.hInstance = Globals.hInstance; // ������ ��ȭ���� ���ø� ���� ���ҽ� ���� �ν��Ͻ� �ڵ�

	printer.Flags = PD_RETURNDC | PD_SELECTION; // ��ȭ���� �ʱ�ȭ�� ��� �÷��� // Set some default flags 

	// �ؽ�Ʈ ������ ���� ��ư ���� // Disable the selection radio button if there is no text selected
	if (GetSelectionTextLength(Globals.hEdit) == 0)
	{
		printer.Flags = printer.Flags | PD_NOSELECTION;
	}

	printer.nFromPage = 0;
	printer.nMinPage = 1;
	// FIXME : �ִ� ��� ��� ��� ����
	printer.nToPage = (WORD)-1;
	printer.nMaxPage = (WORD)-1;

	printer.nCopies = (WORD)PD_USEDEVMODECOPIES;

	printer.hDevMode = Globals.hDevMode; // devmode ����ü�� ������ ���� �޸� �ڵ�
	printer.hDevNames = Globals.hDevNames; // denames ����ü�� ������ ���� �޸� �ڵ�

	if (PrintDlg(&printer) == FALSE) // ����Ʈ ���̾�α׸� ���� // open print dialog
	{
		DeleteObject(font);
		return;
	}
	// ���̾�α׿��� ������ ��� �ش� ����ü �ֽ�ȭ // update structures
	Globals.hDevMode = printer.hDevMode;
	Globals.hDevNames = printer.hDevNames;

	assert(printer.hDC != 0); // ����̽� ���ؽ�Ʈ(������) �ڵ��� ���̸� ���� // if not error occur (debug mode only)

	di.cbSize = sizeof(DOCINFO); // ����ü ũ��
	di.lpszDocName = Globals.szFileTitle; // ���� �̸� ���� 
	di.lpszOutput = NULL; // ��� ���� �̸� ���� 
	di.lpszDatatype = NULL; // �μ� �۾��� ����ϴ� �� ���Ǵ� ������ ���� �����ϴ� null�� ������ ���ڿ��� ������
	di.fwType = 0; // �μ� �۾��� ���� �߰� ���� 

	if (StartDoc(printer.hDC, &di) <= 0) { // ����Ʈ �۾� ����
		DeleteObject(font);
		return;
	}

	if (printer.Flags & PD_SELECTION) { // ����â�� ���õ� �� ���븸 �������� // Get the file text 
		size = GetSelectionTextLength(Globals.hEdit) + 1; // todo : size => textLength
	}
	else { // ��ü �� ������ ��������
		size = GetWindowTextLength(Globals.hEdit) + 1;
	}

	pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(TCHAR)); // ���� �Ҵ� // todo : pTemp => pTextBuf
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

	// ���� �μ� ������ ������ // Get the current printing area 
	rcPrintRect = GetPrintingRect(printer.hDC, Globals.lMargins); // ������ ���� (RECT)
															  
	SetMapMode(printer.hDC, MM_TEXT); // �� �� ���� �ϳ��� �� �ȼ��� ��ġ�ǰ� �� // Ensure that each logical unit maps to one pixel 

	GetTextMetrics(printer.hDC, &tm); // �������� ��Ʈ�� ũ�⸦ �˾Ƴ� // Needed to get the correct height of a text line 

	border = 15;
	for (copycount = 1; copycount <= printer.nCopies; copycount++) { // �ݺ����� ���� ���� ������ ���
		i = 0; // todo: ��ġ ������ �ʿ� �ִ��� ����
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

			old_font = SelectObject(printer.hDC, font); // �μ⿡ ������ ��Ʈ ����

			if (dopage) {
				if (StartPage(printer.hDC) <= 0) { // ������ ������ �˸�
					SelectObject(printer.hDC, old_font);
					EndDoc(printer.hDC);
					DeleteDC(printer.hDC); // �޸𸮿� �Ҵ�ǰ� �޸𸮻󿡼� ����ϴ� DC�� �����ϱ� ���� �뵵
					HeapFree(GetProcessHeap(), 0, pTemp);
					DeleteObject(font); // �޸𸮿��� �ش� ������Ʈ ����
					AlertPrintError();
					return;
				}

				// ����Ʈ ������ ������ 2,3��° �μ��� ���� // set 2,3rd parameters as start point 
				SetViewportOrgEx(printer.hDC, rcPrintRect.left, rcPrintRect.top, NULL); 

				// Write a rectangle and header at the top of each page // �簢���� �׸�
				// �μ�: �ڵ� / ���� ��ǥ / ��� ��ǥ / ���� ��ǥ / �ϴ� ��ǥ
				Rectangle(printer.hDC, border, border, rcPrintRect.right - border, border + tm.tmHeight * 2);
				
				// todo : I don't know what's up with this TextOut command. This comes out kind of mangled.
				TextOut(printer.hDC,
					border * 2, // x ��ǥ 
					border + tm.tmHeight / 2, // y ��ǥ
					Globals.szFileTitle, // ��� ����(���� ����)
					lstrlen(Globals.szFileTitle)); // ���� ������ ����
			}

			// �Ӹ����� ������ ���� �ؽ�Ʈ �κ��� ��Ÿ�� // The starting point for the main text 
			xLeft = 0;
			yTop = border + tm.tmHeight * 4; 

			SelectObject(printer.hDC, old_font);

			// Since outputting strings is giving me problems, output the main text one character at a time. 
			do {
				if (pTemp[i] == '\n') { // �� �ѱ��
					xLeft = 0; // todo : ������ ���� ���� ����
					yTop += tm.tmHeight;
				}
				else if (pTemp[i] != '\r') { // \r �����ϱ�
					if (dopage)
						TextOut(printer.hDC, xLeft, yTop, &pTemp[i], 1);

					// We need to get the width for each individual char, since a proportional font may be used 
					GetTextExtentPoint32(printer.hDC, &pTemp[i], 1, &szMetric); // ���ڿ��� ũ�� ���� -> szMetric�� ����
					xLeft += szMetric.cx;

					// Insert a line break if the current line does not fit into the printing area 
					if (xLeft > rcPrintRect.right) { // ������ �ʰ� �� �����ٷ�
						xLeft = 0;
						yTop = yTop + tm.tmHeight;
					}
				}
			} while (i++ < size && yTop < rcPrintRect.bottom);

			if (dopage)
				EndPage(printer.hDC); // ������ ����
			pagecount++; 
		} while (i < size); // ��� ���� ����� ������ // until all the letters printed
	}

	if (old_font != 0)
		SelectObject(printer.hDC, old_font);
	EndDoc(printer.hDC); // ����Ʈ ���� 
	DeleteDC(printer.hDC);
	HeapFree(GetProcessHeap(), 0, pTemp);
	DeleteObject(font);
}

// ������ ���� // execute exit
VOID DIALOG_FileExit(VOID) 
{
	PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

// ������� // select undo
VOID DIALOG_EditUndo(VOID) 
{
	SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

// �߶󳻱� // execute cut
VOID DIALOG_EditCut(VOID) 
{
	SendMessage(Globals.hEdit, WM_CUT, 0, 0);
}

//�����ϱ� // execute copy
VOID DIALOG_EditCopy(VOID) 
{
	SendMessage(Globals.hEdit, WM_COPY, 0, 0);
}

//�ٿ��ֱ� // execute paste
VOID DIALOG_EditPaste(VOID) 
{
	SendMessage(Globals.hEdit, WM_PASTE, 0, 0);
}

//����� // execute delete
VOID DIALOG_EditDelete(VOID) 
{
	SendMessage(Globals.hEdit, WM_CLEAR, 0, 0);
}

// ��� ���� // execute "select all"
VOID DIALOG_EditSelectAll(VOID) 
{
	SendMessage(Globals.hEdit, EM_SETSEL, 0, (LPARAM)-1);
}

// ���� ��¥ �� �ð� ���� // insert current date & time
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
	SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText); //��¥�� �ð��� �ٿ� ����
}

VOID DoCreateStatusBar(VOID) // ���� �� �����
{
	RECT rc;
	RECT rcstatus;
	BOOL bStatusBarVisible;

	/* Check if status bar object already exists. */
	if (Globals.hStatusBar == NULL)
	{
		/* Try to create the status bar */
		Globals.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_EX_STATICEDGE, // ���¹� ������ �����
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
	DrawMenuBar(Globals.hMainWnd); // �޴� �� �׸���

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
	DIALOG_StatusBarUpdateCaretPos(); // ���¹� ��ġ�� �ֽ�ȭ
}

VOID DoCreateEditWindow(VOID) // ���� â �����
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
			GetWindowText(Globals.hEdit, pTemp, iSize + 1); // ������ �ִ� ���� ��� ���

			if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
				bModified = TRUE;
		}

		/* Restore original window procedure */
		SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);

		/* Destroy the edit control */
		DestroyWindow(Globals.hEdit); // ���� ���� ��Ʈ�� ����
	}

	/* Update wrap status into the main menu and recover style flags */
	if (Globals.bWrapLongLines) // �ڵ� �� �ٲ� ���� ��
	{
		dwStyle = EDIT_STYLE_WRAP;
		EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED); // ���� �� �޴� ��Ȱ��ȭ
	}
	else {
		dwStyle = EDIT_STYLE;
		EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_ENABLED);
	}

	/* Update previous changes */
	DrawMenuBar(Globals.hMainWnd); // �޴� �� �׸���

								   /* Create the new edit control */
	Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, // �� ���� ��Ʈ�� �Ҵ�
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
		SetWindowText(Globals.hEdit, pTemp); // �ӽ������� �ؽ�Ʈ ������ �ٽ� ���� ����â�� ����
		HeapFree(GetProcessHeap(), 0, pTemp);

		if (bModified)
			SendMessage(Globals.hEdit, EM_SETMODIFY, TRUE, 0);
	}

	/* Sub-class a new window callback for row/column detection. */
	Globals.EditProc = (WNDPROC)SetWindowLongPtr(Globals.hEdit,
		GWLP_WNDPROC,
		(LONG_PTR)EDIT_WndProc);


	DoCreateStatusBar(); // ���¹� �����

						 /* Finally shows new edit control and set focus into it. */
	ShowWindow(Globals.hEdit, SW_SHOW);
	SetFocus(Globals.hEdit);
}

VOID DIALOG_EditWrap(VOID) // �ڵ� �� �ٲ� ���� �� ����
{
	Globals.bWrapLongLines = !Globals.bWrapLongLines; // �ڵ� �� �ٲ� toggle 

	if (Globals.bWrapLongLines)
	{
		EnableMenuItem(Globals.hMenu, CMD_GOTO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED); // "�̵�" ��ư�� ��Ȱ��ȭ
	}
	else
	{
		EnableMenuItem(Globals.hMenu, CMD_GOTO, MF_BYCOMMAND | MF_ENABLED);
	}

	DoCreateEditWindow(); // ���� â�� �ٽ� ����(����â�� ����� ���ֱ� ���ؼ� ����â�� �ٽ� ����)
}

VOID DIALOG_SelectFont(VOID) // ��Ʈ �����ϱ�
{
	CHOOSEFONT cf;
	LOGFONT lf = Globals.lfFont;

	ZeroMemory(&cf, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = Globals.hMainWnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;

	if (ChooseFont(&cf)) // ��Ʈ ���� ���̾�α�
	{
		HFONT currfont = Globals.hFont;

		Globals.hFont = CreateFontIndirect(&lf);
		Globals.lfFont = lf;
		SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)TRUE); // ��Ʈ ����
		if (currfont != NULL)
			DeleteObject(currfont);
	}
}

typedef HWND(WINAPI *FINDPROC)(LPFINDREPLACE lpfr);

static VOID DIALOG_SearchDialog(FINDPROC pfnProc) // �˻� ���̾�α�
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

VOID DIALOG_Search(VOID) // �˻�
{
	DIALOG_SearchDialog(FindText);
}

VOID DIALOG_SearchNext(VOID) // ���� ã��
{
	if (Globals.find.lpstrFindWhat != NULL)
		NOTEPAD_FindNext(&Globals.find, FALSE, TRUE);
	else
		DIALOG_Search();
}

VOID DIALOG_Replace(VOID) // �ٲٱ�
{
	DIALOG_SearchDialog(ReplaceText);
}

static INT_PTR
CALLBACK
DIALOG_GoTo_DialogProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam) // "�̵�" ���̾�α� ó��(�ݹ��Լ�)
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
		if (HIWORD(wParam) == BN_CLICKED) // ���콺 ��ư Ŭ�� ��
		{
			if (LOWORD(wParam) == IDOK) // OK ��ư ���� ��
			{
				hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER); // �̵��� ���� �Է�â�� ���� �ڵ��� ����
				GetWindowText(hTextBox, szText, ARRAY_SIZE(szText));
				EndDialog(hwndDialog, _ttoi(szText)); // ���̾�α� ���� (szText���� ������ ��ȯ)
				bResult = TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL) // ��� ��ư ���� ��
			{
				EndDialog(hwndDialog, 0);
				bResult = TRUE;
			}
		}
		break;
	}

	return bResult;
}

VOID DIALOG_GoTo(VOID) // "�̵�" ���̾�α�
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
		DIALOG_GoTo_DialogProc, // �̵� ���̾�α�
		nLine);

	if (nLine >= 1)
	{
		for (i = 0; pszText[i] && (nLine > 1) && (i < nLength - 1); i++)
		{
			if (pszText[i] == '\n')
				nLine--;
		}
		SendMessage(Globals.hEdit, EM_SETSEL, i, i);
		SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0); // ��ũ���� ��ġ�� �°� �̵�
	}
	HeapFree(GetProcessHeap(), 0, pszText);
}

VOID DIALOG_StatusBarUpdateCaretPos(VOID) // ���¹� ��ġ �ֽ�ȭ
{
	int line, col;
	TCHAR buff[MAX_PATH];
	DWORD dwStart, dwSize;

	SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwSize);
	line = SendMessage(Globals.hEdit, EM_LINEFROMCHAR, (WPARAM)dwStart, 0);
	col = dwStart - SendMessage(Globals.hEdit, EM_LINEINDEX, (WPARAM)line, 0); // ���� Į�� ��

	_stprintf(buff, Globals.szStatusBarLineCol, line + 1, col + 1);
	SendMessage(Globals.hStatusBar, SB_SETTEXT, SB_SIMPLEID, (LPARAM)buff);
}

VOID DIALOG_ViewStatusBar(VOID) // ���¹� ���� �� �����
{
	Globals.bShowStatusBar = !Globals.bShowStatusBar;
	DoCreateStatusBar();
}

VOID DIALOG_HelpContents(VOID) // ����
{
	WinHelp(Globals.hMainWnd, helpfile, HELP_INDEX, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID) // �޸��� ����
{
	TCHAR szNotepad[MAX_STRING_LEN];
	HICON notepadIcon = LoadIcon(Globals.hInstance, MAKEINTRESOURCE(IDI_NPICON));

	LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));
	ShellAbout(Globals.hMainWnd, szNotepad, 0, notepadIcon);
	DeleteObject(notepadIcon);
}

INT_PTR
CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) // ������ ����(�۵� �ȵ�)
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

		SetWindowText(hLicenseEditWnd, strLicense); // ���̼��� ���� �Ҵ�

		return TRUE;

	case WM_COMMAND:

		if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
		{
			EndDialog(hDlg, LOWORD(wParam)); // ���̾�α� ����
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
VOID DIALOG_FilePageSetup(void) // ������ ����
{
	PAGESETUPDLG page; // ������ ������ ���� ����ü

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
	Globals.lMargins = page.rtMargin; // ���� ���� �Ҵ�
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*           DIALOG_PAGESETUP_Hook
*/

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) // ������ ���� ���̾�α� (�ݹ��Լ�)
{
	switch (msg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) // ���콺 ��ư�� Ŭ���Ǹ�
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				/* save user input and close dialog */
				GetDlgItemText(hDlg, 0x141, Globals.szHeader, ARRAY_SIZE(Globals.szHeader)); // Ǫ�Ϳ�, ����� ����
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
		SetDlgItemText(hDlg, 0x141, Globals.szHeader); // Ǫ�Ϳ� ����� �޾ƿ���
		SetDlgItemText(hDlg, 0x143, Globals.szFooter);
		break;
	}

	return FALSE;
}
