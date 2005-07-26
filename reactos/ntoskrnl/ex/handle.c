/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/handle.c
 * PURPOSE:         Generic Executive Handle Tables
 *
 * PROGRAMMERS:     Thomas Weidenmueller <w3seek@reactos.com>
 *
 *  TODO:
 *
 *  - the last entry of a subhandle list should be reserved for auditing
 *
 *  ExSweepHandleTable (???)
 *  ExReferenceHandleDebugInfo
 *  ExSnapShotHandleTables
 *  ExpMoveFreeHandles (???)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

static LIST_ENTRY ExpHandleTableHead;
static FAST_MUTEX ExpHandleTableListLock;
static LARGE_INTEGER ExpHandleShortWait;

#define ExAcquireHandleTableListLock()                                         \
  ExAcquireFastMutexUnsafe(&ExpHandleTableListLock)

#define ExReleaseHandleTableListLock()                                         \
  ExReleaseFastMutexUnsafe(&ExpHandleTableListLock)

#define ExAcquireHandleTableLockExclusive(HandleTable)                         \
  ExAcquireResourceExclusiveLite(&(HandleTable)->HandleTableLock, TRUE)

#define ExAcquireHandleTableLockShared(HandleTable)                            \
  ExAcquireResourceSharedLite(&(HandleTable)->HandleTableLock, TRUE)

#define ExReleaseHandleTableLock(HandleTable)                                  \
  ExReleaseResourceLite(&(HandleTable)->HandleTableLock)

/*
   5 bits: reserved
   8 bits: top level index
   10 bits: middle level index
   9 bits: sub handle index
*/
#define N_TLI_BITS 8 /* top level index */
#define N_MLI_BITS 10 /* middle level index */
#define N_EI_BITS 9 /* sub handle index */
#define TLI_OFFSET (N_MLI_BITS + N_EI_BITS)
#define MLI_OFFSET N_EI_BITS
#define EI_OFFSET 0

#define N_TOPLEVEL_POINTERS (1 << N_TLI_BITS)
#define N_MIDDLELEVEL_POINTERS (1 << N_MLI_BITS)
#define N_SUBHANDLE_ENTRIES (1 << N_EI_BITS)
#define EX_MAX_HANDLES (N_TOPLEVEL_POINTERS * N_MIDDLELEVEL_POINTERS * N_SUBHANDLE_ENTRIES)

#define VALID_HANDLE_MASK (((N_TOPLEVEL_POINTERS - 1) << TLI_OFFSET) |         \
  ((N_MIDDLELEVEL_POINTERS - 1) << MLI_OFFSET) | ((N_SUBHANDLE_ENTRIES - 1) << EI_OFFSET))
#define TLI_FROM_HANDLE(index) (ULONG)(((index) >> TLI_OFFSET) & (N_TOPLEVEL_POINTERS - 1))
#define MLI_FROM_HANDLE(index) (ULONG)(((index) >> MLI_OFFSET) & (N_MIDDLELEVEL_POINTERS - 1))
#define ELI_FROM_HANDLE(index) (ULONG)(((index) >> EI_OFFSET) & (N_SUBHANDLE_ENTRIES - 1))

#define N_MAX_HANDLE (N_TOPLEVEL_POINTERS * N_MIDDLELEVEL_POINTERS * N_SUBHANDLE_ENTRIES)

#define BUILD_HANDLE(tli, mli, eli) ((((tli) & (N_TOPLEVEL_POINTERS - 1)) << TLI_OFFSET) | \
  (((mli) & (N_MIDDLELEVEL_POINTERS - 1)) << MLI_OFFSET) | (((eli) & (N_SUBHANDLE_ENTRIES - 1)) << EI_OFFSET))

#define IS_INVALID_EX_HANDLE(index)                                            \
  (((index) & ~VALID_HANDLE_MASK) != 0)
#define IS_VALID_EX_HANDLE(index)                                              \
  (((index) & ~VALID_HANDLE_MASK) == 0)

static BOOLEAN ExpInitialized = FALSE;

/******************************************************************************/

