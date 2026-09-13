/*
 * PROJECT:     ReactOS Networking
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/iphlpapi/iphlpapi_reactos.c
 * PURPOSE:     DHCP helper functions for ReactOS
 * PROGRAMMERS: Pierre Schweitzer <pierre@reactos.org>
 */

#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

DWORD TCPSendIoctl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, PULONG pInBufferSize, LPVOID lpOutBuffer, PULONG pOutBufferSize)
{
    BOOL Hack = FALSE;
    HANDLE Event;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* FIXME: We don't have a global handle opened to \Device\Ip, so open one each time
     * we need. In a future, it would be cool, just to pass it to TCPSendIoctl using the first arg
     */
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        UNICODE_STRING DevName = RTL_CONSTANT_STRING(L"\\Device\\Ip");
        OBJECT_ATTRIBUTES ObjectAttributes;

        FIXME("Using the handle hack\n");
        Hack = TRUE;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &DevName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtCreateFile(&hDevice, GENERIC_EXECUTE, &ObjectAttributes,
                              &IoStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF,
                              0, NULL, 0);
        if (!NT_SUCCESS(Status))
        {
          return RtlNtStatusToDosError(Status);
        }
    }

    /* Sync event */
    Event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (Event == NULL)
    {
        /* FIXME: See upper */
        if (Hack)
        {
            CloseHandle(hDevice);
        }
        return GetLastError();
    }

    /* Reinit, and call the networking stack */
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, Event, NULL, NULL, &IoStatusBlock, dwIoControlCode, lpInBuffer, *pInBufferSize, lpOutBuffer, *pOutBufferSize);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(Event, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Close & return size info */
    CloseHandle(Event);
    *pOutBufferSize = IoStatusBlock.Information;

    /* FIXME: See upper */
    if (Hack)
    {
        CloseHandle(hDevice);
    }

    /* Return result */
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}

typedef struct _MIB_IPINTERFACE_ROW {
    ULONG IfIndex;
    ADDRESS_FAMILY Family;
    ULONG ZoneIndex;
    ULONG UseMetric;
    ULONG Speed;
    IP_ADAPTER_INFO *pAdapterInfo;
  } MIB_IPINTERFACE_ROW, *PMIB_IPINTERFACE_ROW;

typedef struct _MIB_IPINTERFACE_TABLE {
    ULONG               NumEntries;
    MIB_IPINTERFACE_ROW Table[MAX_PATH];
  } MIB_IPINTERFACE_TABLE, *PMIB_IPINTERFACE_TABLE;

