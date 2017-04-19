/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/query.c
 * PURPOSE:     DNSAPI functions built on the ADNS library.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include "precomp.h"
#include <winreg.h>
#include <iphlpapi.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

/* DnsQuery ****************************
 * Begin a DNS query, and allow the result to be placed in the application
 * supplied result pointer.  The result can be manipulated with the record
 * functions.
 *
 * Name                 -- The DNS object to be queried.
 * Type                 -- The type of records to be returned.  These are
 *                          listed in windns.h
 * Options              -- Query options.  DNS_QUERY_STANDARD is the base
 *                          state, and every other option takes precedence.
 *                          multiple options can be combined.  Listed in
 *                          windns.h
 * Servers              -- List of alternate servers (optional)
 * QueryResultSet       -- Pointer to the result pointer that will be filled
 *                          when the response is available.
 * Reserved             -- Response as it appears on the wire.  Optional.
 */

static PCHAR
DnsWToC(const WCHAR *WideString)
{
    PCHAR AnsiString;
    int AnsiLen = WideCharToMultiByte(CP_ACP,
                                      0,
                                      WideString,
                                      -1,
                                      NULL,
                                      0,
                                      NULL,
                                      0);
    if (AnsiLen == 0)
        return NULL;
    AnsiString = RtlAllocateHeap(RtlGetProcessHeap(), 0, AnsiLen);
    if (AnsiString == NULL)
    {
        return NULL;
    }
    WideCharToMultiByte(CP_ACP,
                        0,
                        WideString,
                        -1,
                        AnsiString,
                        AnsiLen,
                        NULL,
                        0);

    return AnsiString;
}

static PWCHAR
DnsCToW(const CHAR *NarrowString)
{
    PWCHAR WideString;
    int WideLen = MultiByteToWideChar(CP_ACP,
                                      0,
                                      NarrowString,
                                      -1,
                                      NULL,
                                      0);
    if (WideLen == 0)
        return NULL;
    WideLen *= sizeof(WCHAR);
    WideString = RtlAllocateHeap(RtlGetProcessHeap(), 0, WideLen);
    if (WideString == NULL)
    {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP,
                        0,
                        NarrowString,
                        -1,
                        WideString,
                        WideLen);

    return WideString;
}

DNS_STATUS WINAPI
DnsQuery_A(LPCSTR Name,
           WORD Type,
           DWORD Options,
           PIP4_ARRAY Servers,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    UINT i;
    PWCHAR Buffer;
    DNS_STATUS Status;
    PDNS_RECORD QueryResultWide;
    PDNS_RECORD ConvertedRecord = 0, LastRecord = 0;

    if (Name == NULL)
        return ERROR_INVALID_PARAMETER;
    if (QueryResultSet == NULL)
        return ERROR_INVALID_PARAMETER;

    Buffer = DnsCToW(Name);

    Status = DnsQuery_W(Buffer, Type, Options, Servers, &QueryResultWide, Reserved);

    while (Status == ERROR_SUCCESS && QueryResultWide)
    {
        switch (QueryResultWide->wType)
        {
        case DNS_TYPE_A:
        case DNS_TYPE_WKS:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = QueryResultWide->wDataLength;
            memcpy(&ConvertedRecord->Data, &QueryResultWide->Data, QueryResultWide->wDataLength);
            break;

        case DNS_TYPE_CNAME:
        case DNS_TYPE_PTR:
        case DNS_TYPE_NS:
        case DNS_TYPE_MB:
        case DNS_TYPE_MD:
        case DNS_TYPE_MF:
        case DNS_TYPE_MG:
        case DNS_TYPE_MR:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = sizeof(DNS_PTR_DATA);
            ConvertedRecord->Data.PTR.pNameHost = DnsWToC((PWCHAR)QueryResultWide->Data.PTR.pNameHost);
            break;

        case DNS_TYPE_MINFO:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = sizeof(DNS_MINFO_DATA);
            ConvertedRecord->Data.MINFO.pNameMailbox = DnsWToC((PWCHAR)QueryResultWide->Data.MINFO.pNameMailbox);
            ConvertedRecord->Data.MINFO.pNameErrorsMailbox = DnsWToC((PWCHAR)QueryResultWide->Data.MINFO.pNameErrorsMailbox);
            break;

        case DNS_TYPE_MX:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = sizeof(DNS_MX_DATA);
            ConvertedRecord->Data.MX.pNameExchange = DnsWToC((PWCHAR)QueryResultWide->Data.MX.pNameExchange);
            ConvertedRecord->Data.MX.wPreference = QueryResultWide->Data.MX.wPreference;
            break;

        case DNS_TYPE_HINFO:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_TXT_DATA) + QueryResultWide->Data.TXT.dwStringCount);
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = sizeof(DNS_TXT_DATA) + (sizeof(PCHAR) * QueryResultWide->Data.TXT.dwStringCount);
            ConvertedRecord->Data.TXT.dwStringCount = QueryResultWide->Data.TXT.dwStringCount;

            for (i = 0; i < ConvertedRecord->Data.TXT.dwStringCount; i++)
                ConvertedRecord->Data.TXT.pStringArray[i] = DnsWToC((PWCHAR)QueryResultWide->Data.TXT.pStringArray[i]);

            break;

        case DNS_TYPE_NULL:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount);
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->wType = QueryResultWide->wType;
            ConvertedRecord->wDataLength = sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount;
            ConvertedRecord->Data.Null.dwByteCount = QueryResultWide->Data.Null.dwByteCount;
            memcpy(&ConvertedRecord->Data.Null.Data, &QueryResultWide->Data.Null.Data, QueryResultWide->Data.Null.dwByteCount);
            break;
        }

        if (LastRecord)
        {
            LastRecord->pNext = ConvertedRecord;
            LastRecord = LastRecord->pNext;
        }
        else
        {
            LastRecord = *QueryResultSet = ConvertedRecord;
        }

        QueryResultWide = QueryResultWide->pNext;
    }

    if (LastRecord)
        LastRecord->pNext = 0;

    /* The name */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    /* The result*/
    if (QueryResultWide) DnsIntFreeRecordList(QueryResultWide);

    return Status;
}

