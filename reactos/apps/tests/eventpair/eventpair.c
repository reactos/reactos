/*
 * Author: Skywing (skywing@valhallalegends.com)
 * Date: 09/09/2003
 * Purpose: Test Thread-EventPair functionality.
 */

#include <windows.h>
#include <stdio.h>
#include <ddk/ntddk.h>

#ifndef NTAPI
#define NTAPI WINAPI
#endif

HANDLE MakeEventPair()
{
	NTSTATUS Status;
	HANDLE EventPair;
	OBJECT_ATTRIBUTES Attributes;

	InitializeObjectAttributes(&Attributes, NULL, 0, NULL, NULL);
	Status = NtCreateEventPair(&EventPair, STANDARD_RIGHTS_ALL, &Attributes);
	printf("Status %08lx creating eventpair\n", Status);
	return EventPair;
}

DWORD __stdcall threadfunc(void* eventpair)
{
	printf("Thread: Set eventpair status %08lx\n", NtSetInformationThread(NtCurrentThread(), ThreadEventPair, &eventpair, sizeof(HANDLE)));
	Sleep(2500);

	printf("Thread: Setting low and waiting high...\n");
	printf("Thread: status = %08lx\n", NtSetLowWaitHighThread());
	printf("Thread: status = %08lx\n", NtSetHighWaitLowThread());
	printf("Thread: Terminating...\n");
	return 0;
}

int main(int ac, char **av)
{
	DWORD id;
	HANDLE EventPair, Thread;

	printf("Main: NtSetLowWaitHighThread is at %08lx\n", NtSetLowWaitHighThread());

	EventPair = MakeEventPair();

	if(!EventPair) {
		printf("Main: Could not create event pair.\n");
		return 0;
	}

	printf("Main: EventPair = %08lx\n", (DWORD)EventPair);
	Thread = CreateThread(0, 0, threadfunc, EventPair, 0, &id);
	printf("Main: ThreadId for new thread is %08lx\n", id);
	printf("Main: Setting high and waiting low\n");
	printf("Main: status = %08lx\n", NtSetHighWaitLowEventPair(EventPair));
	Sleep(2500);
	printf("Main: status = %08lx\n", NtSetLowWaitHighEventPair(EventPair));
	NtClose(EventPair);
	/* WaitForSingleObject(Thread, INFINITE); FIXME: Waiting on thread handle causes double spinlock acquisition (and subsequent crash) in PsUnblockThread -  ntoskrnl/ps/thread.c */
	NtClose(Thread);
	printf("Main: Terminating...\n");
	return 0;
}
