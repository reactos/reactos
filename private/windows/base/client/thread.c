/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thread.c

Abstract:

    This module implements Win32 Thread Object APIs

Author:

    Mark Lucovsky (markl) 21-Sep-1990

Revision History:

--*/

#include "basedll.h"

HANDLE BasepDefaultTimerQueue ;
ULONG BasepTimerQueueInitFlag ;
ULONG BasepTimerQueueDoneFlag ;

HANDLE
APIENTRY
CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    A thread object can be created to execute within the address space of the
    calling process using CreateThread.

    See CreateRemoteThread for a description of the arguments and return value.

--*/
{
    return CreateRemoteThread( NtCurrentProcess(),
                               lpThreadAttributes,
                               dwStackSize,
                               lpStartAddress,
                               lpParameter,
                               dwCreationFlags,
                               lpThreadId
                             );
}

HANDLE
APIENTRY
CreateRemoteThread(
    HANDLE hProcess,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    A thread object can be created to execute within the address space of the
    another process using CreateRemoteThread.

    Creating a thread causes a new thread of execution to begin in the address
    space of the current process. The thread has access to all objects opened
    by the process.

    The thread begins executing at the address specified by the StartAddress
    parameter. If the thread returns from this procedure, the results are
    un-specified.

    The thread remains in the system until it has terminated and
    all handles to the thread
    have been closed through a call to CloseHandle.

    When a thread terminates, it attains a state of signaled satisfying all
    waits on the object.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for thread objects:

        - THREAD_QUERY_INFORMATION - This access is required to read
          certain information from the thread object.

        - SYNCHRONIZE - This access is required to wait on a thread
          object.

        - THREAD_GET_CONTEXT - This access is required to read the
          context of a thread using GetThreadContext.

        - THREAD_SET_CONTEXT - This access is required to write the
          context of a thread using SetThreadContext.

        - THREAD_SUSPEND_RESUME - This access is required to suspend or
          resume a thread using SuspendThread or ResumeThread.

        - THREAD_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for a thread object.

Arguments:

    hProcess - Supplies the handle to the process in which the thread is
        to be create in.

    lpThreadAttributes - An optional parameter that may be used to specify
        the attributes of the new thread.  If the parameter is not
        specified, then the thread is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    dwStackSize - Supplies the size in bytes of the stack for the new thread.
        A value of zero specifies that the thread's stack size should be
        the same size as the stack size of the first thread in the process.
        This size is specified in the application's executable file.

    lpStartAddress - Supplies the starting address of the new thread.  The
        address is logically a procedure that never returns and that
        accepts a single 32-bit pointer argument.

    lpParameter - Supplies a single parameter value passed to the thread.

    dwCreationFlags - Supplies additional flags that control the creation
        of the thread.

        dwCreationFlags Flags:

        CREATE_SUSPENDED - The thread is created in a suspended state.
            The creator can resume this thread using ResumeThread.
            Until this is done, the thread will not begin execution.

    lpThreadId - Returns the thread identifier of the thread.  The
        thread ID is valid until the thread terminates.

Return Value:

    NON-NULL - Returns a handle to the new thread.  The handle has full
        access to the new thread and may be used in any API that
        requires a handle to a thread object.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    CONTEXT ThreadContext;
    INITIAL_TEB InitialTeb;
    CLIENT_ID ClientId;
    ULONG i;

#if !defined(BUILD_WOW6432)
    BASE_API_MSG m;
    PBASE_CREATETHREAD_MSG a = (PBASE_CREATETHREAD_MSG)&m.u.CreateThread;
#endif

#if defined(WX86) || defined(_AXP64_)
    BOOL bWx86 = FALSE;
    HANDLE Wx86Info;
    PWX86TIB Wx86Tib;
#endif



    //
    // Allocate a stack for this thread in the address space of the target
    // process.
    //

    Status = BaseCreateStack(
                hProcess,
                dwStackSize,
                0L,
                &InitialTeb
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }

    //
    // Create an initial context for the new thread.
    //

    BaseInitializeContext(
        &ThreadContext,
        lpParameter,
        (PVOID)lpStartAddress,
        InitialTeb.StackBase,
        BaseContextTypeThread
        );

    pObja = BaseFormatObjectAttributes(&Obja,lpThreadAttributes,NULL);


    Status = NtCreateThread(
                &Handle,
                THREAD_ALL_ACCESS,
                pObja,
                hProcess,
                &ClientId,
                &ThreadContext,
                &InitialTeb,
                TRUE
                );
    if (!NT_SUCCESS(Status)) {
        BaseFreeThreadStack(hProcess,NULL, &InitialTeb);
        BaseSetLastNTError(Status);
        return NULL;
        }



    try {


#if defined(WX86) || defined(_AXP64_)

        //
        // Check the Target Processes to see if this is a Wx86 process
        //
        Status = NtQueryInformationProcess(hProcess,
                                           ProcessWx86Information,
                                           &Wx86Info,
                                           sizeof(Wx86Info),
                                           NULL
                                           );
        if (!NT_SUCCESS(Status)) {
            leave;
            }

        Wx86Tib = (PWX86TIB)NtCurrentTeb()->Vdm;

        //
        // if Wx86 process, setup for emulation
        //
        if ((ULONG_PTR)Wx86Info == sizeof(WX86TIB)) {

            //
            // create a WX86Tib and initialize it's Teb->Vdm.
            //
            Status = BaseCreateWx86Tib(hProcess,
                                       Handle,
                                       (ULONG)((ULONG_PTR)lpStartAddress),
                                       dwStackSize,
                                       0L,
                                       (Wx86Tib &&
                                        Wx86Tib->Size == sizeof(WX86TIB) &&
                                        Wx86Tib->EmulateInitialPc)
                                       );
            if (!NT_SUCCESS(Status)) {
                leave;
                }

            bWx86 = TRUE;

            }
        else if (Wx86Tib && Wx86Tib->EmulateInitialPc) {

            //
            // if not Wx86 process, and caller wants to call x86 code in that
            // process, fail the call.
            //
            Status = STATUS_ACCESS_DENIED;
            leave;

            }

#endif  // WX86


        //
        // Call the Windows server to let it know about the
        // process.
        //
        if ( !BaseRunningInServerProcess ) {

#if defined(BUILD_WOW6432)
            Status = CsrBasepCreateThread(Handle,
                                          ClientId
                                          );
#else
            a->ThreadHandle = Handle;
            a->ClientId = ClientId;
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 NULL,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepCreateThread
                                                    ),
                                 sizeof( *a )
                               );

            Status = m.ReturnValue;
#endif
        }

        else {
            if (hProcess != NtCurrentProcess()) {
                CSRREMOTEPROCPROC ProcAddress;
                ProcAddress = (CSRREMOTEPROCPROC)GetProcAddress(
                                                    GetModuleHandleA("csrsrv"),
                                                    "CsrCreateRemoteThread"
                                                    );
                if (ProcAddress) {
                    Status = (ProcAddress)(Handle, &ClientId);
                    }
                }
            }


        if (!NT_SUCCESS(Status)) {
            Status = (NTSTATUS)STATUS_NO_MEMORY;
            }
        else {

            if ( ARGUMENT_PRESENT(lpThreadId) ) {
                *lpThreadId = HandleToUlong(ClientId.UniqueThread);
                }

            if (!( dwCreationFlags & CREATE_SUSPENDED) ) {
                NtResumeThread(Handle,&i);
                }
            }

        }
    finally {
        if (!NT_SUCCESS(Status)) {
            BaseFreeThreadStack(hProcess,
                                Handle,
                                &InitialTeb
                                );

            NtTerminateThread(Handle, Status);
            NtClose(Handle);
            BaseSetLastNTError(Status);
            Handle = NULL;
            }
        }


    return Handle;

}

NTSTATUS
NTAPI
BaseCreateThreadPoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    HANDLE * ThreadHandle
    )
{
    ULONG Initialized ;
    LARGE_INTEGER TimeOut ;
    DWORD tid ;

    Initialized = FALSE ;

    *ThreadHandle = CreateRemoteThread(
                        NtCurrentProcess(),
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) Function,
                        &Initialized,
                        0,
                        &tid );

    //
    // The thread will set the Initialized flag to 1 when it is initialized.  We
    // cycle here, yielding our quantum until

    if ( *ThreadHandle ) {

        TimeOut.QuadPart = -1 * ( 10 * 10000 ) ;

        while (! (volatile ULONG) Initialized) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

        if (Initialized == 1)
            return STATUS_SUCCESS ;

    }

    return NtCurrentTeb()->LastStatusValue ;
}

NTSTATUS
NTAPI
BaseExitThreadPoolThread(
    NTSTATUS Status
    )
{
    ExitThread( (DWORD) Status );
    return STATUS_SUCCESS ;
}

HANDLE
WINAPI
OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )

