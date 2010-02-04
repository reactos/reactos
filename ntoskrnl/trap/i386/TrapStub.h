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

#define TRAP_STUB_NAMEH tokenpaste(TRAP_STUB_NAME, Handler)

#if (TRAP_STUB_FLAGS & TRAPF_INTERRUPT)
#define TRAP_STUB_PARAM2 tokenpaste(TRAP_STUB_NAME, Interrupt)
PKINTERRUPT TRAP_STUB_PARAM2;
#else
VOID _FASTCALL tokenpaste(TRAP_STUB_NAME, Handler)(KTRAP_FRAME *TrapFrame);
#endif

_NAKED VOID TRAP_STUB_NAME(VOID)
{
	_ASM_BEGIN
		// setup frame
#if (TRAP_STUB_FLAGS & TRAPF_FASTSYSCALL)
		mov esp, ss:[KIP0PCRADDRESS + offset KPCR.TSS]
		mov esp, KTSS.Esp0[esp]
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

		// call handler
#if (TRAP_STUB_FLAGS & TRAPF_INTERRUPT)
		mov edx, TRAP_STUB_PARAM2
		mov ecx, esp
		call PKINTERRUPT.DispatchAddress[edx]
#else
		mov ecx, esp
		call tokenpaste(TRAP_STUB_NAME, Handler)
#endif

		// restore regs
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

		// restore volatle regs and return
		mov eax, KTRAP_FRAME.Eax[esp]

#if (TRAP_STUB_FLAGS & TRAPF_FASTSYSCALL)
		mov ecx, KTRAP_FRAME.HardwareEsp[esp]
		mov edx, KTRAP_FRAME.Eip[esp]
		add esp, dword ptr offset KTRAP_FRAME.V86Es
		sti
		CpuSysExit
#endif
		mov ecx, KTRAP_FRAME.Ecx[esp]
		mov edx, KTRAP_FRAME.Edx[esp]

		add esp, KTRAP_FRAME_EIP
		iretd

	_ASM_END
}
