/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDEAGNT.C;3  9-Feb-93,20:14:14  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin

    NDDEAGNT.C

    NetDDE Agent for Access to Apps in User Space

    Revisions:
    12-92   IgorM.  Wonderware initial create.
     4-93   IgorM.  Wonderware modify to use Trust Share API. Add Init verify.
    10-93   SanfordsS   Auto starting and linkup to Services added.
     6-97   CLupu   Moved to winlogon

   $History: End */


#include "precomp.h"
#pragma hdrstop

#include <windows.h>
#include <dde.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

#include "nddeagnt.h"
#include "nddeapi.h"

#include "immp.h"


#if DBG

BOOL g_bTraceNetddeAgent = FALSE;

#define NDDETRACE(m)            \
{                               \
    if (g_bTraceNetddeAgent) {  \
        KdPrint(m);             \
    }                           \
}

#else
#define NDDETRACE(m)
#endif // DBG

#define WAIT_FOR_NETDDE_TIMEOUT (10 * 1000) // 10 seconds allowed to start NetDDE

TCHAR   gszAppName[]            = SZ_NDDEAGNT_SERVICE;
TCHAR   szTitle[]               = SZ_NDDEAGNT_TITLE;
TCHAR   tmpBuf[128];
TCHAR   szComputerName[ 100 ];
DWORD   cbName                  = sizeof(szComputerName);
TCHAR   szServerName[ 100 ]     = TEXT("[none]");
LPTSTR  lpszServer              = szServerName;
TCHAR   szNetDDE[]              = TEXT("NetDDE");
TCHAR   szNetDDEDSDM[]          = TEXT("NetDDEDSDM");
TCHAR   szClipSrv[]             = TEXT("ClipSrv");
TCHAR   szNddeClass[]           = TEXT("NetDDEMainWdw");

TCHAR   szStartServices[]       = SZ_NDDEAGNT_TOPIC;
TCHAR   szAgentExecRtn[]        = TEXT("NetddeAgentExecRtn");
UINT    wMsgNddeAgntExecRtn;
TCHAR   szAgentWakeUp[]         = TEXT("NetddeAgentWakeUp");
UINT    wMsgNddeAgntWakeUp;
TCHAR   szAgentAlive[]          = TEXT("NetddeAgentAlive");
UINT    wMsgNddeAgntAlive;
TCHAR   szAgentDying[]          = TEXT("NetddeAgentDying");
UINT    wMsgNddeAgntDying;

HWND    g_hwndAppDesktopThread; // netdde agent's main window
HWND    g_hNetDDEMainWnd;       // netdde's main window

HANDLE  hInst;
BOOL    fStartFailed            = 0;
SC_HANDLE hSvcNetDDE            = NULL;
SC_HANDLE hSvcNetDDEDSDM        = NULL;
SC_HANDLE hSvcClipSrv           = NULL;

BOOL IsServiceActive(SC_HANDLE *phSvc, LPTSTR pszSvcName, BOOL fCleanup);
BOOL CompareNddeModifyId(LPTSTR lpszShareName, LPDWORD lpdwIdNdde,
        LPDWORD lpdwOptions);

BOOL    gbADTRunning = FALSE;

PTERMINAL gpCrtTerm;

//*************************************************************
//
//  ApplicationDesktopThread()
//
//  Purpose:    Handles netdde agent stuff. Other candidates for
//              this thread are: userinit.exe ...
//
//  Parameters: pTerm  -  pointer to the current terminal
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

