/* i386-specific implemetation of Translation Buffer Flushing
 * Written By: Alex Ionescu <alex@relsoft.net>
 * Reference: IA-32 Intel® Architecture Software Developer's Manual, Volume 3: System Programming Guide,
 *	      Chapter 10 - Memory Cache Control. Section 10.9 - Invalidating the Translation Lookaside Buffers 
 */

#include <internal/i386/ke.h>


.globl _KeFlushCurrentTb@0
_KeFlushCurrentTb@0:
    /* Check for global page support */	
	testl	$X86_FEATURE_PGE, (_Ke386CpuidFlags)
	jz	  .L1

	/* Modifying the PSE, PGE or PAE Flag in CR4 causes the TLB to be flushed */
	movl 	%cr4, %eax
	andl 	$~X86_CR4_PGE, %eax
	movl	%eax, %cr4
	orl  	$X86_CR4_PGE, %eax
	movl 	%eax, %cr4
	
	ret

.L1:
    /* the old way ... */
	movl 	%cr3, %eax
	movl 	%eax, %cr3
	ret
	


