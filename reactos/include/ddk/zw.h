
/* $Id: zw.h,v 1.50 2002/03/18 16:14:45 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          System call definitions
 * FILE:             include/ddk/zw.h
 * REVISION HISTORY: 
 *              ??/??/??: First few functions (David Welch)
 *              ??/??/??: Complete implementation by Ariadne
 *              13/07/98: Reorganised things a bit (David Welch)
 *              04/08/98: Added some documentation (Ariadne)
 *		14/08/98: Added type TIME and change variable type from [1] to [0]
 *              14/09/98: Added for each Nt call a corresponding Zw Call
 */

#ifndef __DDK_ZW_H
#define __DDK_ZW_H

#include <ntos/security.h>
#include <napi/npipe.h>

//#define LCID ULONG
//#define SECURITY_INFORMATION ULONG
//typedef ULONG SECURITY_INFORMATION;


/*
 * FUNCTION: Checks a clients access rights to a object
 * ARGUMENTS: 
 *	  SecurityDescriptor = Security information against which the access is checked
 *	  ClientToken = Represents a client
 *	  DesiredAcces = 
 *	  GenericMapping =
 *	  PrivilegeSet =
 *	  ReturnLength = Bytes written
 *	  GrantedAccess = 
 *	  AccessStatus = Indicates if the ClientToken allows the requested access
 * REMARKS: The arguments map to the win32 AccessCheck 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PPRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus
	);

NTSTATUS
STDCALL
ZwAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PPRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus
	);

/*
 * FUNCTION: Checks a clients access rights to a object and issues a audit a alarm. ( it logs the access )
 * ARGUMENTS: 
 *	  SubsystemName = Specifies the name of the subsystem, can be "WIN32" or "DEBUG"
 *	  ObjectHandle =
 *	  ObjectAttributes =
 *	  DesiredAcces = 
 *	  GenericMapping =
 *	  ObjectCreation = 
 *	  GrantedAccess = 
 *	  AccessStatus =
 *	  GenerateOnClose =
 * REMARKS: The arguments map to the win32 AccessCheck 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtAccessCheckAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PHANDLE ObjectHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ACCESS_MASK DesiredAccess,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	);

NTSTATUS
STDCALL
ZwAccessCheckAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PHANDLE ObjectHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ACCESS_MASK DesiredAccess,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	);

/*
 * FUNCTION: Adds an atom to the global atom table
 * ARGUMENTS:
 *        AtomString = The string to add to the atom table.
 *        Atom (OUT) = Caller supplies storage for the resulting atom.
 * REMARKS: The arguments map to the win32 add GlobalAddAtom.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAddAtom(
	IN	PWSTR		AtomName,
	IN OUT	PRTL_ATOM	Atom
	);


NTSTATUS
STDCALL
ZwAddAtom(
	IN	PWSTR		AtomName,
	IN OUT	PRTL_ATOM	Atom
	);


/*
 * FUNCTION: Adjusts the groups in an access token
 * ARGUMENTS: 
 *	  TokenHandle = Specifies the access token
 *	  ResetToDefault = If true the NewState parameter is ignored and the groups are set to
 *			their default state, if false the groups specified in
 *			NewState are set.
 *	  NewState = 
 *	  BufferLength = Specifies the size of the buffer for the PreviousState.
 *	  PreviousState = 
 *	  ReturnLength = Bytes written in PreviousState buffer.
 * REMARKS: The arguments map to the win32 AdjustTokenGroups
 * RETURNS: Status
 */

NTSTATUS 
STDCALL 
NtAdjustGroupsToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN  ResetToDefault,
	IN PTOKEN_GROUPS  NewState,
	IN ULONG  BufferLength,
	OUT PTOKEN_GROUPS  PreviousState OPTIONAL,
	OUT PULONG  ReturnLength
	);

NTSTATUS
STDCALL
ZwAdjustGroupsToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN  ResetToDefault,
	IN PTOKEN_GROUPS  NewState,
	IN ULONG  BufferLength,
	OUT PTOKEN_GROUPS  PreviousState,
	OUT PULONG  ReturnLength
	);


/*
 * FUNCTION:
 *
 * ARGUMENTS:
 *        TokenHandle = Handle to the access token
 *        DisableAllPrivileges =  The resulting suspend count.
	  NewState = 
	  BufferLength =
	  PreviousState =
	  ReturnLength =
 * REMARK:
 *	  The arguments map to the win32 AdjustTokenPrivileges
 * RETURNS: Status
 */

NTSTATUS 
STDCALL 
NtAdjustPrivilegesToken(
	IN HANDLE  TokenHandle,
	IN BOOLEAN  DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES  NewState,
	IN ULONG  BufferLength,
	OUT PTOKEN_PRIVILEGES  PreviousState,
	OUT PULONG ReturnLength
	);

NTSTATUS 
STDCALL 
ZwAdjustPrivilegesToken(
	IN HANDLE  TokenHandle,
	IN BOOLEAN  DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES  NewState,
	IN ULONG  BufferLength,
	OUT PTOKEN_PRIVILEGES  PreviousState,
	OUT PULONG ReturnLength
	);


/*
 * FUNCTION: Decrements a thread's suspend count and places it in an alerted 
 *           state.
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        SuspendCount =  The resulting suspend count.
 * REMARK:
 *	  A thread is resumed if its suspend count is 0
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtAlertResumeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG SuspendCount
	);

NTSTATUS
STDCALL
ZwAlertResumeThread(
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

NTSTATUS
STDCALL
ZwAlertThread(
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
	OUT LUID *LocallyUniqueId
	);

NTSTATUS
STDCALL
ZwAllocateLocallyUniqueId(
	OUT PLUID Luid
	);

NTSTATUS
STDCALL
NtAllocateUuids(
	PULARGE_INTEGER Time,
	PULONG Range,
	PULONG Sequence
	);

NTSTATUS
STDCALL
ZwAllocateUuids(
	PULARGE_INTEGER Time,
	PULONG Range,
	PULONG Sequence
	);


/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you supply a non zero
 *			value the system will try to allocate the memory at the address supplied. It rounds
 *			it down to a multiple if the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits that must be zero, ensuring that 
 *			the memory will be allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to allocated,
 *                       can be one of the values : MEM_COMMIT, MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN
 *      Protect = Indicates the protection type of the pages allocated, can be a combination of
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
NtAllocateVirtualMemory (
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG  ZeroBits,
	IN OUT PULONG  RegionSize,
	IN ULONG  AllocationType,
	IN ULONG  Protect
	);

NTSTATUS
STDCALL
ZwAllocateVirtualMemory (
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG  ZeroBits,
	IN OUT PULONG  RegionSize,
	IN ULONG  AllocationType,
	IN ULONG  Protect);

/*
 * FUNCTION: Returns from a callback into user mode
 * ARGUMENTS:
 * RETURN Status
 */
//FIXME: this function might need 3 parameters
NTSTATUS STDCALL NtCallbackReturn(PVOID Result,
				  ULONG ResultLength,
				  NTSTATUS Status);

NTSTATUS STDCALL ZwCallbackReturn(PVOID Result,
				  ULONG ResultLength,
				  NTSTATUS Status);

/*
 * FUNCTION: Cancels a IO request
 * ARGUMENTS: 
 *        FileHandle = Handle to the file
 *        IoStatusBlock  = 
 *
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

NTSTATUS
STDCALL
ZwCancelIoFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	);
/*
 * FUNCTION: Cancels a timer
 * ARGUMENTS: 
 *        TimerHandle = Handle to the timer
 *        CurrentState = Specifies the state of the timer when cancelled.
 * REMARKS:
 *        The arguments to this function map to the function CancelWaitableTimer. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCancelTimer(
	IN HANDLE TimerHandle,
	OUT PBOOLEAN CurrentState OPTIONAL
	);

NTSTATUS
STDCALL
ZwCancelTimer(
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

NTSTATUS
STDCALL
ZwClearEvent(
	IN HANDLE  EventHandle
	);

/*
 * FUNCTION: Closes an object handle
 * ARGUMENTS:
 *         Handle = Handle to the object
 * REMARKS:
 *       This function maps to the win32 function CloseHandle. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtClose(
	IN HANDLE Handle
	);

NTSTATUS
STDCALL
ZwClose(
	IN HANDLE Handle
	);

/*
 * FUNCTION: Generates an audit message when a handle to an object is dereferenced
 * ARGUMENTS:
 *         SubsystemName   = 
	   HandleId	   = Handle to the object
	   GenerateOnClose =
 * REMARKS:
 *       This function maps to the win32 function ObjectCloseAuditAlarm. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtCloseObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN BOOLEAN GenerateOnClose
	);

NTSTATUS
STDCALL
ZwCloseObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN BOOLEAN GenerateOnClose
	);

/*
 * FUNCTION: Continues a thread with the specified context
 * ARGUMENTS: 
 *        Context = Specifies the processor context
 *	  IrqLevel = Specifies the Interupt Request Level to continue with. Can
 *			be PASSIVE_LEVEL or APC_LEVEL
 * REMARKS
 *        NtContinue can be used to continue after an exception or apc.
 * RETURNS: Status
 */
//FIXME This function might need another parameter

NTSTATUS
STDCALL
NtContinue(
	IN PCONTEXT Context,
	IN BOOLEAN TestAlert
	);

NTSTATUS STDCALL ZwContinue(IN PCONTEXT Context, IN CINT IrqLevel);


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

NTSTATUS
STDCALL
ZwCreateDirectoryObject(
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
 *                       using NtResetEvent/NtClearEvent. if false the system will reset the event to a non-signalled state
 *                       automatically after the system has rescheduled a thread waiting on the event.
 *        InitialState = specifies the initial state of the event to be signaled ( TRUE ) or non-signalled (FALSE).
 * REMARKS: This function maps to the win32 CreateEvent. Demanding a out variable  of type HANDLE,
 *          a access mask and a OBJECT_ATTRIBUTES structure mapping to the SECURITY_ATTRIBUTES. ManualReset and InitialState are
 *          both parameters aswell ( possibly the order is reversed ).
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState
	);

NTSTATUS
STDCALL
ZwCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState
	);

/*
 * FUNCTION: Creates an eventpair object
 * ARGUMENTS:
 *        EventPairHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies access to the event
 *        ObjectAttribute = Initialized attributes for the object
 */

NTSTATUS
STDCALL
NtCreateEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwCreateEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
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

NTSTATUS
STDCALL
ZwCreateFile(
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

NTSTATUS
STDCALL
ZwCreateIoCompletion(
	OUT PHANDLE CompletionPort,
	IN ACCESS_MASK DesiredAccess,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG NumberOfConcurrentThreads
	);


/*
 * FUNCTION: Creates a mail slot file
 * ARGUMENTS:
 *        MailSlotFileHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the file
 *        ObjectAttributes = Contains the name of the mailslotfile.
 *        IoStatusBlock = 
 *        FileAttributes =
 *        ShareAccess =
 *        MaxMessageSize =
 *        TimeOut =
 *       
 * REMARKS: This funciton maps to the win32 function CreateMailSlot
 * RETURNS:
 *	Status
 */

NTSTATUS
STDCALL
NtCreateMailslotFile(
	OUT PHANDLE MailSlotFileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG FileAttributes,
	IN ULONG ShareAccess,
	IN ULONG MaxMessageSize,
	IN PLARGE_INTEGER TimeOut
	);

NTSTATUS
STDCALL
ZwCreateMailslotFile(
	OUT PHANDLE MailSlotFileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG FileAttributes,
	IN ULONG ShareAccess,
	IN ULONG MaxMessageSize,
	IN PLARGE_INTEGER TimeOut
	);

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
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	);

