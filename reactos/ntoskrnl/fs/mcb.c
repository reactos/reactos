/* $Id: mcb.c,v 1.9 2003/04/27 16:25:25 ea Exp $
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

/* FsRtlAddMcbEntry: Obsolete */
BOOLEAN STDCALL
FsRtlAddMcbEntry (IN PMCB     Mcb,
		  IN VBN      Vbn,
		  IN LBN      Lbn,
		  IN ULONG    SectorCount)
{
  return FsRtlAddLargeMcbEntry(& Mcb->LargeMcb,
			       (LONGLONG) Vbn,
			       (LONGLONG) Lbn,
			       (LONGLONG) SectorCount);
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


BOOLEAN STDCALL
FsRtlGetNextMcbEntry (IN PMCB     Mcb,
		      IN ULONG    RunIndex,
		      OUT PVBN    Vbn,
		      OUT PLBN    Lbn,
		      OUT PULONG  SectorCount)
{
  BOOLEAN  rc=FALSE;
  LONGLONG llVbn;
  LONGLONG llLbn;
  LONGLONG llSectorCount;

  // FIXME: how should conversion be done
  // FIXME: between 32 and 64 bits?
  rc=FsRtlGetNextLargeMcbEntry (& Mcb->LargeMcb,
				RunIndex,
				& llVbn,
				& llLbn,
				& llSectorCount);
  return(rc);
}


VOID STDCALL
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
			IN POOL_TYPE PoolType)
{
  UNIMPLEMENTED
  Mcb->PoolType = PoolType;
}

/* FsRtlInitializeMcb: Obsolete */
VOID STDCALL
FsRtlInitializeMcb (IN PMCB         Mcb,
		    IN POOL_TYPE    PoolType)
{
  FsRtlInitializeLargeMcb(& Mcb->LargeMcb, PoolType);
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


BOOLEAN STDCALL
FsRtlLookupLastMcbEntry (IN PMCB     Mcb,
			 OUT PVBN    Vbn,
			 OUT PLBN    Lbn)
{
  UNIMPLEMENTED
  return(FALSE);
}


BOOLEAN STDCALL
FsRtlLookupMcbEntry (IN PMCB     Mcb,
		     IN VBN      Vbn,
		     OUT PLBN    Lbn,
		     OUT PULONG  SectorCount OPTIONAL,
		     OUT PULONG  Index)
{
  UNIMPLEMENTED
  return(FALSE);
}


ULONG STDCALL
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
  ULONG NumberOfRuns;
  ExAcquireFastMutex (Mcb->FastMutex);
  NumberOfRuns=Mcb->PairCount;
  ExReleaseFastMutex (Mcb->FastMutex);
  return(NumberOfRuns);
}


/* FsRtlNumberOfRunsInMcb: Obsolete */
ULONG STDCALL
FsRtlNumberOfRunsInMcb (IN PMCB Mcb)
{
  return FsRtlNumberOfRunsInLargeMcb(& Mcb->LargeMcb);
}


VOID STDCALL
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 IN LONGLONG SectorCount)
{
  UNIMPLEMENTED
}


VOID STDCALL
FsRtlRemoveMcbEntry (IN PMCB     Mcb,
		     IN VBN      Vbn,
		     IN ULONG    SectorCount)
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


/* FsRtlTruncateMcb: Obsolete */
VOID STDCALL
FsRtlTruncateMcb (IN PMCB Mcb,
		  IN VBN  Vbn)
{
  FsRtlTruncateLargeMcb (& Mcb->LargeMcb, (LONGLONG) Vbn);
}


VOID STDCALL
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
  UNIMPLEMENTED;
}

/* FsRtlUninitializeMcb: Obsolete */
VOID STDCALL
FsRtlUninitializeMcb (IN PMCB Mcb)
{
  FsRtlUninitializeLargeMcb(& Mcb->LargeMcb);
}


/* EOF */

