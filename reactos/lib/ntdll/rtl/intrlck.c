/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/intrlck.c
 * PURPOSE:         Inter lock increments
 * UPDATE HISTORY:
 *                  Created 30/09/99
 */

/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1997 Onno Hovers
 * Copied from kernel32
 */


/************************************************************************
*           InterlockedIncrement			                *
*									*
* InterlockedIncrement adds 1 to a long variable and returns		*
*  -  a negative number if the result < 0				*
*  -  zero if the result == 0						*
*  -  a positive number if the result > 0				*
*									*
* The returned number need not be equal to the result!!!!		*
* note:									*
*									*
************************************************************************/

#include <windows.h>

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

/************************************************************************
*           InterlockedDecrement			        	*
*									*
* InterlockedIncrement adds 1 to a long variable and returns		*
*  -  a negative number if the result < 0				*
*  -  zero if the result == 0						*
*  -  a positive number if the result > 0				*
*									*
* The returned number need not be equal to the result!!!!		*
************************************************************************/

LONG 
STDCALL
InterlockedDecrement(LPLONG lpAddend)
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

/************************************************************************
 *           InterlockedExchange				
 *
 * Atomically exchanges a pair of values.
 *
 * RETURNS
 *	Prior value of value pointed to by Target
 */
 
LONG 
STDCALL 
InterlockedExchange(LPLONG target, LONG value )
{
	
	long ret;
	__asm__ ( /* lock for SMP systems */
                  "lock\n\txchgl %0,(%1)"
                  :"=r" (ret):"r" (target), "0" (value):"memory" );
	return ret;


}

/************************************************************************
 *           InterlockedCompareExchange		
 *
 * Atomically compares Destination and Comperand, and if found equal exchanges
 * the value of Destination with Exchange
 *
 * RETURNS
 *	Prior value of value pointed to by Destination
 */
LONG
STDCALL 
InterlockedCompareExchange(
	LPLONG Destination, 
	LONG Exchange,     
	LONG Comperand)
{	
	LONG ret;
	__asm__ ( /* lock for SMP systems */
                  "lock\n\t"
                  "cmpxchgl %2,(%1)"
                  :"=r" (ret)
                  :"r" (Destination),"r" (Exchange), "0" (Comperand)
                  :"memory" );
	return ret;

}

/************************************************************************
 *           InterlockedExchangeAdd			
 *
 * Atomically adds Increment to Addend and returns the previous value of
 * Addend
 *
 * RETURNS
 *	Prior value of value pointed to by Addend
 */
LONG 
STDCALL 
InterlockedExchangeAdd(
	    PLONG Addend, 
	    LONG Increment 
) 
{

	LONG ret;
	__asm__ ( /* lock for SMP systems */
                  "lock\n\t"
                  "xaddl %0,(%1)"
                  :"=r" (ret)
                  :"r" (Addend), "0" (Increment)
                  :"memory" );
	return ret;

}
