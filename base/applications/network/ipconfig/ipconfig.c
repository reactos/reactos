/*
 * PROJECT:     ReactOS ipconfig utility
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Display IP info for net adapters
 * COPYRIGHT:   Copyright 2005-2006 Ged Murphy <gedmurphy@gmail.com>
 */
/*
 * TODO:
 * fix renew / release
 * implement registerdns, showclassid, setclassid
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h>
#include <winreg.h>
#include <winnls.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <iphlpapi.h>
#include <ndk/rtlfuncs.h>
#include <inaddr.h>
#include <windns.h>
#include <windns_undoc.h>
#include <strsafe.h>
#include <conutils.h>

#include "resource.h"

typedef struct _RECORDTYPE
{
    WORD wRecordType;
    LPTSTR pszRecordName;
} RECORDTYPE, *PRECORDTYPE;

#define GUID_LEN 40

HINSTANCE hInstance;
HANDLE ProcessHeap;

RECORDTYPE TypeArray[] =
{
    {DNS_TYPE_ZERO,    _T("ZERO")},
    {DNS_TYPE_A,       _T("A")},
    {DNS_TYPE_NS,      _T("NS")},
    {DNS_TYPE_MD,      _T("MD")},
    {DNS_TYPE_MF,      _T("MF")},
    {DNS_TYPE_CNAME,   _T("CNAME")},
    {DNS_TYPE_SOA,     _T("SOA")},
    {DNS_TYPE_MB,      _T("MB")},
    {DNS_TYPE_MG,      _T("MG")},
    {DNS_TYPE_MR,      _T("MR")},
    {DNS_TYPE_NULL,    _T("NULL")},
    {DNS_TYPE_WKS,     _T("WKS")},
    {DNS_TYPE_PTR,     _T("PTR")},
    {DNS_TYPE_HINFO,   _T("HINFO")},
    {DNS_TYPE_MINFO,   _T("MINFO")},
    {DNS_TYPE_MX,      _T("MX")},
    {DNS_TYPE_TEXT,    _T("TXT")},
    {DNS_TYPE_RP,      _T("RP")},
    {DNS_TYPE_AFSDB,   _T("AFSDB")},
    {DNS_TYPE_X25,     _T("X25")},
    {DNS_TYPE_ISDN,    _T("ISDN")},
    {DNS_TYPE_RT,      _T("RT")},
    {DNS_TYPE_NSAP,    _T("NSAP")},
    {DNS_TYPE_NSAPPTR, _T("NSAPPTR")},
    {DNS_TYPE_SIG,     _T("SIG")},
    {DNS_TYPE_KEY,     _T("KEY")},
    {DNS_TYPE_PX,      _T("PX")},
    {DNS_TYPE_GPOS,    _T("GPOS")},
    {DNS_TYPE_AAAA,    _T("AAAA")},
    {DNS_TYPE_LOC,     _T("LOC")},
    {DNS_TYPE_NXT,     _T("NXT")},
    {DNS_TYPE_EID,     _T("EID")},
    {DNS_TYPE_NIMLOC,  _T("NIMLOC")},
    {DNS_TYPE_SRV,     _T("SRV")},
    {DNS_TYPE_ATMA,    _T("ATMA")},
    {DNS_TYPE_NAPTR,   _T("NAPTR")},
    {DNS_TYPE_KX,      _T("KX")},
    {DNS_TYPE_CERT,    _T("CERT")},
    {DNS_TYPE_A6,      _T("A6")},
    {DNS_TYPE_DNAME,   _T("DNAME")},
    {DNS_TYPE_SINK,    _T("SINK")},
    {DNS_TYPE_OPT,     _T("OPT")},
    {DNS_TYPE_UINFO,   _T("UINFO")},
    {DNS_TYPE_UID,     _T("UID")},
    {DNS_TYPE_GID,     _T("GID")},
    {DNS_TYPE_UNSPEC,  _T("UNSPEC")},
    {DNS_TYPE_ADDRS,   _T("ADDRS")},
    {DNS_TYPE_TKEY,    _T("TKEY")},
    {DNS_TYPE_TSIG,    _T("TSIG")},
    {DNS_TYPE_IXFR,    _T("IXFR")},
    {DNS_TYPE_AXFR,    _T("AXFR")},
    {DNS_TYPE_MAILB,   _T("MAILB")},
    {DNS_TYPE_MAILA,   _T("MAILA")},
    {DNS_TYPE_ALL,     _T("ALL")},
    {0, NULL}
};

LPTSTR
GetRecordTypeName(WORD wType)
{
    static TCHAR szType[8];
    INT i;

    for (i = 0; ; i++)
    {
         if (TypeArray[i].pszRecordName == NULL)
             break;

         if (TypeArray[i].wRecordType == wType)
             return TypeArray[i].pszRecordName;
    }

    _stprintf(szType, _T("%hu"), wType);

    return szType;
}

/* print MAC address */
PCHAR PrintMacAddr(PBYTE Mac)
{
    static CHAR MacAddr[20];

    sprintf(MacAddr, "%02X-%02X-%02X-%02X-%02X-%02X",
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4],  Mac[5]);

    return MacAddr;
}


