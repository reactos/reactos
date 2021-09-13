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

#define NDEBUG
#include <debug.h>

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

//#define IP_LOCALHOST 0x0100007F

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

/* Forwards */
INT
WINAPI
mswNSPStartup(
  LPGUID lpProviderId,
  LPNSP_ROUTINE lpRout);

INT
NSP_LookupServiceBeginW(
  PWSHANDLEINTERN data,
  CHAR* hostnameA,
  WCHAR* hostnameW,
  DWORD CallID);

INT
NSP_LookupServiceNextW(
  _In_ PWSHANDLEINTERN data,
  _In_ DWORD dwControlFlags,
  _Inout_ LPWSAQUERYSETW lpRes,
  _Inout_ LPDWORD lpResLen);

INT
NSP_GetHostNameHeapAllocW(
  _Out_ WCHAR** hostname);

INT
NSP_GetHostByNameHeapAllocW(
  _In_ PWSHANDLEINTERN data,
  _In_ DWORD dwControlFlags,
  _Out_ PWSHOSTINFOINTERN hostinfo);

INT
NSP_GetServiceByNameHeapAllocW(
  _In_ PWSHANDLEINTERN data,
  _In_ DWORD dwControlFlags,
  _Out_ PWSHOSTINFOINTERN hostinfo);

/* Implementations - Internal */

