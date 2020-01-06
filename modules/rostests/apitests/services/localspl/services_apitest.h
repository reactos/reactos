/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shared definitions for the test program and the test DLL
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#ifndef _SERVICES_APITEST_H
#define _SERVICES_APITEST_H

#define WIN32_NO_STATUS
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>

// defines for services/framework
#define COMMAND_PIPE_NAME       L"\\\\.\\pipe\\localspl_apitest_command_pipe"
#define OUTPUT_PIPE_NAME        L"\\\\.\\pipe\\localspl_apitest_output_pipe"
#define SERVICE_NAME            L"localspl_apitest_service"
#define SERVICE_EXE_NAME        L"spoolsv.exe"

typedef BOOL (WINAPI *PInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);

#endif
