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

PIO_RESOURCE_REQUIREMENTS_LIST PciZeroIoResourceRequirements;

PCI_CONFIGURATOR PciConfigurators[] =
{
    {
        Device_MassageHeaderForLimitsDetermination,
        Device_RestoreCurrent,
        Device_SaveLimits,
        Device_SaveCurrentSettings,
        Device_ChangeResourceSettings,
        Device_GetAdditionalResourceDescriptors,
        Device_ResetDevice
    },
    {
        PPBridge_MassageHeaderForLimitsDetermination,
        PPBridge_RestoreCurrent,
        PPBridge_SaveLimits,
        PPBridge_SaveCurrentSettings,
        PPBridge_ChangeResourceSettings,
        PPBridge_GetAdditionalResourceDescriptors,
        PPBridge_ResetDevice
    },
    {
        Cardbus_MassageHeaderForLimitsDetermination,
        Cardbus_RestoreCurrent,
        Cardbus_SaveLimits,
        Cardbus_SaveCurrentSettings,
        Cardbus_ChangeResourceSettings,
        Cardbus_GetAdditionalResourceDescriptors,
        Cardbus_ResetDevice
    }
};

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PciComputeNewCurrentSettings(IN PPCI_PDO_EXTENSION PdoExtension,
                             IN PCM_RESOURCE_LIST ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial, InterruptResource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR BaseResource, CurrentDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PreviousDescriptor;
    CM_PARTIAL_RESOURCE_DESCRIPTOR ResourceArray[7];
    PCM_FULL_RESOURCE_DESCRIPTOR FullList;
    BOOLEAN DrainPartial, RangeChange;
    ULONG i, j;
    PPCI_FUNCTION_RESOURCES PciResources;
    PAGED_CODE();

    /* Make sure we have either no resources, or at least one */
    ASSERT((ResourceList == NULL) || (ResourceList->Count == 1));

    /* Initialize no partial, interrupt descriptor, or range change */
    Partial = NULL;
    InterruptResource = NULL;
    RangeChange = FALSE;

    /* Check if there's not actually any resources */
    if (!(ResourceList) || !(ResourceList->Count))
    {
        /* Then just return the hardware update state */
        return PdoExtension->UpdateHardware;
    }

    /* Print the new specified resource list */
    PciDebugPrintCmResList(ResourceList);

    /* Clear the temporary resource array */
    for (i = 0; i < 7; i++) ResourceArray[i].Type = CmResourceTypeNull;

    /* Loop the full resource descriptor */
    FullList = ResourceList->List;
    for (i = 0; i < ResourceList->Count; i++)
    {
        /* Initialize loop variables */
        DrainPartial = FALSE;
        BaseResource = NULL;

        /* Loop the partial descriptors */
        Partial = FullList->PartialResourceList.PartialDescriptors;
        for (j = 0; j < FullList->PartialResourceList.Count; j++)
        {
            /* Check if we were supposed to drain a partial due to device data */
            if (DrainPartial)
            {
                /* Draining complete, move on to the next descriptor then */
                DrainPartial--;
                continue;
            }

            /* Check what kind of descriptor this was */
            switch (Partial->Type)
            {
                /* Base BAR resources */
                case CmResourceTypePort:
                case CmResourceTypeMemory:

                    /* Set it as the base */
                    ASSERT(BaseResource == NULL);
                    BaseResource = Partial;
                    break;

                /* Interrupt resource */
                case CmResourceTypeInterrupt:

                    /* Make sure it's a compatible (and the only) PCI interrupt */
                    ASSERT(InterruptResource == NULL);
                    ASSERT(Partial->u.Interrupt.Level == Partial->u.Interrupt.Vector);
                    InterruptResource = Partial;

                    /* Only 255 interrupts on x86/x64 hardware */
                    if (Partial->u.Interrupt.Level < 256)
                    {
                        /* Use the passed interrupt line */
                        PdoExtension->AdjustedInterruptLine = Partial->u.Interrupt.Level;
                    }
                    else
                    {
                        /* Invalid vector, so ignore it */
                        PdoExtension->AdjustedInterruptLine = 0;
                    }

                    break;

                /* Check for specific device data */
                case CmResourceTypeDevicePrivate:

                    /* Check what kind of data this was */
                    switch (Partial->u.DevicePrivate.Data[0])
                    {
                        /* Not used in the driver yet */
                        case 1:
                            UNIMPLEMENTED_DBGBREAK();
                            break;

                        /* Not used in the driver yet */
                        case 2:
                            UNIMPLEMENTED_DBGBREAK();
                            break;

                        /* A drain request */
                        case 3:
                            /* Shouldn't be a base resource, this is a drain */
                            ASSERT(BaseResource == NULL);
                            DrainPartial = Partial->u.DevicePrivate.Data[1];
                            ASSERT(DrainPartial == TRUE);
                            break;
                    }
                    break;
            }

            /* Move to the next descriptor */
            Partial = CmiGetNextPartialDescriptor(Partial);
        }

        /* We should be starting a new list now */
        ASSERT(BaseResource == NULL);
        FullList = (PVOID)Partial;
    }

    /* Check the current assigned PCI resources */
    PciResources = PdoExtension->Resources;
    if (!PciResources) return FALSE;

    //if... // MISSING CODE
    UNIMPLEMENTED;
    DPRINT1("Missing sanity checking code!\n");

    /* Loop all the PCI function resources */
    for (i = 0; i < 7; i++)
    {
        /* Get the current function resource descriptor, and the new one */
        CurrentDescriptor = &PciResources->Current[i];
        Partial = &ResourceArray[i];

        /* Previous is current during the first loop iteration */
        PreviousDescriptor = &PciResources->Current[(i == 0) ? (0) : (i - 1)];

        /* Check if this new descriptor is different than the old one */
        if (((Partial->Type != CurrentDescriptor->Type) ||
             (Partial->Type != CmResourceTypeNull)) &&
            ((Partial->u.Generic.Start.QuadPart !=
              CurrentDescriptor->u.Generic.Start.QuadPart) ||
             (Partial->u.Generic.Length != CurrentDescriptor->u.Generic.Length)))
        {
            /* Record a change */
            RangeChange = TRUE;

            /* Was there a range before? */
            if (CurrentDescriptor->Type != CmResourceTypeNull)
            {
                /* Print it */
                DbgPrint("      Old range-\n");
                PciDebugPrintPartialResource(CurrentDescriptor);
            }
            else
            {
                /* There was no range */
                DbgPrint("      Previously unset range\n");
            }

            /* Print new one */
            DbgPrint("      changed to\n");
            PciDebugPrintPartialResource(Partial);

            /* Update to new range */
            CurrentDescriptor->Type = Partial->Type;
            PreviousDescriptor->u.Generic.Start = Partial->u.Generic.Start;
            PreviousDescriptor->u.Generic.Length = Partial->u.Generic.Length;
            CurrentDescriptor = PreviousDescriptor;
        }
    }

    /* Either the hardware was updated, or a resource range changed */
    return ((RangeChange) || (PdoExtension->UpdateHardware));
}

VOID
NTAPI
PcipUpdateHardware(IN PVOID Context,
                   IN PVOID Context2)
{
    PPCI_PDO_EXTENSION PdoExtension = Context;
    PPCI_COMMON_HEADER PciData = Context2;

    /* Check if we're allowed to disable decodes */
    PciData->Command = PdoExtension->CommandEnables;
    if (!(PdoExtension->HackFlags & PCI_HACK_PRESERVE_COMMAND))
    {
        /* Disable all decodes */
        PciData->Command &= ~(PCI_ENABLE_IO_SPACE |
                              PCI_ENABLE_MEMORY_SPACE |
                              PCI_ENABLE_BUS_MASTER |
                              PCI_ENABLE_WRITE_AND_INVALIDATE);
    }

    /* Update the device configuration */
    PciData->Status = 0;
    PciWriteDeviceConfig(PdoExtension, PciData, 0, PCI_COMMON_HDR_LENGTH);

    /* Turn decodes back on */
    PciDecodeEnable(PdoExtension, TRUE, &PdoExtension->CommandEnables);
}

