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

#include "iphlpapi_private.h"
#include "ifenum.h"

/* Globals */
const PWCHAR TcpFileName = L"\\Device\\Tcp";

/* Functions */

/* I'm a bit skittish about maintaining this info in memory, as I'd rather
 * not add any mutex or critical section blockers to these functions.  I've
 * encountered far too many windows functions that contribute to deadlock
 * by not announcing themselves. */
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

    status = ZwCreateFile( tcpFile,
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
        ASSERT( !entitySet ); /* We must not have an entity set allocated */
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
( HANDLE tcpFile, TDIEntityID *ent, IFEntrySafelySized *entry ) {
    TCP_REQUEST_QUERY_INFORMATION_EX req = TCP_REQUEST_QUERY_INFORMATION_INIT;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD returnSize;

    DPRINT("TdiGetMibForIfEntity(tcpFile %x,entityId %x)\n",
           (int)tcpFile, (int)ent->tei_instance);

    req.ID.toi_class                = INFO_CLASS_PROTOCOL;
    req.ID.toi_type                 = INFO_TYPE_PROVIDER;
    req.ID.toi_id                   = IF_MIB_STATS_ID;
    req.ID.toi_entity               = *ent;

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
           "  if_physaddrlen ................. %d\n",
           entry->ent.if_index,
           entry->ent.if_type,
           entry->ent.if_mtu,
           entry->ent.if_speed,
           entry->ent.if_physaddrlen);
    DPRINT("  if_physaddr .................... %02x:%02x:%02x:%02x:%02x:%02x\n",
           "  if_descr ....................... %s\n",
           entry->ent.if_physaddr[0] & 0xff,
           entry->ent.if_physaddr[1] & 0xff,
           entry->ent.if_physaddr[2] & 0xff,
           entry->ent.if_physaddr[3] & 0xff,
           entry->ent.if_physaddr[4] & 0xff,
           entry->ent.if_physaddr[5] & 0xff,
           entry->ent.if_descr);
    DPRINT("} status %08x\n",status);
        
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
                          loop_maybe,
                          &entryInfo );

    return !entryInfo.ent.if_type || 
        entryInfo.ent.if_type == IFENT_SOFTWARE_LOOPBACK;
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

static NTSTATUS getInterfaceInfoSet( HANDLE tcpFile, 
                                     IFInfo **infoSet,
                                     PDWORD numInterfaces ) {
    DWORD numEntities;
    TDIEntityID *entIDSet = 0;
    NTSTATUS status = tdiGetEntityIDSet( tcpFile, &entIDSet, &numEntities );
    IFInfo *infoSetInt = 0;
    BOOL interfaceInfoComplete;
    int curInterf = 0, i;

    if( NT_SUCCESS(status) )
        infoSetInt = HeapAlloc( GetProcessHeap(), 0, 
                                sizeof(IFInfo) * numEntities );
    
    if( infoSetInt ) {
        for( i = 0; i < numEntities; i++ ) {
            if( isInterface( &entIDSet[i] ) ) {
                status = tdiGetMibForIfEntity
                    ( tcpFile,
                      &entIDSet[i],
                      &infoSetInt[curInterf].if_info );
		DPRINT("tdiGetMibForIfEntity: %08x\n", status);
                if( NT_SUCCESS(status) ) {
                    DWORD numAddrs;
                    IPAddrEntry *addrs;
                    TDIEntityID ip_ent;
                    int j,k;

                    interfaceInfoComplete = FALSE;
		    status = getNthIpEntity( tcpFile, 0, &ip_ent );
		    if( NT_SUCCESS(status) )
			status = tdiGetIpAddrsForIpEntity
			    ( tcpFile, &ip_ent, &addrs, &numAddrs );
		    for( k = 0; k < numAddrs && NT_SUCCESS(status); k++ ) {
			DPRINT("ADDR %d: index %d (target %d)\n", k, addrs[k].iae_index, infoSetInt[curInterf].if_info.ent.if_index);
			if( addrs[k].iae_index == 
			    infoSetInt[curInterf].if_info.ent.if_index ) {
			    memcpy( &infoSetInt[curInterf].ip_addr,
				    &addrs[k],
				    sizeof( addrs[k] ) );
			    curInterf++;
			    break;
			}
		    }
                }
            }
        }
    }

    *infoSet = infoSetInt;
    *numInterfaces = curInterf;

    return STATUS_SUCCESS;
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

NTSTATUS getInterfaceInfoByIndex( HANDLE tcpFile, DWORD index, IFInfo *info ) {
    IFInfo *ifInfo;
    DWORD numInterfaces;
    NTSTATUS status = getInterfaceInfoSet( tcpFile, &ifInfo, &numInterfaces );
    int i;
    
    if( NT_SUCCESS(status) )
        for( i = 0; i < numInterfaces; i++ ) {
            if( ifInfo[i].if_info.ent.if_index == index ) {
                memcpy( info, &ifInfo[i], sizeof(*info) );
                break;
            }
        }

    if( NT_SUCCESS(status) )
        return i < numInterfaces ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    else
        return status;
}
    
NTSTATUS getInterfaceInfoByName( HANDLE tcpFile, char *name, IFInfo *info ) {
    IFInfo *ifInfo;
    DWORD numInterfaces;
    int i;    
    NTSTATUS status = getInterfaceInfoSet( tcpFile, &ifInfo, &numInterfaces );
    
    if( NT_SUCCESS(status) )
        for( i = 0; i < numInterfaces; i++ ) {
            if( !strcmp(ifInfo[i].if_info.ent.if_descr, name) ) {
                memcpy( info, &ifInfo[i], sizeof(*info) );
                break;
            }
        }
    
    if( NT_SUCCESS(status) )
        return i < numInterfaces ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    else
        return status;
}
    
/* Note that the result of this operation must be freed later */

const char *getInterfaceNameByIndex(DWORD index)
{
    IFInfo ifInfo;
    HANDLE tcpFile;
    char *interfaceName = 0, *adapter_name = 0;
    NTSTATUS status = openTcpFile( &tcpFile );

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByIndex( tcpFile, index, &ifInfo );
        
        if( NT_SUCCESS(status) ) {
            adapter_name = ifInfo.if_info.ent.if_descr;
            
            interfaceName = HeapAlloc( GetProcessHeap(), 0, 
                                       strlen(adapter_name) + 1 );
            strcpy( interfaceName, adapter_name );
            
            closeTcpFile( tcpFile );
        }
    }

    return interfaceName;
}