/*++

Routine Description:

    A handle to a thread object may be created using OpenThread.

    Opening a thread creates a handle to the specified thread.
    Associated with the thread handle is a set of access rights that
    may be performed using the thread handle.  The caller specifies the
    desired access to the thread using the DesiredAccess parameter.

Arguments:

    mDesiredAccess - Supplies the desired access to the thread object.
        For NT/Win32, this access is checked against any security
        descriptor on the target thread.  The following object type
        specific access flags can be specified in addition to the
        STANDARD_RIGHTS_REQUIRED access flags.

        DesiredAccess Flags:

        THREAD_TERMINATE - This access is required to terminate the
            thread using TerminateThread.

        THREAD_SUSPEND_RESUME - This access is required to suspend and
            resume the thread using SuspendThread and ResumeThread.

        THREAD_GET_CONTEXT - This access is required to use the
            GetThreadContext API on a thread object.

        THREAD_SET_CONTEXT - This access is required to use the
            SetThreadContext API on a thread object.

        THREAD_SET_INFORMATION - This access is required to set certain
            information in the thread object.

        THREAD_SET_THREAD_TOKEN - This access is required to set the
            thread token using SetTokenInformation.

        THREAD_QUERY_INFORMATION - This access is required to read
            certain information from the thread object.

        SYNCHRONIZE - This access is required to wait on a thread object.

        THREAD_ALL_ACCESS - This set of access flags specifies all of the
            possible access flags for a thread object.

    bInheritHandle - Supplies a flag that indicates whether or not the
        returned handle is to be inherited by a new process during
        process creation.  A value of TRUE indicates that the new
        process will inherit the handle.

    dwThreadId - Supplies the thread id of the thread to open.

Return Value:

    NON-NULL - Returns an open handle to the specified thread.  The
        handle may be used by the calling process in any API that
        requires a handle to a thread.  If the open is successful, the
        handle is granted access to the thread object only to the
        extent that it requested access through the DesiredAccess
        parameter.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);
    ClientId.UniqueProcess = (HANDLE)NULL;

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenThread(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}


BOOL
APIENTRY
SetThreadPriority(
    HANDLE hThread,
    int nPriority
    )

/*++

Routine Description:

    The specified thread's priority can be set using SetThreadPriority.

    A thread's priority may be set using SetThreadPriority.  This call
    allows the thread's relative execution importance to be communicated
    to the system.  The system normally schedules threads according to
    their priority.  The system is free to temporarily boost the
    priority of a thread when signifigant events occur (e.g.  keyboard
    or mouse input...).  Similarly, as a thread runs without blocking,
    the system will decay its priority.  The system will never decay the
    priority below the value set by this call.

    In the absence of system originated priority boosts, threads will be
    scheduled in a round-robin fashion at each priority level from
    THREAD_PRIORITY_TIME_CRITICAL to THREAD_PRIORITY_IDLE.  Only when there
    are no runnable threads at a higher level, will scheduling of
    threads at a lower level take place.

    All threads initially start at THREAD_PRIORITY_NORMAL.

    If for some reason the thread needs more priority, it can be
    switched to THREAD_PRIORITY_ABOVE_NORMAL or THREAD_PRIORITY_HIGHEST.
    Switching to THREAD_PRIORITY_TIME_CRITICAL should only be done in extreme
    situations.  Since these threads are given the highes priority, they
    should only run in short bursts.  Running for long durations will
    soak up the systems processing bandwidth starving threads at lower
    levels.

    If a thread needs to do low priority work, or should only run there
    is nothing else to do, its priority should be set to
    THREAD_PRIORITY_BELOW_NORMAL or THREAD_PRIORITY_LOWEST.  For extreme
    cases, THREAD_PRIORITY_IDLE can be used.

    Care must be taken when manipulating priorites.  If priorities are
    used carelessly (every thread is set to THREAD_PRIORITY_TIME_CRITICAL),
    the effects of priority modifications can produce undesireable
    effects (e.g.  starvation, no effect...).

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    nPriority - Supplies the priority value for the thread.  The
        following five priority values (ordered from lowest priority to
        highest priority) are allowed.

        nPriority Values:

        THREAD_PRIORITY_IDLE - The thread's priority should be set to
            the lowest possible settable priority.

        THREAD_PRIORITY_LOWEST - The thread's priority should be set to
            the next lowest possible settable priority.

        THREAD_PRIORITY_BELOW_NORMAL - The thread's priority should be
            set to just below normal.

        THREAD_PRIORITY_NORMAL - The thread's priority should be set to
            the normal priority value.  This is the value that all
            threads begin execution at.

        THREAD_PRIORITY_ABOVE_NORMAL - The thread's priority should be
            set to just above normal priority.

        THREAD_PRIORITY_HIGHEST - The thread's priority should be set to
            the next highest possible settable priority.

        THREAD_PRIORITY_TIME_CRITICAL - The thread's priority should be set
            to the highest possible settable priority.  This priority is
            very likely to interfere with normal operation of the
            system.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.
--*/

{
    NTSTATUS Status;
    LONG BasePriority;

    BasePriority = (LONG)nPriority;


    //
    // saturation is indicated by calling with a value of 16 or -16
    //

    if ( BasePriority == THREAD_PRIORITY_TIME_CRITICAL ) {
        BasePriority = ((HIGH_PRIORITY + 1) / 2);
        }
    else if ( BasePriority == THREAD_PRIORITY_IDLE ) {
        BasePriority = -((HIGH_PRIORITY + 1) / 2);
        }
    Status = NtSetInformationThread(
                hThread,
                ThreadBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

int
APIENTRY
GetThreadPriority(
    HANDLE hThread
    )

/*++

Routine Description:

    The specified thread's priority can be read using GetThreadPriority.

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

Return Value:

    The value of the thread's current priority is returned.  If an error
    occured, the value THREAD_PRIORITY_ERROR_RETURN is returned.
    Extended error status is available using GetLastError.

--*/

{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION BasicInfo;
    int returnvalue;

    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (int)THREAD_PRIORITY_ERROR_RETURN;
        }

    returnvalue = (int)BasicInfo.BasePriority;
    if ( returnvalue == ((HIGH_PRIORITY + 1) / 2) ) {
        returnvalue = THREAD_PRIORITY_TIME_CRITICAL;
        }
    else if ( returnvalue == -((HIGH_PRIORITY + 1) / 2) ) {
        returnvalue = THREAD_PRIORITY_IDLE;
        }
    return returnvalue;
}

BOOL
WINAPI
SetThreadPriorityBoost(
    HANDLE hThread,
    BOOL bDisablePriorityBoost
    )
{
    NTSTATUS Status;
    ULONG DisableBoost;

    DisableBoost = bDisablePriorityBoost ? 1 : 0;

    Status = NtSetInformationThread(
                hThread,
                ThreadPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;

}

BOOL
WINAPI
GetThreadPriorityBoost(
    HANDLE hThread,
    PBOOL pDisablePriorityBoost
    )
{
    NTSTATUS Status;
    DWORD DisableBoost;

    Status = NtQueryInformationThread(
                hThread,
                ThreadPriorityBoost,
                &DisableBoost,
                sizeof(DisableBoost),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }


    *pDisablePriorityBoost = DisableBoost;

    return TRUE;
}

VOID
APIENTRY
ExitThread(
    DWORD dwExitCode
    )

/*++

Routine Description:

    The current thread can exit using ExitThread.

    ExitThread is the prefered method of exiting a thread.  When this
    API is called (either explicitly or by returning from a thread
    procedure), The current thread's stack is deallocated and the thread
    terminates.  If the thread is the last thread in the process when
    this API is called, the behavior of this API does not change.  DLLs
    are not notified as a result of a call to ExitThread.

Arguments:

    dwExitCode - Supplies the termination status for the thread.

Return Value:

    None.

--*/

{
    MEMORY_BASIC_INFORMATION MemInfo;
    NTSTATUS st;
    ULONG LastThread;

#if DBG
    PRTL_CRITICAL_SECTION LoaderLock;

    //
    // Assert on exiting while holding loader lock
    //
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) {
        ASSERT(NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);
        }
#endif
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadAmILastThread,
            &LastThread,
            sizeof(LastThread),
            NULL
            );
    if ( st == STATUS_SUCCESS && LastThread ) {
        ExitProcess(dwExitCode);
        }
    else {
#if DBG
        RtlCheckForOrphanedCriticalSections(NtCurrentThread());
#endif
        LdrShutdownThread();
        if ( NtCurrentTeb()->TlsExpansionSlots ) {
            //
            // Serialize with TlsXXX functions so the kernel code that zero's tls slots
            // wont' trash heap
            //
            RtlAcquirePebLock();
            try {
                RtlFreeHeap(RtlProcessHeap(),0,NtCurrentTeb()->TlsExpansionSlots);
                NtCurrentTeb()->TlsExpansionSlots = NULL;
                }
            finally {
                RtlReleasePebLock();
                }
            }
        st = NtQueryVirtualMemory(
                NtCurrentProcess(),
                NtCurrentTeb()->NtTib.StackLimit,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            RtlRaiseStatus(st);
            }

#ifdef _ALPHA_
        //
        // Note stacks on Alpha must be octaword aligned. Probably
        // a good idea on other platforms as well.
        //
        BaseSwitchStackThenTerminate(
            MemInfo.AllocationBase,
            (PVOID)(((ULONG_PTR)&NtCurrentTeb()->User32Reserved[0] - 0x10) & ~0xf),
            dwExitCode
            );
#else // _ALPHA
        //
        // Note stacks on i386 need not be octaword aligned.
        //
        BaseSwitchStackThenTerminate(
            MemInfo.AllocationBase,
            &NtCurrentTeb()->UserReserved[0],
            dwExitCode
            );
#endif // _ALPHA_
        }
}



