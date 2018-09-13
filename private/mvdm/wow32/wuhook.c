/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUHOOK.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wuhook.c);


/*++
    FARPROC SetWindowsHook(<nFilterType>, <lpFilterFunc>)
    int <nFilterType>;
    FARPROC <lpFilterFunc>;

    The %SetWindowsHook% function installs a filter function in a chain. A
    filter function processes events before they are sent to an application's
    message loop in the WinMain function. A chain is a linked list of filter
    functions of the same type.

    <nFilterType>
        Specifies the system hook to be installed. It can be any one of the
        following values:

    WH_CALLWNDPROC      Installs a window-function filter.
    WH_GETMESSAGE       Installs a message filter.
    WH_JOURNALPLAYBACK  Installs a journaling playback filter.
    WH_JOURNALRECORD    Installs a journaling record filter.
    WH_KEYBOARD         Installs a keyboard filter.
    WH_MSGFILTER        Installs a message filter.
    WH_SYSMSGFILTER     Installs a system-wide message filter.

    <lpFilterFunc>
        Is the procedure-instance address of the filter function to be
        installed. See the following Comments section for details.

    The return value points to the procedure-instance address of the previously
    installed filter (if any). It is NULL if there is no previous filter. The
    application or library that calls the %SetWindowsHook% function should save
    this return value in the library's data segment. The fourth argument of the
    %DefHookProc% function points to the location in memory where the library
    saves this return value.

    The return value is -1 if the function fails.

    The WH_CALLWNDPROC hook will affect system performance. It is supplied for
    debugging purposes only.

    The system hooks are a shared resource. Installing a hook affects all
    applications. Most hook functions must be in libraries. The only exception
    is WH_MSGFILTER, which is task-specific. System hooks should be restricted
    to special-purpose applications or as a development aid during debugging of
    an application. Libraries that no longer need the hook should remove the
    filter function.

    To install a filter function, the %SetWindowsHook% function must receive a
    procedure-instance address of the function, and the function must be
    exported in the library's module-definition file. Libraries can pass the
    procedure address directly. Tasks must use %MakeProcInstance% to get a
    procedure-instance address. Dynamic-link libraries must use %GetProcAddress%
    to get a procedure-instance address.

    The following section describes how to support the individual hook
    functions.

    WH_CALLWNDPROC:

    Windows calls the WH_CALLWNDPROC filter function whenever the %SendMessage%
    function is called. Windows does not call the filter function when the
    %PostMessage% function is called.

    The filter function must use the Pascal calling convention and must be
    declared %FAR%. The filter function must have the following form:

    Filter Function:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the application- or library-supplied
    function name. The actual name must be exported by including it in an
    %EXPORTS% statement in the library's module-definition file.

    <nCode>
        Specifies whether the filter function should process the message or call
        the DefHookProc function. If the nCode parameter is less than zero, the
        filter function should pass the message to DefHookProc without further
        processing. <wParam> Specifies whether the message is sent by the
        current task. It is nonzero if the message is sent; otherwise, it is
        NULL.

    <lParam>
        Points to a structure that contains details about the message
        intercepted by the filter. The following shows the order, type, and
        description of each field of the structure:

    %lParam%
        %WORD% Contains the low-order word of the <lParam> parameter of the
        message received by the filter.

    %wParam%
        %WORD% Contains the <wParam> parameter of the message received by the
        filter.

    %wMsg%
        %WORD% Contains the message received by the filter.

    %hwnd%
        %WORD% Contains the window handle of the window that is to receive the
        message.

    The WH_CALLWNDPROC filter function can examine or modify the message as
    desired. Once it returns control to Windows, the message, with any
    modifications, is passed on to the window function. The filter function does
    not require a return value.

    WH_GETMESSAGE:

    Windows calls the WH_GETMESSAGE filter function whenever the %GetMessage%
    function is called. Windows calls the filter function immediately after
    %GetMessage% has retrieved a message from an application queue. The filter
    function must use the Pascal calling convention and must be declared %FAR%.
    The filter function must have the following form:

    Filter Function:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the library-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the library's module-definition file.

    <nCode>
        Specifies whether the filter function should process the message or call
        the DefHookProc function. If the <nCode> parameter is less than zero, the
        filter function should pass the message to DefHookProc without further
        processing.

    <wParam>
        Specifies a NULL value.

    <lParam>
        Points to a message structure.

    The WH_GETMESSAGE filter function can examine or modify the message as
    desired. Once it returns control to Windows, the %GetMessage% function
    returns the message, with any modifications, to the application that
    originally called it. The filter function does not require a return value.

    WH_JOURNALPLAYBACK:

    Windows calls the WH_JOURNALPLAYBACK filter function whenever a request for
    an event message is made. The function is intended to be used to supply a
    previously recorded event message.

    The filter function must use the Pascal calling convention and must be
    declared %FAR%. The filter function must have the following form:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the library-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the library's module-definition file.

    <nCode>
        Specifies whether the filter function should process the message or call
        the DefHookProc function. If the nCode parameter is less then zero, the
        filter function should pass the message to DefHookProc without further
        processing.

    <wParam>
        Specifies a NULL value.

    <lParam>
        Points to the message being processed by the filter function.

    The WH_JOURNALPLAYBACK function should copy an event message to the <lParam>
    parameter. The message must have been previously recorded by using the
    WH_JOURNALRECORD filter. It should not modify the message. The return value
    should be the amount of time (in clock ticks) Windows should wait before
    processing the message. This time can be computed by calculating the
    difference between the %time% fields in the current and previous event
    messages. If the function returns zero, the message is processed
    immediately. Once it returns control to Windows, the message continues to be
    processed. If the <nCode> parameter is HC_SKIP, the filter function should
    prepare to return the next recorded event message on its next call.

    While the WH_JOURNALPLAYBACK function is in effect, Windows ignores all
    mouse and keyboard input.

    WH_JOURNALRECORD:

    Windows calls the WH_JOURNALRECORD filter function whenever it processes a
    message from the event queue. The filter can be used to record the event for
    later playback.

    The filter function must use the Pascal calling convention and must be
    declared %FAR%. The filter function must have the following form:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>,<lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the library-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the library's module-definition file.

    <nCode>
        Specifies whether the filter function should process the message or call
        the DefHookProc function. If the nCode parameter is less than zero, the
        filter function should pass the message to DefHookProc without further
        processing.

    <wParam>
        Specifies a NULL value.

    <lParam>
        Points to a message structure.

    The WH_JOURNALRECORD function should save a copy of the event message for
    later playback. It should not modify the message. Once it returns control to
    Windows, the message continues to be processed. The filter function does not
    require a return value.

    WH_KEYBOARD:

    Windows calls the WH_KEYBOARD filter function whenever the application calls
    the %GetMessage% or %PeekMessage% function and there is a keyboard event
    (WM_KEYUP or WM_KEYDOWN) to process.

    The filter function must use the Pascal calling convention and must be
    declared %FAR%. The filter function must have the following form:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>; DWORD <lParam>;

    <FilterFunc> is a placeholder for the library-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the library's module-definition file.

    <nCode>
        Specifies whether the filter function should process the message or call
        the DefHookProc function. If this value is HC_NOREMOVE, the application
        is using the PeekMessage function with the PM_NOREMOVE option and the
        message will not be removed from the system queue. If this value is less
        than zero, the filter function should pass the message to DefHookProc
        without further processing.

    <wParam>
        Specifies the virtual-key code of the given key.

    <lParam>
        Specifies the repeat count, scan code, key-transition code, previous key
        state, and context code, as shown in the following list. Bit 1 is the
        low-order bit:

    0-15
        (low-order word) Repeat count (the number of times the keystroke is
        repeated as a result of the user holding down the key).

    16-23
        (low byte of high-order word) Scan code (OEM-dependent value).

    24
        Extended key (1 if it is an extended key).

    25-26
        Not used.

    27-28
        (Context code (1 if the ^ALT^ key was held down while the key was
        pressed, 0 otherwise) Used internally by Windows.

    30
        Previous key state (1 if the key was held down before the message was
        sent, 0 if the key was up).

    31
        Transition state (1 if the key is being released, 0 if the key is being
        pressed).

    The return value specifies what should happen to the message. It is zero if
    the message should be processed by Windows; it is 1 if the message should be
    discarded.

    WH_MSGFILTER:

    Windows calls the WH_MSGFILTER filter function whenever a dialog box,
    message box, or menu has retrieved a message, and before it has processed
    that message. The filter allows an application to process or modify the
    messages.

    This is the only task-specific filter. A task may install this filter.

    The WH_MSGFILTER filter function must use the Pascal calling convention and
    must be declared %FAR%. The filter function must have the following form:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the library- or application-supplied
    function name. The actual name must be exported by including it in an
    %EXPORTS% statement in the application's module-definition file.

    <nCode>
        Specifies the type of message being processed. It must be one of the
        following values:

    MSGF_MENU
        Processing keyboard and mouse messages in a menu.

    MSGF_MENU
        Processing keyboard and mouse messages in a menu.

        If the <nCode> parameter is less than zero, the filter function must
        pass the message to %DefHookProc% without further processing and return
        the value returned by %DefHookProc%.

    <wParam>
        Specifies a NULL value.

    <lParam>
        Points to the message structure.

    The return value specifies the outcome of the function. It is nonzero if the
    hook function processes the message. Otherwise, it is zero.

    WH_SYSMSGFILTER:

    Windows calls the WH_SYSMSGFILTER filter function whenever a dialog box,
    message box, or menu has retrieved a message and before it has processed
    that message. The filter allows an application to process or modify messages
    for any application in the system.

    The filter function must use the Pascal calling convention and must be
    declared %FAR%. The filter function must have the following form:

    DWORD FAR PASCAL <FilterFunc>(<nCode>, <wParam>, <lParam>)
    int <nCode>;
    WORD <wParam>;
    DWORD <lParam>;

    <FilterFunc> is a placeholder for the library-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the library's module-definition file.

    <nCode>
        Specifies the type of message being processed. It must be one of the
        following values:

    MSGF_MENU
        Processing keyboard and mouse messages in menu.

    MSGF_MESSAGEBOX
        Processing messages inside the %MessageBox% function.

        If the <nCode> parameter is less than zero, the filter function must
        pass the message to %DefHookProc% without further processing and return
        the value returned by %DefHookProc%.

    <wParam>
        Specifies a NULL value.

    <lParam>
        Points to the message structure.

    The return value specifies the outcome of the function. It is nonzero if the
    hook function processes the message. Otherwise, it is zero.
--*/

