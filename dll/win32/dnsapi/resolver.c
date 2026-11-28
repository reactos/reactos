/*
 * PROJECT:     ReactOS DNS API
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     A record resolver, wire handling after FNET's DNS client
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#include "precomp.h"
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

#define DNS_PORT                53
#define DNS_MAX_UDP             512
#define DNS_MAX_SERVERS         5
#define DNS_UDP_RETRY_MS        2000
#define DNS_UDP_MAX_TRIES       15
#define DNS_TCP_CONNECT_MS      14000
#define DNS_TCP_WAIT_MS         30000
#define DNS_MAX_POINTER_JUMPS   32
#define DNS_MAX_RESTARTS        16
#define DNS_NAME_BUFFER_LENGTH  256

#define DNS_FLAG_QR             0x8000
#define DNS_FLAG_OPCODE         0x7800
#define DNS_FLAG_TC             0x0200
#define DNS_FLAG_RD             0x0100
#define DNS_FLAG_RA             0x0080
#define DNS_RCODE_MASK          0x000F

#define DNS_RCODE_NOERROR       0
#define DNS_RCODE_NXDOMAIN      3

#define DNS_CLASS_IN            1
#define DNS_RTYPE_A             1
#define DNS_RTYPE_NS            2
#define DNS_RTYPE_CNAME         5
#define DNS_RTYPE_SOA           6

#include <pshpack1.h>
typedef struct _DNS_WIRE_HEADER
{
    USHORT Id;
    USHORT Flags;
    USHORT QdCount;
    USHORT AnCount;
    USHORT NsCount;
    USHORT ArCount;
} DNS_WIRE_HEADER, *PDNS_WIRE_HEADER;
#include <poppack.h>

typedef enum _DNS_REPLY_RESULT
{
    DnsReplyIgnore,
    DnsReplyAddress,
    DnsReplyNxDomain,
    DnsReplyFailed,
    DnsReplyChainedCname,
    DnsReplyTruncated,
    DnsReplyRestart
} DNS_REPLY_RESULT;

typedef struct _DNS_QUERY
{
    UCHAR Packet[DNS_MAX_UDP];
    ULONG Length;
    CHAR Owner[DNS_NAME_BUFFER_LENGTH];
    CHAR Cname[DNS_NAME_BUFFER_LENGTH];
    BOOL HaveCname;
    BOOL UseTcp;
    IP4_ADDRESS Address;
} DNS_QUERY, *PDNS_QUERY;

static
BOOL
DnsIntEncodeName(PCSTR Name, PUCHAR Buffer, ULONG Size, PULONG Length)
{
    ULONG Out = 0;
    PCSTR Label = Name;

    while (*Label)
    {
        PCSTR Dot = strchr(Label, '.');
        ULONG LabelLength = Dot ? (ULONG)(Dot - Label) : (ULONG)strlen(Label);

        if (LabelLength == 0 || LabelLength > 63 || Out + LabelLength + 1 >= Size)
            return FALSE;

        Buffer[Out++] = (UCHAR)LabelLength;
        memcpy(&Buffer[Out], Label, LabelLength);
        Out += LabelLength;
        Label += LabelLength;
        if (*Label == '.')
            Label++;
    }

    if (Out == 0 || Out + 1 > Size)
        return FALSE;

    Buffer[Out++] = 0;
    *Length = Out;
    return Out <= DNS_NAME_BUFFER_LENGTH;
}

static
BOOL
DnsIntBuildQuery(PDNS_QUERY Query, PCSTR Name)
{
    PDNS_WIRE_HEADER Header = (PDNS_WIRE_HEADER)Query->Packet;
    ULONG NameLength;
    PUCHAR Tail;

    if (FAILED(StringCchCopyA(Query->Owner, ARRAYSIZE(Query->Owner), Name)))
        return FALSE;

    if (!DnsIntEncodeName(Name, Query->Packet + sizeof(*Header),
                          sizeof(Query->Packet) - sizeof(*Header) - 4, &NameLength))
    {
        return FALSE;
    }

    Header->Id = (USHORT)(GetTickCount() ^ (GetCurrentProcessId() << 4) ^ (ULONG_PTR)Query);
    Header->Flags = htons(DNS_FLAG_RD);
    Header->QdCount = htons(1);
    Header->AnCount = 0;
    Header->NsCount = 0;
    Header->ArCount = 0;

    Tail = Query->Packet + sizeof(*Header) + NameLength;
    Tail[0] = 0;
    Tail[1] = DNS_RTYPE_A;
    Tail[2] = 0;
    Tail[3] = DNS_CLASS_IN;

    Query->Length = sizeof(*Header) + NameLength + 4;
    Query->HaveCname = FALSE;
    return TRUE;
}

/* Expands a possibly compressed name at *Offset and moves *Offset past it */
static
BOOL
DnsIntReadName(const UCHAR *Message, ULONG Length, PULONG Offset, PSTR Name, ULONG NameSize)
{
    ULONG Position = *Offset;
    ULONG Out = 0;
    ULONG Jumps = 0;
    BOOL Jumped = FALSE;

    for (;;)
    {
        UCHAR LabelLength;

        if (Position >= Length)
            return FALSE;

        LabelLength = Message[Position];
        if ((LabelLength & 0xC0) == 0xC0)
        {
            if (Position + 1 >= Length || ++Jumps > DNS_MAX_POINTER_JUMPS)
                return FALSE;
            if (!Jumped)
                *Offset = Position + 2;
            Jumped = TRUE;
            Position = ((LabelLength & 0x3F) << 8) | Message[Position + 1];
            continue;
        }
        if (LabelLength & 0xC0)
            return FALSE;

        Position++;
        if (LabelLength == 0)
            break;

        if (Position + LabelLength > Length || Out + LabelLength + 2 > NameSize)
            return FALSE;
        if (Out)
            Name[Out++] = '.';
        memcpy(&Name[Out], &Message[Position], LabelLength);
        Out += LabelLength;
        Position += LabelLength;
    }

    if (!Jumped)
        *Offset = Position;
    Name[Out] = ANSI_NULL;
    return TRUE;
}

