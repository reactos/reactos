/* $Id: critical.c,v 1.7 2000/06/29 23:35:29 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

/* shouldn't these have correct Rtl equivalents?  I just copied from kernel32 */

PVOID 
STDCALL 
InterlockedCompareExchange(
	    PVOID *Destination, 
	    PVOID Exchange,     
            PVOID Comperand     ) 
{	
	PVOID ret;
	__asm__ ( /* lock for SMP systems */
                  "lock\n\t"
                  "cmpxchgl %2,(%1)"
                  :"=r" (ret)
                  :"r" (Destination),"r" (Exchange), "0" (Comperand)
                  :"memory" );
	return ret;

}

LONG 
STDCALL 
InterlockedIncrement(PLONG Addend)
{
	long ret = 0;
	__asm__
	(	  	 
	   "\tlock\n"	/* for SMP systems */
	   "\tincl	(%1)\n"
	   "\tje	2f\n"
	   "\tjl	1f\n"
	   "\tincl	%0\n"
	   "\tjmp	2f\n"
	   "1:\tdec	%0\n"    	  
	   "2:\n"
	   :"=r" (ret):"r" (Addend), "0" (0): "memory"
	);
	return ret;
}

LONG 
STDCALL
InterlockedDecrement(PLONG lpAddend)
{
	long ret;
	__asm__
	(	  	 
	   "\tlock\n"	/* for SMP systems */
	   "\tdecl	(%1)\n"
	   "\tje	2f\n"
	   "\tjl	1f\n"
	   "\tincl	%0\n"
	   "\tjmp	2f\n"
	   "1:\tdec	%0\n"    	  
	   "2:\n"
	   :"=r" (ret):"r" (lpAddend), "0" (0): "memory"          
	);
	return ret;


}

VOID
STDCALL
RtlDeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   NtClose(lpCriticalSection->LockSemaphore);
   lpCriticalSection->Reserved = -1;
}

VOID
STDCALL
RtlEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
   ULONG ret;
   if( (ret = InterlockedIncrement(&(lpCriticalSection->LockCount) )) != 1 ) {
      if (lpCriticalSection->OwningThread != Thread ) {
	 NtWaitForSingleObject( lpCriticalSection->LockSemaphore, 0, FALSE );
	 lpCriticalSection->OwningThread = Thread;
      }
   }
   else
      lpCriticalSection->OwningThread = Thread;
   
   lpCriticalSection->RecursionCount++;
}

VOID
STDCALL
RtlInitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   pcritical->LockCount = 0;
   pcritical->RecursionCount = 0;
   NtCreateSemaphore( &pcritical->LockSemaphore, STANDARD_RIGHTS_ALL, NULL, 0, 1 );
   pcritical->OwningThread = (HANDLE)-1; // critical section has no owner yet
   pcritical->Reserved = 0;
}

VOID
STDCALL
RtlLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   lpCriticalSection->RecursionCount--;
   if ( lpCriticalSection->RecursionCount == 0 ) {
      lpCriticalSection->OwningThread = (HANDLE)-1;
      // if LockCount > 0 and RecursionCount == 0 there
      // is a waiting thread 
      // ReleaseSemaphore will fire up a waiting thread
      if ( InterlockedDecrement( &lpCriticalSection->LockCount ) > 0 )
	 {
	    NtReleaseSemaphore( lpCriticalSection->LockSemaphore,1,NULL);
	 }
   }
   else InterlockedDecrement( &lpCriticalSection->LockCount );
}

BOOLEAN
STDCALL
RtlTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   if( InterlockedCompareExchange( (PVOID *)&lpCriticalSection->LockCount, (PVOID)1, (PVOID)0 ) == 0 )
      {
	 lpCriticalSection->OwningThread = (HANDLE) NtCurrentTeb()->Cid.UniqueThread;
	 lpCriticalSection->RecursionCount++;
	 return TRUE;
      }
   if( lpCriticalSection->OwningThread == (HANDLE)NtCurrentTeb()->Cid.UniqueThread )
      {
	 lpCriticalSection->RecursionCount++;
	 return TRUE;
      }
   return FALSE;
}


/* EOF */





