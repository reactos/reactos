/*
 * Copyright (c) 2015 Tim Crawford
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * PROJECT:     ReactOS Ping Command
 * LICENSE:     MIT
 * FILE:        base/applications/network/ping/ping.c
 * PURPOSE:     Network test utility
 * PROGRAMMERS: Tim Crawford <crawfxrd@gmail.com>
 */

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <conutils.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

#define SIZEOF_ICMP_ERROR 8
#define SIZEOF_IO_STATUS_BLOCK 8
#define DEFAULT_TIMEOUT 1000
#define MAX_SEND_SIZE 65500

static BOOL ParseCmdLine(int argc, PWSTR argv[]);
static BOOL ResolveTarget(PCWSTR target);
static void Ping(void);
static void PrintStats(void);
static BOOL WINAPI ConsoleCtrlHandler(DWORD ControlType);

static HANDLE hIcmpFile = INVALID_HANDLE_VALUE;
static ULONG Timeout = 4000;
static int Family = AF_UNSPEC;
static ULONG RequestSize = 32;
static ULONG PingCount = 4;
static BOOL PingForever = FALSE;
static PADDRINFOW Target = NULL;
static PCWSTR TargetName = NULL;
static WCHAR Address[46];
static WCHAR CanonName[NI_MAXHOST];
static BOOL ResolveAddress = FALSE;

static ULONG RTTMax = 0;
static ULONG RTTMin = 0;
static ULONG RTTTotal = 0;
static ULONG EchosSent = 0;
static ULONG EchosReceived = 0;
static ULONG EchosSuccessful = 0;

static IP_OPTION_INFORMATION IpOptions;

int
wmain(int argc, WCHAR *argv[])
{
    WSADATA wsaData;
    ULONG i;
    DWORD StrLen = 46;
    int Status;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    IpOptions.Ttl = 128;

    if (!ParseCmdLine(argc, argv))
        return 1;

    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
    {
        DPRINT("Failed to set control handler: %lu\n", GetLastError());
        return 1;
    }

    Status = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (Status != 0)
    {
        ConResPrintf(StdErr, IDS_WINSOCK_FAIL, Status);
        return 1;
    }

    if (!ResolveTarget(TargetName))
    {
        WSACleanup();
        return 1;
    }

    if (WSAAddressToStringW(Target->ai_addr, (DWORD)Target->ai_addrlen, NULL, Address, &StrLen) != 0)
    {
        DPRINT("WSAAddressToStringW failed: %d\n", WSAGetLastError());
        FreeAddrInfoW(Target);
        WSACleanup();
        return 1;
    }

    if (Family == AF_INET6)
        hIcmpFile = Icmp6CreateFile();
    else
        hIcmpFile = IcmpCreateFile();


    if (hIcmpFile == INVALID_HANDLE_VALUE)
    {
        DPRINT("IcmpCreateFile failed: %lu\n", GetLastError());
        FreeAddrInfoW(Target);
        WSACleanup();
        return 1;
    }

    if (*CanonName)
        ConResPrintf(StdOut, IDS_PINGING_HOSTNAME, CanonName, Address);
    else
        ConResPrintf(StdOut, IDS_PINGING_ADDRESS, Address);

    ConResPrintf(StdOut, IDS_PING_SIZE, RequestSize);

    Ping();

    i = 1;
    while (i < PingCount)
    {
        Sleep(1000);
        Ping();

        if (!PingForever)
            i++;
    }

    PrintStats();

    IcmpCloseHandle(hIcmpFile);
    FreeAddrInfoW(Target);
    WSACleanup();

    return 0;
}

