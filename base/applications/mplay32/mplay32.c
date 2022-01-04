/*
 * PROJECT:         ReactOS Multimedia Player
 * FILE:            base/applications/mplay32/mplay32.c
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "mplay32.h"

#define IDT_PLAYTIMER 1000

#define MAIN_WINDOW_HEIGHT    125
#define MAIN_WINDOW_MIN_WIDTH 250
#define MAX_MCISTR            256

#ifdef UNICODE
#define argv __wargv
#else
#define argv __argv
#endif

HINSTANCE hInstance = NULL;
HWND hTrackBar = NULL;
HWND hToolBar = NULL;
HWND hTimeDisplay = NULL;
HMENU hMainMenu = NULL;

TCHAR szAppTitle[256] = _T("");
TCHAR szDefaultFilter[MAX_PATH] = _T("");
TCHAR szCurrentFile[MAX_PATH] = _T("");
LPTSTR szFilter = NULL;

WORD wDeviceId = 0;
BOOL bRepeat = FALSE;
BOOL bIsSingleWindow = FALSE;
UINT MaxFilePos = 0;
RECT PrevWindowPos;

static DWORD GetDeviceMode(HWND hwnd);

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
    {TBICON_FORWARD,   IDC_FORWARD,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
//  {TBICON_PAUSE,     IDC_PAUSE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0}
};

void EnableMenuItems(HWND hwnd)
{
    MCIERROR mciError;
    MCI_GENERIC_PARMS mciGeneric;
    MCI_DGV_RECT_PARMS mciVideoRect;
    MCI_DGV_WINDOW_PARMSW mciVideoWindow;

    EnableMenuItem(hMainMenu, IDM_CLOSE_FILE, MF_BYCOMMAND | MF_ENABLED);

    mciError = mciSendCommand(wDeviceId, MCI_CONFIGURE, MCI_TEST, (DWORD_PTR)&mciGeneric);
    if (mciError == 0)
    {
        EnableMenuItem(hMainMenu, IDM_DEVPROPS, MF_BYCOMMAND | MF_ENABLED);
    }

    mciVideoWindow.hWnd = hwnd;

    mciError = mciSendCommand(wDeviceId, MCI_WINDOW, MCI_DGV_WINDOW_HWND | MCI_TEST, (DWORD_PTR)&mciVideoWindow);
    if (!mciError)
    {
        mciError = mciSendCommand(wDeviceId, MCI_WHERE, MCI_DGV_WHERE_SOURCE | MCI_TEST, (DWORD_PTR)&mciVideoRect);
        if (!mciError)
        {
            EnableMenuItem(hMainMenu, IDM_SWITCHVIEW, MF_BYCOMMAND | MF_ENABLED);
        }
    }
}

void DisableMenuItems(void)
{
    EnableMenuItem(hMainMenu, IDM_CLOSE_FILE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu, IDM_DEVPROPS, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu, IDM_SWITCHVIEW, MF_BYCOMMAND | MF_GRAYED);
}

void ResizeClientArea(HWND hwnd, int nWidth, int nHeight)
{
    RECT rcClientRect;
    RECT rcWindowRect;
    POINT ptDifference;

    GetClientRect(hwnd, &rcClientRect);
    GetWindowRect(hwnd, &rcWindowRect);
    ptDifference.x = (rcWindowRect.right - rcWindowRect.left) - rcClientRect.right;
    ptDifference.y = (rcWindowRect.bottom - rcWindowRect.top) - rcClientRect.bottom;
    MoveWindow(hwnd, rcWindowRect.left, rcWindowRect.top, nWidth + ptDifference.x, nHeight + ptDifference.y, TRUE);
}

void UpdateWindowCaption(HWND hwnd)
{
    TCHAR szNewTitle[MAX_PATH + 3 + 256];
    TCHAR szStatus[128];

    if (wDeviceId == 0)
    {
        SetWindowText(hwnd, szAppTitle);
        return;
    }

    switch (GetDeviceMode(hwnd))
    {
        case MCI_MODE_PAUSE:
        {
            LoadString(hInstance, IDS_MODE_PAUSE, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_STOP:
        {
            LoadString(hInstance, IDS_MODE_STOP, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_PLAY:
        {
            LoadString(hInstance, IDS_MODE_PLAY, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_OPEN:
        {
            LoadString(hInstance, IDS_MODE_OPEN, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_RECORD:
        {
            LoadString(hInstance, IDS_MODE_RECORD, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_SEEK:
        {
            LoadString(hInstance, IDS_MODE_SEEK, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        case MCI_MODE_NOT_READY:
        {
            LoadString(hInstance, IDS_MODE_NOT_READY, szStatus, ARRAYSIZE(szStatus));
            break;
        }

        default:
        {
            LoadString(hInstance, IDS_MODE_UNKNOWN, szStatus, ARRAYSIZE(szStatus));
        }
    }

    StringCbPrintf(szNewTitle, sizeof(szNewTitle), _T("%s - %s (%s)"), szAppTitle, szCurrentFile, szStatus);
    SetWindowText(hwnd, szNewTitle);
}

void UpdateTimeDisplay(HWND hwnd)
{
    MCI_STATUS_PARMS mciStatus;
    TCHAR szTime[MAX_MCISTR];
    DWORD dwTimeFormat;

    if (!wDeviceId)
    {
        SetWindowText(hwnd, _T(""));
        return;
    }

    mciStatus.dwItem = MCI_STATUS_TIME_FORMAT;
    mciStatus.dwReturn = 0;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);
    dwTimeFormat = mciStatus.dwReturn;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciStatus.dwReturn = 0;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

    switch(dwTimeFormat)
    {
        case MCI_FORMAT_MILLISECONDS:
        {
            int s, m, h;

            s = (mciStatus.dwReturn / 1000) % 60;
            m = ((mciStatus.dwReturn / (1000*60)) % 60);
            h = ((mciStatus.dwReturn / (1000*60*60)) % 24);
            StringCbPrintf(szTime, sizeof(szTime), _T("%02lu:%02lu:%02lu"), h, m, s);
            break;
        }

        /* The time format is unknown, so use the returned position as is */
        default:
        {
            StringCbPrintf(szTime, sizeof(szTime), _T("%lu"), mciStatus.dwReturn);
            break;
        }
    }

    SetWindowText(hwnd, szTime);
}

