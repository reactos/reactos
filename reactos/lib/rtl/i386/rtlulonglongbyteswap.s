/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/RtlUlonglongByteSwap.S
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 */

.globl _UlonglongByteSwap
 
.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_UlonglongByteSwap:
                       push  ebp          // save base 
                       mov   ebp,esp      // move stack to base
                       mov   edx,[ebp+8]  // load the higher part of ULONGLONG
                       mov   eax,[ebp+12] // load the lower part of ULONGLONG    
                       bswap edx          // swap the higher part
                       bswap eax          // swap the lower part 
                       pop   ebp          // restore the base   
                       ret
