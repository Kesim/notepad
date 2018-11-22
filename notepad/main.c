/*
 *  Notepad
 *
 *  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
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
 *
 */

#include "notepad.h"

#include <strsafe.h>

NOTEPAD_GLOBALS Globals;
static ATOM aFINDMSGSTRING;

VOID NOTEPAD_EnableSearchMenu()
{
    EnableMenuItem(Globals.hMenu, CMD_SEARCH,
                   MF_BYCOMMAND | ((GetWindowTextLength(Globals.hEdit) == 0) ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
    EnableMenuItem(Globals.hMenu, CMD_SEARCH_NEXT,
                   MF_BYCOMMAND | ((GetWindowTextLength(Globals.hEdit) == 0) ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
}

/***********************************************************************
 *
 *           SetFileName
 *
 *  Sets Global File Name.
 */
VOID SetFileName(LPCTSTR szFileName)
{
    StringCchCopy(Globals.szFileName, ARRAY_SIZE(Globals.szFileName), szFileName);
    Globals.szFileTitle[0] = 0;
    GetFileTitle(szFileName, Globals.szFileTitle, ARRAY_SIZE(Globals.szFileTitle));
}

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam) //단축키 입력 처리
{
    switch (wParam)
    {
    case CMD_NEW:        DIALOG_FileNew(); break;
    case CMD_OPEN:       DIALOG_FileOpen(); break;
    case CMD_SAVE:       DIALOG_FileSave(); break;
    case CMD_SAVE_AS:    DIALOG_FileSaveAs(); break;
    case CMD_PRINT:      DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP: DIALOG_FilePageSetup(); break;
    case CMD_EXIT:       DIALOG_FileExit(); break;

    case CMD_UNDO:       DIALOG_EditUndo(); break;
    case CMD_CUT:        DIALOG_EditCut(); break;
    case CMD_COPY:       DIALOG_EditCopy(); break;
    case CMD_PASTE:      DIALOG_EditPaste(); break;
    case CMD_DELETE:     DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL: DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:  DIALOG_EditTimeDate(); break;

    case CMD_SEARCH:      DIALOG_Search(); break;
    case CMD_SEARCH_NEXT: DIALOG_SearchNext(); break;
    case CMD_REPLACE:     DIALOG_Replace(); break;
    case CMD_GOTO:        DIALOG_GoTo(); break;

    case CMD_WRAP: DIALOG_EditWrap(); break;
    case CMD_FONT: DIALOG_SelectFont(); break;

    case CMD_STATUSBAR: DIALOG_ViewStatusBar(); break;

    case CMD_HELP_CONTENTS: DIALOG_HelpContents(); break;

    case CMD_ABOUT:
        DialogBox(GetModuleHandle(NULL),
                  MAKEINTRESOURCE(IDD_ABOUTBOX),
                  Globals.hMainWnd,
                  AboutDialogProc);
        break;

    case CMD_HELP_ABOUT_NOTEPAD: DIALOG_HelpAboutNotepad(); break;

    default:
        break;
    }
    return 0;
}

/***********************************************************************
 *
 *           NOTEPAD_FindTextAt
 */

static BOOL
NOTEPAD_FindTextAt(FINDREPLACE *pFindReplace, LPCTSTR pszText, int iTextLength, DWORD dwPosition)
{
    BOOL bMatches;
    size_t iTargetLength;

    if ((!pFindReplace) || (!pszText))
    {
        return FALSE;
    }

    iTargetLength = _tcslen(pFindReplace->lpstrFindWhat);

    /* Make proper comparison */
    if (pFindReplace->Flags & FR_MATCHCASE)
        bMatches = !_tcsncmp(&pszText[dwPosition], pFindReplace->lpstrFindWhat, iTargetLength);
    else
        bMatches = !_tcsnicmp(&pszText[dwPosition], pFindReplace->lpstrFindWhat, iTargetLength);

    if (bMatches && pFindReplace->Flags & FR_WHOLEWORD)
    {
        if ((dwPosition > 0) && !_istspace(pszText[dwPosition-1]))
            bMatches = FALSE;
        if ((dwPosition < (DWORD) iTextLength - 1) && !_istspace(pszText[dwPosition+1]))
            bMatches = FALSE;
    }

    return bMatches;
}

/***********************************************************************
 *
 *           NOTEPAD_FindNext
 */

BOOL NOTEPAD_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert)
{
    int iTextLength, iTargetLength;
    size_t iAdjustment = 0;
    LPTSTR pszText = NULL;
    DWORD dwPosition, dwBegin, dwEnd;
    BOOL bMatches = FALSE;
    TCHAR szResource[128], szText[128];
    BOOL bSuccess;

    iTargetLength = (int) _tcslen(pFindReplace->lpstrFindWhat);

    /* Retrieve the window text */
    iTextLength = GetWindowTextLength(Globals.hEdit);
    if (iTextLength > 0)
    {
        pszText = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, (iTextLength + 1) * sizeof(TCHAR));
        if (!pszText)
            return FALSE;

        GetWindowText(Globals.hEdit, pszText, iTextLength + 1);
    }

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwBegin, (LPARAM) &dwEnd);
    if (bReplace && ((dwEnd - dwBegin) == (DWORD) iTargetLength))
    {
        if (NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwBegin))
        {
            SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM) pFindReplace->lpstrReplaceWith);
            iAdjustment = _tcslen(pFindReplace->lpstrReplaceWith) - (dwEnd - dwBegin);
        }
    }

    if (pFindReplace->Flags & FR_DOWN)
    {
        /* Find Down */
        dwPosition = dwEnd;
        while(dwPosition < (DWORD) iTextLength)
        {
            bMatches = NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
            dwPosition++;
        }
    }
    else
    {
        /* Find Up */
        dwPosition = dwBegin;
        while(dwPosition > 0)
        {
            dwPosition--;
            bMatches = NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
        }
    }

    if (bMatches)
    {
        /* Found target */
        if (dwPosition > dwBegin)
            dwPosition += (DWORD) iAdjustment;
        SendMessage(Globals.hEdit, EM_SETSEL, dwPosition, dwPosition + iTargetLength);
        SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
        bSuccess = TRUE;
    }
    else
    {
        /* Can't find target */
        if (bShowAlert)
        {
            LoadString(Globals.hInstance, STRING_CANNOTFIND, szResource, ARRAY_SIZE(szResource));
            _sntprintf(szText, ARRAY_SIZE(szText), szResource, pFindReplace->lpstrFindWhat);
            LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, ARRAY_SIZE(szResource));
            MessageBox(Globals.hFindReplaceDlg, szText, szResource, MB_OK);
        }
        bSuccess = FALSE;
    }

    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
    return bSuccess;
}

