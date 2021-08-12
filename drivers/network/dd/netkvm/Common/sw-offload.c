/*
 * This file contains SW Implementation of checksum computation for IP,TCP,UDP
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "ndis56common.h"

#ifdef WPP_EVENT_TRACING
#include "sw-offload.tmh"
#endif
#include <sal.h>

// till IP header size is 8 bit
#define MAX_SUPPORTED_IPV6_HEADERS  (256 - 4)

typedef ULONG IPV6_ADDRESS[4];

// IPv6 Header RFC 2460 (40 bytes)
typedef struct _tagIPv6Header {
    UCHAR       ip6_ver_tc;            // traffic class(low nibble), version (high nibble)
    UCHAR       ip6_tc_fl;             // traffic class(high nibble), flow label
    USHORT      ip6_fl;                // flow label, the rest
    USHORT      ip6_payload_len;       // length of following headers and payload
    UCHAR       ip6_next_header;       // next header type
    UCHAR       ip6_hoplimit;          // hop limit
    IPV6_ADDRESS ip6_src_address;    //
    IPV6_ADDRESS ip6_dst_address;    //
} IPv6Header;

typedef union
{
    IPv6Header v6;
    IPv4Header v4;
} IPHeader;

// IPv6 Header RFC 2460 (n*8 bytes)
typedef struct _tagIPv6ExtHeader {
    UCHAR       ip6ext_next_header;     // next header type
    UCHAR       ip6ext_hdr_len;         // length of this header in 8 bytes unit, not including first 8 bytes
    USHORT      options;                //
} IPv6ExtHeader;

// IP Pseudo Header RFC 768
typedef struct _tagIPv4PseudoHeader {
    ULONG       ipph_src;               // Source address
    ULONG       ipph_dest;              // Destination address
    UCHAR       ipph_zero;              // 0
    UCHAR       ipph_protocol;          // TCP/UDP
    USHORT      ipph_length;            // TCP/UDP length
}tIPv4PseudoHeader;

// IPv6 Pseudo Header RFC 2460
typedef struct _tagIPv6PseudoHeader {
    IPV6_ADDRESS ipph_src;              // Source address
    IPV6_ADDRESS ipph_dest;             // Destination address
    ULONG        ipph_length;               // TCP/UDP length
    UCHAR        z1;                // 0
    UCHAR        z2;                // 0
    UCHAR        z3;                // 0
    UCHAR        ipph_protocol;             // TCP/UDP
}tIPv6PseudoHeader;


#define PROTOCOL_TCP                    6
#define PROTOCOL_UDP                    17


#define IP_HEADER_LENGTH(pHeader)  (((pHeader)->ip_verlen & 0x0F) << 2)
#define TCP_HEADER_LENGTH(pHeader) ((pHeader->tcp_flags & 0xF0) >> 2)



static __inline USHORT CheckSumCalculator(ULONG val, PVOID buffer, ULONG len)
{
    PUSHORT pus = (PUSHORT)buffer;
    ULONG count = len >> 1;
    while (count--) val += *pus++;
    if (len & 1) val += (USHORT)*(PUCHAR)pus;
    val = (((val >> 16) | (val << 16)) + val) >> 16;
    return (USHORT)~val;
}


/******************************************
    IP header checksum calculator
*******************************************/
static __inline VOID CalculateIpChecksum(IPv4Header *pIpHeader)
{
    pIpHeader->ip_xsum = 0;
    pIpHeader->ip_xsum = CheckSumCalculator(0, pIpHeader, IP_HEADER_LENGTH(pIpHeader));
}

static __inline tTcpIpPacketParsingResult
ProcessTCPHeader(tTcpIpPacketParsingResult _res, PVOID pIpHeader, ULONG len, USHORT ipHeaderSize)
{
    ULONG tcpipDataAt;
    tTcpIpPacketParsingResult res = _res;
    tcpipDataAt = ipHeaderSize + sizeof(TCPHeader);
    res.xxpStatus = ppresXxpIncomplete;
    res.TcpUdp = ppresIsTCP;

    if (len >= tcpipDataAt)
    {
        TCPHeader *pTcpHeader = (TCPHeader *)RtlOffsetToPointer(pIpHeader, ipHeaderSize);
        res.xxpStatus = ppresXxpKnown;
        tcpipDataAt = ipHeaderSize + TCP_HEADER_LENGTH(pTcpHeader);
        res.XxpIpHeaderSize = tcpipDataAt;
    }
    else
    {
        DPrintf(2, ("tcp: %d < min headers %d", len, tcpipDataAt));
    }
    return res;
}

