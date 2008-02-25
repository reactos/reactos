/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/mainwnd.c
 * PURPOSE:     Implements the main window of the application
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "precomp.h"

static const WCHAR szMainWndClass[] = L"VGAFontEditMainWndClass";

static VOID
InitResources(IN PMAIN_WND_INFO Info)
{
    HDC hMemDC;
    HDC hMainDC;
    HPEN hPen;
    RECT rect;

    hMemDC = CreateCompatibleDC(NULL);
    hMainDC = GetDC(Info->hMainWnd);

    // Create the "Box" bitmap
    Info->hBoxBmp = CreateCompatibleBitmap(hMainDC, CHARACTER_BOX_WIDTH, CHARACTER_BOX_HEIGHT);
    SelectObject(hMemDC, Info->hBoxBmp);

    rect.left = 0;
    rect.top = 0;
    rect.right = CHARACTER_INFO_BOX_WIDTH;
    rect.bottom = CHARACTER_INFO_BOX_HEIGHT;
    FillRect( hMemDC, &rect, (HBRUSH)(COLOR_BTNFACE + 1) );

    SelectObject( hMemDC, GetStockObject(WHITE_PEN) );
    Rectangle(hMemDC, 0, 0, CHARACTER_INFO_BOX_WIDTH - 1, 2);
    Rectangle(hMemDC, 0, 2, 2, CHARACTER_INFO_BOX_HEIGHT - 1);

    hPen = CreatePen( PS_SOLID, 1, RGB(128, 128, 128) );
    SelectObject(hMemDC, hPen);
    Rectangle(hMemDC, 1, CHARACTER_INFO_BOX_HEIGHT - 2, CHARACTER_INFO_BOX_WIDTH, CHARACTER_INFO_BOX_HEIGHT);
    Rectangle(hMemDC, CHARACTER_INFO_BOX_WIDTH - 2, 1, CHARACTER_INFO_BOX_WIDTH, CHARACTER_INFO_BOX_HEIGHT - 2);

    SetPixel( hMemDC, CHARACTER_INFO_BOX_WIDTH - 1, 0, RGB(128, 128, 128) );
    SetPixel( hMemDC, 0, CHARACTER_INFO_BOX_HEIGHT - 1, RGB(128, 128, 128) );

    DeleteObject(hPen);
    DeleteDC(hMemDC);
}

static VOID
UnInitResources(IN PMAIN_WND_INFO Info)
{
    DeleteObject(Info->hBoxBmp);
}

static VOID
AddToolbarButton(IN PMAIN_WND_INFO Info, IN INT iBitmap, IN INT idCommand, IN UINT uID)
{
    PWSTR pszTooltip;
    TBBUTTON tbb = {0,};

    if( AllocAndLoadString(&pszTooltip, uID) )
    {
        tbb.fsState = TBSTATE_ENABLED;
        tbb.iBitmap = iBitmap;
        tbb.idCommand = idCommand;
        tbb.iString = (INT_PTR)pszTooltip;

        SendMessageW( Info->hToolbar, TB_ADDBUTTONSW, 1, (LPARAM)&tbb );
        HeapFree(hProcessHeap, 0, pszTooltip);
    }
}

static VOID
SetToolbarButtonState(IN PMAIN_WND_INFO Info, INT idCommand, BOOL bEnabled)
{
    TBBUTTONINFOW tbbi = {0,};

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE;
    tbbi.fsState = (bEnabled ? TBSTATE_ENABLED : 0);

    SendMessageW(Info->hToolbar, TB_SETBUTTONINFOW, idCommand, (LPARAM)&tbbi);
}

VOID
SetToolbarFileButtonState(IN PMAIN_WND_INFO Info, BOOL bEnabled)
{
    SetToolbarButtonState(Info, ID_FILE_SAVE, bEnabled);
    SetToolbarButtonState(Info, ID_EDIT_GLYPH, bEnabled);
    SetToolbarButtonState(Info, ID_EDIT_COPY, bEnabled);
}