DWORD WINAPI GetIpInterfaceTable(
    ADDRESS_FAMILY         Family,
    PMIB_IPINTERFACE_TABLE *Table
  )
  {
    InterfaceIndexTable *if_table;
    PMIB_IPINTERFACE_TABLE out_table;
    DWORD i, count;

    TRACE("GetIpInterfaceTable(Family %d, Table %p)\n", Family, Table);

    if (!Table)
        return ERROR_INVALID_PARAMETER;

    /* Only IPv4 is currently supported here. */
    if (Family != AF_UNSPEC && Family != AF_INET)
        return ERROR_NOT_SUPPORTED;

    if_table = getInterfaceIndexTable();
    if (!if_table)
        return ERROR_OUTOFMEMORY;

    out_table = (PMIB_IPINTERFACE_TABLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                  sizeof(MIB_IPINTERFACE_TABLE));
    if (!out_table)
    {
        free(if_table);
        return ERROR_OUTOFMEMORY;
    }

    /* Clamp to the fixed table capacity. */
    count = if_table->numIndexes;
    if (count > MAX_PATH)
        count = MAX_PATH;

    out_table->NumEntries = 0;
    for (i = 0; i < count; i++)
    {
        MIB_IFROW ifrow;
        DWORD idx = if_table->indexes[i];
        DWORD ret;

        memset(&ifrow, 0, sizeof(ifrow));
        ifrow.dwIndex = idx;
        ret = GetIfEntry(&ifrow);
        if (ret != NO_ERROR)
            continue;

        out_table->Table[out_table->NumEntries].IfIndex = idx;
        out_table->Table[out_table->NumEntries].Family = AF_INET;
        out_table->Table[out_table->NumEntries].ZoneIndex = 0;
        out_table->Table[out_table->NumEntries].UseMetric = 0;
        out_table->Table[out_table->NumEntries].Speed = ifrow.dwSpeed;
        out_table->Table[out_table->NumEntries].pAdapterInfo = NULL;
        out_table->NumEntries++;
    }

    free(if_table);

    *Table = out_table;
    return NO_ERROR;
  }



  typedef struct _MIB_IPFORWARD_ROW2 {
    DWORD    dwForwardDest;
    DWORD    dwForwardMask;
    DWORD    dwForwardPolicy;
    DWORD    dwForwardNextHop;
    IF_INDEX dwForwardIfIndex;
    DWORD    dwForwardType;
    DWORD    dwForwardProto;
    DWORD    dwForwardAge;
    DWORD    dwForwardNextHopAS;
    DWORD    dwForwardMetric1;
    DWORD    dwForwardMetric2;
    DWORD    dwForwardMetric3;
    DWORD    dwForwardMetric4;
    DWORD    dwForwardMetric5;
  } MIB_IPFORWARD_ROW2, *PMIB_IPFORWARD_ROW2;
  

  
typedef struct _MIB_IPFORWARD_TABLE2 {
  ULONG        NumEntries;
  MIB_IPFORWARD_ROW2 Table[MAX_PATH];
} MIB_IPFORWARD_TABLE2, *PMIB_IPFORWARD_TABLE2;


