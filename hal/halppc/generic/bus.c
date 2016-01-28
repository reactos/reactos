/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halppc/generic/bus.c
 * PURPOSE:         Bus Support Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG HalpBusType;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpRegisterKdSupportFunctions(VOID)
{
    /* Register PCI Device Functions */
    KdSetupPciDeviceForDebugging = HalpSetupPciDeviceForDebugging;
    KdReleasePciDeviceforDebugging = HalpReleasePciDeviceForDebugging;

    /* Register memory functions */
    KdMapPhysicalMemory64 = HalpMapPhysicalMemory64;
    KdUnmapVirtualAddress = HalpUnmapVirtualAddress;

    /* Register ACPI stub */
    KdCheckPowerButton = HalpCheckPowerButton;
}

NTSTATUS
NTAPI
HalpAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                        IN PUNICODE_STRING DriverClassName,
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT DeviceObject,
                        IN INTERFACE_TYPE BusType,
                        IN ULONG BusNumber,
                        IN ULONG SlotNumber,
                        IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    BUS_HANDLER BusHandler;
    PAGED_CODE();

    /* Only PCI is supported */
    if (BusType != PCIBus) return STATUS_NOT_IMPLEMENTED;

    /* Setup fake PCI Bus handler */
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    /* Call the PCI function */
    return HalpAssignPCISlotResources(&BusHandler,
                                      &BusHandler,
                                      RegistryPath,
                                      DriverClassName,
                                      DriverObject,
                                      DeviceObject,
                                      SlotNumber,
                                      AllocatedResources);
}

BOOLEAN
NTAPI
HalpTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                        IN ULONG BusNumber,
                        IN PHYSICAL_ADDRESS BusAddress,
                        IN OUT PULONG AddressSpace,
                        OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    /* Translation is easy */
    TranslatedAddress->QuadPart = BusAddress.QuadPart;
    return TRUE;
}

ULONG
NTAPI
HalpGetSystemInterruptVector(IN ULONG BusNumber,
                             IN ULONG BusInterruptLevel,
                             IN ULONG BusInterruptVector,
                             OUT PKIRQL Irql,
                             OUT PKAFFINITY Affinity)
{
    ULONG Vector = IRQ2VECTOR(BusInterruptLevel);
    *Irql = (KIRQL)VECTOR2IRQL(Vector);
    *Affinity = 0xFFFFFFFF;
    return Vector;
}

BOOLEAN
NTAPI
HalpFindBusAddressTranslation(IN PHYSICAL_ADDRESS BusAddress,
                              IN OUT PULONG AddressSpace,
                              OUT PPHYSICAL_ADDRESS TranslatedAddress,
                              IN OUT PULONG_PTR Context,
                              IN BOOLEAN NextBus)
{
    /* Make sure we have a context */
    if (!Context) return FALSE;

    /* If we have data in the context, then this shouldn't be a new lookup */
    if ((*Context) && (NextBus == TRUE)) return FALSE;

    /* Return bus data */
    TranslatedAddress->QuadPart = BusAddress.QuadPart;

    /* Set context value and return success */
    *Context = 1;
    return TRUE;
}

