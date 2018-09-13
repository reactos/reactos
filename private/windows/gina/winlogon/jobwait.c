//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       jobwait.c
//
//  Contents:   Generic "start app in job and wait" functionality
//
//  Classes:
//
//  Functions:
//
//  History:    6-19-98   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop


HANDLE IoPort ;
LIST_ENTRY JobList ;
CRITICAL_SECTION JobLock ;
HANDLE hJobThread ;


#define WINLOGON_JOB_TERMINATE_ON_TIMEOUT       0x00000001
#define WINLOGON_JOB_SIGNAL_ON_TERMINATE        0x00000002
#define WINLOGON_JOB_TERMINATED                 0x00000004
#define WINLOGON_JOB_PROCESS_STARTED            0x00000008
#define WINLOGON_JOB_WATCH_PROCESS              0x00000010
#define WINLOGON_JOB_KILLED                     0x00000020
#define WINLOGON_JOB_CALLBACKS_DONE             0x00000040
#define WINLOGON_JOB_RUN_ON_DELETE              0x00000080

#define JOB_OBJECT_MSG_WINLOGON_TERMINATE_JOB   0x00010000
#define JOB_OBJECT_MSG_WINLOGON_KILL_JOB        0x00010001
#define JOB_OBJECT_MSG_WINLOGON_TERMINATED      0x00010002


VOID
JobRootProcessTermCallback(
    PVOID Context,
    BOOLEAN Timeout
    );

//+---------------------------------------------------------------------------
//
//  Function:   InitializeJobControl
//
//  Synopsis:   Initialize the job control structures
//
//  Arguments:  (none)
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeJobControl(
    VOID
    )
{
    NTSTATUS Status ;

    Status = RtlInitializeCriticalSection( &JobLock );

    InitializeListHead( &JobList );

    return NT_SUCCESS( Status );
}


VOID
DerefWinlogonJob(
    PWINLOGON_JOB Job
    )
{
    EnterCriticalSection( &JobLock );

    DebugLog((DEB_TRACE_JOB, "Deref job %x:%x, current ref %d\n",
                Job->UniqueId.HighPart, Job->UniqueId.LowPart,
                Job->RefCount ));

    Job->RefCount-- ;

    if ( Job->RefCount == 0 )
    {
        if ( Job->Job )
        {
            CloseHandle( Job->Job );
        }

        if ( Job->Event )
        {
            CloseHandle( Job->Event );
        }

        if ( Job->RootProcess )
        {
            CloseHandle( Job->RootProcess );
        }

        if ( Job->List.Flink )
        {
            RemoveEntryList( &Job->List );

            Job->List.Flink = NULL ;
            Job->List.Blink = NULL ;
        }

        LocalFree( Job );

    }

    LeaveCriticalSection( &JobLock );
}