NTSTATUS
STDCALL
ZwCreateMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	);


/*
 * FUNCTION: Creates a paging file.
 * ARGUMENTS:
 *        FileName  = Name of the pagefile
 *        InitialSize = Specifies the initial size in bytes
 *        MaximumSize = Specifies the maximum size in bytes
 *        Reserved = Reserved for future use
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtCreatePagingFile(
	IN PUNICODE_STRING FileName,
	IN PLARGE_INTEGER InitialSize,
	IN PLARGE_INTEGER MaxiumSize,
	IN ULONG Reserved
	);

NTSTATUS
STDCALL
ZwCreatePagingFile(
	IN PUNICODE_STRING FileName,
	IN PLARGE_INTEGER InitialSize,
	IN PLARGE_INTEGER MaxiumSize,
	IN ULONG Reserved
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

NTSTATUS 
STDCALL 
ZwCreateProcess(
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
 * FUNCTION: Creates a profile
 * ARGUMENTS:
 *        ProfileHandle (OUT) = Caller supplied storage for the resulting handle
 *        ObjectAttribute = Initialized attributes for the object
 *        ImageBase = Start address of executable image
 *        ImageSize = Size of the image
 *        Granularity = Bucket size
 *        Buffer =  Caller supplies buffer for profiling info
 *        ProfilingSize = Buffer size
 *        ClockSource = Specify 0 / FALSE ??
 *        ProcessorMask = A value of -1 indicates disables  per processor profiling,
			  otherwise bit set for the processor to profile.
 * REMARKS:
 *        This function maps to the win32 CreateProcess. 
 * RETURNS: Status
 */

NTSTATUS 
STDCALL
NtCreateProfile(OUT PHANDLE ProfileHandle, 
		IN HANDLE ProcessHandle,
		IN PVOID ImageBase, 
		IN ULONG ImageSize, 
		IN ULONG Granularity,
		OUT PULONG Buffer, 
		IN ULONG ProfilingSize,
		IN KPROFILE_SOURCE Source,
		IN ULONG ProcessorMask);

NTSTATUS 
STDCALL
ZwCreateProfile(
	OUT PHANDLE ProfileHandle, 
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG ImageBase, 
	IN ULONG ImageSize, 
	IN ULONG Granularity,
	OUT PVOID Buffer, 
	IN ULONG ProfilingSize,
	IN ULONG ClockSource,
	IN ULONG ProcessorMask
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

NTSTATUS
STDCALL
ZwCreateSection( 
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
	IN LONG InitialCount,
	IN LONG MaximumCount
	);

NTSTATUS
STDCALL
ZwCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN LONG InitialCount,
	IN LONG MaximumCount
	);

/*
 * FUNCTION: Creates a symbolic link object
 * ARGUMENTS:
 *        SymbolicLinkHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the thread. 
 *        ObjectAttributes = Initialized attributes for the object.
 *        Name = Target name of the symbolic link  
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

NTSTATUS
STDCALL
ZwCreateSymbolicLinkObject(
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
 *        ClientId (OUT) = Caller supplies storage for returned process id and thread id.
 *        ThreadContext = Initial processor context for the thread.
 *        InitialTeb = Initial user mode stack context for the thread.
 *        CreateSuspended = Specifies if the thread is ready for scheduling
 * REMARKS:
 *        This function maps to the win32 function CreateThread.  
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtCreateThread(
	OUT	PHANDLE			ThreadHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	HANDLE			ProcessHandle,
	OUT	PCLIENT_ID		ClientId,
	IN	PCONTEXT		ThreadContext,
	IN	PINITIAL_TEB		InitialTeb,
	IN	BOOLEAN			CreateSuspended
	);

NTSTATUS
STDCALL 
ZwCreateThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ProcessHandle,
	OUT PCLIENT_ID ClientId,
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
 *        TimerType = Specifies if the timer should be reset manually.
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
	IN TIMER_TYPE TimerType
	);

NTSTATUS
STDCALL
ZwCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN TIMER_TYPE TimerType
	);

/*
 * FUNCTION: Creates a token.
 * ARGUMENTS:
 *        TokenHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the process can
 *                        be a combinate of STANDARD_RIGHTS_REQUIRED| ..  
 *        ObjectAttribute = Initialized attributes for the object, contains the rootdirectory and the filename
 *        TokenType = 
 *        AuthenticationId = 
 *        ExpirationTime = 
 *        TokenUser = 
 *        TokenGroups =
 *        TokenPrivileges = 
 *        TokenOwner = 
 *        TokenPrimaryGroup = 
 *        TokenDefaultDacl =
 *        TokenSource =
 * REMARKS:
 *        This function does not map to a win32 function
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtCreateToken(
	OUT PHANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TOKEN_TYPE TokenType,
	IN PLUID AuthenticationId,
	IN PLARGE_INTEGER ExpirationTime,
	IN PTOKEN_USER TokenUser,
	IN PTOKEN_GROUPS TokenGroups,
	IN PTOKEN_PRIVILEGES TokenPrivileges,
	IN PTOKEN_OWNER TokenOwner,
	IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
	IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
	IN PTOKEN_SOURCE TokenSource
	);

NTSTATUS
STDCALL
ZwCreateToken(
	OUT PHANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TOKEN_TYPE TokenType,
	IN PLUID AuthenticationId,
	IN PLARGE_INTEGER ExpirationTime,
	IN PTOKEN_USER TokenUser,
	IN PTOKEN_GROUPS TokenGroups,
	IN PTOKEN_PRIVILEGES TokenPrivileges,
	IN PTOKEN_OWNER TokenOwner,
	IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
	IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
	IN PTOKEN_SOURCE TokenSource
	);

/*
 * FUNCTION: Returns the callers thread TEB.
 * RETURNS: The resulting teb.
 */
#if 0
 NT_TEB *
STDCALL 
NtCurrentTeb(VOID
	);
#endif

/*
 * FUNCTION: Delays the execution of the calling thread.
 * ARGUMENTS:
 *        Alertable = If TRUE the thread is alertable during is wait period
 *        Interval  = Specifies the interval to wait.      
 * RETURNS: Status
 */
NTSTATUS STDCALL NtDelayExecution(IN ULONG Alertable, IN TIME* Interval);

NTSTATUS
STDCALL
ZwDelayExecution(
	IN BOOLEAN Alertable,
	IN TIME *Interval
	);


/*
 * FUNCTION: Deletes an atom from the global atom table
 * ARGUMENTS:
 *        Atom = Identifies the atom to delete
 * REMARKS:
 *	 The function maps to the win32 GlobalDeleteAtom
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDeleteAtom(
	IN RTL_ATOM Atom
	);

NTSTATUS
STDCALL
ZwDeleteAtom(
	IN RTL_ATOM Atom
	);

/*
 * FUNCTION: Deletes a file or a directory
 * ARGUMENTS:
 *        ObjectAttributes = Name of the file which should be deleted
 * REMARKS:
 *	 This system call is functionally equivalent to NtSetInformationFile
 *	 setting the disposition information.
 *	 The function maps to the win32 DeleteFile. 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
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
NTSTATUS
STDCALL
ZwDeleteKey(
	IN HANDLE KeyHandle
	);

/*
 * FUNCTION: Generates a audit message when an object is deleted
 * ARGUMENTS:
 *         SubsystemName = Spefies the name of the subsystem can be 'WIN32' or 'DEBUG'
 *         HandleId= Handle to an audit object
 *	   GenerateOnClose = Value returned by NtAccessCheckAndAuditAlarm
 * REMARKS: This function maps to the win32 ObjectCloseAuditAlarm
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtDeleteObjectAuditAlarm ( 
	IN PUNICODE_STRING SubsystemName, 
	IN PVOID HandleId, 
	IN BOOLEAN GenerateOnClose 
	);

NTSTATUS
STDCALL
ZwDeleteObjectAuditAlarm ( 
	IN PUNICODE_STRING SubsystemName, 
	IN PVOID HandleId, 
	IN BOOLEAN GenerateOnClose 
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

NTSTATUS
STDCALL
ZwDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName
	);
/*
 * FUNCTION: Sends IOCTL to the io sub system
 * ARGUMENTS:
 *        DeviceHandle = Points to the handle that is created by NtCreateFile
 *        Event = Event to synchronize on STATUS_PENDING
 *        ApcRoutine = Asynchroneous procedure callback
 *	  ApcContext = Callback context.
 *	  IoStatusBlock = Caller should supply storage for extra information.. 
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

NTSTATUS
STDCALL
ZwDeviceIoControlFile(
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

NTSTATUS
STDCALL
ZwDisplayString(
	IN PUNICODE_STRING DisplayString
	);

/*
 * FUNCTION: Copies a handle from one process space to another
 * ARGUMENTS:
 *         SourceProcessHandle = The source process owning the handle. The source process should have opened
 *			the SourceHandle with PROCESS_DUP_HANDLE access.
 *	   SourceHandle = The handle to the object.
 *	   TargetProcessHandle = The destination process owning the handle 
 *	   TargetHandle (OUT) = Caller should supply storage for the duplicated handle. 
 *	   DesiredAccess = The desired access to the handle.
 *	   InheritHandle = Indicates wheter the new handle will be inheritable or not.
 *	   Options = Specifies special actions upon duplicating the handle. Can be
 *			one of the values DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS.
 *			DUPLICATE_CLOSE_SOURCE specifies that the source handle should be
 *			closed after duplicating. DUPLICATE_SAME_ACCESS specifies to ignore
 *			the DesiredAccess paramter and just grant the same access to the new
 *			handle.
 * RETURNS: Status
 * REMARKS: This function maps to the win32 DuplicateHandle.
 */

NTSTATUS
STDCALL
NtDuplicateObject(
	IN HANDLE SourceProcessHandle,
	IN HANDLE SourceHandle,
	IN HANDLE TargetProcessHandle,
	OUT PHANDLE TargetHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN InheritHandle,
	IN ULONG Options
	);

NTSTATUS
STDCALL
ZwDuplicateObject(
	IN HANDLE SourceProcessHandle,
	IN PHANDLE SourceHandle,
	IN HANDLE TargetProcessHandle,
	OUT PHANDLE TargetHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN InheritHandle,
	IN ULONG Options
	);

NTSTATUS
STDCALL
NtDuplicateToken(  
	IN HANDLE ExistingToken, 
  	IN ACCESS_MASK DesiredAccess, 
 	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
  	IN TOKEN_TYPE TokenType,  
  	OUT PHANDLE NewToken     
	);

