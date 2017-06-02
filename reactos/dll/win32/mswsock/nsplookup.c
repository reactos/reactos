#include "precomp.h"

#include <stdlib.h>
#include <ws2spi.h>
#include <nspapi.h>
#include <windef.h>
#include <winuser.h>
#include <windns.h>
#include <guiddef.h>
#include <svcguid.h>
#include <iptypes.h>
#include <strsafe.h>
#include <winreg.h>

#include "mswhelper.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(mswsock);

#define NSP_CALLID_DNS 0x0001
#define NSP_CALLID_HOSTNAME 0x0002
#define NSP_CALLID_HOSTBYNAME 0x0003
#define NSP_CALLID_SERVICEBYNAME 0x0004

#ifndef BUFSIZ
  #define BUFSIZ 1024
#endif // BUFSIZ
#ifndef WS2_INTERNAL_MAX_ALIAS
  #define WS2_INTERNAL_MAX_ALIAS 512
#endif // WS2_INTERNAL_MAX_ALIAS

//#define NSP_REDIRECT

typedef struct {
    WCHAR* hostnameW;
    DWORD addr4;
    WCHAR* servnameW;
    WCHAR* servprotoW;
    CHAR** servaliasesA; /* array */
    WORD servport;
} WSHOSTINFOINTERN, *PWSHOSTINFOINTERN;

typedef struct {
    GUID providerId; /* Provider-ID */
    DWORD dwControlFlags; /* dwControlFlags (WSALookupServiceBegin) */
    DWORD CallID; /* List for LookupServiceNext-Calls */
    DWORD CallIDCounter; /* call-count of the current CallID. */
    WCHAR* hostnameW; /* hostbyname */
#ifdef NSP_REDIRECT
    HANDLE rdrLookup;
    NSP_ROUTINE rdrproc;
#endif
} WSHANDLEINTERN, *PWSHANDLEINTERN;

static const GUID guid_NULL = {0};
static const GUID guid_HOSTNAME = SVCID_HOSTNAME;
static const GUID guid_INET_HOSTADDRBYINETSTRING = SVCID_INET_HOSTADDRBYINETSTRING;
static const GUID guid_INET_HOSTADDRBYNAME = SVCID_INET_HOSTADDRBYNAME;
static const GUID guid_INET_SERVICEBYNAME = SVCID_INET_SERVICEBYNAME;

/* GUIDs - maybe they should be loaded from registry? */
/* Namespace: 32 */
static const GUID guid_mswsock_TcpIp = {/*Data1:*/ 0x22059D40,
                                        /*Data2:*/ 0x7E9E,
                                        /*Data3:*/ 0x11CF,
                                        /*Data4:*/ {0xAE, 0x5A, 0x00, 0xAA, 0x00, 0xA7, 0x11, 0x2B}};

/* {6642243A-3BA8-4AA6-BAA5-2E0BD71FDD83} */
/* Namespace: 15 */
static const GUID guid_mswsock_NLA = {/*Data1:*/ 0x6642243A,
                                      /*Data2:*/ 0x3BA8,
                                      /*Data3:*/ 0x4AA6,
                                      /*Data4:*/ {0xBA, 0xA5, 0x2E, 0x0B, 0xD7, 0x1F, 0xDD, 0x83}};

#ifdef NSP_REDIRECT

typedef INT
(CALLBACK *lpRdrNSPStartup)(
  LPGUID lpProviderId,
  LPNSP_ROUTINE lpRout);

const rdrLib = "mswsock.dll-original";
lpRdrNSPStartup rdrNSPStartup;
HANDLE hLib;
NSP_ROUTINE rdrproc_tcpip;
NSP_ROUTINE rdrproc_nla;

#endif /* NSP_REDIRECT */

/* Implementations - Internal */

