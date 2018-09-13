
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CONTROL.C

Abstract:

    This file contains the control handler for the eventlog service.

Author:

    Rajen Shah  (rajens)    16-Jul-1991

Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>

//
// DEFINITIONS
//

//
// Service is stoppable only on a developer's build
//
#if DBG
#define     ELF_CONTROLS_ACCEPTED           SERVICE_ACCEPT_STOP | \
                                             SERVICE_ACCEPT_PAUSE_CONTINUE | \
                                             SERVICE_ACCEPT_SHUTDOWN;
#else  // ndef DBG
#define     ELF_CONTROLS_ACCEPTED           SERVICE_ACCEPT_SHUTDOWN;
#endif // DBG

//
// GLOBALS
//

    CRITICAL_SECTION StatusCriticalSection = {0};
    SERVICE_STATUS   ElStatus              = {0};
    DWORD            HintCount             = 0;
    DWORD            ElUninstallCode       = 0;  // reason for uninstalling
    DWORD            ElSpecificCode        = 0;
    DWORD            ElState               = STARTING;



VOID
ElfControlResponse(
    DWORD   opCode
    )

{
    DWORD   state;

    ElfDbgPrint(("[ELF] Inside control handler. Control = %ld\n", opCode));

    //
    // Determine the type of service control message and modify the
    // service status, if necessary.
    //
    switch(opCode) {

        case SERVICE_CONTROL_SHUTDOWN:

#if DBG
        case SERVICE_CONTROL_STOP:
#endif

        {
            HKEY    hKey;
            ULONG   ValueSize;
            ULONG   ShutdownReason = 0xFF;
            ULONG   rc;

            //
            // If the service is installed, shut it down and exit.
            //

            ElfStatusUpdate(STOPPING);

            GetGlobalResource (ELF_GLOBAL_EXCLUSIVE);
            
            //
            // Cause the timestamp writing thread to exit
            //

            if (g_hTimestampEvent != NULL) {
                SetEvent (g_hTimestampEvent);
            }

            //
            // Indicate a normal shutdown in the registry
            //

            ElfWriteTimeStamp(EVENT_NormalShutdown,
                              FALSE);

            //
            // Determine the reason for this normal shutdown
            //

            rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                REGSTR_PATH_RELIABILITY,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                NULL);

            if (rc == ERROR_SUCCESS) {

                ValueSize=sizeof(ULONG);

                rc = RegQueryValueEx(hKey,
                                     REGSTR_VAL_SHUTDOWNREASON,
                                     0,
                                     NULL,
                                     (PUCHAR) &ShutdownReason,
                                     &ValueSize);

                if (rc == ERROR_SUCCESS) {
                    RegDeleteValue (hKey, REGSTR_VAL_SHUTDOWNREASON);
                }                                                                  

                RegCloseKey (hKey);
            }

            //
            // Log an event that says we're stopping
            //

            ElfpCreateElfEvent(EVENT_EventlogStopped,
                               EVENTLOG_INFORMATION_TYPE,
                               0,                    // EventCategory
                               0,                    // NumberOfStrings
                               NULL,                 // Strings
                               &ShutdownReason,      // Data
                               sizeof(ULONG),        // Datalength
                               0                     // flags
                               );

            //
            // Now force it to be written before we shut down
            //

            WriteQueuedEvents();

            ReleaseGlobalResource();

            //
            // If the RegistryMonitor is started, wakeup that
            // worker thread and have it handle the rest of the
            // shutdown.
            //
            // Otherwise The main thread should pick up the
            // fact that a shutdown during startup is occuring.
            //
            if (EventFlags & ELF_STARTED_REGISTRY_MONITOR) {
                StopRegistryMonitor();
            }

            break ;
        }

#if DBG

        case SERVICE_CONTROL_PAUSE:

            //
            // If the service is not already paused, pause it.
            //
            state = GetElState();
            if ((state != PAUSED) && (state != PAUSING)) {

                GetGlobalResource (ELF_GLOBAL_EXCLUSIVE);

                // Announce that the service is about to be paused
                ElfStatusUpdate(PAUSING);

                // Get into a decent state to pause the service
                // (i.e., flush all files to disk)

                ElfpFlushFiles();

                // Set the status and announce that the service is paused
                ElfStatusUpdate(PAUSED);

                ReleaseGlobalResource();
            }

            break ;

        case SERVICE_CONTROL_CONTINUE:

            //
            // If the service is not already running, un-pause it.
            //

            if (GetElState() != RUNNING) {

                GetGlobalResource (ELF_GLOBAL_EXCLUSIVE);

                // Set the status and announce that the service
                // is no longer paused
                //
                ElfStatusUpdate(RUNNING);

                ReleaseGlobalResource();
            }

            break ;

#endif  // DBG

        case SERVICE_CONTROL_INTERROGATE:

            ElfStatusUpdate(UPDATE_ONLY);
            break;

        default:
            // WARNING: This should never happen.
            ASSERT(FALSE);
            break ;
    }

    return;
}


