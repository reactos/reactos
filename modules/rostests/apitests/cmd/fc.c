/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for fc.exe
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <stdlib.h>
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
#define NO_DIFF "FC: no differences encountered\n"
#define RESYNC_FAILED "Resync Failed.  Files are too different.\n"

static const TEST_ENTRY s_entries[] =
{
    /* binary comparison */
    {
        __LINE__, 0, "fc /B" FILES, "", "", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /B" FILES, "A", "B", -1, -1, COMPARING
        "00000000: 41 42\n"
    },
    {
        __LINE__, 1, "fc /B" FILES, "B", "A", -1, -1, COMPARING
        "00000000: 42 41\n"
    },
    {
        __LINE__, 0, "fc /B" FILES, "AB", "AB", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /B" FILES, "AB", "BA", -1, -1, COMPARING
        "00000000: 41 42\n"
        "00000001: 42 41\n"
    },
    {
        __LINE__, 0, "fc /B" FILES, "ABC", "ABC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /B" FILES, "ABC", "ABCD", -1, -1, COMPARING
        "FC: FC-TEST2.TXT longer than fc-test1.txt\n\n"
    },
    {
        __LINE__, 1, "fc /B" FILES, "ABC", "ABDD", -1, -1, COMPARING
        "00000002: 43 44\n"
        "FC: FC-TEST2.TXT longer than fc-test1.txt\n"
    },
    {
        __LINE__, 1, "fc /B /C" FILES, "ABC", "abc", -1, -1, COMPARING
        "00000000: 41 61\n"
        "00000001: 42 62\n"
        "00000002: 43 63\n"
    },
    /* text comparison */
    {
        __LINE__, 0, "fc" FILES, "", "", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc" FILES, "A", "B", -1, -1, COMPARING
        "***** fc-test1.txt\nA\n"
        "***** FC-TEST2.TXT\nB\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc" FILES, "B", "A", -1, -1, COMPARING
        "***** fc-test1.txt\nB\n"
        "***** FC-TEST2.TXT\nA\n"
        "*****\n\n"
    },
    {
        __LINE__, 0, "fc" FILES, "AB", "AB", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc" FILES, "AB", "BA", -1, -1, COMPARING
        "***** fc-test1.txt\nAB\n"
        "***** FC-TEST2.TXT\nBA\n"
        "*****\n\n"
    },
    {
        __LINE__, 0, "fc" FILES, "ABC", "ABC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 0, "fc /C" FILES, "ABC", "abc", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc" FILES, "A\nB\nC\nD\nE\n", "A\nB\nB\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nB\nC\nD\n"
        "***** FC-TEST2.TXT\nB\nB\nD\n"
        "*****\n\n"
    },
    /* Test /A */
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "B\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nB\n"
        "***** FC-TEST2.TXT\nB\nB\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "C\nC\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nB\nC\n"
        "***** FC-TEST2.TXT\nC\nC\nC\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nC\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nB\nC\n"
        "***** FC-TEST2.TXT\nA\nC\nC\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nC\nC\nC\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\n...\nE\n"
        "***** FC-TEST2.TXT\nA\n...\nE\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nD\nF\n", -1, -1, COMPARING
        "***** fc-test1.txt\nD\nE\n"
        "***** FC-TEST2.TXT\nD\nF\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nF\nF\n", -1, -1, COMPARING
        "***** fc-test1.txt\nC\nD\nE\n"
        "***** FC-TEST2.TXT\nC\nF\nF\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nF\nF\nF\n", -1, -1, COMPARING
        "***** fc-test1.txt\nB\n...\nE\n"
        "***** FC-TEST2.TXT\nB\n...\nF\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /A" FILES, "A\nC\nE\nF\nE\n", "A\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nC\nE\n"
        "***** FC-TEST2.TXT\nA\n...\nE\n"
        "*****\n\n"
        "***** fc-test1.txt\nF\nE\n"
        "***** FC-TEST2.TXT\n"
        "*****\n\n"
    },
    /* Test /N /A */
    {
        __LINE__, 1, "fc /N /A" FILES, "A\nB\nC\nD\nE\n", "A\nB\nB\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    2:  B\n    3:  C\n    4:  D\n"
        "***** FC-TEST2.TXT\n    2:  B\n    3:  B\n    4:  D\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /N /A" FILES, "A\nC\nC\nD\nE\n", "A\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    1:  A\n    2:  C\n    3:  C\n"
        "***** FC-TEST2.TXT\n    1:  A\n    2:  B\n    3:  C\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /N /A" FILES, "A\nC\nC\nC\nE\n", "A\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    1:  A\n...\n    5:  E\n"
        "***** FC-TEST2.TXT\n    1:  A\n...\n    5:  E\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /N /A" FILES, "A\nC\nE\nF\nC\n", "A\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    1:  A\n...\n    5:  C\n"
        "***** FC-TEST2.TXT\n    1:  A\n    2:  B\n    3:  C\n"
        "*****\n\n"
        "***** fc-test1.txt\n"
        "***** FC-TEST2.TXT\n    4:  D\n    5:  E\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /N /A" FILES, "A\nC\nE\nF\nE\n", "A\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    1:  A\n    2:  C\n    3:  E\n"
        "***** FC-TEST2.TXT\n    1:  A\n...\n    5:  E\n"
        "*****\n\n"
        "***** fc-test1.txt\n    4:  F\n    5:  E\n"
        "***** FC-TEST2.TXT\n"
        "*****\n\n"
    },
    /* Test tab expansion */
    {
        __LINE__, 0, "fc" FILES, "A\n\tB\nC", "A\n        B\nC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 0, "fc" FILES, "A\n    \tB\nC", "A\n        B\nC", -1, -1, COMPARING
        NO_DIFF
    },
    /* Test /T */
    {
        __LINE__, 1, "fc /T" FILES, "A\n\tB\nC", "A\n        B\nC", -1, -1, COMPARING
        "***** fc-test1.txt\nA\n\tB\nC\n"
        "***** FC-TEST2.TXT\nA\n        B\nC\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /T" FILES, "A\n    \tB\nC", "A\n        B\nC", -1, -1, COMPARING
        "***** fc-test1.txt\nA\n    \tB\nC\n"
        "***** FC-TEST2.TXT\nA\n        B\nC\n"
        "*****\n\n"
    },
    /* Test /W */
    {
        __LINE__, 0, "fc /W" FILES, "A\n    \tB\nC", "A\n        B\nC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 0, "fc /W" FILES, "\tA \nB\n", "A\nB\n", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /W" FILES, "        A \nB\n", "AB\nB\n", -1, -1, COMPARING
        "***** fc-test1.txt\n        A \nB\n"
        "***** FC-TEST2.TXT\nAB\nB\n"
        "*****\n\n"
    },
    /* TEST /W /T */
    {
        __LINE__, 0, "fc /W /T" FILES, "A\n    \tB\nC", "A\n        B\nC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 0, "fc /W /T" FILES, "A\n    \tB\nC", "A\n        B\nC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /W /T" FILES, "\tA \nB\n", "AB\nB\n", -1, -1, COMPARING
        "***** fc-test1.txt\n\tA \nB\n"
        "***** FC-TEST2.TXT\nAB\nB\n"
        "*****\n\n"
    },
    /* Test /N */
    {
        __LINE__, 1, "fc /N" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\n    3:  C\n    4:  D\n    5:  E\n"
        "***** FC-TEST2.TXT\n    3:  C\n    4:  E\n"
        "*****\n\n"
        "***** fc-test1.txt\n"
        "***** FC-TEST2.TXT\n    5:  E\n"
        "*****\n\n"
    },
    /* Test NUL */
    {
        __LINE__, 1, "fc" FILES, "ABC\000DE", "ABC\000\000\000", 6, 6, COMPARING
        "***** fc-test1.txt\nABC\nDE\n"
        "***** FC-TEST2.TXT\nABC\n\n\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc" FILES, "ABC\000DE", "ABC\n\000\000", 6, 6, COMPARING
        "***** fc-test1.txt\nABC\nDE\n"
        "***** FC-TEST2.TXT\nABC\n\n\n"
        "*****\n\n"
    },
    {
        __LINE__, 0, "fc" FILES, "ABC\000DE", "ABC\nDE", 6, 6, COMPARING
        NO_DIFF
    },
    /* Test CR ('\r') */
    {
        __LINE__, 0, "fc" FILES, "ABC\nABC", "ABC\r\nABC", -1, -1, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc" FILES, "ABC\nABC", "ABC\r\r\nABC", -1, -1, COMPARING
        "***** fc-test1.txt\nABC\nABC\n"
        "***** FC-TEST2.TXT\nABC\nABC\n"
        "*****\n\n"
    },
    /* Test '\n' at EOF */
    {
        __LINE__, 0, "fc" FILES, "ABC", "ABC\n", -1, -1, COMPARING
        NO_DIFF
    },
    /* Test /U */
    {
        /* L"AB" */
        __LINE__, 0, "fc /U" FILES, "A\000B\000", "A\000B\000", 4, 4, COMPARING
        NO_DIFF
    },
    {
        __LINE__, 1, "fc /U" FILES, "A\000B\000", "A\000C\000", 4, 4, COMPARING
        "***** fc-test1.txt\nAB\n"
        "***** FC-TEST2.TXT\nAC\n"
        "*****\n\n"
    },
    /* Test /LB2 */
    {
        __LINE__, 1, "fc /LB2" FILES, "A\nB\nC\nD\nE\n", "B\nB\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nB\n"
        "***** FC-TEST2.TXT\nB\nB\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /LB2" FILES, "A\nB\nC\nD\nE\n", "C\nC\nC\nD\nE\n", -1, -1, COMPARING
        RESYNC_FAILED
        "***** fc-test1.txt\nA\nB\n"
        "***** FC-TEST2.TXT\nC\nC\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /LB2" FILES, "A\nB\nC\nD\nE\n", "D\nD\nD\nD\nE\n", -1, -1, COMPARING
        RESYNC_FAILED
        "***** fc-test1.txt\nA\nB\n"
        "***** FC-TEST2.TXT\nD\nD\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /LB2" FILES, "A\nB\nC\nD\nE\n", "A\nC\nC\nD\nE\n", -1, -1, COMPARING
        RESYNC_FAILED
        "***** fc-test1.txt\nA\nB\n"
        "***** FC-TEST2.TXT\nA\nC\n"
        "*****\n\n"
    },
    /* Test /LB3 */
    {
        __LINE__, 1, "fc /LB3" FILES, "A\nB\nC\nD\nE\n", "C\nC\nC\nD\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nA\nB\nC\n"
        "***** FC-TEST2.TXT\nC\nC\n"
        "*****\n\n"
        "***** fc-test1.txt\nC\nD\n"
        "***** FC-TEST2.TXT\nC\nC\nD\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /LB3" FILES, "A\nB\nC\nD\nE\n", "D\nD\nD\nD\nE\n", -1, -1, COMPARING
        RESYNC_FAILED
        "***** fc-test1.txt\nA\nB\nC\n"
        "***** FC-TEST2.TXT\nD\nD\nD\n"
        "*****\n\n"
    },
    /* Test /N /LB2 */
    {
        __LINE__, 1, "fc /N /LB2" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1, COMPARING
        RESYNC_FAILED
        "***** fc-test1.txt\n    3:  C\n    4:  D\n"
        "***** FC-TEST2.TXT\n    3:  C\n    4:  E\n"
        "*****\n\n"
    },
    /* Test /1 */
    {
        __LINE__, 1, "fc /1" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nC\nD\nE\n"
        "***** FC-TEST2.TXT\nC\nE\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /1" FILES, "A\nB\nC\nD\nE\n", "A\nB\nX\nX\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nB\nC\nD\nE\n"
        "***** FC-TEST2.TXT\nB\nX\nX\nE\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /1" FILES, "A\nB\nC\nD\nE\nF\n", "A\nB\nX\nD\nX\nF", -1, -1, COMPARING
        "***** fc-test1.txt\nB\nC\nD\n"
        "***** FC-TEST2.TXT\nB\nX\nD\n"
        "*****\n\n"
        "***** fc-test1.txt\nD\nE\nF\n"
        "***** FC-TEST2.TXT\nD\nX\nF\n"
        "*****\n\n"
    },
    /* Test /3 */
    {
        __LINE__, 1, "fc /3" FILES, "A\nB\nC\nD\nE\n", "A\nB\nC\nE\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nC\nD\nE\n"
        "***** FC-TEST2.TXT\nC\nE\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /3" FILES, "A\nB\nC\nD\nE\n", "A\nB\nX\nX\nE\n", -1, -1, COMPARING
        "***** fc-test1.txt\nB\nC\nD\nE\n"
        "***** FC-TEST2.TXT\nB\nX\nX\nE\n"
        "*****\n\n"
    },
    {
        __LINE__, 1, "fc /3" FILES, "A\nB\nC\nD\nE\nF\n", "A\nB\nX\nD\nX\nF", -1, -1, COMPARING
        "***** fc-test1.txt\nB\nC\nD\nE\nF\n"
        "***** FC-TEST2.TXT\nB\nX\nD\nX\nF\n"
        "*****\n\n"
    },
};

BOOL DoDuplicateHandle(HANDLE hFile, PHANDLE phFile, BOOL bInherit)
{
    HANDLE hProcess = GetCurrentProcess();
    return DuplicateHandle(hProcess, hFile, hProcess, phFile, 0,
                           bInherit, DUPLICATE_SAME_ACCESS);
}

static BOOL
PrepareForRedirect(STARTUPINFOA *psi, PHANDLE phInputWrite, PHANDLE phOutputRead,
                   PHANDLE phErrorRead)
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
    INT file1_size, file2_size;
    HANDLE hOutputRead;
    DWORD cbAvail, cbRead;
    CHAR szOutput[1024];
    BOOL ret;
    DWORD dwExitCode;
    LPSTR psz;

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

    psz = _strdup(pEntry->cmdline);
    ret = CreateProcessA(NULL, psz, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    free(psz);

    ok(ret, "Line %d: CreateProcessA failed\n", pEntry->lineno);

#define TIMEOUT (10 * 1000)
    WaitForSingleObject(pi.hProcess, TIMEOUT);

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

    if (StrCmpNIA(pEntry->output, szOutput, strlen(pEntry->output)) != 0)
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

#define ENGLISH_CP 437 /* English codepage */

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