static __inline tTcpIpPacketParsingResult
ProcessUDPHeader(tTcpIpPacketParsingResult _res, PVOID pIpHeader, ULONG len, USHORT ipHeaderSize)
{
    tTcpIpPacketParsingResult res = _res;
    ULONG udpDataStart = ipHeaderSize + sizeof(UDPHeader);
    res.xxpStatus = ppresXxpIncomplete;
    res.TcpUdp = ppresIsUDP;
    res.XxpIpHeaderSize = udpDataStart;
    if (len >= udpDataStart)
    {
        UDPHeader *pUdpHeader = (UDPHeader *)RtlOffsetToPointer(pIpHeader, ipHeaderSize);
        USHORT datagramLength = swap_short(pUdpHeader->udp_length);
        res.xxpStatus = ppresXxpKnown;
        // may be full or not, but the datagram length is known
        DPrintf(2, ("udp: len %d, datagramLength %d", len, datagramLength));
    }
    return res;
}

static __inline tTcpIpPacketParsingResult
QualifyIpPacket(IPHeader *pIpHeader, ULONG len)
{
    tTcpIpPacketParsingResult res;
    UCHAR  ver_len = pIpHeader->v4.ip_verlen;
    UCHAR  ip_version = (ver_len & 0xF0) >> 4;
    USHORT ipHeaderSize = 0;
    USHORT fullLength = 0;
    res.value = 0;

    if (ip_version == 4)
    {
        ipHeaderSize = (ver_len & 0xF) << 2;
        fullLength = swap_short(pIpHeader->v4.ip_length);
        DPrintf(3, ("ip_version %d, ipHeaderSize %d, protocol %d, iplen %d",
            ip_version, ipHeaderSize, pIpHeader->v4.ip_protocol, fullLength));
        res.ipStatus = (ipHeaderSize >= sizeof(IPv4Header)) ? ppresIPV4 : ppresNotIP;
        if (len < ipHeaderSize) res.ipCheckSum = ppresIPTooShort;
        if (fullLength) {}
        else
        {
            DPrintf(2, ("ip v.%d, iplen %d", ip_version, fullLength));
        }
    }
    else if (ip_version == 6)
    {
        UCHAR nextHeader = pIpHeader->v6.ip6_next_header;
        BOOLEAN bParsingDone = FALSE;
        ipHeaderSize = sizeof(pIpHeader->v6);
        res.ipStatus = ppresIPV6;
        res.ipCheckSum = ppresCSOK;
        fullLength = swap_short(pIpHeader->v6.ip6_payload_len);
        fullLength += ipHeaderSize;
        while (nextHeader != 59)
        {
            IPv6ExtHeader *pExt;
            switch (nextHeader)
            {
                case PROTOCOL_TCP:
                    bParsingDone = TRUE;
                    res.xxpStatus = ppresXxpKnown;
                    res.TcpUdp = ppresIsTCP;
                    res.xxpFull = len >= fullLength ? 1 : 0;
                    res = ProcessTCPHeader(res, pIpHeader, len, ipHeaderSize);
                    break;
                case PROTOCOL_UDP:
                    bParsingDone = TRUE;
                    res.xxpStatus = ppresXxpKnown;
                    res.TcpUdp = ppresIsUDP;
                    res.xxpFull = len >= fullLength ? 1 : 0;
                    res = ProcessUDPHeader(res, pIpHeader, len, ipHeaderSize);
                    break;
                    //existing extended headers
                case 0:
                    __fallthrough;
                case 60:
                    __fallthrough;
                case 43:
                    __fallthrough;
                case 44:
                    __fallthrough;
                case 51:
                    __fallthrough;
                case 50:
                    __fallthrough;
                case 135:
                    if (len >= ((ULONG)ipHeaderSize + 8))
                    {
                        pExt = (IPv6ExtHeader *)((PUCHAR)pIpHeader + ipHeaderSize);
                        nextHeader = pExt->ip6ext_next_header;
                        ipHeaderSize += 8;
                        ipHeaderSize += pExt->ip6ext_hdr_len * 8;
                    }
                    else
                    {
                        DPrintf(0, ("[%s] ERROR: Break in the middle of ext. headers(len %d, hdr > %d)", __FUNCTION__, len, ipHeaderSize));
                        res.ipStatus = ppresNotIP;
                        bParsingDone = TRUE;
                    }
                    break;
                    //any other protocol
                default:
                    res.xxpStatus = ppresXxpOther;
                    bParsingDone = TRUE;
                    break;
            }
            if (bParsingDone)
                break;
        }
        if (ipHeaderSize <= MAX_SUPPORTED_IPV6_HEADERS)
        {
            DPrintf(3, ("ip_version %d, ipHeaderSize %d, protocol %d, iplen %d",
                ip_version, ipHeaderSize, nextHeader, fullLength));
            res.ipHeaderSize = ipHeaderSize;
        }
        else
        {
            DPrintf(0, ("[%s] ERROR: IP chain is too large (%d)", __FUNCTION__, ipHeaderSize));
            res.ipStatus = ppresNotIP;
        }
    }

    if (res.ipStatus == ppresIPV4)
    {
        res.ipHeaderSize = ipHeaderSize;
        res.xxpFull = len >= fullLength ? 1 : 0;
        // bit "more fragments" or fragment offset mean the packet is fragmented
        res.IsFragment = (pIpHeader->v4.ip_offset & ~0xC0) != 0;
        switch (pIpHeader->v4.ip_protocol)
        {
            case PROTOCOL_TCP:
            {
                res = ProcessTCPHeader(res, pIpHeader, len, ipHeaderSize);
            }
            break;
        case PROTOCOL_UDP:
            {
                res = ProcessUDPHeader(res, pIpHeader, len, ipHeaderSize);
            }
            break;
        default:
            res.xxpStatus = ppresXxpOther;
            break;
        }
    }
    return res;
}

