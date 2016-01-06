/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for IsValidDevmodeA/IsValidDevmodeW
 * COPYRIGHT:   Copyright 2016 Colin Finck <colin@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

// With very high probability, this is everything you can test for IsValidDevmode.
// For your own testing, you can fill a DevMode structure with random bytes multiple times,
// always set dmSize and dmDriverExtra appropriately and feed it to IsValidDevmode.
// The function will always succeed.
// I'm not doing that here, because I don't want to introduce randomness.

START_TEST(IsValidDevmodeA)
{
    DEVMODEA DevMode;

    // Give no Devmode at all, this has to fail without crashing.
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(NULL, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // Give a zeroed Devmode, this has to fail, because dmSize isn't set.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // Now set only the dmSize member, IsValidDevmodeA should return TRUE now. The Last Error isn't touched again.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // IsValidDevmodeA should also succeed if the DevMode appears to be larger.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA) + 1;
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // The DevmodeSize parameter may be larger than dmSize, but not the other way round!
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA) + 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    // dmDriverExtra is also taken into account.
    ZeroMemory(&DevMode, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    DevMode.dmDriverExtra = 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeA(&DevMode, sizeof(DEVMODEA)), "IsValidDevmodeA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeA returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeA(&DevMode, sizeof(DEVMODEA) + 1), "IsValidDevmodeA returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeA returns error %lu!\n", GetLastError());
}

START_TEST(IsValidDevmodeW)
{
    DEVMODEW DevMode;

    // Give no Devmode at all, this has to fail without crashing.
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(NULL, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // Give a zeroed Devmode, this has to fail, because dmSize isn't set.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // Now set only the dmSize member, IsValidDevmodeW should return TRUE now. The Last Error isn't touched again.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // IsValidDevmodeW should also succeed if the DevMode appears to be larger.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW) + 1;
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());
    
    // The DevmodeSize parameter may be larger than dmSize, but not the other way round!
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW) + 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    // dmDriverExtra is also taken into account.
    ZeroMemory(&DevMode, sizeof(DEVMODEW));
    DevMode.dmSize = sizeof(DEVMODEW);
    DevMode.dmDriverExtra = 1;
    SetLastError(0xDEADBEEF);
    ok(!IsValidDevmodeW(&DevMode, sizeof(DEVMODEW)), "IsValidDevmodeW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "IsValidDevmodeW returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(IsValidDevmodeW(&DevMode, sizeof(DEVMODEW) + 1), "IsValidDevmodeW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "IsValidDevmodeW returns error %lu!\n", GetLastError());
}
