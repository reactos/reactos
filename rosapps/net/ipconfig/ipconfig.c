/*
 * ipconfig - display IP stack parameters.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Robert Dickenson <robd@reactos.org>, August 15, 2002.
 */
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>

#ifdef __GNUC__
#undef WINAPI
#define WINAPI
#endif

#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>

#ifdef _DEBUG
#include "trace.h"
#endif

////////////////////////////////////////////////////////////////////////////////

/* imported from iphlpapi.dll

GetAdapterOrderMap
GetInterfaceInfo
GetIpStatsFromStack
NhGetInterfaceNameFromGuid
NhpAllocateAndGetInterfaceInfoFromStack

 */

static TCHAR* GetNodeTypeName(int nNodeType)
{
    switch (nNodeType) {
    case 0:  return _T("zero");
    case 1:  return _T("one");
    case 2:  return _T("two");
    case 3:  return _T("three");
    case 4:  return _T("mixed");
    default: return _T("unknown");
    }
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
            _tprintf(_T("ERROR: failed to allocate 0x%08X bytes of memory\n"), OutBufLen);
            return;
        }
    } else {
        _tprintf(_T("ERROR: GetNetworkParams() failed to report required buffer size.\n"));
        return;
    }

    result = GetNetworkParams(pFixedInfo, &OutBufLen);
    if (result == ERROR_SUCCESS) {
        IP_ADDR_STRING* pIPAddr;

             printf("\tHostName. . . . . . . . . . . : %s\n",  pFixedInfo->HostName);
             printf("\tDomainName. . . . . . . . . . : %s\n",  pFixedInfo->DomainName);
//
             printf("\tDNS Servers . . . . . . . . . : %s\n",  pFixedInfo->DnsServerList.IpAddress.String);
             pIPAddr = pFixedInfo->DnsServerList.Next;
             while (pIPAddr) {
                 printf("\t\t\t\t      : %s\n",  pIPAddr->IpAddress.String);
                 pIPAddr = pIPAddr->Next;
             }
//
        _tprintf(_T("\tNodeType. . . . . . . . . . . : %d (%s)\n"), pFixedInfo->NodeType, GetNodeTypeName(pFixedInfo->NodeType));
             printf("\tScopeId . . . . . . . . . . . : %s\n",  pFixedInfo->ScopeId);
        _tprintf(_T("\tEnableRouting . . . . . . . . : %s\n"), pFixedInfo->EnableRouting ? _T("yes") : _T("no"));
        _tprintf(_T("\tEnableProxy . . . . . . . . . : %s\n"), pFixedInfo->EnableProxy ? _T("yes") : _T("no"));
        _tprintf(_T("\tEnableDns . . . . . . . . . . : %s\n"), pFixedInfo->EnableDns ? _T("yes") : _T("no"));
        _tprintf(_T("\n"));
        //_tprintf(_T("\n"),);
        //_tprintf(_T("GetNetworkParams() returned with %d\n"), pIfTable->NumAdapters);

//      _tprintf(_T("\tConnection specific DNS suffix: %s\n"), pFixedInfo->EnableDns ? _T("yes") : _T("no"));

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
            _tprintf(_T("0x%08X - Use FormatMessage to obtain the message string for the returned error\n"), result);
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
        _tprintf(_T("GetNumberOfInterfaces() failed with code 0x%08X - Use FormatMessage to obtain the message string for the returned error\n"), result);
        return;
    } else {
        _tprintf(_T("GetNumberOfInterfaces() returned %d\n"), dwNumIf);
    }

    result = GetInterfaceInfo(pIfTable, &dwOutBufLen);
//    dwOutBufLen = sizeof(IP_INTERFACE_INFO) + dwNumIf * sizeof(IP_ADAPTER_INDEX_MAP);
//    _tprintf(_T("GetNumberOfInterfaces() returned %d, dwOutBufLen %d\n"), dwNumIf, dwOutBufLen);
//    _tprintf(_T("sizeof(IP_INTERFACE_INFO) %d, sizeof(IP_ADAPTER_INDEX_MAP) %d\n"), sizeof(IP_INTERFACE_INFO), sizeof(IP_ADAPTER_INDEX_MAP));

    pIfTable = (IP_INTERFACE_INFO*)malloc(dwOutBufLen);
    if (!pIfTable) {
        _tprintf(_T("ERROR: failed to allocate 0x%08X bytes of memory\n"), dwOutBufLen);
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
        int i;
        _tprintf(_T("GetInterfaceInfo() returned with %d adaptor entries\n"), pIfTable->NumAdapters);
        for (i = 0; i < pIfTable->NumAdapters; i++) {
           wprintf(L"[%d] %s\n", i + 1, pIfTable->Adapter[i].Name);
           //wprintf(L"[%d] %s\n", pIfTable->Adapter[i].Index, pIfTable->Adapter[i].Name);

//  \DEVICE\TCPIP_{DB0E61C1-3498-4C5F-B599-59CDE8A1E357}
//  \DEVICE\TCPIP_{BD445697-0945-4591-AE7F-2AB0F383CA87}
//  \DEVICE\TCPIP_{6D87DC08-6BC5-4E78-AB5F-18CAB785CFFE}

//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces\Tcpip_{DB0E61C1-3498-4C5F-B599-59CDE8A1E357}
        }
    } else {
        switch (result) {
        case ERROR_INVALID_PARAMETER:
            _tprintf(_T("The dwOutBufLen parameter is NULL, or GetInterfaceInterface is unable to write to the memory pointed to by the dwOutBufLen parameter\n"));
            break;
        case ERROR_INSUFFICIENT_BUFFER:
            _tprintf(_T("The buffer pointed to by the pIfTable parameter is not large enough. The required size is returned in the DWORD variable pointed to by the dwOutBufLen parameter\n"));
            _tprintf(_T("\tdwOutBufLen: %d\n"), dwOutBufLen);
            break;
        case ERROR_NOT_SUPPORTED:
            _tprintf(_T("This function is not supported on the operating system in use on the local system\n"));
            break;
        default:
            _tprintf(_T("0x%08X - Use FormatMessage to obtain the message string for the returned error\n"), result);
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
static void ShowAdapterInfo()
{
    IP_ADAPTER_INFO* pAdaptorInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;

    _tprintf(_T("\nAdaptor Information\t\n"));
    pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, sizeof(IP_ADAPTER_INFO));
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    if (ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen)) {
        GlobalFree(pAdaptorInfo);
        pAdaptorInfo = (IP_ADAPTER_INFO*)GlobalAlloc(GPTR, ulOutBufLen);
    }
    if (dwRetVal = GetAdaptersInfo(pAdaptorInfo, &ulOutBufLen)) {
        _tprintf(_T("Call to GetAdaptersInfo failed. Return Value: %08x\n"), dwRetVal);
    } else {
        while (pAdaptorInfo) {
            printf("  AdapterName: %s\n", pAdaptorInfo->AdapterName);
            printf("  Description: %s\n", pAdaptorInfo->Description);
            pAdaptorInfo = pAdaptorInfo->Next;
        }
    }
}

const char szUsage[] = { "USAGE:\n" \
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
    "default gateway for each adapter bound to TCP/IP.\n"
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
	fputs(szUsage, stderr);
}


int main(int argc, char *argv[])
{
    // 10.0.0.100    // As of build 0.0.20 this is hardcoded in the ip stack

    if (argc > 1) {
        usage();
        return 1;
    }
    _tprintf(_T("ReactOS IP Configuration\n"));
    ShowNetworkFixedInfo();
    ShowNetworkInterfaces();
    ShowAdapterInfo();
	return 0;
}
