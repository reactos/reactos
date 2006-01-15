/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 *
 * PROGRAMMERS:     Alex Ionescu (cleaned up code, moved Executiv stuff to ex/init.c)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define BUILD_OSCSDVERSION(major, minor) (((major & 0xFF) << 8) | (minor & 0xFF))


ULONG NtMajorVersion = 5;
ULONG NtMinorVersion = 0;
ULONG NtOSCSDVersion = BUILD_OSCSDVERSION(4, 0);
ULONG NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG NtGlobalFlag = 0;
CHAR  KeNumberProcessors;
KAFFINITY KeActiveProcessors = 1;
LOADER_PARAMETER_BLOCK KeLoaderBlock;
ULONG KeDcacheFlushCount = 0;
ULONG KeIcacheFlushCount = 0;
ULONG KiDmaIoCoherency = 0; /* RISC Architectures only */
ULONG InitSafeBootMode = 0; /* KB83764 */

LOADER_MODULE KeLoaderModules[64];
static CHAR KeLoaderModuleStrings[64][256];
static CHAR KeLoaderCommandLine[256];
ADDRESS_RANGE KeMemoryMap[64];
ULONG KeMemoryMapRangeCount;
ULONG_PTR FirstKrnlPhysAddr;
ULONG_PTR LastKrnlPhysAddr;
ULONG_PTR LastKernelAddress;

PVOID KeUserApcDispatcher = NULL;
PVOID KeUserCallbackDispatcher = NULL;
PVOID KeUserExceptionDispatcher = NULL;
PVOID KeRaiseUserExceptionDispatcher = NULL;

ULONG KeLargestCacheLine = 0x40; /* FIXME: Arch-specific */

/* the initial stacks are declared in main_asm.S */
extern ULONG kernel_stack;
extern ULONG kernel_stack_top;
extern ULONG kernel_trap_stack;
extern ULONG kernel_trap_stack_top;

/* These point to the aligned 3 pages */
ULONG init_stack = (ULONG)&kernel_stack;
ULONG init_stack_top = (ULONG)&kernel_stack_top;
ULONG trap_stack = (ULONG)&kernel_trap_stack;
ULONG trap_stack_top = (ULONG)&kernel_trap_stack_top;

/* Cached modules from the loader block */
PLOADER_MODULE CachedModules[MaximumCachedModuleType];

extern unsigned int _image_base__;
ULONG_PTR KERNEL_BASE = (ULONG_PTR)&_image_base__;

VOID INIT_FUNCTION _main(ULONG MultiBootMagic, PLOADER_PARAMETER_BLOCK _LoaderBlock);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, _main)
#endif

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
ULONG
STDCALL
KeGetRecommendedSharedDataAlignment(VOID)
{
    return KeLargestCacheLine;
}

