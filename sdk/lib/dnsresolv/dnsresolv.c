/*
 * PROJECT:     ReactOS DNS Resolver Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        sdk/lib/dnsresolv/dnsresolv.c
 * PURPOSE:     DNS resolver functions extracted from the nslookup utility
 * COPYRIGHT:   Copyright 2009 Lucas Suggs <lucas.suggs@gmail.com>
 *              Copyright 2024 ReactOS Contributors
 */

#include "dnsresolv.h"
#include <stdio.h>
#include <string.h>

/* String literals for type, class, opcode and rcode names */
static const CHAR szTypeA[]       = "A";
static const CHAR szTypeCNAME[]   = "CNAME";
static const CHAR szTypeMX[]      = "MX";
static const CHAR szTypeNS[]      = "NS";
static const CHAR szTypePTR[]     = "PTR";
static const CHAR szTypeSOA[]     = "SOA";
static const CHAR szTypeWKS[]     = "WKS";
static const CHAR szTypeSRV[]     = "SRV";
static const CHAR szTypeAny[]     = "ANY";
static const CHAR szTypeUnknown[] = "Unknown";

static const CHAR szClassIN[]      = "IN";
static const CHAR szClassAny[]     = "ANY";
static const CHAR szClassUnknown[] = "Unknown";

static const CHAR szOpcodeQuery[]    = "QUERY";
static const CHAR szOpcodeIQuery[]   = "IQUERY";
static const CHAR szOpcodeStatus[]   = "STATUS";
static const CHAR szOpcodeReserved[] = "RESERVED";

static const CHAR szRCodeNOERROR[]  = "NOERROR";
static const CHAR szRCodeFORMERR[]  = "FORMERR";
static const CHAR szRCodeFAILURE[]  = "FAILURE";
static const CHAR szRCodeNXDOMAIN[] = "NXDOMAIN";
static const CHAR szRCodeNOTIMP[]   = "NOTIMP";
static const CHAR szRCodeREFUSED[]  = "REFUSED";
static const CHAR szRCodeReserved[] = "RESERVED";

/* -----------------------------------------------------------------------
 * DnsResolv_InitConfig
 * ----------------------------------------------------------------------- */

VOID
DnsResolv_InitConfig(
    _Out_ PDNS_RESOLVER_CONFIG pConfig)
{
    RtlZeroMemory(pConfig, sizeof(*pConfig));
    pConfig->Port    = DNSRESOLV_DEFAULT_PORT;
    pConfig->Timeout = DNSRESOLV_DEFAULT_TIMEOUT;
    pConfig->Retry   = DNSRESOLV_DEFAULT_RETRY;
    pConfig->Recurse = TRUE;
}

/* -----------------------------------------------------------------------
 * DnsResolv_IsValidIP
 * ----------------------------------------------------------------------- */

BOOL
DnsResolv_IsValidIP(
    _In_ PCHAR pInput)
{
    int i = 0, l = 0, b = 0, c = 1;

    /* Max length of an IPv4 address (e.g. 255.255.255.255) is 15 chars. */
    l = (int)strlen(pInput);
    if (l > 15)
        return FALSE;

    /* b counts digits in the current octet; reset after '.'. */
    for (; i < l; i++)
    {
        if ('.' == pInput[i])
        {
            if (!b)    return FALSE;
            if (b > 3) return FALSE;
            b = 0;
            c++;
        }
        else
        {
            b++;
            if ((pInput[i] < '0') || (pInput[i] > '9'))
                return FALSE;
        }
    }

    if (b > 3) return FALSE;

    /* Need exactly 4 octets. */
    if (c < 4) return FALSE;

    return TRUE;
}

/* -----------------------------------------------------------------------
 * DnsResolv_ReverseIP
 * ----------------------------------------------------------------------- */

VOID
DnsResolv_ReverseIP(
    _In_  PCHAR pIP,
    _Out_ PCHAR pReturn)
{
    int i, j, k = 0;

    j = (int)strlen(pIP) - 1;
    i = j;

    /* Input:  A.B.C.D  ->  Output:  D.C.B.A */

    /* D */
    for (; i > 0; i--)
        if ('.' == pIP[i])
            break;

    strncpy(&pReturn[k], &pIP[i + 1], (j - i));
    k += (j - i);
    pReturn[k++] = '.';
    i--;
    j = i;

    /* C */
    for (; i > 0; i--)
        if ('.' == pIP[i])
            break;

    strncpy(&pReturn[k], &pIP[i + 1], (j - i));
    k += (j - i);
    pReturn[k++] = '.';
    i--;
    j = i;

    /* B */
    for (; i > 0; i--)
        if ('.' == pIP[i])
            break;

    strncpy(&pReturn[k], &pIP[i + 1], (j - i));
    k += (j - i);
    pReturn[k++] = '.';
    i--;
    j = i;

    /* A */
    for (; i > 0; i--);

    strncpy(&pReturn[k], &pIP[i], (j - i) + 1);
    k += (j - i) + 1;

    pReturn[k] = '\0';
}

