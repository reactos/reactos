/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ping utility
 * FILE:        applications/cmdutils/arping/arping.c
 * PURPOSE:     Network test utility
 * PROGRAMMERS: Pierre Schweitzer <pierre@reactos.org>
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
#include <iphlpapi.h>
#include <ws2def.h>
#include <stdio.h>
#include <stdlib.h>

#include "resource.h"

BOOL                NeverStop;
UINT                PingCount;
WCHAR               TargetName[256];
WCHAR               SourceName[256];
DWORD               SourceAddr;
DWORD               TargetAddr;
WCHAR               TargetIP[16];
WCHAR               SourceIP[16];
SOCKADDR_IN         Target;
HANDLE              hStdOutput;
ULONG               Timeout;
LARGE_INTEGER       TicksPerMs;
LARGE_INTEGER       TicksPerUs;
BOOL                UsePerformanceCounter;
UINT                Sent;
UINT                Received;

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

static VOID Usage(VOID)
{
    FormatOutput(IDS_USAGE);
}

static BOOL ParseCmdline(int argc, LPWSTR argv[])
{
    INT i;

    if (argc < 3)
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
                    {
                        FormatOutput(IDS_BAD_OPTION_FORMAT, argv[i]);
                        return FALSE;
                    }
                    break;

                case L's':
                    if (SourceName[0] != 0)
                    {
                        FormatOutput(IDS_BAD_PARAMETER, argv[i]);
                        return FALSE;
                    }

                    if (i + 1 < argc)
                    {
                        wcscpy(SourceName, argv[++i]);
                    }
                    else
                    {
                        FormatOutput(IDS_BAD_OPTION_FORMAT, argv[i]);
                        return FALSE;
                    }
                    break;

                case '?':
                    Usage();
                    return FALSE;

                default:
                    FormatOutput(IDS_BAD_OPTION, argv[i]);
                    return FALSE;
            }
        }
        else
        {
            if (TargetName[0] != 0)
            {
                FormatOutput(IDS_BAD_PARAMETER, argv[i]);
                return FALSE;
            }
            else
            {
                wcscpy(TargetName, argv[i]);
            }
        }
    }

    if (TargetName[0] == 0)
    {
        FormatOutput(IDS_DEST_MUST_BE_SPECIFIED);
        return FALSE;
    }

    if (SourceName[0] == 0)
    {
        FormatOutput(IDS_SRC_MUST_BE_SPECIFIED);
        return FALSE;
    }

    return TRUE;
}

static BOOL WINAPI StopLoop(DWORD dwCtrlType)
{
    NeverStop = FALSE;
    PingCount = 0;

    return TRUE;
}

static BOOL Setup(VOID)
{
    WORD     wVersionRequested;
    WSADATA  WsaData;
    INT      Status;
    PHOSTENT phe;
    CHAR     aTargetName[256];
    IN_ADDR  Target;

    wVersionRequested = MAKEWORD(2, 2);

    Status = WSAStartup(wVersionRequested, &WsaData);
    if (Status != 0)
    {
        FormatOutput(IDS_COULD_NOT_INIT_WINSOCK);
        return FALSE;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, TargetName, -1, aTargetName,
                             sizeof(aTargetName), NULL, NULL))
    {
        FormatOutput(IDS_UNKNOWN_HOST, TargetName);
        return FALSE;
    }

    phe = NULL;
    TargetAddr = inet_addr(aTargetName);
    if (TargetAddr == INADDR_NONE)
    {
        phe = gethostbyname(aTargetName);
        if (phe == NULL)
        {
            FormatOutput(IDS_UNKNOWN_HOST, TargetName);
            return FALSE;
        }

        CopyMemory(&TargetAddr, phe->h_addr, phe->h_length);
    }

    Target.S_un.S_addr = TargetAddr;
    swprintf(TargetIP, L"%d.%d.%d.%d", Target.S_un.S_un_b.s_b1,
                                       Target.S_un.S_un_b.s_b2,
                                       Target.S_un.S_un_b.s_b3,
                                       Target.S_un.S_un_b.s_b4);

    if (!WideCharToMultiByte(CP_ACP, 0, SourceName, -1, aTargetName,
                             sizeof(aTargetName), NULL, NULL))
    {
        FormatOutput(IDS_UNKNOWN_HOST, SourceName);
        return FALSE;
    }

    SourceAddr = inet_addr(aTargetName);
    if (SourceAddr == INADDR_NONE)
    {
        FormatOutput(IDS_UNKNOWN_HOST, SourceName);
        return FALSE;
    }

    SetConsoleCtrlHandler(StopLoop, TRUE);

    return TRUE;
}

static VOID Cleanup(VOID)
{
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

static BOOL Ping(VOID)
{
    LARGE_INTEGER RelativeTime;
    LARGE_INTEGER LargeTime;
    LARGE_INTEGER SentTime;
    DWORD Ret;
    BYTE TargetHW[6];
    ULONG Size;
    WCHAR Sign[2];
    WCHAR Time[100];
    WCHAR StrHwAddr[18];

    QueryTime(&SentTime);
    Size = sizeof(TargetHW);
    memset(TargetHW, 0xff, Size);
    ++Sent;
    Ret = SendARP(TargetAddr, SourceAddr, (PULONG)TargetHW, &Size);
    if (Ret == ERROR_SUCCESS)
    {
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

        swprintf(StrHwAddr, L"%02x:%02x:%02x:%02x:%02x:%02x", TargetHW[0], TargetHW[1],
                                                              TargetHW[2], TargetHW[3],
                                                              TargetHW[4], TargetHW[5]);
        FormatOutput(IDS_REPLY_FROM, TargetIP, StrHwAddr, Sign, Time);
        Received++;

        return TRUE;
    }

    return FALSE;
}

int wmain(int argc, LPWSTR argv[])
{
    UINT Count;
    LARGE_INTEGER PerformanceCounterFrequency;

    PingCount = 4;
    Timeout = 1000;
    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

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

    if (!ParseCmdline(argc, argv) || !Setup())
    {
        return 1;
    }

    FormatOutput(IDS_ARPING_TO_FROM, TargetIP, SourceName);

    Count = 0;
    while (Count < PingCount || NeverStop)
    {
        Ping();
        Count++;
        if (Count < PingCount || NeverStop)
            Sleep(Timeout);
    }

    Cleanup();

    FormatOutput(IDS_ARPING_STATISTICS, Sent, Received);

    return 0;
}