WCHAR
*xstrsave(const WCHAR *str)
{
    WCHAR *p;
    size_t len = 0;

    /* FIXME: how much instead of MAX_PATH? */
    StringCbLengthW(str, MAX_PATH, &len);
    len+=sizeof(WCHAR);

    p = RtlAllocateHeap(RtlGetProcessHeap(), 0, len);

    if (p)
        StringCbCopyW(p, len, str);

    return p;
}

CHAR
*xstrsaveA(const CHAR *str)
{
    CHAR *p;
    size_t len = 0;

    /* FIXME: how much instead of MAX_PATH? */
    StringCbLengthA(str, MAX_PATH, &len);
    len++;

    p = RtlAllocateHeap(RtlGetProcessHeap(), 0, len);

    if (p)
        StringCbCopyA(p, len, str);

    return p;
}

HANDLE
OpenNetworkDatabase(LPCWSTR Name)
{
    PWSTR ExpandedPath;
    PWSTR DatabasePath;
    INT ErrorCode;
    HKEY DatabaseKey;
    DWORD RegType;
    DWORD RegSize = 0;
    size_t StringLength;
    HANDLE ret;

    ExpandedPath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!ExpandedPath)
        return INVALID_HANDLE_VALUE;

    /* Open the database path key */
    ErrorCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                              0,
                              KEY_READ,
                              &DatabaseKey);
    if (ErrorCode == NO_ERROR)
    {
        /* Read the actual path */
        ErrorCode = RegQueryValueExW(DatabaseKey,
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
        ErrorCode = RegQueryValueExW(DatabaseKey,
                                     L"DatabasePath",
                                     NULL,
                                     &RegType,
                                     (LPBYTE)DatabasePath,
                                     &RegSize);

        /* Close the key */
        RegCloseKey(DatabaseKey);

        /* Expand the name */
        ExpandEnvironmentStringsW(DatabasePath, ExpandedPath, MAX_PATH);

        HeapFree(GetProcessHeap(), 0, DatabasePath);
    }
    else
    {
        /* Use defalt path */
        GetSystemDirectoryW(ExpandedPath, MAX_PATH);
        StringCchLengthW(ExpandedPath, MAX_PATH, &StringLength);
        if (ExpandedPath[StringLength - 1] != L'\\')
        {
            /* It isn't, so add it ourselves */
            StringCchCatW(ExpandedPath, MAX_PATH, L"\\");
        }
        StringCchCatW(ExpandedPath, MAX_PATH, L"DRIVERS\\ETC\\");
    }

    /* Make sure that the path is backslash-terminated */
    StringCchLengthW(ExpandedPath, MAX_PATH, &StringLength);
    if (ExpandedPath[StringLength - 1] != L'\\')
    {
        /* It isn't, so add it ourselves */
        StringCchCatW(ExpandedPath, MAX_PATH, L"\\");
    }

    /* Add the database name */
    StringCchCatW(ExpandedPath, MAX_PATH, Name);

    /* Return a handle to the file */
    ret = CreateFileW(ExpandedPath,
                      FILE_READ_DATA,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    HeapFree(GetProcessHeap(), 0, ExpandedPath);
    return ret;
}

/* This function is far from perfect but it works enough */
IP4_ADDRESS
CheckForCurrentHostname(CONST CHAR * Name, PFIXED_INFO network_info)
{
    PCHAR TempName;
    DWORD AdapterAddressesSize, Status;
    IP4_ADDRESS ret = 0, Address;
    PIP_ADAPTER_ADDRESSES Addresses = NULL, pip;
    BOOL Found = FALSE;

    if (network_info->DomainName[0])
    {
        size_t StringLength;
        size_t TempSize = 2;
        StringCchLengthA(network_info->HostName, sizeof(network_info->HostName), &StringLength);
        TempSize += StringLength;
        StringCchLengthA(network_info->DomainName, sizeof(network_info->DomainName), &StringLength);
        TempSize += StringLength;
        TempName = RtlAllocateHeap(RtlGetProcessHeap(), 0, TempSize);
        StringCchCopyA(TempName, TempSize, network_info->HostName);
        StringCchCatA(TempName, TempSize, ".");
        StringCchCatA(TempName, TempSize, network_info->DomainName);
    }
    else
    {
        TempName = RtlAllocateHeap(RtlGetProcessHeap(), 0, 1);
        TempName[0] = 0;
    }
    Found = !stricmp(Name, network_info->HostName) || !stricmp(Name, TempName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, TempName);
    if (!Found)
    {
        return 0;
    }
    /* get adapter info */
    AdapterAddressesSize = 0;
    GetAdaptersAddresses(AF_INET,
                         GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER |
                         GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL,
                         Addresses,
                         &AdapterAddressesSize);
    if (!AdapterAddressesSize)
    {
        return 0;
    }
    Addresses = RtlAllocateHeap(RtlGetProcessHeap(), 0, AdapterAddressesSize);
    Status = GetAdaptersAddresses(AF_INET,
                                  GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER |
                                  GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                  NULL,
                                  Addresses,
                                  &AdapterAddressesSize);
    if (Status)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Addresses);
        return 0;
    }
    for (pip = Addresses; pip != NULL; pip = pip->Next) {
        Address = ((LPSOCKADDR_IN)pip->FirstUnicastAddress->Address.lpSockaddr)->sin_addr.S_un.S_addr;
        if (Address != ntohl(INADDR_LOOPBACK))
            break;
    }
    if (Address && Address != ntohl(INADDR_LOOPBACK))
    {
        ret = Address;
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, Addresses);
    return ret;
}

