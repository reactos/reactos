/****************************** Module Header ******************************\
* Module Name: timeout.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Support routines to implement dialog input timeouts
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define private notification message sent to dialogs on an interrupt
//

#define WM_DLG_INTERRUPT    (WM_USER+600)

//
// Define value returned by a message box or dialog box when interrupted
// This value must be interpreted using the InterrruptType variable
//

#define DLG_INTERRUPT 0 // Must be 0 so we can return it from message boxes

//
// Define the maximum number of nested dialogs we can handle
// per terminal
//

#define MAX_NESTED_TIMEOUTS 5
#define TIMEOUT_DEFAULT     120

//
// Globals - these will need to put in an instance data structure
// when we have multiple logon threads
//

//
// Define array used to store window handles who ask for timeout service.
// This array is used as a push down stack to suppport nested timeouts
//

HWND        hwndArray[MAX_NESTED_TIMEOUTS];
TIMEOUT     TimeoutArray[MAX_NESTED_TIMEOUTS];
PTERMINAL   GlobalsArray[MAX_NESTED_TIMEOUTS];
LONG        NestedIndex;


//
// Variable that stores the required return code for the top dialog.
// This is only used if we force a dialog to return DLG_INTERRUPT.
// We have to use this mechanism because message boxes will not allow
// you to return any random value. DLG_INTERRUPT is 0 which message
// boxes let through.
//

int TopDialogReturnCode = DLG_INTERRUPT;


BOOL        MessageBoxActive ;
HWND        hwndMessageOwner = NULL;// Store the owner of the message box
                                    // we want to find
LPTSTR       TimeoutTitle = NULL;    // Stores the title of the message box we
                                    // are going to timeout on.
BOOL        InputHooksInstalled = FALSE;
BOOL        TimeoutOccurred = FALSE;

#define DialogActive()  (NestedIndex > 0)

//
// Variables that support the input timeout
//

UINT_PTR TimerID = 0;
HHOOK   hhkMouse;
HHOOK   hhkKeyboard;


//
// Internal Prototypes
//

BOOL PushMessageTimeout(HWND hwndOwner, TIMEOUT TimeDelay, LPTSTR Title, PTERMINAL pTerm);
BOOL PopMessageTimeout(VOID);
BOOL PushTimeout(HWND hwnd, TIMEOUT TimeDelay, PTERMINAL pTerm);
BOOL PopTimeout(VOID);
BOOL StartTopTimeout(VOID);
HWND GetTopTimeout(PTIMEOUT pTimeDelay);
BOOL SetTopTimeout(HWND hwnd);
BOOL SetTimeout(TIMEOUT TimeDelay);
BOOL CheckTimeout(HWND hwnd);
BOOL SetInputHooks(VOID);
BOOL ReleaseInputHooks(VOID);
LRESULT WINAPI MouseHookfn(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI KeyboardHookfn(int nCode, WPARAM wParam, LPARAM lParam);
BOOL StartTimer(TIMEOUT TimeDelay);
WORD WINAPI Timerfn(HWND hwnd, WORD Message, int IDEvent, DWORD dwTime);
BOOL StopTimer(VOID);
BOOL FindMessageBox(VOID);
BOOL WINAPI FindEnumfn(HWND, LPARAM);
TIMEOUT InheritTimeout(TIMEOUT TimeDelay);
BOOL HandleScreenSaverTimeout(HWND hwndTop, TIMEOUT Timeout);
BOOL HandleUserLogoff(PTERMINAL pTerm, HWND hwndTop, TIMEOUT Timeout, LONG Flags);


/***************************************************************************\
* ForwardMessage
*
* Sends a message to the window on top of the timeout stack
*
* 12-05-91 Davidc       Created.
\***************************************************************************/
#if 0