static __inline USHORT GetXxpHeaderAndPayloadLen(IPHeader *pIpHeader, tTcpIpPacketParsingResult res)
{
    if (res.ipStatus == ppresIPV4)
    {
        USHORT headerLength = IP_HEADER_LENGTH(&pIpHeader->v4);
        USHORT len = swap_short(pIpHeader->v4.ip_length);
        return len - headerLength;
    }
    if (res.ipStatus == ppresIPV6)
    {
        USHORT fullLength = swap_short(pIpHeader->v6.ip6_payload_len);
        return fullLength + sizeof(pIpHeader->v6) - (USHORT)res.ipHeaderSize;
    }
    return 0;
}

static __inline USHORT CalculateIpv4PseudoHeaderChecksum(IPv4Header *pIpHeader, USHORT headerAndPayloadLen)
{
    tIPv4PseudoHeader ipph;
    USHORT checksum;
    ipph.ipph_src  = pIpHeader->ip_src;
    ipph.ipph_dest = pIpHeader->ip_dest;
    ipph.ipph_zero = 0;
    ipph.ipph_protocol = pIpHeader->ip_protocol;
    ipph.ipph_length = swap_short(headerAndPayloadLen);
    checksum = CheckSumCalculator(0, &ipph, sizeof(ipph));
    return ~checksum;
}


static __inline USHORT CalculateIpv6PseudoHeaderChecksum(IPv6Header *pIpHeader, USHORT headerAndPayloadLen)
{
    tIPv6PseudoHeader ipph;
    USHORT checksum;
    ipph.ipph_src[0]  = pIpHeader->ip6_src_address[0];
    ipph.ipph_src[1]  = pIpHeader->ip6_src_address[1];
    ipph.ipph_src[2]  = pIpHeader->ip6_src_address[2];
    ipph.ipph_src[3]  = pIpHeader->ip6_src_address[3];
    ipph.ipph_dest[0] = pIpHeader->ip6_dst_address[0];
    ipph.ipph_dest[1] = pIpHeader->ip6_dst_address[1];
    ipph.ipph_dest[2] = pIpHeader->ip6_dst_address[2];
    ipph.ipph_dest[3] = pIpHeader->ip6_dst_address[3];
    ipph.z1 = ipph.z2 = ipph.z3 = 0;
    ipph.ipph_protocol = pIpHeader->ip6_next_header;
    ipph.ipph_length = swap_short(headerAndPayloadLen);
    checksum = CheckSumCalculator(0, &ipph, sizeof(ipph));
    return ~checksum;
}