BOOL
ParseV4Address(LPCSTR AddressString,
    OUT PDWORD pAddress)
{
    CHAR * cp = (CHAR *)AddressString;
    DWORD val, base;
    unsigned char c;
    DWORD parts[4], *pp = parts;
    if (!AddressString)
        return FALSE;
    if (!isdigit(*cp)) return FALSE;

again:
    /*
    * Collect number up to ``.''.
    * Values are specified as for C:
    * 0x=hex, 0=octal, other=decimal.
    */
    val = 0; base = 10;
    if (*cp == '0') {
        if (*++cp == 'x' || *cp == 'X')
            base = 16, cp++;
        else
            base = 8;
    }
    while ((c = *cp)) {
        if (isdigit(c)) {
            val = (val * base) + (c - '0');
            cp++;
            continue;
        }
        if (base == 16 && isxdigit(c)) {
            val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
            cp++;
            continue;
        }
        break;
    }
    if (*cp == '.') {
        /*
        * Internet format:
        *    a.b.c.d
        */
        if (pp >= parts + 4) return FALSE;
        *pp++ = val;
        cp++;
        goto again;
    }
    /*
    * Check for trailing characters.
    */
    if (*cp && *cp > ' ') return FALSE;

    if (pp >= parts + 4) return FALSE;
    *pp++ = val;
    /*
    * Concoct the address according to
    * the number of parts specified.
    */
    if ((DWORD)(pp - parts) != 4) return FALSE;
    if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff || parts[3] > 0xff) return FALSE;
    val = (parts[3] << 24) | (parts[2] << 16) | (parts[1] << 8) | parts[0];

    if (pAddress)
        *pAddress = val;

    return TRUE;
}

