/*
 * PROJECT:     ReactOS ipconfig utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/ipconfig/ipconfig.c
 * PURPOSE:     Display IP info for net adapters
 * PROGRAMMERS: Copyright 2005 - 2006 Ged Murphy (gedmurphy@gmail.com)
 */
/*
 * TODO:
 * fix renew / release
 * implement registerdns, showclassid, setclassid
 * allow globbing on adapter names
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <iphlpapi.h>
#include <ndk/rtlfuncs.h>
#include <inaddr.h>
#include <windns.h>
#include <windns_undoc.h>

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

int LoadStringAndOem(HINSTANCE hInst,
                     UINT uID,
                     LPTSTR szNode,
                     int byteSize)
{
    TCHAR *szTmp;
    int res;

    szTmp = (LPTSTR)HeapAlloc(ProcessHeap, 0, byteSize);
    if (szTmp == NULL)
    {
        return 0;
    }

    res = LoadString(hInst, uID, szTmp, byteSize);
    CharToOem(szTmp, szNode);
    HeapFree(ProcessHeap, 0, szTmp);
    return res;
}

LPTSTR GetNodeTypeName(UINT NodeType)
{
    static TCHAR szNode[14];

    switch (NodeType)
    {
        case 1:
            if (!LoadStringAndOem(hInstance, IDS_BCAST, szNode,  sizeof(szNode)))
                return NULL;
            break;

        case 2:
            if (!LoadStringAndOem(hInstance, IDS_P2P, szNode,  sizeof(szNode)))
                return NULL;
            break;

        case 4:
            if (!LoadStringAndOem(hInstance, IDS_MIXED, szNode,  sizeof(szNode)))
                return NULL;
            break;

        case 8:
            if (!LoadStringAndOem(hInstance, IDS_HYBRID, szNode,  sizeof(szNode)))
                return NULL;
            break;

        default :
            if (!LoadStringAndOem(hInstance, IDS_UNKNOWN, szNode,  sizeof(szNode)))
                return NULL;
            break;
    }

    return szNode;
}


LPTSTR GetInterfaceTypeName(UINT InterfaceType)
{
    static TCHAR szIntType[25];

    switch (InterfaceType)
    {
        case MIB_IF_TYPE_OTHER:
            if (!LoadStringAndOem(hInstance, IDS_OTHER, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_ETHERNET:
            if (!LoadStringAndOem(hInstance, IDS_ETH, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_TOKENRING:
            if (!LoadStringAndOem(hInstance, IDS_TOKEN, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_FDDI:
            if (!LoadStringAndOem(hInstance, IDS_FDDI, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_PPP:
            if (!LoadStringAndOem(hInstance, IDS_PPP, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_LOOPBACK:
            if (!LoadStringAndOem(hInstance, IDS_LOOP, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        case MIB_IF_TYPE_SLIP:
            if (!LoadStringAndOem(hInstance, IDS_SLIP, szIntType, sizeof(szIntType)))
                return NULL;
            break;

        default:
            if (!LoadStringAndOem(hInstance, IDS_UNKNOWN, szIntType, sizeof(szIntType)))
                return NULL;
            break;
    }

    return szIntType;
}


/* print MAC address */
PTCHAR PrintMacAddr(PBYTE Mac)
{
    static TCHAR MacAddr[20];

    _stprintf(MacAddr, _T("%02x-%02x-%02x-%02x-%02x-%02x"),
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4],  Mac[5]);

    return MacAddr;
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


