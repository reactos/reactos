/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/gdiobj.c
 * PURPOSE:         General GDI object manipulation routines
 * PROGRAMMERS:     ...
 */

/** INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FIXME include right header for KeRosDumpStackFrames */
VOID NTAPI KeRosDumpStackFrames(PULONG, ULONG);

//#define GDI_DEBUG

#ifdef GDI_DEBUG
BOOLEAN STDCALL KiRosPrintAddress(PVOID Address);
NTSYSAPI ULONG NTAPI RtlWalkFrameChain(OUT PVOID *Callers, IN ULONG Count, IN ULONG Flags);
#endif

#define GDI_ENTRY_TO_INDEX(ht, e)                                              \
  (((ULONG_PTR)(e) - (ULONG_PTR)&((ht)->Entries[0])) / sizeof(GDI_TABLE_ENTRY))
#define GDI_HANDLE_GET_ENTRY(HandleTable, h)                                   \
  (&(HandleTable)->Entries[GDI_HANDLE_GET_INDEX((h))])

/* apparently the first 10 entries are never used in windows as they are empty */
#define RESERVE_ENTRIES_COUNT 10

#define DelayExecution() \
  DPRINT("%s:%i: Delay\n", __FILE__, __LINE__); \
  KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay)

static BOOL INTERNAL_CALL GDI_CleanupDummy(PVOID ObjectBody);

/** GLOBALS *******************************************************************/

typedef struct
{
    BOOL bUseLookaside;
    ULONG_PTR ulBodySize;
    ULONG Tag;
    GDICLEANUPPROC CleanupProc;
} OBJ_TYPE_INFO, *POBJ_TYPE_INFO;

static const
OBJ_TYPE_INFO ObjTypeInfo[] =
{
  {0, 0,                     0,                NULL},             /* 00 reserved entry */
  {1, sizeof(DC),            TAG_DC,           DC_Cleanup},       /* 01 DC */
  {1, 0,                     0,                NULL},             /* 02 UNUSED1 */
  {1, 0,                     0,                NULL},             /* 03 UNUSED2 */
  {1, sizeof(ROSRGNDATA),    TAG_REGION,       REGION_Cleanup},   /* 04 RGN */
  {1, sizeof(BITMAPOBJ),     TAG_SURFACE,      BITMAP_Cleanup},   /* 05 SURFACE */
  {0, sizeof(DC),            TAG_CLIENTOBJ,    GDI_CleanupDummy}, /* 06 CLIENTOBJ: METADC,... FIXME: don't use DC struct */
  {0, 0,                     TAG_PATH,         NULL},             /* 07 PATH, unused */
  {1, sizeof(PALGDI),        TAG_PALETTE,      PALETTE_Cleanup},  /* 08 PAL */
  {0, 0,                     TAG_ICMLCS,       NULL},             /* 09 ICMLCS, unused */
  {1, sizeof(TEXTOBJ),       TAG_LFONT,        GDI_CleanupDummy}, /* 0a LFONT */
  {0, 0,                     TAG_RFONT,        NULL},             /* 0b RFONT, unused */
  {0, 0,                     TAG_PFE,          NULL},             /* 0c PFE, unused */
  {0, 0,                     TAG_PFT,          NULL},             /* 0d PFT, unused */
  {0, 0,                     TAG_ICMCXF,       NULL},             /* 0e ICMCXF, unused */
  {0, 0,                     TAG_SPRITE,       NULL},             /* 0f SPRITE, unused */
  {1, sizeof(GDIBRUSHOBJ),   TAG_BRUSH,        BRUSH_Cleanup},    /* 10 BRUSH, PEN, EXTPEN */
  {0, 0,                     TAG_UMPD,         NULL},             /* 11 UMPD, unused */
  {0, 0,                     0,                NULL},             /* 12 UNUSED4 */
  {0, 0,                     TAG_SPACE,        NULL},             /* 13 SPACE, unused */
  {0, 0,                     0,                NULL},             /* 14 UNUSED5 */
  {0, 0,                     TAG_META,         NULL},             /* 15 META, unused */
  {0, 0,                     TAG_EFSTATE,      NULL},             /* 16 EFSTATE, unused */
  {0, 0,                     TAG_BMFD,         NULL},             /* 17 BMFD, unused */
  {0, 0,                     TAG_VTFD,         NULL},             /* 18 VTFD, unused */
  {0, 0,                     TAG_TTFD,         NULL},             /* 19 TTFD, unused */
  {0, 0,                     TAG_RC,           NULL},             /* 1a RC, unused */
  {0, 0,                     TAG_TEMP,         NULL},             /* 1b TEMP, unused */
  {0, 0,                     TAG_DRVOBJ,       NULL},             /* 1c DRVOBJ, unused */
  {0, 0,                     TAG_DCIOBJ,       NULL},             /* 1d DCIOBJ, unused */
  {0, 0,                     TAG_SPOOL,        NULL},             /* 1e SPOOL, unused */
};

#define BASE_OBJTYPE_COUNT (sizeof(ObjTypeInfo) / sizeof(ObjTypeInfo[0]))

static LARGE_INTEGER ShortDelay;

/** DEBUGGING *****************************************************************/

#ifdef GDI_DEBUG