BOOL
APIENTRY
TerminateThread(
    HANDLE hThread,
    DWORD dwExitCode
    )

/*++

Routine Description:

    A thread may be terminated using TerminateThread.

    TerminateThread is used to cause a thread to terminate user-mode
    execution.  There is nothing a thread can to to predict or prevent
    when this occurs.  If a process has a handle with appropriate
    termination access to the thread or to the threads process, then the
    thread can be unconditionally terminated without notice.  When this
    occurs, the target thread has no chance to execute any user-mode
    code and its initial stack is not deallocated.  The thread attains a
    state of signaled satisfying any waits on the thread.  The thread's
    termination status is updated from its initial value of
    STATUS_PENDING to the value of the TerminationStatus parameter.
    Terminating a thread does not remove a thread from the system.  The
    thread is not removed from the system until the last handle to the
    thread is closed.

Arguments:

    hThread - Supplies a handle to the thread to terminate.  The handle
        must have been created with THREAD_TERMINATE access.

    dwExitCode - Supplies the termination status for the thread.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    NTSTATUS Status;
#if DBG
    PRTL_CRITICAL_SECTION LoaderLock;
    HANDLE ThreadId;
    THREAD_BASIC_INFORMATION ThreadInfo;
#endif

    if ( hThread == NULL ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }
#if DBG
    //
    // Assert on suicide while holding loader lock
    //
    LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) {
        Status = NtQueryInformationThread(
                                hThread,
                                ThreadBasicInformation,
                                &ThreadInfo,
                                sizeof(ThreadInfo),
                                NULL
                                );

        if (NT_SUCCESS(Status)) {
            ASSERT( NtCurrentTeb()->ClientId.UniqueThread != ThreadInfo.ClientId.UniqueThread ||
                    NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);
            }
        }
    RtlCheckForOrphanedCriticalSections(hThread);
#endif

    Status = NtTerminateThread(hThread,(NTSTATUS)dwExitCode);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
APIENTRY
GetExitCodeThread(
    HANDLE hThread,
    LPDWORD lpExitCode
    )

/*++

Routine Description:

    The termination status of a thread can be read using
    GetExitCodeThread.

    If a Thread is in the signaled state, calling this function returns
    the termination status of the thread.  If the thread is not yet
    signaled, the termination status returned is STILL_ACTIVE.

Arguments:

    hThread - Supplies a handle to the thread whose termination status is
        to be read.  The handle must have been created with
        THREAD_QUERY_INFORMATION access.

    lpExitCode - Returns the current termination status of the
        thread.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION BasicInformation;

    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );

    if ( NT_SUCCESS(Status) ) {
        *lpExitCode = BasicInformation.ExitStatus;
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

HANDLE
APIENTRY
GetCurrentThread(
    VOID
    )

/*++

Routine Description:

    A pseudo handle to the current thread may be retrieved using
    GetCurrentThread.

    A special constant is exported by Win32 that is interpreted as a
    handle to the current thread.  This handle may be used to specify
    the current thread whenever a thread handle is required.  On Win32,
    this handle has THREAD_ALL_ACCESS to the current thread.  On
    NT/Win32, this handle has the maximum access allowed by any security
    descriptor placed on the current thread.

Arguments:

    None.

Return Value:

    Returns the pseudo handle of the current thread.

--*/

{
    return NtCurrentThread();
}

DWORD
APIENTRY
GetCurrentThreadId(
    VOID
    )

/*++

Routine Description:

The thread ID of the current thread may be retrieved using
GetCurrentThreadId.

Arguments:

    None.

Return Value:

    Returns a unique value representing the thread ID of the currently
    executing thread.  The return value may be used to identify a thread
    in the system.

--*/

{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

BOOL
APIENTRY
GetThreadContext(
    HANDLE hThread,
    LPCONTEXT lpContext
    )

/*++

Routine Description:

    The context of a specified thread can be retreived using
    GetThreadContext.

    This function is used to retreive the context of the specified
    thread.  The API allows selective context to be retrieved based on
    the value of the ContextFlags field of the context structure.  The
    specified thread does not have to be being debugged in order for
    this API to operate.  The caller must simply have a handle to the
    thread that was created with THREAD_GET_CONTEXT access.

Arguments:

    hThread - Supplies an open handle to a thread whose context is to be
        retreived.  The handle must have been created with
        THREAD_GET_CONTEXT access to the thread.

    lpContext - Supplies the address of a context structure that
        receives the appropriate context of the specified thread.  The
        value of the ContextFlags field of this structure specifies
        which portions of a threads context are to be retreived.  The
        context structure is highly machine specific.  There are
        currently two versions of the context structure.  One version
        exists for x86 processors, and another exists for MIPS
        processors.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;

    Status = NtGetContextThread(hThread,lpContext);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
APIENTRY
SetThreadContext(
    HANDLE hThread,
    CONST CONTEXT *lpContext
    )

/*++

Routine Description:

    This function is used to set the context in the specified thread.
    The API allows selective context to be set based on the value of the
    ContextFlags field of the context structure.  The specified thread
    does not have to be being debugged in order for this API to operate.
    The caller must simply have a handle to the thread that was created
    with THREAD_SET_CONTEXT access.

Arguments:

    hThread - Supplies an open handle to a thread whose context is to be
        written.  The handle must have been created with
        THREAD_SET_CONTEXT access to the thread.

    lpContext - Supplies the address of a context structure that
        contains the context that is to be set in the specified thread.
        The value of the ContextFlags field of this structure specifies
        which portions of a threads context are to be set.  Some values
        in the context structure are not settable and are silently set
        to the correct value.  This includes cpu status register bits
        that specify the priviledged processor mode, debug register
        global enabling bits, and other state that must be completely
        controlled by the operating system.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetContextThread(hThread,(PCONTEXT)lpContext);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

DWORD
APIENTRY
SuspendThread(
    HANDLE hThread
    )

/*++

Routine Description:

    A thread can be suspended using SuspendThread.

    Suspending a thread causes the thread to stop executing user-mode
    (or application) code.  Each thread has a suspend count (with a
    maximum value of MAXIMUM_SUSPEND_COUNT).  If the suspend count is
    greater than zero, the thread is suspended; otherwise, the thread is
    not suspended and is eligible for execution.

    Calling SuspendThread causes the target thread's suspend count to
    increment.  Attempting to increment past the maximum suspend count
    causes an error without incrementing the count.

Arguments:

    hThread - Supplies a handle to the thread that is to be suspended.
        The handle must have been created with THREAD_SUSPEND_RESUME
        access to the thread.

Return Value:

    -1 - The operation failed.  Extended error status is available using
         GetLastError.

    Other - The target thread was suspended. The return value is the thread's
        previous suspend count.

--*/

{
    NTSTATUS Status;
    DWORD PreviousSuspendCount;

    Status = NtSuspendThread(hThread,&PreviousSuspendCount);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
    else {
        return PreviousSuspendCount;
        }
}

DWORD
APIENTRY
ResumeThread(
    IN HANDLE hThread
    )

/*++

Routine Description:

    A thread can be resumed using ResumeThread.

    Resuming a thread object checks the suspend count of the subject
    thread.  If the suspend count is zero, then the thread is not
    currently suspended and no operation is performed.  Otherwise, the
    subject thread's suspend count is decremented.  If the resultant
    value is zero , then the execution of the subject thread is resumed.

    The previous suspend count is returned as the function value.  If
    the return value is zero, then the subject thread was not previously
    suspended.  If the return value is one, then the subject thread's
    the subject thread is still suspended and must be resumed the number
    of times specified by the return value minus one before it will
    actually resume execution.

    Note that while reporting debug events, all threads withing the
    reporting process are frozen.  This has nothing to do with
    SuspendThread or ResumeThread.  Debuggers are expected to use
    SuspendThread and ResumeThread to limit the set of threads that can
    execute within a process.  By suspending all threads in a process
    except for the one reporting a debug event, it is possible to
    "single step" a single thread.  The other threads will not be
    released by a continue if they are suspended.

Arguments:

    hThread - Supplies a handle to the thread that is to be resumed.
        The handle must have been created with THREAD_SUSPEND_RESUME
        access to the thread.

Return Value:

    -1 - The operation failed.  Extended error status is available using
        GetLastError.

    Other - The target thread was resumed (or was not previously
        suspended).  The return value is the thread's previous suspend
        count.

--*/

{
    NTSTATUS Status;
    DWORD PreviousSuspendCount;

    Status = NtResumeThread(hThread,&PreviousSuspendCount);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
    else {
        return PreviousSuspendCount;
        }
}

VOID
APIENTRY
RaiseException(
    DWORD dwExceptionCode,
    DWORD dwExceptionFlags,
    DWORD nNumberOfArguments,
    CONST ULONG_PTR *lpArguments
    )

/*++

Routine Description:

    Raising an exception causes the exception dispatcher to go through
    its search for an exception handler.  This includes debugger
    notification, frame based handler searching, and system default
    actions.

Arguments:

    dwExceptionCode - Supplies the exception code of the exception being
        raised.  This value may be obtained in exception filters and in
        exception handlers by calling GetExceptionCode.

    dwExceptionFlags - Supplies a set of flags associated with the exception.

    dwExceptionFlags Flags:

        EXCEPTION_NONCONTINUABLE - The exception is non-continuable.
            Returning EXCEPTION_CONTINUE_EXECUTION from an exception
            marked in this way causes the
            STATUS_NONCONTINUABLE_EXCEPTION exception.

    nNumberOfArguments - Supplies the number of arguments associated
        with the exception.  This value may not exceed
        EXCEPTION_MAXIMUM_PARAMETERS.  This parameter is ignored if
        lpArguments is NULL.

    lpArguments - An optional parameter, that if present supplies the
        arguments for the exception.

Return Value:

    None.

--*/

{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG n;
    PULONG_PTR s,d;
    ExceptionRecord.ExceptionCode = (DWORD)dwExceptionCode;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)RaiseException;
    if ( ARGUMENT_PRESENT(lpArguments) ) {
        n =  nNumberOfArguments;
        if ( n > EXCEPTION_MAXIMUM_PARAMETERS ) {
            n = EXCEPTION_MAXIMUM_PARAMETERS;
            }
        ExceptionRecord.NumberParameters = n;
        s = (PULONG_PTR)lpArguments;
        d = ExceptionRecord.ExceptionInformation;
        while(n--){
            *d++ = *s++;
            }
        }
    else {
        ExceptionRecord.NumberParameters = 0;
        }
    RtlRaiseException(&ExceptionRecord);
}


