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

#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>

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
WINAPI
AddIPAddress(IPAddr Address, IPMask IpMask, DWORD IfIndex, PULONG NTEContext, PULONG NTEInstance)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
SetIpNetEntry(PMIB_IPNETROW pArpEntry)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
CreateIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    UNIMPLEMENTED
    return 0L;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
    return 0;
}


/*
 * @unimplemented
 */
DWORD WINAPI MyGetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
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

	lErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters", 0, KEY_READ, &hAdapters);
	if(lErr != ERROR_SUCCESS)
		return lErr;

	//	Determine the size of the largest name of any adapter and the number of adapters.
	lErr = RegQueryInfoKey(hAdapters, NULL, NULL, NULL, NULL, &dwAdapterLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(lErr != ERROR_SUCCESS)
	{
		RegCloseKey(hAdapters);
		return lErr;
	}
	dwAdapterLen++;			//	RegQueryInfoKey return value does not include terminating null.

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
		int n = swprintf(strTemp2, L"SYSTEM\\CurrentControlSet\\Services\\%s", strTemp1);

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
		dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "IPAddress", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpAddress, &dwSize);
		dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "SubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpMask, &dwSize);
		if(strstr(pCurrentAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != NULL)
		{	
			dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "DhcpIPAddress", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpAddress, &dwSize);
			dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "DhcpSubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->IpAddressList.IpMask, &dwSize);
		}
			//	TODO	IP_ADDR_STRING GatewayList
			//	IP_ADDR_STRING DhcpServer
		dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "DhcpServer", NULL, NULL, (BYTE*) &pCurrentAdapter->DhcpServer.IpAddress, &dwSize);
		dwSize = 16; lErr = RegQueryValueEx(hIpConfig, "DhcpSubnetMask", NULL, NULL, (BYTE*) &pCurrentAdapter->DhcpServer.IpMask, &dwSize);
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
WINAPI
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
WINAPI
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
        _tprintf(_T("GetInterfaceInfo() failed with code 0x%08X - Use FormatMessage to obtain the message string for the returned error\n"), errCode);
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
                    for (i = 0; i < pIfTable->NumAdapters, *pStr != L'\0'; pStr += wcslen(pStr) + 1) {
                        if (wcsstr(pStr, L"\\Device\\NdisWanIp") == 0) {
                            wcsncpy(pIfTable->Adapter[i].Name, pStr, MAX_ADAPTER_NAME);
                            pIfTable->Adapter[i].Index = i++;
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
 * @implemented
 */
DWORD
WINAPI
GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{
    DWORD result = ERROR_SUCCESS;
    DWORD dwSize;
    HKEY hKey;
    LONG errCode;

    if (pFixedInfo == NULL || pOutBufLen == NULL) return ERROR_INVALID_PARAMETER;

    if (*pOutBufLen < sizeof(FIXED_INFO)) {
        *pOutBufLen = sizeof(FIXED_INFO);
        return ERROR_BUFFER_OVERFLOW;
    }
    memset(pFixedInfo, 0, sizeof(FIXED_INFO));

        errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), 0, KEY_READ, &hKey);
        if (errCode == ERROR_SUCCESS) {
            dwSize = sizeof(pFixedInfo->HostName);
            errCode = RegQueryValueExA(hKey, "Hostname", NULL, NULL, (LPBYTE)&pFixedInfo->HostName, &dwSize);
            dwSize = sizeof(pFixedInfo->DomainName);
            errCode = RegQueryValueExA(hKey, "Domain", NULL, NULL, (LPBYTE)&pFixedInfo->DomainName, &dwSize);
            if (errCode != ERROR_SUCCESS) {
                dwSize = sizeof(pFixedInfo->DomainName);
                errCode = RegQueryValueExA(hKey, "DhcpDomain", NULL, NULL, (LPBYTE)&pFixedInfo->DomainName, &dwSize);
            }
            dwSize = sizeof(pFixedInfo->EnableRouting);
            errCode = RegQueryValueEx(hKey, _T("IPEnableRouter"), NULL, NULL, (LPBYTE)&pFixedInfo->EnableRouting, &dwSize);
            RegCloseKey(hKey);
        } else {
            result = ERROR_NO_DATA; // No adapter information exists for the local computer
        }

        errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters"), 0, KEY_READ, &hKey);
        if (errCode == ERROR_SUCCESS) {
            dwSize = sizeof(pFixedInfo->ScopeId);
            errCode = RegQueryValueExA(hKey, "ScopeId", NULL, NULL, (LPBYTE)&pFixedInfo->ScopeId, &dwSize);
            if (errCode != ERROR_SUCCESS) {
                dwSize = sizeof(pFixedInfo->ScopeId);
                errCode = RegQueryValueExA(hKey, "DhcpScopeId", NULL, NULL, (LPBYTE)&pFixedInfo->ScopeId, &dwSize);
            }
            dwSize = sizeof(pFixedInfo->NodeType);
            errCode = RegQueryValueEx(hKey, _T("NodeType"), NULL, NULL, (LPBYTE)&pFixedInfo->NodeType, &dwSize);
            if (errCode != ERROR_SUCCESS) {
                dwSize = sizeof(pFixedInfo->NodeType);
                errCode = RegQueryValueExA(hKey, "DhcpNodeType", NULL, NULL, (LPBYTE)&pFixedInfo->NodeType, &dwSize);
            }
            dwSize = sizeof(pFixedInfo->EnableProxy);
            errCode = RegQueryValueEx(hKey, _T("EnableProxy"), NULL, NULL, (LPBYTE)&pFixedInfo->EnableProxy, &dwSize);
            dwSize = sizeof(pFixedInfo->EnableDns);
            errCode = RegQueryValueEx(hKey, _T("EnableDNS"), NULL, NULL, (LPBYTE)&pFixedInfo->EnableDns, &dwSize);
            RegCloseKey(hKey);
        } else {
            result = ERROR_NO_DATA; // No adapter information exists for the local computer
        }

    return result;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
FlushIpNetTable(DWORD dwIfIndex)
{
    DWORD result = NO_ERROR;

    return result;
}

/* EOF */
