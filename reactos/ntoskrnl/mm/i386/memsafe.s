.globl _MmSafeCopyFromUser
.globl _MmSafeCopyFromUserEnd
.globl _MmSafeCopyToUser
.globl _MmSafeCopyToUserEnd

	/*
	 * NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, 
	 *                             ULONG NumberOfBytes)
	 */
_MmSafeCopyFromUser:
	pushl	%ebp
	movl	%esp,%ebp

	pushl   %esi
	pushl	%edi
	pushl	%ecx
	
	movl    8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx

	/*
	 * Default return code
	 */ 
	movl    $0,%eax
	
	/*
	 * This is really a synthetic instruction since if we incur a
	 * pagefault then eax will be set to an appropiate STATUS code
	 */ 
	rep movsb

	popl	%ecx
	popl	%edi
	popl	%esi

	ret

_MmSafeCopyFromUserEnd:

	/*
	 * NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, 
	 *			ULONG NumberOfBytes)
	 */ 
_MmSafeCopyToUser:
	pushl	%ebp
	movl	%esp,%ebp

	pushl   %esi
	pushl	%edi
	pushl	%ecx
	
	movl    8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx

	/*
	 * Default return code
	 */ 
	movl    $0,%eax
	
	/*
	 * This is really a synthetic instruction since if we incur a
	 * pagefault then eax will be set to an appropiate STATUS code
	 */ 
	rep movsb

	popl	%ecx
	popl	%edi
	popl	%esi

	ret

_MemSafeCopyToUser:
