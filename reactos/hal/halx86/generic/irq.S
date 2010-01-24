/*
 * FILE:            hal/halx86/generic/irq.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Software, System and Hardware IRQ Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* Enable this (and the define in spinlock.c) to make UP HAL work for MP Kernel */
/* #define CONFIG_SMP */

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

KiI8259MaskTable:
    .long 0                             /* IRQL 0 */
    .long 0                             /* IRQL 1 */
    .long 0                             /* IRQL 2 */
    .long 0                             /* IRQL 3 */
    .long 0xFF800000                    /* IRQL 4 */
    .long 0xFFC00000                    /* IRQL 5 */
    .long 0xFFE00000                    /* IRQL 6 */
    .long 0xFFF00000                    /* IRQL 7 */
    .long 0xFFF80000                    /* IRQL 8 */
    .long 0xFFFC0000                    /* IRQL 9 */
    .long 0xFFFE0000                    /* IRQL 10 */
    .long 0xFFFF0000                    /* IRQL 11 */
    .long 0xFFFF8000                    /* IRQL 12 */
    .long 0xFFFFC000                    /* IRQL 13 */
    .long 0xFFFFE000                    /* IRQL 14 */
    .long 0xFFFFF000                    /* IRQL 15 */
    .long 0xFFFFF800                    /* IRQL 16 */
    .long 0xFFFFFC00                    /* IRQL 17 */
    .long 0xFFFFFE00                    /* IRQL 18 */
    .long 0xFFFFFE00                    /* IRQL 19 */
    .long 0xFFFFFE80                    /* IRQL 20 */
    .long 0xFFFFFEC0                    /* IRQL 21 */
    .long 0xFFFFFEE0                    /* IRQL 22 */
    .long 0xFFFFFEF0                    /* IRQL 23 */
    .long 0xFFFFFEF8                    /* IRQL 24 */
    .long 0xFFFFFEF8                    /* IRQL 25 */
    .long 0xFFFFFEFA                    /* IRQL 26 */
    .long 0xFFFFFFFA                    /* IRQL 27 */
    .long 0xFFFFFFFB                    /* IRQL 28 */
    .long 0xFFFFFFFB                    /* IRQL 29 */
    .long 0xFFFFFFFB                    /* IRQL 30 */
    .long 0xFFFFFFFB                    /* IRQL 31 */

FindHigherIrqlMask:
    .long 0xFFFFFFFE                    /* IRQL  0 */
    .long 0xFFFFFFFC                    /* IRQL 1 */
    .long 0xFFFFFFF8                    /* IRQL 2 */
    .long 0xFFFFFFF0                    /* IRQL 3 */
    .long 0x7FFFFF0                     /* IRQL 4 */
    .long 0x3FFFFF0                     /* IRQL 5 */
    .long 0x1FFFFF0                     /* IRQL 6 */
    .long 0x0FFFFF0                     /* IRQL 7 */
    .long 0x7FFFF0                      /* IRQL 8 */
    .long 0x3FFFF0                      /* IRQL 9 */
    .long 0x1FFFF0                      /* IRQL 10 */
    .long 0x0FFFF0                      /* IRQL 11 */
    .long 0x7FFF0                       /* IRQL 12 */
    .long 0x3FFF0                       /* IRQL 13 */
    .long 0x1FFF0                       /* IRQL 14 */
    .long 0x0FFF0                       /* IRQL 15 */
    .long 0x7FF0                        /* IRQL 16 */
    .long 0x3FF0                        /* IRQL 17 */
    .long 0x1FF0                        /* IRQL 18 */
    .long 0x1FF0                        /* IRQL 19 */
    .long 0x17F0                        /* IRQL 20 */
    .long 0x13F0                        /* IRQL 21 */
    .long 0x11F0                        /* IRQL 22 */
    .long 0x10F0                        /* IRQL 23 */
    .long 0x1070                        /* IRQL 24 */
    .long 0x1030                        /* IRQL 25 */
    .long 0x1010                        /* IRQL 26 */
    .long 0x10                          /* IRQL 27 */
    .long 0                             /* IRQL 28 */
    .long 0                             /* IRQL 29 */
    .long 0                             /* IRQL 30 */
    .long 0                             /* IRQL 31 */

HalpSpecialDismissTable:
    .rept 7
    .long GenericIRQ                    /* IRQ 0-7 */
    .endr
    .long IRQ7                          /* IRQ 7 */
    .rept 5
    .long GenericIRQ                    /* IRQ 8-12 */
    .endr
    .long IRQ13                         /* IRQ 13 */
    .long GenericIRQ                    /* IRQ 14 */
    .long IRQ15                         /* IRQ 15 */
    .rept 20
    .long GenericIRQ                    /* IRQ 16-35 */
    .endr