/* -----------------------------------------------------------------------
 * DnsResolv_ExtractName
 * ----------------------------------------------------------------------- */

int
DnsResolv_ExtractName(
    _In_  PCHAR  pBuffer,
    _Out_ PCHAR  pOutput,
    _In_  USHORT Offset,
    _In_  UCHAR  Limit)
{
    int c = 0, d = 0, i = 0, j = 0, k = 0, l = 0, m = 0;

    i = Offset;

    /* Limit == 0 means "no explicit limit". */
    d = Limit;
    if (0 == Limit)
        d = 255;

    while (d > 0)
    {
        l = pBuffer[i] & 0xFF;
        i++;
        if (!m) c++;

        if (0xC0 == l)
        {
            /* DNS pointer compression */
            if (!m) c++;
            m = 1;
            d += (255 - Limit);
            i = pBuffer[i];
        }
        else
        {
            for (j = 0; j < l; j++)
            {
                pOutput[k] = pBuffer[i];
                i++;
                if (!m) c++;
                k++;
                d--;
            }

            d--;

            if (!pBuffer[i] || (d < 1))
                break;

            pOutput[k++] = '.';
        }
    }

    if (!m)
    {
        if (!Limit)
            c++;
    }

    pOutput[k] = '\0';

    return c;
}

/* -----------------------------------------------------------------------
 * DnsResolv_ExtractIP
 * ----------------------------------------------------------------------- */

int
DnsResolv_ExtractIP(
    _In_  PCHAR  pBuffer,
    _Out_ PCHAR  pOutput,
    _In_  USHORT Offset)
{
    int c = 0, i = 0, v = 0;

    i = Offset;

    v = (UCHAR)pBuffer[i++];
    sprintf(&pOutput[c], "%d.", v);
    c += (int)strlen(&pOutput[c]);

    v = (UCHAR)pBuffer[i++];
    sprintf(&pOutput[c], "%d.", v);
    c += (int)strlen(&pOutput[c]);

    v = (UCHAR)pBuffer[i++];
    sprintf(&pOutput[c], "%d.", v);
    c += (int)strlen(&pOutput[c]);

    v = (UCHAR)pBuffer[i];
    sprintf(&pOutput[c], "%d", v);

    return 4; /* always consumes 4 bytes */
}

/* -----------------------------------------------------------------------
 * DnsResolv_SendRequest
 * ----------------------------------------------------------------------- */

