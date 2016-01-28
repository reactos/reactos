/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ping utility
 * FILE:        base/applications/network/ping/ping.c
 * PURPOSE:     Network test utility
 * PROGRAMMERS:
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <wincon.h>
#define _INC_WINDOWS
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#include "resource.h"

#define NDEBUG

/* General ICMP constants */
#define ICMP_MINSIZE        8     /* Minimum ICMP packet size */
#define ICMP_MAXSIZE        65535 /* Maximum ICMP packet size */

/* ICMP message types */
#define ICMPMSG_ECHOREQUEST 8     /* ICMP ECHO request message */
#define ICMPMSG_ECHOREPLY   0     /* ICMP ECHO reply message */

#pragma pack(4)

/* IPv4 header structure */
typedef struct _IPv4_HEADER
{
    unsigned char IHL:4;
    unsigned char Version:4;
    unsigned char TOS;
    unsigned short Length;
    unsigned short Id;
    unsigned short FragFlags;
    unsigned char TTL;
    unsigned char Protocol;
    unsigned short Checksum;
    unsigned int SrcAddress;
    unsigned int DstAddress;
} IPv4_HEADER, *PIPv4_HEADER;

/* ICMP echo request/reply header structure */
typedef struct _ICMP_HEADER
{
    unsigned char Type;
    unsigned char Code;
    unsigned short Checksum;
    unsigned short Id;
    unsigned short SeqNum;
} ICMP_HEADER, *PICMP_HEADER;

typedef struct _ICMP_ECHO_PACKET
{
    ICMP_HEADER Icmp;
} ICMP_ECHO_PACKET, *PICMP_ECHO_PACKET;

#pragma pack(1)

BOOL                NeverStop;
BOOL                ResolveAddresses;
UINT                PingCount;
UINT                DataSize;   /* ICMP echo request data size */
BOOL                DontFragment;
ULONG               TTLValue;
ULONG               TOSValue;
ULONG               Timeout;
WCHAR               TargetName[256];
SOCKET              IcmpSock;
SOCKADDR_IN         Target;
WCHAR               TargetIP[16];
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
LARGE_INTEGER       SentTime;
BOOL                UsePerformanceCounter;
HANDLE              hStdOutput;

#ifndef NDEBUG
/* Display the contents of a buffer */
static VOID DisplayBuffer(
    PVOID Buffer,
    DWORD Size)
{
    UINT i;
    PCHAR p;

    printf("Buffer (0x%p)  Size (0x%lX).\n", Buffer, Size);

    p = (PCHAR)Buffer;
    for (i = 0; i < Size; i++)
    {
        if (i % 16 == 0)
            printf("\n");
        printf("%02X ", (p[i]) & 0xFF);
    }
}
#endif /* !NDEBUG */

void FormatOutput(UINT uID, ...)
{
    va_list valist;

    WCHAR Buf[1024];
    CHAR AnsiBuf[1024];
    LPWSTR pBuf = Buf;
    PCHAR pAnsiBuf = AnsiBuf;
    WCHAR Format[1024];
    DWORD written;
    UINT DataLength;
    int AnsiLength;

    if (!LoadString(GetModuleHandle(NULL), uID,
                    Format, sizeof(Format) / sizeof(WCHAR)))
    {
        return;
    }

    va_start(valist, uID);

    DataLength = FormatMessage(FORMAT_MESSAGE_FROM_STRING, Format, 0, 0, Buf,\
                  sizeof(Buf) / sizeof(WCHAR), &valist);

    if(!DataLength)
    {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            va_end(valist);
            return;
        }

        DataLength = FormatMessage(FORMAT_MESSAGE_FROM_STRING |\
                                    FORMAT_MESSAGE_ALLOCATE_BUFFER,\
                                    Format, 0, 0, (LPWSTR)&pBuf, 0, &valist);
    }

    if(!DataLength)
    {
        va_end(valist);
        return;
    }

    if(GetFileType(hStdOutput) == FILE_TYPE_CHAR)
    {
        /* Is a console or a printer */
        WriteConsole(hStdOutput, pBuf, DataLength, &written, NULL);
    }
    else
    {
        /* Is a pipe, socket, file or other */
        AnsiLength = WideCharToMultiByte(CP_ACP, 0, pBuf, DataLength,\
                                         NULL, 0, NULL, NULL);

        if(AnsiLength >= sizeof(AnsiBuf))
            pAnsiBuf = (PCHAR)HeapAlloc(GetProcessHeap(), 0, AnsiLength);

        AnsiLength = WideCharToMultiByte(CP_OEMCP, 0, pBuf, DataLength,\
                                         pAnsiBuf, AnsiLength, " ", NULL);

        WriteFile(hStdOutput, pAnsiBuf, AnsiLength, &written, NULL);

        if(pAnsiBuf != AnsiBuf)
            HeapFree(NULL, 0, pAnsiBuf);
    }

    if(pBuf != Buf)
        LocalFree(pBuf);
}

