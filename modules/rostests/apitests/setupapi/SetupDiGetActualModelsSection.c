/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for SetupDiGetActualModelsSectionA/W
 * COPYRIGHT:   Copyright 2026 Alex Mendoza
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <setupapi.h>

static const char TestInfData[] =
    "[Version]\n"
    "Signature=\"$Windows NT$\"\n"
    "Class=TestClass\n"
    "ClassGuid={12345678-1234-1234-1234-123456789ABC}\n"
    "\n"
    "[Manufacturer]\n"
    "%TestMfg%=TestModels,NTx86,NTamd64,NT\n"
    "\n"
    "[TestModels.NTx86]\n"
    "%DeviceDesc%=InstallSection,TEST\\VID_1234&PID_5678\n"
    "\n"
    "[TestModels.NTamd64]\n"
    "%DeviceDesc%=InstallSection,TEST\\VID_1234&PID_5678\n"
    "\n"
    "[TestModels.NT]\n"
    "%DeviceDesc%=InstallSection,TEST\\VID_1234&PID_5678\n"
    "\n"
    "[TestModels]\n"
    "%DeviceDesc%=InstallSection,TEST\\VID_1234&PID_5678\n"
    "\n"
    "[InstallSection]\n"
    "\n"
    "[Strings]\n"
    "TestMfg=\"Test Manufacturer\"\n"
    "DeviceDesc=\"Test Device\"\n";

static void create_test_inf(LPCSTR filename)
{
    HANDLE hFile;
    DWORD written;

    hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    WriteFile(hFile, TestInfData, sizeof(TestInfData) - 1, &written, NULL);
    CloseHandle(hFile);
}

static void test_SetupDiGetActualModelsSectionW(void)
{
    HINF hInf;
    INFCONTEXT ctx;
    WCHAR Buffer[LINE_LEN];
    DWORD Required = 0;
    BOOL ret;
    char path[MAX_PATH];

    GetTempPathA(sizeof(path), path);
    lstrcatA(path, "test_models.inf");
    create_test_inf(path);

    hInf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hInf != INVALID_HANDLE_VALUE, "SetupOpenInfFileA failed: %lu\n", GetLastError());
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DeleteFileA(path);
        return;
    }

    /* Find the manufacturer line */
    ret = SetupFindFirstLineW(hInf, L"Manufacturer", NULL, &ctx);
    ok(ret, "SetupFindFirstLineW failed: %lu\n", GetLastError());

    /* NULL context must fail */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetActualModelsSectionW(NULL, NULL, Buffer, ARRAYSIZE(Buffer), &Required, NULL);
    ok(!ret, "Expected failure with NULL Context\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* Reserved != NULL must fail */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetActualModelsSectionW(&ctx, NULL, Buffer, ARRAYSIZE(Buffer), &Required, (PVOID)1);
    ok(!ret, "Expected failure with non-NULL Reserved\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* Query required size only */
    Required = 0;
    ret = SetupDiGetActualModelsSectionW(&ctx, NULL, NULL, 0, &Required, NULL);
    ok(ret, "SetupDiGetActualModelsSectionW (size query) failed: %lu\n", GetLastError());
    ok(Required > 1, "Expected Required > 1, got %lu\n", Required);

    /* Actual call */
    ZeroMemory(Buffer, sizeof(Buffer));
    Required = 0;
    ret = SetupDiGetActualModelsSectionW(&ctx, NULL, Buffer, ARRAYSIZE(Buffer), &Required, NULL);
    ok(ret, "SetupDiGetActualModelsSectionW failed: %lu\n", GetLastError());
    ok(Buffer[0] != 0, "Expected non-empty section name\n");
    ok(Required == lstrlenW(Buffer) + 1,
       "Required size mismatch: %lu vs %u\n", Required, lstrlenW(Buffer) + 1);

    trace("Got Models section: %s\n", wine_dbgstr_w(Buffer));

    /* Too small buffer */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetActualModelsSectionW(&ctx, NULL, Buffer, 1, &Required, NULL);
    ok(!ret, "Expected failure with tiny buffer\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    SetupCloseInfFile(hInf);
    DeleteFileA(path);
}

static void test_SetupDiGetActualModelsSectionA(void)
{
    HINF hInf;
    INFCONTEXT ctx;
    char Buffer[LINE_LEN];
    DWORD Required = 0;
    BOOL ret;
    char path[MAX_PATH];

    GetTempPathA(sizeof(path), path);
    lstrcatA(path, "test_models.inf");
    create_test_inf(path);

    hInf = SetupOpenInfFileA(path, NULL, INF_STYLE_WIN4, NULL);
    ok(hInf != INVALID_HANDLE_VALUE, "SetupOpenInfFileA failed\n");
    if (hInf == INVALID_HANDLE_VALUE)
    {
        DeleteFileA(path);
        return;
    }

    ret = SetupFindFirstLineA(hInf, "Manufacturer", NULL, &ctx);
    ok(ret, "SetupFindFirstLineA failed\n");

    /* Size query */
    Required = 0;
    ret = SetupDiGetActualModelsSectionA(&ctx, NULL, NULL, 0, &Required, NULL);
    ok(ret, "SetupDiGetActualModelsSectionA (size query) failed: %lu\n", GetLastError());
    ok(Required > 1, "Expected Required > 1\n");

    /* Actual call */
    ZeroMemory(Buffer, sizeof(Buffer));
    ret = SetupDiGetActualModelsSectionA(&ctx, NULL, Buffer, sizeof(Buffer), &Required, NULL);
    ok(ret, "SetupDiGetActualModelsSectionA failed: %lu\n", GetLastError());
    ok(Buffer[0] != 0, "Expected non-empty section name\n");

    trace("Got Models section (A): %s\n", Buffer);

    SetupCloseInfFile(hInf);
    DeleteFileA(path);
}

START_TEST(SetupDiGetActualModelsSection)
{
    test_SetupDiGetActualModelsSectionW();
    test_SetupDiGetActualModelsSectionA();
}