LPTSTR GetConnectionType(LPTSTR lpClass)
{
    HKEY hKey = NULL;
    LPTSTR ConType = NULL;
    LPTSTR ConTypeTmp = NULL;
    TCHAR Path[256];
    LPTSTR PrePath  = _T("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\");
    LPTSTR PostPath = _T("\\Connection");
    DWORD PathSize;
    DWORD dwType;
    DWORD dwDataSize;

    /* don't overflow the buffer */
    PathSize = lstrlen(PrePath) + lstrlen(lpClass) + lstrlen(PostPath) + 1;
    if (PathSize >= 255)
        return NULL;

    wsprintf(Path, _T("%s%s%s"), PrePath, lpClass, PostPath);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     Path,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey,
                            _T("Name"),
                            NULL,
                            &dwType,
                            NULL,
                            &dwDataSize) == ERROR_SUCCESS)
        {
            ConTypeTmp = (LPTSTR)HeapAlloc(ProcessHeap,
                                           0,
                                           dwDataSize);

            if (ConTypeTmp == NULL)
                return NULL;

            ConType = (LPTSTR)HeapAlloc(ProcessHeap,
                                        0,
                                        dwDataSize);

            if (ConType == NULL)
            {
                HeapFree(ProcessHeap, 0, ConTypeTmp);
                return NULL;
            }

            if (RegQueryValueEx(hKey,
                                _T("Name"),
                                NULL,
                                &dwType,
                                (PBYTE)ConTypeTmp,
                                &dwDataSize) != ERROR_SUCCESS)
            {
                HeapFree(ProcessHeap,
                         0,
                         ConType);

                ConType = NULL;
            }

            if (ConType)
                CharToOem(ConTypeTmp, ConType);
            HeapFree(ProcessHeap, 0, ConTypeTmp);
        }
    }

    if (hKey != NULL)
        RegCloseKey(hKey);

    return ConType;
}


