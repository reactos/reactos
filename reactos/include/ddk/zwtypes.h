#ifndef __INCLUDE_DDK_ZWTYPES_H
#define __INCLUDE_DDK_ZWTYPES_H

#define MAX_MESSAGE_DATA   (0x130)

#define UNUSED_MSG_TYPE    (0x0)
#define LPC_REQUEST        (0x1)
#define LPC_REPLY          (0x2)
#define LPC_DATAGRAM       (0x3)
#define LPC_LOST_REPLY     (0x4)
#define LPC_PORT_CLOSED    (0x5)
#define LPC_CLIENT_DIED    (0x6)
#define LPC_EXCEPTION      (0x7)
#define LPC_DEBUG_EVENT    (0x8)
#define LPC_ERROR_EVENT    (0x9)
#define LPC_CONNECTION_REQUEST (0xa)
#define LPC_CONNECTION_REFUSED (0xb)

typedef struct _LPCSECTIONINFO
{
   DWORD Length;
   HANDLE SectionHandle;
   DWORD Unknown1;
   DWORD SectionSize;
   DWORD ClientBaseAddress;
   DWORD ServerBaseAddress;
} LPCSECTION, *PLPCSECTIONINFO;

typedef struct _LPCSECTIONMAPINFO
{
   DWORD Length;
   DWORD SectionSize;
   DWORD ServerBaseAddress;
} LPCSECTIONMAPINFO, *PLPCSECTIONMAPINFO;

typedef struct _LPCMESSAGE
{
   WORD ActualMessageLength;
   WORD TotalMessageLength;
   DWORD MessageType;
   DWORD ClientProcessId;
   DWORD ClientThreadId;
   DWORD MessageId;
   DWORD SharedSectionSize;
   BYTE MessageData[MAX_MESSAGE_DATA];
} LPCMESSAGE, *PLPCMESSAGE;


#define NtCurrentProcess() ( (HANDLE) 0xFFFFFFFF )
#define NtCurrentThread() ( (HANDLE) 0xFFFFFFFE )



// event access mask

#define EVENT_READ_ACCESS			1
#define EVENT_WRITE_ACCESS			2


// file disposition values


#define FILE_SUPERSEDE                  0x0000
#define FILE_OPEN                       0x0001
#define FILE_CREATE                     0x0002
#define FILE_OPEN_IF                    0x0003
#define FILE_OVERWRITE                  0x0004
#define FILE_OVERWRITE_IF               0x0005
#define FILE_MAXIMUM_DISPOSITION        0x0005

//process query / set information class

#define ProcessBasicInformation			0
#define ProcessQuotaLimits			1
#define ProcessIoCounters			2
#define ProcessVmCounters			3
#define ProcessTimes				4
#define ProcessBasePriority			5
#define ProcessRaisePriority			6
#define ProcessDebugPort			7
#define ProcessExceptionPort			8
#define ProcessAccessToken			9
#define ProcessLdtInformation			10
#define ProcessLdtSize				11
#define ProcessDefaultHardErrorMode		12
#define ProcessIoPortHandlers			13
#define ProcessPooledUsageAndLimits		14
#define ProcessWorkingSetWatch			15
#define ProcessUserModeIOPL			16
#define ProcessEnableAlignmentFaultFixup	17
#define ProcessPriorityClass			18
#define ProcessWx86Information			19
#define ProcessHandleCount			20
#define ProcessAffinityMask			21
#define MaxProcessInfoClass			22

// thread query / set information class
#define ThreadBasicInformation			0
#define ThreadTimes				1
#define ThreadPriority				2
#define ThreadBasePriority			3
#define ThreadAffinityMask			4
#define ThreadImpersonationToken		5
#define ThreadDescriptorTableEntry		6
#define ThreadEnableAlignmentFaultFixup		7
#define ThreadEventPair				8
#define ThreadQuerySetWin32StartAddress		9
#define ThreadZeroTlsCell			10
#define ThreadPerformanceCount			11
#define ThreadAmILastThread			12
#define ThreadIdealProcessor			13
#define ThreadPriorityBoost			14
#define MaxThreadInfoClass			15

// object handle information

#define ObjectBasicInformation			0
#define ObjectNameInformation			1
#define ObjectTypeInformation			2
#define ObjectAllInformation			3
#define ObjectDataInformation			4

