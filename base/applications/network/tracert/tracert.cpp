/*
 * PROJECT:     ReactOS trace route utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/tracert/tracert.cpp
 * PURPOSE:     Trace network paths through networks
 * COPYRIGHT:   Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#ifdef __REACTOS__
#define USE_CONUTILS
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#define _INC_WINDOWS
#include <stdlib.h>
#include <winsock2.h>
#include <conutils.h>
#else
#include <winsock2.h>
#include <Windows.h>
#endif
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <strsafe.h>
#include "resource.h"

#define SIZEOF_ICMP_ERROR       8
#define SIZEOF_IO_STATUS_BLOCK  8
#define PACKET_SIZE             32
#define MAX_IPADDRESS           32
#define NUM_OF_PINGS            3

struct TraceInfo
{
    bool ResolveAddresses;
    ULONG MaxHops;
    ULONG Timeout;
    WCHAR HostName[NI_MAXHOST];
    WCHAR TargetIP[MAX_IPADDRESS];
    int Family;

    HANDLE hIcmpFile;
    PADDRINFOW Target;

} Info = { 0 };



#ifndef USE_CONUTILS
static
INT
LengthOfStrResource(
    _In_ HINSTANCE hInst,
    _In_ UINT uID
)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL) return -1;

    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = (WCHAR*)LockResource(hRes)))
    {
        UINT x;
        uID &= 0xF;
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }
        return (int)(*lpStr);
    }
    return -1;
}

static
INT
AllocAndLoadString(
    _In_ UINT uID,
    _Out_ LPWSTR *lpTarget
)
{
    HMODULE hInst;
    INT Length;

    hInst = GetModuleHandleW(NULL);
    Length = LengthOfStrResource(hInst, uID);
    if (Length++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         Length * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, Length)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

static
INT
OutputText(
    _In_ UINT uID,
    ...)
{
    LPWSTR Format;
    DWORD Ret = 0;
    va_list lArgs;

    if (AllocAndLoadString(uID, &Format) > 0)
    {
        va_start(lArgs, uID);

        LPWSTR Buffer;
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             Format,
                             0,
                             0,
                             (LPWSTR)&Buffer,
                             0,
                             &lArgs);
        va_end(lArgs);

        if (Ret)
        {
            wprintf(Buffer);
            LocalFree(Buffer);
        }
        LocalFree((HLOCAL)Format);
    }

    return Ret;
}
#else
#define OutputText(Id, ...) ConResMsgPrintfEx(StdOut, NULL, 0, Id, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), ##__VA_ARGS__)
#endif //USE_CONUTILS

static
VOID
Usage()
{
    OutputText(IDS_USAGE);
}

static ULONG
GetULONG(
    _In_z_ LPWSTR String
)
{
    ULONG Length;
    Length = wcslen(String);

    ULONG i = 0;
    while ((i < Length) && ((String[i] < L'0') || (String[i] > L'9'))) i++;
    if ((i >= Length) || ((String[i] < L'0') || (String[i] > L'9')))
    {
        return (ULONG)-1;
    }

    LPWSTR StopString;
    return wcstoul(&String[i], &StopString, 10);
}

static bool
ResolveTarget()
{
    ADDRINFOW Hints;
    ZeroMemory(&Hints, sizeof(Hints));
    Hints.ai_family = Info.Family;
    Hints.ai_flags = AI_CANONNAME;

    int Status;
    Status = GetAddrInfoW(Info.HostName,
                          NULL,
                          &Hints,
                          &Info.Target);
    if (Status != 0)
    {
        return false;
    }

    Status = GetNameInfoW(Info.Target->ai_addr,
                          Info.Target->ai_addrlen,
                          Info.TargetIP,
                          MAX_IPADDRESS,
                          NULL,
                          0,
                          NI_NUMERICHOST);
    if (Status != 0)
    {
        return false;
    }

    return true;
}

static bool
PrintHopInfo(_In_ PVOID Buffer)
{
    SOCKADDR_IN6 SockAddrIn6 = { 0 };
    SOCKADDR_IN SockAddrIn = { 0 };
    PSOCKADDR SockAddr;
    socklen_t Size;

    if (Info.Family == AF_INET6)
    {
        PIPV6_ADDRESS_EX Ipv6Addr = (PIPV6_ADDRESS_EX)Buffer;
        SockAddrIn6.sin6_family = AF_INET6;
        CopyMemory(SockAddrIn6.sin6_addr.u.Word, Ipv6Addr->sin6_addr, sizeof(SockAddrIn6.sin6_addr));
        //SockAddrIn6.sin6_addr = Ipv6Addr->sin6_addr;
        SockAddr = (PSOCKADDR)&SockAddrIn6;
        Size = sizeof(SOCKADDR_IN6);

    }
    else
    {
        IPAddr *Address = (IPAddr *)Buffer;
        SockAddrIn.sin_family = AF_INET;
        SockAddrIn.sin_addr.S_un.S_addr = *Address;
        SockAddr = (PSOCKADDR)&SockAddrIn;
        Size = sizeof(SOCKADDR_IN);
    }

    INT Status;
    bool Resolved = false;
    WCHAR HostName[NI_MAXHOST];
    if (Info.ResolveAddresses)
    {
        Status = GetNameInfoW(SockAddr,
                              Size,
                              HostName,
                              NI_MAXHOST,
                              NULL,
                              0,
                              NI_NAMEREQD);
        if (Status == 0)
        {
            Resolved = true;
        }
    }

    WCHAR IpAddress[MAX_IPADDRESS];
    Status = GetNameInfoW(SockAddr,
                          Size,
                          IpAddress,
                          MAX_IPADDRESS,
                          NULL,
                          0,
                          NI_NUMERICHOST);
    if (Status == 0)
    {
        if (Resolved)
        {
            OutputText(IDS_HOP_RES_INFO, HostName, IpAddress);
        }
        else
        {
            OutputText(IDS_HOP_IP_INFO, IpAddress);
        }
    }

    return (Status == 0);
}

static bool
DecodeResponse(
    _In_ PVOID ReplyBuffer,
    _In_ bool OutputHopAddress,
    _Out_ bool& FoundTarget
)
{
    ULONG RoundTripTime;
    PVOID AddressInfo;
    ULONG Status;

    if (Info.Family == AF_INET6)
    {
        PICMPV6_ECHO_REPLY EchoReplyV6;
        EchoReplyV6 = (PICMPV6_ECHO_REPLY)ReplyBuffer;
        Status = EchoReplyV6->Status;
        RoundTripTime = EchoReplyV6->RoundTripTime;
        AddressInfo = &EchoReplyV6->Address;
    }
    else
    {
        PICMP_ECHO_REPLY EchoReplyV4;
        EchoReplyV4 = (PICMP_ECHO_REPLY)ReplyBuffer;
        Status = EchoReplyV4->Status;
        RoundTripTime = EchoReplyV4->RoundTripTime;
        AddressInfo = &EchoReplyV4->Address;
    }

    switch (Status)
    {
    case IP_SUCCESS:
    case IP_TTL_EXPIRED_TRANSIT:
        if (RoundTripTime)
        {
            OutputText(IDS_HOP_TIME, RoundTripTime);
        }
        else
        {
            OutputText(IDS_HOP_ZERO);
        }
        break;

    case IP_DEST_HOST_UNREACHABLE:
    case IP_DEST_NET_UNREACHABLE:
        FoundTarget = true;
        PrintHopInfo(AddressInfo);
        OutputText(IDS_HOP_RESPONSE);
        if (Status == IP_DEST_HOST_UNREACHABLE)
        {
            OutputText(IDS_DEST_HOST_UNREACHABLE);
        }
        else if (Status == IP_DEST_NET_UNREACHABLE)
        {
            OutputText(IDS_DEST_NET_UNREACHABLE);
        }
        return true;

    case IP_REQ_TIMED_OUT:
        OutputText(IDS_TIMEOUT);
        break;

    case IP_GENERAL_FAILURE:
        OutputText(IDS_GEN_FAILURE);
        return false;

    default:
        OutputText(IDS_TRANSMIT_FAILED, Status);
        return false;
    }

    if (OutputHopAddress)
    {
        if (Status == IP_SUCCESS)
        {
            FoundTarget = true;
        }
        if (Status == IP_TTL_EXPIRED_TRANSIT || Status == IP_SUCCESS)
        {
            PrintHopInfo(AddressInfo);
            OutputText(IDS_LINEBREAK);
        }
        else if (Status == IP_REQ_TIMED_OUT)
        {
            OutputText(IDS_REQ_TIMED_OUT);
        }
    }

    return true;
}

static bool
RunTraceRoute()
{
    bool Success = false;
    Success = ResolveTarget();
    if (!Success)
    {
        OutputText(IDS_UNABLE_RESOLVE, Info.HostName);
        return false;
    }

    BYTE SendBuffer[PACKET_SIZE];
    ICMPV6_ECHO_REPLY ReplyBufferv6;
#ifdef _WIN64
    ICMP_ECHO_REPLY32 ReplyBufferv432;
#else
    ICMP_ECHO_REPLY ReplyBufferv4;
#endif
    PVOID ReplyBuffer;

    DWORD ReplySize = PACKET_SIZE + SIZEOF_ICMP_ERROR + SIZEOF_IO_STATUS_BLOCK;
    if (Info.Family == AF_INET6)
    {
        ReplyBuffer = &ReplyBufferv6;
        ReplySize += sizeof(ICMPV6_ECHO_REPLY);
    }
    else
    {
#ifdef _WIN64
        ReplyBuffer = &ReplyBufferv432;
        ReplySize += sizeof(ICMP_ECHO_REPLY32);
#else
        ReplyBuffer = &ReplyBufferv4;
        ReplySize += sizeof(ICMP_ECHO_REPLY);
#endif
    }

    if (Info.Family == AF_INET6)
    {
        Info.hIcmpFile = Icmp6CreateFile();
    }
    else
    {
        Info.hIcmpFile = IcmpCreateFile();
    }
    if (Info.hIcmpFile == INVALID_HANDLE_VALUE)
    {
        FreeAddrInfoW(Info.Target);
        return false;
    }

    OutputText(IDS_TRACE_INFO, Info.HostName, Info.TargetIP, Info.MaxHops);

    IP_OPTION_INFORMATION IpOptionInfo;
    ZeroMemory(&IpOptionInfo, sizeof(IpOptionInfo));

    bool Quit = false;
    ULONG HopCount = 1;
    bool FoundTarget = false;
    while ((HopCount <= Info.MaxHops) && (FoundTarget == false) && (Quit == false))
    {
        OutputText(IDS_HOP_COUNT, HopCount);

        for (int Ping = 1; Ping <= NUM_OF_PINGS; Ping++)
        {
            IpOptionInfo.Ttl = static_cast<UCHAR>(HopCount);

            if (Info.Family == AF_INET6)
            {
                struct sockaddr_in6 Source;

                ZeroMemory(&Source, sizeof(Source));
                Source.sin6_family = AF_INET6;

                (void)Icmp6SendEcho2(Info.hIcmpFile,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &Source,
                                     (struct sockaddr_in6 *)Info.Target->ai_addr,
                                     SendBuffer,
                                     (USHORT)PACKET_SIZE,
                                     &IpOptionInfo,
                                     ReplyBuffer,
                                     ReplySize,
                                     Info.Timeout);
            }
            else
            {
                (void)IcmpSendEcho2(Info.hIcmpFile,
                                     NULL,
                                     NULL,
                                     NULL,
                                     ((PSOCKADDR_IN)Info.Target->ai_addr)->sin_addr.s_addr,
                                     SendBuffer,
                                     (USHORT)PACKET_SIZE,
                                     &IpOptionInfo,
                                     ReplyBuffer,
                                     ReplySize,
                                     Info.Timeout);
            }

            if (DecodeResponse(ReplyBuffer, (Ping == NUM_OF_PINGS), FoundTarget) == false)
            {
                Quit = true;
                break;
            }

            if (FoundTarget)
            {
                Success = true;
                break;
            }
        }

        HopCount++;
        Sleep(100);
    }

    OutputText(IDS_TRACE_COMPLETE);

    FreeAddrInfoW(Info.Target);
    if (Info.hIcmpFile)
    {
        IcmpCloseHandle(Info.hIcmpFile);
    }

    return Success;
}

static bool
ParseCmdline(int argc, wchar_t *argv[])
{
    if (argc < 2)
    {
        Usage();
        return false;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
            case 'd':
                Info.ResolveAddresses = FALSE;
                break;

            case 'h':
                Info.MaxHops = GetULONG(argv[++i]);
                break;

            case 'j':
                printf("-j is not yet implemented.\n");
                return false;

            case 'w':
                Info.Timeout = GetULONG(argv[++i]);
                break;

            case '4':
                Info.Family = AF_INET;
                break;

            case '6':
                Info.Family = AF_INET6;
                break;

            default:
            {
                OutputText(IDS_INVALID_OPTION, argv[i]);
                Usage();
                return false;
            }
            }
        }
        else
        {
            StringCchCopyW(Info.HostName, NI_MAXHOST, argv[i]);
            break;
        }
    }

    return true;
}

EXTERN_C
int wmain(int argc, wchar_t *argv[])
{
#ifdef USE_CONUTILS
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();
#endif

    Info.ResolveAddresses = true;
    Info.MaxHops = 30;
    Info.Timeout = 4000;
    Info.Family = AF_UNSPEC;

    if (!ParseCmdline(argc, argv))
    {
        return 1;
    }

    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData))
    {
        return 1;
    }

    bool Success;
    Success = RunTraceRoute();

    WSACleanup();

    return Success ? 0 : 1;
}