UINT
GetErrorMode();

BOOLEAN BasepAlreadyHadHardError = FALSE;

LPTOP_LEVEL_EXCEPTION_FILTER BasepCurrentTopLevelFilter;

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )

/*++

Routine Description:

    This function allows an application to supersede the top level
    exception handler that Win32 places at the top of each thread and
    process.

    If an exception occurs, and it makes it to the Win32 unhandled
    exception filter, and the process is not being debugged, the Win32
    filter will call the unhandled exception filter specified by
    lpTopLevelExceptionFilter.

    This filter may return:

        EXCEPTION_EXECUTE_HANDLER - Return from the Win32
            UnhandledExceptionFilter and execute the associated
            exception handler.  This will usually result in process
            termination

        EXCEPTION_CONTINUE_EXECUTION - Return from the Win32
            UnhandledExceptionFilter and continue execution from the
            point of the exception.  The filter is of course free to
            modify the continuation state my modifying the passed
            exception information.

        EXCEPTION_CONTINUE_SEARCH - Proceed with normal execution of the
            Win32 UnhandledExceptionFilter.  e.g.  obey the SetErrorMode
            flags, or invoke the Application Error popup.

    This function is not a general vectored exception handling
    mechanism.  It is intended to be used to establish a per-process
    exception filter that can monitor unhandled exceptions at the
    process level and respond to these exceptions appropriately.

Arguments:

    lpTopLevelExceptionFilter - Supplies the address of a top level
        filter function that will be called whenever the Win32
        UnhandledExceptionFilter gets control, and the process is NOT
        being debugged.  A value of NULL specifies default handling
        within the Win32 UnhandledExceptionFilter.


Return Value:

    This function returns the address of the previous exception filter
    established with this API.  A value of NULL means that there is no
    current top level handler.

--*/

{
    LPTOP_LEVEL_EXCEPTION_FILTER PreviousTopLevelFilter;

    PreviousTopLevelFilter = BasepCurrentTopLevelFilter;
    BasepCurrentTopLevelFilter = lpTopLevelExceptionFilter;

    return PreviousTopLevelFilter;
}

LONG
BasepCheckForReadOnlyResource(
    PVOID Va
    )
{
    SIZE_T RegionSize;
    ULONG OldProtect;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemInfo;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
    ULONG ResourceSize;
    char *rbase, *va;
    LONG ReturnValue;

    //
    // Locate the base address that continas this va
    //

    Status = NtQueryVirtualMemory(
                NtCurrentProcess(),
                Va,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return EXCEPTION_CONTINUE_SEARCH;
        }

    //
    // if the va is readonly and in an image then continue
    //

    if ( !((MemInfo.Protect == PAGE_READONLY) && (MemInfo.Type == MEM_IMAGE)) ){
        return EXCEPTION_CONTINUE_SEARCH;
        }

    ReturnValue = EXCEPTION_CONTINUE_SEARCH;

    try {
        ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)
            RtlImageDirectoryEntryToData(MemInfo.AllocationBase,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                         &ResourceSize
                                         );

        rbase = (char *)ResourceDirectory;
        va = (char *)Va;

        if ( rbase && va >= rbase && va < rbase+ResourceSize ) {
            RegionSize = 1;
            Status = NtProtectVirtualMemory(
                        NtCurrentProcess(),
                        &va,
                        &RegionSize,
                        PAGE_READWRITE,
                        &OldProtect
                        );
            if ( NT_SUCCESS(Status) ) {
                ReturnValue = EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ;
        }

    return ReturnValue;
}

LONG
UnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    NTSTATUS Status;
    ULONG_PTR Parameters[ 4 ];
    ULONG Response;
    HANDLE DebugPort;
    CHAR AeDebuggerCmdLine[256];
    CHAR AeAutoDebugString[8];
    BOOLEAN AeAutoDebug;
    ULONG ResponseFlag;
    LONG FilterReturn;
    PRTL_CRITICAL_SECTION PebLockPointer;
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimit;

    //
    // If we take a write fault, then attampt to make the memory writable. If this
    // succeeds, then silently proceed
    //

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION
        && ExceptionInfo->ExceptionRecord->ExceptionInformation[0] ) {

        FilterReturn = BasepCheckForReadOnlyResource((PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);

        if ( FilterReturn == EXCEPTION_CONTINUE_EXECUTION ) {
            return FilterReturn;
            }
        }

    //
    // If the process is being debugged, just let the exception happen
    // so that the debugger can see it. This way the debugger can ignore
    // all first chance exceptions.
    //

    DebugPort = (HANDLE)NULL;
    Status = NtQueryInformationProcess(
                GetCurrentProcess(),
                ProcessDebugPort,
                (PVOID)&DebugPort,
                sizeof(DebugPort),
                NULL
                );

    if ( NT_SUCCESS(Status) && DebugPort ) {

        //
        // Process is being debugged.
        // Return a code that specifies that the exception
        // processing is to continue
        //
        return EXCEPTION_CONTINUE_SEARCH;
        }

    if ( BasepCurrentTopLevelFilter ) {
        FilterReturn = (BasepCurrentTopLevelFilter)(ExceptionInfo);
        if ( FilterReturn == EXCEPTION_EXECUTE_HANDLER ||
             FilterReturn == EXCEPTION_CONTINUE_EXECUTION ) {
            return FilterReturn;
            }
        }

    if ( GetErrorMode() & SEM_NOGPFAULTERRORBOX ) {
        return EXCEPTION_EXECUTE_HANDLER;
        }

    //
    // See if the process's job has been programmed to NOGPFAULTERRORBOX
    //
    Status = NtQueryInformationJobObject(
                NULL,
                JobObjectBasicLimitInformation,
                &BasicLimit,
                sizeof(BasicLimit),
                NULL
                );
    if ( NT_SUCCESS(Status) && (BasicLimit.LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION) ) {
        return EXCEPTION_EXECUTE_HANDLER;
        }

    //
    // The process is not being debugged, so do the hard error
    // popup.
    //

    Parameters[ 0 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionCode;
    Parameters[ 1 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;

    //
    // For inpage i/o errors, juggle the real status code to overwrite the
    // read/write field
    //

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR ) {
        Parameters[ 2 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 2 ];
        }
    else {
        Parameters[ 2 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ];
        }

    Parameters[ 3 ] = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];

    //
    // See if a debugger has been programmed in. If so, use the
    // debugger specified. If not then there is no AE Cancel support
    // DEVL systems will default the debugger command line. Retail
    // systems will not.
    //

    ResponseFlag = OptionOk;
    AeAutoDebug = FALSE;

    //
    // If we are holding the PebLock, then the createprocess will fail
    // because a new thread will also need this lock. Avoid this by peeking
    // inside the PebLock and looking to see if we own it. If we do, then just allow
    // a regular popup.
    //

    PebLockPointer = NtCurrentPeb()->FastPebLock;

    if ( PebLockPointer->OwningThread != NtCurrentTeb()->ClientId.UniqueThread ) {

        try {
            if ( GetProfileString(
                    "AeDebug",
                    "Debugger",
                    NULL,
                    AeDebuggerCmdLine,
                    sizeof(AeDebuggerCmdLine)-1
                    ) ) {
                ResponseFlag = OptionOkCancel;
                }

            if ( GetProfileString(
                    "AeDebug",
                    "Auto",
                    "0",
                    AeAutoDebugString,
                    sizeof(AeAutoDebugString)-1
                    ) ) {

                if ( !strcmp(AeAutoDebugString,"1") ) {
                    if ( ResponseFlag == OptionOkCancel ) {
                        AeAutoDebug = TRUE;
                        }
                    }
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            ResponseFlag = OptionOk;
            AeAutoDebug = FALSE;
            }
        }
    if ( !AeAutoDebug ) {
        Status =NtRaiseHardError( STATUS_UNHANDLED_EXCEPTION | HARDERROR_OVERRIDE_ERRORMODE,
                                  4,
                                  0,
                                  Parameters,
                                  BasepAlreadyHadHardError ? OptionOk : ResponseFlag,
                                  &Response
                                );

        }
    else {
        Status = STATUS_SUCCESS;
        Response = ResponseCancel;
        }

    //
    // Internally, send OkCancel. If we get back Ok then die.
    // If we get back Cancel, then enter the debugger
    //

    if ( NT_SUCCESS(Status) && Response == ResponseCancel && BasepAlreadyHadHardError == FALSE) {
        if ( !BaseRunningInServerProcess ) {
            BOOL b;
            STARTUPINFO StartupInfo;
            PROCESS_INFORMATION ProcessInformation;
            CHAR CmdLine[256];
            NTSTATUS Status;
            HANDLE EventHandle;
            SECURITY_ATTRIBUTES sa;

            BasepAlreadyHadHardError = TRUE;
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = TRUE;
            EventHandle = CreateEvent(&sa,TRUE,FALSE,NULL);
            RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
            sprintf(CmdLine,AeDebuggerCmdLine,GetCurrentProcessId(),EventHandle);
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.lpDesktop = "Winsta0\\Default";
            CsrIdentifyAlertableThread();
            b =  CreateProcess(
                    NULL,
                    CmdLine,
                    NULL,
                    NULL,
                    TRUE,
                    0,
                    NULL,
                    NULL,
                    &StartupInfo,
                    &ProcessInformation
                    );

            if ( b && EventHandle) {

                //
                // Do an alertable wait on the event
                //

                do {
                    Status = NtWaitForSingleObject(
                                EventHandle,
                                TRUE,
                                NULL
                                );
                    } while (Status == STATUS_USER_APC || Status == STATUS_ALERTED);
                return EXCEPTION_CONTINUE_SEARCH;
                }

            }
        }

#if DBG
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "BASEDLL: Unhandled exception: %lx  IP: %x\n",
                  ExceptionInfo->ExceptionRecord->ExceptionCode,
                  ExceptionInfo->ExceptionRecord->ExceptionAddress
                );
        }
