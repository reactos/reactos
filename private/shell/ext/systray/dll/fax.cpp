/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fax.cpp

Abstract:

    This module implements the tray icon for fax.
    The peupose of the tray icon os to provide
    status and feedback to the fax user.

Author:

    Wesley Witt (wesw) 14-Oct-1997 (ported from georgeje's code in faxstat)

Revision History:

--*/

#include "stdafx.h"

extern "C" {
#include <systray.h>
}

#include <commctrl.h>
#include "faxreg.h"
#include "winfax.h"
#include "faxhelp.h"
#include <winspool.h>
#include <shlobj.h>

#define MemAlloc(uBytes)       LocalAlloc(LPTR, uBytes)
#define MemFree(lpMem)         LocalFree((HLOCAL) lpMem)

#define FAX_DRIVER_NAME        L"Windows NT Fax Driver"

#define STRING_SIZE            256
#define MAX_EVENTS             100

#define ID_FAX_PROGRESS_TIMER  1
#define ID_FAX_ICON_TIMER      2

// WM_FAXSTAT_CONTROLPANEL already #defined as (WM_USER + 201) in faxreg.h
// WM_FAXSTAT_MMC already #defined as (WM_USER + 202) in faxreg.h
#define WM_FAXSTAT_INITIALIZE  (WM_USER + 203)
#define WM_FAX_STARTED         (WM_USER + 204)
#define WM_TRAYCALLBACK        (WM_USER + 205)
#define WM_FAX_VIEW            (WM_USER + 206)
#define WM_FAX_EVENT           (WM_USER + 300)

typedef struct _CONFIG_OPTIONS {
    DWORD  VisualNotification;   // Dialog box
    DWORD  OnTop;                // Dialog always on top
    DWORD  TaskBar;              // Display icon in status area
    DWORD  SoundNotification;    // Play a sound
    DWORD  ManualAnswerEnabled;  // Enable manual answer for the first device
    BOOL   UserHasAccess;        // User has job manage access
} CONFIG_OPTIONS, *PCONFIG_OPTIONS;

extern "C" HINSTANCE            g_hInstance;
HWND                            hWndFaxStat = NULL;               // Handle to the faxstat window
HWND                            hWndAnswerDlg = NULL;             // Handle to the answer dialog

HMODULE                         hModWinfax;
PFAXCONNECTFAXSERVER            pFaxConnectFaxServer;
PFAXCLOSE                       pFaxClose;
PFAXFREEBUFFER                  pFaxFreeBuffer;
PFAXINITIALIZEEVENTQUEUE        pFaxInitializeEventQueue;
PFAXENUMPORTS                   pFaxEnumPorts;
PFAXOPENPORT                    pFaxOpenPort;
PFAXGETPORT                     pFaxGetPort;
PFAXSETPORT                     pFaxSetPort;
PFAXGETDEVICESTATUS             pFaxGetDeviceStatus;
PFAXABORT                       pFaxAbort;
PFAXACCESSCHECK                 pFaxAccessCheck;

HANDLE                          hFaxSvcHandle = NULL;             // Handle to the fax service

HANDLE                          hFaxStartedEvent = NULL;          // Event to indicate fax service has started
HANDLE                          hFaxStatMutex = NULL;             // Object to serialize access to to sensitive info

DWORD                           dwFaxPortDeviceId = (DWORD) -1;   // Device id to use
DWORD                           dwFaxPortInfoFlags = (DWORD) -1;  // Original send and receive mask of device
DWORD                           dwFaxPortInfoRings = (DWORD) -1;  // Original number of rings

HICON                           hFaxIconNormal;
HICON                           hFaxIconInfo;
NOTIFYICONDATA                  IconData;
BOOL                            bFaxRcvAck = TRUE;                // Indicates a user has acknowledged a received fax

WCHAR                           szCurrentUser[STRING_SIZE];       // Current user name

LPWSTR                          szTimeSep;                        // Minutes / seconds time separator
DWORD                           dwSeconds;                        // Number of second the fax has been in progress

//
// Configuration options and their defaults
//
CONFIG_OPTIONS ConfigOptions =
{
    0,      // OnTop
    1,      // TaskBar
    1,      // VisualNotification
    0,      // SoundNotification
    0,      // ManualAnswerEnabled
    FALSE   // UserHasAccess
};

DWORD monitorHelpIDs[] =
{
    IDC_FAX_ANIMATE,      IDH_FAXMONITOR_ICON,
    IDC_FAX_STATUS,       IDH_FAXMONITOR_STATUS,
    IDC_FAX_ETIME,        IDH_FAXMONITOR_STATUS,
    IDC_FAX_FROMTO,       IDH_FAXMONITOR_STATUS,
    IDC_FAX_DETAILS,      IDH_FAXMONITOR_DETAILS,
    IDC_ANSWER_NEXT_CALL, IDH_FAXMONITOR_ANSWER_NEXT_CALL,
    IDC_END_FAX_CALL,     IDH_FAXMONITOR_END_CALL,
    IDC_FAX_DETAILS_LIST, IDH_FAXDETAILS_DETAILS_LIST,
    0,                    0
};

LRESULT CALLBACK FaxStatWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

typedef BOOL (WINAPI *PENUMPRINTERS) (DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);

int MyLoadString(
    HINSTANCE  hInstance,
    UINT       uID,
    LPTSTR     lpBuffer,
    int        nBufferMax,
    LANGID     LangID
)
{
    HRSRC   hFindRes;  // Handle from FindResourceEx
    HANDLE  hLoadRes;  // Handle from LoadResource
    LPWSTR  pSearch;   // Pointer to search for correct string
    int     cch = 0;   // Count of characters

#ifndef UNICODE
    LPWSTR  pString;   // Pointer to temporary string
#endif

    //
    //  String Tables are broken up into segments of 16 strings each.  Find the segment containing the string we want.
    //
    if ((!(hFindRes = FindResourceEx(hInstance, RT_STRING, (LPTSTR) ((LONG) (((USHORT) uID >> 4) + 1)), (WORD) LangID)))) {
        //
        //  Could not find resource.  Return 0.
        //
        return (cch);
    }

    //
    //  Load the resource.
    //
    hLoadRes = LoadResource(hInstance, hFindRes);

    //
    //  Lock the resource.
    //
    if (pSearch = (LPWSTR) LockResource(hLoadRes)) {
        //
        //  Move past the other strings in this segment. (16 strings in a segment -> & 0x0F)
        //
        uID &= 0x0F;

        //
        //  Find the correct string in this segment.
        //
        while (TRUE) {
            cch = *((WORD *) pSearch++);
            if (uID-- == 0) {
                break;
            }

            pSearch += cch;
        }

        //
        //  Store the found pointer in the given pointer.
        //
        if (nBufferMax < cch) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

#ifndef UNICODE
        pString = MemAlloc(sizeof(WCHAR) * nBufferMax);
        ZeroMemory(pString, sizeof(WCHAR) * nBufferMax);
        CopyMemory(pString, pSearch, sizeof(WCHAR) * cch);

        WideCharToMultiByte(CP_THREAD_ACP, 0, pString, -1, lpBuffer, (cch + 1), NULL, NULL);
        MemFree(pString);
#else
        ZeroMemory(lpBuffer, sizeof(WCHAR) * nBufferMax);
        CopyMemory(lpBuffer, pSearch, sizeof(WCHAR) * cch);
#endif
    }

    //
    //  Return the number of characters in the string.
    //
    return (cch);
}

DWORD
GetRegistryDword(
    HKEY    hKey,
    LPWSTR  szSubKey
)
{
    DWORD  dwValue;
    DWORD  dwSize;

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, szSubKey, NULL, NULL, (LPBYTE) &dwValue, &dwSize) != ERROR_SUCCESS) {
        return 0;
    }

    return dwValue;
}