/***********************************************************************
 *
 *           NOTEPAD_ReplaceAll
 */

static VOID NOTEPAD_ReplaceAll(FINDREPLACE *pFindReplace)
{
    BOOL bShowAlert = TRUE;

    SendMessage(Globals.hEdit, EM_SETSEL, 0, 0);

    while (NOTEPAD_FindNext(pFindReplace, TRUE, bShowAlert))
    {
        bShowAlert = FALSE;
    }
}

/***********************************************************************
 *
 *           NOTEPAD_FindTerm
 */

static VOID NOTEPAD_FindTerm(VOID)
{
    Globals.hFindReplaceDlg = NULL;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(VOID)
{
    LPTSTR p = Globals.szFilter;
    static const TCHAR txt_files[] = _T("*.txt");
    static const TCHAR all_files[] = _T("*.*");

    p += LoadString(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN) + 1;
    _tcscpy(p, txt_files);
    p += ARRAY_SIZE(txt_files);

    p += LoadString(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN) + 1;
    _tcscpy(p, all_files);
    p += ARRAY_SIZE(all_files);
    *p = '\0';
    Globals.find.lpstrFindWhat = NULL;

    Globals.hDevMode = NULL;
    Globals.hDevNames = NULL;
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menu, LPARAM index)
{
    int enable;

    UNREFERENCED_PARAMETER(index);

    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
        MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
    if (!Globals.bWrapLongLines)
    {
        CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_STATUSBAR,
            MF_BYCOMMAND | (Globals.bShowStatusBar ? MF_CHECKED : MF_UNCHECKED));
    }
    EnableMenuItem(menu, CMD_UNDO,
        SendMessage(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    enable = (int) SendMessage(Globals.hEdit, EM_GETSEL, 0, 0);
    enable = (HIWORD(enable) == LOWORD(enable)) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);

    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLength(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);
    DrawMenuBar(Globals.hMainWnd);
}

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{ 
    switch (msg)
    {
        case WM_KEYDOWN: //키를 누르거나 땠을 때
        case WM_KEYUP:
        {
            switch (wParam)
            {
                case VK_UP: //4방향키를 눌렀을 때
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                    DIALOG_StatusBarUpdateCaretPos();
                    break;
                default:
                {
                    UpdateWindowCaption(FALSE);
                    break;
                }
            }
        }
        case WM_LBUTTONUP: //마우스 좌클릭을 땟을 때
        {
            DIALOG_StatusBarUpdateCaretPos();
            break;
        }
    }
    return CallWindowProc( Globals.EditProc, hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */
static LRESULT
WINAPI
NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_CREATE:
        Globals.hMenu = GetMenu(hWnd);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == EN_HSCROLL || HIWORD(wParam) == EN_VSCROLL)
            DIALOG_StatusBarUpdateCaretPos();
        if ((HIWORD(wParam) == EN_CHANGE))
            NOTEPAD_EnableSearchMenu();
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_DESTROYCLIPBOARD:
        /*MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);*/
        break;

    case WM_CLOSE: //윈도우 창을 닫기 직전. Alt+F4 또는 닫기 버튼을 누르는 경우
        if (DoCloseFile()) {
            if (Globals.hFont)
                DeleteObject(Globals.hFont);
            if (Globals.hDevMode)
                GlobalFree(Globals.hDevMode);
            if (Globals.hDevNames)
                GlobalFree(Globals.hDevNames);
            DestroyWindow(hWnd);
        }
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY: //윈도우 창이 파괴됨
        SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);
        NOTEPAD_SaveSettingsToRegistry();
        PostQuitMessage(0);
        break;

    case WM_SIZE:
    {
        if (Globals.bShowStatusBar == TRUE && Globals.bWrapLongLines == FALSE)
        {
            RECT rcStatusBar;
            HDWP hdwp;

            if (!GetWindowRect(Globals.hStatusBar, &rcStatusBar))
                break;

            hdwp = BeginDeferWindowPos(2);
            if (hdwp == NULL)
                break;

            hdwp = DeferWindowPos(hdwp,
                                  Globals.hEdit,
                                  NULL,
                                  0,
                                  0,
                                  LOWORD(lParam),
                                  HIWORD(lParam) - (rcStatusBar.bottom - rcStatusBar.top),
                                  SWP_NOZORDER | SWP_NOMOVE);

            if (hdwp == NULL)
                break;

            hdwp = DeferWindowPos(hdwp,
                                  Globals.hStatusBar,
                                  NULL,
                                  0,
                                  0,
                                  LOWORD(lParam),
                                  LOWORD(wParam),
                                  SWP_NOZORDER);

            if (hdwp != NULL)
                EndDeferWindowPos(hdwp);
        }
        else
            SetWindowPos(Globals.hEdit,
                         NULL,
                         0,
                         0,
                         LOWORD(lParam),
                         HIWORD(lParam),
                         SWP_NOZORDER | SWP_NOMOVE);

        break;
    }

    /* The entire client area is covered by edit control and by
     * the status bar. So there is no need to erase main background.
     * This resolves the horrible flicker effect during windows resizes. */
    case WM_ERASEBKGND:
        return 1;

    case WM_SETFOCUS:
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        TCHAR szFileName[MAX_PATH];
        HDROP hDrop = (HDROP) wParam;

        DragQueryFile(hDrop, 0, szFileName, ARRAY_SIZE(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName);
        break;
    }
    case WM_CHAR:
    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;
    default:
        if (msg == aFINDMSGSTRING)
        {
            FINDREPLACE *pFindReplace = (FINDREPLACE *) lParam;
            Globals.find = *(FINDREPLACE *) lParam;

            if (pFindReplace->Flags & FR_FINDNEXT)
                NOTEPAD_FindNext(pFindReplace, FALSE, TRUE);
            else if (pFindReplace->Flags & FR_REPLACE)
                NOTEPAD_FindNext(pFindReplace, TRUE, TRUE);
            else if (pFindReplace->Flags & FR_REPLACEALL)
                NOTEPAD_ReplaceAll(pFindReplace);
            else if (pFindReplace->Flags & FR_DIALOGTERM)
                NOTEPAD_FindTerm();
            break;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static int AlertFileDoesNotExist(LPCTSTR szFileName)
{
    return DIALOG_StringMsgBox(Globals.hMainWnd, STRING_DOESNOTEXIST,
                               szFileName,
                               MB_ICONEXCLAMATION | MB_YESNO);
}

static BOOL HandleCommandLine(LPTSTR cmdline)
{
    BOOL opt_print = FALSE;

    while (*cmdline == _T(' ') || *cmdline == _T('-') || *cmdline == _T('/'))
    {
        TCHAR option;

        if (*cmdline++ == _T(' ')) continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == _T(' ')) cmdline++;

        switch(option)
        {
            case 'p':
            case 'P':
                opt_print = TRUE;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        LPCTSTR file_name = NULL;
        BOOL file_exists = FALSE;
        TCHAR buf[MAX_PATH];

        if (cmdline[0] == _T('"'))
        {
            cmdline++;
            cmdline[lstrlen(cmdline) - 1] = 0;
        }

        file_name = cmdline;
        if (FileExists(file_name))
        {
            file_exists = TRUE;
        }
        else if (!HasFileExtension(cmdline))
        {
            static const TCHAR txt[] = _T(".txt");

            /* try to find file with ".txt" extension */
            if (!_tcscmp(txt, cmdline + _tcslen(cmdline) - _tcslen(txt)))
            {
                file_exists = FALSE;
            }
            else
            {
                _tcsncpy(buf, cmdline, MAX_PATH - _tcslen(txt) - 1);
                _tcscat(buf, txt);
                file_name = buf;
                file_exists = FileExists(file_name);
            }
        }

        if (file_exists)
        {
            DoOpenFile(file_name);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
            {
                DIALOG_FilePrint();
                return FALSE;
            }
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name)) {
            case IDYES:
                DoOpenFile(file_name);
                break;

            case IDNO:
                break;
            }
        }
    }

    return TRUE;
}

