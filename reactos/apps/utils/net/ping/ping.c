/* $Id: ping.c,v 1.5 2001/06/15 17:48:43 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ping utility
 * FILE:        apps/net/ping/ping.c
 * PURPOSE:     Network test utility
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <winsock2.h>
#include <tchar.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#ifndef _MSC_VER

//#define DBG

/* FIXME: Where should this be? */
#define CopyMemory(Destination, Source, Length) memcpy(Destination, Source, Length);

/* Should be in the header files somewhere (exported by ntdll.dll) */
long atol(const char *str);

#ifndef __int64
typedef long long __int64;
#endif

char * _i64toa(__int64 value, char *string, int radix);

#endif


/* General ICMP constants */
#define ICMP_MINSIZE		8		/* Minimum ICMP packet size */
#define ICMP_MAXSIZE		65535	/* Maximum ICMP packet size */

/* ICMP message types */
#define ICMPMSG_ECHOREQUEST	8		/* ICMP ECHO request message */
#define ICMPMSG_ECHOREPLY	0		/* ICMP ECHO reply message */

#pragma pack(4)

/* IPv4 header structure */
typedef struct _IPv4_HEADER {
	unsigned char	IHL:4;
	unsigned char	Version:4;
	unsigned char	TOS;
	unsigned short	Length;
	unsigned short	Id;
	unsigned short	FragFlags;
	unsigned char	TTL;
	unsigned char	Protocol;
	unsigned short	Checksum;
	unsigned int	SrcAddress;
	unsigned int	DstAddress;
} IPv4_HEADER, *PIPv4_HEADER;

/* ICMP echo request/reply header structure */
typedef struct _ICMP_HEADER {
	unsigned char	Type;
	unsigned char	Code;
	unsigned short	Checksum;
	unsigned short	Id;
	unsigned short	SeqNum;
} ICMP_HEADER, *PICMP_HEADER;

typedef struct _ICMP_ECHO_PACKET {
	ICMP_HEADER   Icmp;
	LARGE_INTEGER Timestamp;
} ICMP_ECHO_PACKET, *PICMP_ECHO_PACKET;

#pragma pack(1)

BOOL                InvalidOption;
BOOL                NeverStop;
BOOL                ResolveAddresses;
UINT                PingCount;
UINT                DataSize;   /* ICMP echo request data size */
BOOL                DontFragment;
ULONG               TTLValue;
ULONG               TOSValue;
ULONG               Timeout;
CHAR                TargetName[256];
SOCKET              IcmpSock;
SOCKADDR_IN         Target;
LPSTR               TargetIP;
FD_SET              Fds;
TIMEVAL             Timeval;
UINT                CurrentSeqNum;
UINT                SentCount;
UINT                LostCount;
BOOL                MinRTTSet;
LARGE_INTEGER       MinRTT;     /* Minimum round trip time in microseconds */
LARGE_INTEGER       MaxRTT;
LARGE_INTEGER       SumRTT;
LARGE_INTEGER       AvgRTT;
LARGE_INTEGER       TicksPerMs; /* Ticks per millisecond */
LARGE_INTEGER       TicksPerUs; /* Ticks per microsecond */
BOOL                UsePerformanceCounter;


/* Display the contents of a buffer */
VOID DisplayBuffer(
    PVOID Buffer,
    DWORD Size)
{
    UINT i;
    PCHAR p;

    printf("Buffer (0x%X)  Size (0x%X).\n", Buffer, Size);

    p = (PCHAR)Buffer;
    for (i = 0; i < Size; i++) {
      if (i % 16 == 0) {
        printf("\n");
      }
      printf("%02X ", (p[i]) & 0xFF);
    }
}

/* Display usage information on screen */
VOID Usage(VOID)
{
	printf("\nUsage: ping [-t] [-n count] [-l size] [-w timeout] destination-host\n\n");
	printf("Options:\n");
	printf("    -t             Ping the specified host until stopped.\n");
	printf("                   To stop - type Control-C.\n");
	printf("    -n count       Number of echo requests to send.\n");
	printf("    -l size        Send buffer size.\n");
	printf("    -w timeout     Timeout in milliseconds to wait for each reply.\n\n");
}