typedef struct _DNS_RR
{
    CHAR Owner[DNS_NAME_BUFFER_LENGTH];
    USHORT Type;
    USHORT Class;
    USHORT DataLength;
    ULONG DataOffset;
} DNS_RR, *PDNS_RR;

static
BOOL
DnsIntReadRecord(const UCHAR *Message, ULONG Length, PULONG Offset, PDNS_RR Record)
{
    ULONG Position = *Offset;

    if (!DnsIntReadName(Message, Length, &Position, Record->Owner, sizeof(Record->Owner)))
        return FALSE;
    if (Position + 10 > Length)
        return FALSE;

    Record->Type = (Message[Position] << 8) | Message[Position + 1];
    Record->Class = (Message[Position + 2] << 8) | Message[Position + 3];
    Record->DataLength = (Message[Position + 8] << 8) | Message[Position + 9];
    Record->DataOffset = Position + 10;
    if (Record->DataOffset + Record->DataLength > Length)
        return FALSE;

    *Offset = Record->DataOffset + Record->DataLength;
    return TRUE;
}

/* Same decisions as ADNS's adns__procdgram for an A query */
static
DNS_REPLY_RESULT
DnsIntProcessReply(PDNS_QUERY Query, const UCHAR *Reply, ULONG Length)
{
    const DNS_WIRE_HEADER *Header = (const DNS_WIRE_HEADER *)Reply;
    PCSTR Expected = Query->Owner;
    USHORT Flags, AnCount, NsCount, Rcode;
    ULONG Offset, i, Wanted = 0;
    BOOL FoundSoa = FALSE, FoundNs = FALSE, CnameHere = FALSE;
    DNS_RR Record;

    if (Length < Query->Length)
        return DnsReplyIgnore;
    if (Header->Id != ((PDNS_WIRE_HEADER)Query->Packet)->Id || ntohs(Header->QdCount) != 1)
        return DnsReplyIgnore;

    Flags = ntohs(Header->Flags);
    if (!(Flags & DNS_FLAG_QR) || (Flags & DNS_FLAG_OPCODE))
        return DnsReplyIgnore;

    /* ADNS compares the whole question as sent */
    if (memcmp(Reply + sizeof(*Header), Query->Packet + sizeof(*Header), Query->Length - sizeof(*Header)))
        return DnsReplyIgnore;

    Rcode = Flags & DNS_RCODE_MASK;
    if (Rcode != DNS_RCODE_NOERROR && Rcode != DNS_RCODE_NXDOMAIN)
        return DnsReplyFailed;

    AnCount = ntohs(Header->AnCount);
    NsCount = ntohs(Header->NsCount);
    Offset = Query->Length;

    for (i = 0; i < AnCount; i++)
    {
        if (!DnsIntReadRecord(Reply, Length, &Offset, &Record))
            goto Truncated;

        if (Record.Class != DNS_CLASS_IN || _stricmp(Record.Owner, Expected))
            continue;

        if (Record.Type == DNS_RTYPE_CNAME)
        {
            ULONG Data = Record.DataOffset;

            if (Query->HaveCname)
                return DnsReplyChainedCname;
            if (Wanted)
                continue;
            if (!DnsIntReadName(Reply, Length, &Data, Query->Cname, sizeof(Query->Cname)))
                goto Truncated;
            Query->HaveCname = TRUE;
            CnameHere = TRUE;
            Expected = Query->Cname;
        }
        else if (Record.Type == DNS_RTYPE_A)
        {
            if (Record.DataLength != sizeof(IP4_ADDRESS))
                return DnsReplyFailed;
            if (!Wanted)
                memcpy(&Query->Address, &Reply[Record.DataOffset], sizeof(IP4_ADDRESS));
            Wanted++;
        }
    }

    if (Flags & DNS_FLAG_TC)
        goto Truncated;

    if (Wanted)
        return DnsReplyAddress;

    for (i = 0; i < NsCount; i++)
    {
        if (!DnsIntReadRecord(Reply, Length, &Offset, &Record))
            goto Truncated;
        if (Record.Class != DNS_CLASS_IN)
            continue;
        if (Record.Type == DNS_RTYPE_SOA)
        {
            FoundSoa = TRUE;
            break;
        }
        if (Record.Type == DNS_RTYPE_NS)
            FoundNs = TRUE;
    }

    if (Rcode == DNS_RCODE_NXDOMAIN)
        return DnsReplyNxDomain;

    /* NODATA */
    if (FoundSoa || !FoundNs)
        return DnsReplyFailed;

    /* A referral after a CNAME: ask again for the CNAME target */
    if (CnameHere)
        return DnsReplyRestart;

    return DnsReplyFailed;

Truncated:
    if (!(Flags & DNS_FLAG_TC))
        return DnsReplyFailed;
    return DnsReplyTruncated;
}