PVOID
MyEnumPrinters(
    LPWSTR  pServerName,
    DWORD   dwLevel,
    PDWORD  pdwNumPrinters,
    DWORD   dwFlags
)
{
    HMODULE        hModWinspool;
    PENUMPRINTERS  pEnumPrinters;
    PBYTE          pPrinterInfo;
    DWORD          cb;

    //
    // Initialize pPrinterInfo and pdwNumPrinters
    //
    pPrinterInfo = NULL;
    *pdwNumPrinters = 0;

    //
    // Load the winspool drv
    //
    hModWinspool = LoadLibrary(L"winspool.drv");
    if (hModWinspool == NULL) {
        return NULL;
    }

    //
    // Get the addresses of the print functions
    //
    pEnumPrinters = (PENUMPRINTERS) GetProcAddress(hModWinspool, "EnumPrintersW");

    if (!pEnumPrinters) {
        FreeLibrary(hModWinspool);
        return NULL;
    }

    if (!dwFlags) {
        dwFlags = PRINTER_ENUM_LOCAL;
    }

    //
    // Get all printers
    //
    cb = 0;
    pEnumPrinters(dwFlags, pServerName, dwLevel, NULL, 0, &cb, pdwNumPrinters);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        pPrinterInfo = (PBYTE) MemAlloc(cb);
        if (!pEnumPrinters(dwFlags, pServerName, dwLevel, pPrinterInfo, cb, &cb, pdwNumPrinters)) {
            MemFree(pPrinterInfo);
            pPrinterInfo = NULL;
        }
    }

    FreeLibrary (hModWinspool);
    return pPrinterInfo;
}

BOOL
IsFaxPrinterInstalled(
)
{
    PPRINTER_INFO_2  pPrinterInfo;
    DWORD            dwNumPrinters;
    BOOL             bFaxPrinterInstalled;
    DWORD            dwIndex;

    //
    // Loop through all printers to see if one is a fax printer
    //
    bFaxPrinterInstalled = FALSE;
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL, 2, &dwNumPrinters, 0);
    if (pPrinterInfo) {
        for (dwIndex = 0; dwIndex < dwNumPrinters; dwIndex++) {
            if (lstrcmpi(pPrinterInfo[dwIndex].pDriverName, FAX_DRIVER_NAME) == 0) {
                bFaxPrinterInstalled = TRUE;
                break;
            }
        }

        MemFree(pPrinterInfo);
    }

    return bFaxPrinterInstalled;
}

BOOL
IsFaxSvcRunning(
)
{
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  Status;

    ZeroMemory(&Status, sizeof(Status));

    hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSvcMgr) {
        goto ExitLevel0;
    }

    hService = OpenService(hSvcMgr, L"Fax", SERVICE_QUERY_STATUS);
    if (!hService) {
        goto ExitLevel1;
    }

    QueryServiceStatus(hService, &Status);

ExitLevel1:
    if (hService) {
        CloseServiceHandle(hService);
    }

ExitLevel0:
    if (hSvcMgr) {
        CloseServiceHandle(hSvcMgr);
    }

    return (Status.dwCurrentState == SERVICE_RUNNING) ? TRUE : FALSE;
}

BOOL
GetConfiguration(
)
{
    HKEY  hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    ConfigOptions.VisualNotification  = GetRegistryDword(hKey, REGVAL_VISUAL_NOTIFICATION);
    ConfigOptions.OnTop               = GetRegistryDword(hKey, REGVAL_ALWAYS_ON_TOP);
    ConfigOptions.TaskBar             = GetRegistryDword(hKey, REGVAL_TASKBAR);
    ConfigOptions.SoundNotification   = GetRegistryDword(hKey, REGVAL_SOUND_NOTIFICATION);
    ConfigOptions.ManualAnswerEnabled = GetRegistryDword(hKey, REGVAL_ENABLE_MANUAL_ANSWER);

    RegCloseKey( hKey );

    return TRUE;
}

VOID
UnloadWinfax(
)
{
    if (hModWinfax) {
        FreeLibrary(hModWinfax);
    }

    hModWinfax                = NULL;

    pFaxConnectFaxServer      = NULL;
    pFaxClose                 = NULL;
    pFaxFreeBuffer            = NULL;
    pFaxInitializeEventQueue  = NULL;
    pFaxEnumPorts             = NULL;
    pFaxOpenPort              = NULL;
    pFaxGetPort               = NULL;
    pFaxSetPort               = NULL;
    pFaxGetDeviceStatus       = NULL;
    pFaxAbort                 = NULL;
    pFaxAccessCheck           = NULL;
}

BOOL
LoadWinfax(
)
{
    //
    // Load the winfax dll
    //
    if (!hModWinfax) {
        hModWinfax = LoadLibrary( L"winfax.dll" );
        if (hModWinfax == NULL) {
            return FALSE;
        }


        //
        // Get the addresses of the fax functions
        //
        pFaxConnectFaxServer      = (PFAXCONNECTFAXSERVER)      GetProcAddress(hModWinfax, "FaxConnectFaxServerW");
        pFaxClose                 = (PFAXCLOSE)                 GetProcAddress(hModWinfax, "FaxClose");
        pFaxFreeBuffer            = (PFAXFREEBUFFER)            GetProcAddress(hModWinfax, "FaxFreeBuffer");
        pFaxInitializeEventQueue  = (PFAXINITIALIZEEVENTQUEUE)  GetProcAddress(hModWinfax, "FaxInitializeEventQueue");
        pFaxEnumPorts             = (PFAXENUMPORTS)             GetProcAddress(hModWinfax, "FaxEnumPortsW");
        pFaxOpenPort              = (PFAXOPENPORT)              GetProcAddress(hModWinfax, "FaxOpenPort");
        pFaxGetPort               = (PFAXGETPORT)               GetProcAddress(hModWinfax, "FaxGetPortW");
        pFaxSetPort               = (PFAXSETPORT)               GetProcAddress(hModWinfax, "FaxSetPortW");
        pFaxGetDeviceStatus       = (PFAXGETDEVICESTATUS)       GetProcAddress(hModWinfax, "FaxGetDeviceStatusW");
        pFaxAbort                 = (PFAXABORT)                 GetProcAddress(hModWinfax, "FaxAbort");
        pFaxAccessCheck           = (PFAXACCESSCHECK)           GetProcAddress(hModWinfax, "FaxAccessCheck");
    }

    if ((pFaxConnectFaxServer == NULL) || (pFaxClose == NULL) || (pFaxFreeBuffer == NULL) ||
        (pFaxInitializeEventQueue == NULL) || (pFaxEnumPorts == NULL) || (pFaxOpenPort == NULL) ||
        (pFaxGetPort == NULL) || (pFaxSetPort == NULL) || (pFaxGetDeviceStatus == NULL) ||
        (pFaxAbort == NULL) || (pFaxAccessCheck == NULL))
    {
        UnloadWinfax();
        return FALSE;
    }

    return TRUE;
}

VOID
Disconnect(
)
{
    if (hFaxSvcHandle) {
        pFaxClose(hFaxSvcHandle);
        hFaxSvcHandle = NULL;
    }
}

BOOL
Connect(
)
{
    //
    // Ensure the winfax dll is loaded
    //
    if (!LoadWinfax()) {
        return FALSE;
    }

    //
    // Check if already connected to the fax service
    //
    if (hFaxSvcHandle) {
        return TRUE;
    }

    //
    // Connect to the fax service
    //
    if (!pFaxConnectFaxServer(NULL, &hFaxSvcHandle)) {
        return FALSE;
    }

    return TRUE;
}

DWORD
WaitForRestartThread(
   LPVOID  ThreadData
)
{
    //
    // Lock down the fax service while it is being used
    //
    WaitForSingleObject(hFaxStatMutex, INFINITE);

    UnloadWinfax();

    ReleaseMutex(hFaxStatMutex);

    //
    // Wait for event to be signaled, indicating fax service started
    //
    WaitForSingleObject(hFaxStartedEvent, INFINITE);

    SendMessage((HWND) ThreadData, WM_FAX_STARTED, 0, 0);

    return 0;
}

VOID
StartWaitForRestart(
    HWND  hWnd
)
{
    HANDLE  hThread;

    hThread = CreateThread(NULL, 0, WaitForRestartThread, (LPVOID) hWnd, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }

    return;
}

