/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    gui.c

Abstract:

    This file implements the ui.

Author:

    Wesley Witt (wesw) 1-Nov-1993

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop

extern AVS Avs;


#ifdef DEBUGVER
DEBUG_VERSION('W', 'R', "WinDbg Remote Shell, DEBUG")
#else
RELEASE_VERSION('W', 'R', "WinDbg Remote Shell")
#endif

#define DEF_POS_X                   0              // window position
#define DEF_POS_Y                   0
#define DEF_SIZE_X                  400            // window size
#define DEF_SIZE_Y                  200

HANDLE      hMessageThread;
HACCEL      HAccTable;
HINSTANCE   g_hInst;
CHAR        szAppName[MAX_PATH];
CHAR        szTransportLayers[4096];
CHAR        szHelpFileName[_MAX_PATH];

extern HWND     HWndFrame;
extern CHAR     ClientId[];
extern BOOL     fConnected;
extern HANDLE   hEventLoadTl;


INT_PTR CALLBACK TransportLayersDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);



LONG APIENTRY
MainWndProc(
    HWND hwnd,
    UINT message,
    UINT wParam,
    LONG lParam
    )
{
    char s[MAX_PATH];
    static UINT uUpdateWindow_TimerId = 1;
    
    switch (message) {

    case WM_CREATE:
        g_CWindbgrmFeedback.UpdateInfo(hwnd);
        SetTimer(hwnd, uUpdateWindow_TimerId, 1500, NULL);
        break;

    case WM_TIMER:
        g_CWindbgrmFeedback.UpdateInfo(hwnd);
        UpdateWindow(hwnd);
        return 0;

    case WM_PAINT:
        // Give the user feedback as to what is going on
        {
            static long lCurrentlyPainting = FALSE;

            if (InterlockedExchange(&lCurrentlyPainting, TRUE)) {
                // Only 1 call at a time. Return 0 to indicate processed.
                return 0;
            }

            PAINTSTRUCT ps ={0};
            HDC hdc = BeginPaint(hwnd, &ps);

            if (hdc) {
                SetBkMode(hdc, TRANSPARENT);
                g_CWindbgrmFeedback.PaintConnectionStatus(hwnd, hdc);
            }

            EndPaint(hwnd, &ps);

            InterlockedExchange(&lCurrentlyPainting, FALSE);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_OPTIONS_EXIT:
            SendMessage( hwnd, WM_CLOSE, 0, 0 );
            break;
            
        case IDM_OPTIONS_DEBUG_DLLS:
            // If the transport layer was specified via the command line,
            //  warn the user, that his actions may cause the selection
            //  of a new TL.
            if (g_RM_Windbgrm_WkSp.m_pszSelectedTL && pszTlName
                && strcmp(pszTlName, g_RM_Windbgrm_WkSp.m_pszSelectedTL)) {
                
                char szWarning[MAX_MSG_TXT];
                
                Dbg(LoadString(g_hInst, IDS_Sys_Warning, szWarning, sizeof(szWarning)));
                
                WKSP_MsgBox(szWarning, IDS_Sys_Default_TL_Overriden, pszTlName, 
                    g_RM_Windbgrm_WkSp.m_pszSelectedTL);
            }
            
            DialogBox( g_hInst,
                MAKEINTRESOURCE(IDD_DLG_WINDBGRM_TRANSPORTLAYER),
                hwnd,
                TransportLayersDlgProc
                );
            SetEvent( hEventLoadTl );
            break;
            
        case IDM_HELP_CONTENTS:
            WinHelp(hwnd, szHelpFileName, HELP_CONTENTS, 0L);
            break;
            
        case IDM_HELP_ABOUT:
            ShellAbout( hwnd, "Windbg Remote", NULL, NULL );
            break;
            
        }
        break;
        
        case WM_SYSCOMMAND:
            if (wParam == IDM_STATUS) {
                if (fConnected) {
                    sprintf(s, "Connected to %s",ClientId);
                } else {
                    strcpy(s, "Not Connected");
                }
                MessageBox( hwnd,
                    s,
                    "WinDbgRm Connection Status",
                    MB_OK | MB_ICONINFORMATION
                    );
            }
            break;
            
        case WM_DESTROY:
            ExitProcess( 0 );
            break;
    }
    
    return DefWindowProc( hwnd, message, wParam, lParam );
}


DWORD
MessagePumpThread(
    LPVOID lpvArg
    )
{
    MSG       msg;
    WNDCLASS  WndClass;
    HMENU     hMenu;
    
    
    g_hInst = GetModuleHandle( NULL );
    LoadString( g_hInst, IDS_APPNAME, szAppName, sizeof(szAppName) );
    
    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = 0;
    WndClass.lpszClassName = szAppName;
    WndClass.lpszMenuName  = szAppName;
    WndClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    WndClass.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    WndClass.hInstance     = g_hInst;
    WndClass.lpfnWndProc   = (WNDPROC)MainWndProc;
    WndClass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    WndClass.hIcon         = LoadIcon( g_hInst, "WindbgRmIcon" );
    
    HAccTable = LoadAccelerators( g_hInst, szAppName );
    
    RegisterClass( &WndClass );
    
    HWndFrame = CreateWindow( szAppName,
        szAppName,
        WS_TILEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        DEF_SIZE_X,
        DEF_SIZE_Y,
        NULL,
        NULL,
        g_hInst,
        NULL
        );
    
    if (!HWndFrame) {
        return FALSE;
    }
    
    ShowWindow( HWndFrame, SW_SHOWMINNOACTIVE );
    
    hMenu = GetSystemMenu( HWndFrame, FALSE );
    AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
    AppendMenu( hMenu, MF_STRING, IDM_STATUS, "Connection Status..." );
    
    while (GetMessage( &msg, NULL, 0, 0 )) {
        TranslateAccelerator( HWndFrame, HAccTable, &msg );
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    
    return 0;
}


BOOL
InitApplication(
    VOID
    )
{
    DWORD     tid;
    char      szDrive[_MAX_DRIVE];
    char      szDir[_MAX_DIR];
    char      szFName[_MAX_FNAME];
    char      szExt[_MAX_EXT];
    
    hMessageThread = CreateThread( NULL, 0, MessagePumpThread, 0, 0, &tid );
    SetThreadPriority( hMessageThread, THREAD_PRIORITY_ABOVE_NORMAL );
    
    //
    // Build help file name from executable path
    //
    
    (void)GetModuleFileName(g_hInst, szHelpFileName, _MAX_PATH);
    _splitpath(szHelpFileName, szDrive, szDir, szFName, szExt);
    strcpy(szHelpFileName, szDrive);
    strcat(szHelpFileName, szDir);
    strcat(szHelpFileName, "windbg.hlp");
    
    if(bHelpOnInit) {
        // this will invoke windbg.hlp with the remote debugging help
        WinHelp(HWndFrame, szHelpFileName, HELP_CONTEXT, _Setting_Up_a_User_Mode_Remote_Debugging_Session );
    }
    
    return TRUE;
}

void
EnableTransportChange(
    BOOL Enable
    )
{
    HMENU hMenu = GetSystemMenu( HWndFrame, FALSE );
    EnableMenuItem(hMenu,
                   IDM_OPTIONS_DEBUG_DLLS,
                   Enable ? MF_ENABLED : MF_DISABLED);
}
