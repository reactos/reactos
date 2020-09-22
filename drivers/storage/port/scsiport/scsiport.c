/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Storage Stack
 * FILE:            drivers/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl
 *                  Aleksey Bragin (aleksey reactos org)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <ntddk.h>
#include <stdio.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <mountdev.h>

#define NDEBUG
#include <debug.h>

#include "scsiport_int.h"

ULONG InternalDebugLevel = 0x00;

#undef ScsiPortMoveMemory

/* GLOBALS *******************************************************************/

static BOOLEAN
SpiGetPciConfigData(IN PDRIVER_OBJECT DriverObject,
                    IN PDEVICE_OBJECT DeviceObject,
                    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
                    IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
                    IN PUNICODE_STRING RegistryPath,
                    IN ULONG BusNumber,
                    IN OUT PPCI_SLOT_NUMBER NextSlotNumber);

static NTSTATUS NTAPI
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp);

static DRIVER_DISPATCH ScsiPortDispatchScsi;
static NTSTATUS NTAPI
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS NTAPI
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp);

static DRIVER_STARTIO ScsiPortStartIo;
static VOID NTAPI
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);

static BOOLEAN NTAPI
ScsiPortStartPacket(IN OUT PVOID Context);

IO_ALLOCATION_ACTION
NTAPI
SpiAdapterControl(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PVOID MapRegisterBase, PVOID Context);

static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun);

static PSCSI_REQUEST_BLOCK_INFO
SpiAllocateSrbStructures(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         PSCSI_PORT_LUN_EXTENSION LunExtension,
                         PSCSI_REQUEST_BLOCK Srb);

static NTSTATUS
SpiSendInquiry(IN PDEVICE_OBJECT DeviceObject,
               IN OUT PSCSI_LUN_INFO LunInfo);

static VOID
SpiScanAdapter(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static NTSTATUS
SpiGetInquiryData (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   IN PIRP Irp);

static PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
              IN UCHAR PathId,
              IN UCHAR TargetId,
              IN UCHAR Lun,
              IN UCHAR QueueTag);

static BOOLEAN NTAPI
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext);

static VOID NTAPI
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext);

static VOID NTAPI
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context);

IO_ALLOCATION_ACTION
NTAPI
ScsiPortAllocateAdapterChannel(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp,
                               IN PVOID MapRegisterBase,
                               IN PVOID Context);

static NTSTATUS
SpiBuildDeviceMap(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                  IN PUNICODE_STRING RegistryPath);

static NTSTATUS
SpiStatusSrbToNt(UCHAR SrbStatus);

static VOID
SpiSendRequestSense(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    IN PSCSI_REQUEST_BLOCK Srb);

static IO_COMPLETION_ROUTINE SpiCompletionRoutine;
NTSTATUS NTAPI
SpiCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp,
                     PVOID Context);

static VOID
NTAPI
SpiProcessCompletedRequest(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                           IN PSCSI_REQUEST_BLOCK_INFO SrbInfo,
                           OUT PBOOLEAN NeedToCallStartIo);

VOID NTAPI
SpiGetNextRequestFromLun(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         IN PSCSI_PORT_LUN_EXTENSION LunExtension);

VOID NTAPI
SpiMiniportTimerDpc(IN struct _KDPC *Dpc,
                    IN PVOID DeviceObject,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2);

static NTSTATUS
SpiCreatePortConfig(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    PHW_INITIALIZATION_DATA HwInitData,
                    PCONFIGURATION_INFO InternalConfigInfo,
                    PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                    BOOLEAN FirstCall);

NTSTATUS NTAPI
SpQueryDeviceCallout(IN PVOID  Context,
                     IN PUNICODE_STRING  PathName,
                     IN INTERFACE_TYPE  BusType,
                     IN ULONG  BusNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *BusInformation,
                     IN CONFIGURATION_TYPE  ControllerType,
                     IN ULONG  ControllerNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *ControllerInformation,
                     IN CONFIGURATION_TYPE  PeripheralType,
                     IN ULONG  PeripheralNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *PeripheralInformation);

static VOID
SpiParseDeviceInfo(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                   IN HANDLE Key,
                   IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                   IN PCONFIGURATION_INFO InternalConfigInfo,
                   IN PUCHAR Buffer);

static VOID
SpiResourceToConfig(IN PHW_INITIALIZATION_DATA HwInitializationData,
                    IN PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor,
                    IN PPORT_CONFIGURATION_INFORMATION PortConfig);

static PCM_RESOURCE_LIST
SpiConfigToResource(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    PPORT_CONFIGURATION_INFORMATION PortConfig);

static VOID
SpiCleanupAfterInit(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static NTSTATUS
SpiHandleAttachRelease(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                       PIRP Irp);

static NTSTATUS
SpiAllocateCommonBuffer(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension, ULONG NonCachedSize);

NTHALAPI ULONG NTAPI HalGetBusData(BUS_DATA_TYPE, ULONG, ULONG, PVOID, ULONG);
NTHALAPI ULONG NTAPI HalGetInterruptVector(INTERFACE_TYPE, ULONG, ULONG, ULONG, PKIRQL, PKAFFINITY);
NTHALAPI NTSTATUS NTAPI HalAssignSlotResources(PUNICODE_STRING, PUNICODE_STRING, PDRIVER_OBJECT, PDEVICE_OBJECT, INTERFACE_TYPE, ULONG, ULONG, PCM_RESOURCE_LIST *);

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURN VALUE
 * 	Status.
 */

NTSTATUS NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
    DPRINT("ScsiPort Driver %s\n", VERSION);
    return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiDebugPrint
 *
 * DESCRIPTION
 *	Prints debugging messages.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DebugPrintLevel
 *		Debug level of the given message.
 *
 *	DebugMessage
 *		Pointer to printf()-compatible format string.
 *
 *	...
  		Additional output data (see printf()).
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */

VOID
ScsiDebugPrint(IN ULONG DebugPrintLevel,
	       IN PCHAR DebugMessage,
	       ...)
{
    char Buffer[256];
    va_list ap;

    if (DebugPrintLevel > InternalDebugLevel)
        return;

    va_start(ap, DebugMessage);
    vsprintf(Buffer, DebugMessage, ap);
    va_end(ap);

    DbgPrint(Buffer);
}

/* An internal helper function for ScsiPortCompleteRequest */
VOID
NTAPI
SpiCompleteRequest(IN PVOID HwDeviceExtension,
                   IN PSCSI_REQUEST_BLOCK_INFO SrbInfo,
                   IN UCHAR SrbStatus)
{
    PSCSI_REQUEST_BLOCK Srb;

    /* Get current SRB */
    Srb = SrbInfo->Srb;

    /* Return if there is no SRB or it is not active */
    if (!Srb || !(Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) return;

    /* Set status */
    Srb->SrbStatus = SrbStatus;

    /* Set data transfered to 0 */
    Srb->DataTransferLength = 0;

    /* Notify */
    ScsiPortNotification(RequestComplete,
                         HwDeviceExtension,
                         Srb);
}

/*
 * @unimplemented
 */
VOID NTAPI
ScsiPortCompleteRequest(IN PVOID HwDeviceExtension,
                        IN UCHAR PathId,
                        IN UCHAR TargetId,
                        IN UCHAR Lun,
                        IN UCHAR SrbStatus)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    PLIST_ENTRY ListEntry;
    ULONG BusNumber;
    ULONG Target;

    DPRINT("ScsiPortCompleteRequest() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    /* Go through all buses */
    for (BusNumber = 0; BusNumber < 8; BusNumber++)
    {
        /* Go through all targets */
        for (Target = 0; Target < DeviceExtension->MaxTargedIds; Target++)
        {
            /* Get logical unit list head */
            LunExtension = DeviceExtension->LunExtensionList[Target % 8];

            /* Go through all logical units */
            while (LunExtension)
            {
                /* Now match what caller asked with what we are at now */
                if ((PathId == SP_UNTAGGED || PathId == LunExtension->PathId) &&
                    (TargetId == SP_UNTAGGED || TargetId == LunExtension->TargetId) &&
                    (Lun == SP_UNTAGGED || Lun == LunExtension->Lun))
                {
                    /* Yes, that's what caller asked for. Complete abort requests */
                    if (LunExtension->CompletedAbortRequests)
                    {
                        /* TODO: Save SrbStatus in this request */
                        DPRINT1("Completing abort request without setting SrbStatus!\n");

                        /* Issue a notification request */
                        ScsiPortNotification(RequestComplete,
                                             HwDeviceExtension,
                                             LunExtension->CompletedAbortRequests);
                    }

                    /* Complete the request using our helper */
                    SpiCompleteRequest(HwDeviceExtension,
                                       &LunExtension->SrbInfo,
                                       SrbStatus);

                    /* Go through the queue and complete everything there too */
                    ListEntry = LunExtension->SrbInfo.Requests.Flink;
                    while (ListEntry != &LunExtension->SrbInfo.Requests)
                    {
                        /* Get the actual SRB info entry */
                        SrbInfo = CONTAINING_RECORD(ListEntry,
                                                    SCSI_REQUEST_BLOCK_INFO,
                                                    Requests);

                        /* Complete it */
                        SpiCompleteRequest(HwDeviceExtension,
                                           SrbInfo,
                                           SrbStatus);

                        /* Advance to the next request in queue */
                        ListEntry = SrbInfo->Requests.Flink;
                    }
                }

                /* Advance to the next one */
                LunExtension = LunExtension->Next;
            }
        }
    }
}

/*
 * @unimplemented
 */
VOID NTAPI
ScsiPortFlushDma(IN PVOID HwDeviceExtension)
{
  DPRINT("ScsiPortFlushDma()\n");
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID NTAPI
ScsiPortFreeDeviceBase(IN PVOID HwDeviceExtension,
		       IN PVOID MappedAddress)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PMAPPED_ADDRESS NextMa, LastMa;

    //DPRINT("ScsiPortFreeDeviceBase() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    /* Initialize our pointers */
    NextMa = DeviceExtension->MappedAddressList;
    LastMa = NextMa;

    while (NextMa)
    {
        if (NextMa->MappedAddress == MappedAddress)
        {
            /* Unmap it first */
            MmUnmapIoSpace(MappedAddress, NextMa->NumberOfBytes);

            /* Remove it from the list */
            if (NextMa == DeviceExtension->MappedAddressList)
            {
                /* Remove the first entry */
                DeviceExtension->MappedAddressList = NextMa->NextMappedAddress;
            }
            else
            {
                LastMa->NextMappedAddress = NextMa->NextMappedAddress;
            }

            /* Free the resources and quit */
            ExFreePool(NextMa);

            return;
        }
        else
        {
            LastMa = NextMa;
            NextMa = NextMa->NextMappedAddress;
        }
    }
}


/*
 * @implemented
 */
ULONG NTAPI
ScsiPortGetBusData(IN PVOID DeviceExtension,
		   IN ULONG BusDataType,
		   IN ULONG SystemIoBusNumber,
		   IN ULONG SlotNumber,
		   IN PVOID Buffer,
		   IN ULONG Length)
{
    DPRINT("ScsiPortGetBusData()\n");

    if (Length)
    {
        /* If Length is non-zero, just forward the call to
        HalGetBusData() function */
        return HalGetBusData(BusDataType,
                             SystemIoBusNumber,
                             SlotNumber,
                             Buffer,
                             Length);
    }

    /* We have a more complex case here */
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
ULONG NTAPI
ScsiPortSetBusDataByOffset(IN PVOID DeviceExtension,
                           IN ULONG BusDataType,
                           IN ULONG SystemIoBusNumber,
                           IN ULONG SlotNumber,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length)
{
    DPRINT("ScsiPortSetBusDataByOffset()\n");
    return HalSetBusDataByOffset(BusDataType,
                                 SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);
}

/*
 * @implemented
 */
PVOID NTAPI
ScsiPortGetDeviceBase(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PHYSICAL_ADDRESS TranslatedAddress;
    PMAPPED_ADDRESS DeviceBase;
    ULONG AddressSpace;
    PVOID MappedAddress;

    //DPRINT ("ScsiPortGetDeviceBase() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    AddressSpace = (ULONG)InIoSpace;
    if (HalTranslateBusAddress(BusType,
                               SystemIoBusNumber,
                               IoAddress,
                               &AddressSpace,
                               &TranslatedAddress) == FALSE)
    {
        return NULL;
    }

    /* i/o space */
    if (AddressSpace != 0)
        return((PVOID)(ULONG_PTR)TranslatedAddress.QuadPart);

    MappedAddress = MmMapIoSpace(TranslatedAddress,
                                 NumberOfBytes,
                                 FALSE);

    DeviceBase = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(MAPPED_ADDRESS), TAG_SCSIPORT);

    if (DeviceBase == NULL)
        return MappedAddress;

    DeviceBase->MappedAddress = MappedAddress;
    DeviceBase->NumberOfBytes = NumberOfBytes;
    DeviceBase->IoAddress = IoAddress;
    DeviceBase->BusNumber = SystemIoBusNumber;

    /* Link it to the Device Extension list */
    DeviceBase->NextMappedAddress = DeviceExtension->MappedAddressList;
    DeviceExtension->MappedAddressList = DeviceBase;

    return MappedAddress;
}

/*
 * @unimplemented
 */
PVOID NTAPI
ScsiPortGetLogicalUnit(IN PVOID HwDeviceExtension,
		       IN UCHAR PathId,
		       IN UCHAR TargetId,
		       IN UCHAR Lun)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    DPRINT("ScsiPortGetLogicalUnit() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    /* Check the extension size */
    if (!DeviceExtension->LunExtensionSize)
    {
        /* They didn't want one */
        return NULL;
    }

    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      PathId,
                                      TargetId,
                                      Lun);
    /* Check that the logical unit exists */
    if (!LunExtension)
    {
        /* Nope, return NULL */
        return NULL;
    }

    /* Return the logical unit miniport extension */
    return (LunExtension + 1);
}


/*
 * @implemented
 */
SCSI_PHYSICAL_ADDRESS NTAPI
ScsiPortGetPhysicalAddress(IN PVOID HwDeviceExtension,
                           IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
                           IN PVOID VirtualAddress,
                           OUT ULONG *Length)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    SCSI_PHYSICAL_ADDRESS PhysicalAddress;
    SIZE_T BufferLength = 0;
    ULONG_PTR Offset;
    PSCSI_SG_ADDRESS SGList;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;

    DPRINT("ScsiPortGetPhysicalAddress(%p %p %p %p)\n",
        HwDeviceExtension, Srb, VirtualAddress, Length);

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    if (Srb == NULL || Srb->SenseInfoBuffer == VirtualAddress)
    {
        /* Simply look it up in the allocated common buffer */
        Offset = (PUCHAR)VirtualAddress - (PUCHAR)DeviceExtension->SrbExtensionBuffer;

        BufferLength = DeviceExtension->CommonBufferLength - Offset;
        PhysicalAddress.QuadPart = DeviceExtension->PhysicalAddress.QuadPart + Offset;
    }
    else if (DeviceExtension->MapRegisters)
    {
        /* Scatter-gather list must be used */
        SrbInfo = SpiGetSrbData(DeviceExtension,
                               Srb->PathId,
                               Srb->TargetId,
                               Srb->Lun,
                               Srb->QueueTag);

        SGList = SrbInfo->ScatterGather;

        /* Find needed item in the SG list */
        Offset = (PCHAR)VirtualAddress - (PCHAR)Srb->DataBuffer;
        while (Offset >= SGList->Length)
        {
            Offset -= SGList->Length;
            SGList++;
        }

        /* We're done, store length and physical address */
        BufferLength = SGList->Length - Offset;
        PhysicalAddress.QuadPart = SGList->PhysicalAddress.QuadPart + Offset;
    }
    else
    {
        /* Nothing */
        PhysicalAddress.QuadPart = (LONGLONG)(SP_UNINITIALIZED_VALUE);
    }

    *Length = (ULONG)BufferLength;
    return PhysicalAddress;
}


/*
 * @unimplemented
 */
PSCSI_REQUEST_BLOCK NTAPI
ScsiPortGetSrb(IN PVOID DeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  DPRINT1("ScsiPortGetSrb() unimplemented\n");
  UNIMPLEMENTED;
  return NULL;
}


/*
 * @implemented
 */
PVOID NTAPI
ScsiPortGetUncachedExtension(IN PVOID HwDeviceExtension,
			     IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     IN ULONG NumberOfBytes)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    DEVICE_DESCRIPTION DeviceDescription;
    ULONG MapRegistersCount;
    NTSTATUS Status;

    DPRINT("ScsiPortGetUncachedExtension(%p %p %lu)\n",
        HwDeviceExtension, ConfigInfo, NumberOfBytes);

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    /* Check for allocated common DMA buffer */
    if (DeviceExtension->SrbExtensionBuffer != NULL)
    {
        DPRINT1("The HBA has already got a common DMA buffer!\n");
        return NULL;
    }

    /* Check for DMA adapter object */
    if (DeviceExtension->AdapterObject == NULL)
    {
        /* Initialize DMA adapter description */
        RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

        DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        DeviceDescription.Master = ConfigInfo->Master;
        DeviceDescription.ScatterGather = ConfigInfo->ScatterGather;
        DeviceDescription.DemandMode = ConfigInfo->DemandMode;
        DeviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
        DeviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
        DeviceDescription.DmaChannel = ConfigInfo->DmaChannel;
        DeviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
        DeviceDescription.DmaWidth = ConfigInfo->DmaWidth;
        DeviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
        DeviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
        DeviceDescription.DmaPort = ConfigInfo->DmaPort;

        /* Get a DMA adapter object */
        DeviceExtension->AdapterObject =
            HalGetAdapter(&DeviceDescription, &MapRegistersCount);

        /* Fail in case of error */
        if (DeviceExtension->AdapterObject == NULL)
        {
            DPRINT1("HalGetAdapter() failed\n");
            return NULL;
        }

        /* Set number of physical breaks */
        if (ConfigInfo->NumberOfPhysicalBreaks != 0 &&
            MapRegistersCount > ConfigInfo->NumberOfPhysicalBreaks)
        {
            DeviceExtension->PortCapabilities.MaximumPhysicalPages =
                ConfigInfo->NumberOfPhysicalBreaks;
        }
        else
        {
            DeviceExtension->PortCapabilities.MaximumPhysicalPages = MapRegistersCount;
        }
    }

    /* Update auto request sense feature */
    DeviceExtension->SupportsAutoSense = ConfigInfo->AutoRequestSense;

    /* Update Srb extension size */
    if (DeviceExtension->SrbExtensionSize != ConfigInfo->SrbExtensionSize)
        DeviceExtension->SrbExtensionSize = ConfigInfo->SrbExtensionSize;

    /* Update Srb extension alloc flag */
    if (ConfigInfo->AutoRequestSense || DeviceExtension->SrbExtensionSize)
        DeviceExtension->NeedSrbExtensionAlloc = TRUE;

    /* Allocate a common DMA buffer */
    Status = SpiAllocateCommonBuffer(DeviceExtension, NumberOfBytes);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SpiAllocateCommonBuffer() failed with Status = 0x%08X!\n", Status);
        return NULL;
    }

    return DeviceExtension->NonCachedExtension;
}

static NTSTATUS
SpiAllocateCommonBuffer(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension, ULONG NonCachedSize)
{
    PVOID *SrbExtension, CommonBuffer;
    ULONG CommonBufferLength, BufSize;

    /* If size is 0, set it to 16 */
    if (!DeviceExtension->SrbExtensionSize)
        DeviceExtension->SrbExtensionSize = 16;

    /* Calculate size */
    BufSize = DeviceExtension->SrbExtensionSize;

    /* Add autosense data size if needed */
    if (DeviceExtension->SupportsAutoSense)
        BufSize += sizeof(SENSE_DATA);


    /* Round it */
    BufSize = (BufSize + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    /* Sum up into the total common buffer length, and round it to page size */
    CommonBufferLength =
        ROUND_TO_PAGES(NonCachedSize + BufSize * DeviceExtension->RequestsNumber);

    /* Allocate it */
    if (!DeviceExtension->AdapterObject)
    {
        /* From nonpaged pool if there is no DMA */
        CommonBuffer = ExAllocatePoolWithTag(NonPagedPool, CommonBufferLength, TAG_SCSIPORT);
    }
    else
    {
        /* Perform a full request since we have a DMA adapter*/
        CommonBuffer = HalAllocateCommonBuffer(DeviceExtension->AdapterObject,
            CommonBufferLength,
            &DeviceExtension->PhysicalAddress,
            FALSE );
    }

    /* Fail in case of error */
    if (!CommonBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero it */
    RtlZeroMemory(CommonBuffer, CommonBufferLength);

    /* Store its size in Device Extension */
    DeviceExtension->CommonBufferLength = CommonBufferLength;

    /* SrbExtension buffer is located at the beginning of the buffer */
    DeviceExtension->SrbExtensionBuffer = CommonBuffer;

    /* Non-cached extension buffer is located at the end of
       the common buffer */
    if (NonCachedSize)
    {
        CommonBufferLength -=  NonCachedSize;
        DeviceExtension->NonCachedExtension = (PUCHAR)CommonBuffer + CommonBufferLength;
    }
    else
    {
        DeviceExtension->NonCachedExtension = NULL;
    }

    if (DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* Look up how many SRB data structures we need */
        DeviceExtension->SrbDataCount = CommonBufferLength / BufSize;

        /* Initialize the free SRB extensions list */
        SrbExtension = (PVOID *)CommonBuffer;
        DeviceExtension->FreeSrbExtensions = SrbExtension;

        /* Fill the remaining pointers (if we have more than 1 SRB) */
        while (CommonBufferLength >= 2 * BufSize)
        {
            *SrbExtension = (PVOID*)((PCHAR)SrbExtension + BufSize);
            SrbExtension = *SrbExtension;

            CommonBufferLength -= BufSize;
        }
    }

    return STATUS_SUCCESS;
}



/*
 * @implemented
 */
PVOID NTAPI
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
                          IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    ULONG Offset;

    DPRINT("ScsiPortGetVirtualAddress(%p %I64x)\n",
        HwDeviceExtension, PhysicalAddress.QuadPart);

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                        SCSI_PORT_DEVICE_EXTENSION,
                                        MiniPortDeviceExtension);

    if (DeviceExtension->PhysicalAddress.QuadPart > PhysicalAddress.QuadPart)
        return NULL;

    Offset = (ULONG)(PhysicalAddress.QuadPart - DeviceExtension->PhysicalAddress.QuadPart);

    if (Offset >= DeviceExtension->CommonBufferLength)
        return NULL;

    return (PVOID)((ULONG_PTR)DeviceExtension->SrbExtensionBuffer + Offset);
}

