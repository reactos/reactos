/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/enum.c
 * PURPOSE:         PCI Bus/Device Enumeration
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PcipIsSameDevice(IN PPCI_PDO_EXTENSION DeviceExtension,
                 IN PPCI_COMMON_HEADER PciData)
{
    BOOLEAN IdMatch, RevMatch, SubsysMatch;
    ULONGLONG HackFlags = DeviceExtension->HackFlags;

    /* Check if the IDs match */
    IdMatch = (PciData->VendorID == DeviceExtension->VendorId) &&
              (PciData->DeviceID == DeviceExtension->DeviceId);
    if (!IdMatch) return FALSE;

    /* If the device has a valid revision, check if it matches */
    RevMatch = (HackFlags & PCI_HACK_NO_REVISION_AFTER_D3) ||
               (PciData->RevisionID == DeviceExtension->RevisionId);
    if (!RevMatch) return FALSE;

    /* For multifunction devices, this is enough to assume they're the same */
    if (PCI_MULTIFUNCTION_DEVICE(PciData)) return TRUE;

    /* For bridge devices, there's also nothing else that can be checked */
    if (DeviceExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) return TRUE;

    /* Devices, on the other hand, have subsystem data that can be compared */
    SubsysMatch = (HackFlags & (PCI_HACK_NO_SUBSYSTEM |
                                PCI_HACK_NO_SUBSYSTEM_AFTER_D3)) ||
                  ((DeviceExtension->SubsystemVendorId ==
                    PciData->u.type0.SubVendorID) &&
                   (DeviceExtension->SubsystemId ==
                    PciData->u.type0.SubSystemID));
    return SubsysMatch;
}