DWORD WINAPI
ApplicationDesktopThread(
    PTERMINAL pTerm)
{
    MSG     msg;
    TCHAR   szBuf[128];

    hInst = g_hInstance;

    gbADTRunning = TRUE;

    NDDETRACE(("NetddeAgentThread: starting ApplicationDesktopThread...\n"));
    
    gpCrtTerm = pTerm;
    
    /*
     * set this thread to run on WinSta0\Default
     */
    
    SetThreadDesktop(pTerm->pWinStaWinlogon->hdeskApplication);

    if (!NDDEAgntInit()) {

        NDDETRACE(("NetddeAgentThread: Nddeagnt.exe failed to initialize\n"));

        LoadString(hInst, IDS_NDDE_FAILED, szBuf, 128);

        MessageBox(NULL, szBuf, gszAppName, MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }


    /*
     * now create the window
     */
    g_hwndAppDesktopThread = CreateWindowEx(WS_EX_TOPMOST, gszAppName,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInst,
        NULL);

    if (!g_hwndAppDesktopThread) {
        NDDETRACE(("NetddeAgentThread: Couldn't create window.\n"));
        return FALSE;
    }

#ifdef HASUI
    ShowWindow(g_hwndAppDesktopThread, SW_MINIMIZE);
    UpdateWindow(g_hwndAppDesktopThread);
#endif

    /* set up lpszServer for NDDEAPI calls */
    GetComputerName(szComputerName, &cbName);
    _tcscpy(lpszServer, TEXT("\\\\"));
    _tcscat(lpszServer, szComputerName);

    /*
     * clean up any trusted shares that have been modified or deleted
     * since we last ran for this user
     */
    if (IsServiceActive(&hSvcNetDDE, szNetDDE, FALSE)) {
        CleanupTrustedShares();
    }

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    gbADTRunning = FALSE;
    
    NDDETRACE(("NetddeAgentThread: ApplicationDesktopThread exiting\n"));

    /*
     *  339012; Remove the UnregisterClass call.  If/When it failed, the next
     *  time we entered this function (the next logon), NDDEAgntInit (RegisterClassW)
     *  would also fail, and the nddeagnt window was not being recreated.
     */
    //  UnregisterClass((LPTSTR)SZ_NDDEAGNT_CLASS, hInst);


    return (DWORD)msg.wParam;
}


#define IMMMODULENAME L"IMM32.DLL"
#define PATHDLM     L'\\'
#define IMMMODULENAMELEN    ((sizeof PATHDLM + sizeof IMMMODULENAME) / sizeof(WCHAR))

VOID GetImmFileName(
    PWSTR wszImmFile)
{
    UINT i = GetSystemDirectoryW(wszImmFile, MAX_PATH);

    if (i > 0 && i < MAX_PATH - IMMMODULENAMELEN) {
        wszImmFile += i;
        if (wszImmFile[-1] != PATHDLM) {
            *wszImmFile++ = PATHDLM;
        }
    }
    wcscpy(wszImmFile, IMMMODULENAME);
}

/****************************************************************************

    FUNCTION: NDDEAgntInit(VOID)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

int NDDEAgntInit(
    VOID)
{
    WNDCLASS WndClass;

    wMsgNddeAgntExecRtn = RegisterWindowMessage(szAgentExecRtn);
    wMsgNddeAgntWakeUp  = RegisterWindowMessage(szAgentWakeUp);
    wMsgNddeAgntAlive   = RegisterWindowMessage(szAgentAlive);
    wMsgNddeAgntDying   = RegisterWindowMessage(szAgentDying);

    {
        WCHAR wszImmFile[MAX_PATH];
        BOOL (WINAPI* fpImmDisableIme)(DWORD);
        HMODULE hImm;

        GetImmFileName(wszImmFile);
        hImm = GetModuleHandleW(wszImmFile);
        if (hImm) {
            fpImmDisableIme = (BOOL (WINAPI*)(DWORD))GetProcAddress(hImm, "ImmDisableIme");
            if (fpImmDisableIme) {
#ifndef LATER
                fpImmDisableIme(0);
#else
                fpImmDisableIme(-1);
#endif
            }
        }
    }

    ZeroMemory(&WndClass, sizeof(WndClass));

    WndClass.lpszClassName = (LPTSTR)SZ_NDDEAGNT_CLASS;
    WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    WndClass.hInstance     = hInst;
    WndClass.lpfnWndProc   = NDDEAgntWndProc;

    RegisterClass(&WndClass);
    return TRUE;
}



/*
 * Returns TRUE if anything went wrong or NetDDE service is active.
 */
BOOL IsServiceActive(
    SC_HANDLE* phSvc,
    LPTSTR     pszSvcName,
    BOOL       fCleanup)
{
    DWORD                cServices;
    DWORD                iEnum = 0;
    SC_HANDLE            hSvcMgr;
    ENUM_SERVICE_STATUS* pess = NULL;
    DWORD                cbData;
    LONG                 status;
    BOOL                 retval;

    if (fCleanup) {
        if (*phSvc != NULL) {
            CloseServiceHandle(*phSvc);
            *phSvc = NULL;
        }
        retval = TRUE;
        goto exitIsServiceActive;   // fSuccess, NOT fActive
    }
    if (*phSvc != NULL) {

        SERVICE_STATUS SvcStatus;

        /*
         * FAST method:
         */
        if (QueryServiceStatus(*phSvc, &SvcStatus)) {
            retval = (SvcStatus.dwCurrentState == SERVICE_RUNNING);
        } else {
            retval = FALSE;
        }
        goto exitIsServiceActive;
    }

    hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (hSvcMgr) {
        cbData = 0;
        if (EnumServicesStatus(hSvcMgr, SERVICE_WIN32, SERVICE_ACTIVE,
                NULL, 0, &cbData, &cServices, &iEnum)) {
            /*
             * Success with cbData==0 implies no services are running
             * so return FALSE.
             */
            retval = FALSE;
            goto exitIsServiceActive;
        }
        status = GetLastError();
        if (status != ERROR_MORE_DATA) {
            /*
             * Not what we expected.
             */
            retval = TRUE;
            goto exitIsServiceActive;
        }
        pess = LocalAlloc(LPTR, cbData);
        if (pess == NULL) {
            CloseServiceHandle(hSvcMgr);
            retval = TRUE;
            goto exitIsServiceActive;
        }
        iEnum = 0;
        if (!EnumServicesStatus(hSvcMgr, SERVICE_WIN32, SERVICE_ACTIVE,
                pess, cbData, &cbData, &cServices, &iEnum)) {
#if DBG
            GetLastError();
#endif
            LocalFree(pess);
            CloseServiceHandle(hSvcMgr);
            retval = TRUE;
            goto exitIsServiceActive;
        }
        if (cServices > 0) {
            while (cServices--) {
                if (!_tcsicmp(pess[cServices].lpServiceName, pszSvcName)) {
                    /*
                     * Cache Service handle
                     */
                    *phSvc = OpenService(hSvcMgr, pszSvcName,
                            SERVICE_QUERY_STATUS);
                    LocalFree(pess);
                    CloseServiceHandle(hSvcMgr);
                    retval = TRUE;
                    goto exitIsServiceActive;
                }
            }
        }
        LocalFree(pess);
        CloseServiceHandle(hSvcMgr);
    }
    retval = FALSE;

exitIsServiceActive:

    NDDETRACE(("NDDEAgnt: IsServiceActive %ws returns %d\n", pszSvcName, retval));
    return retval;
}


BOOL StartNetDDEServices(
    BOOL fNotifyUser)
{
    SC_HANDLE hSvcMgr, hSvc;
    BOOL bOk = TRUE;
    HWND hwndDlg = NULL;

    NDDETRACE(("NDDEAgnt: StartNetDDEServices fNotifyUser %d ...\n", fNotifyUser));

    if (fNotifyUser) {
        hwndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STARTING), NULL, NULL);
        if (hwndDlg != NULL) {
            RECT rc;
            MSG msg;

            GetWindowRect(hwndDlg, &rc);
            SetWindowPos(hwndDlg, HWND_TOPMOST,
                    (GetSystemMetrics(SM_CXFULLSCREEN) - (rc.right - rc.left)) / 2,
                    (GetSystemMetrics(SM_CYFULLSCREEN) - (rc.bottom - rc.top)) / 2,
                    0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
            // SetForegroundWindow(hwndDlg);

            /*
             * Process dialog messages so it can paint, etc.
             */
            while (PeekMessage(&msg, hwndDlg, 0, 0, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
        }
    }
    hSvcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSvcMgr) {
        /*
         * Because ClipSrv is dependent on NetDDE and NetDDE is dependent
         * on nddeDSDM, this call starts all three services.  In addition,
         * the service controller will not return from this call until
         * all dependent services are fully initialized so we are guarenteed
         * that NetDDE and nddeDSDM are ready to go by the time this returns.
         */
        hSvc = OpenService(hSvcMgr, TEXT("ClipSrv"), SERVICE_START);
        if (hSvc) {
            StartService(hSvc, 0, NULL);
            CloseServiceHandle(hSvc);
        } else {
            bOk = FALSE;
        }
        CloseServiceHandle(hSvcMgr);
    } else {
        bOk = FALSE;
    }
    if (hwndDlg != NULL) {
        DestroyWindow(hwndDlg);
    }

    NDDETRACE(("NDDEAgnt: StartNetDDEServices returns %d\n", bOk));

    return bOk;
}


HWND ConnectToNetDDEService(
    DWORD dwTimeout)
{
    HANDLE              hPipe;
    NETDDE_PIPE_MESSAGE nameinfo;
    DWORD               cbRead;
    DWORD               tcStart;
    DWORD               dwMode;
    MSG                 msg;

    NDDETRACE(("NDDEAgnt: ConnectToNetDDEService ...\n"));

    /*
     * locate the NetDDE pipe .. give the system some time
     * to let the NetDDE service create it.
     */
    hPipe = CreateFileW(NETDDE_PIPE,
            GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | SECURITY_SQOS_PRESENT |
            SECURITY_IMPERSONATION, NULL);
    if (hPipe == INVALID_HANDLE_VALUE && dwTimeout == 0) {

        NDDETRACE(("NDDEAgnt: ConnectToNetDDEService could not create pipe\n"));

        return NULL;
    }

    tcStart = GetTickCount();
    while (hPipe == INVALID_HANDLE_VALUE &&
            ((tcStart + dwTimeout) >
            GetTickCount())) {

        /*
         * Wait this way so NetDDE can send us our wakeup call.
         */
        MsgWaitForMultipleObjects(0, NULL, TRUE, 1000, QS_ALLINPUT);
        hPipe = CreateFileW(NETDDE_PIPE,
                GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | SECURITY_SQOS_PRESENT |
                SECURITY_IMPERSONATION, NULL);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
    }
    if (hPipe == INVALID_HANDLE_VALUE) {
        NDDETRACE(("NDDEAgnt: ConnectToNetDDEService could not create pipe 2\n"));
        return NULL;
    }

    NDDETRACE(("NDDEAgnt: ConnectToNetDDEService created pipe 0x%x\n", hPipe));

    /*
     * Package up the windowstation and desktop names and
     * send them to NetDDE.
     */
    wcscpy(nameinfo.awchNames, L"WinSta0");
    nameinfo.dwOffsetDesktop = wcslen(nameinfo.awchNames) + 1;
    wcscpy(&nameinfo.awchNames[nameinfo.dwOffsetDesktop],
            L"default");
    dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
    if (!TransactNamedPipe(hPipe, &nameinfo, sizeof(nameinfo),
            &g_hNetDDEMainWnd, sizeof(HWND), &cbRead, NULL)) {
        g_hNetDDEMainWnd = NULL;
    }
    CloseHandle(hPipe);

    NDDETRACE(("NDDEAgnt: ConnectToNetDDEService returns hWndNddeAgnt = 0x%x\n", g_hNetDDEMainWnd));
    return g_hNetDDEMainWnd;
}


LRESULT
APIENTRY
NDDEAgntWndProc(
    HWND hWnd,         // window handle
    UINT message,      // type of message
    WPARAM uParam,     // additional information
    LPARAM lParam )    // additional information
{
    TCHAR szAppName[256];
    static BOOL fInitializing = FALSE;

    switch (message) {

    case WM_DDE_INITIATE:

        NDDETRACE(("NDDEAgntWndProc: WM_DDE_INITIATE hwndClient 0x%x ...\n", uParam));

        if (GlobalGetAtomName(LOWORD(lParam), szAppName, sizeof(szAppName))) {
            HWND hwndNetDDEMain = NULL;

            NDDETRACE(("    WM_DDE_INITIATE app: %ws\n", szAppName));

            if (_tcsicmp(szAppName, gszAppName) != 0) {
                if (_tcsstr(szAppName, TEXT("NDDE$")) == NULL &&
                    _tcsstr(szAppName, TEXT("\\\\")) == NULL) {
                    /*
                     * Not for NetDDE at all.  let it go.
                     */
                    NDDETRACE(("    WM_DDE_INITIATE - not for NetDDE\n"));
                    return 0;
                }
            } else {
                /*
                 * Its addressing us as a DDE service!
                 *
                 * We use the WM_DDE_INITIATE as a fast execute to this app.
                 * If the atoms are right, we will start the NetDDE services
                 * if needed.  This lets other components do this with
                 * one simple SendMessage() yet does not require new APIs.
                 */
                if (!GlobalGetAtomName(HIWORD(lParam), szAppName, sizeof(szAppName))) {
                    NDDETRACE(("    WM_DDE_INITIATE - Bad App Atom\n"));
                    return 0;
                }
                if (_tcsicmp(szAppName, szStartServices) != 0) {
                    NDDETRACE(("    WM_DDE_INITIATE - Not request to start services\n"));
                    return 0;
                }
            }

            /*
             * If we're already initializing, skip it.
             */
            if (fInitializing) {
                NDDETRACE(("    WM_DDE_INITIATE - fInitializing == TRUE, blow it off\n"));
                return 0;
            }
            fInitializing = TRUE;
            if (IsServiceActive(&hSvcNetDDE, szNetDDE, FALSE) &&
                IsServiceActive(&hSvcNetDDEDSDM, szNetDDEDSDM, FALSE) &&
                IsServiceActive(&hSvcClipSrv, szClipSrv, FALSE)) {

                HWND hNetDDEMainWnd;

                NDDETRACE(("    WM_DDE_INITIATE - all the services running\n"));
                /*
                 * If there is no main window, connect to NetDDE
                 * and get the main window handle
                 */

                NDDETRACE(("    WM_DDE_INITIATE - FindWindow class %ws name %ws ...\n",
                          szNddeClass, szNetDDE));

                hNetDDEMainWnd = FindWindow(szNddeClass, szNetDDE);

                if (hNetDDEMainWnd != g_hNetDDEMainWnd) {

                    NDDETRACE(("    WM_DDE_INITIATE - new or no NetDDE main window\n"));

                    g_hNetDDEMainWnd = hNetDDEMainWnd;

                    ConnectToNetDDEService(0);
                } else {
                    NDDETRACE(("    WM_DDE_INITIATE - window found\n"));
                }
            } else {

                /*
                 * Start the NetDDE service(s)
                 */
                if (fStartFailed == 0) {
                    if (!StartNetDDEServices(TRUE)) {

                        TCHAR szBuf[128];

                        fStartFailed++;

                        NDDETRACE(("    WM_DDE_INITIATE - netdde.exe can't start\n"));

                        LoadString(hInst, IDS_NDDE_CANTSTART, szBuf, 128);

                        MessageBox(NULL, szBuf, szTitle, MB_OK | MB_SYSTEMMODAL);
                    }

                    /*
                     * Connect to NetDDE and get the main window handle
                     */
                    if (ConnectToNetDDEService(WAIT_FOR_NETDDE_TIMEOUT) != NULL) {

                        /*
                         * BUG:  If we pass on this message to netdde, things will
                         * always work the first time... but they may also be
                         * double connected because the app may happen to send
                         * another initiate message directly to the netdde
                         * service - depending on how it does its broadcast.
                         * There is no way to solve this for all cases so we
                         * don't pass it on.  Apps that want to work first time,
                         * everytime must use DDEML (its method of enumeration
                         * works here) or they must start the services themselves.
                         */
                        //return(SendMessage(hwndNetDDEMain, message, uParam, lParam));
                    }
                }
            }
            fInitializing = FALSE;
            return 0;
        }
        break;

    case WM_COPYDATA:
        NDDETRACE(("NDDEAgntWndProc: WM_COPYDATA: hwnd %0X wParam %0X lParam %0lX\n",
                  hWnd, uParam, lParam));
        HandleNetddeCopyData(hWnd, uParam, (PCOPYDATASTRUCT)lParam);
        return TRUE; // processed the msg */

    case WM_ENDSESSION:
        NDDETRACE(("NDDEAgntWndProc: WM_ENDSESSION: hwnd %p\n", hWnd));
        if (uParam == 0)
            break;

        /*
         * intentional fall-through
         */

    case WM_DESTROY:  // message: window being destroyed
        NDDETRACE(("NDDEAgntWndProc: WM_DESTROY: hwnd %p.\n", hWnd));
        
        SendNotifyMessage( (HWND)-1, wMsgNddeAgntDying,
                    (WPARAM) g_hNetDDEMainWnd, 0);
        PostQuitMessage(0);
        IsServiceActive(&hSvcNetDDE, szNetDDE, TRUE);           // cleanup.
        IsServiceActive(&hSvcNetDDEDSDM, szNetDDEDSDM, TRUE);   // cleanup.
        IsServiceActive(&hSvcClipSrv, szClipSrv, TRUE);      // cleanup.

        break;

    case WM_CLOSE:
        NDDETRACE(("NDDEAgntWndProc: WM_CLOSE: hwnd %p NOT PROCESSED !!!\n", hWnd));
        break;
    
    case WM_NOTIFY:
        NDDETRACE(("NDDEAgntWndProc: WM_NOTIFY: hwnd %p wParam 0x%x, lParam %p\n",
                  hWnd, uParam, lParam));

        if ((WPARAM)hWnd == uParam && (LPARAM)hWnd == lParam) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_USERCHANGED:
        if (lParam == (LPARAM)gpCrtTerm->pWinStaWinlogon->hToken &&
            uParam == (WPARAM)hWnd) {
            
            /*
             * Impersonate the logged on USER for this thread
             */
            ImpersonateLoggedOnUser(gpCrtTerm->pWinStaWinlogon->hToken);
        }
        break;

    default:          // Passes it on if unproccessed
        if (message == wMsgNddeAgntWakeUp) {
            NDDETRACE(("NDDEAgntWndProc: WakeUp %x received.  Sending %x to %x.\n",
                    wMsgNddeAgntWakeUp,
                    wMsgNddeAgntAlive,
                    uParam));
            SendMessage( (HWND) uParam, wMsgNddeAgntAlive, (WPARAM) hWnd, 0);
            return TRUE; // processed the msg
        } else {
            NDDETRACE(("NDDEAgntWndProc: Unprocessed NetddeAgent message %0X\n", message));
        }
        return (DefWindowProc(hWnd, message, uParam, lParam));
    }
    return 0;
}



/*
 *  HandleNetddeCopyData()
 *
 *      This handles the WM_COPYDATA message from NetDDE to start an
 *      application in the user's context
 */
BOOL
HandleNetddeCopyData(
    HWND            hWndNddeAgnt,
    WPARAM          wParam,
    PCOPYDATASTRUCT pCopyDataStruct )
{
    PNDDEAGTCMD     pNddeAgtCmd;
    LPTSTR          lpszShareName;
    LPTSTR          lpszCmdLine;
    UINT            uAgntExecRtn = 0;
    UINT            uAgntInitRtn = 0;
    UINT            fuCmdShow = 0;
    DWORD           dwOptions;
    COPYDATASTRUCT  cds;
    BOOL            RetStatus = FALSE;

    NDDETRACE(("NetddeAgnt: HandleNetddeCopyData ...\n"));

    /* sanity checks on the structure coming in */
    if (pCopyDataStruct->cbData < sizeof(NDDEAGTCMD)) {
        return FALSE;
    }
    pNddeAgtCmd = (PNDDEAGTCMD)(pCopyDataStruct->lpData);
    if (pNddeAgtCmd->dwMagic != NDDEAGT_CMD_MAGIC ) {
        return FALSE;
    }

    if (pNddeAgtCmd->dwRev != 1) {
        return FALSE;
    }

    /* passed the sanity checks ... lets's do the command */
    switch( pNddeAgtCmd->dwCmd )  {
    case NDDEAGT_CMD_WINEXEC:

        NDDETRACE(("NetddeAgnt: HandleNetddeCopyData NDDEAGT_CMD_WINEXEC\n"));

        /* get the sharename and cmdline out of szData */
#ifdef UNICODE

        if (!MBToWCS(pNddeAgtCmd->szData, -1, &lpszShareName, -1, TRUE)) {
            return FALSE;
        }
        if (!MBToWCS(pNddeAgtCmd->szData + lstrlen(lpszShareName) + 1, -1, &lpszCmdLine, -1, TRUE)) {
            RtlFreeHeap(RtlProcessHeap(), 0, lpszShareName);
            return FALSE;
        }
#else
        lpszShareName = pNddeAgtCmd->szData;
        lpszCmdLine = lpszShareName + lstrlen(lpszShareName) + 1;
#endif

        NDDETRACE(("NetddeAgnt: CreateProcess sharename: %ws cmdline: %ws\n", lpszShareName, lpszCmdLine));
        /*
         * make sure that no one changed the share since the user told
         * us he trusted it
         */
        if( CompareNddeModifyId( lpszShareName,
                &pNddeAgtCmd->qwModifyId[0], &dwOptions ) )  {
            /*
             * no one has modified the share ... start the app in the
             * user's context
             */
            if (dwOptions & NDDE_TRUST_SHARE_START) {

                STARTUPINFO         si;
                PROCESS_INFORMATION pi;
                BOOL                bSuccess;

                fuCmdShow = pNddeAgtCmd->fuCmdShow;
                if( (dwOptions & NDDE_TRUST_CMD_SHOW) )  {
                    fuCmdShow = dwOptions & NDDE_CMD_SHOW_MASK;
                }

                RtlZeroMemory(&si, sizeof(STARTUPINFO));

                si.cb          = sizeof(STARTUPINFO);
                si.lpDesktop   = L"WinSta0\\Default";
                si.dwFlags     = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_SHOWNORMAL;

                bSuccess = CreateProcess(
                    NULL,           // pointer to name of executable module
                    lpszCmdLine,    // I want to pass here "app.exe param"
                    NULL,           // pointer to process security attributes
                    NULL,           // pointer to thread security attributes
                    FALSE,          // handle inheritance flag
                    0,              // creation flags
                    NULL,           // pointer to new environment block
                    NULL,           // pointer to current directory name
                    &si,            // pointer to STARTUPINFO
                    &pi             // pointer to PROCESS_INFORMATION
                   );

                if (!bSuccess) {
                    NDDETRACE(("NetddeAgnt: CreateProcess failed with: %x\n", GetLastError()));
                    uAgntExecRtn = 0;
                } else {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);

                    NDDETRACE(("NetddeAgnt: CreateProcess successful\n"));
                    uAgntExecRtn = NDDEAGT_INIT_OK;
                }

            } else {
                NDDETRACE(("NetddeAgnt: CreateProcess Start app not allowed\n"));
                uAgntExecRtn = NDDEAGT_START_NO;
            }
        } else {
            NDDETRACE(("NetddeAgnt: CreateProcess CompareNddeModifyId failed\n"));
            uAgntExecRtn = NDDEAGT_START_NO;
        }
#ifdef UNICODE
        RtlFreeHeap(RtlProcessHeap(), 0, lpszCmdLine);
        RtlFreeHeap(RtlProcessHeap(), 0, lpszShareName);
#endif
        RetStatus = TRUE;
        break;

    case NDDEAGT_CMD_WININIT:

        NDDETRACE(("NetddeAgnt: HandleNetddeCopyData NDDEAGT_CMD_WININIT\n"));

        /* get the sharename and cmdline out of szData */
#ifdef UNICODE
        if (!MBToWCS(pNddeAgtCmd->szData, -1, &lpszShareName, -1, TRUE)) {
            return FALSE;
        }
#else
        lpszShareName = pNddeAgtCmd->szData;
#endif

        NDDETRACE(("NetddeAgnt: WinInit sharename: %ws\n", lpszShareName));

        /*
         * make sure that no one changed the share since the user told
         * us he trusted it
         */
        if( CompareNddeModifyId( lpszShareName,
                &pNddeAgtCmd->qwModifyId[0], &dwOptions ) )  {

            /* no one has modified the share ... start the app in the
                user's context */
            if (dwOptions & NDDE_TRUST_SHARE_INIT) {
                uAgntExecRtn = NDDEAGT_INIT_OK;
                NDDETRACE(("NetddeAgnt: WinInit() Init allowed.\n"));
            } else {
                uAgntExecRtn = NDDEAGT_INIT_NO;
                NDDETRACE(("NetddeAgnt: WinInit() Init not allowed.\n"));
            }
        } else {
            NDDETRACE(("NetddeAgnt: WinInit() CompareNddeModifyId failed\n"));
            uAgntExecRtn = NDDEAGT_INIT_NO;
        }

#ifdef UNICODE
        RtlFreeHeap(RtlProcessHeap(), 0, lpszShareName);
#endif
        RetStatus = TRUE;
        break;

    default:
        RetStatus = FALSE;
        break;
    }
    if (RetStatus) {
        /* send a COPYDATA message back to NetDDE */
        cds.dwData = wMsgNddeAgntExecRtn;
        cds.cbData = sizeof(uAgntExecRtn);
        cds.lpData = &uAgntExecRtn;
        SendMessage( (HWND)wParam, WM_COPYDATA, (WPARAM) g_hNetDDEMainWnd,
            (LPARAM) &cds );
    }
    return RetStatus;
}