VOID
CenterWindow(
    HWND  hWndWindow,
    HWND  hWndParent
)
{
    RECT  rcWindow;
    RECT  rcParent;
    RECT  rcCenter;

    if (!hWndWindow) {
        return;
    }

    if (!hWndParent) {
        hWndParent = GetDesktopWindow();
    }

    GetWindowRect(hWndWindow, &rcWindow);
    GetWindowRect(hWndParent, &rcParent);

    //
    // Calculate the new Rect for hWndWindow
    //
    rcCenter.left = rcParent.left + (rcParent.right - rcParent.left + rcWindow.left - rcWindow.right) / 2;
    rcCenter.top = rcParent.top + (rcParent.bottom - rcParent.top + rcWindow.top - rcWindow.bottom) / 2;

    //
    // Position hWndWindow at the new Rect
    //
    SetWindowPos(hWndWindow, NULL, rcCenter.left, rcCenter.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

VOID
MyShowIcon(
    HWND    hWnd,
    HICON   hIcon,
    LPWSTR  szString
)
{
    static BOOL  bIsIconOnTaskBar = FALSE;

    if (hIcon) {
        IconData.hIcon = hIcon;
    }

    if (szString) {
        lstrcpyn(IconData.szTip, szString, sizeof(IconData.szTip) / sizeof(WCHAR));
    }

    if ((hIcon) && (ConfigOptions.TaskBar) && (!bIsIconOnTaskBar)) {
        bIsIconOnTaskBar = TRUE;
        Shell_NotifyIcon(NIM_ADD, &IconData);
    }
    else if ((ConfigOptions.TaskBar) && (bIsIconOnTaskBar)) {
        Shell_NotifyIcon(NIM_MODIFY, &IconData);
    }
    else if ((!ConfigOptions.TaskBar) && (bIsIconOnTaskBar)) {
        bIsIconOnTaskBar = FALSE;
        Shell_NotifyIcon(NIM_DELETE, &IconData);
    }
}

VOID
MyShowWindow(
    HWND  hWnd,
    INT   nCmd
)
{
    static BOOL  bVisible = FALSE;
    static HWND  hWndFaxAnimate = GetDlgItem(hWnd, IDC_FAX_ANIMATE);

    if ((nCmd == SW_SHOWNOACTIVATE) && (!bVisible)) {
        bVisible = TRUE;

        SetWindowPos(hWnd, ConfigOptions.OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

        Animate_Play(hWndFaxAnimate, 0, -1, -1);
        ShowWindow(hWnd, SW_SHOWNOACTIVATE);
    }
    else if (nCmd == SW_SHOW) {
        bVisible = TRUE;

        SetWindowPos(hWnd, ConfigOptions.OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

        Animate_Play(hWndFaxAnimate, 0, -1, -1);
        ShowWindow(hWnd, SW_SHOW);
    }
    else if (nCmd == SW_HIDE) {
        bVisible = FALSE;

        ShowWindow(hWnd, SW_HIDE);
        Animate_Stop(hWndFaxAnimate);

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
    }
    else if ((nCmd == -1) && (bVisible)) {
        SetWindowPos(hWnd, ConfigOptions.OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
    }
}

VOID
CALLBACK
FaxIconTimerProc(
    HWND   hWnd,
    UINT   iMsg,
    UINT   idEvent,
    DWORD  dwTime
)
{
    static WCHAR  szString[STRING_SIZE] = {'\0'};
    static BOOL   bToggle = FALSE;

    //
    // Toggle between the normal icon and the info icon, indicating a fax was received but not acknowledged
    //
    if (!(*szString)) {
        LoadString(g_hInstance, IDS_FAX_RCV_COMPLETE, szString, STRING_SIZE);
    }

    MyShowIcon(hWnd, bToggle ? hFaxIconInfo : hFaxIconNormal, szString);
    bToggle = !bToggle;
}

VOID
CALLBACK
FaxProgressTimerProc(
    HWND   hWnd,
    UINT   iMsg,
    UINT   idEvent,
    DWORD  dwTime
)
{
    static WCHAR  szFormat[STRING_SIZE] = {'\0'};
    WCHAR         szString[STRING_SIZE];

    //
    // Update the elapsed time text
    //
    if (!(*szFormat)) {
        LoadString(g_hInstance, IDS_FAX_ETIME, szFormat, STRING_SIZE);
    }

    dwSeconds++;

    wsprintf(szString, szFormat, dwSeconds / 60, szTimeSep, dwSeconds % 60);

    SetDlgItemText(hWnd, IDC_FAX_ETIME, szString);
}

VOID
PlayAnimation(
    HWND  hWnd,
    UINT  uAnimation
)
{
    Animate_Stop(hWnd);

    Animate_Close(hWnd);

    Animate_Open(hWnd, MAKEINTRESOURCE(uAnimation));

    Animate_Play(hWnd, 0, -1, -1);
}

// delay code added to pause the Fax stuff on first boot. Once the timer fires, we allow
// it to be checked whenever. The pause stops us calling the EnumPrinters APIs.
static BOOL g_bFaxOkToCheck = FALSE;

VOID Fax_StartupTimer( HWND hWnd )
{
    KillTimer( hWnd, FAX_STARTUP_TIMER_ID );
    g_bFaxOkToCheck = TRUE;
    Fax_CheckEnable( hWnd );
}

BOOL
Fax_CheckEnable(
    HWND hWnd
)
{
    HKEY                 hKey;
    DWORD                dwFaxInstalled;
    SECURITY_ATTRIBUTES  sa;
    SECURITY_DESCRIPTOR  sd;
    DWORD                cb;

    if ( ! g_bFaxOkToCheck )
        return FALSE;
        
    //
    // Do not initialize if called more than once.
    //
    if (hFaxStatMutex) {
       return TRUE;
    }

    //
    // Check if fax is installed
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    dwFaxInstalled = GetRegistryDword(hKey, REGVAL_FAXINSTALLED);
    RegCloseKey(hKey);

    if (!dwFaxInstalled) {
        return FALSE;
    }

    if (IsFaxPrinterInstalled() == FALSE) {
        return FALSE;
    }

    //
    // Create hFaxStartedEvent to indicate the fax service has started
    // This event is set by the fax service.  This event must be created with a NULL DACL so it
    // is accessible by fax service which is running in a different context
    //
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = &sd;

    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        return FALSE;
    }

    if(!SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE)) {
        return FALSE;
    }

    hFaxStartedEvent = CreateEvent(&sa, FALSE, FALSE, FAX_STARTED_EVENT_NAME);
    if (!hFaxStartedEvent) {
        return FALSE;
    }

    //
    // Create a mutex to synchronize access to sensitive info
    //
    hFaxStatMutex = CreateMutex(NULL, FALSE, NULL);
    if (!hFaxStatMutex) {
        return FALSE;
    }

    hFaxIconNormal = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FAX_NORMAL));
    hFaxIconInfo   = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FAX_INFO));

    ZeroMemory(szCurrentUser, sizeof(szCurrentUser));
    cb = STRING_SIZE;
    GetUserName(szCurrentUser, &cb);

    cb = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, NULL, 0);
    szTimeSep = (LPWSTR) MemAlloc((cb + 1) * sizeof(WCHAR));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szTimeSep, cb);

    InitCommonControls();

    //
    // Initialize and register the winclass and create the window
    //
    WNDCLASSEX  wndclass;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = FaxStatWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = g_hInstance;
    wndclass.hIcon          = hFaxIconNormal;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) (COLOR_INACTIVEBORDER + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = FAXSTAT_WINCLASS;
    wndclass.hIconSm        = hFaxIconNormal;

    RegisterClassEx(&wndclass);

    GetConfiguration();

    hWndFaxStat = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_FAX_STATUS), NULL, NULL);
    SendMessage(hWndFaxStat, WM_FAXSTAT_INITIALIZE, 0, 0);

    //
    // If the fax service is started, faxstat should start and initialize the event queue
    // If manual answer is started, faxstat should start to enable manual answer
    // Otherwise, wait for the fax service to start
    //
    if ((IsFaxSvcRunning()) || (ConfigOptions.ManualAnswerEnabled)) {
        SendMessage(hWndFaxStat, WM_FAX_STARTED, 0, 0);
    }
    else {
        StartWaitForRestart(hWndFaxStat);
    }

    return TRUE;
}

BOOL
Fax_MsgProcess(
    LPMSG  pMsg
)
{
    return (IsDialogMessage(hWndFaxStat, pMsg) || IsDialogMessage(hWndAnswerDlg, pMsg));
}

BOOL
IsDeviceEvent(
   DWORD  dwEventId
)
{
    switch (dwEventId) {
       case FEI_FAXSVC_ENDED:
       case FEI_JOB_QUEUED:
       case FEI_FAXSVC_STARTED:
           return FALSE;
    }

    return TRUE;
}