static
BOOL
ParseCmdLine(int argc, PWSTR argv[])
{
    int i;

    if (argc < 2)
    {
        ConResPrintf(StdOut, IDS_USAGE);
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            switch (argv[i][1])
            {
            case L't':
                PingForever = TRUE;
                break;

            case L'a':
                ResolveAddress = TRUE;
                break;

            case L'n':
            {
                if (i + 1 < argc)
                {
                    PingForever = FALSE;
                    PingCount = wcstoul(argv[++i], NULL, 0);
                    if (PingCount == 0)
                    {
                        ConResPrintf(StdErr, IDS_BAD_VALUE, argv[i - 1], 1, UINT_MAX);
                        return FALSE;
                    }
                }
                else
                {
                    ConResPrintf(StdErr, IDS_MISSING_VALUE, argv[i]);
                    return FALSE;
                }
                break;
            }

            case L'l':
            {
                if (i + 1 < argc)
                {
                    RequestSize = wcstoul(argv[++i], NULL, 0);
                    if (RequestSize > MAX_SEND_SIZE)
                    {
                        ConResPrintf(StdErr, IDS_BAD_VALUE, argv[i - 1], 0, MAX_SEND_SIZE);
                        return FALSE;
                    }
                }
                else
                {
                    ConResPrintf(StdErr, IDS_MISSING_VALUE, argv[i]);
                    return FALSE;
                }
                break;
            }

            case L'f':
            {
                if (Family == AF_INET6)
                {
                    ConResPrintf(StdErr, IDS_WRONG_FAMILY, argv[i], L"IPv4");
                    return FALSE;
                }

                Family = AF_INET;
                IpOptions.Flags |= IP_FLAG_DF;
                break;
            }

            case L'i':
            {
                if (i + 1 < argc)
                {
                    ULONG Ttl = wcstoul(argv[++i], NULL, 0);

                    if ((Ttl == 0) || (Ttl > UCHAR_MAX))
                    {
                        ConResPrintf(StdErr, IDS_BAD_VALUE, argv[i - 1], 1, UCHAR_MAX);
                        return FALSE;
                    }

                    IpOptions.Ttl = (UCHAR)Ttl;
                }
                else
                {
                    ConResPrintf(StdErr, IDS_MISSING_VALUE, argv[i]);
                    return FALSE;
                }
                break;
            }

            case L'v':
            {
                if (Family == AF_INET6)
                {
                    ConResPrintf(StdErr, IDS_WRONG_FAMILY, argv[i], L"IPv4");
                    return FALSE;
                }

                Family = AF_INET;

                if (i + 1 < argc)
                {
                    /* This option has been deprecated. Don't do anything. */
                    i++;
                }
                else
                {
                    ConResPrintf(StdErr, IDS_MISSING_VALUE, argv[i]);
                    return FALSE;
                }

                break;
            }

            case L'w':
            {
                if (i + 1 < argc)
                {
                    Timeout = wcstoul(argv[++i], NULL, 0);
                    if (Timeout < DEFAULT_TIMEOUT)
                        Timeout = DEFAULT_TIMEOUT;
                }
                else
                {
                    ConResPrintf(StdErr, IDS_MISSING_VALUE, argv[i]);
                    return FALSE;
                }
                break;
            }

            case L'R':
            {
                if (Family == AF_INET)
                {
                    ConResPrintf(StdErr, IDS_WRONG_FAMILY, argv[i], L"IPv6");
                    return FALSE;
                }

                Family = AF_INET6;

                /* This option has been deprecated. Don't do anything. */
                break;
            }

            case L'4':
            {
                if (Family == AF_INET6)
                {
                    ConResPrintf(StdErr, IDS_WRONG_FAMILY, argv[i], L"IPv4");
                    return FALSE;
                }

                Family = AF_INET;
                break;
            }

            case L'6':
            {
                if (Family == AF_INET)
                {
                    ConResPrintf(StdErr, IDS_WRONG_FAMILY, argv[i], L"IPv6");
                    return FALSE;
                }

                Family = AF_INET6;
                break;
            }

            case L'?':
                ConResPrintf(StdOut, IDS_USAGE);
                return FALSE;

            default:
                ConResPrintf(StdErr, IDS_BAD_OPTION, argv[i]);
                ConResPrintf(StdErr, IDS_USAGE);
                return FALSE;
            }
        }
        else
        {
            if (TargetName != NULL)
            {
                ConResPrintf(StdErr, IDS_BAD_PARAMETER, argv[i]);
                return FALSE;
            }

            TargetName = argv[i];
        }
    }

    if (TargetName == NULL)
    {
        ConResPrintf(StdErr, IDS_MISSING_ADDRESS);
        return FALSE;
    }

    return TRUE;
}

