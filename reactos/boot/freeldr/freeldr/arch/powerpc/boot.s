	.section ".text"
	.extern PpcInit
	.globl _start
	.globl call_ofw
_start:
	sync                    
	isync

	lis     %r1,stack@ha    
	addi    %r1,%r1,stack@l 
	addi    %r1,%r1,16384 - 0x10
          
	/* Store ofw call addr */
	mr	%r21,%r5
	lis	%r10,ofw_call_addr@ha
	stw	%r5,ofw_call_addr@l(%r10)

	bl	zero_registers
	
	/* Zero CTR */
	mtcr	%r31

	lis	%r3,PpcInit@ha
	addi	%r3,%r3,PpcInit@l
	mtlr	%r3

	/* Check for ofw */
	lis	%r3,ofw_call_addr@ha
	lwz	%r3,ofw_call_addr@l(%r3)
	cmpw	%r3,%r31 /* Zero? */
	mr	%r3,%r31
	beq	bootme
		
	lis	%r3,call_ofw@ha
	addi	%r3,%r3,call_ofw@l

bootme:	
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
	
ofw_memory_size:
	.long	0
	.long	0
	.long	0
	.long	0
	
	.org	0x1000
stack:
	.space	0x4000

	.globl _bss
	.section ".bss"
_bss:
	.long	0