BOOL
IsBadDuplicateEvent(
    UINT  uResource,
    HWND  hWndDetailsList
)
{
    LV_ITEM  lvi;

    lvi.iItem = ListView_GetItemCount(hWndDetailsList);
    if (lvi.iItem == 0) {
        return FALSE;
    }

    lvi.iItem--;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    ListView_GetItem(hWndDetailsList, &lvi);

    if ((UINT) lvi.lParam == uResource) {
        switch (uResource) {
            case IDS_FAX_RINGING:
            case IDS_FAX_SENDING:
            case IDS_FAX_RECEIVING:
                break;

            default:
                return TRUE;
        }
    }

    return FALSE;
}

VOID
InsertEvent(
    HWND    hWnd,
    UINT    uResource,
    LPWSTR  szString
)
{
    static HWND  hWndDetailsList = GetDlgItem(hWnd, IDC_FAX_DETAILS_LIST);
    LV_ITEM      lvi;
    SYSTEMTIME   SystemTime;
    WCHAR        szTime[STRING_SIZE];

    //
    // Check if event is a duplicate event that should not be inserted again
    //
    if (IsBadDuplicateEvent(uResource, hWndDetailsList)) {
        return;
    }

    lvi.iItem = ListView_GetItemCount(hWndDetailsList);
    if (lvi.iItem == MAX_EVENTS) {
        ListView_DeleteItem(hWndDetailsList, 0);
        lvi.iItem--;
    }

    GetLocalTime(&SystemTime);
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, szTime, STRING_SIZE);

    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = szTime;
    lvi.lParam = uResource;
    lvi.iSubItem = 0;
    ListView_InsertItem(hWndDetailsList, &lvi);

    lvi.mask = LVIF_TEXT;
    lvi.pszText = szString;
    lvi.iSubItem = 1;
    ListView_SetItem(hWndDetailsList, &lvi);

    ListView_EnsureVisible(hWndDetailsList, lvi.iItem, FALSE);
}

BOOL
CheckIdleMessage(
    HWND  hWnd
)
{
    static HWND  hWndDetailsList = GetDlgItem(hWnd, IDC_FAX_DETAILS_LIST);
    UINT         uResource;
    WCHAR        szString[STRING_SIZE];

    //
    // Check if idle is the current state
    //
    if ((IsBadDuplicateEvent(IDS_FAX_IDLE, hWndDetailsList)) || (IsBadDuplicateEvent(IDS_FAX_IDLE_RECEIVE, hWndDetailsList))) {
        //
        // Set correct idle state
        //
        uResource = ((IsDlgButtonChecked(hWnd, IDC_ANSWER_NEXT_CALL)) || (dwFaxPortInfoFlags != (DWORD) -1)) ? IDS_FAX_IDLE_RECEIVE : IDS_FAX_IDLE;
        LoadString(g_hInstance, uResource, szString, STRING_SIZE);

        InsertEvent(hWnd, uResource, szString);
        SetDlgItemText(hWnd, IDC_FAX_STATUS, szString);
        MyShowIcon(hWnd, NULL, szString);

        return TRUE;
    }
    else {
        return FALSE;
    }
}

VOID
AnswerCall(
)
{
    HANDLE          hFaxPort = NULL;
    PFAX_PORT_INFO  pFaxPortInfo = NULL;

    //
    // Set the device's ring count to 1, so fax will answer call on next ring
    //
    if (Connect()) {
        if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
            if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                pFaxPortInfo->Rings = 1;
                pFaxSetPort(hFaxPort, pFaxPortInfo);

                pFaxFreeBuffer(pFaxPortInfo);
            }

            pFaxClose(hFaxPort);
        }

        Disconnect();
    }
}