//+---------------------------------------------------------------------------
//
//  Function:   JobThread
//
//  Synopsis:   Thread that monitors the IO Port when jobs are active
//
//  Arguments:  [Ignored] --
//
//  History:    6-21-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
JobThread(
    PVOID Ignored
    )
{
    LPOVERLAPPED lpOverlapped ;
    OVERLAPPED Overlapped ;
    PLIST_ENTRY Scan ;
    DWORD MinWait ;
    PWINLOGON_JOB Job ;
    DWORD CurrentTick ;
    DWORD DeltaTime ;
    DWORD CompletionCode ;
    PVOID CompletionKey ;
    PWINLOGON_JOB WaitJob = NULL ;
    JOBOBJECT_BASIC_PROCESS_ID_LIST JobInfo ;
    NTSTATUS Status ;
    ULONG ProcessStatus ;

    do
    {
        MinWait = INFINITE ;

        CurrentTick = GetTickCount();

        EnterCriticalSection( &JobLock );

        if ( IsListEmpty( &JobList ) )
        {
            LeaveCriticalSection( &JobLock );

            DebugLog(( DEB_TRACE_JOB, "No more jobs, job thread shutting down\n" ));

            break;
        }

        Scan = JobList.Flink ;

        while ( Scan != &JobList )
        {
            Job = CONTAINING_RECORD( Scan, WINLOGON_JOB, List );

            if ( Job->Timeout != INFINITE )
            {
                //
                // Absolute value:
                //

                if ( Job->Timeout > CurrentTick )
                {
                    DeltaTime = Job->Timeout - CurrentTick ;

                }
                else
                {
                    DeltaTime = CurrentTick - Job->Timeout ;

                }

                if ( MinWait > DeltaTime )
                {
                    MinWait = DeltaTime ;

                    WaitJob = Job ;
                }

            }

            Scan = Scan->Flink ;
        }

        LeaveCriticalSection( &JobLock );

        //
        // Computed correct timeout value.  Wait to see if something comes up
        // from the job object.
        //

        if ( WaitJob )
        {
            DebugLog(( DEB_TRACE_JOB, "WaitJob (%x:%x) timeout in %x\n",
                   WaitJob->UniqueId.HighPart, WaitJob->UniqueId.LowPart,
                   MinWait ));
        }
        else 
        {
            DebugLog(( DEB_TRACE_JOB, "No timeout\n" ));
        }

        if (!GetQueuedCompletionStatus( IoPort,
                                        &CompletionCode,
                                        (PULONG_PTR) &CompletionKey,
                                        &lpOverlapped,
                                        MinWait ) )
        {
            if ( GetLastError() == WAIT_TIMEOUT )
            {
                //
                // Timeout:
                //

                if ( WaitJob->Flags & WINLOGON_JOB_TERMINATE_ON_TIMEOUT )
                {
                    //
                    // Fudge the timeout so that we will always re-wait.
                    //

                    WaitJob->Timeout = INFINITE ;

                    DebugLog(( DEB_TRACE_JOB, "Job %x:%x timed out, posting a kill\n",
                               WaitJob->UniqueId.HighPart,
                               WaitJob->UniqueId.LowPart ));

                    //
                    // Post a private message indicating that we want to kill
                    // the job.
                    //

                    PostQueuedCompletionStatus( IoPort,
                                                JOB_OBJECT_MSG_WINLOGON_TERMINATE_JOB,
                                                (ULONG_PTR) WaitJob,
                                                &Overlapped );
                    //
                    // Let the work happen in the normal place
                    //
                }

                continue;
            }
            //
            // Failure getting status.  Break out:
            //

            break;
        }

        Job = (PWINLOGON_JOB) CompletionKey ;

        switch ( CompletionCode )
        {
            case JOB_OBJECT_MSG_WINLOGON_TERMINATE_JOB:
            case JOB_OBJECT_MSG_WINLOGON_KILL_JOB:

                DebugLog(( DEB_TRACE_JOB, "%s termination for job %x:%x\n",
                           (CompletionCode == JOB_OBJECT_MSG_WINLOGON_TERMINATE_JOB ?
                                "Timeout" : "Root-died" ),
                           Job->UniqueId.HighPart,
                           Job->UniqueId.LowPart ));

                TerminateJobObject( Job->Job, STATUS_TIMEOUT );

                EnterCriticalSection( &JobLock );

                Job->Flags |= WINLOGON_JOB_TERMINATED |
                              WINLOGON_JOB_KILLED ;

                Job->Timeout = INFINITE ;

                //
                // Bump the ref count.  This way, it will stick
                // around if messages come in out of order.
                //

                Job->RefCount++ ;

                LeaveCriticalSection( &JobLock );

                //
                // Need to cycle around again.  Calling TerminateJobObject
                // will still generate the message when the count goes to 0
                //

                
                PostQueuedCompletionStatus( IoPort,
                                            JOB_OBJECT_MSG_WINLOGON_TERMINATED,
                                            (ULONG_PTR) Job,
                                            &Overlapped );

                continue;

                break;

            case JOB_OBJECT_MSG_EXIT_PROCESS:
            case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:

                EnterCriticalSection( &JobLock );

                if ( Job->Flags & WINLOGON_JOB_WATCH_PROCESS )
                {
                    if ( GetExitCodeProcess( Job->RootProcess, &ProcessStatus ) &&
                         ( ProcessStatus != STATUS_PENDING ) )
                    {
                        //
                        // Close the handle so the job can terminate
                        // if it needs to
                        //

                        NtClose( Job->RootProcess );

                        Job->RootProcess = NULL ;

                        //
                        // The initial process has terminated, 
                        // and we're watching it.  If there are other
                        // processes still in the job, we need to kill 
                        // them.
                        //

                        Status = NtQueryInformationJobObject(
                                    Job->Job,
                                    JobObjectBasicProcessIdList,
                                    &JobInfo,
                                    sizeof( JobInfo ),
                                    NULL );

                        if ( ( Status == STATUS_SUCCESS ) ||
                             ( Status == STATUS_BUFFER_OVERFLOW ) )
                        {
                            //
                            // bugbug, 0 or 1?
                            //
                            if ( JobInfo.NumberOfAssignedProcesses > 0 )
                            {
                                DebugLog(( DEB_TRACE_JOB, "Job %x:%x root process terminated\n",
                                           Job->UniqueId.HighPart,
                                           Job->UniqueId.LowPart ));

                                //
                                // Post a private message indicating that we want to kill
                                // the job.
                                //

                                PostQueuedCompletionStatus( IoPort,
                                                            JOB_OBJECT_MSG_WINLOGON_KILL_JOB,
                                                            (ULONG_PTR) Job,
                                                            &Overlapped );
                                //
                                // Let the work happen in the normal place
                                //

                            }
                        }

                    }

                }

                LeaveCriticalSection( &JobLock );

                continue; 

                break;

            case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
            case JOB_OBJECT_MSG_WINLOGON_TERMINATED:

                DebugLog(( DEB_TRACE_JOB, "Job %x:%x completed\n",
                           Job->UniqueId.HighPart,
                           Job->UniqueId.LowPart ));

                EnterCriticalSection( &JobLock );

                Job->Flags |= WINLOGON_JOB_TERMINATED ;

                LeaveCriticalSection( &JobLock );

                if ( ( CompletionCode == JOB_OBJECT_MSG_WINLOGON_TERMINATED ) || 
                     ( (Job->Flags & WINLOGON_JOB_KILLED) == 0 ) || 
                     ( (Job->Flags & WINLOGON_JOB_CALLBACKS_DONE) == 0 ))
                {
                    if ( Job->Event )
                    {
                        SetEvent( Job->Event );
                    }
                    if ( Job->Callback )
                    {
                        Job->Callback( Job->Parameter );
                    }

                    Job->Flags |= WINLOGON_JOB_CALLBACKS_DONE ;

                }

                if ( CompletionCode == JOB_OBJECT_MSG_WINLOGON_TERMINATED )
                {
                    //
                    // For these, we need to keep waiting until the 
                    // job object actually empties.  Take away the ref
                    // that we added when this message was posted, and
                    // continue waiting.
                    //

                    Job->Timeout = INFINITE ;

                    DerefWinlogonJob( Job );

                    continue;
                }



                break;


            default:

                //
                // Not a message we care about
                //

                continue;
        }

        EnterCriticalSection( &JobLock );

        DebugLog(( DEB_TRACE_JOB, "Unlinking Job %x:%x\n",
                   Job->UniqueId.HighPart,
                   Job->UniqueId.LowPart ));

        if ( Job->List.Flink )
        {

            RemoveEntryList( &Job->List );

            Job->List.Flink = NULL ;
            Job->List.Blink = NULL ;

        }

        DerefWinlogonJob( Job );

        LeaveCriticalSection( &JobLock );

    } while ( TRUE );

    //
    // Thread cleanup:
    //

    EnterCriticalSection( &JobLock );

    CloseHandle( IoPort );

    IoPort = NULL ;

    CloseHandle( hJobThread );

    hJobThread = NULL ;

    LeaveCriticalSection( &JobLock );

    return 0;

}


