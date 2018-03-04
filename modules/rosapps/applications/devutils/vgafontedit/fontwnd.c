/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implements the MDI child window for a font
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static const WCHAR szFontWndClass[] = L"VGAFontEditFontWndClass";

static BOOL
InitFont(IN PFONT_WND_INFO Info)
{
    Info->Font = (PBITMAP_FONT) HeapAlloc( hProcessHeap, 0, sizeof(BITMAP_FONT) );

    if(Info->OpenInfo->bCreateNew)
    {
        ZeroMemory( Info->Font, sizeof(BITMAP_FONT) );
        return TRUE;
    }
    else
    {
        // Load a font
        BOOL bRet = FALSE;
        DWORD dwTemp;
        HANDLE hFile;

        hFile = CreateFileW(Info->OpenInfo->pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

        if(hFile == INVALID_HANDLE_VALUE)
        {
            LocalizedError( IDS_OPENERROR, GetLastError() );
            return FALSE;
        }

        // Let's first check the file size to determine the file type
        dwTemp = GetFileSize(hFile, NULL);

        switch(dwTemp)
        {
            case 2048:
                // It should be a binary font file
                Info->OpenInfo->bBinaryFileOpened = TRUE;

                if( ReadFile(hFile, Info->Font, sizeof(BITMAP_FONT), &dwTemp, NULL) )
                    bRet = TRUE;
                else
                    LocalizedError( IDS_READERROR, GetLastError() );

                break;

            case 2052:
            {
                PSF1_HEADER Header;

                // Probably it's a PSFv1 file, check the header to make sure
                if( !ReadFile(hFile, &Header, sizeof(PSF1_HEADER) , &dwTemp, NULL) )
                {
                    LocalizedError( IDS_READERROR, GetLastError() );
                    break;
                }
                else
                {
                    if(Header.uMagic[0] == PSF1_MAGIC0 && Header.uMagic[1] == PSF1_MAGIC1)
                    {
                        // Yes, it is a PSFv1 file.
                        // Check the mode and character size. We only support 8x8 fonts with no special mode.
                        if(Header.uCharSize == 8 && Header.uMode == 0)
                        {
                            // Perfect! The file pointer is already set correctly, so we can just read the font bitmap now.
                            if( ReadFile(hFile, Info->Font, sizeof(BITMAP_FONT), &dwTemp, NULL) )
                                bRet = TRUE;
                            else
                                LocalizedError( IDS_READERROR, GetLastError() );
                        }
                        else
                            LocalizedError(IDS_UNSUPPORTEDPSF);

                        break;
                    }

                    // Fall through if the magic numbers aren't there
                }
            }

            default:
                LocalizedError(IDS_UNSUPPORTEDFORMAT);
        }

        CloseHandle(hFile);
        return bRet;
    }
}

static LRESULT CALLBACK
FontWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PFONT_WND_INFO Info;

    Info = (PFONT_WND_INFO) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if(Info || uMsg == WM_CREATE)
    {
        switch(uMsg)
        {
            case WM_CHILDACTIVATE:
                Info->MainWndInfo->CurrentFontWnd = Info;
                SetToolbarFileButtonState(Info->MainWndInfo, TRUE);
                SetPasteButtonState(Info->MainWndInfo);
                break;

            case WM_CREATE:
                Info = (PFONT_WND_INFO)( ( (LPMDICREATESTRUCT) ( (LPCREATESTRUCT)lParam )->lpCreateParams )->lParam );
                Info->hSelf = hwnd;

                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)Info);

                CreateFontBoxesWindow(Info);

                return 0;

            case WM_USER_APPCLOSE:
            case WM_CLOSE:
                // The user has to close all open edit dialogs first
                if(Info->FirstEditGlyphWnd)
                {
                    PWSTR pszMessage;

                    AllocAndLoadString(&pszMessage, IDS_CLOSEEDIT);
                    MessageBoxW(hwnd, pszMessage, szAppName, MB_OK | MB_ICONEXCLAMATION);
                    HeapFree(hProcessHeap, 0, pszMessage);

                    return 0;
                }

                // Prompt if the current file has been modified
                if(Info->OpenInfo->bModified)
                {
                    INT nMsgBoxResult;
                    PWSTR pszPrompt;
                    WCHAR szFile[MAX_PATH];

                    GetWindowTextW(hwnd, szFile, MAX_PATH);
                    LoadAndFormatString(IDS_SAVEPROMPT, &pszPrompt, szFile);

                    nMsgBoxResult = MessageBoxW(hwnd, pszPrompt, szAppName, MB_YESNOCANCEL | MB_ICONQUESTION);
                    LocalFree(pszPrompt);

                    switch(nMsgBoxResult)
                    {
                        case IDYES:
                            DoFileSave(Info->MainWndInfo, FALSE);
                            break;

                        case IDCANCEL:
                            // 0 = Stop the process of closing the windows (same value for both WM_CLOSE and WM_USER_APPCLOSE)
                            return 0;

                        // IDNO is handled automatically
                    }
                }

                // If there is another child, it will undo the following actions through its WM_CHILDACTIVATE handler.
                // Otherwise CurrentFontWnd will stay NULL, so the main window knows that no more childs are opened.
                Info->MainWndInfo->CurrentFontWnd = NULL;
                SetToolbarFileButtonState(Info->MainWndInfo, FALSE);
                SetPasteButtonState(Info->MainWndInfo);

                if(uMsg == WM_USER_APPCLOSE)
                {
                    // First do the tasks we would do for a normal WM_CLOSE message, then return the value for WM_USER_APPCLOSE
                    // Anything other than 0 indicates that the application shall continue closing the windows
                    DefMDIChildProcW(hwnd, WM_CLOSE, 0, 0);
                    return 1;
                }
                break;

            case WM_DESTROY:
                // Remove the window from the linked list
                if(Info->PrevFontWnd)
                    Info->PrevFontWnd->NextFontWnd = Info->NextFontWnd;
                else
                    Info->MainWndInfo->FirstFontWnd = Info->NextFontWnd;

                if(Info->NextFontWnd)
                    Info->NextFontWnd->PrevFontWnd = Info->PrevFontWnd;
                else
                    Info->MainWndInfo->LastFontWnd = Info->PrevFontWnd;

                // Free memory
                if(Info->Font)
                    HeapFree(hProcessHeap, 0, Info->Font);

                if(Info->OpenInfo->pszFileName)
                    HeapFree(hProcessHeap, 0, Info->OpenInfo->pszFileName);

                HeapFree(hProcessHeap, 0, Info->OpenInfo);
                HeapFree(hProcessHeap, 0, Info);

                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                return 0;

            case WM_SETFOCUS:
                // Set the keyboard focus to the FontBoxes window every time the Font window gets the focus
                SetFocus(Info->hFontBoxesWnd);
                break;

            case WM_SIZE:
            {
                INT nHeight = HIWORD(lParam);
                INT nWidth = LOWORD(lParam);
                POINT pt;
                RECT WndRect;

                // This ugly workaround is necessary for not setting either the Height or the Width of the window with SetWindowPos
                GetWindowRect(Info->hFontBoxesWnd, &WndRect);
                pt.x = WndRect.left;
                pt.y = WndRect.top;
                ScreenToClient(hwnd, &pt);

                if(nHeight < FONT_BOXES_WND_HEIGHT)
                {
                    SCROLLINFO si;

                    // Set the vertical scroll bar
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_RANGE | SIF_PAGE;
                    si.nMin = 0;
                    si.nMax = FONT_BOXES_WND_HEIGHT;
                    si.nPage = nHeight;
                    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                }
                else
                {
                    ShowScrollBar(hwnd, SB_VERT, FALSE);

                    // Store the new y coordinate in pt.y as well (needed for the SetWindowPos call for setting a new x coordinate)
                    pt.y = nHeight / 2 - FONT_BOXES_WND_HEIGHT / 2;
                    SetWindowPos(Info->hFontBoxesWnd,
                                 NULL,
                                 pt.x,
                                 pt.y,
                                 0,
                                 0,
                                 SWP_NOSIZE | SWP_NOZORDER);
                }

                if(nWidth < FONT_BOXES_WND_WIDTH)
                {
                    SCROLLINFO si;

                    // Set the horizontal scroll bar
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_RANGE | SIF_PAGE;
                    si.nMin = 0;
                    si.nMax = FONT_BOXES_WND_WIDTH;
                    si.nPage = nWidth;
                    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
                }
                else
                {
                    ShowScrollBar(hwnd, SB_HORZ, FALSE);

                    SetWindowPos(Info->hFontBoxesWnd,
                                 NULL,
                                 nWidth / 2 - FONT_BOXES_WND_WIDTH / 2,
                                 pt.y,
                                 0,
                                 0,
                                 SWP_NOSIZE | SWP_NOZORDER);
                }

                // We have to call DefMDIChildProcW here as well, otherwise we won't get the Minimize/Maximize/Close buttons for a maximized MDI child.
                break;
            }

            case WM_HSCROLL:
            case WM_VSCROLL:
            {
                INT nBar;
                INT nOrgPos;
                SCROLLINFO si;

                if(uMsg == WM_HSCROLL)
                    nBar = SB_HORZ;
                else
                    nBar = SB_VERT;

                si.cbSize = sizeof(si);
                si.fMask = SIF_ALL;
                GetScrollInfo(hwnd, nBar, &si);

                nOrgPos = si.nPos;

                switch( LOWORD(wParam) )
                {
                    // Constant is the same as SB_LEFT for WM_HSCROLL
                    case SB_TOP:
                        si.nPos = si.nMin;
                        break;

                    // Constant is the same as SB_RIGHT for WM_HSCROLL
                    case SB_BOTTOM:
                        si.nPos = si.nMax;
                        break;

                    // Constant is the same as SB_LINELEFT for WM_HSCROLL
                    case SB_LINEUP:
                        si.nPos -= 20;
                        break;

                    // Constant is the same as SB_LINERIGHT for WM_HSCROLL
                    case SB_LINEDOWN:
                        si.nPos += 20;
                        break;

                    // Constant is the same as SB_PAGELEFT for WM_HSCROLL
                    case SB_PAGEUP:
                        si.nPos -= si.nPage;
                        break;

                    // Constant is the same as SB_PAGERIGHT for WM_HSCROLL
                    case SB_PAGEDOWN:
                        si.nPos += si.nPage;
                        break;

                    case SB_THUMBTRACK:
                        si.nPos = si.nTrackPos;
                        break;
                }

                si.fMask = SIF_POS;
                SetScrollInfo(hwnd, nBar, &si, TRUE);
                GetScrollInfo(hwnd, nBar, &si);

                if(si.nPos != nOrgPos)
                {
                    // This ugly workaround is necessary for not setting the x coordinate
                    POINT pt;
                    RECT WndRect;

                    GetWindowRect(Info->hFontBoxesWnd, &WndRect);
                    pt.x = WndRect.left;
                    pt.y = WndRect.top;
                    ScreenToClient(hwnd, &pt);

                    if(uMsg == WM_HSCROLL)
                        SetWindowPos(Info->hFontBoxesWnd, NULL, -si.nPos, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                    else
                        SetWindowPos(Info->hFontBoxesWnd, NULL, pt.x, -si.nPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                }

                return 0;
            }
        }
    }

    return DefMDIChildProcW(hwnd, uMsg, wParam, lParam);
}