NTSTATUS
STDCALL
ZwDuplicateToken(  
	IN HANDLE ExistingToken, 
  	IN ACCESS_MASK DesiredAccess, 
 	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
  	IN TOKEN_TYPE TokenType,  
  	OUT PHANDLE NewToken     
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
	IN KEY_INFORMATION_CLASS KeyInformationClass,
	OUT PVOID KeyInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
ZwEnumerateKey(
	IN HANDLE KeyHandle,
	IN ULONG Index,
	IN KEY_INFORMATION_CLASS KeyInformationClass,
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
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
ZwEnumerateValueKey(
	IN HANDLE KeyHandle,
	IN ULONG Index,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
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
NTSTATUS
STDCALL
ZwExtendSection(
	IN HANDLE SectionHandle,
	IN ULONG NewMaximumSize
	);

/*
 * FUNCTION: Finds a atom
 * ARGUMENTS:
 *       AtomName = Name to search for.
 *       Atom = Caller supplies storage for the resulting atom
 * RETURNS: Status 
 * REMARKS:
 *	This funciton maps to the win32 GlobalFindAtom
 */
NTSTATUS
STDCALL
NtFindAtom(
	IN	PWSTR		AtomName,
	OUT	PRTL_ATOM	Atom OPTIONAL
	);

NTSTATUS
STDCALL
ZwFindAtom(
	IN	PWSTR		AtomName,
	OUT	PRTL_ATOM	Atom OPTIONAL
	);

/*
 * FUNCTION: Flushes chached file data to disk
 * ARGUMENTS:
 *       FileHandle = Points to the file
 *	 IoStatusBlock = Caller must supply storage to receive the result of the flush
 *		buffers operation. The information field is set to number of bytes
 *		flushed to disk.
 * RETURNS: Status 
 * REMARKS:
 *	This funciton maps to the win32 FlushFileBuffers
 */
NTSTATUS
STDCALL
NtFlushBuffersFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	);

NTSTATUS
STDCALL
ZwFlushBuffersFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock
	);
/*
 * FUNCTION: Flushes a the processors instruction cache
 * ARGUMENTS:
 *       ProcessHandle = Points to the process owning the cache
 *	 BaseAddress = // might this be a image address ????
 *	 NumberOfBytesToFlush = 
 * RETURNS: Status 
 * REMARKS:
 *	This funciton is used by debuggers
 */
NTSTATUS
STDCALL
NtFlushInstructionCache(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN UINT NumberOfBytesToFlush
	);
NTSTATUS
STDCALL
ZwFlushInstructionCache(
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
 *	This funciton maps to the win32 RegFlushKey.
 */
NTSTATUS
STDCALL
NtFlushKey(
	IN HANDLE KeyHandle
	);

NTSTATUS
STDCALL
ZwFlushKey(
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
 *	  Check return status on STATUS_NOT_MAPPED_DATA 
 */
NTSTATUS
STDCALL
NtFlushVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG NumberOfBytesToFlush,
	OUT PULONG NumberOfBytesFlushed OPTIONAL
	);
NTSTATUS
STDCALL
ZwFlushVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG NumberOfBytesToFlush,
	OUT PULONG NumberOfBytesFlushed OPTIONAL
	);
 
/*
 * FUNCTION: Flushes the dirty pages to file
 * RETURNS: Status
 * FIXME: Not sure this does (how is the file specified)
 */
NTSTATUS STDCALL NtFlushWriteBuffer(VOID);
NTSTATUS STDCALL ZwFlushWriteBuffer(VOID);                      

 /*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a 
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of 
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status 
 */
NTSTATUS STDCALL NtFreeVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID  *BaseAddress,	
				     IN PULONG  RegionSize,	
				     IN ULONG  FreeType);
NTSTATUS STDCALL ZwFreeVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID  *BaseAddress,	
				     IN PULONG  RegionSize,	
				     IN ULONG  FreeType);  

/*
 * FUNCTION: Sends FSCTL to the filesystem
 * ARGUMENTS:
 *        DeviceHandle = Points to the handle that is created by NtCreateFile
 *        Event = Event to synchronize on STATUS_PENDING
 *        ApcRoutine = 
 *	  ApcContext =
 *	  IoStatusBlock = Caller should supply storage for 
 *        IoControlCode = Contains the File System Control command. This is an 
 *			index to the structures in InputBuffer and OutputBuffer.
 *		FSCTL_GET_RETRIEVAL_POINTERS  	MAPPING_PAIR
 *		FSCTL_GET_RETRIEVAL_POINTERS  	GET_RETRIEVAL_DESCRIPTOR
 *		FSCTL_GET_VOLUME_BITMAP  	BITMAP_DESCRIPTOR
 *		FSCTL_MOVE_FILE  		MOVEFILE_DESCRIPTOR
 *
 *	  InputBuffer = Caller should supply storage for input buffer if FCTL expects one.
 * 	  InputBufferSize = Size of the input bufffer
 *        OutputBuffer = Caller should supply storage for output buffer if FCTL expects one.
 *        OutputBufferSize  = Size of the input bufffer
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PENDING | STATUS_ACCESS_DENIED | STATUS_INSUFFICIENT_RESOURCES |
 *		STATUS_INVALID_PARAMETER | STATUS_INVALID_DEVICE_REQUEST ]
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

NTSTATUS
STDCALL
ZwFsControlFile(
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

NTSTATUS
STDCALL 
ZwGetContextThread(
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

NTSTATUS
STDCALL 
ZwGetTickCount(
	PULONG UpTime
	);

/*
 * FUNCTION: Sets a thread to impersonate another 
 * ARGUMENTS:
 *        ThreadHandle = Server thread that will impersonate a client.
	  ThreadToImpersonate = Client thread that will be impersonated
	  SecurityQualityOfService = Specifies the impersonation level.
 * RETURNS: Status 
 */

NTSTATUS
STDCALL 
NtImpersonateThread(
	IN HANDLE ThreadHandle,
	IN HANDLE ThreadToImpersonate,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
	);

NTSTATUS
STDCALL 
ZwImpersonateThread(
	IN HANDLE ThreadHandle,
	IN HANDLE ThreadToImpersonate,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
	);

/*
 * FUNCTION: Initializes the registry.
 * ARGUMENTS:
 *        SetUpBoot = This parameter is true for a setup boot.
 * RETURNS: Status 
 */
NTSTATUS
STDCALL 
NtInitializeRegistry(
	BOOLEAN SetUpBoot
	);