//+---------------------------------------------------------------------------
//
//  Function:   InsertJob
//
//  Synopsis:   Insert a job into the wait list.  Will create the port and
//              monitor thread if none is present.
//
//  Arguments:  [Job] --
//
//  History:    6-21-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InsertJob(
    PWINLOGON_JOB Job
    )
{
    DWORD Tid ;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT CompletionPort;
    BOOL Ret ;

    EnterCriticalSection( &JobLock );

    if ( IoPort == NULL )
    {
        IoPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                         NULL,
                                         (ULONG_PTR) Job,
                                         1 );

        if ( IoPort )
        {
            hJobThread = CreateThread( NULL, 0,
                                       JobThread, NULL,
                                       0, &Tid );

            if ( !hJobThread )
            {

                CloseHandle( IoPort );

                IoPort = NULL ;
            }
        }

    }

    //
    // We may have failed to create the thread or port, so we need to check again:
    //

    if ( IoPort != NULL )
    {
        InsertHeadList( &JobList, &Job->List );


        CompletionPort.CompletionKey = Job ;

        CompletionPort.CompletionPort = IoPort;

        if (!SetInformationJobObject( Job->Job,
                                      JobObjectAssociateCompletionPortInformation,
                                      &CompletionPort,
                                      sizeof(CompletionPort) ) )
        {
            DebugLog((DEB_WARN, "Failed to set completion port with %d\n", GetLastError()));

            RemoveEntryList( &Job->List );

            Job->List.Flink = Job->List.Blink = NULL ;

            Ret = FALSE ;
        }
        else
        {
            //
            // Bump the ref count.  As long as the job object is active,
            // that is, in the list.
            //

            Job->RefCount++ ;
            Ret = TRUE ;
        }

    }
    else
    {
        Ret = FALSE ;
    }

    LeaveCriticalSection( &JobLock );

    return Ret ;


}

//+---------------------------------------------------------------------------
//
//  Function:   CreateWinlogonJob
//
//  Synopsis:   Creates a job object.  It is initially empty, and has no
//              settings
//
//  Arguments:  (none)
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PWINLOGON_JOB
CreateWinlogonJob(
    VOID
    )
{
    PWINLOGON_JOB Job ;
    WCHAR JobName[ 64 ];
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION JobTime;

    Job = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof( WINLOGON_JOB ) );

    if ( Job )
    {
        //
        // Generate a unique name so we don't overlap with other winlogons or
        // threads
        //

        AllocateLocallyUniqueId( &Job->UniqueId );

        wsprintf( JobName, TEXT("Winlogon Job %x-%x"), Job->UniqueId.HighPart,
                                                       Job->UniqueId.LowPart );

        Job->Timeout = INFINITE ;
        Job->Flags = WINLOGON_JOB_TERMINATE_ON_TIMEOUT |
                     WINLOGON_JOB_SIGNAL_ON_TERMINATE ;

        Job->RefCount = 1;

        Job->Job = CreateJobObject( NULL, JobName );



        if ( Job->Job )
        {
            //
            // Excellent.  Add the Job the the list of jobs we're monitoring.  This
            // may also create a thread to do the monitoring.
            //

            JobTime.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB;

            if (SetInformationJobObject( Job->Job,
                                         JobObjectEndOfJobTimeInformation,
                                         &JobTime,
                                         sizeof(JobTime) ) )
            {

                if ( InsertJob( Job ) )
                {
                    //
                    // Done!
                    //

                    return Job ;
                }
            }

            CloseHandle( Job->Job );


        }

        LocalFree( Job );

    }

    return NULL ;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetWinlogonJobTimeout
//
//  Synopsis:   Sets the timeout for a winlogon job.  The timeout is in ms.
//
//  Arguments:  [pJob]    --
//              [Timeout] --
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SetWinlogonJobTimeout(
    PWINLOGON_JOB pJob,
    ULONG Timeout
    )
{

    JOBOBJECT_END_OF_JOB_TIME_INFORMATION JobTime;

    EnterCriticalSection( &JobLock );

    JobTime.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB;

    if (!SetInformationJobObject(pJob->Job,
                                 JobObjectEndOfJobTimeInformation,
                                 &JobTime,
                                 sizeof(JobTime) ) )
    {
        DebugLog(( DEB_WARN, "Failed to set job end behavior, %d\n", GetLastError() ));

        LeaveCriticalSection( &JobLock );

        return FALSE ;
    }

    if ( Timeout == INFINITE )
    {
        pJob->Timeout = INFINITE ;
    }
    else 
    {   
        pJob->Timeout = GetTickCount() + Timeout ;
    }


    LeaveCriticalSection( &JobLock );

    return TRUE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   StartProcessInJob
//
//  Synopsis:   Starts a process in a winlogon job
//
//  Arguments:  [pTerm]        -- Terminal for the process
//              [bUser]        -- TRUE if should be started as the user
//              [lpDesktop]    -- Desktop to run process on
//              [pEnvironment] -- Environment pointer, or NULL to inherit ours
//              [lpCmdLine]    -- Command line to run
//              [pJob]         -- Job to associate process with
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
StartProcessInJob(
    IN PTERMINAL pTerm,
    IN JOB_PROCESS_TYPE ProcessType,
    IN LPWSTR lpDesktop,
    IN PVOID pEnvironment,
    IN PWSTR lpCmdLine,
    IN DWORD Flags,
    IN DWORD StartFlags,
    IN PWINLOGON_JOB pJob
    )
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL Result = FALSE;
    HANDLE ImpersonationHandle = NULL ;
    HANDLE ProcessToken, Token ;
    DWORD dwSize, dwType;
    NTSTATUS Status ;
    TOKEN_GROUPS TokenGroups ;

    //
    // Initialize process startup info
    //

    ZeroMemory (&si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.wShowWindow = SW_SHOWMINIMIZED;
    si.lpDesktop = lpDesktop;
    si.dwFlags = StartFlags ;


    //
    // Create the app suspended
    //

    if ( ProcessType != ProcessAsSystem ) {

        //
        // Impersonate the user so we get access checked correctly on
        // the file we're trying to execute
        //

        if ( ProcessType == ProcessAsUser )
        {
            ImpersonationHandle = ImpersonateUser(&pTerm->pWinStaWinlogon->UserProcessData, NULL);
            Token = pTerm->pWinStaWinlogon->UserProcessData.UserToken ;

            if (ImpersonationHandle == NULL) {
                DebugLog(( DEB_TRACE, "Unable to impersonate user\n" ));
                return FALSE ;
            }
        }
        else 
        {
            Status = NtOpenProcessToken(
                            NtCurrentProcess(),
                            TOKEN_ALL_ACCESS,
                            &ProcessToken );

            if ( NT_SUCCESS( Status ) )
            {
                TokenGroups.GroupCount = 1 ;
                TokenGroups.Groups[ 0 ].Attributes = 0 ;
                TokenGroups.Groups[ 0 ].Sid = gAdminSid ;

                Status = NtFilterToken(
                            ProcessToken,
                            DISABLE_MAX_PRIVILEGE,
                            &TokenGroups,
                            NULL,
                            NULL,
                            &Token );

                NtClose( ProcessToken );

            }

            if ( !NT_SUCCESS( Status ) )
            {
                DebugLog(( DEB_TRACE, "Unable to filter token, %x\n", Status ));
                return FALSE ;
            }

            //
            // don't worry about impersonating
            //

        }



        Result = CreateProcessAsUser(
                          Token,
                          NULL,
                          lpCmdLine,
                          NULL,
                          NULL,
                          FALSE,
                          Flags | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                          pEnvironment,
                          NULL,
                          &si,
                          &pi);

        //
        // If we are impersonating, then we are launching as a user, so
        // don't close that token.  If we aren't, we're doing a filtered token,
        // so close it when we're done with it.
        //

        if ( ImpersonationHandle )
        {
            StopImpersonating( ImpersonationHandle );
        }
        else 
        {
            NtClose( Token );
        }

    } else {

        Result = CreateProcess(
                          NULL,
                          lpCmdLine,
                          NULL,
                          NULL,
                          FALSE,
                          CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                          pEnvironment,
                          NULL,
                          &si,
                          &pi);
    }


    //
    // If the app was started, assign the process to the job
    //

    if ( Result )
    {
        EnterCriticalSection( &JobLock );

        if ( AssignProcessToJobObject( pJob->Job,
                                       pi.hProcess ) )
        {
            pJob->Flags |= WINLOGON_JOB_PROCESS_STARTED ;

            ResumeThread( pi.hThread );
        }
        else
        {
            DebugLog(( DEB_WARN, "Could not assign process '%ws' to job, %d\n",
                                    lpCmdLine, GetLastError() ));

            //
            // Nuke the process
            //

            TerminateProcess( pi.hProcess, ERROR_ACCESS_DENIED );

            Result = FALSE ;

        }

        if ( pJob->Flags & WINLOGON_JOB_WATCH_PROCESS )
        {
            pJob->RootProcess = pi.hProcess ;

        }
        else 
        {
            CloseHandle( pi.hProcess );
        }

        LeaveCriticalSection( &JobLock );

        CloseHandle( pi.hThread );
    }

    return Result;

}

//+---------------------------------------------------------------------------
//
//  Function:   CreateJobEvent
//
//  Synopsis:   Make the job waitable
//
//  Arguments:  [Job] -- Job
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CreateJobEvent(
    PWINLOGON_JOB Job
    )
{
#if DBG
    WCHAR EventName[ 128 ];
#endif

    EnterCriticalSection( &JobLock );

    if ( Job->Event != NULL )
    {
        LeaveCriticalSection( &JobLock );

        return TRUE ;
    }

#if DBG
    _snwprintf( EventName, 128, L"Event for Winlogon Job %x-%x",
                Job->UniqueId.HighPart, Job->UniqueId.LowPart );

    Job->Event = CreateEvent( NULL, FALSE, FALSE, EventName );

#else

    Job->Event = CreateEvent( NULL, FALSE, FALSE, NULL );

#endif

    LeaveCriticalSection( &JobLock );

    return (Job->Event != NULL );

}


//+---------------------------------------------------------------------------
//
//  Function:   WaitForJob
//
//  Synopsis:   Waits for a job to complete.  Timeout is in ms.
//
//  Arguments:  [Job]     --
//              [Timeout] --
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WaitForJob(
    PWINLOGON_JOB Job,
    DWORD Timeout
    )
{
    if ( Job->Flags & WINLOGON_JOB_PROCESS_STARTED ) 
    {
        if ( CreateJobEvent( Job ) )
        {
            if ( SetWinlogonJobTimeout( Job, Timeout ) )
            {
                WaitForSingleObjectEx( Job->Event, INFINITE, FALSE );

                return TRUE ;

            }
        }
    }

    return FALSE ;

}


//+---------------------------------------------------------------------------
//
//  Function:   TerminateJob
//
//  Synopsis:   Terminates all processes in a job
//
//  Arguments:  [Job]      --
//              [ExitCode] --
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
TerminateJob(
    PWINLOGON_JOB Job,
    DWORD ExitCode
    )
{
    BOOL Termed ;


    if ( (Job->Flags & WINLOGON_JOB_PROCESS_STARTED) == 0 )
    {
        return FALSE ;
    }

    if ( Job->Flags & WINLOGON_JOB_TERMINATED )
    {
        DebugLog(( DEB_ERROR, "TerminateJob called on a job that has already completed\n" ));

        return FALSE ;
    }

    EnterCriticalSection( &JobLock );

    CreateJobEvent( Job );

    Termed = TerminateJobObject( Job->Job, ExitCode );

    LeaveCriticalSection( &JobLock );

    if ( Termed )
    {
        if ( Job->Event )
        {
            WaitForSingleObjectEx( Job->Event, INFINITE, FALSE );
        }
        else
        {
            //
            // Spin and wait for a while?
            //

            NOTHING ;

        }
    }

    return Termed ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetJobCallback
//
//  Synopsis:   Sets a function to be called when the job completes.
//
//  Arguments:  [Job]       --
//              [Callback]  --
//              [Parameter] --
//
//  History:    9-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SetJobCallback(
    IN PWINLOGON_JOB Job,
    IN LPTHREAD_START_ROUTINE Callback,
    IN PVOID Parameter
    )
{
    EnterCriticalSection( &JobLock );

    Job->Callback = Callback ;
    Job->Parameter = Parameter ;

    LeaveCriticalSection( &JobLock );

    return TRUE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteJob
//
//  Synopsis:   Deletes a job when done with it.
//
//  Arguments:  [Job] --
//
//  History:    9-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DeleteJob(
    PWINLOGON_JOB Job
    )
{
    EnterCriticalSection( &JobLock );

    if ( ( Job->Flags & WINLOGON_JOB_RUN_ON_DELETE ) == 0 )
    {
        if ( ( Job->Flags & WINLOGON_JOB_TERMINATED ) == 0 ) 
        {
            LeaveCriticalSection( &JobLock );

            TerminateJob( Job, 0 );

            EnterCriticalSection( &JobLock );
        }

        CloseHandle( Job->Job );

        Job->Job = NULL ;
    }

    DerefWinlogonJob( Job );

    LeaveCriticalSection( &JobLock );

    return TRUE ;

}

BOOL
IsJobActive(
    PWINLOGON_JOB Job
    )
{
    BOOL Status ;

    EnterCriticalSection( &JobLock );

    Status = ((Job->Flags & WINLOGON_JOB_TERMINATED) == 0 );

    LeaveCriticalSection( &JobLock );

    DebugLog(( DEB_TRACE_JOB, "IsJobActive: Job %x:%x is %s.\n",
               Job->UniqueId.HighPart, Job->UniqueId.LowPart,
               Status ? "active" : "terminated" ));

    return Status ;
}

BOOL
SetWinlogonJobOption(
    PWINLOGON_JOB Job,
    ULONG Options
    )
{
    BOOL Ret = FALSE ;

    EnterCriticalSection( &JobLock );

    if ( Options & WINLOGON_JOB_MONITOR_ROOT_PROCESS )
    {
        if ( (Job->Flags & WINLOGON_JOB_PROCESS_STARTED ) == 0 )
        {
            Job->Flags |= WINLOGON_JOB_WATCH_PROCESS ;

            Ret = TRUE ;
            
        }

    }

    if ( Options & WINLOGON_JOB_AUTONOMOUS )
    {
        Job->Flags |= WINLOGON_JOB_RUN_ON_DELETE ;

        Ret = TRUE ;
    }

    LeaveCriticalSection( &JobLock );

    return Ret ;
}
