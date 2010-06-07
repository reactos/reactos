/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/legacy/bussupp.c
 * PURPOSE:         HAL Legacy Bus Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

PBUS_HANDLER
NTAPI
HalpAllocateBusHandler(IN INTERFACE_TYPE InterfaceType,
                       IN BUS_DATA_TYPE BusDataType,
                       IN ULONG BusNumber,
                       IN INTERFACE_TYPE ParentBusInterfaceType,
                       IN ULONG ParentBusNumber,
                       IN ULONG BusSpecificData)
{
    PBUS_HANDLER Bus;
    
    /* Register the bus handler */
    HalRegisterBusHandler(InterfaceType,
                          BusDataType,
                          BusNumber,
                          ParentBusInterfaceType,
                          ParentBusNumber,
                          BusSpecificData,
                          NULL,
                          &Bus);
    if (!Bus) return NULL;
    
    /* Check for a valid interface */
    if (InterfaceType != InterfaceTypeUndefined)
    {
        /* Allocate address ranges and zero them out */
        Bus->BusAddresses = ExAllocatePoolWithTag(NonPagedPool,
                                                  sizeof(SUPPORTED_RANGES),
                                                  ' laH');
        RtlZeroMemory(Bus->BusAddresses, sizeof(SUPPORTED_RANGES));
        
        /* Build the data structure */
        Bus->BusAddresses->Version = HAL_SUPPORTED_RANGE_VERSION;
        Bus->BusAddresses->Dma.Limit = 7;
        Bus->BusAddresses->Memory.Limit = 0xFFFFFFFF;
        Bus->BusAddresses->IO.Limit = 0xFFFF;
        Bus->BusAddresses->IO.SystemAddressSpace = 1;
        Bus->BusAddresses->PrefetchMemory.Base = 1;
    }
    
    /* Return the bus address */
    return Bus;
}

VOID
NTAPI
HalpRegisterInternalBusHandlers(VOID)
{
    PBUS_HANDLER Bus;
    
    /* Only do processor 1 */
    if (KeGetCurrentPrcb()->Number) return;
    
    /* Register root support */
    HalpInitBusHandler();
    
    /* Allocate the system bus */
    Bus = HalpAllocateBusHandler(Internal,
                                 ConfigurationSpaceUndefined,
                                 0,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    DPRINT1("Registering Internal Bus: %p\n", Bus);
    if (Bus)
    {
        /* Set it up */
        Bus->GetInterruptVector = HalpGetSystemInterruptVector;
        Bus->TranslateBusAddress = HalpTranslateSystemBusAddress;        
    }
    
    /* Allocate the CMOS bus */
    Bus = HalpAllocateBusHandler(InterfaceTypeUndefined,
                                 Cmos,
                                 0,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    DPRINT1("Registering CMOS Bus: %p\n", Bus);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpcGetCmosData;
        Bus->SetBusData = HalpcSetCmosData;
    }
    
    /* Allocate the CMOS bus */
    Bus = HalpAllocateBusHandler(InterfaceTypeUndefined,
                                 Cmos,
                                 1,
                                 InterfaceTypeUndefined,
                                 0,
                                 0);
    DPRINT1("Registering CMOS Bus: %p\n", Bus);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpcGetCmosData;
        Bus->SetBusData = HalpcSetCmosData;
    }

    /* Allocate ISA bus */
    Bus = HalpAllocateBusHandler(Isa,
                                 ConfigurationSpaceUndefined,
                                 0,
                                 Internal,
                                 0,
                                 0);
    DPRINT1("Registering ISA Bus: %p\n", Bus);
    if (Bus)
    {
        /* Set it up */
        Bus->GetBusData = HalpNoBusData;
        Bus->BusAddresses->Memory.Limit = 0xFFFFFF;
        Bus->TranslateBusAddress = HalpTranslateIsaBusAddress;
    }

    /* No support for EISA or MCA */
    ASSERT(HalpBusType == MACHINE_TYPE_ISA);
}

VOID
NTAPI
HalpInitializePciBus(VOID)
{
    /* FIXME: Should do legacy PCI bus detection */
    
    /* FIXME: Should detect chipset hacks */
    
    /* FIXME: Should detect broken PCI hardware and apply hacks */
    
    /* FIXME: Should build resource ranges */
}

VOID
NTAPI
HalpInitBusHandlers(VOID)
{
    /* Register the HAL Bus Handler support */
    HalpRegisterInternalBusHandlers();
}

VOID
NTAPI
HalpRegisterKdSupportFunctions(VOID)
{
    /* Register PCI Device Functions */
    KdSetupPciDeviceForDebugging = HalpSetupPciDeviceForDebugging;
    KdReleasePciDeviceforDebugging = HalpReleasePciDeviceForDebugging;

    /* Register memory functions */
#ifndef _MINIHAL_
    KdMapPhysicalMemory64 = HalpMapPhysicalMemory64;
    KdUnmapVirtualAddress = HalpUnmapVirtualAddress;
#endif

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
HalpGetSystemInterruptVector_Acpi(IN ULONG BusNumber,
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
    return HalpGetSystemInterruptVector_Acpi(BusNumber,
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
