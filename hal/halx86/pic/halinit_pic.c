/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/up/halinit_up.c
 * PURPOSE:       Initialize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

const USHORT HalpBuildType = HAL_BUILD_TYPE;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
HaliInitPowerManagement(
    _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE *PmHalDispatchTable)
{
    DPRINT1("HaliInitPowerManagement()\n");
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
HaliAssignHaltSystem(VOID)
{
    /* Fill out HalDispatchTable */
    HalInitPowerManagement = HaliInitPowerManagement;

    /* Fill out HalPrivateDispatchTable */
    HalHaltSystem = HaliHaltSystem;
}

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default IDR */
    KeGetPcr()->IDR = 0xFFFFFFFB;
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    DPRINT1("HalpInitialize: HalpBusType - %X\n", HalpBusType);

    if (HalpBusType == MACHINE_TYPE_MCA)
    {
        KeBugCheckEx(MISMATCHED_HAL, 3, MACHINE_TYPE_MCA, 0, 0);
    }

    /* Fill out HalDispatchTable */
    HalSetSystemInformation = HaliSetSystemInformation;
}

VOID
HalpInitPhase1(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Enable IRQ 0 */
    HalpEnableInterruptHandler(IDT_DEVICE,
                               0,
                               PRIMARY_VECTOR_BASE,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);

    /* Enable IRQ 8 */
    HalpEnableInterruptHandler(IDT_DEVICE,
                               0,
                               PRIMARY_VECTOR_BASE + 8,
                               PROFILE_LEVEL,
                               HalpProfileInterrupt,
                               Latched);

    Prcb = KeGetCurrentPrcb();

    if (Prcb->CpuType == 3) // 80387 ?
    {
        DPRINT1("HalpInitPhase1: Prcb->CpuType - %X\n", Prcb->CpuType);
        UNIMPLEMENTED;
        ASSERT(FALSE);
        //HalpEnableInterruptHandler(...)
    }

    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
