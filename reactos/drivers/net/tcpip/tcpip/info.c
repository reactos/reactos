/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/info.c
 * PURPOSE:     TDI query and set information routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <info.h>
#include <routines.h>


TDI_STATUS IPTdiQueryInformationEx(
  PTDI_REQUEST Request,
  TDIObjectID *ID,
  PNDIS_BUFFER Buffer,
  PUINT BufferSize,
  PVOID Context)
/*
 * FUNCTION: Returns extended information about network layer
 * ARGUMENTS:
 *   Request    = Pointer to TDI request structure for the request
 *   ID         = TDI object ID
 *   Buffer     = Pointer to buffer with data to use. 
 *   BufferSize = Pointer to buffer with size of Buffer. On return
 *                  this is filled with number of bytes returned
 *   Context    = Pointer to context buffer
 * RETURNS:
 *   Status of operation
 */
{
  PLIST_ENTRY CurrentIFEntry;
  PLIST_ENTRY CurrentADEEntry;
  PADDRESS_ENTRY CurrentADE;
  PIP_INTERFACE CurrentIF;
  IPADDR_ENTRY IpAddress;
  IPSNMP_INFO SnmpInfo;
  KIRQL OldIrql;
  ULONG Entity;
  ULONG Temp;
  UINT Count;
  UINT BufSize;

  BufSize = *BufferSize;

  /* Make return parameters consistent every time */
  *BufferSize = 0;

  Entity = ID->toi_entity.tei_entity;
  if (Entity != CL_NL_ENTITY)
    {
      /* We can't handle this entity */
      return TDI_INVALID_PARAMETER;
    }

  if (ID->toi_entity.tei_instance != TL_INSTANCE)
    {
      /* Only a single instance is supported */
      return TDI_INVALID_REQUEST;
    }

  if (ID->toi_class == INFO_CLASS_GENERIC)
    {
      if ((ID->toi_type == INFO_TYPE_PROVIDER) && 
        (ID->toi_id == ENTITY_TYPE_ID))
        {
          if (BufSize < sizeof(ULONG))
            {
              return TDI_BUFFER_TOO_SMALL;
            }

          Temp = CL_NL_IP;
          Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&Temp, sizeof(ULONG));

          return TDI_SUCCESS;
        }

      return TDI_INVALID_PARAMETER;
    }

  if (ID->toi_class == INFO_CLASS_PROTOCOL)
    {
      if (ID->toi_type != INFO_TYPE_PROVIDER)
        {
          return TDI_INVALID_PARAMETER;
        }

      switch (ID->toi_id)
        {
          case IP_MIB_ADDRTABLE_ENTRY_ID:
            Temp = 0;

            KeAcquireSpinLock(&InterfaceListLock, &OldIrql);

            CurrentIFEntry = InterfaceListHead.Flink;
            while (CurrentIFEntry != &InterfaceListHead)
              {
	              CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);

                if (Temp + sizeof(IPADDR_ENTRY) > BufSize)
                  {
                    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
                    return TDI_BUFFER_TOO_SMALL;
                  }

                IpAddress.Addr      = 0;
                IpAddress.BcastAddr = 0;
                IpAddress.Mask      = 0;

                /* Locate the diffrent addresses and put them the right place */
                CurrentADEEntry = CurrentIF->ADEListHead.Flink;
                while (CurrentADEEntry != &CurrentIF->ADEListHead)
                  {
	                  CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);

                    switch (CurrentADE->Type)
                      {
                        case ADE_UNICAST:
                          IpAddress.Addr = CurrentADE->Address->Address.IPv4Address;
                          break;
                        case ADE_MULTICAST:
                          IpAddress.BcastAddr = CurrentADE->Address->Address.IPv4Address;
                          break;
                        case ADE_ADDRMASK:
                          IpAddress.Mask = CurrentADE->Address->Address.IPv4Address;
                          break;
                        default:
                          /* Should not happen */
                          TI_DbgPrint(MIN_TRACE, ("Unknown address entry type (0x%X)\n", CurrentADE->Type));
                          break;
                      }
                    CurrentADEEntry = CurrentADEEntry->Flink;
                  }

                /* Pack the address information into IPADDR_ENTRY structure */
                IpAddress.Index     = 0;
                IpAddress.ReasmSize = 0;
                IpAddress.Context   = 0;
                IpAddress.Pad       = 0;

                Count = CopyBufferToBufferChain(Buffer, Temp, (PUCHAR)&IpAddress, sizeof(IPADDR_ENTRY));
                Temp += sizeof(IPADDR_ENTRY);

                CurrentIFEntry = CurrentIFEntry->Flink;
              }

            KeReleaseSpinLock(&InterfaceListLock, OldIrql);

            return TDI_SUCCESS;

          case IP_MIB_STATS_ID:
            if (BufSize < sizeof(IPSNMP_INFO))
              {
                return TDI_BUFFER_TOO_SMALL;
              }

            RtlZeroMemory(&SnmpInfo, sizeof(IPSNMP_INFO));

	    /* Count number of interfaces */
	    Count = 0;
	    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);

	    CurrentIFEntry = InterfaceListHead.Flink;
	    while (CurrentIFEntry != &InterfaceListHead)
	      {
		Count++;
		CurrentIFEntry = CurrentIFEntry->Flink;
	      }
	    
	    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
	    
	    SnmpInfo.NumIf = Count;

            /* Count number of addresses */
            Count = 0;
            KeAcquireSpinLock(&InterfaceListLock, &OldIrql);

            CurrentIFEntry = InterfaceListHead.Flink;
            while (CurrentIFEntry != &InterfaceListHead)
              {
	              CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);
                  Count++;
                  CurrentIFEntry = CurrentIFEntry->Flink;
              }

            KeReleaseSpinLock(&InterfaceListLock, OldIrql);

            SnmpInfo.NumAddr = Count;
            Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&SnmpInfo, sizeof(IPSNMP_INFO));

            return TDI_SUCCESS;

          default:
            /* We can't handle this ID */
            return TDI_INVALID_PARAMETER;
        }
    }

  return TDI_INVALID_PARAMETER;
}

