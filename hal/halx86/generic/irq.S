/*
 * FILE:            hal/halx86/generic/irq.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Software, System and Hardware IRQ Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

PICInitTable:

    /* Master PIC */
    .short 0x20                         /* Port */
    .byte 0x11                          /* Edge,, cascade, CAI 8, ICW4 */
    .byte PRIMARY_VECTOR_BASE           /* Base */
    .byte 4                             /* IRQ 4 connected to slave */
    .byte 1                             /* Non buffered, not nested, 8086 */

    /* Slave PIC */
    .short 0xA0                         /* Port */
    .byte 0x11                          /* Edge, cascade, CAI 8, ICW4 */
    .byte PRIMARY_VECTOR_BASE + 8       /* Base */
    .byte 2                             /* Slave ID: Slave 2 */
    .byte 1                             /* Non buffered, not nested, 8086 */

    /* End of initialization table */
    .short 0

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

HalpSysIntHandler:
.rept 7
    .long GenericIRQ                    /* IRQ 0-7 */
.endr
    .long IRQ7                          /* IRQ 7 */
.rept 7
    .long GenericIRQ                    /* IRQ 8-15 */
.endr
    .long IRQ15                         /* IRQ 15 */
.rept 20
    .long GenericIRQ                    /* IRQ 16-35 */
.endr
#if DBG
.rept 172
    .long InvalidIRQ                    /* IRQ 36-207 */
.endr
#endif

SoftIntByteTable:
    .byte PASSIVE_LEVEL                 /* IRR 0 */
    .byte PASSIVE_LEVEL                 /* IRR 1 */
    .byte APC_LEVEL                     /* IRR 2 */
    .byte APC_LEVEL                     /* IRR 3 */
    .byte DISPATCH_LEVEL                /* IRR 4 */
    .byte DISPATCH_LEVEL                /* IRR 5 */
    .byte DISPATCH_LEVEL                /* IRR 6 */
    .byte DISPATCH_LEVEL                /* IRR 7 */

SoftIntHandlerTable:
    .long _KiUnexpectedInterrupt        /* PASSIVE_LEVEL */
    .long _HalpApcInterrupt             /* APC_LEVEL */
    .long _HalpDispatchInterrupt        /* DISPATCH_LEVEL */

SoftIntHandlerTable2:
    .long _KiUnexpectedInterrupt        /* PASSIVE_LEVEL */
    .long _HalpApcInterrupt2ndEntry     /* APC_LEVEL */
    .long _HalpDispatchInterrupt2ndEntry /* DISPATCH_LEVEL */

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

/* FUNCTIONS *****************************************************************/

.globl _HalpInitPICs@0
.func HalpInitPICs@0
_HalpInitPICs@0:

    /* Save ESI and disable interrupts */
    push esi
    pushf
    cli

    /* Read the init table */
    lea esi, PICInitTable
    lodsw

InitLoop:

    /* Put the port in EDX */
    movzx edx, ax

    /* Initialize the PIC, using a delay for each command */
    outsb
    jmp $+2
    inc edx
    outsb
    jmp $+2
    outsb
    jmp $+2
    outsb
    jmp $+2

    /* Mask all interrupts */
    mov al, 0xFF
    out dx, al

    /* Check if we're done, otherwise initialize next PIC */
    lodsw
    cmp ax, 0
    jnz InitLoop

    /* Restore interrupts and return */
    popf
    pop esi
    ret
.endfunc

.globl @HalClearSoftwareInterrupt@4
.func @HalClearSoftwareInterrupt@4, @HalClearSoftwareInterrupt@4
@HalClearSoftwareInterrupt@4:

    /* Get IRR mask */
    mov eax, 1
    shl eax, cl
    not eax

    /* Set IRR */
    and PCR[KPCR_IRR], eax
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
    mov dl, SoftIntByteTable[eax]
    cmp dl, cl
    jbe AfterCall

    /* Call the pending interrupt */
    call SoftIntHandlerTable[edx*4]

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

    /* Get the current PCI Edge/Level control registers */
    mov edx, 0x4D1
    in al, dx
    shl ax, 8
    mov edx, 0x4D0
    in al, dx
    mov dx, 1
    shl dx, cl
    and dx, 0xDEF8

    /* Check if this is a latched interrupt */
    cmp dword ptr [esp+12], 0
    jnz Latched

    /* Use OR for edge interrupt */
    or ax, dx
    jmp AfterMask
Latched:

    /* Mask it out for level interrupt */
    not dx
    and ax, dx

AfterMask:

    /* Set the PCI Edge/Level control registers */
    mov edx, 0x4D0
    out dx, al
    shr ax, 8
    mov edx, 0x4D1
    out dx, al

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
    movzx ebx, byte ptr [esp+8]
    sub ebx, PRIMARY_VECTOR_BASE
    jmp HalpSysIntHandler[ebx*4]

