/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMERS:     arty
                    (i386 implementation by Alex Ionescu)
 */

/* INCLUDES ******************************************************************/

        .globl KiSystemService
	.globl KiSystemService1
	.globl kiss_proceed
	.globl kiss_end
KiSystemService1:
        stw 2,4(1) // r1
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
	/* This save is important */
        stw 0,140(1) // srr0
        mflr 0
        stw 0,128(1)
        mfctr 0
        stw 0,136(1)
        mfsrr1 0
        stw 0,144(1)
        mfdsisr 0
        stw 0,148(1)
        mfdar 0
        stw 0,152(1)
        lis 3,KiSystemService@ha
        addi 3,3,KiSystemService@l
        mtctr 3
        mr 3,1
        subi 1,1,0x100
        bctrl
        addi 1,1,0x100
	/* Return from kernel */
        lwz 3,12(1) /* Result */
        lwz 0,128(1)
        mtlr 0
        lwz 0,140(1)
        mtsrr0 0
        lwz 0,144(1)
        mtsrr1 0
        lwz 1,4(1)    /* Stack */
        rfi
        
        .globl KiDecrementerTrapHandler
        .globl KiDecrementerTrapHandlerEnd
        .globl KiDecrementerTrap
KiDecrementerTrapHandler:
        // switch to trap stack until we figure out where to go
        mtsprg0 1
        lis 1,_kernel_trap_stack@ha
        addi 1,1,_kernel_trap_stack@l
        subi 1,1,0x100
        stw 0,0(1)
        mfsprg0 0
        stw 0,4(1)
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
	/* This save is important */
        stw 0,140(1) // srr0
        mflr 0
        stw 0,128(1)
        mfctr 0
        stw 0,136(1)
        mfsrr1 0
        stw 0,144(1)
        mfdsisr 0
        stw 0,148(1)
        mfdar 0
        stw 0,152(1)
        lis 3,KiDecrementerTrap@ha
        addi 3,3,KiDecrementerTrap@l
        mtctr 3
        lis 3,KiDecrementerTrapFinish@ha
        addi 3,3,KiDecrementerTrapFinish@l
        mtlr 3
        mr 3,1
        subi 1,1,0x100
        bctr
        /* We don't return here */
KiDecrementerTrapHandlerEnd:
        .long 0
        
        /* Decrementer needs to restore the full CPU state */
        .globl KiDecrementerTrapFinish
KiDecrementerTrapFinish:
        addi 1,1,0x100
        lwz 2,8(1)
        lwz 3,12(1)
        lwz 4,16(1)
        lwz 5,20(1)
        lwz 6,24(1)
        lwz 7,28(1)
        lwz 8,32(1)
        lwz 9,36(1)
        lwz 10,40(1)
        lwz 11,44(1)
        lwz 12,48(1)
        lwz 13,52(1)
        lwz 14,56(1)
        lwz 15,60(1)
        lwz 16,64(1)
        lwz 17,68(1)
        lwz 18,72(1)
        lwz 19,76(1)
        lwz 20,80(1)
        lwz 21,84(1)
        lwz 22,88(1)
        lwz 23,92(1)
        lwz 24,96(1)
        lwz 25,100(1)
        lwz 26,104(1)
        lwz 27,108(1)
        lwz 28,112(1)
        lwz 29,116(1)
        lwz 30,120(1)
        lwz 31,124(1)
        lwz 0,140(1)
        mtsrr0 0
        lwz 0,128(1)
        mtlr 0
        lwz 0,136(1)
        mtctr 0
        lwz 0,144(1)
        mtsrr1 0
        lwz 0,148(1)
        mtdsisr 0
        lwz 0,152(1)
        mtdar 0
        // back out r0 and r1
        lwz 0,0(1)
        lwz 1,4(1)
        // Bye!!1
        rfi
