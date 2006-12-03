#include <ndk/asm.h>
#include <../hal/halx86/include/halirq.h>

_KiCommonInterrupt:
	cld
	pushl 	%ds
	pushl 	%es
	pushl 	%fs
	pushl 	%gs
	pushl	$0xceafbeef

    /* Load DS/ES (with override) */
    .intel_syntax noprefix
    mov eax, KGDT_R3_DATA + RPL_MASK
    .byte 0x66
    mov ds, ax
    .byte 0x66
    mov es, ax

    /* Clear gs */
    xor eax, eax
    .byte 0x66
    mov gs, ax
    .att_syntax
	movl	$KGDT_R0_PCR,%eax
	movl	%eax,%fs
	pushl 	%esp
	pushl 	%ebx
	call	_KiInterruptDispatch3
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret


#ifdef CONFIG_SMP

#define BUILD_INTERRUPT_HANDLER(intnum) \
  .global _KiUnexpectedInterrupt##intnum; \
  _KiUnexpectedInterrupt##intnum:; \
  pusha; \
  movl $0x##intnum, %ebx; \
  jmp _KiCommonInterrupt;

/* Interrupt handlers and declarations */

#define B(x,y) \
  BUILD_INTERRUPT_HANDLER(x##y)

#define B16(x) \
  B(x,0) B(x,1) B(x,2) B(x,3) \
  B(x,4) B(x,5) B(x,6) B(x,7) \
  B(x,8) B(x,9) B(x,A) B(x,B) \
  B(x,C) B(x,D) B(x,E) B(x,F)

B16(3) B16(4) B16(5) B16(6)
B16(7) B16(8) B16(9) B16(A)
B16(B) B16(C) B16(D) B16(E)
B16(F)

#undef B
#undef B16
#undef BUILD_INTERRUPT_HANDLER

#else /* CONFIG_SMP */

#define BUILD_INTERRUPT_HANDLER(intnum) \
  .global _irq_handler_##intnum; \
  _irq_handler_##intnum:; \
  pusha; \
  movl $(##intnum + IRQ_BASE), %ebx; \
  jmp _KiCommonInterrupt;

/* Interrupt handlers and declarations */

#define B(x) \
  BUILD_INTERRUPT_HANDLER(x)

B(0) B(1) B(2) B(3)
B(4) B(5) B(6) B(7)
B(8) B(9) B(10) B(11)
B(12) B(13) B(14) B(15)

#undef B
#undef BUILD_INTERRUPT_HANDLER

#endif /* CONFIG_SMP */

.intel_syntax noprefix
.globl _KiUnexpectedInterrupt@0
_KiUnexpectedInterrupt@0:

    /* Bugcheck with invalid interrupt code */
    push 0x12
    call _KeBugCheck@4

