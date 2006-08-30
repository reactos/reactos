/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/RtlUlongByteSwap.S
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 */

.globl _UshortByteSwap
 
.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_UshortByteSwap:
                       push  ebp          // save base 
                       mov   ebp,esp      // move stack to base
                       mov   eax,[ebp+8]  // load the USHORT                       
                       bswap eax          // swap the USHORT, xchg is slow so we use bswap with rol 
                       rol   eax,16       // make it USHORT
                       pop   ebp          // restore the base   
                       ret
