/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests for exception handling
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <apitest.h>
#include <windows.h>
#include <stdio.h>
#include <ndk/rtlfuncs.h>

void
RunHandleRaiseExceptionTest(void)
{
    _SEH2_TRY
    {
       RaiseException(0xDEADBEEF, 0, 0, NULL);
       exit(-1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* This should be executed */
        exit(1);
    }
    _SEH2_END;

    /* This should not be executed */
    exit(-2);
}

void
RunContinueRaiseExceptionTest(void)
{
    _SEH2_TRY
    {
       RaiseException(0xDEADBEEF, 0, 0, NULL);
       exit(1);
    }
    _SEH2_EXCEPT(EXCEPTION_CONTINUE_EXECUTION)
    {
        exit(-1);
    }
    _SEH2_END;
    exit(-2);
}

void
RunHandleNoncontinuableExceptionTest(void)
{
    _SEH2_TRY
    {
       RaiseException(0xDEADBEEF, EXCEPTION_NONCONTINUABLE, 0, NULL);
       exit(-1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        exit(1);
    }
    _SEH2_END;
    exit(-2);
}

void
RunContinueNoncontinuableExceptionTest(void)
{
    int HandlerExecuted = 0;
    ULONG ExceptionCode = 0;

    _SEH2_TRY
    {
        _SEH2_TRY
        {
            RaiseException(0xDEADBEEF, EXCEPTION_NONCONTINUABLE, 0, NULL);
            exit(-1);
        }
        _SEH2_EXCEPT(EXCEPTION_CONTINUE_EXECUTION)
        {
            exit(-2);
        }
        _SEH2_END;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        HandlerExecuted = 1;
        ExceptionCode = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(HandlerExecuted, "Exception handler not executed\n");
    ok(ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION, "Wrong exception code: 0x%lx\n", ExceptionCode);
}

void
RunFilterFastfailTest(void)
{
    _SEH2_TRY
    {
        __fastfail(1);
        exit(-1);
    }
    _SEH2_EXCEPT(exit(-2), EXCEPTION_EXECUTE_HANDLER)
    {
        exit(-3);
    }
    _SEH2_END;
    exit(-4);
}

static
int
RunSubtest(PSTR SubtestName)
{
    CHAR szCmdLine[MAX_PATH];

    GetModuleFileNameA(NULL, szCmdLine, _countof(szCmdLine));
    lstrcatA(szCmdLine, " Exceptions ");
    lstrcatA(szCmdLine, SubtestName);

    // Run the test
    return system(szCmdLine);
}

static
void
DebugSubtest(
    PSTR SubtestName,
    DWORD ExpectedExceptionCode,
    BOOLEAN ExpectedFirstChance)
{
    CHAR szCmdLine[MAX_PATH];
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOLEAN GotException = FALSE;
    DWORD ret;

    GetModuleFileNameA(NULL, szCmdLine, _countof(szCmdLine));
    lstrcatA(szCmdLine, " Exceptions ");
    lstrcatA(szCmdLine, SubtestName);

    /* Start the sub-process in debug mode */
    ret = CreateProcessA(NULL,
                         szCmdLine,
                         NULL,
                         NULL,
                         FALSE,
                         DEBUG_PROCESS,
                         NULL,
                         NULL,
                         &si,
                         &pi);
    ok(ret != 0, "CreateProcessA failed\n");
    if (!ret)
    {
        return;
    }

    DEBUG_EVENT debugEvent;
    while (WaitForDebugEvent(&debugEvent, INFINITE))
    {
        if (debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            DWORD exceptionCode = debugEvent.u.Exception.ExceptionRecord.ExceptionCode;

            /* Ignore debugger break-points */
            if (exceptionCode != STATUS_BREAKPOINT)
            {
                GotException = TRUE;
                ok_eq_hex(debugEvent.dwProcessId, pi.dwProcessId);
                ok_eq_hex(debugEvent.u.Exception.ExceptionRecord.ExceptionCode, ExpectedExceptionCode);
                ok_eq_long(debugEvent.u.Exception.dwFirstChance, (LONG)ExpectedFirstChance);
                break;
            }
        }

        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
    }

    ok(GotException != FALSE, "Didn't get an exception event\n");

    TerminateProcess(pi.hProcess, 0);
}

START_TEST(Exceptions)
{
    int ret;

    /* Check for nested invocation */
    if (__argc > 2)
    {
        /* All of these tests will terminate with an error code that indicates
           whether the test was successful or not. */
        if (strcmp(__argv[2], "HandleRaiseException") == 0)
        {
            RunHandleRaiseExceptionTest();
        }
        if (strcmp(__argv[2], "ContinueRaiseException") == 0)
        {
            RunContinueRaiseExceptionTest();
        }
        else if (strcmp(__argv[2], "HandleNoncontinuableException") == 0)
        {
            RunHandleNoncontinuableExceptionTest();
        }
        else if (strcmp(__argv[2], "ContinueNoncontinuableException") == 0)
        {
            RunContinueNoncontinuableExceptionTest();
        }
        else if (strcmp(__argv[2], "FilterFastfail") == 0)
        {
            RunFilterFastfailTest();
        }
        else
        {
            ok(FALSE, "Unknown subtest: %s\n", __argv[2]);
        }

        return;
    }

    ret = RunSubtest("HandleRaiseException");
    ok(ret == 1, "HandleRaiseException test with wrong exit code 0x%x\n", ret);

    ret = RunSubtest("ContinueRaiseException");
    ok(ret == 1, "ContinueRaiseException test with wrong exit code 0x%x\n", ret);

    ret = RunSubtest("HandleNoncontinuableException");
    ok(ret == 1, "HandleNoncontinuableException test with wrong exit code 0x%x\n", ret);

    ret = RunSubtest("ContinueNoncontinuableException");
    ok(ret == STATUS_ACCESS_VIOLATION, "ContinueNoncontinuableException test with wrong exit code 0x%x\n", ret);

    ret = RunSubtest("FilterFastfail");
    ok(ret == STATUS_STACK_BUFFER_OVERRUN, "FilterFastfail test with wrong exit code 0x%x\n", ret);

    DebugSubtest("FilterFastfail", STATUS_STACK_BUFFER_OVERRUN, FALSE);
}