#if DBG
.rept 172
    .long InvalidIRQ                    /* IRQ 36-207 */
.endr
#endif

HalpSpecialDismissLevelTable:
    .rept 7
    .long GenericIRQLevel               /* IRQ 0-7 */
    .endr
    .long IRQ7Level                     /* IRQ 7 */
    .rept 5
    .long GenericIRQLevel               /* IRQ 8-12 */
    .endr
    .long IRQ13Level                    /* IRQ 13 */
    .long GenericIRQLevel               /* IRQ 14 */
    .long IRQ15Level                    /* IRQ 15 */

SWInterruptLookUpTable:
    .byte PASSIVE_LEVEL                 /* IRR 0 */
    .byte PASSIVE_LEVEL                 /* IRR 1 */
    .byte APC_LEVEL                     /* IRR 2 */
    .byte APC_LEVEL                     /* IRR 3 */
    .byte DISPATCH_LEVEL                /* IRR 4 */
    .byte DISPATCH_LEVEL                /* IRR 5 */
    .byte DISPATCH_LEVEL                /* IRR 6 */
    .byte DISPATCH_LEVEL                /* IRR 7 */

SWInterruptHandlerTable:
    .long _KiUnexpectedInterrupt        /* PASSIVE_LEVEL */
    .long _HalpApcInterrupt             /* APC_LEVEL */
    .long _HalpDispatchInterrupt        /* DISPATCH_LEVEL */

SWInterruptHandlerTable2:
    .long _KiUnexpectedInterrupt        /* PASSIVE_LEVEL */
    .long _HalpApcInterrupt2ndEntry     /* APC_LEVEL */
    .long _HalpDispatchInterrupt2ndEntry /* DISPATCH_LEVEL */

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

/* FUNCTIONS *****************************************************************/

/* HAL interrupt handlers */
GENERATE_HAL_INT_HANDLERS

.globl _HalpHardwareInterruptLevel
.func HalpHardwareInterruptLevel
_HalpHardwareInterruptLevel:

    /* Get IRQL and check for pending interrupts */
    mov eax, PCR[KPCR_IRQL]
    mov ecx, PCR[KPCR_IRR]
    and ecx, FindHigherIrqlMask[eax*4]
    jz NothingHardware

    /* Check the active IRR */
    test dword ptr PCR[KPCR_IRR_ACTIVE], 0xFFFFFFF0
    jnz NothingHardware

    /* Check for pending software interrupts */
    mov eax, ecx
    bsr ecx, eax
    mov eax, 1
    shl eax, cl

    /* Clear IRR */
    xor PCR[KPCR_IRR], eax

    /* Now dispatch the interrupt */
    call SWInterruptHandlerTable[ecx*4]

NothingHardware:
    ret
.endfunc

.globl @HalRequestSoftwareInterrupt@4
.func @HalRequestSoftwareInterrupt@4, @HalRequestSoftwareInterrupt@4
@HalRequestSoftwareInterrupt@4:

    /* Get IRR mask */
    mov eax, 1
    shl eax, cl

    /* Disable interrupts */
    pushf
    cli

    /* Set IRR and get IRQL */
    or PCR[KPCR_IRR], eax
    mov ecx, PCR[KPCR_IRQL]

    /* Get software IRR mask */
    mov eax, PCR[KPCR_IRR]
    and eax, 3

    /* Get highest pending software interrupt and check if it's higher */
    xor edx, edx
    mov dl, SWInterruptLookUpTable[eax]
    cmp dl, cl
    jbe AfterCall

    /* Call the pending interrupt */
    call SWInterruptHandlerTable[edx*4]

AfterCall:

    /* Retore interrupts and return */
    popf
    ret
.endfunc

.globl _HalDisableSystemInterrupt@8
.func HalDisableSystemInterrupt@8
_HalDisableSystemInterrupt@8:

    /* Convert to vector */
    movzx ecx, byte ptr [esp+4]
    sub ecx, PRIMARY_VECTOR_BASE

    /* Disable interrupts and set the new IDR */
    mov edx, 1
    shl edx, cl
    cli
    or PCR[KPCR_IDR], edx

    /* Get the current mask */
    xor eax, eax
    in al, 0xA1
    shl eax, 8
    in al, 0x21

    /* Mask off the interrupt and write the new mask */
    or eax, edx
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Return with interrupts enabled */
    in al, 0xA1
    sti
    ret 8
