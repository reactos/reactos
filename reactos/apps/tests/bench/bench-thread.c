#include <stdio.h>
#include <windows.h>

#define NR_THREADS (0x5)

DWORD WINAPI
thread_main1(LPVOID param)
{
   printf("Thread 1 running (Counter %lu)\n", (DWORD)param);

   return 0;
}

DWORD WINAPI
thread_main2(LPVOID param)
{
   printf("Thread 2 running (Counter %lu)\n", (DWORD)param);

   return 0;
}

int main (void)
{
   DWORD i=0;
   DWORD id;
   
   printf("Creating %d threads...\n",NR_THREADS);
//   for (i=0;i<NR_THREADS;i++)
//     {
	CreateThread(NULL,
		     0,
                     thread_main1,
                     (LPVOID)i,
		     0,
		     &id);
#if 0
	CreateThread(NULL,
		     0,
                     thread_main2,
                     (LPVOID)i,
		     0,
		     &id);
     }
#endif
   printf("Threads created...\n");

//     Sleep (5000);
   SuspendThread (GetCurrentThread());

   return 0;
}
