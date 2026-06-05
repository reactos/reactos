/*
 * PROJECT: ReactOS API tests
 * LICENSE: MIT (https://spdx.org/licenses/MIT)
 * PURPOSE: Tests for SHInvokePrinterCommandW/A
 * PROGRAMMERS: Alex Mendoza
 */

#include "shelltest.h"
#include <shellapi.h>
#include <undocshell.h>

START_TEST(SHInvokePrinterCommand)
{
    BOOL ret;

    /* NULL printer name = invalid */
    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, PRINTACTION_OPEN, NULL, NULL, FALSE);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* Same for ANSI */
    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandA(NULL, PRINTACTION_OPEN, NULL, NULL, FALSE);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, 0xDEAD, L"DummyPrinter", NULL, FALSE);
    ok(ret == FALSE, "Expected FALSE for unknown action, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER for unknown action, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, PRINTACTION_OPEN,
                                  L"\\\\NonExistentServer\\NonExistentPrinter",
                                  NULL, TRUE);
    ok(ret == FALSE, "Expected FALSE for non-existent printer, got %d\n", ret);

    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandA(NULL, PRINTACTION_OPEN,
                                  "\\\\NonExistentServer\\NonExistentPrinter",
                                  NULL, TRUE);
    ok(ret == FALSE, "Expected FALSE for non-existent printer (A), got %d\n", ret);

    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, PRINTACTION_TESTPAGE,
                                  L"\\\\NonExistentServer\\NonExistentPrinter",
                                  NULL, TRUE);
    ok(ret == FALSE, "Expected FALSE for TESTPAGE on non-existent printer, got %d\n", ret);

    SetLastError(0xdeadbeef);
    ret = SHInvokePrinterCommandW(NULL, PRINTACTION_PROPERTIES,
                                  L"\\\\NonExistentServer\\NonExistentPrinter",
                                  NULL, TRUE);
    ok(ret == FALSE, "Expected FALSE for PROPERTIES on non-existent printer, got %d\n", ret);
}