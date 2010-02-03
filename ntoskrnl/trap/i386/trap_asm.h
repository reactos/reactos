#if 0
_INLINEF KiTrapStub(HandlerName, Flags)
{
	// alloc stack frame
	if (Flags & TRAPF_ERRORCODE)
		_ASM sub esp, KTRAP_FRAME.ErrCode
	else
		_ASM sub esp, KTRAP_FRAME.Eip
	// save volatile regs
	_ASM mov KTRAP_FRAME.Eax[esp], eax
	_ASM mov KTRAP_FRAME.Ecx[esp], ecx
	_ASM mov KTRAP_FRAME.Edx[esp], edx
	// save ds & es
	if (Flags & TRAPF_NOSAVEDSES)
	{
		_ASM mov KTRAP_FRAME.SegDs[esp], ds
		_ASM mov KTRAP_FRAME.SegEs[esp], es
	}
	// save fs
	if (Flags & TRAPF_NOSAVEFS)
		_ASM mov KTRAP_FRAME.SegFs[esp], fs
	// save non volatile regs
	if (Flags & TRAPF_SAVENOVOL)
	{
		_ASM mov KTRAP_FRAME.Esi[esp], ebp
		_ASM mov KTRAP_FRAME.Ebx[esp], ebx
		_ASM mov KTRAP_FRAME.Esi[esp], esi
		_ASM mov KTRAP_FRAME.Esi[esp], edi
	}

	// call handler
	_ASM mov ecx, esp
	_ASM call HandlerName

	// return
	if (Flags & TRAPF_NOSAVEDSES)
	{
		_ASM mov ds, KTRAP_FRAME.SegDs[esp]
		_ASM mov es, KTRAP_FRAME.SegEs[esp]
	}
	if (Flags & TRAPF_NOSAVEFS)
		_ASM mov fs, KTRAP_FRAME.SegFs[esp]
	if (Flags & TRAPF_SAVENOVOL)
	{
		_ASM mov KTRAP_FRAME.Esi[esp], ebp
		_ASM mov KTRAP_FRAME.Ebx[esp], ebx
		_ASM mov KTRAP_FRAME.Esi[esp], esi
		_ASM mov KTRAP_FRAME.Esi[esp], edi
	}
	_ASM mov eax, KTRAP_FRAME.Eax[esp]
	_ASM mov ecx, KTRAP_FRAME.Ecx[esp]
	_ASM mov edx, KTRAP_FRAME.Edx[esp]
	_ASM add esp, KTRAP_FRAME.Eip
	_ASM iretd
}
#endif