INT_PTR
CALLBACK
AnswerDlgProc(
    HWND    hDlg,
    UINT    iMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch(iMsg) {
        case WM_INITDIALOG:
            CenterWindow(hDlg, hWndFaxStat);
            hWndAnswerDlg = hDlg;
            SetWindowPos(hWndFaxStat, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDYES:
                    //
                    // Lock down the fax service while it is being used
                    //
                    WaitForSingleObject(hFaxStatMutex, INFINITE);

                    AnswerCall();

                    ReleaseMutex(hFaxStatMutex);

                case IDNO:
                    EndDialog(hDlg, LOWORD(wParam));
                    hWndAnswerDlg = NULL;
                    SetWindowPos(hWndFaxStat, ConfigOptions.OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

DWORD
AnswerCallThread(
   LPVOID  ThreadData
)
{
    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FAX_ANSWER_CALL), (HWND) ThreadData, AnswerDlgProc);

    return 0;
}

VOID
StatusUpdate(
    HWND   hWnd,
    DWORD  dwEventId
)
{
    HANDLE              hFaxPort;
    PFAX_PORT_INFO      pFaxPortInfo;
    PFAX_DEVICE_STATUS  pFaxDeviceStatus;

    static HWND         hWndFaxAnimate = GetDlgItem(hWnd, IDC_FAX_ANIMATE);
    static HWND         hWndDetailsButton = GetDlgItem(hWnd, IDC_FAX_DETAILS);
    static HWND         hWndAnswerNextCallButton = GetDlgItem(hWnd, IDC_ANSWER_NEXT_CALL);
    static HWND         hWndEndFaxCallButton = GetDlgItem(hWnd, IDC_END_FAX_CALL);

    static BOOL         bRcvJob = FALSE;
    static BOOL         bFirstRing = TRUE;
    static BOOL         bFaxActive = FALSE;

    WCHAR               szPhoneNumber[STRING_SIZE];
    DWORD               dwCurrentPage;
    DWORD               dwTotalPages;
    WCHAR               szUserName[STRING_SIZE];
    WCHAR               szTsid[STRING_SIZE];

    UINT                uResource;
    WCHAR               szFormat[STRING_SIZE];
    WCHAR               szString[STRING_SIZE];

    //
    // Lock down the fax service while it is being used
    //
    WaitForSingleObject(hFaxStatMutex, INFINITE);

    //
    // Load the appropriate status string
    //
    switch (dwEventId) {
        case FEI_DIALING:
            uResource = IDS_FAX_DIALING;
            break;

        case FEI_SENDING:
            uResource = IDS_FAX_SENDING;
            break;

        case FEI_RECEIVING:
            uResource = IDS_FAX_RECEIVING;
            break;

        case FEI_COMPLETED:
            uResource = IDS_FAX_COMPLETED;
            break;

        case FEI_BUSY:
            uResource = IDS_FAX_BUSY;
            break;

        case FEI_NO_ANSWER:
            uResource = IDS_FAX_NO_ANSWER;
            break;

        case FEI_BAD_ADDRESS:
            uResource = IDS_FAX_BAD_ADDRESS;
            break;

        case FEI_NO_DIAL_TONE:
            uResource = IDS_FAX_NO_DIAL_TONE;
            break;

        case FEI_DISCONNECTED:
            uResource = IDS_FAX_DISCONNECTED;
            break;

        case FEI_FATAL_ERROR:
            uResource = bRcvJob ? IDS_FAX_FATAL_ERROR_RCV : IDS_FAX_FATAL_ERROR_SND;
            break;

        case FEI_NOT_FAX_CALL:
            uResource = IDS_FAX_NOT_FAX_CALL;
            break;

        case FEI_CALL_DELAYED:
            uResource = IDS_FAX_CALL_DELAYED;
            break;

        case FEI_CALL_BLACKLISTED:
            uResource = IDS_FAX_CALL_BLACKLISTED;
            break;

        case FEI_RINGING:
            uResource = IDS_FAX_RINGING;
            break;

        case FEI_ABORTING:
            uResource = IDS_FAX_ABORTING;
            break;

        case FEI_MODEM_POWERED_ON:
        case FEI_IDLE:
        case FEI_FAXSVC_STARTED:
            uResource = ((IsDlgButtonChecked(hWnd, IDC_ANSWER_NEXT_CALL)) || (dwFaxPortInfoFlags != (DWORD) -1)) ? IDS_FAX_IDLE_RECEIVE : IDS_FAX_IDLE;
            break;

        case FEI_MODEM_POWERED_OFF:
            uResource = IDS_FAX_MODEM_POWERED_OFF;
            break;

        case FEI_FAXSVC_ENDED:
            uResource = IDS_FAX_IDLE;
            break;

        case FEI_ANSWERED:
            uResource = IDS_FAX_ANSWERED;
            break;

        default:
            uResource = IDS_FAX_IDLE;
            break;
    }

    LoadString(g_hInstance, uResource, szFormat, STRING_SIZE);

    //
    // Fax completed and is a receive job, so set the icon to so indicate
    //
    if ((dwEventId == FEI_COMPLETED) && (bRcvJob)) {
        bFaxRcvAck = FALSE;
        SetTimer(hWnd, ID_FAX_ICON_TIMER, 1000, (TIMERPROC) FaxIconTimerProc);
    }

    if ((dwEventId == FEI_DIALING) || (dwEventId == FEI_SENDING) || (dwEventId == FEI_RECEIVING)) {
        //
        // The device is dialing, sending, or receiving, so need to get more info about the job
        //
        ZeroMemory(szPhoneNumber, sizeof(szPhoneNumber));
        dwCurrentPage = 0;
        dwTotalPages = 0;
        ZeroMemory(szUserName, sizeof(szUserName));
        ZeroMemory(szTsid, sizeof(szTsid));

        if (Connect()) {
            if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_QUERY, &hFaxPort)) {
                if (pFaxGetDeviceStatus(hFaxPort, &pFaxDeviceStatus)) {
                    if (pFaxDeviceStatus->PhoneNumber) {
                        lstrcpyn(szPhoneNumber, pFaxDeviceStatus->PhoneNumber, STRING_SIZE);
                    }

                    dwCurrentPage = pFaxDeviceStatus->CurrentPage;
                    dwTotalPages = pFaxDeviceStatus->TotalPages;

                    if (pFaxDeviceStatus->UserName) {
                        lstrcpyn(szUserName, pFaxDeviceStatus->UserName, STRING_SIZE);
                    }

                    if (pFaxDeviceStatus->Tsid) {
                        lstrcpyn(szTsid, pFaxDeviceStatus->Tsid, STRING_SIZE);
                    }

                    pFaxFreeBuffer(pFaxDeviceStatus);
                }

                pFaxClose(hFaxPort);
            }

            Disconnect();
        }

        //
        // Update the status string with the additional info
        //
        if (dwEventId == FEI_DIALING) {
            bRcvJob = FALSE;
            wsprintf(szString, szFormat, szPhoneNumber);
        }
        else if (dwEventId == FEI_SENDING) {
            bRcvJob = FALSE;
            wsprintf(szString, szFormat, dwCurrentPage, dwTotalPages);
        }
        else if (dwEventId == FEI_RECEIVING) {
            bRcvJob = TRUE;
            wsprintf(szString, szFormat, dwCurrentPage);
        }

        if ((!ConfigOptions.UserHasAccess) && (lstrcmpi(szCurrentUser, szUserName)) && (GetFocus() == hWndEndFaxCallButton)) {
            //
            // Set the focus to the details button
            //
            SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
        }

        EnableWindow(hWndEndFaxCallButton, (ConfigOptions.UserHasAccess || (!lstrcmpi(szCurrentUser, szUserName))));
    }
    else {
        if (dwEventId == FEI_ANSWERED) {
            bRcvJob = TRUE;
        }
        else {
            bRcvJob = FALSE;
        }

        lstrcpy(szString, szFormat);

        if (GetFocus() == hWndEndFaxCallButton) {
            //
            // Set the focus to the details button
            //
            SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
        }

        EnableWindow(hWndEndFaxCallButton, FALSE);
    }

    //
    // Insert the event text, set the status text, and update the icon text
    //
    InsertEvent(hWnd, uResource, szString);
    SetDlgItemText(hWnd, IDC_FAX_STATUS, szString);
    MyShowIcon(hWnd, NULL, szString);

    //
    // Job is aborting, so break
    //
    if (dwEventId == FEI_ABORTING) {
        ReleaseMutex(hFaxStatMutex);
        return;
    }

    if ((dwEventId == FEI_RINGING) && (dwFaxPortInfoFlags != (DWORD) -1) && (bFirstRing)) {
        //
        // Manual answer is enabled, so need to determine what to do with the call
        //
        if (IsDlgButtonChecked(hWnd, IDC_ANSWER_NEXT_CALL)) {
            //
            // Answer next call is checked, so immediately answer call
            //
            AnswerCall();
        }
        else {
            HANDLE  hThread;

            //
            // Answer next call is not checked, so display answer call dialog
            //
            MyShowWindow(hWnd, SW_SHOWNOACTIVATE);
            hThread = CreateThread(NULL, 0, AnswerCallThread, (LPVOID) hWnd, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
            }
        }
    }

    if ((dwEventId == FEI_DIALING) || ((dwEventId == FEI_RINGING) && (bFirstRing))) {
        bFirstRing = FALSE;

        //
        // Device is dialing or ringing and first ring, so display window and icon, if necessary
        //
        if (ConfigOptions.TaskBar) {
            MyShowIcon(hWnd, bFaxRcvAck ? hFaxIconNormal : hFaxIconInfo, NULL);
        }

        if (ConfigOptions.VisualNotification) {
            MyShowWindow(hWnd, SW_SHOWNOACTIVATE);
        }
    }

    if (ConfigOptions.SoundNotification) {
        //
        // Device is dialing or ringing, so play sound, if necessary
        //
        if (dwEventId == FEI_DIALING) {
            PlaySound(L"Outgoing-Fax", NULL, SND_ASYNC | SND_APPLICATION);
        }
        else if (dwEventId == FEI_RINGING) {
            PlaySound(L"Incoming-Fax", NULL, SND_ASYNC | SND_APPLICATION);
        }
    }

    if ((dwEventId != FEI_RINGING) && (hWndAnswerDlg)) {
        //
        // Device is no longer ringing, so destroy answer call dialog
        //
        SendMessage(GetDlgItem(hWndAnswerDlg, IDNO), BM_CLICK, 0, 0);
    }

    if ((dwEventId == FEI_SENDING) || (dwEventId == FEI_RECEIVING)) {
        if (!bFaxActive) {
            PlayAnimation(hWndFaxAnimate, (dwEventId == FEI_SENDING) ? IDR_FAX_SEND : IDR_FAX_RECEIVE);

            //
            // Display the sender / receiver
            //
            LoadString(g_hInstance, (dwEventId == FEI_SENDING) ? IDS_FAX_TO : IDS_FAX_FROM, szFormat, STRING_SIZE);
            wsprintf(szString, szFormat, szTsid);
            SetDlgItemText(hWnd, IDC_FAX_FROMTO, szString);

            //
            // Display the elapsed time
            //
            LoadString(g_hInstance, IDS_FAX_ETIME, szFormat, STRING_SIZE);
            wsprintf(szString, szFormat, 0, szTimeSep, 0);
            SetDlgItemText(hWnd, IDC_FAX_ETIME, szString);

            bFaxActive = TRUE;

            dwSeconds = 0;
            SetTimer(hWnd, ID_FAX_PROGRESS_TIMER, 1000, (TIMERPROC) FaxProgressTimerProc);
        }
    }
    else {
        //
        // Reset the dialog
        //
        SetDlgItemText(hWnd, IDC_FAX_FROMTO, L"");
        SetDlgItemText(hWnd, IDC_FAX_ETIME, L"");

        if (bFaxActive) {
            PlayAnimation(hWndFaxAnimate, IDR_FAX_IDLE);
            KillTimer(hWnd, ID_FAX_PROGRESS_TIMER);
            bFaxActive = FALSE;
        }

        if (dwEventId == FEI_IDLE) {
            BOOL  bRetry = TRUE;

            bFirstRing = TRUE;

            if ((ConfigOptions.ManualAnswerEnabled) || (dwFaxPortInfoFlags != (DWORD) -1)) {
                //
                // Ensure manual answer is set correctly
                //
                if (Connect()) {
                    if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                        if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                            if ((ConfigOptions.ManualAnswerEnabled) && (dwFaxPortInfoFlags != (DWORD) -1)) {
                                //
                                // Reset manual answer
                                //
                                pFaxPortInfo->Rings = 99;

                                pFaxSetPort(hFaxPort, pFaxPortInfo);

                                CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                            }
                            else if (ConfigOptions.ManualAnswerEnabled) {
                                //
                                // Manual answer has not yet been enabled, so try now
                                //
                                dwFaxPortInfoFlags = pFaxPortInfo->Flags;
                                dwFaxPortInfoRings = pFaxPortInfo->Rings;

                                pFaxPortInfo->Flags |= FPF_RECEIVE;
                                pFaxPortInfo->Rings = 99;

RetryEnable:
                                if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                                    EnableWindow(hWndAnswerNextCallButton, TRUE);
                                }
                                else if ((bRetry) && (GetLastError() == ERROR_DEVICE_IN_USE)) {
                                    Sleep(1000);
                                    bRetry = FALSE;
                                    goto RetryEnable;
                                }
                                else {
                                    dwFaxPortInfoFlags = (DWORD) -1;
                                    dwFaxPortInfoRings = (DWORD) -1;
                                }
                            }
                            else if (dwFaxPortInfoFlags != (DWORD) -1) {
                                //
                                // Manual answer was turned off
                                //
                                pFaxPortInfo->Flags = dwFaxPortInfoFlags;
                                pFaxPortInfo->Rings = dwFaxPortInfoRings;

RetryDisable:
                                if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, (dwFaxPortInfoFlags & FPF_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);

                                    if (GetFocus() == hWndAnswerNextCallButton) {
                                        //
                                        // Set the focus to the details button
                                        //
                                        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
                                    }

                                    EnableWindow(hWndAnswerNextCallButton, FALSE);

                                    dwFaxPortInfoFlags = (DWORD) -1;
                                    dwFaxPortInfoRings = (DWORD) -1;
                                }
                                else if ((bRetry) && (GetLastError() == ERROR_DEVICE_IN_USE)) {
                                    Sleep(1000);
                                    bRetry = FALSE;
                                    goto RetryDisable;
                                }
                            }

                            pFaxFreeBuffer(pFaxPortInfo);
                        }

                        pFaxClose(hFaxPort);
                    }

                    Disconnect();
                }

                CheckIdleMessage(hWnd);
            }
        }
    }

    ReleaseMutex(hFaxStatMutex);
}

