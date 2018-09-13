/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module implements Win32 Debug APIs

Author:

    Mark Lucovsky (markl) 06-Feb-1991

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

BOOL
APIENTRY
IsDebuggerPresent(
    VOID
    )

/*++

Routine Description:

    This function returns TRUE if the current process is being debugged
    and FALSE if not.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return NtCurrentPeb()->BeingDebugged;
}

//#ifdef i386
//#pragma optimize("",off)
//#endif // i386
VOID
APIENTRY
DebugBreak(
    VOID
    )

/*++

Routine Description:

    This function causes a breakpoint exception to occur in the caller.
    This allows the calling thread to signal the debugger forcing it to
    take some action.  If the process is not being debugged, the
    standard exception search logic is invoked.  In most cases, this
    will cause the calling process to terminate (due to an unhandled
    breakpoint exception).

Arguments:

    None.

Return Value:

    None.

--*/

{
    DbgBreakPoint();
}
//#ifdef i386
//#pragma optimize("",on)
//#endif // i386

VOID
APIENTRY
OutputDebugStringW(
    LPCWSTR lpOutputString
    )

/*++

Routine Description:

    UNICODE thunk to OutputDebugStringA

--*/

{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    RtlInitUnicodeString(&UnicodeString,lpOutputString);
    Status = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    if ( !NT_SUCCESS(Status) ) {
        AnsiString.Buffer = "";
        }
    OutputDebugStringA(AnsiString.Buffer);
    if ( NT_SUCCESS(Status) ) {
        RtlFreeAnsiString(&AnsiString);
        }
}


#define DBWIN_TIMEOUT   10000
HANDLE CreateDBWinMutex(VOID) {

    SECURITY_ATTRIBUTES SecurityAttributes;
    SECURITY_DESCRIPTOR sd;
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidEveryone = NULL;
    PACL pAcl = NULL;
    DWORD cbAcl, aceIndex;
    HANDLE h = NULL;
    //
    // Get the system sid
    //

    Status = RtlAllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                   0, 0, 0, 0, 0, 0, 0, &psidSystem);
    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Get the Admin sid
    //

    Status = RtlAllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                       DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                       0, 0, 0, 0, &psidAdmin);

    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Get the World sid
    //

    Status = RtlAllocateAndInitializeSid(&authWorld, 1, SECURITY_WORLD_RID,
                      0, 0, 0, 0, 0, 0, 0, &psidEveryone);

    if (!NT_SUCCESS(Status))
          goto Exit;


    //
    // Allocate space for the ACL
    //

    cbAcl = sizeof(ACL) +
            3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
            RtlLengthSid(psidSystem) +
            RtlLengthSid(psidAdmin) +
            RtlLengthSid(psidEveryone);

    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }

    Status = RtlCreateAcl(pAcl, cbAcl, ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto Exit;


    //
    // Add Aces.
    //

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE, psidEveryone);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, MUTEX_ALL_ACCESS, psidSystem);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, MUTEX_ALL_ACCESS, psidAdmin);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
       goto Exit;

    Status = RtlSetDaclSecurityDescriptor(&sd, TRUE, pAcl, FALSE);
    if (!NT_SUCCESS(Status))
       goto Exit;


    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = &sd;

    h = CreateMutex(&SecurityAttributes, FALSE, "DBWinMutex");
Exit:
    if (psidSystem) {
        RtlFreeSid(psidSystem);
    }

    if (psidAdmin) {
        RtlFreeSid(psidAdmin);
    }

    if (psidEveryone) {
        RtlFreeSid(psidEveryone);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }
    return h;
}


VOID
APIENTRY
OutputDebugStringA(
    IN LPCSTR lpOutputString
    )

/*++

Routine Description:

    This function allows an application to send a string to its debugger
    for display.  If the application is not being debugged, but the
    system debugger is active, the system debugger displays the string.
    Otherwise, this function has no effect.

Arguments:

    lpOutputString - Supplies the address of the debug string to be sent
        to the debugger.

Return Value:

    None.

--*/

