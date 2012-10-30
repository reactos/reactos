
#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "resource.h"

START_TEST(LoadImage)
{
    char path[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    HANDLE handle;

    char **test_argv;
    int argc = winetest_get_mainargs( &test_argv );

    /* Now check its behaviour regarding Shared icons/cursors */
    handle = LoadImageW( GetModuleHandle(NULL), L"TESTCURSOR", IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE );
    ok(handle != 0, "\n");

    if (argc >= 3)
    {
        HANDLE arg;

        sscanf (test_argv[2], "%lu", (ULONG_PTR*) &arg);

        ok(handle != arg, "Got same handles\n");

        return;
    }

    /* Start child process */
    sprintf( path, "%s LoadImage %lu", test_argv[0], (ULONG_PTR)handle );
    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);
    CreateProcessA( NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );
    WaitForSingleObject (pi.hProcess, INFINITE);
}