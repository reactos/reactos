/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite loader application
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

#include "kmtest.h"
#include <kmt_public.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVICE_NAME        L"Kmtest"
#define SERVICE_PATH        L"kmtest_drv.sys"
#define SERVICE_DESCRIPTION L"ReactOS Kernel-Mode Test Suite Driver"

#define RESULTBUFFER_SIZE   (1024 * 1024)

typedef enum
{
    KMT_DO_NOTHING,
    KMT_LIST_TESTS,
    KMT_LIST_ALL_TESTS,
    KMT_RUN_TEST,
} KMT_OPERATION;

HANDLE KmtestHandle;
SC_HANDLE KmtestServiceHandle;
PCSTR ErrorFileAndLine = "No error";

static void OutputError(IN DWORD Error);
static DWORD ListTests(IN BOOLEAN IncludeHidden);
static PKMT_TESTFUNC FindTest(IN PCSTR TestName);
static DWORD OutputResult(IN PCSTR TestName);
static DWORD RunTest(IN PCSTR TestName);
int __cdecl main(int ArgCount, char **Arguments);

/**
 * @name OutputError
 *
 * Output an error message to the console.
 *
 * @param Error
 *        Win32 error code
 */
static
void
OutputError(
    IN DWORD Error)
{
    PSTR Message;
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&Message, 0, NULL))
    {
        fprintf(stderr, "%s: Could not retrieve error message (error 0x%08lx). Original error: 0x%08lx\n",
            ErrorFileAndLine, GetLastError(), Error);
        return;
    }

    fprintf(stderr, "%s: error 0x%08lx: %s\n", ErrorFileAndLine, Error, Message);

    LocalFree(Message);
}

/**
 * @name CompareTestNames
 *
 * strcmp that skips a leading '-' on either string if present
 *
 * @param Str1
 * @param Str2
 * @return see strcmp
 */
static
INT
CompareTestNames(
    IN PCSTR Str1,
    IN PCSTR Str2)
{
    if (*Str1 == '-')
        ++Str1;
    if (*Str2 == '-')
        ++Str2;
    while (*Str1 && *Str1 == *Str2)
    {
        ++Str1;
        ++Str2;
    }
    return *Str1 - *Str2;
}

/**
 * @name ListTests
 *
 * Output the list of tests to the console.
 * The list will comprise tests as listed by the driver
 * in addition to user-mode tests in TestList.
 *
 * @param IncludeHidden
 *        TRUE to include "hidden" tests prefixed with a '-'
 *
 * @return Win32 error code
 */
static
DWORD
ListTests(
    IN BOOLEAN IncludeHidden)
{
    DWORD Error = ERROR_SUCCESS;
    CHAR Buffer[1024];
    DWORD BytesRead;
    PCSTR TestName = Buffer;
    PCKMT_TEST TestEntry = TestList;
    PCSTR NextTestName;

    puts("Valid test names:");

    // get test list from driver
    if (!DeviceIoControl(KmtestHandle, IOCTL_KMTEST_GET_TESTS, NULL, 0, Buffer, sizeof Buffer, &BytesRead, NULL))
        error_goto(Error, cleanup);

    // output test list plus user-mode tests
    while (TestEntry->TestName || *TestName)
    {
        if (!TestEntry->TestName)
        {
            NextTestName = TestName;
            TestName += strlen(TestName) + 1;
        }
        else if (!*TestName)
        {
            NextTestName = TestEntry->TestName;
            ++TestEntry;
        }
        else
        {
            INT Result = CompareTestNames(TestEntry->TestName, TestName);

            if (Result == 0)
            {
                NextTestName = TestEntry->TestName;
                TestName += strlen(TestName) + 1;
                ++TestEntry;
            }
            else if (Result < 0)
            {
                NextTestName = TestEntry->TestName;
                ++TestEntry;
            }
            else
            {
                NextTestName = TestName;
                TestName += strlen(TestName) + 1;
            }
        }

        if (IncludeHidden && NextTestName[0] == '-')
            ++NextTestName;

        if (NextTestName[0] != '-')
            printf("    %s\n", NextTestName);
    }

cleanup:
    return Error;
}

/**
 * @name FindTest
 *
 * Find a test in TestList by name.
 *
 * @param TestName
 *        Name of the test to look for. Case sensitive
 *
 * @return pointer to test function, or NULL if not found
 */
static
PKMT_TESTFUNC
FindTest(
    IN PCSTR TestName)
{
    PCKMT_TEST TestEntry = TestList;

    for (TestEntry = TestList; TestEntry->TestName; ++TestEntry)
    {
        PCSTR TestEntryName = TestEntry->TestName;

        // skip leading '-' if present
        if (*TestEntryName == '-')
            ++TestEntryName;

        if (!lstrcmpA(TestEntryName, TestName))
            break;
    }

    return TestEntry->TestFunction;
}

/**
 * @name OutputResult
 *
 * Output the test results in ResultBuffer to the console.
 *
 * @param TestName
 *        Name of the test whose result is to be printed
 *
 * @return Win32 error code
 */
