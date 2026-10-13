/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Miniport interface code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static
NTSTATUS
InitializeConfiguration(
    _In_ PPORT_CONFIGURATION_INFORMATION PortConfig,
    _In_ PHW_INITIALIZATION_DATA InitData,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber)
{
    PCONFIGURATION_INFORMATION ConfigInfo;
    ULONG i;

    DPRINT1("InitializeConfiguration(%p %p %lu %lu)\n",
            PortConfig, InitData, BusNumber, SlotNumber);

    /* Get the configurration information */
    ConfigInfo = IoGetConfigurationInformation();

    /* Initialize the port configuration */
    RtlZeroMemory(PortConfig,
                  sizeof(PORT_CONFIGURATION_INFORMATION));

    PortConfig->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
    PortConfig->SystemIoBusNumber = BusNumber;
    PortConfig->SlotNumber = SlotNumber;
    PortConfig->AdapterInterfaceType = InitData->AdapterInterfaceType;

    PortConfig->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
    PortConfig->DmaChannel = SP_UNINITIALIZED_VALUE;
    PortConfig->DmaPort = SP_UNINITIALIZED_VALUE;

    PortConfig->InterruptMode = LevelSensitive;

    PortConfig->Master = TRUE;
    PortConfig->AtdiskPrimaryClaimed = ConfigInfo->AtDiskPrimaryAddressClaimed;
    PortConfig->AtdiskSecondaryClaimed = ConfigInfo->AtDiskSecondaryAddressClaimed;
    PortConfig->Dma32BitAddresses = TRUE;
    PortConfig->DemandMode = FALSE;
    PortConfig->MapBuffers = InitData->MapBuffers;

    PortConfig->NeedPhysicalAddresses = TRUE;
    PortConfig->TaggedQueuing = TRUE;
    PortConfig->AutoRequestSense = TRUE;
    PortConfig->MultipleRequestPerLu = TRUE;
    PortConfig->ReceiveEvent = InitData->ReceiveEvent;
    PortConfig->RealModeInitialized = FALSE;
    PortConfig->BufferAccessScsiPortControlled = TRUE;
    PortConfig->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS_PER_BUS;

    PortConfig->SpecificLuExtensionSize = InitData->SpecificLuExtensionSize;
    PortConfig->SrbExtensionSize = InitData->SrbExtensionSize;
    PortConfig->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;
    PortConfig->WmiDataProvider = TRUE;

    PortConfig->NumberOfAccessRanges = InitData->NumberOfAccessRanges;
    DPRINT1("NumberOfAccessRanges: %lu\n", PortConfig->NumberOfAccessRanges);
    if (PortConfig->NumberOfAccessRanges != 0)
    {
        PortConfig->AccessRanges = ExAllocatePoolWithTag(NonPagedPool,
                                                         PortConfig->NumberOfAccessRanges * sizeof(ACCESS_RANGE),
                                                         TAG_ACCRESS_RANGE);
        if (PortConfig->AccessRanges == NULL)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(PortConfig->AccessRanges,
                      PortConfig->NumberOfAccessRanges * sizeof(ACCESS_RANGE));
    }

    for (i = 0; i < RTL_NUMBER_OF(PortConfig->InitiatorBusId); i++)
    {
        PortConfig->InitiatorBusId[i] = (CCHAR)SP_UNINITIALIZED_VALUE;
    }

    /* Default per-adapter outstanding IO limit, as per storport documentation. */
    PortConfig->MaxNumberOfIO = 1000;

    /* Only a Win8 or later init data block carries the request format flags */
    PortConfig->SrbType = SRB_TYPE_SCSI_REQUEST_BLOCK;
    if (InitData->HwInitializationDataSize >=
        RTL_SIZEOF_THROUGH_FIELD(HW_INITIALIZATION_DATA, SrbTypeFlags) &&
        TEST_FLAG(InitData->SrbTypeFlags, SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK))
    {
        PortConfig->SrbType = SRB_TYPE_STORAGE_REQUEST_BLOCK;
    }

    /* Bus, target and LUN is the only addressing we hand out */
    PortConfig->AddressType = STORAGE_ADDRESS_TYPE_BTL8;

    DPRINT1("SrbType: %u\n", PortConfig->SrbType);

    return STATUS_SUCCESS;
}


