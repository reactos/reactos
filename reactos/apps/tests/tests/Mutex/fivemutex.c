/* fivemutex.c: hungry philosophers problem
 *
 * (c) Copyright D.W.Howells 2000.
 *     All rights reserved
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR(X,Y) do { if (X) { perror(""Y""); return 1; } } while(0)
#define RUNLENGTH 4

int count[5];

const char *names[] = {
	"five/1", "five/2", "five/3", "five/4", "five/5"
};

DWORD WINAPI child(LPVOID tparam)
{
	HANDLE left, right, first, second;
	const char *lname, *rname;
	int pid = (int) tparam;
	int wt;

	lname = names[pid%5];
	rname = names[(pid+1)%5];

	/* create a mutex */
	left = CreateMutex(NULL,0,lname);	ERR(!left,"create left");
	right = CreateMutex(NULL,0,rname);	ERR(!left,"create right");

	printf("[%d] left: %p [%s]\n",pid,left,lname);
	printf("[%d] right: %p [%s]\n",pid,right,rname);

	/* pick the forks up in numerical order, else risk starvation */
	if (pid%5 < (pid+1)%5) {
		first = left;
		second = right;
	}
	else {
		first = right;
		second = left;
	}

	for (;;) {
		/* grab the left mutex */
		wt = WaitForMultipleObjects(1,&first,0,INFINITE);
		if (wt!=WAIT_OBJECT_0)
			goto wait_failed;

		/* grab the right mutex */
		wt = WaitForMultipleObjects(1,&second,0,INFINITE);
		if (wt!=WAIT_OBJECT_0)
			goto wait_failed;

		/* got it */
		count[pid]++;

		/* pass the mutexes */
		ERR(!ReleaseMutex(left),"release left");
		ERR(!ReleaseMutex(right),"release right");
		continue;

	    wait_failed:
		switch (wt) {
		case WAIT_OBJECT_0+1:
			printf("[%d] obtained mutex __1__\n",pid);
			exit(1);
		case WAIT_ABANDONED_0:
		case WAIT_ABANDONED_0+1:
			printf("[%d] abandoned wait\n",pid);
			continue;
		case WAIT_TIMEOUT:
			printf("[%d] wait timed out\n",pid);
			exit(1);
		default:
			ERR(1,"WaitForMultipleObjects");
		}

		return 1;
	}

	/* close the handles */
	ERR(!CloseHandle(left),"close left");
	ERR(!CloseHandle(right),"close right");

	return 0;

}

int main()
{
	HANDLE hThread[5];
	DWORD tid;
	int loop;

	for (loop=0; loop<5; loop++) {

		hThread[loop] = CreateThread(NULL,					/* thread attributes */
									 0,					/* stack size */
									 child,				/* start address */
									 (void*)loop,			/* parameter */
									 0,					/* creation flags */
									 &tid					/* thread ID */
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
		printf("[%d] ate %d times (%d times per second)\n",
		       loop,count[loop],count[loop]/RUNLENGTH
			   );

	return 0;
}
