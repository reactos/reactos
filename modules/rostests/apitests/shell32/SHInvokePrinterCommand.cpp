/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for SHInvokePrinterCommandW/A
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include "shelltest.h"
#include <shellapi.h>
#include <undocshell.h>

START_TEST(SHInvokePrinterCommand)
{
    BOOL ret;

    // NULL printer name
    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, PRINTACTION_OPEN, NULL, NULL, FALSE);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %lu\n", GetLastError());

    // Same for ANSI
    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandA(NULL, PRINTACTION_OPEN, NULL, NULL, FALSE);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %lu\n", GetLastError());

    // Unknown action
    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, 0xDEAD, L"DummyPrinter", NULL, FALSE);
    ok(ret == TRUE, "Expected TRUE for unknown action, got %d\n", ret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %lu\n", GetLastError());
}