static
VOID
AssignResourcesToConfiguration(
    _In_ PPORT_CONFIGURATION_INFORMATION PortConfiguration,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ULONG NumberOfAccessRanges)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PACCESS_RANGE AccessRange;
    INT i, j;
    ULONG RangeNumber = 0, Interrupt = 0, Dma = 0;

    DPRINT1("AssignResourceToConfiguration(%p %p %lu)\n",
            PortConfiguration, ResourceList, NumberOfAccessRanges);

    FullDescriptor = &ResourceList->List[0];
    for (i = 0; i < ResourceList->Count; i++)
    {
        PartialResourceList = &FullDescriptor->PartialResourceList;

        for (j = 0; j < PartialResourceList->Count; j++)
        {
            PartialDescriptor = &PartialResourceList->PartialDescriptors[j];

            switch (PartialDescriptor->Type)
            {
                case CmResourceTypePort:
                    DPRINT1("Port: 0x%I64x (0x%lx)\n",
                            PartialDescriptor->u.Port.Start.QuadPart,
                            PartialDescriptor->u.Port.Length);
                    if (RangeNumber < NumberOfAccessRanges)
                    {
                        AccessRange = &((*(PortConfiguration->AccessRanges))[RangeNumber]);
                        AccessRange->RangeStart = PartialDescriptor->u.Port.Start;
                        AccessRange->RangeLength = PartialDescriptor->u.Port.Length;
                        AccessRange->RangeInMemory = FALSE;
                        RangeNumber++;
                    }
                    break;

                case CmResourceTypeMemory:
                    DPRINT1("Memory: 0x%I64x (0x%lx)\n",
                            PartialDescriptor->u.Memory.Start.QuadPart,
                            PartialDescriptor->u.Memory.Length);
                    if (RangeNumber < NumberOfAccessRanges)
                    {
                        AccessRange = &((*(PortConfiguration->AccessRanges))[RangeNumber]);
                        AccessRange->RangeStart = PartialDescriptor->u.Memory.Start;
                        AccessRange->RangeLength = PartialDescriptor->u.Memory.Length;
                        AccessRange->RangeInMemory = TRUE;
                        RangeNumber++;
                    }
                    break;

                case CmResourceTypeInterrupt:
                    DPRINT1("Interrupt: Level %lu  Vector %lu\n",
                            PartialDescriptor->u.Interrupt.Level,
                            PartialDescriptor->u.Interrupt.Vector);
                    if (Interrupt == 0)
                    {
                        /* Copy interrupt data */
                        PortConfiguration->BusInterruptLevel = PartialDescriptor->u.Interrupt.Level;
                        PortConfiguration->BusInterruptVector = PartialDescriptor->u.Interrupt.Vector;

                        /* Set interrupt mode accordingly to the resource */
                        if (PartialDescriptor->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
                        {
                            PortConfiguration->InterruptMode = Latched;
                        }
                        else if (PartialDescriptor->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
                        {
                            PortConfiguration->InterruptMode = LevelSensitive;
                        }
                    }
                    else if (Interrupt == 1)
                    {
                        /* Copy interrupt data */
                        PortConfiguration->BusInterruptLevel2 = PartialDescriptor->u.Interrupt.Level;
                        PortConfiguration->BusInterruptVector2 = PartialDescriptor->u.Interrupt.Vector;

                        /* Set interrupt mode accordingly to the resource */
                        if (PartialDescriptor->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
                        {
                            PortConfiguration->InterruptMode2 = Latched;
                        }
                        else if (PartialDescriptor->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
                        {
                            PortConfiguration->InterruptMode2 = LevelSensitive;
                        }
                    }
                    Interrupt++;
                    break;

                case CmResourceTypeDma:
                    DPRINT1("Dma: Channel: %lu  Port: %lu\n",
                            PartialDescriptor->u.Dma.Channel,
                            PartialDescriptor->u.Dma.Port);
                    if (Dma == 0)
                    {
                        PortConfiguration->DmaChannel = PartialDescriptor->u.Dma.Channel;
                        PortConfiguration->DmaPort = PartialDescriptor->u.Dma.Port;

                        if (PartialDescriptor->Flags & CM_RESOURCE_DMA_8)
                            PortConfiguration->DmaWidth = Width8Bits;
                        else if ((PartialDescriptor->Flags & CM_RESOURCE_DMA_16) ||
                                 (PartialDescriptor->Flags & CM_RESOURCE_DMA_8_AND_16))
                            PortConfiguration->DmaWidth = Width16Bits;
                        else if (PartialDescriptor->Flags & CM_RESOURCE_DMA_32)
                            PortConfiguration->DmaWidth = Width32Bits;
                    }
                    else if (Dma == 1)
                    {
                        PortConfiguration->DmaChannel2 = PartialDescriptor->u.Dma.Channel;
                        PortConfiguration->DmaPort2 = PartialDescriptor->u.Dma.Port;

                        if (PartialDescriptor->Flags & CM_RESOURCE_DMA_8)
                            PortConfiguration->DmaWidth2 = Width8Bits;
                        else if ((PartialDescriptor->Flags & CM_RESOURCE_DMA_16) ||
                                 (PartialDescriptor->Flags & CM_RESOURCE_DMA_8_AND_16))
                            PortConfiguration->DmaWidth2 = Width16Bits;
                        else if (PartialDescriptor->Flags & CM_RESOURCE_DMA_32)
                            PortConfiguration->DmaWidth2 = Width32Bits;
                    }
                    Dma++;
                    break;

                default:
                    DPRINT1("Other: %u\n", PartialDescriptor->Type);
                    break;
            }
        }

        /* Advance to next CM_FULL_RESOURCE_DESCRIPTOR block in memory. */
        FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)(FullDescriptor->PartialResourceList.PartialDescriptors +
                                                        FullDescriptor->PartialResourceList.Count);
    }
}


NTSTATUS
MiniportInitialize(
    _In_ PMINIPORT Miniport,
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PHW_INITIALIZATION_DATA InitData)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    ULONG Size;
    NTSTATUS Status;

    DPRINT1("MiniportInitialize(%p %p %p)\n",
            Miniport, DeviceExtension, InitData);

    Miniport->DeviceExtension = DeviceExtension;
    Miniport->InitData = InitData;

    /* Calculate the miniport device extension size */
    Status = RtlULongAdd(sizeof(*MiniportExtension),
                         Miniport->InitData->DeviceExtensionSize,
                         &Size);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid miniport extension size %lx\n", Miniport->InitData->DeviceExtensionSize);
        return Status;
    }

    /* Allocate and initialize the miniport device extension */
    MiniportExtension = ExAllocatePoolWithTag(NonPagedPool,
                                              Size,
                                              TAG_MINIPORT_DATA);
    if (MiniportExtension == NULL)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(MiniportExtension, Size);

    MiniportExtension->Miniport = Miniport;
    Miniport->MiniportExtension = MiniportExtension;

    /* Initialize the port configuration */
    Status = InitializeConfiguration(&Miniport->PortConfig,
                                     InitData,
                                     DeviceExtension->BusNumber,
                                     DeviceExtension->SlotNumber);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Assign the resources to the port configuration */
    AssignResourcesToConfiguration(&Miniport->PortConfig,
                                   DeviceExtension->AllocatedResources,
                                   InitData->NumberOfAccessRanges);

    return STATUS_SUCCESS;
}


