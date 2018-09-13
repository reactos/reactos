/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMSG.C
 *  WOW32 16-bit User Message API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop


MODNAME(wumsg.c);


extern HANDLE hmodWOW32;

// SendDlgItemMessage cache
HWND  hdlgSDIMCached = NULL ;

BOOL fWhoCalled = FALSE;



// DDE bit used in GetMessage and PeekMessage

#define fAckReq 0x8000
#define fRelease 0x2000



/*++
    BOOL CallMsgFilter(<lpMsg>, <nCode>)
    LPMSG <lpMsg>;
    int <nCode>;

    The %CallMsgFilter% function passes the given message and code to the
    current message filter function. The message filter function is an
    application-specified function that examines and modifies all messages. An
    application specifies the function by using the %SetWindowsHook% function.

    <lpMsg>
        Points to an %MSG% structure that contains the message to be
        filtered.

    <nCode>
        Specifies a code used by the filter function to determine how to
        process the message.

    The return value specifies the state of message processing. It is FALSE if
    the message should be processed. It is TRUE if the message should not be
    processed further.

    The %CallMsgFilter% function is usually called by Windows to let
    applications examine and control the flow of messages during internal
    processing in menus and scroll bars or when moving or sizing a window.

    Values given for the <nCode> parameter must not conflict with any of the
    MSGF_ and HC_ values passed by Windows to the message filter function.
--*/

