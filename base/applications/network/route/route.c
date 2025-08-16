/* Poor man's route
 *
 * Supported commands:
 *
 * "print"
 * "add" target ["mask" mask] gw ["metric" metric]
 * "delete" target gw
 *
 * Goals:
 *
 * Flexible, simple
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <malloc.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <iphlpapi.h>
#include <conutils.h>

#include "resource.h"

#define IPBUF 17
#define IN_ADDR_OF(x) *((struct in_addr *)&(x))

static int Usage()
{
    ConResPrintf(StdErr, IDS_USAGE);
    return 1;
}

static int PrintRoutes()
{
    PMIB_IPFORWARDTABLE IpForwardTable = NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG Size = 0;
    DWORD Error = 0;
    ULONG adaptOutBufLen = sizeof(IP_ADAPTER_INFO);
    WCHAR DefGate[16];
    WCHAR Destination[IPBUF], Gateway[IPBUF], Netmask[IPBUF];
    unsigned int i;

    /* set required buffer size */
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc( adaptOutBufLen );
    if (pAdapterInfo == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    if (GetAdaptersInfo( pAdapterInfo, &adaptOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
       free (pAdapterInfo);
       pAdapterInfo = (IP_ADAPTER_INFO *) malloc (adaptOutBufLen);
       if (pAdapterInfo == NULL)
       {
           Error = ERROR_NOT_ENOUGH_MEMORY;
           goto Error;
       }
    }

    if( (GetIpForwardTable( NULL, &Size, TRUE )) == ERROR_INSUFFICIENT_BUFFER )
    {
        if (!(IpForwardTable = malloc( Size )))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (((Error = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen)) == NO_ERROR) &&
        ((Error = GetIpForwardTable(IpForwardTable, &Size, TRUE)) == NO_ERROR))
    {
        mbstowcs(DefGate, pAdapterInfo->GatewayList.IpAddress.String, 16);

        ConResPrintf(StdOut, IDS_SEPARATOR);
        ConResPrintf(StdOut, IDS_INTERFACE_LIST);
        /* FIXME - sort by the index! */
        while (pAdapterInfo)
        {
            ConResPrintf(StdOut, IDS_INTERFACE_ENTRY, pAdapterInfo->Index, pAdapterInfo->Description);
            pAdapterInfo = pAdapterInfo->Next;
        }
        ConResPrintf(StdOut, IDS_SEPARATOR);

        ConResPrintf(StdOut, IDS_IPV4_ROUTE_TABLE);
        ConResPrintf(StdOut, IDS_SEPARATOR);
        ConResPrintf(StdOut, IDS_ACTIVE_ROUTES);
        ConResPrintf(StdOut, IDS_ROUTES_HEADER);
        for( i = 0; i < IpForwardTable->dwNumEntries; i++ )
        {
            mbstowcs(Destination, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardDest)), IPBUF);
            mbstowcs(Netmask, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardMask)), IPBUF);
            mbstowcs(Gateway, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardNextHop)), IPBUF);

            ConResPrintf(StdOut, IDS_ROUTES_ENTRY,
                         Destination,
                         Netmask,
                         Gateway,
                         IpForwardTable->table[i].dwForwardIfIndex,
                         IpForwardTable->table[i].dwForwardMetric1);
        }
        ConResPrintf(StdOut, IDS_DEFAULT_GATEWAY, DefGate);
        ConResPrintf(StdOut, IDS_SEPARATOR);

        ConResPrintf(StdOut, IDS_PERSISTENT_ROUTES);
        ConResPrintf(StdOut, IDS_NONE);

        free(IpForwardTable);
        free(pAdapterInfo);

        return ERROR_SUCCESS;
    }
    else
    {
Error:
        if (pAdapterInfo) free(pAdapterInfo);
        if (IpForwardTable) free(IpForwardTable);
        ConResPrintf(StdErr, IDS_ROUTE_ENUM_ERROR);
        return Error;
    }
}

static int convert_add_cmd_line( PMIB_IPFORWARDROW RowToAdd,
              int argc, WCHAR **argv ) {
    int i;
    char addr[16];

    if( argc > 1 )
    {
        wcstombs(addr, argv[0], 16);
        RowToAdd->dwForwardDest = inet_addr( addr );
    }
    else
        return FALSE;

    for( i = 1; i < argc; i++ )
    {
        if( !_wcsicmp( argv[i], L"mask" ) )
        {
            i++; if( i >= argc ) return FALSE;
            wcstombs(addr, argv[i], 16);
            RowToAdd->dwForwardMask = inet_addr( addr );
        }
        else if( !_wcsicmp( argv[i], L"metric" ) )
        {
            i++;
            if( i >= argc )
                return FALSE;
            RowToAdd->dwForwardMetric1 = _wtoi( argv[i] );
        }
        else
        {
            wcstombs(addr, argv[i], 16);
            RowToAdd->dwForwardNextHop = inet_addr( addr );
        }
    }

    return TRUE;
}

static int add_route( int argc, WCHAR **argv ) {
    MIB_IPFORWARDROW RowToAdd = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToAdd, argc, argv ) )
    {
        ConResPrintf(StdErr, IDS_ROUTE_ADD_HELP);
        return 1;
    }

    if( (Error = CreateIpForwardEntry( &RowToAdd )) == ERROR_SUCCESS )
        return 0;

    ConResPrintf(StdErr, IDS_ROUTE_ADD_ERROR);
    return Error;
}

static int del_route( int argc, WCHAR **argv )
{
    MIB_IPFORWARDROW RowToDel = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToDel, argc, argv ) )
    {
        ConResPrintf(StdErr, IDS_ROUTE_ADD_HELP);
        return 1;
    }

    if( (Error = DeleteIpForwardEntry( &RowToDel )) == ERROR_SUCCESS )
        return 0;

    ConResPrintf(StdErr, IDS_ROUTE_DEL_ERROR);
    return Error;
}

int wmain( int argc, WCHAR **argv )
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if( argc < 2 )
        return Usage();
    else if ( !_wcsicmp( argv[1], L"print" ) )
        return PrintRoutes();
    else if( !_wcsicmp( argv[1], L"add" ) )
        return add_route( argc-2, argv+2 );
    else if( !_wcsicmp( argv[1], L"delete" ) )
        return del_route( argc-2, argv+2 );
    else
        return Usage();
}
