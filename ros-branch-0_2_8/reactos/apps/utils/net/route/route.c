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

static int Usage()
{
    _ftprintf( stderr,
               _T("route usage:\n"
                  "route print\n"
                  "  prints the route table\n"
                  "route add <target> [mask <mask>] <gw> [metric <m>]\n"
                  "  adds a route\n"
                  "route delete <target> <gw>\n"
                  "  deletes a route\n") );
    return 1;
}

static int PrintRoutes()
{
    PMIB_IPFORWARDTABLE IpForwardTable = NULL;
    PIP_ADAPTER_INFO pAdapterInfo;
    ULONG Size = 0;
    DWORD Error = 0;
    ULONG adaptOutBufLen = sizeof(IP_ADAPTER_INFO);
    TCHAR DefGate[16];
    TCHAR Destination[IPBUF], Gateway[IPBUF], Netmask[IPBUF];
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
            free(pAdapterInfo);
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (((Error = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen)) == NO_ERROR) &&
        ((Error = GetIpForwardTable(IpForwardTable, &Size, TRUE)) == NO_ERROR))
    {
        _stprintf(DefGate,
#if UNICODE
                  _T("%hs"),
#else
                  _T("%s"),
#endif
                  pAdapterInfo->GatewayList.IpAddress.String);
        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Interface List\n"));
        /* FIXME - sort by the index! */
        while (pAdapterInfo)
        {
            _tprintf(_T("0x%lu ........................... "
#if UNICODE
                        "%hs\n"),
#else
                        "%s\n"),
#endif
                     pAdapterInfo->Index, pAdapterInfo->Description);
            pAdapterInfo = pAdapterInfo->Next;
        }
        _tprintf(_T("===========================================================================\n"));

        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Active Routes:\n"));
        _tprintf( _T("%-27s%-17s%-14s%-11s%-10s\n"),
                  _T("Network Destination"),
                  _T("Netmask"),
                  _T("Gateway"),
                  _T("Interface"),
                  _T("Metric") );
        for( i = 0; i < IpForwardTable->dwNumEntries; i++ )
        {
            _stprintf( Destination,
#if UNICODE
                       _T("%hs"),
#else
                       _T("%s"),
#endif
                       inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardDest) ) );
            _stprintf( Netmask,
#if UNICODE
                       _T("%hs"),
#else
                       _T("%s"),
#endif
                       inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardMask) ) );
            _stprintf( Gateway,
#if UNICODE
                       _T("%hs"),
#else
                       _T("%s"),
#endif
                       inet_ntoa( IN_ADDR_OF(IpForwardTable->table[i].dwForwardNextHop) ) );

            _tprintf( _T("%17s%17s%17s%16ld%9ld\n"),
                      Destination,
                      Netmask,
                      Gateway,
                      IpForwardTable->table[i].dwForwardIfIndex,
                      IpForwardTable->table[i].dwForwardMetric1 );
        }
        _tprintf(_T("Default Gateway:%18s\n"), DefGate);
        _tprintf(_T("===========================================================================\n"));
        _tprintf(_T("Persistent Routes:\n"));

        free(IpForwardTable);
        free(pAdapterInfo);

        return ERROR_SUCCESS;
    }
    else
    {
Error:
        _ftprintf( stderr, _T("Route enumerate failed\n") );
        return Error;
    }
}