NTSTATUS
MiniportFindAdapter(
    _In_ PMINIPORT Miniport)
{
    BOOLEAN Reserved = FALSE;
    PHW_FIND_ADAPTER FindAdapter;
    ULONG Result;
    NTSTATUS Status;

    DPRINT1("MiniportFindAdapter(%p)\n", Miniport);

    /* HwFindAdapter is typeless since Win8, we only drive physical miniports */
    FindAdapter = (PHW_FIND_ADAPTER)Miniport->InitData->HwFindAdapter;

    Result = FindAdapter(&Miniport->MiniportExtension->HwDeviceExtension,
                         NULL,
                         NULL,
                         NULL,
                         &Miniport->PortConfig,
                         &Reserved);
    DPRINT1("HwFindAdapter() returned %lu\n", Result);

    /* Convert the result to a status code */
    switch (Result)
    {
        case SP_RETURN_NOT_FOUND:
            DPRINT1("SP_RETURN_NOT_FOUND\n");
            Status = STATUS_NOT_FOUND;
            break;

        case SP_RETURN_FOUND:
            DPRINT1("SP_RETURN_FOUND\n");
            Status = STATUS_SUCCESS;
            break;

        case SP_RETURN_ERROR:
            DPRINT1("SP_RETURN_ERROR\n");
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
            break;

        case SP_RETURN_BAD_CONFIG:
            DPRINT1("SP_RETURN_BAD_CONFIG\n");
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
            break;

        default:
            DPRINT1("Unknown result: %lu\n", Result);
            Status = STATUS_INTERNAL_ERROR;
            break;
    }

    return Status;
}


