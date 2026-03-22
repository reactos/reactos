/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/query.c
 * PURPOSE:     DNSAPI functions
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

/*
 * Verbose DNS tracing.  Uncomment the define below to enable detailed
 * diagnostic output at every significant step of the resolution path.
 * Leave it commented out for normal (silent) builds.
 */
/* #define DNSAPI_VERBOSE_DEBUG */

#ifdef DNSAPI_VERBOSE_DEBUG
#define VTRACE(fmt, ...) DbgPrint("DNSAPI[%s]: " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define VTRACE(fmt, ...) ((void)0)
#endif

static
BOOL
ParseIpv4Address(
    _In_ PCWSTR AddressString,
    _Out_ PIN_ADDR pAddress)
{
    PCWSTR pTerminator = NULL;
    NTSTATUS Status;

    Status = RtlIpv4StringToAddressW(AddressString,
                                     TRUE,
                                     &pTerminator,
                                     pAddress);
    if (NT_SUCCESS(Status) && pTerminator != NULL && *pTerminator == L'\0')
        return TRUE;

    return FALSE;
}


static
BOOL
ParseIpv6Address(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR pAddress)
{
    PCWSTR pTerminator = NULL;
    NTSTATUS Status;

    Status = RtlIpv6StringToAddressW(AddressString,
                                     &pTerminator,
                                     pAddress);
    if (NT_SUCCESS(Status) && pTerminator != NULL && *pTerminator == L'\0')
        return TRUE;

    return FALSE;
}


static
PDNS_RECORDW
CreateRecordForIpAddress(
    _In_ PCWSTR Name,
    _In_ WORD Type)
{
    IN_ADDR Ip4Address;
    IN6_ADDR Ip6Address;
    PDNS_RECORDW pRecord = NULL;

    if (Type == DNS_TYPE_A)
    {
        if (ParseIpv4Address(Name, &Ip4Address))
        {
            pRecord = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(DNS_RECORDW));
            if (pRecord == NULL)
                return NULL;

            pRecord->pName = RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             (wcslen(Name) + 1) * sizeof(WCHAR));
            if (pRecord == NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, pRecord);
                return NULL;
            }

            wcscpy(pRecord->pName, Name);
            pRecord->wType = DNS_TYPE_A;
            pRecord->wDataLength = sizeof(DNS_A_DATA);
            pRecord->Flags.S.Section = DnsSectionQuestion;
            pRecord->Flags.S.CharSet = DnsCharSetUnicode;
            pRecord->dwTtl = 7 * 24 * 60 * 60;

            pRecord->Data.A.IpAddress = Ip4Address.S_un.S_addr;

            return pRecord;
        }
    }
    else if (Type == DNS_TYPE_AAAA)
    {
        if (ParseIpv6Address(Name, &Ip6Address))
        {
            pRecord = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(DNS_RECORDW));
            if (pRecord == NULL)
                return NULL;

            pRecord->pName = RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             (wcslen(Name) + 1) * sizeof(WCHAR));
            if (pRecord == NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, pRecord);
                return NULL;
            }

            wcscpy(pRecord->pName, Name);
            pRecord->wType = DNS_TYPE_AAAA;
            pRecord->wDataLength = sizeof(DNS_AAAA_DATA);
            pRecord->Flags.S.Section = DnsSectionQuestion;
            pRecord->Flags.S.CharSet = DnsCharSetUnicode;
            pRecord->dwTtl = 7 * 24 * 60 * 60;

            CopyMemory(&pRecord->Data.AAAA.Ip6Address,
                       &Ip6Address.u.Byte,
                       sizeof(IN6_ADDR));

            return pRecord;
        }
    }

    return NULL;
}


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
    WideString = RtlAllocateHeap(RtlGetProcessHeap(), 0, WideLen * sizeof(WCHAR));
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