VOID
ExpInitializeHandleTables(VOID)
{
  ExpHandleShortWait.QuadPart = -50000;
  InitializeListHead(&ExpHandleTableHead);
  ExInitializeFastMutex(&ExpHandleTableListLock);

  ExpInitialized = TRUE;
}

PHANDLE_TABLE
ExCreateHandleTable(IN PEPROCESS QuotaProcess  OPTIONAL)
{
  PHANDLE_TABLE HandleTable;

  PAGED_CODE();

  if(!ExpInitialized)
  {
    KEBUGCHECK(0);
  }

  if(QuotaProcess != NULL)
  {
    /* FIXME - Charge process quota before allocating the handle table! */
  }

  /* allocate enough memory for the handle table and the lowest level */
  HandleTable = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(HANDLE_TABLE) + (N_TOPLEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY*)),
                                      TAG('E', 'x', 'H', 't'));
  if(HandleTable != NULL)
  {
    /* initialize the handle table */
    HandleTable->Flags = 0;
    HandleTable->HandleCount = 0;
    HandleTable->Table = (PHANDLE_TABLE_ENTRY**)(HandleTable + 1);
    HandleTable->QuotaProcess = QuotaProcess;
    HandleTable->FirstFreeTableEntry = -1; /* no entries freed so far */
    HandleTable->NextIndexNeedingPool = 0; /* no entries freed so far, so we have to allocate already for the first handle */
    HandleTable->UniqueProcessId = (QuotaProcess ? QuotaProcess->UniqueProcessId : NULL);

    ExInitializeResource(&HandleTable->HandleTableLock);

    KeInitializeEvent(&HandleTable->HandleContentionEvent,
                      NotificationEvent,
                      FALSE);

    RtlZeroMemory(HandleTable->Table, N_TOPLEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY*));

    /* during bootup KeGetCurrentThread() might be NULL, needs to be fixed... */
    if(KeGetCurrentThread() != NULL)
    {
      /* insert it into the global handle table list */
      KeEnterCriticalRegion();

      ExAcquireHandleTableListLock();
      InsertTailList(&ExpHandleTableHead,
                     &HandleTable->HandleTableList);
      ExReleaseHandleTableListLock();

      KeLeaveCriticalRegion();
    }
    else
    {
      InsertTailList(&ExpHandleTableHead,
                     &HandleTable->HandleTableList);
    }
  }
  else
  {
    /* FIXME - return the quota to the process */
  }

  return HandleTable;
}

static BOOLEAN
ExLockHandleTableEntryNoDestructionCheck(IN PHANDLE_TABLE HandleTable,
                                         IN PHANDLE_TABLE_ENTRY Entry)
{
  ULONG_PTR Current, New;

  PAGED_CODE();

  DPRINT("Entering handle table entry 0x%p lock...\n", Entry);

  ASSERT(HandleTable);
  ASSERT(Entry);

  for(;;)
  {
    Current = (volatile ULONG_PTR)Entry->u1.Object;

    if(!Current)
    {
      DPRINT("Attempted to lock empty handle table entry 0x%p or handle table shut down\n", Entry);
      break;
    }

    if(!(Current & EX_HANDLE_ENTRY_LOCKED))
    {
      New = Current | EX_HANDLE_ENTRY_LOCKED;
      if(InterlockedCompareExchangePointer(&Entry->u1.Object,
                                           (PVOID)New,
                                           (PVOID)Current) == (PVOID)Current)
      {
        DPRINT("SUCCESS handle table 0x%p entry 0x%p lock\n", HandleTable, Entry);
        /* we acquired the lock */
        return TRUE;
      }
    }

    /* wait about 5ms at maximum so we don't wait forever in unfortunate
       co-incidences where releasing the lock in another thread happens right
       before we're waiting on the contention event to get pulsed, which might
       never happen again... */
    KeWaitForSingleObject(&HandleTable->HandleContentionEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          &ExpHandleShortWait);
  }

  return FALSE;
}

