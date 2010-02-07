#ifndef TRAP_STUB_FLAGS
#define TRAP_STUB_FLAGS 0
#endif

#ifndef TRAP_STUB_NAME
#error
#endif

#ifndef TRAP_STUB_DS
#define TRAP_STUB_DS KGDT_R3_DATA | RPL_MASK
#endif

#ifndef TRAP_STUB_FS
#define TRAP_STUB_FS KGDT_R0_PCR
#endif

// #define TRAP_STUB_NAMEH tokenpaste(TRAP_STUB_NAME, Handler)

#if (TRAP_STUB_FLAGS & TRAPF_INTERRUPT)
VOID _FASTCALL tokenpaste(TRAP_STUB_NAME, Handler)(KTRAP_FRAME *TrapFrame, PKINTERRUPT Interrupt);
#else
VOID _FASTCALL tokenpaste(TRAP_STUB_NAME, Handler)(KTRAP_FRAME *TrapFrame);
#endif
VOID _CDECL TRAP_STUB_NAME(VOID);

_NAKED VOID _CDECL TRAP_STUB_NAME(VOID)
{
	_ASM_BEGIN

		// setup frame
#if (TRAP_STUB_FLAGS & TRAPF_FASTSYSCALL)
		mov esp, ss:[KIP0PCRADDRESS + offset KPCR.TSS]
		mov esp, KTSS.Esp0[esp]
		sub esp, dword ptr offset KTRAP_FRAME.V86Es
#elif (TRAP_STUB_FLAGS & TRAPF_INTERRUPT)
		// the primary stub (trap_m.h) pushes a pointer to KINTERRUPT
		sub esp, offset KTRAP_FRAME.ErrCode
#elif (TRAP_STUB_FLAGS & TRAPF_ERRORCODE)
		sub esp, offset KTRAP_FRAME.ErrCode
#elif (TRAP_STUB_FLAGS & TRAPF_SOFTWARE)
		sub esp, offset KTRAP_FRAME.HardwareEsp
		pop eax
#else
		sub esp, offset KTRAP_FRAME.Eip
#endif

		// save volatiles
		mov KTRAP_FRAME.Edx[esp], edx
		mov KTRAP_FRAME.Ecx[esp], ecx
		mov KTRAP_FRAME.Eax[esp], eax

		// !!! only for testing we are always saving novol
// #if (TRAP_STUB_FLAGS & TRAPF_SAVENOVOL)
		mov KTRAP_FRAME.Edi[esp], edi
		mov KTRAP_FRAME.Esi[esp], esi
		mov KTRAP_FRAME.Ebx[esp], ebx
		mov KTRAP_FRAME.Ebp[esp], ebp
// #endif

		// save & load segs
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVESEG)
#if !(TRAP_STUB_FLAGS & TRAPF_NOLOADDS)
		mov ax, TRAP_STUB_DS
#endif
#if (TRAP_STUB_FLAGS & TRAPF_LOADFS)
		mov dx, TRAP_STUB_FS
#endif
		// gs most probably does not need save/restore,
		// but we're playing safe for now
		mov KTRAP_FRAME.SegGs[esp], gs
		mov KTRAP_FRAME.SegEs[esp], es
		mov KTRAP_FRAME.SegDs[esp], ds

#if !(TRAP_STUB_FLAGS & TRAPF_NOLOADDS)
		mov es, ax
		mov ds, ax
#endif
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVEFS)
		mov KTRAP_FRAME.SegFs[esp], fs
#if (TRAP_STUB_FLAGS & TRAPF_LOADFS)
		mov fs, dx
#endif
#endif	// #if !(TRAP_STUB_FLAGS & TRAPF_NOSAVEFS)
#endif	// #if !(TRAP_STUB_FLAGS & TRAPF_NOSAVESEG)

		// call handler
#if (TRAP_STUB_FLAGS & TRAPF_INTERRUPT)
		mov edx, KTRAP_FRAME.ErrCode[esp]
		mov ecx, esp
		call tokenpaste(TRAP_STUB_NAME, Handler)
		// call KINTERRUPT.DispatchAddress[edx]
#else
		mov ecx, esp
		call tokenpaste(TRAP_STUB_NAME, Handler)
#endif

		// restore seg regs
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVESEG)
		mov es, KTRAP_FRAME.SegGs[esp]
		mov es, KTRAP_FRAME.SegEs[esp]
		mov ds, KTRAP_FRAME.SegDs[esp]
#if !(TRAP_STUB_FLAGS & TRAPF_NOSAVEFS)
		mov fs, KTRAP_FRAME.SegFs[esp]
#endif
#endif

// !!! only for testing we're always saving novol
// #if (TRAP_STUB_FLAGS & TRAPF_SAVENOVOL)
		mov edi, KTRAP_FRAME.Edi[esp]
		mov esi, KTRAP_FRAME.Esi[esp]
		mov ebx, KTRAP_FRAME.Ebx[esp]
		mov ebp, KTRAP_FRAME.Ebp[esp]
// #endif

		// restore volatile regs and return
#if (TRAP_STUB_FLAGS & TRAPF_FASTSYSCALL)
		mov eax, KTRAP_FRAME.Eax[esp]
		mov ecx, KTRAP_FRAME.HardwareEsp[esp]
		mov edx, KTRAP_FRAME.Eip[esp]
		add esp, dword ptr offset KTRAP_FRAME.V86Es
		sti
		CpuSysExit
#else
		mov edx, KTRAP_FRAME.Edx[esp]
		mov ecx, KTRAP_FRAME.Ecx[esp]
		mov eax, KTRAP_FRAME.Eax[esp]
		add esp, offset KTRAP_FRAME.Eip
		iretd
#endif

	_ASM_END
}