/*
    hostnameA / hostnameW
    * only used by HOSTBYNAME
    * only one should be set

*/
INT
NSP_LookupServiceBeginW(PWSHANDLEINTERN data,
                        CHAR* hostnameA,
                        WCHAR* hostnameW,
                        DWORD CallID)
{
    HANDLE hHeap;

    TRACE("NSP_LookupServiceBeginW %p %p %p %lx\n", data, hostnameA, hostnameW, CallID);
    if (data->CallID != 0)
        return WSAEFAULT;

    data->CallID = CallID;

    if ((CallID == NSP_CALLID_HOSTBYNAME) ||
        (CallID == NSP_CALLID_SERVICEBYNAME))
    {
        hHeap = GetProcessHeap();

        if (data->hostnameW != NULL)
            HeapFree(hHeap, 0, data->hostnameW);

        if (hostnameA != NULL)
        {
            data->hostnameW = StrA2WHeapAlloc(hHeap, hostnameA);
        }
        else
        {
            data->hostnameW = StrCpyHeapAllocW(hHeap, hostnameW);
        }
    }
    else
    {
        ERR("NSP_LookupServiceBeginW unsupported CallID\n");
        WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return ERROR_SUCCESS;
}

INT
NSP_GetHostNameHeapAllocW(_Out_ WCHAR** hostname)
{
    WCHAR* name;
    HANDLE hHeap = GetProcessHeap();
    DWORD bufCharLen = 0;

    TRACE("NSP_GetHostNameHeapAllocW %p\n", hostname);
    /* FIXME Use DnsGetHostName_W when available */
    GetComputerNameExW(ComputerNameDnsHostname, NULL, &bufCharLen);
    if (!bufCharLen)
    {
        ERR("NSP_GetHostNameHeapAllocW zero size for computername returned\n");
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }
    name = HeapAlloc(hHeap, 0, bufCharLen*sizeof(WCHAR));
    if (!GetComputerNameExW(ComputerNameDnsHostname,
                            name,
                            &bufCharLen))
    {
        ERR("NSP_GetHostNameHeapAllocW error obtaining computername %lx\n", GetLastError());
        HeapFree(hHeap, 0, name);
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    *hostname = name;
    return ERROR_SUCCESS;
}

INT
NSP_GetHostByNameHeapAllocW(_In_ PWSHANDLEINTERN data,
                            _In_ DWORD dwControlFlags,
                            _Out_ PWSHOSTINFOINTERN hostinfo)
{
    HANDLE hHeap = GetProcessHeap();
    DNS_STATUS dns_status = { 0 };
    /* include/WinDNS.h -- look up DNS_RECORD on MSDN */
    PDNS_RECORDW dp;
    PDNS_RECORDW curr;
    INT result = ERROR_SUCCESS;
    DWORD dwQueryFlags = DNS_QUERY_STANDARD;
    PWCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    int AliasIndex = 0;

    TRACE("NSP_GetHostByNameHeapAllocW %p %lx %p\n", data, dwControlFlags, hostinfo);
    /* needed to be cleaned up if != NULL */
    dp = NULL;

    if (!data->hostnameW)
    {
        result = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((data->dwControlFlags & LUP_DEEP) == 0)
    {
        TRACE("NSP_GetHostByNameHeapAllocW LUP_DEEP is not specified. Disabling recursion\n");
        dwQueryFlags |= DNS_QUERY_NO_RECURSION;
    }

    /* DNS_TYPE_A: include/WinDNS.h */
    /* DnsQuery -- lib/dnsapi/dnsapi/query.c */
    dns_status = DnsQuery_W(data->hostnameW,
                            DNS_TYPE_A,
                            dwQueryFlags,
                            NULL /* extra dns servers */,
                            &dp,
                            NULL);
    if (dns_status == ERROR_INVALID_NAME)
    {
        ERR("NSP_GetHostByNameHeapAllocW invalid name\n");
        WSASetLastError(WSAEFAULT);
        result = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((dns_status != 0) || (dp == NULL))
    {
        ERR("NSP_GetHostByNameHeapAllocW not found %lx %p\n", dns_status, dp);
        result = WSAHOST_NOT_FOUND;
        goto cleanup;
    }

    //ASSERT(dp->wType == DNS_TYPE_A);
    //ASSERT(dp->wDataLength == sizeof(DNS_A_DATA));
    curr = dp;
    while ((curr->pNext != NULL) || (curr->wType != DNS_TYPE_A))
    {
        if (curr->wType == DNS_TYPE_CNAME)
        {
            TRACE("NSP_GetHostByNameHeapAllocW found alias %ws\n", curr->Data.Cname.pNameHost);
            Aliases[AliasIndex++] = curr->Data.Cname.pNameHost;
        }
        curr = curr->pNext;
    }

    if (curr->wType != DNS_TYPE_A)
    {
        ERR("NSP_GetHostByNameHeapAllocW last record is not of type A %d\n", curr->wType);
        result = WSASERVICE_NOT_FOUND;
        goto cleanup;
    }
    hostinfo->hostnameW = StrCpyHeapAllocW(hHeap, curr->pName);
    hostinfo->addr4 = curr->Data.A.IpAddress;
    if (AliasIndex)
    {
        hostinfo->servaliasesA = StrAryCpyHeapAllocWToA(hHeap, (WCHAR**)&Aliases);
    }
    result = ERROR_SUCCESS;

cleanup:
    if (dp != NULL)
        DnsRecordListFree(dp, DnsFreeRecordList);

    return result;
}

#define SKIPWS(ptr, act) \
{while(*ptr && isspace(*ptr)) ptr++; if(!*ptr) act;}

#define SKIPANDMARKSTR(ptr, act) \
{while(*ptr && !isspace(*ptr)) ptr++; \
 if(!*ptr) {act;} else { *ptr = 0; ptr++; }}

static
BOOL
DecodeServEntFromString(IN PCHAR ServiceString,
                        OUT PCHAR *ServiceName,
                        OUT PCHAR *PortNumberStr,
                        OUT PCHAR *ProtocolStr,
                        IN PCHAR *Aliases,
                        IN DWORD MaxAlias)
{
    UINT NAliases = 0;

    //WS_DbgPrint(MAX_TRACE, ("Parsing service ent [%s]\n", ServiceString));

    SKIPWS(ServiceString, return FALSE);
    *ServiceName = ServiceString;
    SKIPANDMARKSTR(ServiceString, return FALSE);
    SKIPWS(ServiceString, return FALSE);
    *PortNumberStr = ServiceString;
    SKIPANDMARKSTR(ServiceString, ;);

    while (*ServiceString && NAliases < MaxAlias - 1)
    {
        SKIPWS(ServiceString, break);
        if (*ServiceString)
        {
            SKIPWS(ServiceString, ;);
            if (strlen(ServiceString))
            {
                //WS_DbgPrint(MAX_TRACE, ("Alias: %s\n", ServiceString));
                *Aliases++ = ServiceString;
                NAliases++;
            }
            SKIPANDMARKSTR(ServiceString, ;);
        }
    }
    *Aliases = NULL;

    *ProtocolStr = strchr(*PortNumberStr, '/');

    if (!*ProtocolStr)
        return FALSE;

    **ProtocolStr = 0;
    (*ProtocolStr)++;

    //WS_DbgPrint(MAX_TRACE, ("Parsing done: %s %s %s %d\n",
    //           *ServiceName, *ProtocolStr, *PortNumberStr,
    //            NAliases));

    return TRUE;
}

HANDLE
WSAAPI
OpenNetworkDatabase(_In_ LPCWSTR Name)
{
    PWSTR ExpandedPath;
    PWSTR DatabasePath;
    INT ErrorCode;
    HKEY DatabaseKey;
    DWORD RegType;
    DWORD RegSize = 0;
    size_t StringLength;
    HANDLE Handle;

    TRACE("OpenNetworkDatabase %p\n", Name);
    ExpandedPath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(WCHAR));
    if (!ExpandedPath)
        return INVALID_HANDLE_VALUE;

    /* Open the database path key */
    ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                             0,
                             KEY_READ,
                             &DatabaseKey);
    if (ErrorCode == NO_ERROR)
    {
        TRACE("OpenNetworkDatabase registry key for network database exist\n");
        /* Read the actual path */
        ErrorCode = RegQueryValueEx(DatabaseKey,
                                    L"DatabasePath",
                                    NULL,
                                    &RegType,
                                    NULL,
                                    &RegSize);

        if (!RegSize)
        {
            ERR("OpenNetworkDatabase RegQueryValueEx failed to return size for DatabasePath %lx\n", ErrorCode);
            RegCloseKey(DatabaseKey);
            HeapFree(GetProcessHeap(), 0, ExpandedPath);
            return INVALID_HANDLE_VALUE;
        }
        DatabasePath = HeapAlloc(GetProcessHeap(), 0, RegSize);
        if (!DatabasePath)
        {
            ERR("OpenNetworkDatabase could not allocate %d for DatabasePath\n", RegSize);
            RegCloseKey(DatabaseKey);
            HeapFree(GetProcessHeap(), 0, ExpandedPath);
            return INVALID_HANDLE_VALUE;
        }

        /* Read the actual path */
        ErrorCode = RegQueryValueEx(DatabaseKey,
                                    L"DatabasePath",
                                    NULL,
                                    &RegType,
                                    (LPBYTE)DatabasePath,
                                    &RegSize);

        /* Close the key */
        RegCloseKey(DatabaseKey);

        if (ErrorCode)
        {
            ERR("OpenNetworkDatabase RegQueryValueEx failed to return value for DatabasePath %lx\n", ErrorCode);
            HeapFree(GetProcessHeap(), 0, DatabasePath);
            HeapFree(GetProcessHeap(), 0, ExpandedPath);
            return INVALID_HANDLE_VALUE;
        }

        /* Expand the name */
        ExpandEnvironmentStrings(DatabasePath, ExpandedPath, MAX_PATH);

        HeapFree(GetProcessHeap(), 0, DatabasePath);
    }
    else
    {
        TRACE("OpenNetworkDatabase registry key for network database doesn't exist\n");
        /* Use defalt path */
        GetSystemDirectory(ExpandedPath, MAX_PATH);
        StringCchLength(ExpandedPath, MAX_PATH, &StringLength);
        if (ExpandedPath[StringLength - 1] != L'\\')
        {
            /* It isn't, so add it ourselves */
            StringCchCat(ExpandedPath, MAX_PATH, L"\\");
        }
        StringCchCat(ExpandedPath, MAX_PATH, L"DRIVERS\\ETC\\");
    }

    /* Make sure that the path is backslash-terminated */
    StringCchLength(ExpandedPath, MAX_PATH, &StringLength);
    if (ExpandedPath[StringLength - 1] != L'\\')
    {
        /* It isn't, so add it ourselves */
        StringCchCat(ExpandedPath, MAX_PATH, L"\\");
    }

    /* Add the database name */
    StringCchCat(ExpandedPath, MAX_PATH, Name);

    /* Return a handle to the file */
    Handle = CreateFile(ExpandedPath,
                        FILE_READ_DATA,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    HeapFree(GetProcessHeap(), 0, ExpandedPath);
    return Handle;
}

INT
NSP_GetServiceByNameHeapAllocW(_In_ PWSHANDLEINTERN data,
                               _In_ DWORD dwControlFlags,
                               _Out_ PWSHOSTINFOINTERN hostinfo)
{
    BOOL Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ * sizeof(WCHAR)] = { 0 };
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0,
    ProtocolStr = 0, Comment = 0, EndValid;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = { 0 };
    PCHAR* AliasPtr;
    UINT i = 0;
    DWORD ReadSize = 0;
    HANDLE hHeap;
    PCHAR nameA = NULL;
    PCHAR nameServiceA = NULL;
    PCHAR nameProtoA = NULL;
    INT res = WSANO_RECOVERY;
    
    TRACE("NSP_GetServiceByNameHeapAllocW %p %lx %p\n", data, dwControlFlags, hostinfo);
    if (!data->hostnameW)
    {
        ERR("NSP_GetServiceByNameHeapAllocW service name not provided\n");
        res = WSANO_RECOVERY;
        goto End;
    }

    hHeap = GetProcessHeap();
    nameA = StrW2AHeapAlloc(hHeap, data->hostnameW);

    /* nameA has the form <service-name>/<protocol>
       we split these now */
    nameProtoA = strchr(nameA, '/');
    if (nameProtoA == NULL)
    {
        ERR("NSP_GetServiceByNameHeapAllocW invalid service name %s\n", nameA);
        res = WSANO_RECOVERY;
        goto End;
    }

    nameProtoA++;
    i = (DWORD)(nameProtoA - nameA - 1);
    nameServiceA = (PCHAR)HeapAlloc(hHeap, 0, i + 1);
    StringCbCopyA(nameServiceA, i + 1, nameA);
    nameServiceA[i] = '\0';

    ServicesFile = OpenNetworkDatabase(L"services");
    if (ServicesFile == INVALID_HANDLE_VALUE)
    {
        ERR("NSP_GetServiceByNameHeapAllocW unable to open services file\n");
        return WSANO_RECOVERY;
    }

    /* Scan the services file ...
    *
    * We will be share the buffer on the lines. If the line does not fit in
    * the buffer, then moving it to the beginning of the buffer and read
    * the remnants of line from file.
    */

    /* Initial Read */
    if (!ReadFile(ServicesFile,
                  ServiceDBData,
                  sizeof( ServiceDBData ) - 1,
                  &ReadSize,
                  NULL))
    {
        ERR("NSP_GetServiceByNameHeapAllocW can't read services file %lx\n", GetLastError());
        CloseHandle(ServicesFile);
        return WSANO_RECOVERY;
    }

    ThisLine = NextLine = ServiceDBData;
    EndValid = ServiceDBData + ReadSize;
    ServiceDBData[sizeof(ServiceDBData) - 1] = '\0';

    while (ReadSize)
    {
        for (; *NextLine != '\r' && *NextLine != '\n'; NextLine++)
        {
            if (NextLine == EndValid)
            {
                int LineLen = NextLine - ThisLine;

                if (ThisLine == ServiceDBData)
                {
                    ERR("NSP_GetServiceByNameHeapAllocW line too long\n");
                    CloseHandle(ServicesFile);
                    return WSANO_RECOVERY;
                }

                memmove(ServiceDBData, ThisLine, LineLen);

                if (!ReadFile(ServicesFile,
                              ServiceDBData + LineLen,
                              sizeof( ServiceDBData )-1 - LineLen,
                              &ReadSize,
                              NULL))
                {
                    break;
                }

                EndValid = ServiceDBData + LineLen + ReadSize;
                NextLine = ServiceDBData + LineLen;
                ThisLine = ServiceDBData;

                if (!ReadSize) break;
            }
        }

        *NextLine = '\0';
        Comment = strchr(ThisLine, '#');

        if (Comment)
            *Comment = '\0'; /* Terminate at comment start */

        if (DecodeServEntFromString(ThisLine,
                                    &ServiceName,
                                    &PortNumberStr,
                                    &ProtocolStr,
                                    Aliases,
                                    WS2_INTERNAL_MAX_ALIAS) &&
            (strlen(nameProtoA) == 0 || strcmp(ProtocolStr, nameProtoA) == 0))
        {
            Found = (strcmp(ServiceName, nameServiceA) == 0 || strcmp(PortNumberStr, nameServiceA) == 0);
            AliasPtr = Aliases;
            while ((!Found) && (*AliasPtr != NULL))
            {
                Found = (strcmp(*AliasPtr, nameServiceA) == 0);
                AliasPtr++;
            }
            if (Found)
                break;
        }
        NextLine++;
        ThisLine = NextLine;
    }

    /* This we'll do no matter what */
    CloseHandle(ServicesFile);

    if (!Found)
    {
        ERR("NSP_GetServiceByNameHeapAllocW service not found\n");
        return WSANO_DATA;
    }

    hostinfo->addr4 = 0;
    hostinfo->servnameW = StrA2WHeapAlloc(hHeap, ServiceName);
    hostinfo->servprotoW = StrA2WHeapAlloc(hHeap, ProtocolStr);
    hostinfo->servaliasesA = StrAryCpyHeapAllocA(hHeap, (char**)&Aliases);
    hostinfo->servport = atoi(PortNumberStr);

    res = NO_ERROR;

End:
    if (nameA)
        HeapFree(hHeap, 0, nameA);

    if (nameServiceA)
        HeapFree(hHeap, 0, nameServiceA);

    return res;
}

INT
NSP_LookupServiceNextW(_In_ PWSHANDLEINTERN data,
                       _In_ DWORD dwControlFlags,
                       _Inout_ LPWSAQUERYSETW lpRes,
                       _Inout_ LPDWORD lpResLen)
{
    MSW_BUFFER buf;
    WSHOSTINFOINTERN hostinfo;
    INT result;
    HANDLE hHeap = GetProcessHeap();
    WCHAR* ServiceInstanceNameW = NULL;
    /* cleanup-vars */
    CHAR* ServiceInstanceNameA = NULL;
    CHAR* ServiceProtocolNameA = NULL;

    TRACE("NSP_LookupServiceNextW %p %lx %p %p\n", data, dwControlFlags, lpRes, lpResLen);
    if (!data || (dwControlFlags & (~(DWORD)LUP_FLUSHPREVIOUS)) != 0 || !lpRes || !lpResLen || *lpResLen == 0)
        return WSAEINVAL;
    RtlZeroMemory(&hostinfo, sizeof(hostinfo));

    /* init and build result-buffer */
    mswBufferInit(&buf, (BYTE*)lpRes, *lpResLen);
    mswBufferIncUsed(&buf, sizeof(*lpRes));

    /* QueryDataSet-Size without "blob-data"-size! */
    lpRes->dwSize = sizeof(*lpRes);
    lpRes->dwNameSpace = NS_DNS;

    if ((data->CallID == NSP_CALLID_HOSTNAME) ||
        (data->CallID == NSP_CALLID_HOSTBYNAME) ||
        (data->CallID == NSP_CALLID_SERVICEBYNAME))
    {
        /* FIXME remember what was returned and continue from there */
        if (data->CallIDCounter >= 1)
        {
            ERR("NSP_LookupServiceNextW LUP_FLUSHPREVIOUS and more than one call not supported yet\n", data, dwControlFlags, lpRes, lpResLen);
            result = WSA_E_NO_MORE;
            goto End;
        }
    }
    else
    {
        ERR("NSP_LookupServiceNextW unsupported CallID %lx\n", data->CallID);
        result = WSAEOPNOTSUPP;
        goto End;
    }
    data->CallIDCounter++;

    if (data->CallID == NSP_CALLID_HOSTNAME)
    {
        result = NSP_GetHostNameHeapAllocW(&hostinfo.hostnameW);

        if (result != ERROR_SUCCESS)
            goto End;

        hostinfo.addr4 = 0;
    }
    else if (data->CallID == NSP_CALLID_HOSTBYNAME)
    {
        result = NSP_GetHostByNameHeapAllocW(data,
                                             dwControlFlags,
                                             &hostinfo);
        if (result != ERROR_SUCCESS)
            goto End;
    }
    else
    {
        //ASSERT(data->CallID == NSP_CALLID_SERVICEBYNAME);
        result = NSP_GetServiceByNameHeapAllocW(data,
                                                dwControlFlags,
                                                &hostinfo);
        if (result != ERROR_SUCCESS)
            goto End;
    }

    if (((LUP_RETURN_BLOB & data->dwControlFlags) != 0) ||
        ((LUP_RETURN_NAME & data->dwControlFlags) != 0))
    {
        if (data->CallID == NSP_CALLID_HOSTNAME || data->CallID == NSP_CALLID_HOSTBYNAME)
        {
            ServiceInstanceNameW = hostinfo.hostnameW;
            ServiceInstanceNameA = StrW2AHeapAlloc(hHeap, ServiceInstanceNameW);
            if (!ServiceInstanceNameA)
            {
                ERR("NSP_LookupServiceNextW not enough memory\n");
                result = WSA_NOT_ENOUGH_MEMORY;
                goto End;
            }
        }
        if (data->CallID == NSP_CALLID_SERVICEBYNAME)
        {
            ServiceInstanceNameW = hostinfo.servnameW;
            ServiceInstanceNameA = StrW2AHeapAlloc(hHeap, ServiceInstanceNameW);
            if (!ServiceInstanceNameA)
            {
                ERR("NSP_LookupServiceNextW not enough memory\n");
                result = WSA_NOT_ENOUGH_MEMORY;
                goto End;
            }
            ServiceProtocolNameA = StrW2AHeapAlloc(hHeap, hostinfo.servprotoW);
            if (!ServiceProtocolNameA)
            {
                ERR("NSP_LookupServiceNextW not enough memory\n");
                result = WSA_NOT_ENOUGH_MEMORY;
                goto End;
            }
        }
    }

    if ((LUP_RETURN_ADDR & data->dwControlFlags) != 0)
    {
        if (!mswBufferAppendAddr_AddrInfoW(&buf, lpRes, hostinfo.addr4))
        {
            ERR("NSP_LookupServiceNextW provided buffer is too small\n");
            *lpResLen = buf.bytesUsed;
            result = WSAEFAULT;
            goto End;
        }
    }

    if ((LUP_RETURN_BLOB & data->dwControlFlags) != 0)
    {
        if (data->CallID == NSP_CALLID_HOSTBYNAME)
        {
            /* Write data for PBLOB (hostent) */
            if (!mswBufferAppendBlob_Hostent(&buf,
                                             lpRes,
                                             (LUP_RETURN_ALIASES & data->dwControlFlags) != 0 ? hostinfo.servaliasesA : NULL,
                                             ServiceInstanceNameA,
                                             hostinfo.addr4))
            {
                ERR("NSP_LookupServiceNextW provided buffer is too small\n");
                *lpResLen = buf.bytesUsed;
                result = WSAEFAULT;
                goto End;
            }
        }
        else if (data->CallID == NSP_CALLID_SERVICEBYNAME)
        {
            /* Write data for PBLOB (servent) */
            if (!mswBufferAppendBlob_Servent(&buf,
                                             lpRes,
                                             ServiceInstanceNameA,/* ServiceName */
                                             (LUP_RETURN_ALIASES & data->dwControlFlags) != 0 ? hostinfo.servaliasesA : NULL,
                                             ServiceProtocolNameA,
                                             hostinfo.servport))
            {
                ERR("NSP_LookupServiceNextW provided buffer is too small\n");
                *lpResLen = buf.bytesUsed;
                result = WSAEFAULT;
                goto End;
            }
        }
        else
        {
            ERR("NSP_LookupServiceNextW LUP_RETURN_BLOB is supported only for NSP_CALLID_HOSTBYNAME and NSP_CALLID_SERVICEBYNAME\n");
            result = WSAEINVAL;
            goto End;
        }
    }

    if ((LUP_RETURN_NAME & data->dwControlFlags) != 0)
    {
        /* HostByName sets the ServiceInstanceName to a
           (UNICODE)copy of hostent.h_name */
        lpRes->lpszServiceInstanceName = (LPWSTR)mswBufferEndPtr(&buf);
        if (!mswBufferAppendStrW(&buf, ServiceInstanceNameW))
        {
            ERR("NSP_LookupServiceNextW provided buffer is too small\n");
            lpRes->lpszServiceInstanceName = NULL;
            *lpResLen = buf.bytesUsed;
            result = WSAEFAULT;
            goto End;
        }
    }

    *lpResLen = buf.bytesUsed;

    result = ERROR_SUCCESS;
End:
    /* cleanup */
    if (ServiceInstanceNameA != NULL)
        HeapFree(hHeap, 0, ServiceInstanceNameA);

    if (ServiceProtocolNameA != NULL)
        HeapFree(hHeap, 0, ServiceProtocolNameA);

    if (hostinfo.hostnameW != NULL)
        HeapFree(hHeap, 0, hostinfo.hostnameW);

    if (hostinfo.servnameW != NULL)
        HeapFree(hHeap, 0, hostinfo.servnameW);

    if (hostinfo.servprotoW != NULL)
        HeapFree(hHeap, 0, hostinfo.servprotoW);

    TRACE("NSP_LookupServiceNextW returns %d needed bytes %ld\n", result, buf.bytesUsed);
    return result;
}

INT
WSAAPI
mwsNSPCleanUp(_In_ LPGUID lpProviderId)
{
    return ERROR_SUCCESS;
}

INT
mwsNSPInit(VOID)
{
    return ERROR_SUCCESS;
}

INT
WSAAPI
mwsNSPLookupServiceBegin(_In_ LPGUID lpProviderId,
                         _In_ LPWSAQUERYSETW lpqsRestrictions,
                         _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo,
                         _In_ DWORD dwControlFlags,
                         _Out_ LPHANDLE lphLookup)
{
    PWSHANDLEINTERN pLook;
    int wsaErr;

    TRACE("mwsNSPLookupServiceBegin %p %p %p %lx %p\n", lpProviderId, lpqsRestrictions, lpServiceClassInfo, dwControlFlags, lphLookup);
    if (IsEqualGUID(lpProviderId, &guid_mswsock_TcpIp))
    {
        //OK
        TRACE("TCPIP query\n");
    }
    else if (IsEqualGUID(lpProviderId, &guid_mswsock_NLA))
    {
        ERR("NLA queries are not supported yet\n");
        WSASetLastError(WSASERVICE_NOT_FOUND);
        return SOCKET_ERROR;
    }
    else
    {
        ERR("Unsupported GUID\n");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    /* allocate internal structure */
    pLook = HeapAlloc(GetProcessHeap(), 0, sizeof(WSHANDLEINTERN));
    if (!pLook)
    {
        ERR("Error allocating %d for handle\n", sizeof(WSHANDLEINTERN));
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    *lphLookup = (HANDLE)pLook;

    RtlZeroMemory(pLook, sizeof(*pLook));

    /* Anyway the ControlFlags "should" be needed
       in NSPLookupServiceNext. (see doku) But
       thats not the fact ATM. */
    pLook->dwControlFlags = dwControlFlags;
    pLook->providerId = *lpProviderId;

#ifdef NSP_REDIRECT

    if (IsEqualGUID(lpProviderId, &guid_mswsock_TcpIp))
    {
        pLook->rdrproc = rdrproc_tcpip;
    }
    else if (IsEqualGUID(lpProviderId, &guid_mswsock_NLA))
    {
        pLook->rdrproc = rdrproc_nla;
    }
    else
    {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (pLook->rdrproc.NSPLookupServiceBegin(lpProviderId,
                                             lpqsRestrictions,
                                             lpServiceClassInfo,
                                             dwControlFlags,
                                             &pLook->rdrLookup) == NO_ERROR)
    {
        wsaErr = NO_ERROR;
    }
    else
    {
        wsaErr = WSAGetLastError();
    }

    /*
    if (res)
        res = WSAGetLastError();
    */

#else /* NSP_REDIRECT */

    wsaErr = ERROR_CALL_NOT_IMPLEMENTED;
    if (IsEqualGUID(lpqsRestrictions->lpServiceClassId, &guid_NULL))
    {
        ERR("NULL GUID service class is not implemented yet\n");
        wsaErr = ERROR_CALL_NOT_IMPLEMENTED;
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId, &guid_HOSTNAME))
    {
        TRACE("HOSTNAME GUID\n");
        wsaErr = NSP_LookupServiceBeginW(pLook,
                                         NULL,
                                         NULL,
                                         NSP_CALLID_HOSTNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_HOSTADDRBYNAME))
    {
        TRACE("INET_HOSTADDRBYNAME GUID\n");
        wsaErr = NSP_LookupServiceBeginW(pLook,
                                         NULL,
                                         lpqsRestrictions->lpszServiceInstanceName,
                                         NSP_CALLID_HOSTBYNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_SERVICEBYNAME))
    {
        TRACE("INET_SERVICEBYNAME\n");
        wsaErr = NSP_LookupServiceBeginW(pLook,
                                         NULL,
                                         lpqsRestrictions->lpszServiceInstanceName,
                                         NSP_CALLID_SERVICEBYNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_HOSTADDRBYINETSTRING))
    {
        ERR("INET_HOSTADDRBYINETSTRING GUID service class is not implemented yet\n");
        wsaErr = ERROR_CALL_NOT_IMPLEMENTED;
    }

#endif /* NSP_REDIRECT */

    if (wsaErr != NO_ERROR)
    {
        ERR("mwsNSPLookupServiceBegin wsaErr = %d\n", wsaErr);
        WSASetLastError(wsaErr);
        return SOCKET_ERROR;
    }
    return NO_ERROR;
}

INT
WSAAPI
mwsNSPLookupServiceNext(_In_ HANDLE hLookup,
                        _In_ DWORD dwControlFlags,
                        _Inout_ LPDWORD lpdwBufferLength,
                        //_Out_writes_bytes_to_(*lpdwBufferLength, *lpdwBufferLength)
                        LPWSAQUERYSETW lpqsResults)
{
    PWSHANDLEINTERN pLook = hLookup;
    int wsaErr = 0;

    TRACE("mwsNSPLookupServiceNext %p %lx %p %p\n", pLook, dwControlFlags, lpdwBufferLength, lpqsResults);
#ifdef NSP_REDIRECT

    INT res = pLook->rdrproc.NSPLookupServiceNext(pLook->rdrLookup,
                                                  dwControlFlags,
                                                  lpdwBufferLength,
                                                  lpqsResults);
    wsaErr = WSAGetLastError();
    if (res != ERROR_SUCCESS)
    {
        wsaErr = WSAGetLastError();

        if (wsaErr == 0)
            wsaErr = 0xFFFFFFFF;
    }

#else /* NSP_REDIRECT */

    if ((lpdwBufferLength == NULL) || (*lpdwBufferLength == 0))
    {
        wsaErr = WSA_NOT_ENOUGH_MEMORY;
        goto End;
    }

    RtlZeroMemory(lpqsResults, *lpdwBufferLength);
    lpqsResults->dwSize = sizeof(*lpqsResults);

    wsaErr = NSP_LookupServiceNextW(pLook,
                                    dwControlFlags,
                                    lpqsResults,
                                    lpdwBufferLength);


#endif /* NSP_REDIRECT */

End:
    if (wsaErr != 0)
    {
        ERR("mwsNSPLookupServiceNext wsaErr = %d\n", wsaErr);
        WSASetLastError(wsaErr);
        return SOCKET_ERROR;
    }
    return NO_ERROR;
}

INT
WSAAPI
mwsNSPIoCtl(_In_ HANDLE hLookup,
            _In_ DWORD dwControlCode,
            _In_reads_bytes_(cbInBuffer) LPVOID lpvInBuffer,
            _In_ DWORD cbInBuffer,
            _Out_writes_bytes_to_(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
            _In_ DWORD cbOutBuffer,
            _Out_ LPDWORD lpcbBytesReturned,
            _In_opt_ LPWSACOMPLETION lpCompletion,
            _In_ LPWSATHREADID lpThreadId)
{
    ERR("mwsNSPIoCtl not implemented %p %lx %p %ld %p %ld %p %p %p\n", hLookup, dwControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpCompletion, lpThreadId);
    WSASetLastError(WSAEOPNOTSUPP);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPLookupServiceEnd(_In_ HANDLE hLookup)
{
    PWSHANDLEINTERN pLook = (PWSHANDLEINTERN)hLookup;
    HANDLE hHeap = GetProcessHeap();
    INT res = NO_ERROR;

    TRACE("mwsNSPLookupServiceEnd %p\n", pLook);
#ifdef NSP_REDIRECT
    res = pLook->rdrproc.NSPLookupServiceEnd(pLook->rdrLookup);
#endif

    if (pLook->hostnameW != NULL)
        HeapFree(hHeap, 0, pLook->hostnameW);

    HeapFree(hHeap, 0, pLook);
    return res;
}

INT
WSAAPI
mwsNSPSetService(_In_ LPGUID lpProviderId,
                 _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo,
                 _In_ LPWSAQUERYSETW lpqsRegInfo,
                 _In_ WSAESETSERVICEOP essOperation,
                 _In_ DWORD dwControlFlags)
{
    ERR("mwsNSPSetService not implemented %p %p %p %d %lx %ld %p %p %p\n", lpProviderId, lpServiceClassInfo, lpqsRegInfo, essOperation, dwControlFlags);
    WSASetLastError(WSAEOPNOTSUPP);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPInstallServiceClass(_In_ LPGUID lpProviderId,
                          _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    ERR("mwsNSPInstallServiceClass not implemented %p %p\n", lpProviderId, lpServiceClassInfo);
    WSASetLastError(WSAEOPNOTSUPP);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPRemoveServiceClass(_In_ LPGUID lpProviderId,
                         _In_ LPGUID lpServiceClassId)
{
    ERR("mwsNSPRemoveServiceClass not implemented %p %p\n", lpProviderId, lpServiceClassId);
    WSASetLastError(WSAEOPNOTSUPP);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPGetServiceClassInfo(_In_ LPGUID lpProviderId,
                          _In_ LPDWORD lpdwBufSize,
                          _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    ERR("mwsNSPGetServiceClassInfo not implemented %p %p %p\n", lpProviderId, lpdwBufSize, lpServiceClassInfo);
    WSASetLastError(WSAEOPNOTSUPP);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* Implementations - Exports */
/*
 * @implemented
 */
int
WINAPI
NSPStartup(_In_ LPGUID lpProviderId,
           _Out_ LPNSP_ROUTINE lpRout)
{
    INT ret;

    TRACE("NSPStartup %p %p\n", lpProviderId, lpRout);
    if (!lpRout || (lpRout->cbSize != sizeof(NSP_ROUTINE)))
    {
        ERR("NSPStartup invalid parameter\n");
        WSASetLastError(WSAEINVAL);
        return ERROR_INVALID_PARAMETER;
    }

    mwsNSPInit();

    /* set own Provider GUID - maybe we need
       here to set the original mswsock-GUID?! */

    /* Win2k3 returns
       - Version 1.1
       - no NSPIoctl
       - sets cbSize to 44! */
    lpRout->dwMajorVersion = 1;
    lpRout->dwMinorVersion = 1;
    lpRout->cbSize = sizeof(*lpRout) - sizeof(lpRout->NSPIoctl);
    lpRout->NSPCleanup = &mwsNSPCleanUp;
    lpRout->NSPLookupServiceBegin = &mwsNSPLookupServiceBegin;
    lpRout->NSPLookupServiceNext = &mwsNSPLookupServiceNext;
    lpRout->NSPLookupServiceEnd = &mwsNSPLookupServiceEnd;
    lpRout->NSPSetService = &mwsNSPSetService;
    lpRout->NSPInstallServiceClass = &mwsNSPInstallServiceClass;
    lpRout->NSPRemoveServiceClass = &mwsNSPRemoveServiceClass;
    lpRout->NSPGetServiceClassInfo = &mwsNSPGetServiceClassInfo;
    lpRout->NSPIoctl = NULL;// &mwsNSPIoCtl;

    ret = NO_ERROR;

    return ret;
}