/* This function is far from perfect but it works enough */
IP4_ADDRESS
FindEntryInHosts(CONST CHAR * name)
{
    BOOL Found = FALSE;
    HANDLE HostsFile;
    CHAR HostsDBData[BUFSIZ] = { 0 };
    PCHAR AddressStr, DnsName = NULL, AddrTerm, NameSt, NextLine, ThisLine, Comment;
    UINT ValidData = 0;
    DWORD ReadSize;
    DWORD Address;

    /* Open the network database */
    HostsFile = OpenNetworkDatabase(L"hosts");
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

            if (!stricmp(name, DnsName) || !stricmp(name, AddressStr))
            {
                Found = TRUE;
                break;
            }
        }

        /* Get rid of everything we read so far */
        while (NextLine <= HostsDBData + ValidData &&
            isspace(*NextLine))
        {
            NextLine++;
        }

        if (HostsDBData + ValidData - NextLine <= 0)
            break;

        memmove(HostsDBData, NextLine, HostsDBData + ValidData - NextLine);
        ValidData -= NextLine - HostsDBData;
    }

    CloseHandle(HostsFile);

    if (!Found)
    {
        WSASetLastError(WSANO_DATA);
        return 0;
    }

    if (strstr(AddressStr, ":"))
    {
        WSASetLastError(WSAEINVAL);
        return 0;
    }

    if (!ParseV4Address(AddressStr, &Address))
    {
        WSASetLastError(WSAEINVAL);
        return 0;
    }

    return Address;
}

