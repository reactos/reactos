/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Entry point and helpers
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

EFI_HANDLE GlobalImageHandle;
EFI_SYSTEM_TABLE *GlobalSystemTable;
PVOID UefiServiceStack;
PVOID BasicStack;

void _changestack(VOID);

/* FUNCTIONS ******************************************************************/

EFI_STATUS
EfiEntry(
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable)
{
    PCSTR CmdLine = ""; // FIXME: Determine a command-line from UEFI boot options

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"UEFI EntryPoint: Starting freeldr from UEFI");
    GlobalImageHandle = ImageHandle;
    GlobalSystemTable = SystemTable;

    /* Load the default settings from the command-line */
    LoadSettings(CmdLine);

    /* Debugger pre-initialization */
    DebugInit(BootMgrInfo.DebugString);

    MachInit(CmdLine);

    /* UI pre-initialization */
    if (!UiInitialize(FALSE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        goto Quit;
    }

    /* Initialize memory manager */
    if (!MmInitializeMemoryManager())
    {
        UiMessageBoxCritical("Unable to initialize memory manager.");
        goto Quit;
    }

    /* Initialize I/O subsystem */
    FsInit();

    /* Initialize the module list */
    if (!PeLdrInitializeModuleList())
    {
        UiMessageBoxCritical("Unable to initialize module list.");
        goto Quit;
    }

    if (!MachInitializeBootDevices())
    {
        UiMessageBoxCritical("Error when detecting hardware.");
        goto Quit;
    }

    /* 0x32000 is what UEFI defines, but we can go smaller if we want */
    SIZE_T StackSize = 0x32000;
    PVOID AllocatedStackMem = MmAllocateMemoryWithType(StackSize, LoaderOsloaderStack);

    if (!AllocatedStackMem)
    {
        UiMessageBoxCritical("Unable to allocate OS loader stack.");
        goto Quit;
    }

    /* Stacks grow downward so set the initial stack pointer to the top of the allocated region */
    BasicStack = (PVOID)((ULONG_PTR)AllocatedStackMem + StackSize);
    _changestack();

Quit:
    /* If we reach this point, something went wrong before, therefore reboot */
    Reboot();

    UNREACHABLE;
    return 0;
}

DECLSPEC_NORETURN
void
ExecuteLoaderCleanly(PVOID PreviousStack)
{
    TRACE("ExecuteLoaderCleanly Entry\n");
    UefiServiceStack = PreviousStack;

    RunLoader();
    Reboot();
    UNREACHABLE;
}

DECLSPEC_NORETURN
VOID __cdecl Reboot(VOID)
{
    WARN("Performing cold reset\n");

    if (GlobalSystemTable && GlobalSystemTable->RuntimeServices)
    {
        /* Attempt a native firmware cold reset */
        GlobalSystemTable->RuntimeServices->ResetSystem(
            EfiResetCold,
            EFI_SUCCESS,
            0,
            NULL
        );
    }

    /* Fallback dead-loop if runtime services are missing or fail to respond */
    for (;;)
    {
#if defined(_M_IX86) || defined(_M_AMD64)
        __halt();
#else
        /* Fallback placeholder loop for other platforms */
        NOTHING;
#endif
    }
}
