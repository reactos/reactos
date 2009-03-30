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

#define GDI_ENTRY_TO_INDEX(ht, e)                                              \
  (((ULONG_PTR)(e) - (ULONG_PTR)&((ht)->Entries[0])) / sizeof(GDI_TABLE_ENTRY))
#define GDI_HANDLE_GET_ENTRY(HandleTable, h)                                   \
  (&(HandleTable)->Entries[GDI_HANDLE_GET_INDEX((h))])

/* apparently the first 10 entries are never used in windows as they are empty */
#define RESERVE_ENTRIES_COUNT 10

#define BASE_OBJTYPE_COUNT 32

#define DelayExecution() \
  DPRINT("%s:%i: Delay\n", __FILE__, __LINE__); \
  KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay)

/* static */ /* FIXME: -fno-unit-at-a-time breaks this */
BOOL INTERNAL_CALL GDI_CleanupDummy(PVOID ObjectBody);

/** GLOBALS *******************************************************************/

typedef struct
{
    BOOL bUseLookaside;
    ULONG_PTR ulBodySize;
    ULONG Tag;
    GDICLEANUPPROC CleanupProc;
} OBJ_TYPE_INFO, *POBJ_TYPE_INFO;

static const
OBJ_TYPE_INFO ObjTypeInfo[BASE_OBJTYPE_COUNT] =
{
  {0, 0,                     0,                NULL},             /* 00 reserved entry */
  {1, sizeof(DC),            TAG_DC,           DC_Cleanup},       /* 01 DC */
  {1, 0,                     0,                NULL},             /* 02 UNUSED1 */
  {1, 0,                     0,                NULL},             /* 03 UNUSED2 */
  {1, sizeof(ROSRGNDATA),    TAG_REGION,       REGION_Cleanup},   /* 04 RGN */
  {1, sizeof(SURFACE),       TAG_SURFACE,      SURFACE_Cleanup},  /* 05 SURFACE */
  {1, sizeof(CLIENTOBJ),     TAG_CLIENTOBJ,    GDI_CleanupDummy}, /* 06 CLIENTOBJ: METADC,... */
  {1, sizeof(PATH),          TAG_PATH,         GDI_CleanupDummy}, /* 07 PATH */
  {1, sizeof(PALGDI),        TAG_PALETTE,      PALETTE_Cleanup},  /* 08 PAL */
  {1, sizeof(COLORSPACE),    TAG_ICMLCS,       GDI_CleanupDummy}, /* 09 ICMLCS, */
  {1, sizeof(TEXTOBJ),       TAG_LFONT,        GDI_CleanupDummy}, /* 0a LFONT */
  {0, 0,                     TAG_RFONT,        NULL},             /* 0b RFONT, unused */
  {0, 0,                     TAG_PFE,          NULL},             /* 0c PFE, unused */
  {0, 0,                     TAG_PFT,          NULL},             /* 0d PFT, unused */
  {0, sizeof(GDICLRXFORM),   TAG_ICMCXF,       GDI_CleanupDummy}, /* 0e ICMCXF, */
  {0, 0,                     TAG_SPRITE,       NULL},             /* 0f SPRITE, unused */
  {1, sizeof(BRUSH),         TAG_BRUSH,        BRUSH_Cleanup},    /* 10 BRUSH, PEN, EXTPEN */
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
  {0, sizeof(EDRIVEROBJ),    TAG_DRVOBJ,       DRIVEROBJ_Cleanup},/* 1c DRVOBJ */
  {0, 0,                     TAG_DCIOBJ,       NULL},             /* 1d DCIOBJ, unused */
  {0, 0,                     TAG_SPOOL,        NULL},             /* 1e SPOOL, unused */
  {0, 0,                     0,                NULL},             /* 1f reserved entry */
};

static LARGE_INTEGER ShortDelay;

/** DEBUGGING *****************************************************************/
//#define GDI_DEBUG
#include "gdidbg.c"

