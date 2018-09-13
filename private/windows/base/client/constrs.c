/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    taststrs.c

Abstract:

    Tasking stress test.

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

VOID
NewProcess()
{

    SMALL_RECT Window;

    Window.Left = 0;
    Window.Top = 0;
    Window.Right = 15;
    Window.Bottom = 5;

    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                         TRUE,
                         &Window
                        );
    TerminateProcess(GetCurrentProcess(),0);
}


DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    STARTUPINFO	StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    BOOL Success;
    DWORD st;
    DWORD ProcessCount;
    SMALL_RECT Window;

    ProcessCount = 0;
    if ( strchr(GetCommandLine(),'+') ) {
        NewProcess();
        }

    GetStartupInfo(&StartupInfo);
    Window.Left = 0;
    Window.Top = 0;
    Window.Right = 15;
    Window.Bottom = 5;

    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                         TRUE,
                         &Window
                        );


    while ( TRUE ) {

        Success = CreateProcess(
                        NULL,
                        "constrs +",
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_NEW_CONSOLE,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo
                        );

        if (Success) {
            ProcessCount++;
            printf("Process %d Created\n",ProcessCount);
            st = WaitForSingleObject(ProcessInfo.hProcess,-1);
            (st == 0);
            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
            }
        }
}
