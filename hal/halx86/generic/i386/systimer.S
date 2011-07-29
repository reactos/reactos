/*
 * FILE:            hal/halx86/generic/timer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#include <ks386.inc>

EXTERN _HalpAcquireSystemHardwareSpinLock@0:PROC
EXTERN _HalpReleaseCmosSpinLock@0:PROC
EXTERN _DbgBreakPoint@0:PROC
EXTERN _HalpCurrentRollOver:DWORD
EXTERN _HalpPerfCounterCutoff:DWORD

#define PIC1_BASE         HEX(20) /* IO base address for master PIC */
#define PIC2_BASE         HEX(A0) /* IO base address for slave PIC */
#define PIC1_COMMAND      PIC1_BASE
#define PIC1_DATA         (PIC1_BASE+1)
#define PIC2_COMMAND      PIC2_BASE
#define PIC2_DATA         (PIC2_BASE+1)
#define PIC_EOI           HEX(20)
#define PIC_SPECIFIC_EOI2 HEX(62)

#define CMOS_ADDR HEX(70)
#define CMOS_DATA HEX(71)
#define CMOS_REGISTER_A HEX(0A)
#define CMOS_REGISTER_B HEX(0B)
#define CMOS_REGISTER_C HEX(0C)
#define CMOS_REGISTER_D HEX(0D)

#define PIT_CH0    HEX(40)
#define PIT_MODE   HEX(43)
#define SYSTEM_CTRL_PORT_A HEX(92)

/* GLOBALS *******************************************************************/

.data
ASSUME CS:NOTHING, DS:NOTHING, ES:NOTHING, FS:NOTHING, GS:NOTHING

PUBLIC _HalpPerfCounter
_HalpLastPerfCounterLow:    .long 0
_HalpLastPerfCounterHigh:   .long 0
_HalpPerfCounter:
_HalpPerfCounterLow:        .long 0
_HalpPerfCounterHigh:       .long 0
_HalpSystemHardwareFlags:   .long 0

/* FUNCTIONS *****************************************************************/

.code
PUBLIC _HalpCalibrateStallExecution@0
_HalpCalibrateStallExecution@0:

    /* Setup the stack frame */
    push ebp
    mov ebp, esp
    sub esp, 12

    /* Save EFLAGS and kill interrupts */
    pushfd
    cli
    
    /* Get the current interrupt mask on the PICs */
    xor eax, eax
    in al, PIC2_DATA
    shl eax, 8
    in al, PIC1_DATA
    
    /* Save it */
    push eax
    
    /* Now mask everything except the RTC and PIC 2 chain-interrupt */
    mov eax, NOT (HEX(04) OR HEX(100))
    
    /* Program the PICs */
    out PIC1_DATA, al
    shr eax, 8
    out PIC2_DATA, al
    
    /* Now get the IDT */
    sidt [ebp-8]
    mov ecx, [ebp-6]
    
    /* Get the IDT entry for the RTC */
    mov eax, HEX(38)
    shl eax, 3
    add ecx, eax
    
    /* Save the original RTC ISR */
    push [ecx]
    push [ecx+4]
    push ecx
    
    /* Now load our new handler */
    mov eax, offset OnlyOnePersonCanWriteHalCode
    mov [ecx], ax
    mov word ptr [ecx+2], KGDT_R0_CODE
    mov word ptr [ecx+4], HEX(08E00)
    shr eax, 16
    mov [ecx+6], ax

    /* Reset our counter */
    mov dword ptr [ebp-12], 0
    
    /* Acquire CMOS lock */
    call _HalpAcquireSystemHardwareSpinLock@0
    
    /* Now initialize register A on the CMOS */
    mov ax, HEX(2D00) OR CMOS_REGISTER_A
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register B */
    mov ax, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Don't touch the LastKnownGoodConfig hack */
    and al, 1
    mov ah, al
    
    /* Enable the interrupt */
    or ah, HEX(42)
    
    /* Now write the register B */
    mov al, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register C */
    mov al, CMOS_REGISTER_C
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Read register D */
    mov al, CMOS_REGISTER_D
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Release CMOS lock */
    mov dword ptr [ebp-12], 0
    call _HalpReleaseCmosSpinLock@0
    
    /* Initialize looper */
    xor eax, eax
    
    /* Align to 16 bytes */
    .align 16
    
    /* Enable interrupts! */
    sti
    jmp Looper
    
    /* Align to 16 bytes */
    .align 16
    
    /* Subtract one count */
