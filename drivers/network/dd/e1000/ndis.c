/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Driver entrypoint
 * COPYRIGHT:   Copyright 2013 Cameron Gutman (cameron.gutman@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "nic.h"

#include <debug.h>

ULONG DebugTraceLevel = MIN_TRACE;

NDIS_STATUS
NTAPI
MiniportReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext)
{
    *AddressingReset = FALSE;
    UNIMPLEMENTED_DBGBREAK();
    return NDIS_STATUS_FAILURE;
}

VOID
NTAPI
MiniportHalt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
    PE1000_ADAPTER Adapter = (PE1000_ADAPTER)MiniportAdapterContext;

    ASSERT(Adapter != NULL);

    /* First disable sending / receiving */
    NICDisableTxRx(Adapter);

    /* Then unregister interrupts */
    NICUnregisterInterrupts(Adapter);

    /* Finally, free other resources (Ports, IO ranges,...) */
    NICReleaseIoResources(Adapter);

    /* Destroy the adapter context */
    NdisFreeMemory(Adapter, sizeof(*Adapter), 0);
}

NDIS_STATUS
NTAPI
MiniportInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext)
{
    PE1000_ADAPTER Adapter;
    NDIS_STATUS Status;
    UINT i;
    PNDIS_RESOURCE_LIST ResourceList;
    UINT ResourceListSize;
    PCI_COMMON_CONFIG PciConfig;

    /* Make sure the medium is supported */
    for (i = 0; i < MediumArraySize; i++)
    {
        if (MediumArray[i] == NdisMedium802_3)
        {
            *SelectedMediumIndex = i;
            break;
        }
    }

    if (i == MediumArraySize)
    {
        NDIS_DbgPrint(MIN_TRACE, ("802.3 medium was not found in the medium array\n"));
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    ResourceList = NULL;
    ResourceListSize = 0;

    /* Allocate our adapter context */
    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter,
                                       sizeof(*Adapter),
                                       E1000_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Failed to allocate adapter context (0x%x)\n", Status));
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(Adapter, sizeof(*Adapter));
    Adapter->AdapterHandle = MiniportAdapterHandle;

    /* Notify NDIS of some characteristics of our NIC */
    NdisMSetAttributesEx(MiniportAdapterHandle,
                         Adapter,
                         0,
                         NDIS_ATTRIBUTE_BUS_MASTER,
                         NdisInterfacePci);

    NdisReadPciSlotInformation(Adapter->AdapterHandle,
                               0,
                               FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                               &PciConfig, sizeof(PciConfig));

    Adapter->VendorID = PciConfig.VendorID;
    Adapter->DeviceID = PciConfig.DeviceID;

    Adapter->SubsystemID = PciConfig.u.type0.SubSystemID;
    Adapter->SubsystemVendorID = PciConfig.u.type0.SubVendorID;


    if (!NICRecognizeHardware(Adapter))
    {
        NDIS_DbgPrint(MIN_TRACE, ("Hardware not recognized\n"));
        Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
        goto Cleanup;
    }

    /* Get our resources for IRQ and IO base information */
    NdisMQueryAdapterResources(&Status,
                               WrapperConfigurationContext,
                               ResourceList,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_RESOURCES)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unexpected failure of NdisMQueryAdapterResources (0x%x)\n", Status));
        Status = NDIS_STATUS_FAILURE;
        /* call NdisWriteErrorLogEntry */
        goto Cleanup;
    }

    Status = NdisAllocateMemoryWithTag((PVOID*)&ResourceList,
                                ResourceListSize,
                                E1000_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Failed to allocate resource list (0x%x)\n", Status));
        /* call NdisWriteErrorLogEntry */
        goto Cleanup;
    }

    NdisMQueryAdapterResources(&Status,
                               WrapperConfigurationContext,
                               ResourceList,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unexpected failure of NdisMQueryAdapterResources (0x%x)\n", Status));
        /* call NdisWriteErrorLogEntry */
        goto Cleanup;
    }

    ASSERT(ResourceList->Version == 1);
    ASSERT(ResourceList->Revision == 1);

    Status = NICInitializeAdapterResources(Adapter, ResourceList);

    NdisFreeMemory(ResourceList, ResourceListSize, 0);
    ResourceList = NULL;

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Adapter didn't receive enough resources\n"));
        goto Cleanup;
    }

    /* Allocate the DMA resources */
    Status = NdisMInitializeScatterGatherDma(MiniportAdapterHandle,
                                             FALSE, // 64bit is supported but can be buggy
                                             MAXIMUM_FRAME_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to configure DMA\n"));
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    Status = NICAllocateIoResources(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to allocate resources\n"));
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    /* Adapter setup */
    Status = NICPowerOn(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to power on NIC (0x%x)\n", Status));
        goto Cleanup;
    }

    Status = NICSoftReset(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to reset the NIC (0x%x)\n", Status));
        goto Cleanup;
    }

    Status = NICGetPermanentMacAddress(Adapter, Adapter->PermanentMacAddress);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to get the fixed MAC address (0x%x)\n", Status));
        goto Cleanup;
    }

    RtlCopyMemory(Adapter->MulticastList[0].MacAddress, Adapter->PermanentMacAddress, IEEE_802_ADDR_LENGTH);

    NICUpdateMulticastList(Adapter);

    /* Update link state and speed */
    NICUpdateLinkStatus(Adapter);

    /* We're ready to handle interrupts now */
    Status = NICRegisterInterrupts(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to register interrupt (0x%x)\n", Status));
        goto Cleanup;
    }

    /* Enable interrupts on the NIC */
    Adapter->InterruptMask = DEFAULT_INTERRUPT_MASK;
    NICApplyInterruptMask(Adapter);

    /* Turn on TX and RX now */
    Status = NICEnableTxRx(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Unable to enable TX and RX (0x%x)\n", Status));
        goto Cleanup;
    }

    return NDIS_STATUS_SUCCESS;

Cleanup:
    if (ResourceList != NULL)
    {
        NdisFreeMemory(ResourceList, ResourceListSize, 0);
    }

    MiniportHalt(Adapter);

    return Status;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NDIS_HANDLE WrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS Characteristics = { 0 };
    NDIS_STATUS Status;

    Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    Characteristics.CheckForHangHandler = NULL;
    Characteristics.DisableInterruptHandler = NULL;
    Characteristics.EnableInterruptHandler = NULL;
    Characteristics.HaltHandler = MiniportHalt;
    Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
    Characteristics.InitializeHandler = MiniportInitialize;
    Characteristics.ISRHandler = MiniportISR;
    Characteristics.QueryInformationHandler = MiniportQueryInformation;
    Characteristics.ReconfigureHandler = NULL;
    Characteristics.ResetHandler = MiniportReset;
    Characteristics.SendHandler = MiniportSend;
    Characteristics.SetInformationHandler = MiniportSetInformation;
    Characteristics.TransferDataHandler = NULL;
    Characteristics.ReturnPacketHandler = NULL;
    Characteristics.SendPacketsHandler = NULL;
    Characteristics.AllocateCompleteHandler = NULL;

    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    if (!WrapperHandle)
    {
        return NDIS_STATUS_FAILURE;
    }

    Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(WrapperHandle, 0);
        return NDIS_STATUS_FAILURE;
    }

    return NDIS_STATUS_SUCCESS;
}
