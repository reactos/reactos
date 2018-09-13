#include "precomp.h"
#include "newctx.h"
#include <stdlib.h>

LPCONTEXT_TABLE     gContextTable = NULL;
LPWSHANDLE_CONTEXT  gHandleContexts = NULL;
UINT                gNumHandles = 0;
volatile LONG       gNumThreads = 0;
UINT                gWRratio = 0;
LONG                gInserts = 0;
LONG                gInsertTime = 0;
LONG                gRemoves = 0;
LONG                gRemoveTime = 0;
LONG                gLookups = 0;
LONG                gLookupTime = 0;
volatile BOOL       gRun = FALSE;
DWORD				gNumProcessors;
LONG                gInteropSpins;

DWORD
StressThread (
    PVOID   param
    );


#define USAGE(x)    \
    "Usage:\n"\
    "   %s <num_threads> <num_handles> <write_access_%%> <time_to_run> <spins\n"\
    "   where\n"\
    "       <num_threads>   - number of threads (1-64);\n"\
    "       <num_handles>   - number of handles;\n"\
    "       <write_access_%%>  - percentage of write accesses (0-100);\n"\
    "       <time_to_run>   - time to run in sec;\n"\
    "       <spins>          - number of spins between operations.\n"\
    ,x


int _cdecl
main (
    int argc,
    CHAR **argv
    )
{
    LONG    TimeToRun, RunTime;
    HANDLE  Threads[MAXIMUM_WAIT_OBJECTS];
    DWORD   id, rc;
    UINT    err, i;
    UINT    numThreads;
    ULONG   rnd = GetTickCount ();
    LPWSHANDLE_CONTEXT  ctx;
	SYSTEM_INFO	info;

    if (argc<6) {
        printf (USAGE(argv[0]));
        return 1;
    }

	GetSystemInfo (&info);
	gNumProcessors = info.dwNumberOfProcessors;

    gNumThreads = atoi(argv[1]);
    if ((gNumThreads==0)||(gNumThreads>MAXIMUM_WAIT_OBJECTS)) {
        printf (USAGE(argv[0]));
        return 1;
    }

    gNumHandles = atoi(argv[2]);
    if (gNumHandles==0) {
        printf (USAGE(argv[0]));
        return 1;
    }


    gWRratio = atoi(argv[3]);
    if (gWRratio>100) {
        printf (USAGE(argv[0]));
        return 1;
    }

    TimeToRun = atoi(argv[4]);
    if (TimeToRun==0) {
        printf (USAGE(argv[0]));
        return 1;
    }

    gInteropSpins = atoi(argv[5]);


    err = WahCreateHandleContextTable (&gContextTable);
    if (err!=NO_ERROR) {
        printf ("Failed to create context table, err: %ld.\n", err);
        return 1;
    }

    gHandleContexts = (LPWSHANDLE_CONTEXT)
                            GlobalAlloc (
                                    GPTR,
                                    sizeof (WSHANDLE_CONTEXT)*gNumHandles
                                    );
    if (gHandleContexts==NULL) {
        printf ("Failed to allocate table of handle contexts, gle: %ld.\n",
                            GetLastError ());
        return 1;
    }

    for (i=0, ctx=gHandleContexts; i<gNumHandles; ) {

        ctx->RefCount = 1;
        do {
            ctx->Handle = (HANDLE)((((ULONGLONG)RtlRandom (&rnd)*(ULONGLONG)gNumHandles*(ULONGLONG)20)
                                        /((ULONGLONG)MAXLONG))
                                    &(~((ULONGLONG)3)));
        }
        while (WahReferenceContextByHandle (gContextTable, ctx->Handle)!=NULL);

        if (WahInsertHandleContext (gContextTable, ctx)!=ctx) {
            printf ("Failed to insert handle %lx into the table.\n", ctx->Handle);
            return 1;
        }
        i++; ctx++;
    }

	if (!SetPriorityClass (GetCurrentProcess (), HIGH_PRIORITY_CLASS)) {
		printf ("Failed to set high priority class for the process, err: %ld.\n",
						GetLastError ());
		return 1;
	}

    numThreads = gNumThreads;
    for (i=0; i<numThreads; i++) {
        Threads[i] = CreateThread (NULL,
                                0,
                                StressThread,
                                (PVOID)i,
                                0,
                                &id
                                );
        if (Threads[i]==NULL) {
            printf ("Failed to create thread %ld, gle: %ld.\n",
                        i, GetLastError ());
            return 1;
        }
    }

    while (gNumThreads>0) {
        Sleep (0);
    }
    printf ("Starting...\n");

    RunTime = GetTickCount ();
    gRun = TRUE;
    Sleep (TimeToRun*1000);
    RunTime = GetTickCount()-RunTime;
    gRun = FALSE;
    
    printf ("Done, waiting for %ld threads to terminate...\n", gNumThreads);

    rc = WaitForMultipleObjects (numThreads, Threads, TRUE, INFINITE);


    printf ("Number of inserts    : %8.8lu.\n", gInserts);
    printf ("Inserts per ms       : %8.8lu.\n", gInserts/gInsertTime);
    printf ("Number of removes    : %8.8lu.\n", gRemoves);
    printf ("Removes per ms       : %8.8lu.\n", gRemoves/gRemoveTime);
    printf ("Number of lookups    : %8.8lu.\n", gLookups);
    printf ("Lookups per ms       : %8.8lu.\n", gLookups/gLookupTime);
    printf ("Running time         : %8.8lu.\n", RunTime);
#ifdef _PERF_DEBUG_
    {
        LONG    ContentionCount = 0;
#ifdef _RW_LOCK_
        LONG    ReaderSpins = 0;
        LONG    WriterWaits = 0;
        LONG    FailedSpins = 0;
#endif

        for ( i = 0; i <= gContextTable->HandleToIndexMask; i++ ) {

#ifdef _RW_LOCK_
            ReaderSpins += gContextTable->Tables[i].ReaderSpins;
            WriterWaits += gContextTable->Tables[i].WriterWaits;
            FailedSpins += gContextTable->Tables[i].FailedSpins;
#endif
            if (gContextTable->Tables[i].WriterLock.DebugInfo!=NULL)
                ContentionCount += gContextTable->Tables[i].WriterLock.DebugInfo->ContentionCount;
        }

#ifdef _RW_LOCK_
        printf ("Reader spins: %ld\n", ReaderSpins);
        printf ("Writer waits: %ld\n", WriterWaits);
        printf ("Failed spins: %ld\n", FailedSpins);
#endif
        printf ("Contention count: %ld\n", ContentionCount);
    }
#endif
	WahDestroyHandleContextTable (gContextTable);
    return 0;
}