/*
 *  Given a share name, GetNddeShareModifyId() will retrieve the modify id
 *  associated with the DSDM share
 */
BOOL
GetNddeShareModifyId(
    LPTSTR  lpszShareName,
    LPDWORD lpdwId)
{
    PNDDESHAREINFO lpDdeI = NULL;
    DWORD          avail = 0;
    WORD           items = 0;
    UINT           nRet;
    BOOL           bRetrieved = FALSE;

    NDDETRACE(("NetddeAgnt: GetNddeShareModifyId( %s ): %d\n", lpszShareName, lpdwId ));

    /* get the share information out of the DSDM DB */

    nRet = NDdeShareGetInfo(lpszServer, lpszShareName, 2, (LPBYTE)NULL,
                            0, &avail, &items);

    if (nRet == NDDE_BUF_TOO_SMALL) {
        lpDdeI = (PNDDESHAREINFO)LocalAlloc(LMEM_FIXED, avail);
        if (lpDdeI == NULL) {

            NDDETRACE(("NetddeAgnt: Unable to allocate sufficient (%d) memory: %d\n",
                avail, GetLastError()));

            bRetrieved = FALSE;
        } else {
            items = 0;
            nRet = NDdeShareGetInfo(lpszServer, lpszShareName, 2, (LPBYTE)lpDdeI,
                avail, &avail, &items);

            if( nRet == NDDE_NO_ERROR )  {
                /* compare modify ids */
                bRetrieved = TRUE;
                lpdwId[0] = lpDdeI->qModifyId[0];
                lpdwId[1] = lpDdeI->qModifyId[1];
            } else {

                NDDETRACE(("NetddeAgnt: Unable to access DDE share \"%s\" info: %d\n",
                    lpszShareName, nRet));

                bRetrieved = FALSE;
            }
            LocalFree(lpDdeI);
        }
    } else {
        NDDETRACE(("NetddeAgnt: Unable to probe DDE share \"%s\" info size: %d\n",
            lpszShareName, nRet));

        bRetrieved = FALSE;
    }
    return bRetrieved;
}