ULONG FASTCALL WU32CallMsgFilter(PVDMFRAME pFrame)
{
    INT   f2;
    ULONG ul;
    MSG t1;
    VPMSG16 vpf1;
    register PCALLMSGFILTER16 parg16;
    MSGPARAMEX mpex;

    GETARGPTR(pFrame, sizeof(CALLMSGFILTER16), parg16);

    vpf1 = (VPMSG16)(parg16->f1);
    f2   = INT32(parg16->f2);

    getmsg16(vpf1, &t1, &mpex);

    // Note: getmsg16 may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    BlockWOWIdle(TRUE);

    ul = GETBOOL16(CallMsgFilter(&t1, f2));

    // Note: Call to CallMsgFilter may have caused 16-bit memory to move

    BlockWOWIdle(FALSE);

    // we need to free the struct ret'd by PackDDElParam in the getmsg16 call
    // (actually the call is made in ThunkWMMsg16() which is called by getmsg16)
    if((t1.message >= WM_DDE_FIRST) && (t1.message <= WM_DDE_LAST)) {
        if(t1.message == WM_DDE_ACK       ||
           t1.message == WM_DDE_DATA      ||
           t1.message == WM_DDE_POKE      ||
           t1.message == WM_DDE_ADVISE )            {

            // make sure this isn't in response to an initiate message
            if(!WI32DDEInitiate((HWND16) mpex.Parm16.WndProc.hwnd)) {
                FreeDDElParam(t1.message, t1.lParam);
            }
        }
    }

    FREEMSG16(vpf1, &t1);

    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    LONG CallWindowProc(<lpPrevWndFunc>, <hwnd>, <wMsg>, <wParam>, <lParam>)
    FARPROC <lpPrevWndFunc>;
    HWND <hwnd>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %CallWindowProc% function passes message information to the function
    specified by the <lpPrevWndFunc> parameter. The %CallWindowProc% function is
    used for window subclassing. Normally, all windows with the same class share
    the same window function. A subclass is a window or set of windows belonging
    to the same window class whose messages are intercepted and processed by
    another function (or functions) before being passed to the window function
    of that class.

    The %SetWindowLong% function creates the subclass by changing the window
    function associated with a particular window, causing Windows to call the
    new window function instead of the previous one. Any messages not processed
    by the new window function must be passed to the previous window function by
    calling %CallWindowProc%. This allows a chain of window functions to be
    created.

    <lpPrevWndFunc>
        Is the procedure-instance address of the previous window function.

    <hwnd>
        Identifies the window that receives the message.

    <wMsg>
        Specifies the message number.

    <wParam>
        Specifies additional message-dependent information.

    <lParam>
        Specifies additional message-dependent information.

    The return value specifies the result of the message processing. The
    possible return values depend on the message sent.
--*/

ULONG FASTCALL WU32CallWindowProc(PVDMFRAME pFrame)
{
    ULONG ul;
    PARM16 Parm16;
    register PCALLWINDOWPROC16 parg16;
    WORD  f2, f3, f4;
    LONG  f5;
    DWORD Proc16;
    DWORD Proc32;
    INT  iMsgThunkClass = 0;

    ul = FALSE;
    GETARGPTR(pFrame, sizeof(CALLWINDOWPROC16), parg16);

    Proc16 = DWORD32(parg16->f1);
    f2     = parg16->f2;
    f3     = WORD32(parg16->f3);
    f4     = WORD32(parg16->f4);
    f5     = LONG32(parg16->f5);

    Proc32 = IsThunkWindowProc(Proc16, &iMsgThunkClass);

    // Note: IsThunkWindowProc may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    if (Proc32) {
        HWND hwnd;
        UINT uMsgNew;
        UINT uParamNew;
        LONG lParamNew;
        MSGPARAMEX mpex;

        mpex.Parm16.WndProc.hwnd   = f2;
        mpex.Parm16.WndProc.wMsg   = f3;
        mpex.Parm16.WndProc.wParam = f4;
        mpex.Parm16.WndProc.lParam = f5;
        mpex.iMsgThunkClass = iMsgThunkClass;

        if (hwnd = ThunkMsg16(&mpex)) {

            // Note: ThunkMsg16 may have caused 16-bit memory movement
            // But: we haven't refreshed them since freeing after IsThunkWindowProc above.
            // FREEARGPTR(pFrame);
            // FREEARGPTR(parg16);

            uMsgNew = mpex.uMsg;
            uParamNew = mpex.uParam;
            lParamNew = mpex.lParam;

            //
            // see comment in IsMDIChild()
            //

            if ((uMsgNew == WM_CREATE || uMsgNew == WM_NCCREATE) && iMsgThunkClass == WOWCLASS_MDICLIENT) {
                    FinishThunkingWMCreateMDI16(lParamNew,
                             (LPCLIENTCREATESTRUCT)((LPCREATESTRUCT)lParamNew + 1));
            }

            BlockWOWIdle(TRUE);

            ul = CallWindowProc((WNDPROC)Proc32, hwnd, uMsgNew,
                                                      uParamNew, lParamNew);
            BlockWOWIdle(FALSE);

            if ((uMsgNew == WM_CREATE || uMsgNew == WM_NCCREATE) && iMsgThunkClass == WOWCLASS_MDICLIENT) {
                StartUnThunkingWMCreateMDI16(lParamNew); // does nothing
            }

            if (MSG16NEEDSTHUNKING(&mpex)) {
                mpex.lReturn = ul;
                (mpex.lpfnUnThunk16)(&mpex);
                ul = mpex.lReturn;
            }
        }
    }
    else {
        Parm16.WndProc.hwnd   = f2;
        Parm16.WndProc.wMsg   = f3;
        Parm16.WndProc.wParam = f4;
        Parm16.WndProc.lParam = f5;
        Parm16.WndProc.hInst  = (WORD)GetWindowLong(HWND32(f2), GWL_HINSTANCE);
        CallBack16(RET_WNDPROC, &Parm16, VPFN32(Proc16), (PVPVOID)&ul);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}






/*++
    LONG DefDlgProc(<hDlg>, <wMsg>, <wParam>, <lParam>)
    HWND <hDlg>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %DefDlgProc% function provides default processing for any Windows
    messages that a dialog box with a private window class does not process.

    All window messages that are not explicitly processed by the window function
    must be passed to the %DefDlgProc% function, not the %DefWindowProc%
    function. This ensures that all messages not handled by their private window
    procedure will be handled properly.

    <hDlg>
        Identifies the dialog box.

    <wMsg>
        Specifies the message number.

    <wParam>
        Specifies 16 bits of additional message-dependent information.

    <lParam>
        Specifies 32 bits of additional message-dependent information.

    The return value specifies the result of the message processing and depends
    on the actual message sent.

    The source code for the %DefDlgProc% function is provided on the SDK disks.

    An application creates a dialog box by calling one of the following
    functions:

    %CreateDialog%
        Creates a modeless dialog box.

    %CreateDialogIndirect%
        Creates a modeless dialog box.

    %CreateDialogIndirectParam%
        Creates a modeless dialog box and passes data to it when it is created.

    %CreateDialogParam%
        Creates a modeless dialog box and passes data to it when it is created.

    %DialogBox%
        Creates a modal dialog box.

    %DialogBoxIndirect%
        Creates a modal dialog box.

    %DialogBoxIndirectParam%
        Creates a modal dialog box and passes data to it when it is created.

    %DialogBoxParam%
        Creates a modal dialog box and passes data to it when it is created.
--*/

ULONG FASTCALL WU32DefDlgProc(PVDMFRAME pFrame)
{
    HWND hdlg;
    MSGPARAMEX mpex;
    register PDEFDLGPROC16 parg16;

    GETARGPTR(pFrame, sizeof(DEFDLGPROC16), parg16);

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = parg16->f1;
    mpex.Parm16.WndProc.wMsg   = WORD32(parg16->f2);
    mpex.Parm16.WndProc.wParam = WORD32(parg16->f3);
    mpex.Parm16.WndProc.lParam = LONG32(parg16->f4);
    mpex.iMsgThunkClass = 0;

    if (hdlg = ThunkMsg16(&mpex)) {

        // Note: ThunkMsg16 may have caused 16-bit memory movement
        FREEARGPTR(pFrame);
        FREEARGPTR(parg16);

        BlockWOWIdle(TRUE);
        mpex.lReturn = DefDlgProc(hdlg, mpex.uMsg, mpex.uParam, mpex.lParam);
        BlockWOWIdle(FALSE);

        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}









/*++
    LONG DefFrameProc(<hwnd>, <hwndMDIClient>, <wMsg>, <wParam>, <lParam>)
    HWND <hwnd>;
    HWND <hwndMDIClient>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %DefFrameProc% function provides default processing for any Windows
    messages that the window function of a multiple document interface (MDI)
    frame window does not process. All window messages that are not explicitly
    processed by the window function must be passed to the %DefFrameProc%
    function, not the %DefWindowProc% function.

    <hwnd>
        Identifies the MDI frame window.

    <hwndMDIClient>
        Identifies the MDI client window.

    <wMsg>
        Specifies the message number.

    <wParam>
        Specifies 16 bits of additional message-dependent information.

    <lParam>
        Specifies 32 bits of additional message-dependent information.

    The return value specifies the result of the message processing and depends
    on the actual message sent. If the <hwndMDIClient> parameter is NULL, the
    return value is the same as for the %DefWindowProc% function.

    Normally, when an application's window procedure does not handle a message,
    it passes the message to the %DefWindowProc% function, which processes the
    message. MDI applications use the %fDefFrameProc% and %DefMDIChildProc%
    functions instead of %DefWindowProc% to provide default message processing.
    All messages that an application would normally pass to %DefWindowProc%
    (such as nonclient messages and WM_SETTEXT) should be passed to
    %DefFrameProc% instead. In addition to these, %DefFrameProc% also handles
    the following messages:

    WM_COMMAND
        The frame window of an MDI application receives the WM_COMMAND message
        to activate a particular MDI child window. The window ID accompanying
        this message will be the ID of the MDI child window assigned by Windows,
        starting with the first ID specified by the application when it created
        the MDI client window. This value of the first ID must not conflict with
        menu-item IDs.

    WM_MENUCHAR
        When the ^ALTHYPHEN^ key is pressed, the control menu of the active MDI
        child window will be selected.

    WM_SETFOCUS
        %DefFrameProc% passes focus on to the MDI client, which in turn passes
        the focus on to the active MDI child window.

    WM_SIZE
        If the frame window procedure passes this message to %DefFrameProc%, the
        MDI client window will be resized to fit in the new client area. If the
        frame window procedure sizes the MDI client to a different size, it
        should not pass the message to %DefWindowProc%.
--*/

ULONG FASTCALL WU32DefFrameProc(PVDMFRAME pFrame)
{
    HWND hwnd, hwnd2;

    MSGPARAMEX mpex;
    register PDEFFRAMEPROC16 parg16;

    GETARGPTR(pFrame, sizeof(DEFFRAMEPROC16), parg16);

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = parg16->f1;
    mpex.Parm16.WndProc.wMsg   = WORD32(parg16->f3);
    mpex.Parm16.WndProc.wParam = WORD32(parg16->f4);
    mpex.Parm16.WndProc.lParam = LONG32(parg16->f5);
    mpex.iMsgThunkClass = 0;

    hwnd2 = HWND32(parg16->f2);

    if (hwnd = ThunkMsg16(&mpex)) {

        // Note: ThunkMsg16 may have caused 16-bit memory movement
        FREEARGPTR(pFrame);
        FREEARGPTR(parg16);

        if (mpex.uMsg == WM_CLIENTSHUTDOWN &&
            CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_IGNORECLIENTSHUTDOWN) {

            //
            // TurboCAD picks up an uninitialized stack variable as the
            // message number to pass to DefFrameProc.  In NT 3.51 it
            // got 0x907 so the call was a NOP.  In NT 4.0, because we
            // now save FS and GS in wow16call, they pick up the x86
            // flat FS, 0x3b, which also happens to be WM_CLIENTSHUTDOWN
            // on NT and Win95, an undocumented message.  DefFrameProc
            // passes the message to DefWindowProc, which does some
            // shutdown related processing which causes TurboCAD to fault
            // soon thereafter.
            //
            // We considered renumbering WM_CLIENTSHUTDOWN, but Win95 uses
            // it as well and some apps may have reverse-engineered the
            // value 3b and depend on seeing the message.
            //
            // So instead we eat the call to DefFrameProc here under
            // a compatibility bit.
            //                            -- DaveHart 31-May-96
            //

            mpex.lReturn = 0;

        } else {

            BlockWOWIdle(TRUE);
            mpex.lReturn = DefFrameProc(hwnd, hwnd2,
                                        mpex.uMsg, mpex.uParam, mpex.lParam);
            BlockWOWIdle(FALSE);
        }

        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}










/*++
    LONG DefMDIChildProc(<hwnd>, <wMsg>, <wParam>, <lParam>)
    HWND <hwnd>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %DefMDIChildProc% function provides default processing for any Windows
    messages that the window function of a multiple document interface (MDI)
    child window does not process. All window messages that are not explicitly
    processed by the window function must be passed to the %DefMDIChildProc%
    function, not the %DefWindowProc% function.

    <hwnd>
        Identifies the MDI child window.

    <wMsg>
        Specifies the message number.

    <wParam>
        Specifies 16 bits of additional message-dependent information.

    <lParam>
        Specifies 32 bits of additional message-dependent information.

    The return value specifies the result of the message processing and depends
    on the actual message sent.

    This function assumes that the parent of the window identified by the <hwnd>
    parameter was created with the MDICLIENT class.

    Normally, when an application's window procedure does not handle a message,
    it passes the message to the %DefWindowProc% function, which processes the
    message. MDI applications use the %DefFrameProc% and %DefMDIChildProc%
    functions instead of %DefWindowProc% to provide default message processing.
    All messages that an application would normally pass to %DefWindowProc%
    (such as nonclient messages and WM_SETTEXT) should be passed to
    %DefMDIChildProc% instead. In addition to these, %DefMDIChildProc% also
    handles the following messages:

    WM_CHILDACTIVATE
        Performs activation processing when child windows are sized, moved, or
        shown. This message must be passed.

    WM_GETMINMAXINFO
        Calculates the size of a maximized MDI child window based on the current
        size of the MDI client window.

    WM_MENUCHAR
        Sends the key to the frame window.

    WM_MOVE
        Recalculates MDI client scroll bars, if they are present.

    WM_SETFOCUS
        Activates the child window if it is not the active MDI child.

    WM_SIZE
        Performs necessary operations when changing the size of a window,
        especially when maximizing or restoring an MDI child window. Failing to
        pass this message to %DefMDIChildProc% will produce highly undesirable
        results.

    WM_SYSCOMMAND
        Also handles the next window command.
--*/

ULONG FASTCALL WU32DefMDIChildProc(PVDMFRAME pFrame)
{
    HWND hwnd;
    register PDEFMDICHILDPROC16 parg16;
    MSGPARAMEX mpex;

    GETARGPTR(pFrame, sizeof(DEFMDICHILDPROC16), parg16);

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = parg16->f1;
    mpex.Parm16.WndProc.wMsg   = WORD32(parg16->f2);
    mpex.Parm16.WndProc.wParam = WORD32(parg16->f3);
    mpex.Parm16.WndProc.lParam = LONG32(parg16->f4);
    mpex.iMsgThunkClass = 0;

    if (hwnd = ThunkMsg16(&mpex)) {

        // Note: ThunkMsg16 may have caused 16-bit memory movement
        FREEARGPTR(pFrame);
        FREEARGPTR(parg16);

        BlockWOWIdle(TRUE);
        mpex.lReturn = DefMDIChildProc(hwnd, mpex.uMsg, mpex.uParam,
                                                                mpex.lParam);
        BlockWOWIdle(FALSE);

        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}










/*++
    LONG DefWindowProc(<hwnd>, <wMsg>, <wParam>, <lParam>)
    HWND <hwnd>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %DefWindowProc% function calls the default window procedure. The
    default window procedure provides default processing for any window messages
    that an application does not process. This function is used to ensure that
    every message is processed. It should be called with the same parameters as
    those received by the window procedure.

    <hwnd>
        Identifies the window that received the message.

    <wMsg>
        Specifies the message.

    <wParam>
        Specifies 16 bits of additional message-dependent information.

    <lParam>
        Specifies 32 bits of additional message-dependent information.

    The return value is dependent on the message that was passed to this
    function.
--*/

ULONG FASTCALL WU32DefWindowProc(PVDMFRAME pFrame)
{
    HWND hwnd;
    register PDEFWINDOWPROC16 parg16;
    MSGPARAMEX mpex;

    GETARGPTR(pFrame, sizeof(DEFWINDOWPROC16), parg16);

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = parg16->hwnd;
    mpex.Parm16.WndProc.wMsg   = WORD32(parg16->wMsg);
    mpex.Parm16.WndProc.wParam = WORD32(parg16->wParam);
    mpex.Parm16.WndProc.lParam = LONG32(parg16->lParam);
    mpex.iMsgThunkClass = 0;

    if (hwnd = ThunkMsg16(&mpex)) {

        // Note: ThunkMsg16 may have caused 16-bit memory movement
        FREEARGPTR(pFrame);
        FREEARGPTR(parg16);

        BlockWOWIdle(TRUE);
        mpex.lReturn = DefWindowProc(hwnd, mpex.uMsg, mpex.uParam, mpex.lParam);
        BlockWOWIdle(FALSE);

        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}










/*++
    LONG DispatchMessage(<lpMsg>)
    LPMSG <lpMsg>;

    The %DispatchMessage% function passes the message in the %MSG% structure
    pointed to by the <lpMsg> parameter to the window function of the specified
    window.

    <lpMsg>
        Points to an %MSG% structure that contains message information from
        the Windows application queue.

        The structure must contain valid message values. If <lpMsg> points to a
        WM_TIMER message and the <lParam> parameter of the WM_TIMER message is
        not NULL, then the <lParam> parameter is the address of a function that
        is called instead of the window function.

    The return value specifies the value returned by the window function. Its
    meaning depends on the message being dispatched, but generally the return
    value is ignored.
--*/

ULONG FASTCALL WU32DispatchMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    WORD  wTDB;
    MSG t1;
    register PDISPATCHMESSAGE16 parg16;
    MSGPARAMEX mpex;

    GETARGPTR(pFrame, sizeof(DISPATCHMESSAGE16), parg16);

    wTDB = pFrame->wTDB;

    getmsg16(parg16->f1, &t1, &mpex);

    // Note: getmsg16 may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    if (CACHENOTEMPTY() && !(CURRENTPTD()->dwWOWCompatFlags & WOWCF_DONTRELEASECACHEDDC)) {

        ReleaseCachedDCs(wTDB, 0, 0, 0, SRCHDC_TASK16);
    }

    BlockWOWIdle(TRUE);

    ul = GETLONG16(DispatchMessage(&t1));

    BlockWOWIdle(FALSE);

    // WARNING Don't rely on any 32 bit flat pointers to 16 bit memory
    // After the dispatchmessage call.

    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    BOOL GetMessage(<lpMsg>, <hwnd>, <wMsgFilterMin>, <wMsgFilterMax>)
    LPMSG <lpMsg>;
    HWND <hwnd>;
    WORD <wMsgFilterMin>;
    WORD <wMsgFilterMax>;

    The %GetMessage% function retrieves a message from the application queue and
    places the message in the structure pointed to by the <lpMsg> parameter. If
    no message is available, the %GetMessage% function yields control to other
    applications until a message becomes available.

    %GetMessage% retrieves only messages associated with the window specified by
    the <hwnd> parameter and within the range of message values given by the
    <wMsgFilterMin> and <wMsgFilterMax> parameters. If <hwnd> is NULL,
    %GetMessage% retrieves messages for any window that belongs to the
    application making the call. (The %GetMessage% function does not retrieve
    messages for windows that belong to other applications.) If <wMsgFilterMin>
    and <wMsgFilterMax> are both zero, %GetMessage% returns all available
    messages (no filtering is performed).

    The constants WM_KEYFIRST and WM_KEYLAST can be used as filter values to
    retrieve all messages related to keyboard input; the constants WM_MOUSEFIRST
    and WM_MOUSELAST can be used to retrieve all mouse-related messages.

    <lpMsg>
        Points to an %MSG% structure that contains message information from
        the Windows application queue.

    <hwnd>
        Identifies the window whose messages are to be examined. If
        <hwnd> is NULL, %GetMessage% retrieves messages for any window that
        belongs to the application making the call.

    <wMsgFilterMin>
        Specifies the integer value of the lowest message value to be
        retrieved.

    <wMsgFilterMax>
        Specifies the integer value of the highest message value to be
        retrieved.

    The return value is TRUE if a message other than WM_QUIT is retrieved. It is
    FALSE if the WM_QUIT message is retrieved.

    The return value is usually used to decide whether to terminate the
    application's main loop and exit the program.

    In addition to yielding control to other applications when no messages are
    available, the %GetMessage% and %PeekMessage% functions also yield control
    when WM_PAINT or WM_TIMER messages for other tasks are available.

    The %GetMessage%, %PeekMessage%, and %WaitMessage% functions are the only
    ways to let other applications run. If your application does not call any of
    these functions for long periods of time, other applications cannot run.

    When %GetMessage%, %PeekMessage%, and %WaitMessage% yield control to other
    applications, the stack and data segments of the application calling the
    function may move in memory to accommodate the changing memory requirements
    of other applications. If the application has stored long pointers to
    objects in the data or stack segment (that is, global or local variables),
    these pointers can become invalid after a call to %GetMessage%,
    %PeekMessage%, or %WaitMessage%. The <lpMsg> parameter of the called
    function remains valid in any case.
--*/

ULONG FASTCALL WU32GetMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    MSG t1;
    VPMSG16  vpMsg;
    register PGETMESSAGE16 parg16;
    ULONG ulReturn;

    BlockWOWIdle(TRUE);

// NOTE: pFrame needs to be restored on all GOTO's to get_next_dde_message
get_next_dde_message:

    GETARGPTR(pFrame, sizeof(GETMESSAGE16), parg16);

    vpMsg = parg16->vpMsg;


    ul = GETBOOL16(GetMessage(&t1,
                              HWND32(parg16->hwnd),
                              WORD32(parg16->wMin),
                              WORD32(parg16->wMax)));

    // There Could have been a Task Switch Before GetMessage Returned so
    // Don't Trust any 32 bit flat pointers we have, memory could've been
    // compacted or moved.
    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);


#ifdef DEBUG
    if (t1.message == WM_TIMER) {
        WOW32ASSERT(HIWORD(t1.wParam) == 0);
    }
#endif

    ulReturn = putmsg16(vpMsg, &t1);

    // NOTE: Call to putmsg16 could've caused 16-bit memory movement

    if (((t1.message == WM_DDE_DATA) || (t1.message == WM_DDE_POKE)) && (!ulReturn)) {
        register PMSG16 pmsg16;
        DDEDATA *lpMem32;
        WORD Status;
        UINT dd;
        WORD ww;
        char szMsgBoxText[1024];
        char szCaption[256];

        GETVDMPTR(vpMsg, sizeof(MSG16), pmsg16);

        dd = FETCHDWORD(pmsg16->lParam);
        ww = FETCHWORD(pmsg16->wParam);

        lpMem32 = GlobalLock((HGLOBAL)dd);
        Status = (*((PWORD) lpMem32));
        GlobalUnlock((HGLOBAL)dd);

        (pfnOut.pfnFreeDDEData)((HANDLE)dd, TRUE, TRUE);

        GlobalDeleteAtom (ww);

        if ((Status & fAckReq) || (t1.message == WM_DDE_POKE)) {
            LoadString(hmodWOW32, iszOLEMemAllocFailedFatal, szMsgBoxText, sizeof szMsgBoxText);
            LoadString(hmodWOW32, iszSystemError, szCaption, sizeof szCaption);
            MessageBox(t1.hwnd, (LPCTSTR) szMsgBoxText, szCaption, MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            PostMessage((HWND) t1.wParam, WM_DDE_TERMINATE, (WPARAM)FULLHWND32((WORD)t1.hwnd), (LPARAM)0l);
        }
        else {
            LoadString(hmodWOW32, iszOLEMemAllocFailed, szMsgBoxText, sizeof szMsgBoxText);
            LoadString(hmodWOW32, iszSystemError, szCaption, sizeof szCaption);
            MessageBox(t1.hwnd, (LPCTSTR) szMsgBoxText, szCaption, MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        }

        FREEVDMPTR(pmsg16);

        // restore the frame ptr due to possible 16-bit memory movement
        GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);

        goto get_next_dde_message;
    }

    BlockWOWIdle(FALSE);

    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);
    RETURN(ul);
}







/*++
    DWORD GetMessagePos(VOID)

    The %GetMessagePos% function returns a long value that represents the cursor
    position (in screen coordinates) when the last message obtained by the
    %GetMessage% function occurred.

    This function has no parameters.

    The return value specifies the <x>- and <y>-coordinates of the cursor
    position. The <x>-coordinate is in the low-order word, and the
    <y>-coordinate is in the high-order word. If the return value is assigned to
    a variable, the %MAKEPOINT% macro can be used to obtain a %POINT% structure
    from the return value; the %LOWORD% or %HIWORD% macro can be used to extract
    the <x>- or the <y>-coordinate.

    To obtain the current position of the cursor instead of the position when
    the last message occurred, use the %GetCursorPos% function.
--*/

ULONG FASTCALL WU32GetMessagePos(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETDWORD16(GetMessagePos());

    RETURN(ul);
}







/*++
    DWORD GetMessageTime(VOID)

    The %GetMessageTime% function returns the message time for the last message
    retrieved by the %GetMessage% function. The time is a long integer that
    specifies the elapsed time (in milliseconds) from the time the system was
    booted to the time the message was created (placed in the application
    queue).

    This function has no parameters.

    The return value specifies the message time.

    Do not assume that the return value is always increasing. The return value
    will wrap around to zero if the timer count exceeds the maximum value for
    long integers.

    To calculate time delays between messages, subtract the time of the second
    message from the time of the first message.
--*/

ULONG FASTCALL WU32GetMessageTime(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETLONG16(GetMessageTime());

    RETURN(ul);
}







/*++
    BOOL InSendMessage(VOID)

    The %InSendMessage% function specifies whether the current window function
    is processing a message that is passed to it through a call to the
    %SendMessage% function.

    This function has no parameters.

    The return value specifies the outcome of the function. It is TRUE if the
    window function is processing a message sent to it with %SendMessage%.
    Otherwise, it is FALSE.

    Applications use the %InSendMessage% function to determine how to handle
    errors that occur when an inactive window processes messages. For example,
    if the active window uses %SendMessage% to send a request for information to
    another window, the other window cannot become active until it returns
    control from the %SendMessage% call. The only method an inactive window has
    to inform the user of an error is to create a message box.
--*/

ULONG FASTCALL WU32InSendMessage(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETBOOL16(InSendMessage());

    RETURN(ul);
}







/*++
    BOOL PeekMessage(<lpMsg>, <hwnd>, <wMsgFilterMin>, <wMsgFilterMax>,
        <wRemoveMsg>)
    LPMSG <lpMsg>;
    HWND <hwnd>;
    WORD <wMsgFilterMin>;
    WORD <wMsgFilterMax>;
    WORD <wRemoveMsg>;

    The %PeekMessage% function checks the application queue for a message and
    places the message (if any) in the structure pointed to by the <lpMsg>
    parameter. Unlike the %GetMessage% function, the %PeekMessage% function does
    not wait for a message to be placed in the queue before returning. It does,
    however, yield control (if the PM_NOYIELD flag isn't set) and does not
    return control after the yield until Windows returns control to the
    application.

    %PeekMessage% retrieves only messages associated with the window specified
    by the <hwnd> parameter, or any of its children as specified by the
    %IsChild% function, and within the range of message values given by the
    <wMsgFilterMin> and <wMsgFilterMax> parameters. If <hwnd> is NULL,
    %PeekMessage% retrieves messages for any window that belongs to the
    application making the call. (The %PeekMessage% function does not retrieve
    messages for windows that belong to other applications.) If <hwnd> is -1,
    %PeekMessage% returns only messages with a <hwnd> of NULL as posted by the
    %PostAppMessage% function. If <wMsgFilterMin> and <wMsgFilterMax> are both
    zero, %PeekMessage% returns all available messages (no range filtering is
    performed).

    The WM_KEYFIRST and WM_KEYLAST flags can be used as filter values to
    retrieve all key messages; the WM_MOUSEFIRST and WM_MOUSELAST flags can be
    used to retrieve all mouse messages.

    <lpMsg>
        Points to an %MSG% structure that contains message information from
        the Windows application queue.

    <hwnd>
        Identifies the window whose messages are to be examined.

    <wMsgFilterMin>
        Specifies the value of the lowest message position to be
        examined.

    <wMsgFilterMax>
        Specifies the value of the highest message position to be
        examined.

    <wRemoveMsg>
        Specifies a combination of the flags described in the following
        list. PM_NOYIELD can be combined with either PM_NOREMOVE or PM_REMOVE:

    PM_NOREMOVE
        Messages are not removed from the queue after processing by
        PeekMessage.

    PM_NOYIELD
        Prevents the current task from halting and yielding system resources to
        another task.

    PM_REMOVE
        Messages are removed from the queue after processing by %PeekMessage%.

    The return value specifies whether or not a message is found. It is TRUE if
    a message is available. Otherwise, it is FALSE.

    %PeekMessage% does not remove WM_PAINT messages from the queue. The messages
    remain in the queue until processed. The %GetMessage%, %PeekMessage%, and
    %WaitMessage% functions yield control to other applications. These calls are
    the only way to let other applications run. If your application does not
    call any of these functions for long periods of time, other applications
    cannot run.

    When %GetMessage%, %PeekMessage%, and %WaitMessage% yield control to other
    applications, the stack and data segments of the application calling the
    function may move in memory to accommodate the changing memory requirements
    of other applications.

    If the application has stored long pointers to objects in the data or stack
    segment (global or local variables), and if they are unlocked, these
    pointers can become invalid after a call to %GetMessage%, %PeekMessage%, or
    %WaitMessage%. The <lpMsg> parameter of the called function remains valid in
    any case.
--*/

ULONG FASTCALL WU32PeekMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    VPMSG16 vpf1;
    HANDLE  f2;
    WORD    f3, f4, f5;
    MSG t1;
    register PPEEKMESSAGE16 parg16;
    BOOL fNoYield;

    BlockWOWIdle(TRUE);

// NOTE: pFrame needs to be restored on all GOTO's to get_next_dde_message
get_next_dde_message:

    GETARGPTR(pFrame, sizeof(PEEKMESSAGE16), parg16);

    vpf1 = parg16->f1;
    f2   = HWND32(parg16->f2);
    f3   = WORD32(parg16->f3);
    f4   = WORD32(parg16->f4);
    f5   = parg16->f5;

    fNoYield = f5 & PM_NOYIELD;

    ul = GETBOOL16(PeekMessage(&t1, f2, f3, f4, f5));

    // There could've been a task switch before peekmessage returned
    // so Don't trust any 32 bit flat pointers we have, memory could
    // have been compacted or moved.
    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);

#ifdef DEBUG
    if (ul && t1.message == WM_TIMER) {
        WOW32ASSERT(HIWORD(t1.wParam) == 0);
    }
#endif

    // If PeekMessage returned NULL don't bother to copy anything back

    if (ul) {
        ULONG ulReturn;

        //
        // We need to set/reset fThunkDDEmsg (based on PM_REMOVE flag)
        // so that we know whether to call FreeDDElParam or not while
        // thunking 32 bit message to 16 bit message.
        //

        fThunkDDEmsg = (BOOL) (f5 & PM_REMOVE);
        ulReturn = putmsg16(vpf1, &t1);

        // There Could've been a Task Switch Before putmsg16 Returned so Don't
        // Trust any 32 bit flat pointers we have, memory could have been
        // compacted or moved.
        FREEARGPTR(parg16);
        FREEVDMPTR(pFrame);

        fThunkDDEmsg = TRUE;

        if (((t1.message == WM_DDE_DATA) || (t1.message == WM_DDE_POKE)) && (!ulReturn)) {
            register PMSG16 pmsg16;
            DDEDATA *lpMem32;
            WORD Status;
            UINT dd;
            WORD ww;
            char szMsgBoxText[1024];
            char szCaption[256];

            GETVDMPTR(vpf1, sizeof(MSG16), pmsg16);

            dd = FETCHDWORD(pmsg16->lParam);
            ww = FETCHWORD(pmsg16->wParam);

            lpMem32 = GlobalLock((HGLOBAL)dd);
            Status = (*((PWORD) lpMem32));
            GlobalUnlock((HGLOBAL)dd);

            (pfnOut.pfnFreeDDEData)((HANDLE)dd, TRUE, TRUE);

            GlobalDeleteAtom (ww);

            if (!(f5 & PM_REMOVE)) {

                ul = GETBOOL16(PeekMessage(&t1, f2, f3, f4, f5 | PM_REMOVE));

                // There could've been a task switch before peekmessage returned
                // so Don't trust any 32 bit flat pointers we have, memory could
                // have been compacted or moved.
                FREEARGPTR(parg16);
                FREEVDMPTR(pFrame);
                FREEVDMPTR(pmsg16);

                // uncomment if parg16 is ref'd before goto get_next_dde_message
                //GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
                //GETARGPTR(pFrame, sizeof(PEEKMESSAGE16), parg16);

                // uncomment if pmsg16 is ref'd before goto get_next_dde_message
                //GETVDMPTR(vpf1, sizeof(MSG16), pmsg16);
            }

            if ((Status & fAckReq) || (t1.message == WM_DDE_POKE)) {

                LoadString(hmodWOW32, iszOLEMemAllocFailedFatal, szMsgBoxText, sizeof szMsgBoxText);
                LoadString(hmodWOW32, iszSystemError, szCaption, sizeof szCaption);
                MessageBox(t1.hwnd, (LPCTSTR) szMsgBoxText, szCaption, MB_OK);
                PostMessage ((HWND) t1.wParam, WM_DDE_TERMINATE, (WPARAM)FULLHWND32((WORD)t1.hwnd), (LPARAM)0l);
            }
            else {
                LoadString(hmodWOW32, iszOLEMemAllocFailed, szMsgBoxText, sizeof szMsgBoxText);
                LoadString(hmodWOW32, iszSystemError, szCaption, sizeof szCaption);
                MessageBox(t1.hwnd, (LPCTSTR) szMsgBoxText, szCaption, MB_OK);
            }

            FREEVDMPTR(pmsg16);

            // restore the frame ptr due to possible 16-bit memory movement
            GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);

            goto get_next_dde_message;
        }
    }
    else if (fNoYield && (CURRENTPTD()->dwWOWCompatFlags & WOWCF_SETNULLMESSAGE)) {

        // winproj (help.tutorial) calls peekmessage with PM_REMOVE and
        // PM_NOYIELD  and an lpmsg whose contents are uninitialized. However
        // even if peekmessage returns false, it checks if lpmsg->message is
        // WM_QUIT and if true exits. In WOW by pure coincidence the
        // unintialized lpmsg->message happens to be value 0x12, which is
        // WM_QUIT and thus the tutorial always exits after initialization.
        //
        // So we reset lpmsg->message to zero, if it was called with PM_NOYIELD
        // and if it happens to be WM_QUIT and if peekmessage returns zero.
        //
        //                                                       - nanduri

        // we don't need to reinitialize pFrame etc. 'cause peekmessage was
        // called with PM_NOYIELD and thus the 16bit memory couldn't have moved

        register PMSG16 pmsg16;
        GETVDMPTR(vpf1, sizeof(MSG16), pmsg16);
        if (pmsg16 && (pmsg16->message == WM_QUIT)) {
            pmsg16->message = 0;
        }
        FREEVDMPTR(pmsg16);
    }


    BlockWOWIdle(FALSE);

    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);
    RETURN(ul);
}







