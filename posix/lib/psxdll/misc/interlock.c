/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/misc/interlock.c
 * PURPOSE:     inter-locked increments/decrements
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              20/01/2002: Adapted from lib/kernel32/synch/intrlck.c
 */

/*
 * NOTE by KJK::Hyperion: I do not understand what's behind these functions.
 *                        Don't ask me how they work, or to fix errors in them.
 *                        Please refer to the authors referenced in the original
 *                        file, lib/kernel32/synch/intrlck.c
 */

/* TODO? move these in some shared library */

#include <psx/interlock.h>

int __interlock_inc(int * addend)
{
 int ret = 0;

 __asm__
 (	  	 
  "	lock\n"	/* for SMP systems */
  "	incl	(%1)\n"
  "	je	2f\n"
  "	jl	1f\n"
  "	incl	%0\n"
  "	jmp	2f\n"
  "1:	dec	%0\n"    	  
  "2:\n"
  :"=r" (ret):"r" (addend), "0" (0): "memory"
 );

 return (ret);
}

int __interlock_dec(int * addend)
{
 int ret = 0;

 __asm__
 (	  	 
  "	lock\n"	/* for SMP systems */
  "	decl	(%1)\n"
  "	je	2f\n"
  "	jl	1f\n"
  "	incl	%0\n"
  "	jmp	2f\n"
  "1:	dec	%0\n"    	  
  "2:\n"
  :"=r" (ret):"r" (addend), "0" (0): "memory"          
 );

 return (ret);

}

int __interlock_add(int * addend, int increment)
{
 int ret = 0;

 __asm__
 (
  "	lock\n" /* for SMP systems */
  "	xaddl %0,(%1)"
  :"=r" (ret)
  :"r"  (addend), "0" (increment)
  :"memory"
 );

 return (ret);

}

/* EOF */

