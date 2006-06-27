	.section .text

_start:	
	.long	0xe00000 + 12
	.long	0
	.long	0
	
	.globl ofw_functions_addr
	.globl ofw_dumpregs
	
ofw_functions_addr:
	.long	ofw_functions

	.globl _begin
_begin:
	sync                    
	isync

	lis     %r1,stack@ha    
	addi    %r1,%r1,stack@l 
	addi    %r1,%r1,16384 - 0x10
          
	/* Store ofw call addr */
	mr	%r21,%r5
	lis	%r8,_start@ha
	addi	%r7,%r8,ofw_call_addr - _start
	stw	%r21,0(%r7)

	bl	ofw_dumpregs

foo:
	b	foo

	.align	4
call_freeldr:
	/* Get the address of the functions list --
	 * Note:
	 * Because of little endian switch we must use an even number of
	 * instructions here..  Pad with a nop if needed. */
	mfmsr	%r10
	ori	%r10,%r10,1
	mtmsr	%r10

	nop
	
	/* Note that this is little-endian from here on */
	blr
	nop

	.align  4

call_ofw:
	/* R3 has the function offset to call (n * 4) 
	 * Other arg registers are unchanged.
	 * Note that these 4 instructions are in reverse order due to
	 * little-endian convention */
	andi.	%r0,%r0,65534
	mfmsr	%r0
	mtmsr	%r0
	/* Now normal ordering resumes */
	subi	%r1,%r1,0x100

	stw	%r8,4(%r1)
	stw	%r9,8(%r1)
	stw	%r10,12(%r1)
	mflr	%r8
	stw	%r8,16(%r1)

	lis	%r10,0xe00000@ha
	add	%r9,%r3,%r10
	lwz	%r3,ofw_functions_addr - _start@l(%r9)
	lwz	%r3,0(%r3)
	mtctr	%r3
	
	mr	%r3,%r4
	mr	%r4,%r5
	mr	%r5,%r6
	mr	%r6,%r7
	mr	%r7,%r8

	/* Goto the swapped function */
	bctrl

	lwz	%r8,16(%r1)
	mtlr	%r8

	lwz	%r8,4(%r1)
	lwz	%r9,8(%r1)
	lwz	%r10,12(%r1)

	addi	%r1,%r1,0x100
	/* Ok, go back to little endian */
	mfmsr	%r0
	ori	%r0,%r0,1
	mtmsr	%r0

	/* Note that this is little-endian from here on */
	blr
	nop

zero_registers:
	xor	%r2,%r2,%r2
	mr	%r0,%r2
	mr	%r3,%r2
	
	mr	%r4,%r2
	mr	%r5,%r2
	mr	%r6,%r2
	mr	%r7,%r2

	mr	%r8,%r2
	mr	%r9,%r2
	mr	%r10,%r2
	mr	%r11,%r2

	mr	%r12,%r2
	mr	%r13,%r2
	mr	%r14,%r2
	mr	%r15,%r2
	
	mr	%r12,%r2
	mr	%r13,%r2
	mr	%r14,%r2
	mr	%r15,%r2
	
	mr	%r16,%r2
	mr	%r17,%r2
	mr	%r18,%r2
	mr	%r19,%r2
	
	mr	%r20,%r2
	mr	%r21,%r2
	mr	%r22,%r2
	mr	%r23,%r2
	
	mr	%r24,%r2
	mr	%r25,%r2
	mr	%r26,%r2
	mr	%r27,%r2
	
	mr	%r28,%r2
	mr	%r29,%r2
	mr	%r30,%r2
	mr	%r31,%r2

	blr
	
	.org	0x1000
freeldr_banner:
	.ascii	"ReactOS OpenFirmware Boot Program\r\n\0"

freeldr_halt:
	.ascii	"ReactOS OpenFirmware Boot Program Halting\r\n\0"

freeldr_reg_init:
	.ascii	"r\0"
freeldr_reg_lr:	
	.ascii	"lr \0"
freeldr_reg_cr:	
	.ascii	"cr \0"
freeldr_reg_ctr:	
	.ascii	"ctr\0"
freeldr_reg_msr:	
	.ascii	"msr\0"
			
ofw_memory_size:
	.long	0
	.long	0
	.long	0
	.long	0
	
ofw_chosen_name:
	.ascii	"/chosen\0"

ofw_stdout_name:
	.ascii	"stdout\0"

ofw_memory_name:
	.ascii	"/memory@0\0"

ofw_reg_name:
	.ascii	"reg\0"
	
	.org	0x2000
stack:
	.space	0x4000

