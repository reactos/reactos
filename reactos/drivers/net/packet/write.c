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
//#include <net/miniport.h>
#include <net/ndis.h>
#endif

#include "debug.h"
#include "packet.h"



int nsent;

//-------------------------------------------------------------------
NTSTATUS STDCALL
PacketWrite(
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

	//
	// Check the length of the packet to avoid to use an empty packet
	//
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
		
		//
		//  Try to get a packet from our list of free ones
		//
		NdisAllocatePacket(
			&Status,
			&pPacket,
			Open->PacketPool
			);
		
		if (Status != NDIS_STATUS_SUCCESS) {
			
			//
			//  No free packets
			//
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			IoCompleteRequest (Irp, IO_NO_INCREMENT);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		
		RESERVED(pPacket)->Irp=Irp;
		
		//
		//  Attach the writes buffer to the packet
		//
		NdisChainBufferAtFront(pPacket,Irp->MdlAddress);
		
		//
		//  Call the MAC
		//

		NdisSend(
			&Status,
			Open->AdapterHandle,
			pPacket);

		if (Status != NDIS_STATUS_PENDING) {
			//
			//  The send didn't pend so call the completion handler now
			//
			PacketSendComplete(
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
VOID
PacketSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    )

{
    PIRP              Irp;
    PIO_STACK_LOCATION  irpSp;
    POPEN_INSTANCE      Open;

    IF_LOUD(DbgPrint("Packet: SendComplete, BindingContext=%d\n",ProtocolBindingContext);)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

	if((Open->Nwrites-Open->Multiple_Write_Counter)%100==99)
	 NdisSetEvent(&Open->WriteEvent);
	
	Open->Multiple_Write_Counter--;
	
	Irp=RESERVED(pPacket)->Irp;
	irpSp = IoGetCurrentIrpStackLocation(Irp);
	
	//
	//  recyle the packet
	//
	NdisReinitializePacket(pPacket);
	
	//
	//  Put the packet back on the free list
	//
	NdisFreePacket(pPacket);
	
	if(Open->Multiple_Write_Counter==0){
		//
		//  wake the application
		//
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = irpSp->Parameters.Write.Length;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	}

    return;

}