/* convert time_t to localized string */
_Ret_opt_z_ PTSTR timeToStr(_In_ time_t TimeStamp)
{
    struct tm* ptm;
    SYSTEMTIME SystemTime;
    INT DateCchSize, TimeCchSize, TotalCchSize, i;
    PTSTR DateTimeString, psz;

    /* Convert Unix time to SYSTEMTIME */
    /* localtime_s may be preferred if available */
    ptm = localtime(&TimeStamp);
    if (!ptm)
    {
        return NULL;
    }
    SystemTime.wYear = ptm->tm_year + 1900;
    SystemTime.wMonth = ptm->tm_mon + 1;
    SystemTime.wDay = ptm->tm_mday;
    SystemTime.wHour = ptm->tm_hour;
    SystemTime.wMinute = ptm->tm_min;
    SystemTime.wSecond = ptm->tm_sec;

    /* Get total size in characters required of buffer */
    DateCchSize = GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &SystemTime, NULL, NULL, 0);
    if (!DateCchSize)
    {
        return NULL;
    }
    TimeCchSize = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, NULL, 0);
    if (!TimeCchSize)
    {
        return NULL;
    }
    /* Two terminating null are included, the first one will be replaced by space */
    TotalCchSize = DateCchSize + TimeCchSize;

    /* Allocate buffer and format datetime string */
    DateTimeString = (PTSTR)HeapAlloc(ProcessHeap, 0, TotalCchSize * sizeof(TCHAR));
    if (!DateTimeString)
    {
        return NULL;
    }

    /* Get date string */
    i = GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &SystemTime, NULL, DateTimeString, TotalCchSize);
    if (i)
    {
        /* Append space and move pointer */
        DateTimeString[i - 1] = _T(' ');
        psz = DateTimeString + i;
        TotalCchSize -= i;

        /* Get time string */
        if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, psz, TotalCchSize))
        {
            return DateTimeString;
        }
    }

    HeapFree(ProcessHeap, 0, DateTimeString);
    return NULL;
}


VOID DoFormatMessage(LONG ErrorCode)
{
    LPVOID lpMsgBuf;
    //DWORD ErrorCode;

    if (ErrorCode == 0)
        ErrorCode = GetLastError();

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      ErrorCode,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL))
    {
        _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
}

VOID
GetAdapterFriendlyName(
    _In_ LPSTR lpClass,
    _In_ DWORD cchFriendlyNameLength,
    _Out_ LPWSTR pszFriendlyName)
{
    HKEY hKey = NULL;
    CHAR Path[256];
    LPSTR PrePath  = "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    LPSTR PostPath = "\\Connection";
    DWORD PathSize;
    DWORD dwType;
    DWORD dwDataSize;

    /* don't overflow the buffer */
    PathSize = strlen(PrePath) + strlen(lpClass) + strlen(PostPath) + 1;
    if (PathSize >= 255)
        return;

    sprintf(Path, "%s%s%s", PrePath, lpClass, PostPath);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      Path,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        dwDataSize = cchFriendlyNameLength * sizeof(WCHAR);
        RegQueryValueExW(hKey,
                         L"Name",
                         NULL,
                         &dwType,
                         (PBYTE)pszFriendlyName,
                         &dwDataSize);
    }

    if (hKey != NULL)
        RegCloseKey(hKey);
}

VOID
GetInterfaceFriendlyName(
    _In_ LPWSTR lpDeviceName,
    _In_ DWORD cchFriendlyNameLength,
    _Out_ LPWSTR pszFriendlyName)
{
    HKEY hKey = NULL;
    WCHAR Path[256];
    LPWSTR PrePath  = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    LPWSTR PostPath = L"\\Connection";
    LPWSTR DevicePrefix = L"\\DEVICE\\TCPIP_";
    DWORD PathSize;
    DWORD dwType;
    DWORD dwDataSize;

    DWORD dwPrefixLength = wcslen(DevicePrefix);

    /* don't overflow the buffer */
    PathSize = wcslen(PrePath) + wcslen(lpDeviceName) - dwPrefixLength + wcslen(PostPath) + 1;
    if (PathSize >= 255)
        return;

    swprintf(Path, L"%s%s%s", PrePath, &lpDeviceName[dwPrefixLength], PostPath);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      Path,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        dwDataSize = cchFriendlyNameLength * sizeof(WCHAR);
        RegQueryValueExW(hKey,
                         L"Name",
                         NULL,
                         &dwType,
                         (PBYTE)pszFriendlyName,
                         &dwDataSize);
    }

    if (hKey != NULL)
        RegCloseKey(hKey);
}

