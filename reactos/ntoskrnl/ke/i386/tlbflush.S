/* i386-specific implemetation of Translation Buffer Flushing
 * Written By: Alex Ionescu <alex@relsoft.net>
 * Reference: IA-32 Intel® Architecture Software Developer's Manual, Volume 3: System Programming Guide,
 *	      Chapter 10 - Memory Cache Control. Section 10.9 - Invalidating the Translation Lookaside Buffers 
 */

.globl _KeFlushCurrentTb@0
_KeFlushCurrentTb@0:	
	/* Modifying the PSE, PGE or PAE Flag in CR4 causes the TLB to be flushed */
	andl $0xFFFFFF7F, %eax
	movl %eax, %cr4
	orl $0x80, %eax
	movl %eax, %cr4
	
	ret