VOID
ExDestroyHandleTable(IN PHANDLE_TABLE HandleTable,
                     IN PEX_DESTROY_HANDLE_CALLBACK DestroyHandleCallback  OPTIONAL,
                     IN PVOID Context  OPTIONAL)
{
  PHANDLE_TABLE_ENTRY **tlp, **lasttlp, *mlp, *lastmlp;
  PEPROCESS QuotaProcess;

  PAGED_CODE();

  ASSERT(HandleTable);

  KeEnterCriticalRegion();

  /* ensure there's no other operations going by acquiring an exclusive lock */
  ExAcquireHandleTableLockExclusive(HandleTable);

  ASSERT(!(HandleTable->Flags & EX_HANDLE_TABLE_CLOSING));

  HandleTable->Flags |= EX_HANDLE_TABLE_CLOSING;

  KePulseEvent(&HandleTable->HandleContentionEvent,
               EVENT_INCREMENT,
               FALSE);

  /* remove the handle table from the global handle table list */
  ExAcquireHandleTableListLock();
  RemoveEntryList(&HandleTable->HandleTableList);
  ExReleaseHandleTableListLock();

  /* call the callback function to cleanup the objects associated with the
     handle table */
  if(DestroyHandleCallback != NULL)
  {
    for(tlp = HandleTable->Table, lasttlp = HandleTable->Table + N_TOPLEVEL_POINTERS;
        tlp != lasttlp;
        tlp++)
    {
      if((*tlp) != NULL)
      {
        for(mlp = *tlp, lastmlp = (*tlp) + N_MIDDLELEVEL_POINTERS;
            mlp != lastmlp;
            mlp++)
        {
          if((*mlp) != NULL)
          {
            PHANDLE_TABLE_ENTRY curee, laste;

            for(curee = *mlp, laste = *mlp + N_SUBHANDLE_ENTRIES;
                curee != laste;
                curee++)
            {
              if(curee->u1.Object != NULL && ExLockHandleTableEntryNoDestructionCheck(HandleTable, curee))
              {
                DestroyHandleCallback(HandleTable, curee->u1.Object, curee->u2.GrantedAccess, Context);
                ExUnlockHandleTableEntry(HandleTable, curee);
              }
            }
          }
        }
      }
    }
  }

  QuotaProcess = HandleTable->QuotaProcess;

  /* free the tables */
  for(tlp = HandleTable->Table, lasttlp = HandleTable->Table + N_TOPLEVEL_POINTERS;
      tlp != lasttlp;
      tlp++)
  {
    if((*tlp) != NULL)
    {
      for(mlp = *tlp, lastmlp = (*tlp) + N_MIDDLELEVEL_POINTERS;
          mlp != lastmlp;
          mlp++)
      {
        if((*mlp) != NULL)
        {
          ExFreePool(*mlp);

          if(QuotaProcess != NULL)
          {
            /* FIXME - return the quota to the process */
          }
        }
      }

      ExFreePool(*tlp);

      if(QuotaProcess != NULL)
      {
        /* FIXME - return the quota to the process */
      }
    }
  }

  ExReleaseHandleTableLock(HandleTable);

  KeLeaveCriticalRegion();

  /* free the handle table */
  ExDeleteResource(&HandleTable->HandleTableLock);
  ExFreePool(HandleTable);

  if(QuotaProcess != NULL)
  {
    /* FIXME - return the quota to the process */
  }
}

