	.section .text
	.globl ofw_functions
	.globl ofw_call_addr
call_ofw:	
       /* R3 has the function offset to call (n * 4)
        * Other arg registers are unchanged. */
       subi    %r1,%r1,0x100
       stw     %r8,24(%r1)
       mflr    %r8
       stw     %r8,0(%r1)
       stw     %r3,4(%r1)
       stw     %r4,8(%r1)
       stw     %r5,12(%r1)
       stw     %r6,16(%r1)
       stw     %r7,20(%r1)
       stw     %r9,28(%r1)
       stw     %r10,32(%r1)
       stw     %r20,36(%r1)

       lis     %r10,0xe00000@ha
       addi    %r8,%r10,ofw_functions@l
       add     %r8,%r3,%r8
       lwz     %r9,0(%r8)
       mtctr   %r9

       mr      %r3,%r4
       mr      %r4,%r5
       mr      %r5,%r6
       mr      %r6,%r7
       mr      %r7,%r8
       mr      %r8,%r9

       /* Call ofw proxy function */
       bctrl

       lwz     %r8,0(%r1)
       mtlr    %r8
       lwz     %r4,8(%r1)
       lwz     %r5,12(%r1)
       lwz     %r6,16(%r1)
       lwz     %r7,20(%r1)
       lwz     %r8,24(%r1)
       lwz     %r9,28(%r1)
       lwz     %r10,32(%r1)
       lwz     %r20,36(%r1)
       addi    %r1,%r1,0x100
       blr

prim_strlen:
	mr	%r5,%r3
prim_strlen_loop:	
	lbz	%r4,0(%r3)
	cmpi	0,0,%r4,0
	beq	prim_strlen_done
	addi	%r3,%r3,1
	b	prim_strlen_loop
	
prim_strlen_done:
	sub	%r3,%r3,%r5
	blr
	
copy_bits:
	cmp	0,0,%r3,%r4
	bgelr

	andi.	%r6,%r3,0xfff
	beql	ofw_dumpregs
	mtdec	%r3
	
	lwz	%r6,0(%r3)
	stw	%r6,0(%r5)
	addi	%r3,%r3,4
	addi	%r5,%r5,4
	b	copy_bits

ofw_print_string:
	/* Reserve some stack space */
	subi	%r1,%r1,32

	/* Save args */
	stw	%r3,0(%r1)
	
	/* Save the lr, a scratch register */
	stw	%r8,8(%r1)
	mflr	%r8
	stw	%r8,12(%r1)

	/* Load the package name */
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,ofw_chosen_name - _start

	/* Fire */
	bl	ofw_finddevice

	/* Load up for getprop */
	stw	%r3,16(%r1)

	lis	%r4,0xe00000@ha
	addi	%r4,%r4,ofw_stdout_name - _start

	addi	%r5,%r1,20

	li	%r6,4

	bl	ofw_getprop

	/* Measure the string and remember the length */
	lwz	%r3,0(%r1)
	bl	prim_strlen
	mr	%r5,%r3
	
	lwz	%r3,20(%r1)
	lwz	%r4,0(%r1)

	/* Write the string */
	bl	ofw_write

	/* Return */
	lwz	%r8,12(%r1)
	mtlr	%r8
	lwz	%r8,8(%r1)
	
	addi	%r1,%r1,32
	blr

	/* Print 8 hex digits representing a number in r3 */
ofw_print_number:
	subi	%r1,%r1,32
	stw	%r8,0(%r1)
	mflr	%r8
	stw	%r8,4(%r1)
	stw	%r9,8(%r1)

	xor	%r9,%r9,%r9
	stw	%r9,12(%r1)

	/* Set up and, devide, shift */
	mr	%r8,%r3
	lis	%r6,0xf0000000@ha
	lis	%r7,0x10000000@ha
	li	%r9,8