DWORD WINAPI
GetIpForwardTable2(
        ADDRESS_FAMILY        Family,
        PMIB_IPFORWARD_TABLE2 *Table
  )
  {
    DWORD ret, size = 0;
    PMIB_IPFORWARDTABLE v1_table = NULL;
    PMIB_IPFORWARD_TABLE2 out_table;
    ULONG i, count;

    TRACE("GetIpForwardTable2(Family %d, Table %p)\n", Family, Table);

    if (!Table)
        return ERROR_INVALID_PARAMETER;

    if (Family != AF_UNSPEC && Family != AF_INET)
        return ERROR_NOT_SUPPORTED;

    /* Query size of v1 table */
    ret = GetIpForwardTable(NULL, &size, FALSE);
    if (ret != ERROR_INSUFFICIENT_BUFFER)
        return ret;

    v1_table = (PMIB_IPFORWARDTABLE)HeapAlloc(GetProcessHeap(), 0, size);
    if (!v1_table)
        return ERROR_OUTOFMEMORY;

    ret = GetIpForwardTable(v1_table, &size, FALSE);
    if (ret != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, v1_table);
        return ret;
    }

    out_table = (PMIB_IPFORWARD_TABLE2)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                 sizeof(MIB_IPFORWARD_TABLE2));
    if (!out_table)
    {
        HeapFree(GetProcessHeap(), 0, v1_table);
        return ERROR_OUTOFMEMORY;
    }

    count = v1_table->dwNumEntries;
    if (count > MAX_PATH)
        count = MAX_PATH;

    out_table->NumEntries = 0;
    for (i = 0; i < count; i++)
    {
        const MIB_IPFORWARDROW *r1 = &v1_table->table[i];
        MIB_IPFORWARD_ROW2 *r2 = &out_table->Table[out_table->NumEntries];

        r2->dwForwardDest      = r1->dwForwardDest;
        r2->dwForwardMask      = r1->dwForwardMask;
        r2->dwForwardPolicy    = r1->dwForwardPolicy;
        r2->dwForwardNextHop   = r1->dwForwardNextHop;
        r2->dwForwardIfIndex   = r1->dwForwardIfIndex;
        r2->dwForwardType      = r1->dwForwardType;
        r2->dwForwardProto     = r1->dwForwardProto;
        r2->dwForwardAge       = r1->dwForwardAge;
        r2->dwForwardNextHopAS = r1->dwForwardNextHopAS;
        r2->dwForwardMetric1   = r1->dwForwardMetric1;
        r2->dwForwardMetric2   = r1->dwForwardMetric2;
        r2->dwForwardMetric3   = r1->dwForwardMetric3;
        r2->dwForwardMetric4   = r1->dwForwardMetric4;
        r2->dwForwardMetric5   = r1->dwForwardMetric5;

        out_table->NumEntries++;
    }

    HeapFree(GetProcessHeap(), 0, v1_table);

    *Table = out_table;
    return NO_ERROR;
  }

  typedef void (WINAPI *PIPFORWARD_CHANGE_CALLBACK)(
       HANDLE                         NotificationHandle,
       ADDRESS_FAMILY                 AddressFamily,
       PVOID                          CallerContext,
       PMIB_IPFORWARD_TABLE2          Table
  );
  DWORD WINAPI
  NotifyRouteChange2(
          ADDRESS_FAMILY             AddressFamily,
          PIPFORWARD_CHANGE_CALLBACK Callback,
          PVOID                      CallerContext,
          BOOLEAN                    InitialNotification,
          HANDLE                     *NotificationHandle
  )
  {
    PMIB_IPFORWARD_TABLE2 table = NULL;
    HANDLE handle_value = NULL;

    TRACE("NotifyRouteChange2(Family %d, Callback %p, Ctx %p, Initial %d, Handle %p)\n",
          AddressFamily, Callback, CallerContext, InitialNotification, NotificationHandle);

    if (!NotificationHandle || !Callback)
        return ERROR_INVALID_PARAMETER;

    if (AddressFamily != AF_UNSPEC && AddressFamily != AF_INET)
        return ERROR_NOT_SUPPORTED;

    /* Allocate a dummy handle to represent the registration. */
    handle_value = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD));
    if (!handle_value)
        return ERROR_OUTOFMEMORY;

    *NotificationHandle = handle_value;

    if (InitialNotification)
    {
        if (GetIpForwardTable2(AddressFamily, &table) == NO_ERROR)
        {
            Callback(*NotificationHandle, AddressFamily, CallerContext, table);
            /* Free immediately since we don't yet provide FreeMibTable export here. */
            HeapFree(GetProcessHeap(), 0, table);
        }
    }

    return NO_ERROR;
  }

/******************************************************************
 *    GetBestRoute2 (IPHLPAPI.@)
 *
 * PARAMS
 *  InterfaceLuid    [In]     Interface LUID (can be NULL)
 *  InterfaceIndex   [In]     Interface index (can be 0)
 *  SourceAddress    [In]     Source address (can be NULL)
 *  DestinationAddress [In]   Destination address
 *  AddressSortOptions [In]   Address sorting options
 *  BestRoute        [Out]    Best route information
 *  BestSourceAddress [Out]   Best source address (can be NULL)
 *
 * RETURNS
 *  DWORD
 */
