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
#include "stdarg.h"
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"
#else
#include <ddk/ntddk.h>
#include <net/ndis.h>
#endif

#include "debug.h"
#include "packet.h"


//-------------------------------------------------------------------
NTSTATUS
//STDCALL
NPF_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    POPEN_INSTANCE		Open;
    PIO_STACK_LOCATION	IrpSp;
    PNDIS_PACKET		pPacket;
	UINT				i;
    NDIS_STATUS		    Status;


    IF_LOUD(DbgPrint("Packet: SendAdapter\n");)

    IrpSp = IoGetCurrentIrpStackLocation(Irp);


    Open=IrpSp->FileObject->FsContext;

	// Check the length of the packet to avoid to use an empty packet
	if(IrpSp->Parameters.Write.Length==0)
	{
        Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return NDIS_STATUS_SUCCESS;
	}


    IoMarkIrpPending(Irp);

	Open->Multiple_Write_Counter=Open->Nwrites;

	NdisResetEvent(&Open->WriteEvent);


	for(i=0;i<Open->Nwrites;i++){
		
		//  Try to get a packet from our list of free ones
		NdisAllocatePacket(
			&Status,
			&pPacket,
			Open->PacketPool
			);
		
		if (Status != NDIS_STATUS_SUCCESS) {
			
			//  No free packets
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			IoCompleteRequest (Irp, IO_NO_INCREMENT);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		
		// The packet has a buffer that needs not to be freed after every single write
		RESERVED(pPacket)->FreeBufAfterWrite = FALSE;

		// Save the IRP associated with the packet
		RESERVED(pPacket)->Irp=Irp;
		
		//  Attach the writes buffer to the packet
		NdisChainBufferAtFront(pPacket,Irp->MdlAddress);
		
		//  Call the MAC
		NdisSend(
			&Status,
			Open->AdapterHandle,
			pPacket);

		if (Status != NDIS_STATUS_PENDING) {
			//  The send didn't pend so call the completion handler now
			NPF_SendComplete(
				Open,
				pPacket,
				Status
				);
			
		}
		
		if(i%100==99){
			NdisWaitEvent(&Open->WriteEvent,1000);  
			NdisResetEvent(&Open->WriteEvent);
		}
	}
	
    return(STATUS_PENDING);

}


//-------------------------------------------------------------------

