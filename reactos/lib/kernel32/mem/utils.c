#include <windows.h>
#include <kernel32/kernel32.h>

VOID CopyMemory(PVOID Destination, CONST VOID* Source, DWORD Length)
{
   DWORD i;
   
   for (i=0; i<Length; i++)
     {
	((PCH)Destination)[i] = ((PCH)Source)[i];
     }
}
