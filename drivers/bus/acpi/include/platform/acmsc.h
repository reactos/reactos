#ifndef __ACMSC_H__
#define __ACMSC_H__

#define COMPILER_DEPENDENT_UINT64 unsigned __int64

#if defined(_M_IX86)

#define ACPI_ASM_MACROS
#define causeinterrupt(level)
#define BREAKPOINT3
#define halt() { __asm { sti } __asm { hlt } }
#define wbinvd()

__forceinline void _ACPI_ACQUIRE_GLOBAL_LOCK(void * GLptr, unsigned char * Acq_)
{
	unsigned char Acq;

	__asm
	{
		mov ecx, [GLptr]

	L1:	mov eax, [ecx]
		mov edx, eax
		and edx, ecx
		bts edx, 1
		adc edx, 0
		lock cmpxchg [ecx], edx
		jne L1
		cmp dl, 3
		sbb eax, eax

		mov [Acq], al
	};

	*Acq_ = Acq;
}

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
	_ACPI_ACQUIRE_GLOBAL_LOCK((GLptr), (unsigned char *)&(Acq))

__forceinline void _ACPI_RELEASE_GLOBAL_LOCK(void * GLptr, unsigned char * Acq_)
{
	unsigned char Acq;

	__asm
	{
		mov ecx, [GLptr]

	L1:	mov eax, [ecx]
		mov edx, eax
		and edx, ecx
		lock cmpxchg [ecx], edx
		jnz L1
		and eax, 1

		mov [Acq], al
	};

	*Acq_ = Acq;
}

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
	_ACPI_RELEASE_GLOBAL_LOCK((GLptr), (unsigned char *)&(Acq))

#endif

#endif /* __ACMSC_H__ */
