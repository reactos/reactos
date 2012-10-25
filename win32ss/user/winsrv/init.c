/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "winsrv.h"

/* Public Win32K Headers */
// For calling NtUser...()
#include <ntuser.h>

#define NDEBUG
#include <debug.h>

HANDLE DllHandle = NULL;
// HANDLE WinSrvApiPort = NULL;

/* Memory */
HANDLE UserSrvHeap = NULL;          // Our own heap.
// HANDLE BaseSrvSharedHeap = NULL;    // Shared heap with CSR. (CsrSrvSharedSectionHeap)
// PBASE_STATIC_SERVER_DATA BaseStaticServerData = NULL;   // Data that we can share amongst processes. Initialized inside BaseSrvSharedHeap.


PCSR_API_ROUTINE UserServerApiDispatchTable[UserpMaxApiNumber] =
{
    SrvExitWindowsEx,
    // SrvEndTask,
    // SrvLogon,
    SrvRegisterServicesProcess, // Not present in Win7
    // SrvActivateDebugger,
    // SrvGetThreadConsoleDesktop, // Not present in Win7
    // SrvDeviceEvent,
    SrvRegisterLogonProcess,    // Not present in Win7
    // SrvCreateSystemThreads,
    // SrvRecordShutdownReason,
    // SrvCancelShutdown,              // Added in Vista
    // SrvConsoleHandleOperation,      // Added in Win7
    // SrvGetSetShutdownBlockReason,   // Added in Vista
};

BOOLEAN UserServerApiServerValidTable[UserpMaxApiNumber] =
{
    FALSE,   // SrvExitWindowsEx
    // FALSE,   // SrvEndTask
    // FALSE,   // SrvLogon
    FALSE,   // SrvRegisterServicesProcess
    // FALSE,   // SrvActivateDebugger
    // TRUE,    // SrvGetThreadConsoleDesktop
    // FALSE,   // SrvDeviceEvent
    FALSE,   // SrvRegisterLogonProcess
    // FALSE,   // SrvCreateSystemThreads
    // FALSE,   // SrvRecordShutdownReason
    // FALSE,   // SrvCancelShutdown
    // FALSE,   // SrvConsoleHandleOperation
    // FALSE,   // SrvGetSetShutdownBlockReason

    // FALSE
};

PCHAR UserServerApiNameTable[UserpMaxApiNumber] =
{
    "SrvExitWindowsEx",
    // "SrvEndTask",
    // "SrvLogon",
    "SrvRegisterServicesProcess",
    // "SrvActivateDebugger",
    // "SrvGetThreadConsoleDesktop",
    // "SrvDeviceEvent",
    "SrvRegisterLogonProcess",
    // "SrvCreateSystemThreads",
    // "SrvRecordShutdownReason",
    // "SrvCancelShutdown",
    // "SrvConsoleHandleOperation",
    // "SrvGetSetShutdownBlockReason",

    // NULL
};

