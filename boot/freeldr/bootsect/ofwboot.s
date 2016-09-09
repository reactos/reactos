	.section .text
        .globl  setup_bats

_start:	
	.long	0xe00000 + 12
	.long	0
	.long	0
	
_begin:
	sync                    
	isync

	lis     %r1,stack@ha    
	addi    %r1,%r1,stack@l 
	addi    %r1,%r1,16384 - 0x10
          
	mfmsr   %r8             
	li      %r0,0
	mtmsr   %r0             
	isync                   

	bl	setup_bats	                               

	li	%r8,0x3030
	mtmsr	%r8

	/* Store ofw call addr */
	mr	%r21,%r5
	lis	%r10,0xe00000@ha
	stw	%r5,ofw_call_addr - _start@l(%r10)

	lis	%r4,_binary_freeldr_tmp_end@ha
	addi	%r4,%r4,_binary_freeldr_tmp_end@l
	lis	%r3,_binary_freeldr_tmp_start@ha
	addi	%r3,%r3,_binary_freeldr_tmp_start@l

	lis	%r5,0x8000@ha
	addi	%r5,%r5,0x8000@l

	bl	copy_bits

	bl	zero_registers

	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_banner - _start

	bl	ofw_print_string

	bl	ofw_print_eol

	bl	setup_exc

	/* Zero CTR */
	mtcr	%r31
	
	lis	%r3,0x8000@ha
	addi	%r3,%r3,0x8000@l

	mtlr	%r3
	
	lis	%r3,call_ofw@ha
	addi	%r3,%r3,call_ofw - _start
	
	b	call_freeldr

/*
 * lifted from ppc/boot/openfirmware/misc.S
 * Copyright (C) Paul Mackerras 1997.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation	;  either version
 * 2 of the License, or (at your option) any later version.
 */

/*
 * Use the BAT2 & 3 registers to map the 1st 16MB of RAM to
 * the address given as the 1st argument.
 */
setup_bats:
        mfpvr   5
        rlwinm  5,5,16,16,31            /* r3 = 1 for 601, 4 for 604 */
        cmpwi   0,5,1
        li      0,0
        bne     4f
        mtibatl 3,0                     /* invalidate BAT first */
        ori     3,3,4                   /* set up BAT registers for 601 */
        li      4,0x7f
        mtibatu 2,3
        mtibatl 2,4
        oris    3,3,0x80
        oris    4,4,0x80
        mtibatu 3,3
        mtibatl 3,4
        b       5f
4:	mtdbatu 3,0                     /* invalidate BATs first */
        mtibatu 3,0
        ori     3,3,0xff                /* set up BAT registers for 604 */
        li      4,2
        mtdbatl 2,4
        mtdbatu 2,3
        mtibatl 2,4
        mtibatu 2,3
        oris    3,3,0x80
        oris    4,4,0x80
        mtdbatl 3,4
        mtdbatu 3,3
        mtibatl 3,4
        mtibatu 3,3
5:	sync
        isync
        blr
	
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
	lwz	%r3,ofw_functions - _start@l(%r9)
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
	beqlr
	lwz	%r6,0(%r3)
	stw	%r6,0(%r5)
	addi	%r3,%r3,4
	addi	%r5,%r5,4
	b	copy_bits

ofw_print_string_hook:
	bl	ofw_print_number
	bl	ofw_exit
	
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

ofw_print_regs:	
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
	
ofw_finddevice_hook:
	subi	%r1,%r1,32
	stw	%r3,0(%r1)
	mflr	%r3
	stw	%r3,4(%r1)
	lwz	%r3,0(%r1)
	bl	ofw_finddevice
	stw	%r3,0(%r1)
	lwz	%r3,4(%r1)
	mtlr	%r3
	lwz	%r3,0(%r1)
	addi	%r1,%r1,32	
	blr
	
