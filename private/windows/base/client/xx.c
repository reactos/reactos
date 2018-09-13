#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

VOID
TestThread(
    LPVOID ThreadParameter
    )
{
    OutputDebugString("Thread Started\n");
    while(1);
}

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE Thread;
    DWORD ThreadId;
    DWORD st;

    Thread = CreateThread(NULL,0L,TestThread,(LPVOID)99,0,&ThreadId);
    assert(Thread);

    Sleep(3000);
    OutputDebugString("Out of Sleep\n");

    TerminateThread(Thread,1);

    st = WaitForSingleObject(Thread,-1);
    assert(st == 0);

    assert(GetExitCodeThread(Thread,&st));
    assert(st = 1);
}
