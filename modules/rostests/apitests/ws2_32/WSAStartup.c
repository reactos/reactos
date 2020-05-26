/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for WSAStartup
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "ws2_32.h"
#include <apitest_guard.h>

static
BOOLEAN
CheckStringBuffer(
    PCSTR Buffer,
    SIZE_T MaximumLength,
    PCSTR Expected,
    UCHAR Fill)
{
    SIZE_T Length = strlen(Expected);
    SIZE_T EqualLength;
    BOOLEAN Result = TRUE;
    SIZE_T i;

    EqualLength = RtlCompareMemory(Buffer, Expected, Length);
    if (EqualLength != Length)
    {
        ok(0, "String is '%.*s', expected '%s'\n", (int)MaximumLength, Buffer, Expected);
        Result = FALSE;
    }

    if (Buffer[Length] != ANSI_NULL)
    {
        ok(0, "Not null terminated\n");
        Result = FALSE;
    }

    /* The function nulls the rest of the buffer! */
    for (i = Length + 1; i < MaximumLength; i++)
    {
        UCHAR Char = ((PUCHAR)Buffer)[i];
        if (Char != Fill)
        {
            ok(0, "Found 0x%x at offset %lu, expected 0x%x\n", Char, (ULONG)i, Fill);
            /* Don't count this as a failure unless the string was actually wrong */
            //Result = FALSE;
            /* Don't flood the log */
            break;
        }
    }

    return Result;
}

static
BOOLEAN
IsWinsockInitialized(VOID)
{
    struct hostent *Hostent;

    Hostent = gethostbyname("localhost");
    if (!Hostent)
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    return Hostent != NULL;
}

static
BOOLEAN
AreLegacyFunctionsSupported(VOID)
{
    int Error;

    Error = WSACancelBlockingCall();
    ok(Error == SOCKET_ERROR, "Error = %d\n", Error);
    ok(WSAGetLastError() == WSAEOPNOTSUPP ||
       WSAGetLastError() == WSAEINVAL, "WSAGetLastError = %d\n", WSAGetLastError());

    return WSAGetLastError() != WSAEOPNOTSUPP;
}