/*++
    BOOL PostAppMessage(<hTask>, <wMsg>, <wParam>, <lParam>)
    HANDLE <hTask>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %PostAppMessage% function posts a message to an application identified
    by a task handle, and then returns without waiting for the application to
    process the message. The application receiving the message obtains the
    message by calling the %GetMessage% or %PeekMessage% function. The <hwnd>
    parameter of the returned %MSG% structure is NULL.

    <hTask>
        Identifies the task that is to receive the message. The
        %GetCurrentTask% function returns this handle.

    <wMsg>
        Specifies the type of message posted.

    <wParam>
        Specifies additional message information.

    <lParam>
        Specifies additional message information.

    The return value specifies whether or not the message is posted. It is
    TRUE if the message is posted. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32PostAppMessage(PVDMFRAME pFrame)
{
    register PPOSTAPPMESSAGE16 parg16;
    DWORD    f1;
    MSGPARAMEX mpex;

    GETARGPTR(pFrame, sizeof(POSTAPPMESSAGE16), parg16);

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = 0;
    mpex.Parm16.WndProc.wMsg   = WORD32(parg16->f2);
    mpex.Parm16.WndProc.wParam = WORD32(parg16->f3);
    mpex.Parm16.WndProc.lParam = LONG32(parg16->f4);
    mpex.iMsgThunkClass = 0;

    f1 = THREADID32(parg16->f1);

    ThunkMsg16(&mpex);

    // Note: ThunkMsg16 may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    mpex.lReturn = PostThreadMessage(f1, mpex.uMsg, mpex.uParam, mpex.lParam);

    if (MSG16NEEDSTHUNKING(&mpex)) {
        (mpex.lpfnUnThunk16)(&mpex);
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}







/*++
    BOOL PostMessage(<hwnd>, <msg>, <wParam>, <lParam>)
    HWND <hwnd>;
    WORD <msg>;
    WORD <wParam>;
    LONG <lParam>;

    The %PostMessage% function places a message in a window's application queue,
    and then returns without waiting for the corresponding window to process the
    message. Messages in a message queue are retrieved by calls to the
    %GetMessage% or %PeekMessage% function.

    .*
    .* DA's: the following parameters section should be identical to the
    .*       parameters section in the sdmsg.ref file. If there is a change
    .*       to this section, the identical change should be made in the other
    .*       file.
    .*

    <hwnd>
        Identifies the window that is to receive the message. If this parameter
        is 0xFFFF (-1), the message is sent to all top-level windows.

    <msg>
        Specifies the message to be sent.

    <wParam>
        Specifies additional message information. The contents of this
        parameter depends on the message being sent.

    <lParam>
        Specifies additional message information. The contents of this
        parameter depends on the message being sent.

    The return value is TRUE if the message is posted, or FALSE if it is not.

    An application should never use the %PostMessage% function to send a message
    to a control.

    .cmt
    27-Oct-1990 [ralphw]

    The following is a rewording of the previous documentation. However, it
    needs confirmation from development as to its technical accuracy before it
    can be released for public consumption.

    If the message is being sent to another application, and the <wParam> or
    <lParam> parameters are used to pass a handle or pointer to global memory,
    the memory should be allocated by the %GlobalAlloc% function using the
    GMEM_NOT_BANKED flag. In a system using expanded memory (EMS), this ensures
    that the memory is not in in a different bank of memory from the application
    using the memory.
    .endcmt
--*/

