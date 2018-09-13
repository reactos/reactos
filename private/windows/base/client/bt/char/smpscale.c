#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

BOOL fShowScaling;

#define MAX_THREADS 32


//
// - hStartOfRace is a manual reset event that is signalled when
//   all of the threads are supposed to cut loose and begin working
//
// - hEndOfRace is a manual reset event that is signalled once the end time
//   has been retrieved and it is ok for the threads to exit
//

HANDLE hStartOfRace;
HANDLE hEndOfRace;


//
// - ThreadReadyDoneEvents are an array of autoclearing events. The threads
//   initially signal these events once they have reached their start routines
//   and are ready to being processing. Once they are done processing, they
//   signal thier event to indicate that they are done processing.
//
// - ThreadHandles are an array of thread handles to the worker threads. The
//   main thread waits on these to know when all of the threads have exited.
//

HANDLE ThreadReadyDoneEvents[MAX_THREADS];
HANDLE ThreadHandles[MAX_THREADS];

DWORD WorkerThread(PVOID ThreadIndex);
DWORD ThreadId;
DWORD StartTicks, EndTicks;
HANDLE IoHandle;

#define SIXTY_FOUR_K    (64*1024)
#define SIXTEEN_K       (16*1024)
unsigned int InitialBuffer[SIXTY_FOUR_K/sizeof(unsigned int)];
#define NUMBER_OF_WRITES ((1024*1024*8)/SIXTY_FOUR_K)
#define BUFFER_MAX  (64*1024)
#define FILE_SIZE ((1024*1024*8)-BUFFER_MAX)

/*
// Each thread has a THREAD_WORK structure. This contains the address
// of the cells that this thread is responsible for, and the number of
// cells it is supposed to process.
*/

typedef struct _THREAD_WORK {
    unsigned long *CellVector;
    int NumberOfCells;
    int RecalcResult;
    BOOL GlobalMode;
} THREAD_WORK, *PTHREAD_WORK;

unsigned int GlobalData[MAX_THREADS];
THREAD_WORK ThreadWork[MAX_THREADS];
#define ONE_MB      (1024*1024)

unsigned long Mb = 16;
unsigned long ExpectedRecalcValue;
unsigned long ActualRecalcValue;
unsigned long ContentionValue;
int fGlobalMode;
int WorkIndex;
int BufferSize;
unsigned long *CellVector;