ofw_finddevice:
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,32

	/* Store r8, r9, lr */
	stw	%r8,20(%r1)
	stw	%r9,24(%r1)
	mflr	%r8
	stw	%r8,28(%r1)
	
	/* Get finddevice name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_finddevice_name - _start
	stw	%r9,0(%r1)

	/* 1 Argument and 1 return */
	li	%r9,1
	stw	%r9,4(%r1)
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)

	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1
	
	/* Fire */
	blrl
	
	lwz	%r3,16(%r1)
	
	/* Restore registers */
	lwz	%r8,28(%r1)
	mtlr	%r8
	lwz	%r9,24(%r1)
	lwz	%r8,20(%r1)
	
	addi	%r1,%r1,32
	
	/* Return */
	blr

ofw_open:
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,32

	/* Store r8, r9, lr */
	stw	%r8,20(%r1)
	stw	%r9,24(%r1)
	mflr	%r8
	stw	%r8,28(%r1)
	
	/* Get open name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_open_name - _start
	stw	%r9,0(%r1)

	/* 1 Argument and 1 return */
	li	%r9,1
	stw	%r9,4(%r1)
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)

	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1
	
	/* Fire */
	blrl
	
	lwz	%r3,16(%r1)
	
	/* Restore registers */
	lwz	%r8,28(%r1)
	mtlr	%r8
	lwz	%r9,24(%r1)
	lwz	%r8,20(%r1)
	
	addi	%r1,%r1,32
	
	/* Return */
	blr

ofw_getprop_hook:
	/* Reserve stack space:
	 * 32 bytes for the ofw call
	 * 12 bytes for r8, r9, lr
	 */		
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,48

	/* Store r8, r9, lr */
	stw	%r8,32(%r1)
	stw	%r9,36(%r1)
	mflr	%r8
	stw	%r8,40(%r1)
	
	/* Get getprop name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_getprop_name - _start
	stw	%r9,0(%r1)

	/* 4 Argument and 1 return */
	li	%r9,4
	stw	%r9,4(%r1)
	li	%r9,1
	stw	%r9,8(%r1)

	stw	%r3,12(%r1) /* Package */
	stw	%r4,16(%r1) /* Property */
	stw	%r5,20(%r1) /* Return buffer */
	stw	%r6,24(%r1) /* Buffer size */
	
	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1

	/* Fire */
	blrl

	/* Workaround to a wierd crash ... not sure what causes it.
	 * XXX investigate me */
	bl	ofw_print_nothing
	
	/* Return */
	lwz	%r3,28(%r1)

	/* Restore registers */
	lwz	%r8,40(%r1)
	mtlr	%r8
	lwz	%r9,36(%r1)
	lwz	%r8,32(%r1)
	
	addi	%r1,%r1,48
		
	/* Return */
	blr

ofw_getprop:
	/* Reserve stack space:
	 * 32 bytes for the ofw call
	 * 12 bytes for r8, r9, lr
	 */		
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,48

	/* Store r8, r9, lr */
	stw	%r8,32(%r1)
	stw	%r9,36(%r1)
	mflr	%r8
	stw	%r8,40(%r1)
	
	/* Get getprop name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_getprop_name - _start
	stw	%r9,0(%r1)

	/* 4 Argument and 1 return */
	li	%r9,4
	stw	%r9,4(%r1)
	li	%r9,1
	stw	%r9,8(%r1)

	stw	%r3,12(%r1) /* Package */
	stw	%r4,16(%r1) /* Property */
	stw	%r5,20(%r1) /* Return buffer */
	stw	%r6,24(%r1) /* Buffer size */
	
	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1

	/* Fire */
	blrl
	
	/* Return */
	lwz	%r3,28(%r1)

	/* Restore registers */
	lwz	%r8,40(%r1)

	mtlr	%r8
	lwz	%r9,36(%r1)
	lwz	%r8,32(%r1)
	
	addi	%r1,%r1,48
		
	/* Return */
	blr
		