VOID
NTAPI
PciUpdateHardware(IN PPCI_PDO_EXTENSION PdoExtension,
                  IN PPCI_COMMON_HEADER PciData)
{
    PCI_IPI_CONTEXT Context;

    /* Check for critical devices and PCI Debugging devices */
    if ((PdoExtension->HackFlags & PCI_HACK_CRITICAL_DEVICE) ||
        (PdoExtension->OnDebugPath))
    {
        /* Build the context and send an IPI */
        Context.RunCount = 1;
        Context.Barrier = 1;
        Context.Context = PciData;
        Context.Function = PcipUpdateHardware;
        Context.DeviceExtension = PdoExtension;
        KeIpiGenericCall(PciExecuteCriticalSystemRoutine, (ULONG_PTR)&Context);
    }
    else
    {
        /* Just to the update inline */
        PcipUpdateHardware(PdoExtension, PciData);
    }
}

PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
PciAllocateIoRequirementsList(IN ULONG Count,
                              IN ULONG BusNumber,
                              IN ULONG SlotNumber)
{
    SIZE_T Size;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    /* Calculate the final size of the list, including each descriptor */
    Size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
    if (Count > 1) Size = sizeof(IO_RESOURCE_DESCRIPTOR) * (Count - 1) +
                          sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

    /* Allocate the list */
    RequirementsList = ExAllocatePoolWithTag(PagedPool, Size, 'BicP');
    if (!RequirementsList) return NULL;

    /* Initialize it */
    RtlZeroMemory(RequirementsList, Size);
    RequirementsList->AlternativeLists = 1;
    RequirementsList->BusNumber = BusNumber;
    RequirementsList->SlotNumber = SlotNumber;
    RequirementsList->InterfaceType = PCIBus;
    RequirementsList->ListSize = Size;
    RequirementsList->List[0].Count = Count;
    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;

    /* Return it */
    return RequirementsList;
}

PCM_RESOURCE_LIST
NTAPI
PciAllocateCmResourceList(IN ULONG Count,
                          IN ULONG BusNumber)
{
    SIZE_T Size;
    PCM_RESOURCE_LIST ResourceList;

    /* Calculate the final size of the list, including each descriptor */
    Size = sizeof(CM_RESOURCE_LIST);
    if (Count > 1) Size = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (Count - 1) +
                          sizeof(CM_RESOURCE_LIST);

    /* Allocate the list */
    ResourceList = ExAllocatePoolWithTag(PagedPool, Size, 'BicP');
    if (!ResourceList) return NULL;

    /* Initialize it */
    RtlZeroMemory(ResourceList, Size);
    ResourceList->Count = 1;
    ResourceList->List[0].BusNumber = BusNumber;
    ResourceList->List[0].InterfaceType = PCIBus;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = Count;

    /* Return it */
    return ResourceList;
}