/* Display usage information on screen */
static VOID Usage(VOID)
{
    FormatOutput(IDS_USAGE);
}

/* Reset configuration to default values */
static VOID Reset(VOID)
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

    if (UsePerformanceCounter)
    {
        /* Performance counters may return incorrect results on some multiprocessor
           platforms so we restrict execution on the first processor. This may fail
           on Windows NT so we fall back to GetCurrentTick() for timing */
        if (SetThreadAffinityMask (GetCurrentThread(), 1) == 0)
            UsePerformanceCounter = FALSE;

        /* Convert frequency to ticks per millisecond */
        TicksPerMs.QuadPart = PerformanceCounterFrequency.QuadPart / 1000;
        /* And to ticks per microsecond */
        TicksPerUs.QuadPart = PerformanceCounterFrequency.QuadPart / 1000000;
    }
    if (!UsePerformanceCounter)
    {
        /* 1 tick per millisecond for GetCurrentTick() */
        TicksPerMs.QuadPart = 1;
        /* GetCurrentTick() cannot handle microseconds */
        TicksPerUs.QuadPart = 1;
    }
}

/* Parse command line parameters */
static BOOL ParseCmdline(int argc, LPWSTR argv[])
{
    INT i;
    BOOL FoundTarget = FALSE, InvalidOption = FALSE;

    if (argc < 2)
    {
        Usage();
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            switch (argv[i][1])
            {
                case L't': NeverStop = TRUE; break;
                case L'a': ResolveAddresses = TRUE; break;
                case L'n':
                    if (i + 1 < argc)
                    {
                        PingCount = wcstoul(argv[++i], NULL, 0);

                        if (PingCount == 0)
                        {
                            FormatOutput(IDS_BAD_VALUE_OPTION_N, UINT_MAX);
                            return FALSE;
                        }
                    }
                    else
                        InvalidOption = TRUE;
                    break;
                case L'l':
                    if (i + 1 < argc)
                    {
                        DataSize = wcstoul(argv[++i], NULL, 0);

                        if (DataSize > ICMP_MAXSIZE - sizeof(ICMP_ECHO_PACKET) - sizeof(IPv4_HEADER))
                        {
                            FormatOutput(IDS_BAD_VALUE_OPTION_L, ICMP_MAXSIZE - \
                                         (int)sizeof(ICMP_ECHO_PACKET) - \
                                         (int)sizeof(IPv4_HEADER));
                            return FALSE;
                        }
                    } else
                        InvalidOption = TRUE;
                    break;
                case L'f': DontFragment = TRUE; break;
                case L'i':
                    if (i + 1 < argc)
                        TTLValue = wcstoul(argv[++i], NULL, 0);
                    else
                        InvalidOption = TRUE;
                    break;
                case L'v':
                    if (i + 1 < argc)
                        TOSValue = wcstoul(argv[++i], NULL, 0);
                    else
                        InvalidOption = TRUE;
                    break;
                case L'w':
                    if (i + 1 < argc)
                        Timeout = wcstoul(argv[++i], NULL, 0);
                    else
                        InvalidOption = TRUE;
                    break;
                case '?':
                    Usage();
                    return FALSE;
                default:
                    FormatOutput(IDS_BAD_OPTION, argv[i]);
                    return FALSE;
            }
            if (InvalidOption)
            {
                FormatOutput(IDS_BAD_OPTION_FORMAT, argv[i]);
                return FALSE;
            }
        }
        else
        {
            if (FoundTarget)
            {
                FormatOutput(IDS_BAD_PARAMETER, argv[i]);
                return FALSE;
            }
            else
            {
                wcscpy(TargetName, argv[i]);
                FoundTarget = TRUE;
            }
        }
    }

    if (!FoundTarget)
    {
        FormatOutput(IDS_DEST_MUST_BE_SPECIFIED);
        return FALSE;
    }

    return TRUE;
}

/* Calculate checksum of data */
static WORD Checksum(PUSHORT data, UINT size)
{
    ULONG sum = 0;

    while (size > 1)
    {
        sum  += *data++;
        size -= sizeof(USHORT);
    }

    if (size)
        sum += *(UCHAR*)data;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (USHORT)(~sum);
}