#endif
    if ( BasepAlreadyHadHardError ) {
        NtTerminateProcess(NtCurrentProcess(),ExceptionInfo->ExceptionRecord->ExceptionCode);
        }
    return EXCEPTION_EXECUTE_HANDLER;
}


#define TLS_MASK 0x80000000


DWORD
APIENTRY
TlsAlloc(
    VOID
    )

/*++

Routine Description:

    A TLS index may be allocated using TlsAlloc.  Win32 garuntees a
    minimum number of TLS indexes are available in each process.  The
    constant TLS_MINIMUM_AVAILABLE defines the minimum number of
    available indexes.  This minimum is at least 64 for all Win32
    systems.

Arguments:

    None.

Return Value:

    Not-0xffffffff - Returns a TLS index that may be used in a
        subsequent call to TlsFree, TlsSetValue, or TlsGetValue.  The
        storage associated with the index is initialized to NULL.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    PPEB Peb;
    PTEB Teb;
    DWORD Index;

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    RtlAcquirePebLock();
    try {

        Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsBitmap,1,0);
        if ( Index == 0xffffffff ) {
            Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsExpansionBitmap,1,0);
            if ( Index == 0xffffffff ) {
                BaseSetLastNTError(STATUS_NO_MEMORY);
                }
            else {
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlClearBits((PRTL_BITMAP)Peb->TlsExpansionBitmap,Index,1);
                        Index = 0xffffffff;
                        BaseSetLastNTError(STATUS_NO_MEMORY);
                        return Index;
                        }
                    }
                Teb->TlsExpansionSlots[Index] = NULL;
                Index += TLS_MINIMUM_AVAILABLE;
                }
            }
        else {
            Teb->TlsSlots[Index] = NULL;
            }
        }
    finally {
        RtlReleasePebLock();
        }
#if DBG
    Index |= TLS_MASK;
#endif
    return Index;
}

LPVOID
APIENTRY
TlsGetValue(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    This function is used to retrive the value in the TLS storage
    associated with the specified index.

    If the index is valid this function clears the value returned by
    GetLastError(), and returns the value stored in the TLS slot
    associated with the specified index.  Otherwise a value of NULL is
    returned with GetLastError updated appropriately.

    It is expected, that DLLs will use TlsAlloc and TlsGetValue as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAlloc.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValue.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValue.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValue to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFree call from proceding.

Return Value:

    NON-NULL - The function was successful. The value is the data stored
        in the TLS slot associated with the specified index.

    NULL - The operation failed, or the value associated with the
        specified index was NULL.  Extended error status is available
        using GetLastError.  If this returns non-zero, the index was
        invalid.

--*/
{
    PTEB Teb;
    LPVOID *Slot;

#if DBG
    // See if the Index passed in is from TlsAlloc or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsGetValue\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex < TLS_MINIMUM_AVAILABLE ) {
        Slot = &Teb->TlsSlots[dwTlsIndex];
        Teb->LastErrorValue = 0;
        return *Slot;
        }
    else {
        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE+TLS_EXPANSION_SLOTS ) {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return NULL;
            }
        else {
            Teb->LastErrorValue = 0;
            if ( Teb->TlsExpansionSlots ) {
                return  Teb->TlsExpansionSlots[dwTlsIndex-TLS_MINIMUM_AVAILABLE];
                }
            else {
                return NULL;
                }
            }
        }
}

BOOL
APIENTRY
TlsSetValue(
    DWORD dwTlsIndex,
    LPVOID lpTlsValue
    )

