#include <stdio.h>
#include <windows.h>

int main()
{
   PVOID Base;
   PVOID Ret;
   
   Base = VirtualAlloc(NULL,
		       1048576,
		       MEM_RESERVE,
		       PAGE_READWRITE);
   if (Base == NULL)
     {
	printf("VirtualAlloc failed 1\n");
     }
   
   Ret = VirtualAlloc(Base + 4096,
		      4096,
		      MEM_COMMIT,
		      PAGE_READWRITE);
   if (Ret == NULL)
     {
	printf("VirtualAlloc failed 2\n");
     }
   
   Ret = VirtualAlloc(Base + 12288,
		      4096,
		      MEM_COMMIT,
		      PAGE_READWRITE);
   if (Ret == NULL)
     {
	printf("VirtualAlloc failed 3\n");
     }
   
   Ret = VirtualAlloc(Base + 20480,
		      4096,
		      MEM_COMMIT,
		      PAGE_READWRITE);
   if (Ret == NULL)
     {
	printf("VirtualAlloc failed 4\n");
     }
   
   Ret = VirtualAlloc(Base + 4096,
		      28672,
		      MEM_RESERVE,
		      PAGE_READWRITE);
   if (Ret == NULL)
     {
	printf("VirtualAlloc failed 5\n");
     }
}
