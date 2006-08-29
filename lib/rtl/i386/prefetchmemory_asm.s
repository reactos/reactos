/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            prefetchmemory_asm.S
 * PURPOSE:         Memory functions
 * PROGRAMMERS:     Patrick Baggett (baggett.patrick@gmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Magnus Olsen (magnusolsen@greatlord.com)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  @RtlPrefetchMemoryNonTemporal@8

/* FUNCTIONS ***************************************************************/

@RtlPrefetchMemoryNonTemporal@8:
	ret         /* Overwritten by ntoskrnl/ke/i386/kernel.c if SSE is supported (see Ki386SetProcessorFeatures() ) */

	mov eax, [_Ke386CacheAlignment]    // Get cache line size

	// This is fastcall, so ecx = address, edx = size
fetch_next_line:
	prefetchnta byte ptr [ecx]  // prefechnta(address)
	add ecx, eax                // address = address + cache_line_size
	sub edx, eax                // count = count - cache_line_size
	ja fetch_next_line          //     goto fetch_next_line
	ret