static
BOOL
DnsIntWaitSocket(SOCKET Socket, BOOL Write, DWORD TimeoutMs)
{
    fd_set Set, Error;
    struct timeval Timeout;

    FD_ZERO(&Set);
    FD_SET(Socket, &Set);
    FD_ZERO(&Error);
    FD_SET(Socket, &Error);
    Timeout.tv_sec = TimeoutMs / 1000;
    Timeout.tv_usec = (TimeoutMs % 1000) * 1000;

    if (select(0, Write ? NULL : &Set, Write ? &Set : NULL, &Error, &Timeout) <= 0)
        return FALSE;

    return FD_ISSET(Socket, &Set);
}

static
BOOL
DnsIntReceiveAll(SOCKET Socket, PUCHAR Buffer, ULONG Length, DWORD Deadline)
{
    ULONG Received = 0;

    while (Received < Length)
    {
        LONG Left = (LONG)(Deadline - GetTickCount());
        int Result;

        if (Left <= 0 || !DnsIntWaitSocket(Socket, FALSE, (DWORD)Left))
            return FALSE;

        Result = recv(Socket, (char *)Buffer + Received, Length - Received, 0);
        if (Result <= 0)
            return FALSE;
        Received += Result;
    }

    return TRUE;
}

static
DNS_REPLY_RESULT
DnsIntQueryTcp(PDNS_QUERY Query, const IP4_ADDRESS *Servers, ULONG ServerCount, PULONG ServerIndex)
{
    ULONG Attempt;

    for (Attempt = 0; Attempt < ServerCount; Attempt++)
    {
        SOCKET Socket;
        SOCKADDR_IN Server = {0};
        UCHAR Prefix[2];
        PUCHAR Reply;
        ULONG Length;
        u_long NonBlocking = 1;
        DNS_REPLY_RESULT Result = DnsReplyIgnore;
        DWORD Deadline;

        Server.sin_family = AF_INET;
        Server.sin_port = htons(DNS_PORT);
        Server.sin_addr.s_addr = Servers[*ServerIndex];

        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (Socket == INVALID_SOCKET)
            return DnsReplyFailed;
        ioctlsocket(Socket, FIONBIO, &NonBlocking);

        if (connect(Socket, (struct sockaddr *)&Server, sizeof(Server)) == 0 ||
            (WSAGetLastError() == WSAEWOULDBLOCK && DnsIntWaitSocket(Socket, TRUE, DNS_TCP_CONNECT_MS)))
        {
            Prefix[0] = (UCHAR)(Query->Length >> 8);
            Prefix[1] = (UCHAR)Query->Length;
            NonBlocking = 0;
            ioctlsocket(Socket, FIONBIO, &NonBlocking);

            Deadline = GetTickCount() + DNS_TCP_WAIT_MS;
            if (send(Socket, (char *)Prefix, sizeof(Prefix), 0) == sizeof(Prefix) &&
                send(Socket, (char *)Query->Packet, Query->Length, 0) == (int)Query->Length &&
                DnsIntReceiveAll(Socket, Prefix, sizeof(Prefix), Deadline))
            {
                Length = (Prefix[0] << 8) | Prefix[1];
                Reply = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length ? Length : 1);
                if (Reply)
                {
                    if (DnsIntReceiveAll(Socket, Reply, Length, Deadline))
                        Result = DnsIntProcessReply(Query, Reply, Length);
                    RtlFreeHeap(RtlGetProcessHeap(), 0, Reply);
                }
            }
        }
        closesocket(Socket);

        if (Result == DnsReplyTruncated)
            return DnsReplyFailed;
        if (Result != DnsReplyIgnore)
            return Result;

        *ServerIndex = (*ServerIndex + 1) % ServerCount;
    }

    return DnsReplyFailed;
}