/** INTERNAL FUNCTIONS ********************************************************/

/*
 * Dummy GDI Cleanup Callback
 */
/* static */ /* FIXME: -fno-unit-at-a-time breaks this */
BOOL INTERNAL_CALL
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
        GDIDBG_TRACEDELETER(hObj);
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
    ULONG idxFirst, idxNext, idxPrev;
    PGDI_TABLE_ENTRY pEntry;
    DWORD PrevProcId;

    DPRINT("Enter InterLockedPopFreeEntry\n");

    while (TRUE)
    {
        idxFirst = GdiHandleTable->FirstFree;

        if (!idxFirst)
        {
            /* Increment FirstUnused and get the new index */
            idxFirst = InterlockedIncrement((LONG*)&GdiHandleTable->FirstUnused) - 1;

            /* Check if we have entries left */
            if (idxFirst >= GDI_HANDLE_COUNT)
            {
                DPRINT1("No more gdi handles left!\n");
                return 0;
            }

            /* Return the old index */
            return idxFirst;
        }

        /* Get a pointer to the first free entry */
        pEntry = GdiHandleTable->Entries + idxFirst;

        /* Try to lock the entry */
        PrevProcId = InterlockedCompareExchange((LONG*)&pEntry->ProcessId, 1, 0);
        if (PrevProcId != 0)
        {
            /* The entry was locked or not free, wait and start over */
            DelayExecution();
            continue;
        }

        /* Sanity check: is entry really free? */
        ASSERT(((ULONG_PTR)pEntry->KernelData & ~GDI_HANDLE_INDEX_MASK) == 0);

        /* Try to exchange the FirstFree value */
        idxNext = (ULONG_PTR)pEntry->KernelData;
        idxPrev = InterlockedCompareExchange((LONG*)&GdiHandleTable->FirstFree,
                                             idxNext,
                                             idxFirst);

        /* Unlock the free entry */
        (void)InterlockedExchange((LONG*)&pEntry->ProcessId, 0);

        /* If we succeeded, break out of the loop */
        if (idxPrev == idxFirst)
        {
            break;
        }
    }

    return idxFirst;
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
        pFreeEntry->KernelData = (PVOID)(ULONG_PTR)idxFirstFree;

        idxPrev = InterlockedCompareExchange((LONG*)&GdiHandleTable->FirstFree,
                                             idxToFree,
                                             idxFirstFree);
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
    if (W32Process && W32Process->GDIHandleCount >= 0x2710)
    {
        DPRINT1("Too many objects for process!!!\n");
        GDIDBG_DUMPHANDLETABLE();
        return NULL;
    }

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
        PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, 0);
        if (PrevProcId == NULL)
        {
            PW32THREAD Thread = (PW32THREAD)PsGetCurrentThreadWin32Thread();
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
            (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, CurrentProcessId);

            GDIDBG_CAPTUREALLOCATOR(Index);

            if (W32Process != NULL)
            {
                InterlockedIncrement(&W32Process->GDIHandleCount);
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
 *
 * \bug This function should return VOID and kill the object no matter what...
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
    PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, ProcessId);
    if (PrevProcId == ProcessId)
    {
        if ( (Entry->KernelData != NULL) &&
             ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) == HandleUpper) &&
             ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) == (HandleUpper & GDI_ENTRY_BASETYPE_MASK)) )
        {
            POBJ Object;

            Object = Entry->KernelData;

            if ((Object->cExclusiveLock == 0 ||
                Object->Tid == (PW32THREAD)PsGetCurrentThreadWin32Thread()) &&
                 Object->ulShareCount == 0)
            {
                BOOL Ret;
                PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();

                /* Clear the basetype field so when unlocking the handle it gets finally deleted and increment reuse counter */
                Entry->Type = (Entry->Type + GDI_ENTRY_REUSE_INC) & ~GDI_ENTRY_BASETYPE_MASK;

                /* unlock the handle slot */
                (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, NULL);

                /* push this entry to the free list */
                InterlockedPushFreeEntry(GDI_ENTRY_TO_INDEX(GdiHandleTable, Entry));

                Object->hHmgr = NULL;

                if (W32Process != NULL)
                {
                    InterlockedDecrement(&W32Process->GDIHandleCount);
                }

                /* call the cleanup routine. */
                TypeIndex = GDI_OBJECT_GET_TYPE_INDEX(HandleType);
                Ret = ObjTypeInfo[TypeIndex].CleanupProc(Object);

                /* Now it's time to free the memory */
                GDIOBJ_FreeObj(Object, TypeIndex);

                GDIDBG_CAPTUREDELETER(hObj);
                return Ret;
            }
            else if (Object->ulShareCount != 0)
            {
                DPRINT("Object %p, ulShareCount = %d\n", Object->hHmgr, Object->ulShareCount);
                GDIDBG_TRACECALLER();
                GDIDBG_TRACESHARELOCKER(GDI_HANDLE_GET_INDEX(hObj));
                (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);
                /* Don't wait on shared locks */
                return FALSE;
            }
            else
            {
                /*
                 * The object is currently locked by another thread, so freeing is forbidden!
                 */
                DPRINT1("Object->cExclusiveLock = %d\n", Object->cExclusiveLock);
                GDIDBG_TRACECALLER();
                GDIDBG_TRACELOCKER(GDI_HANDLE_GET_INDEX(hObj));
                (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);
                /* do not assert here for it will call again from dxg.sys it being call twice */

                DelayExecution();
                goto LockHandle;
            }
        }
        else
        {
            LockErrorDebugOutput(hObj, Entry, "GDIOBJ_FreeObj");
            (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);
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
            if ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) == 0)
            {
                DPRINT1("Attempted to free gdi handle 0x%x that is already deleted!\n", hObj);
            }
            else if (((ULONG_PTR)PrevProcId & ~0x1) == 0)
            {
                DPRINT1("Attempted to free global gdi handle 0x%x, caller needs to get ownership first!!!\n", hObj);
            }
            else
            {
                DPRINT1("Attempted to free foreign handle: 0x%x Owner: 0x%x from Caller: 0x%x\n", hObj, (ULONG_PTR)PrevProcId & ~0x1, (ULONG_PTR)ProcessId & ~0x1);
            }
            DPRINT1("Type = 0x%lx, KernelData = 0x%p, ProcessId = 0x%p\n", Entry->Type, Entry->KernelData, Entry->ProcessId);
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
    if ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0)
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
GreDeleteObject(HGDIOBJ hObject)
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