PHANDLE_TABLE
ExDupHandleTable(IN PEPROCESS QuotaProcess  OPTIONAL,
                 IN PEX_DUPLICATE_HANDLE_CALLBACK DuplicateHandleCallback  OPTIONAL,
                 IN PVOID Context  OPTIONAL,
                 IN PHANDLE_TABLE SourceHandleTable)
{
  PHANDLE_TABLE HandleTable;

  PAGED_CODE();

  ASSERT(SourceHandleTable);

  HandleTable = ExCreateHandleTable(QuotaProcess);
  if(HandleTable != NULL)
  {
    PHANDLE_TABLE_ENTRY **tlp, **srctlp, **etlp, *mlp, *srcmlp, *emlp, stbl, srcstbl, estbl;
    LONG tli, mli, eli;

    tli = mli = eli = 0;

    /* make sure the other handle table isn't being changed during the duplication */
    ExAcquireHandleTableLockShared(SourceHandleTable);

    /* allocate enough tables */
    etlp = SourceHandleTable->Table + N_TOPLEVEL_POINTERS;
    for(srctlp = SourceHandleTable->Table, tlp = HandleTable->Table;
        srctlp != etlp;
        srctlp++, tlp++)
    {
      if(*srctlp != NULL)
      {
        /* allocate middle level entry tables */
        if(QuotaProcess != NULL)
        {
          /* FIXME - Charge process quota before allocating the handle table! */
        }

        *tlp = ExAllocatePoolWithTag(PagedPool,
                                     N_MIDDLELEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY),
                                     TAG('E', 'x', 'H', 't'));
        if(*tlp != NULL)
        {
          RtlZeroMemory(*tlp, N_MIDDLELEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY));

          KeMemoryBarrier();

          emlp = *srctlp + N_MIDDLELEVEL_POINTERS;
          for(srcmlp = *srctlp, mlp = *tlp;
              srcmlp != emlp;
              srcmlp++, mlp++)
          {
            if(*srcmlp != NULL)
            {
              /* allocate subhandle tables */
              if(QuotaProcess != NULL)
              {
                /* FIXME - Charge process quota before allocating the handle table! */
              }

              *mlp = ExAllocatePoolWithTag(PagedPool,
                                           N_SUBHANDLE_ENTRIES * sizeof(HANDLE_TABLE_ENTRY),
                                           TAG('E', 'x', 'H', 't'));
              if(*mlp != NULL)
              {
                RtlZeroMemory(*mlp, N_SUBHANDLE_ENTRIES * sizeof(HANDLE_TABLE_ENTRY));
              }
              else
              {
                goto freehandletable;
              }
            }
            else
            {
              *mlp = NULL;
            }
          }
        }
        else
        {
freehandletable:
          DPRINT1("Failed to duplicate handle table 0x%p\n", SourceHandleTable);

          ExReleaseHandleTableLock(SourceHandleTable);

          ExDestroyHandleTable(HandleTable,
                               NULL,
                               NULL);
          /* allocate an empty handle table */
          return ExCreateHandleTable(QuotaProcess);
        }
      }
    }

    /* duplicate the handles */
    HandleTable->HandleCount = SourceHandleTable->HandleCount;
    HandleTable->FirstFreeTableEntry = SourceHandleTable->FirstFreeTableEntry;
    HandleTable->NextIndexNeedingPool = SourceHandleTable->NextIndexNeedingPool;

    /* make sure all tables are zeroed */
    KeMemoryBarrier();

    etlp = SourceHandleTable->Table + N_TOPLEVEL_POINTERS;
    for(srctlp = SourceHandleTable->Table, tlp = HandleTable->Table;
        srctlp != etlp;
        srctlp++, tlp++, tli++)
    {
      if(*srctlp != NULL)
      {
        ASSERT(*tlp != NULL);

        emlp = *srctlp + N_MIDDLELEVEL_POINTERS;
        for(srcmlp = *srctlp, mlp = *tlp;
            srcmlp != emlp;
            srcmlp++, mlp++, mli++)
        {
          if(*srcmlp != NULL)
          {
            ASSERT(*mlp != NULL);

            /* walk all handle entries and duplicate them if wanted */
            estbl = *srcmlp + N_SUBHANDLE_ENTRIES;
            for(srcstbl = *srcmlp, stbl = *mlp;
                srcstbl != estbl;
                srcstbl++, stbl++, eli++)
            {
              /* try to duplicate the source handle */
              if(srcstbl->u1.Object != NULL &&
                 ExLockHandleTableEntry(SourceHandleTable,
                                        srcstbl))
              {
                /* ask the caller if this handle should be duplicated */
                if(DuplicateHandleCallback != NULL &&
                   !DuplicateHandleCallback(HandleTable,
                                            srcstbl,
                                            Context))
                {
                  /* free the entry and chain it into the free list */
                  HandleTable->HandleCount--;
                  stbl->u1.Object = NULL;
                  stbl->u2.NextFreeTableEntry = HandleTable->FirstFreeTableEntry;
                  HandleTable->FirstFreeTableEntry = BUILD_HANDLE(tli, mli, eli);
                }
                else
                {
                  /* duplicate the handle and unlock it */
                  stbl->u2.GrantedAccess = srcstbl->u2.GrantedAccess;
                  stbl->u1.ObAttributes = srcstbl->u1.ObAttributes & ~EX_HANDLE_ENTRY_LOCKED;
                }
                ExUnlockHandleTableEntry(SourceHandleTable,
                                         srcstbl);
              }
              else
              {
                /* this is a free handle table entry, copy over the entire
                   structure as-is */
                *stbl = *srcstbl;
              }
            }
          }
        }
      }
    }

    /* release the source handle table */
    ExReleaseHandleTableLock(SourceHandleTable);
  }

  return HandleTable;
}

