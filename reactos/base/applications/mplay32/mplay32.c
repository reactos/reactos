/*
 * PROJECT:         ReactOS Multimedia Player
 * FILE:            base\applications\mplay32\mplay32.c
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "mplay32.h"
#include <stdio.h>

#define MAIN_WINDOW_HEIGHT    125
#define MAIN_WINDOW_MIN_WIDTH 250

HINSTANCE hInstance = NULL;
HWND hTrackBar = NULL;
HWND hToolBar = NULL;
TCHAR szAppTitle[256] = _T("");
TCHAR szPrevFile[MAX_PATH] = _T("\0");
WORD wDeviceId;
BOOL bIsOpened = FALSE;
BOOL bIsPaused = FALSE;
UINT MaxFilePos = 0;

/* Known types table */
static const TYPEBYEXT ExtTypes[] =
{
    { _T(".wav"),  WAVE_FILE    },
    { _T(".wave"), WAVE_FILE    },
    { _T(".mid"),  MIDI_FILE    },
    { _T(".midi"), MIDI_FILE    },
    { _T(".cda"),  AUDIOCD_FILE },
    { _T(".avi"),  AVI_FILE     },
    { _T("\0"),        0        }
};

/* ToolBar Buttons */
static const TBBUTTON Buttons[] =
{   /* iBitmap,        idCommand,    fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_PLAY,      IDC_PLAY,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_STOP,      IDC_STOP,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_EJECT,     IDC_EJECT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,               0,            TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_BACKWARD,  IDC_BACKWARD, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_SEEKBACK,  IDC_SEEKBACK, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_SEEKFORW,  IDC_SEEKFORW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_FORWARD,   IDC_FORWARD,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0}
};

static VOID
SetImageList(HWND hwnd)
{
    HIMAGELIST hImageList;

    hImageList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR24, 1, 1);

    if (!hImageList)
    {
        MessageBox(hwnd, _T("ImageList it is not created!"), NULL, MB_OK);
        return;
    }

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_PLAYICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_STOPICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_EJECTICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_BACKWARDICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_SEEKBACKICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_SEEKFORWICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_FORWARDICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_Destroy((HIMAGELIST)SendMessage(hToolBar,
                                              TB_SETIMAGELIST,
                                              0,
                                              (LPARAM)hImageList));
}

static VOID
InitControls(HWND hwnd)
{
    INT NumButtons = sizeof(Buttons) / sizeof(Buttons[0]);

    InitCommonControls();

    /* Create trackbar */
    hTrackBar = CreateWindowEx(0,
                               TRACKBAR_CLASS,
                               NULL,
                               TBS_ENABLESELRANGE | WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
                               0,
                               0,
                               340,
                               20,
                               hwnd,
                               NULL,
                               hInstance,
                               NULL);
    if (!hTrackBar)
    {
        MessageBox(hwnd, _T("TrackBar it is not created!"), NULL, MB_OK);
        return;
    }

    /* Create toolbar */
    hToolBar = CreateWindowEx(0,
                              TOOLBARCLASSNAME,
                              NULL,
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS |
                              TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                              0,
                              40,
                              340,
                              30,
                              hwnd,
                              NULL,
                              hInstance,
                              NULL);
    if (!hToolBar)
    {
        MessageBox(hwnd, _T("ToolBar it is not created!"), NULL, MB_OK);
        return;
    }

    SetImageList(hwnd);
    SendMessage(hToolBar, TB_ADDBUTTONS, NumButtons, (LPARAM)Buttons);
}

static UINT
IsSupportedFileExtension(LPTSTR lpFileName)
{
    TCHAR szExt[MAX_PATH];
    INT DotPos = 0, i, j;

    for (i = _tcslen(lpFileName); i >= 0; --i)
    {
        if (lpFileName[i] == '.')
        {
            DotPos = _tcslen(lpFileName) - i;
            break;
        }
    }

    if (!DotPos) return UNSUPPORTED_FILE;

    szExt[DotPos + 1] = _T('\0');
    for (i = _tcslen(lpFileName), j = DotPos; j >= 0; --i, --j)
    {
        szExt[j] = lpFileName[i];
    }

    for (i = 0; ; i++)
    {
        if (ExtTypes[i].uType == UNSUPPORTED_FILE)
        {
            return UNSUPPORTED_FILE;
        }

        if (_tcscmp(ExtTypes[i].szExt, szExt) == 0)
        {
            return ExtTypes[i].uType;
        }
    }

    return UNSUPPORTED_FILE;
}