static VOID
SpiInitOpenKeys(PCONFIGURATION_INFO ConfigInfo, PUNICODE_STRING RegistryPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;

    /* Open the service key */
    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ConfigInfo->ServiceKey,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Unable to open driver's registry key %wZ, status 0x%08x\n", RegistryPath, Status);
        ConfigInfo->ServiceKey = NULL;
    }

    /* If we could open driver's service key, then proceed to the Parameters key */
    if (ConfigInfo->ServiceKey != NULL)
    {
        RtlInitUnicodeString(&KeyName, L"Parameters");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ConfigInfo->ServiceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        /* Try to open it */
        Status = ZwOpenKey(&ConfigInfo->DeviceKey,
                           KEY_READ,
                           &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            /* Yes, Parameters key exist, and it must be used instead of
               the Service key */
            ZwClose(ConfigInfo->ServiceKey);
            ConfigInfo->ServiceKey = ConfigInfo->DeviceKey;
            ConfigInfo->DeviceKey = NULL;
        }
    }

    if (ConfigInfo->ServiceKey != NULL)
    {
        /* Open the Device key */
        RtlInitUnicodeString(&KeyName, L"Device");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ConfigInfo->ServiceKey,
                                   NULL);

        /* We don't check for failure here - not needed */
        ZwOpenKey(&ConfigInfo->DeviceKey,
                  KEY_READ,
                  &ObjectAttributes);
    }
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiPortInitialize
 *
 * DESCRIPTION
 *	Initializes SCSI port driver specific data.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Argument1
 *		Pointer to the miniport driver's driver object.
 *
 *	Argument2
 *		Pointer to the miniport driver's registry path.
 *
 *	HwInitializationData
 *		Pointer to port driver specific configuration data.
 *
 *	HwContext
  		Miniport driver specific context.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */

ULONG NTAPI
ScsiPortInitialize(IN PVOID Argument1,
		   IN PVOID Argument2,
		   IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		   IN PVOID HwContext)
{
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Argument1;
    PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = NULL;
    PCONFIGURATION_INFORMATION SystemConfig;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    PORT_CONFIGURATION_INFORMATION InitialPortConfig;
    CONFIGURATION_INFO ConfigInfo;
    ULONG DeviceExtensionSize;
    ULONG PortConfigSize;
    BOOLEAN Again;
    BOOLEAN DeviceFound = FALSE;
    BOOLEAN FirstConfigCall = TRUE;
    ULONG Result;
    NTSTATUS Status;
    ULONG MaxBus;
    PCI_SLOT_NUMBER SlotNumber;

    PDEVICE_OBJECT PortDeviceObject;
    WCHAR NameBuffer[80];
    UNICODE_STRING DeviceName;
    WCHAR DosNameBuffer[80];
    UNICODE_STRING DosDeviceName;
    PIO_SCSI_CAPABILITIES PortCapabilities;

    KIRQL OldIrql;
    PCM_RESOURCE_LIST ResourceList;
    BOOLEAN Conflict;
    SIZE_T BusConfigSize;


    DPRINT ("ScsiPortInitialize() called!\n");

    /* Check params for validity */
    if ((HwInitializationData->HwInitialize == NULL) ||
        (HwInitializationData->HwStartIo == NULL) ||
        (HwInitializationData->HwInterrupt == NULL) ||
        (HwInitializationData->HwFindAdapter == NULL) ||
        (HwInitializationData->HwResetBus == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set handlers */
    DriverObject->DriverStartIo = ScsiPortStartIo;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScsiPortCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiPortCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiPortDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiPortDispatchScsi;

    /* Obtain configuration information */
    SystemConfig = IoGetConfigurationInformation();

    /* Zero the internal configuration info structure */
    RtlZeroMemory(&ConfigInfo, sizeof(CONFIGURATION_INFO));

    /* Zero starting slot number */
    SlotNumber.u.AsULONG = 0;

    /* Allocate space for access ranges */
    if (HwInitializationData->NumberOfAccessRanges)
    {
        ConfigInfo.AccessRanges =
            ExAllocatePoolWithTag(PagedPool,
            HwInitializationData->NumberOfAccessRanges * sizeof(ACCESS_RANGE), TAG_SCSIPORT);

        /* Fail if failed */
        if (ConfigInfo.AccessRanges == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Open registry keys */
    SpiInitOpenKeys(&ConfigInfo, (PUNICODE_STRING)Argument2);

    /* Last adapter number = not known */
    ConfigInfo.LastAdapterNumber = SP_UNINITIALIZED_VALUE;

    /* Calculate sizes of DeviceExtension and PortConfig */
    DeviceExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) +
        HwInitializationData->DeviceExtensionSize;

    MaxBus = (HwInitializationData->AdapterInterfaceType == PCIBus) ? 8 : 1;
    DPRINT("MaxBus: %lu\n", MaxBus);

    while (TRUE)
    {
        /* Create a unicode device name */
        swprintf(NameBuffer,
                 L"\\Device\\ScsiPort%lu",
                 SystemConfig->ScsiPortCount);
        RtlInitUnicodeString(&DeviceName, NameBuffer);

        DPRINT("Creating device: %wZ\n", &DeviceName);

        /* Create the port device */
        Status = IoCreateDevice(DriverObject,
                                DeviceExtensionSize,
                                &DeviceName,
                                FILE_DEVICE_CONTROLLER,
                                0,
                                FALSE,
                                &PortDeviceObject);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoCreateDevice call failed! (Status 0x%lX)\n", Status);
            PortDeviceObject = NULL;
            break;
        }

        DPRINT ("Created device: %wZ (%p)\n", &DeviceName, PortDeviceObject);

        /* Set the buffering strategy here... */
        PortDeviceObject->Flags |= DO_DIRECT_IO;
        PortDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT; /* FIXME: Is this really needed? */

        /* Fill Device Extension */
        DeviceExtension = PortDeviceObject->DeviceExtension;
        RtlZeroMemory(DeviceExtension, DeviceExtensionSize);
        DeviceExtension->Length = DeviceExtensionSize;
        DeviceExtension->DeviceObject = PortDeviceObject;
        DeviceExtension->PortNumber = SystemConfig->ScsiPortCount;

        /* Driver's routines... */
        DeviceExtension->HwInitialize = HwInitializationData->HwInitialize;
        DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
        DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
        DeviceExtension->HwResetBus = HwInitializationData->HwResetBus;
        DeviceExtension->HwDmaStarted = HwInitializationData->HwDmaStarted;

        /* Extensions sizes */
        DeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
        DeviceExtension->LunExtensionSize = HwInitializationData->SpecificLuExtensionSize;
        DeviceExtension->SrbExtensionSize = HwInitializationData->SrbExtensionSize;

        /* Round Srb extension size to the quadword */
        DeviceExtension->SrbExtensionSize =
            ~(sizeof(LONGLONG) - 1) & (DeviceExtension->SrbExtensionSize +
            sizeof(LONGLONG) - 1);

        /* Fill some numbers (bus count, lun count, etc) */
        DeviceExtension->MaxLunCount = SCSI_MAXIMUM_LOGICAL_UNITS;
        DeviceExtension->RequestsNumber = 16;

        /* Initialize the spin lock in the controller extension */
        KeInitializeSpinLock(&DeviceExtension->IrqLock);
        KeInitializeSpinLock(&DeviceExtension->SpinLock);

        /* Initialize the DPC object */
        IoInitializeDpcRequest(PortDeviceObject,
                               ScsiPortDpcForIsr);

        /* Initialize the device timer */
        DeviceExtension->TimerCount = -1;
        IoInitializeTimer(PortDeviceObject,
                          ScsiPortIoTimer,
                          DeviceExtension);

        /* Initialize miniport timer */
        KeInitializeTimer(&DeviceExtension->MiniportTimer);
        KeInitializeDpc(&DeviceExtension->MiniportTimerDpc,
                        SpiMiniportTimerDpc,
                        PortDeviceObject);

CreatePortConfig:

        Status = SpiCreatePortConfig(DeviceExtension,
                                     HwInitializationData,
                                     &ConfigInfo,
                                     &InitialPortConfig,
                                     FirstConfigCall);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("SpiCreatePortConfig() failed with Status 0x%08X\n", Status);
            break;
        }

        /* Allocate and initialize port configuration info */
        PortConfigSize = (sizeof(PORT_CONFIGURATION_INFORMATION) +
                          HwInitializationData->NumberOfAccessRanges *
                          sizeof(ACCESS_RANGE) + 7) & ~7;
        DeviceExtension->PortConfig = ExAllocatePoolWithTag(NonPagedPool, PortConfigSize, TAG_SCSIPORT);

        /* Fail if failed */
        if (DeviceExtension->PortConfig == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        PortConfig = DeviceExtension->PortConfig;

        /* Copy information here */
        RtlCopyMemory(PortConfig,
                      &InitialPortConfig,
                      sizeof(PORT_CONFIGURATION_INFORMATION));


        /* Copy extension sizes into the PortConfig */
        PortConfig->SpecificLuExtensionSize = DeviceExtension->LunExtensionSize;
        PortConfig->SrbExtensionSize = DeviceExtension->SrbExtensionSize;

        /* Initialize Access ranges */
        if (HwInitializationData->NumberOfAccessRanges != 0)
        {
            PortConfig->AccessRanges = (PVOID)(PortConfig+1);

            /* Align to LONGLONG */
            PortConfig->AccessRanges = (PVOID)((ULONG_PTR)(PortConfig->AccessRanges) + 7);
            PortConfig->AccessRanges = (PVOID)((ULONG_PTR)(PortConfig->AccessRanges) & ~7);

            /* Copy the data */
            RtlCopyMemory(PortConfig->AccessRanges,
                          ConfigInfo.AccessRanges,
                          HwInitializationData->NumberOfAccessRanges * sizeof(ACCESS_RANGE));
        }

      /* Search for matching PCI device */
      if ((HwInitializationData->AdapterInterfaceType == PCIBus) &&
          (HwInitializationData->VendorIdLength > 0) &&
          (HwInitializationData->VendorId != NULL) &&
          (HwInitializationData->DeviceIdLength > 0) &&
          (HwInitializationData->DeviceId != NULL))
      {
          PortConfig->BusInterruptLevel = 0;

          /* Get PCI device data */
          DPRINT("VendorId '%.*s'  DeviceId '%.*s'\n",
                 HwInitializationData->VendorIdLength,
                 HwInitializationData->VendorId,
                 HwInitializationData->DeviceIdLength,
                 HwInitializationData->DeviceId);

          if (!SpiGetPciConfigData(DriverObject,
                                   PortDeviceObject,
                                   HwInitializationData,
                                   PortConfig,
                                   RegistryPath,
                                   ConfigInfo.BusNumber,
                                   &SlotNumber))
          {
              /* Continue to the next bus, nothing here */
              ConfigInfo.BusNumber++;
              DeviceExtension->PortConfig = NULL;
              ExFreePool(PortConfig);
              Again = FALSE;
              goto CreatePortConfig;
          }

          if (!PortConfig->BusInterruptLevel)
          {
              /* Bypass this slot, because no interrupt was assigned */
              DeviceExtension->PortConfig = NULL;
              ExFreePool(PortConfig);
              goto CreatePortConfig;
          }
      }
      else
      {
          DPRINT("Non-pci bus\n");
      }

      /* Note: HwFindAdapter is called once for each bus */
      Again = FALSE;
      DPRINT("Calling HwFindAdapter() for Bus %lu\n", PortConfig->SystemIoBusNumber);
      Result = (HwInitializationData->HwFindAdapter)(&DeviceExtension->MiniPortDeviceExtension,
                                                     HwContext,
                                                     0,  /* BusInformation */
                                                     ConfigInfo.Parameter, /* ArgumentString */
                                                     PortConfig,
                                                     &Again);

      DPRINT("HwFindAdapter() Result: %lu  Again: %s\n",
             Result, (Again) ? "True" : "False");

      /* Free MapRegisterBase, it's not needed anymore */
      if (DeviceExtension->MapRegisterBase != NULL)
      {
          ExFreePool(DeviceExtension->MapRegisterBase);
          DeviceExtension->MapRegisterBase = NULL;
      }

      /* If result is nothing good... */
      if (Result != SP_RETURN_FOUND)
      {
          DPRINT("HwFindAdapter() Result: %lu\n", Result);

          if (Result == SP_RETURN_NOT_FOUND)
          {
              /* We can continue on the next bus */
              ConfigInfo.BusNumber++;
              Again = FALSE;

              DeviceExtension->PortConfig = NULL;
              ExFreePool(PortConfig);
              goto CreatePortConfig;
          }

          /* Otherwise, break */
          Status = STATUS_INTERNAL_ERROR;
          break;
      }

      DPRINT("ScsiPortInitialize(): Found HBA! (%x), adapter Id %d\n",
          PortConfig->BusInterruptVector, PortConfig->InitiatorBusId[0]);

      /* If the SRB extension size was updated */
      if (!DeviceExtension->NonCachedExtension &&
          (PortConfig->SrbExtensionSize != DeviceExtension->SrbExtensionSize))
      {
          /* Set it (rounding to LONGLONG again) */
          DeviceExtension->SrbExtensionSize =
              (PortConfig->SrbExtensionSize +
               sizeof(LONGLONG)) & ~(sizeof(LONGLONG) - 1);
      }

      /* The same with LUN extension size */
      if (PortConfig->SpecificLuExtensionSize != DeviceExtension->LunExtensionSize)
          DeviceExtension->LunExtensionSize = PortConfig->SpecificLuExtensionSize;


      if (!((HwInitializationData->AdapterInterfaceType == PCIBus) &&
          (HwInitializationData->VendorIdLength > 0) &&
          (HwInitializationData->VendorId != NULL) &&
          (HwInitializationData->DeviceIdLength > 0) &&
          (HwInitializationData->DeviceId != NULL)))
      {
          /* Construct a resource list */
          ResourceList = SpiConfigToResource(DeviceExtension,
                                             PortConfig);

          if (ResourceList)
          {
              UNICODE_STRING UnicodeString;
              RtlInitUnicodeString(&UnicodeString, L"ScsiAdapter");
              DPRINT("Reporting resources\n");
              Status = IoReportResourceUsage(&UnicodeString,
                                             DriverObject,
                                             NULL,
                                             0,
                                             PortDeviceObject,
                                             ResourceList,
                                             FIELD_OFFSET(CM_RESOURCE_LIST,
                                                 List[0].PartialResourceList.PartialDescriptors) +
                                                 ResourceList->List[0].PartialResourceList.Count
                                                 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
                                             FALSE,
                                             &Conflict);
              ExFreePool(ResourceList);

              /* In case of a failure or a conflict, break */
              if (Conflict || (!NT_SUCCESS(Status)))
              {
                  if (Conflict)
                      Status = STATUS_CONFLICTING_ADDRESSES;
                  break;
              }
            }
      }

      /* Reset the Conflict var */
      Conflict = FALSE;

      /* Copy all stuff which we ever need from PortConfig to the DeviceExtension */
      if (PortConfig->MaximumNumberOfTargets > SCSI_MAXIMUM_TARGETS_PER_BUS)
          DeviceExtension->MaxTargedIds = SCSI_MAXIMUM_TARGETS_PER_BUS;
      else
          DeviceExtension->MaxTargedIds = PortConfig->MaximumNumberOfTargets;

      DeviceExtension->BusNum = PortConfig->NumberOfBuses;
      DeviceExtension->CachesData = PortConfig->CachesData;
      DeviceExtension->ReceiveEvent = PortConfig->ReceiveEvent;
      DeviceExtension->SupportsTaggedQueuing = PortConfig->TaggedQueuing;
      DeviceExtension->MultipleReqsPerLun = PortConfig->MultipleRequestPerLu;

      /* If something was disabled via registry - apply it */
      if (ConfigInfo.DisableMultipleLun)
          DeviceExtension->MultipleReqsPerLun = PortConfig->MultipleRequestPerLu = FALSE;

      if (ConfigInfo.DisableTaggedQueueing)
          DeviceExtension->SupportsTaggedQueuing = PortConfig->MultipleRequestPerLu = FALSE;

      /* Check if we need to alloc SRB data */
      if (DeviceExtension->SupportsTaggedQueuing ||
          DeviceExtension->MultipleReqsPerLun)
      {
          DeviceExtension->NeedSrbDataAlloc = TRUE;
      }
      else
      {
          DeviceExtension->NeedSrbDataAlloc = FALSE;
      }

      /* Get a pointer to the port capabilities */
      PortCapabilities = &DeviceExtension->PortCapabilities;

      /* Copy one field there */
      DeviceExtension->MapBuffers = PortConfig->MapBuffers;
      PortCapabilities->AdapterUsesPio = PortConfig->MapBuffers;

      if (DeviceExtension->AdapterObject == NULL &&
          (PortConfig->DmaChannel != SP_UNINITIALIZED_VALUE || PortConfig->Master))
      {
          DPRINT1("DMA is not supported yet\n");
          ASSERT(FALSE);
      }

      if (DeviceExtension->SrbExtensionBuffer == NULL &&
          (DeviceExtension->SrbExtensionSize != 0  ||
          PortConfig->AutoRequestSense))
      {
          DeviceExtension->SupportsAutoSense = PortConfig->AutoRequestSense;
          DeviceExtension->NeedSrbExtensionAlloc = TRUE;

          /* Allocate common buffer */
          Status = SpiAllocateCommonBuffer(DeviceExtension, 0);

          /* Check for failure */
          if (!NT_SUCCESS(Status))
              break;
      }

      /* Allocate SrbData, if needed */
      if (DeviceExtension->NeedSrbDataAlloc)
      {
          ULONG Count;
          PSCSI_REQUEST_BLOCK_INFO SrbData;

          if (DeviceExtension->SrbDataCount != 0)
              Count = DeviceExtension->SrbDataCount;
          else
              Count = DeviceExtension->RequestsNumber * 2;

          /* Allocate the data */
          SrbData = ExAllocatePoolWithTag(NonPagedPool, Count * sizeof(SCSI_REQUEST_BLOCK_INFO), TAG_SCSIPORT);
          if (SrbData == NULL)
              return STATUS_INSUFFICIENT_RESOURCES;

          RtlZeroMemory(SrbData, Count * sizeof(SCSI_REQUEST_BLOCK_INFO));

          DeviceExtension->SrbInfo = SrbData;
          DeviceExtension->FreeSrbInfo = SrbData;
          DeviceExtension->SrbDataCount = Count;

          /* Link it to the list */
          while (Count > 0)
          {
              SrbData->Requests.Flink = (PLIST_ENTRY)(SrbData + 1);
              SrbData++;
              Count--;
          }

          /* Mark the last entry of the list */
          SrbData--;
          SrbData->Requests.Flink = NULL;
      }

      /* Initialize port capabilities */
      PortCapabilities = &DeviceExtension->PortCapabilities;
      PortCapabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
      PortCapabilities->MaximumTransferLength = PortConfig->MaximumTransferLength;

      if (PortConfig->ReceiveEvent)
          PortCapabilities->SupportedAsynchronousEvents |= SRBEV_SCSI_ASYNC_NOTIFICATION;

      PortCapabilities->TaggedQueuing = DeviceExtension->SupportsTaggedQueuing;
      PortCapabilities->AdapterScansDown = PortConfig->AdapterScansDown;

      if (PortConfig->AlignmentMask > PortDeviceObject->AlignmentRequirement)
          PortDeviceObject->AlignmentRequirement = PortConfig->AlignmentMask;

      PortCapabilities->AlignmentMask = PortDeviceObject->AlignmentRequirement;

      if (PortCapabilities->MaximumPhysicalPages == 0)
      {
          PortCapabilities->MaximumPhysicalPages =
              BYTES_TO_PAGES(PortCapabilities->MaximumTransferLength);

          /* Apply miniport's limits */
          if (PortConfig->NumberOfPhysicalBreaks < PortCapabilities->MaximumPhysicalPages)
          {
              PortCapabilities->MaximumPhysicalPages = PortConfig->NumberOfPhysicalBreaks;
          }
      }

      /* Deal with interrupts */
      if (DeviceExtension->HwInterrupt == NULL ||
          (PortConfig->BusInterruptLevel == 0 && PortConfig->BusInterruptVector == 0))
      {
          /* No interrupts */
          DeviceExtension->InterruptCount = 0;

          DPRINT1("Interrupt Count: 0\n");

          UNIMPLEMENTED;

          /* This code path will ALWAYS crash so stop it now */
          while(TRUE);
      }
      else
      {
          BOOLEAN InterruptShareable;
          KINTERRUPT_MODE InterruptMode[2];
          ULONG InterruptVector[2], i, MappedIrq[2];
          KIRQL Dirql[2], MaxDirql;
          KAFFINITY Affinity[2];

          DeviceExtension->InterruptLevel[0] = PortConfig->BusInterruptLevel;
          DeviceExtension->InterruptLevel[1] = PortConfig->BusInterruptLevel2;

          InterruptVector[0] = PortConfig->BusInterruptVector;
          InterruptVector[1] = PortConfig->BusInterruptVector2;

          InterruptMode[0] = PortConfig->InterruptMode;
          InterruptMode[1] = PortConfig->InterruptMode2;

          DeviceExtension->InterruptCount = (PortConfig->BusInterruptLevel2 != 0 || PortConfig->BusInterruptVector2 != 0) ? 2 : 1;

          for (i = 0; i < DeviceExtension->InterruptCount; i++)
          {
              /* Register an interrupt handler for this device */
              MappedIrq[i] = HalGetInterruptVector(PortConfig->AdapterInterfaceType,
                                                   PortConfig->SystemIoBusNumber,
                                                   DeviceExtension->InterruptLevel[i],
                                                   InterruptVector[i],
                                                   &Dirql[i],
                                                   &Affinity[i]);
          }

          if (DeviceExtension->InterruptCount == 1 || Dirql[0] > Dirql[1])
              MaxDirql = Dirql[0];
          else
              MaxDirql = Dirql[1];

          for (i = 0; i < DeviceExtension->InterruptCount; i++)
          {
              /* Determine IRQ sharability as usual */
              if (PortConfig->AdapterInterfaceType == MicroChannel ||
                  InterruptMode[i] == LevelSensitive)
              {
                  InterruptShareable = TRUE;
              }
              else
              {
                  InterruptShareable = FALSE;
              }

              Status = IoConnectInterrupt(&DeviceExtension->Interrupt[i],
                                          (PKSERVICE_ROUTINE)ScsiPortIsr,
                                          DeviceExtension,
                                          &DeviceExtension->IrqLock,
                                          MappedIrq[i],
                                          Dirql[i],
                                          MaxDirql,
                                          InterruptMode[i],
                                          InterruptShareable,
                                          Affinity[i],
                                          FALSE);

              if (!(NT_SUCCESS(Status)))
              {
                  DPRINT1("Could not connect interrupt %d\n",
                          InterruptVector[i]);
                  DeviceExtension->Interrupt[i] = NULL;
                  break;
              }
          }

          if (!NT_SUCCESS(Status))
              break;
      }

      /* Save IoAddress (from access ranges) */
      if (HwInitializationData->NumberOfAccessRanges != 0)
      {
          DeviceExtension->IoAddress =
              ((*(PortConfig->AccessRanges))[0]).RangeStart.LowPart;

          DPRINT("Io Address %x\n", DeviceExtension->IoAddress);
      }

      /* Set flag that it's allowed to disconnect during this command */
      DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

      /* Initialize counter of active requests (-1 means there are none) */
      DeviceExtension->ActiveRequestCounter = -1;

      /* Analyze what we have about DMA */
      if (DeviceExtension->AdapterObject != NULL &&
          PortConfig->Master &&
          PortConfig->NeedPhysicalAddresses)
      {
          DeviceExtension->MapRegisters = TRUE;
      }
      else
      {
          DeviceExtension->MapRegisters = FALSE;
      }

      /* Call HwInitialize at DISPATCH_LEVEL */
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

      if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                  DeviceExtension->HwInitialize,
                                  DeviceExtension->MiniPortDeviceExtension))
      {
          DPRINT1("HwInitialize() failed!\n");
          KeLowerIrql(OldIrql);
          Status = STATUS_ADAPTER_HARDWARE_ERROR;
          break;
      }

      /* Check if a notification is needed */
      if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
      {
          /* Call DPC right away, because we're already at DISPATCH_LEVEL */
          ScsiPortDpcForIsr(NULL,
                            DeviceExtension->DeviceObject,
                            NULL,
                            NULL);
      }

      /* Lower irql back to what it was */
      KeLowerIrql(OldIrql);

      /* Start our timer */
      IoStartTimer(PortDeviceObject);

      /* Initialize bus scanning information */
      BusConfigSize = FIELD_OFFSET(BUSES_CONFIGURATION_INFORMATION,
                                   BusScanInfo[DeviceExtension->PortConfig->NumberOfBuses]);
      DeviceExtension->BusesConfig = ExAllocatePoolWithTag(PagedPool,
                                                           BusConfigSize,
                                                           TAG_SCSIPORT);
      if (!DeviceExtension->BusesConfig)
      {
          DPRINT1("Out of resources!\n");
          Status = STATUS_INSUFFICIENT_RESOURCES;
          break;
      }

      /* Zero it */
      RtlZeroMemory(DeviceExtension->BusesConfig, BusConfigSize);

      /* Store number of buses there */
      DeviceExtension->BusesConfig->NumberOfBuses = (UCHAR)DeviceExtension->BusNum;

      /* Scan the adapter for devices */
      SpiScanAdapter(DeviceExtension);

      /* Build the registry device map */
      SpiBuildDeviceMap(DeviceExtension,
                       (PUNICODE_STRING)Argument2);

      /* Create the dos device link */
      swprintf(DosNameBuffer,
               L"\\??\\Scsi%lu:",
              SystemConfig->ScsiPortCount);
      RtlInitUnicodeString(&DosDeviceName, DosNameBuffer);
      IoCreateSymbolicLink(&DosDeviceName, &DeviceName);

      /* Increase the port count */
      SystemConfig->ScsiPortCount++;
      FirstConfigCall = FALSE;

      /* Increase adapter number and bus number respectively */
      ConfigInfo.AdapterNumber++;

      if (!Again)
          ConfigInfo.BusNumber++;

      DPRINT("Bus: %lu  MaxBus: %lu\n", ConfigInfo.BusNumber, MaxBus);

      DeviceFound = TRUE;
    }

    /* Clean up the mess */
    SpiCleanupAfterInit(DeviceExtension);

    /* Close registry keys */
    if (ConfigInfo.ServiceKey != NULL)
        ZwClose(ConfigInfo.ServiceKey);

    if (ConfigInfo.DeviceKey != NULL)
        ZwClose(ConfigInfo.DeviceKey);

    if (ConfigInfo.BusKey != NULL)
        ZwClose(ConfigInfo.BusKey);

    if (ConfigInfo.AccessRanges != NULL)
        ExFreePool(ConfigInfo.AccessRanges);

    if (ConfigInfo.Parameter != NULL)
        ExFreePool(ConfigInfo.Parameter);

    DPRINT("ScsiPortInitialize() done, Status = 0x%08X, DeviceFound = %d!\n",
        Status, DeviceFound);

    return (DeviceFound == FALSE) ? Status : STATUS_SUCCESS;
}

