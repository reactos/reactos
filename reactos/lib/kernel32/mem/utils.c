#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>

VOID CopyMemory(PVOID Destination, CONST VOID* Source, DWORD Length)
{
   DWORD i;
   
   DPRINT("CopyMemory(Destination %x, Source %x, Length %d)\n",
	   Destination,Source,Length);
   
   for (i=0; i<Length; i++)
     {
	((PCH)Destination)[i] = ((PCH)Source)[i];
     }
}
