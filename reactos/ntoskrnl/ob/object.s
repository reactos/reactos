	.file	"object.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16
.globl NtSetInformationObject
	.type	 NtSetInformationObject,@function
NtSetInformationObject:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	movl 8(%ebp),%ebx
	movl 12(%ebp),%ecx
	movl 16(%ebp),%edx
	movl 20(%ebp),%eax
	pushl %eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	call ZwSetInformationObject
	movl -4(%ebp),%ebx
	movl %ebp,%esp
	popl %ebp
	ret $16
.Lfe1:
	.size	 NtSetInformationObject,.Lfe1-NtSetInformationObject
.section	.rodata
.LC0:
	.string	"object.c"
.LC1:
	.string	"ZwSetInformationObject"
.LC2:
	.string	"%s at %s:%d is unimplemented, have a nice day\n"
.text
	.align 16
.globl ZwSetInformationObject
	.type	 ZwSetInformationObject,@function
ZwSetInformationObject:
	pushl %ebp
	movl %esp,%ebp
	pushl $38
	pushl $.LC0
	pushl $.LC1
	pushl $.LC2
	call DbgPrint
	.align 4
.L17:
	jmp .L17
	.align 16
.Lfe2:
	.size	 ZwSetInformationObject,.Lfe2-ZwSetInformationObject
	.align 16
.globl NtQueryObject
	.type	 NtQueryObject,@function
NtQueryObject:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%esi
	movl 12(%ebp),%ebx
	movl 16(%ebp),%ecx
	movl 20(%ebp),%edx
	movl 24(%ebp),%eax
	pushl %eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %esi
	call ZwQueryObject
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	movl %ebp,%esp
	popl %ebp
	ret $20
.Lfe3:
	.size	 NtQueryObject,.Lfe3-NtQueryObject
.section	.rodata
.LC3:
	.string	"ZwQueryObject"
.text
	.align 16
.globl ZwQueryObject
	.type	 ZwQueryObject,@function
ZwQueryObject:
	pushl %ebp
	movl %esp,%ebp
	pushl $60
	pushl $.LC0
	pushl $.LC3
	pushl $.LC2
	call DbgPrint
	.align 4
.L26:
	jmp .L26
	.align 16
.Lfe4:
	.size	 ZwQueryObject,.Lfe4-ZwQueryObject
	.align 16
.globl NtMakeTemporaryObject
	.type	 NtMakeTemporaryObject,@function
NtMakeTemporaryObject:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	pushl %eax
	call ZwMakeTemporaryObject
	movl %ebp,%esp
	popl %ebp
	ret $4
.Lfe5:
	.size	 NtMakeTemporaryObject,.Lfe5-NtMakeTemporaryObject
	.align 16
.globl ZwMakeTemporaryObject
	.type	 ZwMakeTemporaryObject,@function
ZwMakeTemporaryObject:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	movl 8(%ebp),%edx
	pushl $0
	leal -4(%ebp),%eax
	pushl %eax
	pushl $0
	pushl $0
	pushl $0
	pushl %edx
	call ObReferenceObjectByHandle
	addl $24,%esp
	testl %eax,%eax
	jne .L30
	movl -4(%ebp),%eax
	movb $0,-12(%eax)
	movl -4(%ebp),%eax
	pushl %eax
	call ObDereferenceObject
	xorl %eax,%eax
	movl %ebp,%esp
	popl %ebp
	ret $4
	.align 16
.L30:
	movl %ebp,%esp
	popl %ebp
	ret $4
.Lfe6:
	.size	 ZwMakeTemporaryObject,.Lfe6-ZwMakeTemporaryObject
	.align 16
.globl ObGenericCreateObject
	.type	 ObGenericCreateObject,@function
ObGenericCreateObject:
	pushl %ebp
	movl %esp,%ebp
	subl $12,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 16(%ebp),%edi
	movl 20(%ebp),%ecx
	movl 28(%ecx),%eax
	addl $36,%eax
	pushl %eax
	pushl $0
	call ExAllocatePool
	movl %eax,%esi
	addl $8,%esp
	testl %esi,%esi
	je .L46
	testl %edi,%edi
	jne .L35
	pushl %esi
	pushl $0
	movl 20(%ebp),%ecx
	pushl %ecx
	call ObInitializeObjectHeader
	addl $12,%esp
	jmp .L47
	.align 16
.L35:
	movl 8(%edi),%ecx
	movw (%ecx),%dx
	sall $16,%edx
	movzwl -8(%ebp),%eax
	orl %edx,%eax
	movl %eax,-8(%ebp)
	movzwl (%ecx),%eax
	leal 2(,%eax,2),%eax
	pushl %eax
	pushl $0
	call ExAllocatePool
	movl %eax,-4(%ebp)
	addl $8,%esp
	testl %eax,%eax
	jne .L39
.L46:
	xorl %eax,%eax
	jmp .L45
	.align 16
.L39:
	movl 8(%edi),%eax
	pushl %eax
	leal -8(%ebp),%eax
	pushl %eax
	call RtlCopyUnicodeString
	pushl $92
	movl -4(%ebp),%eax
	pushl %eax
	call wcsrchr
	movl %eax,%ebx
	addl $16,%esp
	testl %ebx,%ebx
	jne .L40
	movl -4(%ebp),%ebx
	xorl %edx,%edx
	jmp .L41
	.align 16
.L40:
	movl -4(%ebp),%edx
	movw $0,(%ebx)
	addl $2,%ebx