.endfunc

.globl _HalEnableSystemInterrupt@12
.func HalEnableSystemInterrupt@12
_HalEnableSystemInterrupt@12:

    /* Get the vector and validate it */
    movzx ecx, byte ptr [esp+4]
    sub ecx, PRIMARY_VECTOR_BASE
    jb Invalid
    cmp ecx, CLOCK2_LEVEL
    jnb Invalid

#if 0
    /* Is PCI IRQ Routing enabled? */
    cmp byte ptr _HalpIrqMiniportInitialized, 0
    jz NoMiniport

    /* UNHANDLED */
    UNHANDLED_PATH

NoMiniport:
    /* Check if this is an EISA IRQ */
    bt _HalpEisaIrqIgnore, ecx
    jb IgnoredIrq

    /* Clear the EISA Edge/Level Control Register */
    btr _HalpEisaELCR, ecx

    /* Get the interrupt type */
    mov al, [esp+12]
    cmp al, 0
    jnz Latched

    /* Check the register again */
    bt _HalpEisaELCR, ecx
    jb Dismiss

    /* Check if the miniport is active */
    cmp byte ptr _HalpIrqMiniportInitialized, 0
    jz Dismiss

    /* Update the EISA Edge/Level Control Register */
    bts _HalpEisaELCR, ecx

Dismiss:
    /* Use the level hardware interrupt handler */
    mov dword ptr SWInterruptHandlerTableHardware[ecx*4], offset _HalpHardwareInterruptLevel
    mov edx, HalpSpecialDismissLevelTable[ecx*4]
    mov HalpSpecialDismissTable[ecx*4], edx

Latched:
    /* Is PCI IRQ Routing enabled? */
    cmp byte ptr _HalpIrqMiniportInitialized, 0
    jz IgnoredIrq

    /* UNHANDLED */
    UNHANDLED_PATH
#endif
IgnoredIrq:
    /* Calculate the new IDR */
    mov eax, 1
    shl eax, cl
    not eax
    cli
    and PCR[KPCR_IDR], eax

    /* Get the current IRQL and mask the IRQs in the PIC */
    mov eax, PCR[KPCR_IRQL]
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Enable interrupts and return TRUE */
    sti
    mov eax, 1
    ret 12

Invalid:

    /* Fail, invalid IRQ */
#if DBG
    int 3
#endif
    xor eax, eax
    ret 12
.endfunc

.globl _HalBeginSystemInterrupt@12
.func HalBeginSystemInterrupt@12
_HalBeginSystemInterrupt@12:

    /* Convert to IRQ and call the handler */
    xor ecx, ecx
    mov cl, byte ptr [esp+8]
    sub ecx, PRIMARY_VECTOR_BASE
    jmp HalpSpecialDismissTable[ecx*4]

IRQ15:
    /* This is IRQ 15, check if it's spurious */
    mov al, 0xB
    out 0xA0, al
    jmp $+2
    in al, 0xA0
    test al, 0x80
    jnz GenericIRQ

    /* Cascaded interrupt... dismiss it and return FALSE */
CascadedInterrupt:
    mov al, 0x62
    out 0x20, al
    mov eax, 0
    ret 12

IRQ7:
    /* This is IRQ 7, check if it's spurious */
    mov al, 0xB
    out 0x20, al
    jmp $+2
    in al, 0x20
    test al, 0x80
    jnz GenericIRQ

    /* It is, return FALSE */
    mov eax, 0
    ret 12

IRQ13:
    /* AT 80287 latch clear */
    xor al, al
    out 0xF0, al

GenericIRQ:
    /* Get current and new IRQL */
    xor eax, eax
    mov al, byte ptr [esp+4]
    mov ebx, PCR[KPCR_IRQL]

    /* Set and save old */
    mov PCR[KPCR_IRQL], eax
    mov edx, [esp+12]
    mov [edx], bl

    /* Set IRQ mask in the PIC */
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Check to which PIC the EOI was sent */
    mov eax, ecx
    cmp eax, 8
    jnb Pic1

    /* Write mask to master PIC */
    or al, 0x60
    out 0x20, al

    /* Enable interrupts and return TRUE */
    sti
    mov eax, 1
    ret 12

Pic1:
    /* Write mask to slave PIC */
    mov al, 0x20
    out 0xA0, al
    mov al, 0x62
    out 0x20, al

    /* Enable interrupts and return TRUE */
    sti
    mov eax, 1
    ret 12