static VOID
SpiCleanupAfterInit(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_LUN_INFO LunInfo;
    PVOID Ptr;
    ULONG Bus, Lun;

    /* Check if we have something to clean up */
    if (DeviceExtension == NULL)
        return;

    /* Stop the timer */
    IoStopTimer(DeviceExtension->DeviceObject);

    /* Disconnect the interrupts */
    while (DeviceExtension->InterruptCount)
    {
        if (DeviceExtension->Interrupt[--DeviceExtension->InterruptCount])
            IoDisconnectInterrupt(DeviceExtension->Interrupt[DeviceExtension->InterruptCount]);
    }

    /* Delete ConfigInfo */
    if (DeviceExtension->BusesConfig)
    {
        for (Bus = 0; Bus < DeviceExtension->BusNum; Bus++)
        {
            if (!DeviceExtension->BusesConfig->BusScanInfo[Bus])
                continue;

            LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;

            while (LunInfo)
            {
                /* Free current, but save pointer to the next one */
                Ptr = LunInfo->Next;
                ExFreePool(LunInfo);
                LunInfo = Ptr;
            }

            ExFreePool(DeviceExtension->BusesConfig->BusScanInfo[Bus]);
        }

        ExFreePool(DeviceExtension->BusesConfig);
    }

    /* Free PortConfig */
    if (DeviceExtension->PortConfig)
        ExFreePool(DeviceExtension->PortConfig);

    /* Free LUNs*/
    for(Lun = 0; Lun < LUS_NUMBER; Lun++)
    {
        while (DeviceExtension->LunExtensionList[Lun])
        {
            Ptr = DeviceExtension->LunExtensionList[Lun];
            DeviceExtension->LunExtensionList[Lun] = DeviceExtension->LunExtensionList[Lun]->Next;

            ExFreePool(Ptr);
        }
    }

    /* Free common buffer (if it exists) */
    if (DeviceExtension->SrbExtensionBuffer != NULL &&
        DeviceExtension->CommonBufferLength != 0)
    {
            if (!DeviceExtension->AdapterObject)
            {
                ExFreePool(DeviceExtension->SrbExtensionBuffer);
            }
            else
            {
                HalFreeCommonBuffer(DeviceExtension->AdapterObject,
                                    DeviceExtension->CommonBufferLength,
                                    DeviceExtension->PhysicalAddress,
                                    DeviceExtension->SrbExtensionBuffer,
                                    FALSE);
            }
    }

    /* Free SRB info */
    if (DeviceExtension->SrbInfo != NULL)
        ExFreePool(DeviceExtension->SrbInfo);

    /* Unmap mapped addresses */
    while (DeviceExtension->MappedAddressList != NULL)
    {
        MmUnmapIoSpace(DeviceExtension->MappedAddressList->MappedAddress,
                       DeviceExtension->MappedAddressList->NumberOfBytes);

        Ptr = DeviceExtension->MappedAddressList;
        DeviceExtension->MappedAddressList = DeviceExtension->MappedAddressList->NextMappedAddress;

        ExFreePool(Ptr);
    }

    /* Finally delete the device object */
    DPRINT("Deleting device %p\n", DeviceExtension->DeviceObject);
    IoDeleteDevice(DeviceExtension->DeviceObject);
}

/*
 * @unimplemented
 */
VOID NTAPI
ScsiPortIoMapTransfer(IN PVOID HwDeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb,
		      IN PVOID LogicalAddress,
		      IN ULONG Length)
{
  DPRINT1("ScsiPortIoMapTransfer()\n");
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID NTAPI
ScsiPortLogError(IN PVOID HwDeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
		 IN UCHAR PathId,
		 IN UCHAR TargetId,
		 IN UCHAR Lun,
		 IN ULONG ErrorCode,
		 IN ULONG UniqueId)
{
  //PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT1("ScsiPortLogError() called\n");
  DPRINT1("PathId: 0x%02x  TargetId: 0x%02x  Lun: 0x%02x  ErrorCode: 0x%08lx  UniqueId: 0x%08lx\n",
          PathId, TargetId, Lun, ErrorCode, UniqueId);

  //DeviceExtension = CONTAINING_RECORD(HwDeviceExtension, SCSI_PORT_DEVICE_EXTENSION, MiniPortDeviceExtension);


  DPRINT("ScsiPortLogError() done\n");
}

/*
 * @implemented
 */
VOID NTAPI
ScsiPortMoveMemory(OUT PVOID Destination,
		   IN PVOID Source,
		   IN ULONG Length)
{
  RtlMoveMemory(Destination,
		Source,
		Length);
}


/*
 * @implemented
 */
VOID
ScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
		     IN PVOID HwDeviceExtension,
		     ...)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    va_list ap;

    DPRINT("ScsiPortNotification() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
        SCSI_PORT_DEVICE_EXTENSION,
        MiniPortDeviceExtension);

    DPRINT("DeviceExtension %p\n", DeviceExtension);

    va_start(ap, HwDeviceExtension);

    switch (NotificationType)
    {
    case RequestComplete:
        {
            PSCSI_REQUEST_BLOCK Srb;
            PSCSI_REQUEST_BLOCK_INFO SrbData;

            Srb = (PSCSI_REQUEST_BLOCK) va_arg (ap, PSCSI_REQUEST_BLOCK);

            DPRINT("Notify: RequestComplete (Srb %p)\n", Srb);

            /* Make sure Srb is alright */
            ASSERT(Srb->SrbStatus != SRB_STATUS_PENDING);
            ASSERT(Srb->Function != SRB_FUNCTION_EXECUTE_SCSI || Srb->SrbStatus != SRB_STATUS_SUCCESS || Srb->ScsiStatus == SCSISTAT_GOOD);

            if (!(Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE))
            {
                /* It's been already completed */
                va_end(ap);
                return;
            }

            /* It's not active anymore */
            Srb->SrbFlags &= ~SRB_FLAGS_IS_ACTIVE;

            if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
            {
                /* TODO: Treat it specially */
                ASSERT(FALSE);
            }
            else
            {
                /* Get the SRB data */
                SrbData = SpiGetSrbData(DeviceExtension,
                                        Srb->PathId,
                                        Srb->TargetId,
                                        Srb->Lun,
                                        Srb->QueueTag);

                /* Make sure there are no CompletedRequests and there is a Srb */
                ASSERT(SrbData->CompletedRequests == NULL && SrbData->Srb != NULL);

                /* If it's a read/write request, make sure it has data inside it */
                if ((Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
                    ((Srb->Cdb[0] == SCSIOP_READ) || (Srb->Cdb[0] == SCSIOP_WRITE)))
                {
                        ASSERT(Srb->DataTransferLength);
                }

                SrbData->CompletedRequests = DeviceExtension->InterruptData.CompletedRequests;
                DeviceExtension->InterruptData.CompletedRequests = SrbData;
            }
        }
        break;

    case NextRequest:
        DPRINT("Notify: NextRequest\n");
        DeviceExtension->InterruptData.Flags |= SCSI_PORT_NEXT_REQUEST_READY;
        break;

    case NextLuRequest:
        {
            UCHAR PathId;
            UCHAR TargetId;
            UCHAR Lun;
            PSCSI_PORT_LUN_EXTENSION LunExtension;

            PathId = (UCHAR) va_arg (ap, int);
            TargetId = (UCHAR) va_arg (ap, int);
            Lun = (UCHAR) va_arg (ap, int);

            DPRINT("Notify: NextLuRequest(PathId %u  TargetId %u  Lun %u)\n",
                PathId, TargetId, Lun);

            /* Mark it in the flags field */
            DeviceExtension->InterruptData.Flags |= SCSI_PORT_NEXT_REQUEST_READY;

            /* Get the LUN extension */
            LunExtension = SpiGetLunExtension(DeviceExtension,
                                              PathId,
                                              TargetId,
                                              Lun);

            /* If returned LunExtension is NULL, break out */
            if (!LunExtension) break;

            /* This request should not be processed if */
            if ((LunExtension->ReadyLun) ||
                (LunExtension->SrbInfo.Srb))
            {
                /* Nothing to do here */
                break;
            }

            /* Add this LUN to the list */
            LunExtension->ReadyLun = DeviceExtension->InterruptData.ReadyLun;
            DeviceExtension->InterruptData.ReadyLun = LunExtension;
          }
          break;

      case ResetDetected:
          DPRINT("Notify: ResetDetected\n");
          /* Add RESET flags */
          DeviceExtension->InterruptData.Flags |=
                SCSI_PORT_RESET | SCSI_PORT_RESET_REPORTED;
          break;

      case CallDisableInterrupts:
          DPRINT1("UNIMPLEMENTED SCSI Notification called: CallDisableInterrupts!\n");
          break;

      case CallEnableInterrupts:
          DPRINT1("UNIMPLEMENTED SCSI Notification called: CallEnableInterrupts!\n");
          break;

      case RequestTimerCall:
          DPRINT("Notify: RequestTimerCall\n");
          DeviceExtension->InterruptData.Flags |= SCSI_PORT_TIMER_NEEDED;
          DeviceExtension->InterruptData.HwScsiTimer = (PHW_TIMER)va_arg(ap, PHW_TIMER);
          DeviceExtension->InterruptData.MiniportTimerValue = (ULONG)va_arg(ap, ULONG);
          break;

      case BusChangeDetected:
          DPRINT1("UNIMPLEMENTED SCSI Notification called: BusChangeDetected!\n");
          break;

      default:
	DPRINT1 ("Unsupported notification from WMI: %lu\n", NotificationType);
	break;
    }

    va_end(ap);

    /* Request a DPC after we're done with the interrupt */
    DeviceExtension->InterruptData.Flags |= SCSI_PORT_NOTIFICATION_NEEDED;
}

/*
 * @implemented
 */
BOOLEAN NTAPI
ScsiPortValidateRange(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  DPRINT("ScsiPortValidateRange()\n");
  return(TRUE);
}


/* INTERNAL FUNCTIONS ********************************************************/

static VOID
SpiResourceToConfig(IN PHW_INITIALIZATION_DATA HwInitializationData,
                    IN PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor,
                    IN PPORT_CONFIGURATION_INFORMATION PortConfig)
{
    PACCESS_RANGE AccessRange;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialData;
    ULONG RangeNumber;
    ULONG Index;
    ULONG Interrupt = 0;
    ULONG Dma = 0;

    RangeNumber = 0;

    /* Loop through all entries */
    for (Index = 0; Index < ResourceDescriptor->PartialResourceList.Count; Index++)
    {
        PartialData = &ResourceDescriptor->PartialResourceList.PartialDescriptors[Index];

        switch (PartialData->Type)
        {
        case CmResourceTypePort:
            /* Copy access ranges */
            if (RangeNumber < HwInitializationData->NumberOfAccessRanges)
            {
                AccessRange = &((*(PortConfig->AccessRanges))[RangeNumber]);

                AccessRange->RangeStart = PartialData->u.Port.Start;
                AccessRange->RangeLength = PartialData->u.Port.Length;

                AccessRange->RangeInMemory = FALSE;
                RangeNumber++;
            }
            break;

        case CmResourceTypeMemory:
            /* Copy access ranges */
            if (RangeNumber < HwInitializationData->NumberOfAccessRanges)
            {
                AccessRange = &((*(PortConfig->AccessRanges))[RangeNumber]);

                AccessRange->RangeStart = PartialData->u.Memory.Start;
                AccessRange->RangeLength = PartialData->u.Memory.Length;

                AccessRange->RangeInMemory = TRUE;
                RangeNumber++;
            }
            break;

        case CmResourceTypeInterrupt:

            if (Interrupt == 0)
            {
                /* Copy interrupt data */
                PortConfig->BusInterruptLevel = PartialData->u.Interrupt.Level;
                PortConfig->BusInterruptVector = PartialData->u.Interrupt.Vector;

                /* Set interrupt mode accordingly to the resource */
                if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
                {
                    PortConfig->InterruptMode = Latched;
                }
                else if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
                {
                    PortConfig->InterruptMode = LevelSensitive;
                }
            }
            else if (Interrupt == 1)
            {
                /* Copy interrupt data */
                PortConfig->BusInterruptLevel2 = PartialData->u.Interrupt.Level;
                PortConfig->BusInterruptVector2 = PartialData->u.Interrupt.Vector;

                /* Set interrupt mode accordingly to the resource */
                if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
                {
                    PortConfig->InterruptMode2 = Latched;
                }
                else if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
                {
                    PortConfig->InterruptMode2 = LevelSensitive;
                }
            }

            Interrupt++;
            break;

        case CmResourceTypeDma:

            if (Dma == 0)
            {
                PortConfig->DmaChannel = PartialData->u.Dma.Channel;
                PortConfig->DmaPort = PartialData->u.Dma.Port;

                if (PartialData->Flags & CM_RESOURCE_DMA_8)
                    PortConfig->DmaWidth = Width8Bits;
                else if ((PartialData->Flags & CM_RESOURCE_DMA_16) ||
                         (PartialData->Flags & CM_RESOURCE_DMA_8_AND_16)) //???
                    PortConfig->DmaWidth = Width16Bits;
                else if (PartialData->Flags & CM_RESOURCE_DMA_32)
                    PortConfig->DmaWidth = Width32Bits;
            }
            else if (Dma == 1)
            {
                PortConfig->DmaChannel2 = PartialData->u.Dma.Channel;
                PortConfig->DmaPort2 = PartialData->u.Dma.Port;

                if (PartialData->Flags & CM_RESOURCE_DMA_8)
                    PortConfig->DmaWidth2 = Width8Bits;
                else if ((PartialData->Flags & CM_RESOURCE_DMA_16) ||
                         (PartialData->Flags & CM_RESOURCE_DMA_8_AND_16)) //???
                    PortConfig->DmaWidth2 = Width16Bits;
                else if (PartialData->Flags & CM_RESOURCE_DMA_32)
                    PortConfig->DmaWidth2 = Width32Bits;
            }
            break;
        }
    }
}

