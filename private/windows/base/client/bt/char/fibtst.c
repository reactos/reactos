#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

LPVOID Fibers[4];

VOID
FiberRoutine(
    LPVOID lpParameter
    )
{
    DWORD FiberId;

    FiberId = (DWORD)GetFiberData();

    printf("Init: In Fiber %d, %x Param %d\n",FiberId,GetCurrentFiber(),lpParameter);

    while(1) {
        printf("In Fiber %d %d\n",FiberId,(DWORD)GetFiberData() );
        Sleep(10);
        if ( FiberId == 3 ) {
            SwitchToFiber(Fibers[0]);
            }
        else {
            SwitchToFiber(Fibers[FiberId+1]);
            }
        }
}

int _cdecl main(void)
{
    DWORD IdealProcessor;

    IdealProcessor = SetThreadIdealProcessor(GetCurrentThread(),MAXIMUM_PROCESSORS);
    printf("IdealProcessor %d\n",IdealProcessor);


    Fibers[0] = ConvertThreadToFiber((LPVOID)0);
    Fibers[1] = CreateFiber(0,FiberRoutine,(LPVOID)1);
    Fibers[2] = CreateFiber(0,FiberRoutine,(LPVOID)2);
    Fibers[3] = CreateFiber(0,FiberRoutine,(LPVOID)3);

    FiberRoutine((LPVOID)99);

    return 1;
}