VOID
FASTCALL
IntDeleteHandlesForProcess(struct _EPROCESS *Process, ULONG ObjectType)
{
    PGDI_TABLE_ENTRY Entry, End;
    ULONG Index = RESERVE_ENTRIES_COUNT;
    HANDLE ProcId;
    PW32PROCESS W32Process;

    W32Process = (PW32PROCESS)Process->Win32Process;
    ASSERT(W32Process);

    if (W32Process->GDIHandleCount > 0)
    {
       ProcId = Process->UniqueProcessId;

    /* FIXME - Instead of building the handle here and delete it using GDIOBJ_FreeObj
               we should delete it directly here! */

        End = &GdiHandleTable->Entries[GDI_HANDLE_COUNT];
        for (Entry = &GdiHandleTable->Entries[RESERVE_ENTRIES_COUNT];
             Entry != End;
             Entry++, Index++)
        {
            /* ignore the lock bit */
            if ( (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) == ProcId)
            {
                if ( (Entry->Type & GDI_ENTRY_BASETYPE_MASK) == ObjectType ||
                     ObjectType == GDI_OBJECT_TYPE_DONTCARE)
                {
                    HGDIOBJ ObjectHandle;

                    /* Create the object handle for the entry, the lower(!) 16 bit of the
                       Type field includes the type of the object including the stock
                       object flag - but since stock objects don't have a process id we can
                       simply ignore this fact here. */
                    ObjectHandle = (HGDIOBJ)(Index | (Entry->Type << GDI_ENTRY_UPPER_SHIFT));

                    if (!GDIOBJ_FreeObjByHandle(ObjectHandle, GDI_OBJECT_TYPE_DONTCARE))
                    {
                        DPRINT1("Failed to delete object %p!\n", ObjectHandle);
                    }

                    if (W32Process->GDIHandleCount == 0)
                    {
                        /* there are no more gdi handles for this process, bail */
                        break;
                    }
                }
            }
        }
    }
}


