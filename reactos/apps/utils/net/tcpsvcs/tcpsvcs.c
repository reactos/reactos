#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"
#include "skelserver/skelserver.h"
#include "echo/echo.h"
#include "chargen/chargen.h"

int main(int argc, char *argv[])
{
    PMYDATA pData[MAX_THREADS];
    DWORD dwThreadId[MAX_THREADS];
    HANDLE hThread[MAX_THREADS];
    INT i;

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<MAX_THREADS; i++ )
    {
        /* Allocate memory for thread data. */
        pData[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

        if( pData == NULL )
            ExitProcess(2);

        /* Generate unique data for each thread. */
        pData[0]->Port = ECHO_PORT;
        pData[0]->Service = EchoHandler;
        pData[1]->Port = CHARGEN_PORT;
        pData[1]->Service = ChargenHandler;

        hThread[i] = CreateThread(
            NULL,              // default security attributes
            0,                 // use default stack size
            StartServer,       // thread function
            pData[i],          // argument to thread function
            0,                 // use default creation flags
            &dwThreadId[i]);   // returns the thread identifier

        /* Check the return value for success. */
        if (hThread[i] == NULL)
        {
            ExitProcess(i);
        }
    }

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(MAX_THREADS, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<MAX_THREADS; i++)
    {
        CloseHandle(hThread[i]);
    }

    return 0;
}

