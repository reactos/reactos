/* $Id: mcb.c,v 1.10 2003/07/10 06:27:13 royce Exp $
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
 *
 * @unimplemented
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

/*
 * FsRtlAddMcbEntry: Obsolete
 *
 * @implemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
VOID STDCALL
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
			IN POOL_TYPE PoolType)
{
  UNIMPLEMENTED
  Mcb->PoolType = PoolType;
}

/*
 * FsRtlInitializeMcb: Obsolete
 * @implemented
 */
VOID STDCALL
FsRtlInitializeMcb (IN PMCB         Mcb,
		    IN POOL_TYPE    PoolType)
{
  FsRtlInitializeLargeMcb(& Mcb->LargeMcb, PoolType);
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOLEAN STDCALL
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
			     OUT PLONGLONG Vbn,
			     OUT PLONGLONG Lbn)
{
  UNIMPLEMENTED
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
FsRtlLookupLastMcbEntry (IN PMCB     Mcb,
			 OUT PVBN    Vbn,
			 OUT PLBN    Lbn)
{
  UNIMPLEMENTED
  return(FALSE);
}


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
ULONG STDCALL
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
  ULONG NumberOfRuns;
  ExAcquireFastMutex (Mcb->FastMutex);
  NumberOfRuns=Mcb->PairCount;
  ExReleaseFastMutex (Mcb->FastMutex);
  return(NumberOfRuns);
}


/*
 * FsRtlNumberOfRunsInMcb: Obsolete
 *
 * @implemented
 */
ULONG STDCALL
FsRtlNumberOfRunsInMcb (IN PMCB Mcb)
{
  return FsRtlNumberOfRunsInLargeMcb(& Mcb->LargeMcb);
}


/*
 * @unimplemented
 */
VOID STDCALL
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 IN LONGLONG SectorCount)
{
  UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID STDCALL
FsRtlRemoveMcbEntry (IN PMCB     Mcb,
		     IN VBN      Vbn,
		     IN ULONG    SectorCount)
{
  UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
		   IN LONGLONG Vbn,
		   IN LONGLONG Amount)
{
  UNIMPLEMENTED
  return(FALSE);
}


/*
 * @unimplemented
 */
VOID STDCALL
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
		      IN LONGLONG Vbn)
{
  UNIMPLEMENTED
}


/*
 * FsRtlTruncateMcb: Obsolete
 *
 * @implemented
 */
VOID STDCALL
FsRtlTruncateMcb (IN PMCB Mcb,
		  IN VBN  Vbn)
{
  FsRtlTruncateLargeMcb (& Mcb->LargeMcb, (LONGLONG) Vbn);
}


/*
 * @unimplemented
 */
VOID STDCALL
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
  UNIMPLEMENTED;
}

/*
 * FsRtlUninitializeMcb: Obsolete
 *
 * @implemented
 */
VOID STDCALL
FsRtlUninitializeMcb (IN PMCB Mcb)
{
  FsRtlUninitializeLargeMcb(& Mcb->LargeMcb);
}


/* EOF */