static VOID
ShowLastWin32Error(HWND hwnd)
{
    LPTSTR lpMessageBuffer;
    DWORD dwError = GetLastError();

    if (dwError == ERROR_SUCCESS)
        return;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dwError,
                       LANG_USER_DEFAULT,
                       (LPTSTR)&lpMessageBuffer,
                       0, NULL))
    {
        return;
    }

    MessageBox(hwnd, lpMessageBuffer, szAppTitle, MB_OK | MB_ICONERROR);
    LocalFree(lpMessageBuffer);
}

static VOID
SetImageList(HWND hwnd)
{
    HIMAGELIST hImageList;

    hImageList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR24, 1, 1);
    if (!hImageList)
    {
        ShowLastWin32Error(hwnd);
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

    ImageList_AddMasked(hImageList,
                        LoadImage(hInstance, MAKEINTRESOURCE(IDB_PAUSEICON), IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR),
                        RGB(255, 255, 255));

    ImageList_Destroy((HIMAGELIST)SendMessage(hToolBar,
                                              TB_SETIMAGELIST,
                                              0,
                                              (LPARAM)hImageList));
}

static VOID
ShowMCIError(HWND hwnd, MCIERROR mciError)
{
    TCHAR szErrorMessage[MAX_MCISTR];
    TCHAR szTempMessage[MAX_MCISTR + 44];

    if (mciGetErrorString(mciError, szErrorMessage, ARRAYSIZE(szErrorMessage)) == FALSE)
    {
        LoadString(hInstance, IDS_DEFAULTMCIERRMSG, szErrorMessage, ARRAYSIZE(szErrorMessage));
    }

    StringCbPrintf(szTempMessage, sizeof(szTempMessage), _T("MMSYS%lu: %s"), mciError, szErrorMessage);
    MessageBox(hwnd, szTempMessage, szAppTitle, MB_OK | MB_ICONEXCLAMATION);
}

static VOID
InitControls(HWND hwnd)
{
    INT NumButtons = ARRAYSIZE(Buttons);

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
        ShowLastWin32Error(hwnd);
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
        ShowLastWin32Error(hwnd);
        return;
    }

    hTimeDisplay = CreateWindowEx(0,
                                  L"STATIC",
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN,
                                  195,
                                  4,
                                  135,
                                  18,
                                  hToolBar,
                                  NULL,
                                  hInstance,
                                  NULL);
    if (!hTimeDisplay)
    {
        ShowLastWin32Error(hwnd);
        return;
    }

    SetImageList(hwnd);
    SendMessage(hToolBar, TB_ADDBUTTONS, NumButtons, (LPARAM)Buttons);
}

static VOID
SwitchViewMode(HWND hwnd)
{
    MCIERROR mciError;
    MCI_DGV_RECT_PARMS mciVideoRect;
    MCI_DGV_WINDOW_PARMSW mciVideoWindow;
    RECT rcToolbarRect;
    RECT rcTempRect;

    mciVideoWindow.hWnd = hwnd;

    mciError = mciSendCommand(wDeviceId, MCI_WINDOW, MCI_DGV_WINDOW_HWND | MCI_TEST, (DWORD_PTR)&mciVideoWindow);
    if (mciError != 0)
        return;

    mciError = mciSendCommand(wDeviceId, MCI_WHERE, MCI_DGV_WHERE_SOURCE | MCI_TEST, (DWORD_PTR)&mciVideoRect);
    if (mciError != 0)
        return;

    if (!bIsSingleWindow)
    {
        GetWindowRect(hwnd, &PrevWindowPos);

        SetParent(hTrackBar, hToolBar);

        mciError = mciSendCommand(wDeviceId, MCI_WHERE, MCI_DGV_WHERE_SOURCE, (DWORD_PTR)&mciVideoRect);
        if (mciError != 0)
        {
            ShowMCIError(hwnd, mciError);
            return;
        }

        GetWindowRect(hToolBar, &rcToolbarRect);
        ResizeClientArea(hwnd, mciVideoRect.rc.right, mciVideoRect.rc.bottom + (rcToolbarRect.bottom - rcToolbarRect.top));

        mciError = mciSendCommand(wDeviceId, MCI_WINDOW, MCI_DGV_WINDOW_HWND, (DWORD_PTR)&mciVideoWindow);
        if (mciError != 0)
        {
            ShowMCIError(hwnd, mciError);
            return;
        }

        GetWindowRect(hToolBar, &rcTempRect);
        MoveWindow(hTrackBar, 180, 0, rcTempRect.right - rcTempRect.left - 322, 25, TRUE);
        MoveWindow(hTimeDisplay, rcTempRect.right - rcTempRect.left - 140, 4, 135, 18, TRUE);

        CheckMenuItem(hMainMenu, IDM_SWITCHVIEW, MF_BYCOMMAND | MF_CHECKED);
        bIsSingleWindow = TRUE;
    }
    else
    {
        bIsSingleWindow = FALSE;
        CheckMenuItem(hMainMenu, IDM_SWITCHVIEW, MF_BYCOMMAND | MF_UNCHECKED);

        mciVideoWindow.hWnd = MCI_DGV_WINDOW_DEFAULT;
        mciError = mciSendCommand(wDeviceId, MCI_WINDOW, MCI_DGV_WINDOW_HWND, (DWORD_PTR)&mciVideoWindow);
        if (mciError != 0)
        {
            ShowMCIError(hwnd, mciError);
            return;
        }

        SetParent(hTrackBar, hwnd);

        MoveWindow(hwnd, PrevWindowPos.left, PrevWindowPos.top, PrevWindowPos.right - PrevWindowPos.left, PrevWindowPos.bottom - PrevWindowPos.top, TRUE);
    }
}