DWORD
DoAnInteration(
    int NumberOfThreads,
    BOOL GlobalMode
    )
{
    int i;
    int fShowUsage;
    char c, *p, *whocares;
    int NumberOfDwords;
    int CNumberOfDwords;
    int DwordsPerThread;
    char *Answer;
    unsigned long x;

    fGlobalMode = 0;

    BufferSize = 1024;

    hStartOfRace = CreateEvent(NULL,TRUE,FALSE,NULL);
    hEndOfRace = CreateEvent(NULL,TRUE,FALSE,NULL);

    if ( !hStartOfRace || !hEndOfRace ) {
        fprintf(stderr,"SMPSCALE Race Event Creation Failed\n");
        ExitProcess(1);
        }


    /*
    // Prepare the ready done events. These are auto clearing events
    */

    for(i=0; i<NumberOfThreads; i++ ) {
        ThreadReadyDoneEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
        if ( !ThreadReadyDoneEvents[i] ) {
            fprintf(stderr,"SMPSCALE Ready Done Event Creation Failed %d\n",GetLastError());
            ExitProcess(1);
            }
        }

    NumberOfDwords = (Mb*ONE_MB) / sizeof(unsigned long);
    CNumberOfDwords = NumberOfDwords;
    DwordsPerThread = NumberOfDwords / NumberOfThreads;

    /*
    // Initialize the Cell Vector
    */

    for(i=0, ExpectedRecalcValue=0; i<NumberOfDwords; i++ ){
        ExpectedRecalcValue += i;
        CellVector[i] = i;
        }

    /*
    // Partition the work to the worker threads
    */

    for(i=0; i<NumberOfThreads; i++ ){
        ThreadWork[i].CellVector = &CellVector[i*DwordsPerThread];
        ThreadWork[i].NumberOfCells = DwordsPerThread;
        NumberOfDwords -= DwordsPerThread;

        /*
        // If we have a remainder, give the remaining work to the last thread
        */

        if ( NumberOfDwords < DwordsPerThread ) {
            ThreadWork[i].NumberOfCells += NumberOfDwords;
            }
        }

    /*
    // Create the worker threads
    */

    for(i=0; i<NumberOfThreads; i++ ) {
        ThreadWork[i].RecalcResult = 0;
        ThreadWork[i].GlobalMode = GlobalMode;
        GlobalData[i] = 0;

        ThreadHandles[i] = CreateThread(
                                NULL,
                                0,
                                WorkerThread,
                                (PVOID)i,
                                0,
                                &ThreadId
                                );
        if ( !ThreadHandles[i] ) {
            fprintf(stderr,"SMPSCALE Worker Thread Creation Failed %d\n",GetLastError());
            ExitProcess(1);
            }
        CloseHandle(ThreadHandles[i]);

        }

    /*
    // All of the worker threads will signal thier ready done event
    // when they are idle and ready to proceed. Once all events have been
    // set, then setting the hStartOfRaceEvent will begin the recalc
    */

    i = WaitForMultipleObjects(
            NumberOfThreads,
            ThreadReadyDoneEvents,
            TRUE,
            INFINITE
            );

    if ( i == WAIT_FAILED ) {
        fprintf(stderr,"SMPSCALE Wait for threads to stabalize Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    /*
    // Everthing is set to begin the recalc operation
    */

    StartTicks = GetTickCount();
    if ( !SetEvent(hStartOfRace) ) {
        fprintf(stderr,"SMPSCALE SetEvent(hStartOfRace) Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    /*
    // Now just wait for the recalc to complete
    */

    i = WaitForMultipleObjects(
            NumberOfThreads,
            ThreadReadyDoneEvents,
            TRUE,
            INFINITE
            );

    if ( i == WAIT_FAILED ) {
        fprintf(stderr,"SMPSCALE Wait for threads to complete Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    /*
    // Now pick up the individual recalc values
    */

    for(i=0, ActualRecalcValue = 0; i<NumberOfThreads; i++ ){
        ActualRecalcValue += ThreadWork[i].RecalcResult;
        }

    EndTicks = GetTickCount();

    if ( ActualRecalcValue != ExpectedRecalcValue ) {
        fprintf(stderr,"SMPSCALE Recalc Failuer !\n");
        ExitProcess(1);
        }

    return (EndTicks - StartTicks);
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DWORD Time,GlobalModeTime;
    DWORD BaseLine;
    DWORD i;
    SYSTEM_INFO SystemInfo;

    /*
    // Allocate and initialize the CellVector
    */

    if ( argc > 1 ) {
        fShowScaling = TRUE;
        }
    else {
        fShowScaling = FALSE;
        }

    CellVector = (PDWORD)VirtualAlloc(NULL,Mb*ONE_MB,MEM_COMMIT,PAGE_READWRITE);
    if ( !CellVector ) {
        fprintf(stderr,"SMPSCALE Cell Vector Allocation Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    BaseLine = DoAnInteration(1,FALSE);
    i = 0;
    while(i++<10) {
        Time = DoAnInteration(1,FALSE);
        if ( Time == BaseLine ) {
            break;
            }
        if ( abs(Time-BaseLine) < 2 ) {
            break;
            }
        BaseLine = Time;
        }

    GetSystemInfo(&SystemInfo);

    fprintf(stdout,"%d Processor System. Single Processor BaseLine %dms\n\n",
        SystemInfo.dwNumberOfProcessors,
        BaseLine
        );

    if ( !fShowScaling ) {
            fprintf(stdout,"              Time             Time with Cache Sloshing\n");
            }

    for ( i=0;i<SystemInfo.dwNumberOfProcessors;i++) {
        Time = DoAnInteration(i+1,FALSE);
        GlobalModeTime = DoAnInteration(i+1,TRUE);

        if ( fShowScaling ) {
            if ( i > 0 ) {
                fprintf(stdout,"%1d Processors %4dms (%3d%%)   %4dms (%3d%%) (with cache contention)\n",
                    i+1,Time,((BaseLine*100)/Time-100),GlobalModeTime,((BaseLine*100)/GlobalModeTime-100)
                    );
                }
            else {
                fprintf(stdout,"%1d Processors %4dms          %4dms        (with cache contention)\n",
                    i+1,Time,GlobalModeTime
                    );
                }
            }
        else {
            fprintf(stdout,"%1d Processors %4dms        vs          %4dms\n",
                i+1,Time,GlobalModeTime
                );
            }
        }

    ExitProcess(2);
}


/*
// The worker threads perform the recalc operation on their
// assigned cells. They begin by setting their ready done event
// to indicate that they are ready to begin the recalc. Then they
// wait until the hStartOfRace event is signaled. Once this occurs, they
// do their part of the recalc and when done they signal their ready done
// event and then wait on the hEndOfRaceEvent
*/

DWORD
WorkerThread(
    PVOID ThreadIndex
    )
{

    unsigned long Me;
    unsigned long *MyCellVectorBase;
    unsigned long *CurrentCellVector;
    unsigned long MyRecalcValue;
    unsigned long MyNumberOfCells;
    unsigned long i,j;
    int GlobalMode;
    HANDLE hEvent;
    BOOL b;

    Me = (unsigned long)ThreadIndex;
    MyRecalcValue = 0;
    MyCellVectorBase = ThreadWork[Me].CellVector;
    MyNumberOfCells = ThreadWork[Me].NumberOfCells;
    GlobalMode = ThreadWork[Me].GlobalMode;

    /*
    // Signal that I am ready to go
    */

    if ( !SetEvent(ThreadReadyDoneEvents[Me]) ) {
        fprintf(stderr,"SMPSCALE (1) SetEvent(ThreadReadyDoneEvent[%d]) Failed %d\n",Me,GetLastError());
        ExitProcess(1);
        }

    /*
    // Wait for the master to release us to do the recalc
    */

    i = WaitForSingleObject(hStartOfRace,INFINITE);
    if ( i == WAIT_FAILED ) {
        fprintf(stderr,"SMPSCALE Thread %d Wait for start of recalc Failed %d\n",Me,GetLastError());
        ExitProcess(1);
        }

    /*
    // perform the recalc operation
    */

    for (i=0, CurrentCellVector = MyCellVectorBase,j=0; i<MyNumberOfCells; i++ ) {
        if (GlobalMode){
            GlobalData[Me] += *CurrentCellVector++;
            }
        else {
            MyRecalcValue += *CurrentCellVector++;
            }
        }
    if (GlobalMode){
        MyRecalcValue = GlobalData[Me];
        }
    ThreadWork[Me].RecalcResult = MyRecalcValue;

    /*
    // Signal that I am done and then wait for further instructions
    */

    if ( !SetEvent(ThreadReadyDoneEvents[Me]) ) {
        fprintf(stderr,"SMPSCALE (2) SetEvent(ThreadReadyDoneEvent[%d]) Failed %d\n",Me,GetLastError());
        ExitProcess(1);
        }

    i = WaitForSingleObject(hEndOfRace,INFINITE);
    if ( i == WAIT_FAILED ) {
        fprintf(stderr,"SMPSCALE Thread %d Wait for end of recalc Failed %d\n",Me,GetLastError());
        ExitProcess(1);
        }

    return MyRecalcValue;
}
