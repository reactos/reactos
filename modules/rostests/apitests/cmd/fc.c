/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for fc.exe
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>

typedef struct TEST_ENTRY
{
    INT lineno;
    INT ret;
    LPCSTR cmdline;
    LPCSTR file1_data;
    LPCSTR file2_data;
    INT file1_size; // -1 for zero-terminated
    INT file2_size; // -1 for zero-terminated
    LPCSTR output;
} TEST_ENTRY;

#define FILE1 "fc-test1.txt"
#define FILE2 "fc-test2.txt"
#define FILES " " FILE1 " " FILE2
#define COMPARING "Comparing files fc-test1.txt and FC-TEST2.TXT\n"

static const TEST_ENTRY s_entries[] =
{
    /* binary comparison */
    { __LINE__, 0, "fc /B" FILES, "", "", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc /B" FILES, "A", "B", -1, -1,
      COMPARING "00000000: 41 42\n" },
    { __LINE__, 1, "fc /B" FILES, "B", "A", -1, -1,
      COMPARING "00000000: 42 41\n" },
    { __LINE__, 0, "fc /B" FILES, "AB", "AB", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc /B" FILES, "AB", "BA", -1, -1,
      COMPARING "00000000: 41 42\n00000001: 42 41\n" },
    { __LINE__, 0, "fc /B" FILES, "ABC", "ABC", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc /B /C" FILES, "ABC", "abc", -1, -1,
      COMPARING "00000000: 41 61\n00000001: 42 62\n00000002: 43 63\n" },
    /* text comparison */
    { __LINE__, 0, "fc" FILES, "", "", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc" FILES, "A", "B", -1, -1,
      COMPARING "***** fc-test1.txt\nA\n***** FC-TEST2.TXT\nB\n*****\n" },
    { __LINE__, 1, "fc" FILES, "B", "A", -1, -1,
      COMPARING "***** fc-test1.txt\nB\n***** FC-TEST2.TXT\nA\n*****\n" },
    { __LINE__, 0, "fc" FILES, "AB", "AB", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc" FILES, "AB", "BA", -1, -1,
      COMPARING "***** fc-test1.txt\nAB\n***** FC-TEST2.TXT\nBA\n*****\n" },
    { __LINE__, 0, "fc" FILES, "ABC", "ABC", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 0, "fc /C" FILES, "ABC", "abc", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc" FILES, "A\nB\nC\nD\nE\n", "A\nB\nB\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nB\nC\nD\n***** FC-TEST2.TXT\nB\nB\nD\n*****\n" },
    { __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nB\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nB\nC\nD\n***** FC-TEST2.TXT\nB\nB\nD\n*****\n" },
    { __LINE__, 1, "fc /A" FILES, "A\nC\nC\nD\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nA\nC\nC\n***** FC-TEST2.TXT\nA\nB\nC\n*****\n" },
    { __LINE__, 1, "fc /A" FILES, "A\nC\nC\nC\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nA\n...\nE\n***** FC-TEST2.TXT\nA\n...\nE\n*****\n" },
    { __LINE__, 1, "fc /A" FILES, "A\nC\nE\nF\nC\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nA\n...\nC\n***** FC-TEST2.TXT\nA\nB\nC\n*****\n" },
    { __LINE__, 1, "fc /A" FILES, "A\nC\nE\nF\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\nA\nC\nE\n"
      "***** FC-TEST2.TXT\nA\n...\nE\n"
      "*****\n\n"
      "***** fc-test1.txt\nF\nE\n***** FC-TEST2.TXT\n*****\n"
    },
    { __LINE__, 1, "fc /N /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nB\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    2:  B\n    3:  C\n    4:  D\n"
      "***** FC-TEST2.TXT\n    2:  B\n    3:  B\n    4:  D\n"
      "*****\n"
    },
    { __LINE__, 1, "fc /N /A" FILES, "A\nC\nC\nD\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    1:  A\n    2:  C\n    3:  C\n"
      "***** FC-TEST2.TXT\n    1:  A\n    2:  B\n    3:  C\n"
      "*****\n"
    },
    { __LINE__, 1, "fc /N /A" FILES, "A\nC\nC\nC\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    1:  A\n...\n    5:  E\n"
      "***** FC-TEST2.TXT\n    1:  A\n...\n    5:  E\n"
      "*****\n" },
    { __LINE__, 1, "fc /N /A" FILES, "A\nC\nE\nF\nC\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    1:  A\n...\n    5:  C\n"
      "***** FC-TEST2.TXT\n    1:  A\n    2:  B\n    3:  C\n"
      "*****\n\n"
      "***** fc-test1.txt\n"
      "***** FC-TEST2.TXT\n    4:  D\n    5:  E\n"
      "*****\n"
    },
    { __LINE__, 1, "fc /N /A" FILES, "A\nC\nE\nF\nE\n", "A\nB\nC\nD\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    1:  A\n    2:  C\n    3:  E\n"
      "***** FC-TEST2.TXT\n    1:  A\n...\n    5:  E\n"
      "*****\n\n"
      "***** fc-test1.txt\n    4:  F\n    5:  E\n"
      "***** FC-TEST2.TXT\n"
      "*****\n"
    },
    { __LINE__, 0, "fc" FILES, "A\n\tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 0, "fc" FILES, "A\n    \tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc /T" FILES, "A\n\tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "" },
    { __LINE__, 1, "fc /T" FILES, "A\n    \tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "***** fc-test1.txt\nA\n    \tB\nC\n***** FC-TEST2.TXT\nA\n        B\nC\n*****\n" },
    { __LINE__, 0, "fc /W" FILES, "A\n    \tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 0, "fc /T /W" FILES, "A\n    \tB\nC\nD\nE\n", "A\n        B\nC\nD\nE\n", -1, -1,
      COMPARING "FC: no differences encountered\n" },
    { __LINE__, 1, "fc /N" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    3:  C\n    4:  D\n    5:  E\n"
      "***** FC-TEST2.TXT\n    3:  C\n    4:  E\n"
      "*****\n"
      "\n"
      "***** fc-test1.txt\n"
      "***** FC-TEST2.TXT\n"
      "    5:  E\n"
      "*****\n"
    },
    { __LINE__, 1, "fc /LB3 /N" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1,
      COMPARING
      "***** fc-test1.txt\n    4:  D\n    5:  E\n"
      "***** FC-TEST2.TXT\n    4:  E\n    5:  E\n"
      "*****\n"
    },
};

BOOL DoDuplicateHandle(HANDLE hFile, PHANDLE phFile, BOOL bInherit)
{
    HANDLE hProcess = GetCurrentProcess();
    return DuplicateHandle(hProcess, hFile, hProcess, phFile, 0,
                           bInherit, DUPLICATE_SAME_ACCESS);
}

static BOOL
PrepareForRedirect(STARTUPINFOA *psi, PHANDLE phInputWrite, PHANDLE phOutputRead, PHANDLE phErrorRead)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hInputRead = NULL, hInputWriteTmp = NULL;
    HANDLE hOutputReadTmp = NULL, hOutputWrite = NULL;
    HANDLE hErrorReadTmp = NULL, hErrorWrite = NULL;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (phInputWrite)
    {
        if (CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
        {
            if (!DoDuplicateHandle(hInputWriteTmp, phInputWrite, FALSE))
                return FALSE;
            CloseHandle(hInputWriteTmp);
        }
        else
            goto failure;
    }

    if (phOutputRead)
    {
        if (CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
        {
            if (!DoDuplicateHandle(hOutputReadTmp, phOutputRead, FALSE))
                return FALSE;
            CloseHandle(hOutputReadTmp);
        }
        else
            goto failure;
    }

    if (phOutputRead && phOutputRead == phErrorRead)
    {
        if (!DoDuplicateHandle(hOutputWrite, &hErrorWrite, TRUE))
            return FALSE;
    }
    else if (phErrorRead)
    {
        if (CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0))
        {
            if (!DoDuplicateHandle(hErrorReadTmp, phErrorRead, FALSE))
                return FALSE;
            CloseHandle(hErrorReadTmp);
        }
        else
            goto failure;
    }

    if (phInputWrite)
    {
        psi->hStdInput = hInputRead;
        psi->dwFlags |= STARTF_USESTDHANDLES;
    }
    if (phOutputRead)
    {
        psi->hStdOutput = hOutputWrite;
        psi->dwFlags |= STARTF_USESTDHANDLES;
    }
    if (phErrorRead)
    {
        psi->hStdOutput = hErrorWrite;
        psi->dwFlags |= STARTF_USESTDHANDLES;
    }
    return TRUE;

failure:
    CloseHandle(hInputRead);
    CloseHandle(hInputWriteTmp);
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    CloseHandle(hErrorReadTmp);
    CloseHandle(hErrorWrite);
    return FALSE;
}

static void ConvertOutput(LPSTR psz)
{
    LPSTR pch1, pch2;
    pch1 = pch2 = psz;
    while (*pch1)
    {
        if (*pch1 != '\r')
        {
            *pch2++ = *pch1;
        }
        ++pch1;
    }
    *pch2 = 0;
}

static void DoTestEntry(const TEST_ENTRY* pEntry)
{
    FILE *fp;
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    INT file1_size, file2_size, i;
    HANDLE hOutputRead;
    DWORD cbAvail, cbRead;
    CHAR szOutput[1024];
    BOOL ret;
    DWORD dwExitCode;

    file1_size = pEntry->file1_size;
    file2_size = pEntry->file2_size;
    if (file1_size == -1)
        file1_size = strlen(pEntry->file1_data);
    if (file2_size == -1)
        file2_size = strlen(pEntry->file2_data);

    fp = fopen(FILE1, "wb");
    fwrite(pEntry->file1_data, file1_size, 1, fp);
    fclose(fp);

    fp = fopen(FILE2, "wb");
    fwrite(pEntry->file2_data, file2_size, 1, fp);
    fclose(fp);

    ok(PathFileExistsA(FILE1), "Line %d: PathFileExistsA(FILE1) failed\n", pEntry->lineno);
    ok(PathFileExistsA(FILE2), "Line %d: PathFileExistsA(FILE2) failed\n", pEntry->lineno);

    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PrepareForRedirect(&si, NULL, &hOutputRead, &hOutputRead);

    ret = CreateProcessA(NULL, (LPSTR)pEntry->cmdline, NULL, NULL, TRUE,
                         0, NULL, NULL, &si, &pi);
    ok(ret, "Line %d: CreateProcessA failed\n", pEntry->lineno);

#define RETRY_COUNT 64
#define SLEEP_TIME 100
    for (i = 0; i < RETRY_COUNT; ++i)
    {
        GetExitCodeProcess(pi.hProcess, &dwExitCode);
        if (dwExitCode != STILL_ACTIVE)
            break;
        Sleep(SLEEP_TIME);
    }

    ZeroMemory(szOutput, sizeof(szOutput));
    if (PeekNamedPipe(hOutputRead, NULL, 0, NULL, &cbAvail, NULL))
    {
        if (cbAvail > 0)
        {
            if (cbAvail > sizeof(szOutput))
                cbAvail = sizeof(szOutput) - 1;

            ReadFile(hOutputRead, szOutput, cbAvail, &cbRead, NULL);
        }
    }
    ConvertOutput(szOutput);

    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    ok(dwExitCode == pEntry->ret, "Line %d: dwExitCode was 0x%lx\n", pEntry->lineno, dwExitCode);

    if (StrCmpNIA(pEntry->output, szOutput, lstrlenA(pEntry->output)) != 0)
    {
        ok(FALSE, "Line %d: Output was wrong\n", pEntry->lineno);
        printf("---FROM HERE\n");
        printf("%s\n", szOutput);
        printf("---UP TO HERE\n");
    }
    else
    {
        ok_int(TRUE, TRUE);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(hOutputRead);
    if (si.hStdInput != GetStdHandle(STD_INPUT_HANDLE))
        CloseHandle(si.hStdInput);
    if (si.hStdOutput != GetStdHandle(STD_OUTPUT_HANDLE))
        CloseHandle(si.hStdOutput);
    if (si.hStdError != GetStdHandle(STD_ERROR_HANDLE))
        CloseHandle(si.hStdError);

    DeleteFileA(FILE1);
    DeleteFileA(FILE2);
}

#define ENGLISH_CP 437

START_TEST(fc)
{
    UINT i;
    UINT uOldCP = GetConsoleCP(), uOldOutputCP = GetConsoleOutputCP();
    SetConsoleCP(ENGLISH_CP);
    SetConsoleOutputCP(ENGLISH_CP);

    for (i = 0; i < _countof(s_entries); ++i)
    {
        DoTestEntry(&s_entries[i]);
    }

    SetConsoleCP(uOldCP);
    SetConsoleOutputCP(uOldOutputCP);
}