VOID
NTAPI
HalpInitNonBusHandler(VOID)
{
    /* These should be written by the PCI driver later, but we give defaults */
    HalPciTranslateBusAddress = HalpTranslateBusAddress;
    HalPciAssignSlotResources = HalpAssignSlotResources;
    HalFindBusAddressTranslation = HalpFindBusAddressTranslation;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
HalAdjustResourceList(IN PCM_RESOURCE_LIST Resources)
{
    /* Deprecated, return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
HalAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                       IN PUNICODE_STRING DriverClassName,
                       IN PDRIVER_OBJECT DriverObject,
                       IN PDEVICE_OBJECT DeviceObject,
                       IN INTERFACE_TYPE BusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    /* Check the bus type */
    if (BusType != PCIBus)
    {
        /* Call our internal handler */
        return HalpAssignSlotResources(RegistryPath,
                                       DriverClassName,
                                       DriverObject,
                                       DeviceObject,
                                       BusType,
                                       BusNumber,
                                       SlotNumber,
                                       AllocatedResources);
    }
    else
    {
        /* Call the PCI registered function */
        return HalPciAssignSlotResources(RegistryPath,
                                         DriverClassName,
                                         DriverObject,
                                         DeviceObject,
                                         PCIBus,
                                         BusNumber,
                                         SlotNumber,
                                         AllocatedResources);
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetBusData(IN BUS_DATA_TYPE BusDataType,
              IN ULONG BusNumber,
              IN ULONG SlotNumber,
              IN PVOID Buffer,
              IN ULONG Length)
{
    /* Call the extended function */
    return HalGetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    BUS_HANDLER BusHandler;

    /* Look as the bus type */
    if (BusDataType == Cmos)
    {
        /* Call CMOS Function */
        return HalpGetCmosData(0, SlotNumber, Buffer, Length);
    }
    else if (BusDataType == EisaConfiguration)
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }
    else if ((BusDataType == PCIConfiguration) &&
             (HalpPCIConfigInitialized) &&
             ((BusNumber >= HalpMinPciBus) && (BusNumber <= HalpMaxPciBus)))
    {
        /* Setup fake PCI Bus handler */
        RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        BusHandler.BusNumber = BusNumber;

        /* Call PCI function */
        return HalpGetPCIData(&BusHandler,
                              &BusHandler,
                              *(PPCI_SLOT_NUMBER)&SlotNumber,
                              Buffer,
                              Offset,
                              Length);
    }

    /* Invalid bus */
    return 0;
}

/*
 * @implemented
 */
ULONG
NTAPI
HalGetInterruptVector(IN INTERFACE_TYPE InterfaceType,
                      IN ULONG BusNumber,
                      IN ULONG BusInterruptLevel,
                      IN ULONG BusInterruptVector,
                      OUT PKIRQL Irql,
                      OUT PKAFFINITY Affinity)
{
    /* Call the system bus translator */
    return HalpGetSystemInterruptVector(BusNumber,
                                        BusInterruptLevel,
                                        BusInterruptVector,
                                        Irql,
                                        Affinity);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetBusData(IN BUS_DATA_TYPE BusDataType,
              IN ULONG BusNumber,
              IN ULONG SlotNumber,
              IN PVOID Buffer,
              IN ULONG Length)
{
    /* Call the extended function */
    return HalSetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    BUS_HANDLER BusHandler;

    /* Look as the bus type */
    if (BusDataType == Cmos)
    {
        /* Call CMOS Function */
        return HalpSetCmosData(0, SlotNumber, Buffer, Length);
    }
    else if ((BusDataType == PCIConfiguration) && (HalpPCIConfigInitialized))
    {
        /* Setup fake PCI Bus handler */
        RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        BusHandler.BusNumber = BusNumber;

        /* Call PCI function */
        return HalpSetPCIData(&BusHandler,
                              &BusHandler,
                              *(PPCI_SLOT_NUMBER)&SlotNumber,
                              Buffer,
                              Offset,
                              Length);
    }

    /* Invalid bus */
    return 0;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                       IN ULONG BusNumber,
                       IN PHYSICAL_ADDRESS BusAddress,
                       IN OUT PULONG AddressSpace,
                       OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    /* Look as the bus type */
    if (InterfaceType == PCIBus)
    {
        /* Call the PCI registered function */
        return HalPciTranslateBusAddress(PCIBus,
                                         BusNumber,
                                         BusAddress,
                                         AddressSpace,
                                         TranslatedAddress);
    }
    else
    {
        /* Translation is easy */
        TranslatedAddress->QuadPart = BusAddress.QuadPart;
        return TRUE;
    }
}

/* EOF */