#if DBG
InvalidIRQ:
    /* Dismiss it */
    mov eax, 0
    ret 12
#endif
.endfunc

IRQ15Level:
    /* This is IRQ 15, check if it's spurious */
    mov al, 0xB
    out 0xA0, al
    jmp $+2
    in al, 0xA0
    test al, 0x80
    jnz GenericIRQLevel
    jmp CascadedInterrupt

IRQ7Level:
    /* This is IRQ 7, check if it's spurious */
    mov al, 0xB
    out 0x20, al
    jmp $+2
    in al, 0x20
    test al, 0x80
    jnz GenericIRQLevel

    /* It is, return FALSE */
SpuriousInterrupt:
    mov eax, 0
    ret 12

IRQ13Level:
    /* AT 80287 latch clear */
    xor al, al
    out 0xF0, al

GenericIRQLevel:
    /* Save IRQL */
    xor eax, eax
    mov al, [esp+4]

    /* Set IRQ mask in the PIC */
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Compute new IRR */
    mov eax, ecx
    mov ebx, 1
    add ecx, 4
    shl ebx, cl
    or PCR[KPCR_IRR], ebx

    /* Get IRQLs */
    mov cl, [esp+4]
    mov bl, PCR[KPCR_IRQL]
    mov edx, [esp+12]

    /* Check to which PIC the EOI was sent */
    cmp eax, 8
    jnb Pic1Level

    /* Write mask to master PIC */
    or al, 0x60
    out 0x20, al

    /* Check for spurious */
    cmp cl, bl
    jbe SpuriousInterrupt

    /* Write IRQL values */
    movzx ecx, cl
    mov PCR[KPCR_IRQL], ecx
    mov [edx], bl

    /* Enable interrupts and return TRUE */
    sti
    mov eax, 1
    ret 12

Pic1Level:
    /* Write mask to slave and master PIC */
    add al, 0x58
    out 0xA0, al
    mov al, 0x62
    out 0x20, al

    /* Was this a lower interrupt? */
    cmp cl, bl
    jbe SpuriousInterrupt

    /* Write IRQL values */
    movzx ecx, cl
    mov PCR[KPCR_IRQL], ecx
    mov [edx], bl

    /* Enable interrupts and return TRUE */
    sti
    mov eax, 1
    ret 12

.globl _HalEndSystemInterrupt@8
.func HalEndSystemInterrupt@8
_HalEndSystemInterrupt@8:

    /* Read IRQL */
    xor ecx, ecx
    mov cl, [esp+4]

    /* Check if it's a software interrupt */
    cmp dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    jbe SkipMask2

    /* Hardware interrupt, mask the appropriate IRQs in the PIC */
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

SkipMask2:

    /* Set IRQL and check if there are pending software interrupts */
    mov PCR[KPCR_IRQL], ecx
    mov eax, PCR[KPCR_IRR]
    mov al, SWInterruptLookUpTable[eax]
    cmp al, cl
    ja DoCall
    ret 8

DoCall:

    /* There are pending software interrupts, call their handlers */
    add esp, 12
    jmp SWInterruptHandlerTable2[eax*4]
.endfunc

.globl @KfLowerIrql@4
.func @KfLowerIrql@4
_@KfLowerIrql@4:
@KfLowerIrql@4:

    /* Cleanup IRQL */
    and ecx, 0xFF

    /* Validate IRQL */
    #if DBG
    cmp cl, PCR[KPCR_IRQL]
    ja InvalidIrql
    #endif

    /* Save flags since we'll disable interrupts */
    pushf
    cli

    /* Disable interrupts and check if IRQL is below DISPATCH_LEVEL */
    cmp dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    jbe SkipMask

    /* Clear interrupt masks since there's a pending hardware interrupt */
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

SkipMask:

    /* Set the new IRQL and check if there's a pending software interrupt */
    mov PCR[KPCR_IRQL], ecx
    mov eax, PCR[KPCR_IRR]
    mov al, SWInterruptLookUpTable[eax]
    cmp al, cl
    ja DoCall3

    /* Restore interrupts and return */
    popf
    ret

#if DBG
InvalidIrql:
    /* Set HIGH_LEVEL */
    mov eax, PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], HIGH_LEVEL

    /* Bugcheck the system */
    push 3
    push 0
    push ecx
    push eax
    push IRQL_NOT_LESS_OR_EQUAL
    call _KeBugCheckEx@20
#endif

