/* $Id: qsi.c,v 1.5 2001/01/14 19:59:43 ea Exp $
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
 * 	2001-01-13 (ea)
 * 		New QSI class names used by E.Kohl.
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

/* --- */

static
LPTSTR
KernelObjectName [] =
{
	_T("0"),		/* FIXME */
	_T("1"),		/* FIXME */
	_T("Directory"),
	_T("SymbolicLink"),
	_T("Token"),
	_T("Process"),
	_T("Thread"),
	_T("Event"),
	_T("8"),		/* FIXME */
	_T("Mutant"),
	_T("Semaphore"),
	_T("Timer"),
	_T("12"),		/* FIXME */
	_T("WindowStation"),
	_T("Desktop"),
	_T("Section"),
	_T("Key"),
	_T("Port"),
	_T("18"),		/* FIXME */
	_T("19"),		/* FIXME */
	_T("20"),		/* FIXME */
	_T("21"),		/* FIXME */
	_T("IoCompletion"),
	_T("File"),
	NULL
};


LPTSTR
STDCALL
HandleTypeToObjectName (
	DWORD	HandleType
	)
{
	if (HandleType > 23)	/* FIXME: use a symbol not a literal */
	{
		return _T("Unknown");
	}
	return KernelObjectName [HandleType];
}