/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL INTERNAL_CALL
GDI_CleanupForProcess(struct _EPROCESS *Process)
{
    PEPROCESS CurrentProcess;
    PW32PROCESS W32Process;

    DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Process->UniqueProcessId);
    CurrentProcess = PsGetCurrentProcess();
    if (CurrentProcess != Process)
    {
        KeAttachProcess(&Process->Pcb);
    }

    W32Process = (PW32PROCESS)CurrentProcess->Win32Process;

    /* Delete objects. Begin with types that are not referenced by other types */
    IntDeleteHandlesForProcess(Process, GDILoObjType_LO_DC_TYPE);
    IntDeleteHandlesForProcess(Process, GDILoObjType_LO_BRUSH_TYPE);
    IntDeleteHandlesForProcess(Process, GDILoObjType_LO_BITMAP_TYPE);

    /* Finally finish with what's left */
    IntDeleteHandlesForProcess(Process, GDI_OBJECT_TYPE_DONTCARE);

    if (CurrentProcess != Process)
    {
        KeDetachProcess();
    }

#ifdef GDI_DEBUG
	GdiDbgHTIntegrityCheck();
#endif

    DPRINT("Completed cleanup for process %d\n", Process->UniqueProcessId);
    if (W32Process->GDIHandleCount > 0)
    {
        DPRINT1("Leaking %d handles!\n", W32Process->GDIHandleCount);
    }

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
        GDIDBG_TRACEALLOCATOR(hObj);
        GDIDBG_TRACEDELETER(hObj);
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
        PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId,
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
                PW32THREAD Thread = (PW32THREAD)PsGetCurrentThreadWin32Thread();
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
                        (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

                        DelayExecution();
                        continue;
                    }
                    InterlockedIncrement((PLONG)&Object->cExclusiveLock);
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
            (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

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
        PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId,
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

GDIDBG_CAPTURESHARELOCKER(HandleIndex);
#ifdef GDI_DEBUG3
                if (InterlockedIncrement((PLONG)&Object->ulShareCount) == 1)
                {
                    memset(GDIHandleLocker[HandleIndex], 0x00, GDI_STACK_LEVELS * sizeof(ULONG));
                    RtlCaptureStackBackTrace(1, GDI_STACK_LEVELS, (PVOID*)GDIHandleShareLocker[HandleIndex], NULL);
                }
#else
                InterlockedIncrement((PLONG)&Object->ulShareCount);
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
            (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

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
              (Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 &&
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

    Thread = (PW32THREAD)PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(hObj))
    {
        ProcessId = PsGetCurrentProcessId();
        LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

        Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, hObj);

LockHandle:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, LockedProcessId, ProcessId);
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
            PrevType = InterlockedCompareExchange(&Entry->Type, NewType, OldType);
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
                                InterlockedDecrement(&W32Process->GDIHandleCount);
                            }
                            ObDereferenceObject(OldProcess);
                        }
                    }

                    hObj = (HGDIOBJ)((ULONG)(hObj) | GDI_HANDLE_STOCK_MASK);
                    *phObj = hObj;
                    Object->hHmgr = hObj;

                    /* remove the process id lock and make it global */
                    (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, GDI_GLOBAL_PROCESS);

                    /* we're done, successfully converted the object */
                    return TRUE;
                }
                else
                {
                    GDIDBG_TRACELOOP(hObj, PrevThread, Thread);

                    /* WTF?! The object is already locked by a different thread!
                       Release the lock, wait a bit and try again!
                       FIXME - we should give up after some time unless we want to wait forever! */
                    (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

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

BOOL INTERNAL_CALL
GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
    PGDI_TABLE_ENTRY Entry;
    HANDLE ProcessId, LockedProcessId, PrevProcId;
    PW32THREAD Thread;
    BOOL Ret = TRUE;

    GDIDBG_INITLOOPTRACE();

    DPRINT("GDIOBJ_SetOwnership: hObj: 0x%x, NewProcess: 0x%x\n", ObjectHandle, (NewOwner ? PsGetProcessId(NewOwner) : 0));

    Thread = (PW32THREAD)PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
    {
        ProcessId = PsGetCurrentProcessId();
        LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

        Entry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, ObjectHandle);

LockHandle:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        PrevProcId = InterlockedCompareExchangePointer((PVOID*)&Entry->ProcessId, ProcessId, LockedProcessId);
        if (PrevProcId == ProcessId)
        {
            PW32THREAD PrevThread;

            if ((Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0)
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
                                InterlockedDecrement(&W32Process->GDIHandleCount);
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
                            InterlockedIncrement(&W32Process->GDIHandleCount);
                        }
                    }
                    else
                        ProcessId = 0;

                    /* remove the process id lock and change it to the new process id */
                    (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, ProcessId);

                    /* we're done! */
                    return Ret;
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
                    (void)InterlockedExchangePointer((PVOID*)&Entry->ProcessId, PrevProcId);

                    DelayExecution();
                    goto LockHandle;
                }
            }
            else
            {
                DPRINT1("Attempted to change ownership of an object 0x%x currently being destroyed!!!\n", ObjectHandle);
                DPRINT1("Entry->Type = 0x%lx, Entry->KernelData = 0x%p\n", Entry->Type, Entry->KernelData);
                Ret = FALSE;
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
            Ret = FALSE;
        }
        else
        {
            DPRINT1("Attempted to change owner of invalid handle: 0x%x\n", ObjectHandle);
            Ret = FALSE;
        }
    }
    return Ret;
}