/*++

Routine Description:

    This function is used to store a value in the TLS storage associated
    with the specified index.

    If the index is valid this function stores the value and returns
    TRUE. Otherwise a value of FALSE is returned.

    It is expected, that DLLs will use TlsAlloc and TlsSetValue as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAlloc.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValue.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValue.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValue to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFree call from proceding.

    lpTlsValue - Supplies the value to be stored in the TLS Slot.

Return Value:

    TRUE - The function was successful. The value lpTlsValue was
        stored.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PTEB Teb;

#if DBG
    // See if the Index passed in is from TlsAlloc or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsSetValue\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
        dwTlsIndex -= TLS_MINIMUM_AVAILABLE;
        if ( dwTlsIndex < TLS_EXPANSION_SLOTS ) {
            if ( !Teb->TlsExpansionSlots ) {
                RtlAcquirePebLock();
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlReleasePebLock();
                        BaseSetLastNTError(STATUS_NO_MEMORY);
                        return FALSE;
                        }
                    }
                RtlReleasePebLock();
                }
            Teb->TlsExpansionSlots[dwTlsIndex] = lpTlsValue;
            }
        else {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
            }
        }
    else {
        Teb->TlsSlots[dwTlsIndex] = lpTlsValue;
        }
    return TRUE;
}

BOOL
APIENTRY
TlsFree(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    A valid TLS index may be free'd using TlsFree.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAlloc.  If the
        index is a valid index, it is released by this call and is made
        available for reuse.  DLLs should be carefull to release any
        per-thread data pointed to by all of their threads TLS slots
        before calling this function.  It is expected that DLLs will
        only call this function (if at ALL) during their process detach
        routine.

Return Value:

    TRUE - The operation was successful.  Calling TlsTranslateIndex with
        this index will fail.  TlsAlloc is free to reallocate this
        index.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PPEB Peb;
    BOOLEAN ValidIndex;
    PRTL_BITMAP TlsBitmap;
    NTSTATUS Status;
    DWORD Index2;

#if DBG
    // See if the Index passed in is from TlsAlloc or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsFree\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Peb = NtCurrentPeb();

    RtlAcquirePebLock();
    try {

        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
            Index2 = dwTlsIndex - TLS_MINIMUM_AVAILABLE;
            if ( Index2 >= TLS_EXPANSION_SLOTS ) {
                ValidIndex = FALSE;
                }
            else {
                TlsBitmap = (PRTL_BITMAP)Peb->TlsExpansionBitmap;
                ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
                }
            }
        else {
            TlsBitmap = (PRTL_BITMAP)Peb->TlsBitmap;
            Index2 = dwTlsIndex;
            ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
            }
        if ( ValidIndex ) {

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadZeroTlsCell,
                        &dwTlsIndex,
                        sizeof(dwTlsIndex)
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
                }

            RtlClearBits(TlsBitmap,Index2,1);
            }
        else {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            }
        }
    finally {
        RtlReleasePebLock();
        }
    return ValidIndex;
}



BOOL
WINAPI
GetThreadTimes(
    HANDLE hThread,
    LPFILETIME lpCreationTime,
    LPFILETIME lpExitTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    )

/*++

Routine Description:

    This function is used to return various timing information about the
    thread specified by hThread.

    All times are in units of 100ns increments. For lpCreationTime and lpExitTime,
    the times are in terms of the SYSTEM time or GMT time.

Arguments:

    hThread - Supplies an open handle to the specified thread.  The
        handle must have been created with THREAD_QUERY_INFORMATION
        access.

    lpCreationTime - Returns a creation time of the thread.

    lpExitTime - Returns the exit time of a thread.  If the thread has
        not exited, this value is not defined.

    lpKernelTime - Returns the amount of time that this thread has
        executed in kernel-mode.

    lpUserTime - Returns the amount of time that this thread has
        executed in user-mode.


Return Value:

    TRUE - The API was successful

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/


{
    NTSTATUS Status;
    KERNEL_USER_TIMES TimeInfo;

    Status = NtQueryInformationThread(
                hThread,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    *lpCreationTime = *(LPFILETIME)&TimeInfo.CreateTime;
    *lpExitTime = *(LPFILETIME)&TimeInfo.ExitTime;
    *lpKernelTime = *(LPFILETIME)&TimeInfo.KernelTime;
    *lpUserTime = *(LPFILETIME)&TimeInfo.UserTime;

    return TRUE;
}

DWORD_PTR
WINAPI
SetThreadAffinityMask(
    HANDLE hThread,
    DWORD_PTR dwThreadAffinityMask
    )

/*++

Routine Description:

    This function is used to set the specified thread's processor
    affinity mask.  The thread affinity mask is a bit vector where each
    bit represents the processors that the thread is allowed to run on.
    The affinity mask MUST be a proper subset of the containing process'
    process level affinity mask.

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    dwThreadAffinityMask - Supplies the affinity mask to be used for the
        specified thread.

Return Value:

    non-0 - The API was successful.  The return value is the previous
        affinity mask for the thread.

    0 - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    THREAD_BASIC_INFORMATION BasicInformation;
    NTSTATUS Status;
    DWORD_PTR rv;
    DWORD_PTR LocalThreadAffinityMask;


    Status = NtQueryInformationThread(
                hThread,
                ThreadBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        rv = 0;
        }
    else {
        LocalThreadAffinityMask = dwThreadAffinityMask;

        Status = NtSetInformationThread(
                    hThread,
                    ThreadAffinityMask,
                    &LocalThreadAffinityMask,
                    sizeof(LocalThreadAffinityMask)
                    );
        if ( !NT_SUCCESS(Status) ) {
            rv = 0;
            }
        else {
            rv = BasicInformation.AffinityMask;
            }
        }


    if ( !rv ) {
        BaseSetLastNTError(Status);
        }

    return rv;
}

VOID
BaseDispatchAPC(
    LPVOID lpApcArgument1,
    LPVOID lpApcArgument2,
    LPVOID lpApcArgument3
    )
{
    PAPCFUNC pfnAPC;
    ULONG_PTR dwData;

    pfnAPC = (PAPCFUNC)lpApcArgument1;
    dwData = (ULONG_PTR)lpApcArgument2;
    (pfnAPC)(dwData);
}


WINBASEAPI
DWORD
WINAPI
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
    )
/*++

Routine Description:

    This function is used to queue a user-mode APC to the specified thread. The APC
    will fire when the specified thread does an alertable wait.

Arguments:

    pfnAPC - Supplies the address of the APC routine to execute when the
        APC fires.

    hHandle - Supplies a handle to a thread object.  The caller
        must have THREAD_SET_CONTEXT access to the thread.

    dwData - Supplies a DWORD passed to the APC

Return Value:

    TRUE - The operations was successful

    FALSE - The operation failed. GetLastError() is not defined.

--*/

{
    NTSTATUS Status;

    Status = NtQueueApcThread(
                hThread,
                (PPS_APC_ROUTINE)BaseDispatchAPC,
                (PVOID)pfnAPC,
                (PVOID)dwData,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return 0;
        }
    return 1;
}


DWORD
WINAPI
SetThreadIdealProcessor(
    HANDLE hThread,
    DWORD dwIdealProcessor
    )
{
    NTSTATUS Status;
    ULONG rv;

    Status = NtSetInformationThread(
                hThread,
                ThreadIdealProcessor,
                &dwIdealProcessor,
                sizeof(dwIdealProcessor)
                );
    if ( !NT_SUCCESS(Status) ) {
        rv = (DWORD)0xFFFFFFFF;
        BaseSetLastNTError(Status);
        }
    else {
        rv = (ULONG)Status;
        }

    return rv;
}

WINBASEAPI
LPVOID
WINAPI
CreateFiber(
    DWORD dwStackSize,
    LPFIBER_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    )
{

    NTSTATUS Status;
    PFIBER Fiber;
    INITIAL_TEB InitialTeb;

    Fiber = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof(*Fiber) );
    if ( !Fiber ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return Fiber;
        }

    Status = BaseCreateStack(
                NtCurrentProcess(),
                dwStackSize,
                0L,
                &InitialTeb
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        RtlFreeHeap(RtlProcessHeap(), 0, Fiber);
        return NULL;
        }

    Fiber->FiberData = lpParameter;
    Fiber->StackBase = InitialTeb.StackBase;
    Fiber->StackLimit = InitialTeb.StackLimit;
    Fiber->DeallocationStack = InitialTeb.StackAllocationBase;
    Fiber->ExceptionList = (struct _EXCEPTION_REGISTRATION_RECORD *)-1;
    Fiber->Wx86Tib = NULL;

#ifdef _IA64_

    Fiber->BStoreLimit = InitialTeb.BStoreLimit;
    Fiber->DeallocationBStore = InitialTeb.StackAllocationBase;

#endif // _IA64_

    //
    // Create an initial context for the new fiber.
    //

    BaseInitializeContext(
        &Fiber->FiberContext,
        lpParameter,
        (PVOID)lpStartAddress,
        InitialTeb.StackBase,
        BaseContextTypeFiber
        );

    return Fiber;
}

WINBASEAPI
VOID
WINAPI
DeleteFiber(
    LPVOID lpFiber
    )
{
    SIZE_T dwStackSize;
    PFIBER Fiber = lpFiber;



    //
    // If the current fiber makes this call, then it's just a thread exit
    //

    if ( NtCurrentTeb()->NtTib.FiberData == Fiber ) {
        ExitThread(1);
        }

    dwStackSize = 0;

    NtFreeVirtualMemory( NtCurrentProcess(),
                        &Fiber->DeallocationStack,
                        &dwStackSize,
                        MEM_RELEASE
                        );

#if defined (WX86)

    if (Fiber->Wx86Tib && Fiber->Wx86Tib->Size == sizeof(WX86TIB)) {
        PVOID BaseAddress = Fiber->Wx86Tib->DeallocationStack;

        dwStackSize = 0;

        NtFreeVirtualMemory( NtCurrentProcess(),
                            &BaseAddress,
                            &dwStackSize,
                            MEM_RELEASE
                            );
        }
#endif

    RtlFreeHeap(RtlProcessHeap(),0,Fiber);
}


WINBASEAPI
LPVOID
WINAPI
ConvertThreadToFiber(
    LPVOID lpParameter
    )
{

    PFIBER Fiber;
    PTEB Teb;

    Fiber = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), sizeof(*Fiber) );
    if ( !Fiber ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return Fiber;
        }
    Teb = NtCurrentTeb();
    Fiber->FiberData = lpParameter;
    Fiber->StackBase = Teb->NtTib.StackBase;
    Fiber->StackLimit = Teb->NtTib.StackLimit;
    Fiber->DeallocationStack = Teb->DeallocationStack;
    Fiber->ExceptionList = Teb->NtTib.ExceptionList;
    Fiber->Wx86Tib = NULL;
    Teb->NtTib.FiberData = Fiber;


    return Fiber;
}

BOOL
WINAPI
SwitchToThread(
    VOID
    )

/*++

Routine Description:

    This function causes a yield from the running thread to any other
    thread that is ready and can run on the current processor.  The
    yield will be effective for up to one quantum and then the yielding
    thread will be scheduled again according to its priority and
    whatever other threads may also be avaliable to run.  The thread
    that yields will not bounce to another processor even it another
    processor is idle or running a lower priority thread.

Arguments:

    None

Return Value:

    TRUE - Calling this function caused a switch to another thread to occur
    FALSE - There were no other ready threads, so no context switch occured

--*/

{

    if ( NtYieldExecution() == STATUS_NO_YIELD_PERFORMED ) {
        return FALSE;
        }
    else {
        return TRUE;
        }
}


