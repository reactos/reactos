/* $Id: qsi.c,v 1.2 2000/04/27 23:39:49 ea Exp $
 *
 * PROJECT    : ReactOS Operating System (see http://www.reactos.com/)
 * DESCRIPTION: Tool to query system information
 * FILE       : rosapps/sysutils/qsi.c
 * AUTHOR     : Emanuele Aliberti
 * LICENSE    : GNU GPL (see http://www.gnu.org/)
 * DATE       : 1999-07-28
 * 
 * BUILD INSTRUCTIONS
 * 	If you got this code directly from the CVS repository on
 * 	mok.lcvm.com, it should be ok to run "make sqi.exe" from the
 * 	current directory. Otherwise, be sure the directories
 * 	"rosapps" and "reactos" are siblings (see the FILE file
 * 	in the header).
 *
 * REVISIONS
 * 	2000-02-12 (ea)
 * 		Partially rewritten to run as a tool to query
 * 		every system information class data.
 * 	2000-04-23 (ea)
 * 		Added almost all structures for getting system
 * 		information (from UNDOCNT.H by Dabak et alii).
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define NTOS_MODE_USER
#include <ntos.h>

typedef
struct _FLAGS
{
	DWORD	Verbose:1;	/* print unknown, unused, service fields */
	DWORD	Dump:1;		/* raw dump output buffer */
	DWORD	Batch:1;	/* no shell (for future use) */
} FLAGS;

static
struct
{
	FLAGS	Flag;
	BOOL	Active;
	HANDLE	Heap;
	INT	ExitCode;
	
} Application =
{
	{0, 0},
	FALSE
};

#define ONOFF(b) ((b)?"on":"off")

#define ARGV_SIZE 64

#define BUFFER_SIZE_DEFAULT 65536

#define TF(b) ((b)?"true":"false")

typedef
INT (* COMMAND_CALL) (INT ArgC,LPCSTR ArgV []);



VOID STDCALL PrintStatus (NTSTATUS Status);

#define CMD_REF(n) CMD_##n
#define CMD_DEF(n) INT CMD_REF(n) (INT argc, LPCSTR argv [])
#define CMD_NOT_IMPLEMENTED {printf("%s not implemented\n", argv[0]);return(0);}

typedef
struct _COMMAND_DESCRIPTOR
{
	LPCSTR		Name;
	COMMAND_CALL	EntryPoint;
	LPCSTR		Description;

} COMMAND_DESCRIPTOR, * PCOMMAND_DESCRIPTOR;


/* Fast BYTE to binary representation */

#define BIT(n,m) (((n)&(m))?'1':'0')
LPSTR
STDCALL
ByteToBinaryString (
	BYTE	Byte,
	CHAR	Binary [8]
	)
{
	Binary [7] = BIT(Byte,0x01);
	Binary [6] = BIT(Byte,0x02);
	Binary [5] = BIT(Byte,0x04);
	Binary [4] = BIT(Byte,0x08);
	Binary [3] = BIT(Byte,0x10);
	Binary [2] = BIT(Byte,0x20);
	Binary [1] = BIT(Byte,0x40);
	Binary [0] = BIT(Byte,0x80);
	return (LPSTR) Binary;
}

/* --- */
VOID
STDCALL
DumpData (int Size, PVOID pData )
{
	PBYTE		Buffer = (PBYTE) pData;
	PBYTE		Base = Buffer;
	int		i;
	const int	Width = 16;

	if (! Application.Flag.Dump)
	{
		return;
	}
	while (Size > 0) 
	{
		printf ("%04x:  ", (Buffer - Base));
		for (	i = 0;
			(i < Width);
			++i
			)
		{
			if (Size - i > 0)
			{
				printf (
					"%02x%c",
					Buffer[i],
					(i % 4 == 3) ? '|' : ' '
					);
			}
			else
			{
				printf ("   ");
			}
		}
		printf (" ");
		for (	i = 0;
			(i < Width);
			++i
			)
		{
			if (Size - i > 0)
			{
				printf (
					"%c",
					(	(Buffer[i] > ' ')
						&& (Buffer[i] < 127)
						)
						? Buffer[i]
						: ' '
					);
			}
		}
		printf ("\n");
		Buffer += Width;
		Size -= Width;
	}
	printf ("\n");
}


int
STDCALL
FindRequiredBufferSize (int i, int step)
{
	NTSTATUS	Status = STATUS_INFO_LENGTH_MISMATCH;
	BYTE		Buffer [BUFFER_SIZE_DEFAULT];
	INT		Size;
	LONG		Length = 0;

	
	Size = step = (step > 0 ? step : 1);
	while	(	(Size < sizeof Buffer)
			&& (Status == STATUS_INFO_LENGTH_MISMATCH)
			)
	{
		if (Application.Flag.Verbose)
		{
			printf ("\tTry %d", Size);
		}
		RtlZeroMemory (Buffer, sizeof Buffer);
		Length = 0;
		Status = NtQuerySystemInformation (
				i,
				& Buffer,
				Size,
				& Length
				);
		if (STATUS_SUCCESS == Status)
		{
			printf ("Length = %d\n", Size);
			return Size;
		}
		if (Length > 0)
		{
			Size = Length;
		}
		else
		{
			/* FIXME: slow linear search! */
			Size += step;
		}
	}
	printf ("No valid buffer length found!\n");
	return -1;
}


VOID
STDCALL
PrintStatus (NTSTATUS Status)
{
	LPCSTR StatusName = NULL;
	
	switch (Status)
	{
		case STATUS_INVALID_INFO_CLASS:
			StatusName = "STATUS_INVALID_INFO_CLASS";
			break;
		case STATUS_INFO_LENGTH_MISMATCH:
			StatusName = "STATUS_INFO_LENGTH_MISMATCH";
			break;
		case STATUS_ACCESS_VIOLATION:
			StatusName = "STATUS_ACCESS_VIOLATION";
			break;
		case STATUS_NOT_IMPLEMENTED:
			StatusName = "STATUS_NOT_IMPLEMENTED";
			break;
		case STATUS_BREAKPOINT:
			StatusName = "STATUS_BREAKPOINT";
			break;
	}
	if (NULL != StatusName)
	{
		printf ("\tStatus = %s\n", StatusName);
		return;
	}
	printf ("\tStatus = 0x%08lX\n", Status );
}

/* Auxiliary functions */

PCHAR
DaysOfWeek [] =
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

VOID
STDCALL
PrintUtcDateTime (LPCSTR Template, PTIME UtcTime)
{
	CHAR		UtcTimeString [64];
	TIME_FIELDS	UtcTimeFields;

	RtlTimeToTimeFields (
		(PLARGE_INTEGER) UtcTime,
		& UtcTimeFields
		);
	sprintf (
		UtcTimeString,
		"%s %d-%02d-%02d %02d:%02d:%02d.%03d UTC",
		DaysOfWeek[UtcTimeFields.Weekday],
		UtcTimeFields.Year,
		UtcTimeFields.Month,
		UtcTimeFields.Day,
		UtcTimeFields.Hour,
		UtcTimeFields.Minute,
		UtcTimeFields.Second,
		UtcTimeFields.Milliseconds
		); 
	printf (
		Template,
		UtcTimeString
		);
}