static DWORD
CloseMciDevice(VOID)
{
    MCI_GENERIC_PARMS mciGeneric;
    DWORD dwError;

    if (bIsOpened)
    {
        dwError = mciSendCommand(wDeviceId, MCI_CLOSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);
        if (dwError) return dwError;
        bIsOpened = FALSE;
    }

    return TRUE;
}

static DWORD
OpenMciDevice(HWND hwnd, LPTSTR lpType, LPTSTR lpFileName)
{
    MCI_STATUS_PARMS mciStatus;
    MCI_OPEN_PARMS mciOpen;
    TCHAR szNewTitle[MAX_PATH];
    DWORD dwError;
	WCHAR doom[260];

    if (bIsOpened)
    {
        CloseMciDevice();
    }

    mciOpen.lpstrDeviceType = lpType;
    mciOpen.lpstrElementName = lpFileName;
    mciOpen.dwCallback = 0;
    mciOpen.wDeviceID = 0;
    mciOpen.lpstrAlias = NULL;

    dwError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_WAIT, (DWORD_PTR)&mciOpen);
    if (dwError != 0)
    {
		mciGetErrorString(dwError,doom,260);
        MessageBox(0, doom, NULL, MB_OK);
		return dwError;
    }

    mciStatus.dwItem = MCI_STATUS_LENGTH;

    dwError = mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR)&mciStatus);
    if (dwError != 0)
    {
		mciGetErrorString(dwError,doom,260);
        MessageBox(0, doom, NULL, MB_OK);
        return dwError;
    }

    SendMessage(hTrackBar, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(1, mciStatus.dwReturn));
    SendMessage(hTrackBar, TBM_SETPAGESIZE, 0, 10);
    SendMessage(hTrackBar, TBM_SETLINESIZE, 0, 1);
    SendMessage(hTrackBar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) 1);

    if (mciStatus.dwReturn < 10000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM) 100, (LPARAM) 0);
    }
    else if (mciStatus.dwReturn < 100000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM) 1000, (LPARAM) 0);
    }
    else if (mciStatus.dwReturn < 1000000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM) 10000, (LPARAM) 0);
    }
    else
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM) 100000, (LPARAM) 0);
    }

    _stprintf(szNewTitle, _T("%s - %s"), szAppTitle, lpFileName);
    SetWindowText(hwnd, szNewTitle);

    MaxFilePos = mciStatus.dwReturn;
    wDeviceId = mciOpen.wDeviceID;
    bIsOpened = TRUE;
    _tcscpy(szPrevFile, lpFileName);
    return TRUE;
}

static VOID
StopPlayback(HWND hwnd)
{
    if (bIsOpened)
    {
        SendMessage(hTrackBar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) 1);
        KillTimer(hwnd, IDT_PLAYTIMER);
        CloseMciDevice();
    }
}

static VOID
SeekPlayback(HWND hwnd, DWORD dwNewPos)
{
    MCI_SEEK_PARMS mciSeek;
    MCI_PLAY_PARMS mciPlay;
    DWORD dwError;

    if (bIsOpened)
    {
        mciSeek.dwTo = (DWORD_PTR)dwNewPos;
        dwError = mciSendCommand(wDeviceId, MCI_SEEK, MCI_WAIT | MCI_TO, (DWORD_PTR)&mciSeek);
        if (dwError != 0)
        {
            MessageBox(hwnd, _T("SeekPlayback: Can't seek!"), NULL, MB_OK);
        }

        mciPlay.dwCallback = (DWORD_PTR)hwnd;
        dwError = mciSendCommand(wDeviceId, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciPlay);
        if (dwError != 0)
        {
            MessageBox(hwnd, _T("SeekPlayback: Can't play!"), NULL, MB_OK);
        }
    }
}

static VOID
SeekBackPlayback(HWND hwnd)
{
    MCI_STATUS_PARMS mciStatus;
    DWORD dwNewPos;

    if (!bIsOpened) return;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

    dwNewPos = mciStatus.dwReturn - 1;

    if((UINT)dwNewPos <= 1)
    {
        StopPlayback(hwnd);
    }
    else
    {
        SeekPlayback(hwnd, dwNewPos);
    }
}

static VOID
SeekForwPlayback(HWND hwnd)
{
    MCI_STATUS_PARMS mciStatus;
    DWORD dwNewPos;

    if (!bIsOpened) return;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

    dwNewPos = mciStatus.dwReturn + 1;

    if((UINT)dwNewPos >= MaxFilePos)
    {
        StopPlayback(hwnd);
    }
    else
    {
        SeekPlayback(hwnd, dwNewPos);
    }
}