BOOL INTERNAL_CALL
GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
    PGDI_TABLE_ENTRY FromEntry;
    PW32THREAD Thread;
    HANDLE FromProcessId, FromLockedProcessId, FromPrevProcId;
    BOOL Ret = TRUE;

    GDIDBG_INITLOOPTRACE();

    DPRINT("GDIOBJ_CopyOwnership: from: 0x%x, to: 0x%x\n", CopyFrom, CopyTo);

    Thread = (PW32THREAD)PsGetCurrentThreadWin32Thread();

    if (!GDI_HANDLE_IS_STOCKOBJ(CopyFrom) && !GDI_HANDLE_IS_STOCKOBJ(CopyTo))
    {
        FromEntry = GDI_HANDLE_GET_ENTRY(GdiHandleTable, CopyFrom);

        FromProcessId = (HANDLE)((ULONG_PTR)FromEntry->ProcessId & ~0x1);
        FromLockedProcessId = (HANDLE)((ULONG_PTR)FromProcessId | 0x1);

LockHandleFrom:
        /* lock the object, we must not convert stock objects, so don't check!!! */
        FromPrevProcId = InterlockedCompareExchangePointer((PVOID*)&FromEntry->ProcessId, FromProcessId, FromLockedProcessId);
        if (FromPrevProcId == FromProcessId)
        {
            PW32THREAD PrevThread;
            POBJ Object;

            if ((FromEntry->Type & GDI_ENTRY_BASETYPE_MASK) != 0)
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

                    (void)InterlockedExchangePointer((PVOID*)&FromEntry->ProcessId, FromPrevProcId);
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
                    (void)InterlockedExchangePointer((PVOID*)&FromEntry->ProcessId, FromPrevProcId);

                    DelayExecution();
                    goto LockHandleFrom;
                }
            }
            else
            {
                DPRINT1("Attempted to copy ownership from an object 0x%x currently being destroyed!!!\n", CopyFrom);
                Ret = FALSE;
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
            Ret = FALSE;
        }
    }
    return Ret;
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