BOOL
WINAPI
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
/*++

Routine Description:

    This function registers a wait for a particular object, with an optional
    timeout.  This differs from WaitForSingleObject because the wait is performed
    by a different thread that combines several such calls for efficiency.  The
    function supplied in Callback is called when the object is signalled, or the
    timeout expires.

Arguments:

    phNewWaitObject - pointer to new WaitObject returned by this function.

    hObject -   HANDLE to a Win32 kernel object (Event, Mutex, File, Process,
                Thread, etc.) that will be waited on.  Note:  if the object
                handle does not immediately return to the not-signalled state,
                e.g. an auto-reset event, then either WT_EXECUTEINWAITTHREAD or
                WT_EXECUTEONLYONCE should be specified.  Otherwise, the thread
                pool will continue to fire the callbacks. If WT_EXECUTEINWAITTHREAD
                is specified, the the object should be deregistered or reset in the
                callback.

    Callback -  Function that will be called when the object is signalled or the
                timer expires.

    Context -   Context that will be passed to the callback function.

    dwMilliseconds - timeout for the wait. Each time the timer is fired or the event
                is fired, the timer is reset (except if WT_EXECUTEONLYONCE is set).

    dwFlags -   Flags indicating options for this wait:
                WT_EXECUTEDEFAULT       - Default (0)
                WT_EXECUTEINIOTHREAD    - Select an I/O thread for execution
                WT_EXECUTEINUITHREAD    - Select a UI thread for execution
                WT_EXECUTEINWAITTHREAD  - Execute in the thread that handles waits
                WT_EXECUTEONLYONCE      - The callback function will be called only once
                WT_EXECUTELONGFUNCTION  - The Callback function can potentially block
                                          for a long time. Is valid only if
                                          WT_EXECUTEINWAITTHREAD flag is not set.
Return Value:

    FALSE - an error occurred, use GetLastError() for more information.

    TRUE - success.

--*/
{
    NTSTATUS Status ;
    PPEB Peb;

    *phNewWaitObject = NULL;
    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) )
    {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
    }

    if (CONSOLE_HANDLE(hObject) && VerifyConsoleIoHandle(hObject))
    {
        hObject = GetConsoleInputWaitHandle();
    }

    Status = RtlRegisterWait(
                phNewWaitObject,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}


HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
/*++

Routine Description:

    This function registers a wait for a particular object, with an optional
    timeout.  This differs from WaitForSingleObject because the wait is performed
    by a different thread that combines several such calls for efficiency.  The
    function supplied in Callback is called when the object is signalled, or the
    timeout expires.

Arguments:

    hObject -   HANDLE to a Win32 kernel object (Event, Mutex, File, Process,
                Thread, etc.) that will be waited on.  Note:  if the object
                handle does not immediately return to the not-signalled state,
                e.g. an auto-reset event, then either WT_EXECUTEINWAITTHREAD or
                WT_EXECUTEONLYONCE should be specified.  Otherwise, the thread
                pool will continue to fire the callbacks. If WT_EXECUTEINWAITTHREAD
                is specified, the the object should be deregistered or reset in the
                callback.

    Callback -  Function that will be called when the object is signalled or the
                timer expires.

    Context -   Context that will be passed to the callback function.

    dwMilliseconds - timeout for the wait. Each time the timer is fired or the event
                is fired, the timer is reset (except if WT_EXECUTEONLYONCE is set).

    dwFlags -   Flags indicating options for this wait:
                WT_EXECUTEDEFAULT       - Default (0)
                WT_EXECUTEINIOTHREAD    - Select an I/O thread for execution
                WT_EXECUTEINUITHREAD    - Select a UI thread for execution
                WT_EXECUTEINWAITTHREAD  - Execute in the thread that handles waits
                WT_EXECUTEONLYONCE      - The callback function will be called only once
                WT_EXECUTELONGFUNCTION  - The Callback function can potentially block
                                          for a long time. Is valid only if
                                          WT_EXECUTEINWAITTHREAD flag is not set.
Return Value:

    NULL - an error occurred, use GetLastError() for more information.

    non-NULL - a virtual handle that can be passed later to
               UnregisterWait

--*/
{
    HANDLE WaitHandle ;
    NTSTATUS Status ;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) )
    {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
    }

    if (CONSOLE_HANDLE(hObject) && VerifyConsoleIoHandle(hObject))
    {
        hObject = GetConsoleInputWaitHandle();
    }

    Status = RtlRegisterWait(
                &WaitHandle,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags );

    if ( NT_SUCCESS( Status ) )
    {
        return WaitHandle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;

}

BOOL
WINAPI
UnregisterWait(
    HANDLE WaitHandle
    )
