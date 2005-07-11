/* $Id: ipconfig.c
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Ipconfig utility
 * FILE:            apps/utils/net/ipconfig/ipconfig.c
 * PURPOSE:         Show and set network interface IP parameters
 */ 
/*
 *  History:
 *
 *      15/8/2002 (Robert Dickenson <robd@reactos.org>)
 *          Original version (PUBLIC DOMAIN and NO WARRANTY)
 *
 *      26/6/2005 (Tim Jobling <tjob800@yahoo.co.uk>)
 *          Relicense to GPL. 
 *          Display NodeType with meaningfull Human readable names.
 *          Exclusively use TCHAR strings.
 *          Display Physical Address, DHCP enabled state, IP Addresses/Netmasks,
 *          Default Gateway, DHCP server and DHCP Lease times.
 *          Parse command line options.
 *          Default to only showing the IP/SM/DG is no options specified
 *          Handel option: /All and /?
 *          Display message about all unimplemented options.
 *          Changed C++ style commenting to C style
 *   
*/ 
 
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>

#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>

#ifdef _DEBUG
#include "trace.h"
#endif

/* imported from iphlpapi.dll

GetAdapterOrderMap
GetInterfaceInfo
GetIpStatsFromStack
NhGetInterfaceNameFromGuid
NhpAllocateAndGetInterfaceInfoFromStack

 */

static TCHAR* GetNodeTypeName(UINT nNodeType)
{
    switch (nNodeType) {
    case 1:  return _T("Broadcast");
    case 2:  return _T("Peer To Peer");
    case 4:  return _T("Mixed");
    case 8:  return _T("Hybrid");
    default: return _T("unknown");
    }
}

static TCHAR* GetInterfaceTypeName(UINT nInterfaceType)
{
    switch (nInterfaceType) {
    case MIB_IF_TYPE_OTHER:     return _T("Other");
    case MIB_IF_TYPE_ETHERNET:  return _T("Ethernet");
    case MIB_IF_TYPE_TOKENRING: return _T("Token Ring");
    case MIB_IF_TYPE_FDDI:      return _T("FDDI");
    case MIB_IF_TYPE_PPP:       return _T("PPP");
    case MIB_IF_TYPE_LOOPBACK:  return _T("Loopback");
    case MIB_IF_TYPE_SLIP:      return _T("SLIP");
    default: return _T("unknown");
    }
}

void PrintPhysicalAddr(PBYTE Addr, UINT len)
{
    UINT i=0;
    for (i=0; i<len; i++)
    {
        _tprintf(_T("%02X"), Addr[i]);
        if ((i+1)<len)
            _tprintf(_T("-"));
    }
    _tprintf(_T("\n"));
}

static void ShowNetworkFixedInfo()
{
    FIXED_INFO* pFixedInfo = NULL;
    ULONG OutBufLen = 0;
    DWORD result;

    result = GetNetworkParams(NULL, &OutBufLen);
    if (result == ERROR_BUFFER_OVERFLOW) {
        pFixedInfo = (FIXED_INFO*)malloc(OutBufLen);
        if (!pFixedInfo) {
            _tprintf(_T("ERROR: failed to allocate 0x%08lX bytes of memory\n"), OutBufLen);
            return;
        }
    } else {
        _tprintf(_T("ERROR: GetNetworkParams() failed to report required buffer size.\n"));
        return;
    }

    result = GetNetworkParams(pFixedInfo, &OutBufLen);
    if (result == ERROR_SUCCESS) {
        IP_ADDR_STRING* pIPAddr;

             _tprintf(_T("\tHostName. . . . . . . . . . . : %s\n"),  pFixedInfo->HostName);
             _tprintf(_T("\tDomainName. . . . . . . . . . : %s\n"),  pFixedInfo->DomainName);

             _tprintf(_T("\tDNS Servers . . . . . . . . . : %s\n"),  pFixedInfo->DnsServerList.IpAddress.String);
             pIPAddr = pFixedInfo->DnsServerList.Next;
             while (pIPAddr) {
                 _tprintf(_T("\t\t\t\t      : %s\n"),  pIPAddr->IpAddress.String);
                 pIPAddr = pIPAddr->Next;
             }

        _tprintf(_T("\tNodeType. . . . . . . . . . . : %d (%s)\n"), pFixedInfo->NodeType, GetNodeTypeName(pFixedInfo->NodeType));
        _tprintf(_T("\tScopeId . . . . . . . . . . . : %s\n"),  pFixedInfo->ScopeId);
        _tprintf(_T("\tEnableRouting . . . . . . . . : %s\n"), pFixedInfo->EnableRouting ? _T("yes") : _T("no"));
        _tprintf(_T("\tEnableProxy . . . . . . . . . : %s\n"), pFixedInfo->EnableProxy ? _T("yes") : _T("no"));
        _tprintf(_T("\tEnableDns . . . . . . . . . . : %s\n"), pFixedInfo->EnableDns ? _T("yes") : _T("no"));
        _tprintf(_T("\n"));
/*        
        _tprintf(_T("\n"),);
        _tprintf(_T("GetNetworkParams() returned with %d\n"), pIfTable->NumAdapters);

      _tprintf(_T("\tConnection specific DNS suffix: %s\n"), pFixedInfo->EnableDns ? _T("yes") : _T("no"));
*/
    } else {
        switch (result) {
        case ERROR_BUFFER_OVERFLOW:
            _tprintf(_T("The buffer size indicated by the pOutBufLen parameter is too small to hold the adapter information. The pOutBufLen parameter points to the required size\n"));
            break;
        case ERROR_INVALID_PARAMETER:
            _tprintf(_T("The pOutBufLen parameter is NULL, or the calling process does not have read/write access to the memory pointed to by pOutBufLen, or the calling process does not have write access to the memory pointed to by the pAdapterInfo parameter\n"));
            break;
        case ERROR_NO_DATA:
            _tprintf(_T("No adapter information exists for the local computer\n"));
            break;
        case ERROR_NOT_SUPPORTED:
            _tprintf(_T("This function is not supported on the operating system in use on the local system\n"));
            break;
        default:
            _tprintf(_T("0x%08lX - Use FormatMessage to obtain the message string for the returned error\n"), result);
            break;
        }
    }
}