/**********************************************************************
 *	Dumpers
 **********************************************************************/


/**********************************************************************
 * 
 * DESCRIPTION
 *	Dump whatever we get by calling NtQuerySystemInformation with
 *	a user provided system information class id.
 * NOTE
 * 	NtQuerySystemInformation called with user class id.
 */
CMD_DEF(unknown)
{
	int		_id = atoi ((char*)(argv[0] + 1));	/* "#24" */
								/*   ^   */
	int		Size = -1;
	PBYTE		Buffer = NULL;
	NTSTATUS	Status;

	
	printf ("SystemInformation %d:\n", _id);
	/* Find buffer size */
	Size = FindRequiredBufferSize (_id, 1);
	if (-1 == Size)
	{
		printf("\t(no data)\n");
		return EXIT_FAILURE;
	}
	/* Allocate the buffer */
	Buffer = GlobalAlloc (GMEM_ZEROINIT, Size);
	if (NULL == Buffer)
	{
		printf ("#%d: could not allocate %d bytes\n", _id, Size);
		return EXIT_FAILURE;
	}
	/* Query the executive */
	Status = NtQuerySystemInformation (
			_id,
			Buffer,
			Size,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		FindRequiredBufferSize (_id, 1);
		GlobalFree (Buffer);
		return EXIT_FAILURE;
	}
	/* Print the data */
	DumpData (Size, Buffer);
	/* --- */
	GlobalFree (Buffer);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 0.
 */
CMD_DEF(Basic)
{
	NTSTATUS			Status;
	SYSTEM_BASIC_INFORMATION	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemBasicInformation:\n");
	}
	RtlZeroMemory (
		(PVOID) & Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			SystemBasicInformation,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tAlwaysZero              = 0x%08x\n", Info.AlwaysZero);
	printf ("\tKeMaximumIncrement      = %ld\n", Info.KeMaximumIncrement);
	printf ("\tMmPageSize              = %ld\n", Info.MmPageSize);
	printf ("\tMmNumberOfPhysicalPages = %ld\n", Info.MmNumberOfPhysicalPages);
	printf ("\tMmLowestPhysicalPage    = %ld\n", Info.MmLowestPhysicalPage);
	printf ("\tMmHighestPhysicalPage   = %ld\n", Info.MmHighestPhysicalPage);
	printf ("\tMmLowestUserAddress     = 0x%08x\n", Info.MmLowestUserAddress);
	printf ("\tMmLowestUserAddress1    = 0x%08x\n", Info.MmLowestUserAddress1);
	printf ("\tMmHighestUserAddress    = 0x%08x\n", Info.MmHighestUserAddress);
	printf ("\tKeActiveProcessors      = 0x%08x\n", Info.KeActiveProcessors);
	printf ("\tKeNumberProcessors      = %ld\n", Info.KeNumberProcessors);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 1.
 */
CMD_DEF(Processor)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSOR_INFORMATION	Info;
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemProcessorInformation:\n");
	}
	RtlZeroMemory (
		(PVOID) & Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			SystemProcessorInformation,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tKeProcessorArchitecture = %ld\n", Info.KeProcessorArchitecture);
	printf ("\tKeProcessorLevel        = %ld\n", Info.KeProcessorLevel);
	printf ("\tKeProcessorRevision     = %ld\n", Info.KeProcessorRevision);
	if (Application.Flag.Verbose)
	{
		printf ("\tAlwaysZero              = 0x%08x\n", Info.AlwaysZero);
	}
	printf ("\tKeFeatureBits           = %08x\n", Info.KeFeatureBits);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *	System performance information.
 *	
 * NOTE
 * 	Class 2.
 */
CMD_DEF(Performance)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_PERFORMANCE_INFO	Info;
	LONG				Length = 0;


	if (Application.Flag.Verbose)
	{
		printf ("SystemPerformanceInformation:\n");
	}
	Status = NtQuerySystemInformation (
			SystemPerformanceInformation,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("Not implemented.\n");
#if 0
	LARGE_INTEGER	TotalProcessorTime;
	LARGE_INTEGER	IoReadTransferCount;
	LARGE_INTEGER	IoWriteTransferCount;
	LARGE_INTEGER	IoOtherTransferCount;
	ULONG		IoReadOperationCount;
	ULONG		IoWriteOperationCount;
	ULONG		IoOtherOperationCount;
	ULONG		MmAvailablePages;
	ULONG		MmTotalCommitedPages;
	ULONG		MmTotalCommitLimit;
	ULONG		MmPeakLimit;
	ULONG		PageFaults;
	ULONG		WriteCopies;
	ULONG		TransitionFaults;
	ULONG		Unknown1;
	ULONG		DemandZeroFaults;
	ULONG		PagesInput;
	ULONG		PagesRead;
	ULONG		Unknown2;
	ULONG		Unknown3;
	ULONG		PagesOutput;
	ULONG		PageWrites;
	ULONG		Unknown4;
	ULONG		Unknown5;
	ULONG		PoolPagedBytes;
	ULONG		PoolNonPagedBytes;
	ULONG		Unknown6;
	ULONG		Unknown7;
	ULONG		Unknown8;
	ULONG		Unknown9;
	ULONG		MmTotalSystemFreePtes;
	ULONG		MmSystemCodepage;
	ULONG		MmTotalSystemDriverPages;
	ULONG		MmTotalSystemCodePages;
	ULONG		Unknown10;
	ULONG		Unknown11;
	ULONG		Unknown12;
	ULONG		MmSystemCachePage;
	ULONG		MmPagedPoolPage;
	ULONG		MmSystemDriverPage;
	ULONG		CcFastReadNoWait;
	ULONG		CcFastReadWait;
	ULONG		CcFastReadResourceMiss;
	ULONG		CcFastReadNotPossible;
	ULONG		CcFastMdlReadNoWait;
	ULONG		CcFastMdlReadWait;
	ULONG		CcFastMdlReadResourceMiss;
	ULONG		CcFastMdlReadNotPossible;
	ULONG		CcMapDataNoWait;
	ULONG		CcMapDataWait;
	ULONG		CcMapDataNoWaitMiss;
	ULONG		CcMapDataWaitMiss;
	ULONG		CcPinMappedDataCount;
	ULONG		CcPinReadNoWait;
	ULONG		CcPinReadWait;
	ULONG		CcPinReadNoWaitMiss;
	ULONG		CcPinReadWaitMiss;
	ULONG		CcCopyReadNoWait;
	ULONG		CcCopyReadWait;
	ULONG		CcCopyReadNoWaitMiss;
	ULONG		CcCopyReadWaitMiss;
	ULONG		CcMdlReadNoWait;
	ULONG		CcMdlReadWait;
	ULONG		CcMdlReadNoWaitMiss;
	ULONG		CcMdlReadWaitMiss;
	ULONG		CcReadaheadIos;
	ULONG		CcLazyWriteIos;
	ULONG		CcLazyWritePages;
	ULONG		CcDataFlushes;
	ULONG		CcDataPages;
	ULONG		ContextSwitches;
	ULONG		Unknown13;
	ULONG		Unknown14;
	ULONG		SystemCalls;
#endif
	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 3.
 */
CMD_DEF(Time)
{
	NTSTATUS		Status;
	SYSTEM_TIME_INFORMATION	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemTimeInformation:\n");
	}
	Status = NtQuerySystemInformation (
			SystemTimeInformation,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	PrintUtcDateTime ("\tKeBootTime     : %s\n", & Info.KeBootTime);
	PrintUtcDateTime ("\tKeSystemTime   : %s\n", & Info.KeSystemTime);
	PrintUtcDateTime ("\tExpTimeZoneBias: %s\n", & Info.ExpTimeZoneBias); /* FIXME */
	printf ("\tExpTimeZoneId  : %ld\n", Info.ExpTimeZoneId);
	if (Application.Flag.Verbose)
	{
		printf ("\tUnused         : %08x (?)\n", Info.Unused);
	}

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 4.
 */
CMD_DEF(Path)
{
	NTSTATUS		Status;
	SYSTEM_PATH_INFORMATION	Info;
	CHAR			_Info [_MAX_PATH];
	ULONG			Length = 0;

	RtlZeroMemory (& Info, _MAX_PATH);
	Status = NtQuerySystemInformation (
			SystemPathInformation,
			& _Info,
			_MAX_PATH,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		DumpData (_MAX_PATH, & _Info);
		return EXIT_FAILURE;
	}
	DumpData (_MAX_PATH, & _Info);
	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *	A snapshot of the process+thread tables.
 *
 * NOTE
 * 	Class 5.
 */
CMD_DEF(Process)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_PROCESS_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	ULONG				ThreadIndex;

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check NULL==pInfo */
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemProcessInformation:\n");
	}
	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			SystemProcessInformation,
			pInfo,
			BUFFER_SIZE_DEFAULT,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			/*
			 *	Allocate buffer
			 */
			pInfo = GlobalReAlloc (pInfo, Length, GMEM_ZEROINIT);
			if (NULL == pInfo)
			{
				printf ("\tCould not allocate memory.\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			PrintStatus (Status);
			GlobalFree (pInfo);
			return EXIT_FAILURE;
		}
	}
	/*
	 *	Get process+thread list from ntoskrnl.exe
	 */
	Status = NtQuerySystemInformation (
			SystemProcessInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		GlobalFree (pInfo);
		return EXIT_FAILURE;
	}

	while (1)
	{
		wprintf (L"%s:\n", (pInfo->Name.Length ? pInfo->Name.Buffer : L"*idle*") );
		if (Application.Flag.Verbose)
		{
			wprintf (L"\tRelativeOffset = 0x%08x\n", pInfo->RelativeOffset);
		}
		wprintf (L"\tThreads      = %ld\n", pInfo->ThreadCount);
		wprintf (L"\tHandles      = %ld\n", pInfo->HandleCount);
		wprintf (L"\tBasePriority = %ld\n", pInfo->BasePriority);
		wprintf (L"\tPID          = %ld\n", pInfo->ProcessId);
		wprintf (L"\tPPID         = %ld\n", pInfo->ParentProcessId);
		wprintf (L"\tVirtualSize:\t\tWorkingSetSize:\n");
		wprintf (L"\t\tPeak : %ld\t\t\tPeak : %ld\n",
			pInfo->PeakVirtualSizeBytes,
			pInfo->PeakWorkingSetSizeBytes
			);
		wprintf (L"\t\tTotal: %ld\t\t\tTotal: %ld\n",
			pInfo->TotalVirtualSizeBytes,
			pInfo->TotalWorkingSetSizeBytes
			);
		wprintf (L"\tPagedPoolUsage:\t\tNonPagedPoolUsage:\n");
		wprintf (L"\t\tPeak : %ld\t\t\tPeak : %ld\n",
			pInfo->PeakPagedPoolUsagePages,
			pInfo->TotalPagedPoolUsagePages
			);
		wprintf (L"\t\tTotal: %ld\t\t\tTotal: %ld\n",
			pInfo->PeakNonPagedPoolUsagePages,
			pInfo->TotalNonPagedPoolUsagePages
			);
		wprintf (L"\tPageFileUsage:\n");
		wprintf (L"\t\tPeak : %ld\n", pInfo->PeakPageFileUsageBytes);
		wprintf (L"\t\tTotal: %ld\n", pInfo->TotalPageFileUsageBytes);

		wprintf (L"\tPageFaultCount = %ld\n", pInfo->PageFaultCount);
		wprintf (L"\tTotalPrivateBytes = %ld\n", pInfo->TotalPrivateBytes);
		/* Threads */
		for (	ThreadIndex = 0;
			(ThreadIndex < pInfo->ThreadCount);
			ThreadIndex ++
			)
		{
			wprintf (L"\t%x in %x:\n",
				pInfo->ThreadSysInfo[ThreadIndex].ClientId.UniqueThread,
				pInfo->ThreadSysInfo[ThreadIndex].ClientId.UniqueProcess
				);
			PrintUtcDateTime (
				"\t\tKernelTime      = %s\n",
				& (pInfo->ThreadSysInfo[ThreadIndex].KernelTime)
				);
			PrintUtcDateTime (
				"\t\tUserTime        = %s\n",
				& (pInfo->ThreadSysInfo[ThreadIndex].UserTime)
				);
			PrintUtcDateTime (
				"\t\tCreateTime      = %s\n",
				& (pInfo->ThreadSysInfo[ThreadIndex].CreateTime)
				);
			wprintf (L"\t\tTickCount       = %ld\n",
				pInfo->ThreadSysInfo[ThreadIndex].TickCount
				);
			wprintf (L"\t\tStartEIP        = 0x%08x\n",
				pInfo->ThreadSysInfo[ThreadIndex].StartEIP
				);
			/* CLIENT_ID ClientId; */
			wprintf (L"\t\tDynamicPriority = %d\n",
				pInfo->ThreadSysInfo[ThreadIndex].DynamicPriority
				);
			wprintf (L"\t\tBasePriority    = %d\n",
				pInfo->ThreadSysInfo[ThreadIndex].BasePriority
				);
			wprintf (L"\t\tnSwitches       = %ld\n",
				pInfo->ThreadSysInfo[ThreadIndex].nSwitches
				);
			wprintf (L"\t\tState           = 0x%08x\n",
				pInfo->ThreadSysInfo[ThreadIndex].State
				);
			wprintf (L"\t\tWaitReason      = %ld\n",
				pInfo->ThreadSysInfo[ThreadIndex].WaitReason
				);
		}
		/* Next */
		if (0 == pInfo->RelativeOffset)
		{
			break;
		}
		(ULONG) pInfo += pInfo->RelativeOffset;
	}

	DumpData (Length, pInfo);

	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 6.
 */
CMD_DEF(ServiceDescriptorTable)
{
	NTSTATUS		Status;
	SYSTEM_SDT_INFORMATION	Info;
	ULONG			Length = 0;

/* FIXME */
	if (Application.Flag.Verbose)
	{
		printf ("SystemServiceDescriptorTableInfo:\n");
	}
	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			SystemServiceDescriptorTableInfo,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		DumpData (Length, & Info);
		return EXIT_FAILURE;
	}
	printf ("\tBufferLength                = %ld\n", Info.BufferLength);
	printf ("\tNumberOfSystemServiceTables = %ld\n", Info.NumberOfSystemServiceTables);
	printf ("\tNumberOfServices            = %ld\n", Info.NumberOfServices [0]);
	printf ("\tServiceCounters             = %ld\n", Info.ServiceCounters [0]);

	DumpData (Length, & Info);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 7.
 */
CMD_DEF(IoConfig)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_IOCONFIG_INFORMATION	Info;
	ULONG				Length = 0;

	if (Application.Flag.Verbose)
	{
		printf ("SystemIoConfigInformation:\n");
	}
	Status = NtQuerySystemInformation (
			SystemIoConfigInformation,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tDiskCount    : %ld\n", Info.DiskCount);
	printf ("\tFloppyCount  : %ld\n", Info.FloppyCount);
	printf ("\tCdRomCount   : %ld\n", Info.CdRomCount);
	printf ("\tTapeCount    : %ld\n", Info.TapeCount);
	printf ("\tSerialCount  : %ld\n", Info.SerialCount);
	printf ("\tParallelCount: %ld\n", Info.ParallelCount);

	DumpData (Length, & Info);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 8.
 */
CMD_DEF(ProcessorTime)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSORTIME_INFO	Info;
	ULONG				Length = 0;
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemProcessorTimeInformation:\n");
	}
	Status = NtQuerySystemInformation (
			SystemProcessorTimeInformation,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	PrintUtcDateTime ("\tTotalProcessorRunTime : %s\n", & Info.TotalProcessorRunTime);
	PrintUtcDateTime ("\tTotalProcessorTime    : %s\n", & Info.TotalProcessorTime);
	PrintUtcDateTime ("\tTotalProcessorUserTime: %s\n", & Info.TotalProcessorUserTime);
	PrintUtcDateTime ("\tTotalDPCTime          : %s\n", & Info.TotalDPCTime);
	PrintUtcDateTime ("\tTotalInterruptTime    : %s\n", & Info.TotalInterruptTime);
	printf ("\tTotalInterrupts       : %ld\n", Info.TotalInterrupts);
	if (Application.Flag.Verbose)
	{
		printf ("\tUnused                : %08x\n", Info.Unused);
	}

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 9.
 */
CMD_DEF(NtGlobalFlag)
{
	NTSTATUS		Status;
	SYSTEM_GLOBAL_FLAG_INFO	Info;
	ULONG			Length = 0;
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemNtGlobalFlagInformation:\n");
	}
	Status = NtQuerySystemInformation (
			SystemNtGlobalFlagInformation,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tNtGlobalFlag: %08x\n", Info.NtGlobalFlag);
	/* FIXME: decode each flag */

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 10.
 */
CMD_DEF(10)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 11.
 *
 * NOTE
 * 	Code originally in Yariv Kaplan's NtDriverList,
 * 	at http://www.internals.com/, adapted to ReactOS
 * 	structures layout.
 */
CMD_DEF(Module)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_MODULE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- -------- ---------------------------------------\n";


	if (Application.Flag.Verbose)
	{
		printf ("SystemModuleInformation:\n");
	}
	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			SystemModuleInformation,
			& pInfo,
			0, /* query size */
			& Length
			);
	if (STATUS_INFO_LENGTH_MISMATCH == Status)
	{
		/*
		 *	Allocate buffer
		 */
		pInfo = GlobalAlloc (GMEM_ZEROINIT, Length); 
		if (NULL == pInfo)
		{
			printf ("Could not allocate memory.\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	/*
	 *	Get module list from ntoskrnl.exe
	 */
	Status = NtQuerySystemInformation (
			SystemModuleInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("Index    Address  Size     Name\n");
	printf (hr);

	for (	Index = 0;
		(Index < (int) pInfo->Count);
		Index ++
		)
	{
		printf (
			"%8x %08x %8x %s\n",
			pInfo->Module[Index].ModuleEntryIndex,
			pInfo->Module[Index].ModuleBaseAddress,
			pInfo->Module[Index].ModuleSize,
			pInfo->Module[Index].ModuleName
			);
	}
	printf (hr);

	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 12.
 */
CMD_DEF(ResourceLock)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_RESOURCE_LOCK_INFO	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- -------- -------- -------- -------- ------------\n";

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check NULL==pInfo */

	if (Application.Flag.Verbose)
	{
		printf ("SystemResourceLockInformation:\n");
	}
	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			SystemResourceLockInformation,
			pInfo,
			BUFFER_SIZE_DEFAULT, /* query size */
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			/*
			 *	Allocate buffer
			 */
			pInfo = GlobalReAlloc (pInfo, Length, GMEM_ZEROINIT); 
			if (NULL == pInfo)
			{
				printf ("Could not allocate memory.\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			PrintStatus (Status);
			GlobalFree (pInfo);
			return EXIT_FAILURE;
		}
	}
	/*
	 *	Get locked resource list from ntoskrnl.exe
	 */
	Status = NtQuerySystemInformation (
			SystemResourceLockInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		GlobalFree (pInfo);
		return EXIT_FAILURE;
	}
	printf ("Address  Active # Content# Sh/Wait  Exc/Wait\n");
	printf (hr);

	for (	Index = 0;
		(Index < (int) pInfo->Count);
		Index ++
		)
	{
		printf (
			"%08x %8ld %8ld %8ld %8ld %08x\n",
			pInfo->Lock[Index].ResourceAddress,
			pInfo->Lock[Index].ActiveCount,
			pInfo->Lock[Index].ContentionCount,
			pInfo->Lock[Index].NumberOfSharedWaiters,
			pInfo->Lock[Index].NumberOfExclusiveWaiters,
			pInfo->Lock[Index].Unknown
			);
	}
	printf (hr);

	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 13.
 */
CMD_DEF(13)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 14.
 */
CMD_DEF(14)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 15.
 */
CMD_DEF(15)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 16. You can not pass 0 as the initial output buffer's
 * 	size to get back the needed buffer size.
 */
CMD_DEF(Handle)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_HANDLE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- ---- -------- -------- --------\n";
	CHAR				FlagsString [9] = {0};

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);

	if (Application.Flag.Verbose)
	{
		printf ("SystemHandleInformation:\n");
	}
	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			SystemHandleInformation,
			pInfo,
			BUFFER_SIZE_DEFAULT,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			/*
			 *	Allocate buffer
			 */
			pInfo = GlobalReAlloc (pInfo, Length, GMEM_ZEROINIT); 
			if (NULL == pInfo)
			{
				printf ("\tCould not allocate memory.\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			PrintStatus (Status);
			GlobalFree (pInfo);
			return EXIT_FAILURE;
		}
	}
	/*
	 *	Get handle table from ntoskrnl.exe
	 */
	Status = NtQuerySystemInformation (
			SystemHandleInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		GlobalFree (pInfo);
		return EXIT_FAILURE;
	}
	printf ("Handle   OwnerPID Type ObjPtr   Access   Flags\n");
	printf (hr);

	for (	Index = 0;
		(Index < (int) pInfo->Count);
		Index ++
		)
	{
		printf (
			"%8x %8x %4d %8x %8x %s\n",
			pInfo->Handle[Index].HandleValue,
			pInfo->Handle[Index].OwnerPid,
			pInfo->Handle[Index].ObjectType,
			pInfo->Handle[Index].ObjectPointer,
			pInfo->Handle[Index].AccessMask,
			ByteToBinaryString (
				pInfo->Handle[Index].HandleFlags,
				FlagsString
				)
			);
	}
	printf (hr);

	DumpData (Length, pInfo);

	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 17.
 */
CMD_DEF(Object)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 18.
 */
CMD_DEF(PageFile)
{
	NTSTATUS			Status;
	PSYSTEM_PAGEFILE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	
	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check pInfo */
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemPageFileInformation:\n");
	}
	Status = NtQuerySystemInformation(
			SystemPageFileInformation,
			pInfo,
			BUFFER_SIZE_DEFAULT,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			/*
			 *	Allocate buffer
			 */
			pInfo = GlobalReAlloc (pInfo, Length, GMEM_ZEROINIT); 
			if (NULL == pInfo)
			{
				printf ("Could not allocate memory.\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			PrintStatus (Status);
			GlobalFree (pInfo);
			return EXIT_FAILURE;
		}
	}
	Status = NtQuerySystemInformation (
			SystemPageFileInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		GlobalFree (pInfo);
		return EXIT_FAILURE;
	}

	while (1)
	{
		wprintf (L"\t\"%s\":\n", pInfo->PagefileFileName.Buffer);
		if (Application.Flag.Verbose)
		{
			wprintf (L"\t\tRelativeOffset   = %08x\n", pInfo->RelativeOffset);
		}
		wprintf (L"\t\tCurrentSizePages = %ld\n", pInfo->CurrentSizePages);
		wprintf (L"\t\tTotalUsedPages   = %ld\n", pInfo->TotalUsedPages);
		wprintf (L"\t\tPeakUsedPages    = %ld\n", pInfo->PeakUsedPages);

		if (0 == pInfo->RelativeOffset)
		{
			break;
		}
		printf ("\n");
		(ULONG) pInfo += pInfo->RelativeOffset;
	}

	DumpData (Length, pInfo);
	
	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 19.
 */
CMD_DEF(InstructionEmulation)
{
	NTSTATUS		Status;
	SYSTEM_VDM_INFORMATION	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemInstructionEmulationInfo:\n");
	}
	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			SystemInstructionEmulationInfo,
			& Info,
			sizeof Info,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tVdmSegmentNotPresentCount = %ld\n", Info.VdmSegmentNotPresentCount);
	printf ("\tVdmINSWCount              = %ld\n", Info.VdmINSWCount);
	printf ("\tVdmESPREFIXCount          = %ld\n", Info.VdmESPREFIXCount);
	printf ("\tVdmCSPREFIXCount          = %ld\n", Info.VdmCSPREFIXCount);
	printf ("\tVdmSSPREFIXCount          = %ld\n", Info.VdmSSPREFIXCount);
	printf ("\tVdmDSPREFIXCount          = %ld\n", Info.VdmDSPREFIXCount);
	printf ("\tVdmFSPREFIXCount          = %ld\n", Info.VdmFSPREFIXCount);
	printf ("\tVdmGSPREFIXCount          = %ld\n", Info.VdmGSPREFIXCount);
	printf ("\tVdmOPER32PREFIXCount      = %ld\n", Info.VdmOPER32PREFIXCount);
	printf ("\tVdmADDR32PREFIXCount      = %ld\n", Info.VdmADDR32PREFIXCount);
	printf ("\tVdmINSBCount              = %ld\n", Info.VdmINSBCount);
	printf ("\tVdmINSWV86Count           = %ld\n", Info.VdmINSWV86Count);
	printf ("\tVdmOUTSBCount             = %ld\n", Info.VdmOUTSBCount);
	printf ("\tVdmOUTSWCount             = %ld\n", Info.VdmOUTSWCount);
	printf ("\tVdmPUSHFCount             = %ld\n", Info.VdmPUSHFCount);
	printf ("\tVdmPOPFCount              = %ld\n", Info.VdmPOPFCount);
	printf ("\tVdmINTNNCount             = %ld\n", Info.VdmINTNNCount);
	printf ("\tVdmINTOCount              = %ld\n", Info.VdmINTOCount);
	printf ("\tVdmIRETCount              = %ld\n", Info.VdmIRETCount);
	printf ("\tVdmINBIMMCount            = %ld\n", Info.VdmINBIMMCount);
	printf ("\tVdmINWIMMCount            = %ld\n", Info.VdmINWIMMCount);
	printf ("\tVdmOUTBIMMCount           = %ld\n", Info.VdmOUTBIMMCount);
	printf ("\tVdmOUTWIMMCount           = %ld\n", Info.VdmOUTWIMMCount);
	printf ("\tVdmINBCount               = %ld\n", Info.VdmINBCount);
	printf ("\tVdmINWCount               = %ld\n", Info.VdmINWCount);
	printf ("\tVdmOUTBCount              = %ld\n", Info.VdmOUTBCount);
	printf ("\tVdmOUTWCount              = %ld\n", Info.VdmOUTWCount);
	printf ("\tVdmLOCKPREFIXCount        = %ld\n", Info.VdmLOCKPREFIXCount);
	printf ("\tVdmREPNEPREFIXCount       = %ld\n", Info.VdmREPNEPREFIXCount);
	printf ("\tVdmREPPREFIXCount         = %ld\n", Info.VdmREPPREFIXCount);
	printf ("\tVdmHLTCount               = %ld\n", Info.VdmHLTCount);
	printf ("\tVdmCLICount               = %ld\n", Info.VdmCLICount);
	printf ("\tVdmSTICount               = %ld\n", Info.VdmSTICount);
	printf ("\tVdmBopCount               = %ld\n", Info.VdmBopCount);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 20.
 */
CMD_DEF(20)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 21.
 */
CMD_DEF(Cache)
{
	NTSTATUS			Status;
	SYSTEM_CACHE_INFORMATION	Si;
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemCacheInformation:\n");
	}
	RtlZeroMemory (
		(PVOID) & Si,
		sizeof Si
		);
	Status = NtQuerySystemInformation (
			SystemCacheInformation,
			& Si,
			sizeof Si,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tSize:\n");
	printf ("\t\tCurrent = %ld\n", Si.CurrentSize);
	printf ("\t\tPeak    = %ld\n\n", Si.PeakSize);
	printf ("\tPageFaults:\n\t\tCount   = %ld\n\n", Si.PageFaultCount);
	printf ("\tWorking Set:\n");
	printf ("\t\tMinimum = %ld\n", Si.MinimumWorkingSet );
	printf ("\t\tMaximum = %ld\n", Si.MaximumWorkingSet );

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 * 	Get statistic data about tagged pools. Not implemented in the
 * 	free build.
 *
 * NOTE
 * 	Class 22.
 */
CMD_DEF(PoolTag)
{
	NTSTATUS		Status;
	PSYSTEM_POOL_TAG_INFO	pInfo = NULL;
	ULONG			Length;
	ULONG			PoolIndex;

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check pInfo */
	
	if (Application.Flag.Verbose)
	{
		printf ("SystemPoolTagInformation:\n");
	}
	Status = NtQuerySystemInformation(
			SystemPoolTagInformation,
			pInfo,
			BUFFER_SIZE_DEFAULT,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			/*
			 *	Allocate buffer
			 */
			pInfo = GlobalReAlloc (pInfo, Length, GMEM_ZEROINIT); 
			if (NULL == pInfo)
			{
				printf ("Could not allocate memory.\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			PrintStatus (Status);
			GlobalFree (pInfo);
			return EXIT_FAILURE;
		}
	}
	Status = NtQuerySystemInformation (
			SystemPoolTagInformation,
			pInfo,
			Length,
			& Length
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		GlobalFree (pInfo);
		return EXIT_FAILURE;
	}

	for (	PoolIndex = 0;
		(PoolIndex < pInfo->Count);
		PoolIndex ++
		)
	{
		wprintf (L"\t%08x:\n", pInfo->PoolEntry[PoolIndex].Tag);
		wprintf (L"\t\tPaged:\t\tNon Paged:\n");
		wprintf (
			L"\t\tAllocationCount = %ld\tAllocationCount = %ld\n",
			pInfo->PoolEntry[PoolIndex].Paged.AllocationCount,
			pInfo->PoolEntry[PoolIndex].NonPaged.AllocationCount
			);
		wprintf (
			L"\t\tFreeCount       = %ld\tFreeCount       = %ld\n",
			pInfo->PoolEntry[PoolIndex].Paged.FreeCount,
			pInfo->PoolEntry[PoolIndex].NonPaged.FreeCount
			);
		wprintf (
			L"\t\tSizeBytes       = %ld\tSizeBytes       = %ld\n",
			pInfo->PoolEntry[PoolIndex].Paged.SizeBytes,
			pInfo->PoolEntry[PoolIndex].NonPaged.SizeBytes
			);
	}

	DumpData (Length, pInfo);
	
	GlobalFree (pInfo);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 23.
 */
CMD_DEF(ProcessorSchedule)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSOR_SCHEDULE_INFO	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemProcessorScheduleInfo:\n");
	}
	RtlZeroMemory (
		& Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			SystemProcessorScheduleInfo,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}

	printf ("\tnContextSwitches = %ld\n", Info.nContextSwitches);
	printf ("\tnDPCQueued       = %ld\n", Info.nDPCQueued);
	printf ("\tnDPCRate         = %ld\n", Info.nDPCRate);
	printf ("\tTimerResolution  = %ld\n", Info.TimerResolution);
	printf ("\tnDPCBypasses     = %ld\n", Info.nDPCBypasses);
	printf ("\tnAPCBypasses     = %ld\n", Info.nAPCBypasses);
		
	DumpData (sizeof Info, & Info);

	return EXIT_SUCCESS;

}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 24.
 */
CMD_DEF(Dpc)
{
	NTSTATUS		Status;
	SYSTEM_DPC_INFORMATION	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemDpcInformation:\n");
	}
	RtlZeroMemory (
		& Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			SystemDpcInformation,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}

	if (Application.Flag.Verbose)
	{
		printf ("\tUnused                 = %ld\n", Info.Unused);
	}
	printf ("\tKiMaximumDpcQueueDepth = %ld\n", Info.KiMaximumDpcQueueDepth);
	printf ("\tKiMinimumDpcRate       = %ld\n", Info.KiMinimumDpcRate);
	printf ("\tKiAdjustDpcThreshold   = %ld\n", Info.KiAdjustDpcThreshold);
	printf ("\tKiIdealDpcRate         = %ld\n", Info.KiIdealDpcRate);

	DumpData (sizeof Info, & Info);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 25.
 */
CMD_DEF(25)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 26.
 */
INT CMD_LoadImage (INT argc, LPCSTR argv [])
CMD_NOT_IMPLEMENTED

/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 27.
 */
CMD_DEF(UnloadImage)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 28.
 */
CMD_DEF(TimeAdjustment)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_TIME_ADJUSTMENT_INFO	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemTimeAdjustmentInformation:\n");
	}
	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			SystemTimeAdjustmentInformation,
			& Info,
			sizeof Info,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tKeTimeAdjustment      = %ld\n", Info.KeTimeAdjustment);
	printf ("\tKeMaximumIncrement    = %ld\n", Info.KeMaximumIncrement);
	printf ("\tKeTimeSynchronization = %s\n", TF(Info.KeTimeSynchronization));

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 29.
 */
CMD_DEF(29)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 30.
 */
CMD_DEF(30)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 31.
 */
CMD_DEF(31)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 32.
 */
CMD_DEF(CrashDumpSection)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 33.
 */
CMD_DEF(ProcessorFaultCount)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 34.
 */
CMD_DEF(CrashDumpState)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 35.
 */
CMD_DEF(Debugger)
{
	NTSTATUS		Status;
	SYSTEM_DEBUGGER_INFO	Info;

	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			SystemDebuggerInformation,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tKdDebuggerEnabled = %s\n", TF(Info.KdDebuggerEnabled));
	printf ("\tKdDebuggerPresent = %s\n", TF(Info.KdDebuggerPresent));

	DumpData (sizeof Info, & Info);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 36.
 */
CMD_DEF(ThreadSwitchCounters)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 37.
 */
CMD_DEF(Quota)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_QUOTA_INFORMATION	Info;

	if (Application.Flag.Verbose)
	{
		printf ("SystemQuotaInformation:\n");
	}
	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			SystemQuotaInformation,
			& Info,
			sizeof Info,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("\tCmpGlobalQuota           = %ld\n", Info.CmpGlobalQuota);
	printf ("\tCmpGlobalQuotaUsed       = %ld\n", Info.CmpGlobalQuotaUsed);
	printf ("\tMmSizeofPagedPoolInBytes = %ld\n", Info.MmSizeofPagedPoolInBytes);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 38.
 */
CMD_DEF(LoadDriver)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 39.
 */
CMD_DEF(PrioritySeparation)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 40.
 */
CMD_DEF(40)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 41.
 */
CMD_DEF(41)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 42.
 */
CMD_DEF(42)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 43.
 */
CMD_DEF(43)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *	Dump the system TIME_ZONE_INFORMATION object.
 *
 * NOTE
 * 	Class 44.
 */
CMD_DEF(TimeZone)
{
#if 0
	NTSTATUS			Status;
	TIME_ZONE_INFORMATION		Tzi;
	WCHAR				Name [33];

	if (Application.Flag.Verbose)
	{
		printf ("SystemTimeZoneInformation:\n");
	}
	RtlZeroMemory (& Tzi, sizeof Tzi);
	Status = NtQuerySystemInformation(
			SystemTimeZoneInformation,
			& Tzi,
			sizeof Tzi,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf (
		"12h/24h.....: %dh\n",
		0  /* FIXME: */
		);
	printf (
		"Bias........: %d'\n",
		Tzi.Bias /* LONG */
		);
	
	printf ("Standard\n");
	RtlZeroMemory (
		(PVOID) Name,
		sizeof Name
		);
	lstrcpynW (
		Name,
		Tzi.StandardName, /* WCHAR [32] */
		32
		);
	wprintf (
		L"\tName: \"%s\"\n",
		Name
		);
		
	PrintUtcDateTime (
		"\tDate: %s\n",
		& Tzi.StandardDate	/* SYSTEMTIME */
		);
		
	printf ("\tBias: %d'\n",
		Tzi.StandardBias /* LONG */
		);
	
	printf ("Daylight\n");
	RtlZeroMemory (
		(PVOID) Name,
		sizeof Name
		);
	lstrcpynW (
		Name,
		Tzi.DaylightName, /* WCHAR [32] */ 
		32
		);
	wprintf (
		L"\tName: \"%s\"\n",
		Name
		);
		
	PrintUtcDateTime (
		"\tDate: %s\n",
		& Tzi.DaylightDate /* SYSTEMTIME */ 
		);
		
	printf (
		"\tBias: %d'\n",
		Tzi.DaylightBias /* LONG */ 
		);

#endif
	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 45.
 */
CMD_DEF(Lookaside)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 *	Miscellanea Commands
 **********************************************************************/

CMD_DEF(ver)
{
	INT	Total = 0;
	
	Total =
	printf (
	"ReactOS Operating System - http://www.reactos.com/\n"
	"QSI - Query System Information (compiled on %s, %s)\n"
	"Copyright (c) 1999, 2000 Emanuele Aliberti et alii\n\n",
	__DATE__, __TIME__
	);

	if (Application.Flag.Verbose)
	{
	Total +=
	printf (
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation; either version 2 of the License, or\n"
	"(at your option) any later version.\n\n"

	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n\n"

	"You should have received a copy of the GNU General Public License\n"
	"along with this program; if not, write to the Free Software\n"
	"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
	"(See also http://www.fsf.org/).\n"
	);
	}
	return (Total);
}


CMD_DEF(exit)
{
	Application.Active = FALSE;
	return EXIT_SUCCESS;
}


extern COMMAND_DESCRIPTOR Commands [];

CMD_DEF(help)
{
	int i;
	
	if (Application.Flag.Verbose)
	{
		printf ("Commands:\n");
	}
	for (	i = 0;
		(NULL != Commands[i].Name);
		i ++
		)
	{
		printf (
			(strlen (Commands[i].Name) > 7)
				? "%s\t: %s\n"
				: "%s\t\t: %s\n",
			Commands[i].Name,
			Commands[i].Description
			);
	}
	return EXIT_SUCCESS;
}


CMD_DEF(credits)
{
	return
	printf (
		"\nReactOS (http://www.reactos.com/):\n"
		"\tEmanuele Aliberti\n"
		"\tEric Kohl\n\n"
		
		"HandleEx:\n"
		"\tMark Russinovich (http://www.sysinternals.com/)\n\n"

		"NtDriverList:\n"
		"\tYariv Kaplan (http://www.internals.com/)\n\n"

		"Undocumented SYSTEM_POOL_INFORMATION:\n"
		"\tKlaus P. Gerlicher\n\n"
		
		"Undocumented Windows NT:\n"
		"\tPrasad Dabak, Sandeep Phadke, and Milind Borate\n\n"

		"Windows NT/2000 Native API Reference:\n"
		"\tGary Nebbett\n\n"

		"comp.os.ms-windows.programmer.nt.kernel-mode\n"
		"\t(many postings with sample code)\n"
		);
}


CMD_DEF(verbose)
{
	Application.Flag.Verbose = ~Application.Flag.Verbose;
	return printf (
		"Verbose mode is %s.\n",
		ONOFF(Application.Flag.Verbose)
		);
}


CMD_DEF(dump)
{
	Application.Flag.Dump = ~Application.Flag.Dump;
	return printf (
		"Dump mode is %s.\n",
		ONOFF(Application.Flag.Dump)
		);
}


/**********************************************************************
 *	Commands table
 **********************************************************************/

COMMAND_DESCRIPTOR
Commands [] =
{
/* System information classes */
	
	{						/* 0  Q  */
		"basic",
		CMD_REF(Basic),
		"Basic system information"
	},
	{						/* 1  Q  */
		"processor",	
		CMD_REF(Processor),
		"Processor characteristics"
	},
	{						/* 2  Q  */
		"perf",	
		CMD_REF(Performance),
		"System performance data"
	},
	{						/* 3  Q  */
		"time",
		CMD_REF(Time),
		"System times"
	},
	{						/* 4  Q  */
		"path",
		CMD_REF(Path),
		"unknown"
	},
	{						/* 5  Q  */
		"process",
		CMD_REF(Process),
		"Process & thread tables"
	},
	{						/* 6  Q  */
		"sdt",
		CMD_REF(ServiceDescriptorTable),
		"Service descriptor table (SDT)"
	},
	{						/* 7  Q  */
		"ioconfig",
		CMD_REF(IoConfig),
		"I/O devices in the system, by class"
	},
	{						/* 8  Q  */
		"proctime",
		CMD_REF(ProcessorTime),
		"Print processor times"
	},
	{						/* 9  QS */
		"flag",
		CMD_REF(NtGlobalFlag),
		"Print the system wide flags"
	},
	{						/* 10    */
		"#10",
		CMD_REF(10),
		"UNKNOWN"
	},
	{						/* 11 Q  */
		"module",
		CMD_REF(Module),
		"Table of kernel modules"
	},
	{						/* 12 Q  */
		"reslock",
		CMD_REF(ResourceLock),
		"Table of locks on resources"
	},
	{						/* 13    */
		"#13",
		CMD_REF(13),
		"UNKNOWN"
	},
	{						/* 14    */
		"#14",
		CMD_REF(14),
		"UNKNOWN"
	},
	{						/* 15    */
		"#15",
		CMD_REF(15),
		"UNKNOWN"
	},
	{						/* 16 Q  */
		"handle",
		CMD_REF(Handle),
		"Table of handles (Ps Manager)"
	},
	{						/* 17 Q  */
		"object",
		CMD_REF(Object),
		"Table of objects (Ob Manager)"
	},
	{						/* 18 Q  */
		"pagefile",
		CMD_REF(PageFile),
		"Virtual memory paging files (Cc Subsystem)"
	},
	{						/* 19 Q  */
		"emulation",
		CMD_REF(InstructionEmulation),
		"Virtual DOS Machine instruction emulation (VDM)"
	},
	{						/* 20    */
		"#20",
		CMD_REF(20),
		"UNKNOWN"
	},
	{						/* 21 QS */
		"cache",	
		CMD_REF(Cache),
		"Cache Manager Status"
	},
	{						/* 22 Q  */
		"pooltag",
		CMD_REF(PoolTag),
		"Tagged pools statistics (checked build only)"
	},
	{						/* 23 Q  */
		"procsched",
		CMD_REF(ProcessorSchedule),
		"Processor schedule information"
	},
	{						/* 24 QS */
		"dpc",
		CMD_REF(Dpc),
		"Deferred procedure call (DPC)"
	},
	{						/* 25    */
		"#25",
		CMD_REF(25),
		"UNKNOWN"
	},
	{						/* 26 S (callable) */
		"loadpe",
		CMD_REF(LoadImage),
		"Load a kernel mode DLL (in PE format)"
	},
	{						/* 27 S (callable) */
		"unloadpe",
		CMD_REF(UnloadImage),
		"Unload a kernel mode DLL (module)"
	},
	{						/* 28 QS */
		"timeadj",
		CMD_REF(TimeAdjustment),
		"Time adjustment"
	},
	{						/* 29    */
		"#29",
		CMD_REF(29),
		"UNKNOWN"
	},
	{						/* 30    */
		"#30",
		CMD_REF(30),
		"UNKNOWN"
	},
	{						/* 31    */
		"#31",
		CMD_REF(31),
		"UNKNOWN"
	},
	{						/* 32 Q  */
		"crashsect",
		CMD_REF(CrashDumpSection),
		"Crash Dump Section"
	},
	{						/* 33 Q  */
		"procfault",
		CMD_REF(ProcessorFaultCount),
		"Processor fault count"
	},
	{						/* 34 Q  */
		"crashstate",
		CMD_REF(CrashDumpState),
		"Crash Dump State"
	},
	{						/* 35 Q  */
		"debugger",
		CMD_REF(Debugger),
		"System debugger"
	},
	{						/* 36 Q  */
		"threadsw",
		CMD_REF(ThreadSwitchCounters),
		"Thread switch counters"
	},
	{						/* 37 QS */
		"quota",
		CMD_REF(Quota),
		"System quota values"
	},
	{						/* 38 S  */
		"loaddrv",
		CMD_REF(LoadDriver),
		"Load kernel driver (SYS)"
	},
	{						/* 39 S  */
		"prisep",
		CMD_REF(PrioritySeparation),
		"Priority Separation"
	},
	{						/* 40    */
		"#40",
		CMD_REF(40),
		"UNKNOWN"
	},
	{						/* 41    */
		"#41",
		CMD_REF(41),
		"UNKNOWN"
	},
	{						/* 42    */
		"#42",
		CMD_REF(42),
		"UNKNOWN"
	},
	{						/* 43    */
		"#43",
		CMD_REF(43),
		"UNKNOWN"
	},
	{						/* 44 QS */
		"tz",
		CMD_REF(TimeZone),
		"Time zone (TZ) information"
	},
	{						/* 45 Q  */
		"lookaside",
		CMD_REF(Lookaside),
		"Lookaside"
	},
/* User commands */
	{
		"?",
		CMD_REF(help),
		"Same as 'help'"
	},
	{
		"help",
		CMD_REF(help),
		"Print this command directory"
	},
	{
		"credits",
		CMD_REF(credits),
		"Print the list of people and sources that made QSI possible"
	},
	{
		"ver",
		CMD_REF(ver),
		"Print version number and license information"
	},
	{
		"exit",
		CMD_REF(exit),
		"Exit to operating system"
	},
	{
		"dump",
		CMD_REF(dump),
		"Enable/disable dumping raw data returned by system"
	},
	{
		"verbose",
		CMD_REF(verbose),
		"Enable/disable printing unused, unknown, and service fields"
	},
	
	{ NULL, NULL }
};



/* user input --> command decoder */


COMMAND_CALL
DecodeCommand (LPCSTR Command)
{
	int i;

	for (	i = 0;
		(	Commands[i].Name
			&& stricmp (Commands[i].Name,Command)
			);
		++i
		);
	return Commands[i].EntryPoint;
}

INT
ParseCommandLine (
	LPCSTR	CommandLine,
	LPCSTR	CommandArgv []
	)
{
	INT	ArgC = 0;
	LPCSTR	Separators = " \t";

	for (	CommandArgv [ArgC] = strtok ((char*)CommandLine, (char*)Separators);
		(ArgC < ARGV_SIZE);
		CommandArgv [ArgC] = (LPCSTR) strtok (NULL, (char*)Separators)
		)
	{
		if (NULL == CommandArgv [ArgC++])
		{
			break;
		}
	}
	return (ArgC);
}


int
main (int argc, char * argv [])
{
	CHAR	CommandLine [_MAX_PATH];

	INT	CommandArgc;
	LPCSTR	CommandArgv [ARGV_SIZE];

	/*
	 * Initialize rt data.
	 */
	Application.Heap = GetProcessHeap ();
	Application.Active = TRUE;
	/*
	 *  r-e-p loop.
	 */
	while (Application.Active)
	{
		/* Print the prompt string. */
		if (! Application.Flag.Batch)
		{
			printf ("\r\nsystem> ");
		}
		/* Read user command. */
		gets (CommandLine);
		/* Parse the user command */
		CommandArgc = ParseCommandLine (
				CommandLine,
				CommandArgv
				);
		if (0 != CommandArgc)
		{
			COMMAND_CALL	CommandCall = NULL;

			/* decode */
			if ((CommandCall = DecodeCommand (CommandArgv[0])))
			{
				/* execute */
				Application.ExitCode =
					CommandCall (
						CommandArgc,
						CommandArgv
						);
			}
			else
			{
				printf ("Unknown command (type help for a list of valid commands).\n");
			}
		}
		
	}
	if (! Application.Flag.Batch)
	{
		printf ("Bye\n");
	}
	return (EXIT_SUCCESS);
}

/* EOF */