INT
WSAAPI
mwsNSPCleanUp(_In_ LPGUID lpProviderId)
{
    //WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    //return ERROR_CALL_NOT_IMPLEMENTED;
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

    if (IsEqualGUID(lpProviderId, &guid_mswsock_TcpIp))
    {
        //OK
    }
    else if (IsEqualGUID(lpProviderId, &guid_mswsock_NLA))
    {
        WSASetLastError(WSASERVICE_NOT_FOUND);
        return SOCKET_ERROR;
    }
    else
    {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    /* allocate internal structure */
    pLook = HeapAlloc(GetProcessHeap(), 0, sizeof(WSHANDLEINTERN));
    if (!pLook)
    {
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
        wsaErr = ERROR_CALL_NOT_IMPLEMENTED;
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId, &guid_HOSTNAME))
    {
        wsaErr = NSP_LookupServiceBeginW(pLook,
                                         NULL,
                                         NULL,
                                         NSP_CALLID_HOSTNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_HOSTADDRBYNAME))
    {
       wsaErr = NSP_LookupServiceBeginW(pLook,
                                        NULL,
                                        lpqsRestrictions->lpszServiceInstanceName,
                                        NSP_CALLID_HOSTBYNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_SERVICEBYNAME))
    {
       wsaErr = NSP_LookupServiceBeginW(pLook,
                                        NULL,
                                        lpqsRestrictions->lpszServiceInstanceName,
                                        NSP_CALLID_SERVICEBYNAME);
    }
    else if (IsEqualGUID(lpqsRestrictions->lpServiceClassId,
                         &guid_INET_HOSTADDRBYINETSTRING))
    {
        wsaErr = ERROR_CALL_NOT_IMPLEMENTED;
    }

#endif /* NSP_REDIRECT */

    if (wsaErr != NO_ERROR)
    {
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
    WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPLookupServiceEnd(_In_ HANDLE hLookup)
{
    PWSHANDLEINTERN pLook;
    HANDLE hHeap;
    INT res;

    res = NO_ERROR;
    pLook = (PWSHANDLEINTERN)hLookup;
    hHeap = GetProcessHeap();

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
    WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPInstallServiceClass(_In_ LPGUID lpProviderId,
                          _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPRemoveServiceClass(_In_ LPGUID lpProviderId,
                         _In_ LPGUID lpServiceClassId)
{
    WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INT
WSAAPI
mwsNSPGetServiceClassInfo(_In_ LPGUID lpProviderId,
                          _In_ LPDWORD lpdwBufSize,
                          _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

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

    WSASetLastError(0);

    return ERROR_SUCCESS;
}

INT
NSP_GetHostNameHeapAllocW(_Out_ WCHAR** hostname)
{
    WCHAR* name;
    HANDLE hHeap = GetProcessHeap();
    DWORD bufCharLen = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD bufByteLen = bufCharLen * sizeof(WCHAR);

    name = HeapAlloc(hHeap, 0, bufByteLen);

    if (!GetComputerNameExW(ComputerNameDnsHostname,
                            name,
                            &bufCharLen))
    {
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

    /* needed to be cleaned up if != NULL */
    dp = NULL;

    if (data->hostnameW == NULL)
    {
        result = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((data->dwControlFlags & LUP_DEEP) == 0)
    {
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
        WSASetLastError(WSAEFAULT);
        result = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((dns_status != 0) || (dp == NULL))
    {
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
            Aliases[AliasIndex++] = curr->Data.Cname.pNameHost;
        }
        curr = curr->pNext;
    }

    if (curr->wType != DNS_TYPE_A)
    {
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
    HANDLE ret;

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
        /* Read the actual path */
        ErrorCode = RegQueryValueEx(DatabaseKey,
                                    L"DatabasePath",
                                    NULL,
                                    &RegType,
                                    NULL,
                                    &RegSize);

        DatabasePath = HeapAlloc(GetProcessHeap(), 0, RegSize);
        if (!DatabasePath)
        {
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

        /* Expand the name */
        ExpandEnvironmentStrings(DatabasePath, ExpandedPath, MAX_PATH);

        HeapFree(GetProcessHeap(), 0, DatabasePath);
    }
    else
    {
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
    ret = CreateFile(ExpandedPath,
                      FILE_READ_DATA,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    HeapFree(GetProcessHeap(), 0, ExpandedPath);
    return ret;
}

INT
NSP_GetServiceByNameHeapAllocW(_In_ PWSHANDLEINTERN data,
                               _In_ DWORD dwControlFlags,
                               _Out_ PWSHOSTINFOINTERN hostinfo)
{
    BOOL Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ * sizeof(WCHAR)] = {0};
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0,
    ProtocolStr = 0, Comment = 0, EndValid;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = {0};
    PCHAR* AliasPtr;
    UINT i = 0;
    DWORD ReadSize = 0;
    HANDLE hHeap;
    PCHAR nameA = NULL;
    PCHAR nameServiceA = NULL;
    PCHAR nameProtoA = NULL;
    INT res = WSANO_RECOVERY;

    if (!data->hostnameW)
    {
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
        return WSANO_RECOVERY;
    }

    /* Scan the services file ...
    *
    * We will be share the buffer on the lines. If the line does not fit in
    * the buffer, then moving it to the beginning of the buffer and read
    * the remnants of line from file.
    */

    /* Initial Read */
    ReadFile(ServicesFile,
             ServiceDBData,
             sizeof( ServiceDBData ) - 1,
             &ReadSize,
             NULL);

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
                    //WS_DbgPrint(MIN_TRACE,("Line too long"));
                    return WSANO_RECOVERY;
                }

                memmove(ServiceDBData, ThisLine, LineLen);

                ReadFile(ServicesFile,
                         ServiceDBData + LineLen,
                         sizeof( ServiceDBData )-1 - LineLen,
                         &ReadSize,
                         NULL);

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
        return WSANO_DATA;
    }

    hostinfo->addr4 = 0;
    hostinfo->servnameW = StrA2WHeapAlloc(hHeap, ServiceName);
    hostinfo->servprotoW = StrA2WHeapAlloc(hHeap, ProtocolStr);
    hostinfo->servaliasesA = StrAryCpyHeapAllocA(hHeap, (char**)&Aliases);
    hostinfo->servport = atoi(PortNumberStr);

    res = NO_ERROR;

End:
    if (nameA != NULL)
        HeapFree(hHeap, 0, nameA);

    if (nameServiceA != NULL)
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
        if (data->CallIDCounter >= 1)
        {
            result = WSAENOMORE;
            goto End;
        }
    }
    else
    {
        result = WSANO_RECOVERY;
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
        ASSERT(data->CallID == NSP_CALLID_SERVICEBYNAME);
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
            if (ServiceInstanceNameA == NULL)
            {
                result = WSAEFAULT;
                goto End;

            }
        }
        if (data->CallID == NSP_CALLID_SERVICEBYNAME)
        {
            ServiceInstanceNameW = hostinfo.servnameW;
            ServiceInstanceNameA = StrW2AHeapAlloc(hHeap, ServiceInstanceNameW);
            if (ServiceInstanceNameA == NULL)
            {
                result = WSAEFAULT;
                goto End;

            }
            ServiceProtocolNameA = StrW2AHeapAlloc(hHeap, hostinfo.servprotoW);
            if (ServiceProtocolNameA == NULL)
            {
                result = WSAEFAULT;
                goto End;

            }
        }
    }

    if ((LUP_RETURN_ADDR & data->dwControlFlags) != 0)
    {
        if (!mswBufferAppendAddr_AddrInfoW(&buf, lpRes, hostinfo.addr4))
        {
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
                *lpResLen = buf.bytesUsed;
                result = WSAEFAULT;
                goto End;
            }
        }
        else
        {
            result = WSANO_RECOVERY;
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

    return result;
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

    if ((lpRout == NULL) ||
        (lpRout->cbSize != sizeof(NSP_ROUTINE)))
    {
        WSASetLastError(ERROR_INVALID_PARAMETER);
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