static PCM_RESOURCE_LIST
SpiConfigToResource(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    PPORT_CONFIGURATION_INFORMATION PortConfig)
{
    PCONFIGURATION_INFORMATION ConfigInfo;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    PACCESS_RANGE AccessRange;
    ULONG ListLength = 0, i, FullSize;
    ULONG Interrupt, Dma;

    /* Get current Atdisk usage from the system */
    ConfigInfo = IoGetConfigurationInformation();

    if (PortConfig->AtdiskPrimaryClaimed)
        ConfigInfo->AtDiskPrimaryAddressClaimed = TRUE;

    if (PortConfig->AtdiskSecondaryClaimed)
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

    /* Do we use DMA? */
    if (PortConfig->DmaChannel != SP_UNINITIALIZED_VALUE ||
        PortConfig->DmaPort != SP_UNINITIALIZED_VALUE)
    {
        Dma = 1;

        if (PortConfig->DmaChannel2 != SP_UNINITIALIZED_VALUE ||
            PortConfig->DmaPort2 != SP_UNINITIALIZED_VALUE)
            Dma++;
    }
    else
    {
        Dma = 0;
    }
    ListLength += Dma;

    /* How many interrupts to we have? */
    Interrupt = DeviceExtension->InterruptCount;
    ListLength += Interrupt;

    /* How many access ranges do we use? */
    AccessRange = &((*(PortConfig->AccessRanges))[0]);
    for (i = 0; i < PortConfig->NumberOfAccessRanges; i++)
    {
        if (AccessRange->RangeLength != 0)
            ListLength++;

        AccessRange++;
    }

    /* Allocate the resource list, since we know its size now */
    FullSize = sizeof(CM_RESOURCE_LIST) + (ListLength - 1) *
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    ResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool, FullSize, TAG_SCSIPORT);

    if (!ResourceList)
        return NULL;

    /* Zero it */
    RtlZeroMemory(ResourceList, FullSize);

    /* Initialize it */
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = PortConfig->AdapterInterfaceType;
    ResourceList->List[0].BusNumber = PortConfig->SystemIoBusNumber;
    ResourceList->List[0].PartialResourceList.Count = ListLength;
    ResourceDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

    /* Copy access ranges array over */
    for (i = 0; i < PortConfig->NumberOfAccessRanges; i++)
    {
        AccessRange = &((*(PortConfig->AccessRanges))[i]);

        /* If the range is empty - skip it */
        if (AccessRange->RangeLength == 0)
            continue;

        if (AccessRange->RangeInMemory)
        {
            ResourceDescriptor->Type = CmResourceTypeMemory;
            ResourceDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        }
        else
        {
            ResourceDescriptor->Type = CmResourceTypePort;
            ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
        }

        ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;

        ResourceDescriptor->u.Memory.Start = AccessRange->RangeStart;
        ResourceDescriptor->u.Memory.Length = AccessRange->RangeLength;

        ResourceDescriptor++;
    }

    /* If we use interrupt(s), copy them */
    while (Interrupt)
    {
        ResourceDescriptor->Type = CmResourceTypeInterrupt;

        if (PortConfig->AdapterInterfaceType == MicroChannel ||
            ((Interrupt == 2) ? PortConfig->InterruptMode2 : PortConfig->InterruptMode) == LevelSensitive)
        {
            ResourceDescriptor->ShareDisposition = CmResourceShareShared;
            ResourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        }
        else
        {
            ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            ResourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        }

        ResourceDescriptor->u.Interrupt.Level = (Interrupt == 2) ? PortConfig->BusInterruptLevel2 : PortConfig->BusInterruptLevel;
        ResourceDescriptor->u.Interrupt.Vector = (Interrupt == 2) ? PortConfig->BusInterruptVector2 : PortConfig->BusInterruptVector;
        ResourceDescriptor->u.Interrupt.Affinity = 0;

        ResourceDescriptor++;
        Interrupt--;
    }

    /* Copy DMA data */
    while (Dma)
    {
        ResourceDescriptor->Type = CmResourceTypeDma;
        ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        ResourceDescriptor->u.Dma.Channel = (Dma == 2) ? PortConfig->DmaChannel2 : PortConfig->DmaChannel;
        ResourceDescriptor->u.Dma.Port = (Dma == 2) ? PortConfig->DmaPort2 : PortConfig->DmaPort;
        ResourceDescriptor->Flags = 0;

        if (((Dma == 2) ? PortConfig->DmaWidth2 : PortConfig->DmaWidth) == Width8Bits)
            ResourceDescriptor->Flags |= CM_RESOURCE_DMA_8;
        else if (((Dma == 2) ? PortConfig->DmaWidth2 : PortConfig->DmaWidth) == Width16Bits)
            ResourceDescriptor->Flags |= CM_RESOURCE_DMA_16;
        else
            ResourceDescriptor->Flags |= CM_RESOURCE_DMA_32;

        if (((Dma == 2) ? PortConfig->DmaChannel2 : PortConfig->DmaChannel) == SP_UNINITIALIZED_VALUE)
            ResourceDescriptor->u.Dma.Channel = 0;

        if (((Dma == 2) ? PortConfig->DmaPort2 : PortConfig->DmaPort) == SP_UNINITIALIZED_VALUE)
            ResourceDescriptor->u.Dma.Port = 0;

        ResourceDescriptor++;
        Dma--;
    }

    return ResourceList;
}


static BOOLEAN
SpiGetPciConfigData(IN PDRIVER_OBJECT DriverObject,
                    IN PDEVICE_OBJECT DeviceObject,
                    IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
                    IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
                    IN PUNICODE_STRING RegistryPath,
                    IN ULONG BusNumber,
                    IN OUT PPCI_SLOT_NUMBER NextSlotNumber)
{
    PCI_COMMON_CONFIG PciConfig;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG DataSize;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    CHAR VendorIdString[8];
    CHAR DeviceIdString[8];
    UNICODE_STRING UnicodeStr;
    PCM_RESOURCE_LIST ResourceList = NULL;
    NTSTATUS Status;

    DPRINT ("SpiGetPciConfiguration() called\n");

    SlotNumber.u.AsULONG = 0;

    /* Loop through all devices */
    for (DeviceNumber = NextSlotNumber->u.bits.DeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
        SlotNumber.u.bits.DeviceNumber = DeviceNumber;

        /* Loop through all functions */
        for (FunctionNumber = NextSlotNumber->u.bits.FunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            /* Get PCI config bytes */
            DataSize = HalGetBusData(PCIConfiguration,
                                     BusNumber,
                                     SlotNumber.u.AsULONG,
                                     &PciConfig,
                                     sizeof(ULONG));

            /* If result of HalGetBusData is 0, then the bus is wrong */
            if (DataSize == 0)
                return FALSE;

            /* Check if result is PCI_INVALID_VENDORID or too small */
            if ((DataSize < sizeof(ULONG)) ||
                (PciConfig.VendorID == PCI_INVALID_VENDORID))
            {
                /* Continue to try the next function */
                continue;
            }

            sprintf (VendorIdString, "%04hx", PciConfig.VendorID);
            sprintf (DeviceIdString, "%04hx", PciConfig.DeviceID);

            if (_strnicmp(VendorIdString, HwInitializationData->VendorId, HwInitializationData->VendorIdLength) ||
                _strnicmp(DeviceIdString, HwInitializationData->DeviceId, HwInitializationData->DeviceIdLength))
            {
                /* It is not our device */
                continue;
            }

            DPRINT("Found device 0x%04hx 0x%04hx at %1lu %2lu %1lu\n",
                   PciConfig.VendorID,
                   PciConfig.DeviceID,
                   BusNumber,
                   SlotNumber.u.bits.DeviceNumber,
                   SlotNumber.u.bits.FunctionNumber);


            RtlInitUnicodeString(&UnicodeStr, L"ScsiAdapter");
            Status = HalAssignSlotResources(RegistryPath,
                                            &UnicodeStr,
                                            DriverObject,
                                            DeviceObject,
                                            PCIBus,
                                            BusNumber,
                                            SlotNumber.u.AsULONG,
                                            &ResourceList);

            if (!NT_SUCCESS(Status))
                break;

            /* Create configuration information */
            SpiResourceToConfig(HwInitializationData,
                                ResourceList->List,
                                PortConfig);

            /* Free the resource list */
            ExFreePool(ResourceList);

            /* Set dev & fn numbers */
            NextSlotNumber->u.bits.DeviceNumber = DeviceNumber;
            NextSlotNumber->u.bits.FunctionNumber = FunctionNumber + 1;

            /* Save the slot number */
            PortConfig->SlotNumber = SlotNumber.u.AsULONG;

            return TRUE;
        }
       NextSlotNumber->u.bits.FunctionNumber = 0;
    }

    NextSlotNumber->u.bits.DeviceNumber = 0;
    DPRINT ("No device found\n");

    return FALSE;
}



/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreateClose
 *
 * DESCRIPTION
 *	Answer requests for Create/Close calls: a null operation.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to a device object.
 *
 *	Irp
 *		Pointer to an IRP.
 *
 * RETURN VALUE
 * 	Status.
 */

static NTSTATUS NTAPI
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    DPRINT("ScsiPortCreateClose()\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

static NTSTATUS
SpiHandleAttachRelease(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                       PIRP Irp)
{
    PSCSI_LUN_INFO LunInfo;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    PSCSI_REQUEST_BLOCK Srb;
    KIRQL Irql;

    /* Get pointer to the SRB */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Srb = (PSCSI_REQUEST_BLOCK)IrpStack->Parameters.Others.Argument1;

    /* Check if PathId matches number of buses */
    if (DeviceExtension->BusesConfig == NULL ||
        DeviceExtension->BusesConfig->NumberOfBuses <= Srb->PathId)
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    /* Get pointer to LunInfo */
    LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Srb->PathId]->LunInfo;

    /* Find matching LunInfo */
    while (LunInfo)
    {
        if (LunInfo->PathId == Srb->PathId &&
            LunInfo->TargetId == Srb->TargetId &&
            LunInfo->Lun == Srb->Lun)
        {
            break;
        }

        LunInfo = LunInfo->Next;
    }

    /* If we couldn't find it - exit */
    if (LunInfo == NULL)
        return STATUS_DEVICE_DOES_NOT_EXIST;


    /* Get spinlock */
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    /* Release, if asked */
    if (Srb->Function == SRB_FUNCTION_RELEASE_DEVICE)
    {
        LunInfo->DeviceClaimed = FALSE;
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        return STATUS_SUCCESS;
    }

    /* Attach, if not already claimed */
    if (LunInfo->DeviceClaimed)
    {
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        Srb->SrbStatus = SRB_STATUS_BUSY;

        return STATUS_DEVICE_BUSY;
    }

    /* Save the device object */
    DeviceObject = LunInfo->DeviceObject;

    if (Srb->Function == SRB_FUNCTION_CLAIM_DEVICE)
        LunInfo->DeviceClaimed = TRUE;

    if (Srb->Function == SRB_FUNCTION_ATTACH_DEVICE)
        LunInfo->DeviceObject = Srb->DataBuffer;

    Srb->DataBuffer = DeviceObject;

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDispatchScsi
 *
 * DESCRIPTION
 *	Answer requests for SCSI calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS NTAPI
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PIO_STACK_LOCATION Stack;
    PSCSI_REQUEST_BLOCK Srb;
    KIRQL Irql;
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP NextIrp, IrpList;
    PKDEVICE_QUEUE_ENTRY Entry;

    DPRINT("ScsiPortDispatchScsi(DeviceObject %p  Irp %p)\n",
        DeviceObject, Irp);

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    Srb = Stack->Parameters.Scsi.Srb;
    if (Srb == NULL)
    {
        DPRINT1("ScsiPortDispatchScsi() called with Srb = NULL!\n");
        Status = STATUS_UNSUCCESSFUL;

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return(Status);
    }

    DPRINT("Srb: %p\n", Srb);
    DPRINT("Srb->Function: %lu\n", Srb->Function);
    DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n", Srb->PathId, Srb->TargetId, Srb->Lun);

    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun);
    if (LunExtension == NULL)
    {
        DPRINT("ScsiPortDispatchScsi() called with an invalid LUN\n");
        Status = STATUS_NO_SUCH_DEVICE;

        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return(Status);
    }

    switch (Srb->Function)
    {
    case SRB_FUNCTION_SHUTDOWN:
    case SRB_FUNCTION_FLUSH:
        DPRINT ("  SRB_FUNCTION_SHUTDOWN or FLUSH\n");
        if (DeviceExtension->CachesData == FALSE)
        {
            /* All success here */
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        /* Fall through to a usual execute operation */

    case SRB_FUNCTION_EXECUTE_SCSI:
    case SRB_FUNCTION_IO_CONTROL:
        DPRINT("  SRB_FUNCTION_EXECUTE_SCSI or SRB_FUNCTION_IO_CONTROL\n");
        /* Mark IRP as pending in all cases */
        IoMarkIrpPending(Irp);

        if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
        {
            /* Start IO directly */
            IoStartPacket(DeviceObject, Irp, NULL, NULL);
        }
        else
        {
            KIRQL oldIrql;

            /* We need to be at DISPATCH_LEVEL */
            KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);

            /* Insert IRP into the queue */
            if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                &Irp->Tail.Overlay.DeviceQueueEntry,
                Srb->QueueSortKey))
            {
                /* It means the queue is empty, and we just start this request */
                IoStartPacket(DeviceObject, Irp, NULL, NULL);
            }

            /* Back to the old IRQL */
            KeLowerIrql (oldIrql);
        }
        return STATUS_PENDING;

    case SRB_FUNCTION_CLAIM_DEVICE:
    case SRB_FUNCTION_ATTACH_DEVICE:
        DPRINT ("  SRB_FUNCTION_CLAIM_DEVICE or ATTACH\n");

        /* Reference device object and keep the device object */
        Status = SpiHandleAttachRelease(DeviceExtension, Irp);
        break;

    case SRB_FUNCTION_RELEASE_DEVICE:
        DPRINT ("  SRB_FUNCTION_RELEASE_DEVICE\n");

        /* Dereference device object and clear the device object */
        Status = SpiHandleAttachRelease(DeviceExtension, Irp);
        break;

    case SRB_FUNCTION_RELEASE_QUEUE:
        DPRINT("  SRB_FUNCTION_RELEASE_QUEUE\n");

        /* Guard with the spinlock */
        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        if (!(LunExtension->Flags & LUNEX_FROZEN_QUEUE))
        {
            DPRINT("Queue is not frozen really\n");

            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;

        }

        /* Unfreeze the queue */
        LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

        if (LunExtension->SrbInfo.Srb == NULL)
        {
            /* Get next logical unit request */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);

            /* SpiGetNextRequestFromLun() releases the spinlock */
            KeLowerIrql(Irql);
        }
        else
        {
            DPRINT("The queue has active request\n");
            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        }


        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Status = STATUS_SUCCESS;
        break;

    case SRB_FUNCTION_FLUSH_QUEUE:
        DPRINT("  SRB_FUNCTION_FLUSH_QUEUE\n");

        /* Guard with the spinlock */
        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        if (!(LunExtension->Flags & LUNEX_FROZEN_QUEUE))
        {
            DPRINT("Queue is not frozen really\n");

            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        /* Make sure there is no active request */
        ASSERT(LunExtension->SrbInfo.Srb == NULL);

        /* Compile a list from the device queue */
        IrpList = NULL;
        while ((Entry = KeRemoveDeviceQueue(&LunExtension->DeviceQueue)) != NULL)
        {
                NextIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);

                /* Get the Srb */
                Stack = IoGetCurrentIrpStackLocation(NextIrp);
                Srb = Stack->Parameters.Scsi.Srb;

                /* Set statuse */
                Srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                NextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                /* Add then to the list */
                NextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)IrpList;
                IrpList = NextIrp;
        }

        /* Unfreeze the queue */
        LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

        /* Release the spinlock */
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

        /* Complete those requests */
        while (IrpList)
        {
            NextIrp = IrpList;
            IrpList = (PIRP)NextIrp->Tail.Overlay.ListEntry.Flink;

            IoCompleteRequest(NextIrp, 0);
        }

        Status = STATUS_SUCCESS;
        break;

    default:
        DPRINT1("SRB function not implemented (Function %lu)\n", Srb->Function);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDeviceControl
 *
 * DESCRIPTION
 *	Answer requests for device control calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS NTAPI
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PDUMP_POINTERS DumpPointers;
    NTSTATUS Status;

    DPRINT("ScsiPortDeviceControl()\n");

    Irp->IoStatus.Information = 0;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_SCSI_GET_DUMP_POINTERS:
        DPRINT("  IOCTL_SCSI_GET_DUMP_POINTERS\n");

        if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DUMP_POINTERS))
        {
          Status = STATUS_BUFFER_OVERFLOW;
          Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
          break;
        }

        DumpPointers = Irp->AssociatedIrp.SystemBuffer;
        DumpPointers->DeviceObject = DeviceObject;
        /* More data.. ? */

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
        break;

      case IOCTL_SCSI_GET_CAPABILITIES:
        DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");
        if (Stack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(PVOID))
        {
            *((PVOID *)Irp->AssociatedIrp.SystemBuffer) = &DeviceExtension->PortCapabilities;

            Irp->IoStatus.Information = sizeof(PVOID);
            Status = STATUS_SUCCESS;
            break;
        }

        if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IO_SCSI_CAPABILITIES))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      &DeviceExtension->PortCapabilities,
                      sizeof(IO_SCSI_CAPABILITIES));

        Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
        Status = STATUS_SUCCESS;
        break;

      case IOCTL_SCSI_GET_INQUIRY_DATA:
          DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

          /* Copy inquiry data to the port device extension */
          Status = SpiGetInquiryData(DeviceExtension, Irp);
          break;

      case IOCTL_SCSI_MINIPORT:
          DPRINT1("IOCTL_SCSI_MINIPORT unimplemented!\n");
          Status = STATUS_NOT_IMPLEMENTED;
          break;

      case IOCTL_SCSI_PASS_THROUGH:
          DPRINT1("IOCTL_SCSI_PASS_THROUGH unimplemented!\n");
          Status = STATUS_NOT_IMPLEMENTED;
          break;

      default:
          if (DEVICE_TYPE_FROM_CTL_CODE(Stack->Parameters.DeviceIoControl.IoControlCode) == MOUNTDEVCONTROLTYPE)
          {
            switch (Stack->Parameters.DeviceIoControl.IoControlCode)
            {
            case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
                DPRINT1("Got unexpected IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n");
                break;
            case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
                DPRINT1("Got unexpected IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n");
                break;
            default:
                DPRINT("  got ioctl intended for the mount manager: 0x%lX\n", Stack->Parameters.DeviceIoControl.IoControlCode);
                break;
            }
          } else {
            DPRINT1("  unknown ioctl code: 0x%lX\n", Stack->Parameters.DeviceIoControl.IoControlCode);
          }
          Status = STATUS_NOT_IMPLEMENTED;
          break;
    }

    /* Complete the request with the given status */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


static VOID NTAPI
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    LONG CounterResult;
    NTSTATUS Status;

    DPRINT("ScsiPortStartIo() called!\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DeviceExtension %p\n", DeviceExtension);

    Srb = IrpStack->Parameters.Scsi.Srb;

    /* Apply "default" flags */
    Srb->SrbFlags |= DeviceExtension->SrbFlags;

    /* Get LUN extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun);

    if (DeviceExtension->NeedSrbDataAlloc ||
        DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* Allocate them */
        SrbInfo = SpiAllocateSrbStructures(DeviceExtension,
                                           LunExtension,
                                           Srb);

        /* Couldn't alloc one or both data structures, return */
        if (SrbInfo == NULL)
        {
            /* We have to call IoStartNextPacket, because this request
               was not started */
            if (LunExtension->Flags & LUNEX_REQUEST_PENDING)
                IoStartNextPacket(DeviceObject, FALSE);

            return;
        }
    }
    else
    {
        /* No allocations are needed */
        SrbInfo = &LunExtension->SrbInfo;
        Srb->SrbExtension = NULL;
        Srb->QueueTag = SP_UNTAGGED;
    }

    /* Increase sequence number of SRB */
    if (!SrbInfo->SequenceNumber)
    {
        /* Increase global sequence number */
        DeviceExtension->SequenceNumber++;

        /* Assign it */
        SrbInfo->SequenceNumber = DeviceExtension->SequenceNumber;
    }

    /* Check some special SRBs */
    if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
    {
        /* Some special handling */
        DPRINT1("Abort command! Unimplemented now\n");
    }
    else
    {
        SrbInfo->Srb = Srb;
    }

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
    {
        // Store the MDL virtual address in SrbInfo structure
        SrbInfo->DataOffset = MmGetMdlVirtualAddress(Irp->MdlAddress);

        if (DeviceExtension->MapBuffers)
        {
            /* Calculate offset within DataBuffer */
            SrbInfo->DataOffset = MmGetSystemAddressForMdl(Irp->MdlAddress);
            Srb->DataBuffer = SrbInfo->DataOffset +
                (ULONG)((PUCHAR)Srb->DataBuffer -
                (PUCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        if (DeviceExtension->AdapterObject)
        {
            /* Flush buffers */
            KeFlushIoBuffers(Irp->MdlAddress,
                             Srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE,
                             TRUE);
        }

        if (DeviceExtension->MapRegisters)
        {
            /* Calculate number of needed map registers */
            SrbInfo->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    Srb->DataBuffer,
                    Srb->DataTransferLength);

            /* Allocate adapter channel */
            Status = IoAllocateAdapterChannel(DeviceExtension->AdapterObject,
                                              DeviceExtension->DeviceObject,
                                              SrbInfo->NumberOfMapRegisters,
                                              SpiAdapterControl,
                                              SrbInfo);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoAllocateAdapterChannel() failed!\n");

                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                ScsiPortNotification(RequestComplete,
                                     DeviceExtension + 1,
                                     Srb);

                ScsiPortNotification(NextRequest,
                                     DeviceExtension + 1);

                /* Request DPC for that work */
                IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);
            }

            /* Control goes to SpiAdapterControl */
            return;
        }
    }

    /* Increase active request counter */
    CounterResult = InterlockedIncrement(&DeviceExtension->ActiveRequestCounter);

    if (CounterResult == 0 &&
        DeviceExtension->AdapterObject != NULL &&
        !DeviceExtension->MapRegisters)
    {
        IoAllocateAdapterChannel(
            DeviceExtension->AdapterObject,
            DeviceObject,
            DeviceExtension->PortCapabilities.MaximumPhysicalPages,
            ScsiPortAllocateAdapterChannel,
            LunExtension
            );

        return;
    }

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                ScsiPortStartPacket,
                                DeviceObject))
    {
        DPRINT("Synchronization failed!\n");

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        /* Release the spinlock only */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }


    DPRINT("ScsiPortStartIo() done\n");
}