LONG
ForwardMessage(
    PTERMINAL pTerm,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    TIMEOUT Timeout;
    HWND    hwndTop = GetTopTimeout(&Timeout);

    // If there is nothing on the stack, dump the message
    if (hwndTop == NULL) {
        DebugLog((DEB_ERROR, "No-one on the stack to forward message to !"));
        ASSERT(FALSE);
        return(0);
    }

    switch (Message) {

    case WM_SAS:
        SendMessage(hwndTop, Message, wParam, lParam);
        break;

    case WM_INPUT_TIMEOUT:

        // Check the timeout condition still exists and exit the dialog
        if (CheckTimeout(hwndTop)) {

            EndTopDialog(hwndTop, DLG_INPUT_TIMEOUT);

        }
        break;

    case WM_SCREEN_SAVER_TIMEOUT:

        //
        // Ignore if the current user doesn't have an enabled screen-saver.
        // The timeout notification may have been in the message queue when
        // we changed user - the new user may not have a screen-saver.
        //

        if (ScreenSaverEnabled(pTerm)) {
            HandleScreenSaverTimeout(hwndTop, Timeout);
        }

        break;

    case WM_USER_LOGOFF:

        HandleUserLogoff(pTerm, hwndTop, Timeout, lParam);

        break;

    default:
        DebugLog((DEB_ERROR, "Unexpected message supplied to ForwardMessage"));
        ASSERT(FALSE);
        break;
    }

    return(0);
}


/***************************************************************************\
* HandleScreenSaverTimeout
*
* Deal with a screen-saver timeout notification from windows.
*
* If the topmost window has a timeout, the screen-saver timeout simply
* interrupts the dialog and causes it to return DLG_SCREEN_SAVER_TIMEOUT.
*
* If the topmost window has no timeout, the window stack is searched from
* the top for the first window to have the TIMEOUT_SS_NOTIFY bit set.
* This window is then sent a WM_SCREEN_SAVER_TIMEOUT message.
* This allows a non-timeout window to take responsibility for starting
* the screen-saver from any-non timeout windows above it.
*
* Returns TRUE
*
* 12-05-91 Davidc       Created.
\***************************************************************************/
BOOL
HandleScreenSaverTimeout(
    HWND hwndTop,
    TIMEOUT Timeout
    )
{
    HWND hwndNotify;
    LONG Index;

    //
    // If top top window has a timeout, it should be interrupted
    //

    if (TIMEOUT_VALUE(Timeout) != TIMEOUT_NONE) {

        EndTopDialog(hwndTop, DLG_SCREEN_SAVER_TIMEOUT);

        return(TRUE); // We're done
    }


    //
    // The top window has no timeout, search for window with notify bit set
    //

    for (Index = NestedIndex - 1; Index >= 0; Index --) {
        if (TIMEOUT_NOTIFY(TimeoutArray[Index])) {
            // Found a window with the notify bit set
            break;
        }
    }

    // Q.A.
    ASSERTMSG("Notify window not found on timeout stack", Index >= 0);
    ASSERTMSG("Found notify window with a non-zero timeout !!",
               TIMEOUT_VALUE(TimeoutArray[Index]) == TIMEOUT_NONE);

    hwndNotify = hwndArray[Index];

    ASSERTMSG("Found notify window with a NULL hwnd !!", hwndNotify != NULL);

    //
    // Forward the message to the notify window
    //

    SendMessage(hwndNotify, WM_SCREEN_SAVER_TIMEOUT, 0, 0);

    return(TRUE);   // We've dealt with it
}

#endif

/***************************************************************************\
* MapResultCode
*
* Maps the DLG_INTERRUPT message to the appropriate return code stored
* in TopDialogReturnCode
*
* 03-17-92 Davidc       Created.
\***************************************************************************/

int
MapResultCode(
    int Result,
    int TopDialogReturnCode
    )
{
    if (Result == DLG_INTERRUPT) {

        ASSERT(TopDialogReturnCode != DLG_INTERRUPT);

        Result = TopDialogReturnCode;

#if DBG
        //
        // Reset the stored return code so we can detect if an interrupt is
        // generated without the code being set correctly.
        //

        TopDialogReturnCode = DLG_INTERRUPT;
#endif
    }

    if (Result == -1) {
        DebugLog((DEB_ERROR, "Failed to create dialog or message box"));
    }

    return(Result);
}