static VOID
AddToolbarSeparator(IN PMAIN_WND_INFO Info)
{
    TBBUTTON tbb = {0,};

    tbb.fsStyle = BTNS_SEP;

    SendMessageW( Info->hToolbar, TB_ADDBUTTONSW, 1, (LPARAM)&tbb );
}

static VOID
InitMainWnd(IN PMAIN_WND_INFO Info)
{
    CLIENTCREATESTRUCT ccs;
    INT iCustomBitmaps;
    INT iStandardBitmaps;
    TBADDBITMAP tbab;

    // Add the toolbar
    Info->hToolbar = CreateWindowExW(0,
                                     TOOLBARCLASSNAMEW,
                                     NULL,
                                     WS_VISIBLE | WS_CHILD | TBSTYLE_TOOLTIPS,
                                     0,
                                     0,
                                     0,
                                     0,
                                     Info->hMainWnd,
                                     NULL,
                                     hInstance,
                                     NULL);

    // Identify the used Common Controls version
    SendMessageW(Info->hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Enable Tooltips
    SendMessageW(Info->hToolbar, TB_SETMAXTEXTROWS, 0, 0);

    // Add the toolbar bitmaps
    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = IDB_STD_SMALL_COLOR;
    iStandardBitmaps = SendMessageW(Info->hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

    tbab.hInst = hInstance;
    tbab.nID = IDB_MAIN_TOOLBAR;
    iCustomBitmaps = SendMessageW(Info->hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

    // Add the toolbar buttons
    AddToolbarButton(Info, iStandardBitmaps + STD_FILENEW, ID_FILE_NEW, IDS_TOOLTIP_NEW);
    AddToolbarButton(Info, iStandardBitmaps + STD_FILEOPEN, ID_FILE_OPEN, IDS_TOOLTIP_OPEN);
    AddToolbarButton(Info, iStandardBitmaps + STD_FILESAVE, ID_FILE_SAVE, IDS_TOOLTIP_SAVE);
    AddToolbarSeparator(Info);
    AddToolbarButton(Info, iCustomBitmaps + TOOLBAR_EDIT_GLYPH, ID_EDIT_GLYPH, IDS_TOOLTIP_EDIT_GLYPH);
    AddToolbarSeparator(Info);
    AddToolbarButton(Info, iStandardBitmaps + STD_COPY, ID_EDIT_COPY, IDS_TOOLTIP_COPY);
    AddToolbarButton(Info, iStandardBitmaps + STD_PASTE, ID_EDIT_PASTE, IDS_TOOLTIP_PASTE);

    SetToolbarFileButtonState(Info, FALSE);
    SetPasteButtonState(Info);

    // Add the MDI client area
    ccs.hWindowMenu = GetSubMenu(Info->hMenu, 2);
    ccs.idFirstChild = ID_MDI_FIRSTCHILD;

    Info->hMdiClient = CreateWindowExW(WS_EX_CLIENTEDGE,
                                       L"MDICLIENT",
                                       NULL,
                                       WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL,
                                       0,
                                       0,
                                       0,
                                       0,
                                       Info->hMainWnd,
                                       NULL,
                                       hInstance,
                                       &ccs);

    // Initialize the file handling
    FileInitialize(Info->hMainWnd);
}

static VOID
InitMenuPopup(IN PMAIN_WND_INFO Info)
{
    UINT uState;

    uState = MF_BYCOMMAND | !(Info->CurrentFontWnd);

    EnableMenuItem(Info->hMenu, ID_FILE_CLOSE, uState);
    EnableMenuItem(Info->hMenu, ID_FILE_SAVE, uState);
    EnableMenuItem(Info->hMenu, ID_FILE_SAVE_AS, uState);

    EnableMenuItem(Info->hMenu, ID_EDIT_COPY, uState);
    EnableMenuItem(Info->hMenu, ID_EDIT_GLYPH, uState);

    uState = MF_BYCOMMAND | !(Info->CurrentFontWnd && IsClipboardFormatAvailable(uCharacterClipboardFormat));
    EnableMenuItem(Info->hMenu, ID_EDIT_PASTE, uState);
}

static VOID
DoFileNew(IN PMAIN_WND_INFO Info)
{
    PFONT_OPEN_INFO OpenInfo;

    OpenInfo = (PFONT_OPEN_INFO) HeapAlloc( hProcessHeap, HEAP_ZERO_MEMORY, sizeof(FONT_OPEN_INFO) );
    OpenInfo->bCreateNew = TRUE;

    CreateFontWindow(Info, OpenInfo);
}

static VOID
DoFileOpen(IN PMAIN_WND_INFO Info)
{
    PFONT_OPEN_INFO OpenInfo;

    OpenInfo = (PFONT_OPEN_INFO) HeapAlloc( hProcessHeap, HEAP_ZERO_MEMORY, sizeof(FONT_OPEN_INFO) );
    OpenInfo->pszFileName = HeapAlloc(hProcessHeap, 0, MAX_PATH);
    OpenInfo->pszFileName[0] = 0;

    if( DoOpenFile(OpenInfo->pszFileName) )
    {
        OpenInfo->bCreateNew = FALSE;
        CreateFontWindow(Info, OpenInfo);
    }
}

VOID
DoFileSave(IN PMAIN_WND_INFO Info, IN BOOL bSaveAs)
{
    DWORD dwBytesWritten;
    HANDLE hFile;

    // Show the "Save" dialog
    //   - if "Save As" was clicked
    //   - if the file was not yet saved
    //   - if another format than the binary format was opened
    if(bSaveAs || !Info->CurrentFontWnd->OpenInfo->bBinaryFileOpened)
    {
        if(!Info->CurrentFontWnd->OpenInfo->pszFileName)
        {
            Info->CurrentFontWnd->OpenInfo->pszFileName = (PWSTR) HeapAlloc(hProcessHeap, 0, MAX_PATH);
            Info->CurrentFontWnd->OpenInfo->pszFileName[0] = 0;
        }
        else if(!Info->CurrentFontWnd->OpenInfo->bBinaryFileOpened)
        {
            // For a file in another format, the user has to enter a new file name as well
            Info->CurrentFontWnd->OpenInfo->pszFileName[0] = 0;
        }

        if( !DoSaveFile(Info->CurrentFontWnd->OpenInfo->pszFileName) )
            return;
    }

    // Save the binary font
    hFile = CreateFileW(Info->CurrentFontWnd->OpenInfo->pszFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        LocalizedError( IDS_OPENERROR, GetLastError() );
        return;
    }

    if( !WriteFile(hFile, Info->CurrentFontWnd->Font, sizeof(BITMAP_FONT), &dwBytesWritten, NULL) )
        LocalizedError( IDS_WRITEERROR, GetLastError() );

    CloseHandle(hFile);
}

static VOID
CopyCurrentGlyph(IN PFONT_WND_INFO FontWndInfo)
{
    HGLOBAL hMem;
    PUCHAR pCharacterBits;

    if(!OpenClipboard(NULL))
        return;

    EmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, 8);
    pCharacterBits = GlobalLock(hMem);
    RtlCopyMemory(pCharacterBits, FontWndInfo->Font->Bits + FontWndInfo->uSelectedCharacter * 8, 8);
    GlobalUnlock(hMem);

    SetClipboardData(uCharacterClipboardFormat, hMem);

    CloseClipboard();
}

static VOID
PasteIntoCurrentGlyph(IN PFONT_WND_INFO FontWndInfo)
{
    HGLOBAL hMem;

    if(!IsClipboardFormatAvailable(uCharacterClipboardFormat))
        return;

    if(!OpenClipboard(NULL))
        return;

    hMem = GetClipboardData(uCharacterClipboardFormat);
    if(hMem)
    {
        PUCHAR pCharacterBits;

        pCharacterBits = GlobalLock(hMem);
        if(pCharacterBits)
        {
            RECT CharacterRect;
            UINT uFontRow;
            UINT uFontColumn;

            RtlCopyMemory(FontWndInfo->Font->Bits + FontWndInfo->uSelectedCharacter * 8, pCharacterBits, 8);
            GlobalUnlock(hMem);

            FontWndInfo->OpenInfo->bModified = TRUE;

            GetCharacterPosition(FontWndInfo->uSelectedCharacter, &uFontRow, &uFontColumn);
            GetCharacterRect(uFontRow, uFontColumn, &CharacterRect);
            InvalidateRect(FontWndInfo->hFontBoxesWnd, &CharacterRect, FALSE);
        }
    }

    CloseClipboard();
}

VOID
SetPasteButtonState(IN PMAIN_WND_INFO Info)
{
    SetToolbarButtonState(Info,
                          ID_EDIT_PASTE,
                          (Info->CurrentFontWnd && IsClipboardFormatAvailable(uCharacterClipboardFormat)));
}

static BOOL
MenuCommand(IN INT nMenuItemID, IN PMAIN_WND_INFO Info)
{
    switch(nMenuItemID)
    {
        // File Menu
        case ID_FILE_NEW:
            DoFileNew(Info);
            return TRUE;

        case ID_FILE_OPEN:
            DoFileOpen(Info);
            return TRUE;

        case ID_FILE_CLOSE:
            SendMessageW(Info->CurrentFontWnd->hSelf, WM_CLOSE, 0, 0);
            return TRUE;

        case ID_FILE_SAVE:
            DoFileSave(Info, FALSE);
            return TRUE;

        case ID_FILE_SAVE_AS:
            DoFileSave(Info, TRUE);
            return TRUE;

        case ID_FILE_EXIT:
            PostMessage(Info->hMainWnd, WM_CLOSE, 0, 0);
            return TRUE;

        // Edit Menu
        case ID_EDIT_GLYPH:
            EditCurrentGlyph(Info->CurrentFontWnd);
            return TRUE;

        case ID_EDIT_COPY:
            CopyCurrentGlyph(Info->CurrentFontWnd);
            return TRUE;

        case ID_EDIT_PASTE:
            PasteIntoCurrentGlyph(Info->CurrentFontWnd);
            return TRUE;

        // Window Menu
        case ID_WINDOW_TILE_HORZ:
            SendMessageW(Info->hMdiClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
            return TRUE;

        case ID_WINDOW_TILE_VERT:
            SendMessageW(Info->hMdiClient, WM_MDITILE, MDITILE_VERTICAL, 0);
            return TRUE;

        case ID_WINDOW_CASCADE:
            SendMessageW(Info->hMdiClient, WM_MDICASCADE, 0, 0);
            return TRUE;

        case ID_WINDOW_ARRANGE:
            SendMessageW(Info->hMdiClient, WM_MDIICONARRANGE, 0, 0);
            return TRUE;

        case ID_WINDOW_NEXT:
            SendMessageW(Info->hMdiClient, WM_MDINEXT, 0, 0);
            return TRUE;

        // Help Menu
        case ID_HELP_ABOUT:
            DialogBoxW( hInstance, MAKEINTRESOURCEW(IDD_ABOUT), Info->hMainWnd, AboutDlgProc );
            return TRUE;
    }

    return FALSE;
}

static VOID
MainWndSize(PMAIN_WND_INFO Info, INT cx, INT cy)
{
    HDWP dwp;
    INT iMdiTop;
    RECT ToolbarRect;

    iMdiTop = 0;

    dwp = BeginDeferWindowPos(2);
    if(!dwp)
        return;

    if(Info->hToolbar)
    {
        GetWindowRect(Info->hToolbar, &ToolbarRect);
        iMdiTop += ToolbarRect.bottom - ToolbarRect.top;

        dwp = DeferWindowPos(dwp, Info->hToolbar, NULL, 0, 0, cx, ToolbarRect.bottom - ToolbarRect.top, SWP_NOZORDER);
        if(!dwp)
            return;
    }

    if(Info->hMdiClient)
    {
        dwp = DeferWindowPos(dwp, Info->hMdiClient, NULL, 0, iMdiTop, cx, cy - iMdiTop, SWP_NOZORDER);
        if(!dwp)
            return;
    }

    EndDeferWindowPos(dwp);
}

static LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hNextClipboardViewer;

    PMAIN_WND_INFO Info;

    Info = (PMAIN_WND_INFO) GetWindowLongW(hwnd, GWLP_USERDATA);

    if(Info || uMsg == WM_CREATE)
    {
        switch(uMsg)
        {
            case WM_COMMAND:
                if( MenuCommand( LOWORD(wParam), Info ) )
                    return 0;

                break;

            case WM_CHANGECBCHAIN:
                if((HWND)wParam == hNextClipboardViewer)
                    hNextClipboardViewer = (HWND)lParam;
                else
                    SendMessage(hNextClipboardViewer, uMsg, wParam, lParam);

                return 0;

            case WM_CLOSE:
                if(Info->FirstFontWnd)
                {
                    // Send WM_CLOSE to all subwindows, so they can prompt for saving unsaved files
                    PFONT_WND_INFO pNextWnd;
                    PFONT_WND_INFO pWnd;

                    pWnd = Info->FirstFontWnd;

                    do
                    {
                        // The pWnd structure might already be destroyed after the WM_CLOSE, so we have to preserve the address of the next window here
                        pNextWnd = pWnd->NextFontWnd;

                        // Send WM_USER_APPCLOSE, so we can check for a custom return value
                        // In this case, we check if the user clicked the "Cancel" button in one of the prompts and if so, we don't close the app
                        if( !SendMessage(pWnd->hSelf, WM_USER_APPCLOSE, 0, 0) )
                            return 0;
                    }
                    while( (pWnd = pNextWnd) );
                }
                break;

            case WM_CREATE:
                Info = (PMAIN_WND_INFO)( ( (LPCREATESTRUCT)lParam )->lpCreateParams );
                Info->hMainWnd = hwnd;
                Info->hMenu = GetMenu(hwnd);
                SetWindowLongW(hwnd, GWLP_USERDATA, (LONG)Info);

                hNextClipboardViewer = SetClipboardViewer(hwnd);

                InitMainWnd(Info);
                InitResources(Info);

                ShowWindow(hwnd, Info->nCmdShow);
                return 0;

            case WM_DESTROY:
                UnInitResources(Info);

                HeapFree(hProcessHeap, 0, Info);
                SetWindowLongW(hwnd, GWLP_USERDATA, 0);
                PostQuitMessage(0);
                return 0;

            case WM_DRAWCLIPBOARD:
                SetPasteButtonState(Info);

                // Pass the message to the next clipboard window in the chain
                SendMessage(hNextClipboardViewer, uMsg, wParam, lParam);
                return 0;

            case WM_INITMENUPOPUP:
                InitMenuPopup(Info);
                break;

            case WM_SIZE:
                MainWndSize( Info, LOWORD(lParam), HIWORD(lParam) );
                return 0;
        }
    }

    if(Info && Info->hMdiClient)
        return DefFrameProcW(hwnd, Info->hMdiClient, uMsg, wParam, lParam);
    else
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL
CreateMainWindow(IN INT nCmdShow, OUT PMAIN_WND_INFO* Info)
{
    HWND hMainWnd;

    *Info = (PMAIN_WND_INFO) HeapAlloc( hProcessHeap, HEAP_ZERO_MEMORY, sizeof(MAIN_WND_INFO) );

    if(*Info)
    {
        (*Info)->nCmdShow = nCmdShow;

        hMainWnd = CreateWindowExW(0,
                                   szMainWndClass,
                                   szAppName,
                                   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   NULL,
                                   LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_MAINMENU)),
                                   hInstance,
                                   *Info);

        if(hMainWnd)
            return TRUE;
        else
            HeapFree(hProcessHeap, 0, *Info);
    }

    return FALSE;
}

BOOL
InitMainWndClass(VOID)
{
    WNDCLASSW wc = {0,};

    wc.lpfnWndProc    = MainWndProc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = LoadIconW( hInstance, MAKEINTRESOURCEW(IDI_MAIN) );
    wc.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
    wc.lpszClassName  = szMainWndClass;

    return RegisterClassW(&wc) != 0;
}

VOID
UnInitMainWndClass(VOID)
{
    UnregisterClassW(szMainWndClass, hInstance);
}