static int leak_reported = 0;
#define GDI_STACK_LEVELS 12
static ULONG GDIHandleAllocator[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
static ULONG GDIHandleLocker[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
struct DbgOpenGDIHandle
{
    ULONG idx;
    int count;
};
#define H 1024
static struct DbgOpenGDIHandle h[H];

void IntDumpHandleTable(PGDI_HANDLE_TABLE HandleTable)
{
    int i, n = 0, j, k, J;

    if (leak_reported)
    {
        DPRINT1("gdi handle abusers already reported!\n");
        return;
    }

    leak_reported = 1;
    DPRINT1("reporting gdi handle abusers:\n");

    /* step through GDI handle table and find out who our culprit is... */
    for (i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++)
    {
        for (j = 0; j < n; j++)
        {
next:
            J = h[j].idx;
            for (k = 0; k < GDI_STACK_LEVELS; k++)
            {
                if (GDIHandleAllocator[i][k]
                        != GDIHandleAllocator[J][k])
                {
                    if (++j == n)
                        goto done;
                    else
                        goto next;
                }
            }
            goto done;
        }
done:
        if (j < H)
        {
            if (j == n)
            {
                h[j].idx = i;
                h[j].count = 1;
                n = n + 1;
            }
            else
                h[j].count++;
        }
    }
    /* bubble sort time! weeeeee!! */
    for (i = 0; i < n-1; i++)
    {
        if (h[i].count < h[i+1].count)
        {
            struct DbgOpenGDIHandle t;
            t = h[i+1];
            h[i+1] = h[i];
            j = i;
            while (j > 0 && h[j-1].count < t.count)
                j--;
            h[j] = t;
        }
    }
    /* print the worst offenders... */
    DbgPrint("Worst GDI Handle leak offenders (out of %i unique locations):\n", n);
    for (i = 0; i < n && h[i].count > 1; i++)
    {
        int j;
        DbgPrint(" %i allocs: ", h[i].count);
        for (j = 0; j < GDI_STACK_LEVELS; j++)
        {
            ULONG Addr = GDIHandleAllocator[h[i].idx][j];
            if (!KiRosPrintAddress((PVOID)Addr))
                DbgPrint("<%X>", Addr);
        }
        DbgPrint("\n");
    }
    if (i < n && h[i].count == 1)
        DbgPrint("(list terminated - the remaining entries have 1 allocation only)\n");
}

ULONG
CaptureStackBackTace(PVOID* pFrames, ULONG nFramesToCapture)
{
    ULONG nFrameCount;

    memset(pFrames, 0x00, (nFramesToCapture + 1) * sizeof(PVOID));

    nFrameCount = RtlCaptureStackBackTrace(1, nFramesToCapture, pFrames, NULL);

    if (nFrameCount < nFramesToCapture)
    {
        nFrameCount += RtlWalkFrameChain(pFrames + nFrameCount, nFramesToCapture - nFrameCount, 1);
    }

    return nFrameCount;
}

#define GDIDBG_TRACECALLER() \
  DPRINT1("-> called from:\n"); \
  KeRosDumpStackFrames(NULL, 20);
#define GDIDBG_TRACEALLOCATOR(index) \
  DPRINT1("-> allocated from:\n"); \
  KeRosDumpStackFrames(GDIHandleAllocator[index], GDI_STACK_LEVELS);
#define GDIDBG_TRACELOCKER(index) \
  DPRINT1("-> locked from:\n"); \
  KeRosDumpStackFrames(GDIHandleLocker[index], GDI_STACK_LEVELS);
#define GDIDBG_CAPTUREALLOCATOR(index) \
  CaptureStackBackTace((PVOID*)GDIHandleAllocator[index], GDI_STACK_LEVELS);
#define GDIDBG_CAPTURELOCKER(index) \
  CaptureStackBackTace((PVOID*)GDIHandleLocker[index], GDI_STACK_LEVELS);
#define GDIDBG_DUMPHANDLETABLE() \
  IntDumpHandleTable(GdiHandleTable)
#define GDIDBG_INITLOOPTRACE() \
  ULONG Attempts = 0;
#define GDIDBG_TRACELOOP(Handle, PrevThread, Thread) \
  if ((++Attempts % 20) == 0) \
  { \
    DPRINT1("[%d] Handle 0x%p Locked by 0x%x (we're 0x%x)\n", Attempts, Handle, PrevThread, Thread); \
  }

#else

#define GDIDBG_TRACECALLER()
#define GDIDBG_TRACEALLOCATOR(index)
#define GDIDBG_TRACELOCKER(index)
#define GDIDBG_CAPTUREALLOCATOR(index)
#define GDIDBG_CAPTURELOCKER(index)
#define GDIDBG_DUMPHANDLETABLE()
#define GDIDBG_INITLOOPTRACE()
#define GDIDBG_TRACELOOP(Handle, PrevThread, Thread)

#endif /* GDI_DEBUG */


/** INTERNAL FUNCTIONS ********************************************************/

/*
 * Dummy GDI Cleanup Callback
 */
static BOOL INTERNAL_CALL
GDI_CleanupDummy(PVOID ObjectBody)
{
    return TRUE;
}

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
PGDI_HANDLE_TABLE INTERNAL_CALL
GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject)
{
    PGDI_HANDLE_TABLE HandleTable = NULL;
    LARGE_INTEGER htSize;
    UINT ObjType;
    ULONG ViewSize = 0;
    NTSTATUS Status;

    ASSERT(SectionObject != NULL);

    htSize.QuadPart = sizeof(GDI_HANDLE_TABLE);

    Status = MmCreateSection((PVOID*)SectionObject,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &htSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
        return NULL;

    /* FIXME - use MmMapViewInSessionSpace once available! */
    Status = MmMapViewInSystemSpace(*SectionObject,
                                    (PVOID*)&HandleTable,
                                    &ViewSize);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(*SectionObject);
        *SectionObject = NULL;
        return NULL;
    }

    RtlZeroMemory(HandleTable, sizeof(GDI_HANDLE_TABLE));

    HandleTable->LookasideLists = ExAllocatePoolWithTag(NonPagedPool,
                                  BASE_OBJTYPE_COUNT * sizeof(PAGED_LOOKASIDE_LIST),
                                  TAG_GDIHNDTBLE);
    if (HandleTable->LookasideLists == NULL)
    {
        MmUnmapViewInSystemSpace(HandleTable);
        ObDereferenceObject(*SectionObject);
        *SectionObject = NULL;
        return NULL;
    }

    for (ObjType = 0; ObjType < BASE_OBJTYPE_COUNT; ObjType++)
    {
        if (ObjTypeInfo[ObjType].bUseLookaside)
        {
            ExInitializePagedLookasideList(HandleTable->LookasideLists + ObjType,
                                           NULL,
                                           NULL,
                                           0,
                                           ObjTypeInfo[ObjType].ulBodySize,
                                           ObjTypeInfo[ObjType].Tag,
                                           0);
        }
    }

    ShortDelay.QuadPart = -5000LL; /* FIXME - 0.5 ms? */

    HandleTable->FirstFree = 0;
    HandleTable->FirstUnused = RESERVE_ENTRIES_COUNT;

    return HandleTable;
}

static void FASTCALL
LockErrorDebugOutput(HGDIOBJ hObj, PGDI_TABLE_ENTRY Entry, LPSTR Function)
{
    if ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) == 0)
    {
        DPRINT1("%s: Attempted to lock object 0x%x that is deleted!\n", Function, hObj);
    }
    else if (GDI_HANDLE_GET_REUSECNT(hObj) != GDI_ENTRY_GET_REUSECNT(Entry->Type))
    {
        DPRINT1("%s: Attempted to lock object 0x%x, wrong reuse counter (Handle: 0x%x, Entry: 0x%x)\n",
                Function, hObj, GDI_HANDLE_GET_REUSECNT(hObj), GDI_ENTRY_GET_REUSECNT(Entry->Type));
    }
    else if (GDI_HANDLE_GET_TYPE(hObj) != ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK))
    {
        DPRINT1("%s: Attempted to lock object 0x%x, type mismatch (Handle: 0x%x, Entry: 0x%x)\n",
                Function, hObj, GDI_HANDLE_GET_TYPE(hObj), (Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK);
    }
    else
    {
        DPRINT1("%s: Attempted to lock object 0x%x, something went wrong, typeinfo = 0x%x\n",
                Function, hObj, Entry->Type);
    }
    GDIDBG_TRACECALLER();
}