/***************************************************************************\
* ProcessDialogTimeout
*
* Called as first part of dialog routine that wants to timeout after
* there is no input for a specified time. Simply sets up the hwnd on top
* of our stack.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

VOID ProcessDialogTimeout(
    HWND    hwnd,
    UINT    Message,
    DWORD   wParam,
    LONG    lParam)
{
    switch (Message) {

    case WM_INITDIALOG:
        if (!SetTopTimeout(hwnd)) {
            DebugLog((DEB_ERROR, "ProcessDialogTimeout failed to set the hwnd on top of the timeout stack"));
        }
    }
    DBG_UNREFERENCED_PARAMETER(Message);
    DBG_UNREFERENCED_PARAMETER(wParam);
    DBG_UNREFERENCED_PARAMETER(lParam);
}


/***************************************************************************\
* PushDialogTimeout
*
* Sets up an input timeout for the dialog box that is just about to be created
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL PushDialogTimeout(
    TIMEOUT    TimeDelay,
    PTERMINAL    pTerm)
{
    TimeDelay = InheritTimeout(TimeDelay);

    // Push the timeout with a NULL hwnd (to be filled in later)
    PushTimeout(NULL, TimeDelay, pTerm);


    return(TRUE);
}


/***************************************************************************\
* PushMessageTimeout
*
* Sets up an input timeout for the message box that is just about to be created
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL PushMessageTimeout(
    HWND    hwndOwner,
    TIMEOUT TimeDelay,
    LPTSTR   Title,
    PTERMINAL    pTerm)
{
    TimeDelay = InheritTimeout(TimeDelay);

    // Check the last message box has been dealt with
    ASSERTMSG("Last timeout message box not identified yet !", TimeoutTitle == NULL);

    // Push the timeout with a NULL hwnd (to be filled in later)
    PushTimeout(NULL, TimeDelay, pTerm);

    // Store the message box title so that when we need to operate on the
    // message box, we can go and search for it by title.

    TimeoutTitle = Title;
    hwndMessageOwner = hwndOwner;
    MessageBoxActive = TRUE ;

    return(TRUE);
}


/***************************************************************************\
* PopMessageTimeout
*
* Pops the top timeout from the stack and reenables the timeout for the
* next guy on the stack
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL PopMessageTimeout(VOID)
{
    PopTimeout();

    // Reset our global in case we never went off to search for the
    // message box 'cos we never had to

    TimeoutTitle = NULL;
    MessageBoxActive = FALSE ;

    return(TRUE);
}


/***************************************************************************\
* FindMessageBox
*
* Searches for the message box that our globals tell us we're looking for.
*
* Returns TRUE if found, otherwise FALSE
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL FindMessageBox(VOID)
{
    BOOL Found;

    Found = !EnumWindows(FindEnumfn, 0);

    if (!Found) {
        DebugLog((DEB_ERROR, "Message box window <%ws> not found !!!\n", TimeoutTitle));
    }

    return(Found);
}


/***************************************************************************\
* FindEnumfn
*
* Enumeration function used by FindMessageBox()
*
* Returns FALSE if message box is found, otherwise TRUE
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL WINAPI
FindEnumfn(
    HWND    hwnd,
    LPARAM  lParam)
{
    TCHAR    Title[MAX_STRING_BYTES];

    // See if this hwnd is the one

    Title[ 0 ] = L'\0'; 

    GetWindowText(hwnd, Title, ARRAYSIZE(Title));

    if (lstrcmp(Title, TimeoutTitle) == 0) {

        // Found it

        if (!SetTopTimeout(hwnd)) {
            DebugLog((DEB_ERROR, "failed to set top timeout for message box !!"));
        }

        // Reset our global
        TimeoutTitle = NULL;

        return(FALSE);  // Stop enumeration
    }

    return(TRUE);   // Continue enumeration

    DBG_UNREFERENCED_PARAMETER(lParam);
}


/***************************************************************************\
* InheritTimeout
*
* Converts a timedelay value that is potentially TIMEOUT_CURRENT to
* an independent timedelay value.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

TIMEOUT InheritTimeout(
    TIMEOUT TimeDelay
    )
{
    TIMEOUT ResultantTimeDelay;

    if (TIMEOUT_VALUE(TimeDelay) == TIMEOUT_CURRENT) {
        //
        // Inherit timedelay from previous window
        //
        HWND hwndTop = GetTopTimeout(&ResultantTimeDelay);

        if (!hwndTop)
        {
            DebugLog((DEB_WARN, "Tried to use TIMEOUT_CURRENT with no current timeout\n"));
            ResultantTimeDelay = TIMEOUT_DEFAULT;
        }

        // Don't inherit the TIMEOUT_SS_NOTIFY bit
        ResultantTimeDelay &= TIMEOUT_VALUE_MASK;

        // Set the notify bit to match the one passed in
        ResultantTimeDelay |= TIMEOUT_NOTIFY(TimeDelay);

    } else {
        //
        // No inheritance
        //
        ResultantTimeDelay = TimeDelay;
    }

    return(ResultantTimeDelay);
}


/***************************************************************************\
* PushTimeout
*
* Internal routine to push the timeout information onto a stack
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL PushTimeout(
    HWND    hwnd,
    TIMEOUT    TimeDelay,
    PTERMINAL    pTerm)
{
    // Check we don't have an outstanding unidentified message box
    ASSERTMSG("Last timeout message box not identified yet !", TimeoutTitle == NULL);

    if (NestedIndex < MAX_NESTED_TIMEOUTS) {

// kumarp temporarily disabled this block of code to get rid of 
// the boot break on aphachk11
//#if DBG
#if 0
        // Check we never push a TIMEOUT_NONE dialog when there's
        // a timeout window already on the stack.
        // This would be unusual.

        if (TIMEOUT_VALUE(TimeDelay) == TIMEOUT_NONE) {
            if (NestedIndex > 0) {
                if (TIMEOUT_VALUE(TimeoutArray[NestedIndex-1]) != TIMEOUT_NONE) {
                    ASSERTMSG("Non-timeout window pushed on top of timeout window !!!", FALSE);
                }
            }
        }
#endif
        hwndArray[NestedIndex] = hwnd;
        GlobalsArray[NestedIndex] = pTerm;
        TimeoutArray[NestedIndex++] = TimeDelay;

        StartTopTimeout();

        return(TRUE);

    } else {

        DebugLog((DEB_ERROR, "PushTimeout - Out of input timeout slots !!"));
        return(FALSE);
    }

}


/***************************************************************************\
* PopTimeout
*
* Pops the top timeout from the stack and reenables the timeout for the
* next guy on the stack
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL PopTimeout(VOID)
{
    if (NestedIndex <= 0) {
        DebugLog((DEB_ERROR, "PopTimeout called with an empty timeout stack !!"));
        return(FALSE);
    }

    NestedIndex --;

    // This call will disable timeouts on an empty stack
    StartTopTimeout();

    return(TRUE);
}


/***************************************************************************\
* StartTopTimeout
*
* Starts (or restarts) the timeout on the top of the stack.
* If the stack is empty, timeouts are disabled.
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL StartTopTimeout(VOID)
{
    TIMEOUT    TimeDelay;

    if (NestedIndex > 0) {
        TimeDelay = TimeoutArray[NestedIndex - 1];
        DebugLog((DEB_TRACE_TIMEOUT, "Enabling timeout after %d seconds\n", TIMEOUT_VALUE(TimeDelay)));
    } else {
        DebugLog((DEB_TRACE_TIMEOUT, "Disabling timeouts\n"));
        TimeDelay = TIMEOUT_NONE;   // Disable timeouts
    }

    SetTimeout(TIMEOUT_VALUE(TimeDelay));

    return(TRUE);
}


/***************************************************************************\
* GetTopTimeout
*
* Returns the timeout on the top of the stack.
*
* Returns top hwnd and time-delay or NULL on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

HWND GetTopTimeout(
    PTIMEOUT   pTimeDelay)
{
    HWND    hwndTop = NULL;
    TIMEOUT TimeDelay = 0;

    if (TimeoutTitle != NULL) {
        // Go search for message box before getting the top hwnd
        FindMessageBox();
    }

    if (NestedIndex > 0) {
        hwndTop = hwndArray[NestedIndex - 1];
        TimeDelay = TimeoutArray[NestedIndex - 1];
    }

    if (pTimeDelay != NULL) {
        *pTimeDelay = TimeDelay;
    }

    return(hwndTop);
}

//+---------------------------------------------------------------------------
//
//  Function:   TimeoutUpdateTopTimeout
//
//  Synopsis:   Updates the current timeout, returns FALSE if no window
//              active, TRUE if it was updated.
//
//  Arguments:  [Timeout] --
//
//  History:    3-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
TimeoutUpdateTopTimeout(
    DWORD   Timeout)
{
    if ( DialogActive() )
    {
        if ( TimeoutTitle != NULL )
        {
            FindMessageBox();
        }

        if ( NestedIndex > 0 )
        {
            TimeoutArray[ NestedIndex - 1 ] = Timeout;

            SetTimeout( Timeout );

            return( TRUE );
        }


    }

    return( FALSE );
}



/***************************************************************************\
* SetTopTimeout
*
* Sets the hwnd on top of the timeout stack to the one specified.
* The top timeout value is left unchanged.
*
* Checks if the existing hwnd is NULL, if not this call fails.
*
* Returns TRUE on success, FALSE on failure.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL SetTopTimeout(
    HWND    hwnd)
{
    if (NestedIndex <= 0) {
        DebugLog((DEB_ERROR, "SetTopTimeout : Tried to set the top hwnd on an empty stack !!"));
        return(FALSE);
    }

    if (hwndArray[NestedIndex - 1] != NULL) {
        DebugLog((DEB_ERROR, "SetTopTimeout : hwnd at top of stack is not NULL !!"));
        return(FALSE);
    }

    hwndArray[NestedIndex - 1] = hwnd;

    return(TRUE);
}


/***************************************************************************\
* SetTimeout
*
* Sets an input timeout for the specified window using the specified delay.
*
* If TimeDelay = 0, no timeout is enabled for this window.
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL SetTimeout(
    TIMEOUT    TimeDelay)
{
    if (TimeDelay != 0) {

        if (!InputHooksInstalled) {
            SetInputHooks();
        }
        StartTimer(TimeDelay);

    } else {

        if (InputHooksInstalled) {
            ReleaseInputHooks();
        }

        StopTimer();
    }

    return (TRUE);
}


/***************************************************************************\
* CheckTimeout
*
* Returns TRUE if a timeout is still occurring for this window, otherwise FALSE
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL CheckTimeout(
    HWND    hwnd)
{
    if (GetTopTimeout(NULL) != hwnd) {
        return(FALSE);
    }

    return (TimeoutOccurred);
}


/***************************************************************************\
* StartTimer
*
* Starts (or restarts) the input timer with the specified delay
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL StartTimer(
    TIMEOUT    TimeDelay)
{
    StopTimer();

    TimerID = SetTimer(NULL, TimerID, TimeDelay * 1000, (TIMERPROC)Timerfn);

    return(TimerID != 0);
}


/***************************************************************************\
* StopTimer
*
* Stops the input timer and resets the timeout flag
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL StopTimer(VOID)
{
    if (TimerID != 0) {
        KillTimer(NULL, TimerID);
        TimerID = 0;
    }

    TimeoutOccurred = FALSE;

    return(TRUE);
}


/***************************************************************************\
* Timerfn
*
* Called when the timer goes off.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/
WORD WINAPI Timerfn(
    HWND    hwnd,
    WORD    Message,
    int     IDEvent,
    DWORD   dwTime)
{
    // Set the timeout flag and stop any more timeouts

    TimeoutOccurred = TRUE;

    KillTimer(NULL, TimerID);
    TimerID = 0;

    if (GetTopTimeout(NULL) == NULL) {
        DebugLog((DEB_ERROR, "Input timer went off, but top timeout has NULL hwnd"));
    }

    // Send a message to the window who owns the timeout
    // ForwardMessage(NULL, WM_INPUT_TIMEOUT, FALSE, 0);

    DebugLog((DEB_TRACE_TIMEOUT, "Input timer went off, sending TIMEOUT\n"));
    if (DialogActive())
    {
        EndTopDialog(NULL, WLX_DLG_INPUT_TIMEOUT);
    }
    else
        SASRouter(GlobalsArray[NestedIndex - 1], WLX_SAS_TYPE_TIMEOUT);

    return(0);

    DBG_UNREFERENCED_PARAMETER(hwnd);
    DBG_UNREFERENCED_PARAMETER(Message);
    DBG_UNREFERENCED_PARAMETER(IDEvent);
    DBG_UNREFERENCED_PARAMETER(dwTime);
}


/***************************************************************************\
* SetInputHooks
*
* Installs input hooks
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL SetInputHooks(VOID)
{
    InputHooksInstalled = TRUE;


    hhkMouse = SetWindowsHook(WH_MOUSE, MouseHookfn);
    hhkKeyboard = SetWindowsHook(WH_KEYBOARD, KeyboardHookfn);

    return(TRUE);
}


/***************************************************************************\
* ReleaseInputHooks
*
* Uninstalls input hooks
*
* Returns TRUE on success, FALSE on failure
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL ReleaseInputHooks(VOID)
{
    UnhookWindowsHook(WH_MOUSE, MouseHookfn);
    UnhookWindowsHook(WH_KEYBOARD, KeyboardHookfn);

    InputHooksInstalled = FALSE;

    return(TRUE);
}


/***************************************************************************\
* MouseHookfn
*
* Called when there is mouse input for this app
*
* 12-05-91 Davidc       Created.
\***************************************************************************/
LRESULT WINAPI
MouseHookfn(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (nCode >= 0) {
        if (!TimeoutOccurred) {
            StartTopTimeout();
        }
    }

    return(DefHookProc(nCode, wParam, lParam, &hhkMouse));
}


