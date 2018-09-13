#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

HANDLE ThreadHandles[32];

DWORD
WorkerThread(
    PVOID ThreadIndex
    )
{

    DWORD Processor;
    BOOL DoSleep;

    Processor = (DWORD)ThreadIndex;
    if ( Processor > 100 ) {
        DoSleep = TRUE;
        Processor -= 100;
        }
    else {
        DoSleep = FALSE;
        }
    SetThreadAffinityMask(GetCurrentThread(),1 << Processor);
    for(;;){
        if ( Processor & 1 ) {
            SetThreadPriority(ThreadHandles[Processor-1],THREAD_PRIORITY_ABOVE_NORMAL);
            SetThreadPriority(ThreadHandles[Processor-1],THREAD_PRIORITY_NORMAL);
            }
        else {
            if ( DoSleep ) {
                Sleep(0);
                }
            }
        }
    return 0;
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DWORD i;
    SYSTEM_INFO SystemInfo;
    DWORD ThreadId;
    HANDLE hThread;

    GetSystemInfo(&SystemInfo);
    if ( SystemInfo.dwNumberOfProcessors >= 3 ) {

        for ( i=0;i<SystemInfo.dwNumberOfProcessors;i++) {

            ThreadHandles[i] = CreateThread(
                                NULL,
                                0,
                                WorkerThread,
                                (PVOID)(i),
                                0,
                                &ThreadId
                                );

            if ( !ThreadHandles[i] ) {
                fprintf(stdout,"CreateThread failed %d\n",GetLastError());
                ExitProcess(1);
                }
            }

        for ( i=0;i<SystemInfo.dwNumberOfProcessors;i++) {
            if ( (i & 1) == 0 ) {

                hThread = CreateThread(
                            NULL,
                            0,
                            WorkerThread,
                            (PVOID)(100+i),
                            0,
                            &ThreadId
                            );

                if ( !ThreadHandles[i] ) {
                    fprintf(stdout,"CreateThread failed %d\n",GetLastError());
                    ExitProcess(1);
                    }
                CloseHandle(hThread);
                }
            }
        Sleep(60000);
        }
    ExitProcess(1);
}
