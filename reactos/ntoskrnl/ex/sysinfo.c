/* $Id: sysinfo.c,v 1.11 2001/04/22 23:06:57 ea Exp $
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
#include <internal/ex.h>
#include <internal/ldr.h>

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

	Sbi->Reserved = 0;
	Sbi->TimerResolution = 0; /* FIXME */
	Sbi->PageSize = PAGESIZE; /* FIXME: it should be PAGE_SIZE */
	Sbi->NumberOfPhysicalPages = 0; /* FIXME */
	Sbi->LowestPhysicalPageNumber = 0; /* FIXME */
	Sbi->HighestPhysicalPageNumber = 0; /* FIXME */
	Sbi->AllocationGranularity = 65536; /* hard coded on Intel? */
	Sbi->MinimumUserModeAddress = 0; /* FIXME */
	Sbi->MaximumUserModeAddress = 0; /* FIXME */
	Sbi->ActiveProcessorsAffinityMask = 0x00000001; /* FIXME */
	Sbi->NumberOfProcessors = 1; /* FIXME */

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
	Spi->ProcessorArchitecture = 0; /* FIXME */
	Spi->ProcessorLevel = 0; /* FIXME */
	Spi->ProcessorRevision = 0; /* FIXME */
	Spi->Reserved = 0;
	Spi->ProcessorFeatureBits = 0x00000000; /* FIXME */
	
	return (STATUS_SUCCESS);
}

/* Class 2 - Performance Information */
QSI_DEF(SystemPerformanceInformation)
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
	
	Spi->IdleProcessorTime.QuadPart = 0; /* FIXME */
	Spi->IoReadTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoWriteTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoOtherTransferCount.QuadPart = 0; /* FIXME */
	Spi->IoReadOperationCount = 0; /* FIXME */
	Spi->IoWriteOperationCount = 0; /* FIXME */
	Spi->IoOtherOperationCount = 0; /* FIXME */
	Spi->AvailablePages = 0; /* FIXME */
	Spi->CommitedPages = 0; /* FIXME */
	Spi->CommitLimit = 0; /* FIXME */
	Spi->PeakCommitment = 0; /* FIXME */
	Spi->PageFaultCount = 0; /* FIXME */
	Spi->CopyOnWriteCount = 0; /* FIXME */
	Spi->TransitionCount = 0; /* FIXME */
	Spi->CacheTransitionCount = 0; /* FIXME */
	Spi->DemandZeroCount = 0; /* FIXME */
	Spi->PageReadCount = 0; /* FIXME */
	Spi->PageReadIoCount = 0; /* FIXME */
	Spi->CacheReadCount = 0; /* FIXME */
	Spi->CacheIoCount = 0; /* FIXME */
	Spi->DirtyPagesWriteCount = 0; /* FIXME */
	Spi->DirtyWriteIoCount = 0; /* FIXME */
	Spi->MappedPagesWriteCount = 0; /* FIXME */
	Spi->MappedWriteIoCount = 0; /* FIXME */
	Spi->PagedPoolPages = 0; /* FIXME */
	Spi->NonPagedPoolPages = 0; /* FIXME */
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

