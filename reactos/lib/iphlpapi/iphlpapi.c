/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 IP Helper API DLL
 * FILE:        iphlpapi.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Robert Dickenson (robd@reactos.org)
 * REVISIONS:
 *   RDD August 18, 2002 Created
 */

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdlib.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include "ipprivate.h"
#include "ipregprivate.h"
#include "debug.h"
//#include "trace.h"

#ifdef __GNUC__
#define EXPORT STDCALL
#else
#define EXPORT CALLBACK
#endif

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */

/* To make the linker happy */
//VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


BOOL
EXPORT
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    //WSH_DbgPrint(MIN_TRACE, ("DllMain of iphlpapi.dll\n"));

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
AddIPAddress(IPAddr Address, IPMask IpMask, DWORD IfIndex, PULONG NTEContext, PULONG NTEInstance)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SetIpNetEntry(PMIB_IPNETROW pArpEntry)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
CreateIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
{
	LONG lErr;
	DWORD dwSize;
	DWORD dwIndex;
	BYTE* pNextMemFree = (BYTE*) pAdapterInfo;
	ULONG uUsedMemory = 0;
	PIP_ADAPTER_INFO pPrevAdapter = NULL;
	PIP_ADAPTER_INFO pCurrentAdapter = NULL;
	HKEY hAdapters;
	HKEY hAdapter;
	HKEY hIpConfig;
	wchar_t* strAdapter;
	wchar_t* strTemp1;
	wchar_t* strTemp2;
	DWORD dwAdapterLen;
	char strTemp[MAX_ADAPTER_NAME_LENGTH + 4];

	if(pAdapterInfo == NULL && pOutBufLen == NULL)
		return ERROR_INVALID_PARAMETER;
	ZeroMemory(pAdapterInfo, *pOutBufLen);

	lErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
			     L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters", 0, KEY_READ, &hAdapters);
	if(lErr != ERROR_SUCCESS)
		return lErr;

	//	Determine the size of the largest name of any adapter and the number of adapters.
	lErr = RegQueryInfoKeyW(hAdapters, NULL, NULL, NULL, NULL, &dwAdapterLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(lErr != ERROR_SUCCESS)
	{
		RegCloseKey(hAdapters);
		return lErr;
	}
	dwAdapterLen++; // RegQueryInfoKeyW return value does not include terminating null.

	strAdapter = (wchar_t*) malloc(dwAdapterLen * sizeof(wchar_t));

	//	Enumerate all adapters in SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Adapters.
	for(dwIndex = 0; ; dwIndex++)
	{
		dwSize = dwAdapterLen;			//	Reset size of the strAdapterLen buffer.
		lErr = RegEnumKeyExW(hAdapters, dwIndex, strAdapter, &dwSize, NULL, NULL, NULL, NULL);
		if(lErr == ERROR_NO_MORE_ITEMS)
			break;

		//	TODO	Skip NdisWanIP???
		if(wcsstr(strAdapter, L"NdisWanIp") != 0)
			continue;

		lErr = RegOpenKeyExW(hAdapters, strAdapter, 0, KEY_READ, &hAdapter);
		if(lErr != ERROR_SUCCESS)
			continue;

		//	Read the IpConfig value.
		lErr = RegQueryValueExW(hAdapter, L"IpConfig", NULL, NULL, NULL, &dwSize);
		if(lErr != ERROR_SUCCESS)
			continue;

		strTemp1 = (wchar_t*) malloc(dwSize);
		strTemp2 = (wchar_t*) malloc(dwSize + 35 * sizeof(wchar_t));
		lErr = RegQueryValueExW(hAdapter, L"IpConfig", NULL, NULL, (BYTE*) strTemp1, &dwSize);
		if(lErr != ERROR_SUCCESS)
		{
			free(strTemp1);
			free(strTemp2);
			continue;
		}
		swprintf(strTemp2, L"SYSTEM\\CurrentControlSet\\Services\\%s", strTemp1);

		//	Open the IpConfig key.
		lErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, strTemp2, 0, KEY_READ, &hIpConfig);
		if(lErr != ERROR_SUCCESS)
		{
			free(strTemp1);
			free(strTemp2);
			continue;
		}
		free((void*) strTemp1);
		free((void*) strTemp2);
		

		//	Fill IP_ADAPTER_INFO block.
		pCurrentAdapter = (IP_ADAPTER_INFO*) pNextMemFree;
		pNextMemFree += sizeof(IP_ADAPTER_INFO);
		uUsedMemory += sizeof(IP_ADAPTER_INFO);
		if(uUsedMemory > *pOutBufLen)
			return ERROR_BUFFER_OVERFLOW;				//	TODO	return the needed size

			//	struct _IP_ADAPTER_INFO* Next
		if(pPrevAdapter != NULL)
			pPrevAdapter->Next = pCurrentAdapter;
			//	TODO	DWORD ComboIndex
			//	char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4]
		wcstombs(strTemp, strAdapter, MAX_ADAPTER_NAME_LENGTH + 4);
		strcpy(pCurrentAdapter->AdapterName, strTemp);
			//	TODO	char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4]
			//	TODO	UINT AddressLength
			//	TODO	BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH]
			//	TODO	DWORD Index
			//	TODO	UINT Type
			//	TODO	UINT DhcpEnabled
			//	TODO	PIP_ADDR_STRING CurrentIpAddress
			//	IP_ADDR_STRING IpAddressList
		dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"IPAddress", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpAddress, &dwSize);
		dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"SubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpMask, &dwSize);
		if(strstr(pCurrentAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
		{	
			dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"DhcpIPAddress", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpAddress, &dwSize);
			dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"DhcpSubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpMask, &dwSize);
		}
			//	TODO	IP_ADDR_STRING GatewayList
			//	IP_ADDR_STRING DhcpServer
		dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"DhcpServer", NULL, NULL, (BYTE*) &pCurrentAdapter->DhcpServer.IpAddress, &dwSize);
		dwSize = 16; lErr = RegQueryValueExW(hIpConfig, L"DhcpSubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->DhcpServer.IpMask, &dwSize);
			//	TODO	BOOL HaveWins
			//	TODO	IP_ADDR_STRING PrimaryWinsServer
			//	TODO	IP_ADDR_STRING SecondaryWinsServer
			//	TODO	time_t LeaseObtained
			//	TODO	time_t LeaseExpires

		pPrevAdapter = pCurrentAdapter;
		RegCloseKey(hAdapter);
		RegCloseKey(hIpConfig);
	}

	//	Cleanup
	free(strAdapter);
	RegCloseKey(hAdapters);

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////

/*
 * @implemented
 */
DWORD
STDCALL 
GetNumberOfInterfaces(OUT PDWORD pdwNumIf)
{
    DWORD result = NO_ERROR;
    HKEY hKey;
    LONG errCode;
    int i = 0;

    if (pdwNumIf == NULL) return ERROR_INVALID_PARAMETER;
    *pdwNumIf = 0;
    errCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage", 0, KEY_READ, &hKey);
    if (errCode == ERROR_SUCCESS) {
        DWORD dwSize;
        errCode = RegQueryValueExW(hKey, L"Bind", NULL, NULL, NULL, &dwSize);
        if (errCode == ERROR_SUCCESS) {
            wchar_t* pData = (wchar_t*)malloc(dwSize * sizeof(wchar_t));
            errCode = RegQueryValueExW(hKey, L"Bind", NULL, NULL, (LPBYTE)pData, &dwSize);
            if (errCode == ERROR_SUCCESS) {
                wchar_t* pStr = pData;
                for (i = 0; *pStr != L'\0'; i++) {
                    pStr = pStr + wcslen(pStr) + 1; // next string
                }
            }
            free(pData);
        }
        RegCloseKey(hKey);
        *pdwNumIf = i;
    } else {
        result = errCode;
    }
    return result;
}


/*
 * @implemented
 */
DWORD
STDCALL 
GetInterfaceInfo(PIP_INTERFACE_INFO pIfTable, PULONG pOutBufLen)
{
    DWORD result = ERROR_SUCCESS;
    DWORD dwSize;
    DWORD dwOutBufLen;
    DWORD dwNumIf;
    HKEY hKey;
    LONG errCode;
    int i = 0;

    if ((errCode = GetNumberOfInterfaces(&dwNumIf)) != NO_ERROR) {
        _tprintf(_T("GetInterfaceInfo() failed with code 0x%08X - Use FormatMessage to obtain the message string for the returned error\n"), (int)errCode);
        return errCode;
    }
    if (dwNumIf == 0) return ERROR_NO_DATA; // No adapter information exists for the local computer
    if (pOutBufLen == NULL) return ERROR_INVALID_PARAMETER;
    dwOutBufLen = sizeof(IP_INTERFACE_INFO) + dwNumIf * sizeof(IP_ADAPTER_INDEX_MAP);
    if (*pOutBufLen < dwOutBufLen || pIfTable == NULL) {
        *pOutBufLen = dwOutBufLen;
        return ERROR_INSUFFICIENT_BUFFER;
    }
    memset(pIfTable, 0, dwOutBufLen);
    pIfTable->NumAdapters = dwNumIf - 1;
    errCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage", 0, KEY_READ, &hKey);
    if (errCode == ERROR_SUCCESS) {
            errCode = RegQueryValueExW(hKey, L"Bind", NULL, NULL, NULL, &dwSize);
            if (errCode == ERROR_SUCCESS) {
                wchar_t* pData = (wchar_t*)malloc(dwSize * sizeof(wchar_t));
                errCode = RegQueryValueExW(hKey, L"Bind", NULL, NULL, (LPBYTE)pData, &dwSize);
                if (errCode == ERROR_SUCCESS) {
                    wchar_t* pStr = pData;
                    for (i = 0; i < pIfTable->NumAdapters && *pStr != L'\0'; pStr += wcslen(pStr) + 1) {
                        if (wcsstr(pStr, L"\\Device\\NdisWanIp") == 0) {
                            wcsncpy(pIfTable->Adapter[i].Name, pStr, MAX_ADAPTER_NAME);
                            pIfTable->Adapter[i].Index = i;
							i++;
                        }
                    }

                }
                free(pData);
            }
            RegCloseKey(hKey);
    } else {
        result = errCode;
    }
    return result;
}

/*
 * EnumNameServers
 */

static void EnumNameServers( HANDLE RegHandle, PWCHAR Interface,
			     PVOID Data, EnumNameServersFunc cb ) {
  PWCHAR NameServerString = QueryRegistryValueString(RegHandle, L"NameServer");
  /* Now, count the non-empty comma separated */
  if (NameServerString) {
    DWORD ch;
    DWORD LastNameStart = 0;
    for (ch = 0; NameServerString[ch]; ch++) {
      if (NameServerString[ch] == ',') {
	if (ch - LastNameStart > 0) { /* Skip empty entries */
	  PWCHAR NameServer = malloc(sizeof(WCHAR) * (ch - LastNameStart + 1));
	  if (NameServer) {
	    memcpy(NameServer,NameServerString + LastNameStart,
		   (ch - LastNameStart) * sizeof(WCHAR));
	    NameServer[ch - LastNameStart] = 0;
	    cb( Interface, NameServer, Data );
	    free(NameServer);
	  }
	}	
	LastNameStart = ch + 1; /* The first one after the comma */
      }
    }
    if (ch - LastNameStart > 0) { /* A last name? */
      PWCHAR NameServer = malloc(sizeof(WCHAR) * (ch - LastNameStart + 1));
      memcpy(NameServer,NameServerString + LastNameStart,
	     (ch - LastNameStart) * sizeof(WCHAR));
      NameServer[ch - LastNameStart] = 0;
      cb( Interface, NameServer, Data );
      free(NameServer);
    }
    ConsumeRegValueString(NameServerString);
  }
}

/*
 * EnumInterfaces
 *
 * Call the enumeration function for each name server.
 */

static void EnumInterfaces( PVOID Data, EnumInterfacesFunc cb ) {
  HKEY RegHandle;
  HKEY ChildKeyHandle = 0;
  PWCHAR RegKeyToEnumerate = 
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
  PWCHAR ChildKeyName = 0;
  DWORD CurrentInterface;

  if (OpenChildKeyRead(HKEY_LOCAL_MACHINE,RegKeyToEnumerate,&RegHandle)) {
    return;
  }

  for (CurrentInterface = 0; TRUE; CurrentInterface++) {
    ChildKeyName = GetNthChildKeyName( RegHandle, CurrentInterface );
    if (!ChildKeyName) break;
    if (OpenChildKeyRead(RegHandle,ChildKeyName,
			 &ChildKeyHandle) == 0) {
      cb( ChildKeyHandle, ChildKeyName, Data );
      RegCloseKey( ChildKeyHandle );
    }
    ConsumeChildKeyName( ChildKeyName );
  }
}

static void CreateNameServerListEnumNamesFuncCount( PWCHAR Interface,
						    PWCHAR Server,
						    PVOID _Data ) {
  PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)_Data;
  Data->NumServers++;
}