static void ShowNetworkInterfaces()
{
    IP_INTERFACE_INFO* pIfTable = NULL;
    DWORD result;
    DWORD dwNumIf;
    DWORD dwOutBufLen = 0;

    if ((result = GetNumberOfInterfaces(&dwNumIf)) != NO_ERROR) {
        _tprintf(_T("GetNumberOfInterfaces() failed with code 0x%08lX - Use FormatMessage to obtain the message string for the returned error\n"), result);
        return;
    } else {
        _tprintf(_T("GetNumberOfInterfaces() returned %lu\n"), dwNumIf);
    }

    result = GetInterfaceInfo(pIfTable, &dwOutBufLen);
/*
    dwOutBufLen = sizeof(IP_INTERFACE_INFO) + dwNumIf * sizeof(IP_ADAPTER_INDEX_MAP);
    _tprintf(_T("GetNumberOfInterfaces() returned %d, dwOutBufLen %d\n"), dwNumIf, dwOutBufLen);
    _tprintf(_T("sizeof(IP_INTERFACE_INFO) %d, sizeof(IP_ADAPTER_INDEX_MAP) %d\n"), sizeof(IP_INTERFACE_INFO), sizeof(IP_ADAPTER_INDEX_MAP));
*/
    pIfTable = (IP_INTERFACE_INFO*)malloc(dwOutBufLen);
    if (!pIfTable) {
        _tprintf(_T("ERROR: failed to allocate 0x%08lX bytes of memory\n"), dwOutBufLen);
        return;
    }
/*
typedef struct _IP_ADAPTER_INDEX_MAP {
  ULONG Index;                     // adapter index
  WCHAR Name[MAX_ADAPTER_NAME];    // name of the adapter
} IP_ADAPTER_INDEX_MAP, * PIP_ADAPTER_INDEX_MAP;

typedef struct _IP_INTERFACE_INFO {
  LONG NumAdapters;                 // number of adapters in array
  IP_ADAPTER_INDEX_MAP Adapter[1];  // adapter indices and names
} IP_INTERFACE_INFO,*PIP_INTERFACE_INFO;
 */
    result = GetInterfaceInfo(pIfTable, &dwOutBufLen);
    if (result == NO_ERROR) {
        UINT i;
        _tprintf(_T("GetInterfaceInfo() returned with %ld adaptor entries\n"), pIfTable->NumAdapters);
        for (i = 0; i < pIfTable->NumAdapters; i++) {
           wprintf(L"[%d] %s\n", i + 1, pIfTable->Adapter[i].Name);
           /*wprintf(L"[%d] %s\n", pIfTable->Adapter[i].Index, pIfTable->Adapter[i].Name);*/

/*HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces\Tcpip_{DB0E61C1-3498-4C5F-B599-59CDE8A1E357}*/
        }
    } else {
        switch (result) {
        case ERROR_INVALID_PARAMETER:
            _tprintf(_T("The dwOutBufLen parameter is NULL, or GetInterfaceInterface is unable to write to the memory pointed to by the dwOutBufLen parameter\n"));
            break;
        case ERROR_INSUFFICIENT_BUFFER:
            _tprintf(_T("The buffer pointed to by the pIfTable parameter is not large enough. The required size is returned in the DWORD variable pointed to by the dwOutBufLen parameter\n"));
            _tprintf(_T("\tdwOutBufLen: %lu\n"), dwOutBufLen);
            break;
        case ERROR_NOT_SUPPORTED:
            _tprintf(_T("This function is not supported on the operating system in use on the local system\n"));
            break;
        default:
            _tprintf(_T("0x%08lX - Use FormatMessage to obtain the message string for the returned error\n"), result);
            break;
        }
    }
    free(pIfTable);
}