static
VOID
PrintAdapterDescription(LPSTR lpClass)
{
    HKEY hBaseKey = NULL;
    HKEY hClassKey = NULL;
    LPSTR lpKeyClass = NULL;
    LPSTR lpConDesc = NULL;
    LPTSTR lpPath = NULL;
    TCHAR szPrePath[] = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}\\");
    DWORD dwType;
    DWORD dwDataSize;
    INT i;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szPrePath,
                     0,
                     KEY_READ,
                     &hBaseKey) != ERROR_SUCCESS)
    {
        return;
    }

    for (i = 0; ; i++)
    {
        DWORD PathSize;
        LONG Status;
        TCHAR szName[10];
        DWORD NameLen = 9;

        if ((Status = RegEnumKeyEx(hBaseKey,
                                   i,
                                   szName,
                                   &NameLen,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL)) != ERROR_SUCCESS)
        {
            if (Status == ERROR_NO_MORE_ITEMS)
            {
                DoFormatMessage(Status);
                lpConDesc = NULL;
                goto CLEANUP;
            }
            else
                continue;
        }

        PathSize = lstrlen(szPrePath) + lstrlen(szName) + 1;
        lpPath = (LPTSTR)HeapAlloc(ProcessHeap,
                                   0,
                                   PathSize * sizeof(TCHAR));
        if (lpPath == NULL)
            goto CLEANUP;

        wsprintf(lpPath, _T("%s%s"), szPrePath, szName);

        //MessageBox(NULL, lpPath, NULL, 0);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         lpPath,
                         0,
                         KEY_READ,
                         &hClassKey) != ERROR_SUCCESS)
        {
            goto CLEANUP;
        }

        HeapFree(ProcessHeap, 0, lpPath);
        lpPath = NULL;

        if (RegQueryValueExA(hClassKey,
                             "NetCfgInstanceId",
                             NULL,
                             &dwType,
                             NULL,
                             &dwDataSize) == ERROR_SUCCESS)
        {
            lpKeyClass = (LPSTR)HeapAlloc(ProcessHeap,
                                          0,
                                          dwDataSize);
            if (lpKeyClass == NULL)
                goto CLEANUP;

            if (RegQueryValueExA(hClassKey,
                                 "NetCfgInstanceId",
                                 NULL,
                                 &dwType,
                                 (PBYTE)lpKeyClass,
                                 &dwDataSize) != ERROR_SUCCESS)
            {
                HeapFree(ProcessHeap, 0, lpKeyClass);
                lpKeyClass = NULL;
                continue;
            }
        }
        else
            continue;

        if (!strcmp(lpClass, lpKeyClass))
        {
            HeapFree(ProcessHeap, 0, lpKeyClass);
            lpKeyClass = NULL;

            if (RegQueryValueExA(hClassKey,
                                 "DriverDesc",
                                 NULL,
                                 &dwType,
                                 NULL,
                                 &dwDataSize) == ERROR_SUCCESS)
            {
                lpConDesc = (LPSTR)HeapAlloc(ProcessHeap,
                                             0,
                                             dwDataSize);
                if (lpConDesc != NULL)
                {
                    if (RegQueryValueExA(hClassKey,
                                         "DriverDesc",
                                         NULL,
                                         &dwType,
                                         (PBYTE)lpConDesc,
                                         &dwDataSize) == ERROR_SUCCESS)
                    {
                        printf("%s", lpConDesc);
                    }

                    HeapFree(ProcessHeap, 0, lpConDesc);
                    lpConDesc = NULL;
                }
            }

            break;
        }
    }

CLEANUP:
    if (hBaseKey != NULL)
        RegCloseKey(hBaseKey);
    if (hClassKey != NULL)
        RegCloseKey(hClassKey);
    if (lpPath != NULL)
        HeapFree(ProcessHeap, 0, lpPath);
    if (lpKeyClass != NULL)
        HeapFree(ProcessHeap, 0, lpKeyClass);
}

static
VOID
PrintNodeType(
    _In_ UINT NodeType)
{
    switch (NodeType)
    {
        case BROADCAST_NODETYPE:
            ConResPrintf(StdOut, IDS_NODETYPEBCAST);
            break;

        case PEER_TO_PEER_NODETYPE:
            ConResPrintf(StdOut, IDS_NODETYPEP2P);
            break;

        case MIXED_NODETYPE:
            ConResPrintf(StdOut, IDS_NODETYPEMIXED);
            break;

        case HYBRID_NODETYPE:
            ConResPrintf(StdOut, IDS_NODETYPEHYBRID);
            break;

        default :
            ConResPrintf(StdOut, IDS_NODETYPEUNKNOWN);
            break;
    }
}

