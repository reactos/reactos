#include <stdio.h>
#include <string.h>
#include <windows.h>

#define SIZE (65*1024*1024)

ULONG x[SIZE / 4096];

int main()
{  
   int i;
   PUCHAR BaseAddress;
   
   BaseAddress = VirtualAlloc(NULL,
			      SIZE,
			      MEM_COMMIT,
			      PAGE_READONLY);
   if (BaseAddress == NULL)
     {
	printf("Failed to allocate virtual memory");
	return(1);
     }
   printf("BaseAddress %p\n", BaseAddress);
   for (i = 0; i < (SIZE / 4096); i++)
     {
	printf("%.8x  ", i*4096);
	x[i] = BaseAddress[i*4096];
     }
   
   return(0);
}
