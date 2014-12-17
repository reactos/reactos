/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "usersrv.h"

#include "api.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HINSTANCE UserServerDllInstance = NULL;

/* Handles for Power and Media events. Used by both usersrv and win32k. */
HANDLE ghPowerRequestEvent;
HANDLE ghMediaRequestEvent;

/* Copy of CSR Port handle for win32k */
HANDLE CsrApiPort = NULL;

/* Memory */
HANDLE UserServerHeap = NULL;   // Our own heap.

// Windows Server 2003 table from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
PCSR_API_ROUTINE UserServerApiDispatchTable[UserpMaxApiNumber - USERSRV_FIRST_API_NUMBER] =
{
    SrvExitWindowsEx,
    SrvEndTask,
    SrvLogon,
    SrvRegisterServicesProcess, // Not present in Win7
    SrvActivateDebugger,
    SrvGetThreadConsoleDesktop, // Not present in Win7
    SrvDeviceEvent,
    SrvRegisterLogonProcess,    // Not present in Win7
    SrvCreateSystemThreads,
    SrvRecordShutdownReason,
    // SrvCancelShutdown,              // Added in Vista
    // SrvConsoleHandleOperation,      // Added in Win7
    // SrvGetSetShutdownBlockReason,   // Added in Vista
};

BOOLEAN UserServerApiServerValidTable[UserpMaxApiNumber - USERSRV_FIRST_API_NUMBER] =
{
    FALSE,   // SrvExitWindowsEx
    FALSE,   // SrvEndTask
    FALSE,   // SrvLogon
    FALSE,   // SrvRegisterServicesProcess
    FALSE,   // SrvActivateDebugger
    TRUE,    // SrvGetThreadConsoleDesktop
    FALSE,   // SrvDeviceEvent
    FALSE,   // SrvRegisterLogonProcess
    FALSE,   // SrvCreateSystemThreads
    FALSE,   // SrvRecordShutdownReason
    // FALSE,   // SrvCancelShutdown
    // FALSE,   // SrvConsoleHandleOperation
    // FALSE,   // SrvGetSetShutdownBlockReason
};

/*
 * On Windows Server 2003, CSR Servers contain
 * the API Names Table only in Debug Builds.
 */
#ifdef CSR_DBG
PCHAR UserServerApiNameTable[UserpMaxApiNumber - USERSRV_FIRST_API_NUMBER] =
{
    "SrvExitWindowsEx",
    "SrvEndTask",
    "SrvLogon",
    "SrvRegisterServicesProcess",
    "SrvActivateDebugger",
    "SrvGetThreadConsoleDesktop",
    "SrvDeviceEvent",
    "SrvRegisterLogonProcess",
    "SrvCreateSystemThreads",
    "SrvRecordShutdownReason",
    // "SrvCancelShutdown",
    // "SrvConsoleHandleOperation",
    // "SrvGetSetShutdownBlockReason",
};
#endif

/* FUNCTIONS ******************************************************************/

// PUSER_SOUND_SENTRY. Used in basesrv.dll
BOOL NTAPI _UserSoundSentry(VOID)
{
    // TODO: Do something.
    return TRUE;
}

ULONG
NTAPI
CreateSystemThreads(PVOID pParam)
{
    NtUserCallOneParam((DWORD)pParam, ONEPARAM_ROUTINE_CREATESYSTEMTHREADS);
    DPRINT1("This thread should not terminate!\n");
    return 0;
}

CSR_API(SrvCreateSystemThreads)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvActivateDebugger)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetThreadConsoleDesktop)
{
    PUSER_GET_THREAD_CONSOLE_DESKTOP GetThreadConsoleDesktopRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.GetThreadConsoleDesktopRequest;

    DPRINT1("%s not yet implemented\n", __FUNCTION__);

    /* Return nothing for the moment... */
    GetThreadConsoleDesktopRequest->ConsoleDesktop = NULL;

    /* Always succeeds */
    return STATUS_SUCCESS;
}