ofw_number_loop:
	nop
	cmpi	0,0,%r9,0
	beq	ofw_number_return
	subi	%r9,%r9,1

	/* Body: isolate digit, divide, print */
	and	%r5,%r6,%r8
	divwu	%r4,%r5,%r7
	srwi	%r6,%r6,4
	srwi	%r7,%r7,4

	nop
	
	cmpi	0,0,%r4,10
	bge	ofw_number_letter
	addi	%r4,%r4,'0'
	b	ofw_number_digit_out
	
ofw_number_letter:
	addi	%r4,%r4,'A' - 10

ofw_number_digit_out:	
	stb	%r4,12(%r1)
	addi	%r3,%r1,12

	stw	%r6,16(%r1)
	stw	%r7,20(%r1)
	stw	%r8,24(%r1)
	stw	%r9,28(%r1)
	
	bl	ofw_print_string

	lwz	%r6,16(%r1)
	lwz	%r7,20(%r1)
	lwz	%r8,24(%r1)
	lwz	%r9,28(%r1)
	
	b	ofw_number_loop

ofw_number_return:	
	/* Return */
	lwz	%r9,8(%r1)
	lwz	%r8,4(%r1)
	mtlr	%r8
	lwz	%r8,0(%r1)
	addi	%r1,%r1,32
	blr

ofw_print_eol:
	subi	%r1,%r1,16
	stw	%r8,0(%r1)
	mflr	%r8
	stw	%r8,4(%r1)
	li	%r4,0x0d0a
	sth	%r4,8(%r1)
	xor	%r4,%r4,%r4
	sth	%r4,10(%r1)
	addi	%r3,%r1,8
	bl	ofw_print_string
	lwz	%r8,4(%r1)
	mtlr	%r8
	lwz	%r8,0(%r1)
	addi	%r1,%r1,16
	blr

ofw_print_nothing:
	subi	%r1,%r1,16
	stw	%r8,0(%r1)
	mflr	%r8
	stw	%r8,4(%r1)
	li	%r4,0
	sth	%r4,8(%r1)
	xor	%r4,%r4,%r4
	sth	%r4,10(%r1)
	addi	%r3,%r1,8
	bl	ofw_print_string
	lwz	%r8,4(%r1)
	mtlr	%r8
	lwz	%r8,0(%r1)
	addi	%r1,%r1,16
	blr

ofw_print_space:
	subi	%r1,%r1,16
	stw	%r8,0(%r1)
	mflr	%r8
	stw	%r8,4(%r1)
	li	%r4,0x2000
	sth	%r4,8(%r1)
	xor	%r4,%r4,%r4
	sth	%r4,10(%r1)
	addi	%r3,%r1,8
	bl	ofw_print_string
	lwz	%r8,4(%r1)
	mtlr	%r8
	lwz	%r8,0(%r1)
	addi	%r1,%r1,16
	blr

ofw_dumpregs:	
	/* Construct ofw exit call */
	subi	%r1,%r1,0xa0

	stw	%r0,0(%r1)
	stw	%r1,4(%r1)
	stw	%r2,8(%r1)
	stw	%r3,12(%r1)

	stw	%r4,16(%r1)
	stw	%r5,20(%r1)
	stw	%r6,24(%r1)
	stw	%r7,28(%r1)
	
	stw	%r8,32(%r1)
	stw	%r9,36(%r1)
	stw	%r10,40(%r1)
	stw	%r11,44(%r1)
	
	stw	%r12,48(%r1)
	stw	%r13,52(%r1)
	stw	%r14,56(%r1)
	stw	%r15,60(%r1)
	
	stw	%r16,64(%r1)
	stw	%r17,68(%r1)
	stw	%r18,72(%r1)
	stw	%r19,76(%r1)
	
	stw	%r20,80(%r1)
	stw	%r21,84(%r1)
	stw	%r22,88(%r1)
	stw	%r23,92(%r1)
	
	stw	%r24,96(%r1)
	stw	%r25,100(%r1)
	stw	%r26,104(%r1)
	stw	%r27,108(%r1)
	
	stw	%r28,112(%r1)
	stw	%r29,116(%r1)
	stw	%r30,120(%r1)
	stw	%r31,124(%r1)

	mflr	%r0
	stw	%r0,128(%r1)
	mfcr	%r0
	stw	%r0,132(%r1)
	mfctr	%r0
	stw	%r0,136(%r1)
	mfmsr	%r0
	stw	%r0,140(%r1)

	/* Count at zero */
	xor	%r0,%r0,%r0
	stw	%r0,144(%r1)
	mr	%r3,%r1
	stw	%r3,148(%r1)
	
	/* Body, print the regname, then the register */