IRQ15:
    /* This is IRQ 15, check if it's spurious */
    mov al, 0xB
    out 0xA0, al
    jmp $+2
    in al, 0xA0
    test al, 0x80
    jnz GenericIRQ

    /* Cascaded interrupt... dismiss it and return FALSE */
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

GenericIRQ:
    /* Return the current IRQL */
    mov eax, [esp+12]
    mov ecx, PCR[KPCR_IRQL]
    mov [eax], cl

    /* Set the new IRQL */
    movzx eax, byte ptr [esp+4]
    mov PCR[KPCR_IRQL], eax

    /* Set IRQ mask in the PIC */
    mov eax, KiI8259MaskTable[eax*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Check to which PIC the EOI was sent */
    mov eax, ebx
    cmp eax, 8
    jnb Pic1

    /* Write mask to master PIC */
    or al, 0x60
    out 0x20, al
    jmp DoneBegin

Pic1:
    /* Write mask to slave PIC */
    mov al, 0x20
    out 0xA0, al
    mov al, 0x62
    out 0x20, al

DoneBegin:
    /* Enable interrupts and return TRUE */
    in al, 0x21
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

.globl _HalEndSystemInterrupt@8
.func HalEndSystemInterrupt@8
_HalEndSystemInterrupt@8:

    /* Get the IRQL and check if it's a software interrupt */
    movzx ecx, byte ptr [esp+4]
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
    mov al, SoftIntByteTable[eax]
    cmp al, cl
    ja DoCall
    ret 8

DoCall:

    /* There are pending software interrupts, call their handlers */
    add esp, 12
    jmp SoftIntHandlerTable2[eax*4]
.endfunc

.globl @KfLowerIrql@4
.func @KfLowerIrql@4
_@KfLowerIrql@4:
@KfLowerIrql@4:

    /* Save flags since we'll disable interrupts */
    pushf

    /* Validate IRQL */
    movzx ecx, cl
#if DBG
    cmp cl, PCR[KPCR_IRQL]
    ja InvalidIrql
#endif

    /* Disable interrupts and check if IRQL is below DISPATCH_LEVEL */
    cmp dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    cli
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
    mov al, SoftIntByteTable[eax]
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
    call SoftIntHandlerTable[eax*4]

    /* Restore interrupts and return */
    popf
    ret
.endfunc

.globl @KfRaiseIrql@4
.func @KfRaiseIrql@4
_@KfRaiseIrql@4:
@KfRaiseIrql@4:

    /* Get the IRQL  */
    mov eax, PCR[KPCR_IRQL]
    movzx ecx, cl

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
    mov PCR[KPCR_IRQL], cl

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

.globl _KeGetCurrentIrql@0
.func KeGetCurrentIrql@0
_KeGetCurrentIrql@0:

    /* Return the IRQL */
    mov eax, PCR[KPCR_IRQL]
    ret
.endfunc

.globl _KeRaiseIrqlToDpcLevel@0
.func KeRaiseIrqlToDpcLevel@0
_KeRaiseIrqlToDpcLevel@0:

    /* Get the current IRQL */
    mov eax, PCR[KPCR_IRQL]

    /* Set DISPATCH_LEVEL */
    mov dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL

#if DBG
    /* Make sure we were not higher then dispatch */
    cmp eax, DISPATCH_LEVEL
    ja InvalidRaise
#endif
    ret

#if DBG
InvalidRaise:
    /* Bugcheck the system */
    push 1
    push 0
    push DISPATCH_LEVEL
    push eax
    push IRQL_NOT_GREATER_OR_EQUAL
    call _KeBugCheckEx@20
#endif
.endfunc

.globl _KeRaiseIrqlToSynchLevel@0
.func KeRaiseIrqlToSynchLevel@0
_KeRaiseIrqlToSynchLevel@0:

    /* Disable interrupts */
    pushf
    cli

    /* Mask out interrupts */
    mov eax, KiI8259MaskTable[DISPATCH_LEVEL*4]
    or eax, PCR[KPCR_IDR]
    out 0x21, al
    shr eax, 8
    out 0xA1, al

    /* Return the old IRQL, enable interrupts and set to DISPATCH */
    mov eax, PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    popf

#if DBG
    /* Validate raise */
    cmp eax, DISPATCH_LEVEL
    ja InvalidSyRaise
#endif

    /* Return */
    ret

#if DBG
InvalidSyRaise:
    /* Bugcheck the system */
    push 2
    push 0
    push DISPATCH_LEVEL
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
    mov al, SoftIntByteTable[eax]
    cmp al, cl
    ja DoCall2
    ret 4

DoCall2:
    /* There are pending softwate interrupts, call their handlers */
    add esp, 8
    jmp SoftIntHandlerTable2[eax*4]
.endfunc