/* Reset configuration to default values */
VOID Reset(VOID)
{
    LARGE_INTEGER PerformanceCounterFrequency;

    NeverStop             = FALSE;
    ResolveAddresses      = FALSE;
    PingCount             = 4;
    DataSize              = 32;
    DontFragment          = FALSE;
    TTLValue              = 128;
    TOSValue              = 0;
    Timeout               = 1000;
    UsePerformanceCounter = QueryPerformanceFrequency(&PerformanceCounterFrequency);

    if (UsePerformanceCounter) {
        /* Performance counters may return incorrect results on some multiprocessor
           platforms so we restrict execution on the first processor. This may fail
           on Windows NT so we fall back to GetCurrentTick() for timing */
        if (SetThreadAffinityMask (GetCurrentThread(), 1) == 0) {
            UsePerformanceCounter = FALSE;
        }

        /* Convert frequency to ticks per millisecond */
        TicksPerMs.QuadPart = PerformanceCounterFrequency.QuadPart / 1000;
        /* And to ticks per microsecond */
        TicksPerUs.QuadPart = PerformanceCounterFrequency.QuadPart / 1000000;
    }
    if (!UsePerformanceCounter) {
        /* 1 tick per millisecond for GetCurrentTick() */
        TicksPerMs.QuadPart = 1;
        /* GetCurrentTick() cannot handle microseconds */
        TicksPerUs.QuadPart = 1;
    }
}

/* Return ULONG in a string */
ULONG GetULONG(LPSTR String)
{
    UINT i, Length;
    ULONG Value;

    i = 0;
    Length = strlen(String);
    while ((i < Length) && ((String[i] < '0') || (String[i] > '9'))) i++;
    if ((i >= Length) || ((String[i] < '0') || (String[i] > '9'))) {
        InvalidOption = TRUE;
        return 0;
    }
    Value = (ULONG)atol(&String[i]);

    return Value;
}

/* Return ULONG in a string. Try next paramter if not successful */
ULONG GetULONG2(LPSTR String1, LPSTR String2, PINT i)
{
    ULONG Value;

    Value = GetULONG(String1);
    if (InvalidOption) {
        InvalidOption = FALSE;
        if (String2[0] != '-') {
            Value = GetULONG(String2);
            if (!InvalidOption)
                *i += 1;
        }
    }

    return Value;
}

/* Parse command line parameters */
BOOL ParseCmdline(int argc, char* argv[])
{
    INT i;
    BOOL ShowUsage;
    BOOL FoundTarget;
#if 1
    lstrcpy(TargetName, "127.0.0.1");
    PingCount = 1;
    return TRUE;
#endif
    if (argc < 2) {
        ShowUsage = TRUE;
    } else {
        ShowUsage = FALSE;
    }
    FoundTarget = FALSE;
    InvalidOption = FALSE;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 't': NeverStop = TRUE; break;
            case 'a': ResolveAddresses = TRUE; break;
            case 'n': PingCount = GetULONG2(&argv[i][2], argv[i + 1], &i); break;
            case 'l':
                DataSize = GetULONG2(&argv[i][2], argv[i + 1], &i);
                if ((DataSize < 0) || (DataSize > ICMP_MAXSIZE - sizeof(ICMP_ECHO_PACKET))) {
                    printf("Bad value for option -l, valid range is from 0 to %d.\n",
                        ICMP_MAXSIZE - sizeof(ICMP_ECHO_PACKET));
                    return FALSE;
                }
                break;
            case 'f': DontFragment = TRUE; break;
            case 'i': TTLValue = GetULONG2(&argv[i][2], argv[i + 1], &i); break;
            case 'v': TOSValue = GetULONG2(&argv[i][2], argv[i + 1], &i); break;
            case 'w': Timeout  = GetULONG2(&argv[i][2], argv[i + 1], &i); break;
            default:
                printf("Bad option %s.\n", argv[i]);
                Usage();
                return FALSE;
            }
            if (InvalidOption) {
                printf("Bad option format %s.\n", argv[i]);
                return FALSE;
            }
        } else {
            if (FoundTarget) {
                printf("Bad parameter %s.\n", argv[i]);
                return FALSE;
            } else {
				lstrcpy(TargetName, argv[i]);
                FoundTarget = TRUE;
            }
        }
    }

    if ((!ShowUsage) && (!FoundTarget)) {
        printf("Name or IP address of destination host must be specified.\n");
        return FALSE;
    }

    if (ShowUsage) {
        Usage();
        return FALSE;
    }
    return TRUE;
}