static PHANDLE_TABLE_ENTRY
ExpAllocateHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                            OUT PLONG Handle)
{
  PHANDLE_TABLE_ENTRY Entry = NULL;

  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(Handle);
  ASSERT(KeGetCurrentThread() != NULL);

  DPRINT("HT[0x%p]: HandleCount: %d\n", HandleTable, HandleTable->HandleCount);

  if(HandleTable->HandleCount < EX_MAX_HANDLES)
  {
    ULONG tli, mli, eli;

    if(HandleTable->FirstFreeTableEntry != -1)
    {
      /* there's a free handle entry we can just grab and use */
      tli = TLI_FROM_HANDLE(HandleTable->FirstFreeTableEntry);
      mli = MLI_FROM_HANDLE(HandleTable->FirstFreeTableEntry);
      eli = ELI_FROM_HANDLE(HandleTable->FirstFreeTableEntry);

      /* the pointer should be valid in any way!!! */
      ASSERT(HandleTable->Table[tli]);
      ASSERT(HandleTable->Table[tli][mli]);

      Entry = &HandleTable->Table[tli][mli][eli];

      *Handle = HandleTable->FirstFreeTableEntry;

      /* save the index to the next free handle (if available) */
      HandleTable->FirstFreeTableEntry = Entry->u2.NextFreeTableEntry;
      Entry->u2.NextFreeTableEntry = 0;
      Entry->u1.Object = NULL;

      HandleTable->HandleCount++;
    }
    else
    {
      /* we need to allocate a new subhandle table first */
      PHANDLE_TABLE_ENTRY cure, laste, ntbl, *nmtbl;
      ULONG i;
      BOOLEAN AllocatedMtbl;

      ASSERT(HandleTable->NextIndexNeedingPool <= N_MAX_HANDLE);

      /* the index of the next table to be allocated was saved in
         NextIndexNeedingPool the last time a handle entry was allocated and
         the subhandle entry list was full. the subhandle entry index of
         NextIndexNeedingPool should be 0 here! */
      tli = TLI_FROM_HANDLE(HandleTable->NextIndexNeedingPool);
      mli = MLI_FROM_HANDLE(HandleTable->NextIndexNeedingPool);
      DPRINT("HandleTable->NextIndexNeedingPool: 0x%x\n", HandleTable->NextIndexNeedingPool);
      DPRINT("tli: 0x%x mli: 0x%x eli: 0x%x\n", tli, mli, ELI_FROM_HANDLE(HandleTable->NextIndexNeedingPool));

      ASSERT(ELI_FROM_HANDLE(HandleTable->NextIndexNeedingPool) == 0);

      DPRINT("HandleTable->Table[%d] == 0x%p\n", tli, HandleTable->Table[tli]);

      /* allocate a middle level entry list if required */
      nmtbl = HandleTable->Table[tli];
      if(nmtbl == NULL)
      {
        if(HandleTable->QuotaProcess != NULL)
        {
          /* FIXME - Charge process quota before allocating the handle table! */
        }

        nmtbl = ExAllocatePoolWithTag(PagedPool,
                                      N_MIDDLELEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY),
                                      TAG('E', 'x', 'H', 't'));
        if(nmtbl == NULL)
        {
          if(HandleTable->QuotaProcess != NULL)
          {
            /* FIXME - return the quota to the process */
          }

          return NULL;
        }

        /* clear the middle level entry list */
        RtlZeroMemory(nmtbl, N_MIDDLELEVEL_POINTERS * sizeof(PHANDLE_TABLE_ENTRY));

        /* make sure the table was zeroed before we set one item */
        KeMemoryBarrier();

        /* note, don't set the the pointer in the top level list yet because we
           might screw up lookups if allocating a subhandle entry table failed
           and this newly allocated table might get freed again */
        AllocatedMtbl = TRUE;
      }
      else
      {
        AllocatedMtbl = FALSE;

        /* allocate a subhandle entry table in any case! */
        ASSERT(nmtbl[mli] == NULL);
      }

      DPRINT("HandleTable->Table[%d][%d] == 0x%p\n", tli, mli, nmtbl[mli]);

      if(HandleTable->QuotaProcess != NULL)
      {
        /* FIXME - Charge process quota before allocating the handle table! */
      }

      ntbl = ExAllocatePoolWithTag(PagedPool,
                                   N_SUBHANDLE_ENTRIES * sizeof(HANDLE_TABLE_ENTRY),
                                   TAG('E', 'x', 'H', 't'));
      if(ntbl == NULL)
      {
        if(HandleTable->QuotaProcess != NULL)
        {
          /* FIXME - Return process quota charged  */
        }

        /* free the middle level entry list, if allocated, because it's empty and
           unused */
        if(AllocatedMtbl)
        {
          ExFreePool(nmtbl);

          if(HandleTable->QuotaProcess != NULL)
          {
            /* FIXME - Return process quota charged  */
          }
        }

        return NULL;
      }

      /* let's just use the very first entry */
      Entry = ntbl;
      Entry->u1.ObAttributes = EX_HANDLE_ENTRY_LOCKED;
      Entry->u2.NextFreeTableEntry = 0;

      *Handle = HandleTable->NextIndexNeedingPool;

      HandleTable->HandleCount++;

      /* set the FirstFreeTableEntry member to the second entry and chain the
         free entries */
      HandleTable->FirstFreeTableEntry = HandleTable->NextIndexNeedingPool + 1;
      for(cure = Entry + 1, laste = Entry + N_SUBHANDLE_ENTRIES, i = HandleTable->FirstFreeTableEntry + 1;
          cure != laste;
          cure++, i++)
      {
        cure->u1.Object = NULL;
        cure->u2.NextFreeTableEntry = i;
      }
      /* truncate the free entry list */
      (cure - 1)->u2.NextFreeTableEntry = -1;

      /* save the pointers to the allocated list(s) */
      InterlockedExchangePointer(&nmtbl[mli], ntbl);
      if(AllocatedMtbl)
      {
        InterlockedExchangePointer(&HandleTable->Table[tli], nmtbl);
      }

      /* increment the NextIndexNeedingPool to the next index where we need to
         allocate new memory */
      HandleTable->NextIndexNeedingPool += N_SUBHANDLE_ENTRIES;
    }
  }
  else
  {
    DPRINT1("Can't allocate any more handles in handle table 0x%p!\n", HandleTable);
  }

  return Entry;
}

