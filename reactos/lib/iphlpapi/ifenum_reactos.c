/* Copyright (C) 2003 Art Yerkes
 * A reimplementation of ifenum.c by Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Implementation notes
 * - Our bretheren use IOCTL_TCP_QUERY_INFORMATION_EX to get information
 *   from tcpip.sys and IOCTL_TCP_SET_INFORMATION_EX to set info (such as
 *   the route table).  These ioctls mirror WsControl exactly in usage.
 * - This iphlpapi does not rely on any reactos-only features.
 * - This implementation is meant to be largely correct.  I am not, however,
 *   paying any attention to performance.  It can be done faster, and
 *   someone should definately optimize this code when speed is more of a 
 *   priority than it is now.
 * 
 * Edited implementation notes from the original -- Basically edited to add
 *   information and prune things which are not accurate about this file.
 * Interface index fun:
 * - Windows may rely on an index being cleared in the topmost 8 bits in some
 *   APIs; see GetFriendlyIfIndex and the mention of "backward compatible"
 *   indexes.  It isn't clear which APIs might fail with non-backward-compatible
 *   indexes, but I'll keep them bits clear just in case.
 * FIXME:
 * - We don't support IPv6 addresses here yet -- I moved the upper edge
 *   functions into iphlpv6.c (arty)
 */

#include "ipprivate.h"
#include "ifenum.h"

/* Globals */
const PWCHAR TcpFileName = L"\\Device\\Tcp";

/* Functions */

void interfaceMapInit(void)
{
    /* For now, nothing */
}

void interfaceMapFree(void)
{
    /* Ditto. */
}

NTSTATUS openTcpFile(PHANDLE tcpFile) {
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    TRACE("called.\n");

    /* Shamelessly ripped from CreateFileW */
    RtlInitUnicodeString( &fileName, TcpFileName );

    InitializeObjectAttributes( &objectAttributes,
				&fileName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL );

    status = NtCreateFile( tcpFile,
			   SYNCHRONIZE | GENERIC_EXECUTE,
			   &objectAttributes,
			   &ioStatusBlock,
			   NULL,
			   FILE_ATTRIBUTE_NORMAL,
			   FILE_SHARE_READ | FILE_SHARE_WRITE,
			   FILE_OPEN_IF,
			   FILE_SYNCHRONOUS_IO_NONALERT,
			   0,
			   0 );

    /* String does not need to be freed: it points to the constant
     * string we provided */

    TRACE("returning %08x\n", (int)status);

    return status;
}

void closeTcpFile( HANDLE h ) {
    TRACE("called.\n");
    NtClose( h );
}

/* A generic thing-getting function which interacts in the right way with
 * TDI.  This may seem oblique, but I'm using it to reduce code and hopefully
 * make this thing easier to debug.
 *
 * The things returned can be any of:
 *   TDIEntityID
 *   TDIObjectID
 *   IFEntry
 *   IPSNMPInfo
 *   IPAddrEntry
 *   IPInterfaceInfo
 */