/*
PCSR_API_ROUTINE Win32CsrApiDefinitions[] =
{
    CsrGetHandle,
    CsrGetHandle,
    CsrCloseHandle,
    CsrVerifyHandle,
    CsrDuplicateHandle,
    CsrGetInputWaitHandle,
    CsrFillOutputChar,
    CsrReadInputEvent,
    CsrWriteConsoleOutputChar,
    CsrWriteConsoleOutputAttrib,
    CsrFillOutputAttrib,
    CsrSetTextAttrib,
    CsrWriteConsoleOutput,
    CsrFlushInputBuffer,
    CsrReadConsoleOutputChar,
    CsrReadConsoleOutputAttrib,
    CsrExitReactos,
    CsrHardwareStateProperty,
    CsrCreateDesktop,
    CsrShowDesktop,
    CsrHideDesktop,
    CsrSetLogonNotifyWindow,
    CsrRegisterLogonProcess,
    CsrGenerateCtrlEvent,
};

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
{
    CSRSS_DEFINE_API(GET_INPUT_HANDLE,             CsrGetHandle),
    CSRSS_DEFINE_API(GET_OUTPUT_HANDLE,            CsrGetHandle),
    CSRSS_DEFINE_API(CLOSE_HANDLE,                 CsrCloseHandle),
    CSRSS_DEFINE_API(VERIFY_HANDLE,                CsrVerifyHandle),
    CSRSS_DEFINE_API(DUPLICATE_HANDLE,             CsrDuplicateHandle),
    CSRSS_DEFINE_API(GET_INPUT_WAIT_HANDLE,        CsrGetInputWaitHandle),
    CSRSS_DEFINE_API(WRITE_CONSOLE,                CsrWriteConsole),
    CSRSS_DEFINE_API(READ_CONSOLE,                 CsrReadConsole),
    CSRSS_DEFINE_API(ALLOC_CONSOLE,                CsrAllocConsole),
    CSRSS_DEFINE_API(FREE_CONSOLE,                 CsrFreeConsole),
    CSRSS_DEFINE_API(SCREEN_BUFFER_INFO,           CsrGetScreenBufferInfo),
    CSRSS_DEFINE_API(SET_CURSOR,                   CsrSetCursor),
    CSRSS_DEFINE_API(FILL_OUTPUT,                  CsrFillOutputChar),
    CSRSS_DEFINE_API(READ_INPUT,                   CsrReadInputEvent),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_CHAR,    CsrWriteConsoleOutputChar),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_ATTRIB,  CsrWriteConsoleOutputAttrib),
    CSRSS_DEFINE_API(FILL_OUTPUT_ATTRIB,           CsrFillOutputAttrib),
    CSRSS_DEFINE_API(GET_CURSOR_INFO,              CsrGetCursorInfo),
    CSRSS_DEFINE_API(SET_CURSOR_INFO,              CsrSetCursorInfo),
    CSRSS_DEFINE_API(SET_ATTRIB,                   CsrSetTextAttrib),
    CSRSS_DEFINE_API(GET_CONSOLE_MODE,             CsrGetConsoleMode),
    CSRSS_DEFINE_API(SET_CONSOLE_MODE,             CsrSetConsoleMode),
    CSRSS_DEFINE_API(CREATE_SCREEN_BUFFER,         CsrCreateScreenBuffer),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER,            CsrSetScreenBuffer),
    CSRSS_DEFINE_API(SET_TITLE,                    CsrSetTitle),
    CSRSS_DEFINE_API(GET_TITLE,                    CsrGetTitle),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT,         CsrWriteConsoleOutput),
    CSRSS_DEFINE_API(FLUSH_INPUT_BUFFER,           CsrFlushInputBuffer),
    CSRSS_DEFINE_API(SCROLL_CONSOLE_SCREEN_BUFFER, CsrScrollConsoleScreenBuffer),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_CHAR,     CsrReadConsoleOutputChar),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_ATTRIB,   CsrReadConsoleOutputAttrib),
    CSRSS_DEFINE_API(GET_NUM_INPUT_EVENTS,         CsrGetNumberOfConsoleInputEvents),
    CSRSS_DEFINE_API(EXIT_REACTOS,                 CsrExitReactos),
    CSRSS_DEFINE_API(PEEK_CONSOLE_INPUT,           CsrPeekConsoleInput),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT,          CsrReadConsoleOutput),
    CSRSS_DEFINE_API(WRITE_CONSOLE_INPUT,          CsrWriteConsoleInput),
    CSRSS_DEFINE_API(SETGET_CONSOLE_HW_STATE,      CsrHardwareStateProperty),
    CSRSS_DEFINE_API(GET_CONSOLE_WINDOW,           CsrGetConsoleWindow),
    CSRSS_DEFINE_API(CREATE_DESKTOP,               CsrCreateDesktop),
    CSRSS_DEFINE_API(SHOW_DESKTOP,                 CsrShowDesktop),
    CSRSS_DEFINE_API(HIDE_DESKTOP,                 CsrHideDesktop),
    CSRSS_DEFINE_API(SET_CONSOLE_ICON,             CsrSetConsoleIcon),
    CSRSS_DEFINE_API(SET_LOGON_NOTIFY_WINDOW,      CsrSetLogonNotifyWindow),
    CSRSS_DEFINE_API(REGISTER_LOGON_PROCESS,       CsrRegisterLogonProcess),
    CSRSS_DEFINE_API(GET_CONSOLE_CP,               CsrGetConsoleCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_CP,               CsrSetConsoleCodePage),
    CSRSS_DEFINE_API(GET_CONSOLE_OUTPUT_CP,        CsrGetConsoleOutputCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_OUTPUT_CP,        CsrSetConsoleOutputCodePage),
    CSRSS_DEFINE_API(GET_PROCESS_LIST,             CsrGetProcessList),
    CSRSS_DEFINE_API(ADD_CONSOLE_ALIAS,      CsrAddConsoleAlias),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIAS,      CsrGetConsoleAlias),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES,         CsrGetAllConsoleAliases),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES_LENGTH,  CsrGetAllConsoleAliasesLength),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES,        CsrGetConsoleAliasesExes),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES_LENGTH, CsrGetConsoleAliasesExesLength),
    CSRSS_DEFINE_API(GENERATE_CTRL_EVENT,          CsrGenerateCtrlEvent),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER_SIZE,       CsrSetScreenBufferSize),
    CSRSS_DEFINE_API(GET_CONSOLE_SELECTION_INFO,   CsrGetConsoleSelectionInfo),
    CSRSS_DEFINE_API(GET_COMMAND_HISTORY_LENGTH,   CsrGetCommandHistoryLength),
    CSRSS_DEFINE_API(GET_COMMAND_HISTORY,          CsrGetCommandHistory),
    CSRSS_DEFINE_API(EXPUNGE_COMMAND_HISTORY,      CsrExpungeCommandHistory),
    CSRSS_DEFINE_API(SET_HISTORY_NUMBER_COMMANDS,  CsrSetHistoryNumberCommands),
    CSRSS_DEFINE_API(GET_HISTORY_INFO,             CsrGetHistoryInfo),
    CSRSS_DEFINE_API(SET_HISTORY_INFO,             CsrSetHistoryInfo),
    { 0, 0, NULL }
};
*/


