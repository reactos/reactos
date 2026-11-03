/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     HAL initialization and processor management
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
HalpWaitForInterrupt(VOID);

VOID
NTAPI
HalpInitDma(VOID);

/* GLOBALS *******************************************************************/

/* There is no EISA or MCA on ARM64, the shared DMA code only looks at this */
ULONG HalpBusType = MACHINE_TYPE_ISA;

BOOLEAN HalpProcessorsStarted;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalInitializeProcessor(
    _In_ ULONG ProcessorNumber,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* The boot processor finds the platform before anything else runs */
    if (ProcessorNumber == 0)
    {
        HalpAcpiInitialize(LoaderBlock);
        HalpGicInitializeDistributor();
    }

    HalpGicInitializeCpu(ProcessorNumber);

    /* Match the priority mask to the IRQL the kernel runs at */
    HalpSetPriorityMask(KeGetPcr()->CurrentIrql);
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
HalInitSystem(
    _In_ ULONG BootPhase,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    UNREFERENCED_PARAMETER(LoaderBlock);

    if (BootPhase == 0)
    {
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);

#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG))
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, PRCB_BUILD_DEBUG, 0);
#endif

        HalpInitializeClock();

        /* The ACPI driver enumerates the platform below the HAL device */
        HalInitPnpDriver = HaliInitPnpDriver;
        HalInitPowerManagement = HaliInitPowerManagement;
    }
    else if (BootPhase == 1)
    {
        HalpInitDma();
        HalpStartClockInterrupt();
    }

    return TRUE;
}

BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID)
{
    HalpProcessorsStarted = TRUE;
    return TRUE;
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    /* Secondary processors are not brought up yet */
    UNREFERENCED_PARAMETER(LoaderBlock);
    UNREFERENCED_PARAMETER(ProcessorState);
    return FALSE;
}

VOID
NTAPI
HalProcessorIdle(VOID)
{
    /* The kernel calls this with interrupts off, wfi wakes on a pending one */
    HalpWaitForInterrupt();
    _enable();
}

CODE_SEG("INIT")
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    UNICODE_STRING HalName = RTL_CONSTANT_STRING(L"ARM64 HAL");
    CM_RESOURCE_LIST Empty;

    /* Keep the PCI configuration windows away from the arbiters */
    HalpPublishMmConfigRanges();

    /* Nothing is claimed by the HAL itself */
    RtlZeroMemory(&Empty, sizeof(Empty));
    IoReportHalResourceUsage(&HalName,
                             &Empty,
                             &Empty,
                             FIELD_OFFSET(CM_RESOURCE_LIST, List));
}

VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
    __dsb(_ARM64_BARRIER_SY);
}

PUCHAR KdComPortInUse = NULL;
