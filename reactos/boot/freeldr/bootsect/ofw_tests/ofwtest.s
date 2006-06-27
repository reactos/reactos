	.section .text

_start:	
	.long	0xe00000 + 12
	.long	0
	.long	0
	
	.globl ofw_dumpregs
	
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
	
	.org	0x1000
stack:
	.space	0x4000