NTSTATUS
MiniportHwInitialize(
    _In_ PMINIPORT Miniport)
{
    BOOLEAN Result;

    DPRINT("MiniportHwInitialize(%p)\n", Miniport);

    /* Call the miniport HwInitialize routine */
    Result = Miniport->InitData->HwInitialize(&Miniport->MiniportExtension->HwDeviceExtension);
    DPRINT("HwInitialize() returned %u\n", Result);

    return Result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


BOOLEAN
MiniportHwInterrupt(
    _In_ PMINIPORT Miniport)
{
    BOOLEAN Result;

    DPRINT("MiniportHwInterrupt(%p)\n",
            Miniport);

    Result = Miniport->InitData->HwInterrupt(&Miniport->MiniportExtension->HwDeviceExtension);
    DPRINT("HwInterrupt() returned %u\n", Result);

    return Result;
}


/**
 * @brief Asks the miniport which adapter control requests it handles.
 */
VOID
NTAPI
MiniportQueryAdapterControl(
    _In_ PMINIPORT Miniport)
{
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST List;
    SCSI_ADAPTER_CONTROL_STATUS Result;
    ULONG Length;
    ULONG Type;

    RtlZeroMemory(Miniport->AdapterControlSupported,
                  sizeof(Miniport->AdapterControlSupported));

    if (Miniport->InitData->HwAdapterControl == NULL)
        return;

    Length = sizeof(*List) +
             (ScsiAdapterControlMax * sizeof(List->SupportedTypeList[0]));

    List = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_MINIPORT_DATA);
    if (List == NULL)
        return;

    RtlZeroMemory(List, Length);
    List->MaxControlType = ScsiAdapterControlMax;

    Result = Miniport->InitData->HwAdapterControl(&Miniport->MiniportExtension->HwDeviceExtension,
                                                  ScsiQuerySupportedControlTypes,
                                                  List);
    if (Result == ScsiAdapterControlSuccess)
    {
        RtlCopyMemory(Miniport->AdapterControlSupported,
                      List->SupportedTypeList,
                      sizeof(Miniport->AdapterControlSupported));

        for (Type = 0; Type < ScsiAdapterControlMax; Type++)
        {
            if (Miniport->AdapterControlSupported[Type])
                DPRINT1("Adapter control %lu supported\n", Type);
        }
    }

    ExFreePoolWithTag(List, TAG_MINIPORT_DATA);
}


/**
 * @brief Sends one adapter control request, if the miniport claimed it.
 */
SCSI_ADAPTER_CONTROL_STATUS
NTAPI
MiniportAdapterControl(
    _In_ PMINIPORT Miniport,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_opt_ PVOID Parameters)
{
    DPRINT("MiniportAdapterControl(%p %u %p)\n",
            Miniport, ControlType, Parameters);

    if (ControlType >= ScsiAdapterControlMax ||
        !Miniport->AdapterControlSupported[ControlType])
    {
        return ScsiAdapterControlUnsuccessful;
    }

    return Miniport->InitData->HwAdapterControl(&Miniport->MiniportExtension->HwDeviceExtension,
                                                ControlType,
                                                Parameters);
}


BOOLEAN
NTAPI
MiniportHwMSInterrupt(
    _In_ PMINIPORT Miniport,
    _In_ ULONG MessageId)
{
    BOOLEAN Result;

    DPRINT("MiniportHwMSInterrupt(%p %lu)\n",
            Miniport, MessageId);

    Result = Miniport->PortConfig.HwMSInterruptRoutine(&Miniport->MiniportExtension->HwDeviceExtension,
                                                       MessageId);
    DPRINT("HwMSInterruptRoutine() returned %u\n", Result);

    return Result;
}


BOOLEAN
MiniportStartIo(
    _In_ PMINIPORT Miniport,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    BOOLEAN Result;

    DPRINT("MiniportHwStartIo(%p %p)\n",
            Miniport, Srb);

    Result = Miniport->InitData->HwStartIo(&Miniport->MiniportExtension->HwDeviceExtension, Srb);
    DPRINT("HwStartIo() returned %u\n", Result);

    return Result;
}

/* EOF */
