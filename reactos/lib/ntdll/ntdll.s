	.file	"ntdll.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16
.globl NtAlertThread
	.type	 NtAlertThread,@function
NtAlertThread:
	subl $4,%esp
	movb 8(%esp),%al
	movb %al,3(%esp)
	leal 3(%esp),%edx
	movl $4,%eax
#APP
	int $0x2e
	
#NO_APP
	addl $4,%esp
	ret
.Lfe1:
	.size	 NtAlertThread,.Lfe1-NtAlertThread
	.ident	"GCC: (GNU) 2.7.2.3"
