/* $Id: sysinfo.c,v 1.6 2003/01/15 21:24:35 chorns Exp $
 *
 * reactos/lib/kernel32/misc/sysinfo.c
 *
 */
#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


#define PV_NT351 0x00030033

VOID
STDCALL
GetSystemInfo (
	LPSYSTEM_INFO	Si
	)
{
	SYSTEM_BASIC_INFORMATION	Sbi;
	SYSTEM_PROCESSOR_INFORMATION	Spi;
	DWORD				ProcessVersion;
	NTSTATUS			Status;

	RtlZeroMemory (Si, sizeof (SYSTEM_INFO));
	Status = NtQuerySystemInformation (
			SystemBasicInformation, /* 0 */
			& Sbi,
			sizeof Sbi, /* 44 */
			0
			);
	if (STATUS_SUCCESS != Status)
	{
		SetLastErrorByStatus (Status);
		return;
	}
	Status = NtQuerySystemInformation (
			SystemProcessorInformation, /* 1 */
			& Spi,
			sizeof Spi, /* 12 */
			0
			);
	if (STATUS_SUCCESS != Status)
	{
		SetLastErrorByStatus (Status);
		return;
	}
	/*
	 *	PROCESSOR_ARCHITECTURE_INTEL 0
	 *	PROCESSOR_ARCHITECTURE_MIPS  1
	 *	PROCESSOR_ARCHITECTURE_ALPHA 2
	 *	PROCESSOR_ARCHITECTURE_PPC   3
	 *	PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF
	 */
	Si->u.s.wProcessorArchitecture	= Spi.ProcessorArchitecture;
	/* For future use: always zero */
	Si->u.s.wReserved		= 0;
	Si->dwPageSize			= Sbi.PageSize;
	Si->lpMinimumApplicationAddress	= (PVOID)Sbi.MinimumUserModeAddress;
	Si->lpMaximumApplicationAddress	= (PVOID)Sbi.MaximumUserModeAddress;
	Si->dwActiveProcessorMask	= Sbi.ActiveProcessorsAffinityMask;
	Si->dwNumberOfProcessors	= Sbi.NumberOfProcessors;
	/*
	 * Compatibility (no longer relevant):
	 *	PROCESSOR_INTEL_386	386
	 *	PROCESSOR_INTEL_486	486
	 *	PROCESSOR_INTEL_PENTIUM	586
	 *	PROCESSOR_MIPS_R4000	4000
	 *	PROCESSOR_ALPHA_21064	21064
	 */
	switch (Spi.ProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		switch (Spi.ProcessorLevel)
		{
		case 3:
			Si->dwProcessorType = PROCESSOR_INTEL_386;
			break;
		case 4:
			Si->dwProcessorType = PROCESSOR_INTEL_486;
			break;
		case 5:
			Si->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
			break;
		default:
			/* FIXME: P2, P3, P4...? */
			Si->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
		}
		break;
		
	case PROCESSOR_ARCHITECTURE_MIPS:
		Si->dwProcessorType = PROCESSOR_MIPS_R4000;
		break;
		
	case PROCESSOR_ARCHITECTURE_ALPHA:
		Si->dwProcessorType = PROCESSOR_ALPHA_21064;
		break;
		
	case PROCESSOR_ARCHITECTURE_PPC:
		Si->dwProcessorType = -1; /* FIXME: what value? */
		break;
		
	}
	/* Once hardcoded to 64kb */
	Si->dwAllocationGranularity	= Sbi.AllocationGranularity;
	/* */
	Si->wProcessorLevel		= Spi.ProcessorLevel;
	Si->wProcessorRevision		= Spi.ProcessorRevision;
	/*
	 * Get the version of Windows on which
	 * the process expects to run.
	 */
	ProcessVersion = GetProcessVersion (0); /* current process */
	 /* In NT 3.1 and 3.5 these fields were always zero. */
	if (PV_NT351 > ProcessVersion)
	{
		Si->wProcessorLevel = 0;
		Si->wProcessorRevision = 0;
	}
}


/* EOF */