NTSTATUS tdiGetSetOfThings( HANDLE tcpFile, 
			    DWORD toiClass,
			    DWORD toiType,
			    DWORD toiId,
			    DWORD teiEntity,
			    DWORD fixedPart,
			    DWORD entrySize,
			    PVOID *tdiEntitySet,
			    PDWORD numEntries ) {
    TCP_REQUEST_QUERY_INFORMATION_EX req = TCP_REQUEST_QUERY_INFORMATION_INIT;
    PVOID entitySet = 0;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD allocationSizeForEntityArray = entrySize * MAX_TDI_ENTITIES, 
	arraySize = entrySize * MAX_TDI_ENTITIES;

    DPRINT("TdiGetSetOfThings(tcpFile %x,toiClass %x,toiType %x,toiId %x,"
	   "teiEntity %x,fixedPart %d,entrySize %d)\n",
	   (int)tcpFile, 
	   (int)toiClass, 
	   (int)toiType, 
	   (int)toiId, 
	   (int)teiEntity,
	   (int)fixedPart, 
	   (int)entrySize );

    req.ID.toi_class                = toiClass;
    req.ID.toi_type                 = toiType;
    req.ID.toi_id                   = toiId;
    req.ID.toi_entity.tei_entity    = teiEntity;

    /* There's a subtle problem here...
     * If an interface is added at this exact instant, (as if by a PCMCIA
     * card insertion), the array will still not have enough entries after
     * have allocated it after the first DeviceIoControl call.
     *
     * We'll get around this by repeating until the number of interfaces
     * stabilizes.
     */
    do {
	assert( !entitySet ); /* We must not have an entity set allocated */
	status = DeviceIoControl( tcpFile,
				  IOCTL_TCP_QUERY_INFORMATION_EX,
				  &req,
				  sizeof(req),
				  0,
				  0,
				  &allocationSizeForEntityArray,
				  NULL );
	
	if( !NT_SUCCESS(status) ) {
	    DPRINT("TdiGetSetOfThings() => %08x\n", (int)status);	
	    return status;
	}
	
	arraySize = allocationSizeForEntityArray;
	entitySet = HeapAlloc( GetProcessHeap(), 0, arraySize );
					      
	if( !entitySet ) {
	    status = STATUS_INSUFFICIENT_RESOURCES;
	    DPRINT("TdiGetSetOfThings() => %08x\n", (int)status);
	    return status;
	}

	status = DeviceIoControl( tcpFile,
				  IOCTL_TCP_QUERY_INFORMATION_EX,
				  &req,
				  sizeof(req),
				  entitySet,
				  arraySize,
				  &allocationSizeForEntityArray,
				  NULL );
	
	/* This is why we have the loop -- we might have added an adapter */
	if( arraySize == allocationSizeForEntityArray )
	    break;

	HeapFree( GetProcessHeap(), 0, entitySet );
	entitySet = 0;

	if( !NT_SUCCESS(status) ) {
	    DPRINT("TdiGetSetOfThings() => %08x\n", (int)status);
	    return status;
	}

	DPRINT("TdiGetSetOfThings(): Array changed size: %d -> %d.\n",
	       arraySize, allocationSizeForEntityArray );
    } while( TRUE ); /* We break if the array we received was the size we 
		      * expected.  Therefore, we got here because it wasn't */
    
    *numEntries = (arraySize - fixedPart) / entrySize;
    *tdiEntitySet = entitySet;

    DPRINT("TdiGetSetOfThings() => Success: %d things @ %08x\n", 
	   (int)*numEntries, (int)entitySet);

    return STATUS_SUCCESS;
}

VOID tdiFreeThingSet( PVOID things ) {
    HeapFree( GetProcessHeap(), 0, things );
}

NTSTATUS tdiGetMibForIfEntity
( HANDLE tcpFile, DWORD entityId, IFEntrySafelySized *entry ) {
    TCP_REQUEST_QUERY_INFORMATION_EX req = TCP_REQUEST_QUERY_INFORMATION_INIT;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD returnSize;

    DPRINT("TdiGetMibForIfEntity(tcpFile %x,entityId %x)\n",
	   (int)tcpFile, (int)entityId);

    req.ID.toi_class                = INFO_CLASS_PROTOCOL;
    req.ID.toi_type                 = INFO_TYPE_PROVIDER;
    req.ID.toi_id                   = IF_MIB_STATS_ID;
    req.ID.toi_entity.tei_entity    = IF_ENTITY;
    req.ID.toi_entity.tei_instance  = entityId;

    status = DeviceIoControl( tcpFile,
			      IOCTL_TCP_QUERY_INFORMATION_EX,
			      &req,
			      sizeof(req),
			      entry,
			      sizeof(*entry),
			      &returnSize,
			      NULL );

    if( !NT_SUCCESS(status) ) {
	TRACE("failure: %08x\n", status);
	return status;
    } else TRACE("Success.\n");

    DPRINT("TdiGetMibForIfEntity() => {\n"
	   "  if_index ....................... %x\n"
	   "  if_type ........................ %x\n"
	   "  if_mtu ......................... %d\n"
	   "  if_speed ....................... %x\n"
	   "  if_physaddrlen ................. %d\n"
	   "  if_physaddr .................... %02x:%02x:%02x:%02x:%02x:%02x\n",
	   "  if_descr ....................... %s\n"
	   "} status %08x\n",
	   (int)entry->offset.ent.if_index,
	   (int)entry->offset.ent.if_type,
	   (int)entry->offset.ent.if_mtu,
	   (int)entry->offset.ent.if_speed,
	   (int)entry->offset.ent.if_physaddrlen,
	   entry->offset.ent.if_physaddr[0] & 0xff,
	   entry->offset.ent.if_physaddr[1] & 0xff,
	   entry->offset.ent.if_physaddr[2] & 0xff,
	   entry->offset.ent.if_physaddr[3] & 0xff,
	   entry->offset.ent.if_physaddr[4] & 0xff,
	   entry->offset.ent.if_physaddr[5] & 0xff,
	   entry->offset.ent.if_descr,
	   (int)status);
	
    return status;    
}

