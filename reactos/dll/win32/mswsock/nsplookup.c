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

#include "mswhelper.h"

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
  _In_ DWORD CallID,
  _Inout_ LPWSAQUERYSETW lpRes,
  _Inout_ LPDWORD lpResLen);

INT
NSP_GetHostNameHeapAllocW(
  _Out_ WCHAR** hostname);

INT
NSP_GetHostByNameHeapAllocW(
  _In_ WCHAR* name,
  _In_ GUID* lpProviderId,
  _Out_ PWSHOSTINFOINTERN hostinfo);

INT
NSP_GetServiceByNameHeapAllocW(
  _In_ WCHAR* nameW,
  _In_ GUID* lpProviderId,
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
                                    pLook->CallID,
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

/* This function is far from perfect but it works enough */
IP4_ADDRESS
FindEntryInHosts(IN CONST WCHAR FAR* wname)
{
    BOOL Found = FALSE;
    HANDLE HostsFile;
    CHAR HostsDBData[BUFSIZ] = {0};
    PCHAR SystemDirectory = HostsDBData;
    PCHAR HostsLocation = "\\drivers\\etc\\hosts";
    PCHAR AddressStr, DnsName = NULL, AddrTerm, NameSt, NextLine, ThisLine, Comment;
    UINT SystemDirSize = sizeof(HostsDBData) - 1, ValidData = 0;
    DWORD ReadSize;
    DWORD Address;
    CHAR name[MAX_HOSTNAME_LEN + 1];

    wcstombs(name, wname, MAX_HOSTNAME_LEN);

    /* We assume that the parameters are valid */
    if (!GetSystemDirectoryA(SystemDirectory, SystemDirSize))
    {
        WSASetLastError(WSANO_RECOVERY);
        //WS_DbgPrint(MIN_TRACE, ("Could not get windows system directory.\n"));
        return 0; /* Can't get system directory */
    }

    strncat(SystemDirectory, HostsLocation, SystemDirSize);

    HostsFile = CreateFileA(SystemDirectory,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
    if (HostsFile == INVALID_HANDLE_VALUE)
    {
        WSASetLastError(WSANO_RECOVERY);
        return 0;
    }

    while (!Found && ReadFile(HostsFile,
                              HostsDBData + ValidData,
                              sizeof(HostsDBData) - ValidData,
                              &ReadSize,
                              NULL))
    {
        ValidData += ReadSize;
        ReadSize = 0;
        NextLine = ThisLine = HostsDBData;

        /* Find the beginning of the next line */
        while ((NextLine < HostsDBData + ValidData) &&
               (*NextLine != '\r') &&
               (*NextLine != '\n'))
        {
            NextLine++;
        }

        /* Zero and skip, so we can treat what we have as a string */
        if (NextLine > HostsDBData + ValidData)
            break;

        *NextLine = 0;
        NextLine++;

        Comment = strchr(ThisLine, '#');
        if (Comment)
            *Comment = 0; /* Terminate at comment start */

        AddressStr = ThisLine;
        /* Find the first space separating the IP address from the DNS name */
        AddrTerm = strchr(ThisLine, ' ');
        if (AddrTerm)
        {
            /* Terminate the address string */
            *AddrTerm = 0;

            /* Find the last space before the DNS name */
            NameSt = strrchr(ThisLine, ' ');

            /* If there is only one space (the one we removed above), then just use the address terminator */
            if (!NameSt)
                NameSt = AddrTerm;

            /* Move from the space to the first character of the DNS name */
            NameSt++;

            DnsName = NameSt;

            if (!strcmp(name, DnsName))
            {
                Found = TRUE;
                break;
            }
        }

        /* Get rid of everything we read so far */
        while (NextLine <= HostsDBData + ValidData &&
               isspace (*NextLine))
        {
            NextLine++;
        }

        if (HostsDBData + ValidData - NextLine <= 0)
            break;

        //WS_DbgPrint(MAX_TRACE,("About to move %d chars\n",
        //            HostsDBData + ValidData - NextLine));

        memmove(HostsDBData, NextLine, HostsDBData + ValidData - NextLine);
        ValidData -= NextLine - HostsDBData;
        //WS_DbgPrint(MAX_TRACE,("Valid bytes: %d\n", ValidData));
    }

    CloseHandle(HostsFile);

    if (!Found)
    {
        //WS_DbgPrint(MAX_TRACE,("Not found\n"));
        WSASetLastError(WSANO_DATA);
        return 0;
    }

    if (strstr(AddressStr, ":"))
    {
       //DbgPrint("AF_INET6 NOT SUPPORTED!\n");
       WSASetLastError(WSAEINVAL);
       return 0;
    }

    Address = inet_addr(AddressStr);
    if (Address == INADDR_NONE)
    {
        WSASetLastError(WSAEINVAL);
        return 0;
    }

    return Address;
}

INT
NSP_GetHostByNameHeapAllocW(_In_ WCHAR* name,
                            _In_ GUID* lpProviderId,
                            _Out_ PWSHOSTINFOINTERN hostinfo)
{
    HANDLE hHeap = GetProcessHeap();
    enum addr_type
    {
        GH_INVALID,
        GH_IPV6,
        GH_IPV4,
        GH_RFC1123_DNS
    };
    typedef enum addr_type addr_type;
    addr_type addr;
    INT ret = 0;
    WCHAR* found = 0;
    DNS_STATUS dns_status = {0};
    /* include/WinDNS.h -- look up DNS_RECORD on MSDN */
    PDNS_RECORD dp;
    PDNS_RECORD curr;
    WCHAR* tmpHostnameW;
    CHAR* tmpHostnameA;
    IP4_ADDRESS address;
    INT result = ERROR_SUCCESS;

    /* needed to be cleaned up if != NULL */
    tmpHostnameW = NULL;
    dp = NULL;

    addr = GH_INVALID;

    if (name == NULL)
    {
        result = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    /* Hostname "" / "localhost"
       - convert to "computername" */
    if ((wcscmp(L"", name) == 0) /*||
        (wcsicmp(L"localhost", name) == 0)*/)
    {
        ret = NSP_GetHostNameHeapAllocW(&tmpHostnameW);
        if (ret != ERROR_SUCCESS)
        {
            result = ret;
            goto cleanup;
        }
        name = tmpHostnameW;
    }

    /* Is it an IPv6 address? */
    found = wcschr(name, L':');
    if (found != NULL)
    {
        addr = GH_IPV6;
        goto act;
    }

    /* Is it an IPv4 address? */
    if (!iswalpha(name[0]))
    {
        addr = GH_IPV4;
        goto act;
    }

    addr = GH_RFC1123_DNS;

/* Broken out in case we want to get fancy later */
act:
    switch (addr)
    {
        case GH_IPV6:
            WSASetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            result = ERROR_CALL_NOT_IMPLEMENTED;
            goto cleanup;
        break;

        case GH_INVALID:
            WSASetLastError(WSAEFAULT);
            result = ERROR_INVALID_PARAMETER;
            goto cleanup;
        break;

        /* Note: If passed an IP address, MSDN says that gethostbyname()
                 treats it as an unknown host.
           This is different from the unix implementation. Use inet_addr()
        */
        case GH_IPV4:
        case GH_RFC1123_DNS:
        /* DNS_TYPE_A: include/WinDNS.h */
        /* DnsQuery -- lib/dnsapi/dnsapi/query.c */

        /* Look for the DNS name in the hosts file */
        if ((address = FindEntryInHosts(name)) != 0)
        {
            hostinfo->hostnameW = StrCpyHeapAllocW(hHeap, name);
            hostinfo->addr4 = address;
            result = ERROR_SUCCESS;
            goto cleanup;
        }

        tmpHostnameA = StrW2AHeapAlloc(hHeap, name);
        dns_status = DnsQuery(tmpHostnameA,
                              DNS_TYPE_A,
                              DNS_QUERY_STANDARD,
                              /* extra dns servers */ 0,
                              &dp,
                              0);
        HeapFree(hHeap, 0, tmpHostnameA);

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
            curr = curr->pNext;
        }

        if (curr->wType != DNS_TYPE_A)
        {
            result = WSASERVICE_NOT_FOUND;
            goto cleanup;
        }

        //WS_DbgPrint(MID_TRACE,("populating hostent\n"));
        //WS_DbgPrint(MID_TRACE,("pName is (%s)\n", curr->pName));
        //populate_hostent(p->Hostent,
        //                 (PCHAR)curr->pName,
        //                 curr->Data.A.IpAddress);
        hostinfo->hostnameW = StrA2WHeapAlloc(hHeap, curr->pName);
        hostinfo->addr4 = curr->Data.A.IpAddress;
        result = ERROR_SUCCESS;
        goto cleanup;

        //WS_DbgPrint(MID_TRACE,("Called DnsQuery, but host not found. Err: %i\n",
        //            dns_status));
        //WSASetLastError(WSAHOST_NOT_FOUND);
        //return NULL;

        break;

        default:
            result = WSANO_RECOVERY;
            goto cleanup;
        break;
    }

    result = WSANO_RECOVERY;

cleanup:
    if (dp != NULL)
        DnsRecordListFree(dp, DnsFreeRecordList);

    if (tmpHostnameW != NULL)
        HeapFree(hHeap, 0, tmpHostnameW);

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

INT
NSP_GetServiceByNameHeapAllocW(_In_ WCHAR* nameW,
                               _In_ GUID* lpProviderId,
                               _Out_ PWSHOSTINFOINTERN hostinfo)
{
    BOOL Found = FALSE;
    HANDLE ServicesFile;
    CHAR ServiceDBData[BUFSIZ * sizeof(WCHAR)] = {0};
    PWCHAR SystemDirectory = (PWCHAR)ServiceDBData; /* Reuse this stack space */
    PWCHAR ServicesFileLocation = L"\\drivers\\etc\\services";
    PCHAR ThisLine = 0, NextLine = 0, ServiceName = 0, PortNumberStr = 0,
    ProtocolStr = 0, Comment = 0, EndValid;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS] = {0};
    PCHAR* AliasPtr;
    UINT i = 0,
    SystemDirSize = (sizeof(ServiceDBData) / sizeof(WCHAR)) - 1;
    DWORD ReadSize = 0;
    HANDLE hHeap;
    PCHAR nameA = NULL;
    PCHAR nameServiceA = NULL;
    PCHAR nameProtoA = NULL;
    INT res = WSANO_RECOVERY;
    
    if (!nameW)
    {
        res = WSANO_RECOVERY;
        goto End;
    }

    hHeap = GetProcessHeap();
    nameA = StrW2AHeapAlloc(hHeap, nameW);

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

    if (!GetSystemDirectoryW(SystemDirectory, SystemDirSize))
    {
        /* Can't get system directory */
        res = WSANO_RECOVERY;
        goto End;
    }

    wcsncat(SystemDirectory, ServicesFileLocation, SystemDirSize);

    ServicesFile = CreateFileW(SystemDirectory,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);

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
                       _In_ DWORD CallID,
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

    if ((CallID == NSP_CALLID_HOSTNAME) ||
        (CallID == NSP_CALLID_HOSTBYNAME) ||
        (CallID == NSP_CALLID_SERVICEBYNAME))
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

    if (CallID == NSP_CALLID_HOSTNAME)
    {
        result = NSP_GetHostNameHeapAllocW(&hostinfo.hostnameW);

        if (result != ERROR_SUCCESS)
            goto End;

        hostinfo.addr4 = 0;
    }
    else if (CallID == NSP_CALLID_HOSTBYNAME)
    {
        result = NSP_GetHostByNameHeapAllocW(data->hostnameW,
                                             &data->providerId,
                                             &hostinfo);
        if (result != ERROR_SUCCESS)
            goto End;
    }
    else if (CallID == NSP_CALLID_SERVICEBYNAME)
    {
        result = NSP_GetServiceByNameHeapAllocW(data->hostnameW,
                                                &data->providerId,
                                                &hostinfo);
        if (result != ERROR_SUCCESS)
            goto End;
    }
    else
    {
        result = WSANO_RECOVERY; // Internal error!
        goto End;
    }

    if (((LUP_RETURN_BLOB & data->dwControlFlags) != 0) ||
        ((LUP_RETURN_NAME & data->dwControlFlags) != 0))
    {
        if (CallID == NSP_CALLID_HOSTNAME || CallID == NSP_CALLID_HOSTBYNAME)
        {
            ServiceInstanceNameW = hostinfo.hostnameW;
            ServiceInstanceNameA = StrW2AHeapAlloc(hHeap, ServiceInstanceNameW);
            if (ServiceInstanceNameA == NULL)
            {
                result = WSAEFAULT;
                goto End;

            }
        }
        if (CallID == NSP_CALLID_SERVICEBYNAME)
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
        if (CallID == NSP_CALLID_HOSTBYNAME)
        {
            /* Write data for PBLOB (hostent) */
            if (!mswBufferAppendBlob_Hostent(&buf,
                                             lpRes,
                                             ServiceInstanceNameA,
                                             hostinfo.addr4))
            {
                *lpResLen = buf.bytesUsed;
                result = WSAEFAULT;
                goto End;
            }
        }
        else if (CallID == NSP_CALLID_SERVICEBYNAME)
        {
            /* Write data for PBLOB (servent) */
            if (!mswBufferAppendBlob_Servent(&buf,
                                             lpRes,
                                             ServiceInstanceNameA,/* ServiceName */
                                             hostinfo.servaliasesA,
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
