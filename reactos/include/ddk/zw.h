
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          System call definitions
 * FILE:             include/ddk/zw.h
 * REVISION HISTORY: 
 *              ??/??/??: First few functions (David Welch)
 *              ??/??/??: Complete implementation by Boudewijn Dekker
 *              13/07/98: Reorganised things a bit (David Welch)
 *              04/08/98: Added some documentation (Boudewijn Dekker)
 */

#ifndef __DDK_ZW_H
#define __DDK_ZW_H

#include <windows.h>

/*
 * FUNCTION: Closes an object handle
 * ARGUMENTS:
 *         Handle = Handle to the object
 * RETURNS: Status
 */
NTSTATUS ZwClose(HANDLE Handle);

/*
 * FUNCTION: Creates or opens a directory object, which is a container for
 * other objects
 * ARGUMENTS:
 *        DirectoryHandle (OUT) = Points to a variable which stores the
 *                                handle for the directory on success
 *        DesiredAccess = Type of access the caller requires to the directory
 *        ObjectAttributes = Structures specifing the object attributes,
 *                           initialized with InitializeObjectAttributes
 * RETURNS: Status 
 */
NTSTATUS ZwCreateDirectoryObject(PHANDLE DirectoryHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes);

/*
 * FUNCTION: Creates or opens a registry key
 * ARGUMENTS:
 *        KeyHandle (OUT) = Points to a variable which stores the handle
 *                          for the key on success
 *        DesiredAccess = Access desired by the caller to the key 
 *        ObjectAttributes = Initialized object attributes for the key
 *        TitleIndex = Who knows?
 *        Class = Object class of the key?
 *        CreateOptions = Options for the key creation
 *        Disposition (OUT) = Points to a variable which a status value
 *                            indicating whether a new key was created
 * RETURNS: Status
 */
NTSTATUS ZwCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes,
		     ULONG TitleIndex, PUNICODE_STRING Class,
		     ULONG CreateOptions, PULONG Disposition);

/*
 * FUNCTION: Deletes a registry key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key
 * RETURNS: Status
 */
NTSTATUS ZwDeleteKey(HANDLE KeyHandle);

/*
 * FUNCTION: Returns information about the subkeys of an open key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key whose subkeys are to enumerated
 *         Index = zero based index of the subkey for which information is
 *                 request
 *         KeyInformationClass = Type of information returned
 *         KeyInformation (OUT) = Caller allocated buffer for the information
 *                                about the key
 *         Length = Length in bytes of the KeyInformation buffer
 *         ResultLength (OUT) = Caller allocated storage which holds
 *                              the number of bytes of information retrieved
 *                              on return
 * RETURNS: Status
 */
NTSTATUS ZwEnumerateKey(HANDLE KeyHandle, ULONG Index, 
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation, ULONG Length, 
			PULONG ResultLength);

/*
 * FUNCTION: Returns information about the value entries of an open key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key whose value entries are to enumerated
 *         Index = zero based index of the subkey for which information is
 *                 request
 *         KeyInformationClass = Type of information returned
 *         KeyInformation (OUT) = Caller allocated buffer for the information
 *                                about the key
 *         Length = Length in bytes of the KeyInformation buffer
 *         ResultLength (OUT) = Caller allocated storage which holds
 *                              the number of bytes of information retrieved
 *                              on return
 * RETURNS: Status
 */
NTSTATUS ZwEnumerateValueKey(HANDLE KeyHandle, ULONG Index, 
			KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation, ULONG Length, 
			PULONG ResultLength);


/*
 * FUNCTION: Forces a registry key to be committed to disk
 * ARGUMENTS:
 *        KeyHandle = Handle of the key to be written to disk
 * RETURNS: Status
 */
NTSTATUS ZwFlushKey(HANDLE KeyHandle);

/*
 * FUNCTION: Changes the attributes of an object to temporary
 * ARGUMENTS:
 *        Handle = Handle for the object
 * RETURNS: Status
 */
NTSTATUS ZwMakeTemporaryObject(HANDLE Handle);

/*
 * FUNCTION: Maps a view of a section into the virtual address space of a 
 *           process
 * ARGUMENTS:
 *        SectionHandle = Handle of the section
 *        ProcessHandle = Handle of the process
 *        BaseAddress = Desired base address (or NULL) on entry
 *                      Actual base address of the view on exit
 *        ZeroBits = Number of high order address bits that must be zero
 *        CommitSize = Size in bytes of the initially committed section of 
 *                     the view 
 *        SectionOffset = Offset in bytes from the beginning of the section
 *                        to the beginning of the view
 *        ViewSize = Desired length of map (or zero to map all) on entry
 *                   Actual length mapped on exit
 *        InheritDisposition = Specified how the view is to be shared with
 *                            child processes
 *        AllocateType = Type of allocation for the pages
 *        Protect = Protection for the committed region of the view
 * RETURNS: Status
 */
NTSTATUS ZwMapViewOfSection(HANDLE SectionHandle,
			    HANDLE ProcessHandle,
			    PVOID* BaseAddress,
			    ULONG ZeroBits,
			    ULONG CommitSize,
			    PLARGE_INTEGER SectionOffset,
			    PULONG ViewSize,
			    SECTION_INHERIT InheritDisposition,
			    ULONG AllocationType,
			    ULONG Protect);

/*
 * FUNCTION: Opens an existing key in the registry
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the key
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS ZwOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes);

/*
 * FUNCTION: Opens an existing section object
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the key
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS ZwOpenSection(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes);

/*
 * FUNCTION: Returns information about an open file
 * ARGUMENTS:
 *        FileHandle = Handle of the file to be queried
 *        IoStatusBlock (OUT) = Caller supplied storage for the result
 *        FileInformation (OUT) = Caller supplied storage for the file
 *                                information
 *        Length = Length in bytes of the buffer for file information
 *        FileInformationClass = Type of information to be returned
 * RETURNS: Status
 */
NTSTATUS ZwQueryInformationFile(HANDLE FileHandle,
				PIO_STATUS_BLOCK IoStatusBlock,
				PVOID FileInformation,
				ULONG Length,
				FILE_INFORMATION_CLASS FileInformationClass);



   
#if KERNEL_SUPPORTS_OBJECT_ATTRIBUTES_CORRECTLY
typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	SECURITY_DESCRIPTOR *SecurityDescriptor;       
	SECURITY_QUALITY_OF_SERVICE *SecurityQualityOfService;  
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif
   
#if IOTYPES_DIDNT_DECLARE_THIS
typedef struct _IO_STATUS_BLOCK {
	NTSTATUS Status;
	ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
#endif


//typedef LARGE_INTEGER *PLARGE_INTEGER;

//

#define NtCurrentProcess() ( (HANDLE) 0xFFFFFFFF )
#define NtCurrentThread() ( (HANDLE) 0xFFFFFFFE )



// event access mask

#define EVENT_READ_ACCESS			1
#define EVENT_WRITE_ACCESS			2


//process query / set information class

#define	ProcessBasicInformation 		0
#define	ProcessQuotaLimits			1
#define	ProcessIoCounters			2
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


// key query information class

#define KeyBasicInformation			0
#define KeyNodeInformation			1
#define KeyFullInformation			2


// key set information class

#define KeyWriteTimeInformation			0

// key value information class

#define KeyValueBasicInformation		0
#define	KeyValueFullInformation			1
#define	KeyValuePartialInformation		2

// object handle information

#define HandleInformation			0
#define HandleName				1
#define HandleTypeInformation			2
#define HandleAllInformation			3
#define	HandleBasicInformation			4

// system information
#define SystemPerformanceInformation		 5
#define SystemCacheInformation			21
#define SystemTimeAdjustmentInformation		28


// file information


#define FileDirectoryInformation 		1
#define FileFullDirectoryInformation		2
#define FileBothDirectoryInformation		3
#define FileBasicInformation			4
#define FileStandardInformation			5
#define FileInternalInformation			6
#define FileEaInformation			7
#define FileAccessInformation			8
#define FileNameInformation			9
#define FileRenameInformation			10
#define FileLinkInformation			11
#define FileNamesInformation			12
#define FileDispositionInformation		13
#define FilePositionInformation			14
#define FileFullEaInformation			15
#define FileModeInformation			16
#define FileAlignmentInformation		17
#define FileAllInformation			18
#define FileAllocationInformation		19
#define FileEndOfFileInformation		20
#define FileAlternateNameInformation		21
#define FileStreamInformation			22
#define FilePipeInformation			23
#define FilePipeLocalInformation		24
#define FilePipeRemoteInformation		25
#define FileMailslotQueryInformation		26
#define FileMailslotSetInformation		27
#define FileCompressionInformation		28
#define FileCopyOnWriteInformation		29
#define FileCompletionInformation		30
#define FileMoveClusterInformation		31
#define FileOleClassIdInformation		32
#define FileOleStateBitsInformation		33
#define FileNetworkOpenInformation		34
#define FileObjectIdInformation			35
#define FileOleAllInformation			36
#define FileOleDirectoryInformation		37
#define FileContentIndexInformation		38
#define FileInheritContentIndexInformation	39
#define FileOleInformation			40
#define FileMaximumInformation			41



//file system information class values



#define FileFsVolumeInformation 		1
#define FileFsLabelInformation			2
#define FileFsSizeInformation			3
#define FileFsDeviceInformation			4
#define FileFsAttributeInformation		5
#define FileFsControlInformation		6
#define FileFsQuotaQueryInformation		7
#define FileFsQuotaSetInformation		8
#define FileFsMaximumInformation		9


// shutdown action

typedef enum _SHUTDOWN_ACTION_ {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;



// wait type

#define WaitAll					0
#define WaitAny					1
 
 
// key restore flags

#define REG_WHOLE_HIVE_VOLATILE     		1   
#define REG_REFRESH_HIVE            		2   


// object type  access rights

#define OBJECT_TYPE_CREATE		0x00000001 
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)


// directory access rights

#define DIRECTORY_QUERY				0x0001
#define DIRECTORY_TRAVERSE			0x0002
#define DIRECTORY_CREATE_OBJECT			0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY		0x0008

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

// symbolic link access rights

#define SYMBOLIC_LINK_QUERY 			0x0001
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)
   