ofw_write:
	/* Reserve stack space:
	 * 28 bytes for the ofw call
	 * 12 bytes for r8, r9, lr
	 */		
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,48

	nop
	
	/* Store r8, r9, lr */
	stw	%r8,28(%r1)
	stw	%r9,32(%r1)
	mflr	%r8
	stw	%r8,36(%r1)
	
	/* Get write name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_write_name - _start
	stw	%r9,0(%r1)

	/* 3 Arguments and 1 return */
	li	%r9,3
	stw	%r9,4(%r1)
	li	%r9,1
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)
	stw	%r4,16(%r1)
	stw	%r5,20(%r1)
	
	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1

	/* Fire */
	blrl

	/* Return */
	lwz	%r3,24(%r1)

	/* Restore registers */
	lwz	%r8,36(%r1)
	mtlr	%r8
	lwz	%r9,32(%r1)
	lwz	%r8,28(%r1)
	
	addi	%r1,%r1,48
	
	/* Return */
	blr

ofw_read:
	/* Reserve stack space:
	 * 28 bytes for the ofw call
	 * 12 bytes for r8, r9, lr
	 */		
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,48

	nop
	
	/* Store r8, r9, lr */
	stw	%r8,28(%r1)
	stw	%r9,32(%r1)
	mflr	%r8
	stw	%r8,36(%r1)
	
	/* Get read name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_read_name - _start
	stw	%r9,0(%r1)

	/* 3 Arguments and 1 return */
	li	%r9,3
	stw	%r9,4(%r1)
	li	%r9,1
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)
	stw	%r4,16(%r1)
	stw	%r5,20(%r1)
	
	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1

	/* Fire */
	blrl

	/* Return */
	lwz	%r3,24(%r1)

	/* Restore registers */
	lwz	%r8,36(%r1)
	mtlr	%r8
	lwz	%r9,32(%r1)
	lwz	%r8,28(%r1)
	
	addi	%r1,%r1,48
	
	/* Return */
	blr
		
ofw_exit:
	lis	%r3,0xe00000@ha
	addi	%r3,%r3,freeldr_halt - _start

	bl	ofw_print_string
/*	
ofw_exit_loop:	
	b	ofw_exit_loop
*/
	/* Load the exit name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_exit_name - _start
	stw	%r9,0(%r1)

	/* Zero args, zero returns */
	xor	%r9,%r9,%r9
	stw	%r9,4(%r1)
	stw	%r9,8(%r1)
	
	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	mr	%r3,%r1

	/* Fire */
	blrl
	/* No return from exit */

ofw_child:
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,32

	/* Store r8, r9, lr */
	stw	%r8,20(%r1)
	stw	%r9,24(%r1)
	mflr	%r8
	stw	%r8,28(%r1)
	
	/* Get child name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_child_name - _start
	stw	%r9,0(%r1)

	/* 1 Argument and 1 return */
	li	%r9,1
	stw	%r9,4(%r1)
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)

	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1
	
	/* Fire */
	blrl
	
	lwz	%r3,16(%r1)
	
	/* Restore registers */
	lwz	%r8,28(%r1)
	mtlr	%r8
	lwz	%r9,24(%r1)
	lwz	%r8,20(%r1)
	
	addi	%r1,%r1,32
	
	/* Return */
	blr
	
ofw_peer:
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,32

	/* Store r8, r9, lr */
	stw	%r8,20(%r1)
	stw	%r9,24(%r1)
	mflr	%r8
	stw	%r8,28(%r1)
	
	/* Get peer name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_peer_name - _start
	stw	%r9,0(%r1)

	/* 1 Argument and 1 return */
	li	%r9,1
	stw	%r9,4(%r1)
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)

	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1
	
	/* Fire */
	blrl
	
	lwz	%r3,16(%r1)
	
	/* Restore registers */
	lwz	%r8,28(%r1)
	mtlr	%r8
	lwz	%r9,24(%r1)
	lwz	%r8,20(%r1)
	
	addi	%r1,%r1,32
	
	/* Return */
	blr
				