/* --- */

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
CMD_DEF(0)
{
	NTSTATUS			Status;
	SYSTEM_BASIC_INFORMATION	Info;

	RtlZeroMemory (
		(PVOID) & Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			0,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("  Reserved                     0x%08x\n", Info.Reserved);
	printf ("  TimerResolution              %d\n",    Info.TimerResolution);
	printf ("  PageSize                     %d\n",    Info.PageSize);
	printf ("  NumberOfPhysicalPages        %d\n",    Info.NumberOfPhysicalPages);
	printf ("  LowestPhysicalPageNumber     %d\n",    Info.LowestPhysicalPageNumber);
	printf ("  HighestPhysicalPageNumber    %d\n",    Info.HighestPhysicalPageNumber);
	printf ("  AllocationGranularity        %d\n", Info.AllocationGranularity);
	printf ("  MinimumUserModeAddress       0x%08x (%d)\n", Info.MinimumUserModeAddress, Info.MinimumUserModeAddress);
	printf ("  MaximumUserModeAddress       0x%08x (%d)\n", Info.MaximumUserModeAddress, Info.MaximumUserModeAddress);
	printf ("  ActiveProcessorsAffinityMask 0x%08x\n", Info.ActiveProcessorsAffinityMask);
	printf ("  NumberOfProcessors           %d\n",   (int) Info.NumberOfProcessors);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 1.
 */
CMD_DEF(1)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSOR_INFORMATION	Info;
	
	RtlZeroMemory (
		(PVOID) & Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			1,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("  ProcessorArchitecture %d\n",    Info.ProcessorArchitecture);
	printf ("  ProcessorLevel        %d\n",    Info.ProcessorLevel);
	printf ("  ProcessorRevision     %d\n",    Info.ProcessorRevision);
	printf ("  Reserved              0x%08x\n", Info.Reserved);
	printf ("  FeatureBits           %08x\n",   Info.ProcessorFeatureBits);
	/* FIXME: decode feature bits */

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
CMD_DEF(2)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_PERFORMANCE_INFO	Info;
	LONG				Length = 0;


	Status = NtQuerySystemInformation (
			2,
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
CMD_DEF(3)
{
	NTSTATUS			Status;
	SYSTEM_TIMEOFDAY_INFORMATION	Info;

	Status = NtQuerySystemInformation (
			3,
			& Info,
			sizeof Info,
			NULL
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	PrintUtcDateTime ("  BootTime     %s\n", (PTIME) & Info.BootTime);
	PrintUtcDateTime ("  CurrentTime  %s\n", (PTIME) & Info.CurrentTime);
	PrintUtcDateTime ("  TimeZoneBias %s\n", (PTIME) & Info.TimeZoneBias); /* FIXME */
	printf           ("  TimeZoneId   %ld\n", Info.TimeZoneId);
	printf           ("  Reserved     %08x\n", Info.Reserved);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 4.
 */
CMD_DEF(4)
{
	NTSTATUS		Status;
	SYSTEM_PATH_INFORMATION	Info;
	CHAR			_Info [_MAX_PATH];
	ULONG			Length = 0;

	RtlZeroMemory (& Info, _MAX_PATH);
	Status = NtQuerySystemInformation (
			4,
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
CMD_DEF(5)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_PROCESS_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	ULONG				ThreadIndex;

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check NULL==pInfo */
	
	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			5,
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
CMD_DEF(6)
{
	NTSTATUS		Status;
	SYSTEM_SDT_INFORMATION	Info;
	ULONG			Length = 0;

/* FIXME */
	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			6,
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
	printf ("  BufferLength                = %ld\n", Info.BufferLength);
	printf ("  NumberOfSystemServiceTables = %ld\n", Info.NumberOfSystemServiceTables);
	printf ("  NumberOfServices            = %ld\n", Info.NumberOfServices [0]);
	printf ("  ServiceCounters             = %ld\n", Info.ServiceCounters [0]);

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
CMD_DEF(7)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_DEVICE_INFORMATION	Info;
	ULONG				Length = 0;

	Status = NtQuerySystemInformation (
			7,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("  Number Of Disks          %ld\n", Info.NumberOfDisks);
	printf ("  Number Of Floppies       %ld\n", Info.NumberOfFloppies);
	printf ("  Number Of CD-ROMs        %ld\n", Info.NumberOfCdRoms);
	printf ("  Number Of Tapes          %ld\n", Info.NumberOfTapes);
	printf ("  Number Of Serial Ports   %ld\n", Info.NumberOfSerialPorts);
	printf ("  Number Of Parallel Ports %ld\n", Info.NumberOfParallelPorts);

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
CMD_DEF(8)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSORTIME_INFO	Info;
	ULONG				Length = 0;
	
	Status = NtQuerySystemInformation (
			8,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	PrintUtcDateTime ("  TotalProcessorRunTime : %s\n", & Info.TotalProcessorRunTime);
	PrintUtcDateTime ("  TotalProcessorTime    : %s\n", & Info.TotalProcessorTime);
	PrintUtcDateTime ("  TotalProcessorUserTime: %s\n", & Info.TotalProcessorUserTime);
	PrintUtcDateTime ("  TotalDPCTime          : %s\n", & Info.TotalDPCTime);
	PrintUtcDateTime ("  TotalInterruptTime    : %s\n", & Info.TotalInterruptTime);
	printf           ("  TotalInterrupts       : %ld\n", Info.TotalInterrupts);
	printf           ("  Unused                : %08x\n", Info.Unused);

	return EXIT_SUCCESS;
}


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 9.
 */
CMD_DEF(9)
{
	NTSTATUS			Status;
	SYSTEM_FLAGS_INFORMATION	Info;
	ULONG				Length = 0;
	
	Status = NtQuerySystemInformation (
			9,
			& Info,
			sizeof Info,
			& Length
			);
	if (STATUS_SUCCESS != Status)
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("  NtGlobalFlag: %08x\n", Info.Flags);
	if (FLG_STOP_ON_EXCEPTION & Info.Flags) printf ("\tSTOP_ON_EXCEPTION\n");
	if (FLG_STOP_ON_HANG_GUI & Info.Flags) printf ("\tSTOP_ON_HANG_GUI\n");
	if (FLG_SHOW_LDR_SNAPS & Info.Flags) printf ("\tSHOW_LDR_SNAPS\n");
	if (FLG_DEBUG_INITIAL_COMMAND & Info.Flags) printf ("\tDEBUG_INITIAL_COMMAND\n");
	if (FLG_HEAP_ENABLE_TAIL_CHECK & Info.Flags) printf ("\tHEAP_ENABLE_TAIL_CHECK\n");
	if (FLG_HEAP_ENABLE_FREE_CHECK & Info.Flags) printf ("\tHEAP_ENABLE_FREE_CHECK\n");
	if (FLG_HEAP_ENABLE_TAGGING & Info.Flags) printf ("\tHEAP_ENABLE_TAGGING\n");
	if (FLG_HEAP_ENABLE_TAG_BY_DLL & Info.Flags) printf ("\tHEAP_ENABLE_TAG_BY_DLL\n");
	if (FLG_HEAP_ENABLE_CALL_TRACING & Info.Flags) printf ("\tHEAP_ENABLE_CALL_TRACING\n");
	if (FLG_HEAP_DISABLE_COALESCING & Info.Flags) printf ("\tHEAP_DISABLE_COALESCING\n");
	if (FLG_HEAP_VALIDATE_PARAMETERS & Info.Flags) printf ("\tHEAP_VALIDATE_PARAMETERS\n");
	if (FLG_HEAP_VALIDATE_ALL & Info.Flags) printf ("\tHEAP_VALIDATE_ALL\n");
	if (FLG_POOL_ENABLE_TAIL_CHECK & Info.Flags) printf ("\tPOOL_ENABLE_TAIL_CHECK\n");
	if (FLG_POOL_ENABLE_FREE_CHECK & Info.Flags) printf ("\tPOOL_ENABLE_FREE_CHECK\n");
	if (FLG_POOL_ENABLE_TAGGING & Info.Flags) printf ("\tPOOL_ENABLE_TAGGING\n");
	if (FLG_USER_STACK_TRACE_DB & Info.Flags) printf ("\tUSER_STACK_TRACE_DB\n");
	if (FLG_KERNEL_STACK_TRACE_DB & Info.Flags) printf ("\tKERNEL_STACK_TRACE_DB\n");
	if (FLG_MAINTAIN_OBJECT_TYPELIST & Info.Flags) printf ("\tMAINTAIN_OBJECT_TYPELIST\n");
	if (FLG_IGNORE_DEBUG_PRIV & Info.Flags) printf ("\tIGNORE_DEBUG_PRIV\n");
	if (FLG_ENABLE_CSRDEBUG & Info.Flags) printf ("\tENABLE_CSRDEBUG\n");
	if (FLG_ENABLE_KDEBUG_SYMBOL_LOAD & Info.Flags) printf ("\tENABLE_KDEBUG_SYMBOL_LOAD\n");
	if (FLG_DISABLE_PAGE_KERNEL_STACKS & Info.Flags) printf ("\tDISABLE_PAGE_KERNEL_STACKS\n");
	if (FLG_ENABLE_CLOSE_EXCEPTION & Info.Flags) printf ("\tENABLE_CLOSE_EXCEPTION\n");
	if (FLG_ENABLE_EXCEPTION_LOGGING & Info.Flags) printf ("\tENABLE_EXCEPTION_LOGGING\n");
	if (FLG_ENABLE_DBGPRINT_BUFFERING & Info.Flags) printf ("\tENABLE_DBGPRINT_BUFFERING\n");
	if (FLG_UNKNOWN_01000000 & Info.Flags) printf ("\tUNKNOWN_01000000\n");
	if (FLG_UNKNOWN_02000000 & Info.Flags) printf ("\tUNKNOWN_02000000\n");
	if (FLG_UNKNOWN_04000000 & Info.Flags) printf ("\tUNKNOWN_04000000\n");
	if (FLG_UNKNOWN_10000000 & Info.Flags) printf ("\tUNKNOWN_10000000\n");
	if (FLG_UNKNOWN_20000000 & Info.Flags) printf ("\tUNKNOWN_20000000\n");
	if (FLG_UNKNOWN_40000000 & Info.Flags) printf ("\tUNKNOWN_40000000\n");
	if (FLG_UNKNOWN_80000000 & Info.Flags) printf ("\tUNKNOWN_80000000\n");

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
CMD_DEF(11)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_MODULE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- -------- ---------------------------------------\n";


	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			11,
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
			11,
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
CMD_DEF(12)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_RESOURCE_LOCK_INFO	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- -------- -------- -------- -------- ------------\n";

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check NULL==pInfo */

	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			12,
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
			12,
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
CMD_DEF(16)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PSYSTEM_HANDLE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	INT				Index;
	const PCHAR			hr =
		"-------- -------- -------- -------- -------- ----------\n";
	CHAR				FlagsString [9] = {0};

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);

	/*
	 *	Obtain required buffer size
	 */
	Status = NtQuerySystemInformation (
			16,
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
			16,
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
	printf ("Handle   OwnerPID ObjPtr   Access   Flags    Type\n");
	printf (hr);

	for (	Index = 0;
		(Index < (int) pInfo->Count);
		Index ++
		)
	{
		printf (
			"%8x %8x %8x %8x %s %s\n",
			pInfo->Handle[Index].HandleValue,
			pInfo->Handle[Index].OwnerPid,
			pInfo->Handle[Index].ObjectPointer,
			pInfo->Handle[Index].AccessMask,
			ByteToBinaryString (
				pInfo->Handle[Index].HandleFlags,
				FlagsString
				),
			HandleTypeToObjectName (pInfo->Handle[Index].ObjectType)
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
CMD_DEF(17)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 18.
 */
CMD_DEF(18)
{
	NTSTATUS			Status;
	PSYSTEM_PAGEFILE_INFORMATION	pInfo = NULL;
	LONG				Length = 0;
	
	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check pInfo */
	
	Status = NtQuerySystemInformation(
			18,
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
			18,
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
		wprintf (L"  \"%s\":\n", pInfo->PagefileFileName.Buffer);
		wprintf (L"\tRelativeOffset   %08x\n", pInfo->RelativeOffset);
		wprintf (L"\tCurrentSizePages %ld\n", pInfo->CurrentSizePages);
		wprintf (L"\tTotalUsedPages   %ld\n", pInfo->TotalUsedPages);
		wprintf (L"\tPeakUsedPages    %ld\n", pInfo->PeakUsedPages);

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
CMD_DEF(19)
{
	NTSTATUS		Status;
	SYSTEM_VDM_INFORMATION	Info;

	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			19,
			& Info,
			sizeof Info,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		PrintStatus (Status);
		return EXIT_FAILURE;
	}
	printf ("  VdmSegmentNotPresentCount = %ld\n", Info.VdmSegmentNotPresentCount);
	printf ("  VdmINSWCount              = %ld\n", Info.VdmINSWCount);
	printf ("  VdmESPREFIXCount          = %ld\n", Info.VdmESPREFIXCount);
	printf ("  VdmCSPREFIXCount          = %ld\n", Info.VdmCSPREFIXCount);
	printf ("  VdmSSPREFIXCount          = %ld\n", Info.VdmSSPREFIXCount);
	printf ("  VdmDSPREFIXCount          = %ld\n", Info.VdmDSPREFIXCount);
	printf ("  VdmFSPREFIXCount          = %ld\n", Info.VdmFSPREFIXCount);
	printf ("  VdmGSPREFIXCount          = %ld\n", Info.VdmGSPREFIXCount);
	printf ("  VdmOPER32PREFIXCount      = %ld\n", Info.VdmOPER32PREFIXCount);
	printf ("  VdmADDR32PREFIXCount      = %ld\n", Info.VdmADDR32PREFIXCount);
	printf ("  VdmINSBCount              = %ld\n", Info.VdmINSBCount);
	printf ("  VdmINSWV86Count           = %ld\n", Info.VdmINSWV86Count);
	printf ("  VdmOUTSBCount             = %ld\n", Info.VdmOUTSBCount);
	printf ("  VdmOUTSWCount             = %ld\n", Info.VdmOUTSWCount);
	printf ("  VdmPUSHFCount             = %ld\n", Info.VdmPUSHFCount);
	printf ("  VdmPOPFCount              = %ld\n", Info.VdmPOPFCount);
	printf ("  VdmINTNNCount             = %ld\n", Info.VdmINTNNCount);
	printf ("  VdmINTOCount              = %ld\n", Info.VdmINTOCount);
	printf ("  VdmIRETCount              = %ld\n", Info.VdmIRETCount);
	printf ("  VdmINBIMMCount            = %ld\n", Info.VdmINBIMMCount);
	printf ("  VdmINWIMMCount            = %ld\n", Info.VdmINWIMMCount);
	printf ("  VdmOUTBIMMCount           = %ld\n", Info.VdmOUTBIMMCount);
	printf ("  VdmOUTWIMMCount           = %ld\n", Info.VdmOUTWIMMCount);
	printf ("  VdmINBCount               = %ld\n", Info.VdmINBCount);
	printf ("  VdmINWCount               = %ld\n", Info.VdmINWCount);
	printf ("  VdmOUTBCount              = %ld\n", Info.VdmOUTBCount);
	printf ("  VdmOUTWCount              = %ld\n", Info.VdmOUTWCount);
	printf ("  VdmLOCKPREFIXCount        = %ld\n", Info.VdmLOCKPREFIXCount);
	printf ("  VdmREPNEPREFIXCount       = %ld\n", Info.VdmREPNEPREFIXCount);
	printf ("  VdmREPPREFIXCount         = %ld\n", Info.VdmREPPREFIXCount);
	printf ("  VdmHLTCount               = %ld\n", Info.VdmHLTCount);
	printf ("  VdmCLICount               = %ld\n", Info.VdmCLICount);
	printf ("  VdmSTICount               = %ld\n", Info.VdmSTICount);
	printf ("  VdmBopCount               = %ld\n", Info.VdmBopCount);

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
CMD_DEF(21)
{
	NTSTATUS			Status;
	SYSTEM_CACHE_INFORMATION	Si;
	
	RtlZeroMemory (
		(PVOID) & Si,
		sizeof Si
		);
	Status = NtQuerySystemInformation (
			21,
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
CMD_DEF(22)
{
	NTSTATUS		Status;
	PSYSTEM_POOL_TAG_INFO	pInfo = NULL;
	ULONG			Length;
	ULONG			PoolIndex;

	pInfo = GlobalAlloc (GMEM_ZEROINIT, BUFFER_SIZE_DEFAULT);
	/* FIXME: check pInfo */
	
	Status = NtQuerySystemInformation(
			22,
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
			22,
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
CMD_DEF(23)
{
	NTSTATUS			Status;
	SYSTEM_PROCESSOR_SCHEDULE_INFO	Info;

	RtlZeroMemory (
		& Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			23,
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
CMD_DEF(24)
{
	NTSTATUS		Status;
	SYSTEM_DPC_INFORMATION	Info;

	RtlZeroMemory (
		& Info,
		sizeof Info
		);
	Status = NtQuerySystemInformation (
			24,
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
CMD_DEF(26)
CMD_NOT_IMPLEMENTED

/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 27.
 */
CMD_DEF(27)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 28.
 */
CMD_DEF(28)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_TIME_ADJUSTMENT_INFO	Info;

	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			28,
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
CMD_DEF(32)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 33.
 */
CMD_DEF(33)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 34.
 */
CMD_DEF(34)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 35.
 */
CMD_DEF(35)
{
	NTSTATUS		Status;
	SYSTEM_DEBUGGER_INFO	Info;

	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			35,
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
CMD_DEF(36)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 37.
 */
CMD_DEF(37)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	SYSTEM_QUOTA_INFORMATION	Info;

	RtlZeroMemory (& Info, sizeof Info);
	Status = NtQuerySystemInformation (
			37,
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
CMD_DEF(38)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 39.
 */
CMD_DEF(39)
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
CMD_DEF(44)
{
#if 0
	NTSTATUS			Status;
	TIME_ZONE_INFORMATION		Tzi;
	WCHAR				Name [33];

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
CMD_DEF(45)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 46.
 */
CMD_DEF(46)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 47.
 */
CMD_DEF(47)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 48.
 */
CMD_DEF(48)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 49.
 */
CMD_DEF(49)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 50.
 */
CMD_DEF(50)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 51.
 */
CMD_DEF(51)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 52.
 */
CMD_DEF(52)
CMD_NOT_IMPLEMENTED


/**********************************************************************
 * 
 * DESCRIPTION
 *
 * NOTE
 * 	Class 53.
 */
CMD_DEF(53)
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
	"Copyright (c) 1999-2001 Emanuele Aliberti et alii\n\n"
	"Run the command in verbose mode, for full license information.\n\n",
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
		CMD_REF(0),
		"Basic system information"
	},
	{						/* 1  Q  */
		"processor",	
		CMD_REF(1),
		"Processor characteristics"
	},
	{						/* 2  Q  */
		"perf",	
		CMD_REF(2),
		"System performance data"
	},
	{						/* 3  Q  */
		"time",
		CMD_REF(3),
		"System times"
	},
	{						/* 4  Q  (checked build only) */
		"path",
		CMD_REF(4),
		"Path (checked build only)"
	},
	{						/* 5  Q  */
		"process",
		CMD_REF(5),
		"Process & thread tables"
	},
	{						/* 6  Q  */
		"callcount",
		CMD_REF(6),
		"Call count information"
	},
	{						/* 7  Q  */
		"device",
		CMD_REF(7),
		"I/O devices in the system, by class"
	},
	{						/* 8  Q  */
		"performance",
		CMD_REF(8),
		"Processor performance"
	},
	{						/* 9  QS */
		"flags",
		CMD_REF(9),
		"System wide flags"
	},
	{						/* 10    */
		"call",
		CMD_REF(10),
		"Call time"
	},
	{						/* 11 Q  */
		"module",
		CMD_REF(11),
		"Table of kernel modules"
	},
	{						/* 12 Q  */
		"locks",
		CMD_REF(12),
		"Table of locks on resources"
	},
	{						/* 13    */
		"stack",
		CMD_REF(13),
		"Stack trace"
	},
	{						/* 14    */
		"ppool",
		CMD_REF(14),
		"Paged pool"
	},
	{						/* 15    */
		"nppool",
		CMD_REF(15),
		"Non paged pool"
	},
	{						/* 16 Q  */
		"handle",
		CMD_REF(16),
		"Table of handles"
	},
	{						/* 17 Q  */
		"object",
		CMD_REF(17),
		"Table of executive objects"
	},
	{						/* 18 Q  */
		"pagefile",
		CMD_REF(18),
		"Virtual memory paging files"
	},
	{						/* 19 Q  */
		"vdmie",
		CMD_REF(19),
		"Virtual DOS Machine instruction emulation (VDM)"
	},
	{						/* 20    */
		"vdmbop",
		CMD_REF(20),
		"Bop (VDM)"
	},
	{						/* 21 QS */
		"cache",	
		CMD_REF(21),
		"File cache"
	},
	{						/* 22 Q  */
		"pooltag",
		CMD_REF(22),
		"Tagged pools statistics (checked build only)"
	},
	{						/* 23 Q  */
		"int",
		CMD_REF(23),
		"Processor schedule information (interrupt)"
	},
	{						/* 24 QS */
		"dpc",
		CMD_REF(24),
		"Deferred procedure call behaviour (DPC)"
	},
	{						/* 25    */
		"fullmem",
		CMD_REF(25),
		"Full memory"
	},
	{						/* 26 S (callable) */
		"loadpe",
		CMD_REF(26),
		"Load a kernel mode DLL (module in PE format)"
	},
	{						/* 27 S (callable) */
		"unloadpe",
		CMD_REF(27),
		"Unload a kernel mode DLL (module in Pe format)"
	},
	{						/* 28 QS */
		"timeadj",
		CMD_REF(28),
		"Time adjustment"
	},
	{						/* 29    */
		"smem",
		CMD_REF(29),
		"Summary memory"
	},
	{						/* 30    */
		"event",
		CMD_REF(30),
		"Next event ID"
	},
	{						/* 31    */
		"events",
		CMD_REF(31),
		"Event IDs"
	},
	{						/* 32 Q  */
		"crash",
		CMD_REF(32),
		"Crash Dump Section"
	},
	{						/* 33 Q  */
		"exceptions",
		CMD_REF(33),
		"Exceptions"
	},
	{						/* 34 Q  */
		"crashstate",
		CMD_REF(34),
		"Crash Dump State"
	},
	{						/* 35 Q  */
		"debugger",
		CMD_REF(35),
		"Kernel debugger"
	},
	{						/* 36 Q  */
		"cswitch",
		CMD_REF(36),
		"Thread context switch counters"
	},
	{						/* 37 QS */
		"regquota",
		CMD_REF(37),
		"Registry quota values"
	},
	{						/* 38 S  */
		"est",
		CMD_REF(38),
		"Extended service table"
	},
	{						/* 39 S  */
		"prisep",
		CMD_REF(39),
		"Priority Separation"
	},
	{						/* 40    */
		"ppbus",
		CMD_REF(40),
		"Plug & play bus"
	},
	{						/* 41    */
		"dock",
		CMD_REF(41),
		"Dock"
	},
	{						/* 42    */
		"power",
		CMD_REF(42),
		"Power"
	},
	{						/* 43    */
		"procspeed",
		CMD_REF(43),
		"Processor speed"
	},
	{						/* 44 QS */
		"tz",
		CMD_REF(44),
		"Current time zone (TZ)"
	},
	{						/* 45 Q  */
		"lookaside",
		CMD_REF(45),
		"Lookaside"
	},
	/* NT5 */
	{						/* 46 Q  */
		"tslip",
		CMD_REF(46),
		"Set time slip (5.0)"
	},
	{						/* 47 Q  */
		"csession",
		CMD_REF(47),
		"Create session (5.0)"
	},
	{						/* 48 Q  */
		"dsession",
		CMD_REF(48),
		"Delete session (5.0)"
	},
	{						/* 49 Q  */
		"#49",
		CMD_REF(49),
		"UNKNOWN (5.0)"
	},
	{						/* 50 Q  */
		"range",
		CMD_REF(50),
		"Range start (5.0)"
	},
	{						/* 51 Q  */
		"verifier",
		CMD_REF(51),
		"Verifier (5.0)"
	},
	{						/* 52 Q  */
		"addverif",
		CMD_REF(52),
		"Add verifier (5.0)"
	},
	{						/* 53 Q  */
		"sesproc",
		CMD_REF(53),
		"Session processes (5.0)"
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
