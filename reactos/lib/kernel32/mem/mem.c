/* $Id: mem.c,v 1.2 2000/07/01 17:07:00 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user mode libraries
 * FILE:            kernel32/mem/utils.cc
 * PURPOSE:         Various simple memory initalizations functions
 */

#include <windows.h>

VOID ZeroMemory(PVOID Destination, DWORD Length)
{
   #ifdef __i386__
     
   #endif /* __i386__ */
}

VOID CopyMemory(PVOID Destination, CONST VOID* Source, DWORD Length)
{
   #ifdef __i386__
   #endif /* __i386__ */
}
	   

/* EOF */