static DWORD
GetNumDevices(VOID)
{
    MCI_SYSINFO_PARMS mciSysInfo;
    DWORD dwNumDevices = 0;

    mciSysInfo.dwCallback  = 0;
    mciSysInfo.lpstrReturn = (LPTSTR)&dwNumDevices;
    mciSysInfo.dwRetSize   = sizeof(dwNumDevices);
    mciSysInfo.dwNumber    = 0;
    mciSysInfo.wDeviceType = MCI_ALL_DEVICE_ID;

    mciSendCommand(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_QUANTITY, (DWORD_PTR)&mciSysInfo);

    return *(DWORD*)mciSysInfo.lpstrReturn;
}

static DWORD
GetDeviceName(DWORD dwDeviceIndex, LPTSTR lpDeviceName, DWORD dwDeviceNameSize)
{
    MCI_SYSINFO_PARMS mciSysInfo;

    mciSysInfo.dwCallback  = 0;
    mciSysInfo.lpstrReturn = lpDeviceName;
    mciSysInfo.dwRetSize   = dwDeviceNameSize;
    mciSysInfo.dwNumber    = dwDeviceIndex;
    mciSysInfo.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;

    return mciSendCommand(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME, (DWORD_PTR)&mciSysInfo);
}

static DWORD
GetDeviceFriendlyName(LPTSTR lpDeviceName, LPTSTR lpFriendlyName, DWORD dwFriendlyNameSize)
{
    MCIERROR mciError;
    MCI_OPEN_PARMS mciOpen;
    MCI_INFO_PARMS mciInfo;
    MCI_GENERIC_PARMS mciGeneric;

    mciOpen.dwCallback = 0;
    mciOpen.wDeviceID  = 0;
    mciOpen.lpstrDeviceType  = lpDeviceName;
    mciOpen.lpstrElementName = NULL;
    mciOpen.lpstrAlias = NULL;

    mciError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_WAIT, (DWORD_PTR)&mciOpen);
    if (mciError != 0)
        return mciError;

    mciInfo.dwCallback  = 0;
    mciInfo.lpstrReturn = lpFriendlyName;
    mciInfo.dwRetSize   = dwFriendlyNameSize;

    mciError = mciSendCommand(mciOpen.wDeviceID, MCI_INFO, MCI_INFO_PRODUCT, (DWORD_PTR)&mciInfo);

    mciGeneric.dwCallback = 0;
    mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);

    return mciError;
}

static BOOL
DeviceUsesFiles(LPTSTR lpDeviceName)
{
    MCIERROR mciError;
    MCI_OPEN_PARMS mciOpen;
    MCI_GETDEVCAPS_PARMS mciDevCaps;
    MCI_GENERIC_PARMS mciGeneric;

    mciOpen.dwCallback = 0;
    mciOpen.wDeviceID  = 0;
    mciOpen.lpstrDeviceType  = lpDeviceName;
    mciOpen.lpstrElementName = NULL;
    mciOpen.lpstrAlias = NULL;

    mciError = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_WAIT, (DWORD_PTR)&mciOpen);
    if (mciError != 0)
        return FALSE;

    mciDevCaps.dwCallback  = 0;
    mciDevCaps.dwReturn = 0;
    mciDevCaps.dwItem = MCI_GETDEVCAPS_USES_FILES;

    mciError = mciSendCommand(mciOpen.wDeviceID, MCI_GETDEVCAPS, MCI_WAIT | MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&mciDevCaps);
    if (mciError != 0)
        return FALSE;

    mciGeneric.dwCallback = 0;
    mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);

    return (BOOL)mciDevCaps.dwReturn;
}

static MCIERROR
CloseMciDevice(VOID)
{
    MCIERROR mciError;
    MCI_GENERIC_PARMS mciGeneric;

    if (wDeviceId)
    {
        mciError = mciSendCommand(wDeviceId, MCI_CLOSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);
        if (mciError != 0) return mciError;
        wDeviceId = 0;
    }

    UpdateTimeDisplay(hTimeDisplay);

    DisableMenuItems();

    return 0;
}