DWORD
ElfBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    ExitCode,
    IN DWORD    ServiceSpecificCode
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD  status;

    EnterCriticalSection(&StatusCriticalSection);

    ElfDbgPrint(("BeginForcedShutdown: Errors - %d  0x%lx\n",
                 ExitCode,
                 ServiceSpecificCode));

    //
    // See if the eventlog is already stopping for some reason.
    // It could be that the ControlHandler thread received a control to
    // stop the eventlog just as we decided to stop ourselves.
    //
    if ((ElState != STOPPING) && (ElState != STOPPED)) {

        if (PendingCode == PENDING) {
            ElStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ElState = STOPPING;
        }
        else {
            //
            // The shutdown is to take immediate effect.
            //
            ElStatus.dwCurrentState = SERVICE_STOPPED;
            ElStatus.dwControlsAccepted = 0;
            ElStatus.dwCheckPoint = 0;
            ElStatus.dwWaitHint = 0;
            ElState = STOPPED;
        }

        ElUninstallCode = ExitCode;
        ElSpecificCode = ServiceSpecificCode;

        ElStatus.dwWin32ExitCode = ExitCode;
        ElStatus.dwServiceSpecificExitCode = ServiceSpecificCode;
    }

    //
    // Cause the timestamp writing thread to exit
    //

    if (g_hTimestampEvent != NULL) {
        SetEvent (g_hTimestampEvent);
    }

    //
    // Send the new status to the service controller.
    //

    ASSERT(ElfServiceStatusHandle != 0);

    if (!SetServiceStatus( ElfServiceStatusHandle, &ElStatus )) {

        ElfDbgPrint(("ElfBeginForcedShutdown,SetServiceStatus Failed %d\n",
                     GetLastError()));
    }

    status = ElState;
    LeaveCriticalSection(&StatusCriticalSection);
    return(status);
}


DWORD
ElfStatusUpdate(
    IN DWORD    NewState
    )

/*++

Routine Description:

    Sends a status to the Service Controller via SetServiceStatus.

    The contents of the status message is controlled by this routine.
    The caller simply passes in the desired state, and this routine does
    the rest.  For instance, if the Eventlog passes in a STARTING state,
    This routine will update the hint count that it maintains, and send
    the appropriate information in the SetServiceStatus call.

    This routine uses transitions in state to send determine which status
    to send.  For instance if the status was STARTING, and has changed
    to RUNNING, this routine sends out an INSTALLED to the Service
    Controller.

Arguments:

    NewState - Can be any of the state flags:
                UPDATE_ONLY - Simply send out the current status
                STARTING    - The Eventlog is in the process of initializing
                RUNNING     - The Eventlog has finished with initialization
                STOPPING    - The Eventlog is in the process of shutting down
                STOPPED     - The Eventlog has completed the shutdown.
                PAUSING     - The Eventlog is pausing (DEBUG ONLY)
                PAUSED      - The Eventlog is paused  (DEBUG ONLY)


Return Value:

    CurrentState - This may not be the same as the NewState that was
        passed in.  It could be that the main thread is sending in a new
        install state just after the Control Handler set the state to
        STOPPING.  In this case, the STOPPING state will be returned so as
        to inform the main thread that a shut-down is in process.

Note:


--*/