BOOL
DnsResolv_SendRequest(
    _In_    PCHAR                pInBuffer,
    _In_    ULONG                InBufferLength,
    _Out_   PCHAR                pOutBuffer,
    _Inout_ PULONG               pOutBufferLength,
    _In_    PDNS_RESOLVER_CONFIG pConfig)
{
    int j;
    DWORD Attempt;
    USHORT RequestID, ResponseID;
    BOOL bWait;
    SOCKET s;
    SOCKADDR_IN RecAddr, RecAddr2, SendAddr;
    int SendAddrLen = sizeof(SendAddr);
    DWORD TimeoutMs;
    WSADATA WsaData;

    RtlZeroMemory(&RecAddr,  sizeof(SOCKADDR_IN));
    RtlZeroMemory(&RecAddr2, sizeof(SOCKADDR_IN));
    RtlZeroMemory(&SendAddr, sizeof(SOCKADDR_IN));

    /* Pull the request ID from the outgoing buffer. */
    RequestID = ntohs(((PSHORT)&pInBuffer[0])[0]);

    /*
     * Initialise Winsock for this call.  The caller (e.g. the DNS resolver
     * service) may not have called WSAStartup yet; without it ws2_32 returns
     * WSANOTINITIALISED and socket() fails immediately.
     * WSAStartup/WSACleanup are ref-counted, so calling them here is safe
     * even if the calling process already has Winsock initialised.
     */
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
        return FALSE;

    /* Create UDP socket. */
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
    {
        WSACleanup();
        return FALSE;
    }

    /* Apply receive timeout so recvfrom() doesn't block forever. */
    TimeoutMs = pConfig->Timeout * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&TimeoutMs, sizeof(TimeoutMs));

    /* Destination: the configured DNS server. */
    RecAddr.sin_family      = AF_INET;
    RecAddr.sin_port        = htons(pConfig->Port);
    RecAddr.sin_addr.s_addr = inet_addr(pConfig->ServerAddress);

    /*
     * Bind to any local address and an OS-assigned ephemeral port (port 0).
     * Using port 53 here would make queries appear to originate from a DNS
     * server, causing most routers/forwarders to reject or ignore them.
     */
    RecAddr2.sin_family      = AF_INET;
    RecAddr2.sin_port        = 0;
    RecAddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (SOCKADDR *)&RecAddr2, sizeof(RecAddr2));

    j = SOCKET_ERROR;

    for (Attempt = 0; Attempt <= pConfig->Retry; Attempt++)
    {
        /* Send the DNS query. */
        j = sendto(s,
                   pInBuffer,
                   InBufferLength,
                   0,
                   (SOCKADDR *)&RecAddr,
                   sizeof(RecAddr));
        if (j == SOCKET_ERROR)
        {
            closesocket(s);
            WSACleanup();
            return FALSE;
        }

        bWait = TRUE;
        while (bWait)
        {
            /* Wait for a DNS reply. */
            j = recvfrom(s,
                         pOutBuffer,
                         *pOutBufferLength,
                         0,
                         (SOCKADDR *)&SendAddr,
                         &SendAddrLen);
            if (j == SOCKET_ERROR)
            {
                /* On timeout, retry if we have attempts left. */
                if (WSAGetLastError() == WSAETIMEDOUT && Attempt < pConfig->Retry)
                    break;

                closesocket(s);
                WSACleanup();
                return FALSE;
            }

            ResponseID = ntohs(((PSHORT)&pOutBuffer[0])[0]);

            /* Accept only responses that match our request ID. */
            if (ResponseID == RequestID)
                bWait = FALSE;
        }

        /* If we received a valid response, stop retrying. */
        if (!bWait)
            break;
    }

    closesocket(s);
    WSACleanup();

    if (j == SOCKET_ERROR)
        return FALSE;

    /* Update the caller's buffer-length to the actual response size. */
    *pOutBufferLength = j;

    return TRUE;
}

/* -----------------------------------------------------------------------
 * DnsResolv_Lookup
 * ----------------------------------------------------------------------- */