ofw_register_loop:
	lwz	%r3,144(%r1)
	cmpi	0,0,%r3,32
	beq	ofw_register_special
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_init - _start
	bl	ofw_print_string
	lwz	%r3,144(%r1)
	bl	ofw_print_number
	bl	ofw_print_space
	lwz	%r3,144(%r1)
	mulli	%r3,%r3,4
	add	%r3,%r1,%r3
	lwz	%r3,0(%r3)
	stw	%r3,152(%r1)
	bl	ofw_print_number
	lwz	%r3,144(%r1)
	addi	%r3,%r3,1
	stw	%r3,144(%r1)

	bl	ofw_print_space
	
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_init - _start
	bl	ofw_print_string
	lwz	%r3,144(%r1)
	bl	ofw_print_number
	bl	ofw_print_space
	lwz	%r3,144(%r1)
	mulli	%r3,%r3,4
	add	%r3,%r1,%r3
	lwz	%r3,0(%r3)
	stw	%r3,152(%r1)
	bl	ofw_print_number
	lwz	%r3,144(%r1)
	addi	%r3,%r3,1
	stw	%r3,144(%r1)

	b	done_dump

dump_optional:	
	bl	ofw_print_space
	bl	ofw_print_space
	lwz	%r3,152(%r1)
	lwz	%r3,0(%r3)
	bl	ofw_print_number
	bl	ofw_print_space	
	lwz	%r3,152(%r1)
	lwz	%r3,4(%r3)
	bl	ofw_print_number
	bl	ofw_print_space	
	lwz	%r3,152(%r1)
	lwz	%r3,8(%r3)
	bl	ofw_print_number
	bl	ofw_print_space	
	lwz	%r3,152(%r1)
	lwz	%r3,12(%r3)
	bl	ofw_print_number
	bl	ofw_print_space
done_dump:	
	bl	ofw_print_eol
	b	ofw_register_loop

ofw_register_special:
	/* LR */
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_lr - _start
	bl	ofw_print_string
	bl	ofw_print_space
	lwz	%r3,128(%r1)
	bl	ofw_print_number
	bl	ofw_print_eol
	
	/* CR */
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_cr - _start
	bl	ofw_print_string
	bl	ofw_print_space
	lwz	%r3,132(%r1)
	bl	ofw_print_number
	bl	ofw_print_eol
	
	/* CTR */
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_ctr - _start
	bl	ofw_print_string
	bl	ofw_print_space
	lwz	%r3,136(%r1)
	bl	ofw_print_number
	bl	ofw_print_eol
	
	/* MSR */
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_reg_msr - _start
	bl	ofw_print_string
	bl	ofw_print_space
	lwz	%r3,140(%r1)
	bl	ofw_print_number
	bl	ofw_print_eol

	/* Return */
	lwz	%r0,128(%r1)
	mtlr	%r0
	
	lwz	%r0,0(%r1)
	lwz	%r2,8(%r1)
	lwz	%r3,12(%r1)

	lwz	%r4,16(%r1)
	lwz	%r5,20(%r1)
	lwz	%r6,24(%r1)
	lwz	%r7,28(%r1)

	addi	%r1,%r1,0xa0
		
	blr
	
ofw_chosen_name:
	.ascii	"/chosen\0"

ofw_stdout_name:
	.ascii	"stdout\0"

ofw_memory_name:
	.ascii	"/memory@0\0"

ofw_reg_name:
	.ascii	"reg\0"
	
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