NTSTATUS
STDCALL 
ZwInitializeRegistry(
	BOOLEAN SetUpBoot
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

NTSTATUS
STDCALL 
ZwLoadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

/*
 * FUNCTION: Loads a registry key.
 * ARGUMENTS:
 *       KeyHandle = Handle to the registry key
 *       ObjectAttributes = ???
 * REMARK:
 *      This procedure maps to the win32 procedure RegLoadKey
 * RETURNS: Status
 */
NTSTATUS
STDCALL 
NtLoadKey(
	PHANDLE KeyHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
	);
NTSTATUS
STDCALL 
ZwLoadKey(
	PHANDLE KeyHandle,
	POBJECT_ATTRIBUTES ObjectAttributes
	);

/*
 * FUNCTION: Loads a registry key.
 * ARGUMENTS:
 *       KeyHandle = Handle to the registry key
 *       ObjectAttributes = ???
 *       Unknown3 = ???
 * REMARK:
 *       This procedure maps to the win32 procedure RegLoadKey
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtLoadKey2 (
	PHANDLE			KeyHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	ULONG			Unknown3
	);
NTSTATUS
STDCALL
ZwLoadKey2 (
	PHANDLE			KeyHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	ULONG			Unknown3
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
 *       ByteOffset = Offset 
 *       Length = Number of bytes to lock.
 *       Key  = Special value to give other threads the possibility to unlock the file
		by supplying the key in a call to NtUnlockFile.
 *       FailImmediatedly = If false the request will block untill the lock is obtained. 
 *       ExclusiveLock = Specifies whether a exclusive or a shared lock is obtained.
 * REMARK:
	This procedure maps to the win32 procedure LockFileEx. STATUS_PENDING is returned if the lock could
	not be obtained immediately, the device queue is busy and the IRP is queued.
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PENDING | STATUS_ACCESS_DENIED | STATUS_INSUFFICIENT_RESOURCES |
		STATUS_INVALID_PARAMETER | STATUS_INVALID_DEVICE_REQUEST | STATUS_LOCK_NOT_GRANTED ]

 */	
NTSTATUS 
STDCALL
NtLockFile(
	IN  HANDLE FileHandle,
	IN  HANDLE Event OPTIONAL,
	IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN  PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN  PLARGE_INTEGER ByteOffset,
	IN  PLARGE_INTEGER Length,
	IN  PULONG Key,
	IN  BOOLEAN FailImmediatedly,
	IN  BOOLEAN ExclusiveLock
	);

NTSTATUS 
STDCALL
ZwLockFile(
	IN  HANDLE FileHandle,
	IN  HANDLE Event OPTIONAL,
	IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN  PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN  PLARGE_INTEGER ByteOffset,
	IN  PLARGE_INTEGER Length,
	IN  PULONG Key,
	IN  BOOLEAN FailImmediatedly,
	IN  BOOLEAN ExclusiveLock
	);
/*
 * FUNCTION: Locks a range of virtual memory. 
 * ARGUMENTS: 
 *       ProcessHandle = Handle to the process
 *       BaseAddress =  Lower boundary of the range of bytes to lock. 
 *       NumberOfBytesLock = Offset to the upper boundary.
 *       NumberOfBytesLocked (OUT) = Number of bytes actually locked.
 * REMARK:
	This procedure maps to the win32 procedure VirtualLock 
 * RETURNS: Status [STATUS_SUCCESS | STATUS_WAS_LOCKED ]
 */	
NTSTATUS
STDCALL 
NtLockVirtualMemory(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	ULONG NumberOfBytesToLock,
	PULONG NumberOfBytesLocked
	);
NTSTATUS
STDCALL 
ZwLockVirtualMemory(
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

NTSTATUS
STDCALL
ZwMakeTemporaryObject(
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
	IN ULONG AccessProtection
	);

NTSTATUS
STDCALL
ZwMapViewOfSection(
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
	  Event = 
 *        ApcRoutine   = Start address
 *        ApcContext = Delimits the range of virtual memory
 *				for which the new access protection holds
 *        IoStatusBlock = The new access proctection for the pages
 *        Buffer = Caller supplies storage for resulting information --> FILE_NOTIFY_INFORMATION
 *	  BufferSize = 	Size of the buffer
	  CompletionFilter = Can be one of the following values:
			FILE_NOTIFY_CHANGE_FILE_NAME
			FILE_NOTIFY_CHANGE_DIR_NAME
			FILE_NOTIFY_CHANGE_NAME ( FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME ) 
			FILE_NOTIFY_CHANGE_ATTRIBUTES
			FILE_NOTIFY_CHANGE_SIZE
			FILE_NOTIFY_CHANGE_LAST_WRITE
			FILE_NOTIFY_CHANGE_LAST_ACCESS
			FILE_NOTIFY_CHANGE_CREATION ( change of creation timestamp )
			FILE_NOTIFY_CHANGE_EA
			FILE_NOTIFY_CHANGE_SECURITY
			FILE_NOTIFY_CHANGE_STREAM_NAME
			FILE_NOTIFY_CHANGE_STREAM_SIZE
			FILE_NOTIFY_CHANGE_STREAM_WRITE
	  WatchTree = If true the notify will be installed recursively on the targetdirectory and all subdirectories.
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
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	IN ULONG CompletionFilter,
	IN BOOLEAN WatchTree
	);

NTSTATUS
STDCALL
ZwNotifyChangeDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	IN ULONG CompletionFilter,
	IN BOOLEAN WatchTree
	);

/*
 * FUNCTION: Installs a notfication callback on registry changes
 * ARGUMENTS:
	KeyHandle = Handle to the registry key
	Event = Event that should be signalled on modification of the key
	ApcRoutine = Routine that should be called on modification of the key
	ApcContext = Argument to the ApcRoutine
	IoStatusBlock = ???
	CompletionFilter = Specifies the kind of notification the caller likes to receive.
			Can be a combination of the following values:

			REG_NOTIFY_CHANGE_NAME
			REG_NOTIFY_CHANGE_ATTRIBUTES
			REG_NOTIFY_CHANGE_LAST_SET
			REG_NOTIFY_CHANGE_SECURITY
				
				
	Asynchroneous = If TRUE the changes are reported by signalling an event if false
			the function will not return before a change occurs.
	ChangeBuffer =  Will return the old value
	Length = Size of the change buffer
	WatchSubtree =  Indicates if the caller likes to receive a notification of changes in
			sub keys or not.
 * REMARKS: If the key is closed the event is signalled aswell.
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	);

NTSTATUS
STDCALL
ZwNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	);

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
NTSTATUS
STDCALL
ZwOpenDirectoryObject(
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

NTSTATUS
STDCALL
ZwOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

/*
 * FUNCTION: Opens an existing event pair
 * ARGUMENTS:
 *        EventHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the event
 *        ObjectAttributes = Initialized attributes for the object
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtOpenEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwOpenEventPair(
	OUT PHANDLE EventPairHandle,
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
 *        OpenOptions =
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
	IN ULONG OpenOptions
	);

NTSTATUS
STDCALL
ZwOpenFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG OpenOptions
	);

/*
 * FUNCTION: Opens an existing io completion object
 * ARGUMENTS:
 *        CompletionPort (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the io completion object
 *        ObjectAttributes = Initialized attributes for the object
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtOpenIoCompletion(
	OUT PHANDLE CompetionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwOpenIoCompletion(
	OUT PHANDLE CompetionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
	
/*
 * FUNCTION: Opens an existing key in the registry
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the key
 *        ObjectAttributes = Initialized attributes for the object
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenKey(
	OUT PHANDLE KeyHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwOpenKey(
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
NTSTATUS
STDCALL
ZwOpenMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
NtOpenObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PVOID HandleId,	
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ClientToken,	
	IN ULONG DesiredAccess,	
	IN ULONG GrantedAccess,	
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN ObjectCreation,	
	IN BOOLEAN AccessGranted,	
	OUT PBOOLEAN GenerateOnClose 	
	);

NTSTATUS
STDCALL
ZwOpenObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PVOID HandleId,	
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ClientToken,	
	IN ULONG DesiredAccess,	
	IN ULONG GrantedAccess,	
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN ObjectCreation,	
	IN BOOLEAN AccessGranted,	
	OUT PBOOLEAN GenerateOnClose 	
	);
/*
 * FUNCTION: Opens an existing process
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Requested access to the process
 *        ObjectAttribute = Initialized attributes for the object
 *        ClientId = Identifies the process id to open
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
NTSTATUS 
STDCALL
ZwOpenProcess (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	); 
/*
 * FUNCTION: Opens an existing process
 * ARGUMENTS:
 *        ProcessHandle  = Handle of the process of which owns the token
 *        DesiredAccess = Requested access to the token
 *	  TokenHandle (OUT) = Caller supplies storage for the resulting token.
 * REMARKS:
	This function maps to the win32 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtOpenProcessToken(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
ZwOpenProcessToken(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE TokenHandle
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
NTSTATUS
STDCALL
ZwOpenSection(
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
NTSTATUS
STDCALL
ZwOpenSemaphore(
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
NTSTATUS
STDCALL
ZwOpenSymbolicLinkObject(
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
 *	  ClientId = Identifies the thread to open.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	);
NTSTATUS
STDCALL
ZwOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	);

NTSTATUS
STDCALL
NtOpenThreadToken(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
ZwOpenThreadToken(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	OUT PHANDLE TokenHandle
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
NTSTATUS
STDCALL
ZwOpenTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

/*
 * FUNCTION: Checks an access token for specific privileges
 * ARGUMENTS:
 *        ClientToken = Handle to a access token structure
 *        RequiredPrivileges = Specifies the requested privileges.
 *        Result = Caller supplies storage for the result. If PRIVILEGE_SET_ALL_NECESSARY is
		   set in the Control member of PRIVILEGES_SET Result
		   will only be TRUE if all privileges are present in the access token. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtPrivilegeCheck(
	IN HANDLE ClientToken,
	IN PPRIVILEGE_SET RequiredPrivileges,
	IN PBOOLEAN Result
	);

NTSTATUS
STDCALL
ZwPrivilegeCheck(
	IN HANDLE ClientToken,
	IN PPRIVILEGE_SET RequiredPrivileges,
	IN PBOOLEAN Result
	);

NTSTATUS
STDCALL
NtPrivilegedServiceAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PUNICODE_STRING ServiceName,
	IN HANDLE ClientToken,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted
	);

NTSTATUS
STDCALL
ZwPrivilegedServiceAuditAlarm(
	IN PUNICODE_STRING SubsystemName,	
	IN PUNICODE_STRING ServiceName,	
	IN HANDLE ClientToken,
	IN PPRIVILEGE_SET Privileges,	
	IN BOOLEAN AccessGranted 	
	);	

NTSTATUS
STDCALL
NtPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE ClientToken,
	IN ULONG DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted
	);

NTSTATUS
STDCALL
ZwPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE ClientToken,
	IN ULONG DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted
	);

/*
 * FUNCTION: Entry point for native applications
 * ARGUMENTS:
 *	Peb = Pointes to the Process Environment Block (PEB)
 * REMARKS:
 *	Native applications should use this function instead of a main.
 *	Calling proces should terminate itself.
 * RETURNS: Status
 */ 	
VOID
NtProcessStartup(
	IN	PPEB	Peb
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

NTSTATUS
STDCALL
ZwProtectVirtualMemory(
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
 *        PulseCount = Number of times the action is repeated
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtPulseEvent(
	IN HANDLE EventHandle,
	IN PULONG PulseCount OPTIONAL
	);

NTSTATUS
STDCALL
ZwPulseEvent(
	IN HANDLE EventHandle,
	IN PULONG PulseCount OPTIONAL
	);

/*
 * FUNCTION: Queries the attributes of a file
 * ARGUMENTS:
 *        ObjectAttributes = Initialized attributes for the object
 *        Buffer = Caller supplies storage for the attributes
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtQueryAttributesFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PVOID Buffer
	);

NTSTATUS
STDCALL
ZwQueryAttributesFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PVOID Buffer
	);

/*
 * FUNCTION: Queries the default locale id
 * ARGUMENTS:
 *        UserProfile = Type of locale id
 *              TRUE: thread locale id
 *              FALSE: system locale id
 *        DefaultLocaleId = Caller supplies storage for the locale id
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtQueryDefaultLocale(
	IN BOOLEAN UserProfile,
	OUT PLCID DefaultLocaleId
	);

NTSTATUS
STDCALL
ZwQueryDefaultLocale(
	IN BOOLEAN UserProfile,
	OUT PLCID DefaultLocaleId
	);

/*
 * FUNCTION: Queries a directory file.
 * ARGUMENTS:
 *	  FileHandle = Handle to a directory file
 *        EventHandle  = Handle to the event signaled on completion
 *	  ApcRoutine = Asynchroneous procedure callback, called on completion
 *	  ApcContext = Argument to the apc.
 *	  IoStatusBlock = Caller supplies storage for extended status information.
 *	  FileInformation = Caller supplies storage for the resulting information.
 *
 *		FileNameInformation  		FILE_NAMES_INFORMATION
 *		FileDirectoryInformation  	FILE_DIRECTORY_INFORMATION
 *		FileFullDirectoryInformation 	FILE_FULL_DIRECTORY_INFORMATION
 *		FileBothDirectoryInformation	FILE_BOTH_DIR_INFORMATION
 *
 *	  Length = Size of the storage supplied
 *	  FileInformationClass = Indicates the type of information requested.  
 *	  ReturnSingleEntry = Specify true if caller only requests the first directory found.
 *	  FileName = Initial directory name to query, that may contain wild cards.
 *        RestartScan = Number of times the action should be repeated
 * RETURNS: Status [ STATUS_SUCCESS, STATUS_ACCESS_DENIED, STATUS_INSUFFICIENT_RESOURCES,
 *		     STATUS_INVALID_PARAMETER, STATUS_INVALID_DEVICE_REQUEST, STATUS_BUFFER_OVERFLOW,
 *		     STATUS_INVALID_INFO_CLASS, STATUS_NO_SUCH_FILE, STATUS_NO_MORE_FILES ]
 */

NTSTATUS
STDCALL
NtQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	);

NTSTATUS
STDCALL
ZwQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	);

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
	IN	HANDLE			DirObjHandle,
	OUT	POBJDIR_INFORMATION	DirObjInformation,
	IN	ULONG			BufferLength,
	IN	BOOLEAN			GetNextIndex,
	IN	BOOLEAN			IgnoreInputIndex,
	IN OUT	PULONG			ObjectIndex,
	OUT	PULONG			DataWritten OPTIONAL
	);

NTSTATUS
STDCALL
ZwQueryDirectoryObject(
	IN HANDLE DirObjHandle,
	OUT POBJDIR_INFORMATION DirObjInformation,
	IN ULONG                BufferLength,
	IN BOOLEAN              GetNextIndex,
	IN BOOLEAN              IgnoreInputIndex,
	IN OUT PULONG           ObjectIndex,
	OUT PULONG              DataWritten OPTIONAL
	);