static MCIERROR
OpenMciDevice(HWND hwnd, LPTSTR lpType, LPTSTR lpFileName)
{
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatus;
    MCI_OPEN_PARMS mciOpen;
    DWORD dwFlags = MCI_WAIT;
    LPTSTR lpStr;

    if (wDeviceId)
        CloseMciDevice();

    mciOpen.lpstrDeviceType = lpType;
    mciOpen.lpstrElementName = lpFileName;
    mciOpen.dwCallback = 0;
    mciOpen.wDeviceID = 0;
    mciOpen.lpstrAlias = NULL;

    if (lpType)
        dwFlags |= MCI_OPEN_TYPE;

    if (lpFileName)
        dwFlags |= MCI_OPEN_ELEMENT;

    mciError = mciSendCommand(0, MCI_OPEN, dwFlags, (DWORD_PTR)&mciOpen);
    if (mciError != 0)
        return mciError;

    mciStatus.dwItem = MCI_STATUS_LENGTH;

    mciError = mciSendCommand(mciOpen.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR)&mciStatus);
    if (mciError != 0)
        return mciError;

    SendMessage(hTrackBar, TBM_SETRANGEMIN, (WPARAM)TRUE, (LPARAM)1);
    SendMessage(hTrackBar, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)mciStatus.dwReturn);
    SendMessage(hTrackBar, TBM_SETPAGESIZE, 0, 10);
    SendMessage(hTrackBar, TBM_SETLINESIZE, 0, 1);
    SendMessage(hTrackBar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1);

    if (mciStatus.dwReturn < 10000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM)100, (LPARAM)0);
    }
    else if (mciStatus.dwReturn < 100000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM)1000, (LPARAM)0);
    }
    else if (mciStatus.dwReturn < 1000000)
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM)10000, (LPARAM)0);
    }
    else
    {
        SendMessage(hTrackBar, TBM_SETTICFREQ, (WPARAM)100000, (LPARAM)0);
    }

    MaxFilePos = mciStatus.dwReturn;
    wDeviceId = mciOpen.wDeviceID;

    /* NOTE: Everything above this line may be done instead in OpenMediaFile() */

    if (lpFileName)
    {
        lpStr = _tcsrchr(lpFileName, _T('\\'));
        if (lpStr) // Get only the file name (skip the last path separator)
            lpStr++;
        else
            lpStr = lpFileName;
    }
    else
        lpStr = lpType;

    StringCbCopy(szCurrentFile, sizeof(szCurrentFile), lpStr);

    EnableMenuItems(hwnd);

    UpdateTimeDisplay(hTimeDisplay);
    UpdateWindowCaption(hwnd);

    return 0;
}

static DWORD
GetDeviceMode(HWND hwnd)
{
    MCIERROR mciError;
    MCI_STATUS_PARMS mciStatus;

    mciStatus.dwItem = MCI_STATUS_MODE;
    mciError = mciSendCommand(wDeviceId, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
        return MCI_MODE_NOT_READY;
    }

    return mciStatus.dwReturn;
}

static VOID
StopPlayback(HWND hwnd)
{
    MCIERROR mciError;
    MCI_GENERIC_PARMS mciGeneric;
    MCI_SEEK_PARMS mciSeek;

    if (wDeviceId == 0) return;

    KillTimer(hwnd, IDT_PLAYTIMER);
    SendMessage(hTrackBar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1);

    mciGeneric.dwCallback = (DWORD_PTR)hwnd;
    mciError = mciSendCommand(wDeviceId, MCI_STOP, MCI_WAIT, (DWORD_PTR)&mciGeneric);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
        return;
    }

    mciSendCommand(wDeviceId, MCI_SEEK, MCI_WAIT | MCI_SEEK_TO_START, (DWORD_PTR)&mciSeek);

    UpdateTimeDisplay(hTimeDisplay);
    UpdateWindowCaption(hwnd);

    SendMessage(hToolBar,
                TB_SETCMDID,
                0,
                IDC_PLAY);
    SendMessage(hToolBar,
                TB_CHANGEBITMAP,
                IDC_PLAY,
                IDB_PLAYICON - IDB_PLAYICON);
}

static VOID
SeekPlayback(HWND hwnd, DWORD dwNewPos)
{
    MCIERROR mciError;
    MCI_SEEK_PARMS mciSeek;
    MCI_PLAY_PARMS mciPlay;

    if (wDeviceId == 0) return;

    mciSeek.dwTo = (DWORD_PTR)dwNewPos;
    mciError = mciSendCommand(wDeviceId, MCI_SEEK, MCI_WAIT | MCI_TO, (DWORD_PTR)&mciSeek);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
    }

    mciPlay.dwCallback = (DWORD_PTR)hwnd;
    mciError = mciSendCommand(wDeviceId, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciPlay);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
    }
}

static VOID
SeekBackPlayback(HWND hwnd)
{
    MCI_STATUS_PARMS mciStatus;
    DWORD dwNewPos;

    if (wDeviceId == 0) return;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

    dwNewPos = mciStatus.dwReturn - 1;

    if ((UINT)dwNewPos <= 1)
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

    if (wDeviceId == 0) return;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

    dwNewPos = mciStatus.dwReturn + 1;

    if ((UINT)dwNewPos >= MaxFilePos)
    {
        StopPlayback(hwnd);
    }
    else
    {
        SeekPlayback(hwnd, dwNewPos);
    }
}