START_TEST(WSAStartup)
{
    BOOLEAN Okay;
    LPWSADATA WsaData;
    int Error;
    struct
    {
        WORD Version;
        BOOLEAN ExpectedSuccess;
        WORD ExpectedVersion;
    } Tests[] =
    {
        { MAKEWORD(0, 0),   FALSE, MAKEWORD(2, 2) },
        { MAKEWORD(0, 9),   FALSE, MAKEWORD(2, 2) },
        { MAKEWORD(1, 0),   TRUE },
        { MAKEWORD(1, 1),   TRUE },
        { MAKEWORD(1, 2),   TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(1, 15),  TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(1, 255), TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(2, 0),   TRUE },
        { MAKEWORD(2, 1),   TRUE },
        { MAKEWORD(2, 2),   TRUE },
        { MAKEWORD(2, 3),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(2, 15),  TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(2, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 0),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 1),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 2),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 3),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 15),  TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(15, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(255, 255), TRUE, MAKEWORD(2, 2) },
    };
    const INT TestCount = sizeof(Tests) / sizeof(Tests[0]);
    INT i;

    ok(!IsWinsockInitialized(), "Winsock unexpectedly initialized\n");

    /* parameter checks */
    StartSeh()
        Error = WSAStartup(0, NULL);
        ok_dec(Error, WSAVERNOTSUPPORTED);
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Error = WSAStartup(MAKEWORD(2, 2), NULL);
        ok_dec(Error, WSAEFAULT);
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    EndSeh(STATUS_SUCCESS);
    ok(!IsWinsockInitialized(), "Winsock unexpectedly initialized\n");

    WsaData = AllocateGuarded(sizeof(*WsaData));
    if (!WsaData)
    {
        skip("Out of memory\n");
        return;
    }

    /* test different version */
    for (i = 0; i < TestCount; i++)
    {
        trace("%d: version %d.%d\n",
              i, LOBYTE(Tests[i].Version), HIBYTE(Tests[i].Version));
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSANO_RECOVERY;
        StartSeh()
            Error = WSAStartup(Tests[i].Version, WsaData);
        EndSeh(STATUS_SUCCESS);
        if (Error)
        {
            ok(!Tests[i].ExpectedSuccess, "WSAStartup failed unexpectedly\n");
            ok_dec(Error, WSAVERNOTSUPPORTED);
            ok_dec(WSAGetLastError(), WSANOTINITIALISED);
            ok(!IsWinsockInitialized(), "Winsock unexpectedly initialized\n");
        }
        else
        {
            ok(Tests[i].ExpectedSuccess, "WSAStartup succeeded unexpectedly\n");
            ok_dec(WSAGetLastError(), 0);
            ok(IsWinsockInitialized(), "Winsock not initialized despite success\n");
            if (LOBYTE(Tests[i].Version) < 2)
                ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            else
                ok(!AreLegacyFunctionsSupported(), "Legacy function succeeded\n");
            WSACleanup();
            ok(!IsWinsockInitialized(), "Winsock still initialized after cleanup\n");
        }
        if (Tests[i].ExpectedVersion)
            ok_hex(WsaData->wVersion, Tests[i].ExpectedVersion);
        else
            ok_hex(WsaData->wVersion, Tests[i].Version);
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        Okay = CheckStringBuffer(WsaData->szDescription,
                                 sizeof(WsaData->szDescription),
                                 "WinSock 2.0",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        Okay = CheckStringBuffer(WsaData->szSystemStatus,
                                 sizeof(WsaData->szSystemStatus),
                                 "Running",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        if (LOBYTE(WsaData->wVersion) >= 2)
        {
            ok_dec(WsaData->iMaxSockets, 0);
            ok_dec(WsaData->iMaxUdpDg, 0);
        }
        else
        {
            ok_dec(WsaData->iMaxSockets, 32767);
            ok_dec(WsaData->iMaxUdpDg, 65467);
        }
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    }

    /* upgrade the version */
    FillMemory(WsaData, sizeof(*WsaData), 0x55);
    Error = WSAStartup(MAKEWORD(1, 1), WsaData);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);
    ok_hex(WsaData->wVersion, MAKEWORD(1, 1));
    ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
    Okay = CheckStringBuffer(WsaData->szDescription,
                             sizeof(WsaData->szDescription),
                             "WinSock 2.0",
                             0x55);
    ok(Okay, "CheckStringBuffer failed\n");
    Okay = CheckStringBuffer(WsaData->szSystemStatus,
                             sizeof(WsaData->szSystemStatus),
                             "Running",
                             0x55);
    ok(Okay, "CheckStringBuffer failed\n");
    ok_dec(WsaData->iMaxSockets, 32767);
    ok_dec(WsaData->iMaxUdpDg, 65467);
    ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    ok(IsWinsockInitialized(), "Winsock not initialized\n");
    if (!Error)
    {
        ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSAStartup(MAKEWORD(2, 2), WsaData);
        ok_dec(Error, 0);
        ok_hex(WsaData->wVersion, MAKEWORD(2, 2));
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        Okay = CheckStringBuffer(WsaData->szDescription,
                                 sizeof(WsaData->szDescription),
                                 "WinSock 2.0",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        Okay = CheckStringBuffer(WsaData->szSystemStatus,
                                 sizeof(WsaData->szSystemStatus),
                                 "Running",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        ok_dec(WsaData->iMaxSockets, 0);
        ok_dec(WsaData->iMaxUdpDg, 0);
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
        if (!Error)
        {
            ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            WSACleanup();
            ok(IsWinsockInitialized(), "Winsock prematurely uninitialized\n");
        }
        WSACleanup();
        ok(!IsWinsockInitialized(), "Winsock still initialized after cleanup\n");
    }

    /* downgrade the version */
    FillMemory(WsaData, sizeof(*WsaData), 0x55);
    Error = WSAStartup(MAKEWORD(2, 2), WsaData);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);
    ok_hex(WsaData->wVersion, MAKEWORD(2, 2));
    ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
    Okay = CheckStringBuffer(WsaData->szDescription,
                             sizeof(WsaData->szDescription),
                             "WinSock 2.0",
                             0x55);
    ok(Okay, "CheckStringBuffer failed\n");
    Okay = CheckStringBuffer(WsaData->szSystemStatus,
                             sizeof(WsaData->szSystemStatus),
                             "Running",
                             0x55);
    ok(Okay, "CheckStringBuffer failed\n");
    ok_dec(WsaData->iMaxSockets, 0);
    ok_dec(WsaData->iMaxUdpDg, 0);
    ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    ok(IsWinsockInitialized(), "Winsock not initialized\n");
    if (!Error)
    {
        ok(!AreLegacyFunctionsSupported(), "Legacy function succeeded\n");
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSAStartup(MAKEWORD(1, 1), WsaData);
        ok_dec(Error, 0);
        ok_hex(WsaData->wVersion, MAKEWORD(1, 1));
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        Okay = CheckStringBuffer(WsaData->szDescription,
                                 sizeof(WsaData->szDescription),
                                 "WinSock 2.0",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        Okay = CheckStringBuffer(WsaData->szSystemStatus,
                                 sizeof(WsaData->szSystemStatus),
                                 "Running",
                                 0x55);
        ok(Okay, "CheckStringBuffer failed\n");
        ok_dec(WsaData->iMaxSockets, 32767);
        ok_dec(WsaData->iMaxUdpDg, 65467);
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
        if (!Error)
        {
            ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            WSACleanup();
            ok(IsWinsockInitialized(), "Winsock prematurely uninitialized\n");
        }
        WSACleanup();
        ok(!IsWinsockInitialized(), "Winsock still initialized after cleanup\n");
    }

    FreeGuarded(WsaData);
}