/*
 * FUNCTION: Queries the extended attributes of a file
 * ARGUMENTS:
 *        FileHandle  = Handle to the event
 *        IoStatusBlock = Number of times the action is repeated
 *        Buffer
 *        Length
 *        ReturnSingleEntry
 *        EaList
 *        EaListLength
 *        EaIndex
 *        RestartScan
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtQueryEaFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN BOOLEAN ReturnSingleEntry,
	IN PVOID EaList OPTIONAL,
	IN ULONG EaListLength,
	IN PULONG EaIndex OPTIONAL,
	IN BOOLEAN RestartScan
	);

NTSTATUS
STDCALL
ZwQueryEaFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN BOOLEAN ReturnSingleEntry,
	IN PVOID EaList OPTIONAL,
	IN ULONG EaListLength,
	IN PULONG EaIndex OPTIONAL,
	IN BOOLEAN RestartScan
	);

/*
 * FUNCTION: Queries an event
 * ARGUMENTS:
 *        EventHandle  = Handle to the event
 *        EventInformationClass = Index of the information structure
	
	  EventBasicInformation		EVENT_BASIC_INFORMATION

 *	  EventInformation = Caller supplies storage for the information structure
 *	  EventInformationLength =  Size of the information structure
 *	  ReturnLength = Data written
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtQueryEvent(
	IN HANDLE EventHandle,
	IN EVENT_INFORMATION_CLASS EventInformationClass,
	OUT PVOID EventInformation,
	IN ULONG EventInformationLength,
	OUT PULONG ReturnLength
	);
NTSTATUS
STDCALL
ZwQueryEvent(
	IN HANDLE EventHandle,
	IN EVENT_INFORMATION_CLASS EventInformationClass,
	OUT PVOID EventInformation,
	IN ULONG EventInformationLength,
	OUT PULONG ReturnLength
	);

NTSTATUS
STDCALL
NtQueryFullAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Attributes
	);
NTSTATUS
STDCALL
ZwQueryFullAttributesFile(
	IN HANDLE FileHandle,
	IN PVOID Attributes
	);

NTSTATUS
STDCALL
NtQueryInformationAtom(
	IN	RTL_ATOM		Atom,
	IN	ATOM_INFORMATION_CLASS	AtomInformationClass,
	OUT	PVOID			AtomInformation,
	IN	ULONG			AtomInformationLength,
	OUT	PULONG			ReturnLength OPTIONAL
	);

NTSTATUS
STDCALL
NtQueryInformationAtom(
	IN	RTL_ATOM		Atom,
	IN	ATOM_INFORMATION_CLASS	AtomInformationClass,
	OUT	PVOID			AtomInformation,
	IN	ULONG			AtomInformationLength,
	OUT	PULONG			ReturnLength OPTIONAL
	);


/*
 * FUNCTION: Queries the information of a file object.
 * ARGUMENTS: 
 *        FileHandle = Handle to the file object
 *	  IoStatusBlock = Caller supplies storage for extended information 
 *                        on the current operation.
 *        FileInformation = Storage for the new file information
 *        Lenght = Size of the storage for the file information.
 *        FileInformationClass = Indicates which file information is queried

	  FileDirectoryInformation 		FILE_DIRECTORY_INFORMATION
	  FileFullDirectoryInformation		FILE_FULL_DIRECTORY_INFORMATION
	  FileBothDirectoryInformation		FILE_BOTH_DIRECTORY_INFORMATION
	  FileBasicInformation			FILE_BASIC_INFORMATION
	  FileStandardInformation		FILE_STANDARD_INFORMATION
	  FileInternalInformation		FILE_INTERNAL_INFORMATION
	  FileEaInformation			FILE_EA_INFORMATION
	  FileAccessInformation			FILE_ACCESS_INFORMATION
	  FileNameInformation 			FILE_NAME_INFORMATION
	  FileRenameInformation			FILE_RENAME_INFORMATION
	  FileLinkInformation			
	  FileNamesInformation			FILE_NAMES_INFORMATION
	  FileDispositionInformation		FILE_DISPOSITION_INFORMATION
	  FilePositionInformation		FILE_POSITION_INFORMATION
	  FileFullEaInformation			FILE_FULL_EA_INFORMATION		
	  FileModeInformation			FILE_MODE_INFORMATION
	  FileAlignmentInformation		FILE_ALIGNMENT_INFORMATION
	  FileAllInformation			FILE_ALL_INFORMATION

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
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

NTSTATUS
STDCALL
ZwQueryInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass
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
		ProcessRaisePriority		 KPRIORITY
		ProcessDebugPort		 HANDLE
		ProcessExceptionPort		 HANDLE	
		ProcessAccessToken		 PROCESS_ACCESS_TOKEN
		ProcessLdtInformation		 LDT_ENTRY ??
		ProcessLdtSize			 ULONG
		ProcessDefaultHardErrorMode	 ULONG
		ProcessIoPortHandlers		 // kernel mode only
		ProcessPooledUsageAndLimits 	 POOLED_USAGE_AND_LIMITS
		ProcessWorkingSetWatch 		 PROCESS_WS_WATCH_INFORMATION 		
		ProcessUserModeIOPL		 (I/O Privilege Level)
		ProcessEnableAlignmentFaultFixup BOOLEAN	
		ProcessPriorityClass		 ULONG
		ProcessWx86Information		 ULONG	
		ProcessHandleCount		 ULONG
		ProcessAffinityMask		 ULONG	
		ProcessPooledQuotaLimits 	 QUOTA_LIMITS
		MaxProcessInfoClass		 

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

NTSTATUS
STDCALL
ZwQueryInformationProcess(
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

		ThreadBasicInformation			THREAD_BASIC_INFORMATION	
		ThreadTimes 				KERNEL_USER_TIMES
		ThreadPriority				KPRIORITY	
		ThreadBasePriority			KPRIORITY	
		ThreadAffinityMask			KAFFINITY	
		ThreadImpersonationToken		
		ThreadDescriptorTableEntry		
		ThreadEnableAlignmentFaultFixup		
		ThreadEventPair				
		ThreadQuerySetWin32StartAddress		
		ThreadZeroTlsCell			
		ThreadPerformanceCount			
		ThreadAmILastThread			BOOLEAN
		ThreadIdealProcessor			ULONG
		ThreadPriorityBoost			ULONG	
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
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength 
	);


NTSTATUS
STDCALL
NtQueryInformationToken(
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,
	IN ULONG TokenInformationLength,
	OUT PULONG ReturnLength
	);

NTSTATUS
STDCALL
ZwQueryInformationToken(
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,
	IN ULONG TokenInformationLength,
	OUT PULONG ReturnLength
	);

/*
 * FUNCTION: Query the interval and the clocksource for profiling
 * ARGUMENTS:
	Interval =   
        ClockSource = 
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtQueryIntervalProfile(
	OUT PULONG Interval,
	OUT KPROFILE_SOURCE ClockSource
	);

NTSTATUS
STDCALL
ZwQueryIntervalProfile(
	OUT PULONG Interval,
	OUT KPROFILE_SOURCE ClockSource
	);



NTSTATUS
STDCALL
NtQueryIoCompletion(
	IN HANDLE CompletionPort,
	IN ULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PULONG NumberOfBytesTransferred
	);
NTSTATUS
STDCALL
ZwQueryIoCompletion(
	IN HANDLE CompletionPort,
	IN ULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PULONG NumberOfBytesTransferred
	);


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
	IN KEY_INFORMATION_CLASS KeyInformationClass,
	OUT PVOID KeyInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
ZwQueryKey(
	IN HANDLE KeyHandle,
	IN KEY_INFORMATION_CLASS KeyInformationClass,
	OUT PVOID KeyInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);


// draft

NTSTATUS
STDCALL
NtQueryMultipleValueKey(
	IN HANDLE KeyHandle,
	IN OUT PKEY_VALUE_ENTRY ValueList,
	IN ULONG NumberOfValues,
	OUT PVOID Buffer,
	IN OUT PULONG Length,
	OUT PULONG ReturnLength
	);

NTSTATUS
STDCALL
ZwQueryMultipleValueKey(
	IN HANDLE KeyHandle,
	IN OUT PKEY_VALUE_ENTRY ValueList,
	IN ULONG NumberOfValues,
	OUT PVOID Buffer,
	IN OUT PULONG Length,
	OUT PULONG ReturnLength
	);

/*
 * FUNCTION: Queries the information of a mutant object.
 * ARGUMENTS: 
	MutantHandle = Handle to a mutant
	MutantInformationClass = Index to a certain information structure
	MutantInformation = Caller supplies storage for resulting information
	Length = Size of the supplied storage 
	ResultLength = Bytes written
 */
