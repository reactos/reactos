/* $Id: uuid.c,v 1.2 2004/12/18 13:27:58 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           UUID generator
 * FILE:              kernel/ex/uuid.c
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define SEED_BUFFER_SIZE 6

/* Number of 100ns ticks per clock tick. To be safe, assume that the clock
   resolution is at least 1000 * 100 * (1/1000000) = 1/10 of a second */
#define TICKS_PER_CLOCK_TICK 1000
#define SECSPERDAY  86400
#define TICKSPERSEC 10000000

/* UUID system time starts at October 15, 1582 */
#define SECS_15_OCT_1582_TO_1601  ((17 + 30 + 31 + 365 * 18 + 5) * SECSPERDAY)
#define TICKS_15_OCT_1582_TO_1601 ((ULONGLONG)SECS_15_OCT_1582_TO_1601 * TICKSPERSEC)


/* GLOBALS ****************************************************************/

static FAST_MUTEX UuidMutex;
static LARGE_INTEGER UuidLastTime;
static ULONG UuidSequence;
static BOOLEAN UuidSequenceInitialized = FALSE;
static BOOLEAN UuidSequenceChanged = FALSE;
static UCHAR UuidSeed[SEED_BUFFER_SIZE];
static ULONG UuidCount;



/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
ExpInitUuids(VOID)
{
  ExInitializeFastMutex(&UuidMutex);

  KeQuerySystemTime((PLARGE_INTEGER)&UuidLastTime);
  UuidLastTime.QuadPart += TICKS_15_OCT_1582_TO_1601;

  UuidCount = TICKS_PER_CLOCK_TICK;
  RtlZeroMemory(UuidSeed, SEED_BUFFER_SIZE);
}


static BOOLEAN
ExpLoadUuidSequenceCount(PULONG Sequence)
{
  /* FIXME */
  *Sequence = 0x01234567;
  return TRUE;
}


static VOID
ExpSaveUuidSequenceCount(ULONG Sequence)
{
  /* FIXME */
}


static VOID
ExpGetRandomUuidSequenceCount(PULONG Sequence)
{
  /* FIXME */
  *Sequence = 0x76543210;
}


static NTSTATUS
ExpCreateUuids(PULARGE_INTEGER Time,
	       PULONG Range,
	       PULONG Sequence)
{
  /*
   * Generate time element of the UUID. Account for going faster
   * than our clock as well as the clock going backwards.
   */
  while (1)
  {
    KeQuerySystemTime((PLARGE_INTEGER)Time);
    Time->QuadPart += TICKS_15_OCT_1582_TO_1601;

    if (Time->QuadPart > UuidLastTime.QuadPart)
    {
      UuidCount = 0;
      break;
    }

    if (Time->QuadPart < UuidLastTime.QuadPart)
    {
//      *Sequence = (*Sequence + 1) & 0x1fff;
      (*Sequence)++;
      UuidSequenceChanged = TRUE;
      UuidCount = 0;
      break;
    }

    if (UuidCount < TICKS_PER_CLOCK_TICK)
    {
      UuidCount++;
      break;
    }
  }

  UuidLastTime.QuadPart = Time->QuadPart;
  Time->QuadPart += UuidCount;

  *Range = 10000; /* What does this mean? Ticks per millisecond?*/

  return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtAllocateUuids(OUT PULARGE_INTEGER Time,
		OUT PULONG Range,
		OUT PULONG Sequence,
		OUT PUCHAR Seed)
{
  ULARGE_INTEGER IntTime;
  ULONG IntRange;
  NTSTATUS Status;

  ExAcquireFastMutex(&UuidMutex);

  if (!UuidSequenceInitialized)
  {
    if (!ExpLoadUuidSequenceCount(&UuidSequence))
    {
      ExpGetRandomUuidSequenceCount(&UuidSequence);
    }

    UuidSequenceInitialized = TRUE;
    UuidSequenceChanged = TRUE;
  }

  Status = ExpCreateUuids(&IntTime,
                          &IntRange,
                          &UuidSequence);
  if (!NT_SUCCESS(Status))
  {
    ExReleaseFastMutex(&UuidMutex);
    return Status;
  }

  if (UuidSequenceChanged)
  {
    ExpSaveUuidSequenceCount(UuidSequence);
    UuidSequenceChanged = FALSE;
  }

  ExReleaseFastMutex(&UuidMutex);

  Time->QuadPart = IntTime.QuadPart;
  *Range = IntRange;
  *Sequence = UuidSequence;

  RtlCopyMemory(Seed,
                UuidSeed,
                SEED_BUFFER_SIZE);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetUuidSeed(IN PUCHAR Seed)
{
  RtlCopyMemory(UuidSeed,
                Seed,
                SEED_BUFFER_SIZE);
  return STATUS_SUCCESS;
}

/* EOF */