static
DNS_REPLY_RESULT
DnsIntQueryUdp(SOCKET Socket, PDNS_QUERY Query, const IP4_ADDRESS *Servers, ULONG ServerCount)
{
    UCHAR Reply[DNS_MAX_UDP];
    ULONG Tries, Next = 0, Sent = 0;

    for (Tries = 0; Tries < DNS_UDP_MAX_TRIES; Tries++)
    {
        SOCKADDR_IN Server = {0};
        DWORD Deadline;

        Server.sin_family = AF_INET;
        Server.sin_port = htons(DNS_PORT);
        Server.sin_addr.s_addr = Servers[Next];
        sendto(Socket, (char *)Query->Packet, Query->Length, 0, (struct sockaddr *)&Server, sizeof(Server));
        Sent |= 1 << Next;
        Next = (Next + 1) % ServerCount;

        Deadline = GetTickCount() + DNS_UDP_RETRY_MS;
        for (;;)
        {
            LONG Left = (LONG)(Deadline - GetTickCount());
            SOCKADDR_IN From;
            int FromLength = sizeof(From);
            int Received;
            ULONG i;
            DNS_REPLY_RESULT Result;

            if (Left <= 0 || !DnsIntWaitSocket(Socket, FALSE, (DWORD)Left))
                break;

            Received = recvfrom(Socket, (char *)Reply, sizeof(Reply), 0, (struct sockaddr *)&From, &FromLength);
            /* An ICMP error from one server must not end the query; the timeout handles it */
            if (Received <= 0)
                continue;

            for (i = 0; i < ServerCount; i++)
            {
                if (Servers[i] == From.sin_addr.s_addr && From.sin_port == htons(DNS_PORT))
                    break;
            }
            if (i == ServerCount || !(Sent & (1 << i)))
                continue;

            Result = DnsIntProcessReply(Query, Reply, (ULONG)Received);
            if (Result != DnsReplyIgnore)
                return Result;
        }
    }

    return DnsReplyFailed;
}