static BOOL WINAPI StopLoop(DWORD dwCtrlType)
{
    NeverStop = FALSE;
    PingCount = 0;

    return TRUE;
}

/* Prepare to ping target */
static BOOL Setup(VOID)
{
    WORD     wVersionRequested;
    WSADATA  WsaData;
    INT      Status;
    ULONG    Addr;
    PHOSTENT phe;
    CHAR     aTargetName[256];

    wVersionRequested = MAKEWORD(2, 2);

    Status = WSAStartup(wVersionRequested, &WsaData);
    if (Status != 0)
    {
        FormatOutput(IDS_COULD_NOT_INIT_WINSOCK);
        return FALSE;
    }

    IcmpSock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
    if (IcmpSock == INVALID_SOCKET)
    {
        FormatOutput(IDS_COULD_NOT_CREATE_SOCKET, WSAGetLastError());
        return FALSE;
    }

    if (setsockopt(IcmpSock,
                   IPPROTO_IP,
                   IP_DONTFRAGMENT,
                   (const char *)&DontFragment,
                   sizeof(DontFragment)) == SOCKET_ERROR)
    {
        FormatOutput(IDS_SETSOCKOPT_FAILED, WSAGetLastError());
        return FALSE;
    }

    if (setsockopt(IcmpSock,
                   IPPROTO_IP,
                   IP_TTL,
                   (const char *)&TTLValue,
                   sizeof(TTLValue)) == SOCKET_ERROR)
    {
        FormatOutput(IDS_SETSOCKOPT_FAILED, WSAGetLastError());
        return FALSE;
    }


    if(!WideCharToMultiByte(CP_ACP, 0, TargetName, -1, aTargetName,\
                            sizeof(aTargetName), NULL, NULL))
    {
        FormatOutput(IDS_UNKNOWN_HOST, TargetName);
        return FALSE;
    }

    ZeroMemory(&Target, sizeof(Target));
    phe = NULL;
    Addr = inet_addr(aTargetName);
    if (Addr == INADDR_NONE)
    {
        phe = gethostbyname(aTargetName);
        if (phe == NULL)
        {
            FormatOutput(IDS_UNKNOWN_HOST, TargetName);
            return FALSE;
        }

        CopyMemory(&Target.sin_addr, phe->h_addr, phe->h_length);
        Target.sin_family = phe->h_addrtype;
    }
    else
    {
        Target.sin_addr.s_addr = Addr;
        Target.sin_family = AF_INET;
    }


    swprintf(TargetIP, L"%d.%d.%d.%d", Target.sin_addr.S_un.S_un_b.s_b1,\
                                       Target.sin_addr.S_un.S_un_b.s_b2,\
                                       Target.sin_addr.S_un.S_un_b.s_b3,\
                                       Target.sin_addr.S_un.S_un_b.s_b4);
    CurrentSeqNum = 1;
    SentCount = 0;
    LostCount = 0;
    MinRTT.QuadPart = 0;
    MaxRTT.QuadPart = 0;
    SumRTT.QuadPart = 0;
    MinRTTSet       = FALSE;

    SetConsoleCtrlHandler(StopLoop, TRUE);

    return TRUE;
}

/* Close socket */
static VOID Cleanup(VOID)
{
    if (IcmpSock != INVALID_SOCKET)
        closesocket(IcmpSock);

    WSACleanup();
}