static PCHAR
DnsWToUTF8(const WCHAR *WideString)
{
    PCHAR AnsiString;
    int AnsiLen = WideCharToMultiByte(CP_UTF8,
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
    WideCharToMultiByte(CP_UTF8,
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
DnsUTF8ToW(const CHAR *NarrowString)
{
    PWCHAR WideString;
    int WideLen = MultiByteToWideChar(CP_UTF8,
                                      0,
                                      NarrowString,
                                      -1,
                                      NULL,
                                      0);
    if (WideLen == 0)
        return NULL;
    WideString = RtlAllocateHeap(RtlGetProcessHeap(), 0, WideLen * sizeof(WCHAR));
    if (WideString == NULL)
    {
        return NULL;
    }
    MultiByteToWideChar(CP_UTF8,
                        0,
                        NarrowString,
                        -1,
                        WideString,
                        WideLen);

    return WideString;
}

DNS_STATUS WINAPI
DnsQuery_CodePage(UINT CodePage,
           LPCSTR Name,
           WORD Type,
           DWORD Options,
           PVOID Extra,
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

    switch (CodePage)
    {
    case CP_ACP:
        Buffer = DnsCToW(Name);
        break;

    case CP_UTF8:
        Buffer = DnsUTF8ToW(Name);
        break;

    default:
        return ERROR_INVALID_PARAMETER;
    }

    Status = DnsQuery_W(Buffer, Type, Options, Extra, &QueryResultWide, Reserved);

    while (Status == ERROR_SUCCESS && QueryResultWide)
    {
        switch (QueryResultWide->wType)
        {
        case DNS_TYPE_A:
        case DNS_TYPE_WKS:
        case DNS_TYPE_CNAME:
        case DNS_TYPE_PTR:
        case DNS_TYPE_NS:
        case DNS_TYPE_MB:
        case DNS_TYPE_MD:
        case DNS_TYPE_MF:
        case DNS_TYPE_MG:
        case DNS_TYPE_MR:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
            break;

        case DNS_TYPE_MINFO:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_TXT_DATA) + QueryResultWide->Data.TXT.dwStringCount);
            break;

        case DNS_TYPE_NULL:
            ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount);
            break;
        }
        if (ConvertedRecord == NULL)
        {
            /* The name */
            RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
            /* The result*/
            DnsIntFreeRecordList(QueryResultWide);
            QueryResultSet = NULL;
            return ERROR_OUTOFMEMORY;
        }

        if (CodePage == CP_ACP)
        {
            ConvertedRecord->pName = DnsWToC((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->Flags.S.CharSet = DnsCharSetAnsi;
        }
        else
        {
            ConvertedRecord->pName = DnsWToUTF8((PWCHAR)QueryResultWide->pName);
            ConvertedRecord->Flags.S.CharSet = DnsCharSetUtf8;
        }

        ConvertedRecord->wType = QueryResultWide->wType;

        switch (QueryResultWide->wType)
        {
        case DNS_TYPE_A:
        case DNS_TYPE_WKS:
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
            ConvertedRecord->wDataLength = sizeof(DNS_PTR_DATA);
            if (CodePage == CP_ACP)
                ConvertedRecord->Data.PTR.pNameHost = DnsWToC((PWCHAR)QueryResultWide->Data.PTR.pNameHost);
            else
                ConvertedRecord->Data.PTR.pNameHost = DnsWToUTF8((PWCHAR)QueryResultWide->Data.PTR.pNameHost);
            break;

        case DNS_TYPE_MINFO:
            ConvertedRecord->wDataLength = sizeof(DNS_MINFO_DATA);
            if (CodePage == CP_ACP)
            {
                ConvertedRecord->Data.MINFO.pNameMailbox = DnsWToC((PWCHAR)QueryResultWide->Data.MINFO.pNameMailbox);
                ConvertedRecord->Data.MINFO.pNameErrorsMailbox = DnsWToC((PWCHAR)QueryResultWide->Data.MINFO.pNameErrorsMailbox);
            }
            else
            {
                ConvertedRecord->Data.MINFO.pNameMailbox = DnsWToUTF8((PWCHAR)QueryResultWide->Data.MINFO.pNameMailbox);
                ConvertedRecord->Data.MINFO.pNameErrorsMailbox = DnsWToUTF8((PWCHAR)QueryResultWide->Data.MINFO.pNameErrorsMailbox);
            }
            break;

        case DNS_TYPE_MX:
            ConvertedRecord->wDataLength = sizeof(DNS_MX_DATA);
            if (CodePage == CP_ACP)
                ConvertedRecord->Data.MX.pNameExchange = DnsWToC((PWCHAR)QueryResultWide->Data.MX.pNameExchange);
            else
                ConvertedRecord->Data.MX.pNameExchange = DnsWToUTF8((PWCHAR)QueryResultWide->Data.MX.pNameExchange);
            ConvertedRecord->Data.MX.wPreference = QueryResultWide->Data.MX.wPreference;
            break;

        case DNS_TYPE_HINFO:
            ConvertedRecord->wDataLength = sizeof(DNS_TXT_DATA) + (sizeof(PCHAR) * QueryResultWide->Data.TXT.dwStringCount);
            ConvertedRecord->Data.TXT.dwStringCount = QueryResultWide->Data.TXT.dwStringCount;

            if (CodePage == CP_ACP)
                for (i = 0; i < ConvertedRecord->Data.TXT.dwStringCount; i++)
                    ConvertedRecord->Data.TXT.pStringArray[i] = DnsWToC((PWCHAR)QueryResultWide->Data.TXT.pStringArray[i]);
            else
                for (i = 0; i < ConvertedRecord->Data.TXT.dwStringCount; i++)
                    ConvertedRecord->Data.TXT.pStringArray[i] = DnsWToUTF8((PWCHAR)QueryResultWide->Data.TXT.pStringArray[i]);

            break;

        case DNS_TYPE_NULL:
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

DNS_STATUS WINAPI
DnsQuery_A(LPCSTR Name,
           WORD Type,
           DWORD Options,
           PVOID Extra,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    return DnsQuery_CodePage(CP_ACP, Name, Type, Options, Extra, QueryResultSet, Reserved);
}

DNS_STATUS WINAPI
DnsQuery_UTF8(LPCSTR Name,
              WORD Type,
              DWORD Options,
              PVOID Extra,
              PDNS_RECORD *QueryResultSet,
              PVOID *Reserved)
{
    return DnsQuery_CodePage(CP_UTF8, Name, Type, Options, Extra, QueryResultSet, Reserved);
}

DNS_STATUS
WINAPI
DnsQuery_W(LPCWSTR Name,
           WORD Type,
           DWORD Options,
           PVOID Extra,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    DWORD dwRecords = 0;
    PDNS_RECORDW pRecord = NULL;
    size_t NameLen, i;
    DNS_STATUS Status = ERROR_SUCCESS;

    DPRINT("DnsQuery_W()\n");
    VTRACE("Name='%S' Type=0x%04x Options=0x%08lx\n", Name, (unsigned)Type, (unsigned long)Options);

    if ((Name == NULL) ||
        (QueryResultSet == NULL))
        return ERROR_INVALID_PARAMETER;

    *QueryResultSet = NULL;

    /* Create an A or AAAA record for an IP4 or IP6 address */
    pRecord = CreateRecordForIpAddress(Name,
                                       Type);
    if (pRecord != NULL)
    {
        VTRACE("Name is a literal IP address; returning synthesised record\n");
        *QueryResultSet = (PDNS_RECORD)pRecord;
        return ERROR_SUCCESS;
    }

    /*
     * Check allowed characters
     * According to RFC a-z,A-Z,0-9,-,_, but can't start or end with - or _
     */
    NameLen = wcslen(Name);
    if (Name[0] == L'-' || Name[0] == L'_' || Name[NameLen - 1] == L'-' ||
        Name[NameLen - 1] == L'_' || wcsstr(Name, L"..") != NULL)
    {
        VTRACE("Name failed border/double-dot check; returning ERROR_INVALID_NAME\n");
        return ERROR_INVALID_NAME;
    }

    i = 0;
    while (i < NameLen)
    {
        if (!((Name[i] >= L'a' && Name[i] <= L'z') ||
              (Name[i] >= L'A' && Name[i] <= L'Z') ||
              (Name[i] >= L'0' && Name[i] <= L'9') ||
              Name[i] == L'-' || Name[i] == L'_' || Name[i] == L'.'))
        {
            VTRACE("Invalid character U+%04x at position %u; returning DNS_ERROR_INVALID_NAME_CHAR\n",
                   (unsigned)Name[i], (unsigned)i);
            return DNS_ERROR_INVALID_NAME_CHAR;
        }

        i++;
    }

    VTRACE("Calling R_ResolverQuery via RPC (Name='%S' Type=0x%04x Options=0x%08lx)\n",
           Name, (unsigned)Type, (unsigned long)Options);
    RpcTryExcept
    {
        Status = R_ResolverQuery(NULL,
                                 Name,
                                 Type,
                                 Options,
                                 &dwRecords,
                                 (DNS_RECORDW **)QueryResultSet);
        DPRINT("R_ResolverQuery() returned %lu\n", Status);
        VTRACE("R_ResolverQuery returned Status=%lu dwRecords=%lu\n",
               (unsigned long)Status, (unsigned long)dwRecords);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
        VTRACE("RPC exception code=%lu\n", (unsigned long)Status);
    }
    RpcEndExcept;

    VTRACE("Returning Status=%lu\n", (unsigned long)Status);
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


/* This function is far from perfect but it works enough */
IP4_ADDRESS
CheckForCurrentHostname(CONST CHAR * Name, PFIXED_INFO network_info)
{
    PCHAR TempName;
    DWORD AdapterAddressesSize, Status;
    IP4_ADDRESS ret = 0, Address;
    PIP_ADAPTER_ADDRESSES Addresses = NULL, pip;
    BOOL Found = FALSE;

    VTRACE("Name='%s' HostName='%s' DomainName='%s'\n",
           Name, network_info->HostName, network_info->DomainName);

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
    VTRACE("Comparing Name='%s' against HostName='%s' and FQDN='%s'\n",
           Name, network_info->HostName, TempName);
    Found = !_stricmp(Name, network_info->HostName) || !_stricmp(Name, TempName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, TempName);
    if (!Found)
    {
        VTRACE("Name does not match local host; not a local address\n");
        return 0;
    }
    VTRACE("Name matches local host; enumerating adapters\n");
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
        VTRACE("GetAdaptersAddresses returned zero size; no adapters\n");
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
        VTRACE("GetAdaptersAddresses failed with status %lu\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Addresses);
        return 0;
    }
    for (pip = Addresses; pip != NULL; pip = pip->Next) {
        Address = ((LPSOCKADDR_IN)pip->FirstUnicastAddress->Address.lpSockaddr)->sin_addr.S_un.S_addr;
        VTRACE("Adapter '%S': address 0x%08lx (loopback=0x%08lx)\n",
               pip->FriendlyName, (unsigned long)Address,
               (unsigned long)ntohl(INADDR_LOOPBACK));
        if (Address != ntohl(INADDR_LOOPBACK))
            break;
    }
    if (Address && Address != ntohl(INADDR_LOOPBACK))
    {
        VTRACE("Using local address 0x%08lx\n", (unsigned long)Address);
        ret = Address;
    }
    else
    {
        VTRACE("No non-loopback address found\n");
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, Addresses);
    return ret;
}


/*
 * Query_Main
 *
 * Direct DNS resolver using raw UDP queries (via dnsresolv).
 * Replaces the former ADNS-based implementation so that every DNS
 * resource record is returned with its real wire-format TTL value
 * instead of ADNS's synthesised "expires" timestamp.
 *
 * CNAME chains are followed manually (up to CNAME_LOOP_MAX hops) to
 * match the behaviour of the previous implementation.
 */

#define CNAME_LOOP_MAX 16

/* Simple monotonically-increasing transaction ID.  Not thread-safe but
 * sufficient for a stub resolver that sends one query at a time. */
static USHORT g_DnsQueryID = 1;

/*
 * Query_BuildPacket
 *
 * Fills *pBuf (which must be at least 12 + strlen(Name)+2 + 4 bytes) with a
 * single-question DNS query for <Name>/<Type>/IN.  Returns the packet length.
 */
static ULONG
Query_BuildPacket(
    _Out_ PCHAR  pBuf,
    _In_  PCHAR  Name,
    _In_  USHORT Type,
    _In_  BOOL   Recurse,
    _In_  USHORT RequestID)
{
    int i = 0, j, k;

    /* Transaction ID */
    ((PUSHORT)&pBuf[i])[0] = htons(RequestID);  i += 2;
    /* Flags: QR=0 (query), OPCODE=0, RD=Recurse */
    pBuf[i++] = Recurse ? 0x01 : 0x00;
    pBuf[i++] = 0x00;
    /* QDCOUNT=1, ANCOUNT=NSCOUNT=ARCOUNT=0 */
    ((PUSHORT)&pBuf[i])[0] = htons(1);  i += 2;
    RtlZeroMemory(&pBuf[i], 6);         i += 6;

    /* QNAME: length-prefixed labels */
    j = i++;
    for (k = 0; k < (int)strlen(Name); k++)
    {
        if (Name[k] != '.')
            pBuf[i++] = Name[k];
        else
        {
            pBuf[j] = (CHAR)(i - j - 1);
            j = i++;
        }
    }
    pBuf[j]   = (CHAR)(i - j - 1);
    pBuf[i++] = 0x00;

    /* QTYPE and QCLASS=IN */
    ((PUSHORT)&pBuf[i])[0] = htons(Type);  i += 2;
    ((PUSHORT)&pBuf[i])[0] = htons(1);     /* IN */

    return (ULONG)(i + 2);
}

DNS_STATUS
WINAPI
Query_Main(LPCWSTR Name,
           WORD Type,
           DWORD Options,
           PDNS_RECORD *QueryResultSet)
{
    PFIXED_INFO network_info;
    ULONG network_info_blen = 0;
    DWORD network_info_result;
    PIP_ADDR_STRING pip;
    IP4_ADDRESS Address;
    PCHAR HostWithDomainName;
    PCHAR AnsiName;
    size_t NameLen = 0;
    /* dnsresolv config */
    DNS_RESOLVER_CONFIG Config;
    BOOL bRecurse;
    BOOL bFoundServer;
    /* CNAME-chain state */
    PCHAR CurrentName;
    unsigned CNameLoop;
    PDNS_RECORD pLastRecord;
    DNS_STATUS Status;
    /* Per-iteration buffers */
    PCHAR QueryBuf;
    PCHAR ResponseBuf;
    ULONG QueryLen;
    ULONG ResponseLen;
    USHORT RequestID;
    /* Response parsing */
    int ri, rk, qi;
    USHORT NumQuestions, NumAnswers;
    UCHAR RCode;
    BOOL bGotAnswer;
    PCHAR CNameTarget;

    if (Name == NULL)
        return ERROR_INVALID_PARAMETER;
    if (QueryResultSet == NULL)
        return ERROR_INVALID_PARAMETER;

    *QueryResultSet = NULL;

    VTRACE("Name='%S' Type=0x%04x Options=0x%08lx\n",
           Name, (unsigned)Type, (unsigned long)Options);

    switch (Type)
    {
    case DNS_TYPE_A:
        /* Convert the name from Unicode to ANSI */
        NameLen = WideCharToMultiByte(CP_ACP, 0, Name, -1, NULL, 0, NULL, 0);
        AnsiName = RtlAllocateHeap(RtlGetProcessHeap(), 0, NameLen);
        if (AnsiName == NULL)
            return ERROR_OUTOFMEMORY;
        WideCharToMultiByte(CP_ACP, 0, Name, -1, AnsiName, NameLen, NULL, 0);
        NameLen--;
        VTRACE("ANSI name='%s' (len=%u)\n", AnsiName, (unsigned)NameLen);

        /* Get network parameters (domain name, DNS server list) */
        network_info_result = GetNetworkParams(NULL, &network_info_blen);
        network_info = (PFIXED_INFO)RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                                    (size_t)network_info_blen);
        if (network_info == NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            return ERROR_OUTOFMEMORY;
        }
        network_info_result = GetNetworkParams(network_info, &network_info_blen);
        if (network_info_result != ERROR_SUCCESS)
        {
            VTRACE("GetNetworkParams failed with %lu\n", (unsigned long)network_info_result);
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            return network_info_result;
        }
        VTRACE("NetworkParams: HostName='%s' DomainName='%s' DNS='%s'\n",
               network_info->HostName, network_info->DomainName,
               network_info->DnsServerList.IpAddress.String);

        /* Shortcut: if the name resolves to the local machine, synthesise a
         * record without a network query. */
        if ((Address = CheckForCurrentHostname(NameLen != 0 ? AnsiName
                                                            : network_info->HostName,
                                               network_info)) != 0)
        {
            size_t TempLen = 2, StringLength = 0;
            VTRACE("Local hostname match; returning synthesised A record (IP=0x%08lx)\n",
                   (unsigned long)Address);
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            StringCchLengthA(network_info->HostName,
                             sizeof(network_info->HostName), &StringLength);
            TempLen += StringLength;
            StringCchLengthA(network_info->DomainName,
                             sizeof(network_info->DomainName), &StringLength);
            TempLen += StringLength;
            HostWithDomainName = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(), 0, TempLen);
            StringCchCopyA(HostWithDomainName, TempLen, network_info->HostName);
            if (network_info->DomainName[0])
            {
                StringCchCatA(HostWithDomainName, TempLen, ".");
                StringCchCatA(HostWithDomainName, TempLen, network_info->DomainName);
            }
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);

            *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                                           sizeof(DNS_RECORD));
            if (NULL == *QueryResultSet)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, HostWithDomainName);
                return ERROR_OUTOFMEMORY;
            }

            (*QueryResultSet)->pNext = NULL;
            (*QueryResultSet)->wType = Type;
            (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
            (*QueryResultSet)->Flags.S.Section = DnsSectionAnswer;
            (*QueryResultSet)->Flags.S.CharSet = DnsCharSetUnicode;
            (*QueryResultSet)->dwTtl = 7 * 24 * 60 * 60;
            (*QueryResultSet)->Data.A.IpAddress = Address;
            (*QueryResultSet)->pName = (LPSTR)DnsCToW(HostWithDomainName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, HostWithDomainName);
            return (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        }

        if ((Options & DNS_QUERY_NO_WIRE_QUERY) != 0)
        {
            VTRACE("DNS_QUERY_NO_WIRE_QUERY set; skipping wire query\n");
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);
            return ERROR_FILE_NOT_FOUND;
        }

        /* Build a resolver config from the first usable DNS server */
        bRecurse = (Options & DNS_QUERY_NO_RECURSION) == 0;
        DnsResolv_InitConfig(&Config);
        Config.Recurse = bRecurse;
        bFoundServer = FALSE;

        for (pip = &(network_info->DnsServerList); pip; pip = pip->Next)
        {
            unsigned long addr = inet_addr(pip->IpAddress.String);
            VTRACE("Candidate DNS server: '%s' (addr=0x%08lx)\n",
                   pip->IpAddress.String, addr);
            if (addr != INADDR_ANY && addr != INADDR_NONE)
            {
                StringCchCopyA(Config.ServerAddress,
                               sizeof(Config.ServerAddress),
                               pip->IpAddress.String);
                bFoundServer = TRUE;
                break;
            }
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, network_info);

        if (!bFoundServer)
        {
            VTRACE("No usable DNS server found; returning ERROR_FILE_NOT_FOUND\n");
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            return ERROR_FILE_NOT_FOUND;
        }
        VTRACE("Using DNS server '%s' Recurse=%d\n", Config.ServerAddress, (int)bRecurse);

        /*
         * Follow CNAME chains.  We allow up to CNAME_LOOP_MAX hops to guard
         * against loops in the DNS data.
         */
        CurrentName = AnsiName;
        pLastRecord = NULL;
        Status      = ERROR_FILE_NOT_FOUND;

        for (CNameLoop = 0; CNameLoop < CNAME_LOOP_MAX; CNameLoop++)
        {
            VTRACE("CNAME iteration %u: querying '%s'\n", CNameLoop, CurrentName);

            /* Allocate query packet buffer */
            QueryLen = 12 + ((ULONG)strlen(CurrentName) + 2) + 4;
            QueryBuf = RtlAllocateHeap(RtlGetProcessHeap(), 0, QueryLen);
            if (QueryBuf == NULL)
            {
                Status = ERROR_OUTOFMEMORY;
                break;
            }

            RequestID = g_DnsQueryID++;
            Query_BuildPacket(QueryBuf, CurrentName, DNS_TYPE_A, bRecurse, RequestID);
            VTRACE("Built query packet: ID=0x%04x Len=%lu\n",
                   (unsigned)RequestID, (unsigned long)QueryLen);

            /* Allocate response buffer */
            ResponseLen = 512;
            ResponseBuf = RtlAllocateHeap(RtlGetProcessHeap(), 0, ResponseLen);
            if (ResponseBuf == NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, QueryBuf);
                Status = ERROR_OUTOFMEMORY;
                break;
            }

            /* Send the query; DnsResolv_SendRequest updates ResponseLen */
            VTRACE("Sending query to '%s'...\n", Config.ServerAddress);
            if (!DnsResolv_SendRequest(QueryBuf, QueryLen,
                                       ResponseBuf, &ResponseLen,
                                       &Config))
            {
                VTRACE("DnsResolv_SendRequest failed (timeout or socket error)\n");
                RtlFreeHeap(RtlGetProcessHeap(), 0, QueryBuf);
                RtlFreeHeap(RtlGetProcessHeap(), 0, ResponseBuf);
                Status = ERROR_FILE_NOT_FOUND;
                break;
            }
            VTRACE("Response received: %lu bytes\n", (unsigned long)ResponseLen);

            RtlFreeHeap(RtlGetProcessHeap(), 0, QueryBuf);
            QueryBuf = NULL;

            /* Check the response code */
            RCode = ResponseBuf[3] & 0x0F;
            VTRACE("Response flags byte[2]=0x%02x byte[3]=0x%02x RCODE=%u (%s)\n",
                   (unsigned)(UCHAR)ResponseBuf[2], (unsigned)(UCHAR)ResponseBuf[3],
                   (unsigned)RCode, DnsResolv_RCodeIDtoRCodeName(RCode));
            if (RCode != 0)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, ResponseBuf);
                Status = ERROR_FILE_NOT_FOUND;
                break;
            }

            NumQuestions = ntohs(((PUSHORT)&ResponseBuf[4])[0]);
            NumAnswers   = ntohs(((PUSHORT)&ResponseBuf[6])[0]);
            VTRACE("NumQuestions=%u NumAnswers=%u\n",
                   (unsigned)NumQuestions, (unsigned)NumAnswers);
            rk = 12;

            /* Skip the question section */
            for (qi = 0; qi < (int)NumQuestions; qi++)
            {
                CHAR dummy[256];
                rk += DnsResolv_ExtractName(ResponseBuf, dummy, (USHORT)rk, 0);
                rk += 4; /* QTYPE + QCLASS */
            }

            /* Parse answer resource records */
            bGotAnswer  = FALSE;
            CNameTarget = NULL;
            Status      = ERROR_FILE_NOT_FOUND;

            for (ri = 0; ri < (int)NumAnswers; ri++)
            {
                CHAR    RRName[256];
                USHORT  RRType;
                ULONG   TTL;
                USHORT  RDLength;
                PDNS_RECORD pRecord;

                rk += DnsResolv_ExtractName(ResponseBuf, RRName, (USHORT)rk, 0);
                RRType   = ntohs(((PUSHORT)&ResponseBuf[rk])[0]);  rk += 2;
                /* Class */ rk += 2;
                TTL      = ntohl(((PULONG)&ResponseBuf[rk])[0]);   rk += 4;
                RDLength = ntohs(((PUSHORT)&ResponseBuf[rk])[0]);  rk += 2;

                VTRACE("  RR[%d]: Name='%s' Type=0x%04x (%s) TTL=%lu RDLength=%u\n",
                       ri, RRName, (unsigned)RRType,
                       DnsResolv_TypeIDtoTypeName(RRType),
                       (unsigned long)TTL, (unsigned)RDLength);

                if (RRType == DNS_TYPE_A && RDLength == 4)
                {
                    ULONG ipAddr = *(PULONG)&ResponseBuf[rk];
                    VTRACE("    A record: IP=%u.%u.%u.%u\n",
                           (unsigned)(ipAddr & 0xFF),
                           (unsigned)((ipAddr >> 8) & 0xFF),
                           (unsigned)((ipAddr >> 16) & 0xFF),
                           (unsigned)((ipAddr >> 24) & 0xFF));

                    pRecord = RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(DNS_RECORD));
                    if (pRecord == NULL)
                    {
                        if (CNameTarget)
                            RtlFreeHeap(RtlGetProcessHeap(), 0, CNameTarget);
                        RtlFreeHeap(RtlGetProcessHeap(), 0, ResponseBuf);
                        Status = ERROR_OUTOFMEMORY;
                        goto cleanup;
                    }

                    pRecord->wType         = DNS_TYPE_A;
                    pRecord->wDataLength   = sizeof(DNS_A_DATA);
                    pRecord->Flags.S.Section = DnsSectionAnswer;
                    pRecord->Flags.S.CharSet = DnsCharSetUnicode;
                    pRecord->dwTtl         = TTL; /* raw TTL from the wire */
                    pRecord->Data.A.IpAddress = ipAddr;
                    pRecord->pName         = (LPSTR)DnsCToW(RRName);

                    if (pRecord->pName == NULL)
                    {
                        VTRACE("    DnsCToW failed for RRName\n");
                        RtlFreeHeap(RtlGetProcessHeap(), 0, pRecord);
                        if (CNameTarget)
                            RtlFreeHeap(RtlGetProcessHeap(), 0, CNameTarget);
                        RtlFreeHeap(RtlGetProcessHeap(), 0, ResponseBuf);
                        Status = ERROR_OUTOFMEMORY;
                        goto cleanup;
                    }

                    if (pLastRecord)
                        pLastRecord->pNext = pRecord;
                    else
                        *QueryResultSet = pRecord;
                    pLastRecord = pRecord;

                    bGotAnswer = TRUE;
                    Status     = ERROR_SUCCESS;
                }
                else if (RRType == DNS_TYPE_CNAME && CNameTarget == NULL)
                {
                    /* Remember the CNAME target in case we need to re-query */
                    CHAR CNameBuf[256];
                    DnsResolv_ExtractName(ResponseBuf, CNameBuf,
                                         (USHORT)rk, (UCHAR)RDLength);
                    VTRACE("    CNAME -> '%s'\n", CNameBuf);
                    CNameTarget = xstrsaveA(CNameBuf);
                }
                else
                {
                    VTRACE("    Skipping RR type 0x%04x\n", (unsigned)RRType);
                }

                rk += RDLength;
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, ResponseBuf);

            VTRACE("After parsing: bGotAnswer=%d CNameTarget='%s'\n",
                   (int)bGotAnswer, CNameTarget ? CNameTarget : "(none)");

            if (bGotAnswer)
            {
                /* Got at least one A record — we are done */
                if (CNameTarget)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, CNameTarget);
                break;
            }

            if (CNameTarget == NULL)
            {
                /* No A record and no CNAME to follow */
                VTRACE("No A record and no CNAME; stopping\n");
                Status = ERROR_FILE_NOT_FOUND;
                break;
            }

            /* Follow the CNAME by re-querying */
            VTRACE("Following CNAME to '%s'\n", CNameTarget);
            if (CurrentName != AnsiName)
                RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);
            CurrentName = CNameTarget;
        } /* CNAME loop */

