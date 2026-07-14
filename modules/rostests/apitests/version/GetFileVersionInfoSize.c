/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for GetFileVersionInfoSize[A/W]
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

void Test_GetFileVersionInfoSizeA(void)
{
    CHAR ModuleFileName[MAX_PATH];
    CHAR PathBuffer[MAX_PATH];
    DWORD Result;

    /* Test getting the version info size for this executable */
    GetModuleFileNameA(NULL, ModuleFileName, ARRAYSIZE(ModuleFileName));
    Result = GetFileVersionInfoSizeA(ModuleFileName, NULL);
    ok((Result > 1000) && (Result < 2000), "Unexpected result: %lu\n", Result);

    /* Test support for NT path prefix */
    strcpy(PathBuffer, "\\??\\");
    strcat(PathBuffer, ModuleFileName);
    Result = GetFileVersionInfoSizeA(PathBuffer, NULL);
    ok_eq_ulong(Result, 0);
}

void Test_GetFileVersionInfoSizeW(void)
{
    WCHAR ModuleFileName[MAX_PATH];
    WCHAR PathBuffer[MAX_PATH];
    DWORD Result;

    /* Test getting the version info size for this executable */
    GetModuleFileNameW(NULL, ModuleFileName, ARRAYSIZE(ModuleFileName));
    Result = GetFileVersionInfoSizeW(ModuleFileName, NULL);
    ok((Result > 1000) && (Result < 2000), "Unexpected result: %lu\n", Result);

    /* Test support for NT path prefix */
    wcscpy(PathBuffer, L"\\??\\");
    wcscat(PathBuffer, ModuleFileName);
    Result = GetFileVersionInfoSizeW(PathBuffer, NULL);
    ok_eq_ulong(Result, 0);
}


START_TEST(GetFileVersionInfoSize)
{
    Test_GetFileVersionInfoSizeA();
    Test_GetFileVersionInfoSizeW();
}