BOOL
DnsResolv_Lookup(
    _In_  PCHAR                pAddr,
    _In_  USHORT               QueryType,
    _Out_ PCHAR                pResult,
    _In_  PDNS_RESOLVER_CONFIG pConfig)
{
    PCHAR  Buffer    = NULL;
    PCHAR  RecBuffer = NULL;
    CHAR   pResolve[256];
    ULONG  BufferLength    = 0;
    ULONG  RecBufferLength = 512;
    int    i = 0, j = 0, k = 0, d = 0;
    BOOL   bOk = FALSE;
    USHORT NumQuestions;
    USHORT Type;
    UCHAR  Header2;
    UCHAR  RCode;
    HANDLE hHeap;

    /* Use a tick-count-based request ID so each call gets a unique value. */
    static USHORT s_RequestID = 1;
    USHORT RequestID = s_RequestID++;

    if ((strlen(pAddr) + 1) > 255)
        return FALSE;

    /* Auto-detect query type when the caller passes 0. */
    if (QueryType == 0)
        Type = DnsResolv_IsValidIP(pAddr) ? DNSRESOLV_TYPE_PTR : DNSRESOLV_TYPE_A;
    else
        Type = QueryType;

    /* For PTR lookups of plain IPv4 addresses, build the in-addr.arpa name. */
    if ((Type == DNSRESOLV_TYPE_PTR) && DnsResolv_IsValidIP(pAddr))
    {
        DnsResolv_ReverseIP(pAddr, pResolve);
        strcat(pResolve, DNSRESOLV_ARPA_SIG);
    }
    else
    {
        strcpy(pResolve, pAddr);
    }

    /* Packet layout: 12-byte header + QNAME wire encoding + 4-byte QTYPE/QCLASS */
    BufferLength = 12 + ((ULONG)strlen(pResolve) + 2) + 4;

    hHeap = GetProcessHeap();

    Buffer = (PCHAR)HeapAlloc(hHeap, 0, BufferLength);
    if (!Buffer)
        goto cleanup;

    RecBuffer = (PCHAR)HeapAlloc(hHeap, 0, RecBufferLength);
    if (!RecBuffer)
        goto cleanup;

    /* --- Build the DNS query packet --- */

    /* Transaction ID */
    ((PSHORT)&Buffer[i])[0] = htons(RequestID);
    i += 2;

    /* Flags byte 1: QR=0 (query), Opcode=0 (standard), RD=Recurse */
    Buffer[i] = 0x00;
    if (pConfig->Recurse)
        Buffer[i] |= 0x01;
    i++;

    /* Flags byte 2: all zero for a query */
    Buffer[i] = 0x00;
    i++;

    /* QDCOUNT = 1 */
    ((PSHORT)&Buffer[i])[0] = htons(1);
    i += 2;

    /* ANCOUNT, NSCOUNT, ARCOUNT = 0 */
    Buffer[i]     = 0x00;
    Buffer[i + 1] = 0x00;
    Buffer[i + 2] = 0x00;
    Buffer[i + 3] = 0x00;
    Buffer[i + 4] = 0x00;
    Buffer[i + 5] = 0x00;
    i += 6;

    /* QNAME: length-prefixed labels */
    j = i;
    i++;
    for (k = 0; k < (int)strlen(pResolve); k++)
    {
        if (pResolve[k] != '.')
        {
            Buffer[i] = pResolve[k];
            i++;
        }
        else
        {
            Buffer[j] = (CHAR)((i - j) - 1);
            j = i;
            i++;
        }
    }
    Buffer[j]  = (CHAR)((i - j) - 1);
    Buffer[i]  = 0x00;
    i++;

    /* QTYPE */
    ((PSHORT)&Buffer[i])[0] = htons(Type);
    i += 2;

    /* QCLASS = IN */
    ((PSHORT)&Buffer[i])[0] = htons(DNSRESOLV_CLASS_IN);

    /* --- Send the query and receive the response --- */
    bOk = DnsResolv_SendRequest(Buffer, BufferLength,
                                RecBuffer, &RecBufferLength,
                                pConfig);
    if (!bOk)
        goto cleanup;

    bOk = FALSE;

    /* Check RCODE */
    Header2 = RecBuffer[3];
    RCode   = Header2 & 0x0F;
    if (RCode != DNSRESOLV_RCODE_NOERROR)
        goto cleanup;

    /* --- Parse the response packet --- */
    NumQuestions = ntohs(((PSHORT)&RecBuffer[4])[0]);

    k = 12;

    /* Skip the Questions section. */
    for (i = 0; i < NumQuestions; i++)
    {
        k += DnsResolv_ExtractName(RecBuffer, pResult, (USHORT)k, 0);
        k += 4; /* QTYPE + QCLASS */
    }

    /* Skip the answer RR name. */
    k += DnsResolv_ExtractName(RecBuffer, pResult, (USHORT)k, 0);

    /* Read the answer TYPE (skip Class + TTL). */
    Type  = ntohs(((PUSHORT)&RecBuffer[k])[0]);
    k    += 8; /* TYPE(2) + CLASS(2) + TTL(4) */

    /* RDLENGTH */
    d  = ntohs(((PUSHORT)&RecBuffer[k])[0]);
    k += 2;

    /* Extract the RDATA. */
    if (DNSRESOLV_TYPE_PTR == Type)
    {
        DnsResolv_ExtractName(RecBuffer, pResult, (USHORT)k, (UCHAR)d);
        bOk = TRUE;
    }
    else if (DNSRESOLV_TYPE_A == Type)
    {
        DnsResolv_ExtractIP(RecBuffer, pResult, (USHORT)k);
        bOk = TRUE;
    }
    else if (d > 0)
    {
        /* For all other record types, attempt to decode as a domain name. */
        DnsResolv_ExtractName(RecBuffer, pResult, (USHORT)k, (UCHAR)d);
        bOk = TRUE;
    }

cleanup:
    if (Buffer)    HeapFree(hHeap, 0, Buffer);
    if (RecBuffer) HeapFree(hHeap, 0, RecBuffer);

    return bOk;
}

/* -----------------------------------------------------------------------
 * Conversion utilities
 * ----------------------------------------------------------------------- */

