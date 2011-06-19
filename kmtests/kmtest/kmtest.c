/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Loader Application
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define UNICODE
#include <windows.h>
#include <strsafe.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "kmtest.h"
#include <winioctl.h>
#include <kmt_public.h>
#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

#define LOGBUFFER_SIZE 65000

static void OutputError(FILE *fp, DWORD error);
static DWORD RunTest(char *testName);
static DWORD ListTests(PSTR *testList);
int __cdecl main(int argc, char **argv);

static void OutputError(FILE *fp, DWORD error)
{
    char *message;
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, NULL))
    {
        fprintf(fp, "Could not retrieve error message (error 0x%08lx). Original error: 0x%08lx\n", GetLastError(), error);
    }

    fprintf(fp, "%s\n", message);

    LocalFree(message);
}

static DWORD RunTest(char *testName)
{
    DWORD error = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    DWORD bytesRead, bytesWritten;

    ResultBuffer = KmtAllocateResultBuffer(LOGBUFFER_SIZE);
    if (!ResultBuffer)
    {
        error = GetLastError();
        goto cleanup;
    }

    hDevice = CreateFile(KMTEST_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!DeviceIoControl(hDevice, IOCTL_KMTEST_SET_RESULTBUFFER, ResultBuffer, FIELD_OFFSET(KMT_RESULTBUFFER, LogBuffer[LOGBUFFER_SIZE]), NULL, 0, &bytesRead, NULL))
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!DeviceIoControl(hDevice, IOCTL_KMTEST_RUN_TEST, testName, strlen(testName), NULL, 0, &bytesRead, NULL))
    {
        error = GetLastError();
        goto cleanup;
    }

    KmtFinishTest(testName);

    if (!WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), ResultBuffer->LogBuffer, ResultBuffer->LogBufferLength, &bytesWritten, NULL))
    {
        error = GetLastError();
        goto cleanup;
    }

cleanup:
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);

    if (ResultBuffer)
        KmtFreeResultBuffer(ResultBuffer);

    return error;
}

static DWORD ListTests(PSTR *testList)
{
    DWORD error = ERROR_SUCCESS;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    DWORD bytesRead;
    PSTR buffer = NULL;
    DWORD bufferSize;

    if (!testList)
    {
        error = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    hDevice = CreateFile(KMTEST_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        goto cleanup;
    }

    bufferSize = 1024;
    buffer = HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!buffer)
    {
        error = GetLastError();
        goto cleanup;
    }

    if (!DeviceIoControl(hDevice, IOCTL_KMTEST_GET_TESTS, NULL, 0, buffer, bufferSize, &bytesRead, NULL))
    {
        error = GetLastError();
        goto cleanup;
    }

cleanup:
    if (buffer && error)
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        buffer = NULL;
    }

    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);

    if (testList)
        *testList = buffer;

    return error;
}

int __cdecl main(int argc, char **argv)
{
    int status = EXIT_SUCCESS;
    DWORD error;

    if (argc <= 1)
    {
        /* no arguments: show usage and list tests */
        char *programName = argc == 0 ? "kmtest" : argv[0];
        char *testNames, *testName;
        size_t len;

        printf("Usage: %s test_name\n", programName);
        puts("\nValid test names:");
        puts("    Create");
        puts("    Start");
        puts("    Stop");
        puts("    Delete");

        error = ListTests(&testNames);
        testName = testNames;

        while ((len = strlen(testName)) != 0)
        {
            printf("    %s\n", testName);
            testName += len + 1;
        }

        /* TODO: user-mode test parts */

        if (error)
            OutputError(stdout, error);
    }
    else
    {
        char *testName = argv[1];

        if (argc > 2)
            fputs("Excess arguments ignored\n", stderr);

        if (!lstrcmpiA(testName, "create"))
            error = Service_Control(Service_Create);
        else if (!lstrcmpiA(testName, "delete"))
            error = Service_Control(Service_Delete);
        else if (!lstrcmpiA(testName, "start"))
            error = Service_Control(Service_Start);
        else if (!lstrcmpiA(testName, "stop"))
            error = Service_Control(Service_Stop);
        else
            /* TODO: user-mode test parts */
            error = RunTest(testName);

        OutputError(stdout, error);
    }

    if (error)
        status = EXIT_FAILURE;

    return status;
}
