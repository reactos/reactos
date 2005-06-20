/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS traceroute utility
 * FILE:        apps/utils/net/tracert/tracert.h
 * PURPOSE:     trace a packets route through a network
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 03/05/05 Created
 */

#define ECHO_REPLY 0
#define DEST_UNREACHABLE 3
#define ECHO_REQUEST 8
#define TTL_EXCEEDED 11

#define ICMP_MIN_SIZE 8
#define ICMP_MAX_SIZE 65535
#define PACKET_SIZE 32
/* we need this for packets which have the 'dont fragment' 
 * bit set, as they can get quite large otherwise 
 * (I've seen some reach 182 bytes */
#define MAX_REC_SIZE 200 

/* pack the structures */
#pragma pack(1) 

/* IPv4 Header, 20 bytes */
typedef struct IPv4Header {
    BYTE h_len:4;
    BYTE version:4;
    BYTE tos;
    USHORT length;
    USHORT id;
    USHORT flag_frag;
    BYTE ttl;
    BYTE proto;
    USHORT checksum;
    ULONG source;
    ULONG dest;
} IPv4_HEADER, *PIPv4_HEADER;

/* ICMP Header, 8 bytes */
typedef struct ICMPHeader {
    BYTE type;
    BYTE code;
    USHORT checksum;
    USHORT id; // not used in time exceeded
    USHORT seq; // not used in time exceeded
} ICMP_HEADER, *PICMP_HEADER;

/* ICMP Echo Reply Header, 12 bytes */
typedef struct EchoReplyHeader {
    struct ICMPHeader icmpheader;
    struct timeval timestamp; 
} ECHO_REPLY_HEADER, *PECHO_REPLY_HEADER;

/* ICMP Echo Reply Header, 12 bytes */
typedef struct TTLExceedHeader {
    struct ICMPHeader icmpheader;
    struct IPv4Header ipheader;
    struct ICMPHeader OrigIcmpHeader;
} TTL_EXCEED_HEADER, *PTTL_EXCEED_HEADER;

/* return to normal */
#pragma pack()


/* function definitions */
//BOOL ParseCmdline(int argc, char* argv[]);
INT Driver(void);
INT Setup(INT ttl);
VOID SetupTimingMethod(void);
VOID ResolveHostname(void);
VOID PreparePacket(INT packetSize, INT seqNum);
INT SendPacket(INT datasize);
INT ReceivePacket(INT datasize);
INT DecodeResponse(INT packetSize, INT seqNum);
LONG GetTime(void);
WORD CheckSum(PUSHORT data, UINT size);
VOID Usage(void);
