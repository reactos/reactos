#ifndef _KeSystemCallReturn2
KeSystemCallReturn:
#else
KeSystemCallReturn2:
#endif

#if CHECKED
           /*  Bump Service Counter  */
#endif

           /*  Deallocate the kernel stack frame  */
           movl %ebp,%esp

#ifndef _KeSystemCallReturn2
KeReturnFromSystemCallWithHook:	
#endif
	   /* Call the post system call hook and deliver any pending APCs */
	   pushl %esp
	   pushl %eax
	   call _KiAfterSystemCallHook
	   addl $8,%esp

#ifndef _KeSystemCallReturn2
KeReturnFromSystemCall:
#endif
	
           /* Restore the user context */
	   /* Get a pointer to the current thread */
           movl %fs:0x124, %esi
	
           /* Restore the old trap frame pointer */
           movl 0x3c(%esp), %ebx
	   movl %ebx, KTHREAD_TRAP_FRAME(%esi)
	
	   /* Skip debug information and unsaved registers */
	   addl	$0x30, %esp
	   popl %gs
	   popl %es
	   popl %ds
	   popl %edx
	   popl %ecx
#ifndef _KeSystemCallReturn2
	   addl $0x4, %esp   /* Don't restore eax */
#else
           popl %eax
#endif

	   /* Restore the old previous mode */
	   popl %ebx
	   movb %bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

	   /* Restore the old exception handler list */
	   popl %ebx
	   movl %ebx, %fs:KPCR_EXCEPTION_LIST
	
	   popl %fs 
	   popl %edi
	   popl %esi
	   popl %ebx
	   popl %ebp
	   addl $0x4, %esp  /* Ignore error code */
		
           iret