static VOID
ExpFreeHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                        IN PHANDLE_TABLE_ENTRY Entry,
                        IN LONG Handle)
{
  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(Entry);
  ASSERT(IS_VALID_EX_HANDLE(Handle));

  DPRINT("ExpFreeHandleTableEntry HT:0x%p Entry:0x%p\n", HandleTable, Entry);

  /* automatically unlock the entry if currently locked. We however don't notify
     anyone who waited on the handle because we're holding an exclusive lock after
     all and these locks will fail then */
  InterlockedExchangePointer(&Entry->u1.Object, NULL);
  Entry->u2.NextFreeTableEntry = HandleTable->FirstFreeTableEntry;
  HandleTable->FirstFreeTableEntry = Handle;

  HandleTable->HandleCount--;
}

static PHANDLE_TABLE_ENTRY
ExpLookupHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                          IN LONG Handle)
{
  PHANDLE_TABLE_ENTRY Entry = NULL;

  PAGED_CODE();

  ASSERT(HandleTable);

  if(IS_VALID_EX_HANDLE(Handle))
  {
    ULONG tli, mli, eli;
    PHANDLE_TABLE_ENTRY *mlp;

    tli = TLI_FROM_HANDLE(Handle);
    mli = MLI_FROM_HANDLE(Handle);
    eli = ELI_FROM_HANDLE(Handle);

    mlp = HandleTable->Table[tli];
    if(Handle < HandleTable->NextIndexNeedingPool &&
       mlp != NULL && mlp[mli] != NULL && mlp[mli][eli].u1.Object != NULL)
    {
      Entry = &mlp[mli][eli];
      DPRINT("handle lookup 0x%x -> entry 0x%p [HT:0x%p] ptr: 0x%p\n", Handle, Entry, HandleTable, mlp[mli][eli].u1.Object);
    }
  }
  else
  {
    DPRINT("Looking up invalid handle 0x%x\n", Handle);
  }

  return Entry;
}

