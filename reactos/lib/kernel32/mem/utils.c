#include <windows.h>
#include <kernel32/kernel32.h>

VOID CopyMemory(PVOID Destination, CONST VOID* Source, DWORD Length)
{
   DWORD i;
   
   dprintf("CopyMemory(Destination %x, Source %x, Length %d)\n",
	   Destination,Source,Length);
   
   for (i=0; i<Length; i++)
     {
	((PCH)Destination)[i] = ((PCH)Source)[i];
     }
}