PCHAR
DnsResolv_OpcodeIDtoOpcodeName(
    _In_ UCHAR Opcode)
{
    switch (Opcode & 0x0F)
    {
        case DNSRESOLV_OPCODE_QUERY:  return (PCHAR)szOpcodeQuery;
        case DNSRESOLV_OPCODE_IQUERY: return (PCHAR)szOpcodeIQuery;
        case DNSRESOLV_OPCODE_STATUS: return (PCHAR)szOpcodeStatus;
        default:                      return (PCHAR)szOpcodeReserved;
    }
}

PCHAR
DnsResolv_RCodeIDtoRCodeName(
    _In_ UCHAR RCode)
{
    switch (RCode & 0x0F)
    {
        case DNSRESOLV_RCODE_NOERROR:  return (PCHAR)szRCodeNOERROR;
        case DNSRESOLV_RCODE_FORMERR:  return (PCHAR)szRCodeFORMERR;
        case DNSRESOLV_RCODE_FAILURE:  return (PCHAR)szRCodeFAILURE;
        case DNSRESOLV_RCODE_NXDOMAIN: return (PCHAR)szRCodeNXDOMAIN;
        case DNSRESOLV_RCODE_NOTIMP:   return (PCHAR)szRCodeNOTIMP;
        case DNSRESOLV_RCODE_REFUSED:  return (PCHAR)szRCodeREFUSED;
        default:                       return (PCHAR)szRCodeReserved;
    }
}

PCHAR
DnsResolv_TypeIDtoTypeName(
    _In_ USHORT TypeID)
{
    switch (TypeID)
    {
        case DNSRESOLV_TYPE_A:     return (PCHAR)szTypeA;
        case DNSRESOLV_TYPE_NS:    return (PCHAR)szTypeNS;
        case DNSRESOLV_TYPE_CNAME: return (PCHAR)szTypeCNAME;
        case DNSRESOLV_TYPE_SOA:   return (PCHAR)szTypeSOA;
        case DNSRESOLV_TYPE_WKS:   return (PCHAR)szTypeWKS;
        case DNSRESOLV_TYPE_PTR:   return (PCHAR)szTypePTR;
        case DNSRESOLV_TYPE_MX:    return (PCHAR)szTypeMX;
        case DNSRESOLV_TYPE_SRV:   return (PCHAR)szTypeSRV;
        case DNSRESOLV_TYPE_ANY:   return (PCHAR)szTypeAny;
        default:                   return (PCHAR)szTypeUnknown;
    }
}

USHORT
DnsResolv_TypeNametoTypeID(
    _In_ PCHAR TypeName)
{
    if (!strncmp(TypeName, szTypeA,     strlen(szTypeA)))     return DNSRESOLV_TYPE_A;
    if (!strncmp(TypeName, szTypeNS,    strlen(szTypeNS)))    return DNSRESOLV_TYPE_NS;
    if (!strncmp(TypeName, szTypeCNAME, strlen(szTypeCNAME))) return DNSRESOLV_TYPE_CNAME;
    if (!strncmp(TypeName, szTypeSOA,   strlen(szTypeSOA)))   return DNSRESOLV_TYPE_SOA;
    if (!strncmp(TypeName, szTypeWKS,   strlen(szTypeWKS)))   return DNSRESOLV_TYPE_WKS;
    if (!strncmp(TypeName, szTypePTR,   strlen(szTypePTR)))   return DNSRESOLV_TYPE_PTR;
    if (!strncmp(TypeName, szTypeMX,    strlen(szTypeMX)))    return DNSRESOLV_TYPE_MX;
    if (!strncmp(TypeName, szTypeSRV,   strlen(szTypeSRV)))   return DNSRESOLV_TYPE_SRV;
    if (!strncmp(TypeName, szTypeAny,   strlen(szTypeAny)))   return DNSRESOLV_TYPE_ANY;

    return 0;
}

PCHAR
DnsResolv_ClassIDtoClassName(
    _In_ USHORT ClassID)
{
    switch (ClassID)
    {
        case DNSRESOLV_CLASS_IN:  return (PCHAR)szClassIN;
        case DNSRESOLV_CLASS_ANY: return (PCHAR)szClassAny;
        default:                  return (PCHAR)szClassUnknown;
    }
}

USHORT
DnsResolv_ClassNametoClassID(
    _In_ PCHAR ClassName)
{
    if (!strncmp(ClassName, szClassIN,  strlen(szClassIN)))  return DNSRESOLV_CLASS_IN;
    if (!strncmp(ClassName, szClassAny, strlen(szClassAny))) return DNSRESOLV_CLASS_ANY;

    return 0;
}
