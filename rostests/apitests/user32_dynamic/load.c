/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for loading/unloading user32.dll
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

START_TEST(load)
{
    HMODULE hUser32;
    HMODULE hUser32_2;
    BOOL Ret;
    DWORD Error;

    SetLastError(12345);
    hUser32 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32 != NULL, "LoadLibrary failed\n");
    ok(Error != 12345, "Error = %lu\n", Error);

    SetLastError(12345);
    hUser32_2 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32_2 == hUser32, "LoadLibrary failed\n");
    ok(Error == 12345, "Error = %lu\n", Error);

    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    SetLastError(12345);
    hUser32 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32 != NULL, "LoadLibrary failed\n");
    ok(Error == 12345, "Error = %lu\n", Error);

    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);
}