NTSTATUS
STDCALL
NtQueryMutant(
	IN HANDLE MutantHandle,
	IN CINT MutantInformationClass,
	OUT PVOID MutantInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
ZwQueryMutant(
	IN HANDLE MutantHandle,
	IN CINT MutantInformationClass,
	OUT PVOID MutantInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);
/*
 * FUNCTION: Queries the information of a  object.
 * ARGUMENTS: 
	ObjectHandle = Handle to a object
	ObjectInformationClass = Index to a certain information structure

	ObjectBasicInformation  	
	ObjectTypeInformation 		OBJECT_TYPE_INFORMATION 
	ObjectNameInformation		OBJECT_NAME_INFORMATION
	ObjectDataInformation		OBJECT_DATA_INFORMATION

	ObjectInformation = Caller supplies storage for resulting information
	Length = Size of the supplied storage 
 	ResultLength = Bytes written
 */

NTSTATUS
STDCALL
NtQueryObject(
	IN HANDLE ObjectHandle,
	IN CINT ObjectInformationClass,
	OUT PVOID ObjectInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
ZwQueryObject(
	IN HANDLE ObjectHandle,
	IN CINT ObjectInformationClass,
	OUT PVOID ObjectInformation,
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

NTSTATUS
STDCALL
ZwQueryPerformanceCounter(
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

NTSTATUS
STDCALL
ZwQuerySection(
	IN HANDLE SectionHandle,
	IN CINT SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

NTSTATUS
STDCALL
NtQuerySecurityObject(
	IN HANDLE Object,
	IN CINT SecurityObjectInformationClass,
	OUT PVOID SecurityObjectInformation,
	IN ULONG Length,
	OUT PULONG ReturnLength
	);

NTSTATUS
STDCALL
ZwQuerySecurityObject(
	IN HANDLE Object,
	IN CINT SecurityObjectInformationClass,
	OUT PVOID SecurityObjectInformation,
	IN ULONG Length,
	OUT PULONG ReturnLength
	);


/*
 * FUNCTION: Queries the information of a semaphore.
 * ARGUMENTS: 
 *        SemaphoreHandle = Handle to the semaphore object
 *        SemaphoreInformationClass = Index to a certain information structure

	  SemaphoreBasicInformation	SEMAPHORE_BASIC_INFORMATION

 *	  SemaphoreInformation = Caller supplies storage for the semaphore information structure
 *	  Length = Size of the infomation structure
 */
NTSTATUS
STDCALL
NtQuerySemaphore(
	IN	HANDLE				SemaphoreHandle,
	IN	SEMAPHORE_INFORMATION_CLASS	SemaphoreInformationClass,
	OUT	PVOID				SemaphoreInformation,
	IN	ULONG				Length,
	OUT	PULONG				ReturnLength
	);

NTSTATUS
STDCALL
ZwQuerySemaphore(
	IN	HANDLE				SemaphoreHandle,
	IN	SEMAPHORE_INFORMATION_CLASS	SemaphoreInformationClass,
	OUT	PVOID				SemaphoreInformation,
	IN	ULONG				Length,
	OUT	PULONG				ReturnLength
	);


/*
 * FUNCTION: Queries the information of a symbolic link object.
 * ARGUMENTS: 
 *        SymbolicLinkHandle = Handle to the symbolic link object
 *	  LinkTarget = resolved name of link
 *        DataWritten = size of the LinkName.
 * RETURNS: Status
 *
*/
NTSTATUS
STDCALL
NtQuerySymbolicLinkObject(
	IN HANDLE               SymLinkObjHandle,
	OUT PUNICODE_STRING     LinkTarget,
	OUT PULONG              DataWritten OPTIONAL
	);

NTSTATUS
STDCALL
ZwQuerySymbolicLinkObject(
	IN HANDLE               SymLinkObjHandle,
	OUT PUNICODE_STRING     LinkName,
	OUT PULONG              DataWritten OPTIONAL
	); 


/*
 * FUNCTION: Queries a system environment variable.
 * ARGUMENTS: 
 *        Name = Name of the variable
 *	  Value (OUT) = value of the variable
 *        Length = size of the buffer
 *        ReturnLength = data written
 * RETURNS: Status
 *
*/
NTSTATUS
STDCALL
NtQuerySystemEnvironmentValue(
	IN PUNICODE_STRING Name,
	OUT PVOID Value,
	ULONG Length,
	PULONG ReturnLength
	);

NTSTATUS
STDCALL
ZwQuerySystemEnvironmentValue(
	IN PUNICODE_STRING Name,
	OUT PVOID Value,
	ULONG Length,
	PULONG ReturnLength
	);


/*
 * FUNCTION: Queries the system information.
 * ARGUMENTS: 
 *        SystemInformationClass = Index to a certain information structure

	  SystemTimeAdjustmentInformation 	SYSTEM_TIME_ADJUSTMENT
	  SystemCacheInformation 		SYSTEM_CACHE_INFORMATION
	  SystemConfigurationInformation	CONFIGURATION_INFORMATION

 *	  SystemInformation = caller supplies storage for the information structure
 *        Length = size of the structure
	  ResultLength = Data written
 * RETURNS: Status
 *
*/
NTSTATUS
STDCALL
NtQuerySystemInformation(
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	OUT	PVOID				SystemInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
	);

NTSTATUS
STDCALL
ZwQuerySystemInformation(
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	OUT	PVOID				SystemInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
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
	OUT TIME *CurrentTime
	);

NTSTATUS
STDCALL
ZwQuerySystemTime (
	OUT TIME *CurrentTime
	);

/*
 * FUNCTION: Queries information about a timer
 * ARGUMENTS: 
 *        TimerHandle  = Handle to the timer
	  TimerValueInformationClass = Index to a certain information structure
	  TimerValueInformation = Caller supplies storage for the information structure
	  Length = Size of the information structure
	  ResultLength = Data written
 * RETURNS: Status
 *
*/       
NTSTATUS
STDCALL
NtQueryTimer(
	IN HANDLE TimerHandle,
	IN CINT TimerInformationClass,
	OUT PVOID TimerInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);
NTSTATUS
STDCALL
ZwQueryTimer(
	IN HANDLE TimerHandle,
	IN CINT TimerInformationClass,
	OUT PVOID TimerInformation,
	IN ULONG Length,
	OUT PULONG ResultLength
	);

/*
 * FUNCTION: Queries the timer resolution
 * ARGUMENTS: 
 *        MinimumResolution (OUT) = Caller should supply storage for the resulting time.
	  Maximum Resolution (OUT) = Caller should supply storage for the resulting time.
	  ActualResolution (OUT) = Caller should supply storage for the resulting time.
 * RETURNS: Status
 *
*/        


NTSTATUS
STDCALL 
NtQueryTimerResolution ( 
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution, 
	OUT PULONG ActualResolution 
	); 

NTSTATUS
STDCALL 
ZwQueryTimerResolution ( 
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
    	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    	OUT PVOID KeyValueInformation,
    	IN ULONG Length,
	OUT PULONG ResultLength
    	);

NTSTATUS
STDCALL
ZwQueryValueKey(
    	IN HANDLE KeyHandle,
    	IN PUNICODE_STRING ValueName,
    	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
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
NTSTATUS
STDCALL
ZwQueryVirtualMemory(
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
 *	  FileHandle  = Handle to a file object on the target volume
 *	  IoStatusBlock = Caller should supply storage for additional status information
 *	  ReturnLength = DataWritten
 *	  FsInformation = Caller should supply storage for the information structure.
 *	  Length = Size of the information structure
 *	  FsInformationClass = Index to a information structure

		FileFsVolumeInformation		FILE_FS_VOLUME_INFORMATION
		FileFsLabelInformation		FILE_FS_LABEL_INFORMATION	
		FileFsSizeInformation		FILE_FS_SIZE_INFORMATION
		FileFsDeviceInformation		FILE_FS_DEVICE_INFORMATION
		FileFsAttributeInformation	FILE_FS_ATTRIBUTE_INFORMATION
		FileFsControlInformation	
		FileFsQuotaQueryInformation	--
		FileFsQuotaSetInformation	--
		FileFsMaximumInformation	

 * RETURNS: Status [ STATUS_SUCCESS | STATUS_INSUFFICIENT_RESOURCES | STATUS_INVALID_PARAMETER |
		 STATUS_INVALID_DEVICE_REQUEST | STATUS_BUFFER_OVERFLOW ]
 *
*/
NTSTATUS
STDCALL
NtQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass 
	);

NTSTATUS
STDCALL
ZwQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
	);
// draft
// FIXME: Should I specify if the apc is user or kernel mode somewhere ??
/*
 * FUNCTION: Queues a (user) apc to a thread.
 * ARGUMENTS: 
	  ThreadHandle = Thread to which the apc is queued.
	  ApcRoutine = Points to the apc routine
	  NormalContext = Argument to Apc Routine
 *        SystemArgument1 = Argument of the Apc Routine
	  SystemArgument2 = Argument of the Apc Routine
 * REMARK: If the apc is queued against a thread of a different process than the calling thread
		the apc routine should be specified in the address space of the queued thread's process.
 * RETURNS: Status
*/

NTSTATUS
STDCALL
NtQueueApcThread(
	HANDLE ThreadHandle,
	PKNORMAL_ROUTINE ApcRoutine,
	PVOID NormalContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2);

NTSTATUS
STDCALL
ZwQueueApcThread(
	HANDLE ThreadHandle,
	PKNORMAL_ROUTINE ApcRoutine,
	PVOID NormalContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2);


/*
 * FUNCTION: Raises an exception
 * ARGUMENTS:
 *	  ExceptionRecord = Structure specifying the exception
 *	  Context = Context in which the excpetion is raised 
 *	  IsDebugger = 
 * RETURNS: Status
 *
*/

NTSTATUS
STDCALL
NtRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT Context,
	IN BOOLEAN SearchFrames
	);

NTSTATUS
STDCALL
ZwRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT Context,
	IN BOOLEAN SearchFrames
	);

/*
 * FUNCTION: Raises a hard error (stops the system)
 * ARGUMENTS:
 *	  Status = Status code of the hard error
 *	  Unknown2 = ??
 *	  Unknown3 = ??
 *	  Unknown4 = ??
 *	  Unknown5 = ??
 *	  Unknown6 = ??
 * RETURNS: Status
 *
 */

NTSTATUS
STDCALL
NtRaiseHardError(
	IN NTSTATUS Status,
	ULONG Unknown2,
	ULONG Unknown3,
	ULONG Unknown4,
	ULONG Unknown5,
	ULONG Unknown6
	);

NTSTATUS
STDCALL
ZwRaiseHardError(
	IN NTSTATUS Status,
	ULONG Unknown2,
	ULONG Unknown3,
	ULONG Unknown4,
	ULONG Unknown5,
	ULONG Unknown6
	);

/*
 * FUNCTION: Read a file
 * ARGUMENTS:
 *	  FileHandle = Handle of a file to read
 *	  Event = This event is signalled when the read operation completes
 *	  UserApcRoutine = Call back , if supplied Event should be NULL
 *	  UserApcContext = Argument to the callback
 *	  IoStatusBlock = Caller should supply storage for additional status information
 *	  Buffer = Caller should supply storage to receive the information
 *	  BufferLength = Size of the buffer
 *	  ByteOffset = Offset to start reading the file
 *	  Key = If a range is lock a matching key will allow the read to continue.
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

NTSTATUS
STDCALL
ZwReadFile(
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
	  Key =  Key = If a range is lock a matching key will allow the read to continue.
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
	IN FILE_SEGMENT_ELEMENT BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL	
	); 

NTSTATUS
STDCALL
ZwReadFileScatter( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN  PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
	IN FILE_SEGMENT_ELEMENT BufferDescription[], 
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
NTSTATUS
STDCALL
ZwReadVirtualMemory( 
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG  NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	); 	
	

/*
 * FUNCTION: Debugger can register for thread termination
 * ARGUMENTS: 
 *       TerminationPort = Port on which the debugger likes to be notified.
 * RETURNS: Status
 */
NTSTATUS
STDCALL	
NtRegisterThreadTerminatePort(
	HANDLE TerminationPort
	);
NTSTATUS
STDCALL	
ZwRegisterThreadTerminatePort(
	HANDLE TerminationPort
	);

/*
 * FUNCTION: Releases a mutant
 * ARGUMENTS: 
 *       MutantHandle = Handle to the mutant
 *       ReleaseCount = 
 * RETURNS: Status
 */
NTSTATUS
STDCALL	
NtReleaseMutant(
	IN HANDLE MutantHandle,
	IN PULONG ReleaseCount OPTIONAL
	);

NTSTATUS
STDCALL	
ZwReleaseMutant(
	IN HANDLE MutantHandle,
	IN PULONG ReleaseCount OPTIONAL
	);

/*
 * FUNCTION: Releases a semaphore
 * ARGUMENTS: 
 *       SemaphoreHandle = Handle to the semaphore object
 *       ReleaseCount = Number to decrease the semaphore count
 *       PreviousCount = Previous semaphore count
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtReleaseSemaphore(
	IN	HANDLE	SemaphoreHandle,
	IN	LONG	ReleaseCount,
	OUT	PLONG	PreviousCount
	);

NTSTATUS
STDCALL
ZwReleaseSemaphore(
	IN	HANDLE	SemaphoreHandle,
	IN	LONG	ReleaseCount,
	OUT	PLONG	PreviousCount
	);

/*
 * FUNCTION: Removes an io completion
 * ARGUMENTS:
 *        CompletionPort (OUT) = Caller supplied storage for the resulting handle
 *        CompletionKey = Requested access to the key
 *        IoStatusBlock = Caller provides storage for extended status information
 *        CompletionStatus = Current status of the io operation.
 *        WaitTime = Time to wait if ..
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtRemoveIoCompletion(
	IN HANDLE CompletionPort,
	OUT PULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PULONG CompletionStatus,
	IN PLARGE_INTEGER WaitTime 
	);

NTSTATUS
STDCALL
ZwRemoveIoCompletion(
	IN HANDLE CompletionPort,
	OUT PULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PULONG CompletionStatus,
	IN PLARGE_INTEGER WaitTime 
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
NTSTATUS
STDCALL
ZwReplaceKey(
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
NTSTATUS
STDCALL
ZwResetEvent(
	HANDLE EventHandle,
	PULONG NumberOfWaitingThreads OPTIONAL
	);
//draft
NTSTATUS
STDCALL
NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	);

NTSTATUS
STDCALL
ZwRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	);
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
	OUT PULONG SuspendCount
	);
NTSTATUS
STDCALL
ZwResumeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG SuspendCount
	);
/*
 * FUNCTION: Writes the content of a registry key to ascii file
 * ARGUMENTS: 
 *        KeyHandle = Handle to the key
 *        FileHandle =  Handle of the file
 * REMARKS:
	  This function maps to the Win32 RegSaveKey.
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	);
NTSTATUS
STDCALL
ZwSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	);

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
NTSTATUS
STDCALL
ZwSetContextThread(
	IN HANDLE ThreadHandle,
	IN PCONTEXT Context
	);

/*
 * FUNCTION: Sets the default locale id
 * ARGUMENTS:
 *        UserProfile = Type of locale id
 *              TRUE: thread locale id
 *              FALSE: system locale id
 *        DefaultLocaleId = Locale id
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetDefaultLocale(
	IN BOOLEAN UserProfile,
	IN LCID DefaultLocaleId
	);

NTSTATUS
STDCALL
ZwSetDefaultLocale(
	IN BOOLEAN UserProfile,
	IN LCID DefaultLocaleId
	);

/*
 * FUNCTION: Sets the default hard error port
 * ARGUMENTS:
 *        PortHandle = Handle to the port
 * NOTE: The hard error port is used for first change exception handling
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetDefaultHardErrorPort(
	IN HANDLE PortHandle
	);
NTSTATUS
STDCALL
ZwSetDefaultHardErrorPort(
	IN HANDLE PortHandle
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
NTSTATUS
STDCALL
ZwSetEaFile(
	IN HANDLE FileHandle,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	PVOID EaBuffer,
	ULONG EaBufferSize
	);

//FIXME: should I return the event state ?

/*
 * FUNCTION: Sets the  event to a signalled state.
 * ARGUMENTS: 
 *        EventHandle = Handle to the event
 *        NumberOfThreadsReleased =  The number of threads released
 * REMARK:
 *	  This procedure maps to the win32 SetEvent function. 
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtSetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased
	);

NTSTATUS
STDCALL
ZwSetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased
	);

/*
 * FUNCTION: Sets the high part of an event pair
 * ARGUMENTS: 
	EventPair = Handle to the event pair
 * RETURNS: Status
*/

NTSTATUS
STDCALL
NtSetHighEventPair(
	IN HANDLE EventPairHandle
	);

NTSTATUS
STDCALL
ZwSetHighEventPair(
	IN HANDLE EventPairHandle
	);
/*
 * FUNCTION: Sets the high part of an event pair and wait for the low part
 * ARGUMENTS: 
	EventPair = Handle to the event pair
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetHighWaitLowEventPair(
	IN HANDLE EventPairHandle
	);
NTSTATUS
STDCALL
ZwSetHighWaitLowEventPair(
	IN HANDLE EventPairHandle
	);

/*
 * FUNCTION: Sets the information of a file object.
 * ARGUMENTS: 
 *        FileHandle = Handle to the file object
 *	  IoStatusBlock = Caller supplies storage for extended information 
 *                        on the current operation.
 *        FileInformation = Storage for the new file information
 *        Lenght = Size of the new file information.
 *        FileInformationClass = Indicates to a certain information structure
	 
	  FileNameInformation 			FILE_NAME_INFORMATION
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
	IN	HANDLE			FileHandle,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	PVOID			FileInformation,
	IN	ULONG			Length,
	IN	FILE_INFORMATION_CLASS	FileInformationClass
	);
NTSTATUS
STDCALL
ZwSetInformationFile(
	IN	HANDLE			FileHandle,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	PVOID			FileInformation,
	IN	ULONG			Length,
	IN	FILE_INFORMATION_CLASS	FileInformationClass
	);



/*
 * FUNCTION: Sets the information of a registry key.
 * ARGUMENTS: 
 *       KeyHandle = Handle to the registry key
 *       KeyInformationClass =  Index to the a certain information structure.
			Can be one of the following values:

 *	 KeyWriteTimeInformation  KEY_WRITE_TIME_INFORMATION

	 KeyInformation	= Storage for the new information
 *       KeyInformationLength = Size of the information strucure
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

NTSTATUS
STDCALL
ZwSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	);
/*
 * FUNCTION: Changes a set of object specific parameters
 * ARGUMENTS: 
 *      ObjectHandle = 
 *	ObjectInformationClass = Index to the set of parameters to change. 

			
	ObjectBasicInformation  	
	ObjectTypeInformation 		OBJECT_TYPE_INFORMATION 
	ObjectAllInformation		
	ObjectDataInformation		OBJECT_DATA_INFORMATION
	ObjectNameInformation		OBJECT_NAME_INFORMATION	


 *      ObjectInformation = Caller supplies storage for parameters to set.
 *      Length = Size of the storage supplied
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetInformationObject(
	IN HANDLE ObjectHandle,
	IN CINT ObjectInformationClass,
	IN PVOID ObjectInformation,
	IN ULONG Length 
	);

NTSTATUS
STDCALL
ZwSetInformationObject(
	IN HANDLE ObjectHandle,
	IN CINT ObjectInformationClass,
	IN PVOID ObjectInformation,
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
 *	ProcessAffinityMask			KAFFINITY //??
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
NTSTATUS
STDCALL
ZwSetInformationProcess(
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
 *	ThreadPriority				KPRIORITY //???
 *	ThreadBasePriority			KPRIORITY
 *	ThreadAffinityMask			KAFFINITY //??
 *      ThreadImpersonationToken		ACCESS_TOKEN
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
	IN THREADINFOCLASS ThreadInformationClass,
	IN PVOID ThreadInformation,
	IN ULONG ThreadInformationLength
	);
NTSTATUS
STDCALL
ZwSetInformationThread(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	IN PVOID ThreadInformation,
	IN ULONG ThreadInformationLength
	);

/*
 * FUNCTION: Changes a set of token specific parameters
 * ARGUMENTS: 
 *      TokenHandle = Handle to the token
 *	TokenInformationClass = Index to a certain information structure. 
 *	Can be one of the following values:
 *
   		TokenUser 		TOKEN_USER 
    		TokenGroups		TOKEN_GROUPS
    		TokenPrivileges		TOKEN_PRIVILEGES
    		TokenOwner		TOKEN_OWNER
    		TokenPrimaryGroup	TOKEN_PRIMARY_GROUP
    		TokenDefaultDacl	TOKEN_DEFAULT_DACL
    		TokenSource		TOKEN_SOURCE
    		TokenType		TOKEN_TYPE
    		TokenImpersonationLevel	TOKEN_IMPERSONATION_LEVEL
    		TokenStatistics 	TOKEN_STATISTICS
 *
 *      TokenInformation = Caller supplies storage for information structure.
 *      TokenInformationLength = Size of the information structure
 * RETURNS: Status
*/

NTSTATUS
STDCALL
NtSetInformationToken(
	IN HANDLE TokenHandle,            
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,       
	IN ULONG TokenInformationLength   
	);

NTSTATUS
STDCALL
ZwSetInformationToken(
	IN HANDLE TokenHandle,            
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,       
	IN ULONG TokenInformationLength   
	);


/*
 * FUNCTION: Sets an io completion
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
NTSTATUS
STDCALL
ZwSetIoCompletion(
	IN HANDLE CompletionPort,
	IN ULONG CompletionKey,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG NumberOfBytesToTransfer, 
	OUT PULONG NumberOfBytesTransferred
	);

/*
 * FUNCTION: Set properties for profiling
 * ARGUMENTS: 
 *      Interval = 
 *	ClockSource = 
 * RETURNS: Status
 *
 */

NTSTATUS 
STDCALL
NtSetIntervalProfile(
	ULONG Interval,
	KPROFILE_SOURCE ClockSource
	);

NTSTATUS 
STDCALL
ZwSetIntervalProfile(
	ULONG Interval,
	KPROFILE_SOURCE ClockSource
	);


/*
 * FUNCTION: Sets the low part of an event pair
 * ARGUMENTS: 
	EventPair = Handle to the event pair
 * RETURNS: Status
*/

NTSTATUS
STDCALL
NtSetLowEventPair(
	HANDLE EventPair
	);
NTSTATUS
STDCALL
ZwSetLowEventPair(
	HANDLE EventPair
	);
/*
 * FUNCTION: Sets the low part of an event pair and wait for the high part
 * ARGUMENTS: 
	EventPair = Handle to the event pair
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetLowWaitHighEventPair(
	HANDLE EventPair
	);
NTSTATUS
STDCALL
ZwSetLowWaitHighEventPair(
	HANDLE EventPair
	);

NTSTATUS
STDCALL
NtSetSecurityObject(
	IN HANDLE Handle, 
	IN SECURITY_INFORMATION SecurityInformation, 
	IN PSECURITY_DESCRIPTOR SecurityDescriptor 
	); 

NTSTATUS
STDCALL
ZwSetSecurityObject(
	IN HANDLE Handle, 
	IN SECURITY_INFORMATION SecurityInformation, 
	IN PSECURITY_DESCRIPTOR SecurityDescriptor 
	); 


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
	IN PUNICODE_STRING VariableName,
	IN PUNICODE_STRING Value
	);
NTSTATUS
STDCALL
ZwSetSystemEnvironmentValue(
	IN PUNICODE_STRING VariableName,
	IN PUNICODE_STRING Value
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
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	IN	PVOID				SystemInformation,
	IN	ULONG				SystemInformationLength
	);

NTSTATUS
STDCALL
ZwSetSystemInformation(
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	IN	PVOID				SystemInformation,
	IN	ULONG				SystemInformationLength
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
NTSTATUS
STDCALL
ZwSetSystemTime(
	IN PLARGE_INTEGER SystemTime,
	IN PLARGE_INTEGER NewSystemTime OPTIONAL
	);
/*
 * FUNCTION: Sets the characteristics of a timer
 * ARGUMENTS: 
 *      TimerHandle = Handle to the timer
 *	DueTime = Time before the timer becomes signalled for the first time.
 *      TimerApcRoutine = Completion routine can be called on time completion
 *      TimerContext = Argument to the completion routine
 *      Resume = Specifies if the timer should repeated after completing one cycle
 *      Period = Cycle of the timer
 * REMARKS: This routine maps to the win32 SetWaitableTimer.
 * RETURNS: Status
*/
NTSTATUS
STDCALL
NtSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE TimerApcRoutine,
	IN PVOID TimerContext,
	IN BOOL WakeTimer,
	IN ULONG Period OPTIONAL,
	OUT PBOOLEAN PreviousState OPTIONAL
	);
NTSTATUS
STDCALL
ZwSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE TimerApcRoutine,
	IN PVOID TimerContext,
	IN BOOL WakeTimer,
	IN ULONG Period OPTIONAL,
	OUT PBOOLEAN PreviousState OPTIONAL
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
NTSTATUS
STDCALL
ZwSetTimerResolution(
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
 *		REG_BINARY			Unspecified binary data
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
NTSTATUS
STDCALL
ZwSetValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex OPTIONAL,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	);

/*
 * FUNCTION: Sets the volume information.
 * ARGUMENTS:
 *	FileHandle = Handle to the file
 *	IoStatusBlock = Caller should supply storage for additional status information
 *	VolumeInformation = pointer to a structure containing the new volume information
 *	Length = size of the structure.
 *	VolumeInformationClass = specifies the particular volume information to set
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
	);

NTSTATUS
STDCALL
ZwSetVolumeInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
	);

/*
 * FUNCTION: Shuts the system down
 * ARGUMENTS:
 *        Action = Specifies the type of shutdown, it can be one of the following values:
 *              ShutdownNoReboot, ShutdownReboot, ShutdownPowerOff
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtShutdownSystem(
	IN SHUTDOWN_ACTION Action
	);

NTSTATUS
STDCALL
ZwShutdownSystem(
	IN SHUTDOWN_ACTION Action
	);


/* --- PROFILING --- */

/*
 * FUNCTION: Starts profiling
 * ARGUMENTS: 
 *       ProfileHandle = Handle to the profile
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtStartProfile(
	HANDLE ProfileHandle
	);

NTSTATUS
STDCALL
ZwStartProfile(
	HANDLE ProfileHandle
	);

/*
 * FUNCTION: Stops profiling
 * ARGUMENTS: 
 *       ProfileHandle = Handle to the profile
 * RETURNS: Status    
 */

NTSTATUS
STDCALL
NtStopProfile(
	HANDLE ProfileHandle
	);

NTSTATUS
STDCALL
ZwStopProfile(
	HANDLE ProfileHandle
	);

/* --- PROCESS MANAGEMENT --- */

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
NTSTATUS 
STDCALL 
ZwTerminateProcess(
	IN HANDLE ProcessHandle ,
	IN NTSTATUS ExitStatus
	);

/* --- DEVICE DRIVER CONTROL --- */

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
NTSTATUS 
STDCALL
ZwUnloadDriver(
	IN PUNICODE_STRING DriverServiceName
	);

/* --- VIRTUAL MEMORY MANAGEMENT --- */

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

NTSTATUS
STDCALL 
ZwWriteVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID  BaseAddress,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
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
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PAGE_WAS_ULOCKED ]
 */	
NTSTATUS 
STDCALL
NtUnlockVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG  NumberOfBytesToUnlock,
	OUT PULONG NumberOfBytesUnlocked OPTIONAL
	);

