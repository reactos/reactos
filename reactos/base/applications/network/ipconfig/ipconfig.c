/*
 * PROJECT:     ReactOS ipconfig utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        apps/utils/net/ipconfig/ipconfig.c
 * PURPOSE:     Display IP info for net adapters
 * PROGRAMMERS: Copyright 2005 - 2006 Ged Murphy (gedmurphy@gmail.com)
 */
/*
 * TODO:
 * fix renew / release
 * implement flushdns, registerdns, displaydns, showclassid, setclassid
 * allow globbing on adapter names
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <iphlpapi.h>

HANDLE ProcessHeap;

LPCTSTR GetNodeTypeName(UINT NodeType)
{
    switch (NodeType) 
    {
        case 1:   return _T("Broadcast");
        case 2:   return _T("Peer To Peer");
        case 4:   return _T("Mixed");
        case 8:   return _T("Hybrid");
        default : return _T("unknown");
    }
}

LPCTSTR GetInterfaceTypeName(UINT InterfaceType)
{
    switch (InterfaceType) 
    {
        case MIB_IF_TYPE_OTHER:     return _T("Other Type Of Adapter");
        case MIB_IF_TYPE_ETHERNET:  return _T("Ethernet Adapter");
        case MIB_IF_TYPE_TOKENRING: return _T("Token Ring Adapter");
        case MIB_IF_TYPE_FDDI:      return _T("FDDI Adapter");
        case MIB_IF_TYPE_PPP:       return _T("PPP Adapter");
        case MIB_IF_TYPE_LOOPBACK:  return _T("Loopback Adapter");
        case MIB_IF_TYPE_SLIP:      return _T("SLIP Adapter");
        default:                    return _T("unknown");
    }
}

/* print MAC address */
PTCHAR PrintMacAddr(PBYTE Mac)
{
    static TCHAR MacAddr[20];

    _stprintf(MacAddr, _T("%02x-%02x-%02x-%02x-%02x-%02x"),
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4],  Mac[5]);

    return MacAddr;
}

DWORD DoFormatMessage(DWORD ErrorCode)
{
    LPVOID lpMsgBuf;
    DWORD RetVal;

    if ((RetVal = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            ErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
            (LPTSTR) &lpMsgBuf,
            0,
            NULL ))) {
        _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);

        LocalFree(lpMsgBuf);
        return RetVal;
    }
    else
        return 0;
}