ofw_seek:
	/* Reserve stack space ...
	 * 20 bytes for the ofw call,
	 * r8, r9, and lr */
	subi	%r1,%r1,32

	/* Store r8, r9, lr */
	stw	%r8,20(%r1)
	stw	%r9,24(%r1)
	mflr	%r8
	stw	%r8,28(%r1)
	
	/* Get peer name */
	lis	%r8,0xe00000@ha
	addi	%r9,%r8,ofw_seek_name - _start
	stw	%r9,0(%r1)

	/* 3 Arguments and 1 return */
	li	%r9,3
	stw	%r9,4(%r1)
	li	%r9,1
	stw	%r9,8(%r1)

	stw	%r3,12(%r1)
	stw	%r4,16(%r1)
	stw	%r5,20(%r1)

	/* Load up the call address */
	lwz	%r9,ofw_call_addr - _start(%r8)
	mtlr	%r9

	/* Set argument */
	mr	%r3,%r1
	
	/* Fire */
	blrl
	
	lwz	%r3,16(%r1)
	
	/* Restore registers */
	lwz	%r8,28(%r1)
	mtlr	%r8
	lwz	%r9,24(%r1)
	lwz	%r8,20(%r1)
	
	addi	%r1,%r1,32
	
	/* Return */
	blr


setup_exc:
	subi	%r1,%r1,32

	stw	%r3,0(%r1)
	mflr	%r3
	stw	%r3,4(%r1)
	stw	%r8,8(%r1)
	stw	%r9,12(%r1)
	stw	%r10,16(%r1)
	stw	%r12,20(%r1)
	
	lis	%r8,0xe00000@ha
	xor	%r12,%r12,%r12
	addi	%r12,%r12,0x300
	addi	%r9,%r8,dsi_exc - _start
	addi	%r10,%r8,dsi_end - _start

copy_loop:	
	cmp	0,0,%r9,%r10
	beq	ret_setup_exc

	mr	%r3,%r12
	bl	ofw_print_number
	bl	ofw_print_space
	
	lwz	%r3,0(%r9)
	stw	%r3,0(%r12)

	bl	ofw_print_number
	bl	ofw_print_eol

	addi	%r12,%r12,4
	addi	%r9,%r9,4
	b	copy_loop
	
ret_setup_exc:
	mfmsr	%r12
	andi.	%r12,%r12,0xffbf 
	mtmsr	%r12
	
	lwz	%r12,20(%r1)
	lwz	%r10,16(%r1)
	lwz	%r9,12(%r1)
	lwz	%r8,8(%r1)

	lwz	%r3,4(%r1)
	mtlr	%r3
	lwz	%r3,0(%r1)
	
	blr

dsi_exc:
	subi	%r1,%r1,16
	
	stw	%r0,0(%r1)
	stw	%r3,4(%r1)

	mfsrr0	%r3
	addi	%r3,%r3,4
	mtsrr0	%r3

	/* mfsrr1	%r3 */
	/* ori	%r3,%r3,2 */
	/* mtsrr1	%r3 */
		
	lwz	%r3,4(%r1)
	lwz	%r0,0(%r1)

	addi	%r1,%r1,16
	rfi
dsi_end:	
					
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
			
ofw_call_addr:	
	.long	0

ofw_memory_size:
	.long	0
	.long	0
	.long	0
	.long	0
	
ofw_finddevice_name:
	.ascii	"finddevice\0"
		
ofw_getprop_name:
	.ascii	"getprop\0"

ofw_write_name:
	.ascii	"write\0"

ofw_read_name:
	.ascii	"read\0"
	
ofw_exit_name:
	.ascii	"exit\0"

ofw_open_name:
	.ascii	"open\0"

ofw_child_name:
	.ascii	"child\0"

ofw_peer_name:
	.ascii	"peer\0"
		
ofw_seek_name:
	.ascii	"seek\0"
	
ofw_chosen_name:
	.ascii	"/chosen\0"

ofw_stdout_name:
	.ascii	"stdout\0"

ofw_memory_name:
	.ascii	"/memory@0\0"

ofw_reg_name:
	.ascii	"reg\0"
	
ofw_functions:
	.long	ofw_finddevice_hook
	.long	ofw_getprop_hook
	.long	ofw_write
	.long	ofw_read
	.long	ofw_exit
	.long	ofw_print_regs
	.long	ofw_print_string
	.long	ofw_print_number
	.long	ofw_open
	.long	ofw_child
	.long	ofw_peer
	.long   ofw_seek
	
	.org	0x2000
stack:
	.space	0x4000

