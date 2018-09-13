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

HANDLE Semaphore, Event;

VOID
TestThread(
    LPVOID ThreadParameter
    )
{
    DWORD st;

    (ReleaseSemaphore(Semaphore,1,NULL));

    st = WaitForSingleObject(Event,500);

    ExitThread(0);
}

VOID
NewProcess()
{

    DWORD st;
    DWORD ThreadId;
    HANDLE Thread;
    DWORD NumberOfThreads;
    DWORD MaximumThreadCount;
    DWORD i;

    //
    // Create an Event that is never signaled
    //

    Event = CreateEvent(NULL,TRUE,FALSE,NULL);

    //
    // Create a semaphore signaled by each thread as it starts
    //

    Semaphore = CreateSemaphore(NULL,0,256,NULL);

    (Event);
    (Semaphore);

    MaximumThreadCount = 15;
    NumberOfThreads = 0;

    //
    // Create the threads
    //

    printf("Creating %d threads... ",MaximumThreadCount);
    for ( i = 0;i<MaximumThreadCount;i++ ) {
        Thread = CreateThread(NULL,0L,(PVOID)TestThread,(LPVOID)99,0,&ThreadId);
        if ( Thread ) {
            NumberOfThreads++;
            CloseHandle(Thread);
            }
        }
    printf("%d threads Created\n",NumberOfThreads);

    for(i=0;i<NumberOfThreads;i++) {
        st = WaitForSingleObject((HANDLE)Semaphore,(DWORD)-1);
        (st == 0);
        }
    Sleep(3000);

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
                        "taskstrs +",
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
            st = WaitForSingleObject((HANDLE)ProcessInfo.hProcess,(DWORD)-1);
            (st == 0);
            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
            }
        }

    return TRUE;
}
