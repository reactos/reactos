/* $Id: sysinfo.c,v 1.5 2000/04/25 23:22:56 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sysinfo.c
 * PURPOSE:         System information functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/zwtypes.h>
#include <string.h>
#include <internal/ex.h>

#include <internal/debug.h>

extern ULONG NtGlobalFlag; /* FIXME: it should go in a ddk/?.h */

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtQuerySystemEnvironmentValue (
	IN	PUNICODE_STRING	Name,
	OUT	PVOID		Value,
	IN	ULONG		Length,
	IN OUT	PULONG		ReturnLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetSystemEnvironmentValue (
	IN	PUNICODE_STRING	VariableName,
	IN	PUNICODE_STRING	Value
	)
{
	UNIMPLEMENTED;
}


/* --- Query/Set System Information --- */


/*
 * NOTE: QSI_DEF(n) and SSI_DEF(n) define _cdecl function symbols
 * so the stack is popped only in one place on x86 platform.
 */
#define QSI_USE(n) QSI##n
#define QSI_DEF(n) \
static NTSTATUS QSI_USE(n) (PVOID Buffer, ULONG Size, PULONG ReqSize)

#define SSI_USE(n) SSI##n
#define SSI_DEF(n) \
static NTSTATUS SSI_USE(n) (PVOID Buffer, ULONG Size)

/* Class 0 - Basic Information */
QSI_DEF(SystemBasicInformation)
{
	PSYSTEM_BASIC_INFORMATION Sbi 
		= (PSYSTEM_BASIC_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_BASIC_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_BASIC_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	Sbi->AlwaysZero = 0;
	Sbi->KeMaximumIncrement = 0; /* FIXME */
	Sbi->MmPageSize = PAGESIZE; /* FIXME: it should be PAGE_SIZE */
	Sbi->MmNumberOfPhysicalPages = 0; /* FIXME */
	Sbi->MmLowestPhysicalPage = 0; /* FIXME */
	Sbi->MmHighestPhysicalPage = 0; /* FIXME */
	Sbi->MmLowestUserAddress = 0; /* FIXME */
	Sbi->MmLowestUserAddress1 = 0; /* FIXME */
	Sbi->MmHighestUserAddress = 0; /* FIXME */
	Sbi->KeActiveProcessors = 0x00000001; /* FIXME */
	Sbi->KeNumberProcessors = 1; /* FIXME */

	return (STATUS_SUCCESS);
}

/* Class 1 - Processor Information */
QSI_DEF(SystemProcessorInformation)
{
	PSYSTEM_PROCESSOR_INFORMATION Spi 
		= (PSYSTEM_PROCESSOR_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_PROCESSOR_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_PROCESSOR_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	/* FIXME: add CPU type detection code */
	Spi->KeProcessorArchitecture = 0; /* FIXME */
	Spi->KeProcessorLevel = 0; /* FIXME */
	Spi->KeProcessorRevision = 0; /* FIXME */
	Spi->AlwaysZero = 0;
	Spi->KeFeatureBits = 0x00000000; /* FIXME */
	
	return (STATUS_SUCCESS);
}

/* Class 2 - Performance Information */
QSI_DEF(SystemPerformanceInfo)
{
	PSYSTEM_PERFORMANCE_INFO Spi 
		= (PSYSTEM_PERFORMANCE_INFO) Buffer;

	*ReqSize = sizeof (SYSTEM_PERFORMANCE_INFO);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_PERFORMANCE_INFO))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	
	Spi->TotalProcessorTime.QuadPart = 0; /* FIXME */
	Spi->IoReadTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoWriteTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoOtherTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoReadOperationCount = 0; /* FIXME */
	Spi->IoWriteOperationCount = 0; /* FIXME */
	Spi->IoOtherOperationCount = 0; /* FIXME */
	Spi->MmAvailablePages = 0; /* FIXME */
	Spi->MmTotalCommitedPages = 0; /* FIXME */
	Spi->MmTotalCommitLimit = 0; /* FIXME */
	Spi->MmPeakLimit = 0; /* FIXME */
	Spi->PageFaults = 0; /* FIXME */
	Spi->WriteCopies = 0; /* FIXME */
	Spi->TransitionFaults = 0; /* FIXME */
	Spi->Unknown1 = 0; /* FIXME */
	Spi->DemandZeroFaults = 0; /* FIXME */
	Spi->PagesInput = 0; /* FIXME */
	Spi->PagesRead = 0; /* FIXME */
	Spi->Unknown2 = 0; /* FIXME */
	Spi->Unknown3 = 0; /* FIXME */
	Spi->PagesOutput = 0; /* FIXME */
	Spi->PageWrites = 0; /* FIXME */
	Spi->Unknown4 = 0; /* FIXME */
	Spi->Unknown5 = 0; /* FIXME */
	Spi->PoolPagedBytes = 0; /* FIXME */
	Spi->PoolNonPagedBytes = 0; /* FIXME */
	Spi->Unknown6 = 0; /* FIXME */
	Spi->Unknown7 = 0; /* FIXME */
	Spi->Unknown8 = 0; /* FIXME */
	Spi->Unknown9 = 0; /* FIXME */
	Spi->MmTotalSystemFreePtes = 0; /* FIXME */
	Spi->MmSystemCodepage = 0; /* FIXME */
	Spi->MmTotalSystemDriverPages = 0; /* FIXME */
	Spi->MmTotalSystemCodePages = 0; /* FIXME */
	Spi->Unknown10 = 0; /* FIXME */
	Spi->Unknown11 = 0; /* FIXME */
	Spi->Unknown12 = 0; /* FIXME */
	Spi->MmSystemCachePage = 0; /* FIXME */
	Spi->MmPagedPoolPage = 0; /* FIXME */
	Spi->MmSystemDriverPage = 0; /* FIXME */
	Spi->CcFastReadNoWait = 0; /* FIXME */
	Spi->CcFastReadWait = 0; /* FIXME */
	Spi->CcFastReadResourceMiss = 0; /* FIXME */
	Spi->CcFastReadNotPossible = 0; /* FIXME */
	Spi->CcFastMdlReadNoWait = 0; /* FIXME */
	Spi->CcFastMdlReadWait = 0; /* FIXME */
	Spi->CcFastMdlReadResourceMiss = 0; /* FIXME */
	Spi->CcFastMdlReadNotPossible = 0; /* FIXME */
	Spi->CcMapDataNoWait = 0; /* FIXME */
	Spi->CcMapDataWait = 0; /* FIXME */
	Spi->CcMapDataNoWaitMiss = 0; /* FIXME */
	Spi->CcMapDataWaitMiss = 0; /* FIXME */
	Spi->CcPinMappedDataCount = 0; /* FIXME */
	Spi->CcPinReadNoWait = 0; /* FIXME */
	Spi->CcPinReadWait = 0; /* FIXME */
	Spi->CcPinReadNoWaitMiss = 0; /* FIXME */
	Spi->CcPinReadWaitMiss = 0; /* FIXME */
	Spi->CcCopyReadNoWait = 0; /* FIXME */
	Spi->CcCopyReadWait = 0; /* FIXME */
	Spi->CcCopyReadNoWaitMiss = 0; /* FIXME */
	Spi->CcCopyReadWaitMiss = 0; /* FIXME */
	Spi->CcMdlReadNoWait = 0; /* FIXME */
	Spi->CcMdlReadWait = 0; /* FIXME */
	Spi->CcMdlReadNoWaitMiss = 0; /* FIXME */
	Spi->CcMdlReadWaitMiss = 0; /* FIXME */
	Spi->CcReadaheadIos = 0; /* FIXME */
	Spi->CcLazyWriteIos = 0; /* FIXME */
	Spi->CcLazyWritePages = 0; /* FIXME */
	Spi->CcDataFlushes = 0; /* FIXME */
	Spi->CcDataPages = 0; /* FIXME */
	Spi->ContextSwitches = 0; /* FIXME */
	Spi->Unknown13 = 0; /* FIXME */
	Spi->Unknown14 = 0; /* FIXME */
	Spi->SystemCalls = 0; /* FIXME */
	
	return (STATUS_SUCCESS);
}

/* Class 3 - Time Information */
QSI_DEF(SystemTimeInformation)
{
	PSYSTEM_TIME_INFORMATION Sti
		= (PSYSTEM_TIME_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_TIME_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_TIME_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	Sti->KeBootTime.QuadPart = 0;		/* FIXME */
	Sti->KeSystemTime.QuadPart = 0;		/* FIXME */
	Sti->ExpTimeZoneBias.QuadPart = 0;	/* FIXME */
	Sti->ExpTimeZoneId = 0;			/* FIXME */
	Sti->Unused = 0;			/* FIXME */

	return (STATUS_SUCCESS);
}

/* Class 4 - Path Information */
QSI_DEF(SystemPathInformation)
{
	/* FIXME: QSI returns STATUS_BREAKPOINT. Why? */
	return (STATUS_BREAKPOINT);
}

/* Class 5 - Process Information */
QSI_DEF(SystemProcessInformation)
{
	/* FIXME: scan the process+thread list */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 6 - SDT Information */
QSI_DEF(SystemServiceDescriptorTableInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 7 - I/O Configuration Information */
QSI_DEF(SystemIoConfigInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 8 - Processor Time Information */
QSI_DEF(SystemProcessorTimeInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 9 - Global Flag Information */
QSI_DEF(SystemNtGlobalFlagInformation)
{
	if (sizeof (SYSTEM_GLOBAL_FLAG_INFO) != Size)
	{
		* ReqSize = sizeof (SYSTEM_GLOBAL_FLAG_INFO);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	((PSYSTEM_GLOBAL_FLAG_INFO) Buffer)->NtGlobalFlag = NtGlobalFlag;
	return (STATUS_SUCCESS);
}

SSI_DEF(SystemNtGlobalFlagInformation)
{
	if (sizeof (SYSTEM_GLOBAL_FLAG_INFO) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	NtGlobalFlag = ((PSYSTEM_GLOBAL_FLAG_INFO) Buffer)->NtGlobalFlag;
	return (STATUS_SUCCESS);
}

/* Class 10 - ? Information */
QSI_DEF(SystemInformation10)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 11 - Modules Information */
QSI_DEF(SystemModuleInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 12 - Resource Lock Information */
QSI_DEF(SystemResourceLockInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 13 - ? Information */
QSI_DEF(SystemInformation13)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 14 - ? Information */
QSI_DEF(SystemInformation14)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 15 - ? Information */
QSI_DEF(SystemInformation15)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 16 - Handle Information */
QSI_DEF(SystemHandleInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 17 -  Information */
QSI_DEF(SystemObjectInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 18 -  Information */
QSI_DEF(SystemPageFileInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 19 -  Information */
QSI_DEF(SystemInstructionEmulationInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 20 - ? Information */
QSI_DEF(SystemInformation20)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 21 -  Information */
QSI_DEF(SystemCacheInformation)
{
	if (Size < sizeof (SYSTEM_CACHE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_CACHE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemCacheInformation)
{
	if (Size < sizeof (SYSTEM_CACHE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 22 - Pool Tag Information */
QSI_DEF(SystemPoolTagInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 23 - Processor Schedule Information */
QSI_DEF(SystemProcessorScheduleInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 24 - DPC Information */
QSI_DEF(SystemDpcInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemDpcInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 25 - ? Information */
QSI_DEF(SystemInformation25)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 26 - Load Image (callable) */
SSI_DEF(SystemLoadImage)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 27 - Unload Image (callable) */
SSI_DEF(SystemUnloadImage)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 28 -  Information */
QSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_TIME_ADJUSTMENT_INFO) > Size)
	{
		* ReqSize = sizeof (SYSTEM_TIME_ADJUSTMENT_INFO);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME: */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_TIME_ADJUSTMENT_INFO) > Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME: */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 29 - ? Information */
QSI_DEF(SystemInformation29)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 30 - ? Information */
QSI_DEF(SystemInformation30)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 31 - ? Information */
QSI_DEF(SystemInformation31)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 32 - Crach Dump Information */
QSI_DEF(SystemCrashDumpSectionInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 33 - Processor Fault Information */
QSI_DEF(SystemProcessorFaultCountInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 34 - Crach Dump State Information */
QSI_DEF(SystemCrashDumpStateInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 35 - Debugger Information */
QSI_DEF(SystemDebuggerInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 36 - Thread Switch Counters Information */
QSI_DEF(SystemThreadSwitchCountersInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 37 - Quota Information */
QSI_DEF(SystemQuotaInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemQuotaInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 38 - Load Driver Information */
SSI_DEF(SystemLoadDriverInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 39 - Priority Separation Information */
SSI_DEF(SystemPrioritySeparationInfo)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 40 - ? Information */
QSI_DEF(SystemInformation40)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 41 - ? Information */
QSI_DEF(SystemInformation41)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 42 - ? Information */
QSI_DEF(SystemInformation42)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 43 - ? Information */
QSI_DEF(SystemInformation43)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 44 -  Information */
QSI_DEF(SystemTimeZoneInformation)
{
	* ReqSize = sizeof (TIME_ZONE_INFORMATION);

	if (sizeof (TIME_ZONE_INFORMATION) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* Copy the time zone information struct */
        memcpy (
		Buffer,
                & SystemTimeZoneInfo,
                sizeof (TIME_ZONE_INFORMATION)
		);

	return (STATUS_SUCCESS);
}


SSI_DEF(SystemTimeZoneInformation)
{
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (TIME_ZONE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* Copy the time zone information struct */
	memcpy (
		& SystemTimeZoneInfo,
		(TIME_ZONE_INFORMATION *) Buffer,
		sizeof (TIME_ZONE_INFORMATION)
		);
	return (STATUS_SUCCESS);
}


/* Class 45 -  Information */
QSI_DEF(SystemLookasideInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Query/Set Calls Table */
typedef
struct _QSSI_CALLS
{
	NTSTATUS (* Query) (PVOID,ULONG,PULONG);
	NTSTATUS (* Set) (PVOID,ULONG);

} QSSI_CALLS;

// QS	Query & Set
// QX	Query
// XQ	Set
// XX	unknown behaviour
//
#define SI_QS(n) {QSI_USE(n),SSI_USE(n)}
#define SI_QX(n) {QSI_USE(n),NULL}
#define SI_XS(n) {NULL,SSI_USE(n)}
#define SI_XX(n) {NULL,NULL}

static
QSSI_CALLS
CallQS [] =
{
	SI_QX(SystemBasicInformation),
	SI_QX(SystemProcessorInformation),
	SI_QX(SystemPerformanceInfo),
	SI_QX(SystemTimeInformation),
	SI_QX(SystemPathInformation), /* should be SI_XX */
	SI_QX(SystemProcessInformation),
	SI_QX(SystemServiceDescriptorTableInfo),
	SI_QX(SystemIoConfigInfo),
	SI_QX(SystemProcessorTimeInfo),
	SI_QS(SystemNtGlobalFlagInformation),
	SI_QX(SystemInformation10), /* should be SI_XX */
	SI_QX(SystemModuleInfo),
	SI_QX(SystemResourceLockInfo),
	SI_QX(SystemInformation13), /* should be SI_XX */
	SI_QX(SystemInformation14), /* should be SI_XX */
	SI_QX(SystemInformation15), /* should be SI_XX */
	SI_QX(SystemHandleInfo),
	SI_QX(SystemObjectInformation),
	SI_QX(SystemPageFileInformation),
	SI_QX(SystemInstructionEmulationInfo),
	SI_QX(SystemInformation20), /* it should be SI_XX */
	SI_QS(SystemCacheInformation),
	SI_QX(SystemPoolTagInformation),
	SI_QX(SystemProcessorScheduleInfo),
	SI_QS(SystemDpcInformation),
	SI_QX(SystemInformation25), /* it should be SI_XX */
	SI_XS(SystemLoadImage),
	SI_XS(SystemUnloadImage),
	SI_QS(SystemTimeAdjustmentInformation),
	SI_QX(SystemInformation29), /* it should be SI_XX */
	SI_QX(SystemInformation30), /* it should be SI_XX */
	SI_QX(SystemInformation31), /* it should be SI_XX */
	SI_QX(SystemCrashDumpSectionInfo),
	SI_QX(SystemProcessorFaultCountInfo),
	SI_QX(SystemCrashDumpStateInfo),
	SI_QX(SystemDebuggerInfo),
	SI_QX(SystemThreadSwitchCountersInfo),
	SI_QS(SystemQuotaInformation),
	SI_XS(SystemLoadDriverInfo),
	SI_XS(SystemPrioritySeparationInfo),
	SI_QX(SystemInformation40), /* it should be SI_XX */
	SI_QX(SystemInformation41), /* it should be SI_XX */
	SI_QX(SystemInformation42), /* it should be SI_XX */
	SI_QX(SystemInformation43), /* it should be SI_XX */
	SI_QS(SystemTimeZoneInformation), /* it should be SI_QX */
	SI_QX(SystemLookasideInformation)
};


NTSTATUS
STDCALL
NtQuerySystemInformation (
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	OUT	PVOID				SystemInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
	)
{
	/*
	 * If called from user mode, check 
	 * possible unsafe arguments.
	 */
#if 0
        if (KernelMode != KeGetPreviousMode())
        {
		// Check arguments
		//ProbeForWrite(
		//	SystemInformation,
		//	Length
		//	);
		//ProbeForWrite(
		//	ResultLength,
		//	sizeof (ULONG)
		//	);
        }
#endif
	/*
	 * Clear the user buffer.
	 */
	RtlZeroMemory (SystemInformation, Length);
	/*
	 * Check the request is valid.
	 */
	if (	(SystemInformationClass >= SystemInformationClassMin)
		&& (SystemInformationClass < SystemInformationClassMax)
		)
	{
		if (NULL != CallQS [SystemInformationClass].Query)
		{
			/*
			 * Hand the request to a subhandler.
			 */
			return CallQS [SystemInformationClass].Query (
					SystemInformation,
					Length,
					ResultLength
					);
		}
	}
	return (STATUS_INVALID_INFO_CLASS);
}


NTSTATUS
STDCALL
NtSetSystemInformation (
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	IN	PVOID				SystemInformation,
	IN	ULONG				SystemInformationLength
	)
{
	/*
	 * If called from user mode, check 
	 * possible unsafe arguments.
	 */
#if 0
        if (KernelMode != KeGetPreviousMode())
        {
		// Check arguments
		//ProbeForWrite(
		//	SystemInformation,
		//	Length
		//	);
		//ProbeForWrite(
		//	ResultLength,
		//	sizeof (ULONG)
		//	);
        }
#endif
	/*
	 * Check the request is valid.
	 */
	if (	(SystemInformationClass >= SystemInformationClassMin)
		&& (SystemInformationClass < SystemInformationClassMax)
		)
	{
		if (NULL != CallQS [SystemInformationClass].Set)
		{
			/*
			 * Hand the request to a subhandler.
			 */
			return CallQS [SystemInformationClass].Set (
					SystemInformation,
					SystemInformationLength
					);
		}
	}
	return (STATUS_INVALID_INFO_CLASS);
}


NTSTATUS
STDCALL
NtFlushInstructionCache (
	IN	HANDLE	ProcessHandle,
	IN	PVOID	BaseAddress,
	IN	UINT	NumberOfBytesToFlush
	)
{
	UNIMPLEMENTED;
}


/* EOF */