static
DWORD
OutputResult(
    IN PCSTR TestName)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD BytesWritten;
    DWORD LogBufferLength;
    DWORD Offset = 0;
    /* A console window can't handle a single
     * huge block of data, so split it up */
    const DWORD BlockSize = 8 * 1024;

    KmtFinishTest(TestName);

    LogBufferLength = ResultBuffer->LogBufferLength;
    for (Offset = 0; Offset < LogBufferLength; Offset += BlockSize)
    {
        DWORD Length = min(LogBufferLength - Offset, BlockSize);
        if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), ResultBuffer->LogBuffer + Offset, Length, &BytesWritten, NULL))
            error(Error);
    }

    return Error;
}

/**
 * @name RunTest
 *
 * Run the named test and output its results.
 *
 * @param TestName
 *        Name of the test to run. Case sensitive
 *
 * @return Win32 error code
 */
static
DWORD
RunTest(
    IN PCSTR TestName)
{
    DWORD Error = ERROR_SUCCESS;
    PKMT_TESTFUNC TestFunction;
    DWORD BytesRead;

    assert(TestName != NULL);

    if (!ResultBuffer)
    {
        ResultBuffer = KmtAllocateResultBuffer(RESULTBUFFER_SIZE);
        if (!ResultBuffer)
            error_goto(Error, cleanup);
        if (!DeviceIoControl(KmtestHandle, IOCTL_KMTEST_SET_RESULTBUFFER, ResultBuffer, RESULTBUFFER_SIZE, NULL, 0, &BytesRead, NULL))
            error_goto(Error, cleanup);
    }

    // check test list
    TestFunction = FindTest(TestName);

    if (TestFunction)
    {
        TestFunction();
        goto cleanup;
    }

    // not found in user-mode test list, call driver
    Error = KmtRunKernelTest(TestName);

cleanup:
    if (!Error)
        Error = OutputResult(TestName);

    return Error;
}

/**
 * @name main
 *
 * Program entry point
 *
 * @param ArgCount
 * @param Arguments
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int
main(
    int ArgCount,
    char **Arguments)
{
    INT Status = EXIT_SUCCESS;
    DWORD Error = ERROR_SUCCESS;
    PCSTR AppName = "kmtest.exe";
    PCSTR TestName = NULL;
    KMT_OPERATION Operation = KMT_DO_NOTHING;
    BOOLEAN ShowHidden = FALSE;

    Error = KmtServiceInit();
    if (Error)
        goto cleanup;

    if (ArgCount >= 1)
        AppName = Arguments[0];

    if (ArgCount <= 1)
    {
        printf("Usage: %s <test_name>                 - run the specified test (creates/starts the driver(s) as appropriate)\n", AppName);
        printf("       %s --list                      - list available tests\n", AppName);
        printf("       %s --list-all                  - list available tests, including hidden\n", AppName);
        printf("       %s <create|delete|start|stop>  - manage the kmtest driver\n\n", AppName);
        Operation = KMT_LIST_TESTS;
    }
    else
    {
        TestName = Arguments[1];
        if (!lstrcmpA(TestName, "create"))
            Error = KmtCreateService(SERVICE_NAME, SERVICE_PATH, SERVICE_DESCRIPTION, &KmtestServiceHandle);
        else if (!lstrcmpA(TestName, "delete"))
            Error = KmtDeleteService(SERVICE_NAME, &KmtestServiceHandle);
        else if (!lstrcmpA(TestName, "start"))
            Error = KmtStartService(SERVICE_NAME, &KmtestServiceHandle);
        else if (!lstrcmpA(TestName, "stop"))
            Error = KmtStopService(SERVICE_NAME, &KmtestServiceHandle);

        else if (!lstrcmpA(TestName, "--list"))
            Operation = KMT_LIST_TESTS;
        else if (!lstrcmpA(TestName, "--list-all"))
            Operation = KMT_LIST_ALL_TESTS;
        else
            Operation = KMT_RUN_TEST;
    }

    if (Operation)
    {
        Error = KmtCreateAndStartService(SERVICE_NAME, SERVICE_PATH, SERVICE_DESCRIPTION, &KmtestServiceHandle, FALSE);
        if (Error)
            goto cleanup;

        KmtestHandle = CreateFile(KMTEST_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (KmtestHandle == INVALID_HANDLE_VALUE)
            error_goto(Error, cleanup);

        switch (Operation)
        {
            case KMT_LIST_ALL_TESTS:
                ShowHidden = TRUE;
                /* fall through */
            case KMT_LIST_TESTS:
                Error = ListTests(ShowHidden);
                break;
            case KMT_RUN_TEST:
                Error = RunTest(TestName);
                break;
            default:
                assert(FALSE);
        }
    }

cleanup:
    if (KmtestHandle)
        CloseHandle(KmtestHandle);

    if (ResultBuffer)
        KmtFreeResultBuffer(ResultBuffer);

    KmtCloseService(&KmtestServiceHandle);

    if (Error)
        KmtServiceCleanup(TRUE);
    else
        Error = KmtServiceCleanup(FALSE);

    if (Error)
    {
        OutputError(Error);

        Status = EXIT_FAILURE;
    }

    return Status;
}