NTSTATUS
NTAPI
PciQueryResources(IN PPCI_PDO_EXTENSION PdoExtension,
                  OUT PCM_RESOURCE_LIST *Buffer)
{
    PPCI_FUNCTION_RESOURCES PciResources;
    BOOLEAN HaveVga, HaveMemSpace, HaveIoSpace;
    USHORT BridgeControl, PciCommand;
    ULONG Count, i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial, Resource, LastResource;
    PCM_RESOURCE_LIST ResourceList;
    UCHAR InterruptLine;
    PAGED_CODE();

    /* Assume failure */
    Count = 0;
    HaveVga = FALSE;
    *Buffer = NULL;

    /* Make sure there's some resources to query */
    PciResources = PdoExtension->Resources;
    if (!PciResources) return STATUS_SUCCESS;

    /* Read the decodes */
    PciReadDeviceConfig(PdoExtension,
                        &PciCommand,
                        FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                        sizeof(USHORT));

    /* Check which ones are turned on */
    HaveIoSpace = PciCommand & PCI_ENABLE_IO_SPACE;
    HaveMemSpace = PciCommand & PCI_ENABLE_MEMORY_SPACE;

    /* Loop maximum possible descriptors */
    for (i = 0; i < 7; i++)
    {
        /* Check if the decode for this descriptor is actually turned on */
        Partial = &PciResources->Current[i];
        if (((HaveMemSpace) && (Partial->Type == CmResourceTypeMemory)) ||
            ((HaveIoSpace) && (Partial->Type == CmResourceTypePort)))
        {
            /* One more fully active descriptor */
            Count++;
        }
    }

    /* If there's an interrupt pin associated, check at least one decode is on */
    if ((PdoExtension->InterruptPin) && ((HaveMemSpace) || (HaveIoSpace)))
    {
        /* Read the interrupt line for the pin, add a descriptor if it's valid */
        InterruptLine = PdoExtension->AdjustedInterruptLine;
        if ((InterruptLine) && (InterruptLine != -1)) Count++;
    }

    /* Check for PCI bridge */
    if (PdoExtension->HeaderType == PCI_BRIDGE_TYPE)
    {
        /* Read bridge settings, check if VGA is present */
        PciReadDeviceConfig(PdoExtension,
                            &BridgeControl,
                            FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.BridgeControl),
                            sizeof(USHORT));
        if (BridgeControl & PCI_ENABLE_BRIDGE_VGA)
        {
            /* Remember for later */
            HaveVga = TRUE;

            /* One memory descriptor for 0xA0000, plus the two I/O port ranges */
            if (HaveMemSpace) Count++;
            if (HaveIoSpace) Count += 2;
        }
    }

    /* If there's no descriptors in use, there's no resources, so return */
    if (!Count) return STATUS_SUCCESS;

    /* Allocate a resource list to hold the resources */
    ResourceList = PciAllocateCmResourceList(Count,
                                             PdoExtension->ParentFdoExtension->BaseBus);
    if (!ResourceList) return STATUS_INSUFFICIENT_RESOURCES;

    /* This is where the descriptors will be copied into */
    Resource = ResourceList->List[0].PartialResourceList.PartialDescriptors;
    LastResource = Resource + Count + 1;

    /* Loop maximum possible descriptors */
    for (i = 0; i < 7; i++)
    {
        /* Check if the decode for this descriptor is actually turned on */
        Partial = &PciResources->Current[i];
        if (((HaveMemSpace) && (Partial->Type == CmResourceTypeMemory)) ||
            ((HaveIoSpace) && (Partial->Type == CmResourceTypePort)))
        {
            /* Copy the descriptor into the resource list */
            *Resource++ = *Partial;
        }
    }

    /* Check if earlier the code detected this was a PCI bridge with VGA on it */
    if (HaveVga)
    {
        /* Are the memory decodes enabled? */
        if (HaveMemSpace)
        {
            /* Build a memory descriptor for a 128KB framebuffer at 0xA0000 */
            Resource->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            Resource->u.Generic.Start.HighPart = 0;
            Resource->Type = CmResourceTypeMemory;
            Resource->u.Generic.Start.LowPart = 0xA0000;
            Resource->u.Generic.Length = 0x20000;
            Resource++;
        }

        /* Are the I/O decodes enabled? */
        if (HaveIoSpace)
        {
            /* Build an I/O descriptor for the graphic ports at 0x3B0 */
            Resource->Type = CmResourceTypePort;
            Resource->Flags = CM_RESOURCE_PORT_POSITIVE_DECODE | CM_RESOURCE_PORT_10_BIT_DECODE;
            Resource->u.Port.Start.QuadPart = 0x3B0u;
            Resource->u.Port.Length = 0xC;
            Resource++;

            /* Build an I/O descriptor for the graphic ports at 0x3C0 */
            Resource->Type = CmResourceTypePort;
            Resource->Flags = CM_RESOURCE_PORT_POSITIVE_DECODE | CM_RESOURCE_PORT_10_BIT_DECODE;
            Resource->u.Port.Start.QuadPart = 0x3C0u;
            Resource->u.Port.Length = 0x20;
            Resource++;
        }
    }

    /* If there's an interrupt pin associated, check at least one decode is on */
    if ((PdoExtension->InterruptPin) && ((HaveMemSpace) || (HaveIoSpace)))
    {
         /* Read the interrupt line for the pin, check if it's valid */
         InterruptLine = PdoExtension->AdjustedInterruptLine;
         if ((InterruptLine) && (InterruptLine != -1))
         {
             /* Make sure there's still space */
             ASSERT(Resource < LastResource);

             /* Add the interrupt descriptor */
             Resource->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
             Resource->Type = CmResourceTypeInterrupt;
             Resource->ShareDisposition = CmResourceShareShared;
             Resource->u.Interrupt.Affinity = -1;
             Resource->u.Interrupt.Level = InterruptLine;
             Resource->u.Interrupt.Vector = InterruptLine;
        }
    }

    /* Return the resource list */
    *Buffer = ResourceList;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciQueryTargetDeviceRelations(IN PPCI_PDO_EXTENSION PdoExtension,
                              IN OUT PDEVICE_RELATIONS *pDeviceRelations)
{
    PDEVICE_RELATIONS DeviceRelations;
    PAGED_CODE();

    /* If there were existing relations, free them */
    if (*pDeviceRelations) ExFreePoolWithTag(*pDeviceRelations, 0);

    /* Allocate a new structure for the relations */
    DeviceRelations = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(DEVICE_RELATIONS),
                                            'BicP');
    if (!DeviceRelations) return STATUS_INSUFFICIENT_RESOURCES;

    /* Only one relation: the PDO */
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = PdoExtension->PhysicalDeviceObject;
    ObReferenceObject(DeviceRelations->Objects[0]);

    /* Return the new relations */
    *pDeviceRelations = DeviceRelations;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciQueryEjectionRelations(IN PPCI_PDO_EXTENSION PdoExtension,
                          IN OUT PDEVICE_RELATIONS *pDeviceRelations)
{
    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(pDeviceRelations);

    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PciBuildRequirementsList(IN PPCI_PDO_EXTENSION PdoExtension,
                         IN PPCI_COMMON_HEADER PciData,
                         OUT PIO_RESOURCE_REQUIREMENTS_LIST* Buffer)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(PciData);

    {
        /* There aren't, so use the zero descriptor */
        RequirementsList = PciZeroIoResourceRequirements;

        /* Does it actually exist yet? */
        if (!PciZeroIoResourceRequirements)
        {
            /* Allocate it, and use it for future use */
            RequirementsList = PciAllocateIoRequirementsList(0, 0, 0);
            PciZeroIoResourceRequirements = RequirementsList;
            if (!PciZeroIoResourceRequirements) return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Return the zero requirements list to the caller */
        *Buffer = RequirementsList;
        DPRINT1("PCI - build resource reqs - early out, 0 resources\n");
        return STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciQueryRequirements(IN PPCI_PDO_EXTENSION PdoExtension,
                     IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList)
{
    NTSTATUS Status;
    PCI_COMMON_HEADER PciHeader;
    PAGED_CODE();

    /* Check if the PDO has any resources, or at least an interrupt pin */
    if ((PdoExtension->Resources) || (PdoExtension->InterruptPin))
    {
        /* Read the current PCI header */
        PciReadDeviceConfig(PdoExtension, &PciHeader, 0, PCI_COMMON_HDR_LENGTH);

        /* Use it to build a list of requirements */
        Status = PciBuildRequirementsList(PdoExtension, &PciHeader, RequirementsList);
        if (!NT_SUCCESS(Status)) return Status;

        /* Is this a Compaq PCI Hotplug Controller (r17) on a PAE system ? */
        if ((PciHeader.VendorID == 0xE11) &&
            (PciHeader.DeviceID == 0xA0F7) &&
            (PciHeader.RevisionID == 17) &&
            (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)))
        {
            /* Have not tested this on eVb's machine yet */
            UNIMPLEMENTED_DBGBREAK();
        }

        /* Check if the requirements are actually the zero list */
        if (*RequirementsList == PciZeroIoResourceRequirements)
        {
            /* A simple NULL will suffice for the PnP Manager */
            *RequirementsList = NULL;
            DPRINT1("Returning NULL requirements list\n");
        }
        else
        {
            /* Otherwise, print out the requirements list */
            PciDebugPrintIoResReqList(*RequirementsList);
        }
    }
    else
    {
        /* There aren't any resources, so simply return NULL */
        DPRINT1("PciQueryRequirements returning NULL requirements list\n");
        *RequirementsList = NULL;
    }

    /* This call always succeeds (but maybe with no requirements) */
    return STATUS_SUCCESS;
}

/*
 * 7. The IO/MEM/Busmaster decodes are disabled for the device.
 * 8. The PCI bus driver sets the operating mode bits of the Programming
 *    Interface byte to switch the controller to native mode.
 *
 *    Important: When the controller is set to native mode, it must quiet itself
 *    and must not decode I/O resources or generate interrupts until the operating
 *    system has enabled the ports in the PCI configuration header.
 *    The IO/MEM/BusMaster bits will be disabled before the mode change, but it
 *    is not possible to disable interrupts on the device. The device must not
 *    generate interrupts (either legacy or native mode) while the decodes are
 *    disabled in the command register.
 *
 *    This operation is expected to be instantaneous and the operating system does
 *    not stall afterward. It is also expected that the interrupt pin register in
 *    the PCI Configuration space for this device is accurate. The operating system
 *    re-reads this data after previously ignoring it.
 */
BOOLEAN
NTAPI
PciConfigureIdeController(IN PPCI_PDO_EXTENSION PdoExtension,
                          IN PPCI_COMMON_HEADER PciData,
                          IN BOOLEAN Initial)
{
    UCHAR MasterMode, SlaveMode, MasterFixed, SlaveFixed, ProgIf, NewProgIf;
    BOOLEAN Switched;
    USHORT Command;

    /* Assume it won't work */
    Switched = FALSE;

    /* Get master and slave current settings, and programmability flag */
    ProgIf = PciData->ProgIf;
    MasterMode = (ProgIf & 1) == 1;
    MasterFixed = (ProgIf & 2) == 0;
    SlaveMode = (ProgIf & 4) == 4;
    SlaveFixed = (ProgIf & 8) == 0;

    /*
     * [..] In order for Windows XP SP1 and Windows Server 2003 to switch an ATA
     * ATA controller from compatible mode to native mode, the following must be
     * true:
     *
     * - The controller must indicate in its programming interface that both channels
     *   can be switched to native mode. Windows XP SP1 and Windows Server 2003 do
     *   not support switching only one IDE channel to native mode. See the PCI IDE
     *   Controller Specification Revision 1.0 for details.
     */
    if ((MasterMode != SlaveMode) || (MasterFixed != SlaveFixed))
    {
        /* Windows does not support this configuration, fail */
        DPRINT1("PCI: Warning unsupported IDE controller configuration for VEN_%04x&DEV_%04x!",
                PdoExtension->VendorId,
                PdoExtension->DeviceId);
        return Switched;
    }

    /* Check if the controller is already in native mode */
    if ((MasterMode) && (SlaveMode))
    {
        /* Check if I/O decodes should be disabled */
        if ((Initial) || (PdoExtension->IoSpaceUnderNativeIdeControl))
        {
            /* Read the current command */
            PciReadDeviceConfig(PdoExtension,
                                &Command,
                                FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                sizeof(USHORT));

            /* Disable I/O space decode */
            Command &= ~PCI_ENABLE_IO_SPACE;

            /* Update new command in PCI IDE controller */
            PciWriteDeviceConfig(PdoExtension,
                                 &Command,
                                 FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                 sizeof(USHORT));

            /* Save updated command value */
            PciData->Command = Command;
        }

        /* The controller is now in native mode */
        Switched = TRUE;
    }
    else if (!(MasterFixed) &&
             !(SlaveFixed) &&
             (PdoExtension->BIOSAllowsIDESwitchToNativeMode) &&
             !(PdoExtension->HackFlags & PCI_HACK_DISABLE_IDE_NATIVE_MODE))
    {
        /* Turn off decodes */
        PciDecodeEnable(PdoExtension, FALSE, NULL);

        /* Update the current command */
        PciReadDeviceConfig(PdoExtension,
                            &PciData->Command,
                            FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                            sizeof(USHORT));

        /* Enable native mode */
        ProgIf = PciData->ProgIf | 5;
        PciWriteDeviceConfig(PdoExtension,
                             &ProgIf,
                             FIELD_OFFSET(PCI_COMMON_HEADER, ProgIf),
                             sizeof(UCHAR));

        /* Verify the setting "stuck" */
        PciReadDeviceConfig(PdoExtension,
                            &NewProgIf,
                            FIELD_OFFSET(PCI_COMMON_HEADER, ProgIf),
                            sizeof(UCHAR));
        if (NewProgIf == ProgIf)
        {
            /* Update the header and PDO data with the new programming mode */
            PciData->ProgIf = ProgIf;
            PdoExtension->ProgIf = NewProgIf;

            /* Clear the first four BARs to reset current BAR settings */
            PciData->u.type0.BaseAddresses[0] = 0;
            PciData->u.type0.BaseAddresses[1] = 0;
            PciData->u.type0.BaseAddresses[2] = 0;
            PciData->u.type0.BaseAddresses[3] = 0;
            PciWriteDeviceConfig(PdoExtension,
                                 PciData->u.type0.BaseAddresses,
                                 FIELD_OFFSET(PCI_COMMON_HEADER,
                                              u.type0.BaseAddresses),
                                 4 * sizeof(ULONG));

            /* Re-read the BARs to have the latest data for native mode IDE */
            PciReadDeviceConfig(PdoExtension,
                                PciData->u.type0.BaseAddresses,
                                FIELD_OFFSET(PCI_COMMON_HEADER,
                                             u.type0.BaseAddresses),
                                4 * sizeof(ULONG));

            /* Re-read the interrupt pin used for native mode IDE */
            PciReadDeviceConfig(PdoExtension,
                                &PciData->u.type0.InterruptPin,
                                FIELD_OFFSET(PCI_COMMON_HEADER,
                                             u.type0.InterruptPin),
                                sizeof(UCHAR));

            /* The IDE Controller is now in native mode */
            Switched = TRUE;
        }
        else
        {
            /* Settings did not work, fail */
            DPRINT1("PCI: Warning failed switch to native mode for IDE controller VEN_%04x&DEV_%04x!",
                    PciData->VendorID,
                    PciData->DeviceID);
        }
   }

   /* Return whether or not native mode was enabled on the IDE controller */
   return Switched;
}

VOID
NTAPI
PciApplyHacks(IN PPCI_FDO_EXTENSION DeviceExtension,
              IN PPCI_COMMON_HEADER PciData,
              IN PCI_SLOT_NUMBER SlotNumber,
              IN ULONG OperationType,
              PPCI_PDO_EXTENSION PdoExtension)
{
    ULONG LegacyBaseAddress;
    USHORT Command;
    UCHAR RegValue;

    UNREFERENCED_PARAMETER(SlotNumber);

    /* Check what kind of hack operation this is */
    switch (OperationType)
    {
        /*
         * This is mostly concerned with fixing up incorrect class data that can
         * exist on certain PCI hardware before the 2.0 spec was ratified.
         */
        case PCI_HACK_FIXUP_BEFORE_CONFIGURATION:

            /* Note that the i82375 PCI/EISA and the i82378 PCI/ISA bridges that
             * are present on certain DEC/NT Alpha machines are pre-PCI 2.0 devices
             * and appear as non-classified, so their correct class/subclass data
             * is written here instead.
             */
            if ((PciData->VendorID == 0x8086) &&
                ((PciData->DeviceID == 0x482) || (PciData->DeviceID == 0x484)))
            {
                /* Note that 0x482 is the i82375 (EISA), 0x484 is the i82378 (ISA) */
                PciData->SubClass = PciData->DeviceID == 0x482 ?
                                    PCI_SUBCLASS_BR_EISA : PCI_SUBCLASS_BR_ISA;
                PciData->BaseClass = PCI_CLASS_BRIDGE_DEV;

                /*
                 * Because the software is modifying the actual header data from
                 * the BIOS, this flag tells the driver to ignore failures when
                 * comparing the original BIOS data with the PCI data.
                 */
                if (PdoExtension) PdoExtension->ExpectedWritebackFailure = TRUE;
            }

            /* Note that in this case, an immediate return is issued */
            return;

        /*
         * This is concerned with setting up interrupts correctly for native IDE
         * mode, but will also handle broken VGA decoding on older bridges as
         * well as a PAE-specific hack for certain Compaq Hot-Plug Controllers.
         */
        case PCI_HACK_FIXUP_AFTER_CONFIGURATION:

            /* There should always be a PDO extension passed in */
            ASSERT(PdoExtension);

            /*
             * On the OPTi Viper-M IDE controller, Linux doesn't support IDE-DMA
             * and FreeBSD bug reports indicate that the system crashes when the
             * feature is enabled (so it's disabled on that OS as well). In the
             * NT PCI Bus Driver, it seems Microsoft too, completely disables
             * Native IDE functionality on this controller, so it would seem OPTi
             * simply frelled up this controller.
             */
            if ((PciData->VendorID == 0x1045) && (PciData->DeviceID != 0xC621))
            {
                /* Disable native mode */
                PciData->ProgIf &= ~5;
                PciData->u.type0.InterruptPin = 0;

                /*
                 * Because the software is modifying the actual header data from
                 * the BIOS, this flag tells the driver to ignore failures when
                 * comparing the original BIOS data with the PCI data.
                 */
                PdoExtension->ExpectedWritebackFailure = TRUE;
            }
            else if ((PciData->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
                    (PciData->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR))
            {
                /* For other IDE controllers, start out in compatible mode */
                PdoExtension->BIOSAllowsIDESwitchToNativeMode = FALSE;

                /*
                 * Registry must have enabled native mode (typically as a result
                 * of an INF file directive part of the IDE controller's driver)
                 * and the system must not be booted in Safe Mode. If that checks
                 * out, then evaluate the ACPI NATA method to see if the platform
                 * supports this. See the section "BIOS and Platform Prerequisites
                 * for Switching a Native-Mode-Capable Controller" in the Storage
                 * section of the Windows Driver Kit for more details:
                 *
                 * 5. For each ATA controller enumerated, the PCI bus driver checks
                 *    the Programming Interface register of the IDE controller to
                 *    see if it supports switching both channels to native mode.
                 * 6. The PCI bus driver checks whether the BIOS/platform supports
                 *    switching the controller by checking the NATA method described
                 *    earlier in this article.
                 *
                 *    If an ATA controller does not indicate that it is native
                 *    mode-capable, or if the BIOS NATA control method is missing
                 *    or does not list that device, the PCI bus driver does not
                 *    switch the controller and it is assigned legacy resources.
                 *
                 *  If both the controller and the BIOS indicate that the controller
                 *  can be switched, the process of switching the controller begins
                 *  with the next step.
                 */
                if ((PciEnableNativeModeATA) &&
                    !(InitSafeBootMode) &&
                    (PciIsSlotPresentInParentMethod(PdoExtension, 'ATAN')))
                {
                    /* The platform supports it, remember that */
                    PdoExtension->BIOSAllowsIDESwitchToNativeMode = TRUE;

                    /*
                     * Now switch the controller into native mode if both channels
                     * support native IDE mode. See "How Windows Switches an ATA
                     * Controller to Native Mode" in the Storage section of the
                     * Windows Driver Kit for more details.
                     */
                    PdoExtension->IDEInNativeMode =
                        PciConfigureIdeController(PdoExtension, PciData, TRUE);
                }

                /* Is native mode enabled after all? */
                if ((PciData->ProgIf & 5) != 5)
                {
                    /* Compatible mode, so force ISA-style IRQ14 and IRQ 15 */
                    PciData->u.type0.InterruptPin = 0;
                }
            }

            /* Is this a PCI device with legacy VGA card decodes on the root bus? */
            if ((PdoExtension->HackFlags & PCI_HACK_VIDEO_LEGACY_DECODE) &&
                (PCI_IS_ROOT_FDO(DeviceExtension)) &&
                !(DeviceExtension->BrokenVideoHackApplied))
            {
                /* Tell the arbiter to apply a hack for these older devices */
                ario_ApplyBrokenVideoHack(DeviceExtension);
            }

            /* Is this a Compaq PCI Hotplug Controller (r17) on a PAE system ? */
            if ((PciData->VendorID == 0xE11) &&
                (PciData->DeviceID == 0xA0F7) &&
                (PciData->RevisionID == 17) &&
                (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)))
            {
                /* Turn off the decodes immediately */
                PciData->Command &= ~(PCI_ENABLE_IO_SPACE |
                                      PCI_ENABLE_MEMORY_SPACE |
                                      PCI_ENABLE_BUS_MASTER);
                PciWriteDeviceConfig(PdoExtension,
                                     &PciData->Command,
                                     FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                     sizeof(USHORT));

                /* Do not EVER turn them on again, this will blow up the system */
                PdoExtension->CommandEnables &= ~(PCI_ENABLE_IO_SPACE |
                                                  PCI_ENABLE_MEMORY_SPACE |
                                                  PCI_ENABLE_BUS_MASTER);
                PdoExtension->HackFlags |= PCI_HACK_PRESERVE_COMMAND;
            }
            break;

        /*
         * This is called whenever resources are changed and hardware needs to be
         * updated. It is concerned with two highly specific erratas on an IBM
         * hot-plug docking bridge used on the Thinkpad 600 Series and on Intel's
         * ICH PCI Bridges.
         */
        case PCI_HACK_FIXUP_BEFORE_UPDATE:

            /* There should always be a PDO extension passed in */
            ASSERT(PdoExtension);

            /* Is this an IBM 20H2999 PCI Docking Bridge, used on Thinkpads? */
            if ((PdoExtension->VendorId == 0x1014) &&
                (PdoExtension->DeviceId == 0x95))
            {
                /* Read the current command */
                PciReadDeviceConfig(PdoExtension,
                                    &Command,
                                    FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                    sizeof(USHORT));

                /* Turn off the decodes */
                PciDecodeEnable(PdoExtension, FALSE, &Command);

                /* Apply the required IBM workaround */
                PciReadDeviceConfig(PdoExtension, &RegValue, 0xE0, sizeof(UCHAR));
                RegValue &= ~2;
                RegValue |= 1;
                PciWriteDeviceConfig(PdoExtension, &RegValue, 0xE0, sizeof(UCHAR));

                /* Restore the command to its original value */
                PciWriteDeviceConfig(PdoExtension,
                                     &Command,
                                     FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                     sizeof(USHORT));

            }

            /*
             * Check for Intel ICH PCI-to-PCI (i82801) bridges (used on the i810,
             * i820, i840, i845 Chipsets) that have subtractive decode enabled,
             * and whose hack flags do not specify that this support is broken.
             */
            if ((PdoExtension->HeaderType == PCI_BRIDGE_TYPE) &&
                (PdoExtension->Dependent.type1.SubtractiveDecode) &&
                ((PdoExtension->VendorId == 0x8086) &&
                 ((PdoExtension->DeviceId == 0x2418) ||
                  (PdoExtension->DeviceId == 0x2428) ||
                  (PdoExtension->DeviceId == 0x244E) ||
                  (PdoExtension->DeviceId == 0x2448))) &&
               !(PdoExtension->HackFlags & PCI_HACK_BROKEN_SUBTRACTIVE_DECODE))
            {
                /*
                 * The positive decode window shouldn't be used, these values are
                 * normally all read-only or initialized to 0 by the BIOS, but
                 * it appears Intel doesn't do this, so the PCI Bus Driver will
                 * do it in software instead. Note that this is used to prevent
                 * certain non-compliant PCI devices from breaking down due to the
                 * fact that these ICH bridges have a known "quirk" (which Intel
                 * documents as a known "erratum", although it's not not really
                 * an ICH bug since the PCI specification does allow for it) in
                 * that they will sometimes send non-zero addresses during special
                 * cycles (ie: non-zero data during the address phase). These
                 * broken PCI cards will mistakenly attempt to claim the special
                 * cycle and corrupt their I/O and RAM ranges. Again, in Intel's
                 * defense, the PCI specification only requires stable data, not
                 * necessarily zero data, during the address phase.
                 */
                PciData->u.type1.MemoryBase = 0xFFFF;
                PciData->u.type1.PrefetchBase = 0xFFFF;
                PciData->u.type1.IOBase = 0xFF;
                PciData->u.type1.IOLimit = 0;
                PciData->u.type1.MemoryLimit = 0;
                PciData->u.type1.PrefetchLimit = 0;
                PciData->u.type1.PrefetchBaseUpper32 = 0;
                PciData->u.type1.PrefetchLimitUpper32 = 0;
                PciData->u.type1.IOBaseUpper16 = 0;
                PciData->u.type1.IOLimitUpper16 = 0;
            }
            break;

        default:
            return;
    }

    /* Finally, also check if this is this a CardBUS device? */
    if (PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        /*
         * At offset 44h the LegacyBaseAddress is stored, which is cleared by
         * ACPI-aware versions of Windows, to disable legacy-mode I/O access to
         * CardBus controllers. For more information, see "Supporting CardBus
         * Controllers under ACPI" in the "CardBus Controllers and Windows"
         * Whitepaper on WHDC.
         */
        LegacyBaseAddress = 0;
        PciWriteDeviceConfig(PdoExtension,
                             &LegacyBaseAddress,
                             sizeof(PCI_COMMON_HEADER) + sizeof(ULONG),
                             sizeof(ULONG));
    }
}

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
            ASSERTMSG("PCI Skip Function - Operation type unknown.\n", FALSE);
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

        /* Skip garbage capabilities pointer */
        if (((CapPtr & 0x3) != 0) || (CapPtr < PCI_COMMON_HDR_LENGTH))
        {
            /* Report no extended capabilities */
            PdoExtension->CapabilitiesPtr = 0;
            PdoExtension->HackFlags |= PCI_HACK_NO_PM_CAPS;
        }
        else
        {
            DPRINT1("Device has capabilities at: %lx\n", CapPtr);
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

VOID
NTAPI
PciWriteLimitsAndRestoreCurrent(IN PVOID Reserved,
                                IN PVOID Context2)
{
    PPCI_CONFIGURATOR_CONTEXT Context = Context2;
    PPCI_COMMON_HEADER PciData, Current;
    PPCI_PDO_EXTENSION PdoExtension;

    UNREFERENCED_PARAMETER(Reserved);

    /* Grab all parameters from the context */
    PdoExtension = Context->PdoExtension;
    Current = Context->Current;
    PciData = Context->PciData;

    /* Write the limit discovery header */
    PciWriteDeviceConfig(PdoExtension, PciData, 0, PCI_COMMON_HDR_LENGTH);

    /* Now read what the device indicated the limits are */
    PciReadDeviceConfig(PdoExtension, PciData, 0, PCI_COMMON_HDR_LENGTH);

    /* Then write back the original configuration header */
    PciWriteDeviceConfig(PdoExtension, Current, 0, PCI_COMMON_HDR_LENGTH);

    /* Copy back the original command that was saved in the context */
    Current->Command = Context->Command;
    if (Context->Command)
    {
        /* Program it back into the device */
        PciWriteDeviceConfig(PdoExtension,
                             &Context->Command,
                             FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                             sizeof(USHORT));
    }

    /* Copy back the original status that was saved as well */
    Current->Status = Context->Status;

    /* Call the configurator to restore any other data that might've changed */
    Context->Configurator->RestoreCurrent(Context);
}

NTSTATUS
NTAPI
PcipGetFunctionLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_CONFIGURATOR Configurator;
    PPCI_COMMON_HEADER PciData, Current;
    PPCI_PDO_EXTENSION PdoExtension;
    PCI_IPI_CONTEXT IpiContext;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    ULONG Offset;
    PAGED_CODE();

    /* Grab all parameters from the context */
    PdoExtension = Context->PdoExtension;
    Current = Context->Current;
    PciData = Context->PciData;

    /* Save the current PCI Command and Status word */
    Context->Status = Current->Status;
    Context->Command = Current->Command;

    /* Now that they're saved, clear the status, and disable all decodes */
    Current->Status = 0;
    Current->Command &= ~(PCI_ENABLE_IO_SPACE |
                          PCI_ENABLE_MEMORY_SPACE |
                          PCI_ENABLE_BUS_MASTER);

    /* Make a copy of the current PCI configuration header (with decodes off) */
    RtlCopyMemory(PciData, Current, PCI_COMMON_HDR_LENGTH);

    /* Locate the correct resource configurator for this type of device */
    Configurator = &PciConfigurators[PdoExtension->HeaderType];
    Context->Configurator = Configurator;

    /* Initialize it, which will typically setup the BARs for limit discovery */
    Configurator->Initialize(Context);

    /* Check for critical devices and PCI Debugging devices */
    if ((PdoExtension->HackFlags & PCI_HACK_CRITICAL_DEVICE) ||
        (PdoExtension->OnDebugPath))
    {
        /* Specifically check for a PCI Debugging device */
        if (PdoExtension->OnDebugPath)
        {
            /* Was it enabled for bus mastering? */
            if (Context->Command & PCI_ENABLE_BUS_MASTER)
            {
                /* This decode needs to be re-enabled so debugging can work */
                PciData->Command |= PCI_ENABLE_BUS_MASTER;
                Current->Command |= PCI_ENABLE_BUS_MASTER;
            }

            /* Disable the debugger while the discovery is happening */
            KdDisableDebugger();
        }

        /* For these devices, an IPI must be sent to force high-IRQL discovery */
        IpiContext.Barrier = 1;
        IpiContext.RunCount = 1;
        IpiContext.DeviceExtension = PdoExtension;
        IpiContext.Function = PciWriteLimitsAndRestoreCurrent;
        IpiContext.Context = Context;
        KeIpiGenericCall(PciExecuteCriticalSystemRoutine, (ULONG_PTR)&IpiContext);

        /* Re-enable the debugger if this was a PCI Debugging Device */
        if (PdoExtension->OnDebugPath) KdEnableDebugger();
    }
    else
    {
        /* Otherwise, it's safe to do this in-line at low IRQL */
        PciWriteLimitsAndRestoreCurrent(PdoExtension, Context);
    }

    /*
     * Check if it's valid to compare the headers to see if limit discovery mode
     * has properly exited (the expected case is that the PCI header would now
     * be equal to what it was before). In some cases, it is known that this will
     * fail, because during PciApplyHacks (among other places), software hacks
     * had to be applied to the header, which the hardware-side will not see, and
     * thus the headers would appear "different".
     */
    if (!PdoExtension->ExpectedWritebackFailure)
    {
        /* Read the current PCI header now, after discovery has completed */
        PciReadDeviceConfig(PdoExtension, PciData + 1, 0, PCI_COMMON_HDR_LENGTH);

        /* Check if the current header at entry, is equal to the header now */
        Offset = RtlCompareMemory(PciData + 1, Current, PCI_COMMON_HDR_LENGTH);
        if (Offset != PCI_COMMON_HDR_LENGTH)
        {
            /* It's not, which means configuration somehow changed, dump this */
            DPRINT1("PCI - CFG space write verify failed at offset 0x%x\n", Offset);
            PciDebugDumpCommonConfig(PciData + 1);
            DPRINT1("----------\n");
            PciDebugDumpCommonConfig(Current);
        }
    }

    /* This PDO should not already have resources, since this is only done once */
    ASSERT(PdoExtension->Resources == NULL);

    /* Allocate the structure that will hold the discovered resources and limits */
    PdoExtension->Resources = ExAllocatePoolWithTag(NonPagedPool,
                                                    sizeof(PCI_FUNCTION_RESOURCES),
                                                    'BicP');
    if (!PdoExtension->Resources) return STATUS_INSUFFICIENT_RESOURCES;

    /* Clear it out for now */
    RtlZeroMemory(PdoExtension->Resources, sizeof(PCI_FUNCTION_RESOURCES));

    /* Now call the configurator, which will first store the limits... */
    Configurator->SaveLimits(Context);

    /* ...and then store the current resources being used */
    Configurator->SaveCurrentSettings(Context);

    /* Loop all the limit descriptors backwards */
    IoDescriptor = &PdoExtension->Resources->Limit[PCI_TYPE0_ADDRESSES + 1];
    while (TRUE)
    {
        /* Keep going until a non-null descriptor is found */
        IoDescriptor--;
        if (IoDescriptor->Type != CmResourceTypeNull) break;

        /* This is a null descriptor, is it the last one? */
        if (IoDescriptor == &PdoExtension->Resources->Limit[PCI_TYPE0_ADDRESSES + 1])
        {
            /* This means the descriptor is NULL, which means discovery failed */
            DPRINT1("PCI Resources fail!\n");

            /* No resources will be assigned for the device */
            ExFreePoolWithTag(PdoExtension->Resources, 0);
            PdoExtension->Resources = NULL;
            break;
        }
    }

    /* Return success here, even if the device has no assigned resources */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciGetFunctionLimits(IN PPCI_PDO_EXTENSION PdoExtension,
                     IN PPCI_COMMON_HEADER Current,
                     IN ULONGLONG HackFlags)
{
    NTSTATUS Status;
    PPCI_COMMON_HEADER PciData;
    PCI_CONFIGURATOR_CONTEXT Context;
    PAGED_CODE();

    /* Do the hackflags indicate this device should be skipped? */
    if (PciSkipThisFunction(Current,
                            PdoExtension->Slot,
                            PCI_SKIP_RESOURCE_ENUMERATION,
                            HackFlags))
    {
        /* Do not process its resources */
        return STATUS_SUCCESS;
    }

    /* Allocate a buffer to hold two PCI configuration headers */
    PciData = ExAllocatePoolWithTag(0, 2 * PCI_COMMON_HDR_LENGTH, 'BicP');
    if (!PciData) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set up the context for the resource enumeration, and do it */
    Context.Current = Current;
    Context.PciData = PciData;
    Context.PdoExtension = PdoExtension;
    Status = PcipGetFunctionLimits(&Context);

    /* Enumeration is completed, free the PCI headers and return the status */
    ExFreePoolWithTag(PciData, 0);
    return Status;
}

VOID
NTAPI
PciProcessBus(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PAGED_CODE();

    /* Get the PDO Extension */
    PhysicalDeviceObject = DeviceExtension->PhysicalDeviceObject;
    PdoExtension = (PPCI_PDO_EXTENSION)PhysicalDeviceObject->DeviceExtension;

    /* Cheeck if this is the root bus */
    if (!PCI_IS_ROOT_FDO(DeviceExtension))
    {
        /* Not really handling this year */
        UNIMPLEMENTED_DBGBREAK();

        /* Check for PCI bridges with the ISA bit set, or required */
        if ((PdoExtension) &&
            (PciClassifyDeviceType(PdoExtension) == PciTypePciBridge) &&
            ((PdoExtension->Dependent.type1.IsaBitRequired) ||
             (PdoExtension->Dependent.type1.IsaBitSet)))
        {
            /* We'll need to do some legacy support */
            UNIMPLEMENTED_DBGBREAK();
        }
    }
    else
    {
        /* Scan all of the root bus' children bridges */
        for (PdoExtension = DeviceExtension->ChildBridgePdoList;
             PdoExtension;
             PdoExtension = PdoExtension->NextBridge)
        {
            /* Find any that have the VGA decode bit on */
            if (PdoExtension->Dependent.type1.VgaBitSet)
            {
                /* Again, some more legacy support we'll have to do */
                UNIMPLEMENTED_DBGBREAK();
            }
        }
    }

    /* Check for ACPI systems where the OS assigns bus numbers */
    if (PciAssignBusNumbers)
    {
        /* Not yet supported */
        UNIMPLEMENTED_DBGBREAK();
    }
}

NTSTATUS
NTAPI
PciScanBus(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    ULONG MaxDevice = PCI_MAX_DEVICES;
    BOOLEAN ProcessFlag = FALSE;
    ULONG i, j, k, Size;
    USHORT CapOffset, TempOffset;
    LONGLONG HackFlags;
    PDEVICE_OBJECT DeviceObject;
    UCHAR Buffer[PCI_COMMON_HDR_LENGTH];
    UCHAR BiosBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_HEADER PciData = (PVOID)Buffer;
    PPCI_COMMON_HEADER BiosData = (PVOID)BiosBuffer;
    PCI_SLOT_NUMBER PciSlot;
    PCHAR Name;
    NTSTATUS Status;
    PPCI_PDO_EXTENSION PdoExtension, NewExtension;
    PPCI_PDO_EXTENSION* BridgeExtension;
    PWCHAR DescriptionText;
    USHORT SubVendorId, SubSystemId;
    PCI_CAPABILITIES_HEADER CapHeader, PcixCapHeader;
    UCHAR SecondaryBus;
    DPRINT1("PCI Scan Bus: FDO Extension @ 0x%p, Base Bus = 0x%x\n",
            DeviceExtension, DeviceExtension->BaseBus);

    /* Is this the root FDO? */
    if (!PCI_IS_ROOT_FDO(DeviceExtension))
    {
        /* Get the PDO for the child bus */
        PdoExtension = DeviceExtension->PhysicalDeviceObject->DeviceExtension;
        ASSERT_PDO(PdoExtension);

        /* Check for hack which only allows bus to have one child device */
        if (PdoExtension->HackFlags & PCI_HACK_ONE_CHILD) MaxDevice = 1;

        /* Check if the secondary bus number has changed */
        PciReadDeviceConfig(PdoExtension,
                            &SecondaryBus,
                            FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.SecondaryBus),
                            sizeof(UCHAR));
        if (SecondaryBus != PdoExtension->Dependent.type1.SecondaryBus)
        {
            UNIMPLEMENTED_DBGBREAK("PCI: Bus numbers have been changed!  Restoring originals.\n");
        }
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

            /* Apply any hacks before even analyzing the configuration header */
            PciApplyHacks(DeviceExtension,
                          PciData,
                          PciSlot,
                          PCI_HACK_FIXUP_BEFORE_CONFIGURATION,
                          NULL);

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
                UNIMPLEMENTED_DBGBREAK();
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

            /* Check if the device should be skipped for whatever reason */
            if (PciSkipThisFunction(PciData,
                                    PciSlot,
                                    PCI_SKIP_DEVICE_ENUMERATION,
                                    HackFlags))
            {
                /* Skip this device */
                continue;
            }

            /* Check if a PDO has already been created for this device */
            PdoExtension = PciFindPdoByFunction(DeviceExtension,
                                                PciSlot.u.AsULONG,
                                                PciData);
            if (PdoExtension)
            {
                /* Rescan scenarios are not yet implemented */
                UNIMPLEMENTED_DBGBREAK();
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

            /* Now configure the BARs */
            Status = PciGetFunctionLimits(NewExtension, PciData, HackFlags);

            /* Power up the device */
            PciSetPowerManagedDevicePowerState(NewExtension, PowerDeviceD0, FALSE);

            /* Apply any device hacks required for enumeration */
            PciApplyHacks(DeviceExtension,
                          PciData,
                          PciSlot,
                          PCI_HACK_FIXUP_AFTER_CONFIGURATION,
                          NewExtension);

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

            /* Scan all capabilities */
            CapOffset = NewExtension->CapabilitiesPtr;
            while (CapOffset)
            {
                /* Read this header */
                TempOffset = PciReadDeviceCapability(NewExtension,
                                                     CapOffset,
                                                     0,
                                                     &CapHeader,
                                                     sizeof(PCI_CAPABILITIES_HEADER));
                if (TempOffset != CapOffset)
                {
                    /* This is a strange issue that shouldn't happen normally */
                    DPRINT1("PCI - Failed to read PCI capability at offset 0x%02x\n",
                            CapOffset);
                    ASSERT(TempOffset == CapOffset);
                }

                /* Check for capabilities that this driver cares about */
                switch (CapHeader.CapabilityID)
                {
                    /* Power management capability is heavily used by the bus */
                    case PCI_CAPABILITY_ID_POWER_MANAGEMENT:

                        /* Dump the capability */
                        Name = "POWER";
                        Size = sizeof(PCI_PM_CAPABILITY);
                        break;

                    /* AGP capability is required for AGP bus functionality */
                    case PCI_CAPABILITY_ID_AGP:

                        /* Dump the capability */
                        Name = "AGP";
                        Size = sizeof(PCI_AGP_CAPABILITY);
                        break;

                    /* This driver doesn't really use anything other than that */
                    default:

                        /* Windows prints this, we could do a translation later */
                        Name = "UNKNOWN CAPABILITY";
                        Size = 0;
                        break;
                }

                /* Check if this is a capability that should be dumped */
                if (Size)
                {
                    /* Read the whole capability data */
                    TempOffset = PciReadDeviceCapability(NewExtension,
                                                         CapOffset,
                                                         CapHeader.CapabilityID,
                                                         &CapHeader,
                                                         Size);

                    if (TempOffset != CapOffset)
                    {
                        /* Again, a strange issue that shouldn't be seen */
                        DPRINT1("- Failed to read capability data. ***\n");
                        ASSERT(TempOffset == CapOffset);
                    }
                }

                /* Dump this capability */
                DPRINT1("CAP @%02x ID %02x (%s)\n",
                        CapOffset, CapHeader.CapabilityID, Name);
                for (i = 0; i < Size; i += 2)
                    DPRINT1("  %04x\n", *(PUSHORT)((ULONG_PTR)&CapHeader + i));
                DPRINT1("\n");

                /* Check the next capability */
                CapOffset = CapHeader.Next;
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

            /* Check if the BIOS did not configure a cache line size */
            if (!PciData->CacheLineSize)
            {
                /* Check if the device is disabled */
                if (!(NewExtension->CommandEnables & (PCI_ENABLE_IO_SPACE |
                                                      PCI_ENABLE_MEMORY_SPACE |
                                                      PCI_ENABLE_BUS_MASTER)))
                {
                    /* Check if this is a PCI-X device*/
                    TempOffset = PciReadDeviceCapability(NewExtension,
                                                         NewExtension->CapabilitiesPtr,
                                                         PCI_CAPABILITY_ID_PCIX,
                                                         &PcixCapHeader,
                                                         sizeof(PCI_CAPABILITIES_HEADER));

                    /*
                     * A device with default cache line size and latency timer
                     * settings is considered to be unconfigured. Note that on
                     * PCI-X, the reset value of the latency timer field in the
                     * header is 64, not 0, hence why the check for PCI-X caps
                     * was required, and the value used here below.
                     */
                    if (!(PciData->LatencyTimer) ||
                        ((TempOffset) && (PciData->LatencyTimer == 64)))
                    {
                        /* Keep track of the fact that it needs configuration */
                        DPRINT1("PCI - ScanBus, PDOx %p found unconfigured\n",
                                NewExtension);
                        NewExtension->NeedsHotPlugConfiguration = TRUE;
                    }
                }
            }

            /* Save latency and cache size information */
            NewExtension->SavedLatencyTimer = PciData->LatencyTimer;
            NewExtension->SavedCacheLineSize = PciData->CacheLineSize;

            /* The PDO is now ready to go */
            DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        }
    }

    /* Enumeration completed, do a final pass now that all devices are found */
    if (ProcessFlag) PciProcessBus(DeviceExtension);
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
            DPRINT1("PCI - Old device (pdox) %p not found on rescan.\n",
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
    DPRINT1("PCI QueryDeviceRelations/BusRelations FDOx %p (bus 0x%02x)\n",
            DeviceExtension,
            DeviceExtension->BaseBus);

    /* Loop the current PDO children and the device relation object array */
    PdoExtension = DeviceExtension->ChildPdoList;
    ObjectArray = &NewRelations->Objects[NewRelations->Count];
    while (PdoExtension)
    {
        /* Dump this relation */
        DPRINT1("  QDR PDO %p (x %p)%s\n",
                PdoExtension->PhysicalDeviceObject,
                PdoExtension,
                PdoExtension->NotPresent ?
                "<Omitted, device flaged not present>" : "");

        /* Is this PDO present? */
        if (!PdoExtension->NotPresent)
        {
            /* Reference it and add it to the array */
            DeviceObject = PdoExtension->PhysicalDeviceObject;
            ObReferenceObject(DeviceObject);
            *ObjectArray++ = DeviceObject;
        }

        /* Go to the next PDO */
        PdoExtension = PdoExtension->Next;
    }

    /* Terminate dumping the relations */
    DPRINT1("  QDR Total PDO count = %u (%u already in list)\n",
            NewRelations->Count + PdoCount,
            NewRelations->Count);

    /* Return the final count and the new buffer */
    NewRelations->Count += PdoCount;
    *pDeviceRelations = NewRelations;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciSetResources(IN PPCI_PDO_EXTENSION PdoExtension,
                IN BOOLEAN DoReset,
                IN BOOLEAN SomethingSomethingDarkSide)
{
    PPCI_FDO_EXTENSION FdoExtension;
    UCHAR NewCacheLineSize, NewLatencyTimer;
    PCI_COMMON_HEADER PciData;
    BOOLEAN Native;
    PPCI_CONFIGURATOR Configurator;

    UNREFERENCED_PARAMETER(SomethingSomethingDarkSide);

    /* Get the FDO and read the configuration data */
    FdoExtension = PdoExtension->ParentFdoExtension;
    PciReadDeviceConfig(PdoExtension, &PciData, 0, PCI_COMMON_HDR_LENGTH);

    /* Make sure this is still the same device */
    if (!PcipIsSameDevice(PdoExtension, &PciData))
    {
        /* Fail */
        ASSERTMSG("PCI Set resources - not same device.\n", FALSE);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    /* Nothing to set for a host bridge */
    if ((PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
        (PdoExtension->SubClass == PCI_SUBCLASS_BR_HOST))
    {
        /* Fake success */
        return STATUS_SUCCESS;
    }

    /* Check if an IDE controller is being reset */
    if ((DoReset) &&
        (PdoExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (PdoExtension->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR))
    {
        /* Turn off native mode */
        Native = PciConfigureIdeController(PdoExtension, &PciData, FALSE);
        ASSERT(Native == PdoExtension->IDEInNativeMode);
    }

    /* Check for update of a hotplug device, or first configuration of one */
    if ((PdoExtension->NeedsHotPlugConfiguration) &&
        (FdoExtension->HotPlugParameters.Acquired))
    {
        /* Don't have hotplug devices to test with yet, QEMU 0.14 should */
        UNIMPLEMENTED_DBGBREAK();
    }

    /* Locate the correct resource configurator for this type of device */
    Configurator = &PciConfigurators[PdoExtension->HeaderType];

    /* Apply the settings change */
    Configurator->ChangeResourceSettings(PdoExtension, &PciData);

    /* Assume no update needed */
    PdoExtension->UpdateHardware = FALSE;

    /* Check if a reset is needed */
    if (DoReset)
    {
        /* Reset resources */
        Configurator->ResetDevice(PdoExtension, &PciData);
        PciData.u.type0.InterruptLine = PdoExtension->RawInterruptLine;
    }

    /* Check if the latency timer changed */
    NewLatencyTimer = PdoExtension->SavedLatencyTimer;
    if (PciData.LatencyTimer != NewLatencyTimer)
    {
        /* Debug notification */
        DPRINT1("PCI (pdox %p) changing latency from %02x to %02x.\n",
                PdoExtension,
                PciData.LatencyTimer,
                NewLatencyTimer);
    }

    /* Check if the cache line changed */
    NewCacheLineSize = PdoExtension->SavedCacheLineSize;
    if (PciData.CacheLineSize != NewCacheLineSize)
    {
        /* Debug notification */
        DPRINT1("PCI (pdox %p) changing cache line size from %02x to %02x.\n",
                PdoExtension,
                PciData.CacheLineSize,
                NewCacheLineSize);
    }

    /* Inherit data from PDO extension */
    PciData.LatencyTimer = PdoExtension->SavedLatencyTimer;
    PciData.CacheLineSize = PdoExtension->SavedCacheLineSize;
    PciData.u.type0.InterruptLine = PdoExtension->RawInterruptLine;

    /* Apply any resource hacks required */
    PciApplyHacks(FdoExtension,
                  &PciData,
                  PdoExtension->Slot,
                  PCI_HACK_FIXUP_BEFORE_UPDATE,
                  PdoExtension);

    /* Check if I/O space was disabled by administrator or driver */
    if (PdoExtension->IoSpaceNotRequired)
    {
        /* Don't turn on the decode */
        PdoExtension->CommandEnables &= ~PCI_ENABLE_IO_SPACE;
    }

    /* Update the device with the new settings */
    PciUpdateHardware(PdoExtension, &PciData);

    /* Update complete */
    PdoExtension->RawInterruptLine = PciData.u.type0.InterruptLine;
    PdoExtension->NeedsHotPlugConfiguration = FALSE;
    return STATUS_SUCCESS;
}

/* EOF */
