/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef _MSC_VER
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"
#else
#include <ddk/ntddk.h>
#include <net/ndis.h>
#endif
#include "debug.h"
#include "packet.h"

static NDIS_MEDIUM MediumArray[] = {
	NdisMedium802_3,
	NdisMediumWan,
	NdisMediumFddi,
	NdisMediumArcnet878_2,
	NdisMediumAtm,
	NdisMedium802_5
};

#define NUM_NDIS_MEDIA  (sizeof MediumArray / sizeof MediumArray[0])

ULONG NamedEventsCounter=0;

//Itoa. Replaces the buggy RtlIntegerToUnicodeString
void PacketItoa(UINT n,PUCHAR buf){
int i;

	for(i=0;i<20;i+=2){
		buf[18-i]=(n%10)+48;
		buf[19-i]=0;
		n/=10;
	}

}

/// Global start time. Used as an absolute reference for timestamp conversion.
struct time_conv G_Start_Time = {
	0,	
	{0, 0},	
};

UINT n_Opened_Instances = 0;

NDIS_SPIN_LOCK Opened_Instances_Lock;

//-------------------------------------------------------------------

NTSTATUS STDCALL
NPF_Open(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{

    PDEVICE_EXTENSION DeviceExtension;

    POPEN_INSTANCE    Open;

    PIO_STACK_LOCATION  IrpSp;

    NDIS_STATUS     Status;
    NDIS_STATUS     ErrorStatus;
    UINT            i;
	PUCHAR			tpointer;
    PLIST_ENTRY     PacketListEntry;
	PCHAR			EvName;

    IF_LOUD(DbgPrint("NPF: OpenAdapter\n");)

    DeviceExtension = DeviceObject->DeviceExtension;


    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //  allocate some memory for the open structure
#define NPF_TAG_OPENSTRUCT  TAG('0', 'O', 'W', 'A')
    Open=ExAllocatePoolWithTag(NonPagedPool, sizeof(OPEN_INSTANCE), NPF_TAG_OPENSTRUCT);


    if (Open==NULL) {
        // no memory
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        Open,
        sizeof(OPEN_INSTANCE)
        );


#define NPF_TAG_EVNAME  TAG('1', 'O', 'W', 'A')
	EvName=ExAllocatePoolWithTag(NonPagedPool, sizeof(L"\\BaseNamedObjects\\NPF0000000000"), NPF_TAG_EVNAME);

    if (EvName==NULL) {
        // no memory
        ExFreePool(Open);
	    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //  Save or open here
    IrpSp->FileObject->FsContext=Open;
	
    Open->DeviceExtension=DeviceExtension;
	
	
    //  Save the Irp here for the completeion routine to retrieve
    Open->OpenCloseIrp=Irp;
	
    //  Allocate a packet pool for our xmit and receive packets
    NdisAllocatePacketPool(
        &Status,
        &Open->PacketPool,
        TRANSMIT_PACKETS,
        sizeof(PACKET_RESERVED));
	
	
    if (Status != NDIS_STATUS_SUCCESS) {
		
        IF_LOUD(DbgPrint("NPF: Failed to allocate packet pool\n");)
			
		ExFreePool(Open);
		ExFreePool(EvName);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


	RtlCopyBytes(EvName,L"\\BaseNamedObjects\\NPF0000000000",sizeof(L"\\BaseNamedObjects\\NPF0000000000"));

	//Create the string containing the name of the read event
	RtlInitUnicodeString(&Open->ReadEventName,(PCWSTR) EvName);

	PacketItoa(NamedEventsCounter,(PUCHAR)(Open->ReadEventName.Buffer+21));

	InterlockedIncrement(&NamedEventsCounter);
	
	IF_LOUD(DbgPrint("\nCreated the named event for the read; name=%ws, counter=%d\n", Open->ReadEventName.Buffer,NamedEventsCounter-1);)

	//allocate the event objects
	Open->ReadEvent=IoCreateNotificationEvent(&Open->ReadEventName,&Open->ReadEventHandle);
	if(Open->ReadEvent==NULL){
		ExFreePool(Open);
		ExFreePool(EvName);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	KeInitializeEvent(Open->ReadEvent, NotificationEvent, FALSE);
	KeClearEvent(Open->ReadEvent);
	NdisInitializeEvent(&Open->WriteEvent);
	NdisInitializeEvent(&Open->IOEvent);
 	NdisInitializeEvent(&Open->DumpEvent);
 	NdisInitializeEvent(&Open->IOEvent);
	NdisAllocateSpinLock(&Open->machine_lock);


    //  list to hold irp's want to reset the adapter
    InitializeListHead(&Open->ResetIrpList);
	
	
    //  Initialize the request list
    KeInitializeSpinLock(&Open->RequestSpinLock);
    InitializeListHead(&Open->RequestList);

	// Initializes the extended memory of the NPF machine
#define NPF_TAG_MACHINE  TAG('2', 'O', 'W', 'A')
	Open->mem_ex.buffer = ExAllocatePoolWithTag(NonPagedPool, DEFAULT_MEM_EX_SIZE, NPF_TAG_MACHINE);
	if((Open->mem_ex.buffer) == NULL)
	{
        // no memory
        ExFreePool(Open);
		ExFreePool(EvName);
	    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	
	Open->mem_ex.size = DEFAULT_MEM_EX_SIZE;
	RtlZeroMemory(Open->mem_ex.buffer, DEFAULT_MEM_EX_SIZE);
	
	//
	// Initialize the open instance
	//
	Open->BufSize = 0;
	Open->Buffer = NULL;
	Open->Bhead = 0;
	Open->Btail = 0;
	(INT)Open->BLastByte = -1;
	Open->Dropped = 0;		//reset the dropped packets counter
	Open->Received = 0;		//reset the received packets counter
	Open->Accepted = 0;		//reset the accepted packets counter
	Open->bpfprogram = NULL;	//reset the filter
	Open->mode = MODE_CAPT;
	Open->Nbytes.QuadPart = 0;
	Open->Npackets.QuadPart = 0;
	Open->Nwrites = 1;
	Open->Multiple_Write_Counter = 0;
	Open->MinToCopy = 0;
	Open->TimeOut.QuadPart = (LONGLONG)1;
	Open->Bound = TRUE;
	Open->DumpFileName.Buffer = NULL;
	Open->DumpFileHandle = NULL;
	Open->tme.active = TME_NONE_ACTIVE;
	Open->DumpLimitReached = FALSE;
	Open->MaxFrameSize = 0;

	//allocate the spinlock for the statistic counters
    NdisAllocateSpinLock(&Open->CountersLock);

	//allocate the spinlock for the buffer pointers
    NdisAllocateSpinLock(&Open->BufLock);
	
    //
    //  link up the request stored in our open block
    //
    for (i=0;i<MAX_REQUESTS;i++) {
        ExInterlockedInsertTailList(
            &Open->RequestList,
            &Open->Requests[i].ListElement,
            &Open->RequestSpinLock);
		
    }
	

    IoMarkIrpPending(Irp);
	
    //
    //  Try to open the MAC
    //
    IF_LOUD(DbgPrint("NPF: Openinig the device %ws, BindingContext=%d\n",DeviceExtension->AdapterName.Buffer, Open);)

	NdisOpenAdapter(
        &Status,
        &ErrorStatus,
        &Open->AdapterHandle,
        &Open->Medium,
        MediumArray,
        NUM_NDIS_MEDIA,
        DeviceExtension->NdisProtocolHandle,
        Open,
        &DeviceExtension->AdapterName,
        0,
        NULL);

    IF_LOUD(DbgPrint("NPF: Opened the device, Status=%x\n",Status);)

	if (Status != NDIS_STATUS_PENDING)
    {
		NPF_OpenAdapterComplete(Open,Status,NDIS_STATUS_SUCCESS);
    }
	
    return(STATUS_PENDING);
}

//-------------------------------------------------------------------

VOID NPF_OpenAdapterComplete(
	IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus)
{

    PIRP                Irp;
    POPEN_INSTANCE      Open;
    PLIST_ENTRY			RequestListEntry;
	PINTERNAL_REQUEST	MaxSizeReq;
	NDIS_STATUS			ReqStatus;


    IF_LOUD(DbgPrint("NPF: OpenAdapterComplete\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

    //
    //  get the open irp
    //
    Irp=Open->OpenCloseIrp;

    if (Status != NDIS_STATUS_SUCCESS) {

        IF_LOUD(DbgPrint("NPF: OpenAdapterComplete-FAILURE\n");)

        NdisFreePacketPool(Open->PacketPool);

		//free mem_ex
		Open->mem_ex.size = 0;
		if(Open->mem_ex.buffer != NULL)ExFreePool(Open->mem_ex.buffer);

		ExFreePool(Open->ReadEventName.Buffer);

		ZwClose(Open->ReadEventHandle);


        ExFreePool(Open);
    }
	else {
		NdisAcquireSpinLock(&Opened_Instances_Lock);
		n_Opened_Instances++;
		NdisReleaseSpinLock(&Opened_Instances_Lock);
		
		IF_LOUD(DbgPrint("Opened Instances:%d", n_Opened_Instances);)

		// Get the absolute value of the system boot time.
		// This is used for timestamp conversion.
		TIME_SYNCHRONIZE(&G_Start_Time);

		// Extract a request from the list of free ones
		RequestListEntry=ExInterlockedRemoveHeadList(&Open->RequestList, &Open->RequestSpinLock);

		if (RequestListEntry == NULL)
		{

		    Open->MaxFrameSize = 1514;	// Assume Ethernet

			Irp->IoStatus.Status = Status;
		    Irp->IoStatus.Information = 0;
		    IoCompleteRequest(Irp, IO_NO_INCREMENT);

		    return;
		}

		MaxSizeReq = CONTAINING_RECORD(RequestListEntry, INTERNAL_REQUEST, ListElement);
		MaxSizeReq->Irp = Irp;
		MaxSizeReq->Internal = TRUE;

		
		MaxSizeReq->Request.RequestType = NdisRequestQueryInformation;
		MaxSizeReq->Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_TOTAL_SIZE;

		
		MaxSizeReq->Request.DATA.QUERY_INFORMATION.InformationBuffer = &Open->MaxFrameSize;
		MaxSizeReq->Request.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

		//  submit the request
		NdisRequest(
			&ReqStatus,
			Open->AdapterHandle,
			&MaxSizeReq->Request);


		if (ReqStatus != NDIS_STATUS_PENDING) {
			NPF_RequestComplete(Open, &MaxSizeReq->Request, ReqStatus);
		}

		return;

	}

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;

}

//-------------------------------------------------------------------

NTSTATUS STDCALL
NPF_Close(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{

    POPEN_INSTANCE    Open;
    NDIS_STATUS     Status;
    PIO_STACK_LOCATION  IrpSp;
	LARGE_INTEGER ThreadDelay;

    IF_LOUD(DbgPrint("NPF: CloseAdapter\n");)

	IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Open=IrpSp->FileObject->FsContext;

 	// Reset the buffer size. This tells the dump thread to stop.
 	Open->BufSize = 0;

	if( Open->Bound == FALSE){

		NdisWaitEvent(&Open->IOEvent,10000);

		// Free the filter if it's present
		if(Open->bpfprogram != NULL)
			ExFreePool(Open->bpfprogram);

		// Free the jitted filter if it's present
		if(Open->Filter != NULL)
			BPF_Destroy_JIT_Filter(Open->Filter);

		//free the buffer
		Open->BufSize=0;
		if(Open->Buffer != NULL)ExFreePool(Open->Buffer);
		
		//free mem_ex
		Open->mem_ex.size = 0;
		if(Open->mem_ex.buffer != NULL)ExFreePool(Open->mem_ex.buffer);
				
		NdisFreePacketPool(Open->PacketPool);

		// Free the string with the name of the dump file
		if(Open->DumpFileName.Buffer!=NULL)
			ExFreePool(Open->DumpFileName.Buffer);
			
		ExFreePool(Open->ReadEventName.Buffer);
		ExFreePool(Open);

		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		
		return(STATUS_SUCCESS);
	}

 	// Unfreeze the consumer
 	if(Open->mode & MODE_DUMP)
 		NdisSetEvent(&Open->DumpEvent);
 	else
 		KeSetEvent(Open->ReadEvent,0,FALSE);

    // Save the IRP
    Open->OpenCloseIrp = Irp;

    IoMarkIrpPending(Irp);
 
	// If this instance is in dump mode, complete the dump and close the file
	if((Open->mode & MODE_DUMP) && Open->DumpFileHandle != NULL){
		NTSTATUS wres;

		ThreadDelay.QuadPart = -50000000;
		// Wait the completion of the thread
		wres = KeWaitForSingleObject(Open->DumpThreadObject,
				UserRequest,
				KernelMode,
				TRUE,
				&ThreadDelay);

		ObDereferenceObject(Open->DumpThreadObject);


		// Flush and close the dump file
		NPF_CloseDumpFile(Open);
	}

	// Destroy the read Event
	ZwClose(Open->ReadEventHandle);

	// Close the adapter
	NdisCloseAdapter(
		&Status,
		Open->AdapterHandle
		);

	if (Status != NDIS_STATUS_PENDING) {
		
		NPF_CloseAdapterComplete(
			Open,
			Status
			);
		return STATUS_SUCCESS;
		
	}
	
	return(STATUS_PENDING);
}

//-------------------------------------------------------------------

VOID
NPF_CloseAdapterComplete(IN NDIS_HANDLE  ProtocolBindingContext,IN NDIS_STATUS  Status)
{
    POPEN_INSTANCE    Open;
    PIRP              Irp;

    IF_LOUD(DbgPrint("NPF: CloseAdapterComplete\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

	// free the allocated structures only if the instance is still bound to the adapter
	if(Open->Bound == TRUE){
		
		// Free the filter if it's present
		if(Open->bpfprogram != NULL)
			ExFreePool(Open->bpfprogram);

		// Free the jitted filter if it's present
		if(Open->Filter != NULL)
			BPF_Destroy_JIT_Filter(Open->Filter);
		
		//free the buffer
		Open->BufSize = 0;
		if(Open->Buffer!=NULL)ExFreePool(Open->Buffer);
		
		//free mem_ex
		Open->mem_ex.size = 0;
		if(Open->mem_ex.buffer != NULL)ExFreePool(Open->mem_ex.buffer);
		
		NdisFreePacketPool(Open->PacketPool);
		
		Irp=Open->OpenCloseIrp;
		
		// Free the string with the name of the dump file
		if(Open->DumpFileName.Buffer!=NULL)
			ExFreePool(Open->DumpFileName.Buffer);

		ExFreePool(Open->ReadEventName.Buffer);
		ExFreePool(Open);
		
		// Complete the request only if the instance is still bound to the adapter
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	else
		NdisSetEvent(&Open->IOEvent);

	// Decrease the counter of open instances
	NdisAcquireSpinLock(&Opened_Instances_Lock);
	n_Opened_Instances--;
	NdisReleaseSpinLock(&Opened_Instances_Lock);

	IF_LOUD(DbgPrint("Opened Instances:%d", n_Opened_Instances);)

	if(n_Opened_Instances == 0){
		// Force a synchronization at the next NPF_Open().
		// This hopefully avoids the synchronization issues caused by hibernation or standby.
		TIME_DESYNCHRONIZE(&G_Start_Time);
	}

	return;

}
//-------------------------------------------------------------------

#ifdef NDIS50
NDIS_STATUS
NPF_PowerChange(IN NDIS_HANDLE ProtocolBindingContext, IN PNET_PNP_EVENT pNetPnPEvent)
{
    IF_LOUD(DbgPrint("NPF: PowerChange\n");)

	TIME_DESYNCHRONIZE(&G_Start_Time);

	TIME_SYNCHRONIZE(&G_Start_Time);

	return STATUS_SUCCESS;
}
#endif

//-------------------------------------------------------------------

VOID
NPF_BindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    )
{
	IF_LOUD(DbgPrint("NPF: NPF_BindAdapter\n");)
}

//-------------------------------------------------------------------

VOID
NPF_UnbindAdapter(
    OUT PNDIS_STATUS        Status,
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         UnbindContext
    )
{
    POPEN_INSTANCE   Open =(POPEN_INSTANCE)ProtocolBindingContext;
	NDIS_STATUS		 lStatus;

	IF_LOUD(DbgPrint("NPF: NPF_UnbindAdapter\n");)

	// Reset the buffer size. This tells the dump thread to stop.
 	Open->BufSize=0;

	NdisResetEvent(&Open->IOEvent);

	// This open instance is no more bound to the adapter, set Bound to False
    InterlockedExchange( (PLONG) &Open->Bound, FALSE );

	// Awake a possible pending read on this instance
 	if(Open->mode & MODE_DUMP)
 		NdisSetEvent(&Open->DumpEvent);
 	else
 		KeSetEvent(Open->ReadEvent,0,FALSE);

	// If this instance is in dump mode, complete the dump and close the file
 	if((Open->mode & MODE_DUMP) && Open->DumpFileHandle != NULL)
 		NPF_CloseDumpFile(Open);

	// Destroy the read Event
	ZwClose(Open->ReadEventHandle);

    //  close the adapter
    NdisCloseAdapter(
        &lStatus,
        Open->AdapterHandle
	    );

    if (lStatus != NDIS_STATUS_PENDING) {

        NPF_CloseAdapterComplete(
            Open,
            lStatus
            );

		*Status = NDIS_STATUS_SUCCESS;
        return;

    }

	*Status = NDIS_STATUS_SUCCESS;
    return;
}

//-------------------------------------------------------------------

VOID
NPF_ResetComplete(IN NDIS_HANDLE  ProtocolBindingContext,IN NDIS_STATUS  Status)

{
    POPEN_INSTANCE      Open;
    PIRP                Irp;

    PLIST_ENTRY         ResetListEntry;

    IF_LOUD(DbgPrint("NPF: PacketResetComplte\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;


    //
    //  remove the reset IRP from the list
    //
    ResetListEntry=ExInterlockedRemoveHeadList(
                       &Open->ResetIrpList,
                       &Open->RequestSpinLock
                       );

#if DBG
    if (ResetListEntry == NULL) {
        DbgBreakPoint();
        return;
    }
#endif

    Irp=CONTAINING_RECORD(ResetListEntry,IRP,Tail.Overlay.ListEntry);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IF_LOUD(DbgPrint("NPF: PacketResetComplte exit\n");)

    return;

}