/*
typedef struct _IP_ADAPTER_INFO {
  struct _IP_ADAPTER_INFO* Next;
  DWORD ComboIndex;
  char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
1  char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
  UINT AddressLength;
2  BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
  DWORD Index;
  UINT Type;
3  UINT DhcpEnabled;
5  PIP_ADDR_STRING CurrentIpAddress;
  IP_ADDR_STRING IpAddressList;
7  IP_ADDR_STRING GatewayList;
8  IP_ADDR_STRING DhcpServer;
  BOOL HaveWins;
  IP_ADDR_STRING PrimaryWinsServer;
  IP_ADDR_STRING SecondaryWinsServer;
a  time_t LeaseObtained;
b  time_t LeaseExpires;
} IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;
 */
/*
Ethernet adapter VMware Virtual Ethernet Adapter (Network Address Translation (NAT) for VMnet8):

        Connection-specific DNS Suffix  . :
1        Description . . . . . . . . . . . : VMware Virtual Ethernet Adapter (Network Address Translation (NAT) for VMnet8)
2        Physical Address. . . . . . . . . : 00-50-56-C0-00-08
3        DHCP Enabled. . . . . . . . . . . : Yes
        Autoconfiguration Enabled . . . . : Yes
5        IP Address. . . . . . . . . . . . : 192.168.136.1
        Subnet Mask . . . . . . . . . . . : 255.255.255.0
7        Default Gateway . . . . . . . . . :
8        DHCP Server . . . . . . . . . . . : 192.168.136.254
        DNS Servers . . . . . . . . . . . :
a        Lease Obtained. . . . . . . . . . : Monday, 30 December 2002 5:56:53 PM
b        Lease Expires . . . . . . . . . . : Monday, 30 December 2002 6:26:53 PM
 */
static void ShowAdapterInfo(BOOL ShowAll)
{
    IP_ADAPTER_INFO* pAdaptorInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;
    PIP_ADDR_STRING pIpAddrString;
    struct tm *LeaseTime;


    if (ShowAll)
    {
        _tprintf(_T("\nAdaptor Information\t\n"));
    }

    pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, sizeof(IP_ADAPTER_INFO));
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    if (ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen)) {
        GlobalFree(pAdaptorInfo);
        pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, ulOutBufLen);
    }
    if ((dwRetVal = GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen))) {
        _tprintf(_T("Call to GetAdaptersInfo failed. Return Value: 0x%08lx\n"), dwRetVal);
    } else {
        while (pAdaptorInfo) {

            /* print the type of interface before the Name of it */
            _tprintf(_T("\n%s Adapter %s:\n\n"), GetInterfaceTypeName(pAdaptorInfo->Type), pAdaptorInfo->AdapterName);

            if (ShowAll)
            {
                _tprintf(_T("\tDescription. . . . . . : %s\n"), pAdaptorInfo->Description);

                /* print the Physical address to the screen*/
                _tprintf(_T("\tPhysical Address . . . : "));
                PrintPhysicalAddr(pAdaptorInfo->Address, pAdaptorInfo->AddressLength);

                /* Now the DHCP state */
                _tprintf(_T("\tDHCP Enabled . . . . . : %s\n"), pAdaptorInfo->DhcpEnabled ? _T("Yes") : _T("No"));
            }
            
            /* IP Addresses/Netmasks, there may be more than one */
            pIpAddrString = &pAdaptorInfo->IpAddressList;

            do{
                _tprintf(_T("\tIP Address . . . . . . : %s\n"), pIpAddrString->IpAddress.String);
                _tprintf(_T("\tSubnet Mask. . . . . . : %s\n"), pIpAddrString->IpMask.String);
                pIpAddrString = pIpAddrString->Next;
            }while (pIpAddrString!=NULL);

            /* Default Gateway */
            pIpAddrString = &pAdaptorInfo->GatewayList;
            _tprintf(_T("\tDefault Gateway. . . . : %s\n"), pIpAddrString->IpAddress.String);

            /* Print some stuff that is only relevant it dhcp is enabled */
            if((pAdaptorInfo->DhcpEnabled)&&(ShowAll))
            {
                /* Display the DHCP server address */
                pIpAddrString = &pAdaptorInfo->DhcpServer;
                _tprintf(_T("\tDHCP Server. . . . . . : %s\n"), pIpAddrString->IpAddress.String);

                /* Display the Lease times*/
                LeaseTime = localtime(&pAdaptorInfo->LeaseObtained);
                _tprintf(_T("\tLease Obtained . . . . : %s"), asctime(LeaseTime));

                LeaseTime = localtime(&pAdaptorInfo->LeaseExpires);
                _tprintf(_T("\tLease Expieres . . . . : %s"), asctime(LeaseTime));

            }

            pAdaptorInfo = pAdaptorInfo->Next;
        }
    }
}