/* Calculate checksum of data */
WORD Checksum(PUSHORT data, UINT size)
{
    ULONG sum = 0;

    while (size > 1) {
        sum  += *data++;
        size -= sizeof(USHORT);
    }

    if (size)
        sum += *(UCHAR*)data;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (USHORT)(~sum);
}

/* Prepare to ping target */
BOOL Setup(VOID)
{
    WORD     wVersionRequested;
    WSADATA  WsaData;
    INT	     Status;
    ULONG    Addr;
    PHOSTENT phe;

    wVersionRequested = MAKEWORD(2, 2);
 
    Status = WSAStartup(wVersionRequested, &WsaData);
    if (Status != 0) {
        printf("Could not initialize winsock dll.\n");	
        return FALSE;
    }

    IcmpSock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
    if (IcmpSock == INVALID_SOCKET) {
        printf("Could not create socket (#%d).\n", WSAGetLastError());
        return FALSE;
    }

    ZeroMemory(&Target, sizeof(Target));
    phe = NULL;
    Addr = inet_addr(TargetName);
    if (Addr == INADDR_NONE) {
        phe = gethostbyname(TargetName);
        if (phe == NULL) {
            printf("Unknown host %s.\n", TargetName);
            return FALSE;
        } 
    }
	
    if (phe != NULL) {
        CopyMemory(&Target.sin_addr, phe->h_addr_list, phe->h_length);
    } else {
        Target.sin_addr.s_addr = Addr;
    }

    if (phe != NULL) {
		Target.sin_family = phe->h_addrtype;
    } else {
        Target.sin_family = AF_INET;
    }
	
    TargetIP		= inet_ntoa(Target.sin_addr);
    CurrentSeqNum	= 0;
    SentCount		= 0;
	LostCount		= 0;
    MinRTT.QuadPart = 0;
    MaxRTT.QuadPart = 0;
    SumRTT.QuadPart = 0;
    MinRTTSet       = FALSE;
    return TRUE;
}

/* Close socket */
VOID Cleanup(VOID)
{
    if (IcmpSock != INVALID_SOCKET)
        closesocket(IcmpSock);

    WSACleanup();
}

VOID QueryTime(PLARGE_INTEGER Time)
{
    if (UsePerformanceCounter) {
        if (QueryPerformanceCounter(Time) == 0) {
            /* This should not happen, but we fall
               back to GetCurrentTick() if it does */
            Time->u.LowPart  = (ULONG)GetTickCount();
            Time->u.HighPart = 0;

            /* 1 tick per millisecond for GetCurrentTick() */
            TicksPerMs.QuadPart = 1;
            /* GetCurrentTick() cannot handle microseconds */
            TicksPerUs.QuadPart = 1;

            UsePerformanceCounter = FALSE;
        }
    } else {
        Time->u.LowPart  = (ULONG)GetTickCount();
        Time->u.HighPart = 0;
    }
}

VOID TimeToMsString(LPSTR String, LARGE_INTEGER Time)
{
    UINT          i, Length;
    CHAR          Convstr[40];
    LARGE_INTEGER LargeTime;

    LargeTime.QuadPart = Time.QuadPart / TicksPerMs.QuadPart;
    _i64toa(LargeTime.QuadPart, Convstr, 10);
	strcpy(String, Convstr);
    strcat(String, ",");

    LargeTime.QuadPart = (Time.QuadPart % TicksPerMs.QuadPart) / TicksPerUs.QuadPart;
    _i64toa(LargeTime.QuadPart, Convstr, 10);
    Length = strlen(Convstr);
    if (Length < 4) {
        for (i = 0; i < 4 - Length; i++)
            strcat(String, "0");
    }

    strcat(String, Convstr);
    strcat(String, "ms");
}