/* Class 3 - Time Of Day Information */
QSI_DEF(SystemTimeOfDayInformation)
{
	PSYSTEM_TIMEOFDAY_INFORMATION Sti
		= (PSYSTEM_TIMEOFDAY_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_TIMEOFDAY_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_TIMEOFDAY_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	Sti->BootTime.QuadPart = 0;	/* FIXME */
	Sti->CurrentTime.QuadPart = 0;	/* FIXME */
	Sti->TimeZoneBias.QuadPart = 0;	/* FIXME */
	Sti->TimeZoneId = 0;		/* FIXME */
	Sti->Reserved = 0;		/* FIXME */

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

/* Class 6 - Call Count Information */
QSI_DEF(SystemCallCountInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 7 - Device Information */
QSI_DEF(SystemDeviceInformation)
{
	PSYSTEM_DEVICE_INFORMATION Sdi 
		= (PSYSTEM_DEVICE_INFORMATION) Buffer;
	PCONFIGURATION_INFORMATION ConfigInfo;

	*ReqSize = sizeof (SYSTEM_DEVICE_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_DEVICE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	ConfigInfo = IoGetConfigurationInformation ();

	Sdi->NumberOfDisks = ConfigInfo->DiskCount;
	Sdi->NumberOfFloppies = ConfigInfo->FloppyCount;
	Sdi->NumberOfCdRoms = ConfigInfo->CDRomCount;
	Sdi->NumberOfTapes = ConfigInfo->TapeCount;
	Sdi->NumberOfSerialPorts = ConfigInfo->SerialCount;
	Sdi->NumberOfParallelPorts = ConfigInfo->ParallelCount;

	return (STATUS_SUCCESS);
}

/* Class 8 - Processor Performance Information */
QSI_DEF(SystemProcessorPerformanceInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 9 - Flags Information */
QSI_DEF(SystemFlagsInformation)
{
	if (sizeof (SYSTEM_FLAGS_INFORMATION) != Size)
	{
		* ReqSize = sizeof (SYSTEM_FLAGS_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags = NtGlobalFlag;
	return (STATUS_SUCCESS);
}

SSI_DEF(SystemFlagsInformation)
{
	if (sizeof (SYSTEM_FLAGS_INFORMATION) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	NtGlobalFlag = ((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags;
	return (STATUS_SUCCESS);
}

/* Class 10 - Call Time Information */
QSI_DEF(SystemCallTimeInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 11 - Module Information */
QSI_DEF(SystemModuleInformation)
{
	return LdrpQueryModuleInformation(Buffer, Size, ReqSize);
}

/* Class 12 - Locks Information */
QSI_DEF(SystemLocksInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 13 - Stack Trace Information */
QSI_DEF(SystemStackTraceInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 14 - Paged Pool Information */
QSI_DEF(SystemPagedPoolInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 15 - Non Paged Pool Information */
QSI_DEF(SystemNonPagedPoolInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 16 - Handle Information */
QSI_DEF(SystemHandleInformation)
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

/* Class 19 - Vdm Instemul Information */
QSI_DEF(SystemVdmInstemulInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 20 - Vdm Bop Information */
QSI_DEF(SystemVdmBopInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 21 - File Cache Information */
QSI_DEF(SystemFileCacheInformation)
{
	if (Size < sizeof (SYSTEM_CACHE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_CACHE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemFileCacheInformation)
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

/* Class 23 - Interrupt Information */
QSI_DEF(SystemInterruptInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 24 - DPC Behaviour Information */
QSI_DEF(SystemDpcBehaviourInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemDpcBehaviourInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 25 - Full Memory Information */
QSI_DEF(SystemFullMemoryInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 26 - Load Gdi Driver Information */
SSI_DEF(SystemLoadGdiDriverInformation)
{
	PSYSTEM_GDI_DRIVER_INFORMATION Sdi
		= (PSYSTEM_GDI_DRIVER_INFORMATION) Buffer;

	if (sizeof (SYSTEM_GDI_DRIVER_INFORMATION) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	return LdrLoadGdiDriver (&Sdi->DriverName,
				 &Sdi->ImageAddress,
				 &Sdi->SectionPointer,
				 &Sdi->EntryPoint,
				 &Sdi->ExportSectionPointer);
}

/* Class 27 - Unload Gdi Driver Information */
SSI_DEF(SystemUnloadGdiDriverInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 28 - Time Adjustment Information */
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

/* Class 29 - Summary Memory Information */
QSI_DEF(SystemSummaryMemoryInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 30 - Next Event Id Information */
QSI_DEF(SystemNextEventIdInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 31 - Event Ids Information */
QSI_DEF(SystemEventIdsInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 32 - Crach Dump Information */
QSI_DEF(SystemCrashDumpInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 33 - Exception Information */
QSI_DEF(SystemExceptionInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 34 - Crach Dump State Information */
QSI_DEF(SystemCrashDumpStateInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 35 - Kernel Debugger Information */
QSI_DEF(SystemKernelDebuggerInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 36 - Context Switch Information */
QSI_DEF(SystemContextSwitchInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 37 - Registry Quota Information */
QSI_DEF(SystemRegistryQuotaInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemRegistryQuotaInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 38 - Extend Service Table Information */
SSI_DEF(SystemExtendServiceTableInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 39 - Priority Seperation */
SSI_DEF(SystemPrioritySeperation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 40 - Plug Play Bus Information */
QSI_DEF(SystemPlugPlayBusInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 41 - Dock Information */
QSI_DEF(SystemDockInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 42 - Power Information */
QSI_DEF(SystemPowerInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 43 - Processor Speed Information */
QSI_DEF(SystemProcessorSpeedInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 44 - Current Time Zone Information */
QSI_DEF(SystemCurrentTimeZoneInformation)
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


SSI_DEF(SystemCurrentTimeZoneInformation)
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


/* Class 45 - Lookaside Information */
QSI_DEF(SystemLookasideInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 46 - Set time slip event */
SSI_DEF(SystemSetTimeSlipEvent)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 47 - Create a new session (TSE) */
SSI_DEF(SystemCreateSession)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 48 - Delete an existing session (TSE) */
SSI_DEF(SystemDeleteSession)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 49 - UNKNOWN */
QSI_DEF(SystemInvalidInfoClass4)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 50 - System range start address */
QSI_DEF(SystemRangeStartInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 51 - Driver verifier information */
QSI_DEF(SystemVerifierInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


SSI_DEF(SystemVerifierInformation)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 52 - Add a driver verifier */
SSI_DEF(SystemAddVerifier)
{
	/* FIXME */
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 53 - A session's processes  */
QSI_DEF(SystemSessionProcessesInformation)
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
	SI_QX(SystemPerformanceInformation),
	SI_QX(SystemTimeOfDayInformation),
	SI_QX(SystemPathInformation), /* should be SI_XX */
	SI_QX(SystemProcessInformation),
	SI_QX(SystemCallCountInformation),
	SI_QX(SystemDeviceInformation),
	SI_QX(SystemProcessorPerformanceInformation),
	SI_QS(SystemFlagsInformation),
	SI_QX(SystemCallTimeInformation), /* should be SI_XX */
	SI_QX(SystemModuleInformation),
	SI_QX(SystemLocksInformation),
	SI_QX(SystemStackTraceInformation), /* should be SI_XX */
	SI_QX(SystemPagedPoolInformation), /* should be SI_XX */
	SI_QX(SystemNonPagedPoolInformation), /* should be SI_XX */
	SI_QX(SystemHandleInformation),
	SI_QX(SystemObjectInformation),
	SI_QX(SystemPageFileInformation),
	SI_QX(SystemVdmInstemulInformation),
	SI_QX(SystemVdmBopInformation), /* it should be SI_XX */
	SI_QS(SystemFileCacheInformation),
	SI_QX(SystemPoolTagInformation),
	SI_QX(SystemInterruptInformation),
	SI_QS(SystemDpcBehaviourInformation),
	SI_QX(SystemFullMemoryInformation), /* it should be SI_XX */
	SI_XS(SystemLoadGdiDriverInformation),
	SI_XS(SystemUnloadGdiDriverInformation),
	SI_QS(SystemTimeAdjustmentInformation),
	SI_QX(SystemSummaryMemoryInformation), /* it should be SI_XX */
	SI_QX(SystemNextEventIdInformation), /* it should be SI_XX */
	SI_QX(SystemEventIdsInformation), /* it should be SI_XX */
	SI_QX(SystemCrashDumpInformation),
	SI_QX(SystemExceptionInformation),
	SI_QX(SystemCrashDumpStateInformation),
	SI_QX(SystemKernelDebuggerInformation),
	SI_QX(SystemContextSwitchInformation),
	SI_QS(SystemRegistryQuotaInformation),
	SI_XS(SystemExtendServiceTableInformation),
	SI_XS(SystemPrioritySeperation),
	SI_QX(SystemPlugPlayBusInformation), /* it should be SI_XX */
	SI_QX(SystemDockInformation), /* it should be SI_XX */
	SI_QX(SystemPowerInformation), /* it should be SI_XX */
	SI_QX(SystemProcessorSpeedInformation), /* it should be SI_XX */
	SI_QS(SystemCurrentTimeZoneInformation), /* it should be SI_QX */
	SI_QX(SystemLookasideInformation),
	SI_XS(SystemSetTimeSlipEvent),
	SI_XS(SystemCreateSession),
	SI_XS(SystemDeleteSession),
	SI_QX(SystemInvalidInfoClass4), /* it should be SI_XX */
	SI_QX(SystemRangeStartInformation),
	SI_QS(SystemVerifierInformation),
	SI_XS(SystemAddVerifier),
	SI_QX(SystemSessionProcessesInformation)
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