typedef struct _PROCESS_WS_WATCH_INFORMATION_ 
{
	PVOID FaultingPc;
	PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION_
{
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _QUOTA_LIMITS_ 
{
	ULONG PagedPoolLimit;
	ULONG NonPagedPoolLimit;
	ULONG MinimumWorkingSetSize;
	ULONG MaximumWorkingSetSize;
	ULONG PagefileLimit;
	LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef struct _IO_COUNTERS_
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


typedef struct _PROCESS_ACCESS_TOKEN_ 
{
	HANDLE Token;
	HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _KERNEL_USER_TIMES_ 
{
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

   
typedef struct _OBJECT_NAME_INFORMATION_ 
{               
    UNICODE_STRING Name;                                
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;   


// section information 


// handle information 

//HandleBasicInformation  HANDLE_BASIC_INFORMATION
typedef struct _HANDLE_BASIC_INFORMATION_ 
{
	BOOL bInheritHanlde;
	BOOL bProtectFromClose;
} HANDLE_BASIC_INFORMATION,  *PHANDLE_BASIC_INFORMATION;

//HandleTypeInormation HANDLE_TYPE_INFORMATION 
typedef struct _HANDLE_TYPE_INFORMATION_ {
	UNICODE_STRING	Name;
	UNICODE_STRING Type;
	ULONG TotalHandles;
	ULONG ReferenceCount;
} HANDLE_TYPE_INFORMATION, *PHANDLE_TYPE_INFORMATION;

typedef struct _SYSTEM_TIME_ADJUSTMENT_
{
	ULONG TimeAdjustment;	
	BOOL  TimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

#ifdef __STRUCTS_H_DIDNT_DEFINE_THIS_ALREADY
typedef struct _SYSTEM_INFO_ { 
    	union { 
		DWORD  dwOemId; 
		struct { 
			WORD wProcessorArchitecture; 
			WORD wReserved; 
		}; 
	}; 
	DWORD  dwPageSize; 
	LPVOID lpMinimumApplicationAddress; 
	LPVOID lpMaximumApplicationAddress; 
	DWORD  dwActiveProcessorMask; 
	DWORD  dwNumberOfProcessors; 
	DWORD  dwProcessorType; 
	DWORD  dwAllocationGranularity; 
	WORD  wProcessorLevel; 
	WORD  wProcessorRevision; 
} SYSTEM_INFO; 
#endif

typedef struct _SYSTEM_CACHE_INFORMATION_ {
	ULONG    	CurrentSize;
	ULONG    	PeakSize;
	ULONG    	PageFaultCount;
	ULONG    	MinimumWorkingSet;
	ULONG    	MaximumWorkingSet;
	ULONG    	Unused[4];
} SYSTEM_CACHE_INFORMATION;



// file information

typedef struct _FILE_BASIC_INFORMATION_ 
{                    
	LARGE_INTEGER CreationTime;                             
	LARGE_INTEGER LastAccessTime;                           
	LARGE_INTEGER LastWriteTime;                            
	LARGE_INTEGER ChangeTime;                               
	ULONG FileAttributes;                                   
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;         
                                                            
typedef struct _FILE_STANDARD_INFORMATION_ 
{                 
	LARGE_INTEGER AllocationSize;                           
	LARGE_INTEGER EndOfFile;                                
	ULONG NumberOfLinks;                                    
	BOOLEAN DeletePending;                                  
	BOOLEAN Directory;                                      
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;   
                                                            
typedef struct _FILE_POSITION_INFORMATION_ 
{                 
	LARGE_INTEGER CurrentByteOffset;                        
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;   
                                                            
typedef struct _FILE_ALIGNMENT_INFORMATION_ 
{                
	ULONG AlignmentRequirement;                             
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION; 
                                                            
typedef struct _FILE_DISPOSITION_INFORMATION_
{                  
	BOOLEAN DeleteFile;                                         
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; 
                                                                
typedef struct _FILE_END_OF_FILE_INFORMATION_
{                  
	LARGE_INTEGER EndOfFile;                                    
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION; 
                                                                
typedef struct _FILE_NETWORK_OPEN_INFORMATION_ {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION_
{
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR *EaName;
} FILE_FULL_EA_INFORMATION;
typedef FILE_FULL_EA_INFORMATION *PFILE_FULL_EA_INFORMATION; 


typedef struct _FILE_EA_INFORMATION_ {
	ULONG EaSize;
} FILE_EA_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION_
{
        ULONG NextEntryOffset;
        ULONG StreamNameLength;
        LARGE_INTEGER StreamSize;
        LARGE_INTEGER StreamAllocationSize;
        WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;


#ifdef __IO_TYPE_DIDNT_DEFINE_THIS_ALREADY
typedef struct _IO_COMPLETION_CONTEXT 
{
	PVOID Port;
	ULONG Key; 
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;
#endif

typedef struct _FILE_ALLOCATION_INFORMATION_ {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION;

typedef struct _FILE_NAME_INFORMATION_ {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION_ {
	BOOLEAN Replace;
	HANDLE RootDir;
        ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;


typedef struct _FILE_INTERNAL_INFORMATION_ {
	LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION_ {
	ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION;

#ifdef __DEFINED_ABOVE
typedef struct _FILE_POSITION_INFORMATION_ {
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION;
#endif

typedef struct _FILE_MODE_INFORMATION_ {
	ULONG Mode;
} FILE_MODE_INFORMATION;

#ifdef __DEFINED_ABOVE
typedef struct _FILE_ALIGNMENT_INFORMATION_ {
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION;
#endif

typedef struct _FILE_COMPRESSION_INFORMATION_ {
	LARGE_INTEGER CompressedFileSize;
	USHORT CompressionFormat;
	UCHAR CompressionUnitShift;
	UCHAR ChunkShift;
	UCHAR ClusterShift;
	UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION;

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
} FILE_ALL_INFORMATION;

// file system information structures

typedef struct _FILE_FS_DEVICE_INFORMATION_ {                    
	DEVICE_TYPE DeviceType;                                     
	ULONG Characteristics;                                      
} FILE_FS_DEVICE_INFORMATION;
typedef FILE_FS_DEVICE_INFORMATION *PFILE_FS_DEVICE_INFORMATION;

/* 
	characteristics  is one of the following values:

	FILE_REMOVABLE_MEDIA            0x00000001
        FILE_READ_ONLY_DEVICE           0x00000002
        FILE_FLOPPY_DISKETTE            0x00000004
        FILE_WRITE_ONCE_MEDIA           0x00000008
        FILE_REMOTE_DEVICE              0x00000010
        FILE_DEVICE_IS_MOUNTED          0x00000020
        FILE_VIRTUAL_VOLUME             0x00000040
*/  

typedef struct _FILE_FS_VOLUME_INFORMATION_ {
	LARGE_INTEGER VolumeCreationTime;
	ULONG VolumeSerialNumber;
	ULONG VolumeLabelLength;
	BOOLEAN SupportsObjects;
	WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION_ {
	LARGE_INTEGER TotalAllocationUnits;
	LARGE_INTEGER AvailableAllocationUnits;
	ULONG SectorsPerAllocationUnit;
	ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION_ {
	ULONG FileSystemAttributes;
	LONG MaximumComponentNameLength;
	ULONG FileSystemNameLength;
	WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION;

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


// directory information

typedef struct _OBJDIR_INFORMATION_ {
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName; // Directory, Device ...
	UCHAR          Data[1];        // variable size
} OBJDIR_INFORMATION, *POBJDIR_INFORMATION;


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

typedef struct _FILE_NOTIFY_INFORMATION_ {
	ULONG NextEntryOffset;
	ULONG Action;
	ULONG FileNameLength;
	WCHAR FileName[1]; // variable size
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


//FIXME: I belong somewhere in windows.h
typedef 
VOID 
(*PTIMERAPCROUTINE)( 
	LPVOID lpArgToCompletionRoutine, 
	DWORD dwTimerLowValue, 
	DWORD dwTimerHighValue 
	); 

// NtProcessStartup parameters

typedef struct _ENVIRONMENT_INFORMATION_ {
       ULONG            Unknown[21];     
       UNICODE_STRING   CommandLine;
       UNICODE_STRING   ImageFile;
} ENVIRONMENT_INFORMATION, *PENVIRONMENT_INFORMATION;


typedef struct _STARTUP_ARGUMENT_ {
       ULONG                     Unknown[3];
       PENVIRONMENT_INFORMATION  Environment;
} STARTUP_ARGUMENT, *PSTARTUP_ARGUMENT;


// File System Control commands ( related to defragging )

#define	FSCTL_READ_MFT_RECORD			0x90068 // NTFS only
#define FSCTL_GET_VOLUME_BITMAP			0x9006F
#define FSCTL_GET_RETRIEVAL_POINTERS		0x90073
#define FSCTL_MOVE_FILE				0x90074

typedef struct _MAPPING_PAIR_ {
	ULONGLONG	Vcn;
	ULONGLONG	Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

typedef struct _GET_RETRIEVAL_DESCRIPTOR_ {
	ULONG		NumberOfPairs;
	ULONGLONG	StartVcn;
	MAPPING_PAIR	Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct _BITMAP_DESCRIPTOR_ {
	ULONGLONG	StartLcn;
	ULONGLONG	ClustersToEndOfVol;
	BYTE		Map[1];
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR; 

typedef struct _MOVEFILE_DESCRIPTOR_ {
	HANDLE            FileHandle; 
	ULONG             Reserved;   
	LARGE_INTEGER     StartVcn; 
	LARGE_INTEGER     TargetLcn;
	ULONG             NumVcns; 
	ULONG             Reserved1;	
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;





/*
 * FUNCTION: Adds an atom to the global atom table
 * ARGUMENTS: 
 *        AtomString = The string to add to the atom table.
 * REMARKS: The arguments map to the win32 add GlobalAddAtom. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAddAtom(
	OUT ATOM *Atom,
	IN PUNICODE_STRING pString
	);
/*
 * FUNCTION: Decrements a thread's resume count and places it in an alerted state.
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        SuspendCount =  The resulting resume count.
 * REMARK:
	  A thread is resumed if its resume count is 0
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAlertResumeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG SuspendCount
	);
/*
 * FUNCTION: Puts the thread in a alerted state
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be alerted
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAlertThread(
	IN HANDLE ThreadHandle
	);
/*
 * FUNCTION: Allocates a locally unique id
 * ARGUMENTS: 
 *        LocallyUniqueId = Locally unique number
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAllocateLocallyUniqueId(
	OUT PVOID LocallyUniqueId
	);

/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated
 *      StartAddress  = (OPTIONAL) The requested start address 
 *      NumberOfBytesAllocated = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to allocated,
 *                       can be one of the values : MEM_COMMIT, MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN
 *      ProtectionType = Indicates the protection type of the pages allocated, can be a combination of
 *                      PAGE_READONLY, PAGE_READWRITE, PAGE_EXECUTE_READ,
 *                      PAGE_EXECUTE_READWRITE, PAGE_GUARD, PAGE_NOACCESS, PAGE_NOACCESS
 * REMARKS:
 *       This function maps to the win32 VirtualAllocEx. Virtual memory is process based so the 
 *       protocol starts with a ProcessHandle. I splitted the functionality of obtaining the actual address and specifying
 *       the start address in two parameters ( BaseAddress and StartAddress ) The NumberOfBytesAllocated specify the range
 *       and the AllocationType and ProctectionType map to the other two parameters.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAllocateVirtualMemory( 
	IN HANDLE hProcess,
	OUT PVOID  BaseAddress,
	IN ULONG  StartAddress,
	IN ULONG  NumberOfBytesAllocated,
	IN ULONG  AllocationType, 
	IN ULONG  ProtectionType
	);

/*
 * FUNCTION: Cancels a IO request
 * ARGUMENTS: 
 *        FileHandle = Handle to the file
 *        IoStatusBlock  = Specifies the status of the io request when it was cancelled
 * REMARKS:
 *        This function maps to the win32 CancelIo. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCancelIoFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock   
	);
/*
 * FUNCTION: Cancels a timer
 * ARGUMENTS: 
 *        TimerHandle = Handle to the timer
          ElapsedTime = Specifies the elapsed time the timer has run so far.
 * REMARKS:
          The arguments to this function map to the function CancelWaitableTimer. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCancelTimer(
	IN HANDLE TimerHandle,
	OUT ULONG ElapsedTime
	);
/*
 * FUNCTION: Sets the status of the event back to non-signaled
 * ARGUMENTS: 
 *        EventHandle = Handle to the event
 * REMARKS:
 *       This function maps to win32 function ResetEvent.
 * RETURcNS: Status
 */

NTSTATUS
STDCALL
NtClearEvent( 
	IN HANDLE  EventHandle 
	);

/*
 * FUNCTION: Closes an object handle
 * ARGUMENTS:
 *         Handle = Handle to the object
 * REMARKS:
         This function maps to the win32 function CloseHandle. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtClose(
    	IN HANDLE Handle
    	);


/*
 * FUNCTION: Continues a thread with the specified context
 * ARGUMENTS: 
 *        Context = Specifies the processor context
 * REMARKS
          NtContinue can be used to continue after a exception.
 * RETURNS: Status
 */
//FIXME This function might need another parameter
NTSTATUS
STDCALL
NtContinue(
	IN PCONTEXT Context
	);

/*
 * FUNCTION: Creates a directory object
 * ARGUMENTS:
 *        DirectoryHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies access to the directory
 *        ObjectAttribute = Initialized attributes for the object
 * REMARKS: This function maps to the win32 CreateDirectory. A directory is like a file so it needs a
 *          handle, a access mask and a OBJECT_ATTRIBUTES structure to map the path name and the SECURITY_ATTRIBUTES.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCreateDirectoryObject(
    	OUT PHANDLE DirectoryHandle,
    	IN ACCESS_MASK DesiredAccess,
    	IN POBJECT_ATTRIBUTES ObjectAttributes
    	);
/*
 * FUNCTION: Creates an event object
 * ARGUMENTS:
 *        EventHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies access to the event
 *        ObjectAttribute = Initialized attributes for the object
 *        ManualReset = manual-reset or auto-reset if true you have to reset the state of the event manually
                        using NtResetEvent/NtClearEvent. if false the system will reset the event to a non-signalled state
                        automatically after the system has rescheduled a thread waiting on the event.
 *        InitialState = specifies the initial state of the event to be signaled ( TRUE ) or non-signalled (FALSE).
 * REMARKS: This function maps to the win32 CreateEvent. Demanding a out variable  of type HANDLE,
 *          a access mask and a OBJECT_ATTRIBUTES structure mapping to the SECURITY_ATTRIBUTES. ManualReset and InitialState are
 *          both parameters aswell ( possibly the order is reversed ).
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCreateEvent(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOL ManualReset,
	IN BOOL InitialState
	);
/*
 * FUNCTION: Creates or opens a file, directory or device object.
 * ARGUMENTS:
 *        FileHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the file can
 *                        be a combination of DELETE | FILE_READ_DATA ..  
 *        ObjectAttribute = Initialized attributes for the object, contains the rootdirectory and the filename
 *        IoStatusBlock (OUT) = Caller supplied storage for the resulting status information, indicating if the
 *                              the file is created and opened or allready existed and is just opened.
 *        FileAttributes = file attributes can be a combination of FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN ...
 *        ShareAccess = can be a combination of the following: FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE 
 *        CreateDisposition = specifies what the behavior of the system if the file allready exists.
 *        CreateOptions = specifies the behavior of the system on file creation.
 *        EaBuffer (OPTIONAL) = Extended Attributes buffer, applies only to files and directories.
 *        EaLength = Extended Attributes buffer size,  applies only to files and directories.
 * REMARKS: This function maps to the win32 CreateFile. 
 * RETURNS: Status
 */                                        
NTSTATUS                                        
STDCALL                                           
NtCreateFile(                                   
	OUT PHANDLE FileHandle,                     
	IN ACCESS_MASK DesiredAccess,               
	IN POBJECT_ATTRIBUTES ObjectAttributes,     
	OUT PIO_STATUS_BLOCK IoStatusBlock,         
	IN PLARGE_INTEGER AllocationSize OPTIONAL,  
	IN ULONG FileAttributes,                    
	IN ULONG ShareAccess,                       
	IN ULONG CreateDisposition,                 
	IN ULONG CreateOptions,                     
	IN PVOID EaBuffer OPTIONAL,                 
	IN ULONG EaLength                           
    	);                                          
/*
 * FUNCTION: Creates or opens a file, directory or device object.
 * ARGUMENTS:
 *        CompletionPort (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the port
 *        IoStatusBlock =
 *        NumberOfConcurrentThreads =
 * REMARKS: This function maps to the win32 CreateIoCompletionPort
 * RETURNS:
 *	Status
 */
NTSTATUS
STDCALL
NtCreateIoCompletion(
	OUT PHANDLE CompletionPort,
	IN ACCESS_MASK DesiredAccess,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG NumberOfConcurrentThreads 
	);
/*
 * FUNCTION: Creates a registry key
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the key
 *			It can have a combination of the following values:
 *			KEY_READ | KEY_WRITE | KEY_EXECUTE | KEY_ALL_ACCESS
 *			or
 *                      KEY_QUERY_VALUE	The values of the key can be queried.
 *			KEY_SET_VALUE	The values of the key can be modified.
 *			KEY_CREATE_SUB_KEYS	The key may contain subkeys.
 *			KEY_ENUMERATE_SUB_KEYS	Subkeys  can be queried.
 *			KEY_NOTIFY	
 *			KEY_CREATE_LINK	A symbolic link to the key can be created. 
 *        ObjectAttributes = The name of the key may be specified directly in the name field 
 *				of object attributes or relative
 *				to a key in rootdirectory.
 *	  Class = Specifies the kind of data.
 *        CreateOptions = Specifies additional options with which the key is created
 *			REG_OPTION_VOLATILE		The key is not preserved across boots.
 *			REG_OPTION_NON_VOLATILE		The key is preserved accross boots.
 *			REG_OPTION_CREATE_LINK		The key is a symbolic link to another key. 
 *			REG_OPTION_BACKUP_RESTORE	Key is being opened or created for backup/restore operations. 
 *        Disposition = Indicates if the call to NtCreateKey resulted in the creation of a key it 
 *			can have the following values: REG_CREATED_NEW_KEY | REG_OPENED_EXISTING_KEY
 * RETURNS:
 *	Status
 */
NTSTATUS
STDCALL
NtCreateKey(
	OUT PHANDLE KeyHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG TitleIndex,
	IN PUNICODE_STRING Class OPTIONAL,
	IN ULONG CreateOptions,
	IN PULONG Disposition OPTIONAL
    	);


//NtCreateMailslotFile

/*
 * FUNCTION: Creates or opens a mutex
 * ARGUMENTS:
 *        MutantHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the port
 *        ObjectAttributes = Contains the name of the mutex.
 *        InitialOwner = If true the calling thread acquires ownership 
 *			of the mutex.
 * REMARKS: This funciton maps to the win32 function CreateMutex
 * RETURNS:
 *	Status
 */
NTSTATUS
STDCALL
NtCreateMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOL InitialOwner
	);

//NtCreateNamedPipeFile

/*
 * FUNCTION: Creates a paging file.
 * ARGUMENTS:
 *        PageFileName  = Name of the pagefile
 *        MinimumSize = Specifies the minimum size
 *        MaximumSize = Specifies the maximum size
 *        ActualSize  = Specifies the actual size 
 * RETURNS: Status
*/
NTSTATUS 
STDCALL 
NtCreatePagingFile(
	IN PUNICODE_STRING PageFileName,
	IN ULONG MiniumSize,
	IN ULONG MaxiumSize,
	OUT PULONG ActualSize
	);
/*
 * FUNCTION: Creates a process.
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the process can
 *                        be a combinate of STANDARD_RIGHTS_REQUIRED| ..  
 *        ObjectAttribute = Initialized attributes for the object, contains the rootdirectory and the filename
 *        ParentProcess = Handle to the parent process.
 *        InheritObjectTable = Specifies to inherit the objects of the parent process if true.
 *        SectionHandle = Handle to a section object to back the image file
 *        DebugPort = Handle to a DebugPort if NULL the system default debug port will be used.
 *        ExceptionPort = Handle to a exception port. 
 * REMARKS:
 *        This function maps to the win32 CreateProcess. 
 * RETURNS: Status
 */
NTSTATUS 
STDCALL 
NtCreateProcess(
	OUT PHANDLE ProcessHandle,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        IN HANDLE ParentProcess,
        IN BOOLEAN InheritObjectTable,
        IN HANDLE SectionHandle OPTIONAL,
        IN HANDLE DebugPort OPTIONAL,
        IN HANDLE ExceptionPort OPTIONAL
	);
/*
 * FUNCTION: Creates a section object.
 * ARGUMENTS:
 *        SectionHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the desired access to the section can be a combination of STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_WRITE |  
 *                        SECTION_MAP_READ | SECTION_MAP_EXECUTE.
 *        ObjectAttribute = Initialized attributes for the object can be used to create a named section
 *        MaxiumSize = Maximizes the size of the memory section. Must be non-NULL for a page-file backed section. 
 *                     If value specified for a mapped file and the file is not large enough, file will be extended. 
 *        SectionPageProtection = Can be a combination of PAGE_READONLY | PAGE_READWRITE | PAGE_WRITEONLY | PAGE_WRITECOPY.
 *        AllocationAttributes = can be a combination of SEC_IMAGE | SEC_RESERVE
 *        FileHanlde = Handle to a file to create a section mapped to a file instead of a memory backed section.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCreateSection( 
	OUT PHANDLE SectionHandle, 
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,  
	IN PLARGE_INTEGER MaximumSize OPTIONAL,  
	IN ULONG SectionPageProtection OPTIONAL,
	IN ULONG AllocationAttributes,
	IN HANDLE FileHandle OPTIONAL
	);
/*
 * FUNCTION: Creates a semaphore object for interprocess synchronization.
 * ARGUMENTS:
 *        SemaphoreHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the semaphore. 
 *        ObjectAttribute = Initialized attributes for the object.
 *        InitialCount = Not necessary zero, might be smaller than zero.
 *        MaximumCount = Maxiumum count the semaphore can reach.
 * RETURNS: Status
 * REMARKS: 
 *        The semaphore is set to signaled when its count is greater than zero, and non-signaled when its count is zero.
 */

//FIXME: should a semaphore's initial count allowed to be smaller than zero ??
NTSTATUS
STDCALL
NtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
	);
/*
 * FUNCTION: Creates a symbolic link object
 * ARGUMENTS:
 *        SymbolicLinkHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the thread. 
 *        ObjectAttributes = Initialized attributes for the object.
 *        Name = 
 * REMARKS:
 *        This function map to the win32 function CreateThread. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCreateSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING Name
	);
/*
 * FUNCTION: Creates a user mode thread
 * ARGUMENTS:
 *        ThreadHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the thread. 
 *        ObjectAttributes = Initialized attributes for the object.
 *        ProcessHandle = Handle to the threads parent process.
 *        ClientId      = Caller supplies storage for process id and thread id.
 *        ThreadContext = Initial processor context for the thread.
 *        InitialTeb = Initial Thread Environment Block for the Thread.
 *        CreateSuspended = Specifies if the thread is ready for scheduling
 * REMARKS:
 *        This function maps to the win32 function CreateThread.  The exact arguments are from the usenet. [<6f7cqj$tq9$1@nnrp1.dejanews.com>] 
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtCreateThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ProcessHandle,
	IN PCLIENT_ID ClientId,
	IN PCONTEXT ThreadContext,
	IN PINITIAL_TEB InitialTeb,
	IN BOOLEAN CreateSuspended
	);
/*
 * FUNCTION: Creates a waitable timer.
 * ARGUMENTS:
 *        TimerHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the timer. 
 *        ObjectAttributes = Initialized attributes for the object.
 *        ManualReset = Specifies if the timer should be reset manually.
 * REMARKS:
 *       This function maps to the win32  CreateWaitableTimer. lpTimerAttributes and lpTimerName map to
 *       corresponding fields in OBJECT_ATTRIBUTES structure. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL 
NtCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN BOOL ManualReset 
	);
/*
 * FUNCTION: Returns the callers thread TEB.
 * ARGUMENTS:
 *        Teb (OUT) = Caller supplied storage for the resulting TEB.
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtCurrentTeb(
	NT_TEB *CurrentTeb
	);

/*
 * FUNCTION: Delays the execution of the calling thread.
 * ARGUMENTS:
 *        Alertable = If TRUE the thread is alertable during is wait period
 *        Interval  = Specifies the interval to wait.      
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDelayExecution(
	IN BOOL Alertable,
	IN PLARGE_INTEGER Interval
	);

/*
 * FUNCTION: Deletes an atom from the global atom table
 * ARGUMENTS:
 *        Atom = Atom to delete
 * REMARKS:
	 The function maps to the win32 GlobalDeleteAtom
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDeleteAtom(
	IN ATOM Atom
	);

/*
 * FUNCTION: Deletes a file
 * ARGUMENTS:
 *        FileHandle = Handle to the file which should be deleted
 * REMARKS:
 *	 The function maps to the win32 DeleteFile
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDeleteFile(
	IN HANDLE FileHandle
	);

/*
 * FUNCTION: Deletes a registry key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDeleteKey(
	IN HANDLE KeyHandle
	);
/*
 * FUNCTION: Deletes a value from a registry key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key
 *         ValueName = Name of the value to delete
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName
	);
/*
 * FUNCTION: Sends IOCTL to the io sub system
 * ARGUMENTS:
 *        DeviceHandle = Points to the handle that is created by NtCreateFile
 *        Event = Event to synchronize on STATUS_PENDING
 *        ApcRoutine = 
 *	  ApcContext =
 *	  IoStatusBlock = Caller should supply storage for 
 *        IoControlCode = Contains the IO Control command. This is an 
 *			index to the structures in InputBuffer and OutputBuffer.
 *	  InputBuffer = Caller should supply storage for input buffer if IOTL expects one.
 * 	  InputBufferSize = Size of the input bufffer
 *        OutputBuffer = Caller should supply storage for output buffer if IOTL expects one.
 *        OutputBufferSize  = Size of the input bufffer
 * RETURNS: Status 
 */

NTSTATUS
STDCALL
NtDeviceIoControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	);
/*
 * FUNCTION: Displays a string on the blue screen
 * ARGUMENTS:
 *         DisplayString = The string to display
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtDisplayString(
	IN PUNICODE_STRING DisplayString
	);
/*
 * FUNCTION: Displays a string on the blue screen
 * ARGUMENTS:
 *         SourceProcessHandle = The string to display
	   SourceHandle =
	   TargetProcessHandle =
	   TargetHandle = 
	   DesiredAccess = 
	   InheritHandle = 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtDuplicateObject(
	IN HANDLE SourceProcessHandle,
	IN PHANDLE SourceHandle,
	IN HANDLE TargetProcessHandle,
	OUT PHANDLE TargetHandle,
	IN ULONG DesiredAccess,
	IN BOOL InheritHandle
	);
/*
 * FUNCTION: Returns information about the subkeys of an open key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key whose subkeys are to enumerated
 *         Index = zero based index of the subkey for which information is
 *                 request
 *         KeyInformationClass = Type of information returned
 *         KeyInformation (OUT) = Caller allocated buffer for the information
 *                                about the key
 *         Length = Length in bytes of the KeyInformation buffer
 *         ResultLength (OUT) = Caller allocated storage which holds
 *                              the number of bytes of information retrieved
 *                              on return
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtEnumerateKey(
	IN HANDLE KeyHandle,
	IN ULONG Index,
	IN CINT KeyInformationClass,
	OUT PVOID KeyInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);
/*
 * FUNCTION: Returns information about the value entries of an open key
 * ARGUMENTS:
 *         KeyHandle = Handle of the key whose value entries are to enumerated
 *         Index = zero based index of the subkey for which information is
 *                 request
 *         KeyInformationClass = Type of information returned
 *         KeyInformation (OUT) = Caller allocated buffer for the information
 *                                about the key
 *         Length = Length in bytes of the KeyInformation buffer
 *         ResultLength (OUT) = Caller allocated storage which holds
 *                              the number of bytes of information retrieved
 *                              on return
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtEnumerateValueKey(
	IN HANDLE KeyHandle,
	IN ULONG Index,
	IN CINT KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);
/*
 * FUNCTION: Extends a section
 * ARGUMENTS:
 *       SectionHandle = Handle to the section
 *	 NewMaximumSize = Adjusted size
 * RETURNS: Status 
 */
NTSTATUS
STDCALL
NtExtendSection(
	IN HANDLE SectionHandle,
	IN ULONG NewMaximumSize
	);
/*
 * FUNCTION: Finds a atom
 * ARGUMENTS:
 *       Atom = Caller supplies storage for the resulting atom
 *	 AtomString = String to search for.
 * RETURNS: Status 
 * REMARKS:
	This funciton maps to the win32 GlobalFindAtom
 */
NTSTATUS
STDCALL
NtFindAtom(
	OUT ATOM *Atom,
	IN PUNICODE_STRING AtomString
	);
/*
 * FUNCTION: Flushes chached file data to disk
 * ARGUMENTS:
 *       FileHandle = Points to the file
 * RETURNS: Status 
 * REMARKS:
	This funciton maps to the win32 FlushFileBuffers
 */
NTSTATUS
STDCALL
NtFlushBuffersFile(
	IN HANDLE FileHandle
	);
/*
 * FUNCTION: Flushes a the processors instruction cache
 * ARGUMENTS:
 *       ProcessHandle = Points to the process owning the cache
	 BaseAddress = // might this be a image address ????
	 NumberOfBytesToFlush = 
 * RETURNS: Status 
 * REMARKS:
	This funciton is used by debuggers
 */
NTSTATUS
STDCALL
NtFlushInstructionCache(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN UINT NumberOfBytesToFlush
	);
/*
 * FUNCTION: Flushes a registry key to disk
 * ARGUMENTS:
 *       KeyHandle = Points to the registry key handle
 * RETURNS: Status 
 * REMARKS:
	This funciton maps to the win32 RegFlushKey.
 */
NTSTATUS
STDCALL
NtFlushKey(
	IN HANDLE KeyHandle
	);

/*
 * FUNCTION: Flushes virtual memory to file
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFlush = Limits the range to flush,
 *        NumberOfBytesFlushed = Actual number of bytes flushed
 * RETURNS: Status 
 * REMARKS:
	  Check return status on STATUS_NOT_MAPPED_DATA 
 */
NTSTATUS
STDCALL
NtFlushVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG NumberOfBytesToFlush,
	OUT PULONG NumberOfBytesFlushed OPTIONAL
	);
/*
 * FUNCTION: Flushes virtual memory to file
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFlush = Limits the range to flush,
 *        NumberOfBytesFlushed = Actual number of bytes flushed
 * RETURNS: Status 
 * REMARKS:
	  Check return status on STATUS_NOT_MAPPED_DATA 
 */
VOID
STDCALL                                            
NtFlushWriteBuffer (                            
	VOID                                        
	);              
/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFree = Limits the range to free,
 *        NumberOfBytesFreed = Actual number of bytes freed
 * RETURNS: Status 
 */

NTSTATUS
STDCALL
NtFreeVirtualMemory(
	IN PHANDLE ProcessHandle,
	IN PVOID  BaseAddress,	
	IN ULONG  NumberOfBytesToFree,	
	OUT PULONG  NumberOfBytesFreeed OPTIONAL
	); 

/*
 * FUNCTION: Sends FSCTL to the filesystem
 * ARGUMENTS:
 *        DeviceHandle = Points to the handle that is created by NtCreateFile
 *        Event = Event to synchronize on STATUS_PENDING
 *        ApcRoutine = 
 *	  ApcContext =
 *	  IoStatusBlock = Caller should supply storage for 
 *        IoControlCode = Contains the File System Control command. This is an 
			index to the structures in InputBuffer and OutputBuffer.
		FSCTL_GET_RETRIEVAL_POINTERS  	MAPPING_PAIR
		FSCTL_GET_RETRIEVAL_POINTERS  	GET_RETRIEVAL_DESCRIPTOR
		FSCTL_GET_VOLUME_BITMAP  	BITMAP_DESCRIPTOR
		FSCTL_MOVE_FILE  		MOVEFILE_DESCRIPTOR

 *	  InputBuffer = Caller should supply storage for input buffer if FCTL expects one.
 * 	  InputBufferSize = Size of the input bufffer
 *        OutputBuffer = Caller should supply storage for output buffer if FCTL expects one.
 *        OutputBufferSize  = Size of the input bufffer
 * RETURNS: Status 
 */
NTSTATUS
STDCALL
NtFsControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	);

/*
 * FUNCTION: Retrieves the processor context of a thread
 * ARGUMENTS:
 *        ThreadHandle = Handle to a thread
 *        Context (OUT) = Caller allocated storage for the processor context
 * RETURNS: Status 
 */

NTSTATUS
STDCALL 
NtGetContextThread(
	IN HANDLE ThreadHandle, 
	OUT PCONTEXT Context
	);
/*
 * FUNCTION: Retrieves the uptime of the system
 * ARGUMENTS:
 *        UpTime = Number of clock ticks since boot.
 * RETURNS: Status 
 */
NTSTATUS
STDCALL 
NtGetTickCount(
	PULONG UpTime
	);

//-- NtImpersonateThread

/*
 * FUNCTION: Initializes the registry.
 * ARGUMENTS:
 *        SetUpBoot = This parameter is true for a setup boot.
 * RETURNS: Status 
 */
NTSTATUS
STDCALL 
NtInitializeRegistry(
	BOOL SetUpBoot
	);
/*
 * FUNCTION: Loads a driver. 
 * ARGUMENTS: 
 *      DriverServiceName = Name of the driver to load
 * RETURNS: Status
 */	
NTSTATUS
STDCALL 
NtLoadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

//-- NtLoadKey2
/*
 * FUNCTION: Loads a registry key. 
 * ARGUMENTS: 
 *       KeyHandle = Handle to the registry key
 *	 ObjectAttributes = ???
 * REMARK:
 *	This procedure maps to the win32 procedure RegLoadKey 
 * RETURNS: Status
 */	
NTSTATUS
STDCALL 
NtLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Locks a range of bytes in a file. 
 * ARGUMENTS: 
 *       FileHandle = Handle to the file
 *       Event = Should be null if apc is specified.
 *       ApcRoutine = Asynchroneous Procedure Callback
 *       ApcContext = Argument to the callback
 *       IoStatusBlock (OUT) = Caller should supply storage for a structure containing
 *			 the completion status and information about the requested lock operation.
 *       ByteOffsetOfLock = Offset 
 *       LengthOfLock = Number of bytes to lock.
 *       KeyOfLock  = 
 *       DoNotWaitIfHeld =
 *       IsExclusiveLock =
 * REMARK:
 *	This procedure maps to the win32 procedure LockFileEx 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL
NtLockFile(
  IN  HANDLE FileHandle,
  IN  HANDLE Event OPTIONAL,
  IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN  PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN  PLARGE_INTEGER ByteOffsetOfLock,
  IN  PLARGE_INTEGER LengthOfLock,
  IN  ULONG KeyOfLock,
  IN  BOOLEAN DoNotWaitIfHeld,
  IN  BOOLEAN IsExclusiveLock
  );
/*
 * FUNCTION: Locks a range of virtual memory. 
 * ARGUMENTS: 
 *       ProcessHandle = Handle to the process
 *       BaseAddress =  Lower boundary of the range of bytes to lock. 
 *       NumberOfBytesLock = Offset to the upper boundary.
 *       NumberOfBytesLocked (OUT) = Number of bytes actually locked.
 * REMARK:
 *	This procedure maps to the win32 procedure VirtualLock 
 * RETURNS: Status
 */	
NTSTATUS
STDCALL 
NtLockVirtualMemory(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	ULONG NumberOfBytesToLock,
	PULONG NumberOfBytesLocked
	);
/*
 * FUNCTION: Makes temporary object that will be removed at next boot.
 * ARGUMENTS: 
 *       Handle = Handle to object
 * RETURNS: Status
 */	

NTSTATUS
STDCALL
NtMakeTemporaryObject(
	IN HANDLE Handle 
	);

  
/*
 * FUNCTION: Maps a view of a section into the virtual address space of a 
 *           process
 * ARGUMENTS:
 *        SectionHandle = Handle of the section
 *        ProcessHandle = Handle of the process
 *        BaseAddress = Desired base address (or NULL) on entry
 *                      Actual base address of the view on exit
 *        ZeroBits = Number of high order address bits that must be zero
 *        CommitSize = Size in bytes of the initially committed section of 
 *                     the view 
 *        SectionOffset = Offset in bytes from the beginning of the section
 *                        to the beginning of the view
 *        ViewSize = Desired length of map (or zero to map all) on entry
 *                   Actual length mapped on exit
 *        InheritDisposition = Specified how the view is to be shared with
 *                            child processes
 *        AllocateType = Type of allocation for the pages
 *        Protect = Protection for the committed region of the view
 * RETURNS: Status
 */
NTSTATUS STDCALL
NtMapViewOfSection(
     IN HANDLE SectionHandle,
     IN HANDLE ProcessHandle,
     IN OUT PVOID *BaseAddress,
     IN ULONG ZeroBits,
     IN ULONG CommitSize,
     IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
     IN OUT PULONG ViewSize,
     IN SECTION_INHERIT InheritDisposition,
     IN ULONG AllocationType,
     IN ULONG AccessProtection
    );

/*
 * FUNCTION: Installs a notify for the change of a directory's contents
 * ARGUMENTS:
 *        FileHandle = Handle to the directory
 *	  Event = 
 *        UserApcRoutine   = Start address
 *        UserApcContext = Delimits the range of virtual memory
 *				for which the new access protection holds
 *        IoStatusBlock = The new access proctection for the pages
 *        Buffer = Caller supplies storage for resulting information --> FILE_NOTIFY_INFORMATION
 *	  BufferSize = 	Size of the buffer
 *	  NotifyFilter = // might be completionfileter
 *	  WatchSubtree =
 *
 * REMARKS:
 *	 The function maps to the win32 FindFirstChangeNotification, FindNextChangeNotification 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtNotifyChangeDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	IN ULONG NotifyFilter,
	IN BOOL WatchSubtree
	);

//NtNotifyChangeKey
/*
 * FUNCTION: Opens an existing directory object
 * ARGUMENTS:
 *        FileHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the directory
 *        ObjectAttributes = Initialized attributes for the object
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtOpenDirectoryObject(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing event
 * ARGUMENTS:
 *        EventHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the event
 *        ObjectAttributes = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenEvent(	
	OUT PHANDLE EventHandle,
        IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing file
 * ARGUMENTS:
 *        FileHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the file
 *        ObjectAttributes = Initialized attributes for the object
 *        IoStatusBlock =
 *        ShareAccess =
 *        FileAttributes =
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,   
	IN ULONG ShareAccess,         
   	IN ULONG FileAttributes                                                                    
	);
//--> NtOpenIoCompletion
/*
 * FUNCTION: Opens an existing key in the registry
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the key
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenKey(
	OUT PHANDLE KeyHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing key in the registry
 * ARGUMENTS:
 *        MutantHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the mutant
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
	
/*
 * FUNCTION: Opens an existing process
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the process
 *        ObjectAttribute = Initialized attributes for the object
 *        ClientId = storage for the process id and the thread id
 * RETURNS: Status
 */
NTSTATUS 
STDCALL
NtOpenProcess (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	); 
/*
 * FUNCTION: Opens an existing section object
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the key
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenSection(
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing semaphore
 * ARGUMENTS:
 *        SemaphoreHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the semaphore
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing symbolic link
 * ARGUMENTS:
 *        SymbolicLinkHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the symbolic link
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing thread
 * ARGUMENTS:
 *        ThreadHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the thread
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
/*
 * FUNCTION: Opens an existing timer
 * ARGUMENTS:
 *        TimerHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the timer
 *        ObjectAttribute = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
    	);
/*
 * FUNCTION: Entry point for native applications
 * ARGUMENTS:
 *	Argument = Arguments passed to the application by the system [ at boot time ]
 * REMARKS:
 *	 Native applications should use this function instead of a main.
 * RETURNS: Status
 */ 	
void NtProcessStartup( 
	IN PSTARTUP_ARGUMENT Argument 
	);

/*
 * FUNCTION: Set the access protection of a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Handle to process owning the virtual address space
 *        BaseAddress   = Start address
 *        NumberOfBytesToProtect = Delimits the range of virtual memory
 *				for which the new access protection holds
 *        NewAccessProtection = The new access proctection for the pages
 *        OldAccessProtection = Caller should supply storage for the old 
 *				access protection
 *
 * REMARKS:
 *	 The function maps to the win32 VirtualProtectEx
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtProtectVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG NumberOfBytesToProtect,
	IN ULONG NewAccessProtection,
	OUT PULONG OldAccessProtection 
	);

/*
 * FUNCTION: Signals an event and resets it afterwards.
 * ARGUMENTS:
 *        EventHandle  = Handle to the event
 *        PulseCount = Number of times the action should be repeated
 * RETURNS: Status
 */
NTSTATUS 
STDCALL 
NtPulseEvent(
	IN HANDLE EventHandle,
	IN PULONG PulseCount OPTIONAL
	);

//-- NtQueryAttributesFile
//-- NtQueryDirectoryFile
/*
 * FUNCTION: Query information about the content of a directory object
 * ARGUMENTS:
	DirObjInformation =   Buffer must be large enough to hold the name strings too
        GetNextIndex = If TRUE :return the index of the next object in this directory in ObjectIndex
		       If FALSE:  return the number of objects in this directory in ObjectIndex
        IgnoreInputIndex= If TRUE:  ignore input value of ObjectIndex  always start at index 0
         		  If FALSE use input value of ObjectIndex
	ObjectIndex =   zero based index of object in the directory  depends on GetNextIndex and IgnoreInputIndex
        DataWritten  = Actual size of the ObjectIndex ???
 * RETURNS: Status
 */
NTSTATUS 
STDCALL 
NtQueryDirectoryObject(
	IN HANDLE DirObjHandle,
	OUT POBJDIR_INFORMATION DirObjInformation, 
	IN ULONG                BufferLength, 
	IN BOOLEAN              GetNextIndex, 
	IN BOOLEAN              IgnoreInputIndex, 
	IN OUT PULONG           ObjectIndex,
	OUT PULONG              DataWritten OPTIONAL
	); 


//-- NtQueryEaFile
/*
 * FUNCTION: Queries an event
 * ARGUMENTS:
 *        EventHandle  = Handle to the event
 *        EventInformationClass = Index of the information structure
 *	  EventInformation = Caller supplies storage for the information structure
 *	  EventInformationLength =  Size of the information structure
 *	  ReturnLength = Data written
 * RETURNS: Status
 */


NTSTATUS
STDCALL
NtQueryEvent(
	IN HANDLE EventHandle,
	IN CINT EventInformationClass,
	OUT PVOID EventInformation,
	IN ULONG EventInformationLength,
	OUT PULONG ReturnLength
	); 
//-- NtQueryFullAttributesFile
//-- NtQueryInformationAtom



/*
 * FUNCTION: Queries the information of a file object.
 * ARGUMENTS: 
 *        FileHandle = Handle to the file object
 *	  IoStatusBlock = Caller supplies storage for extended information 
 *                        on the current operation.
 *        FileInformation = Storage for the new file information
 *        Lenght = Size of the storage for the file information.
 *        FileInformationClass = Indicates which file information is queried

	  FileDirectoryInformation 		
	  FileFullDirectoryInformation		
	  FileBothDirectoryInformation		
	  FileBasicInformation			FILE_BASIC_INFORMATION
	  FileStandardInformation		FILE_STANDARD_INFORMATION
	  FileInternalInformation		FILE_INTERNAL_INFORMATION
	  FileEaInformation			FILE_EA_INFORMATION
	  FileAccessInformation			FILE_ACCESS_INFORMATION
	  FileNameInformation 			FILE_NAME_INFORMATION
	  FileRenameInformation			FILE_RENAME_INFORMATION
	  FileLinkInformation			
	  FileNamesInformation			
	  FileDispositionInformation		FILE_DISPOSITION_INFORMATION
	  FilePositionInformation		FILE_POSITION_INFORMATION
	  FileFullEaInformation			FILE_FULL_EA_INFORMATION		
	  FileModeInformation			FILE_MODE_INFORMATION
	  FileAlignmentInformation		FILE_ALIGNMENT_INFORMATION
	  FileAllInformation			FILE_ALL_INFORMATION
	  FileAllocationInformation		FILE_ALLOCATION_INFORMATION
	  FileEndOfFileInformation		FILE_END_OF_FILE_INFORMATION
	  FileAlternateNameInformation		
	  FileStreamInformation			FILE_STREAM_INFORMATION
	  FilePipeInformation			
	  FilePipeLocalInformation		
	  FilePipeRemoteInformation		
	  FileMailslotQueryInformation		
	  FileMailslotSetInformation		
	  FileCompressionInformation		FILE_COMPRESSION_INFORMATION		
	  FileCopyOnWriteInformation		
	  FileCompletionInformation 		IO_COMPLETION_CONTEXT
	  FileMoveClusterInformation		
	  FileOleClassIdInformation		
	  FileOleStateBitsInformation		
	  FileNetworkOpenInformation		FILE_NETWORK_OPEN_INFORMATION
	  FileObjectIdInformation		
	  FileOleAllInformation			
	  FileOleDirectoryInformation		
	  FileContentIndexInformation		
	  FileInheritContentIndexInformation	
	  FileOleInformation			
	  FileMaximumInformation			

 * REMARK:
 *	  This procedure maps to the win32 GetShortPathName, GetLongPathName,
          GetFullPathName, GetFileType, GetFileSize, GetFileTime  functions. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtQueryInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN CINT FileInformationClass
    );

/*
 * FUNCTION: Queries the information of a process object.
 * ARGUMENTS: 
 *        ProcessHandle = Handle to the process object
 *        ProcessInformation = Index to a certain information structure

		ProcessBasicInformation 	 PROCESS_BASIC_INFORMATION
		ProcessQuotaLimits 		 QUOTA_LIMITS
		ProcessIoCounters 		 IO_COUNTERS
		ProcessVmCounters 		 VM_COUNTERS
		ProcessTimes 			 KERNEL_USER_TIMES
		ProcessBasePriority		 KPRIORITY
		ProcessBasePriority		 KPRIORITY
		ProcessRaisePriority		 KPRIORITY
		ProcessDebugPort		 HANDLE
		ProcessExceptionPort		 HANDLE	
		ProcessAccessToken		 PROCESS_ACCESS_TOKEN
		ProcessLdtInformation		 LDT_ENTRY ??
		ProcessLdtSize			 ??
		ProcessDefaultHardErrorMode	 ULONG
		ProcessIoPortHandlers		 // kernel mode only
		ProcessPooledUsageAndLimits 	 POOLED_USAGE_AND_LIMITS
		ProcessWorkingSetWatch 		 PROCESS_WS_WATCH_INFORMATION 		
		ProcessUserModeIOPL		 (I/O Privilege Level)
		ProcessEnableAlignmentFaultFixup BOOLEAN	
		ProcessPriorityClass		 ULONG
		ProcessWx86Information			
		ProcessHandleCount		 ULONG
		ProcessAffinityMask		 ULONG	
		ProcessPooledQuotaLimits 	 QUOTA_LIMITS
		MaxProcessInfoClass		 ??

 *        ProcessInformation = Caller supplies storage for the process information structure
 *	  ProcessInformationLength = Size of the process information structure
 *        ReturnLength  = Actual number of bytes written
		
 * REMARK:
 *	  This procedure maps to the win32 GetProcessTimes, GetProcessVersion,
          GetProcessWorkingSetSize, GetProcessPriorityBoost, GetProcessAffinityMask, GetPriorityClass,
          GetProcessShutdownParameters  functions. 
 * RETURNS: Status
*/



NTSTATUS
STDCALL
NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN CINT ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength 
	);

/*
 * FUNCTION: Queries the information of a thread object.
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread object
 *        ThreadInformationClass = Index to a certain information structure

		ThreadBasicInformation			
		ThreadTimes 			KERNEL_USER_TIMES
		ThreadPriority			KPRIORITY	
		ThreadBasePriority		KPRIORITY	
		ThreadAffinityMask		KAFFINITY	
		ThreadImpersonationToken		
		ThreadDescriptorTableEntry		
		ThreadEnableAlignmentFaultFixup		
		ThreadEventPair				
		ThreadQuerySetWin32StartAddress		
		ThreadZeroTlsCell			
		ThreadPerformanceCount			
		ThreadAmILastThread			
		ThreadIdealProcessor			
		ThreadPriorityBoost			
		MaxThreadInfoClass			
		

 *        ThreadInformation = Caller supplies torage for the thread information
 *	  ThreadInformationLength = Size of the thread information structure
 *        ReturnLength  = Actual number of bytes written
		
 * REMARK:
 *	  This procedure maps to the win32 GetThreadTimes, GetThreadPriority,
          GetThreadPriorityBoost   functions. 
 * RETURNS: Status
*/


NTSTATUS
STDCALL
NtQueryInformationThread(
	IN HANDLE ThreadHandle,
	IN CINT ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength 
	);
// NtQueryInformationToken
//-- NtQueryIoCompletion
/*
 * FUNCTION: Queries the information of a registry key object.
 * ARGUMENTS: 
	KeyHandle = Handle to a registry key
	KeyInformationClass = Index to a certain information structure
	KeyInformation = Caller supplies storage for resulting information
	Length = Size of the supplied storage 
	ResultLength = Bytes written
 */
NTSTATUS
STDCALL
NtQueryKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	OUT PVOID KeyInformation,
	IN ULONG Length,
	OUT PULONG ResultLength 
	);
//-- NtQueryMultipleValueKey
//-- NtQueryMutant


/*
 * FUNCTION: Queries the information of a  object.
 * ARGUMENTS: 
	ObjectHandle = Handle to a object
	HandleInformationClass = Index to a certain information structure

	HandleBasicInformation  	HANDLE_BASIC_INFORMATION
	HandleTypeInormation 		HANDLE_TYPE_INFORMATION 

	HandleInformation = Caller supplies storage for resulting information
	Length = Size of the supplied storage 
 	ResultLength = Bytes written
 */

NTSTATUS
STDCALL
NtQueryObject(
	IN HANDLE ObjectHandle,
	IN CINT HandleInformationClass,
	OUT PVOID HandleInformationPtr,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

/*
 * FUNCTION: Queries the system ( high-resolution ) performance counter.
 * ARGUMENTS: 
 *        Counter = Performance counter
 *	  Frequency = Performance frequency
 * REMARKS:
	This procedure queries a tick count faster than 10ms ( The resolution for  Intel®-based CPUs is about 0.8 microseconds.)
	This procedure maps to the win32 QueryPerformanceCounter, QueryPerformanceFrequency 
 * RETURNS: Status
 *
*/    
NTSTATUS
STDCALL
NtQueryPerformanceCounter(
	IN PLARGE_INTEGER Counter,
	IN PLARGE_INTEGER Frequency
	);
/*
 * FUNCTION: Queries the information of a section object.
 * ARGUMENTS: 
 *        SectionHandle = Handle to the section link object
 *	  SectionInformationClass = Index to a certain information structure
 *        SectionInformation (OUT)= Caller supplies storage for resulting information
 *        Length =  Size of the supplied storage 
 *        ResultLength = Data written
 * RETURNS: Status
 *
*/    
NTSTATUS
STDCALL
NtQuerySection(
	IN HANDLE SectionHandle,
	IN CINT SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG Length, 
	OUT PULONG ResultLength 
	);


//-- NtQuerySemaphore
/*
 * FUNCTION: Queries the information of a symbolic link object.
 * ARGUMENTS: 
 *        SymbolicLinkHandle = Handle to the symbolic link object
 *	  LinkName = resolved name of link
 *        DataWritten = size of the LinkName.
 * RETURNS: Status
 *
*/                       
NTSTATUS
STDCALL 
NtQuerySymbolicLinkObject(
	IN HANDLE               SymLinkObjHandle,
	OUT PUNICODE_STRING     LinkName,    
	OUT PULONG              DataWritten OPTIONAL
	); 
//-- NtQuerySystemEnvironmentValue


/*
 * FUNCTION: Queries the system information.
 * ARGUMENTS: 
 *        SystemInformationClass = Index to a certain information structure

	  SystemTimeAdjustmentInformation 	SYSTEM_TIME_ADJUSTMENT
	  SystemCacheInformation 		SYSTEM_CACHE_INFORMATION

 *	  SystemInformation = caller supplies storage for the information structure
 *        Length = size of the structure
	  ResultLength = Data written
 * RETURNS: Status
 *
*/        
NTSTATUS
STDCALL
NtQuerySystemInformation(
	IN CINT SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

/*
 * FUNCTION: Retrieves the system time
 * ARGUMENTS: 
 *        CurrentTime (OUT) = Caller should supply storage for the resulting time.
 * RETURNS: Status
 *
*/        

NTSTATUS
STDCALL
NtQuerySystemTime (
	OUT PLARGE_INTEGER CurrentTime
	);
//-- NtQueryTimer

/*
 * FUNCTION: Queries the timer resolution
 * ARGUMENTS: 
 *        MinimumResolution (OUT) = Caller should supply storage for the 
 *                                  resulting time.
 *	  Maximum Resolution (OUT) = Caller should supply storage for the 
 *                                   resulting time.
 *	  ActualResolution (OUT) = Caller should supply storage for the 
 *                                 resulting time.
 * RETURNS: Status
 *
 */        


NTSTATUS 
NtQueryTimerResolution ( 
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution,
	OUT PULONG ActualResolution 
	); 

/*
 * FUNCTION: Queries a registry key value
 * ARGUMENTS: 
 *        KeyHandle  = Handle to the registry key
	  ValueName = Name of the value in the registry key
	  KeyValueInformationClass = Index to a certain information structure

		KeyValueBasicInformation = KEY_VALUE_BASIC_INFORMATION
    		KeyValueFullInformation = KEY_FULL_INFORMATION
    		KeyValuePartialInformation = KEY_VALUE_PARTIAL_INFORMATION

	  KeyValueInformation = Caller supplies storage for the information structure
	  Length = Size of the information structure
	  ResultLength = Data written
 * RETURNS: Status
 *
*/       
NTSTATUS
STDCALL
NtQueryValueKey(
    	IN HANDLE KeyHandle,
    	IN PUNICODE_STRING ValueName,
    	IN CINT KeyValueInformationClass,
    	OUT PVOID KeyValueInformation,
    	IN ULONG Length,
	OUT PULONG ResultLength
    	);



/*
 * FUNCTION: Queries the virtual memory information.
 * ARGUMENTS: 
	  ProcessHandle = Process owning the virtual address space
	  BaseAddress = Points to the page where the information is queried for. 
 *        VirtualMemoryInformationClass = Index to a certain information structure

	  MemoryBasicInformation		MEMORY_BASIC_INFORMATION

 *	  VirtualMemoryInformation = caller supplies storage for the information structure
 *        Length = size of the structure
	  ResultLength = Data written
 * RETURNS: Status
 *
*/        

NTSTATUS
STDCALL
NtQueryVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID Address,
	IN IN CINT VirtualMemoryInformationClass,
	OUT PVOID VirtualMemoryInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);
/*
 * FUNCTION: Queries the volume information
 * ARGUMENTS: 
 *        FileHandle  = 
	  ReturnLength = DataWritten
	  FSInformation = Caller should supply storage for the information structure.
	  Length = Size of the information structure
	  FSInformationClass = Index to a information structure

		FileFsVolumeInformation		FILE_FS_VOLUME_INFORMATION
		FileFsLabelInformation			
		FileFsSizeInformation		FILE_FS_SIZE_INFORMATION
		FileFsDeviceInformation		FILE_FS_DEVICE_INFORMATION
		FileFsAttributeInformation	FILE_FS_ATTRIBUTE_INFORMATION
		FileFsControlInformation	
		FileFsQuotaQueryInformation	
		FileFsQuotaSetInformation	
		FileFsMaximumInformation	

 * RETURNS: Status
 *
*/     
NTSTATUS
STDCALL
NtQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FSInformation,
	IN ULONG Length,
	IN CINT FSInformationClass 
    );
// NtQueueApcThread
/*
 * FUNCTION: Raises an exception
 * ARGUMENTS: 
	  ExceptionRecord = Structure specifying the exception
	  Context = Context in which the excpetion is raised 
 *        IsDebugger = 
 * RETURNS: Status
 *
*/        


NTSTATUS
STDCALL
NtRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT Context,
	IN BOOL IsDebugger OPTIONAL
	);

//NtRaiseHardError
/*
 * FUNCTION: Read a file
 * ARGUMENTS: 
	  FileHandle = Handle of a file to read
	  Event = This event is signalled when the read operation completes
 *        UserApcRoutine = Call back , if supplied Event should be NULL
	  UserApcContext = Argument to the callback
	  IoStatusBlock = Caller should supply storage for additional status information
	  Buffer = Caller should supply storage to receive the information
	  BufferLength = Size of the buffer
	  ByteOffset = Offset to start reading the file
	  Key =  unused
 * RETURNS: Status
 *
*/       


NTSTATUS
STDCALL
NtReadFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
	IN PVOID UserApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG BufferLength,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN PULONG Key OPTIONAL	
	);
/*
 * FUNCTION: Read a file using scattered io
 * ARGUMENTS: 
	  FileHandle = Handle of a file to read
	  Event = This event is signalled when the read operation completes
 *        UserApcRoutine = Call back , if supplied Event should be NULL
	  UserApcContext = Argument to the callback
	  IoStatusBlock = Caller should supply storage for additional status information
	  BufferDescription = Caller should supply storage to receive the information
	  BufferLength = Size of the buffer
	  ByteOffset = Offset to start reading the file
	  Key =  unused
 * RETURNS: Status
 *
*/       
NTSTATUS
STDCALL
NtReadFileScatter( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN  PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
	IN LARGE_INTEGER BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL	
	); 
/*
 * FUNCTION: Copies a range of virtual memory to a buffer
 * ARGUMENTS: 
 *       ProcessHandle = Specifies the process owning the virtual address space
 *       BaseAddress =  Points to the address of virtual memory to start the read
 *       Buffer = Caller supplies storage to copy the virtual memory to.
 *       NumberOfBytesToRead = Limits the range to read
 *       NumberOfBytesRead = The actual number of bytes read.
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtReadVirtualMemory( 
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG  NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	); 	
//FIXME: Is the parameters correctly named ? ThreadHandle might be a TerminationPort
/*
 * FUNCTION: Debugger can register for thread termination
 * ARGUMENTS: 
 *       ThreadHandle = 
 * RETURNS: Status
 */

NTSTATUS
STDCALL	
NtRegisterThreadTerminatePort(
	HANDLE ThreadHandle
	);
/*
 * FUNCTION: Releases a mutant
 * ARGUMENTS: 
 *       MutantHandle = 
 *       ReleaseCount = 
 * RETURNS: Status
 */
NTSTATUS
STDCALL	
NtReleaseMutant(
	IN HANDLE MutantHandle,
	IN PULONG ReleaseCount OPTIONAL 
	);
/*
 * FUNCTION: Releases a semaphore 
 * ARGUMENTS: 
 *       SemaphoreHandle = 
 *       ReleaseCount =
 *       PreviousCount =  
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtReleaseSemaphore( 
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount
	);
/*
 * FUNCTION: Removes an io completion
 * ARGUMENTS:
 *        CompletionPort (OUT) = Caller supplied storage for the resulting handle
 *        CompletionKey = Requested access to the key
 *        IoStatusBlock =
 *        ObjectAttribute = Initialized attributes for the object
 *        CompletionStatus =
 *        WaitTime =
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtRemoveIoCompletion(
	IN HANDLE CompletionPort,
	OUT PULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PULONG CompletionStatus,
	ULONG WaitTime 
	);
/*
 * FUNCTION: Replaces one registry key with another
 * ARGUMENTS: 
 *       ObjectAttributes = Specifies the attributes of the key
 *       Key =  Handle to the key
 *       ReplacedObjectAttributes = The function returns the old object attributes
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	);
/*
 * FUNCTION: Resets a event to a non signaled state 
 * ARGUMENTS: 
 *       EventHandle = Handle to the event that should be reset
 *       NumberOfWaitingThreads =  The number of threads released.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtResetEvent(
	HANDLE EventHandle,
	PULONG NumberOfWaitingThreads OPTIONAL
	);
 //NtRestoreKey
/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * REMARK:
 *	  A thread is resumed if its suspend count is 0. This procedure maps to
 *        the win32 ResumeThread function. ( documentation about the the suspend count can be found here aswell )
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtResumeThread(
	IN HANDLE ThreadHandle,
	IN PULONG SuspendCount
	);
 //NtSaveKey
/*
 * FUNCTION: Sets the context of a specified thread.
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread
 *        Context =  The processor context.
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetContextThread(
	IN HANDLE ThreadHandle,
	IN PCONTEXT Context
	);
/*
 * FUNCTION: Sets the extended attributes of a file.
 * ARGUMENTS: 
 *        FileHandle = Handle to the file
 *        IoStatusBlock = Storage for a resulting status and information 
 *                        on the current operation.
 *        EaBuffer = Extended Attributes buffer.
 *        EaBufferSize = Size of the extended attributes buffer
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetEaFile(
	IN HANDLE FileHandle,
	IN PIO_STATUS_BLOCK IoStatusBlock,	
	PVOID EaBuffer, 
	ULONG EaBufferSize
	);

/*
 * FUNCTION: Set an event to a signalled state.
 * ARGUMENTS: 
 *        EventHandle = Handle to the event
 *        Count =  The resulting count.
 * REMARK:
 *	  This procedure maps to the win32 SetEvent function. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetEvent(
	HANDLE EventHandle,
	PULONG Count
	);

/*
 * FUNCTION: Sets the information of a file object.
 * ARGUMENTS: 
 *        FileHandle = Handle to the file object
 *	  IoStatusBlock = Caller supplies storage for extended information 
 *                        on the current operation.
 *        FileInformation = Storage for the new file information
 *        Lenght = Size of the storage for the file information.
 *        FileInformationClass = Indicates which file information is set
	 
	  FileNameInformation 			PUNICODE_STRING
	  FileRenameInformation			FILE_RENAME_INFORMATION
	  FileStreamInformation			FILE_STREAM_INFORMATION
 *	  FileCompletionInformation 		IO_COMPLETION_CONTEXT

 * REMARK:
 *	  This procedure maps to the win32 SetEndOfFile, SetFileAttributes, 
 *	  SetNamedPipeHandleState, SetMailslotInfo functions. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetInformationFile(
    	IN HANDLE FileHandle,
    	IN PIO_STATUS_BLOCK IoStatusBlock,
    	IN PVOID FileInformation,
   	IN ULONG Length,
    	IN CINT FileInformationClass
    	);


/*
 * FUNCTION: Sets the information of a registry key.
 * ARGUMENTS: 
 *       KeyHandle = Handle to the registry key
 *       KeyInformationClass =  Index to the a certain set of parameter to set.
			Can be one of the following values:

 *	 KeyWriteTimeInformation  KEY_WRITE_TIME_INFORMATION

	 KeyInformation	= Storage for the new key information
 *       KeyInformationLength = Size of the storage allocated for the key information
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	);
/*
 * FUNCTION: Changes a set of object specific parameters
 * ARGUMENTS: 
 *      ObjectHandle = 
 *	HandleInformationClass = Index to the set of parameters to change. 

	HandleInformation			
	HandleName				
	HandleTypeInformation			
	HandleAllInformation			
	HandleBasicInformation			


 *      HandleInformation = Caller supplies storage for parameters to set.
 *      Length = Size of the storage supplied
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetInformationObject(
	IN HANDLE ObjectHandle,
	IN CINT HandleInformationClass,
	IN PVOID HandleInformationPtr,
	IN ULONG Length 
	);

/*
 * FUNCTION: Changes a set of process specific parameters
 * ARGUMENTS: 
 *      ProcessHandle = Handle to the process
 *	ProcessInformationClass = Index to a information structure. 
 *
 *	ProcessBasicInformation 		PROCESS_BASIC_INFORMATION
 *	ProcessQuotaLimits			QUOTA_LIMITS
 *	ProcessBasePriority			KPRIORITY
 *	ProcessRaisePriority			KPRIORITY
 *	ProcessDebugPort			HANDLE
 *	ProcessExceptionPort			HANDLE	
 *	ProcessAccessToken		 	PROCESS_ACCESS_TOKEN	
 *	ProcessDefaultHardErrorMode		ULONG
 *	ProcessPriorityClass			ULONG
 *	ProcessAffinityMask			KAFFINITY
 *
 *      ProcessInformation = Caller supplies storage for information to set.
 *      ProcessInformationLength = Size of the information structure
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetInformationProcess(
	IN HANDLE ProcessHandle,
	IN CINT ProcessInformationClass,
	IN PVOID ProcessInformation,
	IN ULONG ProcessInformationLength
	);
/*
 * FUNCTION: Changes a set of thread specific parameters
 * ARGUMENTS: 
 *      ThreadHandle = Handle to the thread
 *	ThreadInformationClass = Index to the set of parameters to change. 
 *	Can be one of the following values:
 *
 *	ThreadBasicInformation			THREAD_BASIC_INFORMATION
 *	ThreadPriority				KPRIORITY
 *	ThreadBasePriority			KPRIORITY
 *	ThreadAffinityMask			KAFFINITY
 *	ThreadIdealProcessor			ULONG
 *	ThreadPriorityBoost			ULONG
 *
 *      ThreadInformation = Caller supplies storage for parameters to set.
 *      ThreadInformationLength = Size of the storage supplied
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetInformationThread(
	IN HANDLE ThreadHandle,
	IN CINT ThreadInformationClass,
	IN PVOID ThreadInformation,
	IN ULONG ThreadInformationLength
	);
//FIXME: Are the arguments correct
/*
 * FUNCTION: Sets a io completion
 * ARGUMENTS: 
 *      CompletionPort = 
 *	CompletionKey = 
 *      IoStatusBlock =
 *      NumberOfBytesToTransfer =
 *      NumberOfBytesTransferred =
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetIoCompletion(
	IN HANDLE CompletionPort,
	IN ULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG NumberOfBytesToTransfer, 
	OUT PULONG NumberOfBytesTransferred
	);
//FIXME: Should I have more parameters ?

typedef struct
 {    
 } LDT_ENTR, *PLDT_ENTR;
 
/*
 * FUNCTION: Initializes the Local Descriptor Table
 * ARGUMENTS: 
 *	ProcessHandle = 
 *	LdtEntry =
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetLdtEntries(
	HANDLE ProcessHandle,
	PLDT_ENTR LdtEntry
	);

//FIXME: Should Value be a void pointer or a pointer to a unicode string ?
/*
 * FUNCTION: Sets a system environment variable
 * ARGUMENTS: 
 *      ValueName = Name of the environment variable
 *	Value = Value of the environment variable
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetSystemEnvironmentValue(
	IN PUNICODE_STRING ValueName,
	IN PVOID Value
	);

/*
 * FUNCTION: Sets system parameters
 * ARGUMENTS: 
 *      SystemInformationClass = Index to a particular set of system parameters
 *			Can be one of the following values:
 *
 *	SystemTimeAdjustmentInformation		SYSTEM_TIME_ADJUSTMENT
 *
 *	SystemInformation = Structure containing the parameters.
 *      SystemInformationLength = Size of the structure.
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetSystemInformation(
	IN CINT SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);

/*
 * FUNCTION: Sets the system time
 * ARGUMENTS: 
 *      SystemTime = Old System time
 *	NewSystemTime = New System time
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetSystemTime(
	IN PLARGE_INTEGER SystemTime,
	IN PLARGE_INTEGER NewSystemTime OPTIONAL
	);
/*
 * FUNCTION: Sets the characteristics of a timer
 * ARGUMENTS: 
 *      TimerHandle = 
 *	DueTime = 
 *      CompletionRoutine = 
 *      ArgToCompletionRoutine =
 *      Resume = 
 *      Period = 
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE CompletionRoutine,
	IN PVOID ArgToCompletionRoutine,
	IN BOOL Resume,
	IN ULONG Period
	);
/*
 * FUNCTION: Sets the frequency of the system timer
 * ARGUMENTS: 
 *      RequestedResolution = 
 *	SetOrUnset = 
 *      ActualResolution = 
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetTimerResolution(
	IN ULONG RequestedResolution,
	IN BOOL SetOrUnset,
	OUT PULONG ActualResolution
	);
/*
 * FUNCTION: Sets the value of a registry key
 * ARGUMENTS: 
 *      KeyHandle = Handle to a registry key
 *	ValueName = Name of the value entry to change
 *	TitleIndex = pointer to a structure containing the new volume information
 *      Type = Type of the registry key. Can be one of the values:
 *		REG_BINARY		
 *		REG_DWORD			A 32 bit value
 *		REG_DWORD_LITTLE_ENDIAN		Same as REG_DWORD
 *		REG_DWORD_BIG_ENDIAN		A 32 bit value whose least significant byte is at the highest address
 *		REG_EXPAND_SZ			A zero terminated wide character string with unexpanded environment variables  ( "%PATH%" )
 *		REG_LINK			A zero terminated wide character string referring to a symbolic link.
 *		REG_MULTI_SZ			A series of zero-terminated strings including a additional trailing zero
 *		REG_NONE			Unspecified type
 *		REG_SZ				A wide character string ( zero terminated )
 *		REG_RESOURCE_LIST		??
 *		REG_RESOURCE_REQUIREMENTS_LIST	??
 *		REG_FULL_RESOURCE_DESCRIPTOR	??
 *      Data = Contains the data for the registry key.
 *	DataSize = size of the data.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex OPTIONAL,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	);
/*
 * FUNCTION: Sets the volume information of a file. 
 * ARGUMENTS: 
 *      FileHandle = Handle to the file
 *	VolumeInformationClass = specifies the particular volume information to set
 *	VolumeInformation = pointer to a structure containing the new volume information
 *	Length = size of the structure.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	IN CINT VolumeInformationClass,
	PVOID VolumeInformation,
	ULONG Length
	);
/*
 * FUNCTION: Shuts the system down
 * ARGUMENTS: 
 *        Action: 
 * RETURNS: Status
 */ 
NTSTATUS 
STDCALL 
NtShutdownSystem(
	IN SHUTDOWN_ACTION Action
	);
/*
 * FUNCTION: Signals an event and wait for it to be signaled again.
 * ARGUMENTS: 
 *        EventHandle = Handle to the event that should be signaled
 *        Alertable =  True if the wait is alertable
 *        Time = The time to wait
 *        NumberOfWaitingThreads = Number of waiting threads
 * RETURNS: Status
 */ 

NTSTATUS 
STDCALL 
NtSignalAndWaitForSingleObject(
        IN HANDLE EventHandle,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time,
	PULONG NumberOfWaitingThreads OPTIONAL 
	);
/*
 * FUNCTION: Increments a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        SuspendCount =  The resulting suspend count.
 * REMARK:
 *	  A thread will be suspended if its suspend count is greater than 0. This procedure maps to
 *        the win32 SuspendThread function. ( documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */ 
NTSTATUS 
STDCALL 
NtSuspendThread(
	IN HANDLE ThreadHandle,
	IN PULONG PreviousSuspendCount 
	);

//--NtSystemDebugControl
/*
 * FUNCTION: Terminates the execution of a process. 
 * ARGUMENTS: 
 *      ThreadHandle = Handle to the process
 *      ExitStatus  = The exit status of the process to terminate with.
 * REMARKS
	Native applications should kill themselves using this function.
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL 
NtTerminateProcess(
	IN HANDLE ProcessHandle ,
	IN NTSTATUS ExitStatus
	);
/*
 * FUNCTION: Terminates the execution of a thread. 
 * ARGUMENTS: 
 *      ThreadHandle = Handle to the thread
 *      ExitStatus  = The exit status of the thread to terminate with.
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL 
NtTerminateThread(
	IN HANDLE ThreadHandle ,
	IN NTSTATUS ExitStatus
	);
/*
 * FUNCTION: Test to see if there are any pending alerts for the calling thread 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL 
NtTestAlert(
	VOID 
	);
/*
 * FUNCTION: Unloads a driver. 
 * ARGUMENTS: 
 *      DriverServiceName = Name of the driver to unload
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL
NtUnloadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

//FIXME: NtUnloadKey needs more arguments
/*
 * FUNCTION: Unload a registry key. 
 * ARGUMENTS: 
 *       KeyHandle = Handle to the registry key
 * REMARK:
	This procedure maps to the win32 procedure RegUnloadKey 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL
NtUnloadKey(
	HANDLE KeyHandle
	);

/*
 * FUNCTION: Unlocks a range of bytes in a file. 
 * ARGUMENTS: 
 *       FileHandle = Handle to the file
 *       IoStatusBlock = Caller should supply storage for a structure containing
 *			 the completion status and information about the requested unlock operation.
 *       StartAddress = StartAddress of the range of bytes to unlock 
 *       NumberOfBytesToUnlock = Number of bytes to unlock.
 *       NumberOfBytesUnlocked (OUT) = Number of bytes actually unlocked.
 * REMARK:
	This procedure maps to the win32 procedure UnlockFileEx 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL
NtUnlockFile(
	IN HANDLE FileHandle,
	OUT IO_STATUS_BLOCK IoStatusBlock,
	IN LARGE_INTEGER StartAddress,
	IN LARGE_INTEGER NumberOfBytesToUnlock,
	OUT PLARGE_INTEGER NumberOfBytesUnlocked OPTIONAL
	);
	
/*
 * FUNCTION: Unlocks a range of virtual memory. 
 * ARGUMENTS: 
 *       ProcessHandle = Handle to the process
 *       BaseAddress =   Lower boundary of the range of bytes to unlock. 
 *       NumberOfBytesToUnlock = Offset to the upper boundary to unlock.
 *       NumberOfBytesUnlocked (OUT) = Number of bytes actually unlocked.
 * REMARK:
	This procedure maps to the win32 procedure VirtualUnlock 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL
NtUnlockVirtualMemory(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	ULONG  NumberOfBytesToUnlock,
	PULONG NumberOfBytesUnlocked OPTIONAL
	);
/*
 * FUNCTION: Unmaps a piece of virtual memory backed by a file. 
 * ARGUMENTS: 
 *       ProcessHandle = Handle to the process
 *       BaseAddress =  The address where the mapping begins
 * REMARK:
	This procedure maps to the win32 UnMapViewOfFile
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtUnmapViewOfSection(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress
	);
/*
 * FUNCTION: Waits for multiple objects to become signalled. 
 * ARGUMENTS: 
 *       Count = The number of objects
 *       Object = The array of object handles
 *       WaitType = 
 *       Alertable = If true the wait is alertable.
 *       Time = The maximum wait time. 
 * REMARKS:
 * 	This function maps to the win32 WaitForMultipleObjectEx.
 * RETURNS: Status    
 */
NTSTATUS
STDCALL
NtWaitForMultipleObjects (
	IN ULONG Count,
	IN PHANDLE Object[],
	IN CINT WaitType,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time 
	);
/*
 * FUNCTION: Waits for an object to become signalled. 
 * ARGUMENTS: 
 *       Object = The object handle
 *       Alertable = If true the wait is alertable.
 *       Time = The maximum wait time.
 * REMARKS:
 * 	This function maps to the win32 WaitForSingleObjectEx. 
 * RETURNS: Status    
 */
NTSTATUS
STDCALL
NtWaitForSingleObject (
	IN PHANDLE Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time 
	);
/*
 * FUNCTION: Writes data to a file
 * ARGUMENTS: 
 *       FileHandle = The handle a file ( from NtCreateFile )
 *       Event  = 
 *       ApcRoutine = Asynchroneous Procedure Callback [ Should not be used by device drivers ]
 *       ApcContext = Argument to the Apc Routine 
 *       IoStatusBlock = Caller should supply storage for a structure containing the completion status and information about the requested write operation.
 *       Buffer = Caller should supply storage for a buffer that will contain the information to be written to file.
 *       Length = Size in bytest of the buffer
 *       ByteOffset = Points to a file offset. If a combination of Length and BytesOfSet is past the end-of-file mark the file will be enlarged.
 *		      BytesOffset is ignored if the file is created with FILE_APPEND_DATA in the DesiredAccess. BytesOffset is also ignored if
 *                    the file is created with CreateOptions flags FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT set, in that case a offset
 *                    should be created by specifying FILE_USE_FILE_POINTER_POSITION.
 *       Key =  Unused
 * REMARKS:
 *	 This function maps to the win32 WriteFile. 
 *	 Callers to NtWriteFile should run at IRQL PASSIVE_LEVEL.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtWriteFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset ,
	IN PULONG Key OPTIONAL
    );

/*
 * FUNCTION: Writes a file 
 * ARGUMENTS: 
 *       FileHandle = The handle of the file 
 *       Event  = 
 *       ApcRoutine = Asynchroneous Procedure Callback [ Should not be used by device drivers ]
 *       ApcContext = Argument to the Apc Routine 
 *       IoStatusBlock = Caller should supply storage for a structure containing the completion status and information about the requested write operation.
 *       BufferDescription = Caller should supply storage for a buffer that will contain the information to be written to file.
 *       BufferLength = Size in bytest of the buffer
 *       ByteOffset = Points to a file offset. If a combination of Length and BytesOfSet is past the end-of-file mark the file will be enlarged.
 *		      BytesOffset is ignored if the file is created with FILE_APPEND_DATA in the DesiredAccess. BytesOffset is also ignored if
 *                    the file is created with CreateOptions flags FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT set, in that case a offset
 *                    should be created by specifying FILE_USE_FILE_POINTER_POSITION.
 *       Key =  Unused
 * REMARKS:
 *	 This function maps to the win32 WriteFile. 
 *	 Callers to NtWriteFile should run at IRQL PASSIVE_LEVEL.
 * RETURNS: Status
 */

NTSTATUS
STDCALL NtWriteFileScatter( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN LARGE_INTEGER BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL
	); 

/*
 * FUNCTION: Writes a range of virtual memory
 * ARGUMENTS: 
 *       ProcessHandle = The handle to the process owning the address space.
 *       BaseAddress  = The points to the address to  write to
 *       Buffer = Pointer to the buffer to write
 *       NumberOfBytesToWrite = Offset to the upper boundary to write
 *       NumberOfBytesWritten = Total bytes written
 * REMARKS:
 *	 This function maps to the win32 WriteProcessMemory
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtWriteVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID  BaseAddress,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);
/*
 * FUNCTION: Yields the callers thread.
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtYieldExecution(
	VOID
	);



#endif /* __DDK_ZW_H */