static __inline USHORT CalculateIpPseudoHeaderChecksum(IPHeader *pIpHeader,
                                                       tTcpIpPacketParsingResult res,
                                                       USHORT headerAndPayloadLen)
{
    if (res.ipStatus == ppresIPV4)
        return CalculateIpv4PseudoHeaderChecksum(&pIpHeader->v4, headerAndPayloadLen);
    if (res.ipStatus == ppresIPV6)
        return CalculateIpv6PseudoHeaderChecksum(&pIpHeader->v6, headerAndPayloadLen);
    return 0;
}

static __inline BOOLEAN
CompareNetCheckSumOnEndSystem(USHORT computedChecksum, USHORT arrivedChecksum)
{
    //According to RFC 1624 sec. 3
    //Checksum verification mechanism should treat 0xFFFF
    //checksum value from received packet as 0x0000
    if(arrivedChecksum == 0xFFFF)
        arrivedChecksum = 0;

    return computedChecksum == arrivedChecksum;
}

/******************************************
  Calculates IP header checksum calculator
  it can be already calculated
  the header must be complete!
*******************************************/
static __inline tTcpIpPacketParsingResult
VerifyIpChecksum(
    IPv4Header *pIpHeader,
    tTcpIpPacketParsingResult known,
    BOOLEAN bFix)
{
    tTcpIpPacketParsingResult res = known;
    if (res.ipCheckSum != ppresIPTooShort)
    {
        USHORT saved = pIpHeader->ip_xsum;
        CalculateIpChecksum(pIpHeader);
        res.ipCheckSum = CompareNetCheckSumOnEndSystem(pIpHeader->ip_xsum, saved) ? ppresCSOK : ppresCSBad;
        if (!bFix)
            pIpHeader->ip_xsum = saved;
        else
            res.fixedIpCS = res.ipCheckSum == ppresCSBad;
    }
    return res;
}

/*********************************************
Calculates UDP checksum, assuming the checksum field
is initialized with pseudoheader checksum
**********************************************/
static VOID CalculateUdpChecksumGivenPseudoCS(UDPHeader *pUdpHeader, ULONG udpLength)
{
    pUdpHeader->udp_xsum = CheckSumCalculator(0, pUdpHeader, udpLength);
}

/*********************************************
Calculates TCP checksum, assuming the checksum field
is initialized with pseudoheader checksum
**********************************************/
static __inline VOID CalculateTcpChecksumGivenPseudoCS(TCPHeader *pTcpHeader, ULONG tcpLength)
{
    pTcpHeader->tcp_xsum = CheckSumCalculator(0, pTcpHeader, tcpLength);
}