void consumeInterfaceName(const char *name) {
    HeapFree( GetProcessHeap(), 0, (char *)name );
}

DWORD getInterfaceIndexByName(const char *name, PDWORD index)
{
    IFInfo ifInfo;
    HANDLE tcpFile;
    NTSTATUS status = openTcpFile( &tcpFile );

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByName( tcpFile, (char *)name, &ifInfo );
        
        if( NT_SUCCESS(status) ) {
            *index = ifInfo.if_info.ent.if_index;
            closeTcpFile( tcpFile );
        }
    }

    return status;
}

InterfaceIndexTable *getInterfaceIndexTableInt( BOOL nonLoopbackOnly ) {
  DWORD numInterfaces, curInterface = 0;
  int i;
  IFInfo *ifInfo;
  InterfaceIndexTable *ret = 0;
  HANDLE tcpFile;
  NTSTATUS status = openTcpFile( &tcpFile );

  if( NT_SUCCESS(status) ) {
      status = getInterfaceInfoSet( tcpFile, &ifInfo, &numInterfaces );

      DPRINT("InterfaceInfoSet: %08x, %04x:%08x\n", 
	     status, 
	     ifInfo->entity_id.tei_entity,
	     ifInfo->entity_id.tei_instance);

      if( NT_SUCCESS(status) ) {
          ret = (InterfaceIndexTable *)
              calloc(1,
                     sizeof(InterfaceIndexTable) + 
                     (numInterfaces - 1) * sizeof(DWORD));
          
          if (ret) {
              ret->numAllocated = numInterfaces;
	      DPRINT("NumInterfaces = %d\n", numInterfaces);
          
              for( i = 0; i < numInterfaces; i++ ) {
		  DPRINT("Examining interface %d\n", i);
                  if( !nonLoopbackOnly || 
                      !isLoopback( tcpFile, &ifInfo[i].entity_id ) ) {
		      DPRINT("Interface %d matches (%d)\n", i, curInterface);
                      ret->indexes[curInterface++] = 
                          ifInfo[i].if_info.ent.if_index;
                  }
              } 

              ret->numIndexes = curInterface;
          }
          
          tdiFreeThingSet( ifInfo );
      }
      closeTcpFile( tcpFile );
  }
              
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

NTSTATUS getIPAddrEntryForIf(HANDLE tcpFile, 
                             char *name,
                             DWORD index,
                             IFInfo *ifInfo) {
    NTSTATUS status = 
        name ? 
        getInterfaceInfoByName( tcpFile, name, ifInfo ) :
        getInterfaceInfoByIndex( tcpFile, index, ifInfo );
    return status;
}

DWORD getAddrByIndexOrName( char *name, DWORD index, IPHLPAddrType addrType ) {
    IFInfo ifInfo;
    HANDLE tcpFile = INVALID_HANDLE_VALUE;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD addrOut = INADDR_ANY;

    status = openTcpFile( &tcpFile );

    if( NT_SUCCESS(status) ) {
        status = getIPAddrEntryForIf( tcpFile, name, index, &ifInfo );
        if( NT_SUCCESS(status) ) {
            switch( addrType ) {
            case IPAAddr:  addrOut = ifInfo.ip_addr.iae_addr; break;
            case IPABcast: addrOut = ifInfo.ip_addr.iae_bcastaddr; break;
            case IPAMask:  addrOut = ifInfo.ip_addr.iae_mask; break;
            case IFMtu:    addrOut = ifInfo.if_info.ent.if_mtu; break;
            case IFStatus: addrOut = ifInfo.if_info.ent.if_operstatus; break;
            }
        }
        closeTcpFile( &tcpFile );
    }

    return addrOut;
}                           

DWORD getInterfaceIPAddrByIndex(DWORD index) {
    return getAddrByIndexOrName( 0, index, IPAAddr );
}

DWORD getInterfaceBCastAddrByName(const char *name) {
    return getAddrByIndexOrName( (char *)name, 0, IPABcast );
}

DWORD getInterfaceBCastAddrByIndex(DWORD index) {
    return getAddrByIndexOrName( 0, index, IPABcast );
}

DWORD getInterfaceMaskByName(const char *name) {
    return getAddrByIndexOrName( (char *)name, 0, IPAMask );
}

DWORD getInterfaceMaskByIndex(DWORD index) {
    return getAddrByIndexOrName( 0, index, IPAMask );
}

void getInterfacePhysicalFromInfo( IFInfo *info, 
                                   PDWORD len, PBYTE addr, PDWORD type ) {
    *len = info->if_info.ent.if_physaddrlen;
    memcpy( addr, info->if_info.ent.if_physaddr, *len );
    *type = info->if_info.ent.if_type;
}

DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
                                 PDWORD type)
{
    HANDLE tcpFile;
    IFInfo info;
    NTSTATUS status = openTcpFile( &tcpFile );

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByName( tcpFile, (char *)name, &info );
        if( NT_SUCCESS(status) ) 
            getInterfacePhysicalFromInfo( &info, len, addr, type );
        closeTcpFile( tcpFile );
    }

    return status;
}