VOID ShowInfo(BOOL bAll)
{
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG adaptOutBufLen = 0;

    PFIXED_INFO pFixedInfo = NULL;
    PIP_ADDR_STRING pIPAddr = NULL;
    ULONG netOutBufLen = 0;

    DWORD ErrRet = 0;

    /* assign memory for call to GetNetworkParams */
    pFixedInfo = (FIXED_INFO *)HeapAlloc(ProcessHeap, 0, sizeof(FIXED_INFO));
    if (pFixedInfo == NULL)
    {
        _tprintf(_T("memory allocation error"));
        return;
    }

    /* set required buffer size */
    if(GetNetworkParams(pFixedInfo, &netOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        HeapFree(ProcessHeap, 0, pFixedInfo);
        pFixedInfo = (FIXED_INFO *)HeapAlloc(ProcessHeap, 0, netOutBufLen);
        if (pFixedInfo == NULL)
        {
            _tprintf(_T("memory allocation error"));
            return;
        }
    }

    if ((ErrRet = GetNetworkParams(pFixedInfo, &netOutBufLen)) != NO_ERROR)
    {
        _tprintf(_T("GetNetworkParams failed : "));
        DoFormatMessage(ErrRet);
        HeapFree(ProcessHeap, 0, pFixedInfo);
        return;
    }



    /* assign memory for call to GetAdapterInfo */
    pAdapterInfo = (IP_ADAPTER_INFO *)HeapAlloc(ProcessHeap, 0, sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL)
    {
        _tprintf(_T("memory allocation error"));
        return;
    }

    /* set required buffer size */
    if (GetAdaptersInfo( pAdapterInfo, &adaptOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        HeapFree(ProcessHeap, 0, pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)HeapAlloc(ProcessHeap, 0, adaptOutBufLen);
        if (pAdapterInfo == NULL)
        {
            _tprintf(_T("memory allocation error"));
            return;
        }
    }

    if ((ErrRet = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen)) != NO_ERROR)
    {
        _tprintf(_T("GetAdaptersInfo failed : "));
        DoFormatMessage(ErrRet);
        HeapFree(ProcessHeap, 0, pAdapterInfo);
        return ;
    }


    pAdapter = pAdapterInfo;

    _tprintf(_T("\nReactOS IP Configuration\n\n"));

    if (bAll)
    {
        _tprintf(_T("\tHost Name . . . . . . . . . . . . : %s\n"), pFixedInfo->HostName);
        _tprintf(_T("\tPrimary DNS Suffix. . . . . . . . : \n"));
        _tprintf(_T("\tNode Type . . . . . . . . . . . . : %s\n"), GetNodeTypeName(pFixedInfo->NodeType));
        if (pFixedInfo->EnableRouting)
            _tprintf(_T("\tIP Routing Enabled. . . . . . . . : Yes\n"));
        else
            _tprintf(_T("\tIP Routing Enabled. . . . . . . . : No\n"));
        if (pAdapter->HaveWins)
            _tprintf(_T("\tWINS Proxy enabled. . . . . . . . : Yes\n"));
        else
            _tprintf(_T("\tWINS Proxy enabled. . . . . . . . : No\n"));
        _tprintf(_T("\tDNS Suffix Search List. . . . . . : %s\n"), pFixedInfo->DomainName);
    }

    while (pAdapter)
    {
        _tprintf(_T("\n%s ...... : \n\n"), GetInterfaceTypeName(pAdapter->Type));

        /* check if the adapter is connected to the media */
        if (_tcscmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") == 0)
        {
            _tprintf(_T("\tMedia State . . . . . . . . . . . : Media disconnected\n"));
            pAdapter = pAdapter->Next;
            continue;
        }

        _tprintf(_T("\tConnection-specific DNS Suffix. . : %s\n"), pFixedInfo->DomainName);

        if (bAll)
        {
            _tprintf(_T("\tDescription . . . . . . . . . . . : %s\n"), pAdapter->Description);
            _tprintf(_T("\tPhysical Address. . . . . . . . . : %s\n"), PrintMacAddr(pAdapter->Address));
            if (pAdapter->DhcpEnabled)
                _tprintf(_T("\tDHCP Enabled. . . . . . . . . . . : Yes\n"));
            else
                _tprintf(_T("\tDHCP Enabled. . . . . . . . . . . : No\n"));
            _tprintf(_T("\tAutoconfiguration Enabled . . . . : \n"));
        }

        _tprintf(_T("\tIP Address. . . . . . . . . . . . : %s\n"), pAdapter->IpAddressList.IpAddress.String);
        _tprintf(_T("\tSubnet Mask . . . . . . . . . . . : %s\n"), pAdapter->IpAddressList.IpMask.String);
        _tprintf(_T("\tDefault Gateway . . . . . . . . . : %s\n"), pAdapter->GatewayList.IpAddress.String);

        if (bAll)
        {
            if (pAdapter->DhcpEnabled)
               _tprintf(_T("\tDHCP Server . . . . . . . . . . . : %s\n"), pAdapter->DhcpServer.IpAddress.String);

            _tprintf(_T("\tDNS Servers . . . . . . . . . . . : "));
            _tprintf(_T("%s\n"), pFixedInfo->DnsServerList.IpAddress.String);
            pIPAddr = pFixedInfo -> DnsServerList.Next;
            while (pIPAddr)
            {
                _tprintf(_T("\t\t\t\t\t    %s\n"), pIPAddr ->IpAddress.String );
                pIPAddr = pIPAddr ->Next;
            }
            if (pAdapter->HaveWins)
            {
                _tprintf(_T("\tPrimary WINS Server . . . . . . . : %s\n"), pAdapter->PrimaryWinsServer.IpAddress.String);
                _tprintf(_T("\tSecondard WINS Server . . . . . . : %s\n"), pAdapter->SecondaryWinsServer.IpAddress.String);
            }
            if (pAdapter->DhcpEnabled)
            {
                _tprintf(_T("\tLease Obtained. . . . . . . . . . : %s"), _tasctime(localtime(&pAdapter->LeaseObtained)));
                _tprintf(_T("\tLease Expires . . . . . . . . . . : %s"), _tasctime(localtime(&pAdapter->LeaseExpires)));
            }
        }
        _tprintf(_T("\n"));

        pAdapter = pAdapter->Next;

    }

    HeapFree(ProcessHeap, 0, pFixedInfo);
    HeapFree(ProcessHeap, 0, pAdapterInfo);
}

VOID Release(LPTSTR Index)
{
    IP_ADAPTER_INDEX_MAP AdapterInfo;
    DWORD dwRetVal = 0;

    /* if interface is not given, query GetInterfaceInfo */
    if (Index == NULL)
    {
        PIP_INTERFACE_INFO pInfo = NULL;
        ULONG ulOutBufLen = 0;

        pInfo = (IP_INTERFACE_INFO *)HeapAlloc(ProcessHeap, 0, sizeof(IP_INTERFACE_INFO));
        if (pInfo == NULL)
        {
            _tprintf(_T("memory allocation error"));
            return;
        }

        /* Make an initial call to GetInterfaceInfo to get
         * the necessary size into the ulOutBufLen variable */
        if ( GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
        {
            HeapFree(ProcessHeap, 0, pInfo);
            pInfo = (IP_INTERFACE_INFO *)HeapAlloc(ProcessHeap, 0, ulOutBufLen);
            if (pInfo == NULL)
            {
                _tprintf(_T("memory allocation error"));
                return;
            }
        }

        /* Make a second call to GetInterfaceInfo to get the actual data we want */
        if ((dwRetVal = GetInterfaceInfo(pInfo, &ulOutBufLen)) == NO_ERROR )
        {
            CopyMemory(&AdapterInfo, &pInfo->Adapter[0], sizeof(IP_ADAPTER_INDEX_MAP));
            _tprintf(_T("name - %S\n"), pInfo->Adapter[0].Name);
        }
        else
        {
            _tprintf(_T("\nGetInterfaceInfo failed : "));
            DoFormatMessage(dwRetVal);
        }

        HeapFree(ProcessHeap, 0, pInfo);
    }
    else
    {
        ;
        /* FIXME:
         * we need to be able to release connections by name with support for globbing
         * i.e. ipconfig /release Eth* will release all cards starting with Eth...
         *      ipconfig /release *con* will release all cards with 'con' in their name
         */
    }


    /* Call IpReleaseAddress to release the IP address on the specified adapter. */
    if ((dwRetVal = IpReleaseAddress(&AdapterInfo)) != NO_ERROR)
    {
        _tprintf(_T("\nAn error occured while releasing interface %s : "), _T("*name*"));
        DoFormatMessage(dwRetVal);
    }

}




VOID Renew(LPTSTR Index)
{
    IP_ADAPTER_INDEX_MAP AdapterInfo;
    DWORD dwRetVal = 0;

    /* if interface is not given, query GetInterfaceInfo */
    if (Index == NULL)
    {
        PIP_INTERFACE_INFO pInfo;
        ULONG ulOutBufLen = 0;

        pInfo = (IP_INTERFACE_INFO *)HeapAlloc(ProcessHeap, 0, sizeof(IP_INTERFACE_INFO));
        if (pInfo == NULL)
        {
            _tprintf(_T("memory allocation error"));
            return;
        }

        /* Make an initial call to GetInterfaceInfo to get
         * the necessary size into the ulOutBufLen variable */
        if ( GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
        {
            HeapFree(ProcessHeap, 0, pInfo);
            pInfo = (IP_INTERFACE_INFO *)HeapAlloc(ProcessHeap, 0, ulOutBufLen);
            if (pInfo == NULL)
            {
                _tprintf(_T("memory allocation error"));
                return;
            }
        }

        /* Make a second call to GetInterfaceInfo to get the actual data we want */
        if ((dwRetVal = GetInterfaceInfo(pInfo, &ulOutBufLen)) == NO_ERROR )
        {
            CopyMemory(&AdapterInfo, &pInfo->Adapter[0], sizeof(IP_ADAPTER_INDEX_MAP));
            _tprintf(_T("name - %S\n"), pInfo->Adapter[0].Name);
        }
        else
        {
            _tprintf(_T("\nGetInterfaceInfo failed : "));
            DoFormatMessage(dwRetVal);
        }

        HeapFree(ProcessHeap, 0, pInfo);
    }
    else
    {
        ;
        /* FIXME:
         * we need to be able to renew connections by name with support for globbing
         * i.e. ipconfig /renew Eth* will renew all cards starting with Eth...
         *      ipconfig /renew *con* will renew all cards with 'con' in their name
         */
    }


    /* Call IpRenewAddress to renew the IP address on the specified adapter. */
    if ((dwRetVal = IpRenewAddress(&AdapterInfo)) != NO_ERROR)
    {
        _tprintf(_T("\nAn error occured while renew interface %s : "), _T("*name*"));
        DoFormatMessage(dwRetVal);
    }
}



VOID Usage(VOID)
{
    _tprintf(_T("\nUSAGE:\n"
    "    ipconfig [/? | /all | /renew [adapter] | /release [adapter] |\n"
    "              /flushdns | /displaydns | /registerdns |\n"
    "              /showclassid adapter |\n"
    "              /setclassid adapter [classid] ]\n"
    "\n"
    "where\n"
    "    adapter         Connection name\n"
    "                   (wildcard characters * and ? allowed, see examples)\n"
    "\n"
    "    Options:\n"
    "       /?           Display this help message\n"
    "       /all         Display full configuration information.\n"
    "       /release     Release the IP address for the specified adapter.\n"
    "       /renew       Renew the IP address for the specified adapter.\n"
    "       /flushdns    Purges the DNS Resolver cache.\n"
    "       /registerdns Refreshes all DHCP leases and re-registers DNS names.\n"
    "       /displaydns  Display the contents of the DNS Resolver Cache.\n"
    "       /showclassid Displays all the dhcp class IDs allowed for adapter.\n"
    "       /setclassid  Modifies the dhcp class id.\n"
    "\n"
    "The default is to display only the IP address, subnet mask and\n"
    "default gateway for each adapter bound to TCP/IP.\n"
    "\n"
    "For Release and Renew, if no adapter name is specified, then the IP address\n"
    "leases for all adapters bound to TCP/IP will be released or renewed.\n"
    "\n"
    "For Setclassid, if no ClassId is specified, then the ClassId is removed.\n"
    "\n"
    "Examples:\n"
    "    > ipconfig                   ... Show information.\n"
    "    > ipconfig /all              ... Show detailed information\n"
    "    > ipconfig /renew            ... renew all adapters\n"
    "    > ipconfig /renew EL*        ... renew any connection that has its\n"
    "                                     name starting with EL\n"
    "    > ipconfig /release *Con*    ... release all matching connections,\n"
    "                                     eg. \"Local Area Connection 1\" or\n"
    "                                         \"Local Area Connection 2\"\n"));
}

int main(int argc, char *argv[])
{
    BOOL DoUsage=FALSE;
    BOOL DoAll=FALSE;
    BOOL DoRelease=FALSE;
    BOOL DoRenew=FALSE;
    BOOL DoFlushdns=FALSE;
    BOOL DoRegisterdns=FALSE;
    BOOL DoDisplaydns=FALSE;
    BOOL DoShowclassid=FALSE;
    BOOL DoSetclassid=FALSE;

    ProcessHeap = GetProcessHeap();

    /* Parse command line for options we have been given. */
    if ( (argc > 1)&&(argv[1][0]=='/') )
    {
        if( !_tcsicmp( &argv[1][1], _T("?") ))
        {
            DoUsage = TRUE;
        }
        else if( !_tcsnicmp( &argv[1][1], _T("ALL"), _tcslen(&argv[1][1]) ))
        {
           DoAll = TRUE;
        }
        else if( !_tcsnicmp( &argv[1][1], _T("RELEASE"), _tcslen(&argv[1][1]) ))
        {
            DoRelease = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("RENEW"), _tcslen(&argv[1][1]) ))
        {
            DoRenew = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("FLUSHDNS"), _tcslen(&argv[1][1]) ))
        {
            DoFlushdns = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("FLUSHREGISTERDNS"), _tcslen(&argv[1][1]) ))
        {
            DoRegisterdns = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("DISPLAYDNS"), _tcslen(&argv[1][1]) ))
        {
            DoDisplaydns = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("SHOWCLASSID"), _tcslen(&argv[1][1]) ))
        {
            DoShowclassid = TRUE;
        }
        else if( ! _tcsnicmp( &argv[1][1], _T("SETCLASSID"), _tcslen(&argv[1][1]) ))
        {
            DoSetclassid = TRUE;
        }
    }

    switch (argc)
    {
        case 1:  /* Default behaviour if no options are given*/
            ShowInfo(FALSE);
            break;
        case 2:  /* Process all the options that take no parameters */
            if (DoUsage)
                Usage();
            else if (DoAll)
                ShowInfo(TRUE);
            else if (DoRelease)
                Release(NULL);
            else if (DoRenew)
                Renew(NULL);
            else if (DoFlushdns)
                _tprintf(_T("\nSorry /flushdns is not implemented yet\n"));
            else if (DoRegisterdns)
                _tprintf(_T("\nSorry /registerdns is not implemented yet\n"));
            else if (DoDisplaydns)
                _tprintf(_T("\nSorry /displaydns is not implemented yet\n"));
            else
                Usage();
            break;
        case 3: /* Process all the options that can have 1 parameter */
            if (DoRelease)
                _tprintf(_T("\nSorry /release [adapter] is not implemented yet\n"));
                //Release(argv[2]);
            else if (DoRenew)
                _tprintf(_T("\nSorry /renew [adapter] is not implemented yet\n"));
            else if (DoShowclassid)
                _tprintf(_T("\nSorry /showclassid adapter is not implemented yet\n"));
            else if (DoSetclassid)
                _tprintf(_T("\nSorry /setclassid adapter is not implemented yet\n"));
            else
                Usage();
            break;
        case 4:  /* Process all the options that can have 2 parameters */
            if (DoSetclassid)
                _tprintf(_T("\nSorry /setclassid adapter [classid]is not implemented yet\n"));
            else
                Usage();
            break;
        default:
            Usage();
    }

    return 0;
}