VOID
#ifdef __GNUC__
__attribute((noinline))
#endif
KiSystemStartup(BOOLEAN BootProcessor)
{
    DPRINT("KiSystemStartup(%d)\n", BootProcessor);

    /* Initialize the Application Processor */
    if (!BootProcessor) KeApplicationProcessorInit();

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(KeNumberProcessors, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Load the Kernel if this is the Boot CPU, else inialize the App CPU only */
    if (BootProcessor) {

        /* Initialize the Kernel Executive */
        ExpInitializeExecutive();

        /* Free Initial Memory */
        MiFreeInitMemory();

        /* Never returns */
#if 0
	/* FIXME:
         *   The initial thread isn't a real ETHREAD object, we cannot call PspExitThread.
	 */
	PspExitThread(STATUS_SUCCESS);
#else
	while (1) {
	   LARGE_INTEGER Timeout;
	   Timeout.QuadPart = 0x7fffffffffffffffLL;
	   KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
	}
#endif
    } else {

        /* Do application processor initialization */
        KeApplicationProcessorInitDispatcher();

        /* Lower IRQL and go to Idle Thread */
        KeLowerIrql(PASSIVE_LEVEL);
        PsIdleThreadMain(NULL);
    }

    /* Bug Check and loop forever if anything failed */
    KEBUGCHECK(0);
    for(;;);
}

/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          LoaderBlock = Pointer to boot parameters initialized by the boot
 *                        loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
VOID
INIT_FUNCTION
_main(ULONG MultiBootMagic,
      PLOADER_PARAMETER_BLOCK _LoaderBlock)
{
    ULONG i;
    ULONG size;
    ULONG HalBase;
    ULONG DriverBase;
    ULONG DriverSize;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_OPTIONAL_HEADER OptHead;
    CHAR* s;

    /* Copy the Loader Block Data locally since Low-Memory will be wiped */
    memcpy(&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
    memcpy(&KeLoaderModules[1],
           (PVOID)KeLoaderBlock.ModsAddr,
           sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
    KeLoaderBlock.ModsCount++;
    KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

    /* Save the Base Address */
    MmSystemRangeStart = (PVOID)KeLoaderBlock.KernelBase;

    /* Set the Command Line */
    strcpy(KeLoaderCommandLine, (PCHAR)_LoaderBlock->CommandLine);
    KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;

    /* Write the first Module (the Kernel) */
    strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
    KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
    KeLoaderModules[0].ModStart = KERNEL_BASE;

    /* Read PE Data */
    NtHeader = RtlImageNtHeader((PVOID)KeLoaderModules[0].ModStart);
    OptHead = &NtHeader->OptionalHeader;

    /* Set Kernel Ending */
    KeLoaderModules[0].ModEnd = KeLoaderModules[0].ModStart + PAGE_ROUND_UP((ULONG)OptHead->SizeOfImage);

    /* Create a block for each module */
    for (i = 1; i < KeLoaderBlock.ModsCount; i++) {

        /* Check if we have to copy the path or not */
        if ((s = strrchr((PCHAR)KeLoaderModules[i].String, '/')) != 0) {

            strcpy(KeLoaderModuleStrings[i], s + 1);

        } else {

            strcpy(KeLoaderModuleStrings[i], (PCHAR)KeLoaderModules[i].String);
        }

        /* Substract the base Address in Physical Memory */
        KeLoaderModules[i].ModStart -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeLoaderModules[i].ModStart += KERNEL_BASE;

        /* Substract the base Address in Physical Memory */
        KeLoaderModules[i].ModEnd -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeLoaderModules[i].ModEnd += KERNEL_BASE;

        /* Select the proper String */
        KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

    /* Choose last module address as the final kernel address */
    LastKernelAddress = PAGE_ROUND_UP(KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd);

    /* Low level architecture specific initialization */
    KeInit1((PCHAR)KeLoaderBlock.CommandLine, &LastKernelAddress);

    /* Select the HAL Base */
    HalBase = KeLoaderModules[1].ModStart;

    /* Choose Driver Base */
    DriverBase = LastKernelAddress;
    LdrHalBase = (ULONG_PTR)DriverBase;

    /* Initialize Module Management */
    LdrInitModuleManagement();

    /* Load HAL.DLL with the PE Loader */
    LdrSafePEProcessModule((PVOID)HalBase,
                            (PVOID)DriverBase,
                            (PVOID)KERNEL_BASE,
                            &DriverSize);

    /* Increase the last kernel address with the size of HAL */
    LastKernelAddress += PAGE_ROUND_UP(DriverSize);

    /* Load the Kernel with the PE Loader */
    LdrSafePEProcessModule((PVOID)KERNEL_BASE,
                           (PVOID)KERNEL_BASE,
                           (PVOID)DriverBase,
                           &DriverSize);

    /* Now select the final beginning and ending Kernel Addresses */
    FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - KERNEL_BASE + 0x200000;
    LastKrnlPhysAddr = LastKernelAddress - KERNEL_BASE + 0x200000;

    KeMemoryMapRangeCount = 0;
    if (KeLoaderBlock.Flags & MB_FLAGS_MMAP_INFO) {

        /* We have a memory map from the nice BIOS */
        size = *((PULONG)(KeLoaderBlock.MmapAddr - sizeof(ULONG)));
        i = 0;

        /* Map it until we run out of size */
        while (i < KeLoaderBlock.MmapLength) {

            /* Copy into the Kernel Memory Map */
            memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
                    (PVOID)(KeLoaderBlock.MmapAddr + i),
                    sizeof(ADDRESS_RANGE));

            /* Increase Memory Map Count */
            KeMemoryMapRangeCount++;

            /* Increase Size */
            i += size;
        }

        /* Save data */
        KeLoaderBlock.MmapLength = KeMemoryMapRangeCount * sizeof(ADDRESS_RANGE);
        KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;

    } else {

        /* Nothing from BIOS */
        KeLoaderBlock.MmapLength = 0;
        KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }

    /* Initialize the Debugger */
    KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Initialize HAL */
    HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Display separator + ReactOS version at start of the debug log */
    DPRINT1("---------------------------------------------------------------\n");
    DPRINT1("ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");

    /* Do general System Startup */
    KiSystemStartup(1);
}

/* EOF */
