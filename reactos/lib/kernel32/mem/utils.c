/* $Id: utils.c,v 1.5 2000/07/01 17:07:00 ea Exp $
 *
 * FILE: lib/kernel32/mem/utils.c
 */
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


/* EOF */
