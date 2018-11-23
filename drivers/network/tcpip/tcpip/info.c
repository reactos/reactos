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

VOID AddEntity(ULONG EntityType, PVOID Context, ULONG Flags)
{
    KIRQL OldIrql;
    ULONG i, Instance = 0;
    BOOLEAN ChoseIndex = FALSE;

    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    while (!ChoseIndex)
    {
         ChoseIndex = TRUE;
         for (i = 0; i < EntityCount; i++)
         {
             if (EntityList[i].tei_entity == EntityType &&
                 EntityList[i].tei_instance == Instance)
             {
                 Instance++;
                 ChoseIndex = FALSE;
             }
         }
    }

    EntityList[EntityCount].tei_entity = EntityType;
    EntityList[EntityCount].tei_instance = Instance;
    EntityList[EntityCount].context = Context;
    EntityList[EntityCount].flags = Flags;
    EntityCount++;

    TcpipReleaseSpinLock(&EntityListLock, OldIrql);
}

VOID RemoveEntityByContext(PVOID Context)
{
   ULONG i;
   KIRQL OldIrql;

   TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

   for (i = 0; i < EntityCount; i++) {
	if( EntityList[i].context == Context ) {
	    if( i != EntityCount - 1 ) {
		memcpy( &EntityList[i],
			&EntityList[--EntityCount],
			sizeof(EntityList[i]) );
	    } else {
		EntityCount--;
	    }
	}
    }

    TcpipReleaseSpinLock(&EntityListLock, OldIrql);
}

PVOID GetContext(TDIEntityID ID)
{
    UINT i;
    KIRQL OldIrql;
    PVOID Context;

    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    for (i = 0; i < EntityCount; i++)
    {
        if (EntityList[i].tei_entity == ID.tei_entity &&
            EntityList[i].tei_instance == ID.tei_instance)
            break;
    }

    if (i == EntityCount)
    {
        TcpipReleaseSpinLock(&EntityListLock, OldIrql);
        DbgPrint("WARNING: Unable to get context for %d %d\n", ID.tei_entity, ID.tei_instance);
        return NULL;
    }

    Context = EntityList[i].context;

    TcpipReleaseSpinLock(&EntityListLock, OldIrql);

    return Context;
}

TDI_STATUS InfoCopyOut( PCHAR DataOut, UINT SizeOut,
			PNDIS_BUFFER ClientBuf, PUINT ClientBufSize ) {
    UINT RememberedCBSize = *ClientBufSize;
    *ClientBufSize = SizeOut;

    /* The driver returns success even when it couldn't fit every available
     * byte. */
    if( RememberedCBSize < SizeOut || !ClientBuf )
	return TDI_SUCCESS;
    else {
	CopyBufferToBufferChain( ClientBuf, 0, (PCHAR)DataOut, SizeOut );
	return TDI_SUCCESS;
    }
}

TDI_STATUS InfoTdiQueryEntityType(TDIEntityID ID,
                                  PNDIS_BUFFER Buffer,
				  PUINT BufferSize)
{
    KIRQL OldIrql;
    UINT i, Flags = 0;

    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    for (i = 0; i < EntityCount; i++)
    {
        if (EntityList[i].tei_entity == ID.tei_entity &&
            EntityList[i].tei_instance == ID.tei_instance)
            break;
    }

    if (i == EntityCount)
    {
        TcpipReleaseSpinLock(&EntityListLock, OldIrql);
        return TDI_INVALID_PARAMETER;
    }

    Flags = EntityList[i].flags;

    InfoCopyOut((PCHAR)&Flags,
                sizeof(ULONG),
                Buffer,
                BufferSize);

    TcpipReleaseSpinLock(&EntityListLock, OldIrql);

    return TDI_SUCCESS;
}

