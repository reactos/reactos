/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/sysinfo.c
 * PURPOSE:         SystemInfo functions
 * PROGRAMMER:      Emanuele Aliberti
 *                  Christoph von Wittich
 *                  Thomas Weidenmueller
 *                  Gunnar Andre Dalsnes
 * UPDATE HISTORY:
 *     2000-04-26 created
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>


#define PV_NT351 0x00030033

/*
 * @implemented
 */
SIZE_T
WINAPI
GetLargePageMinimum(VOID)
{
	return SharedUserData->LargePageMinimum;
}

/*
 * @unimplemented
 */
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
	Si->wReserved			= 0;
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

	case PROCESSOR_ARCHITECTURE_IA64:
		Si->dwProcessorType = PROCESSOR_INTEL_IA64;
		break;

	case PROCESSOR_ARCHITECTURE_PPC:
		switch (Spi.ProcessorLevel)
		{
		case 1:
			Si->dwProcessorType = PROCESSOR_PPC_601;
			break;
		case 3:
			Si->dwProcessorType = PROCESSOR_PPC_603;
			break;
		case 4:
			Si->dwProcessorType = PROCESSOR_PPC_604;
			break;
		case 6:
			/* PPC 603+ */
			Si->dwProcessorType = PROCESSOR_PPC_603;
			break;
		case 9:
			/* PPC 604+ */
			Si->dwProcessorType = PROCESSOR_PPC_604;
			break;
		case 20:
			Si->dwProcessorType = PROCESSOR_PPC_620;
			break;
		default:
			Si->dwProcessorType = 0;
		}
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


/*
 * @implemented
 */
BOOL STDCALL
IsProcessorFeaturePresent(DWORD ProcessorFeature)
{
  if (ProcessorFeature >= PROCESSOR_FEATURE_MAX)
    return(FALSE);

  return((BOOL)SharedUserData->ProcessorFeatures[ProcessorFeature]);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSystemRegistryQuota(PDWORD pdwQuotaAllowed,
                       PDWORD pdwQuotaUsed)
{
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;
    ULONG BytesWritten;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemRegistryQuotaInformation,
                                      &srqi,
                                      sizeof(srqi),
                                      &BytesWritten);
    if(NT_SUCCESS(Status))
    {
      if(pdwQuotaAllowed != NULL)
      {
        *pdwQuotaAllowed = srqi.RegistryQuotaAllowed;
      }
      if(pdwQuotaUsed != NULL)
      {
        *pdwQuotaUsed = srqi.RegistryQuotaUsed;
      }

      return TRUE;
    }

    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
VOID
STDCALL
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    //FIXME: GetNativeSystemInfo should return always the real Hardware Processorarchitecture
    // in case a Program is running in 32bit Mode on AMD64
    // GetSystemInfo should return PROCESSOR_ARCHITECTURE_INTEL and
    // GetNativeSystemInfo should return PROCESSOR_ARCHITECTURE_AMD64
    GetSystemInfo(lpSystemInfo);
}