static VOID
PausePlayback(HWND hwnd)
{
    MCI_GENERIC_PARMS mciGeneric;
    DWORD dwError;

    if (bIsOpened)
    {
        dwError = mciSendCommand(wDeviceId, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);
        if (dwError != 0)
        {
            MessageBox(hwnd, _T("Can't pause!"), NULL, MB_OK);
        }
        bIsPaused = TRUE;
    }
}

static VOID
ResumePlayback(HWND hwnd)
{
    MCI_GENERIC_PARMS mciGeneric;
    DWORD dwError;

    if (bIsPaused)
    {
        dwError = mciSendCommand(wDeviceId, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&mciGeneric);
        if (dwError != 0)
        {
            MessageBox(hwnd, _T("Can't resume!"), NULL, MB_OK);
        }
        bIsPaused = FALSE;
    }
}

VOID CALLBACK
PlayTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    MCI_STATUS_PARMS mciStatus;
    DWORD dwPos;

    if (!bIsOpened) KillTimer(hwnd, IDT_PLAYTIMER);

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);
    dwPos = mciStatus.dwReturn;

    if((UINT)dwPos >= MaxFilePos)
    {
        StopPlayback(hwnd);
    }
    else
    {
        SendMessage(hTrackBar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) dwPos);
    }
}

static VOID
PlayFile(HWND hwnd, LPTSTR lpFileName)
{
    MCI_PLAY_PARMS mciPlay;
    TCHAR szLocalFileName[MAX_PATH];
    UINT FileType;
    DWORD dwError;

    if (lpFileName == NULL)
    {
        if (szPrevFile[0] == _T('\0'))
            return;

        _tcscpy(szLocalFileName, szPrevFile);
    }
    else
    {
        _tcscpy(szLocalFileName, lpFileName);
    }

    if (GetFileAttributes(szLocalFileName) == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }

    FileType = IsSupportedFileExtension(szLocalFileName);

    switch (FileType)
    {
        case UNSUPPORTED_FILE:
            MessageBox(hwnd, _T("Unsupported format!"), NULL, MB_OK);
            return;
        case WAVE_FILE:
            OpenMciDevice(hwnd, _T("waveaudio"), szLocalFileName);
            break;
        case MIDI_FILE:
            OpenMciDevice(hwnd, _T("sequencer"), szLocalFileName);
            break;
        case AUDIOCD_FILE:
            OpenMciDevice(hwnd, _T("cdaudio"), szLocalFileName);
            break;
        case AVI_FILE:
            OpenMciDevice(hwnd, _T("avivideo"), szLocalFileName);
            break;
    }

    SetTimer(hwnd, IDT_PLAYTIMER, 100, (TIMERPROC) PlayTimerProc);

    dwError = mciSendCommand(wDeviceId, MCI_SEEK, MCI_WAIT | MCI_SEEK_TO_START, 0);

    mciPlay.dwCallback = (DWORD_PTR)hwnd;
    mciPlay.dwFrom = 0;
    mciPlay.dwTo = MaxFilePos;

    dwError = mciSendCommand(wDeviceId, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD_PTR)&mciPlay);
    if (dwError != 0)
    {
        MessageBox(hwnd, _T("Can't play!"), NULL, MB_OK);
    }
}