TDI_STATUS InfoTdiQueryListEntities(PNDIS_BUFFER Buffer,
				    UINT BufSize,
				    PUINT BufferSize)
{
    UINT Count, Size, Temp;
    KIRQL OldIrql;
    PLIST_ENTRY CurrentIFEntry;

    /* Count Adapters */
    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    CurrentIFEntry = InterfaceListHead.Flink;
    Count = EntityCount;
    
    while( CurrentIFEntry != &InterfaceListHead ) {
	Count++;
	CurrentIFEntry = CurrentIFEntry->Flink;
    }
    
    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
    
    Size = Count * sizeof(TDIEntityID);
    *BufferSize = Size;
    
    if (BufSize < Size)
    {
	/* The buffer is too small to contain requested data */
	return TDI_BUFFER_TOO_SMALL;
    }
    
    DbgPrint("About to copy %d TDIEntityIDs (%d bytes) to user\n",
	     Count, Size);
    
    KeAcquireSpinLock(&EntityListLock, &OldIrql);
    
    /* Update entity list */
    for( Temp = EntityCount; Temp < Count; Temp++ ) {
	EntityList[Temp].tei_entity = IF_ENTITY;
	EntityList[Temp].tei_instance = Temp - EntityCount;
    }
    EntityMax = Count;
    
    /* Return entity list */
    Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)EntityList, Size);
    
    KeReleaseSpinLock(&EntityListLock, OldIrql);
    
    *BufferSize = Size;

    return TDI_SUCCESS;
}

