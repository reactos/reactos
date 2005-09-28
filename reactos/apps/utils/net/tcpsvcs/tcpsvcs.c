#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

LPTHREAD_START_ROUTINE
ServiceHandler[NUM_SERVICES] = {
    EchoHandler,
    ChargenHandler,
    DaytimeHandler,
    NULL,
    QotdHandler
};

INT ServicePort[NUM_SERVICES] = {
    ECHO_PORT,
    CHARGEN_PORT,
    DAYTIME_PORT,
    DISCARD_PORT,
    QOTD_PORT
};

int main(int argc, char *argv[])
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