static BOOLEAN NTAPI
ScsiPortStartPacket(IN OUT PVOID Context)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    BOOLEAN Result;
    BOOLEAN StartTimer;

    DPRINT("ScsiPortStartPacket() called\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IrpStack = IoGetCurrentIrpStackLocation(DeviceObject->CurrentIrp);
    Srb = IrpStack->Parameters.Scsi.Srb;

    /* Get LUN extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun);

    /* Check if we are in a reset state */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET)
    {
        /* Mark the we've got requests while being in the reset state */
        DeviceExtension->InterruptData.Flags |= SCSI_PORT_RESET_REQUEST;
        return TRUE;
    }

    /* Set the time out value */
    DeviceExtension->TimerCount = Srb->TimeOutValue;

    /* We are busy */
    DeviceExtension->Flags |= SCSI_PORT_DEVICE_BUSY;

    if (LunExtension->RequestTimeout != -1)
    {
        /* Timer already active */
        StartTimer = FALSE;
    }
    else
    {
        /* It hasn't been initialized yet */
        LunExtension->RequestTimeout = Srb->TimeOutValue;
        StartTimer = TRUE;
    }

    if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
    {
        /* Handle bypass-requests */

        /* Is this an abort request? */
        if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
        {
            /* Get pointer to SRB info structure */
            SrbInfo = SpiGetSrbData(DeviceExtension,
                                    Srb->PathId,
                                    Srb->TargetId,
                                    Srb->Lun,
                                    Srb->QueueTag);

            /* Check if the request is still "active" */
            if (SrbInfo == NULL ||
                SrbInfo->Srb == NULL ||
                !(SrbInfo->Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE))
            {
                /* It's not, mark it as active then */
                Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

                if (StartTimer)
                    LunExtension->RequestTimeout = -1;

                DPRINT("Request has been already completed, but abort request came\n");
                Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

                /* Notify about request complete */
                ScsiPortNotification(RequestComplete,
                                     DeviceExtension->MiniPortDeviceExtension,
                                     Srb);

                /* and about readiness for the next request */
                ScsiPortNotification(NextRequest,
                                     DeviceExtension->MiniPortDeviceExtension);

                /* They might ask for some work, so queue the DPC for them */
                IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);

                /* We're done in this branch */
                return TRUE;
            }
        }
        else
        {
            /* Add number of queued requests */
            LunExtension->QueueCount++;
        }

        /* Bypass requests don't need request sense */
        LunExtension->Flags &= ~LUNEX_NEED_REQUEST_SENSE;

        /* Is disconnect disabled for this request? */
        if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        {
            /* Set the corresponding flag */
            DeviceExtension->Flags &= ~SCSI_PORT_DISCONNECT_ALLOWED;
        }

        /* Transfer timeout value from Srb to Lun */
        LunExtension->RequestTimeout = Srb->TimeOutValue;
    }
    else
    {
        if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        {
            /* It's a disconnect, so no more requests can go */
            DeviceExtension->Flags &= ~SCSI_PORT_DISCONNECT_ALLOWED;
        }

        LunExtension->Flags |= SCSI_PORT_LU_ACTIVE;

        /* Increment queue count */
        LunExtension->QueueCount++;

        /* If it's tagged - special thing */
        if (Srb->QueueTag != SP_UNTAGGED)
        {
            SrbInfo = &DeviceExtension->SrbInfo[Srb->QueueTag - 1];

            /* Chek for consistency */
            ASSERT(SrbInfo->Requests.Blink == NULL);

            /* Insert it into the list of requests */
            InsertTailList(&LunExtension->SrbInfo.Requests, &SrbInfo->Requests);
        }
    }

    /* Mark this Srb active */
    Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

    /* Call HwStartIo routine */
    Result = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
                                        Srb);

    /* If notification is needed, then request a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
        IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);

    return Result;
}

IO_ALLOCATION_ACTION
NTAPI
SpiAdapterControl(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp,
                  PVOID MapRegisterBase,
                  PVOID Context)
{
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_SG_ADDRESS ScatterGatherList;
    KIRQL CurrentIrql;
    PIO_STACK_LOCATION IrpStack;
    ULONG TotalLength = 0;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PUCHAR DataVA;
    BOOLEAN WriteToDevice;

    /* Get pointers to SrbInfo and DeviceExtension */
    SrbInfo = (PSCSI_REQUEST_BLOCK_INFO)Context;
    DeviceExtension = DeviceObject->DeviceExtension;

    /* Get pointer to SRB */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Srb = (PSCSI_REQUEST_BLOCK)IrpStack->Parameters.Others.Argument1;

    /* Depending on the map registers number, we allocate
       either from NonPagedPool, or from our static list */
    if (SrbInfo->NumberOfMapRegisters > MAX_SG_LIST)
    {
        SrbInfo->ScatterGather = ExAllocatePoolWithTag(
            NonPagedPool, SrbInfo->NumberOfMapRegisters * sizeof(SCSI_SG_ADDRESS), TAG_SCSIPORT);

        if (SrbInfo->ScatterGather == NULL)
            ASSERT(FALSE);

        Srb->SrbFlags |= SRB_FLAGS_SGLIST_FROM_POOL;
    }
    else
    {
        SrbInfo->ScatterGather = SrbInfo->ScatterGatherList;
    }

    /* Use chosen SG list source */
    ScatterGatherList = SrbInfo->ScatterGather;

    /* Save map registers base */
    SrbInfo->BaseOfMapRegister = MapRegisterBase;

    /* Determine WriteToDevice flag */
    WriteToDevice = Srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE;

    /* Get virtual address of the data buffer */
    DataVA = (PUCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress) +
                ((PCHAR)Srb->DataBuffer - SrbInfo->DataOffset);

    /* Build the actual SG list */
    while (TotalLength < Srb->DataTransferLength)
    {
        if (!ScatterGatherList)
            break;

        ScatterGatherList->Length = Srb->DataTransferLength - TotalLength;
        ScatterGatherList->PhysicalAddress = IoMapTransfer(DeviceExtension->AdapterObject,
                                                           Irp->MdlAddress,
                                                           MapRegisterBase,
                                                           DataVA + TotalLength,
                                                           &ScatterGatherList->Length,
                                                           WriteToDevice);

        TotalLength += ScatterGatherList->Length;
        ScatterGatherList++;
    }

    /* Schedule an active request */
    InterlockedIncrement(&DeviceExtension->ActiveRequestCounter );
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &CurrentIrql);
    KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                           ScsiPortStartPacket,
                           DeviceObject);
    KeReleaseSpinLock(&DeviceExtension->SpinLock, CurrentIrql);

    return DeallocateObjectKeepRegisters;
}

static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG LunExtensionSize;

    DPRINT("SpiAllocateLunExtension(%p)\n", DeviceExtension);

    /* Round LunExtensionSize first to the sizeof LONGLONG */
    LunExtensionSize = (DeviceExtension->LunExtensionSize +
                        sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    LunExtensionSize += sizeof(SCSI_PORT_LUN_EXTENSION);
    DPRINT("LunExtensionSize %lu\n", LunExtensionSize);

    LunExtension = ExAllocatePoolWithTag(NonPagedPool, LunExtensionSize, TAG_SCSIPORT);
    if (LunExtension == NULL)
    {
        DPRINT1("Out of resources!\n");
        return NULL;
    }

    /* Zero everything */
    RtlZeroMemory(LunExtension, LunExtensionSize);

    /* Initialize a list of requests */
    InitializeListHead(&LunExtension->SrbInfo.Requests);

    /* Initialize timeout counter */
    LunExtension->RequestTimeout = -1;

    /* Set maximum queue size */
    LunExtension->MaxQueueCount = 256;

    /* Initialize request queue */
    KeInitializeDeviceQueue(&LunExtension->DeviceQueue);

    return LunExtension;
}

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    DPRINT("SpiGetLunExtension(%p %u %u %u) called\n",
        DeviceExtension, PathId, TargetId, Lun);

    /* Get appropriate list */
    LunExtension = DeviceExtension->LunExtensionList[(TargetId + Lun) % LUS_NUMBER];

    /* Iterate it until we find what we need */
    while (LunExtension)
    {
        if (LunExtension->TargetId == TargetId &&
            LunExtension->Lun == Lun &&
            LunExtension->PathId == PathId)
        {
            /* All matches, return */
            return LunExtension;
        }

        /* Advance to the next item */
        LunExtension = LunExtension->Next;
    }

    /* We did not find anything */
    DPRINT("Nothing found\n");
    return NULL;
}

static PSCSI_REQUEST_BLOCK_INFO
SpiAllocateSrbStructures(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         PSCSI_PORT_LUN_EXTENSION LunExtension,
                         PSCSI_REQUEST_BLOCK Srb)
{
    PCHAR SrbExtension;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;

    /* Spinlock must be held while this function executes */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Allocate SRB data structure */
    if (DeviceExtension->NeedSrbDataAlloc)
    {
        /* Treat the abort request in a special way */
        if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
        {
            SrbInfo = SpiGetSrbData(DeviceExtension,
                                    Srb->PathId,
                                    Srb->TargetId,
                                    Srb->Lun,
                                    Srb->QueueTag);
        }
        else if (Srb->SrbFlags &
                 (SRB_FLAGS_QUEUE_ACTION_ENABLE | SRB_FLAGS_NO_QUEUE_FREEZE) &&
                 !(Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
                 )
        {
            /* Do not process tagged commands if need request sense is set */
            if (LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE)
            {
                ASSERT(!(LunExtension->Flags & LUNEX_REQUEST_PENDING));

                LunExtension->PendingRequest = Srb->OriginalRequest;
                LunExtension->Flags |= LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE;

                /* Release the spinlock and return */
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return NULL;
            }

            ASSERT(LunExtension->SrbInfo.Srb == NULL);
            SrbInfo = DeviceExtension->FreeSrbInfo;

            if (SrbInfo == NULL)
            {
                /* No SRB structures left in the list. We have to leave
                   and wait while we are called again */

                DeviceExtension->Flags |= SCSI_PORT_REQUEST_PENDING;
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return NULL;
            }

            DeviceExtension->FreeSrbInfo = (PSCSI_REQUEST_BLOCK_INFO)SrbInfo->Requests.Flink;

            /* QueueTag must never be 0, so +1 to it */
            Srb->QueueTag = (UCHAR)(SrbInfo - DeviceExtension->SrbInfo) + 1;
        }
        else
        {
            /* Usual untagged command */
            if (
                (!IsListEmpty(&LunExtension->SrbInfo.Requests) ||
                LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE) &&
                !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
                )
            {
                /* Mark it as pending and leave */
                ASSERT(!(LunExtension->Flags & LUNEX_REQUEST_PENDING));
                LunExtension->Flags |= LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE;
                LunExtension->PendingRequest = Srb->OriginalRequest;

                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return(NULL);
            }

            Srb->QueueTag = SP_UNTAGGED;
            SrbInfo = &LunExtension->SrbInfo;
        }
    }
    else
    {
        Srb->QueueTag = SP_UNTAGGED;
        SrbInfo = &LunExtension->SrbInfo;
    }

    /* Allocate SRB extension structure */
    if (DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* Check the list of free extensions */
        SrbExtension = DeviceExtension->FreeSrbExtensions;

        /* If no free extensions... */
        if (SrbExtension == NULL)
        {
            /* Free SRB data */
            if (Srb->Function != SRB_FUNCTION_ABORT_COMMAND &&
                Srb->QueueTag != SP_UNTAGGED)
            {
                SrbInfo->Requests.Blink = NULL;
                SrbInfo->Requests.Flink = (PLIST_ENTRY)DeviceExtension->FreeSrbInfo;
                DeviceExtension->FreeSrbInfo = SrbInfo;
            }

            /* Return, in order to be called again later */
            DeviceExtension->Flags |= SCSI_PORT_REQUEST_PENDING;
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
            return NULL;
        }

        /* Remove that free SRB extension from the list (since
           we're going to use it) */
        DeviceExtension->FreeSrbExtensions = *((PVOID *)SrbExtension);

        /* Spinlock can be released now */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        Srb->SrbExtension = SrbExtension;

        if (Srb->SenseInfoBuffer != NULL &&
            DeviceExtension->SupportsAutoSense)
        {
            /* Store pointer to the SenseInfo buffer */
            SrbInfo->SaveSenseRequest = Srb->SenseInfoBuffer;

            /* Does data fit the buffer? */
            if (Srb->SenseInfoBufferLength > sizeof(SENSE_DATA))
            {
                /* No, disabling autosense at all */
                Srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;
            }
            else
            {
                /* Yes, update the buffer pointer */
                Srb->SenseInfoBuffer = SrbExtension + DeviceExtension->SrbExtensionSize;
            }
        }
    }
    else
    {
        /* Cleanup... */
        Srb->SrbExtension = NULL;
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    return SrbInfo;
}


static NTSTATUS
SpiSendInquiry(IN PDEVICE_OBJECT DeviceObject,
               IN OUT PSCSI_LUN_INFO LunInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    KIRQL Irql;
    PIRP Irp;
    NTSTATUS Status;
    PINQUIRYDATA InquiryBuffer;
    PSENSE_DATA SenseBuffer;
    BOOLEAN KeepTrying = TRUE;
    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

    DPRINT("SpiSendInquiry() called\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    InquiryBuffer = ExAllocatePoolWithTag(NonPagedPool, INQUIRYDATABUFFERSIZE, TAG_SCSIPORT);
    if (InquiryBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SenseBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE, TAG_SCSIPORT);
    if (SenseBuffer == NULL)
    {
        ExFreePoolWithTag(InquiryBuffer, TAG_SCSIPORT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (KeepTrying)
    {
        /* Initialize event for waiting */
        KeInitializeEvent(&Event,
                          NotificationEvent,
                          FALSE);

        /* Create an IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            InquiryBuffer,
                                            INQUIRYDATABUFFERSIZE,
                                            TRUE,
                                            &Event,
                                            &IoStatusBlock);
        if (Irp == NULL)
        {
            DPRINT("IoBuildDeviceIoControlRequest() failed\n");

            /* Quit the loop */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KeepTrying = FALSE;
            continue;
        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb.OriginalRequest = Irp;
        Srb.PathId = LunInfo->PathId;
        Srb.TargetId = LunInfo->TargetId;
        Srb.Lun = LunInfo->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation (Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = LunInfo->Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING)
        {
            DPRINT("SpiSendInquiry(): Waiting for the driver to process request...\n");
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        DPRINT("SpiSendInquiry(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            /* All fine, copy data over */
            RtlCopyMemory(LunInfo->InquiryData,
                          InquiryBuffer,
                          INQUIRYDATABUFFERSIZE);

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
            continue;
        }

        DPRINT("Inquiry SRB failed with SrbStatus 0x%08X\n", Srb.SrbStatus);

        /* Check if the queue is frozen */
        if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            /* Something weird happened, deal with it (unfreeze the queue) */
            KeepTrying = FALSE;

            DPRINT("SpiSendInquiry(): the queue is frozen at TargetId %d\n", Srb.TargetId);

            LunExtension = SpiGetLunExtension(DeviceExtension,
                                              LunInfo->PathId,
                                              LunInfo->TargetId,
                                              LunInfo->Lun);

            /* Clear frozen flag */
            LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Acquire the spinlock */
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

            /* Process the request */
            SpiGetNextRequestFromLun(DeviceObject->DeviceExtension, LunExtension);

            /* SpiGetNextRequestFromLun() releases the spinlock,
                so we just lower irql back to what it was before */
            KeLowerIrql(Irql);
        }

        /* Check if data overrun happened */
        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            DPRINT("Data overrun at TargetId %d\n", LunInfo->TargetId);

            /* Nothing dramatic, just copy data, but limiting the size */
            RtlCopyMemory(LunInfo->InquiryData,
                            InquiryBuffer,
                            (Srb.DataTransferLength > INQUIRYDATABUFFERSIZE) ?
                            INQUIRYDATABUFFERSIZE : Srb.DataTransferLength);

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
        }
        else if ((Srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                 SenseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)
        {
            /* LUN is not valid, but some device responds there.
                Mark it as invalid anyway */

            /* Quit the loop */
            Status = STATUS_INVALID_DEVICE_REQUEST;
            KeepTrying = FALSE;
        }
        else
        {
            /* Retry a couple of times if no timeout happened */
            if ((RetryCount < 2) &&
                (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_NO_DEVICE) &&
                (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT))
            {
                RetryCount++;
                KeepTrying = TRUE;
            }
            else
            {
                /* That's all, quit the loop */
                KeepTrying = FALSE;

                /* Set status according to SRB status */
                if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_FUNCTION ||
                    SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_SRB_BLOCK_LENGTH)
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                }
                else
                {
                    Status = STATUS_IO_DEVICE_ERROR;
                }
            }
        }
    }

    /* Free buffers */
    ExFreePoolWithTag(InquiryBuffer, TAG_SCSIPORT);
    ExFreePoolWithTag(SenseBuffer, TAG_SCSIPORT);

    DPRINT("SpiSendInquiry() done with Status 0x%08X\n", Status);

    return Status;
}


/* Scans all SCSI buses */
static VOID
SpiScanAdapter(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG Bus;
    ULONG Target;
    ULONG Lun;
    PSCSI_BUS_SCAN_INFO BusScanInfo;
    PSCSI_LUN_INFO LastLunInfo, LunInfo, LunInfoExists;
    BOOLEAN DeviceExists;
    ULONG Hint;
    NTSTATUS Status;
    ULONG DevicesFound;

    DPRINT("SpiScanAdapter() called\n");

    /* Scan all buses */
    for (Bus = 0; Bus < DeviceExtension->BusNum; Bus++)
    {
        DPRINT("    Scanning bus %d\n", Bus);
        DevicesFound = 0;

        /* Get pointer to the scan information */
        BusScanInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus];

        if (BusScanInfo)
        {
            /* Find the last LUN info in the list */
            LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;
            LastLunInfo = LunInfo;

            while (LunInfo != NULL)
            {
                LastLunInfo = LunInfo;
                LunInfo = LunInfo->Next;
            }
        }
        else
        {
            /* We need to allocate this buffer */
            BusScanInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_BUS_SCAN_INFO), TAG_SCSIPORT);
            if (!BusScanInfo)
            {
                DPRINT1("Out of resources!\n");
                return;
            }

            /* Store the pointer in the BusScanInfo array */
            DeviceExtension->BusesConfig->BusScanInfo[Bus] = BusScanInfo;

            /* Fill this struct (length and bus ids for now) */
            BusScanInfo->Length = sizeof(SCSI_BUS_SCAN_INFO);
            BusScanInfo->LogicalUnitsCount = 0;
            BusScanInfo->BusIdentifier = DeviceExtension->PortConfig->InitiatorBusId[Bus];
            BusScanInfo->LunInfo = NULL;

            /* Set pointer to the last LUN info to NULL */
            LastLunInfo = NULL;
        }

        /* Create LUN information structure */
        LunInfo = ExAllocatePoolWithTag(PagedPool, sizeof(SCSI_LUN_INFO), TAG_SCSIPORT);
        if (!LunInfo)
        {
            DPRINT1("Out of resources!\n");
            return;
        }

        RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

        /* Create LunExtension */
        LunExtension = SpiAllocateLunExtension(DeviceExtension);

        /* And send INQUIRY to every target */
        for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
        {
            /* TODO: Support scan bottom-up */

            /* Skip if it's the same address */
            if (Target == BusScanInfo->BusIdentifier)
                continue;

            /* Try to find an existing device here */
            DeviceExists = FALSE;
            LunInfoExists = BusScanInfo->LunInfo;

            /* Find matching address on this bus */
            while (LunInfoExists)
            {
                if (LunInfoExists->TargetId == Target)
                {
                    DeviceExists = TRUE;
                    break;
                }

                /* Advance to the next one */
                LunInfoExists = LunInfoExists->Next;
            }

            /* No need to bother rescanning, since we already did that before */
            if (DeviceExists)
                continue;

            /* Scan all logical units */
            for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
            {
                if ((!LunExtension) || (!LunInfo))
                    break;

                /* Add extension to the list */
                Hint = (Target + Lun) % LUS_NUMBER;
                LunExtension->Next = DeviceExtension->LunExtensionList[Hint];
                DeviceExtension->LunExtensionList[Hint] = LunExtension;

                /* Fill Path, Target, Lun fields */
                LunExtension->PathId = LunInfo->PathId = (UCHAR)Bus;
                LunExtension->TargetId = LunInfo->TargetId = (UCHAR)Target;
                LunExtension->Lun = LunInfo->Lun = (UCHAR)Lun;

                /* Set flag to prevent race conditions */
                LunExtension->Flags |= SCSI_PORT_SCAN_IN_PROGRESS;

                /* Zero LU extension contents */
                if (DeviceExtension->LunExtensionSize)
                {
                    RtlZeroMemory(LunExtension + 1,
                                  DeviceExtension->LunExtensionSize);
                }

                /* Finally send the inquiry command */
                Status = SpiSendInquiry(DeviceExtension->DeviceObject, LunInfo);

                if (NT_SUCCESS(Status))
                {
                    /* Let's see if we really found a device */
                    PINQUIRYDATA InquiryData = (PINQUIRYDATA)LunInfo->InquiryData;

                    /* Check if this device is unsupported */
                    if (InquiryData->DeviceTypeQualifier == DEVICE_QUALIFIER_NOT_SUPPORTED)
                    {
                        DeviceExtension->LunExtensionList[Hint] =
                            DeviceExtension->LunExtensionList[Hint]->Next;

                        continue;
                    }

                    /* Clear the "in scan" flag */
                    LunExtension->Flags &= ~SCSI_PORT_SCAN_IN_PROGRESS;

                    DPRINT("SpiScanAdapter(): Found device of type %d at bus %d tid %d lun %d\n",
                        InquiryData->DeviceType, Bus, Target, Lun);

                    /*
                     * Cache the inquiry data into the LUN extension (or alternatively
                     * we could save a pointer to LunInfo within the LunExtension?)
                     */
                    RtlCopyMemory(&LunExtension->InquiryData,
                                  InquiryData,
                                  INQUIRYDATABUFFERSIZE);

                    /* Add this info to the linked list */
                    LunInfo->Next = NULL;
                    if (LastLunInfo)
                        LastLunInfo->Next = LunInfo;
                    else
                        BusScanInfo->LunInfo = LunInfo;

                    /* Store the last LUN info */
                    LastLunInfo = LunInfo;

                    /* Store DeviceObject */
                    LunInfo->DeviceObject = DeviceExtension->DeviceObject;

                    /* Allocate another buffer */
                    LunInfo = ExAllocatePoolWithTag(PagedPool, sizeof(SCSI_LUN_INFO), TAG_SCSIPORT);
                    if (!LunInfo)
                    {
                        DPRINT1("Out of resources!\n");
                        break;
                    }

                    RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

                    /* Create a new LU extension */
                    LunExtension = SpiAllocateLunExtension(DeviceExtension);

                    DevicesFound++;
                }
                else
                {
                    /* Remove this LUN from the list */
                    DeviceExtension->LunExtensionList[Hint] =
                        DeviceExtension->LunExtensionList[Hint]->Next;

                    /* Decide whether we are continuing or not */
                    if (Status == STATUS_INVALID_DEVICE_REQUEST)
                        continue;
                    else
                        break;
                }
            }
        }

        /* Free allocated buffers */
        if (LunExtension)
            ExFreePoolWithTag(LunExtension, TAG_SCSIPORT);

        if (LunInfo)
            ExFreePoolWithTag(LunInfo, TAG_SCSIPORT);

        /* Sum what we found */
        BusScanInfo->LogicalUnitsCount += (UCHAR)DevicesFound;
        DPRINT("    Found %d devices on bus %d\n", DevicesFound, Bus);
    }

    DPRINT("SpiScanAdapter() done\n");
}