ULONG
FASTCALL
InterlockedPopFreeEntry()
{
    ULONG idxFirstFree, idxNextFree, idxPrev;
    PGDI_TABLE_ENTRY pFreeEntry;

    DPRINT("Enter InterLockedPopFreeEntry\n");

    do
    {
        idxFirstFree = GdiHandleTable->FirstFree;
        if (idxFirstFree)
        {
            pFreeEntry = GdiHandleTable->Entries + idxFirstFree;
            ASSERT(((ULONG)pFreeEntry->KernelData & ~GDI_HANDLE_INDEX_MASK) == 0);
            idxNextFree = (ULONG)pFreeEntry->KernelData;
            idxPrev = (ULONG)_InterlockedCompareExchange((LONG*)&GdiHandleTable->FirstFree, idxNextFree, idxFirstFree);
        }
        else
        {
            idxFirstFree = GdiHandleTable->FirstUnused;
            idxNextFree = idxFirstFree + 1;
            if (idxNextFree >= GDI_HANDLE_COUNT)
            {
                DPRINT1("No more gdi handles left!\n");
                return 0;
            }
            idxPrev = (ULONG)_InterlockedCompareExchange((LONG*)&GdiHandleTable->FirstUnused, idxNextFree, idxFirstFree);
        }
    }
    while (idxPrev != idxFirstFree);

    return idxFirstFree;
}

/* Pushes an entry of the handle table to the free list,
   The entry must be unlocked and the base type field must be 0 */
VOID
FASTCALL
InterlockedPushFreeEntry(ULONG idxToFree)
{
    ULONG idxFirstFree, idxPrev;
    PGDI_TABLE_ENTRY pFreeEntry;

    DPRINT("Enter InterlockedPushFreeEntry\n");

    pFreeEntry = GdiHandleTable->Entries + idxToFree;
    ASSERT((pFreeEntry->Type & GDI_ENTRY_BASETYPE_MASK) == 0);
    ASSERT(pFreeEntry->ProcessId == 0);
    pFreeEntry->UserData = NULL;

    do
    {
        idxFirstFree = GdiHandleTable->FirstFree;
        pFreeEntry->KernelData = (PVOID)idxFirstFree;

        idxPrev = (ULONG)_InterlockedCompareExchange((LONG*)&GdiHandleTable->FirstFree, idxToFree, idxFirstFree);
    }
    while (idxPrev != idxFirstFree);
}


BOOL
INTERNAL_CALL
GDIOBJ_ValidateHandle(HGDIOBJ hObj, ULONG ObjectType)
{
    PGDI_TABLE_ENTRY Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, hObj);
    if ((((ULONG_PTR)hObj & GDI_HANDLE_TYPE_MASK) == ObjectType) &&
        (Entry->Type << GDI_ENTRY_UPPER_SHIFT) == GDI_HANDLE_GET_UPPER(hObj))
    {
        HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
        if (pid == NULL || pid == PsGetCurrentProcessId())
        {
            return TRUE;
        }
    }
    return FALSE;
}

POBJ INTERNAL_CALL
GDIOBJ_AllocObj(UCHAR BaseType)
{
    POBJ pObject;

    ASSERT((BaseType & ~GDIObjTypeTotal) == 0);
//    BaseType &= GDI_HANDLE_BASETYPE_MASK;

    if (ObjTypeInfo[BaseType].bUseLookaside)
    {
        PPAGED_LOOKASIDE_LIST LookasideList;

        LookasideList = GdiHandleTable->LookasideLists + BaseType;
        pObject = ExAllocateFromPagedLookasideList(LookasideList);
    }
    else
    {
        pObject = ExAllocatePoolWithTag(PagedPool,
                                        ObjTypeInfo[BaseType].ulBodySize,
                                        ObjTypeInfo[BaseType].Tag);
    }

    if (pObject)
    {
        RtlZeroMemory(pObject, ObjTypeInfo[BaseType].ulBodySize);
    }

    return pObject;
}


/*!
 * Allocate memory for GDI object and return handle to it.
 *
 * \param ObjectType - type of object \ref GDI object types
 *
 * \return Pointer to the allocated object, which is locked.
*/
POBJ INTERNAL_CALL
GDIOBJ_AllocObjWithHandle(ULONG ObjectType)
{
    PW32PROCESS W32Process;
    POBJ  newObject = NULL;
    HANDLE CurrentProcessId, LockedProcessId;
    UCHAR TypeIndex;

    GDIDBG_INITLOOPTRACE();

    W32Process = PsGetCurrentProcessWin32Process();
    /* HACK HACK HACK: simplest-possible quota implementation - don't allow a process
       to take too many GDI objects, itself. */
    if (W32Process && W32Process->GDIObjects >= 0x2710)
        return NULL;

    ASSERT(ObjectType != GDI_OBJECT_TYPE_DONTCARE);

    TypeIndex = GDI_OBJECT_GET_TYPE_INDEX(ObjectType);

    newObject = GDIOBJ_AllocObj(TypeIndex);
    if (!newObject)
    {
        DPRINT1("Not enough memory to allocate gdi object!\n");
        return NULL;
    }

    UINT Index;
    PGDI_TABLE_ENTRY Entry;
    LONG TypeInfo;

    CurrentProcessId = PsGetCurrentProcessId();
    LockedProcessId = (HANDLE)((ULONG_PTR)CurrentProcessId | 0x1);

//    RtlZeroMemory(newObject, ObjTypeInfo[TypeIndex].ulBodySize);

    /* On Windows the higher 16 bit of the type field don't contain the
       full type from the handle, but the base type.
       (type = BRSUH, PEN, EXTPEN, basetype = BRUSH) */
    TypeInfo = (ObjectType & GDI_HANDLE_BASETYPE_MASK) | (ObjectType >> GDI_ENTRY_UPPER_SHIFT);

    Index = InterlockedPopFreeEntry();
    if (Index != 0)
    {
        HANDLE PrevProcId;

        Entry = &GdiHandleTable->Entries[Index];

LockHandle:
        PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, 0);
        if (PrevProcId == NULL)
        {
            PW32THREAD Thread = PsGetCurrentThreadWin32Thread();
            HGDIOBJ Handle;

            Entry->KernelData = newObject;

            /* copy the reuse-counter */
            TypeInfo |= Entry->Type & GDI_ENTRY_REUSE_MASK;

            /* we found a free entry, no need to exchange this field atomically
               since we're holding the lock */
            Entry->Type = TypeInfo;

            /* Create a handle */
            Handle = (HGDIOBJ)((Index & 0xFFFF) | (TypeInfo << GDI_ENTRY_UPPER_SHIFT));

            /* Initialize BaseObject fields */
            newObject->hHmgr = Handle;
            newObject->ulShareCount = 0;
            newObject->cExclusiveLock = 1;
            newObject->Tid = Thread;

            /* unlock the entry */
            (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, CurrentProcessId);

            GDIDBG_CAPTUREALLOCATOR(Index);

            if (W32Process != NULL)
            {
                _InterlockedIncrement(&W32Process->GDIObjects);
            }

            DPRINT("GDIOBJ_AllocObj: 0x%x ob: 0x%x\n", Handle, newObject);
            return newObject;
        }
        else
        {
            GDIDBG_TRACELOOP(Index, PrevProcId, CurrentProcessId);
            /* damn, someone is trying to lock the object even though it doesn't
               even exist anymore, wait a little and try again!
               FIXME - we shouldn't loop forever! Give up after some time! */
            DelayExecution();
            /* try again */
            goto LockHandle;
        }
    }

    GDIOBJ_FreeObj(newObject, TypeIndex);

    DPRINT1("Failed to insert gdi object into the handle table, no handles left!\n");
    GDIDBG_DUMPHANDLETABLE();

    return NULL;
}


