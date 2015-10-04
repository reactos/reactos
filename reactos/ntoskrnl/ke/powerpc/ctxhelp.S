/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/powerpc/ctxhelp.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMERS:     arty
                    (i386 implementation by Alex Ionescu)
 */

/* INCLUDES ******************************************************************/

        .text
        .globl syscall_start
        .globl syscall_end
        .globl KiSystemService
syscall_start:
        mtsprg0 0
        mtsprg1 1
        /* Save and modify srr0 */
        /* Make a place to store the old srr0 and srr1 ... we may fault
         * getting KiSystemService1 which will clobber them. */
        lis 1,1
	mfsrr0 0
        stw 0,-16(1)
        mfsrr1 0
        stw 0,-12(1)
        /* Load the target address */
	lis 1,KiSystemService1@ha
	addi 1,1,KiSystemService1@l
	mtsrr0 1
        mfsprg0 0
        mfsprg1 1
	rfi
syscall_end:
	.space 4

        .globl KiSystemService
	.globl KiSystemService1
	.globl kiss_proceed
	.globl kiss_end
        .align 8
KiSystemService1:
        stwu 1,-256(1)
        stw 0,16(1)
        mflr 0
        stw 0,264(1)
        addi 1,1,16
        stw 2,8(1)
        stw 3,12(1)
        stw 4,16(1)
        stw 5,20(1)
        stw 6,24(1)
        stw 7,28(1)
        stw 8,32(1)
        stw 9,36(1)
        stw 10,40(1)
        stw 11,44(1)
        stw 12,48(1)
        stw 13,52(1)
        stw 14,56(1)
        stw 15,60(1)
        stw 16,64(1)
        stw 17,68(1)
        stw 18,72(1)
        stw 19,76(1)
        stw 20,80(1)
        stw 21,84(1)
        stw 22,88(1)
        stw 23,92(1)
        stw 24,96(1)
        stw 25,100(1)
        stw 26,104(1)
        stw 27,108(1)
        stw 28,112(1)
        stw 29,116(1)
        stw 30,120(1)
        stw 31,124(1)
        mflr 0
        stw 0,128(1)
        mfctr 0
        stw 0,136(1)
        mfmsr 0
        andi. 0,0,0xffef
        mtmsr 0
        lis 2,1
        lwz 30,-12(2)
        lwz 31,-16(2)
        mfmsr 0
        ori 0,0,0x10
        mtmsr 0
        stw 31,140(1)
        stw 30,144(1)
        mfdsisr 0
        stw 0,148(1)
        mfdar 0
        stw 0,152(1)
        lis 3,KiSystemService@ha
        addi 3,3,KiSystemService@l
        mtctr 3
        mr 3,1
        subi 1,1,16
        bctrl
        addi 1,1,16
	/* Return from kernel */
        lwz 3,32(1) /* Result */
        lwz 0,128(1)
        mtlr 0
        lwz 0,140(1)
        mtsrr0 0
        lwz 0,144(1)
        mtsrr1 0
        addi 1,1,0x100 - 16
        rfi
        
        .globl KiDecrementerTrapHandler
        .globl KiDecrementerTrapHandlerEnd
        .globl KiDecrementerTrap
KiDecrementerTrapHandler:
        mtsprg0 0
        mtsprg1 1
        /* Save and modify srr0 */
        /* Make a place to store the old srr0 and srr1 ... we may fault
         * getting KiSystemService1 which will clobber them. */
        lis 1,1
        mfsprg1 0
        stw 0,-24(1)
        mfsrr1 0
        stw 0,-28(1)
	mfsrr0 0
        stw 0,-32(1)
        /* Load the target address */
        lis 1,KiDecrementerTrapUpper@ha
        addi 1,1,KiDecrementerTrapUpper@l
	mtsrr0 1
        mfsprg0 0
        mfsprg1 1
	rfi
KiDecrementerTrapHandlerEnd:
        .long 0
        
        /* Decrementer needs to restore the full CPU state */
        .globl KiDecrementerTrapUpper
        .align 8
KiDecrementerTrapUpper:
        lis 1,_kernel_trap_stack@ha
        addi 1,1,_kernel_trap_stack@l
        subi 1,1,0x200
        stw 0,0x5c(1)
        /* Stack handled a bit later */
        stw 2,0x64(1)
        stw 3,0x68(1)
        stw 4,0x6c(1)
        stw 5,0x70(1)
        stw 6,0x74(1)
        stw 7,0x78(1)
        stw 8,0x7c(1)
        stw 9,0x80(1)
        stw 10,0x84(1)
        stw 11,0x88(1)
        stw 12,0x8c(1)
        stw 13,0x90(1)
        stw 14,0x94(1)
        stw 15,0x98(1)
        stw 16,0x9c(1)
        stw 17,0xa0(1)
        stw 18,0xa4(1)
        stw 19,0xa8(1)
        stw 20,0xac(1)
        stw 21,0xb0(1)
        stw 22,0xb4(1)
        stw 23,0xb8(1)
        stw 24,0xbc(1)
        stw 25,0xc0(1)
        stw 26,0xc4(1)
        stw 27,0xc8(1)
        stw 28,0xcc(1)
        stw 29,0xd0(1)
        stw 30,0xd4(1)
        stw 31,0xd8(1)
        mfcr 0
        stw 0,0x108(1)
        mfxer 0
        stw 0,0x10c(1)
        mflr 0
        stw 0,0x118(1)
        mfctr 0
        stw 0,0x11c(1)
        mfmsr 0
        andi. 0,0,0x7fef
        mtmsr 0
        lis 2,1
        lwz 29,-24(2) // Stack
        lwz 30,-28(2) // srr1
        lwz 31,-32(2) // srr0
        mfmsr 0
        ori 0,0,0x30
        mtmsr 0
        stw 29,0x60(1)  // Stack
        stw 30,0x110(1) // srr1
        stw 31,0x114(1) // srr0
        mr 3,1
        subi 1,1,16
        bl KiDecrementerTrap
        addi 1,1,16
        lwz 2,0x64(1)
        lwz 3,0x68(1)
        lwz 4,0x6c(1)
        lwz 5,0x70(1)
        lwz 6,0x74(1)
        lwz 7,0x78(1)
        lwz 8,0x7c(1)
        lwz 9,0x80(1)
        lwz 10,0x84(1)
        lwz 11,0x88(1)
        lwz 12,0x8c(1)
        lwz 13,0x90(1)
        lwz 14,0x94(1)
        lwz 15,0x98(1)
        lwz 16,0x9c(1)
        lwz 17,0xa0(1)
        lwz 18,0xa4(1)
        lwz 19,0xa8(1)
        lwz 20,0xac(1)
        lwz 21,0xb0(1)
        lwz 22,0xb4(1)
        lwz 23,0xb8(1)
        lwz 24,0xbc(1)
        lwz 25,0xc0(1)
        lwz 26,0xc4(1)
        lwz 27,0xc8(1)
        lwz 28,0xcc(1)
        lwz 29,0xd0(1)
        lwz 30,0xd4(1)
        lwz 31,0xd8(1)
        lwz 0,0x108(1)
        mtcr 0
        lwz 0,0x10c(1)
        mtxer 0
        lwz 0,0x110(1)
        mtsrr1 0
        lwz 0,0x114(1)
        mtsrr0 0
        lwz 0,0x118(1)
        mtlr 0
        lwz 0,0x11c(1)
        mtctr 0
        // back out r0 and r1
        lwz 0,0x5c(1)
        lwz 1,0x60(1)
        // Bye!!1
        rfi
