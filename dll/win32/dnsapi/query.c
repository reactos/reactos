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

    if ((Name == NULL) ||
        (QueryResultSet == NULL))
        return ERROR_INVALID_PARAMETER;

    *QueryResultSet = NULL;

    /* Create an A or AAAA record for an IP4 or IP6 address */
    pRecord = CreateRecordForIpAddress(Name,
                                       Type);
    if (pRecord != NULL)
    {
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
            return DNS_ERROR_INVALID_NAME_CHAR;
        }

        i++;
    }

    RpcTryExcept
    {
        Status = R_ResolverQuery(NULL,
                                 Name,
                                 Type,
                                 Options,
                                 &dwRecords,
                                 (DNS_RECORDW **)QueryResultSet);
        DPRINT("R_ResolverQuery() returned %lu\n", Status);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
        DPRINT("Exception returned %lu\n", Status);
    }
    RpcEndExcept;

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
    Found = !_stricmp(Name, network_info->HostName) || !_stricmp(Name, TempName);
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


DNS_STATUS
WINAPI
Query_Main(LPCWSTR Name,
           WORD Type,
           DWORD Options,
           PDNS_RECORD *QueryResultSet)
{
    adns_state astate;
    int quflags = (Options & DNS_QUERY_NO_RECURSION) == 0 ? adns_qf_search : 0;
    int adns_error;
    adns_answer *answer;
    LPSTR CurrentName;
    unsigned CNameLoop;
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

    *QueryResultSet = NULL;

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
            (*QueryResultSet)->Flags.S.Section = DnsSectionAnswer;
            (*QueryResultSet)->Flags.S.CharSet = DnsCharSetUnicode;
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

        if (!adns_numservers(astate))
        {
            /* There are no servers to query so bail out */
            adns_finish(astate);
            RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiName);
            return ERROR_FILE_NOT_FOUND;
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
                (*QueryResultSet)->Flags.S.Section = DnsSectionAnswer;
                (*QueryResultSet)->Flags.S.CharSet = DnsCharSetUnicode;
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
