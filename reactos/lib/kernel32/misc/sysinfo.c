/* $Id: sysinfo.c,v 1.4 2002/09/07 15:12:27 chorns Exp $
 *
 * reactos/lib/kernel32/misc/sysinfo.c
 *
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


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
	Si->wProcessorArchitecture	= Spi.ProcessorArchitecture;
	/* For future use: always zero */
	Si->wReserved		= 0;
	Si->dwPageSize			= Sbi.PhysicalPageSize;
	Si->lpMinimumApplicationAddress	= (PVOID)Sbi.LowestUserAddress;
	Si->lpMaximumApplicationAddress	= (PVOID)Sbi.HighestUserAddress;
	Si->dwActiveProcessorMask	= Sbi.ActiveProcessors;
	Si->dwNumberOfProcessors	= Sbi.NumberProcessors;
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
