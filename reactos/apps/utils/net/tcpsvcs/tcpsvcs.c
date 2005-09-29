#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

static
LPTHREAD_START_ROUTINE
ServiceHandler[NUM_SERVICES] =
{
    EchoHandler,
    DiscardHandler,
    DaytimeHandler,
    QotdHandler,
    ChargenHandler
};

static int
ServicePort[NUM_SERVICES] =
{
    ECHO_PORT,
    DISCARD_PORT,
    DAYTIME_PORT,
    QOTD_PORT,
    CHARGEN_PORT
};

#if 0
static
SERVICE_TABLE_ENTRY
ServiceTable[2] =
{
    {TEXT("tcpsvcs"), ServiceMain},
    {NULL, NULL}
};
#endif

int main(void)
{
    PSERVICES pServices[NUM_SERVICES];
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    INT i;

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<NUM_SERVICES; i++ )
    {
        /* Allocate memory for thread data. */
        pServices[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SERVICES));

        if( pServices[i] == NULL )
            ExitProcess(2);

        /* Generate unique data for each thread. */
        pServices[i]->Service = ServiceHandler[i];
        pServices[i]->Port = ServicePort[i];

        hThread[i] = CreateThread(
            NULL,              // default security attributes
            0,                 // use default stack size
            StartServer,       // thread function
            pServices[i],      // argument to thread function
            0,                 // use default creation flags
            &dwThreadId[i]);   // returns the thread identifier

        /* Check the return value for success. */
        if (hThread[i] == NULL)
        {
            ExitProcess(i);
        }
    }

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }
    return 0;
}

#if 0
static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{
    PSERVICES pServices[NUM_SERVICES];
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    INT i;

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<NUM_SERVICES; i++ )
    {
        /* Allocate memory for thread data. */
        pServices[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SERVICES));

        if( pServices[i] == NULL )
            ExitProcess(2);

        /* Generate unique data for each thread. */
        pServices[i]->Service = ServiceHandler[i];
        pServices[i]->Port = ServicePort[i];

        hThread[i] = CreateThread(
            NULL,              // default security attributes
            0,                 // use default stack size
            StartServer,       // thread function
            pServices[i],      // argument to thread function
            0,                 // use default creation flags
            &dwThreadId[i]);   // returns the thread identifier

        /* Check the return value for success. */
        if (hThread[i] == NULL)
        {
            ExitProcess(i);
        }
    }

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }

}

int
main(int argc, char *argv[])
{
    DPRINT("tcpsvcs: main() started\n");

    StartServiceCtrlDispatcher(ServiceTable);

    DPRINT("Umpnpmgr: main() done\n");

    ExitThread(0);

    return 0;
}
#endif
