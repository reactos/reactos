/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "winsrv.h"
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

BOOLEAN UserServerApiServerValidTable[UserpMaxApiNumber - USERSRV_FIRST_API_NUMBER] =
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
};

PCHAR UserServerApiNameTable[UserpMaxApiNumber - USERSRV_FIRST_API_NUMBER] =
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
};


/* FUNCTIONS ******************************************************************/

// PUSER_SOUND_SENTRY. Used in basesrv.dll
BOOL WINAPI _UserSoundSentry(VOID)
{
    // TODO: Do something.
    return TRUE;
}

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
 * UserpInitVideo
 *
 * TODO: we need a virtual device for sessions other than
 * TODO: the console one
 */
NTSTATUS
UserpInitVideo(VOID)
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

/*** HACK from win32csr... ***/
static HHOOK hhk = NULL;

LRESULT
CALLBACK
KeyboardHookProc(int nCode,
                 WPARAM wParam,
                 LPARAM lParam)
{
    return CallNextHookEx(hhk, nCode, wParam, lParam);
}
/*** END - HACK from win32csr... ***/

DWORD
WINAPI
CreateSystemThreads(PVOID pParam)
{
    NtUserCallOneParam((DWORD)pParam, ONEPARAM_ROUTINE_CREATESYSTEMTHREADS);
    DPRINT1("This thread should not terminate!\n");
    return 0;
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
    UserpInitVideo();
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
    LoadedServerDll->DisconnectCallback = NULL;
    LoadedServerDll->HardErrorCallback = UserServerHardError;
    LoadedServerDll->ShutdownProcessCallback = NULL;

    UserServerDllInstance = LoadedServerDll->ServerHandle;

/*** From win32csr... See r54125 ***/
    /* Start the Raw Input Thread and the Desktop Thread */
    for (i = 0; i < 2; ++i)
    {
        Status = RtlCreateUserThread(NtCurrentProcess(), NULL, TRUE, 0, 0, 0, (PTHREAD_START_ROUTINE)CreateSystemThreads, (PVOID)i, &ServerThread, &ClientId);
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

BOOL
WINAPI
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstanceDll);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        DPRINT1("WINSRV - HACK: Use keyboard hook hack\n");
/*** HACK from win32csr... ***/
//
// HACK HACK HACK ReactOS to BOOT! Initialization BUG ALERT! See bug 5655.
//
        hhk = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
// BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//  BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//   BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!

/*** END - HACK from win32csr... ***/
    }

    return TRUE;
}

/* EOF */