VOID CALLBACK
PlayTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    MCI_STATUS_PARMS mciStatus;
    DWORD dwPos;

    if (wDeviceId == 0) KillTimer(hwnd, IDT_PLAYTIMER);

    mciStatus.dwItem = MCI_STATUS_POSITION;
    mciSendCommand(wDeviceId, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);
    dwPos = mciStatus.dwReturn;

    SendMessage(hTrackBar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)dwPos);
    UpdateTimeDisplay(hTimeDisplay);
}

static VOID
StartPlayback(HWND hwnd)
{
    MCIERROR mciError;
    MCI_PLAY_PARMS mciPlay;
    MCI_SEEK_PARMS mciSeek;

    SetTimer(hwnd, IDT_PLAYTIMER, 100, (TIMERPROC)PlayTimerProc);

    mciSendCommand(wDeviceId, MCI_SEEK, MCI_WAIT | MCI_SEEK_TO_START, (DWORD_PTR)&mciSeek);

    mciPlay.dwCallback = (DWORD_PTR)hwnd;
    mciPlay.dwFrom = 0;
    mciPlay.dwTo = MaxFilePos;

    mciError = mciSendCommand(wDeviceId, MCI_PLAY, MCI_NOTIFY | MCI_FROM /*| MCI_TO*/, (DWORD_PTR)&mciPlay);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
        return;
    }

    UpdateWindowCaption(hwnd);

    SendMessage(hToolBar,
                TB_SETCMDID,
                0,
                IDC_PAUSE);
    SendMessage(hToolBar,
                TB_CHANGEBITMAP,
                IDC_PAUSE,
                IDB_PAUSEICON - IDB_PLAYICON);
}

static VOID
TogglePlaybackState(HWND hwnd)
{
    MCIERROR mciError;
    MCI_GENERIC_PARMS mciGeneric;
    ULONG idBmp = IDB_PLAYICON;
    ULONG idCmd = IDC_PLAY;

    if (wDeviceId == 0) return;

    switch (GetDeviceMode(hwnd))
    {
        case MCI_MODE_OPEN:
        case MCI_MODE_STOP:
        {
            StartPlayback(hwnd);
            return;
        }

        case MCI_MODE_PLAY:
        {
            mciGeneric.dwCallback = (DWORD_PTR)hwnd;
            mciError = mciSendCommand(wDeviceId, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&mciGeneric);
            idBmp = IDB_PLAYICON;
            idCmd = IDC_PLAY;
            break;
        }

        case MCI_MODE_PAUSE:
        {
            mciGeneric.dwCallback = (DWORD_PTR)hwnd;
            mciError = mciSendCommand(wDeviceId, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&mciGeneric);
            idBmp = IDB_PAUSEICON;
            idCmd = IDC_PAUSE;
            break;
        }

        default:
        {
            return;
        }
    }

    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
        return;
    }

    UpdateWindowCaption(hwnd);

    SendMessage(hToolBar,
                TB_SETCMDID,
                0,
                idCmd);
    SendMessage(hToolBar,
                TB_CHANGEBITMAP,
                idCmd,
                idBmp - IDB_PLAYICON);
}

static VOID
ShowDeviceProperties(HWND hwnd)
{
    MCIERROR mciError;
    MCI_GENERIC_PARMS mciGeneric;

    mciError = mciSendCommand(wDeviceId, MCI_CONFIGURE, MCI_WAIT, (DWORD_PTR)&mciGeneric);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
    }
}

static VOID
CloseMediaFile(HWND hwnd)
{
    StopPlayback(hwnd);

    if (bIsSingleWindow)
        SwitchViewMode(hwnd);

    CloseMciDevice();
    UpdateWindowCaption(hwnd);
}

static VOID
OpenMediaFile(HWND hwnd, LPTSTR lpFileName, LPTSTR lpType)
{
    MCIERROR mciError;

    if (GetFileAttributes(lpFileName) == INVALID_FILE_ATTRIBUTES)
        return;

    if (wDeviceId)
        CloseMediaFile(hwnd);

    mciError = OpenMciDevice(hwnd, lpType, lpFileName);
    if (mciError != 0)
    {
        ShowMCIError(hwnd, mciError);
        return;
    }

    StartPlayback(hwnd);
}

static DWORD
InsertDeviceMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition, UINT uItemID, DWORD dwDeviceIndex)
{
    MENUITEMINFO lpmii;
    MCIERROR mciError;
    TCHAR szDeviceName[MAX_MCISTR];
    TCHAR szFriendlyName[MAX_MCISTR];

    mciError = GetDeviceName(dwDeviceIndex, szDeviceName, sizeof(szDeviceName));
    if (mciError)
    {
        return mciError;
    }

    mciError = GetDeviceFriendlyName(szDeviceName, szFriendlyName, sizeof(szFriendlyName));
    if (mciError)
    {
        return mciError;
    }

    if (DeviceUsesFiles(szDeviceName))
    {
        StringCbCat(szFriendlyName, sizeof(szFriendlyName), _T("..."));
    }

    ZeroMemory(&lpmii, sizeof(MENUITEMINFO));
    lpmii.cbSize = sizeof(lpmii);
    lpmii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
    lpmii.wID = uItemID;
    lpmii.fType = MF_STRING;
    lpmii.dwTypeData = szFriendlyName;
    lpmii.dwItemData = dwDeviceIndex;
    InsertMenuItem(hMenu, uItem, fByPosition, &lpmii);

    return 0;
}

