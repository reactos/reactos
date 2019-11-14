/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for cmd.exe
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#define TIMEOUT 3000

typedef struct TEST_ENTRY
{
    INT line;
    DWORD dwExitCode;
    const char *cmdline;
    BOOL bStdOutput;
    BOOL bStdError;
} TEST_ENTRY;

static const TEST_ENTRY s_exit_entries[] =
{
    { __LINE__, 0,      "cmd /c exit" },
    { __LINE__, 0,      "cmd /c exit 0" },
    { __LINE__, 0,      "cmd /c exit \"\"" },
    { __LINE__, 0,      "cmd /c exit ABC" },
    { __LINE__, 0,      "cmd /c exit \"ABC" },
    { __LINE__, 0,      "cmd /c exit \"ABC\"" },
    { __LINE__, 1234,   "cmd /c exit 1234" },
};

static const TEST_ENTRY s_echo_entries[] =
{
    { __LINE__, 0,      "cmd /c echo", TRUE, FALSE },
    { __LINE__, 0,      "cmd /c echo.", TRUE, FALSE },
};

static const TEST_ENTRY s_cd_entries[] =
{
    { __LINE__, 0,      "cmd /c cd \"C:\\ ", },
    { __LINE__, 0,      "cmd /c cd C:/", },
    { __LINE__, 0,      "cmd /c cd \"\"", TRUE, FALSE },
    { __LINE__, 0,      "cmd /c cd", TRUE, FALSE },
    { __LINE__, 1,      "cmd /c cd \" \" && exit 1234", FALSE, TRUE },
    { __LINE__, 1234,   "cmd /c cd C:\\Program Files && exit 1234" },
    { __LINE__, 1234,   "cmd /c cd \"C:\\ \" && exit 1234", },
    { __LINE__, 1234,   "cmd /c cd \"C:\\Program Files\" && exit 1234", },
    { __LINE__, 1234,   "cmd /c cd \"\" && exit 1234", TRUE, FALSE },
    { __LINE__, 1234,   "cmd /c cd \\ && exit 1234" },
};

static const TEST_ENTRY s_pushd_entries[] =
{
    { __LINE__, 0,      "cmd /c pushd \"C:\\ " },
    { __LINE__, 0,      "cmd /c pushd \"C:\\ \"" },
    { __LINE__, 0,      "cmd /c pushd \"C:\\ \"\"" },
    { __LINE__, 0,      "cmd /c pushd \"C:\\\"" },
    { __LINE__, 1,      "cmd /c popd /?", TRUE, FALSE },
    { __LINE__, 1,      "cmd /c popd ABC" },
    { __LINE__, 1,      "cmd /c popd \" " },
    { __LINE__, 1,      "cmd /c popd \"\"" },
    { __LINE__, 1,      "cmd /c popd" },
    { __LINE__, 1,      "cmd /c pushd ABC", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd C:/Program Files && popd && exit 1234", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd C:\\ C:\\ && popd && exit 1234", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd C:\\Invalid Directory && exit 1234", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd C:\\Invalid Directory && popd && exit 1234", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd \" C:\\ ", FALSE, TRUE },
    { __LINE__, 1,      "cmd /c pushd \"C:\\ C:\\\" && popd && exit 1234", FALSE, TRUE },
    { __LINE__, 1234,   "cmd /c pushd && exit 1234 " },
    { __LINE__, 1234,   "cmd /c pushd C:\\ && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd C:\\ \"\" && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd C:\\Program Files && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd \"C:/Program Files/\" && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd \"C:/Program Files\" && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd \"C:\\Program Files\" && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd \"C:\\Program Files\\\" && popd && exit 1234" },
    { __LINE__, 1234,   "cmd /c pushd \"C:\\\" && popd && exit 1234" },
};

static BOOL MyDuplicateHandle(HANDLE hFile, PHANDLE phFile, BOOL bInherit)
{
    HANDLE hProcess = GetCurrentProcess();
    return DuplicateHandle(hProcess, hFile, hProcess, phFile, 0,
                           bInherit, DUPLICATE_SAME_ACCESS);
}