BOOL
CompareModifyIds(
    LPTSTR lpszShareName)
{
    DWORD       dwIdNdde[2];
    DWORD       dwIdTrusted[2];
    DWORD       dwOptions;
    UINT        RetCode;
    BOOL        bRetrievedNdde;

    NDDETRACE(("NetddeAgnt: ComaperModifyIds(%s)\n", lpszShareName));

    bRetrievedNdde = GetNddeShareModifyId( lpszShareName, &dwIdNdde[0] );

    if (!bRetrievedNdde) {
        return(FALSE);
    }
    RetCode = NDdeGetTrustedShare( lpszServer,lpszShareName,
        &dwOptions, &dwIdTrusted[0], &dwIdTrusted[1] );
    if (RetCode != NDDE_NO_ERROR) {

        NDDETRACE(("NetddeAgnt: Error getting trusted share \"%s\" modify id: %d\n",
            lpszShareName, RetCode));

        return FALSE;
    }
    NDDETRACE(("NetddeAgnt: CompareModifyIds() returns: NddeId: %08X%08X, TrustedId: %08X%08X\n",
                dwIdNdde[0], dwIdNdde[1], dwIdTrusted[0], dwIdTrusted[1]));
    if( (dwIdNdde[0] == dwIdTrusted[0])
        && (dwIdNdde[1] == dwIdTrusted[1]) )  {
        return TRUE;
    } else {
        return FALSE;
    }
}