BOOLEAN
NTAPI
PciSkipThisFunction(IN PPCI_COMMON_HEADER PciData,
                    IN PCI_SLOT_NUMBER Slot,
                    IN UCHAR OperationType,
                    IN ULONGLONG HackFlags)
{
    do
    {
        /* Check if this is device enumeration */
        if (OperationType == PCI_SKIP_DEVICE_ENUMERATION)
        {
            /* Check if there's a hackflag saying not to enumerate this device */
            if (HackFlags & PCI_HACK_NO_ENUM_AT_ALL) break;

            /* Check if this is the high end of a double decker device */
            if ((HackFlags & PCI_HACK_DOUBLE_DECKER) &&
                (Slot.u.bits.DeviceNumber >= 16))
            {
                /* It belongs to the same device, so skip it */
                DPRINT1("    Device (Ven %04x Dev %04x (d=0x%x, f=0x%x)) is a ghost.\n",
                        PciData->VendorID,
                        PciData->DeviceID,
                        Slot.u.bits.DeviceNumber,
                        Slot.u.bits.FunctionNumber);
                break;
            }
        }
        else if (OperationType == PCI_SKIP_RESOURCE_ENUMERATION)
        {
            /* Resource enumeration, check for a hackflag saying not to do it */
            if (HackFlags & PCI_HACK_ENUM_NO_RESOURCE) break;
        }
        else
        {
            /* Logic error in the driver */
            ASSERTMSG(FALSE, "PCI Skip Function - Operation type unknown.");
        }

        /* Check for legacy bridges during resource enumeration */
        if ((PciData->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
            (PciData->SubClass <= PCI_SUBCLASS_BR_MCA) &&
            (OperationType == PCI_SKIP_RESOURCE_ENUMERATION))
        {
            /* Their resources are not enumerated, only PCI and Cardbus/PCMCIA */
            break;
        }
        else if (PciData->BaseClass == PCI_CLASS_NOT_DEFINED)
        {
            /* Undefined base class (usually a PCI BIOS/ROM bug) */
            DPRINT1("    Vendor %04x, Device %04x has class code of PCI_CLASS_NOT_DEFINED\n",
                    PciData->VendorID,
                    PciData->DeviceID);

            /*
             * The Alder has an Intel Extended Express System Support Controller
             * which presents apparently spurious BARs. When the PCI resource
             * code tries to reassign these BARs, the second IO-APIC gets
             * disabled (with disastrous consequences). The first BAR is the
             * actual IO-APIC, the remaining five bars seem to be spurious
             * resources, so ignore this device completely.
             */
            if ((PciData->VendorID == 0x8086) && (PciData->DeviceID == 8)) break;
        }

        /* Other normal PCI cards and bridges are enumerated */
        if (PCI_CONFIGURATION_TYPE(PciData) <= PCI_CARDBUS_BRIDGE_TYPE) return FALSE;
    } while (FALSE);

    /* Hit one of the known bugs/hackflags, or this is a new kind of PCI unit */
    DPRINT1("   Device skipped (not enumerated).\n");
    return TRUE;
}

VOID
NTAPI
PciGetEnhancedCapabilities(IN PPCI_PDO_EXTENSION PdoExtension,
                           IN PPCI_COMMON_HEADER PciData)
{
    ULONG HeaderType, CapPtr, TargetAgpCapabilityId;
    DEVICE_POWER_STATE WakeLevel;
    PCI_CAPABILITIES_HEADER AgpCapability;
    PCI_PM_CAPABILITY PowerCapabilities;
    PAGED_CODE();

    /* Assume no known wake level */
    PdoExtension->PowerState.DeviceWakeLevel = PowerDeviceUnspecified;

    /* Make sure the device has capabilities */
    if (!(PciData->Status & PCI_STATUS_CAPABILITIES_LIST))
    {
        /* If it doesn't, there will be no power management */
        PdoExtension->CapabilitiesPtr = 0;
        PdoExtension->HackFlags |= PCI_HACK_NO_PM_CAPS;
    }
    else
    {
        /* There's capabilities, need to figure out where to get the offset */
        HeaderType = PCI_CONFIGURATION_TYPE(PciData);
        if (HeaderType == PCI_CARDBUS_BRIDGE_TYPE)
        {
            /* Use the bridge's header */
            CapPtr = PciData->u.type2.CapabilitiesPtr;
        }
        else
        {
            /* Use the device header */
            ASSERT(HeaderType <= PCI_CARDBUS_BRIDGE_TYPE);
            CapPtr = PciData->u.type0.CapabilitiesPtr;
        }

        /* Make sure the pointer is spec-aligned and located, and save it */
        DPRINT1("Device has capabilities at: %lx\n", CapPtr);
        ASSERT(((CapPtr & 0x3) == 0) && (CapPtr >= PCI_COMMON_HDR_LENGTH));
        PdoExtension->CapabilitiesPtr = CapPtr;

        /* Check for PCI-to-PCI Bridges and AGP bridges */
        if ((PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
            ((PdoExtension->SubClass == PCI_SUBCLASS_BR_HOST) ||
             (PdoExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI)))
        {
            /* Query either the raw AGP capabilitity, or the Target AGP one */
            TargetAgpCapabilityId = (PdoExtension->SubClass ==
                                     PCI_SUBCLASS_BR_PCI_TO_PCI) ?
                                     PCI_CAPABILITY_ID_AGP_TARGET :
                                     PCI_CAPABILITY_ID_AGP;
            if (PciReadDeviceCapability(PdoExtension,
                                        PdoExtension->CapabilitiesPtr,
                                        TargetAgpCapabilityId,
                                        &AgpCapability,
                                        sizeof(PCI_CAPABILITIES_HEADER)))
            {
                /* AGP target ID was found, store it */
                DPRINT1("AGP ID: %lx\n", TargetAgpCapabilityId);
                PdoExtension->TargetAgpCapabilityId = TargetAgpCapabilityId;
            }
        }

        /* Check for devices that are known not to have proper power management */
        if (!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS))
        {
            /* Query if this device supports power management */
            if (!PciReadDeviceCapability(PdoExtension,
                                         PdoExtension->CapabilitiesPtr,
                                         PCI_CAPABILITY_ID_POWER_MANAGEMENT,
                                         &PowerCapabilities.Header,
                                         sizeof(PCI_PM_CAPABILITY)))
            {
                /* No power management, so act as if it had the hackflag set */
                DPRINT1("No PM caps, disabling PM\n");
                PdoExtension->HackFlags |= PCI_HACK_NO_PM_CAPS;
            }
            else
            {
                /* Otherwise, pick the highest wake level that is supported */
                WakeLevel = PowerDeviceUnspecified;
                if (PowerCapabilities.PMC.Capabilities.Support.PMED0)
                    WakeLevel = PowerDeviceD0;
                if (PowerCapabilities.PMC.Capabilities.Support.PMED1)
                    WakeLevel = PowerDeviceD1;
                if (PowerCapabilities.PMC.Capabilities.Support.PMED2)
                    WakeLevel = PowerDeviceD2;
                if (PowerCapabilities.PMC.Capabilities.Support.PMED3Hot)
                    WakeLevel = PowerDeviceD3;
                if (PowerCapabilities.PMC.Capabilities.Support.PMED3Cold)
                    WakeLevel = PowerDeviceD3;
                PdoExtension->PowerState.DeviceWakeLevel = WakeLevel;

                /* Convert the PCI power state to the NT power state */
                PdoExtension->PowerState.CurrentDeviceState =
                    PowerCapabilities.PMCSR.ControlStatus.PowerState + 1;

                /* Save all the power capabilities */
                PdoExtension->PowerCapabilities = PowerCapabilities.PMC.Capabilities;
                DPRINT1("PM Caps Found! Wake Level: %d Power State: %d\n",
                        WakeLevel, PdoExtension->PowerState.CurrentDeviceState);
            }
        }
    }

    /* At the very end of all this, does this device not have power management? */
    if (PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS)
    {
        /* Then guess the current state based on whether the decodes are on */
        PdoExtension->PowerState.CurrentDeviceState =
            PciData->Command & (PCI_ENABLE_IO_SPACE |
                                PCI_ENABLE_MEMORY_SPACE |
                                PCI_ENABLE_BUS_MASTER) ?
            PowerDeviceD0: PowerDeviceD3;
        DPRINT1("PM is off, so assumed device is: %d based on enables\n",
                PdoExtension->PowerState.CurrentDeviceState);
    }
}

NTSTATUS
NTAPI
PciScanBus(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    ULONG MaxDevice = PCI_MAX_DEVICES;
    BOOLEAN ProcessFlag = FALSE;
    ULONG i, j, k;
    LONGLONG HackFlags;
    PDEVICE_OBJECT DeviceObject;
    UCHAR Buffer[PCI_COMMON_HDR_LENGTH];
    UCHAR BiosBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_HEADER PciData = (PVOID)Buffer;
    PPCI_COMMON_HEADER BiosData = (PVOID)BiosBuffer;
    PCI_SLOT_NUMBER PciSlot;
    NTSTATUS Status;
    PPCI_PDO_EXTENSION PdoExtension, NewExtension;
    PPCI_PDO_EXTENSION* BridgeExtension;
    PWCHAR DescriptionText;
    USHORT SubVendorId, SubSystemId;
    DPRINT1("PCI Scan Bus: FDO Extension @ 0x%x, Base Bus = 0x%x\n",
            DeviceExtension, DeviceExtension->BaseBus);

    /* Is this the root FDO? */
    if (!PCI_IS_ROOT_FDO(DeviceExtension))
    {
        /* Other FDOs are not currently supported */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Loop every device on the bus */
    PciSlot.u.bits.Reserved = 0;
    i = DeviceExtension->BaseBus;
    for (j = 0; j < MaxDevice; j++)
    {
        /* Loop every function of each device */
        PciSlot.u.bits.DeviceNumber = j;
        for (k = 0; k < PCI_MAX_FUNCTION; k++)
        {
            /* Build the final slot structure */
            PciSlot.u.bits.FunctionNumber = k;

            /* Read the vendor for this slot */
            PciReadSlotConfig(DeviceExtension,
                              PciSlot,
                              PciData,
                              0,
                              sizeof(USHORT));

            /* Skip invalid device */
            if (PciData->VendorID == PCI_INVALID_VENDORID) continue;

            /* Now read the whole header */
            PciReadSlotConfig(DeviceExtension,
                              PciSlot,
                              &PciData->DeviceID,
                              sizeof(USHORT),
                              PCI_COMMON_HDR_LENGTH - sizeof(USHORT));

            /* Dump device that was found */
            DPRINT1("Scan Found Device 0x%x (b=0x%x, d=0x%x, f=0x%x)\n",
                    PciSlot.u.AsULONG,
                    i,
                    j,
                    k);

            /* Dump the device's header */
            PciDebugDumpCommonConfig(PciData);

            /* Find description for this device for the debugger's sake */
            DescriptionText = PciGetDeviceDescriptionMessage(PciData->BaseClass,
                                                             PciData->SubClass);
            DPRINT1("Device Description \"%S\".\n",
                    DescriptionText ? DescriptionText : L"(NULL)");
            if (DescriptionText) ExFreePoolWithTag(DescriptionText, 0);

            /* Check if there is an ACPI Watchdog Table */
            if (WdTable)
            {
                /* Check if this PCI device is the ACPI Watchdog Device... */
                UNIMPLEMENTED;
                while (TRUE);
            }

            /* Check for non-simple devices */
            if ((PCI_MULTIFUNCTION_DEVICE(PciData)) ||
                (PciData->BaseClass == PCI_CLASS_BRIDGE_DEV))
            {
                /* No subsystem data defined for these kinds of bridges */
                SubVendorId = 0;
                SubSystemId = 0;
            }
            else
            {
                /* Read the subsystem information from the PCI header */
                SubVendorId = PciData->u.type0.SubVendorID;
                SubSystemId = PciData->u.type0.SubSystemID;
            }

            /* Get any hack flags for this device */
            HackFlags = PciGetHackFlags(PciData->VendorID,
                                        PciData->DeviceID,
                                        SubVendorId,
                                        SubSystemId,
                                        PciData->RevisionID);

            /* Check if this device is considered critical by the OS */
            if (PciIsCriticalDeviceClass(PciData->BaseClass, PciData->SubClass))
            {
                /* Check if normally the decodes would be disabled */
                if (!(HackFlags & PCI_HACK_DONT_DISABLE_DECODES))
                {
                    /* Because this device is critical, don't disable them */
                    DPRINT1("Not allowing PM Because device is critical\n");
                    HackFlags |= PCI_HACK_CRITICAL_DEVICE;
                }
            }

            /* PCI bridges with a VGA card are also considered critical */
            if ((PciData->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                (PciData->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI) &&
                (PciData->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA) &&
               !(HackFlags & PCI_HACK_DONT_DISABLE_DECODES))
            {
                /* Do not disable their decodes either */
                DPRINT1("Not allowing PM because device is VGA\n");
                HackFlags |= PCI_HACK_CRITICAL_DEVICE;
            }

            /* Also skip devices that should not be enumerated */
            if (PciSkipThisFunction(PciData, PciSlot, 1, HackFlags)) continue;

            /* Check if a PDO has already been created for this device */
            PdoExtension = PciFindPdoByFunction(DeviceExtension,
                                                PciSlot.u.AsULONG,
                                                PciData);
            if (PdoExtension)
            {
                /* Rescan scenarios are not yet implemented */
                UNIMPLEMENTED;
                while (TRUE);
            }

            /* Bus processing will need to happen */
            ProcessFlag = TRUE;

            /* Create the PDO for this device */
            Status = PciPdoCreate(DeviceExtension, PciSlot, &DeviceObject);
            ASSERT(NT_SUCCESS(Status));
            NewExtension = (PPCI_PDO_EXTENSION)DeviceObject->DeviceExtension;

            /* Check for broken devices with wrong/no class codes */
            if (HackFlags & PCI_HACK_FAKE_CLASS_CODE)
            {
                /* Setup a default one */
                PciData->BaseClass = PCI_CLASS_BASE_SYSTEM_DEV;
                PciData->SubClass = PCI_SUBCLASS_SYS_OTHER;

                /* Device will behave erratically when reading back data */
                NewExtension->ExpectedWritebackFailure = TRUE;
            }

            /* Clone all the information from the header */
            NewExtension->VendorId = PciData->VendorID;
            NewExtension->DeviceId = PciData->DeviceID;
            NewExtension->RevisionId = PciData->RevisionID;
            NewExtension->ProgIf = PciData->ProgIf;
            NewExtension->SubClass = PciData->SubClass;
            NewExtension->BaseClass = PciData->BaseClass;
            NewExtension->HeaderType = PCI_CONFIGURATION_TYPE(PciData);

            /* Check for modern bridge types, which are managed by the driver */
            if ((NewExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                ((NewExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI) ||
                 (NewExtension->SubClass == PCI_SUBCLASS_BR_CARDBUS)))
            {
                /* Acquire this device's lock */
                KeEnterCriticalRegion();
                KeWaitForSingleObject(&DeviceExtension->ChildListLock,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                /* Scan the bridge list until the first free entry */
                for (BridgeExtension = &DeviceExtension->ChildBridgePdoList;
                     *BridgeExtension;
                     BridgeExtension = &(*BridgeExtension)->NextBridge);

                /* Add this PDO as a bridge */
                *BridgeExtension = NewExtension;
                ASSERT(NewExtension->NextBridge == NULL);

                /* Release this device's lock */
                KeSetEvent(&DeviceExtension->ChildListLock,
                           IO_NO_INCREMENT,
                           FALSE);
                KeLeaveCriticalRegion();
            }

            /* Get the PCI BIOS configuration saved in the registry */
            Status = PciGetBiosConfig(NewExtension, BiosData);
            if (NT_SUCCESS(Status))
            {
                /* This path has not yet been fully tested by eVb */
                DPRINT1("Have BIOS configuration!\n");
                UNIMPLEMENTED;

                /* Check if the PCI BIOS configuration has changed */
                if (!PcipIsSameDevice(NewExtension, BiosData))
                {
                    /* This is considered failure, and new data will be saved */
                    Status = STATUS_UNSUCCESSFUL;
                }
                else
                {
                    /* Data is still correct, check for interrupt line change */
                    if (BiosData->u.type0.InterruptLine !=
                        PciData->u.type0.InterruptLine)
                    {
                        /* Update the current BIOS with the saved interrupt line */
                        PciWriteDeviceConfig(NewExtension,
                                             &BiosData->u.type0.InterruptLine,
                                             FIELD_OFFSET(PCI_COMMON_HEADER,
                                                          u.type0.InterruptLine),
                                             sizeof(UCHAR));
                    }

                    /* Save the BIOS interrupt line and the initial command */
                    NewExtension->RawInterruptLine = BiosData->u.type0.InterruptLine;
                    NewExtension->InitialCommand = BiosData->Command;
                }
            }

            /* Check if no saved data was present or if it was a mismatch */
            if (!NT_SUCCESS(Status))
            {
                /* Save the new data */
                Status = PciSaveBiosConfig(NewExtension, PciData);
                ASSERT(NT_SUCCESS(Status));

                /* Save the interrupt line and command from the device */
                NewExtension->RawInterruptLine = PciData->u.type0.InterruptLine;
                NewExtension->InitialCommand = PciData->Command;
            }

            /* Save original command from the device and hack flags */
            NewExtension->CommandEnables = PciData->Command;
            NewExtension->HackFlags = HackFlags;

            /* Get power, AGP, and other capability data */
            PciGetEnhancedCapabilities(NewExtension, PciData);

            /* Power up the device */
            PciSetPowerManagedDevicePowerState(NewExtension, PowerDeviceD0, FALSE);

            /* Save interrupt pin */
            NewExtension->InterruptPin = PciData->u.type0.InterruptPin;

            /*
             * Use either this device's actual IRQ line or, if it's connected on
             * a master bus whose IRQ line is actually connected to the host, use
             * the HAL to query the bus' IRQ line and store that as the adjusted
             * interrupt line instead
             */
            NewExtension->AdjustedInterruptLine = PciGetAdjustedInterruptLine(NewExtension);

            /* Check if this device is used for PCI debugger cards */
            NewExtension->OnDebugPath = PciIsDeviceOnDebugPath(NewExtension);

            /* Check for devices with invalid/bogus subsystem data */
            if (HackFlags & PCI_HACK_NO_SUBSYSTEM)
            {
                /* Set the subsystem information to zero instead */
                NewExtension->SubsystemVendorId = 0;
                NewExtension->SubsystemId = 0;
            }

            /* Check for IDE controllers */
            if ((NewExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
                (NewExtension->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR))
            {
                /* Do not allow them to power down completely */
                NewExtension->DisablePowerDown = TRUE;
            }

            /*
             * Check if this is a legacy bridge. Note that the i82375 PCI/EISA
             * bridge that is present on certain NT Alpha machines appears as
             * non-classified so detect it manually by scanning for its VID/PID.
             */
            if (((NewExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                ((NewExtension->SubClass == PCI_SUBCLASS_BR_ISA) ||
                 (NewExtension->SubClass == PCI_SUBCLASS_BR_EISA) ||
                 (NewExtension->SubClass == PCI_SUBCLASS_BR_MCA))) ||
                ((NewExtension->VendorId == 0x8086) &&
                 (NewExtension->DeviceId == 0x482)))
            {
                /* Do not allow these legacy bridges to be powered down */
                NewExtension->DisablePowerDown = TRUE;
            }

            /* Save latency and cache size information */
            NewExtension->SavedLatencyTimer = PciData->LatencyTimer;
            NewExtension->SavedCacheLineSize = PciData->CacheLineSize;

            /* The PDO is now ready to go */
            DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        }
    }

    /* Enumeration is completed */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciQueryDeviceRelations(IN PPCI_FDO_EXTENSION DeviceExtension,
                        IN OUT PDEVICE_RELATIONS *pDeviceRelations)
{
    NTSTATUS Status;
    PPCI_PDO_EXTENSION PdoExtension;
    ULONG PdoCount = 0;
    PDEVICE_RELATIONS DeviceRelations, NewRelations;
    SIZE_T Size;
    PDEVICE_OBJECT DeviceObject, *ObjectArray;
    PAGED_CODE();

    /* Make sure the FDO is started */
    ASSERT(DeviceExtension->DeviceState == PciStarted);

    /* Synchronize while we enumerate the bus */
    Status = PciBeginStateTransition(DeviceExtension, PciSynchronizedOperation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Scan all children PDO */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Invalidate them */
        PdoExtension->NotPresent = TRUE;
    }

    /* Scan the PCI Bus */
    Status = PciScanBus(DeviceExtension);
    ASSERT(NT_SUCCESS(Status));

    /* Enumerate all children PDO again */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Check for PDOs that are still invalidated */
        if (PdoExtension->NotPresent)
        {
            /* This means this PDO existed before, but not anymore */
            PdoExtension->ReportedMissing = TRUE;
            DPRINT1("PCI - Old device (pdox) %08x not found on rescan.\n",
                    PdoExtension);
        }
        else
        {
            /* Increase count of detected PDOs */
            PdoCount++;
        }
    }

    /* Read the current relations and add the newly discovered relations */
    DeviceRelations = *pDeviceRelations;
    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
           PdoCount * sizeof(PDEVICE_OBJECT);
    if (DeviceRelations) Size += sizeof(PDEVICE_OBJECT) * DeviceRelations->Count;

    /* Allocate the device relations */
    NewRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(0, Size, 'BicP');
    if (!NewRelations)
    {
        /* Out of space, cancel the operation */
        PciCancelStateTransition(DeviceExtension, PciSynchronizedOperation);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check if there were any older relations */
    NewRelations->Count = 0;
    if (DeviceRelations)
    {
        /* Copy the old relations into the new buffer, then free the old one */
        RtlCopyMemory(NewRelations,
                      DeviceRelations,
                      FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
                      DeviceRelations->Count * sizeof(PDEVICE_OBJECT));
        ExFreePoolWithTag(DeviceRelations, 0);
    }

    /* Print out that we're ready to dump relations */
    DPRINT1("PCI QueryDeviceRelations/BusRelations FDOx %08x (bus 0x%02x)\n",
            DeviceExtension,
            DeviceExtension->BaseBus);

    /* Loop the current PDO children and the device relation object array */
    PdoExtension = DeviceExtension->ChildPdoList;
    ObjectArray = &NewRelations->Objects[NewRelations->Count];
    while (PdoExtension)
    {
        /* Dump this relation */
        DPRINT1("  QDR PDO %08x (x %08x)%s\n",
                PdoExtension->PhysicalDeviceObject,
                PdoExtension,
                PdoExtension->NotPresent ?
                "<Omitted, device flaged not present>" : "");

        /* Is this PDO present? */
        if (!PdoExtension->NotPresent)
        {
            /* Reference it and add it to the array */
            DeviceObject = PdoExtension->PhysicalDeviceObject;
            ObfReferenceObject(DeviceObject);
            *ObjectArray++ = DeviceObject;
        }

        /* Go to the next PDO */
        PdoExtension = PdoExtension->Next;
    }

    /* Terminate dumping the relations */
    DPRINT1("  QDR Total PDO count = %d (%d already in list)\n",
            NewRelations->Count + PdoCount,
            NewRelations->Count);

    /* Return the final count and the new buffer */
    NewRelations->Count += PdoCount;
    *pDeviceRelations = NewRelations;
    return STATUS_SUCCESS;
}

/* EOF */