BOOL
DoFaxContextMenu(
    HWND  hWnd
)
{
    HMENU      hMenu;
    UINT       iMenuIndex;
    POINT      pt;
    WCHAR      szString[STRING_SIZE];
    WCHAR      szPath[MAX_PATH];
    WCHAR      szResourcePath[MAX_PATH];
    WCHAR      szMyFaxes[MAX_PATH];
    HINSTANCE  hResource;

    //
    // Create a context menu
    //
    hMenu = CreatePopupMenu();
    if (!hMenu) {
        return FALSE;
    }

    //
    // Set the foreground window
    //
    SetForegroundWindow(hWnd);

    LoadString(g_hInstance, IDS_FAX_MENU_CFG, szString, STRING_SIZE);
    AppendMenu(hMenu, MF_STRING,IDS_FAX_MENU_CFG, szString);

    LoadString(g_hInstance, IDS_FAX_MENU_QUEUE, szString, STRING_SIZE);
    AppendMenu(hMenu, MF_STRING,IDS_FAX_MENU_QUEUE, szString);

    LoadString(g_hInstance, IDS_FAX_MENU_FOLDER, szString, STRING_SIZE);
    AppendMenu(hMenu, MF_STRING,IDS_FAX_MENU_FOLDER, szString);

    GetCursorPos(&pt);

    iMenuIndex = TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);

    switch (iMenuIndex) {
        case IDS_FAX_MENU_QUEUE:
            //
            // Launch the fax queue
            //
            ShellExecute(hWnd, NULL, L"faxqueue", NULL, L".", SW_SHOWNORMAL);
            break;

        case IDS_FAX_MENU_CFG:
            //
            // Launch the fax control panel applet
            //
            ShellExecute(hWnd, L"open", L"rundll32", L"shell32.dll,Control_RunDLL fax.cpl,,2", L".", SW_SHOWNORMAL);
            break;

        case IDS_FAX_MENU_FOLDER:
            //
            // Open the My Faxes folder
            //
            SHGetFolderPathW(hWnd, CSIDL_COMMON_DOCUMENTS, NULL, 0, szPath);
            if (*szPath == 0) {
                break;
            }

            ExpandEnvironmentStrings(L"%SystemRoot%\\system32\\faxocm.dll", szResourcePath, sizeof(szResourcePath) / sizeof(WCHAR));
            hResource = LoadLibrary(szResourcePath);
            if (!hResource) {
                break;
            }

            if (!MyLoadString(hResource, 615, szMyFaxes, sizeof(szMyFaxes) / sizeof(WCHAR), GetSystemDefaultUILanguage())) {
                FreeLibrary(hResource);
                break;
            }

            FreeLibrary(hResource);

            lstrcat(szPath, L"\\");
            lstrcat(szPath, szMyFaxes);
            ShellExecute(hWnd, L"open", L"explorer", szPath, L".", SW_SHOWNORMAL);

            break;
    }

    DestroyMenu(hMenu);

    return TRUE;
}