/*
 *  CleanupTrustedShares() goes through all the truested shares for this user
 *  on this machine and makes certain that no one has modified the shares
 *  since the time the user said they were ok.
 */
VOID
CleanupTrustedShares(
    void)
{
    UINT    RetCode;
    DWORD   avail, entries;
    LPBYTE  lpBuf;
    LPTSTR  lpShareName;

    RetCode = NDdeTrustedShareEnum ( lpszServer, 0, (LPBYTE)&avail, 0, &entries, &avail );
    if (RetCode != NDDE_BUF_TOO_SMALL) {
        NDDETRACE(("NetddeAgnt: Probing for Number of Trusted Shares Failed: %d\n", RetCode));

        return;
    }
    lpBuf = LocalAlloc(LPTR, avail);
    if (lpBuf == NULL) {
        NDDETRACE(("NetddeAgnt: Unable to allocate sufficient memory to enumerate trusted shares. Needed: %d\n", avail));

        return;
    }
    RetCode = NDdeTrustedShareEnum ( lpszServer, 0, lpBuf, avail,
            &entries, &avail );
    if (RetCode != NDDE_NO_ERROR) {
        NDDETRACE(("NetddeAgnt: Unable to enumerate trusted shares: %d\n", RetCode));

        LocalFree(lpBuf);
        return;
    }
    for ( lpShareName = (LPTSTR)lpBuf; *lpShareName;
            lpShareName += lstrlen(lpShareName) + 1 ) {
        if( !CompareModifyIds( lpShareName ) )  {
            /* if they don't match exactly ... get rid of it */
            RetCode = NDdeSetTrustedShare( lpszServer, lpShareName, 0);   /* delete option */
            if (RetCode != NDDE_NO_ERROR) {
                NDDETRACE(("NetddeAgnt: Unable to delete obsolete trusted share \"%s\": %d\n",
                    lpShareName, RetCode));
            }
        }
    }
    LocalFree(lpBuf);
    return;
}

