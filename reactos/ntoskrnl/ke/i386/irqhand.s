
#include <internal/i386/segment.h>

.global _irq_handler_0
_irq_handler_0:
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs
        inb	$0x21,%al
        orb	$1<<0,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$0
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret

.global _irq_handler_1
_irq_handler_1: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<1,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$1
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_2
_irq_handler_2: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<2,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$2
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_3
_irq_handler_3: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<3,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$3
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_4
_irq_handler_4: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<4,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$4
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_5
_irq_handler_5: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<5,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$5
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_6
_irq_handler_6: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<6,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$6
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_7
_irq_handler_7: 
	pusha
        pushl	%ds
        pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0x21,%al
        orb	$1<<7,%al
        outb	%al,$0x21
        pushl	%esp
        pushl	$7
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
		
.global _irq_handler_8
_irq_handler_8:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(8-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$8
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret

.global _irq_handler_9
_irq_handler_9:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(9-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$9
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_10
_irq_handler_10:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(10-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$10
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_11
_irq_handler_11:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(11-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$11
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_12
_irq_handler_12:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(12-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$12
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_13
_irq_handler_13:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(13-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$13
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_14
_irq_handler_14:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(14-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$14
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
.global _irq_handler_15
_irq_handler_15:
        pusha
	pushl	%ds
	pushl	%es
        pushl	%fs
        movl	$0xceafbeef,%eax
        pushl	%eax
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs	
        inb	$0xa1,%al
        orb	$1<<(15-8),%al
        outb    %al,$0xa1
        pushl	%esp
        pushl	$15
        call	_KiInterruptDispatch
        popl	%eax
        popl	%eax
        popl	%eax
        popl	%fs
        popl	%es
        popl	%ds
        popa
        iret
	
	