ULONG FASTCALL WU32SetWindowsHookInternal(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETWINDOWSHOOKINTERNAL16 parg16;
    HOOKSTATEDATA HkData;
    HAND16        hMod16;
    INT           iHook;
    DWORD         Proc16;
    DWORD         ThreadId;
    PTD ptd = CURRENTPTD();


    GETARGPTR(pFrame, sizeof(SETWINDOWSHOOKINTERNAL16), parg16);
    hMod16 = FETCHWORD(parg16->f1);
    iHook = INT32(parg16->f2);
    Proc16 = DWORD32(parg16->f3);

    //
    // HACKHACK - Work around MS Mail 3.0's journal record hook.
    //            This hook is used only to keep track of the input
    //            activity in the system.  When the hook is called,
    //            Mail simply stores the current time.  Later, on
    //            expiration of a timer, Mail determines whether or
    //            not to start background database compression, using
    //            the amount of time since input was received as a
    //            determining factor.  If the hook hasn't been called
    //            in a while, Mail is more likely to start slow
    //            compression or switch to fast compression.
    //
    //            The problem is that WH_JOURNALRECORD causes all
    //            threads in the system to share one input queue,
    //            thereby meaning that any app that stops processing
    //            input hangs the entire UI.
    //
    //            For now, just disable the hook.
    //

    if (WH_JOURNALRECORD == iHook &&
        (ptd->dwWOWCompatFlags & WOWCF_FAKEJOURNALRECORDHOOK)) {
        return 0;
    }

    /*
    ** Micrografx Draw installs a hook, then when minimized, it unhooks the
    ** hook by re-hooking the return value from the original hook call.
    ** This works in Win 3.1 because the hook return value is a proc address.
    ** We can detect this by looking at the HIWORD of the proc address.  If
    ** it is NULL, we assume they are passing us a hook handle instead of
    ** a proc address.  If this is the case, then what they really want is
    ** unhooking.  -BobDay
    */
    if ( HIWORD(Proc16) == HOOK_ID ) {
        ul = GETBOOL16(UnhookWindowsHookEx(W32FreeHHookOfIndex(GETHHOOKINDEX(Proc16))));
        FREEARGPTR(parg16);
        return( ul );
    }

    if (!(ul = (ULONG)W32IsDuplicateHook(iHook, Proc16, ptd->htask16))) {
        if (W32GetThunkHookProc(iHook, Proc16, &HkData)) {

            // We pass threadid=0, for all hooks except WH_MSGFILTER.
            // because it is the only task-specific filter in WIN30.
            //
            // The understanding between USER and WOW is this:
            //    When a WOW thread sets a hook, with thread ID = 0,
            //    USER does the following:
            //       If Journal Hooks are being set, USER will set the
            //          WOW hook 'globally', ie. system wide.
            //
            //       For all the other hooks, USER sets the hook for all
            //       'WOW' threads i.e., the hook is global for the  WOW
            //       process.
            //
            //   If the threadiD != 0, then no special processing is done.
            //   the hook is set only for that particular thread.
            //

            if (iHook == (INT)WH_MSGFILTER)
                ThreadId = (DWORD)THREADID32(HkData.TaskId);
            else
                ThreadId = 0;

            ul = (ULONG)SetWindowsHookEx(iHook, (HOOKPROC)HkData.Proc32,
                                        (HINSTANCE)HkData.hMod, ThreadId);
            HkData.hHook = (HANDLE)ul;
            HkData.hMod16 = hMod16;

            // Excel looks at the hiword; so instead of passing back just an
            // index we make the hiword a hook identifier.
            if (ul == (ULONG)NULL)
                HkData.InUse = FALSE;
            else
                ul = MAKEHHOOK(HkData.iIndex);

            W32SetHookStateData(&HkData);
        }
        else
            ul = (ULONG)NULL;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL UnhookWindowsHook(<nHook>, <lpfnHook>)

    The %UnhookWindowsHook% function removes the Windows hook function pointed
    to by the <lpfnHook> parameter from a chain of hook functions. A Windows
    hook function processes events before they are sent to an application's
    message loop in the WinMain function.

    <nHook>
        int Specifies the type of hook function removed. It may be one of the
        following values:

    WH_CALLWNDPROC
        Installs a window-function filter.

    WH_GETMESSAGE
        Installs a message filter.

    WH_JOURNALPLAYBACK
        Installs a journaling playback filter.

    WH_JOURNALRECORD
        Installs a journaling record filter.

    WH_KEYBOARD
        Install a keyboard filter.

    WH_MSGFILTER
        Installs a message filter.

    The return value specifies the outcome of the function. It is TRUE if the
    hook function is successfully removed. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32UnhookWindowsHook(PVDMFRAME pFrame)
{
    ULONG                         ul;
    register PUNHOOKWINDOWSHOOK16 parg16;
    INT                           iHook;
    DWORD                         Proc16;

    GETARGPTR(pFrame, sizeof(UNHOOKWINDOWSHOOK16), parg16);
    iHook = INT32(parg16->f1);
    Proc16 = DWORD32(parg16->f2);


    ul = GETBOOL16(UnhookWindowsHookEx(W32FreeHHook(iHook, Proc16)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    CallNextHookEx - similar to DefHookProc
--*/

ULONG FASTCALL WU32CallNextHookEx(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PCALLNEXTHOOKEX16 parg16;
    HOOKSTATEDATA HkData;
    ULONG         hHook16;
    INT           nCode;
    LONG          wParam;
    LONG          lParam;
    DWORD         iHookCode;

    GETARGPTR(pFrame, sizeof(CALLNEXTHOOKEX16), parg16);


    hHook16 = DWORD32(parg16->f1);
    nCode = INT32(parg16->f2);
    wParam = WORD32(parg16->f3);
    lParam = DWORD32(parg16->f4);

    if (ISVALIDHHOOK(hHook16)) {
        iHookCode = GETHHOOKINDEX(hHook16);
        HkData.iIndex = (INT)iHookCode;
        if ( W32GetHookStateData( &HkData ) ) {
            ul = (ULONG)WU32StdDefHookProc(nCode, wParam, lParam, iHookCode);
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    SetWindowsHookEx - similar to SetWindowsHook.

--*/

ULONG FASTCALL WU32SetWindowsHookEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETWINDOWSHOOKEX16 parg16;
    HOOKSTATEDATA HkData;
    INT           iHook;
    DWORD         Proc16;


    GETARGPTR(pFrame, sizeof(SETWINDOWSHOOKEX16), parg16);
    iHook = INT32(parg16->f1);
    Proc16 = DWORD32(parg16->f2);

    if (W32GetThunkHookProc(iHook, Proc16, &HkData)) {
        ul = (ULONG)SetWindowsHookEx(iHook, (HOOKPROC)HkData.Proc32,
                                   (HINSTANCE)HkData.hMod, (DWORD)THREADID32(parg16->f4));
        HkData.hHook = (HANDLE)ul;
        if (ul == (ULONG)NULL) {
            HkData.InUse = FALSE;
        } else {
            ul = MAKEHHOOK(HkData.iIndex);
            HkData.hMod16 = GetExePtr16(parg16->f3);
        }

        W32SetHookStateData(&HkData);
    }
    else
        ul = (ULONG)NULL;

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    UnhookWindowsHookEx - similar to unhookwindowshook

--*/

ULONG FASTCALL WU32UnhookWindowsHookEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PUNHOOKWINDOWSHOOKEX16 parg16;

    GETARGPTR(pFrame, sizeof(UNHOOKWINDOWSHOOKEX16), parg16);

    ul = GETBOOL16(UnhookWindowsHookEx(W32FreeHHookOfIndex(GETHHOOKINDEX(INT32(parg16->f1)))));

    FREEARGPTR(parg16);
    RETURN(ul);
}