DWORD
StressThread (
    PVOID   param
    )
{
    ULONG               idx;
    ULONG               rnd = GetTickCount ();
    LPWSHANDLE_CONTEXT  ctx;
    ULONG               RunTime;
	ULONGLONG			cTime, eTime, kTime1, kTime2, uTime1, uTime2;
	LONG                lInserts = 0, lInsertTime = 0;
	LONG                lRemoves = 0, lRemoveTime = 0;
	LONG                lLookups = 0, lLookupTime = 0;
    LONG                Spin;

	if (!SetThreadAffinityMask (GetCurrentThread (),
						1<<(((DWORD)param)%gNumProcessors))) {
		printf ("Failed to set thread's %ld affinity mask, err: %ld.\n",
					GetLastError ());
		ExitProcess (1);
	}

    InterlockedDecrement ((PLONG)&gNumThreads);
    // printf ("Thread %ld ready.\n", param);
    while (!gRun)
        ;

	InterlockedIncrement ((PLONG)&gNumThreads);
    RunTime = GetTickCount ();
	GetThreadTimes (GetCurrentThread (), (LPFILETIME)&cTime,
											(LPFILETIME)&eTime,
											(LPFILETIME)&kTime1,
											(LPFILETIME)&uTime1);

    while (gRun) {
        idx = (ULONG)(((ULONGLONG)RtlRandom (&rnd)*(ULONGLONG)gNumHandles)/(ULONGLONG)MAXLONG);
        ctx = &gHandleContexts[idx];
        if ( (ULONG)(((ULONGLONG)rnd*(ULONGLONG)100)/(ULONGLONG)MAXLONG) < gWRratio) {
            lRemoveTime -= NtGetTickCount ();
            if (WahRemoveHandleContext (gContextTable, ctx)==NO_ERROR) {
                lRemoveTime += NtGetTickCount ();

                Spin = gInteropSpins;
                while (Spin--);

                ctx->RefCount = 1;
                lInsertTime -= NtGetTickCount ();
                WahInsertHandleContext (gContextTable, ctx);
                lInsertTime += NtGetTickCount ();
                lInserts += 1;
            }
            else {
                lRemoveTime += NtGetTickCount ();
            }
            lRemoves += 1;

        }
        else {
            lLookupTime -= NtGetTickCount ();
            WahReferenceContextByHandle (gContextTable, ctx->Handle);
            lLookupTime += NtGetTickCount ();
            lLookups += 1;
        }

        Spin = gInteropSpins;
        while (Spin--);
    }
	GetThreadTimes (GetCurrentThread (), (LPFILETIME)&cTime,
											(LPFILETIME)&eTime,
											(LPFILETIME)&kTime2,
											(LPFILETIME)&uTime2);
    //printf ("Thread %d ran for %lu ms, kernel mode: %lu, user mode: %lu.\n",
    //				param,
	//			GetTickCount ()-RunTime,
	//			(ULONG)((kTime2-kTime1)/(ULONGLONG)10000),
	//			(ULONG)((uTime2-uTime1)/(ULONGLONG)10000)
	//			);

	InterlockedExchangeAdd (&gInserts, lInserts);
	InterlockedExchangeAdd (&gInsertTime, lInsertTime);
	InterlockedExchangeAdd (&gRemoves, lRemoves);
	InterlockedExchangeAdd (&gRemoveTime, lRemoveTime);
	InterlockedExchangeAdd (&gLookups, lLookups);
	InterlockedExchangeAdd (&gLookupTime, lLookupTime);

    return 0;
}