NTSTATUS 
STDCALL
ZwUnlockVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG  NumberOfBytesToUnlock,
	OUT PULONG NumberOfBytesUnlocked OPTIONAL
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
NTSTATUS
STDCALL
ZwUnmapViewOfSection(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress
	);

/* --- OBJECT SYNCHRONIZATION --- */

/*
 * FUNCTION: Signals an object and wait for an other one.
 * ARGUMENTS: 
 *        SignalObject = Handle to the object that should be signaled
 *        WaitObject = Handle to the object that should be waited for
 *        Alertable = True if the wait is alertable
 *        Time = The time to wait
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject(
	IN	HANDLE		SignalObject,
	IN	HANDLE		WaitObject,
	IN	BOOLEAN		Alertable,
	IN	PLARGE_INTEGER	Time
	);

NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject(
	IN	HANDLE		SignalObject,
	IN	HANDLE		WaitObject,
	IN	BOOLEAN		Alertable,
	IN	PLARGE_INTEGER	Time
	);

/*
 * FUNCTION: Waits for multiple objects to become signalled.
 * ARGUMENTS: 
 *       Count = The number of objects
 *       Object = The array of object handles
 *       WaitType = Can be one of the values UserMode or KernelMode
 *       Alertable = If true the wait is alertable.
 *       Time = The maximum wait time.
 * REMARKS:
 *       This function maps to the win32 WaitForMultipleObjectEx.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtWaitForMultipleObjects (
	IN ULONG Count,
	IN HANDLE Object[],
	IN CINT WaitType,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time
	);

NTSTATUS
STDCALL
ZwWaitForMultipleObjects (
	IN ULONG Count,
	IN HANDLE Object[],
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
 *       This function maps to the win32 WaitForSingleObjectEx.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtWaitForSingleObject (
	IN HANDLE Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time
	);

NTSTATUS
STDCALL
ZwWaitForSingleObject (
	IN HANDLE Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time
	);

/* --- EVENT PAIR OBJECT --- */