BOOL
CreateFontWindow(IN PMAIN_WND_INFO MainWndInfo, IN PFONT_OPEN_INFO OpenInfo)
{
    HWND hFontWnd;
    PFONT_WND_INFO Info;

    Info = (PFONT_WND_INFO) HeapAlloc( hProcessHeap, HEAP_ZERO_MEMORY, sizeof(FONT_WND_INFO) );

    if(Info)
    {
        Info->MainWndInfo = MainWndInfo;
        Info->OpenInfo = OpenInfo;

        if( InitFont(Info) )
        {
            PWSTR pszWindowTitle;

            if(OpenInfo->pszFileName)
                pszWindowTitle = wcsrchr(OpenInfo->pszFileName, '\\') + 1;
            else
                LoadAndFormatString(IDS_DOCNAME, &pszWindowTitle, ++MainWndInfo->uDocumentCounter);

            hFontWnd = CreateMDIWindowW( szFontWndClass,
                                         pszWindowTitle,
                                         0,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         FONT_WND_MIN_WIDTH,
                                         FONT_WND_MIN_HEIGHT,
                                         MainWndInfo->hMdiClient,
                                         hInstance,
                                         (LPARAM)Info );

            if(!OpenInfo->pszFileName)
                LocalFree(pszWindowTitle);

            if(hFontWnd)
            {
                // Add the new window to the linked list
                Info->PrevFontWnd = Info->MainWndInfo->LastFontWnd;

                if(Info->MainWndInfo->LastFontWnd)
                    Info->MainWndInfo->LastFontWnd->NextFontWnd = Info;
                else
                    Info->MainWndInfo->FirstFontWnd = Info;

                Info->MainWndInfo->LastFontWnd = Info;

                return TRUE;
            }
        }

        HeapFree(hProcessHeap, 0, Info);
    }

    return FALSE;
}

BOOL
InitFontWndClass(VOID)
{
    WNDCLASSW wc = {0,};

    wc.lpfnWndProc    = FontWndProc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = LoadIconW( hInstance, MAKEINTRESOURCEW(IDI_DOC) );
    wc.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
    wc.lpszClassName  = szFontWndClass;

    return RegisterClassW(&wc) != 0;
}

VOID
UnInitFontWndClass(VOID)
{
    UnregisterClassW(szFontWndClass, hInstance);
}
