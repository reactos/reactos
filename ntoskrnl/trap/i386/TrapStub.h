#ifndef TRAP_STUB_FLAGS
#define TRAP_STUB_FLAGS 0
#endif

#ifndef TRAP_STUB_NAME
#error
#endif

#define TRAP_STUB_NAMEH0(x) x##Handler
#define TRAP_STUB_NAMEH1(x) TRAP_STUB_NAMEH0(x)
#define TRAP_STUB_NAMEH TRAP_STUB_NAMEH1(TRAP_STUB_NAME)

VOID _FASTCALL TRAP_STUB_NAMEH(KTRAP_FRAME *TrapFrame);

_NAKED VOID TRAP_STUB_NAME(VOID)
{
	_ASM_BEGIN
		// setup frame
#if (TRAP_STUB_FLAGS & TRAPF_SYSENTER)
		mov esp, ss:[KIP0PCRADDRESS + offset KPCR.TSS]
		mov esp, KTSS.Esp0[esp]
		// sub esp, dword ptr offset KTRAP_FRAME.V86Es
		sub esp, dword ptr offset KTRAP_FRAME.V86Es
#elif (TRAP_STUB_FLAGS & TRAPF_ERRORCODE)
		sub esp, offset KTRAP_FRAME.ErrCode
#elif (TRAP_STUB_FLAGS & TRAPF_SOFTWARE)
		sub esp, offset KTRAP_FRAME.HardwareEsp
		pop eax
#else
		sub esp, offset KTRAP_FRAME.Eip
#endif

		mov KTRAP_FRAME.Eax[esp], eax
		mov KTRAP_FRAME.Ecx[esp], ecx
		mov KTRAP_FRAME.Edx[esp], edx

#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVESEG)
		mov KTRAP_FRAME.SegDs[esp], ds
		mov KTRAP_FRAME.SegEs[esp], es
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVEFS)
		mov KTRAP_FRAME.SegFs[esp], fs
#endif
#if !(TRAP_STUB_FLAGS & TRAPF_NOLOADDS)
#ifndef TRAP_STUB_DS
#define TRAP_STUB_DS (KGDT_R3_DATA | RPL_MASK)
#endif
		mov ax, TRAP_STUB_DS
		mov ds, ax
		mov es, ax
#endif
#endif

#if (TRAP_STUB_FLAGS & TRAPF_SAVENOVOL)
		mov KTRAP_FRAME.Ebp[esp], ebp
		mov KTRAP_FRAME.Ebx[esp], ebx
		mov KTRAP_FRAME.Esi[esp], esi
		mov KTRAP_FRAME.Edi[esp], edi
#endif

#if (TRAP_STUB_FLAGS & TRAPF_VECTOR)
:scadr:
		mov edx, 0
		call edx
#endif
		// call handler
		mov ecx, esp
		call TRAP_STUB_NAMEH

	_ASM_END

		// asmcall(TRAP_STUB_NAMEH);
		// call TRAP_STUB_NAMEH
		// call HandlerName

	_ASM_BEGIN
		// return
#if (TRAP_STUB_FLAGS & TRAPF_SAVENOVOL)
		mov ebp, KTRAP_FRAME.Ebp[esp]
		mov ebx, KTRAP_FRAME.Ebx[esp]
		mov esi, KTRAP_FRAME.Esi[esp]
		mov edi, KTRAP_FRAME.Edi[esp]
#endif

#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVESEG)
		mov ds, KTRAP_FRAME.SegDs[esp]
		mov es, KTRAP_FRAME.SegEs[esp]
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVEFS)
		mov fs, KTRAP_FRAME.SegFs[esp]
#endif
#endif

		mov eax, KTRAP_FRAME.Eax[esp]
		mov ecx, KTRAP_FRAME.Ecx[esp]
		mov edx, KTRAP_FRAME.Edx[esp]
		iretd

	_ASM_END
}