/* One name, following truncation and CNAME referrals the way ADNS restarts a query */
static
DNS_REPLY_RESULT
DnsIntQueryName(SOCKET Socket, PCSTR Name, const IP4_ADDRESS *Servers, ULONG ServerCount, PDNS_QUERY Query)
{
    ULONG TcpServer = 0, Restarts;
    BOOL UseTcp = FALSE;
    DNS_REPLY_RESULT Result;

    if (!DnsIntBuildQuery(Query, Name))
        return DnsReplyFailed;

    for (Restarts = 0; Restarts < DNS_MAX_RESTARTS; Restarts++)
    {
        if (UseTcp)
            Result = DnsIntQueryTcp(Query, Servers, ServerCount, &TcpServer);
        else
            Result = DnsIntQueryUdp(Socket, Query, Servers, ServerCount);

        if (Result == DnsReplyTruncated)
            UseTcp = TRUE;
        else if (Result != DnsReplyRestart)
            return Result;

        if (Query->HaveCname)
        {
            CHAR Target[DNS_NAME_BUFFER_LENGTH];

            StringCchCopyA(Target, ARRAYSIZE(Target), Query->Cname);
            if (!DnsIntBuildQuery(Query, Target))
                return DnsReplyFailed;
        }
    }

    return DnsReplyFailed;
}

/* DNS_INFO_NO_RECORDS with Cname set means the reply chained CNAMEs, the caller requeries the target */
DNS_STATUS
DnsIntResolveA(
    _In_ PCSTR Name,
    _In_ BOOL Search,
    _In_opt_ PCSTR SearchDomain,
    _In_reads_(ServerCount) const IP4_ADDRESS *Servers,
    _In_ ULONG ServerCount,
    _Out_ IP4_ADDRESS *Address,
    _Out_writes_(CnameSize) PSTR Cname,
    _In_ ULONG CnameSize)
{
    CHAR Absolute[DNS_NAME_BUFFER_LENGTH];
    CHAR Candidates[2][DNS_NAME_BUFFER_LENGTH];
    ULONG CandidateCount = 0, Dots = 0, i;
    PCSTR Dot;
    size_t Length;
    WSADATA WsaData;
    SOCKET Socket;
    PDNS_QUERY Query;
    DNS_STATUS Status = ERROR_FILE_NOT_FOUND;

    *Address = 0;
    Cname[0] = ANSI_NULL;

    if (ServerCount == 0 || FAILED(StringCchCopyA(Absolute, ARRAYSIZE(Absolute), Name)))
        return ERROR_FILE_NOT_FOUND;

    Length = strlen(Absolute);
    if (Length && Absolute[Length - 1] == '.')
    {
        Absolute[Length - 1] = ANSI_NULL;
        Search = FALSE;
    }
    if (!Absolute[0])
        return ERROR_FILE_NOT_FOUND;

    for (Dot = Absolute; (Dot = strchr(Dot, '.')); Dot++)
        Dots++;

    /* ADNS order: with at least ndots (1) dots try the name as given first */
    if (!Search || !SearchDomain || !SearchDomain[0])
    {
        StringCchCopyA(Candidates[CandidateCount++], DNS_NAME_BUFFER_LENGTH, Absolute);
    }
    else if (Dots >= 1)
    {
        StringCchCopyA(Candidates[CandidateCount++], DNS_NAME_BUFFER_LENGTH, Absolute);
        StringCchPrintfA(Candidates[CandidateCount++], DNS_NAME_BUFFER_LENGTH, "%s.%s", Absolute, SearchDomain);
    }
    else
    {
        StringCchPrintfA(Candidates[CandidateCount++], DNS_NAME_BUFFER_LENGTH, "%s.%s", Absolute, SearchDomain);
        StringCchCopyA(Candidates[CandidateCount++], DNS_NAME_BUFFER_LENGTH, Absolute);
    }

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
        return ERROR_OUTOFMEMORY;

    Query = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*Query));
    Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!Query || Socket == INVALID_SOCKET)
    {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    for (i = 0; i < CandidateCount; i++)
    {
        DNS_REPLY_RESULT Result = DnsIntQueryName(Socket, Candidates[i], Servers, ServerCount, Query);

        if (Result == DnsReplyNxDomain)
            continue;

        if (Result == DnsReplyAddress)
        {
            *Address = Query->Address;
            Status = ERROR_SUCCESS;
        }
        else if (Result == DnsReplyChainedCname)
        {
            StringCchCopyA(Cname, CnameSize, Query->Cname);
            Status = DNS_INFO_NO_RECORDS;
        }
        break;
    }

Cleanup:
    if (Socket != INVALID_SOCKET)
        closesocket(Socket);
    if (Query)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Query);
    WSACleanup();
    return Status;
}