.L41:
	leal -12(%ebp),%eax
	pushl %eax
	leal 28(%esi),%eax
	pushl %eax
	pushl %edx
	movl 4(%edi),%eax
	pushl %eax
	call ObLookupObject
	pushl %esi
	pushl %ebx
	movl 20(%ebp),%ecx
	pushl %ecx
	call ObInitializeObjectHeader
	pushl %esi
	movl 28(%esi),%eax
	pushl %eax
	call ObCreateEntry
	addl $36,%esp
.L47:
	cmpl $0,8(%ebp)
	je .L42
	pushl $0
	movl 12(%ebp),%ecx
	pushl %ecx
	leal 36(%esi),%eax
	pushl %eax
	call KeGetCurrentProcess
	pushl %eax
	call ObInsertHandle
	movl 8(%ebp),%ecx
	movl %eax,(%ecx)
.L42:
	leal 36(%esi),%eax
.L45:
	leal -24(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe7:
	.size	 ObGenericCreateObject,.Lfe7-ObGenericCreateObject
	.align 16
.globl ObInitializeObjectHeader
	.type	 ObInitializeObjectHeader,@function
ObInitializeObjectHeader:
	pushl %ebp
	movl %esp,%ebp
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%eax
	movl 12(%ebp),%esi
	movl 16(%ebp),%ebx
	movl $0,20(%ebx)
	movl $0,16(%ebx)
	movl %eax,32(%ebx)
	movb $0,24(%ebx)
	testl %esi,%esi
	jne .L49
	movw $0,(%ebx)
	movl $0,4(%ebx)
	jmp .L50
	.align 16
.L49:
	pushl %esi
	call wstrlen
	movw %ax,2(%ebx)
	andl $65535,%eax
	leal 2(,%eax,2),%eax
	pushl %eax
	pushl $0
	call ExAllocatePool
	movl %eax,4(%ebx)
	pushl %esi
	pushl %ebx
	call RtlInitUnicodeString
.L50:
	leal -8(%ebp),%esp
	popl %ebx
	popl %esi
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe8:
	.size	 ObInitializeObjectHeader,.Lfe8-ObInitializeObjectHeader
	.align 16
.globl ObReferenceObjectByPointer
	.type	 ObReferenceObjectByPointer,@function
ObReferenceObjectByPointer:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	incl -20(%eax)
	xorl %eax,%eax
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe9:
	.size	 ObReferenceObjectByPointer,.Lfe9-ObReferenceObjectByPointer
	.align 16
.globl ObPerformRetentionChecks
	.type	 ObPerformRetentionChecks,@function
ObPerformRetentionChecks:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	movl 8(%ebp),%ebx
	cmpl $0,16(%ebx)
	jne .L54
	cmpl $0,20(%ebx)
	jne .L54
	cmpb $0,24(%ebx)
	jne .L54
	pushl %ebx
	call ObRemoveEntry
	pushl %ebx
	call ExFreePool
.L54:
	xorl %eax,%eax
	movl -4(%ebp),%ebx
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe10:
	.size	 ObPerformRetentionChecks,.Lfe10-ObPerformRetentionChecks
	.align 16
.globl ObDereferenceObject
	.type	 ObDereferenceObject,@function
ObDereferenceObject:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	leal -36(%eax),%edx
	decl -20(%eax)
	pushl %edx
	call ObPerformRetentionChecks
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe11:
	.size	 ObDereferenceObject,.Lfe11-ObDereferenceObject
	.align 16
.globl NtClose
	.type	 NtClose,@function
NtClose:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	pushl %eax
	call ZwClose
	movl %ebp,%esp
	popl %ebp
	ret $4
.Lfe12:
	.size	 NtClose,.Lfe12-NtClose
	.align 16
.globl ZwClose
	.type	 ZwClose,@function
ZwClose:
	pushl %ebp
	movl %esp,%ebp
	movl 8(%ebp),%eax
	pushl %eax
	call KeGetCurrentProcess
	pushl %eax
	call ObTranslateHandle
	movl %eax,%edx
	addl $8,%esp
	testl %edx,%edx
	je .L59
	movl (%edx),%eax
	movl $0,(%edx)
	leal -36(%eax),%edx
	decl -16(%eax)
	pushl %edx
	call ObPerformRetentionChecks
	xorl %eax,%eax
	movl %ebp,%esp
	popl %ebp
	ret $4
	.align 16
.L59:
	movl $-1073741816,%eax
	movl %ebp,%esp
	popl %ebp
	ret $4
.Lfe13:
	.size	 ZwClose,.Lfe13-ZwClose
	.align 16
.globl ObReferenceObjectByHandle
	.type	 ObReferenceObjectByHandle,@function
ObReferenceObjectByHandle:
	pushl %ebp
	movl %esp,%ebp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%eax
	movl 16(%ebp),%ebx
	movl 24(%ebp),%esi
	pushl %eax
	call KeGetCurrentProcess
	pushl %eax
	call ObTranslateHandle
	testl %eax,%eax
	je .L64
	movl (%eax),%edx
	testl %edx,%edx
	jne .L63
.L64:
	movl $-1073741816,%eax
	jmp .L68
	.align 16
.L63:
	leal -36(%edx),%ecx
	testl %ebx,%ebx
	je .L66
	cmpl %ebx,-4(%edx)
	je .L66
	movl $-2147483605,%eax
	jmp .L68
	.align 16
.L66:
	movl 12(%ebp),%edi
	testl %edi,4(%eax)
	jne .L67
	incl 16(%ecx)
	movl (%eax),%eax
	movl %eax,(%esi)
	xorl %eax,%eax
	jmp .L68
	.align 16
.L67:
	movl $-2147483557,%eax
.L68:
	leal -12(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe14:
	.size	 ObReferenceObjectByHandle,.Lfe14-ObReferenceObjectByHandle
	.ident	"GCC: (GNU) 2.7.2.3"
