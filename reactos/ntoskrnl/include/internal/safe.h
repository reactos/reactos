#ifndef __NTOSKRNL_INCLUDE_INTERNAL_SAFE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_SAFE_H

NTSTATUS MmSafeCopyFromUser(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);
NTSTATUS MmSafeCopyToUser(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);

NTSTATUS STDCALL
MmCopyFromCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);
NTSTATUS STDCALL
MmCopyToCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);

NTSTATUS
RtlCaptureUnicodeString(OUT PUNICODE_STRING Dest,
	                IN KPROCESSOR_MODE CurrentMode,
	                IN POOL_TYPE PoolType,
	                IN BOOLEAN CaptureIfKernel,
			IN PUNICODE_STRING UnsafeSrc);

VOID
RtlRelaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
	                       IN KPROCESSOR_MODE CurrentMode,
	                       IN BOOLEAN CaptureIfKernel);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_SAFE_Hb */