DNS_STATUS WINAPI
DnsQuery_W(LPCWSTR Name,
           WORD Type,
           DWORD Options,
           PIP4_ARRAY Servers,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    adns_state astate;
    int quflags = (Options & DNS_QUERY_NO_RECURSION) == 0 ? adns_qf_search : 0;
    int adns_error;
    adns_answer *answer;
    LPSTR CurrentName;
    unsigned i, CNameLoop;
    PFIXED_INFO network_info;
    ULONG network_info_blen = 0;
    DWORD network_info_result;
    PIP_ADDR_STRING pip;
    IP4_ADDRESS Address;
    struct in_addr addr;
    PCHAR HostWithDomainName;
    PCHAR AnsiName;
    size_t NameLen = 0;

    if (Name == NULL)
        return ERROR_INVALID_PARAMETER;
    if (QueryResultSet == NULL)
        return ERROR_INVALID_PARAMETER;
    if ((Options & DNS_QUERY_WIRE_ONLY) != 0 && (Options & DNS_QUERY_NO_WIRE_QUERY) != 0)
        return ERROR_INVALID_PARAMETER;

    *QueryResultSet = 0;

    switch (Type)
    {
    case DNS_TYPE_A:
        /* FIXME: how much instead of MAX_PATH? */
        NameLen = WideCharToMultiByte(CP_ACP,
                                      0,
                                      Name,
                                      -1,
                                      NULL,
                                      0,
                                      NULL,
                                      0);
        AnsiName = RtlAllocateHeap(RtlGetProcessHeap(), 0, NameLen);
        if (NULL == AnsiName)
        {
            return ERROR_OUTOFMEMORY;
        }
        WideCharToMultiByte(CP_ACP,
                            0,
                            Name,
                            -1,
                            AnsiName,
                            NameLen,
                            NULL,
                            0);
        NameLen--;
        /* Is it an IPv4 address? */
        if (ParseV4Address(AnsiName, &Address))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));

            if (NULL == *QueryResultSet)
            {
                return ERROR_OUTOFMEMORY;
            }

            (*QueryResultSet)->pNext = NULL;
            (*QueryResultSet)->wType = Type;
            (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
            (*QueryResultSet)->Data.A.IpAddress = Address;

            (*QueryResultSet)->pName = (LPSTR)xstrsave(Name);

            return (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        }

        /* Check allowed characters
        * According to RFC a-z,A-Z,0-9,-,_, but can't start or end with - or _
        */
        if (AnsiName[0] == '-' || AnsiName[0] == '_' || AnsiName[NameLen - 1] == '-' ||
            AnsiName[NameLen - 1] == '_' || strstr(AnsiName, "..") != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            return ERROR_INVALID_NAME;
        }
        i = 0;
        while (i < NameLen)
        {
            if (!((AnsiName[i] >= 'a' && AnsiName[i] <= 'z') ||
                  (AnsiName[i] >= 'A' && AnsiName[i] <= 'Z') ||
                  (AnsiName[i] >= '0' && AnsiName[i] <= '9') ||
                  AnsiName[i] == '-' || AnsiName[i] == '_' || AnsiName[i] == '.'))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                return ERROR_INVALID_NAME;
            }
            i++;
        }

        if ((Options & DNS_QUERY_NO_HOSTS_FILE) == 0)
        {
            if ((Address = FindEntryInHosts(AnsiName)) != 0)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));

                if (NULL == *QueryResultSet)
                {
                    return ERROR_OUTOFMEMORY;
                }

                (*QueryResultSet)->pNext = NULL;
                (*QueryResultSet)->wType = Type;
                (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
                (*QueryResultSet)->Data.A.IpAddress = Address;

                (*QueryResultSet)->pName = (LPSTR)xstrsave(Name);

                return (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
            }
        }

        network_info_result = GetNetworkParams(NULL, &network_info_blen);
        network_info = (PFIXED_INFO)RtlAllocateHeap(RtlGetProcessHeap(), 0, (size_t)network_info_blen);
        if (NULL == network_info)
        {
            return ERROR_OUTOFMEMORY;
        }

        network_info_result = GetNetworkParams(network_info, &network_info_blen);
        if (network_info_result != ERROR_SUCCESS)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            return network_info_result;
        }

        if ((Address = CheckForCurrentHostname(NameLen != 0 ? AnsiName : network_info->HostName, network_info)) != 0)
        {
            size_t TempLen = 2, StringLength = 0;
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            StringCchLengthA(network_info->HostName, sizeof(network_info->HostName), &StringLength);
            TempLen += StringLength;
            StringCchLengthA(network_info->DomainName, sizeof(network_info->DomainName), &StringLength);
            TempLen += StringLength;
            HostWithDomainName = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(), 0, TempLen);
            StringCchCopyA(HostWithDomainName, TempLen, network_info->HostName);
            if (network_info->DomainName[0])
            {
                StringCchCatA(HostWithDomainName, TempLen, ".");
                StringCchCatA(HostWithDomainName, TempLen, network_info->DomainName);
            }
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));

            if (NULL == *QueryResultSet)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, HostWithDomainName);
                return ERROR_OUTOFMEMORY;
            }

            (*QueryResultSet)->pNext = NULL;
            (*QueryResultSet)->wType = Type;
            (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
            (*QueryResultSet)->Data.A.IpAddress = Address;

            (*QueryResultSet)->pName = (LPSTR)DnsCToW(HostWithDomainName);

            RtlFreeHeap(RtlGetProcessHeap(), 0, HostWithDomainName);
            return (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        }

        if ((Options & DNS_QUERY_NO_WIRE_QUERY) != 0)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            return ERROR_FILE_NOT_FOUND;
        }

        adns_error = adns_init(&astate, adns_if_noenv | adns_if_noerrprint | adns_if_noserverwarn, 0);
        if (adns_error != adns_s_ok)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            return DnsIntTranslateAdnsToDNS_STATUS(adns_error);
        }
        for (pip = &(network_info->DnsServerList); pip; pip = pip->Next)
        {
            addr.s_addr = inet_addr(pip->IpAddress.String);
            if ((addr.s_addr != INADDR_ANY) && (addr.s_addr != INADDR_NONE))
                adns_addserver(astate, addr);
        }
        if (network_info->DomainName[0])
        {
            adns_ccf_search(astate, "LOCALDOMAIN", -1, network_info->DomainName);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);

        if (Servers)
        {
            for (i = 0; i < Servers->AddrCount; i++)
            {
                adns_addserver(astate, *((struct in_addr *)&Servers->AddrArray[i]));
            }
        }

        /*
        * adns doesn't resolve chained CNAME records (a CNAME which points to
        * another CNAME pointing to another... pointing to an A record), according
        * to a mailing list thread the authors believe that chained CNAME records
        * are invalid and the DNS entries should be fixed. That's a nice academic
        * standpoint, but there certainly are chained CNAME records out there,
        * even some fairly major ones (at the time of this writing
        * download.mozilla.org is a chained CNAME). Everyone else seems to resolve
        * these fine, so we should too. So we loop here to try to resolve CNAME
        * chains ourselves. Of course, there must be a limit to protect against
        * CNAME loops.
        */