/*
 * FUNCTION: Waits for the high part of an eventpair to become signalled
 * ARGUMENTS:
 *       EventPairHandle = Handle to the event pair.
 * RETURNS: Status
 */

NTSTATUS
STDCALL
NtWaitHighEventPair(
	IN HANDLE EventPairHandle
	);

NTSTATUS
STDCALL
ZwWaitHighEventPair(
	IN HANDLE EventPairHandle
	);

/*
 * FUNCTION: Waits for the low part of an eventpair to become signalled
 * ARGUMENTS:
 *       EventPairHandle = Handle to the event pair.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtWaitLowEventPair(
	IN HANDLE EventPairHandle
	);

NTSTATUS
STDCALL
ZwWaitLowEventPair(
	IN HANDLE EventPairHandle
	);

/* --- FILE MANAGEMENT --- */

/*
 * FUNCTION: Unlocks a range of bytes in a file. 
 * ARGUMENTS: 
 *       FileHandle = Handle to the file
 *       IoStatusBlock = Caller should supply storage for a structure containing
 *			 the completion status and information about the requested unlock operation.
			The information field is set to the number of bytes unlocked.
 *       ByteOffset = Offset to start the range of bytes to unlock 
 *       Length = Number of bytes to unlock.
 *       Key = Special value to enable other threads to unlock a file than the
		thread that locked the file. The key supplied must match with the one obtained
		in a previous call to NtLockFile.
 * REMARK:
	This procedure maps to the win32 procedure UnlockFileEx. STATUS_PENDING is returned if the lock could
	not be obtained immediately, the device queue is busy and the IRP is queued.
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PENDING | STATUS_ACCESS_DENIED | STATUS_INSUFFICIENT_RESOURCES |
	STATUS_INVALID_PARAMETER | STATUS_INVALID_DEVICE_REQUEST | STATUS_RANGE_NOT_LOCKED ]
 */	
NTSTATUS 
STDCALL
NtUnlockFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER ByteOffset,
	IN PLARGE_INTEGER Lenght,
	OUT PULONG Key OPTIONAL
	);
NTSTATUS 
STDCALL
ZwUnlockFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER ByteOffset,
	IN PLARGE_INTEGER Lenght,
	OUT PULONG Key OPTIONAL
	);
	
/*
 * FUNCTION: Writes data to a file
 * ARGUMENTS: 
 *       FileHandle = The handle a file ( from NtCreateFile )
 *       Event  = Specifies a event that will become signalled when the write operation completes.
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
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PENDING | STATUS_ACCESS_DENIED | STATUS_INSUFFICIENT_RESOURCES
	STATUS_INVALID_PARAMETER | STATUS_INVALID_DEVICE_REQUEST | STATUS_FILE_LOCK_CONFLICT ]
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
	IN PLARGE_INTEGER ByteOffset,
	IN PULONG Key OPTIONAL
    );

NTSTATUS
STDCALL
ZwWriteFile(
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
 *                    should be created by specifying FILE_USE_FILE_POINTER_POSITION. Use FILE_WRITE_TO_END_OF_FILE to write to the EOF.
 *       Key = If a matching key [ a key provided at NtLockFile ] is provided the write operation will continue even if a byte range is locked.
 * REMARKS:
 *	 This function maps to the win32 WriteFile. 
 *	 Callers to NtWriteFile should run at IRQL PASSIVE_LEVEL.
 * RETURNS: Status [ STATUS_SUCCESS | STATUS_PENDING | STATUS_ACCESS_DENIED | STATUS_INSUFFICIENT_RESOURCES
		STATUS_INVALID_PARAMETER | STATUS_INVALID_DEVICE_REQUEST | STATUS_FILE_LOCK_CONFLICT ]
 */

NTSTATUS
STDCALL 
NtWriteFileGather( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN FILE_SEGMENT_ELEMENT BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL
	); 

NTSTATUS
STDCALL 
ZwWriteFileGather( 
	IN HANDLE FileHandle, 
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN FILE_SEGMENT_ELEMENT BufferDescription[], 
	IN ULONG BufferLength, 
	IN PLARGE_INTEGER ByteOffset, 
	IN PULONG Key OPTIONAL
	); 


/* --- THREAD MANAGEMENT --- */

/*
 * FUNCTION: Increments a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
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

NTSTATUS 
STDCALL 
ZwSuspendThread(
	IN HANDLE ThreadHandle,
	IN PULONG PreviousSuspendCount 
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
NTSTATUS 
STDCALL 
ZwTerminateThread(
	IN HANDLE ThreadHandle ,
	IN NTSTATUS ExitStatus
	);
/*
 * FUNCTION: Tests to see if there are any pending alerts for the calling thread 
 * RETURNS: Status
 */	
NTSTATUS 
STDCALL 
NtTestAlert(
	VOID 
	);
NTSTATUS 
STDCALL 
ZwTestAlert(
	VOID 
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

NTSTATUS
STDCALL 
ZwYieldExecution(
	VOID
	);


/*
 * --- Local Procedure Call Facility
 * These prototypes are unknown as yet
 * (stack sizes by Peter-Michael Hager)
 */

/* --- REGISTRY --- */

/*
 * FUNCTION: Unloads a registry key.
 * ARGUMENTS:
 *       KeyHandle = Handle to the registry key
 * REMARK:
 *       This procedure maps to the win32 procedure RegUnloadKey
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtUnloadKey(
	HANDLE KeyHandle
	);
NTSTATUS
STDCALL
ZwUnloadKey(
	HANDLE KeyHandle
	);


/* --- PLUG AND PLAY --- */

NTSTATUS
STDCALL
NtPlugPlayControl (
	VOID
	);

NTSTATUS
STDCALL
NtGetPlugPlayEvent (
	VOID
	);

/* --- POWER MANAGEMENT --- */

NTSTATUS STDCALL 
NtSetSystemPowerState(IN POWER_ACTION SystemAction,
		      IN SYSTEM_POWER_STATE MinSystemState,
		      IN ULONG Flags);

/* --- DEBUG SUBSYSTEM --- */

NTSTATUS STDCALL 
NtSystemDebugControl(DEBUG_CONTROL_CODE ControlCode,
		     PVOID InputBuffer,
		     ULONG InputBufferLength,
		     PVOID OutputBuffer,
		     ULONG OutputBufferLength,
		     PULONG ReturnLength);

/* --- VIRTUAL DOS MACHINE (VDM) --- */

NTSTATUS
STDCALL
NtVdmControl (ULONG ControlCode, PVOID ControlData);


/* --- WIN32 --- */

NTSTATUS STDCALL
NtW32Call(IN ULONG RoutineIndex,
	  IN PVOID Argument,
	  IN ULONG ArgumentLength,
	  OUT PVOID* Result OPTIONAL,
	  OUT PULONG ResultLength OPTIONAL);

/* --- CHANNELS --- */

NTSTATUS
STDCALL
NtCreateChannel (
	VOID
	);

NTSTATUS
STDCALL
NtListenChannel (
	VOID
	);

NTSTATUS
STDCALL
NtOpenChannel (
	VOID
	);

NTSTATUS
STDCALL
NtReplyWaitSendChannel (
	VOID
	);

NTSTATUS
STDCALL
NtSendWaitReplyChannel (
	VOID
	);

NTSTATUS
STDCALL
NtSetContextChannel (
	VOID
	);

/* --- MISCELLANEA --- */

//NTSTATUS STDCALL NtSetLdtEntries(VOID);
NTSTATUS
STDCALL
NtSetLdtEntries (
	HANDLE	Thread,
	ULONG		FirstEntry,
	PULONG	Entries
	);


NTSTATUS
STDCALL
NtQueryOleDirectoryFile (
	VOID
	);

#endif /* __DDK_ZW_H */