static
VOID
PrintAdapterTypeAndName(
    PIP_ADAPTER_INFO pAdapterInfo)
{
    WCHAR szFriendlyName[MAX_PATH];

    GetAdapterFriendlyName(pAdapterInfo->AdapterName, MAX_PATH, szFriendlyName);

    switch (pAdapterInfo->Type)
    {
        case MIB_IF_TYPE_OTHER:
            ConResPrintf(StdOut, IDS_OTHER, szFriendlyName);
            break;

        case MIB_IF_TYPE_ETHERNET:
            ConResPrintf(StdOut, IDS_ETH, szFriendlyName);
            break;

        case MIB_IF_TYPE_TOKENRING:
            ConResPrintf(StdOut, IDS_TOKEN, szFriendlyName);
            break;

        case MIB_IF_TYPE_FDDI:
            ConResPrintf(StdOut, IDS_FDDI, szFriendlyName);
            break;

        case MIB_IF_TYPE_PPP:
            ConResPrintf(StdOut, IDS_PPP, szFriendlyName);
            break;

        case MIB_IF_TYPE_LOOPBACK:
            ConResPrintf(StdOut, IDS_LOOP, szFriendlyName);
            break;

        case MIB_IF_TYPE_SLIP:
            ConResPrintf(StdOut, IDS_SLIP, szFriendlyName);
            break;

        case IF_TYPE_IEEE80211:
            ConResPrintf(StdOut, IDS_WIFI, szFriendlyName);
            break;

        default:
            ConResPrintf(StdOut, IDS_UNKNOWNADAPTER, szFriendlyName);
            break;
    }
}