VOID INTERNAL_CALL
GDIOBJ_FreeObj(POBJ pObject, UCHAR BaseType)
{
    /* Object must not have a handle! */
    ASSERT(pObject->hHmgr == NULL);

    if (ObjTypeInfo[BaseType].bUseLookaside)
    {
        PPAGED_LOOKASIDE_LIST LookasideList;

        LookasideList = GdiHandleTable->LookasideLists + BaseType;
        ExFreeToPagedLookasideList(LookasideList, pObject);
    }
    else
    {
        ExFreePool(pObject);
    }
}

/*!
 * Free memory allocated for the GDI object. For each object type this function calls the
 * appropriate cleanup routine.
 *
 * \param hObj       - handle of the object to be deleted.
 *
 * \return Returns TRUE if succesful.
 * \return Returns FALSE if the cleanup routine returned FALSE or the object doesn't belong
 * to the calling process.
*/
BOOL INTERNAL_CALL
GDIOBJ_FreeObjByHandle(HGDIOBJ hObj, DWORD ExpectedType)
{
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, LockedProcessId, PrevProcId;
    ULONG HandleType, HandleUpper, TypeIndex;
    BOOL Silent;

    GDIDBG_INITLOOPTRACE();

    DPRINT("GDIOBJ_FreeObj: hObj: 0x%08x\n", hObj);

    if (GDI_HANDLE_IS_STOCKOBJ(hObj))
    {
        DPRINT1("GDIOBJ_FreeObj() failed, can't delete stock object handle: 0x%x !!!\n", hObj);
        GDIDBG_TRACECALLER();
        return FALSE;
    }

    ProcessId = PsGetCurrentProcessId();
    LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

    Silent = (ExpectedType & GDI_OBJECT_TYPE_SILENT);
    ExpectedType &= ~GDI_OBJECT_TYPE_SILENT;

    HandleType = GDI_HANDLE_GET_TYPE(hObj);
    HandleUpper = GDI_HANDLE_GET_UPPER(hObj);

    /* Check if we have the requested type */
    if ( (ExpectedType != GDI_OBJECT_TYPE_DONTCARE &&
          HandleType != ExpectedType) ||
         HandleType == 0 )
    {
        DPRINT1("Attempted to free object 0x%x of wrong type (Handle: 0x%x, expected: 0x%x)\n",
                hObj, HandleType, ExpectedType);
        GDIDBG_TRACECALLER();
        return FALSE;
    }

    Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, hObj);

LockHandle:
    /* lock the object, we must not delete global objects, so don't exchange the locking
       process ID to zero when attempting to lock a global object... */
    PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, ProcessId);
    if (PrevProcId == ProcessId)
    {
        if ( (Entry->KernelData != NULL) &&
             ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) == HandleUpper) &&
             ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) == (HandleUpper & GDI_ENTRY_BASETYPE_MASK)) )
        {
            POBJ Object;

            Object = Entry->KernelData;

            if (Object->cExclusiveLock == 0)
            {
                BOOL Ret;
                PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();

                /* Clear the basetype field so when unlocking the handle it gets finally deleted and increment reuse counter */
                Entry->Type = (Entry->Type + GDI_ENTRY_REUSE_INC) & ~GDI_ENTRY_BASETYPE_MASK;

                /* unlock the handle slot */
                (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, NULL);

                /* push this entry to the free list */
                InterlockedPushFreeEntry(GDI_ENTRY_TO_INDEX(GdiHandleTable, Entry));

                Object->hHmgr = NULL;

                if (W32Process != NULL)
                {
                    _InterlockedDecrement(&W32Process->GDIObjects);
                }

                /* call the cleanup routine. */
                TypeIndex = GDI_OBJECT_GET_TYPE_INDEX(HandleType);
                Ret = ObjTypeInfo[TypeIndex].CleanupProc(Object);

                /* Now it's time to free the memory */
                GDIOBJ_FreeObj(Object, TypeIndex);

                return Ret;
            }
            else
            {
                /*
                 * The object is currently locked, so freeing is forbidden!
                 */
                DPRINT1("Object->cExclusiveLock = %d\n", Object->cExclusiveLock);
                GDIDBG_TRACECALLER();
                GDIDBG_TRACELOCKER(GDI_HANDLE_GET_INDEX(hObj));
                ASSERT(FALSE);
            }
        }
        else
        {
            LockErrorDebugOutput(hObj, Entry, "GDIOBJ_FreeObj");
            (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);
        }
    }
    else if (PrevProcId == LockedProcessId)
    {
        GDIDBG_TRACELOOP(hObj, PrevProcId, ProcessId);

        /* the object is currently locked, wait some time and try again.
           FIXME - we shouldn't loop forever! Give up after some time! */
        DelayExecution();
        /* try again */
        goto LockHandle;
    }
    else
    {
        if (!Silent)
        {
            if (((ULONG_PTR)PrevProcId & ~0x1) == 0)
            {
                DPRINT1("Attempted to free global gdi handle 0x%x, caller needs to get ownership first!!!\n", hObj);
                DPRINT1("Type = 0x%lx, KernelData = 0x%p, ProcessId = 0x%p\n", Entry->Type, Entry->KernelData, Entry->ProcessId);
            }
            else
            {
                DPRINT1("Attempted to free foreign handle: 0x%x Owner: 0x%x from Caller: 0x%x\n", hObj, (ULONG_PTR)PrevProcId & ~0x1, (ULONG_PTR)ProcessId & ~0x1);
            }
            GDIDBG_TRACECALLER();
            GDIDBG_TRACEALLOCATOR(GDI_HANDLE_GET_INDEX(hObj));
        }
    }

    return FALSE;
}

BOOL
FASTCALL
IsObjectDead(HGDIOBJ hObject)
{
    INT Index = GDI_HANDLE_GET_INDEX(hObject);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    // We check to see if the objects are knocking on deaths door.
    if ((Entry->Type & ~GDI_ENTRY_REUSE_MASK) != 0 && Entry->KernelData != NULL)
        return FALSE;
    else
    {
        DPRINT1("Object 0x%x currently being destroyed!!!\n",hObject);
        return TRUE; // return true and move on.
    }
}