DWORD WINAPI
GetBestRoute2(
    IN PVOID InterfaceLuid,
    IN ULONG InterfaceIndex,
    IN CONST PVOID SourceAddress,
    IN CONST PVOID DestinationAddress,
    IN ULONG AddressSortOptions,
    OUT PMIB_IPFORWARD_ROW2 BestRoute,
    OUT PVOID BestSourceAddress)
{
    DWORD ret;
    MIB_IPFORWARDROW oldRoute;
    DWORD destAddr, srcAddr = 0;

    TRACE("GetBestRoute2(%p, %lu, %p, %p, %lu, %p, %p)\n",
          InterfaceLuid, InterfaceIndex, SourceAddress, DestinationAddress,
          AddressSortOptions, BestRoute, BestSourceAddress);

    if (!DestinationAddress || !BestRoute)
        return ERROR_INVALID_PARAMETER;

    /* For now, assume IPv4 addresses passed as SOCKADDR_IN structures */
    /* This is a simplified implementation - real GetBestRoute2 supports IPv6 */
    PSOCKADDR_IN destSockAddr = (PSOCKADDR_IN)DestinationAddress;
    PSOCKADDR_IN srcSockAddr = (PSOCKADDR_IN)SourceAddress;

    if (destSockAddr->sin_family != AF_INET)
    {
        FIXME("GetBestRoute2: Only IPv4 supported in this implementation\n");
        return ERROR_NOT_SUPPORTED;
    }

    destAddr = destSockAddr->sin_addr.s_addr;
    if (srcSockAddr && srcSockAddr->sin_family == AF_INET)
        srcAddr = srcSockAddr->sin_addr.s_addr;

    /* Use the old GetBestRoute function for IPv4 */
    ret = GetBestRoute(destAddr, srcAddr, &oldRoute);
    if (ret != ERROR_SUCCESS)
        return ret;

    /* Convert old route structure to new format */
    ZeroMemory(BestRoute, sizeof(MIB_IPFORWARD_ROW2));
    
    /* Copy basic route information - using simplified structure */
    BestRoute->dwForwardDest = oldRoute.dwForwardDest;
    BestRoute->dwForwardMask = oldRoute.dwForwardMask;
    BestRoute->dwForwardPolicy = oldRoute.dwForwardPolicy;
    BestRoute->dwForwardNextHop = oldRoute.dwForwardNextHop;
    BestRoute->dwForwardIfIndex = oldRoute.dwForwardIfIndex;
    BestRoute->dwForwardType = oldRoute.dwForwardType;
    BestRoute->dwForwardProto = oldRoute.dwForwardProto;
    BestRoute->dwForwardAge = oldRoute.dwForwardAge;
    BestRoute->dwForwardNextHopAS = oldRoute.dwForwardNextHopAS;
    BestRoute->dwForwardMetric1 = oldRoute.dwForwardMetric1;
    BestRoute->dwForwardMetric2 = oldRoute.dwForwardMetric2;
    BestRoute->dwForwardMetric3 = oldRoute.dwForwardMetric3;
    BestRoute->dwForwardMetric4 = oldRoute.dwForwardMetric4;
    BestRoute->dwForwardMetric5 = oldRoute.dwForwardMetric5;
    
    /* Set best source address if requested */
    if (BestSourceAddress)
    {
        PSOCKADDR_IN bestSrcAddr = (PSOCKADDR_IN)BestSourceAddress;
        ZeroMemory(bestSrcAddr, sizeof(SOCKADDR_IN));
        bestSrcAddr->sin_family = AF_INET;
        
        /* Try to find the best source address for this route */
        PMIB_IPADDRTABLE addrTable = NULL;
        DWORD addrSize = 0;
        
        ret = GetIpAddrTable(NULL, &addrSize, FALSE);
        if (ret == ERROR_INSUFFICIENT_BUFFER)
        {
            addrTable = (PMIB_IPADDRTABLE)HeapAlloc(GetProcessHeap(), 0, addrSize);
            if (addrTable)
            {
                ret = GetIpAddrTable(addrTable, &addrSize, FALSE);
                if (ret == ERROR_SUCCESS)
                {
                    /* Find address on the same interface */
                    for (DWORD i = 0; i < addrTable->dwNumEntries; i++)
                    {
                        if (addrTable->table[i].dwIndex == oldRoute.dwForwardIfIndex)
                        {
                            bestSrcAddr->sin_addr.s_addr = addrTable->table[i].dwAddr;
                            break;
                        }
                    }
                }
                HeapFree(GetProcessHeap(), 0, addrTable);
            }
        }
    }

    return ERROR_SUCCESS;
}