/************************************************
Checks (and fix if required) the TCP checksum
sets flags in result structure according to verification
TcpPseudoOK if valid pseudo CS was found
TcpOK if valid TCP checksum was found
************************************************/
static __inline tTcpIpPacketParsingResult
VerifyTcpChecksum( IPHeader *pIpHeader, ULONG len, tTcpIpPacketParsingResult known, ULONG whatToFix)
{
    USHORT  phcs;
    tTcpIpPacketParsingResult res = known;
    TCPHeader *pTcpHeader = (TCPHeader *)RtlOffsetToPointer(pIpHeader, res.ipHeaderSize);
    USHORT saved = pTcpHeader->tcp_xsum;
    USHORT xxpHeaderAndPayloadLen = GetXxpHeaderAndPayloadLen(pIpHeader, res);
    if (len >= res.ipHeaderSize)
    {
        phcs = CalculateIpPseudoHeaderChecksum(pIpHeader, res, xxpHeaderAndPayloadLen);
        res.xxpCheckSum = CompareNetCheckSumOnEndSystem(phcs, saved) ?  ppresPCSOK : ppresCSBad;
        if (res.xxpCheckSum != ppresPCSOK || whatToFix)
        {
            if (whatToFix & pcrFixPHChecksum)
            {
                if (len >= (ULONG)(res.ipHeaderSize + sizeof(*pTcpHeader)))
                {
                    pTcpHeader->tcp_xsum = phcs;
                    res.fixedXxpCS = res.xxpCheckSum != ppresPCSOK;
                }
                else
                    res.xxpStatus = ppresXxpIncomplete;
            }
            else if (res.xxpFull)
            {
                //USHORT ipFullLength = swap_short(pIpHeader->v4.ip_length);
                pTcpHeader->tcp_xsum = phcs;
                CalculateTcpChecksumGivenPseudoCS(pTcpHeader, xxpHeaderAndPayloadLen);
                if (CompareNetCheckSumOnEndSystem(pTcpHeader->tcp_xsum, saved))
                    res.xxpCheckSum = ppresCSOK;

                if (!(whatToFix & pcrFixXxpChecksum))
                    pTcpHeader->tcp_xsum = saved;
                else
                    res.fixedXxpCS =
                        res.xxpCheckSum == ppresCSBad || res.xxpCheckSum == ppresPCSOK;
            }
            else if (whatToFix)
            {
                res.xxpStatus = ppresXxpIncomplete;
            }
        }
        else if (res.xxpFull)
        {
            // we have correct PHCS and we do not need to fix anything
            // there is a very small chance that it is also good TCP CS
            // in such rare case we give a priority to TCP CS
            CalculateTcpChecksumGivenPseudoCS(pTcpHeader, xxpHeaderAndPayloadLen);
            if (CompareNetCheckSumOnEndSystem(pTcpHeader->tcp_xsum, saved))
                res.xxpCheckSum = ppresCSOK;
            pTcpHeader->tcp_xsum = saved;
        }
    }
    else
        res.ipCheckSum = ppresIPTooShort;
    return res;
}

/************************************************
Checks (and fix if required) the UDP checksum
sets flags in result structure according to verification
UdpPseudoOK if valid pseudo CS was found
UdpOK if valid UDP checksum was found
************************************************/
static __inline tTcpIpPacketParsingResult
VerifyUdpChecksum( IPHeader *pIpHeader, ULONG len, tTcpIpPacketParsingResult known, ULONG whatToFix)
{
    USHORT  phcs;
    tTcpIpPacketParsingResult res = known;
    UDPHeader *pUdpHeader = (UDPHeader *)RtlOffsetToPointer(pIpHeader, res.ipHeaderSize);
    USHORT saved = pUdpHeader->udp_xsum;
    USHORT xxpHeaderAndPayloadLen = GetXxpHeaderAndPayloadLen(pIpHeader, res);
    if (len >= res.ipHeaderSize)
    {
        phcs = CalculateIpPseudoHeaderChecksum(pIpHeader, res, xxpHeaderAndPayloadLen);
        res.xxpCheckSum = CompareNetCheckSumOnEndSystem(phcs, saved) ?  ppresPCSOK : ppresCSBad;
        if (whatToFix & pcrFixPHChecksum)
        {
            if (len >= (ULONG)(res.ipHeaderSize + sizeof(UDPHeader)))
            {
                pUdpHeader->udp_xsum = phcs;
                res.fixedXxpCS = res.xxpCheckSum != ppresPCSOK;
            }
            else
                res.xxpStatus = ppresXxpIncomplete;
        }
        else if (res.xxpCheckSum != ppresPCSOK || (whatToFix & pcrFixXxpChecksum))
        {
            if (res.xxpFull)
            {
                pUdpHeader->udp_xsum = phcs;
                CalculateUdpChecksumGivenPseudoCS(pUdpHeader, xxpHeaderAndPayloadLen);
                if (CompareNetCheckSumOnEndSystem(pUdpHeader->udp_xsum, saved))
                    res.xxpCheckSum = ppresCSOK;

                if (!(whatToFix & pcrFixXxpChecksum))
                    pUdpHeader->udp_xsum = saved;
                else
                    res.fixedXxpCS =
                        res.xxpCheckSum == ppresCSBad || res.xxpCheckSum == ppresPCSOK;
            }
            else
                res.xxpCheckSum = ppresXxpIncomplete;
        }
        else if (res.xxpFull)
        {
            // we have correct PHCS and we do not need to fix anything
            // there is a very small chance that it is also good UDP CS
            // in such rare case we give a priority to UDP CS
            CalculateUdpChecksumGivenPseudoCS(pUdpHeader, xxpHeaderAndPayloadLen);
            if (CompareNetCheckSumOnEndSystem(pUdpHeader->udp_xsum, saved))
                res.xxpCheckSum = ppresCSOK;
            pUdpHeader->udp_xsum = saved;
        }
    }
    else
        res.ipCheckSum = ppresIPTooShort;

    return res;
}

