/* rapidmutex.c: rapid mutex passing test client [Win32 threaded]
 *
 * (c) Copyright D.W.Howells 2000.
 *     All rights reserved
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR(X,Y) do { if (X) { perror(Y); return 1; } } while(0)
#define RUNLENGTH 4

int count[5];

DWORD WINAPI child(LPVOID tparam)
{
	HANDLE mymutex;
	int pid = (int) tparam;

	/* create a mutex */
	mymutex = CreateMutex(NULL,0,"wibble");
	ERR(!mymutex,"create mutex");
	printf("[%d] Handle: %p\n",pid,mymutex);

	for (;;) {
		/* grab the mutex */
		switch (WaitForMultipleObjects(1,&mymutex,0,INFINITE))
		{
		    case WAIT_OBJECT_0:
			/* got it */
			count[pid]++;

			/* pass the mutex */
			ERR(!ReleaseMutex(mymutex),"release mutex");
			break;

		    case WAIT_OBJECT_0+1:
			printf("[%d] obtained mutex __1__\n",pid);
			exit(0);
		    case WAIT_ABANDONED_0:
		    case WAIT_ABANDONED_0+1:
			printf("[%d] abandoned wait\n",pid);
			exit(0);
		    case WAIT_TIMEOUT:
			printf("[%d] wait timed out\n",pid);
			exit(0);
		    default:
			ERR(1,"WaitForMultipleObjects");
		}
	}

	return 0;
}

int main()
{
	HANDLE hThread[5];
	DWORD tid;
	int loop;

	for (loop=0; loop<5; loop++) {

		hThread[loop] =
			CreateThread(NULL,		/* thread attributes */
				     0,			/* stack size */
				     child,		/* start address */
				     (void*)loop,	/* parameter */
				     0,			/* creation flags */
				     &tid		/* thread ID */
				     );
		if (!hThread[loop])
		{
			ERR(1,"CreateThread");
		}
	}

	WaitForMultipleObjects(5,hThread,0,RUNLENGTH*1000);

	for (loop=0; loop<5; loop++)
		TerminateThread(hThread[loop],0);

	for (loop=0; loop<5; loop++)
		printf("[%d] obtained the mutex %d times"
		       " (%d times per second)\n",
		       loop,count[loop],count[loop]/RUNLENGTH
		       );

	return 0;
}