// semaphore information

#define SemaphoreBasicInformation		0

// event information

#define EventBasicInformation			0

// system information

#define SystemInformation0			0
#define SystemInformation1			1
#define SystemPerformanceInformation		2
#define SystemInformation3			3
#define SystemProcessInformation		5
#define	SystemGlobalFlagInformation		9
#define SystemDriverInformation			11
#define SystemPageFileInformation		18
#define SystemCacheInformation			21
#define	SystemPoolTagStatsInformation		22
#define SystemTimeAdjustmentInformation		28
#define SystemTimeZoneInformation		44

// memory information

#define MemoryBasicInformation			0

// shutdown action

typedef enum SHUTDOWN_ACTION_TAG {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

// wait type

#define WaitAll					0
#define WaitAny					1

// number of wait objects

#define THREAD_WAIT_OBJECTS			3
//#define MAXIMUM_WAIT_OBJECTS			64

// key restore flags

#define REG_WHOLE_HIVE_VOLATILE			1
#define REG_REFRESH_HIVE			2

// object type  access rights

#define OBJECT_TYPE_CREATE		0x0001
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

// directory access rights

#define DIRECTORY_QUERY				0x0001
#define DIRECTORY_TRAVERSE			0x0002
#define DIRECTORY_CREATE_OBJECT			0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY		0x0008

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

// symbolic link access rights

#define SYMBOLIC_LINK_QUERY			0x0001
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
	PVOID FaultingPc;
	PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _QUOTA_LIMITS
{
	ULONG PagedPoolLimit;
	ULONG NonPagedPoolLimit;
	ULONG MinimumWorkingSetSize;
	ULONG MaximumWorkingSetSize;
	ULONG PagefileLimit;
	TIME TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef struct _IO_COUNTERS
{
	ULONG ReadOperationCount;
	ULONG WriteOperationCount;
	ULONG OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;


typedef struct _VM_COUNTERS_
{
	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage;
	ULONG PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;


typedef struct _POOLED_USAGE_AND_LIMITS_
{
	ULONG PeakPagedPoolUsage;
	ULONG PagedPoolUsage;
	ULONG PagedPoolLimit;
	ULONG PeakNonPagedPoolUsage;
	ULONG NonPagedPoolUsage;
	ULONG NonPagedPoolLimit;
	ULONG PeakPagefileUsage;
	ULONG PagefileUsage;
	ULONG PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;


typedef struct _PROCESS_ACCESS_TOKEN
{
	HANDLE Token;
	HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _KERNEL_USER_TIMES
{
	TIME CreateTime;
	TIME ExitTime;
	TIME KernelTime;
	TIME UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

// thread information

// incompatible with MS NT

typedef struct _THREAD_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PVOID TebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueThreadId;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

// object information

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING	Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;



typedef struct _OBJECT_DATA_INFORMATION
{
	BOOLEAN bInheritHandle;
	BOOLEAN bProtectFromClose;
} OBJECT_DATA_INFORMATION, *POBJECT_DATA_INFORMATION;


typedef struct _OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING	Name;
	UNICODE_STRING Type;
	ULONG TotalHandles;
	ULONG ReferenceCount;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

// system information

#if 0
#pragma pack(2)
typedef struct _SYSTEM_THREAD_INFORMATION
{
	FILETIME	ftCreationTime;
	DWORD		dwUnknown1;
	PVOID		dwStartAddress;
	DWORD		dwOwningPID;
	DWORD		dwThreadID;
	DWORD		dwCurrentPriority;
	DWORD		dwBasePriority;
	DWORD		dwContextSwitches;
	DWORD		dwThreadState;
	DWORD		dwWaitReason;
	DWORD		dwUnknown2 [ 5 ];
	
	
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;


typedef struct _SYSTEM_PROCESS_INFORMATION
{
	DWORD				dwOffset;
	DWORD				dwThreadCount;
	DWORD 				dwUnknown1 [6];
	FILETIME			ftCreationTime;
	DWORD				dwUnknown2 [5];
	WCHAR				* pszProcessName;
	DWORD				dwBasePriority;
	DWORD				dwProcessID;
	DWORD				dwParentProcessID;
	DWORD				dwHandleCount;
	DWORD				dwUnknown3;
	DWORD				dwUnknown4;
	DWORD				dwVirtualBytesPeak;
	DWORD				dwVirtualBytes;
	DWORD				dwPageFaults;
	DWORD				dwWorkingSetPeak;
	DWORD				dwWorkingSet;
	DWORD				dwUnknown5;
	DWORD				dwPagedPool;
	DWORD				dwUnknown6;
	DWORD				dwNonPagedPool;
	DWORD				dwPageFileBytesPeak;
	DWORD				dwPrivateBytes;
	DWORD				dwPageFileBytes;
	DWORD				dwUnknown7 [4];
	SYSTEM_THREAD_INFORMATION	Threads [1];

} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;
#endif


typedef
struct _SYSTEM_GLOBAL_FLAGS_INFO
{
	DWORD	GlobalFlags;

} SYSTEM_GLOBAL_FLAGS_INFO, * PSYSTEM_GLOBAL_FLAGS_INFO;


#if 0
#pragma pack(4)
typedef struct _SYSTEM_DRIVER_INFO
{
	PVOID BaseAddress;
	DWORD Unknown1;
	DWORD Unknown2;
	DWORD EntryIndex;
	DWORD Unknown4;
	CHAR  DriverName [256];

} SYSTEM_DRIVER_INFO, * PSYSTEM_DRIVER_INFO;


typedef struct _SYSTEM_DRIVERS_INFO
{
	DWORD DriverCount;
	SYSTEM_DRIVER_INFO DriverInfo[1];
} SYSTEM_DRIVERS_INFO, *PSYSTEM_DRIVERS_INFO;

#pragma pack(4)
typedef struct _SYSTEM_TIME_ADJUSTMENT
{
	TIME	TimeAdjustment;
	BOOL	TimeAdjustmentDisabled;

} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

typedef struct _SYSTEM_CONFIGURATION_INFO {
	union {
		ULONG  	OemId; 
		struct { 
			WORD ProcessorArchitecture; 
			WORD Reserved; 
		} tag1; 
	} tag2; 
	ULONG  PageSize; 
	PVOID  MinimumApplicationAddress; 
	PVOID  MaximumApplicationAddress; 
	ULONG  ActiveProcessorMask; 
	ULONG  NumberOfProcessors; 
	ULONG  ProcessorType; 
	ULONG  AllocationGranularity; 
	WORD   ProcessorLevel; 
	WORD   ProcessorRevision; 
} SYSTEM_CONFIGURATION_INFO, *PSYSTEM_CONFIGURATION_INFO; 


typedef struct _SYSTEM_PAGEFILE_INFORMATION
{
	DWORD	Unknown [6];
	WCHAR	PagefileName [16];

} SYSTEM_PAGEFILE_INFORMATION, * PSYSTEM_PAGEFILE_INFORMATION;


typedef struct _SYSTEM_CACHE_INFORMATION
{
	ULONG	CurrentSize;
	ULONG	PeakSize;
	ULONG	PageFaultCount;
	ULONG	MinimumWorkingSet;
	ULONG	MaximumWorkingSet;
	ULONG	Unused[4];
} SYSTEM_CACHE_INFORMATION;


/* SYSTEM_POOL_ENTRY_INFO, SYSTEM_POOL_INFORMATION
 * found by Klaus P. Gerlicher */
typedef
struct _SYSTEM_POOL_ENTRY_INFO
{
	ULONG	Tag;
	ULONG	NP_Allocs;
	ULONG	NP_Frees;
	ULONG	NP_Used;
	ULONG	P_Allocs;
	ULONG	P_Frees;
	ULONG	P_Used;

} SYSTEM_POOL_ENTRY_INFO, * PSYSTEM_POOL_ENTRY_INFO;


typedef
struct _SYSTEM_POOL_INFORMATION
{
	ULONG			Count;
	SYSTEM_POOL_ENTRY_INFO	PoolEntry [1];
    
} SYSTEM_POOL_INFORMATION, *PSYSTEM_POOL_INFORMATION;

#endif

// file information

typedef struct _FILE_BASIC_INFORMATION
{
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN DeletePending;
	BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION
{
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
	BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION
{
	LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION
{
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR  EaName[0];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;


typedef struct _FILE_EA_INFORMATION {
	ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;


typedef struct _FILE_GET_EA_INFORMATION {
	ULONG NextEntryOffset;
	UCHAR EaNameLength;
	CHAR EaName[0];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION {
	ULONG NextEntryOffset;
	ULONG StreamNameLength;
	LARGE_INTEGER StreamSize;
	LARGE_INTEGER StreamAllocationSize;
	WCHAR StreamName[0];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
	LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION 
{
	ULONG NextEntryOffset;
	ULONG FileIndex;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;


typedef struct _FILE_RENAME_INFORMATION {
	BOOLEAN Replace;
	HANDLE RootDir;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;


typedef struct _FILE_INTERNAL_INFORMATION {
	LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
	ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;


typedef struct _FILE_MODE_INFORMATION {
	ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
	LARGE_INTEGER CompressedFileSize;
	USHORT CompressionFormat;
	UCHAR CompressionUnitShift;
	UCHAR ChunkShift;
	UCHAR ClusterShift;
	UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
	FILE_BASIC_INFORMATION BasicInformation;
	FILE_STANDARD_INFORMATION StandardInformation;
	FILE_INTERNAL_INFORMATION InternalInformation;
	FILE_EA_INFORMATION EaInformation;
	FILE_ACCESS_INFORMATION AccessInformation;
	FILE_POSITION_INFORMATION PositionInformation;
	FILE_MODE_INFORMATION ModeInformation;
	FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

// file system information structures

typedef struct _FILE_FS_DEVICE_INFORMATION {
	DEVICE_TYPE DeviceType;
	ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION,  *PFILE_FS_DEVICE_INFORMATION;


typedef struct _FILE_FS_VOLUME_INFORMATION {
	TIME VolumeCreationTime;
	ULONG VolumeSerialNumber;
	ULONG VolumeLabelLength;
	BOOLEAN SupportsObjects;
	WCHAR VolumeLabel[0];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
	LARGE_INTEGER TotalAllocationUnits;
	LARGE_INTEGER AvailableAllocationUnits;
	ULONG SectorsPerAllocationUnit;
	ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
	ULONG FileSystemAttributes;
	LONG MaximumComponentNameLength;
	ULONG FileSystemNameLength;
	WCHAR FileSystemName[0];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

/*
	FileSystemAttributes is one of the following values:

	FILE_CASE_SENSITIVE_SEARCH      0x00000001
        FILE_CASE_PRESERVED_NAMES       0x00000002
        FILE_UNICODE_ON_DISK            0x00000004
        FILE_PERSISTENT_ACLS            0x00000008
        FILE_FILE_COMPRESSION           0x00000010
        FILE_VOLUME_QUOTAS              0x00000020
        FILE_VOLUME_IS_COMPRESSED       0x00008000
*/
typedef struct _FILE_FS_LABEL_INFORMATION {
	ULONG VolumeLabelLength;
	WCHAR VolumeLabel[0];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

// read file scatter / write file scatter
//FIXME I am a win32 struct aswell

typedef union _FILE_SEGMENT_ELEMENT {
	PVOID Buffer;
	ULONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

// directory information

typedef struct _OBJDIR_INFORMATION {
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName; // Directory, Device ...
	UCHAR          Data[0];
} OBJDIR_INFORMATION, *POBJDIR_INFORMATION;


typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG	NextEntryOffset;
	ULONG	FileIndex;
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[0];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
	ULONG	NextEntryOffset;
	ULONG	FileIndex;
	TIME CreationTime;
	TIME LastAccessTime;
	TIME LastWriteTime;
	TIME ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	WCHAR FileName[0]; // variable size
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION,
  FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;


typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
	ULONG		NextEntryOffset;
	ULONG		FileIndex;
	TIME 		CreationTime;
	TIME 		LastAccessTime;
	TIME 		LastWriteTime;
	TIME 		ChangeTime;
	LARGE_INTEGER 	EndOfFile;
	LARGE_INTEGER 	AllocationSize;
	ULONG 		FileAttributes;
	ULONG 		FileNameLength;
	ULONG 		EaSize;
	CHAR 		ShortNameLength;
	WCHAR 		ShortName[12]; // 8.3 name
	WCHAR 		FileName[0];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;


/*
	NotifyFilter / CompletionFilter:

	FILE_NOTIFY_CHANGE_FILE_NAME        0x00000001
	FILE_NOTIFY_CHANGE_DIR_NAME         0x00000002
	FILE_NOTIFY_CHANGE_NAME             0x00000003
	FILE_NOTIFY_CHANGE_ATTRIBUTES       0x00000004
	FILE_NOTIFY_CHANGE_SIZE             0x00000008
	FILE_NOTIFY_CHANGE_LAST_WRITE       0x00000010
	FILE_NOTIFY_CHANGE_LAST_ACCESS      0x00000020
	FILE_NOTIFY_CHANGE_CREATION         0x00000040
	FILE_NOTIFY_CHANGE_EA               0x00000080
	FILE_NOTIFY_CHANGE_SECURITY         0x00000100
	FILE_NOTIFY_CHANGE_STREAM_NAME      0x00000200
	FILE_NOTIFY_CHANGE_STREAM_SIZE      0x00000400
	FILE_NOTIFY_CHANGE_STREAM_WRITE     0x00000800
*/

typedef struct _FILE_NOTIFY_INFORMATION {
	ULONG Action;
	ULONG FileNameLength;
	WCHAR FileName[0]; 
} FILE_NOTIFY_INFORMATION;


/*
	 Action is one of the following values:

	FILE_ACTION_ADDED      	    	0x00000001
	FILE_ACTION_REMOVED     	0x00000002
	FILE_ACTION_MODIFIED       	0x00000003
	FILE_ACTION_RENAMED_OLD_NAME	0x00000004
	FILE_ACTION_RENAMED_NEW_NAME 	0x00000005
	FILE_ACTION_ADDED_STREAM   	0x00000006
	FILE_ACTION_REMOVED_STREAM  	0x00000007
	FILE_ACTION_MODIFIED_STREAM  	0x00000008

*/


//FIXME: I am a win32 object
typedef
VOID
(*PTIMERAPCROUTINE)(
	LPVOID lpArgToCompletionRoutine,
	DWORD dwTimerLowValue,
	DWORD dwTimerHighValue
	);


// File System Control commands ( related to defragging )

#define	FSCTL_READ_MFT_RECORD			0x90068 // NTFS only
#define FSCTL_GET_VOLUME_BITMAP			0x9006F
#define FSCTL_GET_RETRIEVAL_POINTERS		0x90073
#define FSCTL_MOVE_FILE				0x90074

typedef struct _MAPPING_PAIR
{
	ULONGLONG	Vcn;
	ULONGLONG	Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

typedef struct _GET_RETRIEVAL_DESCRIPTOR
{
	ULONG		NumberOfPairs;
	ULONGLONG	StartVcn;
	MAPPING_PAIR	Pair[0]; // variable size 
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct _BITMAP_DESCRIPTOR
{
	ULONGLONG	StartLcn;
	ULONGLONG	ClustersToEndOfVol;
	BYTE		Map[0]; // variable size
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR;

typedef struct _MOVEFILE_DESCRIPTOR
{
	HANDLE            FileHandle;
	ULONG             Reserved;
	LARGE_INTEGER     StartVcn;
	LARGE_INTEGER     TargetLcn;
	ULONG             NumVcns;
	ULONG             Reserved1;
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;


// semaphore information

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	ULONG CurrentCount;
	ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

// event information

typedef struct _EVENT_BASIC_INFORMATION
{
	BOOL AutomaticReset;
	BOOL Signaled;
} EVENT_BASIC_INFORMATION, *PEVENT_INFORMATION;

//typedef enum _TIMER_TYPE 
//{
//	NotificationTimer,
//	SynchronizationTimer
//} TIMER_TYPE;

typedef
struct _LPC_PORT_BASIC_INFORMATION
{
	DWORD	Unknown0;
	DWORD	Unknown1;
	DWORD	Unknown2;
	DWORD	Unknown3;
	DWORD	Unknown4;
	DWORD	Unknown5;
	DWORD	Unknown6;
	DWORD	Unknown7;
	DWORD	Unknown8;
	DWORD	Unknown9;
	DWORD	Unknown10;
	DWORD	Unknown11;
	DWORD	Unknown12;
	DWORD	Unknown13;

} LPC_PORT_BASIC_INFORMATION, * PLPC_PORT_BASIC_INFORMATION;

#endif