TDI_STATUS InfoTdiQueryGetInterfaceMIB(TDIObjectID *ID,
				       PNDIS_BUFFER Buffer,
				       UINT BufSize,
				       PUINT BufferSize) {
    PIFENTRY OutData;
    UINT ListedIfIndex, Count, Size;
    PLIST_ENTRY CurrentADEEntry;
    PADDRESS_ENTRY CurrentADE;
    PLIST_ENTRY CurrentIFEntry;
    PIP_INTERFACE CurrentIF;
    PCHAR IFDescr;
    KIRQL OldIrql;

    OutData = ExAllocatePool( NonPagedPool, 
			      sizeof(IFENTRY) + MAX_IFDESCR_LEN );

    if( !OutData ) return STATUS_NO_MEMORY;

    RtlZeroMemory( OutData,sizeof(IFENTRY) + MAX_IFDESCR_LEN );

    KeAcquireSpinLock(&EntityListLock, &OldIrql);
    ListedIfIndex = ID->toi_entity.tei_instance - EntityCount;
    if( ListedIfIndex > EntityMax ) {
	KeReleaseSpinLock(&EntityListLock,OldIrql);
	return TDI_INVALID_REQUEST;
    }
    
    CurrentIFEntry = InterfaceListHead.Flink;
    
    for( Count = 0; Count < ListedIfIndex; Count++ )
	CurrentIFEntry = CurrentIFEntry->Flink;
    
    CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);

    CurrentADEEntry = CurrentIF->ADEListHead.Flink;
    if( CurrentADEEntry == &CurrentIF->ADEListHead ) {
	KeReleaseSpinLock( &EntityListLock, OldIrql );
	return TDI_INVALID_REQUEST;
    }

    CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
    
    OutData->Index = Count + 1; /* XXX - arty What goes here?? */
    OutData->Type = CurrentADE->Type;
    OutData->Mtu = CurrentIF->MTU;
    OutData->Speed = 10000000; /* XXX - arty Not sure */
    memcpy(OutData->PhysAddr,
	   CurrentIF->Address,CurrentIF->AddressLength);
    OutData->PhysAddrLen = CurrentIF->AddressLength;
    OutData->AdminStatus = TRUE;
    OutData->OperStatus = TRUE;
    IFDescr = (PCHAR)&OutData[1];
    strcpy(IFDescr,"ethernet adapter");
    OutData->DescrLen = strlen(IFDescr);
    IFDescr = IFDescr + strlen(IFDescr);
    Size = IFDescr - (PCHAR)OutData;

    KeReleaseSpinLock(&InterfaceListLock, OldIrql);

    *BufferSize = Size;

    if( BufSize < Size ) {
	return TDI_BUFFER_TOO_SMALL;
    } else {
	CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&OutData, Size);
	return TDI_SUCCESS;
    }
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
  PLIST_ENTRY CurrentIFEntry;
  PLIST_ENTRY CurrentADEEntry;
  PLIST_ENTRY CurrentADFEntry;
  PADDRESS_FILE CurrentADF;
  PADDRESS_ENTRY CurrentADE;
  PIP_INTERFACE CurrentIF;
  ADDRESS_INFO Info;
  KIRQL OldIrql;
  UINT BufSize;
  UINT Entity;
  UINT Offset;
  ULONG Temp;
  UINT Count;
  UINT Size;
  TDIEntityID EntityId;

  Offset = 0;
  BufSize = *BufferSize;

  /* Check wether it is a query for a list of entities */
  Entity = ID->toi_entity.tei_entity;
  if (Entity == GENERIC_ENTITY)
    {
      if ((ID->toi_class  != INFO_CLASS_GENERIC) ||
          (ID->toi_type != INFO_TYPE_PROVIDER) ||
          (ID->toi_id != ENTITY_LIST_ID))
        {
          return TDI_INVALID_PARAMETER;
        }

      return InfoTdiQueryListEntities(Buffer, BufSize, BufferSize);
    }

  /* Get an IFENTRY */
  if (ID->toi_class == INFO_CLASS_PROTOCOL && 
      ID->toi_type  == INFO_TYPE_PROVIDER &&
      ID->toi_id    == IF_MIB_STATS_ID) 
    {
      if(ID->toi_entity.tei_entity != IF_ENTITY)
	return TDI_INVALID_REQUEST;

      return InfoTdiQueryGetInterfaceMIB(ID, Buffer, BufSize, BufferSize);
    }

    if ((Entity != CL_TL_ENTITY) && (Entity != CO_TL_ENTITY))
      {
        /* We can't handle this entity, pass it on */
        return IPTdiQueryInformationEx(
          Request, ID, Buffer, BufferSize, Context);
      }

    /* Make return parameters consistent every time */
    *BufferSize = 0;

    if (ID->toi_entity.tei_instance != TL_INSTANCE)
      {
        /* We only support a single instance */
        return TDI_INVALID_REQUEST;
      }

    if (ID->toi_class == INFO_CLASS_GENERIC)
      {
        if ((ID->toi_type != INFO_TYPE_PROVIDER) ||
            (ID->toi_id != ENTITY_TYPE_ID))
            return TDI_INVALID_PARAMETER;

        if (BufSize < sizeof(ULONG))
          {
            return TDI_BUFFER_TOO_SMALL;
          }

        if (Entity == CL_TL_ENTITY)
          {
            Temp = CL_TL_UDP;
          }
        else if (Entity == CO_TL_ENTITY)
          {
            Temp = CO_TL_TCP;
          }
        else
          {
            return TDI_INVALID_PARAMETER;
          }

        Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&Temp, sizeof(ULONG));

        return TDI_SUCCESS;
    }

  if (ID->toi_class == INFO_CLASS_PROTOCOL)
    {
      if (ID->toi_type != INFO_TYPE_PROVIDER)
        {
          return TDI_INVALID_PARAMETER;
        }

      switch (ID->toi_id)
        {
          case UDP_MIB_STAT_ID:
            if (Entity != CL_TL_ENTITY)
              {
                return TDI_INVALID_PARAMETER;
              }

            if (BufSize < sizeof(UDPStats))
              {
                return TDI_BUFFER_TOO_SMALL;
              }

            Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&UDPStats, sizeof(UDP_STATISTICS));

            return TDI_SUCCESS;

          case UDP_MIB_TABLE_ID:
            if (Entity != CL_TL_ENTITY)
            {
                return TDI_INVALID_PARAMETER;
            }

            Offset = 0;

            KeAcquireSpinLock(&AddressFileListLock, &OldIrql);

            CurrentADFEntry = AddressFileListHead.Flink;
            while (CurrentADFEntry != &AddressFileListHead)
              {
	              CurrentADF = CONTAINING_RECORD(CurrentADFEntry, ADDRESS_FILE, ListEntry);

                if (Offset + sizeof(ADDRESS_INFO) > BufSize)
                  {
                    KeReleaseSpinLock(&AddressFileListLock, OldIrql);
                    *BufferSize = Offset;
                    return TDI_BUFFER_OVERFLOW;
                  }

                Info.LocalAddress = CurrentADF->ADE->Address->Address.IPv4Address;
                Info.LocalPort = CurrentADF->Port;

                Count = CopyBufferToBufferChain(Buffer, Offset, (PUCHAR)&Info, sizeof(ADDRESS_INFO));
                Offset += Count;

                CurrentADFEntry = CurrentADFEntry->Flink;
              }

            KeReleaseSpinLock(&AddressFileListLock, OldIrql);

            *BufferSize = Offset;

            return STATUS_SUCCESS;

          default:
            /* We can't handle this ID */
            return TDI_INVALID_PARAMETER;
        }
    }

    return TDI_INVALID_PARAMETER;
}

TDI_STATUS InfoTdiSetInformationEx(
  PTDI_REQUEST Request,
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
	if( ID->toi_entity.tei_entity == CL_NL_ENTITY &&
	    ID->toi_entity.tei_instance == TL_INSTANCE &&
	    BufferSize >= sizeof(IPROUTE_ENTRY) ) {
	    /* Add route -- buffer is an IPRouteEntry */
	    PIPROUTE_ENTRY ire = (PIPROUTE_ENTRY)Buffer;
	  RouteFriendlyAddRoute( ire );				 
	} else {
	    return TDI_INVALID_PARAMETER; 
	    /* In my experience, we are being over
	       protective compared to windows */
	}
	break;
      }
      break;
    }
    break;
  }

  return TDI_INVALID_PARAMETER;
}
    
