/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    ctrlc.c

Abstract:

    This module implements ctrl-c handling

Author:

    Therese Stowell (thereses) 1-Mar-1991

Revision History:   

--*/

#include "precomp.h"
#pragma hdrstop

#if !defined(BUILD_WOW64)

#define LIST_INCREMENT 2    // amount to grow handler list
#define INITIAL_LIST_SIZE 1 // initial length of handler list

PHANDLER_ROUTINE SingleHandler[INITIAL_LIST_SIZE]; // initial handler list
ULONG HandlerListLength;            // used length of handler list
ULONG AllocatedHandlerListLength;   // allocated length of handler list
PHANDLER_ROUTINE *HandlerList;      // pointer to handler list

#define NUMBER_OF_CTRL_EVENTS 7     // number of ctrl events
#define SYSTEM_CLOSE_EVENT 4

#define IGNORE_CTRL_C   0x01

BOOL LastConsoleEventActive;


BOOL
DefaultHandler(
    IN ULONG CtrlType
    )

/*++

    This is the default ctrl handler.

Parameters:

    CtrlType - type of ctrl event (ctrl-c, ctrl-break).

Return Value:

    none.

--*/

{
    ExitProcess((DWORD)CONTROL_C_EXIT);
    return TRUE;
    UNREFERENCED_PARAMETER(CtrlType);
}

NTSTATUS
InitializeCtrlHandling( VOID )

/*++

    This routine initializes ctrl handling.  It is called by AllocConsole
    and the dll initialization code.

Parameters:

    none.

Return Value:

    none.

--*/

{
    AllocatedHandlerListLength = HandlerListLength = INITIAL_LIST_SIZE;
    HandlerList = SingleHandler;
    SingleHandler[0] = DefaultHandler;
    return STATUS_SUCCESS;
}

