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
BOOL WINAPI _UserSoundSentry(VOID)
{
    // TODO: Do something.
    return TRUE;
}

// From win32ss/user/win32csr/dllmain.c
VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
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

    /* Initialize the video */
    NtUserInitialize(0, NULL, NULL); //
    PrivateCsrssManualGuiCheck(0);

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = USERSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = UserpMaxApiNumber;
    LoadedServerDll->DispatchTable = UserServerApiDispatchTable;
    LoadedServerDll->ValidTable = UserServerApiServerValidTable;
#ifdef CSR_DBG
    LoadedServerDll->NameTable = UserServerApiNameTable;
#endif
    LoadedServerDll->SizeOfProcessData = 0;
    LoadedServerDll->ConnectCallback = NULL;
    LoadedServerDll->DisconnectCallback = NULL;
    LoadedServerDll->HardErrorCallback = UserServerHardError;
    LoadedServerDll->ShutdownProcessCallback = NULL;

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
            DPRINT1("Cannot start Raw Input Thread!\n");
    }
/*** END - From win32csr... ***/

    /* All done */
    return STATUS_SUCCESS;
}

/* EOF */