LPTSTR GetConnectionDescription(LPTSTR lpClass)
{
    HKEY hBaseKey = NULL;
    HKEY hClassKey = NULL;
    LPTSTR lpKeyClass = NULL;
    LPTSTR lpConDesc = NULL;
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
        return NULL;
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

        if (RegQueryValueEx(hClassKey,
                            _T("NetCfgInstanceId"),
                            NULL,
                            &dwType,
                            NULL,
                            &dwDataSize) == ERROR_SUCCESS)
        {
            lpKeyClass = (LPTSTR)HeapAlloc(ProcessHeap,
                                           0,
                                           dwDataSize);
            if (lpKeyClass == NULL)
                goto CLEANUP;

            if (RegQueryValueEx(hClassKey,
                                _T("NetCfgInstanceId"),
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

        if (!lstrcmp(lpClass, lpKeyClass))
        {
            HeapFree(ProcessHeap, 0, lpKeyClass);
            lpKeyClass = NULL;

            if (RegQueryValueEx(hClassKey,
                                _T("DriverDesc"),
                                NULL,
                                &dwType,
                                NULL,
                                &dwDataSize) == ERROR_SUCCESS)
            {
                lpConDesc = (LPTSTR)HeapAlloc(ProcessHeap,
                                              0,
                                              dwDataSize);
                if (lpConDesc == NULL)
                    goto CLEANUP;

                if (RegQueryValueEx(hClassKey,
                                    _T("DriverDesc"),
                                    NULL,
                                    &dwType,
                                    (PBYTE)lpConDesc,
                                    &dwDataSize) != ERROR_SUCCESS)
                {
                    HeapFree(ProcessHeap, 0, lpConDesc);
                    lpConDesc = NULL;
                    goto CLEANUP;
                }
            }
            else
            {
                lpConDesc = NULL;
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

    return lpConDesc;
}


VOID ShowInfo(BOOL bAll)
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

    _tprintf(_T("\nReactOS IP Configuration\n\n"));
    if (bAll)
    {
        _tprintf(_T("\tHost Name . . . . . . . . . . . . : %s\n"), pFixedInfo->HostName);
        _tprintf(_T("\tPrimary DNS Suffix. . . . . . . . : %s\n"), (pszDomainName != NULL) ? pszDomainName : "");
        _tprintf(_T("\tNode Type . . . . . . . . . . . . : %s\n"), GetNodeTypeName(pFixedInfo->NodeType));
        if (pFixedInfo->EnableRouting)
            _tprintf(_T("\tIP Routing Enabled. . . . . . . . : Yes\n"));
        else
            _tprintf(_T("\tIP Routing Enabled. . . . . . . . : No\n"));
        if (pAdapter && pAdapter->HaveWins)
            _tprintf(_T("\tWINS Proxy enabled. . . . . . . . : Yes\n"));
        else
            _tprintf(_T("\tWINS Proxy enabled. . . . . . . . : No\n"));
        if (pszDomainName != NULL && pszDomainName[0] != 0)
        {
            _tprintf(_T("\tDNS Suffix Search List. . . . . . : %s\n"), pszDomainName);
            _tprintf(_T("\t                                    %s\n"), pFixedInfo->DomainName);
        }
        else
        {
            _tprintf(_T("\tDNS Suffix Search List. . . . . . : %s\n"), pFixedInfo->DomainName);
        }
    }

    while (pAdapter)
    {
        LPTSTR IntType, myConType;
        BOOLEAN bConnected = TRUE;

        mibEntry.dwIndex = pAdapter->Index;
        GetIfEntry(&mibEntry);

        IntType = GetInterfaceTypeName(pAdapter->Type);
        myConType = GetConnectionType(pAdapter->AdapterName);

        _tprintf(_T("\n%s %s: \n\n"), IntType , myConType);

        if (myConType != NULL) HeapFree(ProcessHeap, 0, myConType);

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
            _tprintf(_T("\tMedia State . . . . . . . . . . . : Media disconnected\n"));
        }
        else
        {
            _tprintf(_T("\tConnection-specific DNS Suffix. . : %s\n"), pFixedInfo->DomainName);
        }

        if (bAll)
        {
            LPTSTR lpDesc = GetConnectionDescription(pAdapter->AdapterName);
            _tprintf(_T("\tDescription . . . . . . . . . . . : %s\n"), lpDesc);
            HeapFree(ProcessHeap, 0, lpDesc);
            _tprintf(_T("\tPhysical Address. . . . . . . . . : %s\n"), PrintMacAddr(pAdapter->Address));
            if (bConnected)
            {
                if (pAdapter->DhcpEnabled)
                {
                    _tprintf(_T("\tDHCP Enabled. . . . . . . . . . . : Yes\n"));
                    if (pPerAdapterInfo != NULL)
                    {
                        if (pPerAdapterInfo->AutoconfigEnabled)
                            _tprintf(_T("\tAutoconfiguration Enabled . . . . : Yes\n"));
                        else
                            _tprintf(_T("\tAutoconfiguration Enabled . . . . : No\n"));
                    }
                }
                else
                {
                    _tprintf(_T("\tDHCP Enabled. . . . . . . . . . . : No\n"));
                }
            }
        }

        if (!bConnected)
        {
            pAdapter = pAdapter->Next;
            continue;
        }

        _tprintf(_T("\tIP Address. . . . . . . . . . . . : %s\n"), pAdapter->IpAddressList.IpAddress.String);
        _tprintf(_T("\tSubnet Mask . . . . . . . . . . . : %s\n"), pAdapter->IpAddressList.IpMask.String);
        if (pAdapter->GatewayList.IpAddress.String[0] != '0')
            _tprintf(_T("\tDefault Gateway . . . . . . . . . : %s\n"), pAdapter->GatewayList.IpAddress.String);
        else
            _tprintf(_T("\tDefault Gateway . . . . . . . . . :\n"));

        if (bAll)
        {
            PIP_ADDR_STRING pIPAddr;

            if (pAdapter->DhcpEnabled)
               _tprintf(_T("\tDHCP Server . . . . . . . . . . . : %s\n"), pAdapter->DhcpServer.IpAddress.String);

            _tprintf(_T("\tDNS Servers . . . . . . . . . . . : "));
            _tprintf(_T("%s\n"), pFixedInfo->DnsServerList.IpAddress.String);
            pIPAddr = pFixedInfo->DnsServerList.Next;
            while (pIPAddr)
            {
                _tprintf(_T("\t\t\t\t\t    %s\n"), pIPAddr ->IpAddress.String );
                pIPAddr = pIPAddr->Next;
            }

            if (pAdapter->HaveWins)
            {
                _tprintf(_T("\tPrimary WINS Server . . . . . . . : %s\n"), pAdapter->PrimaryWinsServer.IpAddress.String);
                _tprintf(_T("\tSecondary WINS Server . . . . . . : %s\n"), pAdapter->SecondaryWinsServer.IpAddress.String);
            }

            if (pAdapter->DhcpEnabled && _tcscmp(pAdapter->DhcpServer.IpAddress.String, _T("255.255.255.255")))
            {
                _tprintf(_T("\tLease Obtained. . . . . . . . . . : %s"), _tasctime(localtime(&pAdapter->LeaseObtained)));
                _tprintf(_T("\tLease Expires . . . . . . . . . . : %s"), _tasctime(localtime(&pAdapter->LeaseExpires)));
            }
        }
        _tprintf(_T("\n"));

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

VOID Release(LPTSTR Index)
{
    IP_ADAPTER_INDEX_MAP AdapterInfo;
    DWORD ret;
    DWORD i;

    /* if interface is not given, query GetInterfaceInfo */
    if (Index == NULL)
    {
        PIP_INTERFACE_INFO pInfo = NULL;
        ULONG ulOutBufLen = 0;

        if (GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
        {
            pInfo = (IP_INTERFACE_INFO *)HeapAlloc(ProcessHeap, 0, ulOutBufLen);
            if (pInfo == NULL)
                return;

            if (GetInterfaceInfo(pInfo, &ulOutBufLen) == NO_ERROR )
            {
                for (i = 0; i < pInfo->NumAdapters; i++)
                {
                    CopyMemory(&AdapterInfo, &pInfo->Adapter[i], sizeof(IP_ADAPTER_INDEX_MAP));
                    _tprintf(_T("name - %ls\n"), pInfo->Adapter[i].Name);

                    /* Call IpReleaseAddress to release the IP address on the specified adapter. */
                    if ((ret = IpReleaseAddress(&AdapterInfo)) != NO_ERROR)
                    {
                        _tprintf(_T("\nAn error occured while releasing interface %ls : \n"), AdapterInfo.Name);
                        DoFormatMessage(ret);
                    }
                }

                HeapFree(ProcessHeap, 0, pInfo);
            }
            else
            {
                DoFormatMessage(0);
                HeapFree(ProcessHeap, 0, pInfo);
                return;
            }
        }
        else
        {
            DoFormatMessage(0);
            return;
        }
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
}




VOID Renew(LPTSTR Index)
{
    IP_ADAPTER_INDEX_MAP AdapterInfo;
    DWORD i;

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
        if (GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
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
        if (GetInterfaceInfo(pInfo, &ulOutBufLen) == NO_ERROR)
        {
            for (i = 0; i < pInfo->NumAdapters; i++)
            {
                CopyMemory(&AdapterInfo, &pInfo->Adapter[i], sizeof(IP_ADAPTER_INDEX_MAP));
                _tprintf(_T("name - %ls\n"), pInfo->Adapter[i].Name);

                /* Call IpRenewAddress to renew the IP address on the specified adapter. */
                if (IpRenewAddress(&AdapterInfo) != NO_ERROR)
                {
                    _tprintf(_T("\nAn error occured while renew interface %s : "), _T("*name*"));
                    DoFormatMessage(0);
                }
            }
        }
        else
        {
            _tprintf(_T("\nGetInterfaceInfo failed : "));
            DoFormatMessage(0);
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
}

VOID
FlushDns(VOID)
{
    _tprintf(_T("\nReactOS IP Configuration\n\n"));

    if (DnsFlushResolverCache())
        _tprintf(_T("The DNS Resolver Cache has been deleted.\n"));
    else
        DoFormatMessage(GetLastError());
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
            _tprintf(_T("\t%S\n"), pszName);
            _tprintf(_T("\t----------------------------------------\n"));
            _tprintf(_T("\tName does not exist\n\n"));
        }
        else if (Status == DNS_INFO_NO_RECORDS)
        {
            _tprintf(_T("\t%S\n"), pszName);
            _tprintf(_T("\t----------------------------------------\n"));
            _tprintf(_T("\tNo records of type %s\n\n"), GetRecordTypeName(wType));
        }
        return;
    }

    _tprintf(_T("\t%S\n"), pszName);
    _tprintf(_T("\t----------------------------------------\n"));

    pThisRecord = pQueryResults;
    while (pThisRecord != NULL)
    {
        pNextRecord = pThisRecord->pNext;

        _tprintf(_T("\tRecord Name . . . . . : %S\n"), pThisRecord->pName);
        _tprintf(_T("\tRecord Type . . . . . : %hu\n"), pThisRecord->wType);
        _tprintf(_T("\tTime To Live. . . . . : %lu\n"), pThisRecord->dwTtl);
        _tprintf(_T("\tData Length . . . . . : %hu\n"), pThisRecord->wDataLength);

        switch (pThisRecord->Flags.S.Section)
        {
            case DnsSectionQuestion:
                _tprintf(_T("\tSection . . . . . . . : Question\n"));
                break;

            case DnsSectionAnswer:
                _tprintf(_T("\tSection . . . . . . . : Answer\n"));
                break;

            case DnsSectionAuthority:
                _tprintf(_T("\tSection . . . . . . . : Authority\n"));
                break;

            case DnsSectionAdditional:
                _tprintf(_T("\tSection . . . . . . . : Additional\n"));
                break;
        }

        switch (pThisRecord->wType)
        {
            case DNS_TYPE_A:
                Addr4.S_un.S_addr = pThisRecord->Data.A.IpAddress;
                RtlIpv4AddressToStringW(&Addr4, szBuffer);
                _tprintf(_T("\tA (Host) Record . . . : %S\n"), szBuffer);
                break;

            case DNS_TYPE_NS:
                _tprintf(_T("\tNS Record . . . . . . : %S\n"), pThisRecord->Data.NS.pNameHost);
                break;

            case DNS_TYPE_CNAME:
                _tprintf(_T("\tCNAME Record. . . . . : %S\n"), pThisRecord->Data.CNAME.pNameHost);
                break;

            case DNS_TYPE_SOA:
                _tprintf(_T("\tSOA Record. . . . . . : \n"));
                break;

            case DNS_TYPE_PTR:
                _tprintf(_T("\tPTR Record. . . . . . : %S\n"), pThisRecord->Data.PTR.pNameHost);
                break;

            case DNS_TYPE_MX:
                _tprintf(_T("\tMX Record . . . . . . : \n"));
                break;

            case DNS_TYPE_AAAA:
                RtlCopyMemory(&Addr6, &pThisRecord->Data.AAAA.Ip6Address, sizeof(IN6_ADDR));
                RtlIpv6AddressToStringW(&Addr6, szBuffer);
                _tprintf(_T("\tAAAA Record . . . . . : %S\n"), szBuffer);
                break;

            case DNS_TYPE_ATMA:
                _tprintf(_T("\tATMA Record . . . . . : \n"));
                break;

            case DNS_TYPE_SRV:
                _tprintf(_T("\tSRV Record. . . . . . : \n"));
                break;
        }
        _tprintf(_T("\n\n"));

        pThisRecord = pNextRecord;
    }

    DnsRecordListFree((PDNS_RECORD)pQueryResults, DnsFreeRecordList);
}


VOID
DisplayDns(VOID)
{
    PDNS_CACHE_ENTRY DnsEntry = NULL, pThisEntry, pNextEntry;

    _tprintf(_T("\nReactOS IP Configuration\n\n"));

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

        if (pThisEntry->wType1 != 0)
            DisplayDnsRecord(pThisEntry->pszName, pThisEntry->wType1);

        if (pThisEntry->wType2 != 0)
            DisplayDnsRecord(pThisEntry->pszName, pThisEntry->wType2);

        if (pThisEntry->pszName)
            LocalFree(pThisEntry->pszName);
        LocalFree(pThisEntry);

        pThisEntry = pNextEntry;
    }
}

VOID Usage(VOID)
{
    HRSRC hRes;
    LPTSTR lpUsage;
    DWORD Size;

    LPTSTR lpName = (LPTSTR)MAKEINTRESOURCE((IDS_USAGE >> 4) + 1);

    hRes = FindResource(hInstance,
                        lpName,
                        RT_STRING);
    if (hRes != NULL)
    {
        if ((Size = SizeofResource(hInstance,
                                   hRes)))
        {
            lpUsage = (LPTSTR)HeapAlloc(ProcessHeap,
                                        0,
                                        Size);
            if (lpUsage == NULL)
                return;

            if (LoadStringAndOem(hInstance,
                           IDS_USAGE,
                           lpUsage,
                           Size))
            {
                _tprintf(_T("%s"), lpUsage);
            }

            HeapFree(ProcessHeap, 0, lpUsage);
        }
    }
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