/*!
 * Delete GDI object
 * \param	hObject object handle
 * \return	if the function fails the returned value is FALSE.
*/
BOOL
FASTCALL
NtGdiDeleteObject(HGDIOBJ hObject)
{
    DPRINT("NtGdiDeleteObject handle 0x%08x\n", hObject);
    if (!IsObjectDead(hObject))
    {
        return NULL != hObject
               ? GDIOBJ_FreeObjByHandle(hObject, GDI_OBJECT_TYPE_DONTCARE) : FALSE;
    }
    else
    {
        DPRINT1("Attempt DeleteObject 0x%x currently being destroyed!!!\n",hObject);
        return TRUE; // return true and move on.
    }
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL INTERNAL_CALL
GDI_CleanupForProcess(struct _EPROCESS *Process)
{
    PGDI_TABLE_ENTRY Entry, End;
    PEPROCESS CurrentProcess;
    PW32PROCESS W32Process;
    HANDLE ProcId;
    ULONG Index = RESERVE_ENTRIES_COUNT;

    DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Process->UniqueProcessId);
    CurrentProcess = PsGetCurrentProcess();
    if (CurrentProcess != Process)
    {
        KeAttachProcess(&Process->Pcb);
    }
    W32Process = (PW32PROCESS)Process->Win32Process;
    ASSERT(W32Process);

    if (W32Process->GDIObjects > 0)
    {
        /* FIXME - Instead of building the handle here and delete it using GDIOBJ_FreeObj
                   we should delete it directly here! */
        ProcId = Process->UniqueProcessId;

        End = &GdiHandleTable->Entries[GDI_HANDLE_COUNT];
        for (Entry = &GdiHandleTable->Entries[RESERVE_ENTRIES_COUNT];
                Entry != End;
                Entry++, Index++)
        {
            /* ignore the lock bit */
            if ( (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) == ProcId &&
                 (Entry->Type & ~GDI_ENTRY_REUSE_MASK) != 0 )
            {
                HGDIOBJ ObjectHandle;

                /* Create the object handle for the entry, the lower(!) 16 bit of the
                   Type field includes the type of the object including the stock
                   object flag - but since stock objects don't have a process id we can
                   simply ignore this fact here. */
                ObjectHandle = (HGDIOBJ)(Index | (Entry->Type << GDI_ENTRY_UPPER_SHIFT));

                if (GDIOBJ_FreeObjByHandle(ObjectHandle, GDI_OBJECT_TYPE_DONTCARE) &&
                        W32Process->GDIObjects == 0)
                {
                    /* there are no more gdi handles for this process, bail */
                    break;
                }
            }
        }
    }

    if (CurrentProcess != Process)
    {
        KeDetachProcess();
    }

    DPRINT("Completed cleanup for process %d\n", Process->UniqueProcessId);

    return TRUE;
}

/*!
 * Return pointer to the object by handle.
 *
 * \param hObj 		Object handle
 * \return		Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Get rid of the ExpectedType parameter!
*/
PGDIOBJ INTERNAL_CALL
GDIOBJ_LockObj(HGDIOBJ hObj, DWORD ExpectedType)
{
    ULONG HandleIndex;
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, HandleProcessId, LockedProcessId, PrevProcId;
    POBJ Object = NULL;
    ULONG HandleType, HandleUpper;

    HandleIndex = GDI_HANDLE_GET_INDEX(hObj);
    HandleType = GDI_HANDLE_GET_TYPE(hObj);
    HandleUpper = GDI_HANDLE_GET_UPPER(hObj);

    /* Check that the handle index is valid. */
    if (HandleIndex >= GDI_HANDLE_COUNT)
        return NULL;

    Entry = &GdiHandleTable->Entries[HandleIndex];

    /* Check if we have the requested type */
    if ( (ExpectedType != GDI_OBJECT_TYPE_DONTCARE &&
          HandleType != ExpectedType) ||
         HandleType == 0 )
    {
        DPRINT1("Attempted to lock object 0x%x of wrong type (Handle: 0x%x, requested: 0x%x)\n",
                hObj, HandleType, ExpectedType);
        GDIDBG_TRACECALLER();
        GDIDBG_TRACEALLOCATOR(GDI_HANDLE_GET_INDEX(hObj));
        return NULL;
    }

    ProcessId = (HANDLE)((ULONG_PTR)PsGetCurrentProcessId() & ~1);
    HandleProcessId = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~1);

    /* Check for invalid owner. */
    if (ProcessId != HandleProcessId && HandleProcessId != NULL)
    {
        DPRINT1("Tried to lock object (0x%p) of wrong owner! ProcessId = %p, HandleProcessId = %p\n", hObj, ProcessId, HandleProcessId);
        GDIDBG_TRACECALLER();
        GDIDBG_TRACEALLOCATOR(GDI_HANDLE_GET_INDEX(hObj));
        return NULL;
    }

    /*
     * Prevent the thread from being terminated during the locking process.
     * It would result in undesired effects and inconsistency of the global
     * handle table.
     */

    KeEnterCriticalRegion();

    /*
     * Loop until we either successfully lock the handle entry & object or
     * fail some of the check.
     */

    for (;;)
    {
        /* Lock the handle table entry. */
        LockedProcessId = (HANDLE)((ULONG_PTR)HandleProcessId | 0x1);
        PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId,
                                                        LockedProcessId,
                                                        HandleProcessId);

        if (PrevProcId == HandleProcessId)
        {
            /*
             * We're locking an object that belongs to our process or it's a
             * global object if HandleProcessId is 0 here.
             */

            if ( (Entry->KernelData != NULL) &&
                 ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) == HandleUpper) )
            {
                PW32THREAD Thread = PsGetCurrentThreadWin32Thread();
                Object = Entry->KernelData;

                if (Object->cExclusiveLock == 0)
                {
                    Object->Tid = Thread;
                    Object->cExclusiveLock = 1;
                    GDIDBG_CAPTURELOCKER(GDI_HANDLE_GET_INDEX(hObj))
                }
                else
                {
                    if (Object->Tid != Thread)
                    {
                        /* Unlock the handle table entry. */
                        (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

                        DelayExecution();
                        continue;
                    }
                    _InterlockedIncrement((PLONG)&Object->cExclusiveLock);
                }
            }
            else
            {
                /*
                 * Debugging code. Report attempts to lock deleted handles and
                 * locking type mismatches.
                 */
                LockErrorDebugOutput(hObj, Entry, "GDIOBJ_LockObj");
            }

            /* Unlock the handle table entry. */
            (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

            break;
        }
        else
        {
            /*
             * The handle is currently locked, wait some time and try again.
             */

            DelayExecution();
            continue;
        }
    }

    KeLeaveCriticalRegion();

    return Object;
}


