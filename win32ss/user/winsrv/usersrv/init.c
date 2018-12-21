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
#include "api.h"            // USERSRV Public server APIs definitions
#include "../consrv/api.h"  //  CONSRV Public server APIs definitions

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

BOOL CALLBACK
FindTopLevelWnd(
    IN HWND hWnd,
    IN LPARAM lParam)
{
    if (GetWindow(hWnd, GW_OWNER) == NULL)
    {
        *(HWND*)lParam = hWnd;
        return FALSE;
    }
    return TRUE;
}

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
#if 1
    // WARNING: Here we have a very ugly hack in order for ghost windows to work.
    // In windows the ghost class is implemented in win32k but in reactos
    // it is implemented in user32. At first I thought that there would be no problem 
    // for win32k to use a class registered in user32. However it turns out that
    // most of the time the user mode classes are not registered at all by winsrv as
    // they are not needed. This makes it impossible however for win32k to create 
    // the ghost window whenever it likes as the ghost class may not be registered yet.
    // This means that at some point csrss needs to register the classes of user32
    // before win32k tries to create the ghost window. 
    // To make matters worse our user32 has no clue if it is running in the context of
    // csrss or in a client process so as long as we have the ghost class in user32 and
    // user32 has no clue if it is in csrss or not, the hack to register the ghost class
    // needs to be in winsrv.
    // To avoid complicating the situation even more by registering all classes on boot 
    // before any desktop is created, the following hack checks if the input desktop is
    // already created and if it is, it will force user32 to register its classes.
    // Keep in mind that the desktop check means that this hack will only trigger when 
    // we first try to create the ghost thread and hence there are no bad consequences because of it.
    // If the ghost class is ever moved to win32k from user32, this hack can be removed.
    HDESK hDeskInput = OpenInputDesktop(0, FALSE, /*DESKTOP_ALL_ACCESS*/0x01ff);
    if (hDeskInput)
    {
        /* This clause will only run when we are creating the ghost thread */
        WNDCLASSEXW wcx;

        /* Use a desktop before registering the system classes */
        SetThreadDesktop(hDeskInput);

        /* Querying the class info we force user32 to register the class if it was not registered yet. */
        GetClassInfoExW(NULL, L"Ghost", &wcx);
    }
#endif

    NtUserCallOneParam((DWORD_PTR)pParam, ONEPARAM_ROUTINE_CREATESYSTEMTHREADS);
    RtlExitUserThread(0);
    return 0;
}

CSR_API(SrvCreateSystemThreads)
{
    NTSTATUS Status = CsrExecServerThread(CreateSystemThreads, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot start system thread!\n");
    }

    return Status;
}

CSR_API(SrvActivateDebugger)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvGetThreadConsoleDesktop)
{
    NTSTATUS Status;
    PUSER_GET_THREAD_CONSOLE_DESKTOP GetThreadConsoleDesktopRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.GetThreadConsoleDesktopRequest;

    Status = GetThreadConsoleDesktop(GetThreadConsoleDesktopRequest->ThreadId,
                                     &GetThreadConsoleDesktopRequest->ConsoleDesktop);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetThreadConsoleDesktop(%lu) failed with Status 0x%08x\n",
                GetThreadConsoleDesktopRequest->ThreadId, Status);
    }

    /* Windows-compatibility: Always return success since User32 relies on this! */
    return STATUS_SUCCESS;
}

CSR_API(SrvDeviceEvent)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvLogon)
{
    PUSER_LOGON LogonRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.LogonRequest;

    DPRINT1("We are logged %s\n", LogonRequest->IsLogon ? "on" : "off");

    /* Impersonate the caller in order to retrieve settings in its context */
    if (!CsrImpersonateClient(NULL))
        return STATUS_UNSUCCESSFUL;

    GetTimeouts(&ShutdownSettings);

    /* We are done */
    CsrRevertToSelf();
    return STATUS_SUCCESS;
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

    DPRINT("UserClientConnect\n");

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
    NTSTATUS Status;

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

    /* Initialize the hard errors cache */
    UserInitHardErrorsCache();

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
