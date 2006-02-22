.globl @MmSafeReadPtr@4
.globl _MmSafeReadPtrStart
.globl _MmSafeReadPtrEnd

/*****************************************************************************/

	/*
	 * PVOID FASTCALL MmSafeReadPtr(PVOID Source)
	 */
@MmSafeReadPtr@4:
_MmSafeReadPtrStart:
	/*
	 * If we incur a pagefault, eax will be set NULL
	 */
	movl	(%ecx),%eax
_MmSafeReadPtrEnd:
	ret