/*
  Since Brush/Pen and Region objects are sharable,,, we can just use
  UserHeapAlloc to allocate the small attribute objects.

   Example Allocating:

    // Save Kernel Space Pointer
    (PBRUSH)->pBrushAttr = IntGdiAllocObjAttr(GDIObjType_BRUSH_TYPE);

    // Kernel Space to User Space Pointer
    (PGDI_TABLE_ENTRY)->UserData = pBrushAttr;
    // Gdi will adjust for heap delta.

   Example Freeing:

    (PGDI_TABLE_ENTRY)->UserData = NULL;      // Zero the user ptr.
    UserHeapFree((PBRUSH)->pBrushAttr); // Free from kernel ptr.
    (PBRUSH)->pBrushAttr = NULL;

   Notes:
    Testing with DC_ATTR works but has drawing difficulties.
    Base on observation, (Over looking the obvious) we need to supply heap delta
    to user space gdi. Now, with testing, looks all normal.

 */
PVOID
FASTCALL
IntGdiAllocObjAttr(GDIOBJTYPE Type)
{
  PVOID pMemAttr = NULL;

  switch( Type )
  {
     case GDIObjType_DC_TYPE:
        pMemAttr = UserHeapAlloc(sizeof(DC_ATTR));
        if (pMemAttr) RtlZeroMemory(pMemAttr, sizeof(DC_ATTR));
        break;
     case GDIObjType_RGN_TYPE:
        pMemAttr = UserHeapAlloc(sizeof(RGN_ATTR));
        if (pMemAttr) RtlZeroMemory(pMemAttr, sizeof(RGN_ATTR));
        break;
     case GDIObjType_BRUSH_TYPE:
        pMemAttr = UserHeapAlloc(sizeof(BRUSH_ATTR));
        if (pMemAttr) RtlZeroMemory(pMemAttr, sizeof(BRUSH_ATTR));
        break;
     default:
        break;
  }
  return pMemAttr;
}


BOOL
FASTCALL
IntGdiSetBrushOwner(PBRUSH pbr, DWORD OwnerMask)
{
  HBRUSH hBR;
  PEPROCESS Owner = NULL;
  PGDI_TABLE_ENTRY pEntry = NULL;

  if (!pbr) return FALSE;

  hBR = pbr->BaseObject.hHmgr;

  if (!hBR || (GDI_HANDLE_GET_TYPE(hBR) != GDI_OBJECT_TYPE_BRUSH))
     return FALSE;
  else
  {
     INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hBR);
     pEntry = &GdiHandleTable->Entries[Index];
  }

  if (pbr->flAttrs & GDIBRUSH_IS_GLOBAL)
  {
     GDIOBJ_ShareUnlockObjByPtr((POBJ)pbr);
     return TRUE;
  }

  if ((OwnerMask == GDI_OBJ_HMGR_PUBLIC) || OwnerMask == GDI_OBJ_HMGR_NONE)
  {
     // Set this Brush to inaccessible mode and to an Owner of NONE.
//     if (OwnerMask == GDI_OBJ_HMGR_NONE) Owner = OwnerMask;

     if (!GDIOBJ_SetOwnership((HGDIOBJ) hBR, Owner))
        return FALSE;

     // Deny user access to User Data.
     pEntry->UserData = NULL; // This hBR is inaccessible!
  }

  if (OwnerMask == GDI_OBJ_HMGR_POWNED)
  {
     if (!GDIOBJ_SetOwnership((HGDIOBJ) hBR, PsGetCurrentProcess() ))
        return FALSE;

     // Allow user access to User Data.
     pEntry->UserData = pbr->pBrushAttr;
  }
  return TRUE;
}


