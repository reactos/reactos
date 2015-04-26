/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for CRT command-line handling.
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO <hermes.belusca@sfr.fr>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/umtypes.h>

#include "./CmdLineUtil/CmdLineUtil.h"

#define COUNT_OF(x) (sizeof((x))/sizeof((x)[0]))

/**
 * Extracts the command tail from the command line
 * (deletes the program's name and keep the rest).
 **/
#define SPACECHAR   L' '
#define DQUOTECHAR  L'"'

LPWSTR ExtractCmdLine(IN LPWSTR lpszCommandLine)
{
    BOOL inDoubleQuote = FALSE;

    /*
     * Skip the program's name (the first token in the command line).
     * Handle quoted program's name.
     */
    if (lpszCommandLine)
    {
        while ( (*lpszCommandLine > SPACECHAR) ||
                (*lpszCommandLine && inDoubleQuote) )
        {
            if (*lpszCommandLine == DQUOTECHAR)
                inDoubleQuote = !inDoubleQuote;

            ++lpszCommandLine;
        }

        /* Skip all white spaces preceeding the second token. */
        while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
            ++lpszCommandLine;
    }

    return lpszCommandLine;
}

VOID ExtractCmdLine_U(IN OUT PUNICODE_STRING pCommandLine_U)
{
    BOOL inDoubleQuote = FALSE;
    PWSTR lpszCommandLine;

    /*
     * Skip the program's name (the first token in the command line).
     * Handle quoted program's name.
     */
    if (pCommandLine_U && pCommandLine_U->Buffer && (pCommandLine_U->Length != 0))
    {
        lpszCommandLine = pCommandLine_U->Buffer;

        while ( (pCommandLine_U->Length > 0) &&
                ( (*lpszCommandLine > SPACECHAR) ||
                  (*lpszCommandLine && inDoubleQuote) ) )
        {
            if (*lpszCommandLine == DQUOTECHAR)
                inDoubleQuote = !inDoubleQuote;

            ++lpszCommandLine;
            pCommandLine_U->Length -= sizeof(WCHAR);
        }

        /* Skip all white spaces preceeding the second token. */
        while ((pCommandLine_U->Length > 0) && *lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
        {
            ++lpszCommandLine;
            pCommandLine_U->Length -= sizeof(WCHAR);
        }

        pCommandLine_U->Buffer = lpszCommandLine;
    }

    return;
}

/******************************************************************************/

/* The path to the utility program run by this test. */
static WCHAR UtilityProgramDirectory[MAX_PATH];

/* The list of tests. */
typedef struct _TEST_CASE
{
    LPWSTR CmdLine;
    BOOL   bEncloseProgramNameInQuotes;
} TEST_CASE, *PTEST_CASE;

static TEST_CASE TestCases[] =
{
    {L"", FALSE},
    {L"foo bar", FALSE},
    {L"\"foo bar\"", FALSE},
    {L"foo \"bar John\" Doe", FALSE},

    {L"", TRUE},
    {L"foo bar", TRUE},
    {L"\"foo bar\"", TRUE},
    {L"foo \"bar John\" Doe", TRUE},
};

static void Test_CommandLine(IN ULONG TestNumber,
                             IN PTEST_CASE TestCase)
{
    BOOL bRet;

    BOOL bWasntInQuotes = (UtilityProgramDirectory[0] != L'"');
    WCHAR CmdLine[MAX_PATH] = L"";
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);


    /* Initialize the command line. */
    if (TestCase->bEncloseProgramNameInQuotes && bWasntInQuotes)
        wcscpy(CmdLine, L"\"");

    wcscat(CmdLine, UtilityProgramDirectory);

    if (TestCase->bEncloseProgramNameInQuotes && bWasntInQuotes)
        wcscat(CmdLine, L"\"");

    /* Add a separating space and copy the tested command line parameters. */
    wcscat(CmdLine, L" ");
    wcscat(CmdLine, TestCase->CmdLine);


    /*
     * Launch the utility program and wait till it's terminated.
     */
    bRet = CreateProcessW(NULL,
                          CmdLine,
                          NULL, NULL,
                          FALSE,
                          CREATE_UNICODE_ENVIRONMENT,
                          NULL, NULL,
                          &si, &pi);
    ok(bRet, "Test %lu - Failed to launch ' %S ', error = %lu.\n", TestNumber, CmdLine, GetLastError());

    if (bRet)
    {
        /* Wait until child process exits. */
        WaitForSingleObject(pi.hProcess, INFINITE);

        /* Close process and thread handles. */
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    /*
     * Analyses the result.
     */
    {
        /* Open the data file. */
        HANDLE hFile = CreateFileW(DATAFILE,
                                   GENERIC_READ,
                                   0, NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        ok(hFile != INVALID_HANDLE_VALUE, "Test %lu - Failed to open the data file 'C:\\cmdline.dat', error = %lu.\n", TestNumber, GetLastError());

        if (hFile != INVALID_HANDLE_VALUE)
        {
            WCHAR BuffWinMain[MAX_PATH]; LPWSTR WinMainCmdLine = BuffWinMain;
            WCHAR BuffWin32[MAX_PATH]  ; LPWSTR Win32CmdLine   = BuffWin32  ;
            WCHAR BuffNT[0xffff /* Maximum USHORT size */];
            UNICODE_STRING NTCmdLine;

            DWORD dwSize, dwStringSize;

            /*
             * Format of the data file :
             *
             * [size_of_string 4 bytes][null_terminated_C_string]
             * [size_of_string 4 bytes][null_terminated_C_string]
             * [UNICODE_STRING_structure][string_buffer_of_UNICODE_STRING]
             */

            /* 1- Read the WinMain's command line. */
            dwStringSize = 0;

            ReadFile(hFile,
                     &dwStringSize,
                     sizeof(dwStringSize),
                     &dwSize,
                     NULL);

            dwStringSize = min(dwStringSize, sizeof(BuffWinMain));
            ReadFile(hFile,
                     WinMainCmdLine,
                     dwStringSize,
                     &dwSize,
                     NULL);
            *(LPWSTR)((ULONG_PTR)WinMainCmdLine + dwStringSize) = 0;

            /* 2- Read the Win32 mode command line. */
            dwStringSize = 0;

            ReadFile(hFile,
                     &dwStringSize,
                     sizeof(dwStringSize),
                     &dwSize,
                     NULL);

            dwStringSize = min(dwStringSize, sizeof(BuffWin32));
            ReadFile(hFile,
                     Win32CmdLine,
                     dwStringSize,
                     &dwSize,
                     NULL);
            *(LPWSTR)((ULONG_PTR)Win32CmdLine + dwStringSize) = 0;

            /* 3- Finally, read the UNICODE_STRING command line. */
            ReadFile(hFile,
                     &NTCmdLine,
                     sizeof(NTCmdLine),
                     &dwSize,
                     NULL);

            NTCmdLine.Buffer = BuffNT;
            ReadFile(hFile,
                     NTCmdLine.Buffer,
                     NTCmdLine.Length,
                     &dwSize,
                     NULL);

            /* Now close the file. */
            CloseHandle(hFile);

            /*
             * Remove the program's name in the Win32 and NT command lines.
             */
            Win32CmdLine = ExtractCmdLine(Win32CmdLine);
            ExtractCmdLine_U(&NTCmdLine);

            /* Print the results */
            /*
            *(LPWSTR)((ULONG_PTR)NTCmdLine.Buffer + NTCmdLine.Length) = 0;
            printf("WinMain cmdline = '%S'\n"
                   "Win32   cmdline = '%S'\n"
                   "NT      cmdline = '%S'\n"
                   "NT       length = %u\n",
                   WinMainCmdLine,
                   Win32CmdLine,
                   NTCmdLine.Buffer, NTCmdLine.Length);
            */

            /*
             * Now check the results.
             */
            dwStringSize = min(wcslen(WinMainCmdLine), wcslen(Win32CmdLine));
            ok(wcslen(WinMainCmdLine) == wcslen(Win32CmdLine), "Test %lu - WinMain and Win32 command lines do not have the same length !\n", TestNumber);
            ok(wcsncmp(WinMainCmdLine, Win32CmdLine, dwStringSize) == 0, "Test %lu - WinMain and Win32 command lines are different !\n", TestNumber);

            dwStringSize = min(wcslen(WinMainCmdLine), NTCmdLine.Length / sizeof(WCHAR));
            ok(wcsncmp(WinMainCmdLine, NTCmdLine.Buffer, dwStringSize) == 0, "Test %lu - WinMain and NT command lines are different !\n", TestNumber);

            dwStringSize = min(wcslen(Win32CmdLine), NTCmdLine.Length / sizeof(WCHAR));
            ok(wcsncmp(Win32CmdLine, NTCmdLine.Buffer, dwStringSize) == 0, "Test %lu - Win32 and NT command lines are different !\n", TestNumber);
        }
    }

    /*
     * Always delete the data file.
     */
    DeleteFileW(DATAFILE);
}

START_TEST(CommandLine)
{
    ULONG i;

    DWORD dwRet;
    LPWSTR p = NULL;


    /*
     * Initialize the UtilityProgramDirectory variable.
     */
    dwRet = GetModuleFileNameW(NULL, UtilityProgramDirectory, COUNT_OF(UtilityProgramDirectory));
    ok(dwRet != 0, "ERROR: Cannot retrieve the path to the current running process, last error %lu\n", GetLastError());
    if (dwRet == 0) return;

    /* Path : executable.exe or "executable.exe" or C:\path\executable.exe or "C:\path\executable.exe" */
    p = wcsrchr(UtilityProgramDirectory, L'\\');
    if (p && *p != 0)
        *++p = 0; /* Null-terminate there : C:\path\ or "C:\path\ */
    else
        UtilityProgramDirectory[0] = 0; /* Suppress the executable.exe name */

    wcscat(UtilityProgramDirectory, L"testdata\\CmdLineUtil.exe");

    /* Close the opened quote if needed. */
    if (UtilityProgramDirectory[0] == L'"') wcscat(UtilityProgramDirectory, L"\"");


    /*
     * Now launch the tests.
     */
    for (i = 0 ; i < COUNT_OF(TestCases) ; ++i)
    {
        Test_CommandLine(i, &TestCases[i]);
    }
}

/* EOF */