/* FUNCTIONS ******************************************************************/

/*
VOID WINAPI UserStaticServerThread(PVOID x)
{
    // NTSTATUS Status = STATUS_SUCCESS;
    PPORT_MESSAGE Request = (PPORT_MESSAGE)x;
    PPORT_MESSAGE Reply = NULL;
    ULONG MessageType = 0;

    DPRINT("WINSRV: %s(%08lx) called\n", __FUNCTION__, x);

    MessageType = Request->u2.s2.Type;
    DPRINT("WINSRV: %s(%08lx) received a message (Type=%d)\n",
           __FUNCTION__, x, MessageType);
    switch (MessageType)
    {
        default:
            Reply = Request;
            /\* Status = *\/ NtReplyPort(WinSrvApiPort, Reply);
            break;
    }
}
*/

ULONG
InitializeVideoAddressSpace(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PhysMemName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
    NTSTATUS Status;
    HANDLE PhysMemHandle;
    PVOID BaseAddress;
    LARGE_INTEGER Offset;
    SIZE_T ViewSize;
    CHAR IVTAndBda[1024+256];
    
    /* Free the 1MB pre-reserved region. In reality, ReactOS should simply support us mapping the view into the reserved area, but it doesn't. */
    BaseAddress = 0;
    ViewSize = 1024 * 1024;
    Status = ZwFreeVirtualMemory(NtCurrentProcess(), 
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't unmap reserved memory (%x)\n", Status);
        return 0;
    }
    
    /* Open the physical memory section */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PhysMemName,
                               0,
                               NULL,
                               NULL);
    Status = ZwOpenSection(&PhysMemHandle,
                           SECTION_ALL_ACCESS,
                           &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't open \\Device\\PhysicalMemory\n");
        return 0;
    }

    /* Map the BIOS and device registers into the address space */
    Offset.QuadPart = 0xa0000;
    ViewSize = 0x100000 - 0xa0000;
    BaseAddress = (PVOID)0xa0000;
    Status = ZwMapViewOfSection(PhysMemHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't map physical memory (%x)\n", Status);
        ZwClose(PhysMemHandle);
        return 0;
    }

    /* Close physical memory section handle */
    ZwClose(PhysMemHandle);

    if (BaseAddress != (PVOID)0xa0000)
    {
        DPRINT1("Couldn't map physical memory at the right address (was %x)\n",
                BaseAddress);
        return 0;
    }

    /* Allocate some low memory to use for the non-BIOS
     * parts of the v86 mode address space
     */
    BaseAddress = (PVOID)0x1;
    ViewSize = 0xa0000 - 0x1000;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate virtual memory (Status %x)\n", Status);
        return 0;
    }
    if (BaseAddress != (PVOID)0x0)
    {
        DPRINT1("Failed to allocate virtual memory at right address (was %x)\n",
                BaseAddress);
        return 0;
    }

    /* Get the real mode IVT and BDA from the kernel */
    Status = NtVdmControl(VdmInitialize, IVTAndBda);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtVdmControl failed (status %x)\n", Status);
        return 0;
    }

    /* Return success */
    return 1;
}

