/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/RtlUlongByteSwap.S
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 */

.globl _UlongByteSwap
 
.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_UlongByteSwap:
                       push  ebp          // save base 
                       mov   ebp,esp      // move stack to base
                       mov   eax,[ebp+8]  // load the ULONG                       
                       bswap eax          // swap the ULONG
                       pop   ebp          // restore the base   
                       ret