/*!
 * Return pointer to the object by handle (and allow sharing of the handle
 * across threads).
 *
 * \param hObj 		Object handle
 * \return		Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Get rid of the ExpectedType parameter!
*/
PGDIOBJ INTERNAL_CALL
GDIOBJ_ShareLockObj(HGDIOBJ hObj, DWORD ExpectedType)
{
    ULONG HandleIndex;
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, HandleProcessId, LockedProcessId, PrevProcId;
    POBJ Object = NULL;
    ULONG_PTR HandleType, HandleUpper;

    HandleIndex = GDI_HANDLE_GET_INDEX(hObj);
    HandleType = GDI_HANDLE_GET_TYPE(hObj);
    HandleUpper = GDI_HANDLE_GET_UPPER(hObj);

    /* Check that the handle index is valid. */
    if (HandleIndex >= GDI_HANDLE_COUNT)
        return NULL;

    /* Check if we have the requested type */
    if ( (ExpectedType != GDI_OBJECT_TYPE_DONTCARE &&
          HandleType != ExpectedType) ||
         HandleType == 0 )
    {
        DPRINT1("Attempted to lock object 0x%x of wrong type (Handle: 0x%x, requested: 0x%x)\n",
                hObj, HandleType, ExpectedType);
        return NULL;
    }

    Entry = &GdiHandleTable->Entries[HandleIndex];

    ProcessId = (HANDLE)((ULONG_PTR)PsGetCurrentProcessId() & ~1);
    HandleProcessId = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~1);

    /* Check for invalid owner. */
    if (ProcessId != HandleProcessId && HandleProcessId != NULL)
    {
        return NULL;
    }

    /*
     * Prevent the thread from being terminated during the locking process.
     * It would result in undesired effects and inconsistency of the global
     * handle table.
     */

    KeEnterCriticalRegion();

    /*
     * Loop until we either successfully lock the handle entry & object or
     * fail some of the check.
     */

    for (;;)
    {
        /* Lock the handle table entry. */
        LockedProcessId = (HANDLE)((ULONG_PTR)HandleProcessId | 0x1);
        PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId,
                                                        LockedProcessId,
                                                        HandleProcessId);

        if (PrevProcId == HandleProcessId)
        {
            /*
             * We're locking an object that belongs to our process or it's a
             * global object if HandleProcessId is 0 here.
             */

            if ( (Entry->KernelData != NULL) &&
                 (HandleUpper == (Entry->Type << GDI_ENTRY_UPPER_SHIFT)) )
            {
                Object = (POBJ)Entry->KernelData;

#ifdef GDI_DEBUG
                if (_InterlockedIncrement((PLONG)&Object->ulShareCount) == 1)
                {
                    memset(GDIHandleLocker[HandleIndex], 0x00, GDI_STACK_LEVELS * sizeof(ULONG));
                    RtlCaptureStackBackTrace(1, GDI_STACK_LEVELS, (PVOID*)GDIHandleLocker[HandleIndex], NULL);
                }
#else
                _InterlockedIncrement((PLONG)&Object->ulShareCount);
#endif
            }
            else
            {
                /*
                 * Debugging code. Report attempts to lock deleted handles and
                 * locking type mismatches.
                 */
                LockErrorDebugOutput(hObj, Entry, "GDIOBJ_ShareLockObj");
            }

            /* Unlock the handle table entry. */
            (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

            break;
        }
        else
        {
            /*
             * The handle is currently locked, wait some time and try again.
             */

            DelayExecution();
            continue;
        }
    }

    KeLeaveCriticalRegion();

    return Object;
}


/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param Object 	Object pointer (as returned by GDIOBJ_LockObj).
 */
VOID INTERNAL_CALL
GDIOBJ_UnlockObjByPtr(POBJ Object)
{
    if (_InterlockedDecrement((PLONG)&Object->cExclusiveLock) < 0)
    {
        DPRINT1("Trying to unlock non-existant object\n");
    }
}

VOID INTERNAL_CALL
GDIOBJ_ShareUnlockObjByPtr(POBJ Object)
{
    if (_InterlockedDecrement((PLONG)&Object->ulShareCount) < 0)
    {
        DPRINT1("Trying to unlock non-existant object\n");
    }
}

BOOL INTERNAL_CALL
GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle)
{
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId;
    BOOL Ret;

    DPRINT("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);

    if (!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
    {
        ProcessId = PsGetCurrentProcessId();

        Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, ObjectHandle);
        Ret = Entry->KernelData != NULL &&
              (Entry->Type & ~GDI_ENTRY_REUSE_MASK) != 0 &&
              (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) == ProcessId;

        return Ret;
    }

    return FALSE;
}

BOOL INTERNAL_CALL
GDIOBJ_ConvertToStockObj(HGDIOBJ *phObj)
{
    /*
     * FIXME !!!!! THIS FUNCTION NEEDS TO BE FIXED - IT IS NOT SAFE WHEN OTHER THREADS
     *             MIGHT ATTEMPT TO LOCK THE OBJECT DURING THIS CALL!!!
     */
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, LockedProcessId, PrevProcId;
    PW32THREAD Thread;
    HGDIOBJ hObj;

    GDIDBG_INITLOOPTRACE();

    ASSERT(phObj);
    hObj = *phObj;

    DPRINT("GDIOBJ_ConvertToStockObj: hObj: 0x%08x\n", hObj);

    Thread = PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(hObj))
    {
        ProcessId = PsGetCurrentProcessId();
        LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

        Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, hObj);

LockHandle:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, ProcessId);
        if (PrevProcId == ProcessId)
        {
            LONG NewType, PrevType, OldType;

            /* we're locking an object that belongs to our process. First calculate
               the new object type including the stock object flag and then try to
               exchange it.*/
            /* On Windows the higher 16 bit of the type field don't contain the
               full type from the handle, but the base type.
               (type = BRSUH, PEN, EXTPEN, basetype = BRUSH) */
            OldType = ((ULONG)hObj & GDI_HANDLE_BASETYPE_MASK) | ((ULONG)hObj >> GDI_ENTRY_UPPER_SHIFT);
            /* We are currently not using bits 24..31 (flags) of the type field, but for compatibility
               we copy them as we can't get them from the handle */
            OldType |= Entry->Type & GDI_ENTRY_FLAGS_MASK;

            /* As the object should be a stock object, set it's flag, but only in the lower 16 bits */
            NewType = OldType | GDI_ENTRY_STOCK_MASK;

            /* Try to exchange the type field - but only if the old (previous type) matches! */
            PrevType = _InterlockedCompareExchange(&Entry->Type, NewType, OldType);
            if (PrevType == OldType && Entry->KernelData != NULL)
            {
                PW32THREAD PrevThread;
                POBJ Object;

                /* We successfully set the stock object flag.
                   KernelData should never be NULL here!!! */
                ASSERT(Entry->KernelData);

                Object = Entry->KernelData;

                PrevThread = Object->Tid;
                if (Object->cExclusiveLock == 0 || PrevThread == Thread)
                {
                    /* dereference the process' object counter */
                    if (PrevProcId != GDI_GLOBAL_PROCESS)
                    {
                        PEPROCESS OldProcess;
                        PW32PROCESS W32Process;
                        NTSTATUS Status;

                        /* FIXME */
                        Status = PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)PrevProcId & ~0x1), &OldProcess);
                        if (NT_SUCCESS(Status))
                        {
                            W32Process = (PW32PROCESS)OldProcess->Win32Process;
                            if (W32Process != NULL)
                            {
                                _InterlockedDecrement(&W32Process->GDIObjects);
                            }
                            ObDereferenceObject(OldProcess);
                        }
                    }

                    /* remove the process id lock and make it global */
                    (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, GDI_GLOBAL_PROCESS);

                    hObj = (HGDIOBJ)((ULONG)(hObj) | GDI_HANDLE_STOCK_MASK);
                    *phObj = hObj;

                    /* we're done, successfully converted the object */
                    return TRUE;
                }
                else
                {
                    GDIDBG_TRACELOOP(hObj, PrevThread, Thread);

                    /* WTF?! The object is already locked by a different thread!
                       Release the lock, wait a bit and try again!
                       FIXME - we should give up after some time unless we want to wait forever! */
                    (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

                    DelayExecution();
                    goto LockHandle;
                }
            }
            else
            {
                DPRINT1("Attempted to convert object 0x%x that is deleted! Should never get here!!!\n", hObj);
                DPRINT1("OldType = 0x%x, Entry->Type = 0x%x, NewType = 0x%x, Entry->KernelData = 0x%x\n", OldType, Entry->Type, NewType, Entry->KernelData);
            }
        }
        else if (PrevProcId == LockedProcessId)
        {
            GDIDBG_TRACELOOP(hObj, PrevProcId, ProcessId);

            /* the object is currently locked, wait some time and try again.
               FIXME - we shouldn't loop forever! Give up after some time! */
            DelayExecution();
            /* try again */
            goto LockHandle;
        }
        else
        {
            DPRINT1("Attempted to convert invalid handle: 0x%x\n", hObj);
        }
    }

    return FALSE;
}

