#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <string.h>
#include <time.h>

#define ECHO_REPLY 0
#define DEST_UNREACHABLE 3
#define ECHO_REQUEST 8
#define TTL_EXCEEDED 11

#define MAX_PING_PACKET_SIZE 1024
#define MAX_PING_DATA_SIZE (MAX_PING_PACKET_SIZE + sizeof(IPv4Header)
#define PACKET_SIZE 32
#define ICMP_MIN_SIZE 8

/* we need this for packets which have the 'dont fragment'
 * bit set, as they can get quite large otherwise */
#define MAX_REC_SIZE 200

/* pack the structures */
#include <pshpack1.h>

/* IPv4 Header, 20 bytes */
typedef struct IPv4Header
{
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
typedef struct ICMPHeader
{
    BYTE type;
    BYTE code;
    USHORT checksum;
    USHORT id; // not used in time exceeded
    USHORT seq; // not used in time exceeded
} ICMP_HEADER, *PICMP_HEADER;

/* ICMP Echo Reply Header, 12 bytes */
typedef struct EchoReplyHeader
{
    struct ICMPHeader icmpheader;
    struct timeval timestamp;
} ECHO_REPLY_HEADER, *PECHO_REPLY_HEADER;

/* ICMP Echo Reply Header, 12 bytes */
typedef struct TTLExceedHeader
{
    struct ICMPHeader icmpheader;
    struct IPv4Header ipheader;
    struct ICMPHeader OrigIcmpHeader;
} TTL_EXCEED_HEADER, *PTTL_EXCEED_HEADER;

#include <poppack.h>

typedef struct _APPINFO
{
    SOCKET icmpSock;                // socket descriptor
    SOCKADDR_IN source, dest;       // source and destination address info
    PECHO_REPLY_HEADER SendPacket;   // ICMP echo packet
    PIPv4_HEADER RecvPacket;         // return reveive packet

    BOOL bUsePerformanceCounter;    // whether to use the high res performance counter
    LARGE_INTEGER TicksPerMs;       // number of millisecs in relation to proc freq
    LARGE_INTEGER TicksPerUs;       // number of microsecs in relation to proc freq
    LONGLONG lTimeStart;            // send packet, timer start
    LONGLONG lTimeEnd;              // receive packet, timer end

    BOOL bResolveAddresses;         // -d  MS ping defaults to true.
    INT iMaxHops;                   // -h  Max number of hops before trace ends
    INT iHostList;                  // -j  Source route
    INT iTimeOut;                   // -w  time before packet times out

} APPINFO, *PAPPINFO;
