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

char
*xstrsave(const char *str)
{
    char *p;

    p = RtlAllocateHeap(RtlGetProcessHeap(), 0, strlen(str) + 1);

    if(p)
        strcpy(p, str);

    return p;
}

DNS_STATUS WINAPI
DnsQuery_A(LPCSTR Name,
           WORD Type,
           DWORD Options,
           PIP4_ARRAY Servers,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    adns_state astate;
    int quflags = 0, i;
    int adns_error;
    adns_answer *answer;
    LPSTR CurrentName;
    unsigned CNameLoop;

    *QueryResultSet = 0;

    switch(Type)
    {
        case DNS_TYPE_A:
            adns_error = adns_init(&astate, adns_if_noenv | adns_if_noerrprint | adns_if_noserverwarn | (Servers ? adns_if_noserver : 0), 0);

            if(adns_error != adns_s_ok)
                return DnsIntTranslateAdnsToDNS_STATUS(adns_error);

            if (Servers)
            {
                for(i = 0; i < Servers->AddrCount; i++)
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

            CurrentName = (LPSTR) Name;

            for (CNameLoop = 0; CNameLoop < CNAME_LOOP_MAX; CNameLoop++)
            {
                adns_error = adns_synchronous(astate, CurrentName, adns_r_addr, quflags, &answer);

                if(adns_error != adns_s_ok)
                {
                    adns_finish(astate);

                    if (CurrentName != Name)
                        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                    return DnsIntTranslateAdnsToDNS_STATUS(adns_error);
                }

                if(answer && answer->rrs.addr)
                {
                    if (CurrentName != Name)
                        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                    *QueryResultSet = (PDNS_RECORD)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));

                    if (NULL == *QueryResultSet)
                    {
                        adns_finish( astate );
                        return ERROR_OUTOFMEMORY;
                    }

                    (*QueryResultSet)->pNext = NULL;
                    (*QueryResultSet)->wType = Type;
                    (*QueryResultSet)->wDataLength = sizeof(DNS_A_DATA);
                    (*QueryResultSet)->Data.A.IpAddress = answer->rrs.addr->addr.inet.sin_addr.s_addr;

                    adns_finish(astate);

                    (*QueryResultSet)->pName = xstrsave( Name );

                    return NULL != (*QueryResultSet)->pName ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
                }

                if (NULL == answer || adns_s_prohibitedcname != answer->status || NULL == answer->cname)
                {
                    adns_finish(astate);

                    if (CurrentName != Name)
                        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                    return ERROR_FILE_NOT_FOUND;
                }

                if (CurrentName != Name)
                    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);

                CurrentName = xstrsave(answer->cname);

                if (!CurrentName)
                {
                    adns_finish(astate);
                    return ERROR_OUTOFMEMORY;
                }
            }

            adns_finish(astate);
            RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentName);
            return ERROR_FILE_NOT_FOUND;

        default:
            return ERROR_OUTOFMEMORY; /* XXX arty: find a better error code. */
    }
}

static PCHAR
DnsWToC(const WCHAR *WideString)
{
    int chars = wcstombs(NULL, WideString, 0);
    PCHAR out = RtlAllocateHeap(RtlGetProcessHeap(), 0, chars + 1);

    wcstombs(out, WideString, chars + 1);

    return out;
}

static PWCHAR
DnsCToW(const CHAR *NarrowString)
{
    int chars = mbstowcs(NULL, NarrowString, 0);
    PWCHAR out = RtlAllocateHeap(RtlGetProcessHeap(), 0, (chars + 1) * sizeof(WCHAR));

    mbstowcs(out, NarrowString, chars + 1);

    return out;
}