ULONG FASTCALL WU32PostMessage(PVDMFRAME pFrame)
{
    LONG l;
    UINT f2;
    WPARAM f3;
    LPARAM f4;
    HWND hwnd;
    register PPOSTMESSAGE16 parg16;
    MSGPARAMEX mpex;
    DWORD err = NO_ERROR;

    GETARGPTR(pFrame, sizeof(POSTMESSAGE16), parg16);

    // Apps should never use PostMessage to post messages that have
    // pointers to structures, because those messages will show up in
    // GetMessage, and if GetMessage tries to thunk them (ie, tries to
    // call back to the 16-bit kernel to allocate some 16-bit memory to
    // copy the converted 32-bit structure into), we have no way of
    // knowing when to free that 16-bit memory.
    //
    // BUGBUG 22-Aug-91 JeffPar:  a flag should be added to ThunkMsg16
    // indicating whether or not such allocations are permissible;  this
    // flag should be passed on to all the ThunkXXMsg16 subfunctions,
    // and each of those subfunctions should assert the flag is false
    // whenever allocating 16-bit memory.


    //
    // Used by 16->32 DDE thunkers.
    //

    WOW32ASSERT(fWhoCalled == FALSE);
    fWhoCalled = WOWDDE_POSTMESSAGE;

    f2 = (UINT)WORD32(parg16->f2);
    f3 = (WPARAM)(WORD32(parg16->f3));
    f4 = (LPARAM)(LONG32(parg16->f4));

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = parg16->f1;
    mpex.Parm16.WndProc.wMsg   = (WORD)f2;
    mpex.Parm16.WndProc.wParam = (WORD)f3;
    mpex.Parm16.WndProc.lParam = f4;
    mpex.iMsgThunkClass = 0;

    // The Reader.exe shipped with Lotus 123MM version has a message
    // synchronization problem.  Force proper synchronization by
    // converting this PostMessage call to a SendMessage().
    if ((f2 == WM_VSCROLL) &&
         ((f3 == SB_THUMBTRACK) || (f3 == SB_THUMBPOSITION)) &&
         (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SENDPOSTEDMSG) ) {

        l = (LONG)WU32SendMessage(pFrame);
        FREEARGPTR(parg16);
        RETURN((ULONG) l);
    }

    hwnd = ThunkMsg16(&mpex);

    // Note: ThunkMsg16 may have caused 16-bit memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    WOW32ASSERT(fWhoCalled == WOWDDE_POSTMESSAGE);
    fWhoCalled = FALSE;
    if (hwnd) {

        l = PostMessage(hwnd, mpex.uMsg, mpex.uParam, mpex.lParam);

        if (!l)
            err = GetLastError();

        mpex.lReturn = l;
        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }


        // If the post message failed, then the message was probably one
        // that has pointers and therefore can not be posted. (MetaDesign
        // tries to post these kind of messages.) If the destination was a
        // WOW app, then make it into a private message, and try the post
        // again.  We don't have to worry about thunking since both the source
        // and destination are in the WOW address space.

        if (err == ERROR_INVALID_PARAMETER) {
            PWW   pww;
            DWORD dwpid;

            pww = FindPWW(hwnd);

            // was added for WM_DRAWITEM messages which are probably intended
            // for owner drawn std-type classes.  see bug #2047 NTBUG4
            if (pww != NULL && GETICLASS(pww, hwnd) != WOWCLASS_WIN16) {

                // make sure we're in the same vdm process
                if (!(GetWindowThreadProcessId(hwnd, &dwpid) &&
                      (dwpid == GetCurrentProcessId()))) {
                          return 0;
                }

                mpex.lReturn = PostMessage(hwnd, f2 | WOWPRIVATEMSG, f3, f4);
            }
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}







/*++
    void PostQuitMessage(<nExitCode>)
    int <nExitCode>;

    The %PostQuitMessage% function informs Windows that the application wishes
    to terminate execution. It is typically used in response to a WM_DESTROY
    message.

    The %PostQuitMessage% function posts a WM_QUIT message to the application
    and returns immediately; the function merely informs the system that the
    application wants to quit sometime in the future.

    When the application receives the WM_QUIT message, it should exit the
    message loop in the main function and return control to Windows. The exit
    code returned to Windows must be the <wParam> parameter of the WM_QUIT
    message.

    <nExitCode>
        Specifies an application exit code. It is used as the wParam parameter
        of the WM_QUIT message.

    This function does not return a value.
--*/

ULONG FASTCALL WU32PostQuitMessage(PVDMFRAME pFrame)
{
    register PPOSTQUITMESSAGE16 parg16;

    GETARGPTR(pFrame, sizeof(POSTQUITMESSAGE16), parg16);

    PostQuitMessage(INT32(parg16->wExitCode));

    FREEARGPTR(parg16);
    RETURN(0);
}







/*++
    WORD RegisterWindowMessage(<lpString>)
    LPSTR <lpString>;

    This function defines a new window message that is guaranteed to be unique
    throughout the system. The returned message value can be used when calling
    the %SendMessage% or %PostMessage% function.

    %RegisterWindowMessage% is typically used for communication between two
    cooperating applications.

    If the same message string is registered by two different applications, the
    same message value is returned. The message remains registered until the
    user ends the Windows session.

    <lpString>
        Points to the message string to be registered.

    The return value specifies the outcome of the function. It is an unsigned
    short integer within the range 0xC000 to 0xFFFF if the message is
    successfully registered. Otherwise, it is zero.

    Use the %RegisterWindowMessage% function only when the same message must be
    understood by more than one application. For sending private messages within
    an application, an application can use any integer within the range WM_USER
    to 0xBFFF.
--*/

ULONG FASTCALL WU32RegisterWindowMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PREGISTERWINDOWMESSAGE16 parg16;

    GETARGPTR(pFrame, sizeof(REGISTERWINDOWMESSAGE16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    ul = GETWORD16(RegisterWindowMessage(psz1));

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    void ReplyMessage(<lReply>)
    LONG <lReply>;

    The %ReplyMessage% function is used to reply to a message sent through the
    %SendMessage% function without returning control to the function that called
    %SendMessage.%

    By calling this function, the window function that receives the message
    allows the task that called %SendMessage% to continue to execute as though
    the task that received the message had returned control. The task that calls
    %ReplyMessage% also continues to execute.

    Normally a task that calls %SendMessage% to send a message to another task
    will not continue executing until the window procedure that Windows calls to
    receive the message returns. However, if a task that is called to receive a
    message needs to perform some type of operation that might yield control
    (such as calling the %MessageBox% or %DialogBox% functions), Windows could
    be placed in a deadlock situation where the sending task needs to execute
    and process messages but cannot because it is waiting for %SendMessage% to
    return. An application can avoid this problem if the task receiving the
    message calls %ReplyMessage% before performing any operation that could
    cause the task to yield.

    The %ReplyMessage% function has no effect if the message was not sent
    through the %SendMessage% function or if the message was sent by the same
    task.

    <lReply>
        Specifies the result of the message processing. The possible values
        depend on the actual message sent.

    This function does not return a value.
--*/

ULONG FASTCALL WU32ReplyMessage(PVDMFRAME pFrame)
{
    register PREPLYMESSAGE16 parg16;

    GETARGPTR(pFrame, sizeof(REPLYMESSAGE16), parg16);

    ReplyMessage(LONG32(parg16->f1));

    // WARNING - Don't use any 32 bit flat pointers after call to ReplyMessage,
    //           other tasks might have run and made the pointers invalid.

    FREEARGPTR(parg16);
    RETURN(0);
}







/*++
    DWORD SendDlgItemMessage(<hDlg>, <nIDDlgItem>, <wMsg>, <wParam>, <lParam>)
    HWND <hDlg>;
    int <nIDDlgItem>;
    WORD <wMsg>;
    WORD <wParam>;
    DWORD <lParam>;

    The %SendDlgItemMessage% function sends a message to the control specified
    by the <nIDDlgItem> parameter within the dialog box specified by the <hDlg>
    parameter. The %SendDlgItemMessage% function does not return until the
    message has been processed.

    <hDlg>
        Identifies the dialog box that contains the control.

    <nIDDlgItem>
        Specifies the integer identifier of the dialog item that is to
        receive the message.

    <wMsg>
        Specifies the message value.

    <wParam>
        Specifies additional message information.

    <lParam>
        Specifies additional message information.

    The return value specifies the outcome of the function. It is the value
    returned by the control's window function, or zero if the control identifier
    is not valid.

    Using %SendDlgItemMessage% is identical to obtaining a handle to the given
    control and calling the %SendMessage% function.
--*/

#define W31EM_GETRECT (WM_USER+2)  // w31 EM_GETRECT != NT EM_GETRECT

ULONG FASTCALL WU32SendDlgItemMessage(PVDMFRAME pFrame)
{
    HWND hdlg, hwndItem, hwnd;
    register PSENDDLGITEMMESSAGE16 parg16;
    MSGPARAMEX mpex;

static HWND  hwndCached = NULL ;
static DWORD dwCachedItem = 0L ;

    GETARGPTR(pFrame, sizeof(SENDDLGITEMMESSAGE16), parg16);

    // QuarkExpress v3.31 passes a hard coded 7fff:0000 as the pointer to the
    // RECT struct for EM_GETRECT message - W3.1 rejects it in validation layer
    if( (DWORD32(parg16->f5) == 0x7FFF0000)    &&
        (WORD32(parg16->f3) == W31EM_GETRECT)  &&
        (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_BOGUSPOINTER) ) {

        FREEARGPTR(parg16);
        RETURN((ULONG)0);
    }

    // Need unique handle
    hdlg = (HWND)FULLHWND32(parg16->f1);

    //
    // Caching the hwnd for the dialog item because EForm will
    // call SendDlgItemMessage in a tight loop.
    //
    if ( hdlg == hdlgSDIMCached && WORD32(parg16->f2) == dwCachedItem ) {

        // Set from cached
        hwndItem = hwndCached ;
    }
    else {
        if ( hwndItem = GetDlgItem(hdlg, WORD32(parg16->f2)) ) {

            // and cache needed information
            hdlgSDIMCached     = hdlg ;
            hwndCached         = hwndItem ;
            dwCachedItem       = WORD32(parg16->f2) ;
        }
        else {
            FREEARGPTR(parg16);
            RETURN((ULONG)0);
        }
    }

    mpex.lReturn = 0;
    if (hwndItem) {
        mpex.Parm16.WndProc.hwnd   = GETHWND16(hwndItem);
        mpex.Parm16.WndProc.wMsg   = WORD32(parg16->f3);
        mpex.Parm16.WndProc.wParam = WORD32(parg16->f4);
        mpex.Parm16.WndProc.lParam = LONG32(parg16->f5);
        mpex.iMsgThunkClass = 0;

        if (hwnd = ThunkMsg16(&mpex)) {

            // Note: ThunkMsg16 may have caused memory movement
            FREEARGPTR(pFrame);
            FREEARGPTR(parg16);

            /*
            ** Since we already know which window the message is going to
            ** don't make USER32 look it up again. - MarkRi
            */
            mpex.lReturn = SendMessage(hwndItem, mpex.uMsg, mpex.uParam,
                                                                mpex.lParam);
            // to keep common dialog structs in sync (see wcommdlg.c)
            Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd,
                                (VPVOID)mpex.Parm16.WndProc.lParam);

            if (MSG16NEEDSTHUNKING(&mpex)) {
                (mpex.lpfnUnThunk16)(&mpex);
            }
        }
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}







/*++
    DWORD SendMessage(<hwnd>, <msg>, <wParam>, <lParam>)
    HWND <hwnd>;
    WORD <msg>;
    WORD <wParam>;
    LONG <lParam>;

    The %SendMessage% function sends a message to a window or windows. The
    %SendMessage% function calls the window procedure for the specified window,
    and does not return until that window procedure has processed the message.
    This is in contrast to the %PostMessage% function which places the message
    into the specified window's message queue and returns immediately.

    <hwnd>
        Identifies the window that is to receive the message. If this parameter
        is 0xFFFF (-1), the message is sent to all top-level windows.

    <msg>
        Specifies the message to be sent.

    <wParam>
        Specifies additional message information. The contents of this
        parameter depends on the message being sent.

    <lParam>
        Specifies additional message information. The contents of this
        parameter depends on the message being sent.

    The return value is the result returned by the invoked window procedure; its
    value depends on the message being sent.
--*/

ULONG FASTCALL WU32SendMessage(PVDMFRAME pFrame)
{
    // NOTE: This can be called directly by WU32PostMessage!!!

    HWND hwnd;
    register PSENDMESSAGE16 parg16;
    MSGPARAMEX mpex;
    HWND16 hwndOld;
    UINT uMsgOld;
    UINT uParamOld;
    LONG lParamOld;
#ifdef DBCS
    HMEM16 hMem16;
    LPSZ lpBuf16,lpBuf32;
#endif // DBCS

    GETARGPTR(pFrame, sizeof(SENDMESSAGE16), parg16);

    hwndOld   = parg16->f1;
    uMsgOld   = WORD32(parg16->f2);
    uParamOld = WORD32(parg16->f3);
    lParamOld = LONG32(parg16->f4);

    //
    // Check for funky apps sending WM_SYSCOMMAND - SC_CLOSE to progman
    //
    if ( uMsgOld == WM_SYSCOMMAND && uParamOld == SC_CLOSE ) {
        if ( hwndOld == GETHWND16(hwndProgman) && hwndProgman != (HWND)0 ) {
            //
            // Now if shift key is down, they must be trying to save
            // settings in progman.
            //
            if ( GetKeyState( VK_SHIFT ) < 0 ) {
                uMsgOld = RegisterWindowMessage("SaveSettings");
            }
        }
    }


    //
    // This is for the apps that use DDE protocol wrongly, like AmiPro.
    //

    WOW32ASSERT(fWhoCalled == FALSE);
    fWhoCalled = WOWDDE_POSTMESSAGE;

    mpex.lReturn = 0;
    mpex.Parm16.WndProc.hwnd   = hwndOld;
    mpex.Parm16.WndProc.wMsg   = (WORD)uMsgOld;
#ifdef DBCS
    //
    // For WIN3.1J's BUG ?
    // SendMessage( hwnd, WM_GETTEXT, 2, lpBuffer )
    // if string is DBCS, return is DBCS-leadbyte.
    // KKSUZUKA:#1731
    // 1994.8.8 add by V-HIDEKK
    //
    if( uMsgOld == WM_GETTEXT && uParamOld == 2 ){
        mpex.Parm16.WndProc.wParam = (WORD)(uParamOld + 1);
        mpex.Parm16.WndProc.lParam = GlobalAllocLock16( GMEM_SHARE | GMEM_MOVEABLE, uParamOld +1, &hMem16 );
    }
    else {
        mpex.Parm16.WndProc.wParam = (WORD)uParamOld;
        mpex.Parm16.WndProc.lParam = lParamOld;
    }
#else // !DBCS
    mpex.Parm16.WndProc.wParam = (WORD)uParamOld;
    mpex.Parm16.WndProc.lParam = lParamOld;
#endif // !DBCS
    mpex.iMsgThunkClass = 0;

    hwnd = ThunkMsg16(&mpex);

    // Note: ThunkMsg16 may have caused memory movement
    FREEARGPTR(pFrame);
    FREEARGPTR(parg16);

    WOW32ASSERT(fWhoCalled == WOWDDE_POSTMESSAGE);
    fWhoCalled = FALSE;

    if (hwnd) {

        BlockWOWIdle(TRUE);

        mpex.lReturn = SendMessage(hwnd, mpex.uMsg, mpex.uParam, mpex.lParam);

        BlockWOWIdle(FALSE);
#ifdef DBCS
    //
    // For WIN3.1J's BUG ?
    // SendMessage( hwnd, WM_GETTEXT, 2, lpBuffer )
    // if string is DBCS, return is DBCSLeadbyte.
    // KKSUZUKA:#1731
    // 1994.8.8 add by V-HIDEKK
    //
        if( uMsgOld == WM_GETTEXT && uParamOld == 2 ){

            GETVDMPTR(mpex.Parm16.WndProc.lParam,mpex.Parm16.WndProc.wParam,lpBuf32);
            GETVDMPTR(lParamOld,uParamOld,lpBuf16);
            lpBuf16[0] = lpBuf32[0];
            if( mpex.lReturn == 2 ){
                lpBuf16[1] = 0;
                mpex.lReturn = 1;
            }
            else {
                lpBuf16[1] = lpBuf32[1];
            }
            FREEVDMPTR(lpBuf16);
            FREEVDMPTR(lpBuf32);
            GlobalUnlockFree16( mpex.Parm16.WndProc.lParam );
            mpex.Parm16.WndProc.wParam = (WORD)uParamOld;
            mpex.Parm16.WndProc.lParam = lParamOld;
        }
#endif // DBCS


        WOW32ASSERT(fWhoCalled == FALSE);
        fWhoCalled = WOWDDE_POSTMESSAGE;
        if (MSG16NEEDSTHUNKING(&mpex)) {
            (mpex.lpfnUnThunk16)(&mpex);
        }
        WOW32ASSERT(fWhoCalled == WOWDDE_POSTMESSAGE);
        fWhoCalled = FALSE;
    }

    FREEARGPTR(parg16);
    RETURN((ULONG)mpex.lReturn);
}







/*++
    int TranslateAccelerator(<hwnd>, <hAccTable>, <lpMsg>)

    The %TranslateAccelerator% function processes keyboard accelerators for menu
    commands. The %TranslateAccelerator% function translates WM_KEYUP and
    WM_KEYDOWN messages to WM_COMMAND or WM_SYSCOMMAND messages, if there is an
    entry for the key in the application's accelerator table. The high-order
    word of the <lParam> parameter of the WM_COMMAND or WM_SYSCOMMAND message
    contains the value 1 to differentiate the message from messages sent by
    menus or controls.

    WM_COMMAND or WM_SYSCOMMAND messages are sent directly to the window, rather
    than being posted to the application queue. The %TranslateAccelerator%
    function does not return until the message is processed.

    Accelerator key strokes that are defined to select items from the system
    menu are translated into WM_SYSCOMMAND messages; all other accelerators are
    translated into WM_COMMAND messages.

    <hwnd>
        Identifies the window whose messages are to be translated.

    <hAccTable>
        %HANDLE% Identifies an accelerator table (loaded by using the
        %LoadAccelerators% function).

    <lpMsg>
        Points to a message retrieved by using the %GetMessage% or
        %PeekMessage% function. The message must be an %MSG% structure and
        contain message information from the Windows application queue.

    .cmt
    19-Sep-1990 [johnca]
    Doesn't this function really return a BOOL?
    .endcmt

    The return value specifies the outcome of the function. It is nonzero if
    translation occurs. Otherwise, it is zero.

    When %TranslateAccelerator% returns nonzero (meaning that the message is
    translated), the application should <not> process the message again by using
    the %TranslateMessage% function.

    Commands in accelerator tables do not have to correspond to menu items.

    If the accelerator command does correspond to a menu item, the application
    is sent WM_INITMENU and WM_INITMENUPOPUP messages, just as if the user were
    trying to display the menu. However, these messages are not sent if any of
    the following conditions are present:

    o   The window is disabled.

    o   The menu item is disabled.

    o   The command is not in the System menu and the window is minimized.

    o   A mouse capture is in effect (for more information, see the %SetCapture%
        function, earlier in this chapter).

    If the window is the active window and there is no keyboard focus (generally
    true if the window is minimized), then WM_SYSKEYUP and WM_SYSKEYDOWN
    messages are translated instead of WM_KEYUP and WM_KEYDOWN messages.

    If an accelerator key stroke that corresponds to a menu item occurs when the
    window that owns the menu is iconic, no WM_COMMAND message is sent. However,
    if an accelerator key stroke that does not match any of the items on the
    window's menu or the System menu occurs, a WM_COMMAND message is sent, even
    if the window is iconic.
--*/

ULONG FASTCALL WU32TranslateAccelerator(PVDMFRAME pFrame)
{
    ULONG ul;
    MSG t3;
    register PTRANSLATEACCELERATOR16 parg16;

    GETARGPTR(pFrame, sizeof(TRANSLATEACCELERATOR16), parg16);

    W32CopyMsgStruct(parg16->f3, &t3, TRUE);
    ul = GETINT16(TranslateAccelerator(HWND32(parg16->f1),
                                       HACCEL32(parg16->f2), &t3 ));

    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    BOOL TranslateMDISysAccel(<hwndClient>, <lpMsg>)

    The %TranslateMDISysAccel% function processes keyboard accelerators for
    multiple document interface (MDI) child window System-menu commands. The
    %TranslateMDISysAccel% function translates WM_KEYUP and WM_KEYDOWN messages
    to WM_SYSCOMMAND messages. The high-order word of the <lParam> parameter of
    the WM_SYSCOMMAND message contains the value 1 to differentiate the message
    from messages sent by menus or controls.

    <hwndClient>
        Identifies the parent MDI client window.

    <lpMsg>
        Points to a message retrieved by using the %GetMessage% or
        %PeekMessage% function. The message must be an %MSG% structure and
        contain message information from the Windows application queue.

    The return value is TRUE if the function translated a message into a system
    command. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32TranslateMDISysAccel(PVDMFRAME pFrame)
{
    ULONG ul;
    MSG t2;
    register PTRANSLATEMDISYSACCEL16 parg16;

    GETARGPTR(pFrame, sizeof(TRANSLATEMDISYSACCEL16), parg16);

    W32CopyMsgStruct(parg16->f2, &t2, TRUE);

    ul = GETBOOL16(TranslateMDISysAccel(HWND32(parg16->f1), &t2));

    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    BOOL TranslateMessage(<lpMsg>)

    The %TranslateMessage% function translates virtual-key messages into
    character messages, as follows:

    o   WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or a WM_DEADCHAR
        message.

    o   WM_SYSKEYDOWN/WM_SYSKEYUP combinations produce a WM_SYSCHAR or a
        WM_SYSDEADCHAR message.

    The character messages are posted to the application queue, to be read the
    next time the application calls the %GetMessage% or %PeekMessage% function.

    <lpMsg>
        Points to a %MSG% structure retrieved through the GetMessage or
        PeekMessage function. The structure contains message information from
        the Windows application queue.

    The return value specifies the outcome of the function. It is TRUE if the
    message is translated (that is, character messages are posted to the
    application queue). Otherwise, it is FALSE.

    The %TranslateMessage% function does not modify the message given by the
    <lpMsg> parameter.

    %TranslateMessage% produces WM_CHAR messages only for keys which are mapped
    to ASCII characters by the keyboard driver.

    An application should not call %TranslateMessage% if the application
    processes virtual-key messages for some other purpose. For instance, an
    application should not call the %TranslateMessage% function if the
    %TranslateAccelerator% function returns TRUE.
--*/

ULONG FASTCALL WU32TranslateMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    MSG t1;
    register PTRANSLATEMESSAGE16 parg16;

    GETARGPTR(pFrame, sizeof(TRANSLATEMESSAGE16), parg16);

    W32CopyMsgStruct(parg16->f1, &t1, TRUE);

    ul = GETBOOL16(TranslateMessage( &t1 ));

    FREEARGPTR(parg16);
    RETURN(ul);
}







/*++
    void WaitMessage(VOID)

    The %WaitMessage% function yields control to other applications when an
    application has no other tasks to perform. The %WaitMessage% function
    suspends the application and does not return until a new message is placed
    in the application's queue.

    This function has no parameters.

    This function does not return a value.

    The %GetMessage%, %PeekMessage%, and %WaitMessage% functions yield control
    to other applications. These calls are the only way to let other
    applications run. If your application does not call any of these functions
    for long periods of time, other applications cannot run.

    When %GetMessage%, %PeekMessage%, and %WaitMessage% yield control to other
    applications, the stack and data segments of the application calling the
    function may move in memory to accommodate the changing memory requirements
    of other applications. If the application has stored long pointers to
    objects in the data or stack segment (that is, global or local variables),
    these pointers can become invalid after a call to %GetMessage%,
    %PeekMessage%, or %WaitMessage%.
--*/

ULONG FASTCALL WU32WaitMessage(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);

    BlockWOWIdle(TRUE);

    WaitMessage();

    BlockWOWIdle(FALSE);

    RETURN(0);
}
