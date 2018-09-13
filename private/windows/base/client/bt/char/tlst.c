#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


_declspec(thread)   DWORD Id;


VOID
TestThread(
    LPVOID ThreadParameter
    )
{
    DWORD st;

    Id = GetCurrentThreadId();

    printf("    TEST THREAD Id = %d vs %d\n",Id,GetCurrentThreadId());

    Sleep(1000);

    printf("    TEST THREAD Id = %d vs %d\n",Id,GetCurrentThreadId());

    ExitThread(0);
}













int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    HANDLE Thread;
    DWORD ThreadId;
    int i;

    Id = GetCurrentThreadId();

    printf("MAIN THREAD Id = %d vs %d\n",Id,GetCurrentThreadId());

    for(i=0;i<10;i++) {
        Thread = CreateThread(NULL,0L,(PVOID)TestThread,(LPVOID)99,0,&ThreadId);

        if ( !Thread ) {
            printf("Thread creation failed %d\n",GetLastError());
            }
        WaitForSingleObject(Thread,INFINITE);
        CloseHandle(Thread);

        printf("\nMAIN THREAD Id = %d vs %d\n",Id,GetCurrentThreadId());
        }

    return 1;
}
