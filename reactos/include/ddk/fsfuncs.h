#ifndef __INCLUDE_DDK_FSFUNCS_H
#define __INCLUDE_DDK_FSFUNCS_H
VOID
STDCALL
FsRtlAddLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	);
VOID
STDCALL
FsRtlAddMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
VOID
STDCALL
FsRtlGetNextLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlGetNextMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlInitializeLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
FsRtlInitializeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	NTSTATUS	NtStatus
	);
BOOLEAN
STDCALL
FsRtlIsTotalDeviceFailure (
	NTSTATUS	NtStatus
	);
VOID
STDCALL
FsRtlLookupLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7
	);
VOID
STDCALL
FsRtlLookupLastLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlLookupLastMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlLookupMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus (
	NTSTATUS	NtStatusToNormalize,
	NTSTATUS	NormalizedNtStatus
	);
VOID
STDCALL
FsRtlNumberOfRunsInLargeMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlNumberOfRunsInMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlRemoveLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlRemoveMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlSplitLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlTruncateLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlTruncateMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
FsRtlUninitializeLargeMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlUninitializeMcb (
	DWORD	Unknown0
	);

#endif /* __INCLUDE_DDK_FSFUNCS_H */