CSR_API(SrvDeviceEvent)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UserClientConnect(IN PCSR_PROCESS CsrProcess,
                  IN OUT PVOID  ConnectionInfo,
                  IN OUT PULONG ConnectionInfoLength)
{
    NTSTATUS Status;
    // PUSERCONNECT
    PUSERSRV_API_CONNECTINFO ConnectInfo = (PUSERSRV_API_CONNECTINFO)ConnectionInfo;

    DPRINT1("UserClientConnect\n");

    /* Check if we don't have an API port yet */
    if (CsrApiPort == NULL)
    {
        /* Query the API port and save it globally */
        CsrApiPort = CsrQueryApiPort();

        /* Inform win32k about the API port */
        Status = NtUserSetInformationThread(NtCurrentThread(),
                                            UserThreadCsrApiPort,
                                            &CsrApiPort,
                                            sizeof(CsrApiPort));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Check connection info validity */
    if ( ConnectionInfo       == NULL ||
         ConnectionInfoLength == NULL ||
        *ConnectionInfoLength != sizeof(*ConnectInfo) )
    {
        DPRINT1("USERSRV: Connection failed - ConnectionInfo = 0x%p ; ConnectionInfoLength = 0x%p (%lu), expected %lu\n",
                ConnectionInfo,
                ConnectionInfoLength,
                ConnectionInfoLength ? *ConnectionInfoLength : (ULONG)-1,
                sizeof(*ConnectInfo));

        return STATUS_INVALID_PARAMETER;
    }

    /* Pass the request to win32k */
    ConnectInfo->dwDispatchCount = 0; // gDispatchTableValues;
    Status = NtUserProcessConnect(CsrProcess->ProcessHandle,
                                  ConnectInfo,
                                  *ConnectionInfoLength);

    return Status;
}

CSR_SERVER_DLL_INIT(UserServerDllInitialization)
{
/*** From win32csr... ***/
    HANDLE ServerThread;
    CLIENT_ID ClientId;
    NTSTATUS Status;
    UINT i;
/*** END - From win32csr... ***/

    /* Initialize the memory */
    UserServerHeap = RtlGetProcessHeap();

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = USERSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = UserpMaxApiNumber;
    LoadedServerDll->DispatchTable = UserServerApiDispatchTable;
    LoadedServerDll->ValidTable = UserServerApiServerValidTable;
#ifdef CSR_DBG
    LoadedServerDll->NameTable = UserServerApiNameTable;
#endif
    LoadedServerDll->SizeOfProcessData = 0;
    LoadedServerDll->ConnectCallback = UserClientConnect;
    LoadedServerDll->DisconnectCallback = NULL;
    LoadedServerDll->HardErrorCallback = UserServerHardError;
    LoadedServerDll->ShutdownProcessCallback = UserClientShutdown;

    UserServerDllInstance = LoadedServerDll->ServerHandle;

/*** From win32csr... See r54125 ***/
    /* Start the Raw Input Thread and the Desktop Thread */
    for (i = 0; i < 2; ++i)
    {
        Status = RtlCreateUserThread(NtCurrentProcess(),
                                     NULL, TRUE, 0, 0, 0,
                                     CreateSystemThreads,
                                     (PVOID)i, &ServerThread, &ClientId);
        if (NT_SUCCESS(Status))
        {
            NtResumeThread(ServerThread, NULL);
            NtClose(ServerThread);
        }
        else
        {
            DPRINT1("Cannot start Raw Input Thread!\n");
        }
    }
/*** END - From win32csr... ***/

    /* Create the power request event */
    Status = NtCreateEvent(&ghPowerRequestEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Power request event creation failed with Status 0x%08x\n", Status);
        return Status;
    }

    /* Create the media request event */
    Status = NtCreateEvent(&ghMediaRequestEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Media request event creation failed with Status 0x%08x\n", Status);
        return Status;
    }

    /* Set the process creation notify routine for BASE */
    BaseSetProcessCreateNotify(NtUserNotifyProcessCreate);

    /* Initialize the kernel mode subsystem */
    Status = NtUserInitialize(USER_VERSION,
                              ghPowerRequestEvent,
                              ghMediaRequestEvent);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtUserInitialize failed with Status 0x%08x\n", Status);
        return Status;
    }

    /* All done */
    return STATUS_SUCCESS;
}

/* EOF */