static
BOOL
ResolveTarget(PCWSTR target)
{
    ADDRINFOW hints;
    int Status;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = Family;
    hints.ai_flags = AI_NUMERICHOST;

    Status = GetAddrInfoW(target, NULL, &hints, &Target);
    if (Status != 0)
    {
        hints.ai_flags = AI_CANONNAME;

        Status = GetAddrInfoW(target, NULL, &hints, &Target);
        if (Status != 0)
        {
            ConResPrintf(StdOut, IDS_UNKNOWN_HOST, target);
            return FALSE;
        }

        wcsncpy(CanonName, Target->ai_canonname, wcslen(Target->ai_canonname));
    }
    else if (ResolveAddress)
    {
        Status = GetNameInfoW(Target->ai_addr, Target->ai_addrlen,
                              CanonName, _countof(CanonName),
                              NULL, 0,
                              NI_NAMEREQD);
        if (Status != 0)
        {
            DPRINT("GetNameInfoW failed: %d\n", WSAGetLastError());
        }
    }

    Family = Target->ai_family;

    return TRUE;
}

static
void
Ping(void)
{
    PVOID ReplyBuffer = NULL;
    PVOID SendBuffer = NULL;
    DWORD ReplySize = 0;
    DWORD Status;

    SendBuffer = malloc(RequestSize);
    if (SendBuffer == NULL)
    {
        ConResPrintf(StdErr, IDS_NO_RESOURCES);
        exit(1);
    }

    ZeroMemory(SendBuffer, RequestSize);

    if (Family == AF_INET6)
        ReplySize += sizeof(ICMPV6_ECHO_REPLY);
    else
        ReplySize += sizeof(ICMP_ECHO_REPLY);

    ReplySize += RequestSize + SIZEOF_ICMP_ERROR + SIZEOF_IO_STATUS_BLOCK;

    ReplyBuffer = malloc(ReplySize);
    if (ReplyBuffer == NULL)
    {
        ConResPrintf(StdErr, IDS_NO_RESOURCES);
        free(SendBuffer);
        exit(1);
    }

    ZeroMemory(ReplyBuffer, ReplySize);

    EchosSent++;

    if (Family == AF_INET6)
    {
        struct sockaddr_in6 Source;

        ZeroMemory(&Source, sizeof(Source));
        Source.sin6_family = AF_INET6;

        Status = Icmp6SendEcho2(hIcmpFile, NULL, NULL, NULL,
                                &Source,
                                (struct sockaddr_in6 *)Target->ai_addr,
                                SendBuffer, (USHORT)RequestSize, &IpOptions,
                                ReplyBuffer, ReplySize, Timeout);
    }
    else
    {
        Status = IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
                               ((PSOCKADDR_IN)Target->ai_addr)->sin_addr.s_addr,
                               SendBuffer, (USHORT)RequestSize, &IpOptions,
                               ReplyBuffer, ReplySize, Timeout);
    }

    free(SendBuffer);

    if (Status == 0)
    {
        Status = GetLastError();
        switch (Status)
        {
        case IP_DEST_HOST_UNREACHABLE:
            ConResPrintf(StdOut, IDS_DEST_HOST_UNREACHABLE);
            break;

        case IP_DEST_NET_UNREACHABLE:
            ConResPrintf(StdOut, IDS_DEST_NET_UNREACHABLE);
            break;

        case IP_REQ_TIMED_OUT:
            ConResPrintf(StdOut, IDS_REQUEST_TIMED_OUT);
            break;

        default:
            ConResPrintf(StdOut, IDS_TRANSMIT_FAILED, Status);
            break;
        }
    }
    else
    {
        EchosReceived++;

        ConResPrintf(StdOut, IDS_REPLY_FROM, Address);

        if (Family == AF_INET6)
        {
            PICMPV6_ECHO_REPLY pEchoReply;

            pEchoReply = (PICMPV6_ECHO_REPLY)ReplyBuffer;

            switch (pEchoReply->Status)
            {
            case IP_SUCCESS:
            {
                EchosSuccessful++;

                if (pEchoReply->RoundTripTime == 0)
                    ConResPrintf(StdOut, IDS_REPLY_TIME_0MS);
                else
                    ConResPrintf(StdOut, IDS_REPLY_TIME_MS, pEchoReply->RoundTripTime);

                if (pEchoReply->RoundTripTime < RTTMin || RTTMin == 0)
                    RTTMin = pEchoReply->RoundTripTime;

                if (pEchoReply->RoundTripTime > RTTMax || RTTMax == 0)
                    RTTMax = pEchoReply->RoundTripTime;

                ConPuts(StdOut, L"\n");

                RTTTotal += pEchoReply->RoundTripTime;
                break;
            }

            case IP_TTL_EXPIRED_TRANSIT:
                ConResPrintf(StdOut, IDS_TTL_EXPIRED);
                break;

            default:
                ConResPrintf(StdOut, IDS_REPLY_STATUS, pEchoReply->Status);
                break;
            }
        }
        else
        {
            PICMP_ECHO_REPLY pEchoReply;

            pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;

            switch (pEchoReply->Status)
            {
            case IP_SUCCESS:
            {
                EchosSuccessful++;

                ConResPrintf(StdOut, IDS_REPLY_BYTES, pEchoReply->DataSize);

                if (pEchoReply->RoundTripTime == 0)
                    ConResPrintf(StdOut, IDS_REPLY_TIME_0MS);
                else
                    ConResPrintf(StdOut, IDS_REPLY_TIME_MS, pEchoReply->RoundTripTime);

                ConResPrintf(StdOut, IDS_REPLY_TTL, pEchoReply->Options.Ttl);

                if (pEchoReply->RoundTripTime < RTTMin || RTTMin == 0)
                    RTTMin = pEchoReply->RoundTripTime;

                if (pEchoReply->RoundTripTime > RTTMax || RTTMax == 0)
                    RTTMax = pEchoReply->RoundTripTime;

                RTTTotal += pEchoReply->RoundTripTime;
                break;
            }

            case IP_TTL_EXPIRED_TRANSIT:
                ConResPrintf(StdOut, IDS_TTL_EXPIRED);
                break;

            default:
                ConResPrintf(StdOut, IDS_REPLY_STATUS, pEchoReply->Status);
                break;
            }
        }
    }

    free(ReplyBuffer);
}

static
void
PrintStats(void)
{
    ULONG EchosLost = EchosSent - EchosReceived;
    ULONG PercentLost = (ULONG)((EchosLost / (double)EchosSent) * 100.0);

    ConResPrintf(StdOut, IDS_STATISTICS, Address, EchosSent, EchosReceived, EchosLost, PercentLost);

    if (EchosSuccessful > 0)
    {
        ULONG RTTAverage = RTTTotal / EchosSuccessful;
        ConResPrintf(StdOut, IDS_APPROXIMATE_RTT, RTTMin, RTTMax, RTTAverage);
    }
}

static
BOOL
WINAPI
ConsoleCtrlHandler(DWORD ControlType)
{
    switch (ControlType)
    {
    case CTRL_C_EVENT:
        PrintStats();
        ConResPrintf(StdOut, IDS_CTRL_C);
        return FALSE;

    case CTRL_BREAK_EVENT:
        PrintStats();
        ConResPrintf(StdOut, IDS_CTRL_BREAK);
        return TRUE;

    case CTRL_CLOSE_EVENT:
        PrintStats();
        return FALSE;

    default:
        return FALSE;
    }
}