static int convert_add_cmd_line( PMIB_IPFORWARDROW RowToAdd,
              int argc, TCHAR **argv ) {
    int i;
#if UNICODE
    char addr[16];
#endif

    if( argc > 1 )
    {
#if UNICODE
        sprintf( addr, "%ls", argv[0] );
        RowToAdd->dwForwardDest = inet_addr( addr );
#else
        RowToAdd->dwForwardDest = inet_addr( argv[0] );
#endif
    }
    else
        return FALSE;
    for( i = 1; i < argc; i++ )
    {
        if( !_tcscmp( argv[i], _T("mask") ) )
        {
            i++; if( i >= argc ) return FALSE;
#if UNICODE
            sprintf( addr, "%ls", argv[i] );
            RowToAdd->dwForwardDest = inet_addr( addr );
#else
            RowToAdd->dwForwardMask = inet_addr( argv[i] );
#endif
        }
        else if( !_tcscmp( argv[i], _T("metric") ) )
        {
            i++;
            if( i >= argc )
                return FALSE;
            RowToAdd->dwForwardMetric1 = _ttoi( argv[i] );
        }
        else
        {
#if UNICODE
            sprintf( addr, "%ls", argv[i] );
            RowToAdd->dwForwardNextHop = inet_addr( addr );
#else
            RowToAdd->dwForwardNextHop = inet_addr( argv[i] );
#endif
        }
    }

    return TRUE;
}

static int add_route( int argc, TCHAR **argv ) {
    MIB_IPFORWARDROW RowToAdd = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToAdd, argc, argv ) )
    {
        _ftprintf( stderr,
                   _T("route add usage:\n"
                      "route add <target> [mask <mask>] <gw> [metric <m>]\n"
                      "  Adds a route to the IP route table.\n"
                      "  <target> is the network or host to add a route to.\n"
                      "  <mask>   is the netmask to use (autodetected if unspecified)\n"
                      "  <gw>     is the gateway to use to access the network\n"
                      "  <m>      is the metric to use (lower is preferred)\n") );
        return 1;
    }

    if( (Error = CreateIpForwardEntry( &RowToAdd )) == ERROR_SUCCESS )
        return 0;

    _ftprintf( stderr, _T("Route addition failed\n") );
    return Error;
}

static int del_route( int argc, TCHAR **argv )
{
    MIB_IPFORWARDROW RowToDel = { 0 };
    DWORD Error;

    if( argc < 2 || !convert_add_cmd_line( &RowToDel, argc, argv ) )
    {
        _ftprintf( stderr,
                    _T("route delete usage:\n"
                       "route delete <target> <gw>\n"
                       "  Removes a route from the IP route table.\n"
                       "  <target> is the network or host to add a route to.\n"
                       "  <gw>     is the gateway to remove the route from.\n") );
        return 1;
    }

    if( (Error = DeleteIpForwardEntry( &RowToDel )) == ERROR_SUCCESS )
        return 0;

    _ftprintf( stderr, _T("Route addition failed\n") );
    return Error;
}

#if defined(_UNICODE) && defined(__GNUC__)
static
#endif
int _tmain( int argc, TCHAR **argv )
{
    if( argc < 2 )
        return Usage();
    else if ( !_tcscmp( argv[1], _T("print") ) )
        return PrintRoutes();
    else if( !_tcscmp( argv[1], _T("add") ) )
        return add_route( argc-2, argv+2 );
    else if( !_tcscmp( argv[1], _T("delete") ) )
        return del_route( argc-2, argv+2 );
    else
        return Usage();
}

#if defined(_UNICODE) && defined(__GNUC__)
/* HACK - MINGW HAS NO OFFICIAL SUPPORT FOR wmain()!!! */
int main( int argc, char **argv )
{
    WCHAR **argvW;
    int i, j, Ret = 1;

    if ((argvW = malloc(argc * sizeof(WCHAR*))))
    {
        /* convert the arguments */
        for (i = 0, j = 0; i < argc; i++)
        {
            if (!(argvW[i] = malloc((strlen(argv[i]) + 1) * sizeof(WCHAR))))
            {
                j++;
            }
            swprintf(argvW[i], L"%hs", argv[i]);
        }
        
        if (j == 0)
        {
            /* no error converting the parameters, call wmain() */
            Ret = wmain(argc, argvW);
        }
        
        /* free the arguments */
        for (i = 0; i < argc; i++)
        {
            if (argvW[i])
                free(argvW[i]);
        }
        free(argvW);
    }
    
    return Ret;
}
#endif