VOID
ShowInfo(
    BOOL bShowHeader,
    BOOL bAll)
{
    MIB_IFROW mibEntry;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG adaptOutBufLen = 0;
    PFIXED_INFO pFixedInfo = NULL;
    ULONG netOutBufLen = 0;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    ULONG ulPerAdapterInfoLength = 0;
    PSTR pszDomainName = NULL;
    DWORD dwDomainNameSize = 0;
    ULONG ret = 0;

    GetComputerNameExA(ComputerNameDnsDomain,
                       NULL,
                       &dwDomainNameSize);
    if (dwDomainNameSize > 0)
    {
        pszDomainName = HeapAlloc(ProcessHeap,
                                  0,
                                  dwDomainNameSize * sizeof(TCHAR));
        if (pszDomainName != NULL)
            GetComputerNameExA(ComputerNameDnsDomain,
                               pszDomainName,
                               &dwDomainNameSize);
    }

    /* call GetAdaptersInfo to obtain the adapter info */
    ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (IP_ADAPTER_INFO *)HeapAlloc(ProcessHeap, 0, adaptOutBufLen);
        if (pAdapterInfo == NULL)
            goto done;

        ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
        if (ret != NO_ERROR)
        {
            DoFormatMessage(0);
            goto done;
        }
    }
    else
    {
        if (ret != ERROR_NO_DATA)
        {
            DoFormatMessage(0);
            goto done;
        }
    }

    /* call GetNetworkParams to obtain the network info */
    if (GetNetworkParams(pFixedInfo, &netOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pFixedInfo = (FIXED_INFO *)HeapAlloc(ProcessHeap, 0, netOutBufLen);
        if (pFixedInfo == NULL)
        {
            goto done;
        }
        if (GetNetworkParams(pFixedInfo, &netOutBufLen) != NO_ERROR)
        {
            DoFormatMessage(0);
            goto done;
        }
    }
    else
    {
        DoFormatMessage(0);
        goto done;
    }

    pAdapter = pAdapterInfo;

    if (bShowHeader)
        ConResPrintf(StdOut, IDS_HEADER);

    if (bAll)
    {
        ConResPrintf(StdOut, IDS_HOSTNAME, pFixedInfo->HostName);
        ConResPrintf(StdOut, IDS_PRIMARYDNSSUFFIX, (pszDomainName != NULL) ? pszDomainName : "");

        PrintNodeType(pFixedInfo->NodeType);

        if (pFixedInfo->EnableRouting)
            ConResPrintf(StdOut, IDS_IPROUTINGYES);
        else
            ConResPrintf(StdOut, IDS_IPROUTINGNO);

        if (pAdapter && pAdapter->HaveWins)
            ConResPrintf(StdOut, IDS_WINSPROXYYES);
        else
            ConResPrintf(StdOut, IDS_WINSPROXYNO);

        if (pszDomainName != NULL && pszDomainName[0] != 0)
        {
            ConResPrintf(StdOut, IDS_DNSSUFFIXLIST, pszDomainName);
            ConResPrintf(StdOut, IDS_EMPTYLINE, pFixedInfo->DomainName);
        }
        else
        {
            ConResPrintf(StdOut, IDS_DNSSUFFIXLIST, pFixedInfo->DomainName);
        }
    }

    while (pAdapter)
    {
        BOOLEAN bConnected = TRUE;

        mibEntry.dwIndex = pAdapter->Index;
        GetIfEntry(&mibEntry);

        PrintAdapterTypeAndName(pAdapter);

        if (GetPerAdapterInfo(pAdapter->Index, pPerAdapterInfo, &ulPerAdapterInfoLength) == ERROR_BUFFER_OVERFLOW)
        {
            pPerAdapterInfo = (PIP_PER_ADAPTER_INFO)HeapAlloc(ProcessHeap, 0, ulPerAdapterInfoLength);
            if (pPerAdapterInfo != NULL)
            {
                GetPerAdapterInfo(pAdapter->Index, pPerAdapterInfo, &ulPerAdapterInfoLength);
            }
        }

        /* check if the adapter is connected to the media */
        if (mibEntry.dwOperStatus != MIB_IF_OPER_STATUS_CONNECTED && mibEntry.dwOperStatus != MIB_IF_OPER_STATUS_OPERATIONAL)
        {
            bConnected = FALSE;
            ConResPrintf(StdOut, IDS_MEDIADISCONNECTED);
        }
        else
        {
            ConResPrintf(StdOut, IDS_CONNECTIONDNSSUFFIX, pFixedInfo->DomainName);
        }

        if (bAll)
        {
            ConResPrintf(StdOut, IDS_DESCRIPTION);
            PrintAdapterDescription(pAdapter->AdapterName);
            printf("\n");

            ConResPrintf(StdOut, IDS_PHYSICALADDRESS, PrintMacAddr(pAdapter->Address));

            if (bConnected)
            {
                if (pAdapter->DhcpEnabled)
                {
                    ConResPrintf(StdOut, IDS_DHCPYES);

                    if (pPerAdapterInfo != NULL)
                    {
                        if (pPerAdapterInfo->AutoconfigEnabled)
                            ConResPrintf(StdOut, IDS_AUTOCONFIGYES);
                        else
                            ConResPrintf(StdOut, IDS_AUTOCONFIGNO);
                    }
                }
                else
                {
                    ConResPrintf(StdOut, IDS_DHCPNO);
                }
            }
        }

        if (!bConnected)
        {
            pAdapter = pAdapter->Next;
            continue;
        }

        ConResPrintf(StdOut, IDS_IPADDRESS, pAdapter->IpAddressList.IpAddress.String);
        ConResPrintf(StdOut, IDS_SUBNETMASK, pAdapter->IpAddressList.IpMask.String);

        if (strcmp(pAdapter->GatewayList.IpAddress.String, "0.0.0.0"))
            ConResPrintf(StdOut, IDS_DEFAULTGATEWAY, pAdapter->GatewayList.IpAddress.String);
        else
            ConResPrintf(StdOut, IDS_DEFAULTGATEWAY, "");

        if (bAll)
        {
            PIP_ADDR_STRING pIPAddr;

            if (pAdapter->DhcpEnabled)
                ConResPrintf(StdOut, IDS_DHCPSERVER, pAdapter->DhcpServer.IpAddress.String);

            ConResPrintf(StdOut, IDS_DNSSERVERS, pFixedInfo->DnsServerList.IpAddress.String);
            pIPAddr = pFixedInfo->DnsServerList.Next;
            while (pIPAddr)
            {
                ConResPrintf(StdOut, IDS_EMPTYLINE, pIPAddr ->IpAddress.String);
                pIPAddr = pIPAddr->Next;
            }

            if (pAdapter->HaveWins)
            {
                ConResPrintf(StdOut, IDS_PRIMARYWINSSERVER, pAdapter->PrimaryWinsServer.IpAddress.String);
                ConResPrintf(StdOut, IDS_SECONDARYWINSSERVER, pAdapter->SecondaryWinsServer.IpAddress.String);
            }

            if (pAdapter->DhcpEnabled && strcmp(pAdapter->DhcpServer.IpAddress.String, "255.255.255.255"))
            {
                PTSTR DateTimeString;
                DateTimeString = timeToStr(pAdapter->LeaseObtained);
                ConResPrintf(StdOut, IDS_LEASEOBTAINED, DateTimeString ? DateTimeString : _T("N/A"));
                if (DateTimeString)
                {
                    HeapFree(ProcessHeap, 0, DateTimeString);
                }
                DateTimeString = timeToStr(pAdapter->LeaseExpires);
                ConResPrintf(StdOut, IDS_LEASEEXPIRES, DateTimeString ? DateTimeString : _T("N/A"));
                if (DateTimeString)
                {
                    HeapFree(ProcessHeap, 0, DateTimeString);
                }
            }
        }

        HeapFree(ProcessHeap, 0, pPerAdapterInfo);
        pPerAdapterInfo = NULL;

        pAdapter = pAdapter->Next;
    }

done:
    if (pszDomainName)
        HeapFree(ProcessHeap, 0, pszDomainName);
    if (pFixedInfo)
        HeapFree(ProcessHeap, 0, pFixedInfo);
    if (pAdapterInfo)
        HeapFree(ProcessHeap, 0, pAdapterInfo);
}

