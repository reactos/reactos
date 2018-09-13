/****************************** Module Header ******************************\
* Module Name: timeout.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines apis to support dialog and message-box input timeouts
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/


//
// Define timeout type - represents timeout value in seconds
//
typedef ULONG TIMEOUT;
typedef TIMEOUT * PTIMEOUT;


//
// Define special timeout values
//
// The top bit of the timeout value is the 'notify bit'.
// This bit is only used when the timeout value = TIMEOUT_NONE
//
// When a screen-saver timeout occurs, the timeout stack is searched
// from the top down for the first occurrence of
//  1) A window with a timeout OR
//  2) The first TIMEOUT_NONE window with the notify-bit set.
//
// In case 1), the window is timed-out and returns DLG_SCREEN_SAVER_TIMEOUT.
//
// In case 2), a WM_SCREEN_SAVER_TIMEOUT message is posted to the window.
//
// The notify bit is never inherited. If required, it must be specified in
// addition to TIMEOUT_CURRENT.
//
// NOTE SAS messages are always sent to the topmost timeout window
//
// NOTE User logoff messages cause the top window to return DLG_USER_LOGOFF
// if it has a non-0 timeout, otherwise the window receives a WM_USER_LOGOFF
// message.
//

#define TIMEOUT_VALUE_MASK  (0x0fffffff)
#define TIMEOUT_NOTIFY_MASK (0x10000000)

#define TIMEOUT_VALUE(t)    (t & TIMEOUT_VALUE_MASK)
#define TIMEOUT_NOTIFY(t)   (t & TIMEOUT_NOTIFY_MASK)

#define TIMEOUT_SS_NOTIFY   (TIMEOUT_NOTIFY_MASK)
#define TIMEOUT_CURRENT     (TIMEOUT_VALUE_MASK)    // Use existing timeout
#define TIMEOUT_NONE        (0)                     // Disable input timeout






//
// Exported function prototypes
//


LONG ForwardMessage(
    PTERMINAL pTerm,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID ProcessDialogTimeout(
    HWND    hwnd,
    UINT    Message,
    DWORD   wParam,
    LONG    lParam
    );

int TimeoutMessageBoxEx(
    PTERMINAL    pTerm,
    HWND hWnd,
    UINT IdText,
    UINT IdCaption,
    UINT wType,
    TIMEOUT Timeout
    );

int TimeoutMessageBoxlpstr(
    PTERMINAL pTerm,
    HWND hWnd,
    LPTSTR Text,
    LPTSTR Caption,
    UINT wType,
    TIMEOUT Timeout
    );

int
TimeoutDialogBoxParam(
    PTERMINAL    pTerm,
    HANDLE hInstance,
    LPTSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    TIMEOUT Timeout
    );

int
TimeoutDialogBoxIndirectParam(
    PTERMINAL    pTerm,
    HANDLE hInstance,
    LPDLGTEMPLATE Template,
    HWND hwndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    TIMEOUT Timeout
    );

BOOL EndTopDialog(
    HWND    hwnd,
    int DlgResult
    );

BOOL SetTopTimeout(HWND hwnd);


BOOL
TimeoutUpdateTopTimeout(
    DWORD   Timeout);
