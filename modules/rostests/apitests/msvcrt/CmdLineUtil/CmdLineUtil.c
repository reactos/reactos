/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for CRT command-line handling - Utility GUI program.
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO <hermes.belusca@sfr.fr>
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <ndk/rtlfuncs.h>

#include "CmdLineUtil.h"

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    /*
     * Get the unparsed command line as seen in Win32 mode,
     * and the NT-native mode one.
     */
    LPWSTR CmdLine = GetCommandLineW();
    UNICODE_STRING CmdLine_U = NtCurrentPeb()->ProcessParameters->CommandLine;

    /* Write the results into a file. */
    HANDLE hFile = CreateFileW(DATAFILE,
                               GENERIC_WRITE,
                               0, NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwSize, dwStringSize;

        /*
         * Format of the data file :
         *
         * [size_of_string 4 bytes][null_terminated_C_string]
         * [size_of_string 4 bytes][null_terminated_C_string]
         * [UNICODE_STRING_structure][string_buffer_of_UNICODE_STRING]
         */

        /* 1- Write the WinMain's command line. */
        dwStringSize = (lstrlenW(lpCmdLine) + 1) * sizeof(WCHAR);

        WriteFile(hFile,
                  &dwStringSize,
                  sizeof(dwStringSize),
                  &dwSize,
                  NULL);

        WriteFile(hFile,
                  lpCmdLine,
                  dwStringSize,
                  &dwSize,
                  NULL);

        /* 2- Write the Win32 mode command line. */
        dwStringSize = (lstrlenW(CmdLine) + 1) * sizeof(WCHAR);

        WriteFile(hFile,
                  &dwStringSize,
                  sizeof(dwStringSize),
                  &dwSize,
                  NULL);

        WriteFile(hFile,
                  CmdLine,
                  dwStringSize,
                  &dwSize,
                  NULL);

        /* 3- Finally, write the UNICODE_STRING command line. */
        WriteFile(hFile,
                  &CmdLine_U,
                  sizeof(CmdLine_U),
                  &dwSize,
                  NULL);

        WriteFile(hFile,
                  CmdLine_U.Buffer,
                  CmdLine_U.Length,
                  &dwSize,
                  NULL);

        /* Now close the file. */
        CloseHandle(hFile);
    }

    return 0;
}

/* EOF */