static BOOL PrepareForRedirect(STARTUPINFOA *psi, PHANDLE phInputWrite,
                               PHANDLE phOutputRead, PHANDLE phErrorRead)
{
    HANDLE hInputRead = NULL, hInputWriteTmp = NULL;
    HANDLE hOutputReadTmp = NULL, hOutputWrite = NULL;
    HANDLE hErrorReadTmp = NULL, hErrorWrite = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };

    psi->hStdInput = NULL;
    psi->hStdOutput = NULL;
    psi->hStdError = NULL;

    if (phInputWrite)
    {
        if (CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
        {
            if (!MyDuplicateHandle(hInputWriteTmp, phInputWrite, FALSE))
                goto failed;

            CloseHandle(hInputWriteTmp);
        }
        else
            goto failed;
    }

    if (phOutputRead)
    {
        if (CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
        {
            if (!MyDuplicateHandle(hOutputReadTmp, phOutputRead, FALSE))
                goto failed;

            CloseHandle(hOutputReadTmp);
        }
        else
            goto failed;
    }

    if (phOutputRead && phOutputRead == phErrorRead)
    {
        if (!MyDuplicateHandle(hOutputWrite, &hErrorWrite, TRUE))
            goto failed;
    }
    else if (phErrorRead)
    {
        if (CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0))
        {
            if (!MyDuplicateHandle(hErrorReadTmp, phErrorRead, FALSE))
                goto failed;
            CloseHandle(hErrorReadTmp);
        }
        else
            goto failed;
    }

    if (phInputWrite)
        psi->hStdInput = hInputRead;
    if (phOutputRead)
        psi->hStdOutput = hOutputWrite;
    if (phErrorRead)
        psi->hStdError = hErrorWrite;

    return TRUE;

failed:
    CloseHandle(hInputRead);
    CloseHandle(hInputWriteTmp);
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    CloseHandle(hErrorReadTmp);
    CloseHandle(hErrorWrite);
    return FALSE;
}

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD dwExitCode, dwWait;
    HANDLE hOutputRead = NULL;
    HANDLE hErrorRead = NULL;
    BYTE b;
    DWORD dwRead;
    BOOL bStdOutput, bStdError;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    if (!PrepareForRedirect(&si, NULL, &hOutputRead, &hErrorRead))
    {
        skip("PrepareForRedirect failed\n");
        return;
    }

    if (CreateProcessA(NULL, (char *)pEntry->cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(si.hStdInput);
        dwWait = WaitForSingleObject(pi.hProcess, TIMEOUT);
        if (dwWait == WAIT_TIMEOUT)
        {
            TerminateProcess(pi.hProcess, 9999);
        }
        GetExitCodeProcess(pi.hProcess, &dwExitCode);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
    {
        dwExitCode = 8888;
    }

    PeekNamedPipe(hOutputRead, &b, 1, &dwRead, NULL, NULL);
    bStdOutput = dwRead != 0;
    PeekNamedPipe(hErrorRead, &b, 1, &dwRead, NULL, NULL);
    bStdError = dwRead != 0;

    if (si.hStdInput)
        CloseHandle(si.hStdInput);
    if (si.hStdOutput)
        CloseHandle(si.hStdOutput);
    if (si.hStdError)
        CloseHandle(si.hStdError);

    ok(pEntry->bStdOutput == bStdOutput,
       "Line %u: bStdOutput %d vs %d\n",
       pEntry->line, pEntry->bStdOutput, bStdOutput);

    ok(pEntry->bStdError == bStdError,
       "Line %u: bStdError %d vs %d\n",
       pEntry->line, pEntry->bStdError, bStdError);

    ok(pEntry->dwExitCode == dwExitCode,
       "Line %u: dwExitCode %ld vs %ld\n",
       pEntry->line, pEntry->dwExitCode, dwExitCode);
}

START_TEST(exit)
{
    SIZE_T i;
    for (i = 0; i < ARRAYSIZE(s_exit_entries); ++i)
    {
        DoTestEntry(&s_exit_entries[i]);
    }
}

START_TEST(echo)
{
    SIZE_T i;
    for (i = 0; i < ARRAYSIZE(s_echo_entries); ++i)
    {
        DoTestEntry(&s_echo_entries[i]);
    }
}

START_TEST(cd)
{
    SIZE_T i;
    for (i = 0; i < ARRAYSIZE(s_cd_entries); ++i)
    {
        DoTestEntry(&s_cd_entries[i]);
    }
}

START_TEST(pushd)
{
    SIZE_T i;
    for (i = 0; i < ARRAYSIZE(s_pushd_entries); ++i)
    {
        DoTestEntry(&s_pushd_entries[i]);
    }
}