/***************************************************************************\
* KeyboardHookfn
*
* Called when there is keyboard input for this app
*
* 12-05-91 Davidc       Created.
\***************************************************************************/
LRESULT WINAPI
KeyboardHookfn(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (nCode >= 0) {
        if (!TimeoutOccurred) {
            StartTopTimeout();
        }
    }

    return(DefHookProc(nCode, wParam, lParam, &hhkKeyboard));
}


/***************************************************************************\
* TimeoutMessageBox
*
* Same as a normal message box, but times out if there is no user input
* for the specified number of seconds
* For convenience, this api takes string resource ids rather than string
* pointers as input. The resources are loaded from the .exe module
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

int TimeoutMessageBoxEx(
    PTERMINAL pTerm,
    HWND hwnd,
    UINT IdText,
    UINT IdCaption,
    UINT wType,
    TIMEOUT Timeout)
{
    TCHAR    CaptionBuffer[MAX_STRING_BYTES];
    PTCHAR   Caption = CaptionBuffer;
    TCHAR    Text[MAX_STRING_BYTES];

    LoadString(NULL, IdText, Text, ARRAYSIZE(Text));

    if (IdCaption != 0) {
        LoadString(NULL, IdCaption, Caption, ARRAYSIZE(CaptionBuffer));
    } else {
        Caption = NULL;
    }

    return TimeoutMessageBoxlpstr(pTerm, hwnd, Text, Caption, wType, Timeout);
}


/***************************************************************************\
* TimeoutMessageBoxlpstr
*
* Same as a normal message box, but times out if there is no user input
* for the specified number of seconds
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

int TimeoutMessageBoxlpstr(
    PTERMINAL pTerm,
    HWND hwnd,
    LPTSTR Text,
    LPTSTR Caption,
    UINT wType,
    TIMEOUT Timeout)
{
    int     Result;
    int DlgResult;

#if DBG
    //
    // Reset the return code so if we forget to set if before returning
    // an interrupt MapResultCode will detect it.
    // Also this allows us to check the interrupt type is not overwritten
    // by a second interrupt before we clear it.
    //
    TopDialogReturnCode = DLG_INTERRUPT;
#endif

    // Set up input timeout
    PushMessageTimeout(hwnd, Timeout, Caption, pTerm);

    Result = MessageBox(hwnd, Text, Caption, wType);

    // Re-enable previous timeout
    PopMessageTimeout();

    DlgResult = MapResultCode(Result, TopDialogReturnCode);

#if DBG
    //
    // Reset the stored return code so we can detect if an interrupt is
    // generated without the code being set.
    //

    TopDialogReturnCode = DLG_INTERRUPT;
#endif

    return(DlgResult);
}


/***************************************************************************\
* TimeoutDialogBoxParam
*
* Same as a normal dialog box, but times out if there is no user input
* for the specified number of seconds
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

int
TimeoutDialogBoxParam(
    PTERMINAL pTerm,
    HANDLE hInstance,
    LPTSTR lpTemplateName,
    HWND hwndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    TIMEOUT Timeout)
{
    int Result;
    MSG msg;

#if DBG
    //
    // Reset the return code so if we forget to set if before returning
    // an interrupt MapResultCode will detect it.
    // Also this allows us to check the interrupt type is not overwritten
    // by a second interrupt before we clear it.
    //
    TopDialogReturnCode = DLG_INTERRUPT;
#endif

    while (TRUE) {
        PushDialogTimeout(Timeout, pTerm);

        Result = (int)DialogBoxParam(hInstance,
                                     lpTemplateName,
                                     hwndParent,
                                     lpDialogFunc,
                                     dwInitParam
                                     );
#if DBG
        if (Result < 0)
        {
            if ((DWORD_PTR) lpTemplateName > 0x00010000)
            {
                DebugLog((DEB_WARN, "DialogBoxParam(%#x, %ws, %#x, %#x, %#x) failed, error %d\n",
                            hInstance, lpTemplateName, hwndParent, lpDialogFunc,
                            dwInitParam, GetLastError() ));
            }
            else
            {
                DebugLog((DEB_WARN, "DialogBoxParam(%#x, %#x, %#x, %#x, %#x) failed, error %d\n",
                            hInstance, lpTemplateName, hwndParent, lpDialogFunc,
                            dwInitParam, GetLastError() ));

            }
        }
#endif

        // Re-enable previous timeout
        PopTimeout();

        //
        // If the dialog returned due to WM_QUIT, eat the message
        // and do the dialog again.
        //

        if (!PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE)) {
            break;
        }
    }

    return(Result);
}


/***************************************************************************\
* TimeoutDialogBoxIndirectParam
*
* Same as a normal dialog box, but times out if there is no user input
* for the specified number of seconds
*
* 05-15-92 Davidc       Created.
\***************************************************************************/

