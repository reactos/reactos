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

extern KSPIN_LOCK HalpPCIConfigLock;
ULONG HalpPciIrqMask;

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

#ifndef _MINIHAL_
NTSTATUS
NTAPI
HalpMarkChipsetDecode(BOOLEAN OverrideEnable)
{
    NTSTATUS Status;
    UNICODE_STRING KeyString;
    ULONG Data = OverrideEnable;
    HANDLE KeyHandle, Handle;
    
    /* Open CCS key */
    RtlInitUnicodeString(&KeyString,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");
    Status = HalpOpenRegistryKey(&Handle, 0, &KeyString, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Open PNP Bios key */
        RtlInitUnicodeString(&KeyString, L"Control\\Biosinfo\\PNPBios");
        Status = HalpOpenRegistryKey(&KeyHandle,
                                     Handle,
                                     &KeyString,
                                     KEY_ALL_ACCESS,
                                     TRUE);
        
        /* Close root key */
        ZwClose(Handle);
        
        /* Check if PNP BIOS key exists */
        if (NT_SUCCESS(Status))
        {
            /* Set the override value */
            RtlInitUnicodeString(&KeyString, L"FullDecodeChipsetOverride");           
            Status = ZwSetValueKey(KeyHandle,
                                   &KeyString,
                                   0,
                                   REG_DWORD,
                                   &Data,
                                   sizeof(Data));
            
            /* Close subkey */
            ZwClose(KeyHandle);
        }
    }
    
    /* Return status */
    return Status;
}

PBUS_HANDLER
NTAPI
HalpAllocateAndInitPciBusHandler(IN ULONG PciType,
                                 IN ULONG BusNo,
                                 IN BOOLEAN TestAllocation)
{
    PBUS_HANDLER Bus;
    PPCIPBUSDATA BusData;
    
    /* Allocate the bus handler */
    Bus = HalpAllocateBusHandler(PCIBus,
                                 PCIConfiguration,
                                 BusNo,
                                 Internal,
                                 0,
                                 sizeof(PCIPBUSDATA));
    
    /* Set it up */
    Bus->GetBusData = (PGETSETBUSDATA)HalpGetPCIData;
    Bus->SetBusData = (PGETSETBUSDATA)HalpSetPCIData;
    Bus->GetInterruptVector = (PGETINTERRUPTVECTOR)HalpGetPCIIntOnISABus;
    Bus->AdjustResourceList = (PADJUSTRESOURCELIST)HalpAdjustPCIResourceList;
    Bus->AssignSlotResources = (PASSIGNSLOTRESOURCES)HalpAssignPCISlotResources;
    Bus->BusAddresses->Dma.Limit = 0;
    
    /* Get our custom bus data */
    BusData = (PPCIPBUSDATA)Bus->BusData;
    
    /* Setup custom bus data */
    BusData->CommonData.Tag = PCI_DATA_TAG;
    BusData->CommonData.Version = PCI_DATA_VERSION;
    BusData->CommonData.ReadConfig = (PciReadWriteConfig)HalpReadPCIConfig;
    BusData->CommonData.WriteConfig = (PciReadWriteConfig)HalpWritePCIConfig;
    BusData->CommonData.Pin2Line = (PciPin2Line)HalpPCIPin2ISALine;
    BusData->CommonData.Line2Pin = (PciLine2Pin)HalpPCIISALine2Pin;
    BusData->MaxDevice = PCI_MAX_DEVICES;
    BusData->GetIrqRange = (PciIrqRange)HalpGetISAFixedPCIIrq;
    
    /* Initialize the bitmap */
    RtlInitializeBitMap(&BusData->DeviceConfigured, BusData->ConfiguredBits, 256);
    
    /* Check the type of PCI bus */
    switch (PciType)
    {
        /* Type 1 PCI Bus */
        case 1:
            
            /* Copy the Type 1 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType1,
                          sizeof(PCIConfigHandler));
            
            /* Set correct I/O Ports */
            BusData->Config.Type1.Address = PCI_TYPE1_ADDRESS_PORT;
            BusData->Config.Type1.Data = PCI_TYPE1_DATA_PORT;
            break;
            
        /* Type 2 PCI Bus */
        case 2:
            
            /* Copy the Type 1 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType2,
                          sizeof (PCIConfigHandler));
            
            /* Set correct I/O Ports */
            BusData->Config.Type2.CSE = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base = PCI_TYPE2_ADDRESS_BASE;
            
            /* Only 16 devices supported, not 32 */
            BusData->MaxDevice = 16;
            break;
            
        default:
            
            /* Invalid type */
            DbgPrint("HAL: Unnkown PCI type\n");
    }

    /* Return the bus handler */
    return Bus;
}

BOOLEAN
NTAPI
HalpIsValidPCIDevice(IN PBUS_HANDLER BusHandler,
                     IN PCI_SLOT_NUMBER Slot)
{
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciHeader = (PVOID)DataBuffer;
    ULONG i;
    ULONG_PTR Address;
    
    /* Read the PCI header */
    HalpReadPCIConfig(BusHandler, Slot, PciHeader, 0, PCI_COMMON_HDR_LENGTH);
    
    /* Make sure it's a valid device */
    if ((PciHeader->VendorID == PCI_INVALID_VENDORID) ||
        (PCI_CONFIGURATION_TYPE(PciHeader) != PCI_DEVICE_TYPE))
    {
        /* Bail out */
        return FALSE;
    }
    
    /* Make sure interrupt numbers make sense */
    if (((PciHeader->u.type0.InterruptPin) &&
         (PciHeader->u.type0.InterruptPin > 4)) ||
        (PciHeader->u.type0.InterruptLine & 0x70))
    {
        /* Bail out */
        return FALSE;
    }
    
    /* Now scan PCI BARs */
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
        /* Check what kind of address it is */
        Address = PciHeader->u.type0.BaseAddresses[i];
        if (Address & PCI_ADDRESS_IO_SPACE)
        {
            /* Highest I/O port is 65535 */
            if (Address > 0xFFFF) return FALSE;
        }
        else
        {
            /* MMIO should be higher than 0x80000 */
            if ((Address > 0xF) && (Address < 0x80000)) return FALSE;
        }
        
        /* Is this a 64-bit address? */
        if (!(Address & PCI_ADDRESS_IO_SPACE) &&
            ((Address & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT))
        {
            /* Check the next-next entry, since this one 64-bits wide */
            i++;
        }
    }

    /* Header, interrupt and address data all make sense */
    return TRUE;
}

static BOOLEAN WarningsGiven[5];

NTSTATUS
NTAPI
HalpGetChipHacks(IN USHORT VendorId,
                 IN USHORT DeviceId,
                 IN UCHAR RevisionId,
                 IN PULONG HackFlags)
{
    /* Not yet implemented */
    if (!WarningsGiven[0]++) DbgPrint("HAL: Not checking for PCI Chipset Hacks. Your hardware may malfunction!\n");
    *HackFlags = 0;
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
HalpIsRecognizedCard(IN PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo,
                     IN PPCI_COMMON_CONFIG PciData,
                     IN ULONG Flags)
{
    /* Not yet implemented */
    if (!WarningsGiven[1]++) DbgPrint("HAL: Not checking for PCI Cards with Extended Addressing. Your hardware may malfunction!\n");
    return FALSE;
}

BOOLEAN
NTAPI
HalpIsIdeDevice(IN PPCI_COMMON_CONFIG PciData)
{
    /* Not yet implemented */
    if (!WarningsGiven[2]++) DbgPrint("HAL: Not checking for PCI Cards that are IDE devices. Your hardware may malfunction!\n");
    return FALSE;  
}

BOOLEAN
NTAPI
HalpGetPciBridgeConfig(IN ULONG PciType,
                       IN PUCHAR MaxPciBus)
{
    /* Not yet implemented */
    if (!WarningsGiven[3]++)  DbgPrint("HAL: Not checking for PCI-to-PCI Bridges. Your hardware may malfunction!\n");
    return FALSE; 
}

VOID
NTAPI
HalpFixupPciSupportedRanges(IN ULONG MaxBuses)
{
    /* Not yet implemented */
    if (!WarningsGiven[4]++) DbgPrint("HAL: Not adjusting Bridge-to-Child PCI Address Ranges. Your hardware may malfunction!\n");  
}
#endif

VOID
NTAPI
HalpInitializePciBus(VOID)
{
#ifndef _MINIHAL_
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;
    UCHAR PciType;
    PCI_SLOT_NUMBER PciSlot;
    ULONG i, j, k;
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciData = (PPCI_COMMON_CONFIG)DataBuffer;
    PBUS_HANDLER BusHandler;
    ULONG HackFlags;
    BOOLEAN ExtendedAddressDecoding = FALSE;
    
    /* Query registry information */
    PciRegistryInfo = HalpQueryPciRegistryInfo();
    if (!PciRegistryInfo) return;
    
    /* Initialize the PCI configuration lock */
    KeInitializeSpinLock(&HalpPCIConfigLock);
    
    /* Get the type and free the info structure */
    PciType = PciRegistryInfo->HardwareMechanism & 0xF;
    
    /* Check if this is a type 2 PCI bus with at least one bus */
    if ((PciRegistryInfo->NoBuses) && (PciType == 2))
    {
        /* Setup the PCI slot */
        PciSlot.u.bits.Reserved = 0;
        PciSlot.u.bits.FunctionNumber = 0;
        
        /* Loop all slots */
        for (i = 0; i < 32; i++)
        {
            /* Try to setup a Type 2 PCI slot */
            PciType = 2;
            BusHandler = HalpAllocateAndInitPciBusHandler(2, 0, TRUE);
            if (!BusHandler) break;
            
            /* Now check if it's valid */
            if (HalpIsValidPCIDevice(BusHandler, PciSlot)) break;
            
            /* Heh, the BIOS lied... try Type 1 */
            PciType = 1;
            BusHandler = HalpAllocateAndInitPciBusHandler(1, 0, TRUE);
            if (!BusHandler) break;
            
            /* Now check if it's valid */
            if (HalpIsValidPCIDevice(BusHandler, PciSlot)) break;
            
            /* Keep trying */
            PciType = 2;
        }
        
        /* Now allocate the correct kind of handler */
        HalpAllocateAndInitPciBusHandler(PciType, 0, FALSE);
    }
    
    /* Okay, now loop all PCI bridges */
    do
    {
        /* Loop all PCI buses */
        for (i = 0; i < PciRegistryInfo->NoBuses; i++)
        {
            /* Check if we have a handler for it */
            if (!HalHandlerForBus(PCIBus, i))
            {
                /* Allocate it */
                HalpAllocateAndInitPciBusHandler(PciType, i, FALSE);
            }
        }
        /* Go to the next bridge */
    } while (HalpGetPciBridgeConfig(PciType, &PciRegistryInfo->NoBuses));
             
    /* Now build correct address range informaiton */
    HalpFixupPciSupportedRanges(PciRegistryInfo->NoBuses);
    
    /* Loop every bus */
    PciSlot.u.bits.Reserved = 0;
    for (i = 0; i < PciRegistryInfo->NoBuses; i++)
    {
        /* Get the bus handler */
        BusHandler = HalHandlerForBus(PCIBus, i);
        
        /* Loop every device */
        for (j = 0; j < 32; j++)
        {
            /* Loop every function */
            PciSlot.u.bits.DeviceNumber = j;
            for (k = 0; k < 8; k++)
            {
                /* Build the final slot structure */
                PciSlot.u.bits.FunctionNumber = k;
                
                /* Read the configuration information */
                HalpReadPCIConfig(BusHandler,
                                  PciSlot,
                                  PciData,
                                  0,
                                  PCI_COMMON_HDR_LENGTH);
                
                /* Skip if this is an invalid function */
                if (PciData->VendorID == PCI_INVALID_VENDORID) continue;
                
                /* Check if this is a Cardbus bridge */
                if (PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE)
                {
                    /* Not supported */
                    DbgPrint("HAL: Your machine has a PCI Cardbus Bridge. This is not supported!\n");
                    continue;
                }
                
                /* Check if this is a PCI device */
                if (PCI_CONFIGURATION_TYPE(PciData) != PCI_BRIDGE_TYPE)
                {
                    /* Check if it has an interrupt pin and line registered */
                    if ((PciData->u.type1.InterruptPin) && 
                        (PciData->u.type1.InterruptLine))
                    {
                        /* Check if this interrupt line is connected to the bus */
                        if (PciData->u.type1.InterruptLine < 16)
                        {
                            /* Is this an IDE device? */
                            DbgPrint("HAL: Found PCI device with IRQ line\n");
                            if (!HalpIsIdeDevice(PciData))
                            {
                                /* We'll mask out this interrupt then */
                                DbgPrint("HAL: Device is not an IDE Device. Should be masking IRQ %d! This is not supported!\n",
                                         PciData->u.type1.InterruptLine);
                                HalpPciIrqMask |= (1 << PciData->u.type1.InterruptLine);
                            }
                        }
                    }
                }
                
                /* Check for broken Intel chips */
                if (PciData->VendorID == 0x8086)
                {
                    /* Check for broken 82830 PCI controller */
                    if ((PciData->DeviceID == 0x04A3) &&
                        (PciData->RevisionID < 0x11))
                    {
                        /* Skip */
                        DbgPrint("HAL: Your machine has a broken Intel 82430 PCI Controller. This is not supported!\n");
                        continue;
                    }
                    
                    /* Check for broken 82378 PCI-to-ISA Bridge */
                    if ((PciData->DeviceID == 0x0484) &&
                        (PciData->RevisionID <= 3))
                    {
                        /* Skip */
                        DbgPrint("HAL: Your machine has a broken Intel 82378 PCI-to-ISA Bridge. This is not supported!\n");
                        continue;
                    }
                    
                    /* Check for broken 82450 PCI Bridge */
                    if ((PciData->DeviceID == 0x84C4) &&
                        (PciData->RevisionID <= 4))
                    {
                        DbgPrint("HAL: Your machine has an Intel Orion 82450 PCI Bridge. This is not supported!\n");
                        continue;
                    }
                }
                
                /* Do we know this card? */
                if (!ExtendedAddressDecoding)
                {
                    /* Check for it */
                    if (HalpIsRecognizedCard(PciRegistryInfo, PciData, 1))
                    {
                        /* We'll do chipset checks later */
                        DbgPrint("HAL: Recognized a PCI Card with Extended Address Decoding. This is not supported!\n");
                        ExtendedAddressDecoding = TRUE;
                    }
                }
                
                /* Check if this is a USB controller */
                if ((PciData->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                    (PciData->SubClass == PCI_SUBCLASS_SB_USB))
                {
                    /* Check if this is an OHCI controller */
                    if (PciData->ProgIf == 0x10)
                    {
                        DbgPrint("HAL: Your machine has an OHCI (USB) PCI Expansion Card. This is not supported!\n");
                        continue;
                    }
                    
                    /* Check for Intel UHCI controller */
                    if (PciData->VendorID == 0x8086)
                    {
                        DbgPrint("HAL: Your machine has an Intel UHCI (USB) Controller. This is not supported!\n");
                        continue;
                    }
                    
                    /* Check for VIA UHCI controller */
                    if (PciData->VendorID == 0x1106)
                    {
                        DbgPrint("HAL: Your machine has a VIA UHCI (USB) Controller. This is not supported!\n");
                        continue;
                    }
                }
                
                /* Now check the registry for chipset hacks */
                if (NT_SUCCESS(HalpGetChipHacks(PciData->VendorID,
                                                PciData->DeviceID,
                                                PciData->RevisionID,
                                                &HackFlags)))
                {
                    /* Check for hibernate-disable */
                    if (HackFlags & HAL_PCI_CHIP_HACK_DISABLE_HIBERNATE)
                    {
                        DbgPrint("HAL: Your machine has a broken PCI device which is incompatible with hibernation. This is not supported!\n");
                        continue;
                    }
                    
                    /* Check for USB controllers that generate SMIs */
                    if (HackFlags & HAL_PCI_CHIP_HACK_USB_SMI_DISABLE)
                    {
                        DbgPrint("HAL: Your machine has a USB controller which generates SMIs. This is not supported!\n");
                        continue;
                    }
                }
            }
        }
    }
    
    /* Initialize NMI Crash Flag */
    HalpGetNMICrashFlag();
        
    /* Free the registry data */
    ExFreePool(PciRegistryInfo);
    
    /* Tell PnP if this hard supports correct decoding */
    HalpMarkChipsetDecode(ExtendedAddressDecoding);
    DPRINT1("PCI BUS Setup complete\n");
#endif
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