{
    ULONG_PTR ExceptionArguments[2];

    //
    // Raise an exception. If APP is being debugged, the debugger
    // will catch and handle this. Otherwise, kernel debugger is
    // called.
    //

    try {
        ExceptionArguments[0]=strlen(lpOutputString)+1;
        ExceptionArguments[1]=(ULONG_PTR)lpOutputString;
        RaiseException(DBG_PRINTEXCEPTION_C,0,2,ExceptionArguments);
        }
    except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We caught the debug exception, so there's no user-mode
        // debugger.  If there is a DBWIN running, send the string
        // to it.  If not, use DbgPrint to send it to the kernel
        // debugger.  DbgPrint can only handle 511 characters at a
        // time, so force-feed it.
        //

        char   szBuf[512];
        size_t cchRemaining;
        LPCSTR pszRemainingOutput;

        HANDLE SharedFile = NULL;
        LPSTR SharedMem = NULL;
        HANDLE AckEvent = NULL;
        HANDLE ReadyEvent = NULL;

        static HANDLE DBWinMutex = NULL;
        static BOOLEAN CantGetMutex = FALSE;

        //
        // look for DBWIN.
        //

        if (!DBWinMutex && !CantGetMutex) {
            DBWinMutex = CreateDBWinMutex();
            if (!DBWinMutex)
                CantGetMutex = TRUE;
        }

        if (DBWinMutex) {

            WaitForSingleObject(DBWinMutex, INFINITE);

            SharedFile = OpenFileMapping(FILE_MAP_WRITE, FALSE, "DBWIN_BUFFER");

            if (SharedFile) {

                SharedMem = MapViewOfFile( SharedFile,
                                        FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
                if (SharedMem) {

                    AckEvent = OpenEvent(SYNCHRONIZE, FALSE,
                                                         "DBWIN_BUFFER_READY");
                    if (AckEvent) {
                        ReadyEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE,
                                                           "DBWIN_DATA_READY");
                        }
                    }
                }

            if (!ReadyEvent) {
                ReleaseMutex(DBWinMutex);
                }

            }

        try {
            pszRemainingOutput = lpOutputString;
            cchRemaining = strlen(pszRemainingOutput);

            while (cchRemaining > 0) {
                int used;

                if (ReadyEvent && WaitForSingleObject(AckEvent, DBWIN_TIMEOUT)
                                                            == WAIT_OBJECT_0) {

                    *((DWORD *)SharedMem) = GetCurrentProcessId();

                    used = (int)((cchRemaining < 4095 - sizeof(DWORD)) ?
                                         cchRemaining : (4095 - sizeof(DWORD)));

                    RtlCopyMemory(SharedMem+sizeof(DWORD),
                                  pszRemainingOutput,
                                  used);
                    SharedMem[used+sizeof(DWORD)] = 0;
                    SetEvent(ReadyEvent);

                    }
                else {
                    used = (int)((cchRemaining < sizeof(szBuf) - 1) ?
                                           cchRemaining : (int)(sizeof(szBuf) - 1));

                    RtlCopyMemory(szBuf, pszRemainingOutput, used);
                    szBuf[used] = 0;
                    DbgPrint("%s", szBuf);
                    }

                pszRemainingOutput += used;
                cchRemaining       -= used;

                }
            }
        except(STATUS_ACCESS_VIOLATION == GetExceptionCode()) {
            DbgPrint("\nOutputDebugString faulted during output\n");
            }

        if (AckEvent) {
            CloseHandle(AckEvent);
            }
        if (SharedMem) {
            UnmapViewOfFile(SharedMem);
            }
        if (SharedFile) {
            CloseHandle(SharedFile);
            }
        if (ReadyEvent) {
            CloseHandle(ReadyEvent);
            ReleaseMutex(DBWinMutex);
            }

        }

}