NTSTATUS tdiGetEntityIDSet( HANDLE tcpFile,
			    TDIEntityID **entitySet, 
			    PDWORD numEntities ) {
    NTSTATUS status = tdiGetSetOfThings( tcpFile,
					 INFO_CLASS_GENERIC,
					 INFO_TYPE_PROVIDER,
					 ENTITY_LIST_ID,
					 GENERIC_ENTITY,
					 0,
					 sizeof(TDIEntityID),
					 (PVOID *)entitySet,
					 numEntities );
    if( NT_SUCCESS(status) ) {
	int i;

	for( i = 0; i < *numEntities; i++ ) {
	    DPRINT("%-4d: %04x:%08x\n",
		   i,
		   (*entitySet)[i].tei_entity, 
		   (*entitySet)[i].tei_instance );
	}
    }
    
    return status;
}

static BOOL isInterface( TDIEntityID *if_maybe ) {
    return 
	if_maybe->tei_entity == IF_ENTITY;
}

static BOOL isLoopback( HANDLE tcpFile, TDIEntityID *loop_maybe ) {
    IFEntrySafelySized entryInfo;

    tdiGetMibForIfEntity( tcpFile, 
			  loop_maybe->tei_instance,
			  &entryInfo );

    return !entryInfo.offset.ent.if_type || 
	entryInfo.offset.ent.if_type == IFENT_SOFTWARE_LOOPBACK;
}

NTSTATUS tdiGetEntityType( HANDLE tcpFile, TDIEntityID *ent, PULONG type ) {
    TCP_REQUEST_QUERY_INFORMATION_EX req = TCP_REQUEST_QUERY_INFORMATION_INIT;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD returnSize;

    DPRINT("TdiGetEntityType(tcpFile %x,entityId %x)\n",
	   (DWORD)tcpFile, ent->tei_instance);

    req.ID.toi_class                = INFO_CLASS_GENERIC;
    req.ID.toi_type                 = INFO_TYPE_PROVIDER;
    req.ID.toi_id                   = ENTITY_TYPE_ID;
    req.ID.toi_entity.tei_entity    = ent->tei_entity;
    req.ID.toi_entity.tei_instance  = ent->tei_instance;

    status = DeviceIoControl( tcpFile,
			      IOCTL_TCP_QUERY_INFORMATION_EX,
			      &req,
			      sizeof(req),
			      type,
			      sizeof(*type),
			      &returnSize,
			      NULL );

    DPRINT("TdiGetEntityType() => %08x %08x\n", *type, status);

    return status;
}

static DWORD getNumInterfacesInt(BOOL onlyLoopback)
{
    DWORD numEntities, numInterfaces = 0;
    TDIEntityID *entitySet;
    HANDLE tcpFile;
    NTSTATUS status;
    int i;

    status = openTcpFile( &tcpFile );

    if( !NT_SUCCESS(status) ) {
	DPRINT("getNumInterfaces: failed %08x\n", status );
	return 0;
    }

    status = tdiGetEntityIDSet( tcpFile, &entitySet, &numEntities );

    if( !NT_SUCCESS(status) ) {
	DPRINT("getNumInterfaces: failed %08x\n", status );
	return 0;
    }

    closeTcpFile( tcpFile );

    for( i = 0; i < numEntities; i++ ) {
	if( isInterface( &entitySet[i] ) &&
	    (!onlyLoopback || isLoopback( tcpFile, &entitySet[i] )) )
	    numInterfaces++;
    }

    DPRINT("getNumInterfaces: success: %d %d %08x\n", 
	   onlyLoopback, numInterfaces, status );

    tdiFreeThingSet( entitySet );
    
    return numInterfaces;
}

DWORD getNumInterfaces(void)
{
    return getNumInterfacesInt( FALSE );
}

DWORD getNumNonLoopbackInterfaces(void)
{
    return getNumInterfacesInt( TRUE );
}

DWORD getNthInterfaceEntity( HANDLE tcpFile, DWORD index, TDIEntityID *ent ) {
    DWORD numEntities = 0;
    DWORD numInterfaces = 0;
    TDIEntityID *entitySet = 0;
    NTSTATUS status = tdiGetEntityIDSet( tcpFile, &entitySet, &numEntities );
    int i;

    if( !NT_SUCCESS(status) )
	return status;

    for( i = 0; i < numEntities; i++ ) {
	if( isInterface( &entitySet[i] ) ) {
	    if( numInterfaces == index ) break;
	    else numInterfaces++;
	}
    }

    DPRINT("Index %d is entity #%d - %04x:%08x\n", index, i, 
	   entitySet[i].tei_entity, entitySet[i].tei_instance );

    if( numInterfaces == index && i < numEntities ) {
	memcpy( ent, &entitySet[i], sizeof(*ent) );
	tdiFreeThingSet( entitySet );
	return STATUS_SUCCESS;
    } else {
	tdiFreeThingSet( entitySet );
	return STATUS_UNSUCCESSFUL;
    }
}
    