Looper:
    sub eax, 1
    jnz Looper
    
    /* ASSERT: If we got here, then the RTC never fired */
    call _DbgBreakPoint@0
    jmp Looper
    
OnlyOnePersonCanWriteHalCode:
    /*********************** THIS IS THE RTC HANDLER **************************/

    /* Increment the interrupt count and check if this is the first one */
    inc dword ptr [ebp-12]
    cmp dword ptr [ebp-12], 1
    jnz ComputeStall
    
    /*
     * It is the first one -- we'll ignore it, since it fires randomly!
     * Get rid of the old return address and push the new one in (our looper)
     */
    pop eax
    push offset Looper
    
    /* Acquire CMOS lock */
    call _HalpAcquireSystemHardwareSpinLock@0
    
    /* Now initialize register A on the CMOS */
    mov ax, HEX(2D00) OR CMOS_REGISTER_A
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register B */
    mov ax, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Don't touch the LastKnownGoodConfig hack */
    and al, 1
    mov ah, al
    
    /* Enable the interrupt */
    or ah, HEX(42)
    
    /* Now write the register B */
    mov al, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register C */
    mov al, CMOS_REGISTER_C
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Read register D */
    mov al, CMOS_REGISTER_D
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Release CMOS lock */
    call _HalpReleaseCmosSpinLock@0
    
    /* Dismiss the interrupt */
    mov al, PIC_EOI
    out PIC2_COMMAND, al
    mov al, PIC_SPECIFIC_EOI2
    out PIC1_COMMAND, al
    
    /* Reset the counter and return back to the looper */
    xor eax, eax
    iretd
    
    /******************* THIS IS THE 2ND RTC HANDLER **************************/
ComputeStall:

    /* Do the calculation */
    neg eax
    xor edx, edx
    mov ecx, 125000 /* RTC fires every 125 ms */
    div ecx
    
    /* Is the remainder 0? */
    cmp edx, 0
    jz FoundFactor
    
    /* Otherwise fix-up the loop count */
    inc eax
    
FoundFactor:
    /* Save the stall scale factor */
    mov fs:[KPCR_STALL_SCALE_FACTOR], eax
    
    /* Prepare for interrupt return */
    pop eax
    push offset AndItsNotYou
    mov eax, HEX(13)
    
    /* Acquire CMOS lock */
    call _HalpAcquireSystemHardwareSpinLock@0
    
    /* Now initialize register A on the CMOS */
    mov ax, HEX(2D00) OR CMOS_REGISTER_A
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register B */
    mov ax, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Don't touch the LastKnownGoodConfig hack */
    and al, 1
    mov ah, al
    
    /* Disable the interrupt */
    or ah, 2
    
    /* Now write the register B */
    mov al, CMOS_REGISTER_B
    out CMOS_ADDR, al
    jmp $+2
    mov al, ah
    out CMOS_DATA, al
    jmp $+2
    
    /* Read register C */
    mov al, CMOS_REGISTER_C
    out CMOS_ADDR, al
    jmp $+2
    in al, CMOS_DATA
    jmp $+2
    
    /* Release CMOS lock */
    call _HalpReleaseCmosSpinLock@0
    
    /* Dismiss the interrupt */
    mov al, PIC_EOI
    out PIC2_COMMAND, al
    mov al, PIC_SPECIFIC_EOI2
    out PIC1_COMMAND, al
    
    /* Disable interrupts on return */
    and word ptr [esp+8], NOT EFLAGS_INTERRUPT_MASK
    iretd
    
    /************************* WE ARE BACK FROM RTC ***************************/
AndItsNotYou:

    /* Restore the IDT */
    pop ecx
    pop [ecx+4]
    pop [ecx]
    
    /* Restore the mask */
    pop eax
    out PIC1_DATA, al
    shr eax, 8
    out PIC2_DATA, al
    
    /* Restore EFLAGS */
    popf
    
    /* Restore stack and return */
    mov esp, ebp
    pop ebp
    ret


#ifndef _MINIHAL_
PUBLIC _KeStallExecutionProcessor@4
_KeStallExecutionProcessor@4:

    /* Get the number of microseconds required */
    mov ecx, [esp+4]
    jecxz Done

    /* Multiply by the stall factor */
    mov eax, fs:[KPCR_STALL_SCALE_FACTOR]
    mul ecx

    /* Align to 16 bytes */
    .align 16

    /* Jump to subtraction loop */
    jmp SubtractLoop

    /* Align to 16 bytes */
    .align 16

    /* Subtract one count */
SubtractLoop:
    sub eax, 1
    jnz SubtractLoop

Done:
    /* Return */
    ret 4
#endif

PUBLIC _KeQueryPerformanceCounter@4
_KeQueryPerformanceCounter@4:

    /* Check if we were called too early */
    cmp dword ptr _HalpCurrentRollOver, 0
    je NoCount

    /* Save volatiles */
    push ebx
    push esi

LoopPreInt:

    /* Disable interrupts */
    pushf
    cli

LoopPostInt:

    /* Get the current value */
    mov ebx, dword ptr _HalpPerfCounterLow
    mov esi, dword ptr _HalpPerfCounterHigh

    /* Read 8254 timer */
    mov al, 0 /* Interrupt on terminal count */
    out PIT_MODE, al
    in al, SYSTEM_CTRL_PORT_A
    or al, byte ptr _HalpPerfCounterCutoff
    out SYSTEM_CTRL_PORT_A, al
    jmp $+2
    in al, PIT_CH0
    jmp $+2
    movzx ecx, al
    in al, PIT_CH0
    mov ch, al

    /* Enable interrupts and do a short wait */
    popf
    nop
    jmp $+2

    /* Disable them again */
    pushf
    cli

    /* Get the counter value again */
    mov eax, dword ptr _HalpPerfCounterLow
    mov edx, dword ptr _HalpPerfCounterHigh

    /* Check if someone updated the counter */
    cmp eax, ebx
    jnz LoopPostInt
    cmp edx, esi
    jnz LoopPostInt

    /* Check if the current 8254 value causes rollover */
    neg ecx
    add ecx, dword ptr _HalpCurrentRollOver
    jnb DoRollOver

SetSum:

    /* Calculate the sum */
    add eax, ecx
    adc edx, 0

    /* Check if we're above or below the last high value */
    cmp edx, dword ptr _HalpLastPerfCounterHigh
    jb short BelowHigh
    jnz short BelowLow

    /* Check if we're above or below the last low value */
    cmp eax, dword ptr _HalpLastPerfCounterLow
    jb BelowHigh

BelowLow:

    /* Update the last value and bring back interrupts */
    mov dword ptr _HalpLastPerfCounterLow, eax
    mov dword ptr _HalpLastPerfCounterHigh, edx
    popf

    /* Check if caller wants frequency */
    cmp dword ptr [esp+12], 0
    jz ReturnNoFreq

    /* Save hard-coded frequency */
    mov ecx, dword ptr [esp+12]
    mov dword ptr [ecx], 1193182
    mov dword ptr [ecx+4], 0

ReturnNoFreq:

    /* Restore volatiles */
    pop esi
    pop ebx
    ret 4

NoCount:

    /* Return empty, called too soon */
    mov eax, 0
    mov edx, 0
    ret 4

DoRollOver:

    /* We might have an incoming interrupt, save EFLAGS and reset rollover */
    mov esi, [esp]
    mov ecx, dword ptr _HalpCurrentRollOver
    popf

    /* Check if interrupts were enabled and try again */
    test esi, EFLAGS_INTERRUPT_MASK
    jnz LoopPreInt

    /* They're not, continue where we left */
    pushf
    jmp SetSum

BelowHigh:

    /* Get the last counter values */
    mov ebx, dword ptr _HalpLastPerfCounterLow
    mov esi, dword ptr _HalpLastPerfCounterHigh

    /* Check if the previous value was 0 and go back if yes */
    mov ecx, ebx
    or ecx, esi
    jz BelowLow

    /* Make sure that the count is still valid */
    sub ebx, eax
    sbb esi, edx
    jnz InvalidCount
    cmp ebx, dword ptr _HalpCurrentRollOver
    jg InvalidCount

    /* Fixup the count with the last known value */
    sub eax, ebx
    sbb edx, esi

    /* We might have an incoming interrupt, save EFLAGS */
    mov ecx, [esp]
    popf

    /* Check if interrupts were enabled and try again */
    test ecx, EFLAGS_INTERRUPT_MASK
    jnz LoopPreInt

    /* They're not, continue where we left */
    pushf
    jmp BelowLow

InvalidCount:
    popf
    xor eax, eax
    mov dword ptr _HalpLastPerfCounterLow, eax
    mov dword ptr _HalpLastPerfCounterHigh, eax
    jmp LoopPreInt

END