void INTERNAL_CALL
GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, LockedProcessId, PrevProcId;
    PW32THREAD Thread;

    GDIDBG_INITLOOPTRACE();

    DPRINT("GDIOBJ_SetOwnership: hObj: 0x%x, NewProcess: 0x%x\n", ObjectHandle, (NewOwner ? PsGetProcessId(NewOwner) : 0));

    Thread = PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
    {
        ProcessId = PsGetCurrentProcessId();
        LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

        Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, ObjectHandle);

LockHandle:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        PrevProcId = _InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, ProcessId, LockedProcessId);
        if (PrevProcId == ProcessId)
        {
            PW32THREAD PrevThread;

            if ((Entry->Type & ~GDI_ENTRY_REUSE_MASK) != 0 && Entry->KernelData != NULL)
            {
                POBJ Object = Entry->KernelData;

                PrevThread = Object->Tid;
                if (Object->cExclusiveLock == 0 || PrevThread == Thread)
                {
                    PEPROCESS OldProcess;
                    PW32PROCESS W32Process;
                    NTSTATUS Status;

                    /* dereference the process' object counter */
                    /* FIXME */
                    if ((ULONG_PTR)PrevProcId & ~0x1)
                    {
                        Status = PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)PrevProcId & ~0x1), &OldProcess);
                        if (NT_SUCCESS(Status))
                        {
                            W32Process = (PW32PROCESS)OldProcess->Win32Process;
                            if (W32Process != NULL)
                            {
                                _InterlockedDecrement(&W32Process->GDIObjects);
                            }
                            ObDereferenceObject(OldProcess);
                        }
                    }

                    if (NewOwner != NULL)
                    {
                        ProcessId = PsGetProcessId(NewOwner);

                        /* Increase the new process' object counter */
                        W32Process = (PW32PROCESS)NewOwner->Win32Process;
                        if (W32Process != NULL)
                        {
                            _InterlockedIncrement(&W32Process->GDIObjects);
                        }
                    }
                    else
                        ProcessId = 0;

                    /* remove the process id lock and change it to the new process id */
                    (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, ProcessId);

                    /* we're done! */
                    return;
                }
                else
                {
                    GDIDBG_TRACELOOP(ObjectHandle, PrevThread, Thread);

                    /* WTF?! The object is already locked by a different thread!
                       Release the lock, wait a bit and try again! DO reset the pid lock
                       so we make sure we don't access invalid memory in case the object is
                       being deleted in the meantime (because we don't have aquired a reference
                       at this point).
                       FIXME - we should give up after some time unless we want to wait forever! */
                    (void)_InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

                    DelayExecution();
                    goto LockHandle;
                }
            }
            else
            {
                DPRINT1("Attempted to change ownership of an object 0x%x currently being destroyed!!!\n", ObjectHandle);
                DPRINT1("Entry->Type = 0x%lx, Entry->KernelData = 0x%p\n", Entry->Type, Entry->KernelData);
            }
        }
        else if (PrevProcId == LockedProcessId)
        {
            GDIDBG_TRACELOOP(ObjectHandle, PrevProcId, ProcessId);

            /* the object is currently locked, wait some time and try again.
               FIXME - we shouldn't loop forever! Give up after some time! */
            DelayExecution();
            /* try again */
            goto LockHandle;
        }
        else if (((ULONG_PTR)PrevProcId & ~0x1) == 0)
        {
            /* allow changing ownership of global objects */
            ProcessId = NULL;
            LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);
            goto LockHandle;
        }
        else if ((HANDLE)((ULONG_PTR)PrevProcId & ~0x1) != PsGetCurrentProcessId())
        {
            DPRINT1("Attempted to change ownership of object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", ObjectHandle, (ULONG_PTR)PrevProcId & ~0x1, PsGetCurrentProcessId());
        }
        else
        {
            DPRINT1("Attempted to change owner of invalid handle: 0x%x\n", ObjectHandle);
        }
    }
}

void INTERNAL_CALL
GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
    PGDI_TABLE_ENTRY FromEntry;
    PW32THREAD Thread;
    HANDLE FromProcessId, FromLockedProcessId, FromPrevProcId;

    GDIDBG_INITLOOPTRACE();

    DPRINT("GDIOBJ_CopyOwnership: from: 0x%x, to: 0x%x\n", CopyFrom, CopyTo);

    Thread = PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(CopyFrom) && !GDI_HANDLE_IS_STOCKOBJ(CopyTo))
    {
        FromEntry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, CopyFrom);

        FromProcessId = (HANDLE)((ULONG_PTR)FromEntry->ProcessId & ~0x1);
        FromLockedProcessId = (HANDLE)((ULONG_PTR)FromProcessId | 0x1);

LockHandleFrom:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        FromPrevProcId = _InterlockedCompareExchangePointer((PVOID*)&FromEntry->ProcessId, FromProcessId, FromLockedProcessId);
        if (FromPrevProcId == FromProcessId)
        {
            PW32THREAD PrevThread;
            POBJ Object;

            if ((FromEntry->Type & ~GDI_ENTRY_REUSE_MASK) != 0 && FromEntry->KernelData != NULL)
            {
                Object = FromEntry->KernelData;

                /* save the pointer to the calling thread so we know it was this thread
                   that locked the object */
                PrevThread = Object->Tid;
                if (Object->cExclusiveLock == 0 || PrevThread == Thread)
                {
                    /* now let's change the ownership of the target object */

                    if (((ULONG_PTR)FromPrevProcId & ~0x1) != 0)
                    {
                        PEPROCESS ProcessTo;
                        /* FIXME */
                        if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1), &ProcessTo)))
                        {
                            GDIOBJ_SetOwnership(CopyTo, ProcessTo);
                            ObDereferenceObject(ProcessTo);
                        }
                    }
                    else
                    {
                        /* mark the object as global */
                        GDIOBJ_SetOwnership(CopyTo, NULL);
                    }

                    (void)_InterlockedExchangePointer((PVOID*)&FromEntry->ProcessId, FromPrevProcId);
                }
                else
                {
                    GDIDBG_TRACELOOP(CopyFrom, PrevThread, Thread);

                    /* WTF?! The object is already locked by a different thread!
                       Release the lock, wait a bit and try again! DO reset the pid lock
                       so we make sure we don't access invalid memory in case the object is
                       being deleted in the meantime (because we don't have aquired a reference
                       at this point).
                       FIXME - we should give up after some time unless we want to wait forever! */
                    (void)_InterlockedExchangePointer((PVOID*)&FromEntry->ProcessId, FromPrevProcId);

                    DelayExecution();
                    goto LockHandleFrom;
                }
            }
            else
            {
                DPRINT1("Attempted to copy ownership from an object 0x%x currently being destroyed!!!\n", CopyFrom);
            }
        }
        else if (FromPrevProcId == FromLockedProcessId)
        {
            GDIDBG_TRACELOOP(CopyFrom, FromPrevProcId, FromProcessId);

            /* the object is currently locked, wait some time and try again.
               FIXME - we shouldn't loop forever! Give up after some time! */
            DelayExecution();
            /* try again */
            goto LockHandleFrom;
        }
        else if ((HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1) != PsGetCurrentProcessId())
        {
            /* FIXME - should we really allow copying ownership from objects that we don't even own? */
            DPRINT1("WARNING! Changing copying ownership of object 0x%x (pid: 0x%x) to pid 0x%x!!!\n", CopyFrom, (ULONG_PTR)FromPrevProcId & ~0x1, PsGetCurrentProcessId());
            FromProcessId = (HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1);
            FromLockedProcessId = (HANDLE)((ULONG_PTR)FromProcessId | 0x1);
            goto LockHandleFrom;
        }
        else
        {
            DPRINT1("Attempted to copy ownership from invalid handle: 0x%x\n", CopyFrom);
        }
    }
}