BOOLEAN
ExLockHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                       IN PHANDLE_TABLE_ENTRY Entry)
{
  ULONG_PTR Current, New;

  PAGED_CODE();

  DPRINT("Entering handle table entry 0x%p lock...\n", Entry);

  ASSERT(HandleTable);
  ASSERT(Entry);

  for(;;)
  {
    Current = (volatile ULONG_PTR)Entry->u1.Object;

    if(!Current || (HandleTable->Flags & EX_HANDLE_TABLE_CLOSING))
    {
      DPRINT("Attempted to lock empty handle table entry 0x%p or handle table shut down\n", Entry);
      break;
    }

    if(!(Current & EX_HANDLE_ENTRY_LOCKED))
    {
      New = Current | EX_HANDLE_ENTRY_LOCKED;
      if(InterlockedCompareExchangePointer(&Entry->u1.Object,
                                           (PVOID)New,
                                           (PVOID)Current) == (PVOID)Current)
      {
        DPRINT("SUCCESS handle table 0x%p entry 0x%p lock\n", HandleTable, Entry);
        /* we acquired the lock */
        return TRUE;
      }
    }

    /* wait about 5ms at maximum so we don't wait forever in unfortunate
       co-incidences where releasing the lock in another thread happens right
       before we're waiting on the contention event to get pulsed, which might
       never happen again... */
    KeWaitForSingleObject(&HandleTable->HandleContentionEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          &ExpHandleShortWait);
  }

  return FALSE;
}

VOID
ExUnlockHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                         IN PHANDLE_TABLE_ENTRY Entry)
{
  ULONG_PTR Current, New;

  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(Entry);

  DPRINT("ExUnlockHandleTableEntry HT:0x%p Entry:0x%p\n", HandleTable, Entry);

  Current = (volatile ULONG_PTR)Entry->u1.Object;

  ASSERT(Current & EX_HANDLE_ENTRY_LOCKED);

  New = Current & ~EX_HANDLE_ENTRY_LOCKED;

  InterlockedExchangePointer(&Entry->u1.Object,
                             (PVOID)New);

  /* we unlocked the entry, pulse the contention event so threads who're waiting
     on the release can continue */
  KePulseEvent(&HandleTable->HandleContentionEvent,
               EVENT_INCREMENT,
               FALSE);
}

LONG
ExCreateHandle(IN PHANDLE_TABLE HandleTable,
               IN PHANDLE_TABLE_ENTRY Entry)
{
  PHANDLE_TABLE_ENTRY NewHandleTableEntry;
  LONG Handle = EX_INVALID_HANDLE;

  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(Entry);

  /* The highest bit in Entry->u1.Object has to be 1 so we make sure it's a
     pointer to kmode memory. It will cleared though because it also indicates
     the lock */
  ASSERT((ULONG_PTR)Entry->u1.Object & EX_HANDLE_ENTRY_LOCKED);

  KeEnterCriticalRegion();
  ExAcquireHandleTableLockExclusive(HandleTable);

  NewHandleTableEntry = ExpAllocateHandleTableEntry(HandleTable,
                                                    &Handle);
  if(NewHandleTableEntry != NULL)
  {
    *NewHandleTableEntry = *Entry;

    ExUnlockHandleTableEntry(HandleTable,
                             NewHandleTableEntry);
  }

  ExReleaseHandleTableLock(HandleTable);
  KeLeaveCriticalRegion();

  return Handle;
}

