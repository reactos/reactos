#include <stdio.h>
#include <windows.h>

#define NR_THREADS (10)

ULONG nr;

DWORD WINAPI thread_main1(LPVOID param)
{
   ULONG s;
   
   printf("Thread %d running\n", (DWORD)param);
   s = nr = ((nr * 1103515245) + 12345) & 0x7fffffff;
   s = s % 10;
   printf("s %d\n", s);
   Sleep(s);
   return 0;
}

int main (int argc, char* argv[])
{
   HANDLE hThread;
   DWORD i=0;
   DWORD id;
   ULONG nr;
   
   nr = atoi(argv[1]);
   printf("Seed %d\n", nr);
   
   printf("Creating %d threads...\n",NR_THREADS*2);
   for (i=0;i<NR_THREADS;i++)
     {
	CreateThread(NULL,
		     0,
		     thread_main1,
		     (LPVOID)i,
		     0,
		     &id);

     }

   printf("All threads created...\n");
   for(;;);
   return 0;
}