static NTSTATUS
SpiGetInquiryData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                  IN PIRP Irp)
{
    ULONG InquiryDataSize;
    PSCSI_LUN_INFO LunInfo;
    ULONG BusCount, LunCount, Length;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PSCSI_BUS_DATA BusData;
    ULONG Bus;
    PUCHAR Buffer;

    DPRINT("SpiGetInquiryData() called\n");

    /* Get pointer to the buffer */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    /* Initialize bus and LUN counters */
    BusCount = DeviceExtension->BusesConfig->NumberOfBuses;
    LunCount = 0;

    /* Calculate total number of LUNs */
    for (Bus = 0; Bus < BusCount; Bus++)
        LunCount += DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

    /* Calculate size of inquiry data, rounding up to sizeof(ULONG) */
    InquiryDataSize =
        ((sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE +
        sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

    /* Calculate data size */
    Length = sizeof(SCSI_ADAPTER_BUS_INFO) + (BusCount - 1) * sizeof(SCSI_BUS_DATA);

    Length += InquiryDataSize * LunCount;

    /* Check, if all data is going to fit into provided buffer */
    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < Length)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Store data size in the IRP */
    Irp->IoStatus.Information = Length;

    DPRINT("Data size: %lu\n", Length);

    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;

    AdapterBusInfo->NumberOfBuses = (UCHAR)BusCount;

    /* Point InquiryData to the corresponding place inside Buffer */
    InquiryData = (PSCSI_INQUIRY_DATA)(Buffer + sizeof(SCSI_ADAPTER_BUS_INFO) +
                    (BusCount - 1) * sizeof(SCSI_BUS_DATA));

    /* Loop each bus */
    for (Bus = 0; Bus < BusCount; Bus++)
    {
        BusData = &AdapterBusInfo->BusData[Bus];

        /* Calculate and save an offset of the inquiry data */
        BusData->InquiryDataOffset = (ULONG)((PUCHAR)InquiryData - Buffer);

        /* Get a pointer to the LUN information structure */
        LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;

        /* Store Initiator Bus Id */
        BusData->InitiatorBusId =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->BusIdentifier;

        /* Store LUN count */
        BusData->NumberOfLogicalUnits =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

        /* Loop all LUNs */
        while (LunInfo != NULL)
        {
            DPRINT("(Bus %lu Target %lu Lun %lu)\n",
                   Bus, LunInfo->TargetId, LunInfo->Lun);

            /* Fill InquiryData with values */
            InquiryData->PathId = LunInfo->PathId;
            InquiryData->TargetId = LunInfo->TargetId;
            InquiryData->Lun = LunInfo->Lun;
            InquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
            InquiryData->DeviceClaimed = LunInfo->DeviceClaimed;
            InquiryData->NextInquiryDataOffset =
                (ULONG)((PUCHAR)InquiryData + InquiryDataSize - Buffer);

            /* Copy data in it */
            RtlCopyMemory(InquiryData->InquiryData,
                          LunInfo->InquiryData,
                          INQUIRYDATABUFFERSIZE);

            /* Move to the next LUN */
            LunInfo = LunInfo->Next;
            InquiryData = (PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData + InquiryDataSize);
        }

        /* Either mark the end, or set offset to 0 */
        if (BusData->NumberOfLogicalUnits != 0)
            ((PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData - InquiryDataSize))->NextInquiryDataOffset = 0;
        else
            BusData->InquiryDataOffset = 0;
    }

    /* Finish with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

static PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
              IN UCHAR PathId,
              IN UCHAR TargetId,
              IN UCHAR Lun,
              IN UCHAR QueueTag)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    if (QueueTag == SP_UNTAGGED)
    {
        /* Untagged request, get LU and return pointer to SrbInfo */
        LunExtension = SpiGetLunExtension(DeviceExtension,
                                          PathId,
                                          TargetId,
                                          Lun);

        /* Return NULL in case of error */
        if (!LunExtension)
            return(NULL);

        /* Return the pointer to SrbInfo */
        return &LunExtension->SrbInfo;
    }
    else
    {
        /* Make sure the tag is valid, if it is - return the data */
        if (QueueTag > DeviceExtension->SrbDataCount || QueueTag < 1)
            return NULL;
        else
            return &DeviceExtension->SrbInfo[QueueTag -1];
    }
}

static VOID
SpiSendRequestSense(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    IN PSCSI_REQUEST_BLOCK InitialSrb)
{
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    LARGE_INTEGER LargeInt;
    PVOID *Ptr;

    DPRINT("SpiSendRequestSense() entered, InitialSrb %p\n", InitialSrb);

    /* Allocate Srb */
    Srb = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK) + sizeof(PVOID), TAG_SCSIPORT);
    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    /* Allocate IRP */
    LargeInt.QuadPart = (LONGLONG) 1;
    Irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
                                        DeviceExtension->DeviceObject,
                                        InitialSrb->SenseInfoBuffer,
                                        InitialSrb->SenseInfoBufferLength,
                                        &LargeInt,
                                        NULL);

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)SpiCompletionRoutine,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    if (!Srb)
    {
        DPRINT("SpiSendRequestSense() failed, Srb %p\n", Srb);
        return;
    }

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_SCSI;

    /* Put Srb address into Irp... */
    IrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    /* ...and vice versa */
    Srb->OriginalRequest = Irp;

    /* Save Srb */
    Ptr = (PVOID *)(Srb+1);
    *Ptr = InitialSrb;

    /* Build CDB for REQUEST SENSE */
    Srb->CdbLength = 6;
    Cdb = (PCDB)Srb->Cdb;

    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)InitialSrb->SenseInfoBufferLength;
    Cdb->CDB6INQUIRY.Control = 0;

    /* Set address */
    Srb->TargetId = InitialSrb->TargetId;
    Srb->Lun = InitialSrb->Lun;
    Srb->PathId = InitialSrb->PathId;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    /* Timeout will be 2 seconds */
    Srb->TimeOutValue = 2;

    /* No auto request sense */
    Srb->SenseInfoBufferLength = 0;
    Srb->SenseInfoBuffer = NULL;

    /* Set necessary flags */
    Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_DISCONNECT;

    /* Transfer disable synch transfer flag */
    if (InitialSrb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)
        Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    Srb->DataBuffer = InitialSrb->SenseInfoBuffer;

    /* Fill the transfer length */
    Srb->DataTransferLength = InitialSrb->SenseInfoBufferLength;

    /* Clear statuses */
    Srb->ScsiStatus = Srb->SrbStatus = 0;
    Srb->NextSrb = 0;

    /* Call the driver */
    (VOID)IoCallDriver(DeviceExtension->DeviceObject, Irp);

    DPRINT("SpiSendRequestSense() done\n");
}


static
VOID
NTAPI
SpiProcessCompletedRequest(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                           IN PSCSI_REQUEST_BLOCK_INFO SrbInfo,
                           OUT PBOOLEAN NeedToCallStartIo)
{
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    LONG Result;
    PIRP Irp;
    //ULONG SequenceNumber;

    Srb = SrbInfo->Srb;
    Irp = Srb->OriginalRequest;

    /* Get Lun extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                     Srb->PathId,
                                     Srb->TargetId,
                                     Srb->Lun);

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION &&
        DeviceExtension->MapBuffers &&
        Irp->MdlAddress)
    {
        /* MDL is shared if transfer is broken into smaller parts */
        Srb->DataBuffer = (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress) +
            ((PCCHAR)Srb->DataBuffer - SrbInfo->DataOffset);

        /* In case of data going in, flush the buffers */
        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
        {
            KeFlushIoBuffers(Irp->MdlAddress,
                             TRUE,
                             FALSE);
        }
    }

    /* Flush adapter if needed */
    if (SrbInfo->BaseOfMapRegister)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Clear the request */
    SrbInfo->Srb = NULL;

    /* If disconnect is disabled... */
    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
    {
        /* Acquire the spinlock since we mess with flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Set corresponding flag */
        DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

        /* Clear the timer if needed */
        if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET))
            DeviceExtension->TimerCount = -1;

        /* Spinlock is not needed anymore */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        if (!(DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING) &&
            !(DeviceExtension->Flags & SCSI_PORT_DEVICE_BUSY) &&
            !(*NeedToCallStartIo))
        {
            /* We're not busy, but we have a request pending */
            IoStartNextPacket(DeviceExtension->DeviceObject, FALSE);
        }
    }

    /* Scatter/gather */
    if (Srb->SrbFlags & SRB_FLAGS_SGLIST_FROM_POOL)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Acquire spinlock (we're freeing SrbExtension) */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Free it (if needed) */
    if (Srb->SrbExtension)
    {
        if (Srb->SenseInfoBuffer != NULL && DeviceExtension->SupportsAutoSense)
        {
            ASSERT(Srb->SenseInfoBuffer == NULL || SrbInfo->SaveSenseRequest != NULL);

            if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
            {
                /* Copy sense data to the buffer */
                RtlCopyMemory(SrbInfo->SaveSenseRequest,
                              Srb->SenseInfoBuffer,
                              Srb->SenseInfoBufferLength);
            }

            /* And restore the pointer */
            Srb->SenseInfoBuffer = SrbInfo->SaveSenseRequest;
        }

        /* Put it into the free srb extensions list */
        *((PVOID *)Srb->SrbExtension) = DeviceExtension->FreeSrbExtensions;
        DeviceExtension->FreeSrbExtensions = Srb->SrbExtension;
    }

    /* Save transfer length in the IRP */
    Irp->IoStatus.Information = Srb->DataTransferLength;

    //SequenceNumber = SrbInfo->SequenceNumber;
    SrbInfo->SequenceNumber = 0;

    /* Decrement the queue count */
    LunExtension->QueueCount--;

    /* Free Srb, if needed*/
    if (Srb->QueueTag != SP_UNTAGGED)
    {
        /* Put it into the free list */
        SrbInfo->Requests.Blink = NULL;
        SrbInfo->Requests.Flink = (PLIST_ENTRY)DeviceExtension->FreeSrbInfo;
        DeviceExtension->FreeSrbInfo = SrbInfo;
    }

    /* SrbInfo is not used anymore */
    SrbInfo = NULL;

    if (DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING)
    {
        /* Clear the flag */
        DeviceExtension->Flags &= ~SCSI_PORT_REQUEST_PENDING;

        /* Note the caller about StartIo */
        *NeedToCallStartIo = TRUE;
    }

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
        /* Start the packet */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (!(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
            LunExtension->RequestTimeout == -1)
        {
            /* Start the next packet */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        }
        else
        {
            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }

        DPRINT("IoCompleting request IRP 0x%p\n", Irp);

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        /* Decrement number of active requests, and analyze the result */
        Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

        if (Result < 0 &&
            !DeviceExtension->MapRegisters &&
            DeviceExtension->AdapterObject != NULL)
        {
            /* Nullify map registers */
            DeviceExtension->MapRegisterBase = NULL;
            IoFreeAdapterChannel(DeviceExtension->AdapterObject);
        }

         /* Exit, we're done */
        return;
    }

    /* Decrement number of active requests, and analyze the result */
    Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

    if (Result < 0 &&
        !DeviceExtension->MapRegisters &&
        DeviceExtension->AdapterObject != NULL)
    {
        /* Result is negative, so this is a slave, free map registers */
        DeviceExtension->MapRegisterBase = NULL;
        IoFreeAdapterChannel(DeviceExtension->AdapterObject);
    }

    /* Convert status */
    Irp->IoStatus.Status = SpiStatusSrbToNt(Srb->SrbStatus);

    /* It's not a bypass, it's busy or the queue is full? */
    if ((Srb->ScsiStatus == SCSISTAT_BUSY ||
         Srb->SrbStatus == SRB_STATUS_BUSY ||
         Srb->ScsiStatus == SCSISTAT_QUEUE_FULL) &&
         !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE))
    {

        DPRINT("Busy SRB status %x\n", Srb->SrbStatus);

        /* Requeue, if needed */
        if (LunExtension->Flags & (LUNEX_FROZEN_QUEUE | LUNEX_BUSY))
        {
            DPRINT("it's being requeued\n");

            Srb->SrbStatus = SRB_STATUS_PENDING;
            Srb->ScsiStatus = 0;

            if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                                          &Irp->Tail.Overlay.DeviceQueueEntry,
                                          Srb->QueueSortKey))
            {
                /* It's a big f.ck up if we got here */
                Srb->SrbStatus = SRB_STATUS_ERROR;
                Srb->ScsiStatus = SCSISTAT_BUSY;

                ASSERT(FALSE);
                goto Error;
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        }
        else if (LunExtension->AttemptCount++ < 20)
        {
            /* LUN is still busy */
            Srb->ScsiStatus = 0;
            Srb->SrbStatus = SRB_STATUS_PENDING;

            LunExtension->BusyRequest = Irp;
            LunExtension->Flags |= LUNEX_BUSY;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }
        else
        {
Error:
            /* Freeze the queue*/
            Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
            LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

            /* "Unfull" the queue */
            LunExtension->Flags &= ~LUNEX_FULL_QUEUE;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Return status that the device is not ready */
            Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

        return;
    }

    /* Start the next request, if LUN is idle, and this is sense request */
    if (((Srb->ScsiStatus != SCSISTAT_CHECK_CONDITION) ||
        (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) ||
        !Srb->SenseInfoBuffer || !Srb->SenseInfoBufferLength)
        && (Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        if (LunExtension->RequestTimeout == -1)
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        else
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
    else
    {
        /* Freeze the queue */
        Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
        LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

        /* Do we need a request sense? */
        if (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
            !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength)
        {
            /* If LUN is busy, we have to requeue it in order to allow request sense */
            if (LunExtension->Flags & LUNEX_BUSY)
            {
                DPRINT("Requeuing busy request to allow request sense\n");

                if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                    &LunExtension->BusyRequest->Tail.Overlay.DeviceQueueEntry,
                    Srb->QueueSortKey))
                {
                    /* We should never get here */
                    ASSERT(FALSE);

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                    return;

                }

                /* Clear busy flags */
                LunExtension->Flags &= ~(LUNEX_FULL_QUEUE | LUNEX_BUSY);
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Send RequestSense */
            SpiSendRequestSense(DeviceExtension, Srb);

            /* Exit */
            return;
        }

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* Complete the request */
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
}

NTSTATUS
NTAPI
SpiCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp,
                     PVOID Context)
{
    PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)Context;
    PSCSI_REQUEST_BLOCK InitialSrb;
    PIRP InitialIrp;

    DPRINT("SpiCompletionRoutine() entered, IRP %p \n", Irp);

    if ((Srb->Function == SRB_FUNCTION_RESET_BUS) ||
        (Srb->Function == SRB_FUNCTION_ABORT_COMMAND))
    {
        /* Deallocate SRB and IRP and exit */
        ExFreePool(Srb);
        IoFreeIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get a pointer to the SRB and IRP which were initially sent */
    InitialSrb = *((PVOID *)(Srb+1));
    InitialIrp = InitialSrb->OriginalRequest;

    if ((SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN))
    {
        /* Sense data is OK */
        InitialSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        /* Set length to be the same */
        InitialSrb->SenseInfoBufferLength = (UCHAR)Srb->DataTransferLength;
    }

    /* Make sure initial SRB's queue is frozen */
    ASSERT(InitialSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN);

    /* Complete this request */
    IoCompleteRequest(InitialIrp, IO_DISK_INCREMENT);

    /* Deallocate everything (internal) */
    ExFreePool(Srb);

    if (Irp->MdlAddress != NULL)
    {
		MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static BOOLEAN NTAPI
ScsiPortIsr(IN PKINTERRUPT Interrupt,
            IN PVOID ServiceContext)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

    DPRINT("ScsiPortIsr() called!\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)ServiceContext;

    /* If interrupts are disabled - we don't expect any */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_DISABLE_INTERRUPTS)
        return FALSE;

    /* Call miniport's HwInterrupt routine */
    if (DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension) == FALSE)
    {
        /* This interrupt doesn't belong to us */
        return FALSE;
    }

    /* If flag of notification is set - queue a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
    {
        IoRequestDpc(DeviceExtension->DeviceObject,
                     DeviceExtension->CurrentIrp,
                     DeviceExtension);
    }

    return TRUE;
}

BOOLEAN
NTAPI
SpiSaveInterruptData(IN PVOID Context)
{
    PSCSI_PORT_SAVE_INTERRUPT InterruptContext = Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo, NextSrbInfo;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN IsTimed;

    /* Get pointer to the device extension */
    DeviceExtension = InterruptContext->DeviceExtension;

    /* If we don't have anything pending - return */
    if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED))
        return FALSE;

    /* Actually save the interrupt data */
    *InterruptContext->InterruptData = DeviceExtension->InterruptData;

    /* Clear the data stored in the device extension */
    DeviceExtension->InterruptData.Flags &=
        (SCSI_PORT_RESET | SCSI_PORT_RESET_REQUEST | SCSI_PORT_DISABLE_INTERRUPTS);
    DeviceExtension->InterruptData.CompletedAbort = NULL;
    DeviceExtension->InterruptData.ReadyLun = NULL;
    DeviceExtension->InterruptData.CompletedRequests = NULL;

    /* Loop through the list of completed requests */
    SrbInfo = InterruptContext->InterruptData->CompletedRequests;

    while (SrbInfo)
    {
        /* Make sure we have SRV */
        ASSERT(SrbInfo->Srb);

        /* Get SRB and LunExtension */
        Srb = SrbInfo->Srb;

        LunExtension = SpiGetLunExtension(DeviceExtension,
                                          Srb->PathId,
                                          Srb->TargetId,
                                          Srb->Lun);

        /* We have to check special cases if request is unsuccessful*/
        if (Srb->SrbStatus != SRB_STATUS_SUCCESS)
        {
            /* Check if we need request sense by a few conditions */
            if (Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength &&
                Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
                !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID))
            {
                if (LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE)
                {
                    /* It means: we tried to send REQUEST SENSE, but failed */

                    Srb->ScsiStatus = 0;
                    Srb->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
                }
                else
                {
                    /* Set the corresponding flag, so that REQUEST SENSE
                       will be sent */
                    LunExtension->Flags |= LUNEX_NEED_REQUEST_SENSE;
                }

            }

            /* Check for a full queue */
            if (Srb->ScsiStatus == SCSISTAT_QUEUE_FULL)
            {
                /* TODO: Implement when it's encountered */
                ASSERT(FALSE);
            }
        }

        /* Let's decide if we need to watch timeout or not */
        if (Srb->QueueTag == SP_UNTAGGED)
        {
            IsTimed = TRUE;
        }
        else
        {
            if (LunExtension->SrbInfo.Requests.Flink == &SrbInfo->Requests)
                IsTimed = TRUE;
            else
                IsTimed = FALSE;

            /* Remove it from the queue */
            RemoveEntryList(&SrbInfo->Requests);
        }

        if (IsTimed)
        {
            /* We have to maintain timeout counter */
            if (IsListEmpty(&LunExtension->SrbInfo.Requests))
            {
                LunExtension->RequestTimeout = -1;
            }
            else
            {
                NextSrbInfo = CONTAINING_RECORD(LunExtension->SrbInfo.Requests.Flink,
                                                SCSI_REQUEST_BLOCK_INFO,
                                                Requests);

                Srb = NextSrbInfo->Srb;

                /* Update timeout counter */
                LunExtension->RequestTimeout = Srb->TimeOutValue;
            }
        }

        SrbInfo = SrbInfo->CompletedRequests;
    }

    return TRUE;
}

VOID
NTAPI
SpiGetNextRequestFromLun(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         IN PSCSI_PORT_LUN_EXTENSION LunExtension)
{
    PIO_STACK_LOCATION IrpStack;
    PIRP NextIrp;
    PKDEVICE_QUEUE_ENTRY Entry;
    PSCSI_REQUEST_BLOCK Srb;


    /* If LUN is not active or queue is more than maximum allowed  */
    if (LunExtension->QueueCount >= LunExtension->MaxQueueCount ||
        !(LunExtension->Flags & SCSI_PORT_LU_ACTIVE))
    {
        /* Release the spinlock and exit */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        return;
    }

    /* Check if we can get a next request */
    if (LunExtension->Flags &
        (LUNEX_NEED_REQUEST_SENSE | LUNEX_BUSY |
         LUNEX_FULL_QUEUE | LUNEX_FROZEN_QUEUE | LUNEX_REQUEST_PENDING))
    {
        /* Pending requests can only be started if the queue is empty */
        if (IsListEmpty(&LunExtension->SrbInfo.Requests) &&
            !(LunExtension->Flags &
              (LUNEX_BUSY | LUNEX_FROZEN_QUEUE | LUNEX_FULL_QUEUE | LUNEX_NEED_REQUEST_SENSE)))
        {
            /* Make sure we have SRB */
            ASSERT(LunExtension->SrbInfo.Srb == NULL);

            /* Clear active and pending flags */
            LunExtension->Flags &= ~(LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE);

            /* Get next Irp, and clear pending requests list */
            NextIrp = LunExtension->PendingRequest;
            LunExtension->PendingRequest = NULL;

            /* Set attempt counter to zero */
            LunExtension->AttemptCount = 0;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Start the next pending request */
            IoStartPacket(DeviceExtension->DeviceObject, NextIrp, (PULONG)NULL, NULL);

            return;
        }
        else
        {
            /* Release the spinlock, without clearing any flags and exit */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return;
        }
    }

    /* Reset active flag */
    LunExtension->Flags &= ~SCSI_PORT_LU_ACTIVE;

    /* Set attempt counter to zero */
    LunExtension->AttemptCount = 0;

    /* Remove packet from the device queue */
    Entry = KeRemoveByKeyDeviceQueue(&LunExtension->DeviceQueue, LunExtension->SortKey);

    if (Entry != NULL)
    {
        /* Get pointer to the next irp */
        NextIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);

        /* Get point to the SRB */
        IrpStack = IoGetCurrentIrpStackLocation(NextIrp);
        Srb = (PSCSI_REQUEST_BLOCK)IrpStack->Parameters.Others.Argument1;

        /* Set new key*/
        LunExtension->SortKey = Srb->QueueSortKey;
        LunExtension->SortKey++;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        /* Start the next pending request */
        IoStartPacket(DeviceExtension->DeviceObject, NextIrp, (PULONG)NULL, NULL);
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
}