/***********************************************************************
 *
 *           WinMain
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE prev, LPTSTR cmdline, int show)
{
    MSG msg;
    HACCEL hAccel;
    WNDCLASSEX wndclass;
    HMONITOR monitor;
    MONITORINFO info;
    INT x, y;

    static const TCHAR className[] = _T("Notepad"); //클래스 이름
    static const TCHAR winName[] = _T("Notepad"); //윈도우 창 이름

    switch (GetUserDefaultUILanguage()) //유저 기본 UI 언어를 가져옴
    {
		case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
			SetProcessDefaultLayout(LAYOUT_RTL);
			break;

		default:
			break;
    }

    UNREFERENCED_PARAMETER(prev);

    aFINDMSGSTRING = (ATOM)RegisterWindowMessage(FINDMSGSTRING);

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance = hInstance; //이 프로그램의 인스턴스 핸들
    NOTEPAD_LoadSettingsFromRegistry(); //레지스트리에서 설정값 가져옴 --> setting.c

    ZeroMemory(&wndclass, sizeof(wndclass));
    wndclass.cbSize = sizeof(wndclass); //윈도우 구조체 크기
    wndclass.lpfnWndProc = NOTEPAD_WndProc; //문제 발생시 함수 콜
    wndclass.hInstance = Globals.hInstance; //프로그램 번호
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NPICON)); //아이콘
    wndclass.hCursor = LoadCursor(0, IDC_ARROW); //커서 모양
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); //배경색
    wndclass.lpszMenuName = MAKEINTRESOURCE(MAIN_MENU); //메뉴 리소스
    wndclass.lpszClassName = className; //이름
    wndclass.hIconSm = (HICON)LoadImage(hInstance, //작은 아이콘
                                        MAKEINTRESOURCE(IDI_NPICON),
                                        IMAGE_ICON,
                                        16,
                                        16,
                                        0);

    if (!RegisterClassEx(&wndclass)) return FALSE; //윈도우에 등록이 안되있으면 false

    /* Setup windows */

    monitor = MonitorFromRect(&Globals.main_rect, MONITOR_DEFAULTTOPRIMARY);
    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);

    x = Globals.main_rect.left;
    y = Globals.main_rect.top;
    if (Globals.main_rect.left >= info.rcWork.right ||
        Globals.main_rect.top >= info.rcWork.bottom ||
        Globals.main_rect.right < info.rcWork.left ||
        Globals.main_rect.bottom < info.rcWork.top)
        x = y = CW_USEDEFAULT;

    Globals.hMainWnd = CreateWindow(className, //생성할 클래스
                                    winName, //윈도우 캡션(윈도우 창에 뜨는 이름)
                                    WS_OVERLAPPEDWINDOW, //기본값
                                    x, //윈도우 상 x,y 좌표
                                    y,
                                    Globals.main_rect.right - Globals.main_rect.left, //윈도우 사이즈 (width, height)
                                    Globals.main_rect.bottom - Globals.main_rect.top,
                                    NULL, //부모 윈도우
                                    NULL, //윈도우의 menu id값
                                    Globals.hInstance, //프로그램 번호와 윈도우 연결
                                    NULL); //특수 목적
    if (!Globals.hMainWnd) //생성 에러일 경우 에러 리턴
    {
        ShowLastError();
        ExitProcess(1);
    }

    DoCreateEditWindow();

    NOTEPAD_InitData();
    DIALOG_FileNew();

    ShowWindow(Globals.hMainWnd, show); //윈도우를 보여줌
    UpdateWindow(Globals.hMainWnd);
    DragAcceptFiles(Globals.hMainWnd, TRUE); //파일 드래그 허용

    DIALOG_ViewStatusBar();

    if (!HandleCommandLine(cmdline))
    {
        return 0;
    }

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCEL));

    while (GetMessage(&msg, 0, 0, 0))
    {
        if (!IsDialogMessage(Globals.hFindReplaceDlg, &msg) &&
            !TranslateAccelerator(Globals.hMainWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}
