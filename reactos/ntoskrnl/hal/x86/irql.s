	.file	"irql.c"
gcc2_compiled.:
___gnu_compiled_c:
.globl _CurrentIrql
.data
_CurrentIrql:
	.byte 19
.text
	.p2align 2
_pic_get_current_mask:
	pushl %ebp
	movl %esp,%ebp
/APP
	inb $33,%al
	outb %al,$0x80
	inb $161,%al
	outb %al,$0x80
/NO_APP
	leave
	ret
	.p2align 2
_pic_set_current_mask:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
/APP
	outb %al,$33
	outb %al,$0x80
/NO_APP
	shrl $8,%eax
/APP
	outb %al,$161
	outb %al,$0x80
/NO_APP
	leave
	ret
	.p2align 2
_switch_irql:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %ebx
	movb _CurrentIrql,%al
	cmpb $19,%al
	jne L62
	pushl $65535
	call _pic_set_current_mask
	jmp L61
	.align 2,0x90
L62:
	cmpb $2,%al
	jbe L71
	movl $0,-4(%ebp)
	movzbl _CurrentIrql,%eax
	leal -2(%eax),%edx
	cmpl $2,%edx
	jbe L65
	movl $16,%ebx
	leal -4(%ebp),%ecx
	.align 2,0x90
L67:
	movl %ebx,%eax
	subl %edx,%eax
/APP
	btsl %eax,(%ecx)
	sbbl %eax,%eax
/NO_APP
	decl %edx
	cmpl $2,%edx
	ja L67
L65:
	pushl -4(%ebp)
	call _pic_set_current_mask
/APP
	sti
	
/NO_APP
	jmp L61
	.align 2,0x90
L71:
	pushl $0
	call _pic_set_current_mask
/APP
	sti
	
/NO_APP
L61:
	movl -8(%ebp),%ebx
	leave
	ret
	.p2align 2
.globl _KeSetCurrentIrql
_KeSetCurrentIrql:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	movb %al,_CurrentIrql
	leave
	ret
	.p2align 2
.globl _KeGetCurrentIrql
_KeGetCurrentIrql:
	pushl %ebp
	movl %esp,%ebp
	movzbl _CurrentIrql,%eax
	leave
	ret
LC0:
	.ascii "irql.c\0"
LC1:
	.ascii "(%s:%d) \0"
LC2:
	.ascii "NewIrql %x CurrentIrql %x\12\0"
LC3:
	.ascii "Assertion NewIrql <= CurrentIrql failed at %s:%d\12\0"
	.p2align 2
.globl _KeLowerIrql
_KeLowerIrql:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	movb 8(%ebp),%bl
/APP
	cli
	
/NO_APP
	pushl $103
	pushl $LC0
	pushl $LC1
	call _printk
	movzbl _CurrentIrql,%eax
	pushl %eax
	movzbl %bl,%eax
	pushl %eax
	pushl $LC2
	call _printk
	addl $24,%esp
	cmpb %bl,_CurrentIrql
	jae L79
	pushl $104
	pushl $LC0
	pushl $LC3
	call _printk
	.align 2,0x90
L82:
	jmp L82
	.align 2,0x90
L79:
	movb %bl,_CurrentIrql
	call _switch_irql
	movl -4(%ebp),%ebx
	leave
	ret
LC4:
	.ascii "%s:%d\12\0"
LC5:
	.ascii "NewIrql %x OldIrql %x CurrentIrql %x\12\0"
	.p2align 2
.globl _KeRaiseIrql
_KeRaiseIrql:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	movl 12(%ebp),%esi
	movb 8(%ebp),%al
	cmpb %al,_CurrentIrql
	jbe L84
	pushl $122
	pushl $LC0
	pushl $LC4
	call _printk
	.align 2,0x90
L87:
	jmp L87
	.align 2,0x90
L84:
/APP
	cli
	
/NO_APP
	movzbl %al,%ebx
	pushl %ebx
	pushl $_CurrentIrql
	call _InterlockedExchange
	movb %al,(%esi)
	pushl $129
	pushl $LC0
	pushl $LC1
	call _printk
	movzbl _CurrentIrql,%eax
	pushl %eax
	movzbl (%esi),%eax
	pushl %eax
	pushl %ebx
	pushl $LC5
	call _printk
	addl $28,%esp
	call _switch_irql
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	leave
	ret
.comm ___ProcessHeap,4