//    ScsiPortDpcForIsr
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID NTAPI
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = DpcDeviceObject->DeviceExtension;
    SCSI_PORT_INTERRUPT_DATA InterruptData;
    SCSI_PORT_SAVE_INTERRUPT Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    BOOLEAN NeedToStartIo;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    LARGE_INTEGER TimerValue;

    DPRINT("ScsiPortDpcForIsr(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
           Dpc, DpcDeviceObject, DpcIrp, DpcContext);

    /* We need to acquire spinlock */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

	RtlZeroMemory(&InterruptData, sizeof(SCSI_PORT_INTERRUPT_DATA));

TryAgain:

    /* Interrupt structure must be snapshotted, and only then analyzed */
    Context.InterruptData = &InterruptData;
    Context.DeviceExtension = DeviceExtension;

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                SpiSaveInterruptData,
                                &Context))
    {
        /* Nothing - just return (don't forget to release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        DPRINT("ScsiPortDpcForIsr() done\n");
        return;
    }

    /* If flush of adapters is needed - do it */
    if (InterruptData.Flags & SCSI_PORT_FLUSH_ADAPTERS)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check for IoMapTransfer */
    if (InterruptData.Flags & SCSI_PORT_MAP_TRANSFER)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check if timer is needed */
    if (InterruptData.Flags & SCSI_PORT_TIMER_NEEDED)
    {
        /* Save the timer routine */
        DeviceExtension->HwScsiTimer = InterruptData.HwScsiTimer;

        if (InterruptData.MiniportTimerValue == 0)
        {
            /* Cancel the timer */
            KeCancelTimer(&DeviceExtension->MiniportTimer);
        }
        else
        {
            /* Convert timer value */
            TimerValue.QuadPart = Int32x32To64(InterruptData.MiniportTimerValue, -10);

            /* Set the timer */
            KeSetTimer(&DeviceExtension->MiniportTimer,
                       TimerValue,
                       &DeviceExtension->MiniportTimerDpc);
        }
    }

    /* If it's ready for the next request */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
    {
        /* Check for a duplicate request (NextRequest+NextLuRequest) */
        if ((DeviceExtension->Flags &
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED)) ==
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED))
        {
            /* Clear busy flag set by ScsiPortStartPacket() */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;

            if (!(InterruptData.Flags & SCSI_PORT_RESET))
            {
                /* Ready for next, and no reset is happening */
                DeviceExtension->TimerCount = -1;
            }
        }
        else
        {
            /* Not busy, but not ready for the next request */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;
            InterruptData.Flags &= ~SCSI_PORT_NEXT_REQUEST_READY;
        }
    }

    /* Any resets? */
    if (InterruptData.Flags & SCSI_PORT_RESET_REPORTED)
    {
        /* Hold for a bit */
        DeviceExtension->TimerCount = 4;
    }

    /* Any ready LUN? */
    if (InterruptData.ReadyLun != NULL)
    {

        /* Process all LUNs from the list*/
        while (TRUE)
        {
            /* Remove it from the list first (as processed) */
            LunExtension = InterruptData.ReadyLun;
            InterruptData.ReadyLun = LunExtension->ReadyLun;
            LunExtension->ReadyLun = NULL;

            /* Get next request for this LUN */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);

            /* Still ready requests exist?
               If yes - get spinlock, if no - stop here */
            if (InterruptData.ReadyLun != NULL)
                KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            else
                break;
        }
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* If we ready for next packet, start it */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
        IoStartNextPacket(DeviceExtension->DeviceObject, FALSE);

    NeedToStartIo = FALSE;

    /* Loop the completed request list */
    while (InterruptData.CompletedRequests)
    {
        /* Remove the request */
        SrbInfo = InterruptData.CompletedRequests;
        InterruptData.CompletedRequests = SrbInfo->CompletedRequests;
        SrbInfo->CompletedRequests = NULL;

        /* Process it */
        SpiProcessCompletedRequest(DeviceExtension,
                                  SrbInfo,
                                  &NeedToStartIo);
    }

    /* Loop abort request list */
    while (InterruptData.CompletedAbort)
    {
        LunExtension = InterruptData.CompletedAbort;

        /* Remove the request */
        InterruptData.CompletedAbort = LunExtension->CompletedAbortRequests;

        /* Get spinlock since we're going to change flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* TODO: Put SrbExtension to the list of free extensions */
        ASSERT(FALSE);
    }

    /* If we need - call StartIo routine */
    if (NeedToStartIo)
    {
        /* Make sure CurrentIrp is not null! */
        ASSERT(DpcDeviceObject->CurrentIrp != NULL);
        ScsiPortStartIo(DpcDeviceObject, DpcDeviceObject->CurrentIrp);
    }

    /* Everything has been done, check */
    if (InterruptData.Flags & SCSI_PORT_ENABLE_INT_REQUEST)
    {
        /* Synchronize using spinlock */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Request an interrupt */
        DeviceExtension->HwInterrupt(DeviceExtension->MiniPortDeviceExtension);

        ASSERT(DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET);

        /* Should interrupts be enabled again? */
        if (DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET)
        {
            /* Clear this flag */
            DeviceExtension->Flags &= ~SCSI_PORT_DISABLE_INT_REQUESET;

            /* Call a special routine to do this */
            ASSERT(FALSE);
#if 0
            KeSynchronizeExecution(DeviceExtension->Interrupt,
                                   SpiEnableInterrupts,
                                   DeviceExtension);
#endif
        }

        /* If we need a notification again - loop */
        if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
            goto TryAgain;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    DPRINT("ScsiPortDpcForIsr() done\n");
}

BOOLEAN
NTAPI
SpiProcessTimeout(PVOID ServiceContext)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    ULONG Bus;

    DPRINT("SpiProcessTimeout() entered\n");

    DeviceExtension->TimerCount = -1;

    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET)
    {
        DeviceExtension->InterruptData.Flags &= ~SCSI_PORT_RESET;

        if (DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET_REQUEST)
        {
            DeviceExtension->InterruptData.Flags &=  ~SCSI_PORT_RESET_REQUEST;
            ScsiPortStartPacket(ServiceContext);
        }

        return FALSE;
    }
    else
    {
        DPRINT("Resetting the bus\n");

        for (Bus = 0; Bus < DeviceExtension->BusNum; Bus++)
        {
            DeviceExtension->HwResetBus(DeviceExtension->MiniPortDeviceExtension, Bus);

            /* Reset flags and set reset timeout to 4 seconds */
            DeviceExtension->InterruptData.Flags |= SCSI_PORT_RESET;
            DeviceExtension->TimerCount = 4;
        }

        /* If miniport requested - request a dpc for it */
        if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
            IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);
    }

    return TRUE;
}


BOOLEAN
NTAPI
SpiResetBus(PVOID ServiceContext)
{
    PRESETBUS_PARAMS ResetParams = (PRESETBUS_PARAMS)ServiceContext;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

    /* Perform the bus reset */
    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)ResetParams->DeviceExtension;
    DeviceExtension->HwResetBus(DeviceExtension->MiniPortDeviceExtension,
                                ResetParams->PathId);

    /* Set flags and start the timer */
    DeviceExtension->InterruptData.Flags |= SCSI_PORT_RESET;
    DeviceExtension->TimerCount = 4;

    /* If miniport requested - give him a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
        IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);

    return TRUE;
}

//    ScsiPortIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID NTAPI
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
                PVOID Context)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG Lun;
    PIRP Irp;

    DPRINT("ScsiPortIoTimer()\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Protect with the spinlock */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Check timeouts */
    if (DeviceExtension->TimerCount > 0)
    {
        /* Decrease the timeout counter */
        DeviceExtension->TimerCount--;

        if (DeviceExtension->TimerCount == 0)
        {
            /* Timeout, process it */
            if (KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                       SpiProcessTimeout,
                                       DeviceExtension->DeviceObject))
            {
                DPRINT("Error happened during processing timeout, but nothing critical\n");
            }
        }

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        /* We should exit now, since timeout is processed */
        return;
    }

    /* Per-Lun scanning of timeouts is needed... */
    for (Lun = 0; Lun < LUS_NUMBER; Lun++)
    {
        LunExtension = DeviceExtension->LunExtensionList[Lun];

        while (LunExtension)
        {
            if (LunExtension->Flags & LUNEX_BUSY)
            {
                if (!(LunExtension->Flags &
                    (LUNEX_NEED_REQUEST_SENSE | LUNEX_FROZEN_QUEUE)))
                {
                    DPRINT("Retrying busy request\n");

                    /* Clear flags, and retry busy request */
                    LunExtension->Flags &= ~(LUNEX_BUSY | LUNEX_FULL_QUEUE);
                    Irp = LunExtension->BusyRequest;

                    /* Clearing busy request */
                    LunExtension->BusyRequest = NULL;

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                    IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);

                    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
                }
            }
            else if (LunExtension->RequestTimeout == 0)
            {
                RESETBUS_PARAMS ResetParams;

                LunExtension->RequestTimeout = -1;

                DPRINT("Request timed out, resetting bus\n");

                /* Pass params to the bus reset routine */
                ResetParams.PathId = LunExtension->PathId;
                ResetParams.DeviceExtension = DeviceExtension;

                if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                            SpiResetBus,
                                            &ResetParams))
                {
                    DPRINT1("Reset failed\n");
                }
            }
            else if (LunExtension->RequestTimeout > 0)
            {
                /* Decrement the timeout counter */
                LunExtension->RequestTimeout--;
            }

            LunExtension = LunExtension->Next;
        }
    }

    /* Release the spinlock */
    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
}

/**********************************************************************
 * NAME                         INTERNAL
 *  SpiBuildDeviceMap
 *
 * DESCRIPTION
 *  Builds the registry device map of all device which are attached
 *  to the given SCSI HBA port. The device map is located at:
 *    \Registry\Machine\DeviceMap\Scsi
 *
 * RUN LEVEL
 *  PASSIVE_LEVEL
 *
 * ARGUMENTS
 *  DeviceExtension
 *      ...
 *
 *  RegistryPath
 *      Name of registry driver service key.
 *
 * RETURNS
 *  NTSTATUS
 */