/* Note that the result of this operation must be freed later */

const char *getInterfaceNameByIndex(DWORD index)
{
    TDIEntityID ent;
    IFEntrySafelySized entityInfo;
    HANDLE tcpFile = INVALID_HANDLE_VALUE;
    NTSTATUS status = STATUS_SUCCESS;
    PCHAR interfaceName = 0;
    char simple_name_buf[100];
    char *adapter_name;

    status = openTcpFile( &tcpFile );
    if( !NT_SUCCESS(status) ) {
	DPRINT("failed %08x\n", status );
	return 0;
    }

    status = getNthInterfaceEntity( tcpFile, index, &ent );

    if( !NT_SUCCESS(status) ) {
	DPRINT("failed %08x\n", status );
	return 0;
    }

    status = tdiGetMibForIfEntity( tcpFile, 
				   ent.tei_instance,
				   &entityInfo );
    if( NT_SUCCESS(status) ) {
	adapter_name = entityInfo.offset.ent.if_descr; 
    } else {
	sprintf( simple_name_buf, "eth%x", 
		 (int)ent.tei_instance );
	adapter_name = simple_name_buf;
    }
    interfaceName = HeapAlloc( GetProcessHeap(), 0, 
			       strlen(adapter_name) + 1 );
    strcpy( interfaceName, adapter_name );

    closeTcpFile( tcpFile );
    
    return interfaceName;
}

void consumeInterfaceName(const char *name) {
    HeapFree( GetProcessHeap(), 0, (char *)name );
}

DWORD getInterfaceIndexByName(const char *name, PDWORD index)
{
    DWORD ret = STATUS_SUCCESS;
    int numInterfaces = getNumInterfaces();
    const char *iname = 0;
    int i;
    HANDLE tcpFile;

    ret = openTcpFile( &tcpFile );

    if( !NT_SUCCESS(ret) ) {
	DPRINT("Failure: %08x\n", ret);
	return ret;
    }

    for( i = 0; i < numInterfaces; i++ ) {
	iname = getInterfaceNameByIndex( i );
	if( !strcmp(iname, name) ) {
	    *index = i;
	}
	HeapFree( GetProcessHeap(), 0, (char *)iname );
    }

    closeTcpFile( tcpFile );

    return ret;
}

InterfaceIndexTable *getInterfaceIndexTableInt( BOOL nonLoopbackOnly ) {
  HANDLE tcpFile;
  DWORD numInterfaces, curInterface = 0;
  int i;
  InterfaceIndexTable *ret;
  TDIEntityID *entitySet;
  DWORD numEntities;
  NTSTATUS status;

  numInterfaces = getNumInterfaces();
  TRACE("getInterfaceIndexTable: numInterfaces: %d\n", numInterfaces);
  ret = (InterfaceIndexTable *)calloc(1,
				      sizeof(InterfaceIndexTable) + (numInterfaces - 1) * sizeof(DWORD));
  if (ret) {
      ret->numAllocated = numInterfaces;
  }
  
  status = openTcpFile( &tcpFile );
  tdiGetEntityIDSet( tcpFile, &entitySet, &numEntities );
  
  for( i = 0; i < numEntities; i++ ) {
      if( isInterface( &entitySet[i] ) &&
	  (!nonLoopbackOnly || !isLoopback( tcpFile, &entitySet[i] )) ) {
	  ret->indexes[curInterface++] = entitySet[i].tei_instance;
      }
  }
  
  tdiFreeThingSet( entitySet );
  closeTcpFile( tcpFile );
  ret->numIndexes = curInterface;
  
  return ret;
}

InterfaceIndexTable *getInterfaceIndexTable(void) {
    return getInterfaceIndexTableInt( FALSE );
}

InterfaceIndexTable *getNonLoopbackInterfaceIndexTable(void) {
    return getInterfaceIndexTableInt( TRUE );
}

DWORD getInterfaceIPAddrByName(const char *name)
{
    return INADDR_ANY;
}

DWORD getInterfaceIPAddrByIndex(DWORD index)
{
    return INADDR_ANY;
}

DWORD getInterfaceBCastAddrByName(const char *name)
{
    return INADDR_ANY;
}

DWORD getInterfaceBCastAddrByIndex(DWORD index)
{
    return INADDR_ANY;
}

DWORD getInterfaceMaskByName(const char *name)
{
  DWORD ret = INADDR_NONE;
  return ret;
}