{
    DWORD       status;
    BOOL        inhibit = FALSE;    // Used to inhibit sending the status
                                    // to the service controller.

    EnterCriticalSection(&StatusCriticalSection);

    ElfDbgPrint(("ElfStatusUpdate (entry) NewState = %d, OldState = %d\n",
                 NewState,
                 ElState));

    if (NewState == STOPPED) {
        if (ElState == STOPPED) {
            //
            // It was already stopped, don't send another SetServiceStatus.
            //
            inhibit = TRUE;
        }
        else {
            //
            // The shut down is complete, indicate that the eventlog
            // has stopped.
            //
            ElStatus.dwCurrentState =  SERVICE_STOPPED;
            ElStatus.dwControlsAccepted = 0;
            ElStatus.dwCheckPoint = 0;
            ElStatus.dwWaitHint = 0;

            ElStatus.dwWin32ExitCode = ElUninstallCode;
            ElStatus.dwServiceSpecificExitCode = ElSpecificCode;

        }
        ElState = NewState;
    }
    else if (NewState != UPDATE_ONLY) {

        //
        // We are not being asked to change to the STOPPED state.
        //
        switch(ElState) {

        case STARTING:
            if (NewState == STOPPING) {

                ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                ElStatus.dwControlsAccepted = 0;
                ElStatus.dwCheckPoint = HintCount++;
                ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
                ElState = NewState;

                EventlogShutdown = TRUE;
            }

            else if (NewState == RUNNING) {

                //
                // The Eventlog Service has completed installation.
                //
                ElStatus.dwCurrentState =  SERVICE_RUNNING;
                ElStatus.dwCheckPoint = 0;
                ElStatus.dwWaitHint = 0;

                ElStatus.dwControlsAccepted = ELF_CONTROLS_ACCEPTED;
                ElState = NewState;
            }

            else {
                //
                // The NewState must be STARTING.  So update the pending
                // count
                //

                ElStatus.dwCurrentState =  SERVICE_START_PENDING;
                ElStatus.dwControlsAccepted = 0;
                ElStatus.dwCheckPoint = HintCount++;
                ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
            }
            break;

        case RUNNING:

            if (NewState == STOPPING) {

                ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                ElStatus.dwControlsAccepted = 0;

                EventlogShutdown = TRUE;
            }

#if DBG
            else if (NewState == PAUSING) {
                ElStatus.dwCurrentState =  SERVICE_PAUSE_PENDING;
            }
            else if (NewState == PAUSED) {
                ElStatus.dwCurrentState =  SERVICE_PAUSED;
            }

#endif // DBG

            ElStatus.dwCheckPoint = HintCount++;
            ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
            ElState = NewState;

            break;

        case STOPPING:
            //
            // No matter what else was passed in, force the status to
            // indicate that a shutdown is pending.
            //
            ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
            ElStatus.dwControlsAccepted = 0;
            ElStatus.dwCheckPoint = HintCount++;
            ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
            EventlogShutdown = TRUE;

            break;

        case STOPPED:

            ASSERT(NewState == STARTING);

            //
            // The Eventlog Service is starting up after being stopped.
            // This can occur if the service is manually started after
            // failing to start or after the service has been stopped
            // manually on a developer's build.
            //
            ElStatus.dwCurrentState =  SERVICE_START_PENDING;
            ElStatus.dwCheckPoint = 0;
            ElStatus.dwWaitHint = 0;

            ElStatus.dwControlsAccepted = ELF_CONTROLS_ACCEPTED;
            ElState = NewState;
            break;

#if DBG

        case PAUSING:
            if (NewState == PAUSED) {
                ElStatus.dwCurrentState =  SERVICE_PAUSED;
                ElStatus.dwCheckPoint = 0;
                ElStatus.dwWaitHint = 0;
            }
            else {
                ElStatus.dwCheckPoint = HintCount++;
                ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
            }
            ElState = NewState;
            break;

        case PAUSED:

            if (NewState == RUNNING) {
            
                //
                // The Eventlog Service has completed installation.
                //
                ElStatus.dwCurrentState =  SERVICE_RUNNING;
                ElStatus.dwCheckPoint = 0;
                ElStatus.dwWaitHint = 0;

                ElStatus.dwControlsAccepted = ELF_CONTROLS_ACCEPTED;
                ElState = NewState;
            }
            break;

#endif // DBG

        }
    }

    if (!inhibit) {

        ASSERT(ElfServiceStatusHandle != 0);

        if (!SetServiceStatus( ElfServiceStatusHandle, &ElStatus )) {

            ElfDbgPrint(("ElfStatusUpdate, SetServiceStatus Failed %d\n",
                         GetLastError()));
        }
    }

    ElfDbgPrint(("ElfStatusUpdate (exit) State = %d\n",ElState));
    status = ElState;
    LeaveCriticalSection(&StatusCriticalSection);

    return(status);
}


DWORD
GetElState (
    VOID
    )

/*++

Routine Description:

    Obtains the state of the Eventlog Service.  This state information
    is protected as a critical section such that only one thread can
    modify or read it at a time.

Arguments:

    none

Return Value:

    The Eventlog State is returned as the return value.

--*/
{
    DWORD   status;

    EnterCriticalSection(&StatusCriticalSection);
    status = ElState;
    LeaveCriticalSection(&StatusCriticalSection);

    return(status);
}


VOID
ElInitStatus(VOID)

/*++

Routine Description:

    Initializes the critical section that is used to guard access to the
    status database.

Arguments:

    none

Return Value:

    none

Note:


--*/
{
    ElStatus.dwCurrentState = SERVICE_START_PENDING;
    ElStatus.dwServiceType  = SERVICE_WIN32;

    InitializeCriticalSection(&StatusCriticalSection);
}


VOID
ElCleanupStatus(VOID)

/*++

Routine Description:

    Deletes the critical section used to control access to the thread and
    status database.

Arguments:

    none

Return Value:

    none

Note:


--*/
{
    DeleteCriticalSection(&StatusCriticalSection);
}