/* Locate the ICMP data and print it. Returns TRUE if the packet was good,
   FALSE if not */
BOOL DecodeResponse(PCHAR buffer, UINT size, PSOCKADDR_IN from)
{
    PIPv4_HEADER      IpHeader;
    PICMP_ECHO_PACKET Icmp;
  	UINT              IphLength;
    CHAR              Time[100];
    LARGE_INTEGER     RelativeTime;
    LARGE_INTEGER     LargeTime;

    IpHeader = (PIPv4_HEADER)buffer;

    IphLength = IpHeader->IHL * 4;

    if (size  < IphLength + ICMP_MINSIZE) {
#ifdef DBG
        printf("Bad size (0x%X < 0x%X)\n", size, IphLength + ICMP_MINSIZE);
#endif DBG
        return FALSE;
    }

    Icmp = (PICMP_ECHO_PACKET)(buffer + IphLength);

    if (Icmp->Icmp.Type != ICMPMSG_ECHOREPLY) {
#ifdef DBG
        printf("Bad ICMP type (0x%X should be 0x%X)\n", Icmp->Icmp.Type, ICMPMSG_ECHOREPLY);
#endif DBG
        return FALSE;
    }

    if (Icmp->Icmp.Id != (USHORT)GetCurrentProcessId()) {
#ifdef DBG
        printf("Bad ICMP id (0x%X should be 0x%X)\n", Icmp->Icmp.Id, (USHORT)GetCurrentProcessId());
#endif DBG
        return FALSE;
    }

    QueryTime(&LargeTime);

    RelativeTime.QuadPart = (LargeTime.QuadPart - Icmp->Timestamp.QuadPart);

    TimeToMsString(Time, RelativeTime);

    printf("Reply from %s: bytes=%d time=%s TTL=%d\n", inet_ntoa(from->sin_addr), 
      size - IphLength - sizeof(ICMP_ECHO_PACKET), Time, IpHeader->TTL);
    if (RelativeTime.QuadPart < MinRTT.QuadPart) {
		  MinRTT.QuadPart = RelativeTime.QuadPart;
      MinRTTSet = TRUE;
    }
	  if (RelativeTime.QuadPart > MaxRTT.QuadPart)
	      MaxRTT.QuadPart = RelativeTime.QuadPart;
    SumRTT.QuadPart += RelativeTime.QuadPart;

	return TRUE;
}