DWORD getInterfaceMaskByIndex(DWORD index)
{
  DWORD ret = INADDR_NONE;
  return ret;
}

DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
 PDWORD type)
{
  DWORD ret;
  DWORD addrLen;

  if (!name || !len || !addr || !type)
    return ERROR_INVALID_PARAMETER;

  if (addrLen > *len) {
      ret = ERROR_INSUFFICIENT_BUFFER;
      *len = addrLen;
  }
  else {
      /* zero out remaining bytes for broken implementations */
      memset(addr + addrLen, 0, *len - addrLen);
      *len = addrLen;
      ret = NO_ERROR;
  }

  ret = ERROR_NO_MORE_FILES;
  return ret;
}

DWORD getInterfacePhysicalByIndex(DWORD index, PDWORD len, PBYTE addr,
 PDWORD type)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfacePhysicalByName(name, len, addr, type);
  else
    return ERROR_INVALID_DATA;
}

DWORD getInterfaceMtuByName(const char *name, PDWORD mtu)
{
    *mtu = 0;
    return ERROR_SUCCESS;
}

DWORD getInterfaceMtuByIndex(DWORD index, PDWORD mtu)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfaceMtuByName(name, mtu);
  else
    return ERROR_INVALID_DATA;
}

DWORD getInterfaceStatusByName(const char *name, PDWORD status)
{
  DWORD ret;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!status)
    return ERROR_INVALID_PARAMETER;

  ret = ERROR_NO_MORE_FILES;
  return ret;
}

DWORD getInterfaceStatusByIndex(DWORD index, PDWORD status)
{
  const char *name = getInterfaceNameByIndex(index);

  if (name)
    return getInterfaceStatusByName(name, status);
  else
    return ERROR_INVALID_DATA;
}

DWORD getInterfaceEntryByName(const char *name, PMIB_IFROW entry)
{
  BYTE addr[MAX_INTERFACE_PHYSADDR];
  DWORD ret, len = sizeof(addr), type;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!entry)
    return ERROR_INVALID_PARAMETER;

  if (getInterfacePhysicalByName(name, &len, addr, &type) == NO_ERROR) {
    WCHAR *assigner;
    const char *walker;

    memset(entry, 0, sizeof(MIB_IFROW));
    for (assigner = entry->wszName, walker = name; *walker; 
     walker++, assigner++)
      *assigner = *walker;
    *assigner = 0;
    getInterfaceIndexByName(name, &entry->dwIndex);
    entry->dwPhysAddrLen = len;
    memcpy(entry->bPhysAddr, addr, len);
    memset(entry->bPhysAddr + len, 0, sizeof(entry->bPhysAddr) - len);
    entry->dwType = type;
    /* FIXME: how to calculate real speed? */
    getInterfaceMtuByName(name, &entry->dwMtu);
    /* lie, there's no "administratively down" here */
    entry->dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
    getInterfaceStatusByName(name, &entry->dwOperStatus);
    /* punt on dwLastChange? */
    entry->dwDescrLen = min(strlen(name), MAX_INTERFACE_DESCRIPTION - 1);
    memcpy(entry->bDescr, name, entry->dwDescrLen);
    entry->bDescr[entry->dwDescrLen] = '\0';
    entry->dwDescrLen++;
    ret = NO_ERROR;
  }
  else
    ret = ERROR_INVALID_DATA;
  return ret;
}

DWORD getInterfaceEntryByIndex(DWORD index, PMIB_IFROW entry)
{
    HANDLE tcpFile;
    NTSTATUS status = openTcpFile( &tcpFile );
    TDIEntityID entity;

    DPRINT("Called.\n");

    if( !NT_SUCCESS(status) ) {
	DPRINT("Failed: %08x\n", status);
	return status;
    }

    status = getNthInterfaceEntity( tcpFile, index, &entity );
    
    if( !NT_SUCCESS(status) ) {
	DPRINT("Failed: %08x\n", status);
	closeTcpFile( tcpFile );
	return status;
    }

    status = tdiGetMibForIfEntity( tcpFile, 
				   entity.tei_instance,
				   (IFEntrySafelySized *)
				   &entry->wszName[MAX_INTERFACE_NAME_LEN] );
    
    closeTcpFile( tcpFile );
    return status;
}

char *toIPAddressString(unsigned int addr, char string[16])
{
  if (string) {
    struct in_addr iAddr;

    iAddr.s_addr = addr;
    /* extra-anal, just to make auditors happy */
    strncpy(string, inet_ntoa(iAddr), 16);
    string[16] = '\0';
  }
  return string;
}
