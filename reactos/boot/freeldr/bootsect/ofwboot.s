	.section .text

_start:	
	.long	0xe00000 + 12
	.long	0
	.long	0
	
	.globl	_begin
	.globl	call_ofw
	.globl	ofw_functions
_begin:
	sync                    
	isync

	lis     %r1,stack@ha    
	addi    %r1,%r1,stack@l 
	addi    %r1,%r1,16384 - 0x10
          
	/* Store ofw call addr */
	mr	%r21,%r5
	lis	%r10,0xe00000@ha
	stw	%r5,ofw_call_addr - _start@l(%r10)

	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_banner - _start

	bl	ofw_print_string
	bl	ofw_print_eol
	bl	zero_registers

	/* Zero CTR */
	mtcr	%r31

	lis	%r3,0xe17000@ha
	addi	%r3,%r3,0xe17000@l

	mtlr	%r3
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,call_ofw - _start@l

	blr

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
	
freeldr_banner:
	.ascii	"ReactOS OpenFirmware Boot Program\r\n\0"

freeldr_halt:
	.ascii	"ReactOS OpenFirmware Boot Program Halting\r\n\0"

ofw_memory_size:
	.long	0
	.long	0
	.long	0
	.long	0
	
	.org	0x1000
stack:
	.space	0x4000

