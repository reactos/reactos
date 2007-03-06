/*
 * Author: Skywing (skywing@valhallalegends.com)
 * Date: 09/09/2003
 * Purpose: Probe for PsUnblockThread crash due to double-acquire spin lock.
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

DWORD __stdcall threadfunc(void* UNREFERENCED)
{
	printf("Thread: Initialized\n");
	Sleep(2500);
	printf("Thread: Terminating...\n");
	return 0;
}

int main(int ac, char **av)
{
	DWORD id;
	HANDLE Thread;

	Thread = CreateThread(0, 0, threadfunc, 0, 0, &id);
	printf("Main: ThreadId for new thread is %08lx\n", id);
	printf("Main: Waiting on thread...\n");
	WaitForSingleObject(Thread, INFINITE);
	printf("Main: OK, somebody fixed the PsUnblockThread spinlock double-acquire crash\n");
	NtClose(Thread);
	printf("Main: Terminating...\n");
	return 0;
}
