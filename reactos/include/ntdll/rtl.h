/* $Id: rtl.h,v 1.11 2000/02/25 23:57:21 ekohl Exp $
 *
 */


/*
 * Preliminary data type!!
 *
 * This definition is not finished yet. It will change in the future.
 */
typedef struct _RTL_USER_PROCESS_INFO
{
	ULONG		Unknown1;		// 0x00
	HANDLE		ProcessHandle;		// 0x04
	HANDLE		ThreadHandle;		// 0x08
	CLIENT_ID	ClientId;		// 0x0C
	ULONG		Unknown5;		// 0x14
	LONG		StackZeroBits;		// 0x18
	LONG		StackReserved;		// 0x1C
	LONG		StackCommit;		// 0x20
	ULONG		Unknown9;		// 0x24
// more data ... ???
} RTL_USER_PROCESS_INFO, *PRTL_USER_PROCESS_INFO;


//VOID WINAPI __RtlInitHeap(PVOID	base,
//			  ULONG	minsize,
//			  ULONG	maxsize);

#define HEAP_BASE (0xa0000000)

VOID RtlDeleteCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
VOID RtlEnterCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
VOID RtlInitializeCriticalSection (LPCRITICAL_SECTION	pcritical);
VOID RtlLeaveCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
WINBOOL RtlTryEntryCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);

UINT
STDCALL
RtlCompactHeap (
	HANDLE	heap,
	DWORD	flags
	);

VOID
STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
	);

/* Path functions */

ULONG
STDCALL
RtlDetermineDosPathNameType_U (
	PWSTR Path
	);

BOOLEAN
STDCALL
RtlDoesFileExists_U (
	PWSTR FileName
	);

BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U (
	PWSTR		dosname,
	PUNICODE_STRING	ntname,
	PWSTR		*shortname,
	PCURDIR		nah
	);

ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetCurrentDirectory_U (
	ULONG MaximumLength,
	PWSTR Buffer
	);

ULONG
STDCALL
RtlGetFullPathName_U (
	WCHAR *dosname,
	ULONG size,
	WCHAR *buf,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetLongestNtPathLength (
	VOID
	);

ULONG
STDCALL
RtlIsDosDeviceName_U (
	PWSTR DeviceName
	);

NTSTATUS
STDCALL
RtlSetCurrentDirectory_U (
	PUNICODE_STRING name
	);

/* Environment functions */
VOID
STDCALL
RtlAcquirePebLock (
	VOID
	);

VOID
STDCALL
RtlReleasePebLock (
	VOID
	);

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Inherit,
	PVOID	*Environment
	);

VOID
STDCALL
RtlDestroyEnvironment (
	PVOID	Environment
	);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PVOID		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

VOID
STDCALL
RtlSetCurrentEnvironment (
	PVOID	NewEnvironment,
	PVOID	*OldEnvironment
	);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

NTSTATUS
STDCALL
RtlCreateUserThread (
	IN	HANDLE			ProcessHandle,
	IN	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	IN	BOOLEAN			CreateSuspended,
	IN	LONG			StackZeroBits,
	IN OUT	PULONG			StackReserved,
	IN OUT	PULONG			StackCommit,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN	PVOID			Parameter,
	IN OUT	PHANDLE			ThreadHandle,
	IN OUT	PCLIENT_ID		ClientId
	);

/*
 * Preliminary prototype!!
 *
 * This prototype is not finished yet. It will change in the future.
 */
NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING			CommandLine,
	ULONG				Unknown2,
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,	// verified
	PSECURITY_DESCRIPTOR		ProcessSd,
	PSECURITY_DESCRIPTOR		ThreadSd,
	WINBOOL				bInheritHandles,
	DWORD				dwCreationFlags,
	ULONG				Unknown8,
	ULONG				Unknown9,
	PRTL_USER_PROCESS_INFO		ProcessInfo		// verified
	);

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	IN	PUNICODE_STRING	CommandLine,
	IN	PUNICODE_STRING	DllPath,
	IN	PUNICODE_STRING	CurrentDirectory,
	IN	PUNICODE_STRING	ImagePathName,
	IN	PVOID		Environment,
	IN	PUNICODE_STRING	WindowTitle,
	IN	PUNICODE_STRING	DesktopInfo,
	IN	PUNICODE_STRING	ShellInfo,
	IN	PUNICODE_STRING	RuntimeData
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

VOID
STDCALL
RtlDestroyProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

/* EOF */
