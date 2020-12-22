/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

UCHAR HalpInitLevel = 0xFF;

/* PRIVATE FUNCTIONS *********************************************************/

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalInitializeProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default stall count */
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;

    /* Update the processor mask */
    InterlockedBitTestAndSet((PLONG)&HalpActiveProcessors, ProcessorNumber);

    /* Hal specific initialization for this cpu */
    HalpInitProcessor(ProcessorNumber, LoaderBlock);
}

/*
 * @implemented
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Check the boot phase */
    if (BootPhase == 0)
    {
        HalpInitLevel = 0;

        /* Phase 0... save bus type */
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xFF;

        /* Get command-line parameters */
        if (LoaderBlock && LoaderBlock->LoadOptions)
        {
            HalpGetParameters(LoaderBlock->LoadOptions);
        }

        /* Check for PRCB version mismatch */
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        /* Checked/free HAL requires checked/free kernel */
        if (Prcb->BuildType != HalpBuildType)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, HalpBuildType, 0);
        }

        /* Setup I/O space */
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;

#if !defined(SARCH_PC98) && !defined(SARCH_XBOX)
#ifdef _M_IX86
        if (HalpBusType == MACHINE_TYPE_EISA)
        {
            DPRINT1("HalpBusType - MACHINE_TYPE_EISA\n");
            HalpEisaIoSpace.Next = &HalpDefaultIoSpace;
            HalpAddressUsageList = &HalpEisaIoSpace;
        }
#endif
#endif
        /* Do some HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);

#ifdef _M_AMD64
        HalInitializeBios(0, LoaderBlock);
#endif
    }
    else if (BootPhase == 1)
    {
        HalpInitLevel = 1;

        /* Initialize bus handlers */
        HalpInitBusHandlers();

        /* Do some HAL-specific initialization */
        HalpInitPhase1();

#ifdef _M_AMD64
        HalInitializeBios(1, LoaderBlock);
#endif
    }

    /* All done, return */
    return TRUE;
}

NTSTATUS
NTAPI
HalpAllocateMapRegisters(_In_ PADAPTER_OBJECT AdapterObject,
                         _In_ ULONG Unknown,
                         _In_ ULONG Unknown2,
                         PMAP_REGISTER_ENTRY Registers)
{
    DPRINT1("HalpAllocateMapRegisters: FIXME. AdapterObject %X\n", AdapterObject);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
HaliLocateHiberRanges(_In_ PVOID MemoryMap)
{
    DPRINT1("HaliLocateHiberRanges: FIXME. MemoryMap %X\n", MemoryMap);
}

/* EOF */
