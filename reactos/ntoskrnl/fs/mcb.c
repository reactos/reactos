/* $Id: mcb.c,v 1.5 2002/09/08 10:23:20 chorns Exp $
 *
 * reactos/ntoskrnl/fs/mcb.c
 *
 */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

#include <internal/debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTES
 */
BOOLEAN STDCALL
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
		      IN LONGLONG Vbn,
		      IN LONGLONG Lbn,
		      IN LONGLONG SectorCount)
{
  UNIMPLEMENTED
  return(FALSE);
}


VOID
STDCALL
FsRtlAddMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	UNIMPLEMENTED
}


BOOLEAN STDCALL
FsRtlGetNextLargeMcbEntry(IN PLARGE_MCB Mcb,
			  IN ULONG RunIndex,
			  OUT PLONGLONG Vbn,
			  OUT PLONGLONG Lbn,
			  OUT PLONGLONG SectorCount)
{
  UNIMPLEMENTED
  return(FALSE);
}


VOID
STDCALL
FsRtlGetNextMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	UNIMPLEMENTED
}


VOID STDCALL
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
			IN POOL_TYPE PoolType)
{
  UNIMPLEMENTED
	Mcb->PoolType = PoolType;
}


VOID
STDCALL
FsRtlInitializeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED
}


BOOLEAN STDCALL
FsRtlLookupLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 OUT PLONGLONG Lbn OPTIONAL,
			 OUT PLONGLONG SectorCountFromLbn OPTIONAL,
			 OUT PLONGLONG StartingLbn OPTIONAL,
			 OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
			 OUT PULONG Index OPTIONAL)
{
  UNIMPLEMENTED
  return(FALSE);
}


BOOLEAN STDCALL
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
			     OUT PLONGLONG Vbn,
			     OUT PLONGLONG Lbn)
{
  UNIMPLEMENTED
  return(FALSE);
}


VOID
STDCALL
FsRtlLookupLastMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED
}


VOID
STDCALL
FsRtlLookupMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	UNIMPLEMENTED
}


ULONG STDCALL
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
  UNIMPLEMENTED
  return(0);
}


VOID
STDCALL
FsRtlNumberOfRunsInMcb (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED
}


VOID STDCALL
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 IN LONGLONG SectorCount)
{
  UNIMPLEMENTED
}


VOID
STDCALL
FsRtlRemoveMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED
}


BOOLEAN STDCALL
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
		   IN LONGLONG Vbn,
		   IN LONGLONG Amount)
{
  UNIMPLEMENTED
  return(FALSE);
}


VOID STDCALL
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
		      IN LONGLONG Vbn)
{
  UNIMPLEMENTED
}


VOID
STDCALL
FsRtlTruncateMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED
}


VOID STDCALL
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
FsRtlUninitializeMcb (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED
}


/* EOF */

