/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for GetIpErrorString
 * COPYRIGHT:   Copyright 2026 Eudald Tarradas <etarradas@calella.escolesfreta.cat>
 */

#include <apitest.h>
#include <winsock2.h>
#include <iphlpapi.h>

START_TEST(GetIpErrorString)
{
    DWORD Err;
    DWORD Size;
    WCHAR Buffer[256];

    /* NULL Buffer */
    Size = 256;
    Err = GetIpErrorString(IP_SUCCESS, NULL, &Size);
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", Err);

    /* NULL Size */
    Err = GetIpErrorString(IP_SUCCESS, Buffer, NULL);
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", Err);

    Size = 256;
    Err = GetIpErrorString(0xDEADBEEF, Buffer, &Size);
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", Err);

    Size = 1;
    Err = GetIpErrorString(IP_DEST_HOST_UNREACHABLE, Buffer, &Size);
    ok(Err == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %lu\n", Err);
    ok(Size > 1, "Expected Size to be updated to required length, got %lu\n", Size);

    Err = GetIpErrorString(IP_DEST_HOST_UNREACHABLE, Buffer, &Size);
    ok(Err == NO_ERROR, "Expected NO_ERROR, got %lu\n", Err);
    ok(wcslen(Buffer) == Size, "Expected wcslen(Buffer) == Size, got %lu vs %lu\n", (DWORD)wcslen(Buffer), Size);

    Size = 256;
    Err = GetIpErrorString(IP_SUCCESS, Buffer, &Size);
    ok(Err == NO_ERROR, "Expected NO_ERROR, got %lu\n", Err);
    ok(Size == wcslen(Buffer), "Expected Size == wcslen(Buffer), got %lu vs %lu\n", Size, (DWORD)wcslen(Buffer));
}