/**********************************************************************
 * CsrpInitVideo/3
 *
 * TODO: we need a virtual device for sessions other than
 * TODO: the console one
 */
NTSTATUS
CsrpInitVideo(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\??\\DISPLAY1");
    IO_STATUS_BLOCK Iosb;
    HANDLE VideoHandle = (HANDLE) 0;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    InitializeVideoAddressSpace();

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               0,
                               NULL,
                               NULL);
    Status = NtOpenFile(&VideoHandle,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        0);
    if (NT_SUCCESS(Status))
    {
        NtClose(VideoHandle);
    }

    return Status;
}

VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

CSR_SERVER_DLL_INIT(UserServerDllInitialization)
{
/*
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("WINSRV: %s called\n", __FUNCTION__);

    // Get the listening port from csrsrv.dll
    WinSrvApiPort = CsrQueryApiPort ();
    if (NULL == WinSrvApiPort)
    {
        return STATUS_UNSUCCESSFUL;
    }
    // Register our message dispatcher
    Status = CsrAddStaticServerThread (UserStaticServerThread);
    if (NT_SUCCESS(Status))
    {
        //TODO: perform the real user server internal initialization here
    }
    return Status;
*/

    /* Initialize memory */
    UserSrvHeap = RtlGetProcessHeap();  // Initialize our own heap.
    // BaseSrvSharedHeap = LoadedServerDll->SharedSection; // Get the CSR shared heap.
    // LoadedServerDll->SharedSection = BaseStaticServerData;

    CsrpInitVideo();
    NtUserInitialize(0, NULL, NULL);
    PrivateCsrssManualGuiCheck(0);

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = USERSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = UserpMaxApiNumber;
    LoadedServerDll->DispatchTable = UserServerApiDispatchTable;
    LoadedServerDll->ValidTable = UserServerApiServerValidTable;
    LoadedServerDll->NameTable = UserServerApiNameTable;
    LoadedServerDll->SizeOfProcessData = 0;
    LoadedServerDll->ConnectCallback = NULL;
    // LoadedServerDll->DisconnectCallback = Win32CsrReleaseConsole;
    // LoadedServerDll->NewProcessCallback = Win32CsrDuplicateHandleTable;
    LoadedServerDll->HardErrorCallback = Win32CsrHardError;

    /* All done */
    return STATUS_SUCCESS;
}

// PUSER_SOUND_SENTRY. Used in basesrv.dll
BOOL WINAPI _UserSoundSentry(VOID)
{
    // Do something.
    return TRUE;
}

BOOL
WINAPI
DllMain(IN HANDLE hDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        DllHandle = hDll;
    }

    return TRUE;
}

/* EOF */
