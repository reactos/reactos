//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cscst.h
//
//--------------------------------------------------------------------------

#ifndef _CSCST_H_
#define _CSCST_H_

// Private messages to the CSC Hidden Window
#define PWM_STDBGOUT              (WM_USER + 400)
#define PWM_STATUSDLG             (WM_USER + 401)
#define PWM_TRAYCALLBACK          (WM_USER + 402)
#define PWM_RESET_REMINDERTIMER   (WM_USER + 403)
#define PWM_REFRESH_SHELL         (WM_USER + 406)
#define PWM_QUERY_UISTATE         (WM_USER + 407)
#define PWM_HANDLE_LOGON_TASKS    (WM_USER + 408)

//
// Custom private message defined for the notification window.
// Initiates a status check of the cache and update of the systray
// UI if appropriate.
//
#define STWM_STATUSCHECK        (STWM_CSCCLOSEDIALOGS + 10) 

//
// Enumeration of unique systray UI states.
//
typedef enum { STS_INVALID = 0,
               STS_ONLINE,        // All servers online.
               STS_DIRTY,         // One server has dirty files.
               STS_MDIRTY,        // Multiple servers have dirty files.
               STS_SERVERBACK,    // One server is ready for connection.
               STS_MSERVERBACK,   // Multiple servers ready for connection
               STS_OFFLINE,       // One server is offline.
               STS_MOFFLINE,      // Multiple servers are offline.
               STS_NONET          // No net interface available.
               } eSysTrayState;


// Function for finding the hidden window
HWND _FindNotificationWindow();
LRESULT SendToSystray(UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL PostToSystray(UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL SendCopyDataToSystrayAsync(DWORD dwData, DWORD cbData, PVOID pData);


#if DBG
//
// Use STDBOUT to output text to the CSC "hidden" window when it
// is not hidden.  To make it visible, build a checked version and 
// set the following reg DWORD value to a number 1-5.
// 1 = least verbose output, 5 = most verbose.
// If the value is not present or is 0, the systray window will be
// created hidden.
//
// HKLM\Software\Microsoft\Windows\CurrentVersion\NetCache\SysTrayOutput
//
// The STDBGOUT macro should be used like this.
//
// STDBGOUT((<level>,<fmt string>,arg,arg,arg));
//
// STDBGOUT((1, TEXT("Function foo failed with error %d"), dwError));
//
// Note that no newline is required in the fmt string.
// Entire macro arg set must be enclosed in separate set of parens.
// STDBGOUT stands for "SysTray Debug Output".
//
void STDebugOut(int iLevel, LPCTSTR pszFmt, ...);
#define STDBGOUT(x) STDebugOut x

#else

#define STDBGOUT(x)

#endif

#endif _CSCST_H_