LRESULT
CALLBACK
FaxStatWndProc(
    HWND    hWnd,
    UINT    iMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    static HWND   hWndDetailsButton;
    static HWND   hWndDetailsList;
    static HWND   hWndAnswerNextCallButton;
    static HWND   hWndEndFaxCallButton;

    LV_COLUMN     lvc;
    LV_ITEM       lvi;
    RECT          rcClient;
    WCHAR         szString[STRING_SIZE];

    static RECT   rcExpand;
    static RECT   rcCollapse;
    static WCHAR  szDetailsExpand[STRING_SIZE];
    static WCHAR  szDetailsCollapse[STRING_SIZE];
    static BOOL   bDetailsExpanded;

    DWORD         dwEventId;
    DWORD         dwDeviceId;
    static DWORD  dwJobId = 0;

    if ((iMsg >= WM_FAX_EVENT) && (iMsg <= WM_FAX_EVENT + FEI_NEVENTS)) {
        dwEventId = (DWORD) (iMsg - WM_FAX_EVENT);
        dwDeviceId = (DWORD) wParam;

        //
        // If this event is for a different fax device, ignore it.
        //
        if ((IsDeviceEvent(dwEventId)) && (dwDeviceId != dwFaxPortDeviceId)) {
            return 0;
        }
        else {
            dwJobId = (DWORD) lParam;
        }

        switch(dwEventId) {
            case FEI_ROUTING:
            case FEI_JOB_QUEUED:
                break;

            case FEI_FAXSVC_ENDED:
                //
                // The fax service is stopping, so disable the dialog and wait for the fax service to start
                //
                CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                EnableWindow(hWndAnswerNextCallButton, FALSE);
                ResetEvent(hFaxStartedEvent);
                StartWaitForRestart(hWnd);
            default:
                StatusUpdate(hWnd, dwEventId);
                break;
        }

        return 0;
    }

    switch (iMsg) {
        case WM_FAXSTAT_INITIALIZE:
            //
            // Initialize the icon data
            //
            IconData.cbSize             = sizeof(NOTIFYICONDATA);
            IconData.hWnd               = hWnd;
            IconData.uID                = (UINT) IDI_FAX_NORMAL;
            IconData.uFlags             = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            IconData.uCallbackMessage   = WM_TRAYCALLBACK;

            //
            // Center the window and get its expanded size
            //
            CenterWindow(hWnd, GetDesktopWindow());
            GetWindowRect(hWnd, &rcExpand);

            hWndDetailsButton = GetDlgItem(hWnd, IDC_FAX_DETAILS);
            hWndDetailsList = GetDlgItem(hWnd, IDC_FAX_DETAILS_LIST);
            hWndAnswerNextCallButton = GetDlgItem(hWnd, IDC_ANSWER_NEXT_CALL);
            hWndEndFaxCallButton = GetDlgItem(hWnd, IDC_END_FAX_CALL);

            //
            // Compute the collapsed size of the window and load the text for the details button
            //
            GetWindowRect(hWndDetailsList, &rcClient);
            rcCollapse.left = rcExpand.left;
            rcCollapse.top = rcExpand.top;
            rcCollapse.right = rcExpand.right;
            rcCollapse.bottom = rcExpand.bottom - (rcClient.bottom - rcClient.top) - ((HIWORD(GetDialogBaseUnits()) * 5) / 8);

            LoadString(g_hInstance, IDS_FAX_DETAILS_EXPAND, szDetailsExpand, STRING_SIZE);
            LoadString(g_hInstance, IDS_FAX_DETAILS_COLLAPSE, szDetailsCollapse, STRING_SIZE);
            SetDlgItemText(hWnd, IDC_FAX_DETAILS, szDetailsCollapse);
            bDetailsExpanded = TRUE;

            //
            // Click the details button to collapse the dialog
            //
            SendMessage(hWndDetailsButton, BM_CLICK, 0, 0);

            //
            // Initialize the details list
            //
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;

            LoadString(g_hInstance, IDS_FAX_TIMELABEL, szString, STRING_SIZE);
            lvc.pszText = szString;
            //
            // Set the width of the column to be about 3 / 10 of the width of the details list, allowing for the width of the borders and scroll bar
            //
            lvc.cx = ((rcClient.right - rcClient.left - 4 * GetSystemMetrics(SM_CXBORDER) - GetSystemMetrics(SM_CXVSCROLL)) * 3) / 10;
            lvc.iSubItem = 0;
            ListView_InsertColumn(hWndDetailsList, 0, &lvc );

            LoadString(g_hInstance, IDS_FAX_EVENTLABEL, szString, STRING_SIZE);
            lvc.pszText = szString;
            //
            // Set the width of the column to be about 7 / 10 of the width of the details list, allowing for the width of the borders and scroll bar
            //
            lvc.cx = (lvc.cx / 3) * 7;
            lvc.iSubItem = 1;
            ListView_InsertColumn(hWndDetailsList, 1, &lvc );

            EnableWindow(hWndAnswerNextCallButton, FALSE);
            EnableWindow(hWndEndFaxCallButton, FALSE);

            PlayAnimation(GetDlgItem(hWnd, IDC_FAX_ANIMATE), IDR_FAX_IDLE);

            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

            return TRUE;

        case WM_SETFOCUS:
            //
            // Verify correct control has the focus
            //
            if ((!IsWindowEnabled(hWndEndFaxCallButton)) && (SendMessage(hWndEndFaxCallButton, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)) {
                //
                // Set the focus to the details button
                //
                SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
            }

            break;

        case WM_CLOSE:
            MyShowWindow(hWnd, SW_HIDE);
            return 0;

        case WM_FAXSTAT_CONTROLPANEL:
            PFAX_PORT_INFO  pFaxPortInfo;
            DWORD           dwDevices;
            HANDLE          hFaxPort;

            //
            // Lock down the fax service while it is being used
            //
            WaitForSingleObject(hFaxStatMutex, INFINITE);

            if ((((BOOL) wParam) && (dwFaxPortInfoFlags == (DWORD) -1)) || ((!(BOOL) wParam) && (dwFaxPortInfoFlags != (DWORD) -1))) {
                if (Connect()) {
                    if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                        if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                            if ((BOOL) wParam) {
                                //
                                // Manual answer was turned on
                                //
                                dwFaxPortInfoFlags = pFaxPortInfo->Flags;
                                dwFaxPortInfoRings = pFaxPortInfo->Rings;

                                pFaxPortInfo->Flags |= FPF_RECEIVE;
                                pFaxPortInfo->Rings = 99;

                                if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                                    EnableWindow(hWndAnswerNextCallButton, TRUE);
                                }
                                else {
                                    dwFaxPortInfoFlags = (DWORD) -1;
                                    dwFaxPortInfoRings = (DWORD) -1;
                                }
                            }
                            else {
                                //
                                // Manual answer was turned off
                                //
                                pFaxPortInfo->Flags = dwFaxPortInfoFlags;
                                pFaxPortInfo->Rings = dwFaxPortInfoRings;

                                if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, (dwFaxPortInfoFlags & FPF_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);

                                    if (GetFocus() == hWndAnswerNextCallButton) {
                                        //
                                        // Set the focus to the details button
                                        //
                                        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
                                    }

                                    EnableWindow(hWndAnswerNextCallButton, FALSE);

                                    dwFaxPortInfoFlags = (DWORD) -1;
                                    dwFaxPortInfoRings = (DWORD) -1;
                                }
                            }

                            pFaxFreeBuffer(pFaxPortInfo);
                        }

                        pFaxClose(hFaxPort);
                    }

                    Disconnect();
                }

                CheckIdleMessage(hWnd);
            }

            GetConfiguration();

            MyShowWindow(hWnd, -1);
            MyShowIcon(hWnd, NULL, NULL);

            ReleaseMutex(hFaxStatMutex);

            return 0;

        case WM_FAXSTAT_MMC:
            BOOL  bManualAnswerEnabled;

            bManualAnswerEnabled = FALSE;

            //
            // Lock down the fax service while it is being used
            //
            WaitForSingleObject(hFaxStatMutex, INFINITE);

            if (Connect()) {
                ConfigOptions.UserHasAccess = pFaxAccessCheck(hFaxSvcHandle, FAX_JOB_MANAGE);

                //
                // Update the current device
                //
                if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                    if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                        if (ConfigOptions.ManualAnswerEnabled) {
                            //
                            // Manual answer is enabled, so the device should be checked for manual answer
                            //
                            if (dwFaxPortInfoFlags == (DWORD) -1) {
                                //
                                // Manual answer has not yet been enabled, so try now
                                //
                                dwFaxPortInfoFlags = pFaxPortInfo->Flags;
                                dwFaxPortInfoRings = pFaxPortInfo->Rings;

                                pFaxPortInfo->Flags |= FPF_RECEIVE;
                                pFaxPortInfo->Rings = 99;

                                if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                                    EnableWindow(hWndAnswerNextCallButton, TRUE);
                                
                                    bManualAnswerEnabled = TRUE;
                                }
                                else {
                                    dwFaxPortInfoFlags = (DWORD) -1;
                                    dwFaxPortInfoRings = (DWORD) -1;
                                }
                            }
                            else if ((pFaxPortInfo->Flags != (dwFaxPortInfoFlags | FPF_RECEIVE)) || (pFaxPortInfo->Rings != 99)) {
                                //
                                // Device settings were changed, so manual answer needs to be reset
                                //
                                if (pFaxPortInfo->Flags != (dwFaxPortInfoFlags | FPF_RECEIVE)) {
                                    dwFaxPortInfoFlags = pFaxPortInfo->Flags;
                                }

                                if (pFaxPortInfo->Rings != 99) {
                                    dwFaxPortInfoRings = pFaxPortInfo->Rings;
                                }

                                if (!(pFaxPortInfo->Flags & FPF_RECEIVE) || (pFaxPortInfo->Rings != 99)) {
                                    bManualAnswerEnabled = TRUE;
                                }

                                pFaxPortInfo->Flags |= FPF_RECEIVE;
                                pFaxPortInfo->Rings = 99;

                                pFaxSetPort(hFaxPort, pFaxPortInfo);
                            }
                        }
                        else {
                            //
                            // Device settings may have changed, so IDC_ANSWER_NEXT_CALL needs to be reset
                            //
                            CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, (pFaxPortInfo->Flags & FPF_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
                        }

                        pFaxFreeBuffer(pFaxPortInfo);
                    }

                    pFaxClose(hFaxPort);
                }

                Disconnect();
            }

            CheckIdleMessage(hWnd);

            ReleaseMutex(hFaxStatMutex);

            return ((dwFaxPortDeviceId == (DWORD) wParam) && (bManualAnswerEnabled));

        case WM_FAX_STARTED:
            DWORD  dwOldFaxPortDeviceId;
            BOOL   bRcvEnabled;

            //
            // Lock down the fax service while it is being used
            //
            WaitForSingleObject(hFaxStatMutex, INFINITE);

            if (!Connect()) {
                ReleaseMutex(hFaxStatMutex);
                return 0;
            }

            //
            // Get the user's job manage access
            //
            ConfigOptions.UserHasAccess = pFaxAccessCheck(hFaxSvcHandle, FAX_JOB_MANAGE);

            dwOldFaxPortDeviceId = dwFaxPortDeviceId;

            //
            // Get the first device
            //
            if (pFaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwDevices)) {
                dwFaxPortDeviceId = pFaxPortInfo[0].DeviceId;
                bRcvEnabled = pFaxPortInfo[0].Flags & FPF_RECEIVE;
                pFaxFreeBuffer(pFaxPortInfo);
            }
            else {
                dwFaxPortDeviceId = dwOldFaxPortDeviceId;
                bRcvEnabled = FALSE;
            }

            if ((dwFaxPortInfoFlags != (DWORD) -1) && (dwOldFaxPortDeviceId != dwFaxPortDeviceId)) {
                //
                // The device settings are set meaning the device is probably still set for manual answer, so use that device
                //
                dwFaxPortDeviceId = dwOldFaxPortDeviceId;
            }

            if (!ConfigOptions.ManualAnswerEnabled) {
                if (dwFaxPortInfoFlags != (DWORD) -1) {
                    //
                    // Manual answer was disabled, so the device should not be set for manual answer
                    //
                    if (pFaxOpenPort(hFaxSvcHandle, dwOldFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                        if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                            pFaxPortInfo->Flags = dwFaxPortInfoFlags;
                            pFaxPortInfo->Rings = dwFaxPortInfoRings;

                            if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                                CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, (dwFaxPortInfoFlags & FPF_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);

                                if (GetFocus() == hWndAnswerNextCallButton) {
                                    //
                                    // Set the focus to the details button
                                    //
                                    SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
                                }

                                EnableWindow(hWndAnswerNextCallButton, FALSE);

                                dwFaxPortInfoFlags = (DWORD) -1;
                                dwFaxPortInfoRings = (DWORD) -1;
                            }

                            pFaxFreeBuffer(pFaxPortInfo);
                        }

                        pFaxClose(hFaxPort);
                    }
                }
                else {
                    CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, bRcvEnabled ? BST_CHECKED : BST_UNCHECKED);
                }
            }

            if ((ConfigOptions.ManualAnswerEnabled) && (dwFaxPortInfoFlags == (DWORD) -1)) {
                //
                // Manual answer is enabled, so the device should be set for manual answer
                //
                if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                    if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                        dwFaxPortInfoFlags = pFaxPortInfo->Flags;
                        dwFaxPortInfoRings = pFaxPortInfo->Rings;

                        pFaxPortInfo->Flags |= FPF_RECEIVE;
                        pFaxPortInfo->Rings = 99;

                        if (pFaxSetPort(hFaxPort, pFaxPortInfo)) {
                            CheckDlgButton(hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED);
                            EnableWindow(hWndAnswerNextCallButton, TRUE);
                        }
                        else {
                            dwFaxPortInfoFlags = (DWORD) -1;
                            dwFaxPortInfoRings = (DWORD) -1;
                        }

                        pFaxFreeBuffer(pFaxPortInfo);
                    }

                    pFaxClose(hFaxPort);
                }
            }

            pFaxInitializeEventQueue(hFaxSvcHandle, NULL, 0, hWndFaxStat, WM_FAX_EVENT);

            Disconnect();

            ReleaseMutex(hFaxStatMutex);

            return 0;

        case WM_TRAYCALLBACK:
            switch (lParam) {
                case WM_LBUTTONDOWN:
                    MyShowWindow(hWnd, SW_SHOW);
                    if (hWndAnswerDlg) {
                        SetForegroundWindow(hWndAnswerDlg);
                    }
                    else {
                        SetForegroundWindow(hWnd);
                    }

                    if (!bFaxRcvAck) {
                        KillTimer(hWnd, ID_FAX_ICON_TIMER);
                        bFaxRcvAck = TRUE;
                        SendMessage(hWnd, WM_FAX_VIEW, 0, 0);

                        lvi.iItem = ListView_GetItemCount(hWndDetailsList);
                        if (lvi.iItem != 0) {
                            lvi.iItem--;

                            lvi.mask = LVIF_TEXT;
                            lvi.pszText = szString;
                            lvi.cchTextMax = sizeof(szString);
                            lvi.iSubItem = 1;
                            ListView_GetItem(hWndDetailsList, &lvi);

                            MyShowIcon(hWnd, hFaxIconNormal, szString);
                        }
                        else {
                            MyShowIcon(hWnd, hFaxIconNormal, L"");
                        }
                    }
                    return 0;

                case WM_RBUTTONUP:
                    DoFaxContextMenu(hWnd);
                    return 0;
            }
            break;

        case WM_FAX_VIEW:
            WCHAR      szPath[MAX_PATH];
            WCHAR      szResourcePath[MAX_PATH];
            WCHAR      szMyFaxes[MAX_PATH];
            HINSTANCE  hResource;

            //
            // Open the My Faxes folder
            //
            SHGetFolderPathW(hWnd, CSIDL_COMMON_DOCUMENTS, NULL, 0, szPath);
            if (*szPath == 0) {
                return 0;
            }

            ExpandEnvironmentStrings(L"%SystemRoot%\\system32\\faxocm.dll", szResourcePath, sizeof(szResourcePath) / sizeof(WCHAR));
            hResource = LoadLibrary(szResourcePath);
            if (!hResource) {
                return 0;
            }

            if (!MyLoadString(hResource, 615, szMyFaxes, sizeof(szMyFaxes) / sizeof(WCHAR), GetSystemDefaultUILanguage())) {
                FreeLibrary(hResource);
                return 0;
            }

            FreeLibrary(hResource);

            lstrcat(szPath, L"\\");
            lstrcat(szPath, szMyFaxes);
            ShellExecute(hWnd, L"open", L"explorer", szPath, L".", SW_SHOWNORMAL);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_FAX_DETAILS:
                    //
                    // Collapse or expand the window
                    //
                    if (bDetailsExpanded) {
                        if (GetFocus() == hWndDetailsList) {
                            SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
                        }
                        SetDlgItemText(hWnd, IDC_FAX_DETAILS, szDetailsExpand);
                        EnableWindow(hWndDetailsList, FALSE);
                        SetWindowPos(hWnd, NULL, 0, 0, rcCollapse.right - rcCollapse.left, rcCollapse.bottom - rcCollapse.top, SWP_NOMOVE | SWP_NOZORDER);
                    }
                    else {
                        SetDlgItemText(hWnd, IDC_FAX_DETAILS, szDetailsCollapse);
                        EnableWindow(hWndDetailsList, TRUE);
                        SetWindowPos(hWnd, NULL, 0, 0, rcExpand.right - rcExpand.left, rcExpand.bottom - rcExpand.top, SWP_NOMOVE | SWP_NOZORDER);
                    }
                    bDetailsExpanded = !bDetailsExpanded;

                    break;

                case IDC_END_FAX_CALL:
                    //
                    // Lock down the fax service while it is being used
                    //
                    WaitForSingleObject(hFaxStatMutex, INFINITE);

                    if (Connect()) {
                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                        if (!pFaxAbort(hFaxSvcHandle, dwJobId)) {
                            SetCursor(LoadCursor(NULL, IDC_ARROW));
                        }
                        else {
                            SetCursor(LoadCursor(NULL, IDC_ARROW));

                            if (GetFocus() == hWndEndFaxCallButton) {
                                //
                                // Set the focus to the details button
                                //
                                SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) hWndDetailsButton, MAKELONG(TRUE, 0));
                            }

                            EnableWindow(hWndEndFaxCallButton, FALSE);
                        }

                        Disconnect();
                    }

                    ReleaseMutex(hFaxStatMutex);

                    break;
            }

            break;

        case WM_QUERYENDSESSION:
            //
            // Lock down the fax service while it is being used
            //
            WaitForSingleObject(hFaxStatMutex, INFINITE);

            if (dwFaxPortInfoFlags != (DWORD) -1) {
                if (Connect()) {
                    if (pFaxOpenPort(hFaxSvcHandle, dwFaxPortDeviceId, PORT_OPEN_MODIFY, &hFaxPort)) {
                        if (pFaxGetPort(hFaxPort, &pFaxPortInfo)) {
                            pFaxPortInfo->Flags = dwFaxPortInfoFlags;
                            pFaxPortInfo->Rings = dwFaxPortInfoRings;

                            pFaxSetPort(hFaxPort, pFaxPortInfo);

                            pFaxFreeBuffer(pFaxPortInfo);
                        }

                        pFaxClose(hFaxPort);
                    }

                    Disconnect();
                }
            }

            ReleaseMutex(hFaxStatMutex);

            break;

        case WM_ENDSESSION:
            //
            // Lock down the fax service while it is being used
            //
            WaitForSingleObject(hFaxStatMutex, INFINITE);

            if (IsFaxSvcRunning()) {
                //
                // Fax service may have an open impersonation token to this desktop and it must be
                // closed in order for this user to log off.  This is accomplished by calling
                // FaxInitializeEventQueue() with CompletionPort set to -1
                //

                if (Connect()) {
                    pFaxInitializeEventQueue(hFaxSvcHandle, NULL, (ULONG_PTR) -1, hWnd, WM_FAX_EVENT);

                    Disconnect();
                }
            }

            ReleaseMutex(hFaxStatMutex);
            CloseHandle(hFaxStatMutex);

            CloseHandle(hFaxStartedEvent);

            DestroyWindow(hWnd);

            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            FAXWINHELP(iMsg, wParam, lParam, monitorHelpIDs);
            break;

    }

    return DefDlgProc(hWnd, iMsg, wParam, lParam);
}