/*++

Routine Description:

    This function cancels a wait for a particular object.
    All objects that were registered by the RtlWaitForSingleObject(Ex) call
    should be deregistered. This is a non-blocking call, and the associated
    callback function can still be executing after the return of this function.

Arguments:

    WaitHandle - Handle returned from RegisterWaitForSingleObject(Ex)

Return Value:

    TRUE - The wait was cancelled
    FALSE - an error occurred or a callback function was still executing,
            use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( WaitHandle )
    {
        Status = RtlDeregisterWait( WaitHandle );

        // set error if it is a non-blocking call and STATUS_PENDING was returned

        if ( Status == STATUS_PENDING  || !NT_SUCCESS( Status ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }

        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}


BOOL
WINAPI
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function cancels a wait for a particular object.
    All objects that were registered by the RtlWaitForSingleObject(Ex) call
    should be deregistered.

Arguments:

    WaitHandle - Handle returned from RegisterWaitForSingleObject

    CompletionEvent - Handle to wait on for completion.
        NULL - NonBlocking call.
        INVALID_HANDLE_VALUE - Blocking call. Block till all Callback functions
                    associated with the WaitHandle have completed
        Event - NonBlocking call. The Object is deregistered. The Event is signalled
                    when the last callback function has completed execution.
Return Value:

    TRUE - The wait was cancelled
    FALSE - an error occurred or a callback was still executing,
            use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( WaitHandle )
    {
        Status = RtlDeregisterWaitEx( WaitHandle, CompletionEvent );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        
        if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
            || ( ! NT_SUCCESS( Status ) ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
        
        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}

BOOL
WINAPI
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    )
/*++

Routine Description:

    This function queues a work item to a thread out of the thread pool.  The
    function passed is invoked in a different thread, and passed the Context
    pointer.  The caller can specify whether the thread pool should select
    a thread that can have I/O pending, or any thread.

Arguments:

    Function -  Function to call

    Context -   Pointer passed to the function when it is invoked.

    Flags   -
        - WT_EXECUTEINIOTHREAD
                Indictes to the thread pool that this thread will perform
                I/O.  A thread that starts an asynchronous I/O operation
                must wait for it to complete.  If a thread exits with
                outstanding I/O requests, those requests will be cancelled.
                This flag is a hint to the thread pool that this function
                will start I/O, so that a thread which can have pending I/O
                will be used.

        - WT_EXECUTELONGFUNCTION
                Indicates to the thread pool that the function might block
                for a long time.

Return Value:

    TRUE - The work item was queued to another thread.
    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    Status = RtlQueueWorkItem(
                (WORKERCALLBACKFUNC) Function,
                Context,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}

BOOL
WINAPI
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    )
/*++

Routine Description:

    This function binds the FileHandle opened for overlapped operations
    to the IO completion port associated with worker threads.

Arguments:

    FileHandle -  File Handle on which IO operations will be initiated.

    Function -    Function executed in a non-IO worker thread when the
                  IO operation completes.

    Flags   -     Currently set to 0. Not used.

Return Value:

    TRUE - The file handle was associated with the IO completion port.
    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    Status = RtlSetIoCompletionCallback(
                FileHandle,
                (APC_CALLBACK_FUNCTION) Function,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}


//+---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   BasepCreateDefaultTimerQueue
//
//  Synopsis:   Creates the default timer queue for the process
//
//  Arguments:  (none)
//
//  History:    5-26-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
BasepCreateDefaultTimerQueue(
    VOID
    )
{
    NTSTATUS Status ;

    while ( 1 )
    {
        if ( !InterlockedExchange( &BasepTimerQueueInitFlag, 1 ) )
        {
            //
            // Force the done flag to 0.  If it was 1, so one already tried to
            // init and failed.
            //

            InterlockedExchange( &BasepTimerQueueDoneFlag, 0 );

            Status = RtlCreateTimerQueue( &BasepDefaultTimerQueue );

            if ( NT_SUCCESS( Status ) )
            {
                InterlockedIncrement( &BasepTimerQueueDoneFlag );

                return TRUE ;
            }

            //
            // This is awkward.  We aren't able to create a timer queue,
            // probably because of low memory.  We will fail this call, but decrement
            // the init flag, so that others can try again later.  Need to increment
            // the done flag, or any other threads would be stuck.
            //

            BaseSetLastNTError( Status );

            InterlockedIncrement( &BasepTimerQueueDoneFlag );

            InterlockedDecrement( &BasepTimerQueueInitFlag );

            return FALSE ;
        }
        else
        {
            LARGE_INTEGER TimeOut ;

            TimeOut.QuadPart = -1 * 10 * 10000 ;

            //
            // yield the quantum so that the other thread can
            // try to create the timer queue.
            //

            while ( !BasepTimerQueueDoneFlag )
            {
                NtDelayExecution( FALSE, &TimeOut );
            }

            //
            // Make sure it was created.  Otherwise, try it again (memory might have
            // freed up).  This way, every thread gets an individual chance to create
            // the queue if another thread failed.
            //

            if ( BasepDefaultTimerQueue )
            {
                return TRUE ;
            }

        }
    }
}

HANDLE
WINAPI
CreateTimerQueue(
    VOID
    )
/*++

Routine Description:

    This function creates a queue for timers.  Timers on a timer queue are
    lightweight objects that allow the caller to specify a function to
    be called at some point in the future.  Any number of timers can be
    created in a particular timer queue.

Arguments:

    None.

Return Value:

    non-NULL  - a timer queue handle that can be passed to SetTimerQueueTimer,
                ChangeTimerQueueTimer, CancelTimerQueueTimer, and
                DeleteTimerQueue.

    NULL - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;
    HANDLE Handle ;

    Status = RtlCreateTimerQueue( &Handle );

    if ( NT_SUCCESS( Status ) )
    {
        return Handle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;

}


BOOL
WINAPI
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
    )
/*++

Routine Description:

    This function creates a timer queue timer, a lightweight timer that
    will fire at the DueTime, and then every Period milliseconds afterwards.
    When the timer fires, the function passed in Callback will be invoked,
    and passed the Parameter pointer.

Arguments:

    phNewTimer - pointer to new timer handle

    TimerQueue - Timer Queue to attach this timer to.  NULL indicates that the
                 default process timer queue be used.

    Function -  Function to call

    Context -   Pointer passed to the function when it is invoked.

    DueTime -   Time from now that the timer should fire, expressed in
                milliseconds. If set to INFINITE, then it will never fire.
                If set to 0, then it will fire immediately.

    Period -    Time in between firings of this timer.
                If 0, then it is a one shot timer.

    Flags  -  by default the Callback function is queued to a non-IO worker thread.

            - WT_EXECUTEINIOTHREAD
                Indictes to the thread pool that this thread will perform
                I/O.  A thread that starts an asynchronous I/O operation
                must wait for it to complete.  If a thread exits with
                outstanding I/O requests, those requests will be cancelled.
                This flag is a hint to the thread pool that this function
                will start I/O, so that a thread with I/O already pending
                will be used.

            - WT_EXECUTEINTIMERTHREAD
                The callback function will be executed in the timer thread.

            - WT_EXECUTELONGFUNCTION
                Indicates that the function might block for a long time. Useful
                only if it is queued to a worker thread.

Return Value:

    TRUE -  no error

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    *phNewTimer = NULL ;

    //
    // if the passed timer queue is NULL, use the default one.  If it is null,
    // call the initializer that will do it in a nice thread safe fashion.
    //

    if ( !TimerQueue )
    {
        if ( !BasepDefaultTimerQueue )
        {
            if ( !BasepCreateDefaultTimerQueue( ) )
            {
                return FALSE ;
            }
        }

        TimerQueue = BasepDefaultTimerQueue ;
    }

    Status = RtlCreateTimer(
                TimerQueue,
                phNewTimer,
                Callback,
                Parameter,
                DueTime,
                Period,
                Flags );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}



BOOL
WINAPI
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    )
/*++

Routine Description:

    This function updates a timer queue timer created with SetTimerQueueTimer.

Arguments:

    TimerQueue - Timer Queue to attach this timer to.  NULL indicates the default
                 process timer queue.

    Timer -     Handle returned from SetTimerQueueTimer.

    DueTime -   Time from now that the timer should fire, expressed in
                milliseconds.

    Period -    Time in between firings of this timer. If set to 0, then it becomes
                a one shot timer.


Return Value:

    TRUE - the timer was changed

    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlUpdateTimer( TimerQueue,
                             Timer,
                             DueTime,
                             Period );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}


BOOL
WINAPI
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function cancels a timer queue timer created with SetTimerQueueTimer.

Arguments:

    TimerQueue - Timer Queue that this timer was created on.

    Timer -     Handle returned from SetTimerQueueTimer.

    CompletionEvent -
            - NULL : NonBlocking call. returns immediately.
            - INVALID_HANDLE_VALUE : Blocking call. Returns after all callbacks have executed
            - Event (handle to an event) : NonBlocking call. Returns immediately.
                    Event signalled after all callbacks have executed.

Return Value:

    TRUE - the timer was cancelled.

    FALSE - an error occurred or the call is pending, use GetLastError()
            for more information.

--*/
{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlDeleteTimer( TimerQueue, Timer, CompletionEvent );

    // set error if it is a non-blocking call and STATUS_PENDING was returned
    
    if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
        || ( ! NT_SUCCESS( Status ) ) )
    {

        BaseSetLastNTError( Status );
        return FALSE;
    }
    
    return TRUE ;

}


BOOL
WINAPI
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
    )
/*++

Routine Description:

    This function deletes a timer queue created with CreateTimerQueue.
    Any pending timers on the timer queue are cancelled and deleted.

Arguments:

    TimerQueue - Timer Queue to delete.

    CompletionEvent -
            - NULL : NonBlocking call. returns immediately.
            - INVALID_HANDLE_VALUE : Blocking call. Returns after all callbacks
                    have executed
            - Event (handle to an event) : NonBlocking call. Returns immediately.
                    Event signalled after all callbacks have executed.

Return Value:

    TRUE - the timer queue was deleted.

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( TimerQueue )
    {
        Status = RtlDeleteTimerQueueEx( TimerQueue, CompletionEvent );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        
        if ( (CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING)
            || ( ! NT_SUCCESS( Status ) ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
       
        return TRUE ;

    }


    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE ;
}

BOOL
WINAPI
ThreadPoolCleanup (
    ULONG Flags
    )
/*++

Routine Description:

    Called by terminating process for thread pool to cleanup and
    delete all its threads.

Arguments:

    Flags - currently not used

Return Value:

    NO_ERROR

--*/
{

    // RtlThreadPoolCleanup( Flags ) ;

    return TRUE ;
}


/*OBSOLETE FUNCTION - REPLACED BY CreateTimerQueueTimer */
HANDLE
WINAPI
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    BOOL PreferIo
    )
/*OBSOLETE FUNCTION - REPLACED BY CreateTimerQueueTimer */
{
    NTSTATUS Status ;
    HANDLE Handle ;

    //
    // if the passed timer queue is NULL, use the default one.  If it is null,
    // call the initializer that will do it in a nice thread safe fashion.
    //

    if ( !TimerQueue )
    {
        if ( !BasepDefaultTimerQueue )
        {
            if ( !BasepCreateDefaultTimerQueue( ) )
            {
                return NULL ;
            }
        }

        TimerQueue = BasepDefaultTimerQueue ;
    }

    Status = RtlCreateTimer(
                TimerQueue,
                &Handle,
                Callback,
                Parameter,
                DueTime,
                Period,
                (PreferIo ? WT_EXECUTEINIOTHREAD : 0 ) );

    if ( NT_SUCCESS( Status ) )
    {
        return Handle ;
    }

    BaseSetLastNTError( Status );

    return NULL ;
}


/*OBSOLETE: Replaced by DeleteTimerQueueEx */
BOOL
WINAPI
DeleteTimerQueue(
    HANDLE TimerQueue
    )
/*++

  OBSOLETE: Replaced by DeleteTimerQueueEx

Routine Description:

    This function deletes a timer queue created with CreateTimerQueue.
    Any pending timers on the timer queue are cancelled and deleted.
    This is a non-blocking call. Callbacks might still be running after
    this call returns.

Arguments:

    TimerQueue - Timer Queue to delete.

Return Value:

    TRUE - the timer queue was deleted.

    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if (TimerQueue)
    {
        Status = RtlDeleteTimerQueueEx( TimerQueue, NULL );

        // set error if it is a non-blocking call and STATUS_PENDING was returned
        /*
        if ( Status == STATUS_PENDING || ! NT_SUCCESS( Status ) )
        {

            BaseSetLastNTError( Status );
            return FALSE;
        }
        */
        return TRUE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}


/*OBSOLETE: USE DeleteTimerQueueTimer*/
BOOL
WINAPI
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
    )
/*OBSOLETE: USE DeleteTimerQueueTimer*/
{
    NTSTATUS Status ;

    //
    // Use the default timer queue if none was passed in.  If there isn't one, then
    // the process hasn't created one with SetTimerQueueTimer, and that's an error.
    //

    if ( !TimerQueue )
    {
        TimerQueue = BasepDefaultTimerQueue ;

        if ( !TimerQueue )
        {
            SetLastError( ERROR_INVALID_PARAMETER );

            return FALSE ;
        }
    }

    Status = RtlDeleteTimer( TimerQueue, Timer, NULL );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;

}
