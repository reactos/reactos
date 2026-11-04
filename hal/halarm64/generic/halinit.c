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

/* Debug ports are device memory */
static
PVOID
NTAPI
HalpKdMapPhysicalMemory(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG NumberPages)
{
    return HalpMapPhysicalMemory(PhysicalAddress, NumberPages, TRUE);
}

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

        /* The debugger looks for its port right after this */
        KdGetAcpiTablePhase0 = HalAcpiGetTable;
        KdMapPhysicalMemory64 = HalpKdMapPhysicalMemory;
        KdUnmapVirtualAddress = HalpUnmapVirtualAddress;
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
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PCM_RESOURCE_LIST List;
    ULONG Size;

    /* Keep the PCI configuration windows away from the arbiters */
    HalpPublishMmConfigRanges();

    /* The interrupt controller registers belong to the HAL */
    Size = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[2]);
    List = ExAllocatePoolZero(PagedPool, Size, TAG_HAL);
    if (!List)
        return;

    List->Count = 1;
    List->List[0].InterfaceType = Internal;
    List->List[0].BusNumber = 0;
    List->List[0].PartialResourceList.Version = 1;
    List->List[0].PartialResourceList.Revision = 1;

    Descriptor = List->List[0].PartialResourceList.PartialDescriptors;
    if (HalpPlatform.GicdBase.QuadPart)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start = HalpPlatform.GicdBase;
        Descriptor->u.Memory.Length = HALP_GICD_SIZE;
        Descriptor++;
    }

    /* Without a discovery range every processor has one frame pair */
    if (HalpPlatform.GicrBase.QuadPart)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start = HalpPlatform.GicrBase;
        Descriptor->u.Memory.Length = HalpPlatform.GicrLength ?
                                      HalpPlatform.GicrLength :
                                      KeNumberProcessors * HALP_GICR_FRAME_SIZE;
        Descriptor++;
    }

    /* GICv2 has no redistributors, the CPU interface frame takes their place */
    if (!HalpPlatform.GicrBase.QuadPart && HalpPlatform.GiccBase.QuadPart)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start = HalpPlatform.GiccBase;
        Descriptor->u.Memory.Length = HALP_GICC_SIZE;
        Descriptor++;
    }

    List->List[0].PartialResourceList.Count =
        (ULONG)(Descriptor - List->List[0].PartialResourceList.PartialDescriptors);
    Size = (ULONG)((PUCHAR)Descriptor - (PUCHAR)List);

    /* Physical and bus addresses are the same, so both lists match */
    IoReportHalResourceUsage(&HalName, List, List, Size);
    ExFreePoolWithTag(List, TAG_HAL);
}

VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
    __dsb(_ARM64_BARRIER_SY);
}

PUCHAR KdComPortInUse = NULL;