static void CreateNameServerListEnumIfFuncCount( HANDLE RegHandle,
						 PWCHAR InterfaceName,
						 PVOID _Data ) {
  PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)_Data;
  EnumNameServers(RegHandle,InterfaceName,Data,
		  CreateNameServerListEnumNamesFuncCount);
}

static void CreateNameServerListEnumNamesFunc( PWCHAR Interface,
					       PWCHAR Server,
					       PVOID _Data ) {
  PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)_Data;
  wcstombs(Data->AddrString[Data->CurrentName].IpAddress.String,
	   Server,
	   sizeof(IP_ADDRESS_STRING));
  strcpy(Data->AddrString[Data->CurrentName].IpMask.String,"0.0.0.0");
  Data->AddrString[Data->CurrentName].Context = 0;
  if (Data->CurrentName < Data->NumServers - 1) {
    Data->AddrString[Data->CurrentName].Next = 
      &Data->AddrString[Data->CurrentName+1];
  } else
    Data->AddrString[Data->CurrentName].Next = 0;

  Data->CurrentName++;
}

static void CreateNameServerListEnumIfFunc( HANDLE RegHandle,
					    PWCHAR InterfaceName,
					    PVOID _Data ) {
  PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)_Data;
  EnumNameServers(RegHandle,InterfaceName,Data,
		  CreateNameServerListEnumNamesFunc);
}