static NTSTATUS
SpiBuildDeviceMap(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                  IN PUNICODE_STRING RegistryPath)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    WCHAR NameBuffer[64];
    ULONG Disposition;
    HANDLE ScsiKey;
    HANDLE ScsiPortKey = NULL;
    HANDLE ScsiBusKey = NULL;
    HANDLE ScsiInitiatorKey = NULL;
    HANDLE ScsiTargetKey = NULL;
    HANDLE ScsiLunKey = NULL;
    ULONG BusNumber;
    ULONG Target;
    ULONG CurrentTarget;
    ULONG Lun;
    PWCHAR DriverName;
    ULONG UlongData;
    PWCHAR TypeName;
    NTSTATUS Status;

    DPRINT("SpiBuildDeviceMap() called\n");

    if (DeviceExtension == NULL || RegistryPath == NULL)
    {
        DPRINT1("Invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Open or create the 'Scsi' subkey */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               0,
                               NULL);
    Status = ZwCreateKey(&ScsiKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'Scsi Port X' subkey */
    DPRINT("Scsi Port %lu\n", DeviceExtension->PortNumber);

    swprintf(NameBuffer,
             L"Scsi Port %lu",
             DeviceExtension->PortNumber);
    RtlInitUnicodeString(&KeyName, NameBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE,
                               ScsiKey,
                               NULL);
    Status = ZwCreateKey(&ScsiPortKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    ZwClose(ScsiKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }

    /*
     * Create port-specific values
     */

    /* Set 'DMA Enabled' (REG_DWORD) value */
    UlongData = (ULONG)!DeviceExtension->PortCapabilities.AdapterUsesPio;
    DPRINT("  DMA Enabled = %s\n", UlongData ? "TRUE" : "FALSE");
    RtlInitUnicodeString(&ValueName, L"DMA Enabled");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('DMA Enabled') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'Driver' (REG_SZ) value */
    DriverName = wcsrchr(RegistryPath->Buffer, L'\\') + 1;
    RtlInitUnicodeString(&ValueName, L"Driver");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_SZ,
                           DriverName,
                           (ULONG)((wcslen(DriverName) + 1) * sizeof(WCHAR)));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('Driver') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'Interrupt' (REG_DWORD) value (NT4 only) */
    UlongData = (ULONG)DeviceExtension->PortConfig->BusInterruptLevel;
    DPRINT("  Interrupt = %lu\n", UlongData);
    RtlInitUnicodeString(&ValueName, L"Interrupt");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('Interrupt') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'IOAddress' (REG_DWORD) value (NT4 only) */
    UlongData = ScsiPortConvertPhysicalAddressToUlong((*DeviceExtension->PortConfig->AccessRanges)[0].RangeStart);
    DPRINT("  IOAddress = %lx\n", UlongData);
    RtlInitUnicodeString(&ValueName, L"IOAddress");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('IOAddress') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Enumerate buses */
    for (BusNumber = 0; BusNumber < DeviceExtension->PortConfig->NumberOfBuses; BusNumber++)
    {
        /* Create 'Scsi Bus X' key */
        DPRINT("    Scsi Bus %lu\n", BusNumber);
        swprintf(NameBuffer,
                 L"Scsi Bus %lu",
                 BusNumber);
        RtlInitUnicodeString(&KeyName, NameBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   0,
                                   ScsiPortKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiBusKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
            goto ByeBye;
        }

        /* Create 'Initiator Id X' key */
        DPRINT("      Initiator Id %lu\n",
               DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
        swprintf(NameBuffer,
                 L"Initiator Id %lu",
                 (ULONG)(UCHAR)DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
        RtlInitUnicodeString(&KeyName, NameBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   0,
                                   ScsiBusKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiInitiatorKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
            goto ByeBye;
        }

        /* FIXME: Are there any initiator values (??) */

        ZwClose(ScsiInitiatorKey);
        ScsiInitiatorKey = NULL;


        /* Enumerate targets */
        CurrentTarget = (ULONG)-1;
        ScsiTargetKey = NULL;
        for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
        {
            for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
            {
                LunExtension = SpiGetLunExtension(DeviceExtension,
                                                  (UCHAR)BusNumber,
                                                  (UCHAR)Target,
                                                  (UCHAR)Lun);
                if (LunExtension == NULL)
                    continue;

                if (Target != CurrentTarget)
                {
                    /* Close old target key */
                    if (ScsiTargetKey != NULL)
                    {
                        ZwClose(ScsiTargetKey);
                        ScsiTargetKey = NULL;
                    }

                    /* Create 'Target Id X' key */
                    DPRINT("      Target Id %lu\n", Target);
                    swprintf(NameBuffer,
                             L"Target Id %lu",
                             Target);
                    RtlInitUnicodeString(&KeyName, NameBuffer);
                    InitializeObjectAttributes(&ObjectAttributes,
                                               &KeyName,
                                               0,
                                               ScsiBusKey,
                                               NULL);
                    Status = ZwCreateKey(&ScsiTargetKey,
                                         KEY_ALL_ACCESS,
                                         &ObjectAttributes,
                                         0,
                                         NULL,
                                         REG_OPTION_VOLATILE,
                                         &Disposition);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
                        goto ByeBye;
                    }

                    CurrentTarget = Target;
                }

                /* Create 'Logical Unit Id X' key */
                DPRINT("        Logical Unit Id %lu\n", Lun);
                swprintf(NameBuffer,
                         L"Logical Unit Id %lu",
                         Lun);
                RtlInitUnicodeString(&KeyName, NameBuffer);
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyName,
                                           0,
                                           ScsiTargetKey,
                                           NULL);
                Status = ZwCreateKey(&ScsiLunKey,
                                     KEY_ALL_ACCESS,
                                     &ObjectAttributes,
                                     0,
                                     NULL,
                                     REG_OPTION_VOLATILE,
                                     &Disposition);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                /* Set 'Identifier' (REG_SZ) value */
                swprintf(NameBuffer,
                         L"%.8S%.16S%.4S",
                         LunExtension->InquiryData.VendorId,
                         LunExtension->InquiryData.ProductId,
                         LunExtension->InquiryData.ProductRevisionLevel);
                DPRINT("          Identifier = '%S'\n", NameBuffer);
                RtlInitUnicodeString(&ValueName, L"Identifier");
                Status = ZwSetValueKey(ScsiLunKey,
                                       &ValueName,
                                       0,
                                       REG_SZ,
                                       NameBuffer,
                                       (ULONG)((wcslen(NameBuffer) + 1) * sizeof(WCHAR)));
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwSetValueKey('Identifier') failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                /* Set 'Type' (REG_SZ) value */
                /*
                 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices
                 * and https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-scsi-devices
                 * for a list of types with their human-readable forms.
                 */
                switch (LunExtension->InquiryData.DeviceType)
                {
                    case 0:
                        TypeName = L"DiskPeripheral";
                        break;
                    case 1:
                        TypeName = L"TapePeripheral";
                        break;
                    case 2:
                        TypeName = L"PrinterPeripheral";
                        break;
                    // case 3: "ProcessorPeripheral", classified as 'other': fall back to default case.
                    case 4:
                        TypeName = L"WormPeripheral";
                        break;
                    case 5:
                        TypeName = L"CdRomPeripheral";
                        break;
                    case 6:
                        TypeName = L"ScannerPeripheral";
                        break;
                    case 7:
                        TypeName = L"OpticalDiskPeripheral";
                        break;
                    case 8:
                        TypeName = L"MediumChangerPeripheral";
                        break;
                    case 9:
                        TypeName = L"CommunicationsPeripheral";
                        break;

                    /* New peripheral types (SCSI only) */
                    case 10: case 11:
                        TypeName = L"ASCPrePressGraphicsPeripheral";
                        break;
                    case 12:
                        TypeName = L"ArrayPeripheral";
                        break;
                    case 13:
                        TypeName = L"EnclosurePeripheral";
                        break;
                    case 14:
                        TypeName = L"RBCPeripheral";
                        break;
                    case 15:
                        TypeName = L"CardReaderPeripheral";
                        break;
                    case 16:
                        TypeName = L"BridgePeripheral";
                        break;

                    default:
                        TypeName = L"OtherPeripheral";
                        break;
                }
                DPRINT("          Type = '%S'\n", TypeName);
                RtlInitUnicodeString(&ValueName, L"Type");
                Status = ZwSetValueKey(ScsiLunKey,
                                       &ValueName,
                                       0,
                                       REG_SZ,
                                       TypeName,
                                       (ULONG)((wcslen(TypeName) + 1) * sizeof(WCHAR)));
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwSetValueKey('Type') failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                ZwClose(ScsiLunKey);
                ScsiLunKey = NULL;
            }

            /* Close old target key */
            if (ScsiTargetKey != NULL)
            {
                ZwClose(ScsiTargetKey);
                ScsiTargetKey = NULL;
            }
        }

        ZwClose(ScsiBusKey);
        ScsiBusKey = NULL;
    }

ByeBye:
    if (ScsiLunKey != NULL)
        ZwClose(ScsiLunKey);

    if (ScsiInitiatorKey != NULL)
        ZwClose(ScsiInitiatorKey);

    if (ScsiTargetKey != NULL)
        ZwClose(ScsiTargetKey);

    if (ScsiBusKey != NULL)
        ZwClose(ScsiBusKey);

    if (ScsiPortKey != NULL)
        ZwClose(ScsiPortKey);

    DPRINT("SpiBuildDeviceMap() done\n");

    return Status;
}

VOID
NTAPI
SpiMiniportTimerDpc(IN struct _KDPC *Dpc,
                    IN PVOID DeviceObject,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

    DPRINT("Miniport timer DPC\n");

    DeviceExtension = ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;

    /* Acquire the spinlock */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Call the timer routine */
    if (DeviceExtension->HwScsiTimer != NULL)
    {
        DeviceExtension->HwScsiTimer(&DeviceExtension->MiniPortDeviceExtension);
    }

    /* Release the spinlock */
    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
    {
        ScsiPortDpcForIsr(NULL,
                          DeviceExtension->DeviceObject,
                          NULL,
                          NULL);
    }
}

static NTSTATUS
SpiCreatePortConfig(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    PHW_INITIALIZATION_DATA HwInitData,
                    PCONFIGURATION_INFO InternalConfigInfo,
                    PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                    BOOLEAN ZeroStruct)
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCONFIGURATION_INFORMATION DdkConfigInformation;
    HANDLE RootKey, Key;
    BOOLEAN Found;
    WCHAR DeviceBuffer[16];
    WCHAR StrBuffer[512];
    ULONG Bus;
    NTSTATUS Status;

    /* Zero out the struct if told so */
    if (ZeroStruct)
    {
        /* First zero the portconfig */
        RtlZeroMemory(ConfigInfo, sizeof(PORT_CONFIGURATION_INFORMATION));

        /* Then access ranges */
        RtlZeroMemory(InternalConfigInfo->AccessRanges,
                      HwInitData->NumberOfAccessRanges * sizeof(ACCESS_RANGE));

        /* Initialize the struct */
        ConfigInfo->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
        ConfigInfo->AdapterInterfaceType = HwInitData->AdapterInterfaceType;
        ConfigInfo->InterruptMode = Latched;
        ConfigInfo->DmaChannel = SP_UNINITIALIZED_VALUE;
        ConfigInfo->DmaPort = SP_UNINITIALIZED_VALUE;
        ConfigInfo->DmaChannel2 = SP_UNINITIALIZED_VALUE;
        ConfigInfo->DmaPort2 = SP_UNINITIALIZED_VALUE;
        ConfigInfo->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
        ConfigInfo->NumberOfAccessRanges = HwInitData->NumberOfAccessRanges;
        ConfigInfo->MaximumNumberOfTargets = 8;

        /* Store parameters */
        ConfigInfo->NeedPhysicalAddresses = HwInitData->NeedPhysicalAddresses;
        ConfigInfo->MapBuffers = HwInitData->MapBuffers;
        ConfigInfo->AutoRequestSense = HwInitData->AutoRequestSense;
        ConfigInfo->ReceiveEvent = HwInitData->ReceiveEvent;
        ConfigInfo->TaggedQueuing = HwInitData->TaggedQueuing;
        ConfigInfo->MultipleRequestPerLu = HwInitData->MultipleRequestPerLu;

        /* Get the disk usage */
        DdkConfigInformation = IoGetConfigurationInformation();
        ConfigInfo->AtdiskPrimaryClaimed = DdkConfigInformation->AtDiskPrimaryAddressClaimed;
        ConfigInfo->AtdiskSecondaryClaimed = DdkConfigInformation->AtDiskSecondaryAddressClaimed;

        /* Initiator bus id is not set */
        for (Bus = 0; Bus < 8; Bus++)
            ConfigInfo->InitiatorBusId[Bus] = (CCHAR)SP_UNINITIALIZED_VALUE;
    }

    ConfigInfo->NumberOfPhysicalBreaks = 17;

    /* Clear this information */
    InternalConfigInfo->DisableTaggedQueueing = FALSE;
    InternalConfigInfo->DisableMultipleLun = FALSE;

    /* Store Bus Number */
    ConfigInfo->SystemIoBusNumber = InternalConfigInfo->BusNumber;

TryNextAd:

    if (ConfigInfo->AdapterInterfaceType == Internal)
    {
        /* Open registry key for HW database */
        InitializeObjectAttributes(&ObjectAttributes,
                                   DeviceExtension->DeviceObject->DriverObject->HardwareDatabase,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwOpenKey(&RootKey,
                           KEY_READ,
                           &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            /* Create name for it */
            swprintf(StrBuffer, L"ScsiAdapter\\%lu",
                InternalConfigInfo->AdapterNumber);

            RtlInitUnicodeString(&UnicodeString, StrBuffer);

            /* Open device key */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       RootKey,
                                       NULL);

            Status = ZwOpenKey(&Key,
                               KEY_READ,
                               &ObjectAttributes);

            ZwClose(RootKey);

            if (NT_SUCCESS(Status))
            {
                if (InternalConfigInfo->LastAdapterNumber != InternalConfigInfo->AdapterNumber)
                {
                    DPRINT("Hardware info found at %S\n", StrBuffer);

                    /* Parse it */
                    SpiParseDeviceInfo(DeviceExtension,
                                       Key,
                                       ConfigInfo,
                                       InternalConfigInfo,
                                       (PUCHAR)StrBuffer);

                     InternalConfigInfo->BusNumber = 0;
                }
                else
                {
                    /* Try the next adapter */
                    InternalConfigInfo->AdapterNumber++;
                    goto TryNextAd;
                }
            }
            else
            {
                /* Info was not found, exit */
                DPRINT1("ZwOpenKey() failed with Status=0x%08X\n", Status);
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }
        }
        else
        {
            DPRINT1("ZwOpenKey() failed with Status=0x%08X\n", Status);
        }
    }

    /* Look at device params */
    Key = NULL;
    if (InternalConfigInfo->Parameter)
    {
        ExFreePool(InternalConfigInfo->Parameter);
        InternalConfigInfo->Parameter = NULL;
    }

    if (InternalConfigInfo->ServiceKey != NULL)
    {
        swprintf(DeviceBuffer, L"Device%lu", InternalConfigInfo->AdapterNumber);
        RtlInitUnicodeString(&UnicodeString, DeviceBuffer);

        /* Open the service key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   InternalConfigInfo->ServiceKey,
                                   NULL);

        Status = ZwOpenKey(&Key,
                           KEY_READ,
                           &ObjectAttributes);
    }

    /* Parse device key */
    if (InternalConfigInfo->DeviceKey != NULL)
    {
        SpiParseDeviceInfo(DeviceExtension,
                           InternalConfigInfo->DeviceKey,
                           ConfigInfo,
                           InternalConfigInfo,
                           (PUCHAR)StrBuffer);
    }

    /* Then parse hw info */
    if (Key != NULL)
    {
        if (InternalConfigInfo->LastAdapterNumber != InternalConfigInfo->AdapterNumber)
        {
            SpiParseDeviceInfo(DeviceExtension,
                               Key,
                               ConfigInfo,
                               InternalConfigInfo,
                               (PUCHAR)StrBuffer);

            /* Close the key */
            ZwClose(Key);
        }
        else
        {
            /* Adapter not found, go try the next one */
            InternalConfigInfo->AdapterNumber++;

            /* Close the key */
            ZwClose(Key);

            goto TryNextAd;
        }
    }

    /* Update the last adapter number */
    InternalConfigInfo->LastAdapterNumber = InternalConfigInfo->AdapterNumber;

    /* Do we have this kind of bus at all? */
    Found = FALSE;
    Status = IoQueryDeviceDescription(&HwInitData->AdapterInterfaceType,
                                      &InternalConfigInfo->BusNumber,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      SpQueryDeviceCallout,
                                      &Found);

    /* This bus was not found */
    if (!Found)
    {
        INTERFACE_TYPE InterfaceType = Eisa;

        /* Check for EISA */
        if (HwInitData->AdapterInterfaceType == Isa)
        {
            Status = IoQueryDeviceDescription(&InterfaceType,
                                              &InternalConfigInfo->BusNumber,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              SpQueryDeviceCallout,
                                              &Found);

            /* Return respectively */
            if (Found)
                return STATUS_SUCCESS;
            else
                return STATUS_DEVICE_DOES_NOT_EXIST;
        }
        else
        {
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

static VOID
SpiParseDeviceInfo(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                   IN HANDLE Key,
                   IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                   IN PCONFIGURATION_INFO InternalConfigInfo,
                   IN PUCHAR Buffer)
{
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_SCSI_DEVICE_DATA ScsiDeviceData;
    ULONG Length, Count, Dma = 0, Interrupt = 0;
    ULONG Index = 0, RangeCount = 0;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    NTSTATUS Status = STATUS_SUCCESS;


    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    /* Loop through all values in the device node */
    while(TRUE)
    {
        Status = ZwEnumerateValueKey(Key,
                                     Index,
                                     KeyValueFullInformation,
                                     Buffer,
                                     512,
                                     &Length);

        if (!NT_SUCCESS(Status))
            return;

        Index++;

        /* Length for DWORD is ok? */
        if (KeyValueInformation->Type == REG_DWORD &&
            KeyValueInformation->DataLength != sizeof(ULONG))
        {
            continue;
        }

        /* Get MaximumLogicalUnit */
        if (_wcsnicmp(KeyValueInformation->Name, L"MaximumLogicalUnit",
            KeyValueInformation->NameLength/2) == 0)
        {

            if (KeyValueInformation->Type != REG_DWORD)
            {
                DPRINT("Bad data type for MaximumLogicalUnit\n");
                continue;
            }

            DeviceExtension->MaxLunCount = *((PUCHAR)
                (Buffer + KeyValueInformation->DataOffset));

            /* Check / reset if needed */
            if (DeviceExtension->MaxLunCount > SCSI_MAXIMUM_LOGICAL_UNITS)
                DeviceExtension->MaxLunCount = SCSI_MAXIMUM_LOGICAL_UNITS;

            DPRINT("MaximumLogicalUnit = %d\n", DeviceExtension->MaxLunCount);
        }

        /* Get InitiatorTargetId */
        if (_wcsnicmp(KeyValueInformation->Name, L"InitiatorTargetId",
            KeyValueInformation->NameLength / 2) == 0)
        {

            if (KeyValueInformation->Type != REG_DWORD)
            {
                DPRINT("Bad data type for InitiatorTargetId\n");
                continue;
            }

            ConfigInfo->InitiatorBusId[0] = *((PUCHAR)
                (Buffer + KeyValueInformation->DataOffset));

            /* Check / reset if needed */
            if (ConfigInfo->InitiatorBusId[0] > ConfigInfo->MaximumNumberOfTargets - 1)
                ConfigInfo->InitiatorBusId[0] = (CCHAR)-1;

            DPRINT("InitiatorTargetId = %d\n", ConfigInfo->InitiatorBusId[0]);
        }

        /* Get ScsiDebug */
        if (_wcsnicmp(KeyValueInformation->Name, L"ScsiDebug",
            KeyValueInformation->NameLength/2) == 0)
        {
            DPRINT("ScsiDebug key not supported\n");
        }

        /* Check for a breakpoint */
        if (_wcsnicmp(KeyValueInformation->Name, L"BreakPointOnEntry",
            KeyValueInformation->NameLength/2) == 0)
        {
            DPRINT1("Breakpoint on entry requested!\n");
            DbgBreakPoint();
        }

        /* Get DisableSynchronousTransfers */
        if (_wcsnicmp(KeyValueInformation->Name, L"DisableSynchronousTransfers",
            KeyValueInformation->NameLength/2) == 0)
        {
            DeviceExtension->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
            DPRINT("Synch transfers disabled\n");
        }

        /* Get DisableDisconnects */
        if (_wcsnicmp(KeyValueInformation->Name, L"DisableDisconnects",
            KeyValueInformation->NameLength/2) == 0)
        {
            DeviceExtension->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;
            DPRINT("Disconnects disabled\n");
        }

        /* Get DisableTaggedQueuing */
        if (_wcsnicmp(KeyValueInformation->Name, L"DisableTaggedQueuing",
            KeyValueInformation->NameLength/2) == 0)
        {
            InternalConfigInfo->DisableTaggedQueueing = TRUE;
            DPRINT("Tagged queueing disabled\n");
        }

        /* Get DisableMultipleRequests */
        if (_wcsnicmp(KeyValueInformation->Name, L"DisableMultipleRequests",
            KeyValueInformation->NameLength/2) == 0)
        {
            InternalConfigInfo->DisableMultipleLun = TRUE;
            DPRINT("Multiple requests disabled\n");
        }

        /* Get DriverParameters */
        if (_wcsnicmp(KeyValueInformation->Name, L"DriverParameters",
            KeyValueInformation->NameLength/2) == 0)
        {
            /* Skip if nothing */
            if (KeyValueInformation->DataLength == 0)
                continue;

            /* If there was something previously allocated - free it */
            if (InternalConfigInfo->Parameter != NULL)
                ExFreePool(InternalConfigInfo->Parameter);

            /* Allocate it */
            InternalConfigInfo->Parameter = ExAllocatePoolWithTag(NonPagedPool,
                                                           KeyValueInformation->DataLength, TAG_SCSIPORT);

            if (InternalConfigInfo->Parameter != NULL)
            {
                if (KeyValueInformation->Type != REG_SZ)
                {
                    /* Just copy */
                    RtlCopyMemory(
                        InternalConfigInfo->Parameter,
                        (PCCHAR)KeyValueInformation + KeyValueInformation->DataOffset,
                        KeyValueInformation->DataLength);
                }
                else
                {
                    /* If it's a unicode string, convert it to ansi */
                    UnicodeString.Length = (USHORT)KeyValueInformation->DataLength;
                    UnicodeString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    UnicodeString.Buffer =
                        (PWSTR)((PCCHAR)KeyValueInformation + KeyValueInformation->DataOffset);

                    AnsiString.Length = 0;
                    AnsiString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    AnsiString.Buffer = (PCHAR)InternalConfigInfo->Parameter;

                    Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                          &UnicodeString,
                                                          FALSE);

                    /* In case of error, free the allocated space */
                    if (!NT_SUCCESS(Status))
                    {
                        ExFreePool(InternalConfigInfo->Parameter);
                        InternalConfigInfo->Parameter = NULL;
                    }

                }
            }

            DPRINT("Found driver parameter\n");
        }

        /* Get MaximumSGList */
        if (_wcsnicmp(KeyValueInformation->Name, L"MaximumSGList",
            KeyValueInformation->NameLength/2) == 0)
        {
            if (KeyValueInformation->Type != REG_DWORD)
            {
                DPRINT("Bad data type for MaximumSGList\n");
                continue;
            }

            ConfigInfo->NumberOfPhysicalBreaks = *((PUCHAR)(Buffer + KeyValueInformation->DataOffset));

            /* Check / fix */
            if (ConfigInfo->NumberOfPhysicalBreaks > SCSI_MAXIMUM_PHYSICAL_BREAKS)
            {
                ConfigInfo->NumberOfPhysicalBreaks = SCSI_MAXIMUM_PHYSICAL_BREAKS;
            }
            else if (ConfigInfo->NumberOfPhysicalBreaks < SCSI_MINIMUM_PHYSICAL_BREAKS)
            {
                ConfigInfo->NumberOfPhysicalBreaks = SCSI_MINIMUM_PHYSICAL_BREAKS;
            }

            DPRINT("MaximumSGList = %d\n", ConfigInfo->NumberOfPhysicalBreaks);
        }

        /* Get NumberOfRequests */
        if (_wcsnicmp(KeyValueInformation->Name, L"NumberOfRequests",
            KeyValueInformation->NameLength/2) == 0)
        {
            if (KeyValueInformation->Type != REG_DWORD)
            {
                DPRINT("NumberOfRequests has wrong data type\n");
                continue;
            }

            DeviceExtension->RequestsNumber = *((PUCHAR)(Buffer + KeyValueInformation->DataOffset));

            /* Check / fix */
            if (DeviceExtension->RequestsNumber < 16)
            {
                DeviceExtension->RequestsNumber = 16;
            }
            else if (DeviceExtension->RequestsNumber > 512)
            {
                DeviceExtension->RequestsNumber = 512;
            }

            DPRINT("Number Of Requests = %d\n", DeviceExtension->RequestsNumber);
        }

        /* Get resource list */
        if (_wcsnicmp(KeyValueInformation->Name, L"ResourceList",
                KeyValueInformation->NameLength/2) == 0 ||
            _wcsnicmp(KeyValueInformation->Name, L"Configuration Data",
                KeyValueInformation->NameLength/2) == 0 )
        {
            if (KeyValueInformation->Type != REG_FULL_RESOURCE_DESCRIPTOR ||
                KeyValueInformation->DataLength < sizeof(REG_FULL_RESOURCE_DESCRIPTOR))
            {
                DPRINT("Bad data type for ResourceList\n");
                continue;
            }
            else
            {
                DPRINT("Found ResourceList\n");
            }

            FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR)(Buffer + KeyValueInformation->DataOffset);

            /* Copy some info from it */
            InternalConfigInfo->BusNumber = FullResource->BusNumber;
            ConfigInfo->SystemIoBusNumber = FullResource->BusNumber;

            /* Loop through it */
            for (Count = 0; Count < FullResource->PartialResourceList.Count; Count++)
            {
                /* Get partial descriptor */
                PartialDescriptor =
                    &FullResource->PartialResourceList.PartialDescriptors[Count];

                /* Check datalength */
                if ((ULONG)((PCHAR)(PartialDescriptor + 1) -
                    (PCHAR)FullResource) > KeyValueInformation->DataLength)
                {
                    DPRINT("Resource data is of incorrect size\n");
                    break;
                }

                switch (PartialDescriptor->Type)
                {
                case CmResourceTypePort:
                    if (RangeCount >= ConfigInfo->NumberOfAccessRanges)
                    {
                        DPRINT("Too many access ranges\n");
                        continue;
                    }

                    InternalConfigInfo->AccessRanges[RangeCount].RangeInMemory = FALSE;
                    InternalConfigInfo->AccessRanges[RangeCount].RangeStart = PartialDescriptor->u.Port.Start;
                    InternalConfigInfo->AccessRanges[RangeCount].RangeLength = PartialDescriptor->u.Port.Length;
                    RangeCount++;

                    break;

                case CmResourceTypeMemory:
                    if (RangeCount >= ConfigInfo->NumberOfAccessRanges)
                    {
                        DPRINT("Too many access ranges\n");
                        continue;
                    }

                    InternalConfigInfo->AccessRanges[RangeCount].RangeInMemory = TRUE;
                    InternalConfigInfo->AccessRanges[RangeCount].RangeStart = PartialDescriptor->u.Memory.Start;
                    InternalConfigInfo->AccessRanges[RangeCount].RangeLength = PartialDescriptor->u.Memory.Length;
                    RangeCount++;

                    break;

                case CmResourceTypeInterrupt:

                    if (Interrupt == 0)
                    {
                        ConfigInfo->BusInterruptLevel =
                            PartialDescriptor->u.Interrupt.Level;

                        ConfigInfo->BusInterruptVector =
                            PartialDescriptor->u.Interrupt.Vector;

                        ConfigInfo->InterruptMode = (PartialDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? Latched : LevelSensitive;
                    }
                    else if (Interrupt == 1)
                    {
                        ConfigInfo->BusInterruptLevel2 =
                        PartialDescriptor->u.Interrupt.Level;

                        ConfigInfo->BusInterruptVector2 =
                        PartialDescriptor->u.Interrupt.Vector;

                        ConfigInfo->InterruptMode2 = (PartialDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? Latched : LevelSensitive;
                    }

                    Interrupt++;
                    break;

                case CmResourceTypeDma:

                    if (Dma == 0)
                    {
                        ConfigInfo->DmaChannel = PartialDescriptor->u.Dma.Channel;
                        ConfigInfo->DmaPort = PartialDescriptor->u.Dma.Port;

                        if (PartialDescriptor->Flags & CM_RESOURCE_DMA_8)
                            ConfigInfo->DmaWidth = Width8Bits;
                        else if ((PartialDescriptor->Flags & CM_RESOURCE_DMA_16) ||
                                 (PartialDescriptor->Flags & CM_RESOURCE_DMA_8_AND_16)) //???
                            ConfigInfo->DmaWidth = Width16Bits;
                        else if (PartialDescriptor->Flags & CM_RESOURCE_DMA_32)
                            ConfigInfo->DmaWidth = Width32Bits;
                    }
                    else if (Dma == 1)
                    {
                        ConfigInfo->DmaChannel2 = PartialDescriptor->u.Dma.Channel;
                        ConfigInfo->DmaPort2 = PartialDescriptor->u.Dma.Port;

                        if (PartialDescriptor->Flags & CM_RESOURCE_DMA_8)
                            ConfigInfo->DmaWidth2 = Width8Bits;
                        else if ((PartialDescriptor->Flags & CM_RESOURCE_DMA_16) ||
                                 (PartialDescriptor->Flags & CM_RESOURCE_DMA_8_AND_16)) //???
                            ConfigInfo->DmaWidth2 = Width16Bits;
                        else if (PartialDescriptor->Flags & CM_RESOURCE_DMA_32)
                            ConfigInfo->DmaWidth2 = Width32Bits;
                    }

                    Dma++;
                    break;

                case CmResourceTypeDeviceSpecific:
                    if (PartialDescriptor->u.DeviceSpecificData.DataSize <
                        sizeof(CM_SCSI_DEVICE_DATA) ||
                        (PCHAR) (PartialDescriptor + 1) - (PCHAR)FullResource +
                        PartialDescriptor->u.DeviceSpecificData.DataSize >
                        KeyValueInformation->DataLength)
                    {
                        DPRINT("Resource data length is incorrect");
                        break;
                    }

                    /* Set only one field from it */
                    ScsiDeviceData = (PCM_SCSI_DEVICE_DATA) (PartialDescriptor+1);
                    ConfigInfo->InitiatorBusId[0] = ScsiDeviceData->HostIdentifier;
                    break;
                }
            }
        }
    }
}

NTSTATUS
NTAPI
SpQueryDeviceCallout(IN PVOID  Context,
                     IN PUNICODE_STRING  PathName,
                     IN INTERFACE_TYPE  BusType,
                     IN ULONG  BusNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *BusInformation,
                     IN CONFIGURATION_TYPE  ControllerType,
                     IN ULONG  ControllerNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *ControllerInformation,
                     IN CONFIGURATION_TYPE  PeripheralType,
                     IN ULONG  PeripheralNumber,
                     IN PKEY_VALUE_FULL_INFORMATION  *PeripheralInformation)
{
    PBOOLEAN Found = (PBOOLEAN)Context;
    /* We just set our Found variable to TRUE */

    *Found = TRUE;
    return STATUS_SUCCESS;
}

IO_ALLOCATION_ACTION
NTAPI
ScsiPortAllocateAdapterChannel(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp,
                               IN PVOID MapRegisterBase,
                               IN PVOID Context)
{
    KIRQL Irql;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    /* Guard access with the spinlock */
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    /* Save MapRegisterBase we've got here */
    DeviceExtension->MapRegisterBase = MapRegisterBase;

    /* Start pending request */
    KeSynchronizeExecution(DeviceExtension->Interrupt[0],
        ScsiPortStartPacket, DeviceObject);

    /* Release spinlock we took */
    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

    return KeepObject;
}

static
NTSTATUS
SpiStatusSrbToNt(UCHAR SrbStatus)
{
    switch (SRB_STATUS(SrbStatus))
    {
    case SRB_STATUS_TIMEOUT:
    case SRB_STATUS_COMMAND_TIMEOUT:
        return STATUS_IO_TIMEOUT;

    case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
    case SRB_STATUS_BAD_FUNCTION:
        return STATUS_INVALID_DEVICE_REQUEST;

    case SRB_STATUS_NO_DEVICE:
    case SRB_STATUS_INVALID_LUN:
    case SRB_STATUS_INVALID_TARGET_ID:
    case SRB_STATUS_NO_HBA:
        return STATUS_DEVICE_DOES_NOT_EXIST;

    case SRB_STATUS_DATA_OVERRUN:
        return STATUS_BUFFER_OVERFLOW;

    case SRB_STATUS_SELECTION_TIMEOUT:
        return STATUS_DEVICE_NOT_CONNECTED;

    default:
        return STATUS_IO_DEVICE_ERROR;
    }

    return STATUS_IO_DEVICE_ERROR;
}


#undef ScsiPortConvertPhysicalAddressToUlong
/*
 * @implemented
 */
ULONG NTAPI
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  DPRINT("ScsiPortConvertPhysicalAddressToUlong()\n");
  return(Address.u.LowPart);
}

/* EOF */