int
TimeoutDialogBoxIndirectParam(
    PTERMINAL pTerm,
    HANDLE hInstance,
    LPDLGTEMPLATE Template,
    HWND hwndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    TIMEOUT Timeout)
{
    int Result;
    MSG msg;


    while (TRUE) {
        PushDialogTimeout(Timeout, pTerm);

        Result = (int)DialogBoxIndirectParam(hInstance,
                                             Template,
                                             hwndParent,
                                             lpDialogFunc,
                                             dwInitParam
                                             );

        // Re-enable previous timeout
        PopTimeout();

        //
        // If the dialog returned due to WM_QUIT, eat the message
        // and do the dialog again.
        //

        if (!PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE)) {
            break;
        }
    }

    return(Result);
}



/***************************************************************************\
* EndTopDialog
*
* Interrupts the dialog on top of the timeout stack and causes it to return
* the code specified.
*
* The passed hwnd is that of the caller. This is not used other than as
* a debugging aid.
*
* The dlgresult passed must be one of the interrupt types.
*
* Returns TRUE on success, FALSE on failure.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL EndTopDialog(
    HWND    hwnd,
    int DlgResult
    )
{
    HWND    hwndTop = GetTopTimeout(NULL);

    if (hwndTop == NULL) {
        DebugLog((DEB_ERROR, "EndTopDialog called with no dialogs on the stack!"));
        ASSERT(FALSE);
        return(FALSE);
    }

#ifdef WINLOGON_TEST
    if (hwndTop != hwnd) {
        DebugLog((DEB_ERROR, "Ending top dialog from lower-level dialog!"));
    }
#endif

    //
    // Check the value has not already been set
    //

    ASSERT(TopDialogReturnCode == DLG_INTERRUPT);

    TopDialogReturnCode = DlgResult;

    return (EndDialog(hwndTop, DLG_INTERRUPT));
}

BOOL
KillMessageBox( DWORD SasCode )
{
    DWORD   EndCode;

    //
    // This will kill any outstanding message boxes due to an incoming
    // SAS event.  This is used by SendSasToTopWindow when it is forwarding
    // along an GINA specific SAS.  This kills pending message boxes so that
    // the dialog regains control.
    //

    if ( MessageBoxActive )
    {
        switch ( SasCode )
        {
            case WLX_SAS_TYPE_TIMEOUT:
                EndCode = WLX_DLG_INPUT_TIMEOUT ;
                break;

            case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
                EndCode = WLX_DLG_SCREEN_SAVER_TIMEOUT ;
                break;

            default:
                EndCode = WLX_DLG_SAS;
                break;
        }

        EndTopDialog(NULL, EndCode);
        return(TRUE);
    }
    return(FALSE);
}