static VOID QueryTime(PLARGE_INTEGER Time)
{
    if (UsePerformanceCounter)
    {
        if (QueryPerformanceCounter(Time) == 0)
        {
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
    }
    else
    {
        Time->u.LowPart  = (ULONG)GetTickCount();
        Time->u.HighPart = 0;
    }
}

static VOID TimeToMsString(LPWSTR String, ULONG Length, LARGE_INTEGER Time)
{
    WCHAR         Convstr[40];
    LARGE_INTEGER LargeTime;
    LPWSTR ms;

    LargeTime.QuadPart = Time.QuadPart / TicksPerMs.QuadPart;

    _i64tow(LargeTime.QuadPart, Convstr, 10);
    wcscpy(String, Convstr);
    ms = String + wcslen(String);
    LoadString(GetModuleHandle(NULL), IDS_MS, ms, Length - (ms - String));
}

/* Locate the ICMP data and print it. Returns TRUE if the packet was good,
   FALSE if not */
static BOOL DecodeResponse(PCHAR buffer, UINT size, PSOCKADDR_IN from)
{
    PIPv4_HEADER      IpHeader;
    PICMP_ECHO_PACKET Icmp;
    UINT              IphLength;
    WCHAR             Time[100];
    LARGE_INTEGER     RelativeTime;
    LARGE_INTEGER     LargeTime;
    WCHAR             Sign[2];
    WCHAR wfromIP[16];

    IpHeader = (PIPv4_HEADER)buffer;

    IphLength = IpHeader->IHL * 4;

    if (size  < IphLength + ICMP_MINSIZE)
    {
#ifndef NDEBUG
        printf("Bad size (0x%X < 0x%X)\n", size, IphLength + ICMP_MINSIZE);
#endif /* !NDEBUG */
        return FALSE;
    }

    Icmp = (PICMP_ECHO_PACKET)(buffer + IphLength);

    if (Icmp->Icmp.Type != ICMPMSG_ECHOREPLY)
    {
#ifndef NDEBUG
        printf("Bad ICMP type (0x%X should be 0x%X)\n", Icmp->Icmp.Type, ICMPMSG_ECHOREPLY);
#endif /* !NDEBUG */
        return FALSE;
    }

    if (Icmp->Icmp.Id != (USHORT)GetCurrentProcessId())
    {
#ifndef NDEBUG
        printf("Bad ICMP id (0x%X should be 0x%X)\n", Icmp->Icmp.Id, (USHORT)GetCurrentProcessId());
#endif /* !NDEBUG */
        return FALSE;
    }

    if (from->sin_addr.s_addr != Target.sin_addr.s_addr)
    {
#ifndef NDEBUG
        printf("Bad source address (%s should be %s)\n", inet_ntoa(from->sin_addr), inet_ntoa(Target.sin_addr));
#endif /* !NDEBUG */
        return FALSE;
    }

    QueryTime(&LargeTime);

    RelativeTime.QuadPart = (LargeTime.QuadPart - SentTime.QuadPart);

    if ((RelativeTime.QuadPart / TicksPerMs.QuadPart) < 1)
    {
        wcscpy(Sign, L"<");
        LoadString(GetModuleHandle(NULL), IDS_1MS, Time, sizeof(Time) / sizeof(WCHAR));
    }
    else
    {
        wcscpy(Sign, L"=");
        TimeToMsString(Time, sizeof(Time) / sizeof(WCHAR), RelativeTime);
    }


    swprintf(wfromIP, L"%d.%d.%d.%d", from->sin_addr.S_un.S_un_b.s_b1,\
                                      from->sin_addr.S_un.S_un_b.s_b2,\
                                      from->sin_addr.S_un.S_un_b.s_b3,\
                                      from->sin_addr.S_un.S_un_b.s_b4);
    FormatOutput(IDS_REPLY_FROM, wfromIP,\
                 size - IphLength - (int)sizeof(ICMP_ECHO_PACKET),\
                 Sign, Time, IpHeader->TTL);

    if (RelativeTime.QuadPart < MinRTT.QuadPart || !MinRTTSet)
    {
        MinRTT.QuadPart = RelativeTime.QuadPart;
        MinRTTSet = TRUE;
    }
    if (RelativeTime.QuadPart > MaxRTT.QuadPart)
        MaxRTT.QuadPart = RelativeTime.QuadPart;

    SumRTT.QuadPart += RelativeTime.QuadPart;

    return TRUE;
}

/* Send and receive one ping */
static BOOL Ping(VOID)
{
    INT                 Status;
    SOCKADDR            From;
    INT                 Length;
    PVOID               Buffer;
    UINT                Size;
    PICMP_ECHO_PACKET   Packet;

    /* Account for extra space for IP header when packet is received */
    Size   = DataSize + 128;
    Buffer = GlobalAlloc(0, Size);
    if (!Buffer)
    {
        FormatOutput(IDS_NOT_ENOUGH_RESOURCES);
        return FALSE;
    }

    ZeroMemory(Buffer, Size);
    Packet = (PICMP_ECHO_PACKET)Buffer;

    /* Assemble ICMP echo request packet */
    Packet->Icmp.Type     = ICMPMSG_ECHOREQUEST;
    Packet->Icmp.Code     = 0;
    Packet->Icmp.Id       = (USHORT)GetCurrentProcessId();
    Packet->Icmp.SeqNum   = htons((USHORT)CurrentSeqNum);
    Packet->Icmp.Checksum = 0;

    /* Calculate checksum for ICMP header and data area */
    Packet->Icmp.Checksum = Checksum((PUSHORT)&Packet->Icmp, sizeof(ICMP_ECHO_PACKET) + DataSize);

    CurrentSeqNum++;

    /* Send ICMP echo request */

    FD_ZERO(&Fds);
    FD_SET(IcmpSock, &Fds);
    Timeval.tv_sec  = Timeout / 1000;
    Timeval.tv_usec = Timeout % 1000;
    Status = select(0, NULL, &Fds, NULL, &Timeval);
    if ((Status != SOCKET_ERROR) && (Status != 0))
    {

#ifndef NDEBUG
        printf("Sending packet\n");
        DisplayBuffer(Buffer, sizeof(ICMP_ECHO_PACKET) + DataSize);
        printf("\n");
#endif /* !NDEBUG */

        Status = sendto(IcmpSock, Buffer, sizeof(ICMP_ECHO_PACKET) + DataSize,
            0, (SOCKADDR*)&Target, sizeof(Target));
        QueryTime(&SentTime);
        SentCount++;
    }
    if (Status == SOCKET_ERROR)
    {
        if (WSAGetLastError() == WSAEHOSTUNREACH)
            FormatOutput(IDS_DEST_UNREACHABLE);
        else
            FormatOutput(IDS_COULD_NOT_TRANSMIT, WSAGetLastError());
        GlobalFree(Buffer);
        return FALSE;
    }

    /* Expect to receive ICMP echo reply */
    FD_ZERO(&Fds);
    FD_SET(IcmpSock, &Fds);
    Timeval.tv_sec  = Timeout / 1000;
    Timeval.tv_usec = Timeout % 1000;

    do {
        Status = select(0, &Fds, NULL, NULL, &Timeval);
        if ((Status != SOCKET_ERROR) && (Status != 0))
        {
            Length = sizeof(From);
            Status = recvfrom(IcmpSock, Buffer, Size, 0, &From, &Length);

#ifndef NDEBUG
            printf("Received packet\n");
            DisplayBuffer(Buffer, Status);
            printf("\n");
#endif /* !NDEBUG */
        }
        else
            LostCount++;
        if (Status == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAETIMEDOUT)
            {
                FormatOutput(IDS_COULD_NOT_RECV, WSAGetLastError());
                GlobalFree(Buffer);
                return FALSE;
            }
            Status = 0;
        }

        if (Status == 0)
        {
            FormatOutput(IDS_REQUEST_TIMEOUT);
            GlobalFree(Buffer);
            return TRUE;
        }

    } while (!DecodeResponse(Buffer, Status, (PSOCKADDR_IN)&From));

    GlobalFree(Buffer);
    return TRUE;
}