TDI_STATUS InfoTdiQueryListEntities(PNDIS_BUFFER Buffer,
				    PUINT BufferSize)
{
    UINT Count, Size, BufSize = *BufferSize;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_INFO,("About to copy %d TDIEntityIDs to user\n",
			   EntityCount));

    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    Size = EntityCount * sizeof(TDIEntityID);
    *BufferSize = Size;

    TI_DbgPrint(DEBUG_INFO,("BufSize: %d, NeededSize: %d\n", BufSize, Size));

    if (BufSize < Size || !Buffer)
    {
	TcpipReleaseSpinLock( &EntityListLock, OldIrql );
	/* The buffer is too small to contain requested data, but we return
         * success anyway, as we did everything we wanted. */
	return TDI_SUCCESS;
    }

    /* Return entity list -- Copy only the TDIEntityID parts. */
    for( Count = 0; Count < EntityCount; Count++ ) {
	CopyBufferToBufferChain(Buffer,
				Count * sizeof(TDIEntityID),
				(PCHAR)&EntityList[Count],
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
    PVOID EntityListContext;

    TI_DbgPrint(DEBUG_INFO,
		("InfoEx Req: %x %x %x!%04x:%d\n",
		 ID->toi_class,
		 ID->toi_type,
		 ID->toi_id,
		 ID->toi_entity.tei_entity,
		 ID->toi_entity.tei_instance));

    switch (ID->toi_class)
    {
        case INFO_CLASS_GENERIC:
           switch (ID->toi_id)
           {
              case ENTITY_LIST_ID:
                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 return InfoTdiQueryListEntities(Buffer, BufferSize);

              case ENTITY_TYPE_ID:
                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 return InfoTdiQueryEntityType(ID->toi_entity, Buffer, BufferSize);

              default:
                 return TDI_INVALID_REQUEST;
           }

        case INFO_CLASS_PROTOCOL:
           if (ID->toi_type == INFO_TYPE_ADDRESS_OBJECT)
           {
               if ((EntityListContext = GetContext(ID->toi_entity)))
                    return GetAddressFileInfo(ID, EntityListContext, Buffer, BufferSize);
               else
                    return TDI_INVALID_PARAMETER;
           }

           switch (ID->toi_id)
           {
              case IF_MIB_STATS_ID:
                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 if (ID->toi_entity.tei_entity == IF_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetInterfaceMIB(ID->toi_entity, EntityListContext, Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else if (ID->toi_entity.tei_entity == CL_NL_ENTITY ||
                          ID->toi_entity.tei_entity == CO_NL_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetIPSnmpInfo(ID->toi_entity, EntityListContext, Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else
                     return TDI_INVALID_PARAMETER;

              case IP_MIB_ADDRTABLE_ENTRY_ID:
                 if (ID->toi_entity.tei_entity != CL_NL_ENTITY && 
                     ID->toi_entity.tei_entity != CO_NL_ENTITY)
                     return TDI_INVALID_PARAMETER;

                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 return InfoTdiQueryGetAddrTable(ID->toi_entity, Buffer, BufferSize);

              case IP_MIB_ARPTABLE_ENTRY_ID:
                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 if (ID->toi_entity.tei_entity == AT_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetArptableMIB(ID->toi_entity, EntityListContext,
                                                           Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else if (ID->toi_entity.tei_entity == CO_NL_ENTITY ||
                          ID->toi_entity.tei_entity == CL_NL_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetRouteTable(EntityListContext, Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else if (ID->toi_entity.tei_entity == CO_TL_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetConnectionTcpTable(EntityListContext, Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else if (ID->toi_entity.tei_entity == CL_TL_ENTITY)
                     if ((EntityListContext = GetContext(ID->toi_entity)))
                         return InfoTdiQueryGetConnectionUdpTable(EntityListContext, Buffer, BufferSize);
                     else
                         return TDI_INVALID_PARAMETER;
                 else
                     return TDI_INVALID_PARAMETER;

#if 0
              case IP_INTFC_INFO_ID:
                 if (ID->toi_type != INFO_TYPE_PROVIDER)
                     return TDI_INVALID_PARAMETER;

                 return InfoTdiQueryGetIFInfo(Context, Buffer, BufferSize);
#endif

              default:
                 return TDI_INVALID_REQUEST;
           }

        default:
           return TDI_INVALID_REQUEST;
    }
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
    PVOID EntityListContext;

    switch (ID->toi_class)
    {
        case INFO_CLASS_PROTOCOL:
        {
            switch (ID->toi_type)
            {
                case INFO_TYPE_ADDRESS_OBJECT:
                {
                    if ((EntityListContext = GetContext(ID->toi_entity)))
                        return SetAddressFileInfo(ID, EntityListContext, Buffer, BufferSize);
                    else
                        return TDI_INVALID_PARAMETER;
                }
                case INFO_TYPE_CONNECTION:
                {
                    PADDRESS_FILE AddressFile = GetContext(ID->toi_entity);
                    if (AddressFile == NULL)
                        return TDI_INVALID_PARAMETER;
                    return SetConnectionInfo(ID, AddressFile->Connection, Buffer, BufferSize);
                }
                case INFO_TYPE_PROVIDER:
                {
                    switch (ID->toi_id)
                    {
                        case IP_MIB_ARPTABLE_ENTRY_ID:
                            if (ID->toi_type != INFO_TYPE_PROVIDER)
                                return TDI_INVALID_PARAMETER;

                            if (ID->toi_entity.tei_entity == AT_ENTITY)
                                if ((EntityListContext = GetContext(ID->toi_entity)))
                                    return InfoTdiSetArptableMIB(EntityListContext, Buffer,
                                        BufferSize);
                                else
                                    return TDI_INVALID_PARAMETER;
                            else if (ID->toi_entity.tei_entity == CL_NL_ENTITY
                                || ID->toi_entity.tei_entity == CO_NL_ENTITY)
                                if ((EntityListContext = GetContext(ID->toi_entity)))
                                    return InfoTdiSetRoute(EntityListContext, Buffer, BufferSize);
                                else
                                    return TDI_INVALID_PARAMETER;
                            else
                                return TDI_INVALID_PARAMETER;

                        default:
                            return TDI_INVALID_REQUEST;
                    }
                }
                default:
                    DbgPrint("TCPIP: IOCTL_TCP_SET_INFORMATION_EX - Unrecognized information type for INFO_CLASS_PROTOCOL: %#x.\n", ID->toi_type);
                    return TDI_INVALID_PARAMETER;
            }
            break;
        }
        default:
            DbgPrint("TCPIP: IOCTL_TCP_SET_INFORMATION_EX - Unrecognized information class %#x.\n", ID->toi_class);
            return TDI_INVALID_REQUEST;
    }
}
