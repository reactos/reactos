#include <stdio.h>
#include <windows.h>

#define NR_THREADS (0x1000)

ULONG thread_main(PVOID param)
{
}

int main()
{
   unsigned int i=0;
   DWORD id;
   
   printf("Creating %d threads\n",NR_THREADS);
   for (i=0;i<NR_THREADS;i++)
     {
	CreateThread(NULL,
		     0,
		     thread_main,
		     NULL,
		     0,
		     &id);
     }
}