PVOID INTERNAL_CALL
GDI_MapHandleTable(PSECTION_OBJECT SectionObject, PEPROCESS Process)
{
    PVOID MappedView = NULL;
    NTSTATUS Status;
    LARGE_INTEGER Offset;
    ULONG ViewSize = sizeof(GDI_HANDLE_TABLE);

    Offset.QuadPart = 0;

    ASSERT(SectionObject != NULL);
    ASSERT(Process != NULL);

    Status = MmMapViewOfSection(SectionObject,
                                Process,
                                &MappedView,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_READONLY);

    if (!NT_SUCCESS(Status))
        return NULL;

    return MappedView;
}

/** PUBLIC FUNCTIONS **********************************************************/

W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    IN ULONG ulType
)
{
// ATM we use DC object for KernelData. This is wrong.
// The real type consists of BASEOBJECT and a pointer.
// The UserData is set in user mode, so it is always NULL.
// HANDLE should be HGDIOBJ
//
    POBJ pObject;
    HANDLE handle;

    /* Mask out everything that would change the type in a wrong manner */
    ulType &= (GDI_HANDLE_TYPE_MASK & ~GDI_HANDLE_BASETYPE_MASK);

    /* Allocate a new object */
    pObject = GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_CLIOBJ | ulType);
    if (!pObject)
    {
        return NULL;
    }

    /* get the handle */
    handle = pObject->hHmgr;

    /* Unlock it */
    GDIOBJ_UnlockObjByPtr(pObject);

    return handle;
}

W32KAPI
BOOL
APIENTRY
NtGdiDeleteClientObj(
    IN HANDLE h
)
{
    /* We first need to get the real type from the handle */
    ULONG type = GDI_HANDLE_GET_TYPE(h);

    /* Check if it's really a CLIENTOBJ */
    if ((type & GDI_HANDLE_BASETYPE_MASK) != GDILoObjType_LO_CLIENTOBJ_TYPE)
    {
        /* FIXME: SetLastError? */
        return FALSE;
    }
    return GDIOBJ_FreeObjByHandle(h, type);
}

INT
FASTCALL
IntGdiGetObject(IN HANDLE Handle,
                IN INT cbCount,
                IN LPVOID lpBuffer)
{
  PVOID pGdiObject;
  INT Result = 0;
  DWORD dwObjectType;

  pGdiObject = GDIOBJ_LockObj(Handle, GDI_OBJECT_TYPE_DONTCARE);
  if (!pGdiObject)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  dwObjectType = GDIOBJ_GetObjectType(Handle);
  switch (dwObjectType)
    {
      case GDI_OBJECT_TYPE_PEN:
      case GDI_OBJECT_TYPE_EXTPEN:
        Result = PEN_GetObject((PGDIBRUSHOBJ) pGdiObject, cbCount, (PLOGPEN) lpBuffer); // IntGdiCreatePenIndirect
        break;

      case GDI_OBJECT_TYPE_BRUSH:
        Result = BRUSH_GetObject((PGDIBRUSHOBJ ) pGdiObject, cbCount, (LPLOGBRUSH)lpBuffer);
        break;

      case GDI_OBJECT_TYPE_BITMAP:
        Result = BITMAP_GetObject((BITMAPOBJ *) pGdiObject, cbCount, lpBuffer);
        break;
      case GDI_OBJECT_TYPE_FONT:
        Result = FontGetObject((PTEXTOBJ) pGdiObject, cbCount, lpBuffer);
#if 0
        // Fix the LOGFONT structure for the stock fonts
        if (FIRST_STOCK_HANDLE <= Handle && Handle <= LAST_STOCK_HANDLE)
          {
            FixStockFontSizeW(Handle, cbCount, lpBuffer);
          }
#endif
        break;

      case GDI_OBJECT_TYPE_PALETTE:
        Result = PALETTE_GetObject((PPALGDI) pGdiObject, cbCount, lpBuffer);
        break;

      default:
        DPRINT1("GDI object type 0x%08x not implemented\n", dwObjectType);
        break;
    }

  GDIOBJ_UnlockObjByPtr(pGdiObject);

  return Result;
}

W32KAPI
INT
APIENTRY
NtGdiExtGetObjectW(IN HANDLE hGdiObj,
                   IN INT cbCount,
                   OUT LPVOID lpBuffer)
{
    INT iRetCount = 0;
    INT cbCopyCount;
    union
    {
        BITMAP bitmap;
        DIBSECTION dibsection;
        LOGPEN logpen;
        LOGBRUSH logbrush;
        LOGFONTW logfontw;
        EXTLOGFONTW extlogfontw;
        ENUMLOGFONTEXDVW enumlogfontexdvw;
    } Object;

    // Normalize to the largest supported object size
    cbCount = min((UINT)cbCount, sizeof(Object));

    // Now do the actual call
    iRetCount = IntGdiGetObject(hGdiObj, cbCount, lpBuffer ? &Object : NULL);
    cbCopyCount = min((UINT)cbCount, (UINT)iRetCount);

    // Make sure we have a buffer and a copy size
    if ((cbCopyCount) && (lpBuffer))
    {
        // Enter SEH for buffer transfer
        _SEH_TRY
        {
            // Probe the buffer and copy it
            ProbeForWrite(lpBuffer, cbCopyCount, sizeof(WORD));
            RtlCopyMemory(lpBuffer, &Object, cbCopyCount);
        }
        _SEH_HANDLE
        {
            // Clear the return value.
            // Do *NOT* set last error here!
            iRetCount = 0;
        }
        _SEH_END;
    }
    // Return the count
    return iRetCount;
}

/* EOF */
