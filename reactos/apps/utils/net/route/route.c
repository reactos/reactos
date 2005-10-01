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

#include <stdio.h>
#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <tchar.h>

#define IPBUF 17
#define IN_ADDR_OF(x) *((struct in_addr *)&(x))

int usage()
{
    fprintf( stderr,
         "route usage:\n"
         "route print\n"
         "  prints the route table\n"
         "route add <target> [mask <mask>] <gw> [metric <m>]\n"
         "  adds a route\n"
         "route delete <target> <gw>\n"
         "  deletes a route\n" );
    return 1;
}

int print_routes()
{
    PMIB_IPFORWARDTABLE IpForwardTable = NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD Error;
    ULONG Size = 0;
    ULONG adaptOutBufLen;
    TCHAR DefGate[16];
    char Destination[IPBUF], Gateway[IPBUF], Netmask[IPBUF];
    unsigned int i;

    /* set required buffer size */
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
    if (GetAdaptersInfo( pAdapterInfo, &adaptOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
       pAdapterInfo = (IP_ADAPTER_INFO *) malloc (adaptOutBufLen);

    if( (GetIpForwardTable( NULL, &Size, TRUE )) == ERROR_INSUFFICIENT_BUFFER )
        IpForwardTable = malloc( Size );

    if ((GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen) == NO_ERROR) &&
        GetIpForwardTable( IpForwardTable, &Size, TRUE ) == NO_ERROR)
    {
        _tcsncpy(DefGate, pAdapterInfo->GatewayList.IpAddress.String, 16);
        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Interface List\n"));
        while (pAdapterInfo)
        {
            _tprintf(_T("0x%lu ........................... %s\n"), pAdapterInfo->Index, pAdapterInfo->Description);
            pAdapterInfo = pAdapterInfo->Next;
        }
        _tprintf(_T("===========================================================================\n"));

        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Active Routes:\n"));
        printf( "%-27s%-17s%-14s%-11s%-10s\n",
            "Network Destination",
            "Netmask",
            "Gateway",
            "Interface",
            "Metric" );
        for( i = 0; i < IpForwardTable->dwNumEntries; i++ )
        {
            strcpy( Destination, inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardDest) ) );
            strcpy( Netmask, inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardMask) ) );
            strcpy( Gateway, inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardNextHop) ) );

            printf( "%17s%17s%17s%16ld%9ld\n",
                Destination,
                Netmask,
                Gateway,
                IpForwardTable->table[i].dwForwardIfIndex,
                IpForwardTable->table[i].dwForwardMetric1 );
        }
        _tprintf(_T("Default Gateway:%18s\n"), DefGate);
        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Persistent Routes:\n"));

        free( IpForwardTable );

        return ERROR_SUCCESS;
    }
    else
    {
        fprintf( stderr, "Route enumerate failed\n" );
        return Error;
    }
}

int convert_add_cmd_line( PMIB_IPFORWARDROW RowToAdd,
              int argc, char **argv ) {
    int i;

    if( argc > 1 )
        RowToAdd->dwForwardDest = inet_addr( argv[0] );
    else
        return FALSE;
    for( i = 1; i < argc; i++ )
    {
        if( !strcasecmp( argv[i], "mask" ) )
        {
            i++; if( i >= argc ) return FALSE;
            RowToAdd->dwForwardMask = inet_addr( argv[i] );
        }
        else if( !strcasecmp( argv[i], "metric" ) )
        {
            i++;
            if( i >= argc )
                return FALSE;
            RowToAdd->dwForwardMetric1 = atoi( argv[i] );
        }
        else
            RowToAdd->dwForwardNextHop = inet_addr( argv[i] );
    }

    return TRUE;
}

int add_route( int argc, char **argv ) {
    MIB_IPFORWARDROW RowToAdd = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToAdd, argc, argv ) )
    {
        fprintf( stderr,
            "route add usage:\n"
            "route add <target> [mask <mask>] <gw> [metric <m>]\n"
            "  Adds a route to the IP route table.\n"
            "  <target> is the network or host to add a route to.\n"
            "  <mask>   is the netmask to use (autodetected if unspecified)\n"
            "  <gw>     is the gateway to use to access the network\n"
            "  <m>      is the metric to use (lower is preferred)\n" );
        return 1;
    }

    if( (Error = CreateIpForwardEntry( &RowToAdd )) == ERROR_SUCCESS )
        return 0;

    fprintf( stderr, "Route addition failed\n" );
    return Error;
}

int del_route( int argc, char **argv )
{
    MIB_IPFORWARDROW RowToDel = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToDel, argc, argv ) )
    {
        fprintf( stderr,
            "route delete usage:\n"
            "route delete <target> <gw>\n"
            "  Removes a route from the IP route table.\n"
            "  <target> is the network or host to add a route to.\n"
            "  <gw>     is the gateway to remove the route from.\n" );
        return 1;
    }

    if( (Error = DeleteIpForwardEntry( &RowToDel )) == ERROR_SUCCESS )
        return 0;

    fprintf( stderr, "Route addition failed\n" );
    return Error;
}

int main( int argc, char **argv )
{

    if( argc < 2 )
        return usage();
    else if ( !strcasecmp( argv[1], "print" ) )
        return print_routes();
    else if( !strcasecmp( argv[1], "add" ) )
        return add_route( argc-2, argv+2 );
    else if( !strcasecmp( argv[1], "delete" ) )
        return del_route( argc-2, argv+2 );
    else
        return usage();
}