DWORD
CtrlRoutine(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This thread is created when ctrl-c or ctrl-break is entered,
    or when close is selected.  it calls the appropriate handlers.

Arguments:

    lpThreadParameter - what type of event happened.

Return Value:

    STATUS_SUCCESS

--*/

{
    ULONG i;
    ULONG EventNumber,OriginalEventNumber;
    DWORD fNoExit;
    DWORD dwExitCode;
    EXCEPTION_RECORD ExceptionRecord;

    SetThreadPriority(NtCurrentThread(), THREAD_PRIORITY_HIGHEST);
    OriginalEventNumber = EventNumber = PtrToUlong(lpThreadParameter);

    //
    // If this bit is set, it means we don't want to cause this process
    // to exit itself if it is a logoff or shutdown event.
    //
    fNoExit = 0x80000000 & EventNumber;
    EventNumber &= ~0x80000000;

    //
    // the ctrl_close event is set when the user selects the window
    // close option from the system menu, or EndTask, or Settings-Terminate.
    // the system close event is used when another ctrl-thread times out.
    //

    switch (EventNumber) {
    default:
        ASSERT (EventNumber < NUMBER_OF_CTRL_EVENTS);
        if (EventNumber >= NUMBER_OF_CTRL_EVENTS)
            return (DWORD)STATUS_UNSUCCESSFUL;
        break;

    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        //
        // If the process is being debugged, give the debugger
        // a shot. If the debugger handles the exception, then
        // go back and wait.
        //

        if (!IsDebuggerPresent())
            break;

        if ( EventNumber == CTRL_C_EVENT ) {
            ExceptionRecord.ExceptionCode = DBG_CONTROL_C;
            }
        else {
            ExceptionRecord.ExceptionCode = DBG_CONTROL_BREAK;
            }
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.ExceptionAddress = (PVOID)DefaultHandler;
        ExceptionRecord.NumberParameters = 0;

        try {
            RtlRaiseException(&ExceptionRecord);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            LockDll();
            try {
                if (EventNumber != CTRL_C_EVENT ||
                        NtCurrentPeb()->ProcessParameters->ConsoleFlags != IGNORE_CTRL_C) {
                    for (i=HandlerListLength;i>0;i--) {
                        if ((HandlerList[i-1])(EventNumber)) {
                            break;
                        }
                    }
                }
            } finally {
                UnlockDll();
            }
        }
        ExitThread(0);
        break;

    case SYSTEM_CLOSE_EVENT:
        ExitProcess((DWORD)CONTROL_C_EXIT);
        break;

    case SYSTEM_ROOT_CONSOLE_EVENT:
        if (!LastConsoleEventActive)
            ExitThread(0);
        break;

    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        //if (LastConsoleEventActive)
            //EventNumber = SYSTEM_ROOT_CONSOLE_EVENT;
        break;
    }

    LockDll();
    dwExitCode = 0;
    try {
        if (EventNumber != CTRL_C_EVENT ||
                NtCurrentPeb()->ProcessParameters->ConsoleFlags != IGNORE_CTRL_C) {
            for (i=HandlerListLength;i>0;i--) {

                //
                // Don't call the last handler (the default one which calls
                // ExitProcess() if this process isn't supposed to exit (system
                // process are not supposed to exit because of shutdown or
                // logoff event notification).
                //

                if ((i-1) == 0 && fNoExit) {
                    if (EventNumber == CTRL_LOGOFF_EVENT ||
                        EventNumber == CTRL_SHUTDOWN_EVENT) {
                        break;
                    }
                }

                if ((HandlerList[i-1])(EventNumber)) {
                    switch (EventNumber) {
                    case CTRL_CLOSE_EVENT:
                    case CTRL_LOGOFF_EVENT:
                    case CTRL_SHUTDOWN_EVENT:
                    case SYSTEM_ROOT_CONSOLE_EVENT:
                        dwExitCode = OriginalEventNumber;
                        break;
                    }
                    break;
                }
            }
        }
    } finally {
        UnlockDll();
    }
    ExitThread(dwExitCode);
    return STATUS_SUCCESS;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

VOID
APIENTRY
SetLastConsoleEventActiveInternal( VOID )

/*++

Routine Description:

    Sends a ConsolepNotifyLastClose command to the server.

Arguments:

    none.

Return Value:

    None.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_NOTIFYLASTCLOSE_MSG a = &m.u.SetLastConsoleEventActive;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepNotifyLastClose
                                            ),
                         sizeof( *a )
                       );
}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

VOID
APIENTRY
SetLastConsoleEventActive( VOID )
// private api
{

    LastConsoleEventActive = TRUE;
    SetLastConsoleEventActiveInternal();
}

BOOL
SetCtrlHandler(
    IN PHANDLER_ROUTINE HandlerRoutine
    )

/*++

Routine Description:

    This routine adds a ctrl handler to the process's list.

Arguments:

    HandlerRoutine - pointer to ctrl handler.

Return Value:

    TRUE - success.

--*/

{
    PHANDLER_ROUTINE *NewHandlerList;

    //
    // NULL handler routine is not stored in table. It is
    // used to temporarily inhibit ^C event handling
    //

    if ( !HandlerRoutine ) {
        NtCurrentPeb()->ProcessParameters->ConsoleFlags = IGNORE_CTRL_C;
        return TRUE;
        }

    if (HandlerListLength == AllocatedHandlerListLength) {

        //
        // grow list
        //

        NewHandlerList = (PHANDLER_ROUTINE *) RtlAllocateHeap( RtlProcessHeap(), 0,
                                                 sizeof(PHANDLER_ROUTINE) * (HandlerListLength + LIST_INCREMENT));
        if (!NewHandlerList) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        //
        // copy list
        //

        RtlCopyMemory(NewHandlerList,HandlerList,sizeof(PHANDLER_ROUTINE) * HandlerListLength);

        if (HandlerList != SingleHandler) {

            //
            // free old list
            //

            RtlFreeHeap(RtlProcessHeap(), 0, HandlerList);
        }
        HandlerList = NewHandlerList;
        AllocatedHandlerListLength += LIST_INCREMENT;
    }
    ASSERT (HandlerListLength < AllocatedHandlerListLength);

    HandlerList[HandlerListLength] = HandlerRoutine;
    HandlerListLength++;
    return TRUE;
}

BOOL
RemoveCtrlHandler(
    IN PHANDLER_ROUTINE HandlerRoutine
    )

/*++

Routine Description:

    This routine removes a ctrl handler from the process's list.

Arguments:

    HandlerRoutine - pointer to ctrl handler.

Return Value:

    TRUE - success.

--*/

{
    ULONG i;

    //
    // NULL handler routine is not stored in table. It is
    // used to temporarily inhibit ^C event handling. Removing
    // this handler allows normal processing to occur
    //

    if ( !HandlerRoutine ) {
        NtCurrentPeb()->ProcessParameters->ConsoleFlags = 0;
        return TRUE;
        }

    for (i=0;i<HandlerListLength;i++) {
        if (*(HandlerList+i) == HandlerRoutine) {
            if (i < (HandlerListLength-1)) {
                memmove(&HandlerList[i],&HandlerList[i+1],sizeof(PHANDLER_ROUTINE) * (HandlerListLength - i - 1));
            }
            HandlerListLength -= 1;
            return TRUE;
        }
    }
    SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
APIENTRY
SetConsoleCtrlHandler(
    IN PHANDLER_ROUTINE HandlerRoutine,
    IN BOOL Add       // add or delete
    )

/*++

Routine Description:

    This routine adds or removes a ctrl handler from the process's list.

Arguments:

    HandlerRoutine - pointer to ctrl handler.

    Add - if TRUE, add handler.  else remove.

Return Value:

    TRUE - success.

--*/

{
    BOOL Success;

    LockDll();
    if (Add) {
        Success = SetCtrlHandler(HandlerRoutine);
    }
    else {
        Success = RemoveCtrlHandler(HandlerRoutine);
    }
    UnlockDll();
    return Success;
}

#endif //!defined(BUILD_WOW64)
