/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Shared definitions for the test program and the test DLL
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _LOCALSPL_APITEST_H
#define _LOCALSPL_APITEST_H

#define COMMAND_PIPE_NAME       L"\\\\.\\pipe\\localspl_apitest_command_pipe"
#define OUTPUT_PIPE_NAME        L"\\\\.\\pipe\\localspl_apitest_output_pipe"
#define SERVICE_NAME            L"localspl_apitest_service"

typedef BOOL (WINAPI *PInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);

#endif