BOOLEAN
ExDestroyHandle(IN PHANDLE_TABLE HandleTable,
                IN LONG Handle)
{
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  BOOLEAN Ret = FALSE;

  PAGED_CODE();

  ASSERT(HandleTable);

  KeEnterCriticalRegion();
  ExAcquireHandleTableLockExclusive(HandleTable);

  HandleTableEntry = ExpLookupHandleTableEntry(HandleTable,
                                               Handle);

  if(HandleTableEntry != NULL && ExLockHandleTableEntry(HandleTable, HandleTableEntry))
  {
    /* free and automatically unlock the handle. However we don't need to pulse
       the contention event since other locks on this entry will fail */
    ExpFreeHandleTableEntry(HandleTable,
                            HandleTableEntry,
                            Handle);
    Ret = TRUE;
  }

  ExReleaseHandleTableLock(HandleTable);
  KeLeaveCriticalRegion();

  return Ret;
}

VOID
ExDestroyHandleByEntry(IN PHANDLE_TABLE HandleTable,
                       IN PHANDLE_TABLE_ENTRY Entry,
                       IN LONG Handle)
{
  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(Entry);

  /* This routine requires the entry to be locked */
  ASSERT((ULONG_PTR)Entry->u1.Object & EX_HANDLE_ENTRY_LOCKED);

  DPRINT("DestroyHandleByEntry HT:0x%p Entry:0x%p\n", HandleTable, Entry);

  KeEnterCriticalRegion();
  ExAcquireHandleTableLockExclusive(HandleTable);

  /* free and automatically unlock the handle. However we don't need to pulse
     the contention event since other locks on this entry will fail */
  ExpFreeHandleTableEntry(HandleTable,
                          Entry,
                          Handle);

  ExReleaseHandleTableLock(HandleTable);
  KeLeaveCriticalRegion();
}

PHANDLE_TABLE_ENTRY
ExMapHandleToPointer(IN PHANDLE_TABLE HandleTable,
                     IN LONG Handle)
{
  PHANDLE_TABLE_ENTRY HandleTableEntry;

  PAGED_CODE();

  ASSERT(HandleTable);

  ExAcquireHandleTableLockShared(HandleTable);
  HandleTableEntry = ExpLookupHandleTableEntry(HandleTable,
                                               Handle);
  if (HandleTableEntry != NULL && ExLockHandleTableEntry(HandleTable, HandleTableEntry))
  {
    DPRINT("ExMapHandleToPointer HT:0x%p Entry:0x%p locked\n", HandleTable, HandleTableEntry);
    ExReleaseHandleTableLock(HandleTable);
    return HandleTableEntry;
  }
  ExReleaseHandleTableLock(HandleTable);
  return NULL;
}

BOOLEAN
ExChangeHandle(IN PHANDLE_TABLE HandleTable,
               IN LONG Handle,
               IN PEX_CHANGE_HANDLE_CALLBACK ChangeHandleCallback,
               IN PVOID Context)
{
  PHANDLE_TABLE_ENTRY HandleTableEntry;
  BOOLEAN Ret = FALSE;

  PAGED_CODE();

  ASSERT(HandleTable);
  ASSERT(ChangeHandleCallback);

  KeEnterCriticalRegion();
  ExAcquireHandleTableLockShared(HandleTable);

  HandleTableEntry = ExpLookupHandleTableEntry(HandleTable,
                                               Handle);

  if(HandleTableEntry != NULL && ExLockHandleTableEntry(HandleTable, HandleTableEntry))
  {
    ExReleaseHandleTableLock(HandleTable);
    Ret = ChangeHandleCallback(HandleTable,
                               HandleTableEntry,
                               NULL);

    ExUnlockHandleTableEntry(HandleTable,
                             HandleTableEntry);
  }
  else
  {
    ExReleaseHandleTableLock(HandleTable);
  }
  KeLeaveCriticalRegion();

  return Ret;
}

/* EOF */
