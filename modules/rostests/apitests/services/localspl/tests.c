/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck (colin@reactos.org)
 */

/*
 * The original localspl.dll from Windows Server 2003 is not easily testable.
 * It relies on a proper initialization inside spoolsv.exe, so we can't just load it in an API-Test as usual.
 * See https://www.reactos.org/pipermail/ros-dev/2015-June/017395.html for more information.
 * 
 * To make testing possible anyway, this program basically does four things:
 *     - Injecting our testing code into spoolsv.exe.
 *     - Registering and running us as a service in the SYSTEM security context like spoolsv.exe, so that injection is possible at all.
 *     - Sending the test name and receiving the console output over named pipes.
 *     - Redirecting the received console output to stdout again, so it looks and feels like a standard API-Test.
 *
 * To simplify debugging of the injected code, it is entirely separated into a DLL file localspl_apitest.dll.
 * What we actually inject is a LoadLibraryW call, so that the DLL is loaded gracefully without any hacks.
 * Therefore, you can just attach your debugger to the spoolsv.exe process and set breakpoints on the localspl_apitest.dll code.
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winsvc.h>
#include <winspool.h>
#include <winsplp.h>

#include "../framework/remotetests.h"
#include "services_apitest.h"

static void
_RunRemoteTestSPL(const char* szTestName)
{
    DWORD cbRead;
    DWORD cbWritten;

    // Do a dummy EnumPrintersW call.
    // This guarantees that the Spooler Service has actually loaded localspl.dll, which is a requirement for our injected DLL to work properly.
    EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, NULL, 1, NULL, 0, &cbRead, &cbWritten);

    _RunRemoteTest(szTestName);
}

START_TEST(fpEnumPrinters)
{
    _RunRemoteTestSPL("fpEnumPrinters");
}

START_TEST(fpGetPrintProcessorDirectory)
{
    _RunRemoteTestSPL("fpGetPrintProcessorDirectory");
}

START_TEST(fpSetJob)
{
    _RunRemoteTestSPL("fpSetJob");
}