static
BOOL
MatchWildcard(
    _In_ PWSTR pszExpression,
    _In_ PWSTR pszName)
{
    WCHAR *pCharE, *pCharN, charE, charN;

    if (pszExpression == NULL)
        return TRUE;

    if (pszName == NULL)
        return FALSE;

    pCharE = pszExpression;
    pCharN = pszName;
    while (*pCharE != UNICODE_NULL)
    {
        charE = towlower(*pCharE);
        charN = towlower(*pCharN);

        if (charE == L'*')
        {
            if (*(pCharE + 1) != charN)
                pCharN++;
            else
                pCharE++;
        }
        else if (charE == L'?')
        {
            pCharE++;
            pCharN++;
        }
        else if (charE == charN)
        {
            pCharE++;
            pCharN++;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}

static
VOID
BuildAdapterMap(
    PIP_ADAPTER_INDEX_MAP pAdapterMap,
    PIP_ADAPTER_INFO pAdapterInfo)
{
    int i, l1, l2;

    pAdapterMap->Index = pAdapterInfo->Index;

    wcscpy(pAdapterMap->Name, L"\\DEVICE\\TCPIP_");
    l1 = wcslen(pAdapterMap->Name);
    l2 = strlen(pAdapterInfo->AdapterName);
    for (i = 0; i < l2; i++)
        pAdapterMap->Name[i + l1] = (WCHAR)pAdapterInfo->AdapterName[i];
    pAdapterMap->Name[i + l1] = UNICODE_NULL;
}

VOID
Release(
    LPWSTR pszAdapterName)
{
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG adaptOutBufLen = 0;
    ULONG ret = 0;
    WCHAR szFriendlyName[MAX_PATH];
    MIB_IFROW mibEntry;
    IP_ADAPTER_INDEX_MAP AdapterMap;
    BOOL bFoundAdapter = FALSE;

    ConResPrintf(StdOut, IDS_HEADER);

    /* call GetAdaptersInfo to obtain the adapter info */
    ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
    if (ret != ERROR_BUFFER_OVERFLOW)
    {
        DoFormatMessage(ret);
        return;
    }

    pAdapterInfo = (IP_ADAPTER_INFO *)HeapAlloc(ProcessHeap, 0, adaptOutBufLen);
    if (pAdapterInfo == NULL)
    {
        _tprintf(_T("memory allocation error"));
        return;
    }

    ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
    if (ret != NO_ERROR)
    {
        DoFormatMessage(0);
        goto done;
    }

    pAdapter = pAdapterInfo;

    while (pAdapter)
    {
        GetAdapterFriendlyName(pAdapterInfo->AdapterName, MAX_PATH, szFriendlyName);

        if ((pszAdapterName == NULL) || MatchWildcard(pszAdapterName, szFriendlyName))
        {
            bFoundAdapter = TRUE;

            mibEntry.dwIndex = pAdapter->Index;
            GetIfEntry(&mibEntry);

            if (mibEntry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED ||
                mibEntry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
            {
                if (pAdapter->DhcpEnabled)
                {
                    if (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0"))
                    {
                        BuildAdapterMap(&AdapterMap, pAdapter);

                        /* Call IpReleaseAddress to release the IP address on the specified adapter. */
                        ret = IpReleaseAddress(&AdapterMap);
                        if (ret != NO_ERROR)
                        {
                            ConResPrintf(StdOut, IDS_DHCPRELEASEERROR, szFriendlyName);
                            DoFormatMessage(ret);
                        }
                    }
                    else
                    {
                        ConResPrintf(StdOut, IDS_DHCPRELEASED);
                    }
                }
                else
                {
                    ConResPrintf(StdOut, IDS_DHCPNOTENABLED, szFriendlyName);
                }
            }
            else
            {
                ConResPrintf(StdOut, IDS_DHCPNOTCONNECTED, szFriendlyName);
            }
        }

        pAdapter = pAdapter->Next;
    }

    if (bFoundAdapter == FALSE)
    {
        ConResPrintf(StdOut, IDS_DHCPNOADAPTER);
    }
    else
    {
        ShowInfo(FALSE, FALSE);
    }

done:
    if (pAdapterInfo)
        HeapFree(ProcessHeap, 0, pAdapterInfo);
}

VOID
Renew(
    LPWSTR pszAdapterName)
{
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG adaptOutBufLen = 0;
    ULONG ret = 0;
    WCHAR szFriendlyName[MAX_PATH];
    MIB_IFROW mibEntry;
    IP_ADAPTER_INDEX_MAP AdapterMap;
    BOOL bFoundAdapter = FALSE;

    ConResPrintf(StdOut, IDS_HEADER);

    /* call GetAdaptersInfo to obtain the adapter info */
    ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
    if (ret != ERROR_BUFFER_OVERFLOW)
    {
        DoFormatMessage(ret);
        return;
    }

    pAdapterInfo = (IP_ADAPTER_INFO *)HeapAlloc(ProcessHeap, 0, adaptOutBufLen);
    if (pAdapterInfo == NULL)
    {
        _tprintf(_T("memory allocation error"));
        return;
    }

    ret = GetAdaptersInfo(pAdapterInfo, &adaptOutBufLen);
    if (ret != NO_ERROR)
    {
        DoFormatMessage(0);
        goto done;
    }

    pAdapter = pAdapterInfo;

    while (pAdapter)
    {
        GetAdapterFriendlyName(pAdapterInfo->AdapterName, MAX_PATH, szFriendlyName);

        if ((pszAdapterName == NULL) || MatchWildcard(pszAdapterName, szFriendlyName))
        {
            bFoundAdapter = TRUE;

            mibEntry.dwIndex = pAdapter->Index;
            GetIfEntry(&mibEntry);

            if (mibEntry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED ||
                mibEntry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
            {
                if (pAdapter->DhcpEnabled)
                {
                    BuildAdapterMap(&AdapterMap, pAdapter);

                    /* Call IpRenewAddress to renew the IP address on the specified adapter. */
                    ret = IpRenewAddress(&AdapterMap);
                    if (ret != NO_ERROR)
                    {
                        ConResPrintf(StdOut, IDS_DHCPRENEWERROR, szFriendlyName);
                        DoFormatMessage(ret);
                    }
                }
                else
                {
                    ConResPrintf(StdOut, IDS_DHCPNOTENABLED, szFriendlyName);
                }
            }
            else
            {
                ConResPrintf(StdOut, IDS_DHCPNOTCONNECTED, szFriendlyName);
            }
        }

        pAdapter = pAdapter->Next;
    }

    if (bFoundAdapter == FALSE)
    {
        ConResPrintf(StdOut, IDS_DHCPNOADAPTER);
    }
    else
    {
        ShowInfo(FALSE, FALSE);
    }

done:
    if (pAdapterInfo)
        HeapFree(ProcessHeap, 0, pAdapterInfo);
}

VOID
FlushDns(VOID)
{
    ConResPrintf(StdOut, IDS_HEADER);

    if (DnsFlushResolverCache())
    {
        ConResPrintf(StdOut, IDS_DNSFLUSHSUCCESS);
    }
    else
    {
        ConResPrintf(StdOut, IDS_DNSFLUSHERROR);
        DoFormatMessage(GetLastError());
    }
}

VOID
RegisterDns(VOID)
{
    /* FIXME */
    _tprintf(_T("\nSorry /registerdns is not implemented yet\n"));
}

static
VOID
DisplayDnsRecord(
    PWSTR pszName,
    WORD wType)
{
    PDNS_RECORDW pQueryResults = NULL, pThisRecord, pNextRecord;
    WCHAR szBuffer[48];
    IN_ADDR Addr4;
    IN6_ADDR Addr6;
    DNS_STATUS Status;

    ConResPrintf(StdOut, IDS_DNSNAME, pszName);
    ConResPrintf(StdOut, IDS_DNSLINE);

    pQueryResults = NULL;
    Status = DnsQuery_W(pszName,
                        wType,
                        DNS_QUERY_NO_WIRE_QUERY,
                        NULL,
                        (PDNS_RECORD *)&pQueryResults,
                        NULL);
    if (Status != ERROR_SUCCESS)
    {
        if (Status == DNS_ERROR_RCODE_NAME_ERROR)
        {
            ConResPrintf(StdOut, IDS_DNSNONAME);
        }
        else if (Status == DNS_INFO_NO_RECORDS)
        {
            ConResPrintf(StdOut, IDS_DNSNORECORD, GetRecordTypeName(wType));
        }
        return;
    }

    pThisRecord = pQueryResults;
    while (pThisRecord != NULL)
    {
        pNextRecord = pThisRecord->pNext;

        ConResPrintf(StdOut, IDS_DNSRECORDNAME, pThisRecord->pName);
        ConResPrintf(StdOut, IDS_DNSRECORDTYPE, pThisRecord->wType);
        ConResPrintf(StdOut, IDS_DNSRECORDTTL, pThisRecord->dwTtl);
        ConResPrintf(StdOut, IDS_DNSRECORDLENGTH, pThisRecord->wDataLength);

        switch (pThisRecord->Flags.S.Section)
        {
            case DnsSectionQuestion:
                ConResPrintf(StdOut, IDS_DNSSECTIONQUESTION);
                break;

            case DnsSectionAnswer:
                ConResPrintf(StdOut, IDS_DNSSECTIONANSWER);
                break;

            case DnsSectionAuthority:
                ConResPrintf(StdOut, IDS_DNSSECTIONAUTHORITY);
                break;

            case DnsSectionAdditional:
                ConResPrintf(StdOut, IDS_DNSSECTIONADDITIONAL);
                break;
        }

        switch (pThisRecord->wType)
        {
            case DNS_TYPE_A:
                Addr4.S_un.S_addr = pThisRecord->Data.A.IpAddress;
                RtlIpv4AddressToStringW(&Addr4, szBuffer);
                ConResPrintf(StdOut, IDS_DNSTYPEA, szBuffer);
                break;

            case DNS_TYPE_NS:
                ConResPrintf(StdOut, IDS_DNSTYPENS, pThisRecord->Data.NS.pNameHost);
                break;

            case DNS_TYPE_CNAME:
                ConResPrintf(StdOut, IDS_DNSTYPECNAME, pThisRecord->Data.CNAME.pNameHost);
                break;

            case DNS_TYPE_SOA:
                ConResPrintf(StdOut, IDS_DNSTYPESOA1,
                             pThisRecord->Data.SOA.pNamePrimaryServer,
                             pThisRecord->Data.SOA.pNameAdministrator,
                             pThisRecord->Data.SOA.dwSerialNo);
                ConResPrintf(StdOut, IDS_DNSTYPESOA2,
                             pThisRecord->Data.SOA.dwRefresh,
                             pThisRecord->Data.SOA.dwRetry,
                             pThisRecord->Data.SOA.dwExpire,
                             pThisRecord->Data.SOA.dwDefaultTtl);
                break;

            case DNS_TYPE_PTR:
                ConResPrintf(StdOut, IDS_DNSTYPEPTR, pThisRecord->Data.PTR.pNameHost);
                break;

            case DNS_TYPE_MX:
                ConResPrintf(StdOut, IDS_DNSTYPEMX,
                             pThisRecord->Data.MX.pNameExchange,
                             pThisRecord->Data.MX.wPreference,
                             pThisRecord->Data.MX.Pad);
                break;

            case DNS_TYPE_AAAA:
                RtlCopyMemory(&Addr6, &pThisRecord->Data.AAAA.Ip6Address, sizeof(IN6_ADDR));
                RtlIpv6AddressToStringW(&Addr6, szBuffer);
                ConResPrintf(StdOut, IDS_DNSTYPEAAAA, szBuffer);
                break;

            case DNS_TYPE_ATMA:
                ConResPrintf(StdOut, IDS_DNSTYPEATMA);
                break;

            case DNS_TYPE_SRV:
                ConResPrintf(StdOut, IDS_DNSTYPESRV,
                             pThisRecord->Data.SRV.pNameTarget,
                             pThisRecord->Data.SRV.wPriority,
                             pThisRecord->Data.SRV.wWeight,
                             pThisRecord->Data.SRV.wPort);
                break;
        }
        ConPuts(StdOut, L"\n\n");

        pThisRecord = pNextRecord;
    }

    DnsRecordListFree((PDNS_RECORD)pQueryResults, DnsFreeRecordList);
}

VOID
DisplayDns(VOID)
{
    PDNS_CACHE_ENTRY DnsEntry = NULL, pThisEntry, pNextEntry;

    ConResPrintf(StdOut, IDS_HEADER);

    if (!DnsGetCacheDataTable(&DnsEntry))
    {
        DoFormatMessage(GetLastError());
        return;
    }

    if (DnsEntry == NULL)
        return;

    pThisEntry = DnsEntry;
    while (pThisEntry != NULL)
    {
        pNextEntry = pThisEntry->pNext;

        if (pThisEntry->wType1 != DNS_TYPE_ZERO)
            DisplayDnsRecord(pThisEntry->pszName, pThisEntry->wType1);

        if (pThisEntry->wType2 != DNS_TYPE_ZERO)
            DisplayDnsRecord(pThisEntry->pszName, pThisEntry->wType2);

        if (pThisEntry->pszName)
            LocalFree(pThisEntry->pszName);
        LocalFree(pThisEntry);

        pThisEntry = pNextEntry;
    }
}

VOID Usage(VOID)
{
    ConResPrintf(StdOut, IDS_USAGE);
}

int wmain(int argc, wchar_t *argv[])
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

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    hInstance = GetModuleHandle(NULL);
    ProcessHeap = GetProcessHeap();

    /* Parse command line for options we have been given. */
    if ((argc > 1) && (argv[1][0]=='/' || argv[1][0]=='-'))
    {
        if (!_tcsicmp(&argv[1][1], _T("?")))
        {
            DoUsage = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("ALL"), _tcslen(&argv[1][1])))
        {
           DoAll = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("RELEASE"), _tcslen(&argv[1][1])))
        {
            DoRelease = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("RENEW"), _tcslen(&argv[1][1])))
        {
            DoRenew = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("FLUSHDNS"), _tcslen(&argv[1][1])))
        {
            DoFlushdns = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("FLUSHREGISTERDNS"), _tcslen(&argv[1][1])))
        {
            DoRegisterdns = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("DISPLAYDNS"), _tcslen(&argv[1][1])))
        {
            DoDisplaydns = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("SHOWCLASSID"), _tcslen(&argv[1][1])))
        {
            DoShowclassid = TRUE;
        }
        else if (!_tcsnicmp(&argv[1][1], _T("SETCLASSID"), _tcslen(&argv[1][1])))
        {
            DoSetclassid = TRUE;
        }
    }

    switch (argc)
    {
        case 1:  /* Default behaviour if no options are given*/
            ShowInfo(TRUE, FALSE);
            break;
        case 2:  /* Process all the options that take no parameters */
            if (DoUsage)
                Usage();
            else if (DoAll)
                ShowInfo(TRUE, TRUE);
            else if (DoRelease)
                Release(NULL);
            else if (DoRenew)
                Renew(NULL);
            else if (DoFlushdns)
                FlushDns();
            else if (DoRegisterdns)
                RegisterDns();
            else if (DoDisplaydns)
                DisplayDns();
            else
                Usage();
            break;
        case 3: /* Process all the options that can have 1 parameter */
            if (DoRelease)
                Release(argv[2]);
            else if (DoRenew)
                Renew(argv[2]);
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