static LPCSTR __inline GetPacketCase(tTcpIpPacketParsingResult res)
{
    static const char *const IPCaseName[4] = { "not tested", "Non-IP", "IPv4", "IPv6" };
    if (res.xxpStatus == ppresXxpKnown) return res.TcpUdp == ppresIsTCP ?
        (res.ipStatus == ppresIPV4 ? "TCPv4" : "TCPv6") :
        (res.ipStatus == ppresIPV4 ? "UDPv4" : "UDPv6");
    if (res.xxpStatus == ppresXxpIncomplete) return res.TcpUdp == ppresIsTCP ? "Incomplete TCP" : "Incomplete UDP";
    if (res.xxpStatus == ppresXxpOther) return "IP";
    return  IPCaseName[res.ipStatus];
}

static LPCSTR __inline GetIPCSCase(tTcpIpPacketParsingResult res)
{
    static const char *const CSCaseName[4] = { "not tested", "(too short)", "OK", "Bad" };
    return CSCaseName[res.ipCheckSum];
}

static LPCSTR __inline GetXxpCSCase(tTcpIpPacketParsingResult res)
{
    static const char *const CSCaseName[4] = { "-", "PCS", "CS", "Bad" };
    return CSCaseName[res.xxpCheckSum];
}

static __inline VOID PrintOutParsingResult(
    tTcpIpPacketParsingResult res,
    int level,
    LPCSTR procname)
{
    DPrintf(level, ("[%s] %s packet IPCS %s%s, checksum %s%s", procname,
        GetPacketCase(res),
        GetIPCSCase(res),
        res.fixedIpCS ? "(fixed)" : "",
        GetXxpCSCase(res),
        res.fixedXxpCS ? "(fixed)" : ""));
}

tTcpIpPacketParsingResult ParaNdis_CheckSumVerify(PVOID buffer, ULONG size, ULONG flags, LPCSTR caller)
{
    tTcpIpPacketParsingResult res = QualifyIpPacket(buffer, size);
    if (res.ipStatus == ppresIPV4)
    {
        if (flags & pcrIpChecksum)
            res = VerifyIpChecksum(buffer, res, (flags & pcrFixIPChecksum) != 0);
        if(res.xxpStatus == ppresXxpKnown)
        {
            if (res.TcpUdp == ppresIsTCP) /* TCP */
            {
                if(flags & pcrTcpV4Checksum)
                {
                    res = VerifyTcpChecksum(buffer, size, res, flags & (pcrFixPHChecksum | pcrFixTcpV4Checksum));
                }
            }
            else /* UDP */
            {
                if (flags & pcrUdpV4Checksum)
                {
                    res = VerifyUdpChecksum(buffer, size, res, flags & (pcrFixPHChecksum | pcrFixUdpV4Checksum));
                }
            }
        }
    }
    else if (res.ipStatus == ppresIPV6)
    {
        if(res.xxpStatus == ppresXxpKnown)
        {
            if (res.TcpUdp == ppresIsTCP) /* TCP */
            {
                if(flags & pcrTcpV6Checksum)
                {
                    res = VerifyTcpChecksum(buffer, size, res, flags & (pcrFixPHChecksum | pcrFixTcpV6Checksum));
                }
            }
            else /* UDP */
            {
                if (flags & pcrUdpV6Checksum)
                {
                    res = VerifyUdpChecksum(buffer, size, res, flags & (pcrFixPHChecksum | pcrFixUdpV6Checksum));
                }
            }
        }
    }
    PrintOutParsingResult(res, 1, caller);
    return res;
}

tTcpIpPacketParsingResult ParaNdis_ReviewIPPacket(PVOID buffer, ULONG size, LPCSTR caller)
{
    tTcpIpPacketParsingResult res = QualifyIpPacket(buffer, size);
    PrintOutParsingResult(res, 1, caller);
    return res;
}