static VOID
BuildFileFilterAndDeviceMenu(VOID)
{
    TCHAR szDeviceName[MAX_MCISTR];
    TCHAR szFriendlyName[MAX_MCISTR];
    TCHAR *szDevice = NULL;
    static TCHAR szDefaultExtension[] = _T("*.*");
    TCHAR *szExtensionList = NULL;
    TCHAR *szExtension = NULL;
    TCHAR *c = NULL;
    TCHAR *d = NULL;
    DWORD dwNumValues;
    DWORD dwNumDevices;
    DWORD dwValueNameLen;
    DWORD dwValueDataSize;
    DWORD dwMaskLen;
    DWORD dwFilterSize;
    DWORD dwDeviceSize;
    DWORD dwExtensionLen;
    DWORD dwPosition = 0;
    DWORD i;
    DWORD j;
    size_t uSizeRemain;
    size_t uMaskRemain;
    HKEY hKey = NULL;

    /* Always load the default (all files) filter */
    LoadString(hInstance, IDS_ALL_TYPES_FILTER, szDefaultFilter, ARRAYSIZE(szDefaultFilter));

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MCI Extensions"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        goto Failure;
    }

    if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwNumValues, &dwValueNameLen, &dwValueDataSize, NULL, NULL) != ERROR_SUCCESS)
    {
        goto Failure;
    }

    dwMaskLen = ((dwValueNameLen + 3) * dwNumValues) + 1;

    szExtensionList = malloc(dwMaskLen * sizeof(TCHAR));
    if (!szExtensionList)
        goto Failure;

    dwNumDevices = GetNumDevices();

    /* Allocate space for every pair of Device and Extension Filter */
    dwFilterSize = (MAX_MCISTR + (dwMaskLen * 2) + 5) * dwNumDevices;

    /* Add space for the "All supported" entry */
    dwFilterSize = (dwFilterSize + (dwMaskLen * 2) + 7) * sizeof(TCHAR) + sizeof(szDefaultFilter);

    szFilter = malloc(dwFilterSize);
    if (!szFilter)
        goto Failure;

    szExtension = malloc((dwValueNameLen + 1) * sizeof(TCHAR));
    if (!szExtension)
        goto Failure;

    szDevice = malloc(dwValueDataSize + sizeof(TCHAR));
    if (!szDevice)
        goto Failure;

    ZeroMemory(szFilter, dwFilterSize);

    uSizeRemain = dwFilterSize;
    c = szFilter;

    for (j = 1; j <= dwNumDevices; j++)
    {
        if (GetDeviceName(j, szDeviceName, sizeof(szDeviceName)))
        {
            continue;
        }

        if (GetDeviceFriendlyName(szDeviceName, szFriendlyName, sizeof(szFriendlyName)))
        {
            continue;
        }

        /* Insert a menu item under the "Device" menu for every found MCI device */
        InsertDeviceMenuItem(GetSubMenu(hMainMenu, 3), dwPosition, TRUE, IDM_DEVICE_FIRST + dwPosition, j);
        dwPosition++;

        /* Copy the default extension list, that may be overwritten after... */
        StringCbCopy(szExtensionList, dwMaskLen * sizeof(TCHAR), szDefaultExtension);

        /* Try to determine the real extension list */
        uMaskRemain = dwMaskLen * sizeof(TCHAR);
        d = szExtensionList;

        for (i = 0; i < dwNumValues; i++)
        {
            dwExtensionLen = dwValueNameLen + 1;
            dwDeviceSize   = dwValueDataSize + sizeof(TCHAR);

            ZeroMemory(szDevice, dwDeviceSize);

            if (RegEnumValue(hKey, i, szExtension, &dwExtensionLen, NULL, NULL, (LPBYTE)szDevice, &dwDeviceSize) == ERROR_SUCCESS)
            {
                CharLowerBuff(szDevice, dwDeviceSize / sizeof(TCHAR));
                CharLowerBuff(szDeviceName, ARRAYSIZE(szDeviceName));
                if (_tcscmp(szDeviceName, szDevice) == 0)
                {
                     CharLowerBuff(szExtension, dwExtensionLen);
                     StringCbPrintfEx(d, uMaskRemain, &d, &uMaskRemain, 0, _T("%s%s%s"), _T("*."), szExtension, _T(";"));
                }
            }
        }

        /* Remove the last separator */
        d--;
        uSizeRemain += sizeof(*d);
        *d = _T('\0');

        /* Add the description */
        StringCbPrintfEx(c, uSizeRemain, &c, &uSizeRemain, 0, _T("%s (%s)"), szFriendlyName, szExtensionList);

        /* Skip one char to separate the description from the filter mask */
        c++;
        uSizeRemain -= sizeof(*c);

        /* Append the filter mask */
        StringCbCopyEx(c, uSizeRemain, szExtensionList, &c, &uSizeRemain, 0);

        /* Skip another char to separate the elements of the filter mask */
        c++;
        uSizeRemain -= sizeof(*c);
    }

    /* Build the full list of supported extensions */
    uMaskRemain = dwMaskLen * sizeof(TCHAR);
    d = szExtensionList;

    for (i = 0; i < dwNumValues; i++)
    {
        dwExtensionLen = dwValueNameLen + 1;

        if (RegEnumValue(hKey, i, szExtension, &dwExtensionLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            CharLowerBuff(szExtension, dwExtensionLen);
            StringCbPrintfEx(d, uMaskRemain, &d, &uMaskRemain, 0, _T("%s%s%s"), _T("*."), szExtension, _T(";"));
        }
    }

    /* Remove the last separator */
    d--;
    uSizeRemain += sizeof(*d);
    *d = _T('\0');

    /* Add the default (all files) description */
    StringCbPrintfEx(c, uSizeRemain, &c, &uSizeRemain, 0, _T("%s (%s)"), szDefaultFilter, szExtensionList);

    /* Skip one char to separate the description from the filter mask */
    c++;
    uSizeRemain -= sizeof(*c);

    /* Append the filter mask */
    StringCbCopyEx(c, uSizeRemain, szExtensionList, &c, &uSizeRemain, 0);

Cleanup:
    if (szExtensionList) free(szExtensionList);
    if (szExtension)     free(szExtension);
    if (szDevice)        free(szDevice);
    RegCloseKey(hKey);

    return;

Failure:
    /* We failed at retrieving the supported files, so use the default filter */
    if (szFilter) free(szFilter);
    szFilter = szDefaultFilter;

    uSizeRemain = sizeof(szDefaultFilter);
    c = szFilter;

    /* Add the default (all files) description */
    StringCbPrintfEx(c, uSizeRemain, &c, &uSizeRemain, 0, _T("%s (%s)"), szDefaultFilter, szDefaultExtension);

    /* Skip one char to separate the description from the filter mask */
    c++;
    uSizeRemain -= sizeof(*c);

    /* Append the filter mask */
    StringCbCopyEx(c, uSizeRemain, szDefaultExtension, &c, &uSizeRemain, 0);

    goto Cleanup;
}