DNS_STATUS WINAPI
DnsQuery_W(LPCWSTR Name,
           WORD Type,
           DWORD Options,
           PIP4_ARRAY Servers,
           PDNS_RECORD *QueryResultSet,
           PVOID *Reserved)
{
    UINT i;
    PCHAR Buffer;
    DNS_STATUS Status;
    PDNS_RECORD QueryResultWide;
    PDNS_RECORD ConvertedRecord = 0, LastRecord = 0;

    Buffer = DnsWToC(Name);

    Status = DnsQuery_A(Buffer, Type, Options, Servers, &QueryResultWide, Reserved);

    while(Status == ERROR_SUCCESS && QueryResultWide)
    {
        switch(QueryResultWide->wType)
        {
            case DNS_TYPE_A:
            case DNS_TYPE_WKS:
                ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
                ConvertedRecord->pName = (PCHAR)DnsCToW(QueryResultWide->pName);
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = QueryResultWide->wDataLength;
                memcpy(ConvertedRecord, QueryResultWide, QueryResultWide->wDataLength);
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
                ConvertedRecord->pName = (PCHAR)DnsCToW(QueryResultWide->pName);
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = sizeof(DNS_PTR_DATA);
                ConvertedRecord->Data.PTR.pNameHost = (PCHAR)DnsCToW(QueryResultWide->Data.PTR.pNameHost);
                break;

            case DNS_TYPE_MINFO:
                ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
                ConvertedRecord->pName = (PCHAR)DnsCToW(QueryResultWide->pName);
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = sizeof(DNS_MINFO_DATA);
                ConvertedRecord->Data.MINFO.pNameMailbox = (PCHAR)DnsCToW(QueryResultWide->Data.MINFO.pNameMailbox);
                ConvertedRecord->Data.MINFO.pNameErrorsMailbox = (PCHAR)DnsCToW(QueryResultWide->Data.MINFO.pNameErrorsMailbox);
                break;

            case DNS_TYPE_MX:
                ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_RECORD));
                ConvertedRecord->pName = (PCHAR)DnsCToW(QueryResultWide->pName);
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = sizeof(DNS_MX_DATA);
                ConvertedRecord->Data.MX.pNameExchange = (PCHAR)DnsCToW( QueryResultWide->Data.MX.pNameExchange);
                ConvertedRecord->Data.MX.wPreference = QueryResultWide->Data.MX.wPreference;
                break;

            case DNS_TYPE_HINFO:
                ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_TXT_DATA) + QueryResultWide->Data.TXT.dwStringCount);
                ConvertedRecord->pName = (PCHAR)DnsCToW( QueryResultWide->pName );
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = sizeof(DNS_TXT_DATA) + (sizeof(PWCHAR) * QueryResultWide->Data.TXT.dwStringCount);
                ConvertedRecord->Data.TXT.dwStringCount = QueryResultWide->Data.TXT.dwStringCount;

                for(i = 0; i < ConvertedRecord->Data.TXT.dwStringCount; i++)
                    ConvertedRecord->Data.TXT.pStringArray[i] = (PCHAR)DnsCToW(QueryResultWide->Data.TXT.pStringArray[i]);

                break;

            case DNS_TYPE_NULL:
                ConvertedRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount);
                ConvertedRecord->pName = (PCHAR)DnsCToW(QueryResultWide->pName);
                ConvertedRecord->wType = QueryResultWide->wType;
                ConvertedRecord->wDataLength = sizeof(DNS_NULL_DATA) + QueryResultWide->Data.Null.dwByteCount;
                ConvertedRecord->Data.Null.dwByteCount = QueryResultWide->Data.Null.dwByteCount;
                memcpy(&ConvertedRecord->Data.Null.Data, &QueryResultWide->Data.Null.Data, QueryResultWide->Data.Null.dwByteCount);
                break;
        }

        if(LastRecord)
        {
            LastRecord->pNext = ConvertedRecord;
            LastRecord = LastRecord->pNext;
        }
        else
        {
            LastRecord = *QueryResultSet = ConvertedRecord;
        }
    }

    LastRecord->pNext = 0;

    /* The name */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    return Status;
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