DWORD getInterfacePhysicalByIndex(DWORD index, PDWORD len, PBYTE addr,
 PDWORD type)
{
    HANDLE tcpFile;
    IFInfo info;
    NTSTATUS status = openTcpFile( &tcpFile );

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByIndex( tcpFile, index, &info );
        if( NT_SUCCESS(status) ) 
            getInterfacePhysicalFromInfo( &info, len, addr, type );
        closeTcpFile( tcpFile );
    }

    return status;
}

DWORD getInterfaceMtuByName(const char *name, PDWORD mtu) {
    *mtu = getAddrByIndexOrName( (char *)name, 0, IFMtu );
    return STATUS_SUCCESS;
}

DWORD getInterfaceMtuByIndex(DWORD index, PDWORD mtu) {
    *mtu = getAddrByIndexOrName( 0, index, IFMtu );
    return STATUS_SUCCESS;
}

DWORD getInterfaceStatusByName(const char *name, PDWORD status) {
    *status = getAddrByIndexOrName( (char *)name, 0, IFStatus );
    return STATUS_SUCCESS;
}

DWORD getInterfaceStatusByIndex(DWORD index, PDWORD status)
{
    *status = getAddrByIndexOrName( 0, index, IFStatus );
    return STATUS_SUCCESS;
}

DWORD getInterfaceEntryByName(const char *name, PMIB_IFROW entry)
{
    HANDLE tcpFile;
    IFInfo info;
    NTSTATUS status = openTcpFile( &tcpFile );

    DPRINT("Called.\n");

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByName( tcpFile, (char *)name, &info );
        
        if( NT_SUCCESS(status) ) {
            memcpy( &entry->wszName[MAX_INTERFACE_NAME_LEN],
                    &info.if_info,
                    sizeof(info.if_info) );
        }
        
        closeTcpFile( tcpFile );
    }

    return status;
}

DWORD getInterfaceEntryByIndex(DWORD index, PMIB_IFROW entry)
{
    HANDLE tcpFile;
    IFInfo info;
    NTSTATUS status = openTcpFile( &tcpFile );

    DPRINT("Called.\n");

    if( NT_SUCCESS(status) ) {
        status = getInterfaceInfoByIndex( tcpFile, index, &info );
        
        if( NT_SUCCESS(status) ) {
            memcpy( &entry->wszName[MAX_INTERFACE_NAME_LEN],
                    &info.if_info,
                    sizeof(info.if_info) );
        }
        
        closeTcpFile( tcpFile );
    }

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