/* Send and receive one ping */
BOOL Ping(VOID)
{
    INT                 Status;
    SOCKADDR            From;
    UINT                Length;
    PVOID               Buffer;
    UINT                Size;
    PICMP_ECHO_PACKET   Packet;

    /* Account for extra space for IP header when packet is received */
    Size   = DataSize + 128;
    Buffer = GlobalAlloc(0, Size);
    if (!Buffer) {
        printf("Not enough free resources available.\n");
        return FALSE;
    }

    ZeroMemory(Buffer, Size);
    Packet = (PICMP_ECHO_PACKET)Buffer;

    /* Assemble ICMP echo request packet */
    Packet->Icmp.Type     = ICMPMSG_ECHOREQUEST;
    Packet->Icmp.Code     = 0;
    Packet->Icmp.Id	      = (USHORT)GetCurrentProcessId();
    Packet->Icmp.SeqNum   = (USHORT)CurrentSeqNum;
    Packet->Icmp.Checksum = 0;

    /* Timestamp is part of data area */
    QueryTime(&Packet->Timestamp);

    CopyMemory(Buffer, &Packet->Icmp, sizeof(ICMP_ECHO_PACKET) + DataSize);
    /* Calculate checksum for ICMP header and data area */
    Packet->Icmp.Checksum = Checksum((PUSHORT)&Packet->Icmp, sizeof(ICMP_ECHO_PACKET) + DataSize);

    CurrentSeqNum++;

	/* Send ICMP echo request */

    FD_ZERO(&Fds);
    FD_SET(IcmpSock, &Fds);
    Timeval.tv_sec  = Timeout / 1000;
    Timeval.tv_usec = Timeout % 1000;
    Status = select(0, NULL, &Fds, NULL, &Timeval);
    if ((Status != SOCKET_ERROR) && (Status != 0)) {

#ifdef DBG
        printf("Sending packet\n");
        DisplayBuffer(Buffer, sizeof(ICMP_ECHO_PACKET) + DataSize);
        printf("\n");
#endif DBG

        Status = sendto(IcmpSock, Buffer, sizeof(ICMP_ECHO_PACKET) + DataSize,
            0, (SOCKADDR*)&Target, sizeof(Target));
        SentCount++;
    }
    if (Status == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEHOSTUNREACH) {
            printf("Destination host unreachable.\n");
        } else {
            printf("Could not transmit data (%d).\n", WSAGetLastError());
        }
        GlobalFree(Buffer);
        return FALSE;
    }

    /* Expect to receive ICMP echo reply */
    FD_ZERO(&Fds);
    FD_SET(IcmpSock, &Fds);
    Timeval.tv_sec  = Timeout / 1000;
    Timeval.tv_usec = Timeout % 1000;

    Status = select(0, &Fds, NULL, NULL, &Timeval);
    if ((Status != SOCKET_ERROR) && (Status != 0)) {
        Length = sizeof(From);
        Status = recvfrom(IcmpSock, Buffer, Size, 0, &From, &Length);

#ifdef DBG
        printf("Received packet\n");
        DisplayBuffer(Buffer, Status);
        printf("\n");
#endif DBG
    }
    if (Status == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAETIMEDOUT) {
            printf("Could not receive data (%d).\n", WSAGetLastError());
            GlobalFree(Buffer);
            return FALSE;
        }
        Status = 0;
    }

    if (Status == 0) {
        printf("Request timed out.\n");
        LostCount++;
        GlobalFree(Buffer);
        return TRUE;
    }

    if (!DecodeResponse(Buffer, Status, (PSOCKADDR_IN)&From)) {
        /* FIXME: Wait again as it could be another ICMP message type */
        printf("Request timed out (incomplete datagram received).\n");
        LostCount++;
    }

    GlobalFree(Buffer);
    return TRUE;
}


/* Program entry point */
int main(int argc, char* argv[])
{
    UINT Count;
    CHAR MinTime[20];
    CHAR MaxTime[20];
    CHAR AvgTime[20];

    Reset();

    if ((ParseCmdline(argc, argv)) && (Setup())) {

        printf("\nPinging %s [%s] with %d bytes of data:\n\n",
            TargetName, TargetIP, DataSize);
		
		Count = 0;
		while ((NeverStop) || (Count < PingCount)) {
			Ping();
			Sleep(Timeout);
			Count++;
		};

        Cleanup();

		/* Calculate avarage round trip time */
        if ((SentCount - LostCount) > 0) {
            AvgRTT.QuadPart = SumRTT.QuadPart / (SentCount - LostCount);
        } else {
            AvgRTT.QuadPart = 0;
        }

        /* Calculate loss percent */
        if (LostCount > 0) {
            Count = (SentCount * 100) / LostCount;
        } else {
            Count = 0;
        }

        if (!MinRTTSet)
            MinRTT = MaxRTT;

        TimeToMsString(MinTime, MinRTT);
        TimeToMsString(MaxTime, MaxRTT);
        TimeToMsString(AvgTime, AvgRTT);

        /* Print statistics */
        printf("\nPing statistics for %s:\n", TargetIP);
        printf("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss),\n",
            SentCount, SentCount - LostCount, LostCount, Count);
        printf("Approximate round trip times in milli-seconds:\n");
        printf("    Minimum = %s, Maximum = %s, Average = %s\n",
            MinTime, MaxTime, AvgTime);
    }
	return 0;
}

/* EOF */
