/* $Id: rtl.h,v 1.4 1999/12/01 15:16:56 ekohl Exp $
 *
 */

VOID
WINAPI
__RtlInitHeap (
	PVOID	base,
	ULONG	minsize,
	ULONG	maxsize
	);

#define HEAP_BASE (0xa0000000)

VOID
RtlDeleteCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);
VOID
RtlEnterCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);
VOID
RtlInitializeCriticalSection (
	LPCRITICAL_SECTION	pcritical
	);
VOID
RtlLeaveCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);
WINBOOL
RtlTryEntryCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);
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
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
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


VOID
STDCALL
RtlDeNormalizeProcessParams (
	IN OUT	PSTARTUP_ARGUMENT	pArgument
	);

VOID
STDCALL
RtlDestroyProcessParameters (
	IN OUT	PSTARTUP_ARGUMENT	pArgument
	);

VOID
STDCALL
RtlNormalizeProcessParams (
	IN OUT	PSTARTUP_ARGUMENT	pArgument
	);


/* Preliminary prototype!! */

NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING		ApplicationName,
	PSECURITY_DESCRIPTOR	ProcessSd,
	PSECURITY_DESCRIPTOR	ThreadSd,
	WINBOOL			bInheritHandles,
	DWORD			dwCreationFlags,
	PCLIENT_ID		ClientId,
	PHANDLE			ProcessHandle,
	PHANDLE			ThreadHandle
	);


/* EOF */