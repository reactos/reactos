#include <ddk/ntddk.h>


void MmSafeCopyToUserRestart();
void MmSafeCopyToUserUnsafeStart();
void MmSafeCopyFromUserUnsafeStart();
void MmSafeCopyFromUserRestart();

	/*
	 * NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, 
	 *                             ULONG NumberOfBytes)
	 */
__declspec(naked)
NTSTATUS MmSafeCopyFromUser(PVOID Dest, const VOID *Src, ULONG Count)
{
	__asm
	{
		push	ebp
		mov	ebp,esp

		push	esi
		push	edi
		push	ecx
		
		mov	edi, 8[ebp]
		mov	esi, 12[ebp]
		mov	ecx, 16[ebp]

		/*
		 * Default return code
		 */ 
		xor		eax,eax

		jmp		MmSafeCopyFromUserUnsafeStart
	}
}


__declspec(naked)
void MmSafeCopyFromUserUnsafeStart()
{
	__asm
	{
		/*
		 * This is really a synthetic instruction since if we incur a
		 * pagefault then eax will be set to an appropiate STATUS code
		 */
		cld 
		rep movsb

		jmp		MmSafeCopyFromUserRestart
	}
}

__declspec(naked)
void MmSafeCopyFromUserRestart()
{
	__asm
	{
		pop		ecx
		pop		edi
		pop		esi

		pop		ebp
		ret
	}
}

/*****************************************************************************/

	/*
	 * NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, 
	 *			ULONG NumberOfBytes)
	 */ 
NTSTATUS MmSafeCopyToUser(PVOID Dest, const VOID *Src, ULONG Count)
{
	__asm
	{
		push	ebp
		mov	esp,ebp

		push	esi
		push	edi
		push	ecx

		mov	edi, 8[ebp]
		mov	esi, 12[ebp]
		mov	ecx, 16[ebp]

		/*
		 * Default return code
		 */ 
		xor		eax,eax

		jmp		MmSafeCopyToUserUnsafeStart
	}
}

__declspec(naked)
void MmSafeCopyToUserUnsafeStart()
{
	__asm
	{
	/*
	 * This is really a synthetic instruction since if we incur a
	 * pagefault then eax will be set to an appropiate STATUS code
	 */
	cld 
	rep movsb
	jmp MmSafeCopyToUserRestart
	}
}

__declspec(naked)
void MmSafeCopyToUserRestart()
{
	__asm
	{
		pop		ecx
		pop		edi
		pop		esi
		
		pop		ebp
		ret
	}
}