DoCall3:
    /* There is, call it */
    call SWInterruptHandlerTable[eax*4]

    /* Restore interrupts and return */
    popf
    ret
.endfunc

.globl @KfRaiseIrql@4
.func @KfRaiseIrql@4
_@KfRaiseIrql@4:
@KfRaiseIrql@4:

    /* Get the IRQL */
    movzx ecx, cl
    mov eax, PCR[KPCR_IRQL]

#if DBG
    /* Validate it */
    cmp eax, ecx
    ja InvalidKfRaise
#endif

    /* Check if it's in the software level */
    cmp cl, DISPATCH_LEVEL
    jbe SetIrql

    /* Save the current IRQL */
    mov edx, eax

    /* It's a hardware IRQL, so disable interrupts */
    pushf
    cli

    /* Set the new IRQL */
    mov PCR[KPCR_IRQL], ecx

    /* Mask the interrupts in the PIC */
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Restore interrupts and return old IRQL */
    popf
    mov eax, edx
    ret

SetIrql:
    /* Set the IRQL and return */
    mov PCR[KPCR_IRQL], ecx
    ret

#if DBG
InvalidKfRaise:
    /* Set to passive */
    mov dword ptr PCR[KPCR_IRQL], PASSIVE_LEVEL

    /* Bugcheck the system */
    push 9
    push 0
    push ecx
    push eax
    push IRQL_NOT_GREATER_OR_EQUAL
    call _KeBugCheckEx@20
#endif
.endfunc

.globl _HalpApcInterrupt
.func HalpApcInterrupt
TRAP_FIXUPS hapc_a, hapc_t, DoFixupV86, DoFixupAbios
_HalpApcInterrupt:

    /* Create fake interrupt stack */
    pop eax
    pushf
    push cs
    push eax

    /* Enter interrupt */
    INT_PROLOG hapc_a, hapc_t, DoPushFakeErrorCode
.endfunc

.globl _HalpApcInterrupt2ndEntry
.func HalpApcInterrupt2ndEntry
_HalpApcInterrupt2ndEntry:

    /* Save current IRQL and set to APC level */
    push PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], APC_LEVEL
    and dword ptr PCR[KPCR_IRR], ~(1 << APC_LEVEL)

    /* Enable interrupts and check if we came from User/V86 mode */
    sti
    mov eax, [ebp+KTRAP_FRAME_CS]
    and eax, MODE_MASK
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jz DeliverApc

    /* Set user mode delivery */
    or eax, UserMode

DeliverApc:

    /* Deliver the APCs */
    push ebp
    push 0
    push eax
    call _KiDeliverApc@12

    /* Disable interrupts and end it */
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp _Kei386EoiHelper@0
.endfunc

.globl _HalpDispatchInterrupt
.func HalpDispatchInterrupt
TRAP_FIXUPS hdpc_a, hdpc_t, DoFixupV86, DoFixupAbios
_HalpDispatchInterrupt:

    /* Create fake interrupt stack */
    pop eax
    pushf
    push cs
    push eax

    /* Enter interrupt */
    INT_PROLOG hdpc_a, hdpc_t, DoPushFakeErrorCode
.endfunc

.globl _HalpDispatchInterrupt2ndEntry
.func HalpDispatchInterrupt2ndEntry
_HalpDispatchInterrupt2ndEntry:

    /* Save current IRQL and set to DPC level */
    push PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    and dword ptr PCR[KPCR_IRR], ~(1 << DISPATCH_LEVEL)

    /* Enable interrupts and let the kernel handle this */
    sti
    call _KiDispatchInterrupt@0

    /* Disable interrupts and end it */
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp _Kei386EoiHelper@0
.endfunc

.globl _HalpEndSoftwareInterrupt@4
.func HalpEndSoftwareInterrupt@4
_HalpEndSoftwareInterrupt@4:

    /* Get the IRQL and check if we're in the software region */
    movzx ecx, byte ptr [esp+4]
    cmp dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    jbe SoftwareInt

    /* Set the right mask in the PIC for the hardware IRQ */
    mov eax, KiI8259MaskTable[ecx*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

SoftwareInt:
    /* Check if there are pending software interrupts */
    mov PCR[KPCR_IRQL], ecx
    mov eax, PCR[KPCR_IRR]
    mov al, SWInterruptLookUpTable[eax]
    cmp al, cl
    ja DoCall2
    ret 4

DoCall2:
    /* There are pending softwate interrupts, call their handlers */
    add esp, 8
    jmp SWInterruptHandlerTable2[eax*4]
.endfunc