cleanup:
        if (CurrentName != AnsiName)
            RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);

        /* On failure, free any partially-built record list */
        if (Status != ERROR_SUCCESS && *QueryResultSet != NULL)
        {
            DnsIntFreeRecordList(*QueryResultSet);
            *QueryResultSet = NULL;
        }

        VTRACE("Returning Status=%lu\n", (unsigned long)Status);
        return Status;

    default:
        VTRACE("Unsupported query type 0x%04x\n", (unsigned)Type);
        return ERROR_OUTOFMEMORY; /* XXX arty: find a better error code. */
    }
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

BOOL
WINAPI
DnsFlushResolverCache(VOID)
{
    DNS_STATUS Status = ERROR_SUCCESS;

    DPRINT("DnsFlushResolverCache()\n");

    RpcTryExcept
    {
        Status = R_ResolverFlushCache(NULL);
        DPRINT("R_ResolverFlushCache() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

    return (Status == ERROR_SUCCESS);
}


BOOL
WINAPI
DnsFlushResolverCacheEntry_A(
    _In_ LPCSTR pszEntry)
{
    DNS_STATUS Status = ERROR_SUCCESS;
    LPWSTR pszUnicodeEntry;

    DPRINT1("DnsFlushResolverCacheEntry_A(%s)\n", pszEntry);

    if (pszEntry == NULL)
        return FALSE;

    pszUnicodeEntry = DnsCToW(pszEntry);
    if (pszUnicodeEntry == NULL)
        return FALSE;

    RpcTryExcept
    {
        Status = R_ResolverFlushCacheEntry(NULL, pszUnicodeEntry, DNS_TYPE_ANY);
        DPRINT("R_ResolverFlushCacheEntry() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

    RtlFreeHeap(RtlGetProcessHeap(), 0, pszUnicodeEntry);

    return (Status == ERROR_SUCCESS);
}


BOOL
WINAPI
DnsFlushResolverCacheEntry_UTF8(
    _In_ LPCSTR pszEntry)
{
    DNS_STATUS Status = ERROR_SUCCESS;
    LPWSTR pszUnicodeEntry;

    DPRINT1("DnsFlushResolverCacheEntry_UTF8(%s)\n", pszEntry);

    if (pszEntry == NULL)
        return FALSE;

    pszUnicodeEntry = DnsCToW(pszEntry);
    if (pszUnicodeEntry == NULL)
        return FALSE;

    RpcTryExcept
    {
        Status = R_ResolverFlushCacheEntry(NULL, pszUnicodeEntry, DNS_TYPE_ANY);
        DPRINT("R_ResolverFlushCacheEntry() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

    RtlFreeHeap(RtlGetProcessHeap(), 0, pszUnicodeEntry);

    return (Status == ERROR_SUCCESS);
}


BOOL
WINAPI
DnsFlushResolverCacheEntry_W(
    _In_ LPCWSTR pszEntry)
{
    DNS_STATUS Status = ERROR_SUCCESS;

    DPRINT1("DnsFlushResolverCacheEntry_W(%S)\n", pszEntry);

    if (pszEntry == NULL)
        return FALSE;

    RpcTryExcept
    {
        Status = R_ResolverFlushCacheEntry(NULL, pszEntry, DNS_TYPE_ANY);
        DPRINT("R_ResolverFlushCacheEntry() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

    return (Status == ERROR_SUCCESS);
}


BOOL
WINAPI
DnsGetCacheDataTable(
    _Out_ PDNS_CACHE_ENTRY *DnsCache)
{
    DNS_STATUS Status = ERROR_SUCCESS;
    PDNS_CACHE_ENTRY CacheEntries = NULL;

    if (DnsCache == NULL)
        return FALSE;

    RpcTryExcept
    {
        Status = CRrReadCache(NULL,
                              &CacheEntries);
        DPRINT("CRrReadCache() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT1("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

    if (Status != ERROR_SUCCESS)
        return FALSE;

    if (CacheEntries == NULL)
        return FALSE;

    *DnsCache = CacheEntries;

    return TRUE;
}

DWORD
WINAPI
GetCurrentTimeInSeconds(VOID)
{
    FILETIME Time;
    FILETIME Adjustment;
    ULARGE_INTEGER lTime, lAdj;
    SYSTEMTIME st = {1970, 1, 0, 1, 0, 0, 0};

    SystemTimeToFileTime(&st, &Adjustment);
    memcpy(&lAdj, &Adjustment, sizeof(lAdj));
    GetSystemTimeAsFileTime(&Time);
    memcpy(&lTime, &Time, sizeof(lTime));
    lTime.QuadPart -= lAdj.QuadPart;
    return (DWORD)(lTime.QuadPart/10000000ULL);
}
