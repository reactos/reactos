
#include <internal/i386/segment.h>

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
	pushl	$0
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$1
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$2
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$3
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$4
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$5
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$6
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$7
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$8
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$9
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$10
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$11
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$12
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$13
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$14
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
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
	pushl	$15
	call	_KiInterruptDispatch
	popl	%eax
	popl	%eax
	popl	%eax
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	iret