const TCHAR szUsage[] = { _T("USAGE:\n" \
    "   ipconfig [/? | /all | /release [adapter] | /renew [adapter]\n" \
    "            | /flushdns | /registerdns\n" \
    "            | /showclassid adapter\n" \
    "            | /showclassid adapter\n" \
    "            | /setclassid adapter [classidtoset] ]\n" \
    "\n" \
    "adapter    Full name or pattern with '*' and '?' to 'match',\n" \
    "           * matches any character, ? matches one character.\n" \
    "   Options\n" \
    "       /?           Display this help message.\n" \
    "       /all         Display full configuration information.\n" \
    "       /release     Release the IP address for the specified adapter.\n" \
    "       /renew       Renew the IP address for the specified adapter.\n" \
    "       /flushdns    Purges the DNS Resolver cache.\n" \
    "       /registerdns Refreshes all DHCP leases and re-registers DNS names\n" \
    "       /displaydns  Display the contents of the DNS Resolver Cache.\n" \
    "       /showclassid Displays all the dhcp class IDs allowed for adapter.\n" \
    "       /setclassid  Modifies the dhcp class id.\n" \
    "\n" \
    "The default is to display only the IP address, subnet mask and\n" \
    "default gateway for each adapter bound to TCP/IP.\n")
};
/*
    "\n" \
    "For Release and Renew, if no adapter name is specified, then the IP address\n" \
    "leases for all adapters bound to TCP/IP will be released or renewed.\n" \
    "\n" \
    "For SetClassID, if no class id is specified, then the classid is removed.\n" \
    "\n" \
    "Examples:\n" \
    "    > ipconfig                       ... Show information.\n" \
    "    > ipconfig /all                  ... Show detailed information\n" \
    "    > ipconfig /renew                ... renew all adapaters\n" \
    "    > ipconfig /renew EL*            ... renew adapters named EL....\n" \
    "    > ipconfig /release *ELINK?21*   ... release all matching adapters,\n" \
                                         eg. ELINK-21, myELELINKi21adapter.\n"
 */

static void usage(void)
{
    _fputts(szUsage, stderr);
}


int _tmain(int argc, TCHAR *argv[])
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

    _tprintf(_T("\nReactOS IP Configuration\n\n"));

    /* 
       Parse command line for options we have been given. 
    */
    if ( ((argc > 1))&&((argv[1][0]=='/')||(argv[1][0]=='-')) )
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
        else if( ! _tcsnicmp( &argv[1][1], _T("REGISTERDNS"), _tcslen(&argv[1][1]) ))
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
        case 1:  /* Default behaviour if options are given specified*/
            ShowAdapterInfo(FALSE);
            break;
        case 2:  /* Process all the options that take no paramiters */
            if ( DoUsage)
                usage();
            else if ( DoAll)
            {
                ShowNetworkFixedInfo();
                ShowNetworkInterfaces();
                ShowAdapterInfo(TRUE);
            }
            else if ( DoRelease)
                printf("\nSorry /Release is not implemented yet\n");
            else if ( DoRenew)
                printf("\nSorry /Renew is not implemented yet\n");
            else if ( DoFlushdns)
                printf("\nSorry /Flushdns is not implemented yet\n");
            else if ( DoRegisterdns)
                printf("\nSorry /Registerdns is not implemented yet\n");
            else if ( DoDisplaydns)
                printf("\nSorry /Displaydns is not implemented yet\n");
            else
                usage();
            break;
        case 3: /* Process all the options that can have 1 paramiters */
            if ( DoRelease)
                printf("\nSorry /Release is not implemented yet\n");
            else if ( DoRenew)
                printf("\nSorry /Renew is not implemented yet\n");
            else if ( DoShowclassid)
                printf("\nSorry /Showclassid is not implemented yet\n");
            else
                usage();
            break;
        case 4:  /* Process all the options that can have 2 paramiters */
            if ( DoSetclassid)
                printf("\nSorry /Setclassid is not implemented yet\n");
            else
                usage();
            break;
        default:
            usage();
    }

    return 0;
}