static int CountNameServers( PNAME_SERVER_LIST_PRIVATE PrivateData ) {
  EnumInterfaces(PrivateData,CreateNameServerListEnumIfFuncCount);
  return PrivateData->NumServers;
}

static void MakeNameServerList( PNAME_SERVER_LIST_PRIVATE PrivateData ) {
  EnumInterfaces(PrivateData,CreateNameServerListEnumIfFunc);
}

/*
 * @implemented
 */
DWORD
STDCALL 
GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{
  DWORD result = ERROR_SUCCESS;
  DWORD dwSize;
  HKEY hKey;
  LONG errCode;
  NAME_SERVER_LIST_PRIVATE PrivateNSEnum = { 0 };

  CountNameServers( &PrivateNSEnum );

  if (pOutBufLen == NULL) return ERROR_INVALID_PARAMETER;

  if (*pOutBufLen < sizeof(FIXED_INFO))
  {
    *pOutBufLen = sizeof(FIXED_INFO) + 
      ((PrivateNSEnum.NumServers - 1) * sizeof(IP_ADDR_STRING));
    return ERROR_BUFFER_OVERFLOW;
  }
  if (pFixedInfo == NULL) return ERROR_INVALID_PARAMETER;
  memset(pFixedInfo, 0, sizeof(FIXED_INFO));

  errCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
			  L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_READ, &hKey);
  if (errCode == ERROR_SUCCESS)
  {
    dwSize = sizeof(pFixedInfo->HostName);
    errCode = RegQueryValueExA(hKey, "Hostname", NULL, NULL, (LPBYTE)&pFixedInfo->HostName, &dwSize);
    dwSize = sizeof(pFixedInfo->DomainName);
    errCode = RegQueryValueExA(hKey, "Domain", NULL, NULL, (LPBYTE)&pFixedInfo->DomainName, &dwSize);
    if (errCode != ERROR_SUCCESS)
    {
      dwSize = sizeof(pFixedInfo->DomainName);
      errCode = RegQueryValueExA(hKey, "DhcpDomain", NULL, NULL, (LPBYTE)&pFixedInfo->DomainName, &dwSize);
    }
    dwSize = sizeof(pFixedInfo->EnableRouting);
    errCode = RegQueryValueExW(hKey, L"IPEnableRouter", NULL, NULL, (LPBYTE)&pFixedInfo->EnableRouting, &dwSize);
    RegCloseKey(hKey);

    /* Get the number of name servers */
    PIP_ADDR_STRING AddressAfterFixedInfo;
    AddressAfterFixedInfo = (PIP_ADDR_STRING)&pFixedInfo[1];
    DWORD NumberOfServersAllowed = 0, CurrentServer = 0;

    while( &AddressAfterFixedInfo[NumberOfServersAllowed] < 
	   (PIP_ADDR_STRING)(((PCHAR)pFixedInfo) + *pOutBufLen) ) 
      NumberOfServersAllowed++;

    NumberOfServersAllowed++; /* One struct is built in */

    /* Since the first part of the struct is built in, we have to do some
       fiddling */
    PrivateNSEnum.AddrString = 
      malloc(NumberOfServersAllowed * sizeof(IP_ADDR_STRING));
    if (PrivateNSEnum.NumServers > NumberOfServersAllowed) 
      PrivateNSEnum.NumServers = NumberOfServersAllowed;
    MakeNameServerList( &PrivateNSEnum );

    /* Now we have the name servers, place the first one in the struct,
       and follow it with the rest */
    if (!PrivateNSEnum.NumServers)
      RtlZeroMemory( &pFixedInfo->DnsServerList, 
		     sizeof(pFixedInfo->DnsServerList) );
    else
      memcpy( &pFixedInfo->DnsServerList, &PrivateNSEnum.AddrString[0],
	      sizeof(PrivateNSEnum.AddrString[0]) );

    if (PrivateNSEnum.NumServers > 1) {
      pFixedInfo->CurrentDnsServer = &pFixedInfo->DnsServerList;
      memcpy( &AddressAfterFixedInfo[0],
	      &PrivateNSEnum.AddrString[1],
	      sizeof(IP_ADDR_STRING) * (PrivateNSEnum.NumServers - 1) );
    } else if (PrivateNSEnum.NumServers == 0) {
      pFixedInfo->CurrentDnsServer = &pFixedInfo->DnsServerList;
    } else {
      pFixedInfo->CurrentDnsServer = 0;
    }

    for( CurrentServer = 0; 
	 PrivateNSEnum.NumServers &&
	   CurrentServer < PrivateNSEnum.NumServers - 1;
	 CurrentServer++ ) {
      pFixedInfo->CurrentDnsServer->Next = &AddressAfterFixedInfo[CurrentServer];
      pFixedInfo->CurrentDnsServer = &AddressAfterFixedInfo[CurrentServer];
      pFixedInfo->CurrentDnsServer->Next = 0;
    }

    /* For now, set the first server as the current server, if there are any */
    if( PrivateNSEnum.NumServers ) {
      pFixedInfo->CurrentDnsServer = &pFixedInfo->DnsServerList;
    }

    free(PrivateNSEnum.AddrString);
  }
  else
  {
    WSH_DbgPrint( MIN_TRACE,
		  ("Open Tcpip parameters key: error %08x\n", errCode ));
    result = ERROR_NO_DATA; // No adapter information exists for the local computer
  }

  errCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", 0, KEY_READ, &hKey);
  if (errCode == ERROR_SUCCESS)
  {
    dwSize = sizeof(pFixedInfo->ScopeId);
    errCode = RegQueryValueExA(hKey, "ScopeId", NULL, NULL, (LPBYTE)&pFixedInfo->ScopeId, &dwSize);
    if (errCode != ERROR_SUCCESS)
    {
      dwSize = sizeof(pFixedInfo->ScopeId);
      errCode = RegQueryValueExA(hKey, "DhcpScopeId", NULL, NULL, (LPBYTE)&pFixedInfo->ScopeId, &dwSize);
    }
    dwSize = sizeof(pFixedInfo->NodeType);
    errCode = RegQueryValueExW(hKey, L"NodeType", NULL, NULL, (LPBYTE)&pFixedInfo->NodeType, &dwSize);
    if (errCode != ERROR_SUCCESS)
    {
      dwSize = sizeof(pFixedInfo->NodeType);
      errCode = RegQueryValueExA(hKey, "DhcpNodeType", NULL, NULL, (LPBYTE)&pFixedInfo->NodeType, &dwSize);
    }
    dwSize = sizeof(pFixedInfo->EnableProxy);
    errCode = RegQueryValueExW(hKey, L"EnableProxy", NULL, NULL, (LPBYTE)&pFixedInfo->EnableProxy, &dwSize);
    dwSize = sizeof(pFixedInfo->EnableDns);
    errCode = RegQueryValueExW(hKey, L"EnableDNS", NULL, NULL, (LPBYTE)&pFixedInfo->EnableDns, &dwSize);
    RegCloseKey(hKey);
  }
  else
  {
    WSH_DbgPrint(MIN_TRACE,
		 ("iphlpapi: Ignoring lack of netbios data for now.\n"));
#if 0
    result = ERROR_NO_DATA; // No adapter information exists for the local computer
#endif
  }

  return result;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetTcpStatistics(PMIB_TCPSTATS pStats)
{
    DWORD result = NO_ERROR;

    result = ERROR_NO_DATA;

    return result;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetTcpTable(PMIB_TCPTABLE pTcpTable, PDWORD pdwSize, BOOL bOrder)
{
    DWORD result = NO_ERROR;

    result = ERROR_NO_DATA;

    return result;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetUdpStatistics(PMIB_UDPSTATS pStats)
{
    DWORD result = NO_ERROR;

    result = ERROR_NO_DATA;

    return result;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
GetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
    DWORD result = NO_ERROR;

    result = ERROR_NO_DATA;

    return result;
}


/*
 * @unimplemented
 */
DWORD
STDCALL 
FlushIpNetTable(DWORD dwIfIndex)
{
    DWORD result = NO_ERROR;

    return result;
}

/******************************************************************
 *    GetIfEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIfRow [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD
STDCALL  
GetIfEntry(PMIB_IFROW pIfRow)
{
    DWORD result = NO_ERROR;

    return result;
}


/******************************************************************
 *    GetIfTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIfTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD
STDCALL 
GetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder)
{
    DWORD result = NO_ERROR;

    return result;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize,
 BOOL bOrder)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize,
 BOOL bOrder)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable,
 PULONG pdwSize, BOOL bOrder)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpStatistics(PMIB_IPSTATS pStats)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpStatisticsEx(PMIB_IPSTATS pStats, DWORD dwFamily)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIcmpStatistics(PMIB_ICMP pStats)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetTcpStatisticsEx(PMIB_TCPSTATS pStats, DWORD dwFamily)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetUdpStatisticsEx(PMIB_UDPSTATS pStats, DWORD dwFamily)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetIfEntry(PMIB_IFROW pIfRow)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL DeleteIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetIpStatistics(PMIB_IPSTATS pIpStats)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetIpTTL(UINT nTTL)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL CreateIpNetEntry(PMIB_IPNETROW pArpEntry)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL DeleteIpNetEntry(PMIB_IPNETROW pArpEntry)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL CreateProxyArpEntry(DWORD dwAddress, DWORD dwMask,
 DWORD dwIfIndex)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL DeleteProxyArpEntry(DWORD dwAddress, DWORD dwMask,
 DWORD dwIfIndex)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetTcpEntry(PMIB_TCPROW pTcpRow)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetUniDirectionalAdapterInfo(
 PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pIPIfInfo, PULONG dwOutBufLen)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetBestInterface(IPAddr dwDestAddr, PDWORD pdwBestIfIndex)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr,
 PMIB_IPFORWARDROW   pBestRoute)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL NotifyAddrChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL NotifyRouteChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL DeleteIPAddress(ULONG NTEContext)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetPerAdapterInfo(ULONG IfIndex,
 PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL IpReleaseAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL IpRenewAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL SendARP(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr,
 PULONG  PhyAddrLen)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetRTTAndHopCount(IPAddr DestIpAddress, PULONG HopCount,
 ULONG  MaxHops, PULONG RTT)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetFriendlyIfIndex(DWORD IfIndex)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL EnableRouter(HANDLE* pHandle, OVERLAPPED* pOverlapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL UnenableRouter(OVERLAPPED* pOverlapped, LPDWORD lpdwEnableCount)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIcmpStatisticsEx(PMIB_ICMP_EX pStats,DWORD dwFamily)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL NhpAllocateAndGetInterfaceInfoFromStack(IP_INTERFACE_NAME_INFO **ppTable,PDWORD pdwCount,BOOL bOrder,HANDLE hHeap,DWORD dwFlags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetBestInterfaceEx(struct sockaddr *pDestAddr,PDWORD pdwBestIfIndex)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
BOOL STDCALL CancelIPChangeNotify(LPOVERLAPPED notifyOverlapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
PIP_ADAPTER_ORDER_MAP STDCALL GetAdapterOrderMap(VOID)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetAdaptersAddresses(ULONG Family,DWORD Flags,PVOID Reserved,PIP_ADAPTER_ADDRESSES pAdapterAddresses,PULONG pOutBufLen)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL DisableMediaSense(HANDLE *pHandle,OVERLAPPED *pOverLapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL RestoreMediaSense(OVERLAPPED* pOverlapped,LPDWORD lpdwEnableCount)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetIpErrorString(IP_STATUS ErrorCode,PWCHAR Buffer,PDWORD Size)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
HANDLE STDCALL  IcmpCreateFile(
    VOID
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
HANDLE STDCALL  Icmp6CreateFile(
    VOID
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
BOOL STDCALL  IcmpCloseHandle(
    HANDLE  IcmpHandle
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL  IcmpSendEcho(
    HANDLE                 IcmpHandle,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL 
IcmpSendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    FARPROC                  ApcRoutine,
    PVOID                    ApcContext,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL 
Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    FARPROC                  ApcRoutine,
    PVOID                    ApcContext,
    struct sockaddr_in6     *SourceAddress,
    struct sockaddr_in6     *DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
IcmpParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
Icmp6ParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetIfTableFromStack(PMIB_IFTABLE *ppIfTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetIpAddrTableFromStack(PMIB_IPADDRTABLE *ppIpAddrTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *
 ppIpForwardTable, BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetTcpTableFromStack(PMIB_TCPTABLE *ppTcpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
    UNIMPLEMENTED
    return 0L;
}