BOOL
FASTCALL
IntGdiSetDCOwnerEx( HDC hDC, DWORD OwnerMask, BOOL NoSetBrush)
{
  PDC pDC;
  BOOL Ret = FALSE;

  if (!hDC || (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)) return FALSE;

  if ((OwnerMask == GDI_OBJ_HMGR_PUBLIC) || OwnerMask == GDI_OBJ_HMGR_NONE)
  {
     pDC = DC_LockDc ( hDC );
     MmCopyFromCaller(&pDC->dcattr, pDC->pdcattr, sizeof(DC_ATTR));
     DC_UnlockDc( pDC );

     DC_FreeDcAttr( hDC );         // Free the dcattr!

     if (!DC_SetOwnership( hDC, NULL )) // This hDC is inaccessible!
        return Ret;
  }

  if (OwnerMask == GDI_OBJ_HMGR_POWNED)
  {
     pDC = DC_LockDc ( hDC );
     ASSERT(pDC->pdcattr == &pDC->dcattr);
     DC_UnlockDc( pDC );

     if (!DC_SetOwnership( hDC, PsGetCurrentProcess() )) return Ret;

     DC_AllocateDcAttr( hDC );      // Allocate new dcattr

     DCU_SynchDcAttrtoUser( hDC );  // Copy data from dc to dcattr
  }

  if ((OwnerMask != GDI_OBJ_HMGR_NONE) && !NoSetBrush)
  {
     pDC = DC_LockDc ( hDC );
     if (IntGdiSetBrushOwner((PBRUSH)pDC->dclevel.pbrFill, OwnerMask))
         IntGdiSetBrushOwner((PBRUSH)pDC->dclevel.pbrLine, OwnerMask);
     DC_UnlockDc( pDC );
  }
  return TRUE;
}

INT
FASTCALL
GreGetObjectOwner(HGDIOBJ Handle, GDIOBJTYPE ObjType)
{
  INT Ret = GDI_OBJ_HMGR_RESTRICTED;

  if ( GDI_HANDLE_GET_INDEX(Handle) < GDI_HANDLE_COUNT )
  {
     PGDI_TABLE_ENTRY pEntry = &GdiHandleTable->Entries[GDI_HANDLE_GET_INDEX(Handle)];

     if (pEntry->ObjectType == ObjType)
     {
        if (pEntry->FullUnique == (GDI_HANDLE_GET_UPPER(Handle) >> GDI_ENTRY_UPPER_SHIFT))
           Ret = pEntry->ProcessId & ~1;
     }
  }
  return Ret;
}


W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    IN ULONG ulType
)
{
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
        Result = PEN_GetObject((PBRUSH) pGdiObject, cbCount, (PLOGPEN) lpBuffer); // IntGdiCreatePenIndirect
        break;

      case GDI_OBJECT_TYPE_BRUSH:
        Result = BRUSH_GetObject((PBRUSH ) pGdiObject, cbCount, (LPLOGBRUSH)lpBuffer);
        break;

      case GDI_OBJECT_TYPE_BITMAP:
        Result = BITMAP_GetObject((SURFACE *) pGdiObject, cbCount, lpBuffer);
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
        _SEH2_TRY
        {
            // Probe the buffer and copy it
            ProbeForWrite(lpBuffer, cbCopyCount, sizeof(WORD));
            RtlCopyMemory(lpBuffer, &Object, cbCopyCount);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            // Clear the return value.
            // Do *NOT* set last error here!
            iRetCount = 0;
        }
        _SEH2_END;
    }
    // Return the count
    return iRetCount;
}

/* EOF */