/* Program entry point */
int wmain(int argc, LPWSTR argv[])
{
    UINT Count;
    WCHAR MinTime[20];
    WCHAR MaxTime[20];
    WCHAR AvgTime[20];

    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    Reset();

    if ((ParseCmdline(argc, argv)) && (Setup()))
    {

        FormatOutput(IDS_PING_WITH_BYTES, TargetName, TargetIP, DataSize);

        Count = 0;
        while ((NeverStop) || (Count < PingCount))
        {
            Ping();
            Count++;
            if((NeverStop) || (Count < PingCount))
                Sleep(Timeout);
        };

        Cleanup();

        /* Calculate avarage round trip time */
        if ((SentCount - LostCount) > 0)
            AvgRTT.QuadPart = SumRTT.QuadPart / (SentCount - LostCount);
        else
            AvgRTT.QuadPart = 0;

        /* Calculate loss percent */
        Count = SentCount ? (LostCount * 100) / SentCount : 0;

        if (!MinRTTSet)
            MinRTT = MaxRTT;

        TimeToMsString(MinTime, sizeof(MinTime) / sizeof(WCHAR), MinRTT);
        TimeToMsString(MaxTime, sizeof(MaxTime) / sizeof(WCHAR), MaxRTT);
        TimeToMsString(AvgTime, sizeof(AvgTime) / sizeof(WCHAR), AvgRTT);

        /* Print statistics */
        FormatOutput(IDS_PING_STATISTICS, TargetIP);
        FormatOutput(IDS_PACKETS_SENT_RECEIVED_LOST,\
                     SentCount, SentCount - LostCount, LostCount, Count);


        /* Print approximate times or NO approximate times if 100% loss */
        if ((SentCount - LostCount) > 0)
        {
            FormatOutput(IDS_APPROXIMATE_ROUND_TRIP);
            FormatOutput(IDS_MIN_MAX_AVERAGE, MinTime, MaxTime, AvgTime);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

/* EOF */
