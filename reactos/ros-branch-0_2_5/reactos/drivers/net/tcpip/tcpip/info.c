/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/info.c
 * PURPOSE:     TDI query and set information routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"
#include <debug.h>
#include <route.h>

TDI_STATUS InfoCopyOut( PCHAR DataOut, UINT SizeOut,
			PNDIS_BUFFER ClientBuf, PUINT ClientBufSize ) {
    UINT RememberedCBSize = *ClientBufSize;
    *ClientBufSize = SizeOut;
    if( RememberedCBSize < SizeOut )
	return TDI_BUFFER_TOO_SMALL;
    else {
	CopyBufferToBufferChain( ClientBuf, 0, (PUCHAR)DataOut, SizeOut );
	return TDI_SUCCESS;
    }
}

VOID InsertTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    KIRQL OldIrql;
    UINT Count = 0, i;

    TI_DbgPrint(MAX_TRACE, 
		("Inserting interface %08x (%d entities already)\n", 
		 Interface, EntityCount));

    TcpipAcquireSpinLock( &EntityListLock, &OldIrql );

    /* Count IP Entities */
    for( i = 0; i < EntityCount; i++ )
	if( EntityList[i].tei_entity == IF_ENTITY ) {
	    Count++;
	    TI_DbgPrint(MAX_TRACE, ("Entity %d is an IF.  Found %d\n", 
				    i, Count));
	}
    
    EntityList[EntityCount].tei_entity = IF_ENTITY;
    EntityList[EntityCount].tei_instance = Count;
    EntityList[EntityCount].context  = Interface;
    EntityList[EntityCount].info_req = InfoInterfaceTdiQueryEx;
    EntityList[EntityCount].info_set = InfoInterfaceTdiSetEx;
    
    EntityCount++;

    TcpipReleaseSpinLock( &EntityListLock, OldIrql );
}

VOID RemoveTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    KIRQL OldIrql;
    UINT i;

    TcpipAcquireSpinLock( &EntityListLock, &OldIrql );
    
    /* Remove entities that have this interface as context
     * In the future, this might include AT_ENTITY types, too
     */
    for( i = 0; i < EntityCount; i++ ) {
	if( EntityList[i].context == Interface ) {
	    if( i != EntityCount-1 ) 
		memcpy( &EntityList[i], 
			&EntityList[--EntityCount],
			sizeof(EntityList[i]) );
	}
    }

    TcpipReleaseSpinLock( &EntityListLock, OldIrql );
}

TDI_STATUS InfoTdiQueryListEntities(PNDIS_BUFFER Buffer,
				    PUINT BufferSize)
{
    UINT Count, Size, BufSize = *BufferSize;
    KIRQL OldIrql;

    TI_DbgPrint(MAX_TRACE,("About to copy %d TDIEntityIDs to user\n",
			   EntityCount));
    
    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    Size = EntityCount * sizeof(TDIEntityID);
    *BufferSize = Size;
    
    if (BufSize < Size)
    {
	TcpipReleaseSpinLock( &EntityListLock, OldIrql );
	/* The buffer is too small to contain requested data */
	return TDI_BUFFER_TOO_SMALL;
    }
        
    /* Return entity list -- Copy only the TDIEntityID parts. */
    for( Count = 0; Count < EntityCount; Count++ ) {
	CopyBufferToBufferChain(Buffer, 
				Count * sizeof(TDIEntityID), 
				(PUCHAR)&EntityList[Count], 
				sizeof(TDIEntityID));
    }
    
    TcpipReleaseSpinLock(&EntityListLock, OldIrql);
    
    return TDI_SUCCESS;
}

TDI_STATUS InfoTdiQueryInformationEx(
  PTDI_REQUEST Request,
  TDIObjectID *ID,
  PNDIS_BUFFER Buffer,
  PUINT BufferSize,
  PVOID Context)
/*
 * FUNCTION: Returns extended information
 * ARGUMENTS:
 *   Request    = Pointer to TDI request structure for the request
 *   ID         = TDI object ID
 *   Buffer     = Pointer to buffer with data to use
 *   BufferSize = Pointer to buffer with size of Buffer. On return
 *                this is filled with number of bytes returned
 *   Context    = Pointer to context buffer
 * RETURNS:
 *   Status of operation
 */
{
    KIRQL OldIrql;
    UINT i;
    PVOID context;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL FoundEntity = FALSE;
    InfoRequest_f InfoRequest;

    TI_DbgPrint(MAX_TRACE,
		("InfoEx Req: %x %x %x!%04x:%d\n",
		 ID->toi_class,
		 ID->toi_type,
		 ID->toi_id,
		 ID->toi_entity.tei_entity,
		 ID->toi_entity.tei_instance));

    /* Check wether it is a query for a list of entities */
    if (ID->toi_entity.tei_entity == GENERIC_ENTITY)
    {
	if ((ID->toi_class != INFO_CLASS_GENERIC) ||
	    (ID->toi_type != INFO_TYPE_PROVIDER) ||
	    (ID->toi_id != ENTITY_LIST_ID)) {
	    TI_DbgPrint(MAX_TRACE,("Invalid parameter\n"));
	    Status = TDI_INVALID_PARAMETER;
        } else
	    Status = InfoTdiQueryListEntities(Buffer, BufferSize);
    } else {
	TcpipAcquireSpinLock( &EntityListLock, &OldIrql );
	
	for( i = 0; i < EntityCount; i++ ) {
	    if( EntityList[i].tei_entity == ID->toi_entity.tei_entity &&
		EntityList[i].tei_instance == ID->toi_entity.tei_instance ) {
		InfoRequest = EntityList[i].info_req;
		context = EntityList[i].context;
		FoundEntity = TRUE;
		break;
	    }
	}
	
	TcpipReleaseSpinLock( &EntityListLock, OldIrql );
	
	if( FoundEntity ) {
	    TI_DbgPrint(MAX_TRACE,
			("Calling Entity %d (%04x:%d) InfoEx (%x,%x,%x)\n",
			 i, ID->toi_entity.tei_entity,
			 ID->toi_entity.tei_instance,
			 ID->toi_class, ID->toi_type, ID->toi_id));
	    Status = InfoRequest( ID->toi_class,
				  ID->toi_type,
				  ID->toi_id,
				  context,
				  &ID->toi_entity,
				  Buffer,
				  BufferSize );
	}
    }

    TI_DbgPrint(MAX_TRACE,("Status: %08x\n", Status));

    return Status;
}

TDI_STATUS InfoTdiSetInformationEx
(PTDI_REQUEST Request,
 TDIObjectID *ID,
 PVOID Buffer,
 UINT BufferSize)
/*
 * FUNCTION: Sets extended information
 * ARGUMENTS:
 *   Request    = Pointer to TDI request structure for the request
 *   ID         = Pointer to TDI object ID
 *   Buffer     = Pointer to buffer with data to use
 *   BufferSize = Size of Buffer
 * RETURNS:
 *   Status of operation
 */
{
    switch( ID->toi_class ) {
    case INFO_CLASS_PROTOCOL:
	switch( ID->toi_type ) {
	case INFO_TYPE_PROVIDER:
	    switch( ID->toi_id ) {
	    case IP_MIB_ROUTETABLE_ENTRY_ID:
		return InfoNetworkLayerTdiSetEx
		    ( ID->toi_class,
		      ID->toi_type,
		      ID->toi_id,
		      NULL,
		      &ID->toi_entity,
		      Buffer,
		      BufferSize );
	    }
	}
	break;
    }
    
    return TDI_INVALID_PARAMETER;
}