/*
 *  CompareNddeModifyId() takes in the computer name, sharename and
 *  the modify id from a NetDDE share and looks up the sharename in
 *  the truested share db and verifies the modify ids are the same.
 */
BOOL
CompareNddeModifyId(
    LPTSTR      lpszShareName,
    LPDWORD     lpdwIdNdde,
    LPDWORD     lpdwOptions )
{
    UINT        RetCode;
    DWORD       dwIdTrusted[2]  = {0, 0};
    BOOL        bMatch = FALSE;

    RetCode = NDdeGetTrustedShare(lpszServer, lpszShareName,
        lpdwOptions, &dwIdTrusted[0], &dwIdTrusted[1]);
    if (RetCode != NDDE_NO_ERROR) {
        NDDETRACE(("NetddeAgnt: Unable to access trusted share \"%s\" info: %d\n",
            lpszShareName, RetCode));
        return FALSE;
    }

    NDDETRACE(("NetddeAgnt: CompareNddeModifyId() returns: NddeId: %08X%08X, TrustedId: %08X%08X\n",
            lpdwIdNdde[0], lpdwIdNdde[1],
            dwIdTrusted[0], dwIdTrusted[1]));

    if( (lpdwIdNdde[0] == dwIdTrusted[0])
        && (lpdwIdNdde[1] == dwIdTrusted[1]) )  {
            bMatch = TRUE;
    }

    return bMatch;
}

//*************************************************************
//
//  StartAppDesktopThread()
//
//  Purpose:    Handles netdde agent stuff. Other candidates for
//              this thread are: userinit.exe ...
//
//  Parameters: pTerm  -  pointer to the current terminal
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

VOID StartAppDesktopThread(
    PTERMINAL pTerm)
{
    HANDLE hAppDesktopThread;
    DWORD  dwThreadId;

    if (gbADTRunning) {
        NDDETRACE(("NetddeAgentThread: aready running\n"));
        return;
    }
    
    hAppDesktopThread = CreateThread(NULL, 0, ApplicationDesktopThread,
                                     (LPVOID)pTerm, 0, &dwThreadId);

    CloseHandle(hAppDesktopThread);
}