static VOID
CleanupFileFilter(VOID)
{
    if (szFilter && szFilter != szDefaultFilter) free(szFilter);
}

static VOID
OpenFileDialog(HWND hwnd, DWORD dwFilterIndex, LPTSTR lpType)
{
    OPENFILENAME OpenFileName;
    TCHAR szFile[MAX_PATH + 1] = _T("");

    ZeroMemory(&OpenFileName, sizeof(OpenFileName));

    OpenFileName.lStructSize     = sizeof(OpenFileName);
    OpenFileName.hwndOwner       = hwnd;
    OpenFileName.hInstance       = hInstance;
    OpenFileName.lpstrFilter     = szFilter;
    OpenFileName.lpstrFile       = szFile;
    OpenFileName.nMaxFile        = ARRAYSIZE(szFile);
    OpenFileName.Flags           = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE;
    OpenFileName.lpstrDefExt     = _T("\0");
    OpenFileName.nFilterIndex = dwFilterIndex;

    if (!GetOpenFileName(&OpenFileName))
        return;

    OpenMediaFile(hwnd, OpenFileName.lpstrFile, lpType);
}

static VOID
HandleDeviceMenuItem(HWND hwnd, UINT uItem)
{
    MENUITEMINFO lpmii;
    TCHAR szDeviceName[MAX_MCISTR];
    MCIERROR mciError;

    ZeroMemory(&lpmii, sizeof(MENUITEMINFO));
    lpmii.cbSize = sizeof(lpmii);
    lpmii.fMask = MIIM_DATA;
    GetMenuItemInfo(hMainMenu, uItem, FALSE, &lpmii);

    mciError = GetDeviceName(lpmii.dwItemData, szDeviceName, sizeof(szDeviceName));
    if (mciError)
    {
        ShowMCIError(hwnd, mciError);
        return;
    }

    if (DeviceUsesFiles(szDeviceName))
    {
        OpenFileDialog(hwnd, uItem - IDM_DEVICE_FIRST + 1, szDeviceName);
        return;
    }

    mciError = OpenMciDevice(hwnd, szDeviceName, NULL);
    if (mciError)
    {
        ShowMCIError(hwnd, mciError);
    }

    return;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
        {
            InitControls(hwnd);
            hMainMenu = GetMenu(hwnd);
            break;
        }

        case WM_DROPFILES:
        {
            HDROP drophandle;
            TCHAR droppedfile[MAX_PATH];

            drophandle = (HDROP)wParam;
            DragQueryFile(drophandle, 0, droppedfile, ARRAYSIZE(droppedfile));
            DragFinish(drophandle);
            OpenMediaFile(hwnd, droppedfile, NULL);
            break;
        }

        case MM_MCINOTIFY:
        {
            if (wParam == MCI_NOTIFY_SUCCESSFUL)
            {
                StopPlayback(hwnd);
                if (bRepeat)
                {
                    StartPlayback(hwnd);
                }
            }
            break;
        }

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
                        case IDC_PAUSE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PAUSE);
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

            if (!bIsSingleWindow)
            {
                if (pRect->right - pRect->left < MAIN_WINDOW_MIN_WIDTH)
                    pRect->right = pRect->left + MAIN_WINDOW_MIN_WIDTH;

                if (pRect->bottom - pRect->top != MAIN_WINDOW_HEIGHT)
                    pRect->bottom = pRect->top + MAIN_WINDOW_HEIGHT;
            }
            return TRUE;
        }

        case WM_SIZE:
        {
            RECT Rect;

            if (hToolBar && hTrackBar)
            {
                SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
                SendMessage(hToolBar, TB_GETITEMRECT, 1, (LPARAM)&Rect);
                MoveWindow(hTimeDisplay, LOWORD(lParam) - 140, 4, 135, 18, TRUE);

                if (!bIsSingleWindow)
                {
                    UINT Size = GetSystemMetrics(SM_CYMENU) + Rect.bottom;
                    MoveWindow(hTrackBar, 0, 0, LOWORD(lParam), HIWORD(lParam) - Size, TRUE);
                }
                else
                {
                    RECT ToolbarRect;
                    MCI_DGV_PUT_PARMS mciPut;

                    MoveWindow(hTrackBar, 180, 0, LOWORD(lParam) - 322, 25, TRUE);

                    GetClientRect(hwnd, &Rect);
                    GetClientRect(hToolBar, &ToolbarRect);

                    mciPut.rc.top = 0;
                    mciPut.rc.left = 0;
                    mciPut.rc.right = Rect.right;
                    mciPut.rc.bottom = Rect.bottom - (ToolbarRect.bottom - ToolbarRect.top) - 2;

                    mciSendCommand(wDeviceId, MCI_PUT, MCI_DGV_PUT_DESTINATION | MCI_DGV_RECT | MCI_WAIT, (DWORD_PTR)&mciPut);
                }
            }
            return 0L;
        }

        case WM_HSCROLL:
        {
            if (hTrackBar == (HWND)lParam)
            {
                if (wDeviceId)
                {
                    DWORD dwNewPos = (DWORD)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
                    SeekPlayback(hwnd, dwNewPos);
                }
                else
                {
                    SendMessage(hTrackBar, TBM_SETPOS, TRUE, 0);
                }
            }
        }
        break;

        case WM_NCLBUTTONDBLCLK:
        {
            if (wParam == HTCAPTION)
            {
                SwitchViewMode(hwnd);
            }
        }
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) >= IDM_DEVICE_FIRST)
            {
                HandleDeviceMenuItem(hwnd, LOWORD(wParam));
                break;
            }

            switch (LOWORD(wParam))
            {
                case IDC_PLAY:
                case IDC_PAUSE:
                {
                    if (wDeviceId)
                        TogglePlaybackState(hwnd);
                    else
                        OpenFileDialog(hwnd, 1, NULL);

                    break;
                }

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
                    OpenFileDialog(hwnd, 1, NULL);
                    return 0;

                case IDM_CLOSE_FILE:
                    CloseMediaFile(hwnd);
                    break;

                case IDM_REPEAT:
                {
                    if (!bRepeat)
                    {
                        CheckMenuItem(hMainMenu, IDM_REPEAT, MF_BYCOMMAND | MF_CHECKED);
                        bRepeat = TRUE;
                    }
                    else
                    {
                        CheckMenuItem(hMainMenu, IDM_REPEAT, MF_BYCOMMAND | MF_UNCHECKED);
                        bRepeat = FALSE;
                    }
                    break;
                }

                case IDM_SWITCHVIEW:
                    SwitchViewMode(hwnd);
                    break;

                case IDM_DEVPROPS:
                    ShowDeviceProperties(hwnd);
                    break;

                case IDM_VOLUMECTL:
                    ShellExecute(hwnd, NULL, _T("SNDVOL32.EXE"), NULL, NULL, SW_SHOWNORMAL);
                    break;

                case IDM_ABOUT:
                {
                    HICON mplayIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
                    ShellAbout(hwnd, szAppTitle, NULL, mplayIcon);
                    DeleteObject(mplayIcon);
                    break;
                }

                case IDM_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
            }
            break;
        }

        case WM_DESTROY:
            CloseMediaFile(hwnd);
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
    DWORD dwError;
    HANDLE hAccel;

    hInstance = hInst;

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }

    LoadString(hInstance, IDS_APPTITLE, szAppTitle, ARRAYSIZE(szAppTitle));

    WndClass.cbSize            = sizeof(WndClass);
    WndClass.lpszClassName     = szClassName;
    WndClass.lpfnWndProc       = MainWndProc;
    WndClass.hInstance         = hInstance;
    WndClass.style             = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon             = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    WndClass.hCursor           = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground     = (HBRUSH)(COLOR_BTNFACE + 1);
    WndClass.lpszMenuName      = MAKEINTRESOURCE(IDR_MAINMENU);

    if (!RegisterClassEx(&WndClass))
    {
        ShowLastWin32Error(NULL);
        return 0;
    }

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
    if (!hwnd)
    {
        ShowLastWin32Error(NULL);
        return 0;
    }

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCELERATORS));

    BuildFileFilterAndDeviceMenu();

    DragAcceptFiles(hwnd, TRUE);

    DisableMenuItems();

    dwError = SearchPath(NULL, _T("SNDVOL32.EXE"), NULL, 0, NULL, NULL);
    if (dwError == 0)
    {
        EnableMenuItem(hMainMenu, IDM_VOLUMECTL, MF_BYCOMMAND | MF_GRAYED);
    }

    /* Show it */
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    if (*lpCmdLine == _T('"'))
    {
        OpenMediaFile(hwnd, argv[1], NULL);
    }
    else
    {
        OpenMediaFile(hwnd, lpCmdLine, NULL);
    }

    /* Message Loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CleanupFileFilter();

    DestroyAcceleratorTable(hAccel);

    return (INT)msg.wParam;
}