#define CNAME_LOOP_MAX 16

        CurrentName = AnsiName;

        for (CNameLoop = 0; CNameLoop < CNAME_LOOP_MAX; CNameLoop++)
        {
            adns_error = adns_synchronous(astate, CurrentName, adns_r_addr, quflags, &answer);

            if (adns_error != adns_s_ok)
            {
                adns_finish(astate);

                if (CurrentName != AnsiName)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                return DnsIntTranslateAdnsToDNS_STATUS(adns_error);
            }

            if (answer && answer->rrs.addr)
            {
                if (CurrentName != AnsiName)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));

                if (NULL == *QueryResultSet)
                {
                    adns_finish(astate);
                    return ERROR_OUTOFMEMORY;
                }

                (*QueryResultSet)->pNext = NULL;
                (*QueryResultSet)->wType = Type;
                (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
                (*QueryResultSet)->Data.A.IpAddress = answer->rrs.addr->addr.inet.sin_addr.s_addr;

                adns_finish(astate);

                (*QueryResultSet)->pName = (LPSTR)xstrsave(Name);

                return (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
            }

            if (NULL == answer || adns_s_prohibitedcname != answer->status || NULL == answer->cname)
            {
                adns_finish(astate);

                if (CurrentName != AnsiName)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                return ERROR_FILE_NOT_FOUND;
            }

            if (CurrentName != AnsiName)
                RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

            CurrentName = (LPSTR)xstrsaveA(answer->cname);

            if (!CurrentName)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
                adns_finish(astate);
                return ERROR_OUTOFMEMORY;
            }
        }

        adns_finish(astate);
        RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);
        return ERROR_FILE_NOT_FOUND;

    default:
        return ERROR_OUTOFMEMORY; /* XXX arty: find a better error code. */
    }
}

DNS_STATUS WINAPI
DnsQuery_UTF8(LPCSTR Name,
              WORD Type,
              DWORD Options,
              PIP4_ARRAY Servers,
              PDNS_RECORD *QueryResultSet,
              PVOID *Reserved)
{
    UNIMPLEMENTED;
    return ERROR_OUTOFMEMORY;
}

void
DnsIntFreeRecordList(PDNS_RECORD ToDelete)
{
    UINT i;
    PDNS_RECORD next = 0;

    while(ToDelete)
    {
        if(ToDelete->pName)
            RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete->pName);

        switch(ToDelete->wType)
        {
            case DNS_TYPE_CNAME:
            case DNS_TYPE_PTR:
            case DNS_TYPE_NS:
            case DNS_TYPE_MB:
            case DNS_TYPE_MD:
            case DNS_TYPE_MF:
            case DNS_TYPE_MG:
            case DNS_TYPE_MR:
                RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete->Data.PTR.pNameHost);
                break;

            case DNS_TYPE_MINFO:
            case DNS_TYPE_MX:
                RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete->Data.MX.pNameExchange);
                break;

            case DNS_TYPE_HINFO:
                for(i = 0; i < ToDelete->Data.TXT.dwStringCount; i++)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete->Data.TXT.pStringArray[i]);

                RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete->Data.TXT.pStringArray);
                break;
        }

        next = ToDelete->pNext;
        RtlFreeHeap(RtlGetProcessHeap(), 0, ToDelete);
        ToDelete = next;
    }
}