BOOL
APIENTRY
WaitForDebugEvent(
    LPDEBUG_EVENT lpDebugEvent,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A debugger waits for a debug event to occur in one of its debuggees
    using WaitForDebugEvent:

    Upon successful completion of this API, the lpDebugEvent structure
    contains the relevant information of the debug event.

Arguments:

    lpDebugEvent - Receives information specifying the type of debug
        event that occured.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test for debug
        events A timeout value of -1 specifies an infinite timeout
        period.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed (or timed out).  Extended error
        status is available using GetLastError.

--*/

{
    NTSTATUS Status;
    DBGUI_WAIT_STATE_CHANGE StateChange;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    HANDLE hThread;
    OBJECT_ATTRIBUTES Obja;


    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);

again:
    Status = DbgUiWaitStateChange(&StateChange,pTimeOut);
    if ( Status == STATUS_ALERTED || Status == STATUS_USER_APC) {
        goto again;
        }
    if ( !NT_SUCCESS(Status) && Status != DBG_UNABLE_TO_PROVIDE_HANDLE ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( Status == STATUS_TIMEOUT ) {
        SetLastError(ERROR_SEM_TIMEOUT);
        return FALSE;
        }

    lpDebugEvent->dwProcessId = HandleToUlong(StateChange.AppClientId.UniqueProcess);
    lpDebugEvent->dwThreadId = HandleToUlong(StateChange.AppClientId.UniqueThread);

    switch ( StateChange.NewState ) {

    case DbgCreateThreadStateChange :
        lpDebugEvent->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        lpDebugEvent->u.CreateThread.hThread =
            StateChange.StateInfo.CreateThread.HandleToThread;
        lpDebugEvent->u.CreateThread.lpStartAddress =
            (LPTHREAD_START_ROUTINE)StateChange.StateInfo.CreateThread.NewThread.StartAddress;
        Status = NtQueryInformationThread(
                    StateChange.StateInfo.CreateThread.HandleToThread,
                    ThreadBasicInformation,
                    &ThreadBasicInfo,
                    sizeof(ThreadBasicInfo),
                    NULL
                    );
        if (!NT_SUCCESS(Status)) {
            lpDebugEvent->u.CreateThread.lpThreadLocalBase = NULL;
            }
        else {
            lpDebugEvent->u.CreateThread.lpThreadLocalBase = ThreadBasicInfo.TebBaseAddress;
            }
        break;

    case DbgCreateProcessStateChange :
        lpDebugEvent->dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
        lpDebugEvent->u.CreateProcessInfo.hProcess =
            StateChange.StateInfo.CreateProcessInfo.HandleToProcess;
        lpDebugEvent->u.CreateProcessInfo.hThread =
            StateChange.StateInfo.CreateProcessInfo.HandleToThread;
        lpDebugEvent->u.CreateProcessInfo.hFile =
            StateChange.StateInfo.CreateProcessInfo.NewProcess.FileHandle;
        lpDebugEvent->u.CreateProcessInfo.lpBaseOfImage =
            StateChange.StateInfo.CreateProcessInfo.NewProcess.BaseOfImage;
        lpDebugEvent->u.CreateProcessInfo.dwDebugInfoFileOffset =
            StateChange.StateInfo.CreateProcessInfo.NewProcess.DebugInfoFileOffset;
        lpDebugEvent->u.CreateProcessInfo.nDebugInfoSize =
            StateChange.StateInfo.CreateProcessInfo.NewProcess.DebugInfoSize;
        lpDebugEvent->u.CreateProcessInfo.lpStartAddress =
            (LPTHREAD_START_ROUTINE)StateChange.StateInfo.CreateProcessInfo.NewProcess.InitialThread.StartAddress;
        Status = NtQueryInformationThread(
                    StateChange.StateInfo.CreateProcessInfo.HandleToThread,
                    ThreadBasicInformation,
                    &ThreadBasicInfo,
                    sizeof(ThreadBasicInfo),
                    NULL
                    );
        if (!NT_SUCCESS(Status)) {
            lpDebugEvent->u.CreateProcessInfo.lpThreadLocalBase = NULL;
            }
        else {
            lpDebugEvent->u.CreateProcessInfo.lpThreadLocalBase = ThreadBasicInfo.TebBaseAddress;
            }
        lpDebugEvent->u.CreateProcessInfo.lpImageName = NULL;
        lpDebugEvent->u.CreateProcessInfo.fUnicode = 1;
        break;

    case DbgExitThreadStateChange :
        lpDebugEvent->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
        lpDebugEvent->u.ExitThread.dwExitCode =
            (DWORD)StateChange.StateInfo.ExitThread.ExitStatus;
        break;

    case DbgExitProcessStateChange :
        lpDebugEvent->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        lpDebugEvent->u.ExitProcess.dwExitCode =
            (DWORD)StateChange.StateInfo.ExitProcess.ExitStatus;
        break;

    case DbgExceptionStateChange :
    case DbgBreakpointStateChange :
    case DbgSingleStepStateChange :
        if ( StateChange.StateInfo.Exception.ExceptionRecord.ExceptionCode ==
            DBG_PRINTEXCEPTION_C ) {
            lpDebugEvent->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;

            lpDebugEvent->u.DebugString.lpDebugStringData =
                (PVOID)StateChange.StateInfo.Exception.ExceptionRecord.ExceptionInformation[1];
            lpDebugEvent->u.DebugString.nDebugStringLength =
                (WORD)StateChange.StateInfo.Exception.ExceptionRecord.ExceptionInformation[0];
            lpDebugEvent->u.DebugString.fUnicode = (WORD)0;
            }
        else if ( StateChange.StateInfo.Exception.ExceptionRecord.ExceptionCode ==
            DBG_RIPEXCEPTION ) {
            lpDebugEvent->dwDebugEventCode = RIP_EVENT;

            lpDebugEvent->u.RipInfo.dwType =
                (DWORD)StateChange.StateInfo.Exception.ExceptionRecord.ExceptionInformation[1];
            lpDebugEvent->u.RipInfo.dwError =
                (DWORD)StateChange.StateInfo.Exception.ExceptionRecord.ExceptionInformation[0];
            }
        else {
            lpDebugEvent->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
            lpDebugEvent->u.Exception.ExceptionRecord =
                StateChange.StateInfo.Exception.ExceptionRecord;
            lpDebugEvent->u.Exception.dwFirstChance =
                StateChange.StateInfo.Exception.FirstChance;
            }
        break;

    case DbgLoadDllStateChange :
        lpDebugEvent->dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
        lpDebugEvent->u.LoadDll.lpBaseOfDll =
            StateChange.StateInfo.LoadDll.BaseOfDll;
        lpDebugEvent->u.LoadDll.hFile =
            StateChange.StateInfo.LoadDll.FileHandle;
        lpDebugEvent->u.LoadDll.dwDebugInfoFileOffset =
            StateChange.StateInfo.LoadDll.DebugInfoFileOffset;
        lpDebugEvent->u.LoadDll.nDebugInfoSize =
            StateChange.StateInfo.LoadDll.DebugInfoSize;
        {
            //
            // pick up the image name
            //

            InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
            Status = NtOpenThread(
                        &hThread,
                        THREAD_QUERY_INFORMATION,
                        &Obja,
                        &StateChange.AppClientId
                        );
            if ( NT_SUCCESS(Status) ) {
                Status = NtQueryInformationThread(
                            hThread,
                            ThreadBasicInformation,
                            &ThreadBasicInfo,
                            sizeof(ThreadBasicInfo),
                            NULL
                            );
                NtClose(hThread);
                }
            if ( NT_SUCCESS(Status) ) {
                lpDebugEvent->u.LoadDll.lpImageName = &ThreadBasicInfo.TebBaseAddress->NtTib.ArbitraryUserPointer;
                }
            else {
                lpDebugEvent->u.LoadDll.lpImageName = NULL;
                }
            lpDebugEvent->u.LoadDll.fUnicode = 1;
        }


        break;

    case DbgUnloadDllStateChange :
        lpDebugEvent->dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
        lpDebugEvent->u.UnloadDll.lpBaseOfDll =
            StateChange.StateInfo.UnloadDll.BaseAddress;
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
ContinueDebugEvent(
    DWORD dwProcessId,
    DWORD dwThreadId,
    DWORD dwContinueStatus
    )

/*++

Routine Description:

    A debugger can continue a thread that previously reported a debug
    event using ContinueDebugEvent.

    Upon successful completion of this API, the specified thread is
    continued.  Depending on the debug event previously reported by the
    thread certain side effects occur.

    If the continued thread previously reported an exit thread debug
    event, the handle that the debugger has to the thread is closed.

    If the continued thread previously reported an exit process debug
    event, the handles that the debugger has to the thread and to the
    process are closed.

Arguments:

    dwProcessId - Supplies the process id of the process to continue. The
        combination of process id and thread id must identify a thread that
        has previously reported a debug event.

    dwThreadId - Supplies the thread id of the thread to continue. The
        combination of process id and thread id must identify a thread that
        has previously reported a debug event.

    dwContinueStatus - Supplies the continuation status for the thread
        reporting the debug event.

        dwContinueStatus Values:

            DBG_CONTINUE - If the thread being continued had
                previously reported an exception event, continuing with
                this value causes all exception processing to stop and
                the thread continues execution.  For any other debug
                event, this continuation status simply allows the thread
                to continue execution.

            DBG_EXCEPTION_NOT_HANDLED - If the thread being continued
                had previously reported an exception event, continuing
                with this value causes exception processing to continue.
                If this is a first chance exception event, then
                structured exception handler search/dispatch logic is
                invoked.  Otherwise, the process is terminated.  For any
                other debug event, this continuation status simply
                allows the thread to continue execution.

            DBG_TERMINATE_THREAD - After all continue side effects are
                processed, this continuation status causes the thread to
                jump to a call to ExitThread.  The exit code is the
                value DBG_TERMINATE_THREAD.

            DBG_TERMINATE_PROCESS - After all continue side effects are
                processed, this continuation status causes the thread to
                jump to a call to ExitProcess.  The exit code is the
                value DBG_TERMINATE_PROCESS.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;
    CLIENT_ID ClientId;

    ClientId.UniqueProcess = (HANDLE)LongToHandle(dwProcessId);
    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);

    Status = DbgUiContinue(&ClientId,(NTSTATUS)dwContinueStatus);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

BOOL
APIENTRY
DebugActiveProcess(
    DWORD dwProcessId
    )

/*++

Routine Description:

    This API allows a debugger to attach to an active process and debug
    the process.  The debugger specifies the process that it wants to
    debug through the process id of the target process.  The debugger
    gets debug access to the process as if it had created the process
    with the DEBUG_ONLY_THIS_PROCESS creation flag.

    The debugger must have approriate access to the calling process such
    that it can open the process for PROCESS_ALL_ACCESS.  For Dos/Win32
    this never fails (the process id just has to be a valid process id).
    For NT/Win32 this check can fail if the target process was created
    with a security descriptor that denies the debugger approriate
    access.

    Once the process id check has been made and the system determines
    that a valid debug attachment is being made, this call returns
    success to the debugger.  The debugger is then expected to wait for
    debug events.  The system will suspend all threads in the process
    and feed the debugger debug events representing the current state of
    the process.

    The system will feed the debugger a single create process debug
    event representing the process specified by dwProcessId.  The
    lpStartAddress field of the create process debug event is NULL.  For
    each thread currently part of the process, the system will send a
    create thread debug event.  The lpStartAddress field of the create
    thread debug event is NULL.  For each DLL currently loaded into the
    address space of the target process, the system will send a LoadDll
    debug event.  The system will arrange for the first thread in the
    process to execute a breakpoint instruction after it is resumed.
    Continuing this thread causes the thread to return to whatever it
    was doing prior to the debug attach.

    After all of this has been done, the system resumes all threads within
    the process. When the first thread in the process resumes, it will
    execute a breakpoint instruction causing an exception debug event
    to be sent to the debugger.

    All future debug events are sent to the debugger using the normal
    mechanism and rules.


Arguments:

    dwProcessId - Supplies the process id of a process the caller
        wants to debug.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    HANDLE Process, Thread;
    NTSTATUS Status;
    DWORD ThreadId;
    PBASE_API_MSG m;
    PBASE_DEBUGPROCESS_MSG a;

    //
    // Determine if a valid process id has been specified. If
    // so than call the server to do the attach.
    //

    if ( dwProcessId != -1 ) {
        Process = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwProcessId);
        if ( !Process ) {
            return FALSE;
            }

        //
        // Call server to see if we can really debug this process
        //

        {

#if defined(BUILD_WOW6432)
            Status = CsrBasepDebugProcess(NtCurrentTeb()->ClientId,
                                          dwProcessId,
                                          NULL);
            if (!NT_SUCCESS(Status)) {
                SetLastError(ERROR_ACCESS_DENIED);
                CloseHandle(Process);
                return FALSE;
                }
#else
            BASE_API_MSG mm;
            PBASE_DEBUGPROCESS_MSG aa= (PBASE_DEBUGPROCESS_MSG)&mm.u.DebugProcess;

            aa->DebuggerClientId = NtCurrentTeb()->ClientId;
            aa->dwProcessId = dwProcessId;
            aa->AttachCompleteRoutine = NULL;

            CsrClientCallServer(
                         (PCSR_API_MSG)&mm,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDebugProcess
                                            ),
                         sizeof( BASE_DEBUGPROCESS_MSG )
                        );
            if (!NT_SUCCESS((NTSTATUS)mm.ReturnValue)) {
                SetLastError(ERROR_ACCESS_DENIED);
                CloseHandle(Process);
                return FALSE;
                }
#endif
        }
        CloseHandle(Process);
        }
    else {
        //
        // Call server to see if we can really debug this process
        //

        {

#if defined(BUILD_WOW6432)
            Status = CsrBasepDebugProcess(NtCurrentTeb()->ClientId,
                                          dwProcessId,
                                          NULL);
            if (!NT_SUCCESS(Status)) {
                SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
                }
#else
            BASE_API_MSG mm;
            PBASE_DEBUGPROCESS_MSG aa= (PBASE_DEBUGPROCESS_MSG)&mm.u.DebugProcess;

            aa->DebuggerClientId = NtCurrentTeb()->ClientId;
            aa->dwProcessId = dwProcessId;
            aa->AttachCompleteRoutine = NULL;

            CsrClientCallServer(
                         (PCSR_API_MSG)&mm,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDebugProcess
                                            ),
                         sizeof( BASE_DEBUGPROCESS_MSG )
                        );
            if (!NT_SUCCESS((NTSTATUS)mm.ReturnValue)) {
                SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
                }
#endif

        }

        }
    //
    // Connect to dbgss as a user interface
    //

    Status = DbgUiConnectToDbg();
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    m = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof(*m));
    if ( !m ) {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return FALSE;
        }
    a = (PBASE_DEBUGPROCESS_MSG)&m->u.DebugProcess;
    a->DebuggerClientId = NtCurrentTeb()->ClientId;
    a->dwProcessId = dwProcessId;
    a->AttachCompleteRoutine = (PVOID)BaseAttachComplete;

    Thread = CreateThread(
                NULL,
                0L,
                BaseDebugAttachThread,
                (LPVOID)m,
                0,
                &ThreadId
                );
    if ( !Thread ) {
        RtlFreeHeap(RtlProcessHeap(), 0,m);
        return FALSE;
        }
    CloseHandle(Thread);
    return TRUE;
}

DWORD
BaseDebugAttachThread(
    LPVOID ThreadParameter
    )

/*++

Routine Description:

    This thread is created as part of the debug attach procedure.  It
    runs in the context of the attaching debugger.  Its basic function
    is to call the server and block until the server completes the
    attachment. It then exits. This thread is needed because the debugger
    making the attach call must be free to service debug events from the
    server. Rather than have the server have added complexity of doing
    the debug attach procedure asynchronously, we grab a debugger thread
    to block.

Arguments:

    ThreadParameter - Supplies the address of the base api message Not used.

Return Value:

    None.

--*/

{

#if defined(BUILD_WOW6432)
    PBASE_DEBUGPROCESS_MSG a;
#else
    PBASE_API_MSG m;
#endif
    NTSTATUS Status;
    ULONG Response;

#if defined(BUILD_WOW6432)
    a = (PBASE_DEBUGPROCESS_MSG)ThreadParameter;

    Status = CsrBasepDebugProcess(a->DebuggerClientId,
                                  a->dwProcessId,
                                  a->AttachCompleteRoutine);
    if (!NT_SUCCESS(Status)) {
#else
    m = (PBASE_API_MSG)ThreadParameter;
    CsrClientCallServer( (PCSR_API_MSG)ThreadParameter,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDebugProcess
                                            ),
                         sizeof( BASE_DEBUGPROCESS_MSG )
                       );
    if (!NT_SUCCESS((NTSTATUS)m->ReturnValue)) {
#endif
        //
        // Unexpected attach failure
        //

        Status =NtRaiseHardError( STATUS_DEBUG_ATTACH_FAILED,
                                  0,
                                  0,
                                  NULL,
                                  OptionOkCancel,
                                  &Response
                                );

        if ( NT_SUCCESS(Status) && Response == ResponseOk ) {
            ExitProcess(Status);
            }
        }
    RtlFreeHeap(RtlProcessHeap(), 0,ThreadParameter);
    ExitThread((DWORD)0);
    return 0;
}

VOID
BaseAttachComplete(
    PCONTEXT Context
    )

/*++

Routine Description:

    This function is remote called to after a successful debug attach. Its
    purpose is to issue a breakpoint and the continue.

Arguments:

    Context - Supplies the context record that is to be restored upon
        completion of this API.

Return Value:

    None.

--*/

{
    HANDLE DebugPort;
    NTSTATUS Status;

    DebugPort = (HANDLE)NULL;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDebugPort,
                (PVOID)&DebugPort,
                sizeof(DebugPort),
                NULL
                );

    if ( NT_SUCCESS(Status) && DebugPort ) {
        DbgBreakPoint();
        }
    if ( !Context ) {
        ExitThread(0);
        ASSERT(FALSE);
        }
    else {
        NtContinue(Context,FALSE);
        }
}

BOOL
APIENTRY
GetThreadSelectorEntry(
    HANDLE hThread,
    DWORD dwSelector,
    LPLDT_ENTRY lpSelectorEntry
    )

/*++

Routine Description:

    This function is used to return a descriptor table entry for the
    specified thread corresponding to the specified selector.

    This API is only functional on x86 based systems. For non x86 based
    systems. A value of FALSE is returned.

    This API is used by a debugger so that it can convert segment
    relative addresses to linear virtual address (since this is the only
    format supported by ReadProcessMemory and WriteProcessMemory.

Arguments:

    hThread - Supplies a handle to the thread that contains the
        specified selector.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

    dwSelector - Supplies the selector value to lookup.  The selector
        value may be a global selector or a local selector.

    lpSelectorEntry - If the specified selector is contained withing the
        threads descriptor tables, this parameter returns the selector
        entry corresponding to the specified selector value.  This data
        can be used to compute the linear base address that segment
        relative addresses refer to.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
#if defined(i386)

    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    NTSTATUS Status;

    DescriptorEntry.Selector = dwSelector;
    Status = NtQueryInformationThread(
                hThread,
                ThreadDescriptorTableEntry,
                &DescriptorEntry,
                sizeof(DescriptorEntry),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    *lpSelectorEntry = DescriptorEntry.Descriptor;
    return TRUE;

#else
    BaseSetLastNTError(STATUS_NOT_SUPPORTED);
    return FALSE;
#endif // i386

}
