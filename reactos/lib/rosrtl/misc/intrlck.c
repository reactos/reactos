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

/* PowerPC Functions from gatomic.c in glib
 *
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * g_atomic_*: atomic operations.
 * Copyright (C) 2003 Sebastian Wilhelmi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
#ifdef _M_IX86
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
#elif defined(_M_PPC)
        ret = *Addend;
        ret = InterlockedExchangeAdd( Addend, ret + 1 );
#endif
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
#ifdef _M_IX86
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
#elif defined(_M_PPC)
        ret = *lpAddend;
        ret = InterlockedExchangeAdd( lpAddend, ret - 1 );
#endif
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
#ifdef _M_IX86
	__asm__ ( /* lock for SMP systems */
                  "lock\n\txchgl %0,(%1)"
                  :"=r" (ret):"r" (target), "0" (value):"memory" );
#elif defined(_M_PPC)
        do {
            ret = *(volatile LONG *)target;
        } while( InterlockedCompareExchange( target, value, ret ) != ret );
#endif
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
            LONG Comperand     ) 
{
    LONG ret;
#ifdef _M_IX86
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (Destination), "r" (Exchange), "0" (Comperand) : "memory" );
#elif defined(_M_PPC)
    __asm__ __volatile__ ("sync\n"
                          "1: lwarx   %0,0,%1\n"
                          "   subf.   %0,%2,%0\n"
                          "   bne     2f\n"
                          "   stwcx.  %3,0,%1\n"
                          "   bne-    1b\n"
                          "2: isync"
                          : "=&r" (ret)
                          : "b" (Destination), "r" (Comperand), "r" (Exchange)
                          : "cr0", "memory"); 
#endif
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
#ifdef _M_IX86
	__asm__ ( /* lock for SMP systems */
                  "lock\n\t"
                  "xaddl %0,(%1)"
                  :"=r" (ret)
                  :"r" (Addend), "0" (Increment)
                  :"memory" );
#elif defined(_M_PPC)
        LONG newval;
        do {
            ret = *(volatile LONG *)Addend;
            newval = ret + Increment;
        } while (InterlockedCompareExchange(Addend, ret, newval) != ret);
#endif
	return ret;
}
