//****************************************************************************
//
//  Module:     RASAPI32.DLL
//  File:       scripter.h
//  Description:Dial-Up Networking script assignment applet
//  Content:    This file contains global header for the applet
//  History:
//      Tue 09-May-1995 08:13:41  -by-  Viroon  Touranachun [viroont]
//        Created
//
//      Wed 13-Dec-1995 16:31:02  -by-  Viroon  Touranachun [viroont]
//        Moved from scripter.exe
//
//****************************************************************************

//**************************************************************************
//  Function Prototypes
//**************************************************************************

BOOL CALLBACK ScriptAppletDlgProc (HWND    hwnd,
                                   UINT    message,
                                   WPARAM  wParam,
                                   LPARAM  lParam);

DWORD NEAR PASCAL InitScriptDlg (HWND hwnd);
DWORD NEAR PASCAL DeInitScriptDlg (HWND hwnd);
DWORD NEAR PASCAL EditScriptFile(HWND hwnd);
DWORD NEAR PASCAL BrowseScriptFile(HWND hwnd);
DWORD NEAR PASCAL CheckScriptDlgData(HWND hwnd);
DWORD NEAR PASCAL SaveScriptDlgData(HWND hwnd);
