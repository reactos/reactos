/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          System call definitions
 * FILE:             include/ddk/zw.h
 * REVISION HISTORY: 
 *              ??/??/??: First few functions (David Welch)
 *              ??/??/??: Complete implementation by Boudewijn Dekker
 *              13/07/98: Reorganised things a bit (David Welch)
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

#ifndef _NTNATIVE
#define _NTNATIVE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <kernel32/heap.h>
   
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

#define EVENT_READ_ACCESS	0x0001
#define EVENT_WRITE_ACCESS	0x0002


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

#define KeyWriteTimeInformation		0

// key value information class

#define KeyValueBasicInformation		0
#define	KeyValueFullInformation		1
#define	KeyValuePartialInformation		2

// object handle information

#define	HandleBasicInformation			4

// system information

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

// wait type

#define WaitAll					0
#define WaitAny					1
 
 
// key restore flags

#define REG_WHOLE_HIVE_VOLATILE     (0x00000001L)   
#define REG_REFRESH_HIVE            (0x00000002L)   


#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')

// object type  access rights

#define OBJECT_TYPE_CREATE (0x0001)
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)


// directory access rights

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

// symbolic link access rights

#define SYMBOLIC_LINK_QUERY (0x0001)
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)
   
typedef struct _PROCESS_WS_WATCH_INFORMATION {
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _QUOTA_LIMITS {
    ULONG PagedPoolLimit;
    ULONG NonPagedPoolLimit;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    ULONG PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef struct _IO_COUNTERS {
    ULONG ReadOperationCount;
    ULONG WriteOperationCount;
    ULONG OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;


typedef struct _VM_COUNTERS {
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


typedef struct _POOLED_USAGE_AND_LIMITS {
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


typedef struct _PROCESS_ACCESS_TOKEN {
    HANDLE Token;
    HANDLE Thread;

} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

// exception structures


#define TEB_EXCEPTION_FRAME(pcontext) ((PEXCEPTION_FRAME)((TEB *)GET_SEL_BASE((pcontext)->SegFs))->except)

   
typedef struct _OBJECT_NAME_INFORMATION {               
    UNICODE_STRING Name;                                
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;   


// section information 


// handle information 

//#define HANDLE_FLAG_INHERIT             0x00000001 
//#define HANDLE_FLAG_PROTECT_FROM_CLOSE  0x00000002 


typedef struct _HANDLE_INFO 
{
	BOOL bInheritHanlde;
	BOOL bProtectFromClose;
} HANDLE_INFO;
typedef HANDLE_INFO *PHANDLE_INFO;



typedef struct _SYSTEM_TIME_ADJUSTMENT
{
	DWORD dwTimeAdjustment;	
	BOOL bTimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;
	

// file information

// asynchorneous procedure call 


typedef struct _FILE_BASIC_INFORMATION {                    
    LARGE_INTEGER CreationTime;                             
    LARGE_INTEGER LastAccessTime;                           
    LARGE_INTEGER LastWriteTime;                            
    LARGE_INTEGER ChangeTime;                               
    ULONG FileAttributes;                                   
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;         
                                                            
typedef struct _FILE_STANDARD_INFORMATION {                 
    LARGE_INTEGER AllocationSize;                           
    LARGE_INTEGER EndOfFile;                                
    ULONG NumberOfLinks;                                    
    BOOLEAN DeletePending;                                  
    BOOLEAN Directory;                                      
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;   
                                                            
typedef struct _FILE_POSITION_INFORMATION {                 
    LARGE_INTEGER CurrentByteOffset;                        
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;   
                                                            
typedef struct _FILE_ALIGNMENT_INFORMATION {                
    ULONG AlignmentRequirement;                             
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION; 
                                                            
typedef struct _FILE_DISPOSITION_INFORMATION {                  
    BOOLEAN DeleteFile;                                         
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; 
                                                                
typedef struct _FILE_END_OF_FILE_INFORMATION {                  
    LARGE_INTEGER EndOfFile;                                    
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION; 
                                                                

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION;
typedef FILE_FULL_EA_INFORMATION *PFILE_FULL_EA_INFORMATION; 



// file system information structures

typedef struct _FILE_FS_DEVICE_INFORMATION {                    
    DEVICE_TYPE DeviceType;                                     
    ULONG Characteristics;                                      
} FILE_FS_DEVICE_INFORMATION;
typedef FILE_FS_DEVICE_INFORMATION *PFILE_FS_DEVICE_INFORMATION;   


// timer apc routine [ possible incompatible with ms winnt ]

typedef
VOID
(*PTIMERAPCROUTINE) (
    PVOID Argument,
    PVOID Context
    );

// shutdown action [ possible incompatible with ms winnt ]
// this might be parameter to specify how to shutdown
typedef
VOID
(*SHUTDOWN_ACTION) (
    VOID
    );

 //NtAcceptConnectPort
 //NtAccessCheck
 //NtAccessCheckAndAuditAlarm

NTSTATUS
STDCALL
NtAddAtom(
	IN PUNICODE_STRING pString
	);
 
 //NtAdjustGroupsToken
 //NtAdjustPrivilegesToken
 //NtAlertResumeThread
 //NtAlertThread
 //NtAllocateLocallyUniqueId
 //NtAllocateUuids


NTSTATUS
STDCALL
NtAllocateVirtualMemory( 
	IN HANDLE hProcess,
	OUT LPVOID  lpAddress,
	IN ULONG uWillThingAbThis,
	IN DWORD dwSize,
	IN DWORD  flAllocationType, 
	IN DWORD  flProtect
	);

// NtCallbackReturn
// NtCancelIoFile

NTSTATUS
STDCALL
NtCancelTimer(
	IN HANDLE TimerHandle,
	IN BOOL Resume
	);

NTSTATUS
STDCALL
NtClearEvent( 
	IN HANDLE  EventHandle 
	);


NTSTATUS
STDCALL
NtClose(
    	IN HANDLE Handle
    	);

// NtCloseObjectAuditAlarm
// NtCompleteConnectPort
// NtConnectPort

NTSTATUS
STDCALL
NtContinue(
	IN PCONTEXT Context
	);

//NtCreateChannel


NTSTATUS
STDCALL
NtCreateDirectoryObject(
    	OUT PHANDLE DirectoryHandle,
    	IN ACCESS_MASK DesiredAccess,
    	IN POBJECT_ATTRIBUTES ObjectAttributes
    	);

NTSTATUS
STDCALL
NtCreateEvent(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOL ManualReset,
	IN BOOL InitialState
	);
//NtCreateEventPair

                                        
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

// NtCreateIoCompletion

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
// NtCreateMailslotFile
//-- NtCreateMutant
//-- NtCreateNamedPipeFile
//-- NtCreatePagingFile
//-- NtCreatePort
 
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
// NtCreateProfile

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

NTSTATUS
STDCALL
NtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
	);
// NtCreateSymbolicLinkObject

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

NTSTATUS
STDCALL 
NtCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN LONG ManualReset 
	);
//-- NtCreateToken

NT_TEB
STDCALL 
NtCurrentTeb(
	VOID
	);

NTSTATUS
STDCALL
NtDelayExecution(
	IN BOOL Alertable,
	IN PLARGE_INTEGER Interval
	);


NTSTATUS
STDCALL
NtDeleteAtom(
	IN ATOM Atom
	);


NTSTATUS
STDCALL
NtDeleteFile(
	IN HANDLE FileHandle
	);


NTSTATUS
STDCALL
NtDeleteKey(
	IN HANDLE KeyHandle
	);
// NtDeleteObjectAuditAlarm
// NtDeleteValueKey
// NtDeviceIoControlFile

NTSTATUS
STDCALL
NtDisplayString(
	IN PUNICODE_STRING DisplayString
	);

NTSTATUS
STDCALL
NtDuplicateObject(
	IN HANDLE SourceProcessHandle,
	IN PHANDLE SourceHandle,
	IN HANDLE TargetProcessHandle,
	OUT PHANDLE TargetHandle,
	IN ULONG dwDesiredAccess,
	IN ULONG InheritHandle
	);
// NtDuplicateToken

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
 //NtExtendSection

ATOM
STDCALL
NtFindAtom(
	IN PUNICODE_STRING AtomString
	);

NTSTATUS
STDCALL
NtFlushBuffersFile(
	IN HANDLE FileHandle
	);

NTSTATUS
STDCALL
NtFlushInstructionCache(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN UINT Size
	);

NTSTATUS
STDCALL
NtFlushKey(
	IN HANDLE KeyHandle
          );

/*
 * FIXME: Is the return type correct? (David Welch)
 */
NTSTATUS
STDCALL
NtFlushVirtualMemory(
	IN HANDLE ProcessHandle,
	IN VOID *BaseAddress,
	IN ULONG NumberOfBytesToFlush,
	IN PULONG NumberOfBytesFlushed
	);

VOID
STDCALL                                            
NtFlushWriteBuffer (                            
	VOID                                        
	);              

NTSTATUS
STDCALL
NtFreeVirtualMemory(
	IN PHANDLE hProcess,
	IN LPVOID  lpAddress,	// address of region of committed pages  
	IN DWORD  dwSize,	// size of region 
	IN DWORD  dwFreeType
	); 
//-- NtFsControlFile

NTSTATUS
STDCALL 
NtGetContextThread(
	IN HANDLE ThreadHandle, 
	OUT PCONTEXT Context
	);
// NtGetPlugPlayEvent
// NtGetTickCount
// NtImpersonateClientOfPort
// NtImpersonateThread
// NtInitializeRegistry
// NtListenChannel
// NtListenPort

NTSTATUS
STDCALL 
NtLoadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

 //NtLoadKey2
 //NtLoadKey
 //NtLockFile
 //NtLockVirtualMemory

NTSTATUS
STDCALL
NtMakeTemporaryObject(
	OUT HANDLE Handle
	);

NTSTATUS
STDCALL
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
     IN ULONG Protect
    );

// NtNotifyChangeDirectoryFile
// NtNotifyChangeKey
// NtOpenChannel
 
NTSTATUS
STDCALL
NtOpenDirectoryObject(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
 
NTSTATUS
STDCALL
NtOpenEvent(	
	OUT PHANDLE EventHandle,
        IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
// NtOpenEventPair
 
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
// NtOpenIoCompletion

NTSTATUS
STDCALL
NtOpenKey(
	OUT PHANDLE KeyHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
// NtOpenMutant
// NtOpenObjectAuditAlarm
NTSTATUS NtOpenProcess (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	); 

// NtOpenProcessToken

NTSTATUS
STDCALL
NtOpenSection(
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
// NtOpenSemaphore
NTSTATUS
STDCALL
NtOpenSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
NtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
//NtOpenThreadToken

NTSTATUS
STDCALL
NtOpenTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
    	);
// NtPlugPlayControl
// NtPrivilegeCheck
// NtPrivilegeObjectAuditAlarm
// NtPrivilegedServiceAuditAlarm
// NtProtectVirtualMemory
 
NTSTATUS 
STDCALL 
NtPulseEvent(
	IN HANDLE EventHandle,
	IN BOOL Unknown OPTIONAL
	);
// NtQueryAttributesFile
// NtQueryDefaultLocale
// NtQueryDirectoryFile
 
NTSTATUS 
STDCALL 
NtQueryDirectoryObject(
	IN HANDLE DirectoryHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER AllocationSize OPTIONAL,  
	IN ULONG FileAttributes,                    
	IN ULONG ShareAccess,                       
	IN PVOID Buffer ,                 
	IN ULONG Length
	);

NTSTATUS
STDCALL
NtQueryEaFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID EaBuffer ,
	IN ULONG EaLength,
	ULONG Unknown1 ,
	ULONG Unknown2 ,
	ULONG Unknown3 ,
	ULONG Unknown4 ,
	IN CINT FileInformationClass
	);
//-- NtQueryEvent
//-- NtQueryFullAttributesFile
//-- NtQueryInformationAtom

NTSTATUS
STDCALL
NtQueryInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN CINT FileInformationClass
    );
//NtQueryInformationPort

/*
ProcessWorkingSetWatch PROCESS_WS_WATCH_INFORMATION 
ProcessBasicInfo PROCESS_BASIC_INFORMATION
ProcessQuotaLimits QUOTA_LIMITS
ProcessPooledQuotaLimits QUOTA_LIMITS
ProcessIoCounters IO_COUNTERS
ProcessVmCounters VM_COUNTERS
ProcessPooledUsageAndLimits POOLED_USAGE_AND_LIMITS
ProcessTimes KERNEL_USER_TIMES
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
ThreadTimes KERNEL_USER_TIMES
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
// NtQueryIntervalProfile
// NtQueryIoCompletion

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

NTSTATUS
STDCALL
NtQueryObject(
	IN HANDLE ObjectHandle,
	IN CINT HandleInformationClass,
	OUT PHANDLE_INFO HandleInfo,
	IN ULONG Length,
	OUT PULONG ResultLength);
 //NtQueryOleDirectoryFile

NTSTATUS
STDCALL
NtQueryPerformanceCounter(
	IN PULONG Count,
	IN PULONG Frequency
	);

// NtQuerySection
//-- NtQuerySecurityObject
//-- NtQuerySemaphore

NTSTATUS
STDCALL
NtQuerySymbolicLinkObject(
    	IN HANDLE SymbolicLinkHandle,     
    	OUT PUNICODE_STRING TargetName,    /* target device name */
    	IN PULONG Length
	);   
//-- NtQuerySystemEnvironmentValue
NTSTATUS
STDCALL
NtQuerySystemInformation(
	IN CINT SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);

NTSTATUS
STDCALL
NtQuerySystemTime (
	OUT PLARGE_INTEGER CurrentTime
	);
//-- NtQueryTimer
//-- NtQueryTimerResolution

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
//-- NtQueryVirtualMemory

NTSTATUS
STDCALL
NtQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID VolumeInformation,
	IN ULONG Length,
	IN CINT FSInformationClass // dont know
    );
// NtQueueApcThread

NTSTATUS
STDCALL
NtRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT CONTEXT,
	IN BOOL bUnknown OPTIONAL
	);

NTSTATUS
STDCALL
NtRaiseHardError(
	IN OUT ULONG Unknown1,
	IN OUT ULONG Unknown2,
	IN OUT PVOID Unknow3,
	IN OUT ULONG Unknow4,
	IN PEXCEPTION_RECORD ExceptionRecord
	);


NTSTATUS
STDCALL
NtReadFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset ,
	IN PULONG Key OPTIONAL	
	);
NTSTATUS 
NtReadFileScatter( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN  PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK UserIosb , 
	IN LARGE_INTEGER BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL	
	); 

//NtReadRequestData

NTSTATUS
STDCALL
NtReadVirtualMemory( 
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN DWORD nSize,
	OUT PDWORD lpNumberOfBytesRead
	); 	
 //--NtRegisterThreadTerminatePort
 //--NtReleaseMutant

NTSTATUS
STDCALL
NtReleaseSemaphore( 
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount
	);
 //NtRemoveIoCompletion
 //NtReplaceKey
 //NtReplyPort
 //NtReplyWaitReceivePort
 //NtReplyWaitReplyPort
 //NtReplyWaitSendChannel
 //NtRequestPort
 //NtRequestWaitReplyPort
 //--NtResetEvent
 //NtRestoreKey

NTSTATUS
STDCALL
NtResumeThread(
	IN HANDLE ThreadHandle,
	IN PCONTEXT Context
	);
 //NtSaveKey
 //NtSendWaitReplyChannel
 //NtSetContextChannel

NTSTATUS
STDCALL
NtSetContextThread(
	IN HANDLE ThreadHandle,
	IN PCONTEXT Context
	);
 //--NtSetDefaultHardErrorPort
 //--NtSetDefaultLocale
 //--NtSetEaFile
 //--NtSetEvent
 //--NtSetHighEventPair
 //--NtSetHighWaitLowEventPair
 //--NtSetHighWaitLowThread


NTSTATUS
STDCALL
NtSetInformationFile(
    	IN HANDLE FileHandle,
    	IN PIO_STATUS_BLOCK IoStatusBlock,
    	IN PVOID FileInformation,
   	IN ULONG Length,
    	IN CINT FileInformationClass
    	);

//KeyWriteTimeInformation  KEY_WRITE_TIME_INFORMATION

NTSTATUS
STDCALL
NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeySetInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	);
NTSTATUS
STDCALL
NtSetInformationObject(
	IN HANDLE ObjectHandle,
	IN CINT HandleInformationClass,
	IN PVOID HandleInfo,
	IN ULONG Length 
	);
/*

ProcessQuotaLimits QUOTA_LIMITS
ProcessAccessToken PROCESS_ACCESS_TOKEN

*/

NTSTATUS
STDCALL
NtSetInformationProcess(
	IN HANDLE ProcessHandle,
	IN CINT ProcessInformationClass,
	IN PVOID ProcessInformation,
	IN ULONG ProcessInformationLength
	);

NTSTATUS
STDCALL
NtSetInformationThread(
	IN HANDLE ThreadHandle,
	IN CINT ThreadInformationClass,
	IN PVOID ThreadInformation,
	IN ULONG ThreadInformationLength
	);
// NtSetInformationToken
// NtSetIntervalProfile
// NtSetIoCompletion
// NtSetLdtEntries
// NtSetLowEventPair
// NtSetLowWaitHighEventPair
// NtSetLowWaitHighThread
// NtSetSecurityObject
 //NtSetSystemEnvironmentValue
/*
SystemTimeAdjustmentInformation SYSTEM_TIME_ADJUSTMENT
*/
NTSTATUS
STDCALL
NtSetSystemInformation(
	IN CINT SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);
 //NtSetSystemPowerState

NTSTATUS
STDCALL
NtSetSystemTime(
	IN PLARGE_INTEGER SystemTime,
	IN BOOL Unknown OPTIONAL
	);

NTSTATUS
STDCALL
NtSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE CompletionRoutine,
	IN LPVOID ArgToCompletionRoutine,
	IN BOOL Resume,
	IN LONG Period
	);
// NtSetTimerResolution

NTSTATUS
STDCALL
NtSetValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex ,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	);

//-- NtSetVolumeInformationFile

NTSTATUS 
STDCALL 
NtShutdownSystem(
	IN SHUTDOWN_ACTION Action
	);
//-- NtSignalAndWaitForSingleObject
// NtStartProfile
// NtStopProfile
 
NTSTATUS 
STDCALL 
NtSuspendThread(
	IN HANDLE ThreadHandle,
	IN PULONG PreviousSuspendCount 
	);
 //--NtSystemDebugControl
 
NTSTATUS 
STDCALL 
NtTerminateProcess(
	IN HANDLE ProcessHandle ,
	IN NTSTATUS ExitStatus
	);

NTSTATUS 
STDCALL 
NtTerminateThread(
	IN HANDLE ThreadHandle ,
	IN NTSTATUS ExitStatus
	);

 //--NtTestAlert

NTSTATUS 
STDCALL
NtUnloadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

 //--NtUnloadKey
 //--NtUnlockFile
 //--NtUnlockVirtualMemory

NTSTATUS
STDCALL
NtUnmapViewOfSection(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress
	);
 //NtVdmControl
 //NtW32Call

NTSTATUS
STDCALL
NtWaitForMultipleObjects (
	IN ULONG Count,
	IN PVOID Object[],
	IN CINT WaitType,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time 
	);

NTSTATUS
STDCALL
NtWaitForSingleObject (
	IN PVOID Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time 
	);
 //--NtWaitHighEventPair
 //--NtWaitLowEventPair

NTSTATUS
STDCALL
NtWriteFile(
	IN HANDLE FileHandle,
	IN HANDLE Event ,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset ,
	IN PULONG Key OPTIONAL
    );

NTSTATUS
STDCALL NtWriteFileScatter( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK UserIosb,
	IN LARGE_INTEGER BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL
	); 

 //NtWriteRequestData

NTSTATUS
STDCALL 
NtWriteVirtualMemory(
	IN HANDLE ProcessHandle,
	IN VOID *Buffer,
	IN ULONG Size,
	OUT PULONG NumberOfBytesWritten
	);

NTSTATUS
STDCALL 
NtYieldExecution(
	VOID
	);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

#endif /* __DDK_ZW_H */
