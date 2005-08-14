#include <ndk/i386/segment.h>
#include <../hal/halx86/include/halirq.h>

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

_KiCommonInterrupt:
	cld
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	movl	$0xceafbeef,%eax
	pushl %eax
	movl	$KERNEL_DS,%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl %eax,%gs
	movl	$PCR_SELECTOR,%eax
	movl	%eax,%fs
	pushl %esp
	pushl %ebx
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

B16(3) B16(4) B16(5) B16(6)
B16(7) B16(8) B16(9) B16(A)
B16(B) B16(C) B16(D) B16(E)
B16(F)

#undef B
#undef B16

#else /* CONFIG_SMP */

.global _irq_handler_0
_irq_handler_0:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 0)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_1
_irq_handler_1:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 1)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_2
_irq_handler_2:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 2)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_3
_irq_handler_3:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 3)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_4
_irq_handler_4:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 4)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_5
_irq_handler_5:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 5)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_6
_irq_handler_6:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 6)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_7
_irq_handler_7:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 7)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_8
_irq_handler_8:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 8)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_9
_irq_handler_9:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 9)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_10
_irq_handler_10:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 10)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_11
_irq_handler_11:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 11)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_12
_irq_handler_12:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 12)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_13
_irq_handler_13:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 13)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_14
_irq_handler_14:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 14)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

.global _irq_handler_15
_irq_handler_15:
	cld
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	movl	$0xceafbeef,%eax
	pushl	%eax
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
	pushl	%esp
	pushl	$(IRQ_BASE + 15)
	call	_KiInterruptDispatch
	addl	$0xC, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret

#endif /* CONFIG_SMP */