INT
NPF_BufferedWrite(
	IN PIRP Irp, 
	IN PCHAR UserBuff, 
	IN ULONG UserBuffSize, 
	BOOLEAN Sync)
{
    POPEN_INSTANCE		Open;
    PIO_STACK_LOCATION	IrpSp;
    PNDIS_PACKET		pPacket;
	UINT				i;
    NDIS_STATUS		    Status;
	LARGE_INTEGER		StartTicks, CurTicks, TargetTicks;
	LARGE_INTEGER		TimeFreq;
	struct timeval		BufStartTime;
	struct sf_pkthdr	*winpcap_hdr;
	PMDL				TmpMdl;
	PCHAR				CurPos;
	PCHAR				EndOfUserBuff = UserBuff + UserBuffSize;
	
    IF_LOUD(DbgPrint("NPF: BufferedWrite, UserBuff=%x, Size=%u\n", UserBuff, UserBuffSize);)
		
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
    Open=IrpSp->FileObject->FsContext;
	
	// Security check on the length of the user buffer
	if(UserBuff==0)
	{
		return 0;
	}
		
	// Start from the first packet
	winpcap_hdr = (struct sf_pkthdr*)UserBuff;
	
	// Retrieve the time references
	StartTicks = KeQueryPerformanceCounter(&TimeFreq);
	BufStartTime.tv_sec = winpcap_hdr->ts.tv_sec;
	BufStartTime.tv_usec = winpcap_hdr->ts.tv_usec;
	
	// Chech the consistency of the user buffer
	if( (PCHAR)winpcap_hdr + winpcap_hdr->caplen + sizeof(struct sf_pkthdr) > EndOfUserBuff )
	{
		IF_LOUD(DbgPrint("Buffered Write: bogus packet buffer\n");)

		return -1;
	}
	
	// Save the current time stamp counter
	CurTicks = KeQueryPerformanceCounter(NULL);
	
	// Main loop: send the buffer to the wire
	while( TRUE ){
		
		if(winpcap_hdr->caplen ==0 || winpcap_hdr->caplen > 65536)
		{
			// Malformed header
			IF_LOUD(DbgPrint("NPF_BufferedWrite: malformed user buffer, aborting write.\n");)
			
			return -1;
		}
		
		// Allocate an MDL to map the packet data
		TmpMdl=IoAllocateMdl((PCHAR)winpcap_hdr + sizeof(struct sf_pkthdr),
			winpcap_hdr->caplen,
			FALSE,
			FALSE,
			NULL);
		
		if (TmpMdl == NULL)
		{
			// Unable to map the memory: packet lost
			IF_LOUD(DbgPrint("NPF_BufferedWrite: unable to allocate the MDL.\n");)

			return -1;
		}
		
		MmBuildMdlForNonPagedPool(TmpMdl);	// XXX can this line be removed?
		
		// Allocate a packet from our free list
		NdisAllocatePacket( &Status, &pPacket, Open->PacketPool);
		
		if (Status != NDIS_STATUS_SUCCESS) {
			//  No free packets
			IF_LOUD(DbgPrint("NPF_BufferedWrite: no more free packets, returning.\n");)

			return (PCHAR)winpcap_hdr - UserBuff;
		}
		
		// The packet has a buffer that needs to be freed after every single write
		RESERVED(pPacket)->FreeBufAfterWrite = TRUE;
		
		// Attach the MDL to the packet
		NdisChainBufferAtFront(pPacket,TmpMdl);
		
		// Call the MAC
		NdisSend( &Status, Open->AdapterHandle,	pPacket);
		
		if (Status != NDIS_STATUS_PENDING) {
			// The send didn't pend so call the completion handler now
			NPF_SendComplete(
				Open,
				pPacket,
				Status
				);				
		}
		
		// Step to the next packet in the buffer
		(PCHAR)winpcap_hdr += winpcap_hdr->caplen + sizeof(struct sf_pkthdr);
		
		// Check if the end of the user buffer has been reached
		if( (PCHAR)winpcap_hdr >= EndOfUserBuff )
		{
			IF_LOUD(DbgPrint("NPF_BufferedWrite: End of buffer.\n");)

			return (PCHAR)winpcap_hdr - UserBuff;
		}
		
		if( Sync ){

#if 0
			// Release the application if it has been blocked for approximately more than 1 seconds
			if( winpcap_hdr->ts.tv_sec - BufStartTime.tv_sec > 1 )
			{
				IF_LOUD(DbgPrint("NPF_BufferedWrite: timestamp elapsed, returning.\n");)
					
				return (PCHAR)winpcap_hdr - UserBuff;
			}
			
			// Calculate the time interval to wait before sending the next packet
			TargetTicks.QuadPart = StartTicks.QuadPart +
				(LONGLONG)((winpcap_hdr->ts.tv_sec - BufStartTime.tv_sec) * 1000000 +
				winpcap_hdr->ts.tv_usec - BufStartTime.tv_usec) *
				(TimeFreq.QuadPart) / 1000000;
			
			// Wait until the time interval has elapsed
			while( CurTicks.QuadPart <= TargetTicks.QuadPart )
				CurTicks = KeQueryPerformanceCounter(NULL);
#endif

		}
		
	}
			
	return (PCHAR)winpcap_hdr - UserBuff;
		
}


//-------------------------------------------------------------------

VOID
NPF_SendComplete(
				   IN NDIS_HANDLE   ProtocolBindingContext,
				   IN PNDIS_PACKET  pPacket,
				   IN NDIS_STATUS   Status
				   )
				   
{
	PIRP              Irp;
	PIO_STACK_LOCATION  irpSp;
	POPEN_INSTANCE      Open;
	PMDL TmpMdl;
	
	IF_LOUD(DbgPrint("NPF: SendComplete, BindingContext=%d\n",ProtocolBindingContext);)
		
	Open= (POPEN_INSTANCE)ProtocolBindingContext;
	
	if( RESERVED(pPacket)->FreeBufAfterWrite ){
		// Free the MDL associated with the packet
		NdisUnchainBufferAtFront(pPacket, &TmpMdl);
		IoFreeMdl(TmpMdl);
	}
	else{
		if((Open->Nwrites - Open->Multiple_Write_Counter) %100 == 99)
			NdisSetEvent(&Open->WriteEvent);
		
		Open->Multiple_Write_Counter--;
	}
	
	//  recyle the packet
	NdisReinitializePacket(pPacket);
	
	//  Put the packet back on the free list
	NdisFreePacket(pPacket);
	
	if( !(RESERVED(pPacket)->FreeBufAfterWrite) ){
		if(Open->Multiple_Write_Counter==0){
			// Release the buffer and awake the application
			NdisUnchainBufferAtFront(pPacket, &TmpMdl);
			
			Irp=RESERVED(pPacket)->Irp;
			irpSp = IoGetCurrentIrpStackLocation(Irp);

			Irp->IoStatus.Status = Status;
			Irp->IoStatus.Information = irpSp->Parameters.Write.Length;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			
		}
	}

	return;
}