static VOID
OpenFileDialog(HWND hwnd)
{
    OPENFILENAME OpenFileName;
    TCHAR szFile[MAX_PATH + 1] = _T("\0");
    TCHAR szFilter[MAX_PATH], szCurrentDir[MAX_PATH];

    ZeroMemory(&OpenFileName, sizeof(OpenFileName));

    LoadString(hInstance, IDS_ALL_TYPES_FILTER, szFilter, sizeof(szFilter) / sizeof(TCHAR));

    if (!GetCurrentDirectory(sizeof(szCurrentDir) / sizeof(TCHAR), szCurrentDir))
    {
        _tcscpy(szCurrentDir, _T("c:\\"));
    }

    OpenFileName.lStructSize     = sizeof(OpenFileName);
    OpenFileName.hwndOwner       = hwnd;
    OpenFileName.hInstance       = hInstance;
    OpenFileName.lpstrFilter     = szFilter;
    OpenFileName.lpstrFile       = szFile;
    OpenFileName.nMaxFile        = sizeof(szFile) / sizeof((szFile)[0]);
    OpenFileName.lpstrInitialDir = szCurrentDir;
    OpenFileName.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE;
    OpenFileName.lpstrDefExt     = _T("\0");

    if (GetOpenFileName(&OpenFileName))
    {
        PlayFile(hwnd, OpenFileName.lpstrFile);
    }
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
            InitControls(hwnd);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
                    UINT idButton = (UINT)lpttt->hdr.idFrom;

                    switch (idButton)
                    {
                        case IDC_PLAY:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PLAY);
                        break;
                        case IDC_STOP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_STOP);
                        break;
                        case IDC_EJECT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EJECT);
                        break;
                        case IDC_BACKWARD:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_BACKWARD);
                        break;
                        case IDC_SEEKBACK:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SEEKBACK);
                        break;
                        case IDC_SEEKFORW:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SEEKFORW);
                        break;
                        case IDC_FORWARD:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_FORWARD);
                        break;
                    }
                    break;
                }
            }
        }
        break;

        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;

            if (pRect->right - pRect->left < MAIN_WINDOW_MIN_WIDTH)
                pRect->right = pRect->left + MAIN_WINDOW_MIN_WIDTH;

            if (pRect->bottom - pRect->top != MAIN_WINDOW_HEIGHT)
                pRect->bottom = pRect->top + MAIN_WINDOW_HEIGHT;

            return TRUE;
        }

        case WM_SIZE:
        {
            RECT Rect;
            UINT Size;

            if (hToolBar && hTrackBar)
            {
                SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
                SendMessage(hToolBar, TB_GETITEMRECT, 1, (LPARAM)&Rect);

                Size = GetSystemMetrics(SM_CYMENU) + Rect.bottom;
                MoveWindow(hTrackBar, 0, 0, LOWORD(lParam), HIWORD(lParam) - Size, TRUE);
            }
            return 0L;
        }

        case WM_HSCROLL:
        {
            if (hTrackBar == (HWND) lParam)
            {
                if (bIsOpened)
                {
                    DWORD dwNewPos = (DWORD) SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
                    SeekPlayback(hwnd, dwNewPos);
                }
                else
                {
                    SendMessage(hTrackBar, TBM_SETPOS, TRUE, 0);
                }
            }
        }
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_PLAY:
                    if (bIsOpened)
                    {
                        if (bIsPaused)
                            ResumePlayback(hwnd);
                        else
                            PausePlayback(hwnd);
                    }
                    else
                    {
                        if (szPrevFile[0] == _T('\0'))
                            OpenFileDialog(hwnd);
                        else
                            PlayFile(hwnd, NULL);
                    }
                    break;

                case IDC_STOP:
                    StopPlayback(hwnd);
                    break;

                case IDC_EJECT:
                    break;

                case IDC_BACKWARD:
                    break;

                case IDC_SEEKBACK:
                    SeekBackPlayback(hwnd);
                    break;

                case IDC_SEEKFORW:
                    SeekForwPlayback(hwnd);
                    break;

                case IDC_FORWARD:
                    break;

                case IDM_OPEN_FILE:
                    OpenFileDialog(hwnd);
                    return 0;

                case IDM_CLOSE_FILE:
                    StopPlayback(hwnd);
                    _tcscpy(szPrevFile, _T("\0"));
                    break;

                case IDM_ABOUT:
                    ShellAbout(hwnd, szAppTitle, 0, 0);
                    break;

                case IDM_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
            }
            break;

        case WM_DESTROY:
            StopPlayback(hwnd);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, Message, wParam, lParam);
}

INT WINAPI
_tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASSEX WndClass = {0};
    TCHAR szClassName[] = _T("ROSMPLAY32");
    HWND hwnd;
    MSG msg;

    hInstance = hInst;

    LoadString(hInstance, IDS_APPTITLE, szAppTitle, sizeof(szAppTitle) / sizeof(TCHAR));

    WndClass.cbSize            = sizeof(WNDCLASSEX);
    WndClass.lpszClassName     = szClassName;
    WndClass.lpfnWndProc       = MainWndProc;
    WndClass.hInstance         = hInstance;
    WndClass.style             = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon             = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    WndClass.hCursor           = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground     = (HBRUSH)(COLOR_BTNFACE + 1);
    WndClass.lpszMenuName      = MAKEINTRESOURCE(IDR_MAINMENU);

    RegisterClassEx(&WndClass);

    hwnd = CreateWindow(szClassName,
                        szAppTitle,
                        WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION | WS_CLIPCHILDREN,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        350,
                        MAIN_WINDOW_HEIGHT,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

    /* Show it */
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    PlayFile(hwnd, lpCmdLine);

    /* Message Loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
