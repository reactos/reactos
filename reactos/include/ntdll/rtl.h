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
