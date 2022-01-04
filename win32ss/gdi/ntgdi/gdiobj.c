/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/gdiobj.c
 * PURPOSE:         General GDI object manipulation routines
 * PROGRAMMERS:     Timo Kreuzer
 */

/*
 * If you want to understand this code, you need to start thinking in portals.
 * - gpaulRefCount is a global pointer to an allocated array of ULONG values,
 * one for each handle. Bits 0 - 22 contain a reference count for the handle.
 * It gets increased for each handle lock / reference. Bit 23 contains a valid
 * bit. If this bit is 0, the handle got deleted and will be pushed to the free
 * list, once all references are gone. Bits 24 - 31 contain the reuse value of
 * the handle, which allows to check if the entry was changed before atomically
 * exchanging the reference count.
 * - Objects can exist with or without a handle
 *   - Objects with a handle can be locked either exclusively or shared.
 *     Both locks increase the handle reference count in gpaulRefCount.
 *     Exclusive locks also increase the BASEOBJECT's cExclusiveLock field
 *     and the first lock (can be acquired recursively) acquires a pushlock
 *     that is also stored in the BASEOBJECT.
 *   - Objects without a handle cannot have exclusive locks. Their reference
 *     count is tracked in the BASEOBJECT's ulShareCount field.
 * - An object that is inserted in the handle table automatically has an
 *   exclusive lock. For objects that are "shared objects" (BRUSH, PALETTE, ...)
 *   this is the only way it can ever be exclusively locked. It prevents the
 *   object from being locked by another thread. A shared lock will simply fail,
 *   while an exclusive lock will succeed after the object was unlocked.
 *
 * Ownership:
 *
 * Owner:               POWNED  PUBLIC  NONE    spec
 * ---------------------------------------------------
 * LockForRead          +       +       -       PUBLIC
 * LockForWrite         +       -       -       POWNED
 * LockAny              +       +       +       NONE
 * NtGdiDeleteObjectApp +       -       -       PUBLIC
 * GreDeleteObject      +       +       +       NONE
 * GreSetOwner(POWNED)  -       -       +       -
 * GreSetOwner(PUBLIC)  +       -       +       -
 * GreSetOwner(NONE)    +       -       -       -
 *
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

FORCEINLINE
ULONG
InterlockedReadUlong(
    _In_ _Interlocked_operand_ ULONG volatile *Source)
{
    return *Source;
}

FORCEINLINE
void
INCREASE_THREAD_LOCK_COUNT(
    _In_ HANDLE hobj)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    DBG_UNREFERENCED_PARAMETER(hobj);
    if (pti)
    {
#if DBG
        pti->acExclusiveLockCount[((ULONG_PTR)hobj >> 16) & 0x1f]++;
#endif
        pti->cExclusiveLocks++;
    }
}

FORCEINLINE
void
DECREASE_THREAD_LOCK_COUNT(
    _In_ HANDLE hobj)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    DBG_UNREFERENCED_PARAMETER(hobj);
    if (pti)
    {
#if DBG
        pti->acExclusiveLockCount[((ULONG_PTR)hobj >> 16) & 0x1f]--;
#endif
        pti->cExclusiveLocks--;
    }
}

#if DBG
VOID
ASSERT_LOCK_ORDER(
    _In_ UCHAR objt)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    ULONG i;

    if (pti)
    {
        /* Ensure correct locking order! */
        for (i = objt + 1; i < GDIObjTypeTotal; i++)
        {
            NT_ASSERT(pti->acExclusiveLockCount[i] == 0);
        }
    }
}
#define ASSERT_SHARED_OBJECT_TYPE(objt) \
    ASSERT((objt) == GDIObjType_SURF_TYPE || \
           (objt) == GDIObjType_PAL_TYPE || \
           (objt) == GDIObjType_LFONT_TYPE || \
           (objt) == GDIObjType_PATH_TYPE || \
           (objt) == GDIObjType_BRUSH_TYPE)
#define ASSERT_EXCLUSIVE_OBJECT_TYPE(objt) \
    ASSERT((objt) == GDIObjType_DC_TYPE || \
           (objt) == GDIObjType_RGN_TYPE || \
           (objt) == GDIObjType_UMPD_TYPE || \
           (objt) == GDIObjType_META_TYPE)
#define ASSERT_TRYLOCK_OBJECT_TYPE(objt) \
    ASSERT((objt) == GDIObjType_DRVOBJ_TYPE)
#else
#define ASSERT_LOCK_ORDER(hobj)
#define ASSERT_SHARED_OBJECT_TYPE(objt)
#define ASSERT_EXCLUSIVE_OBJECT_TYPE(objt)
#define ASSERT_TRYLOCK_OBJECT_TYPE(objt)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#define InterlockedOr16 _InterlockedOr16
#endif

#define GDIOBJ_POOL_TAG(type) ('00hG' + (((type) & 0x1f) << 24))

enum
{
    REF_MASK_REUSE = 0xff000000,
    REF_INC_REUSE  = 0x01000000,
    REF_MASK_VALID = 0x00800000,
    REF_MASK_COUNT = 0x007fffff,
    REF_MASK_INUSE = 0x00ffffff,
};

/* GLOBALS *******************************************************************/

/* Per session handle table globals */
static PVOID gpvGdiHdlTblSection = NULL;
PENTRY gpentHmgr;
PULONG gpaulRefCount;
volatile ULONG gulFirstFree;
volatile ULONG gulFirstUnused;
static PPAGED_LOOKASIDE_LIST gpaLookasideList;

static VOID NTAPI GDIOBJ_vCleanup(PVOID ObjectBody);

static const
GDICLEANUPPROC
apfnCleanup[] =
{
    NULL,              /* 00 GDIObjType_DEF_TYPE */
    DC_vCleanup,       /* 01 GDIObjType_DC_TYPE */
    NULL,              /* 02 GDIObjType_UNUSED1_TYPE */
    NULL,              /* 03 GDIObjType_UNUSED2_TYPE */
    REGION_vCleanup,   /* 04 GDIObjType_RGN_TYPE */
    SURFACE_vCleanup,  /* 05 GDIObjType_SURF_TYPE */
    GDIOBJ_vCleanup,   /* 06 GDIObjType_CLIENTOBJ_TYPE */
    GDIOBJ_vCleanup,   /* 07 GDIObjType_PATH_TYPE */
    PALETTE_vCleanup,  /* 08 GDIObjType_PAL_TYPE */
    GDIOBJ_vCleanup,   /* 09 GDIObjType_ICMLCS_TYPE */
    GDIOBJ_vCleanup,   /* 0a GDIObjType_LFONT_TYPE */
    NULL,              /* 0b GDIObjType_RFONT_TYPE, unused */
    NULL,              /* 0c GDIObjType_PFE_TYPE, unused */
    NULL,              /* 0d GDIObjType_PFT_TYPE, unused */
    GDIOBJ_vCleanup,   /* 0e GDIObjType_ICMCXF_TYPE */
    NULL,              /* 0f GDIObjType_SPRITE_TYPE, unused */
    NULL,              /* 10 GDIObjType_BRUSH_TYPE, BRUSH, PEN, EXTPEN */
    NULL,              /* 11 GDIObjType_UMPD_TYPE, unused */
    NULL,              /* 12 GDIObjType_UNUSED4_TYPE */
    NULL,              /* 13 GDIObjType_SPACE_TYPE, unused */
    NULL,              /* 14 GDIObjType_UNUSED5_TYPE */
    GDIOBJ_vCleanup,   /* 15 GDIObjType_META_TYPE */
    NULL,              /* 16 GDIObjType_EFSTATE_TYPE, unused */
    NULL,              /* 17 GDIObjType_BMFD_TYPE, unused */
    NULL,              /* 18 GDIObjType_VTFD_TYPE, unused */
    NULL,              /* 19 GDIObjType_TTFD_TYPE, unused */
    NULL,              /* 1a GDIObjType_RC_TYPE, unused */
    NULL,              /* 1b GDIObjType_TEMP_TYPE, unused */
    DRIVEROBJ_vCleanup,/* 1c GDIObjType_DRVOBJ_TYPE */
    NULL,              /* 1d GDIObjType_DCIOBJ_TYPE, unused */
    NULL,              /* 1e GDIObjType_SPOOL_TYPE, unused */
    NULL,              /* 1f reserved entry */
};

static const
GDIOBJDELETEPROC
apfnDelete[] =
{
    NULL,              /* 00 GDIObjType_DEF_TYPE */
    NULL,              /* 01 GDIObjType_DC_TYPE */
    NULL,              /* 02 GDIObjType_UNUSED1_TYPE */
    NULL,              /* 03 GDIObjType_UNUSED2_TYPE */
    NULL,              /* 04 GDIObjType_RGN_TYPE */
    NULL,              /* 05 GDIObjType_SURF_TYPE */
    NULL,              /* 06 GDIObjType_CLIENTOBJ_TYPE */
    NULL,              /* 07 GDIObjType_PATH_TYPE */
    NULL,              /* 08 GDIObjType_PAL_TYPE */
    NULL,              /* 09 GDIObjType_ICMLCS_TYPE */
    NULL,              /* 0a GDIObjType_LFONT_TYPE */
    NULL,              /* 0b GDIObjType_RFONT_TYPE, unused */
    NULL,              /* 0c GDIObjType_PFE_TYPE, unused */
    NULL,              /* 0d GDIObjType_PFT_TYPE, unused */
    NULL,              /* 0e GDIObjType_ICMCXF_TYPE */
    NULL,                 /* 0f GDIObjType_SPRITE_TYPE, unused */
    BRUSH_vDeleteObject,  /* 10 GDIObjType_BRUSH_TYPE, BRUSH, PEN, EXTPEN */
    NULL,              /* 11 GDIObjType_UMPD_TYPE, unused */
    NULL,              /* 12 GDIObjType_UNUSED4_TYPE */
    NULL,              /* 13 GDIObjType_SPACE_TYPE, unused */
    NULL,              /* 14 GDIObjType_UNUSED5_TYPE */
    NULL,              /* 15 GDIObjType_META_TYPE, unused */
    NULL,              /* 16 GDIObjType_EFSTATE_TYPE, unused */
    NULL,              /* 17 GDIObjType_BMFD_TYPE, unused */
    NULL,              /* 18 GDIObjType_VTFD_TYPE, unused */
    NULL,              /* 19 GDIObjType_TTFD_TYPE, unused */
    NULL,              /* 1a GDIObjType_RC_TYPE, unused */
    NULL,              /* 1b GDIObjType_TEMP_TYPE, unused */
    NULL,              /* 1c GDIObjType_DRVOBJ_TYPE */
    NULL,              /* 1d GDIObjType_DCIOBJ_TYPE, unused */
    NULL,              /* 1e GDIObjType_SPOOL_TYPE, unused */
    NULL,              /* 1f reserved entry */
};

/* INTERNAL FUNCTIONS ********************************************************/

static
VOID
NTAPI
GDIOBJ_vCleanup(PVOID ObjectBody)
{
    /* Nothing to do */
}

static
VOID
InitLookasideList(UCHAR objt, ULONG cjSize)
{
    ExInitializePagedLookasideList(&gpaLookasideList[objt],
                                   NULL,
                                   NULL,
                                   0,
                                   cjSize,
                                   GDITAG_HMGR_LOOKASIDE_START + (objt << 24),
                                   0);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitGdiHandleTable(void)
{
    NTSTATUS status;
    LARGE_INTEGER liSize;
    PVOID pvSection;
    SIZE_T cjViewSize = 0;

    /* Create a section for the shared handle table */
    liSize.QuadPart = sizeof(GDI_HANDLE_TABLE); // GDI_HANDLE_COUNT * sizeof(ENTRY);
    status = MmCreateSection(&gpvGdiHdlTblSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &liSize,
                             PAGE_READWRITE,
                             SEC_COMMIT | 0x1,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("INITGDI: Could not allocate a GDI handle table.\n");
        return status;
    }

    /* Map the section in session space */
    status = MmMapViewInSessionSpace(gpvGdiHdlTblSection,
                                     (PVOID*)&gpentHmgr,
                                     &cjViewSize);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("INITGDI: Failed to map handle table section\n");
        ObDereferenceObject(gpvGdiHdlTblSection);
        return status;
    }

    /* Allocate memory for the reference counter table */
    gpaulRefCount = EngAllocSectionMem(&pvSection,
                                     FL_ZERO_MEMORY,
                                     GDI_HANDLE_COUNT * sizeof(ULONG),
                                     'frHG');
    if (!gpaulRefCount)
    {
        DPRINT1("INITGDI: Failed to allocate reference table.\n");
        ObDereferenceObject(gpvGdiHdlTblSection);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    gulFirstFree = 0;
    gulFirstUnused = RESERVE_ENTRIES_COUNT;

    GdiHandleTable = (PVOID)gpentHmgr;

    /* Initialize the lookaside lists */
    gpaLookasideList = ExAllocatePoolWithTag(NonPagedPool,
                           GDIObjTypeTotal * sizeof(PAGED_LOOKASIDE_LIST),
                           TAG_GDIHNDTBLE);
    if(!gpaLookasideList)
        return STATUS_NO_MEMORY;

    InitLookasideList(GDIObjType_DC_TYPE, sizeof(DC));
    InitLookasideList(GDIObjType_RGN_TYPE, sizeof(REGION));
    InitLookasideList(GDIObjType_SURF_TYPE, sizeof(SURFACE));
    InitLookasideList(GDIObjType_CLIENTOBJ_TYPE, sizeof(CLIENTOBJ));
    InitLookasideList(GDIObjType_PATH_TYPE, sizeof(PATH));
    InitLookasideList(GDIObjType_PAL_TYPE, sizeof(PALETTE));
    InitLookasideList(GDIObjType_ICMLCS_TYPE, sizeof(COLORSPACE));
    InitLookasideList(GDIObjType_LFONT_TYPE, sizeof(TEXTOBJ));
    InitLookasideList(GDIObjType_BRUSH_TYPE, sizeof(BRUSH));

    return STATUS_SUCCESS;
}

FORCEINLINE
VOID
IncrementCurrentProcessGdiHandleCount(void)
{
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
    if (ppi) InterlockedIncrement((LONG*)&ppi->GDIHandleCount);
}

FORCEINLINE
VOID
DecrementCurrentProcessGdiHandleCount(void)
{
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
    if (ppi) InterlockedDecrement((LONG*)&ppi->GDIHandleCount);
}

static inline
VOID
IncrementGdiHandleCount(ULONG ulProcessId)
{
    PEPROCESS pep;
    PPROCESSINFO ppi;
    NTSTATUS Status;

    Status = PsLookupProcessByProcessId(ULongToHandle(ulProcessId), &pep);
    NT_ASSERT(NT_SUCCESS(Status));
    __analysis_assume(NT_SUCCESS(Status));

    ppi = PsGetProcessWin32Process(pep);
    if (ppi) InterlockedIncrement((LONG*)&ppi->GDIHandleCount);
    if (NT_SUCCESS(Status)) ObDereferenceObject(pep);
}

static inline
VOID
DecrementGdiHandleCount(ULONG ulProcessId)
{
    PEPROCESS pep;
    PPROCESSINFO ppi;
    NTSTATUS Status;

    Status = PsLookupProcessByProcessId(ULongToHandle(ulProcessId), &pep);
    NT_ASSERT(NT_SUCCESS(Status));
    __analysis_assume(NT_SUCCESS(Status));

    ppi = PsGetProcessWin32Process(pep);
    if (ppi) InterlockedDecrement((LONG*)&ppi->GDIHandleCount);
    if (NT_SUCCESS(Status)) ObDereferenceObject(pep);
}

static
PENTRY
ENTRY_pentPopFreeEntry(VOID)
{
    ULONG iFirst, iNext, iPrev;
    PENTRY pentFree;

    DPRINT("Enter InterLockedPopFreeEntry\n");

    do
    {
        /* Get the index and sequence number of the first free entry */
        iFirst = InterlockedReadUlong(&gulFirstFree);

        /* Check if we have a free entry */
        if (!(iFirst & GDI_HANDLE_INDEX_MASK))
        {
            /* Increment FirstUnused and get the new index */
            iFirst = InterlockedIncrement((LONG*)&gulFirstUnused) - 1;

            /* Check if we have unused entries left */
            if (iFirst >= GDI_HANDLE_COUNT)
            {
                DPRINT1("No more GDI handles left!\n");
#if DBG_ENABLE_GDIOBJ_BACKTRACES
                DbgDumpGdiHandleTableWithBT();
#endif
                InterlockedDecrement((LONG*)&gulFirstUnused);
                return 0;
            }

            /* Return the old entry */
            return &gpentHmgr[iFirst];
        }

        /* Get a pointer to the first free entry */
        pentFree = &gpentHmgr[iFirst & GDI_HANDLE_INDEX_MASK];

        /* Create a new value with an increased sequence number */
        iNext = GDI_HANDLE_GET_INDEX(pentFree->einfo.hFree);
        iNext |= (iFirst & ~GDI_HANDLE_INDEX_MASK) + 0x10000;

        /* Try to exchange the FirstFree value */
        iPrev = InterlockedCompareExchange((LONG*)&gulFirstFree,
                                           iNext,
                                           iFirst);
    }
    while (iPrev != iFirst);

    /* Sanity check: is entry really free? */
    ASSERT(((ULONG_PTR)pentFree->einfo.pobj & ~GDI_HANDLE_INDEX_MASK) == 0);

    return pentFree;
}

/* Pushes an entry of the handle table to the free list,
   The entry must not have any references left */
static
VOID
ENTRY_vPushFreeEntry(PENTRY pentFree)
{
    ULONG iToFree, iFirst, iPrev, idxToFree;

    DPRINT("Enter ENTRY_vPushFreeEntry\n");

    idxToFree = pentFree - gpentHmgr;
    ASSERT((gpaulRefCount[idxToFree] & REF_MASK_INUSE) == 0);

    /* Initialize entry */
    pentFree->Objt = GDIObjType_DEF_TYPE;
    pentFree->ObjectOwner.ulObj = 0;
    pentFree->pUser = NULL;

    /* Increase reuse counter in entry and reference counter */
    InterlockedExchangeAdd((LONG*)&gpaulRefCount[idxToFree], REF_INC_REUSE);
    pentFree->FullUnique += 0x0100;

    do
    {
        /* Get the current first free index and sequence number */
        iFirst = InterlockedReadUlong(&gulFirstFree);

        /* Set the einfo.pobj member to the index of the first free entry */
        pentFree->einfo.pobj = UlongToPtr(iFirst & GDI_HANDLE_INDEX_MASK);

        /* Combine new index and increased sequence number in iToFree */
        iToFree = idxToFree | ((iFirst & ~GDI_HANDLE_INDEX_MASK) + 0x10000);

        /* Try to atomically update the first free entry */
        iPrev = InterlockedCompareExchange((LONG*)&gulFirstFree,
                                           iToFree,
                                           iFirst);
    }
    while (iPrev != iFirst);
}

static
PENTRY
ENTRY_ReferenceEntryByHandle(HGDIOBJ hobj, FLONG fl)
{
    ULONG ulIndex, cNewRefs, cOldRefs;
    PENTRY pentry;

    /* Get the handle index and check if its too big */
    ulIndex = GDI_HANDLE_GET_INDEX(hobj);

    /* Get pointer to the entry */
    pentry = &gpentHmgr[ulIndex];

    /* Get the current reference count */
    cOldRefs = gpaulRefCount[ulIndex];

    do
    {
        /* Check if the slot is deleted */
        if ((cOldRefs & REF_MASK_VALID) == 0)
        {
            DPRINT("GDIOBJ: Slot is not valid: 0x%lx, hobh=%p\n", cOldRefs, hobj);
            return NULL;
        }

        /* Check if the unique value matches */
        if (pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16))
        {
            DPRINT("GDIOBJ: Wrong unique value. Handle: 0x%4x, entry: 0x%4x\n",
                   (USHORT)((ULONG_PTR)hobj >> 16), pentry->FullUnique);
            return NULL;
        }

        /* Check if the object owner is this process or public */
        if (!(fl & GDIOBJFLAG_IGNOREPID) &&
            pentry->ObjectOwner.ulObj != GDI_OBJ_HMGR_PUBLIC &&
            pentry->ObjectOwner.ulObj != PtrToUlong(PsGetCurrentProcessId()))
        {
            DPRINT("GDIOBJ: Cannot reference foreign handle %p, pentry=%p:%lx.\n",
                    hobj, pentry, pentry->ObjectOwner.ulObj);
            return NULL;
        }

        /* Try to atomically increment the reference count */
        cNewRefs = cOldRefs + 1;
        cOldRefs = InterlockedCompareExchange((PLONG)&gpaulRefCount[ulIndex],
                                              cNewRefs,
                                              cOldRefs);
    }
    while (cNewRefs != cOldRefs + 1);

    /* Integrity checks */
    ASSERT((pentry->FullUnique & 0x1f) == pentry->Objt);
    ASSERT(pentry->einfo.pobj != NULL);

    /* Check if lower 32 bits match, the upper 32 bits are ignored */
    ASSERT(pentry->einfo.pobj->hHmgr == UlongToPtr(PtrToUlong(hobj)));

    return pentry;
}

static
HGDIOBJ
ENTRY_hInsertObject(PENTRY pentry, POBJ pobj, UCHAR objt, ULONG ulOwner)
{
    ULONG ulIndex;

    /* Calculate the handle index */
    ulIndex = pentry - gpentHmgr;

    /* Update the fields in the ENTRY */
    pentry->einfo.pobj = pobj;
    pentry->Objt = objt & 0x1f;
    pentry->FullUnique = (pentry->FullUnique & 0xff00) | objt;
    pentry->ObjectOwner.ulObj = ulOwner;

    /* Make the handle valid with 1 reference */
    ASSERT((gpaulRefCount[ulIndex] & REF_MASK_INUSE) == 0);
    InterlockedOr((LONG*)&gpaulRefCount[ulIndex], REF_MASK_VALID | 1);

    /* Return the handle */
    return (HGDIOBJ)(((ULONG_PTR)pentry->FullUnique << 16) | ulIndex);
}

POBJ
NTAPI
GDIOBJ_AllocateObject(UCHAR objt, ULONG cjSize, FLONG fl)
{
    POBJ pobj;

    if (fl & BASEFLAG_LOOKASIDE)
    {
        /* Allocate the object from a lookaside list */
        pobj = ExAllocateFromPagedLookasideList(&gpaLookasideList[objt & 0x1f]);
    }
    else
    {
        /* Allocate the object from paged pool */
        pobj = ExAllocatePoolWithTag(PagedPool, cjSize, GDIOBJ_POOL_TAG(objt));
    }

    if (!pobj) return NULL;

    /* Initialize the object */
    RtlZeroMemory(pobj, cjSize);
    pobj->hHmgr = (HGDIOBJ)((ULONG_PTR)objt << 16);
    pobj->cExclusiveLock = 0;
    pobj->ulShareCount = 1;
    pobj->BaseFlags = fl & 0xffff;
    DBG_INITLOG(&pobj->slhLog);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_ALLOCATE, 0);
#if DBG_ENABLE_GDIOBJ_BACKTRACES
    DbgCaptureStackBackTace(pobj->apvBackTrace, 1, GDI_OBJECT_STACK_LEVELS);
#endif /* GDI_DEBUG */

    return pobj;
}

VOID
NTAPI
GDIOBJ_vFreeObject(POBJ pobj)
{
    UCHAR objt;

    DBG_CLEANUP_EVENT_LIST(&pobj->slhLog);

    /* Get the object type */
    objt = ((ULONG_PTR)pobj->hHmgr >> 16) & 0x1f;

    /* Check if we have a delete procedure (for C++ based objects) */
    if (apfnDelete[objt] != NULL)
    {
        /* Invoke the delete procedure */
        apfnDelete[objt](pobj);
    }
    else
    {
        /* Call the cleanup procedure */
        NT_ASSERT(apfnCleanup[objt]);
        apfnCleanup[objt](pobj);

        /* Check if the object is allocated from a lookaside list */
        if (pobj->BaseFlags & BASEFLAG_LOOKASIDE)
        {
            ExFreeToPagedLookasideList(&gpaLookasideList[objt], pobj);
        }
        else
        {
            ExFreePoolWithTag(pobj, GDIOBJ_POOL_TAG(objt));
        }
    }
}

VOID
NTAPI
GDIOBJ_vDereferenceObject(POBJ pobj)
{
    ULONG cRefs, ulIndex;

    /* Calculate the index */
    ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);

    /* Check if the object has a handle */
    if (ulIndex)
    {
        /* Decrement reference count */
        if ((gpaulRefCount[ulIndex] & REF_MASK_COUNT) == 0)
        {
            DBG_DUMP_EVENT_LIST(&pobj->slhLog);
        }
        ASSERT((gpaulRefCount[ulIndex] & REF_MASK_COUNT) > 0);
        cRefs = InterlockedDecrement((LONG*)&gpaulRefCount[ulIndex]);
        DBG_LOGEVENT(&pobj->slhLog, EVENT_DEREFERENCE, cRefs);

        /* Check if we reached 0 and handle bit is not set */
        if ((cRefs & REF_MASK_INUSE) == 0)
        {
            /* Make sure it's ok to delete the object */
            ASSERT(pobj->BaseFlags & BASEFLAG_READY_TO_DIE);

            /* Check if the handle was process owned */
            if (gpentHmgr[ulIndex].ObjectOwner.ulObj != GDI_OBJ_HMGR_PUBLIC &&
                gpentHmgr[ulIndex].ObjectOwner.ulObj != GDI_OBJ_HMGR_NONE)
            {
                /* Decrement the process handle count */
                ASSERT(gpentHmgr[ulIndex].ObjectOwner.ulObj ==
                       HandleToUlong(PsGetCurrentProcessId()));
                DecrementCurrentProcessGdiHandleCount();
            }

            /* Push entry to the free list */
            ENTRY_vPushFreeEntry(&gpentHmgr[ulIndex]);

            /* Free the object */
            GDIOBJ_vFreeObject(pobj);
        }
    }
    else
    {
        /* Decrement the objects reference count */
        ASSERT(pobj->ulShareCount > 0);
        cRefs = InterlockedDecrement((LONG*)&pobj->ulShareCount);
        DBG_LOGEVENT(&pobj->slhLog, EVENT_DEREFERENCE, cRefs);

        /* Check if we reached 0 */
        if (cRefs == 0)
        {
            /* Free the object */
            GDIOBJ_vFreeObject(pobj);
        }
    }
}

POBJ
NTAPI
GDIOBJ_ReferenceObjectByHandle(
    HGDIOBJ hobj,
    UCHAR objt)
{
    PENTRY pentry;
    POBJ pobj;

    /* Check if the handle type matches */
    ASSERT_SHARED_OBJECT_TYPE(objt);
    if ((((ULONG_PTR)hobj >> 16) & 0x1f) != objt)
    {
        DPRINT("GDIOBJ: Wrong type. handle=%p, type=%x\n", hobj, objt);
        return NULL;
    }

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(hobj, 0);
    if (!pentry)
    {
        DPRINT("GDIOBJ: Requested handle 0x%p is not valid.\n", hobj);
        return NULL;
    }

    /* Get the pointer to the BASEOBJECT */
    pobj = pentry->einfo.pobj;

    /* Check if the object is exclusively locked */
    if (pobj->cExclusiveLock != 0)
    {
        DPRINT1("GDIOBJ: Cannot reference object %p with exclusive lock.\n", hobj);
        GDIOBJ_vDereferenceObject(pobj);
        DBG_DUMP_EVENT_LIST(&pobj->slhLog);
        return NULL;
    }

    DBG_LOGEVENT(&pobj->slhLog, EVENT_REFERENCE, gpaulRefCount[pentry - gpentHmgr]);

    /* All is well, return the object */
    return pobj;
}

VOID
NTAPI
GDIOBJ_vReferenceObjectByPointer(POBJ pobj)
{
    ULONG cRefs;

    /* Check if the object has a handle */
    if (GDI_HANDLE_GET_INDEX(pobj->hHmgr))
    {
        /* Increase the handle's reference count */
        ULONG ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);
        ASSERT((gpaulRefCount[ulIndex] & REF_MASK_COUNT) > 0);
        cRefs = InterlockedIncrement((LONG*)&gpaulRefCount[ulIndex]);
    }
    else
    {
        /* Increase the object's reference count */
        cRefs = InterlockedIncrement((LONG*)&pobj->ulShareCount);
    }

    DBG_LOGEVENT(&pobj->slhLog, EVENT_REFERENCE, cRefs);
}

PGDIOBJ
NTAPI
GDIOBJ_TryLockObject(
    HGDIOBJ hobj,
    UCHAR objt)
{
    PENTRY pentry;
    POBJ pobj;
    DWORD dwThreadId;

    /* Check if the handle type matches */
    ASSERT_TRYLOCK_OBJECT_TYPE(objt);
    if ((((ULONG_PTR)hobj >> 16) & 0x1f) != objt)
    {
        DPRINT("Wrong object type: hobj=0x%p, objt=0x%x\n", hobj, objt);
        return NULL;
    }

    /* Make sure lock order is correct */
    ASSERT_LOCK_ORDER(objt);

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(hobj, 0);
    if (!pentry)
    {
        DPRINT("GDIOBJ: Requested handle 0x%p is not valid.\n", hobj);
        return NULL;
    }

    /* Get the pointer to the BASEOBJECT */
    pobj = pentry->einfo.pobj;

    /* Check if we already own the lock */
    dwThreadId = PtrToUlong(PsGetCurrentThreadId());
    if (pobj->dwThreadId != dwThreadId)
    {
        /* Disable APCs and try acquiring the push lock */
        KeEnterCriticalRegion();
        if(!ExTryAcquirePushLockExclusive(&pobj->pushlock))
        {
            ULONG cRefs, ulIndex;
            /* Already owned. Clean up and leave. */
            KeLeaveCriticalRegion();

            /* Calculate the index */
            ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);

            /* Decrement reference count */
            ASSERT((gpaulRefCount[ulIndex] & REF_MASK_COUNT) > 0);
            cRefs = InterlockedDecrement((LONG*)&gpaulRefCount[ulIndex]);
            ASSERT(cRefs & REF_MASK_VALID);

            return NULL;
        }

        /* Set us as lock owner */
        ASSERT(pobj->dwThreadId == 0);
        pobj->dwThreadId = dwThreadId;
    }

    /* Increase lock count */
    pobj->cExclusiveLock++;
    INCREASE_THREAD_LOCK_COUNT(hobj);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_LOCK, 0);

    /* Return the object */
    return pobj;
}

PGDIOBJ
NTAPI
GDIOBJ_LockObject(
    HGDIOBJ hobj,
    UCHAR objt)
{
    PENTRY pentry;
    POBJ pobj;
    DWORD dwThreadId;

    /* Check if the handle type matches */
    ASSERT_EXCLUSIVE_OBJECT_TYPE(objt);
    if ((((ULONG_PTR)hobj >> 16) & 0x1f) != objt)
    {
        DPRINT("Wrong object type: hobj=0x%p, objt=0x%x\n", hobj, objt);
        return NULL;
    }

    /* Make sure lock order is correct */
    ASSERT_LOCK_ORDER(objt);

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(hobj, 0);
    if (!pentry)
    {
        DPRINT("GDIOBJ: Requested handle 0x%p is not valid.\n", hobj);
        return NULL;
    }

    /* Get the pointer to the BASEOBJECT */
    pobj = pentry->einfo.pobj;

    /* Check if we already own the lock */
    dwThreadId = PtrToUlong(PsGetCurrentThreadId());
    if (pobj->dwThreadId != dwThreadId)
    {
        /* Disable APCs and acquire the push lock */
        KeEnterCriticalRegion();
        ExAcquirePushLockExclusive(&pobj->pushlock);

        /* Set us as lock owner */
        ASSERT(pobj->dwThreadId == 0);
        pobj->dwThreadId = dwThreadId;
    }

    /* Increase lock count */
    pobj->cExclusiveLock++;
    INCREASE_THREAD_LOCK_COUNT(hobj);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_LOCK, 0);

    /* Return the object */
    return pobj;
}

VOID
NTAPI
GDIOBJ_vUnlockObject(POBJ pobj)
{
    ULONG cRefs, ulIndex;
    ASSERT(pobj->cExclusiveLock > 0);

    /* Decrease lock count */
    pobj->cExclusiveLock--;
    DECREASE_THREAD_LOCK_COUNT(pobj->hHmgr);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_UNLOCK, 0);

    /* Check if this was the last lock */
    if (pobj->cExclusiveLock == 0)
    {
        /* Reset lock owner */
        pobj->dwThreadId = 0;

        /* Release the pushlock and reenable APCs */
        ExReleasePushLockExclusive(&pobj->pushlock);
        KeLeaveCriticalRegion();
    }

    /* Calculate the index */
    ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);

    /* Decrement reference count */
    ASSERT((gpaulRefCount[ulIndex] & REF_MASK_COUNT) > 0);
    cRefs = InterlockedDecrement((LONG*)&gpaulRefCount[ulIndex]);
    ASSERT(cRefs & REF_MASK_VALID);
}

HGDIOBJ
NTAPI
GDIOBJ_hInsertObject(
    POBJ pobj,
    ULONG ulOwner)
{
    PENTRY pentry;
    UCHAR objt;

    /* Must have no handle and only one reference */
    ASSERT(GDI_HANDLE_GET_INDEX(pobj->hHmgr) == 0);
    ASSERT(pobj->cExclusiveLock == 0);
    ASSERT(pobj->ulShareCount == 1);

    /* Get a free handle entry */
    pentry = ENTRY_pentPopFreeEntry();
    if (!pentry)
    {
        DPRINT1("GDIOBJ: Could not get a free entry.\n");
        return NULL;
    }

    /* Make the object exclusively locked */
    ExInitializePushLock(&pobj->pushlock);
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&pobj->pushlock);
    pobj->cExclusiveLock = 1;
    pobj->dwThreadId = PtrToUlong(PsGetCurrentThreadId());
    INCREASE_THREAD_LOCK_COUNT(pobj->hHmgr);

    /* Get object type from the hHmgr field */
    objt = ((ULONG_PTR)pobj->hHmgr >> 16) & 0xff;
    ASSERT(objt != GDIObjType_DEF_TYPE);

    /* Check if current process is requested owner */
    if (ulOwner == GDI_OBJ_HMGR_POWNED)
    {
        /* Increment the process handle count */
        IncrementCurrentProcessGdiHandleCount();

        /* Use Process id */
        ulOwner = HandleToUlong(PsGetCurrentProcessId());
    }

    /* Insert the object into the handle table */
    pobj->hHmgr = ENTRY_hInsertObject(pentry, pobj, objt, ulOwner);

    /* Return the handle */
    DPRINT("GDIOBJ: Created handle: %p\n", pobj->hHmgr);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_CREATE_HANDLE, 0);
    return pobj->hHmgr;
}

VOID
NTAPI
GDIOBJ_vSetObjectOwner(
    POBJ pobj,
    ULONG ulNewOwner)
{
    PENTRY pentry;
    ULONG ulOldOwner;

    /* This is a ugly HACK, needed to fix IntGdiSetDCOwnerEx */
    if (GDI_HANDLE_IS_STOCKOBJ(pobj->hHmgr))
    {
        DPRINT("Trying to set ownership of stock object %p to %lx\n", pobj->hHmgr, ulNewOwner);
        return;
    }

    /* Get the handle entry */
    NT_ASSERT(GDI_HANDLE_GET_INDEX(pobj->hHmgr));
    pentry = &gpentHmgr[GDI_HANDLE_GET_INDEX(pobj->hHmgr)];

    /* Check if the new owner is the same as the old one */
    ulOldOwner = pentry->ObjectOwner.ulObj;
    if (ulOldOwner == ulNewOwner)
    {
        /* Nothing to do */
        return;
    }

    /* Is the current process requested? */
    if (ulNewOwner == GDI_OBJ_HMGR_POWNED)
    {
        /* Use process id */
        ulNewOwner = HandleToUlong(PsGetCurrentProcessId());
    }

    // HACK
    if (ulNewOwner == GDI_OBJ_HMGR_NONE)
        ulNewOwner = GDI_OBJ_HMGR_PUBLIC;

    /* Was the object process owned? */
    if ((ulOldOwner != GDI_OBJ_HMGR_PUBLIC) &&
        (ulOldOwner != GDI_OBJ_HMGR_NONE))
    {
        /* Decrement the previous owners handle count */
        DecrementGdiHandleCount(ulOldOwner);
    }

    /* Is the new owner a process? */
    if ((ulNewOwner != GDI_OBJ_HMGR_PUBLIC) &&
        (ulNewOwner != GDI_OBJ_HMGR_NONE))
    {
        /* Increment the new owners handle count */
        IncrementGdiHandleCount(ulNewOwner);
    }
    else
    {
        /* Make sure we don't leak user mode memory */
        NT_ASSERT(pentry->pUser == NULL);
    }

    /* Set new owner */
    pentry->ObjectOwner.ulObj = ulNewOwner;
    DBG_LOGEVENT(&pobj->slhLog, EVENT_SET_OWNER, 0);
}

/* Locks 2 or 3 objects at a time */
BOOL
NTAPI
GDIOBJ_bLockMultipleObjects(
    IN ULONG ulCount,
    IN HGDIOBJ* ahObj,
    OUT PGDIOBJ* apObj,
    IN UCHAR objt)
{
    UINT auiIndices[3] = {0, 1, 2};
    UINT i, j, tmp;

    ASSERT(ulCount <= 3);

    /* Sort the handles */
    for (i = 0; i < ulCount - 1; i++)
    {
        for (j = i + 1; j < ulCount; j++)
        {
            if ((ULONG_PTR)ahObj[auiIndices[i]] <
                (ULONG_PTR)ahObj[auiIndices[j]])
            {
                tmp = auiIndices[i];
                auiIndices[i] = auiIndices[j];
                auiIndices[j] = tmp;
            }
        }
    }

    /* Lock the objects in safe order */
    for (i = 0; i < ulCount; i++)
    {
        /* Skip NULL handles */
        if (ahObj[auiIndices[i]] == NULL)
        {
            apObj[auiIndices[i]] = NULL;
            continue;
        }

        /* Lock the object */
        apObj[auiIndices[i]] = GDIOBJ_LockObject(ahObj[auiIndices[i]], objt);

        /* Check for failure */
        if (apObj[auiIndices[i]] == NULL)
        {
            /* Cleanup */
            while (i--)
            {
                if (apObj[auiIndices[i]])
                    GDIOBJ_vUnlockObject(apObj[auiIndices[i]]);
            }
            return FALSE;
        }
    }

    return TRUE;
}

PVOID
NTAPI
GDIOBJ_pvGetObjectAttr(POBJ pobj)
{
    ULONG ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);
    return gpentHmgr[ulIndex].pUser;
}

VOID
NTAPI
GDIOBJ_vSetObjectAttr(POBJ pobj, PVOID pvObjAttr)
{
    ULONG ulIndex;

    ASSERT(pobj->hHmgr);

    /* Get the handle index */
    ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);

    /* Set pointer to the usermode attribute */
    gpentHmgr[ulIndex].pUser = pvObjAttr;
}

VOID
NTAPI
GDIOBJ_vDeleteObject(POBJ pobj)
{
    ULONG ulIndex;

    /* Set the object's delete flag */
    InterlockedOr16((SHORT*)&pobj->BaseFlags, BASEFLAG_READY_TO_DIE);
    DBG_LOGEVENT(&pobj->slhLog, EVENT_DELETE, 0);

    /* Get the handle index */
    ulIndex = GDI_HANDLE_GET_INDEX(pobj->hHmgr);
    if (ulIndex)
    {
        /* Reset the handle valid bit */
        InterlockedAnd((LONG*)&gpaulRefCount[ulIndex], ~REF_MASK_VALID);

        /* Check if the object is exclusively locked */
        if (pobj->cExclusiveLock != 0)
        {
            /* Reset lock owner and lock count */
            pobj->dwThreadId = 0;
            pobj->cExclusiveLock = 0;

            /* Release the pushlock and reenable APCs */
            ExReleasePushLockExclusive(&pobj->pushlock);
            KeLeaveCriticalRegion();
            DECREASE_THREAD_LOCK_COUNT(pobj->hHmgr);
        }
    }

    /* Dereference the object (will take care of deletion) */
    GDIOBJ_vDereferenceObject(pobj);
}

BOOL
NTAPI
GreIsHandleValid(HGDIOBJ hobj)
{
    PENTRY pentry;

    pentry = ENTRY_ReferenceEntryByHandle(hobj, 0);
    if (!pentry) return FALSE;
    GDIOBJ_vDereferenceObject(pentry->einfo.pobj);
    return TRUE;
}

BOOL
NTAPI
GreDeleteObject(HGDIOBJ hobj)
{
    PENTRY pentry;

    /* Check for stock objects */
    if (GDI_HANDLE_IS_STOCKOBJ(hobj))
    {
        DPRINT1("GreDeleteObject: Cannot delete stock object %p.\n", hobj);
        return FALSE;
    }

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(hobj, 0);
    if (!pentry)
    {
        DPRINT1("GreDeleteObject: Trying to delete invalid object %p\n", hobj);
        return FALSE;
    }

    /* Check for public owner */
    if (pentry->ObjectOwner.ulObj == GDI_OBJ_HMGR_PUBLIC)
    {
        DPRINT1("GreDeleteObject: Trying to delete global object %p\n", hobj);
        GDIOBJ_vDereferenceObject(pentry->einfo.pobj);
        return FALSE;
    }

    /* Delete the object */
    GDIOBJ_vDeleteObject(pentry->einfo.pobj);
    return TRUE;
}

ULONG
NTAPI
GreGetObjectOwner(HGDIOBJ hobj)
{
    ULONG ulIndex, ulOwner;

    /* Get the handle index */
    ulIndex = GDI_HANDLE_GET_INDEX(hobj);

    /* Check if the handle is valid */
    if (ulIndex >= GDI_HANDLE_COUNT ||
        gpentHmgr[ulIndex].Objt == GDIObjType_DEF_TYPE ||
        ((ULONG_PTR)hobj >> 16) != gpentHmgr[ulIndex].FullUnique)
    {
        DPRINT1("GreGetObjectOwner: invalid handle 0x%p.\n", hobj);
        return GDI_OBJ_HMGR_RESTRICTED;
    }

    /* Get the object owner */
    ulOwner = gpentHmgr[ulIndex].ObjectOwner.ulObj;

    if (ulOwner == HandleToUlong(PsGetCurrentProcessId()))
        return GDI_OBJ_HMGR_POWNED;

    if (ulOwner == GDI_OBJ_HMGR_PUBLIC)
        return GDI_OBJ_HMGR_PUBLIC;

    return GDI_OBJ_HMGR_RESTRICTED;
}

BOOL
NTAPI
GreSetObjectOwnerEx(
    HGDIOBJ hobj,
    ULONG ulOwner,
    ULONG Flags)
{
    PENTRY pentry;

    /* Check for stock objects */
    if (GDI_HANDLE_IS_STOCKOBJ(hobj))
    {
        DPRINT("GreSetObjectOwner: Got stock object %p\n", hobj);
        return FALSE;
    }

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(hobj, Flags);
    if (!pentry)
    {
        DPRINT("GreSetObjectOwner: Invalid handle 0x%p.\n", hobj);
        return FALSE;
    }

    /* Call internal function */
    GDIOBJ_vSetObjectOwner(pentry->einfo.pobj, ulOwner);

    /* Dereference the object */
    GDIOBJ_vDereferenceObject(pentry->einfo.pobj);

    return TRUE;
}

BOOL
NTAPI
GreSetObjectOwner(
    HGDIOBJ hobj,
    ULONG ulOwner)
{
    return GreSetObjectOwnerEx(hobj, ulOwner, 0);
}

INT
NTAPI
GreGetObject(
    IN HGDIOBJ hobj,
    IN INT cbCount,
    OUT PVOID pvBuffer)
{
    PVOID pvObj;
    UCHAR objt;
    INT iResult = 0;

    /* Verify object type */
    objt = ((ULONG_PTR)hobj >> 16) & 0x1f;
    if (objt != GDIObjType_BRUSH_TYPE &&
        objt != GDIObjType_SURF_TYPE &&
        objt != GDIObjType_LFONT_TYPE &&
        objt != GDIObjType_PAL_TYPE)
    {
        DPRINT1("GreGetObject: Invalid object type\n");
        return 0;
    }

    pvObj = GDIOBJ_ReferenceObjectByHandle(hobj, objt);
    if (!pvObj)
    {
        DPRINT("GreGetObject: Could not lock object\n");
        return 0;
    }

    switch (GDI_HANDLE_GET_TYPE(hobj))
    {
        case GDILoObjType_LO_PEN_TYPE:
        case GDILoObjType_LO_EXTPEN_TYPE:
            iResult = PEN_GetObject(pvObj, cbCount, pvBuffer);
            break;

        case GDILoObjType_LO_BRUSH_TYPE:
            iResult = BRUSH_GetObject(pvObj, cbCount, pvBuffer);
            break;

        case GDILoObjType_LO_BITMAP_TYPE:
            iResult = BITMAP_GetObject(pvObj, cbCount, pvBuffer);
            break;

        case GDILoObjType_LO_FONT_TYPE:
            iResult = FontGetObject(pvObj, cbCount, pvBuffer);
            break;

        case GDILoObjType_LO_PALETTE_TYPE:
            iResult = PALETTE_GetObject(pvObj, cbCount, pvBuffer);
            break;

        default:
            DPRINT1("GDI object type of 0x%p not implemented\n", hobj);
            break;
    }

    GDIOBJ_vDereferenceObject(pvObj);
    return iResult;
}

W32KAPI
INT
APIENTRY
NtGdiExtGetObjectW(
    IN HANDLE hobj,
    IN INT cjBufferSize,
    OUT LPVOID lpBuffer)
{
    UINT iResult, cjMaxSize;
    union
    {
        BITMAP bitmap;
        DIBSECTION dibsection;
        LOGPEN logpen;
        LOGBRUSH logbrush;
        LOGFONTW logfontw;
        EXTLOGFONTW extlogfontw;
        ENUMLOGFONTEXDVW enumlogfontexdvw;
    } object;

    /* Normalize to the largest supported object size */
    cjMaxSize = min((UINT)cjBufferSize, sizeof(object));

    /* Now do the actual call */
    iResult = GreGetObject(hobj, cjMaxSize, lpBuffer ? &object : NULL);

    /* Check if we have a buffer and data */
    if ((lpBuffer != NULL) && (iResult != 0))
    {
        /* Enter SEH for buffer transfer */
        _SEH2_TRY
        {
            /* Probe the buffer and copy it */
            cjMaxSize = min(cjMaxSize, iResult);
            ProbeForWrite(lpBuffer, cjMaxSize, sizeof(WORD));
            RtlCopyMemory(lpBuffer, &object, cjMaxSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Clear the return value.
             * Do *NOT* set last error here! */
            iResult = 0;
        }
        _SEH2_END;
    }

    /* Return the count */
    return iResult;
}

W32KAPI
HANDLE
APIENTRY
NtGdiCreateClientObj(
    IN ULONG ulType)
{
    POBJ pObject;
    HANDLE handle;

    /* Check if ulType is valid */
    if ((ulType != GDILoObjType_LO_METAFILE16_TYPE) &&
        (ulType != GDILoObjType_LO_METAFILE_TYPE) &&
        (ulType != GDILoObjType_LO_METADC16_TYPE))
    {
        DPRINT1("NtGdiCreateClientObj: Invalid object type 0x%lx.\n", ulType);
        return NULL;
    }

    /* Allocate a new object */
    pObject = GDIOBJ_AllocateObject(GDIObjType_CLIENTOBJ_TYPE,
                                    sizeof(CLIENTOBJ),
                                    BASEFLAG_LOOKASIDE);
    if (!pObject)
    {
        DPRINT1("NtGdiCreateClientObj: Could not allocate a clientobj.\n");
        return NULL;
    }

    /* Set the real object type */
    pObject->hHmgr = UlongToHandle(ulType | GDILoObjType_LO_CLIENTOBJ_TYPE);

    /* Create a handle */
    handle = GDIOBJ_hInsertObject(pObject, GDI_OBJ_HMGR_POWNED);
    if (!handle)
    {
        DPRINT1("NtGdiCreateClientObj: Could not create a handle.\n");
        GDIOBJ_vFreeObject(pObject);
        return NULL;
    }

    /* Unlock it */
    GDIOBJ_vUnlockObject(pObject);

    return handle;
}

W32KAPI
BOOL
APIENTRY
NtGdiDeleteClientObj(
    IN HANDLE hobj)
{
    /* We first need to get the real type from the handle */
    ULONG ulType = GDI_HANDLE_GET_TYPE(hobj);

    /* Check if it's really a CLIENTOBJ */
    if ((ulType & GDI_HANDLE_BASETYPE_MASK) != GDILoObjType_LO_CLIENTOBJ_TYPE)
    {
        /* FIXME: SetLastError? */
        return FALSE;
    }

    return GreDeleteObject(hobj);
}



PGDI_HANDLE_TABLE GdiHandleTable = NULL;

PGDIOBJ NTAPI
GDIOBJ_ShareLockObj(HGDIOBJ hObj, DWORD ExpectedType)
{
    if (ExpectedType == GDI_OBJECT_TYPE_DONTCARE)
        ExpectedType = GDI_HANDLE_GET_TYPE(hObj);
    return GDIOBJ_ReferenceObjectByHandle(hObj, (ExpectedType >> 16) & 0x1f);
}

// This function is not safe to use with concurrent deleting attempts
// That shouldn't be a problem, since we don't have any processes yet,
// that could delete the handle
BOOL
NTAPI
GDIOBJ_ConvertToStockObj(HGDIOBJ *phObj)
{
    PENTRY pentry;
    POBJ pobj;

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(*phObj, 0);
    if (!pentry)
    {
        DPRINT1("GDIOBJ: Requested handle 0x%p is not valid.\n", *phObj);
        return FALSE;
    }

    /* Update the entry */
    pentry->FullUnique |= GDI_ENTRY_STOCK_MASK;
    pentry->ObjectOwner.ulObj = 0;

    /* Get the pointer to the BASEOBJECT */
    pobj = pentry->einfo.pobj;

    /* Calculate the new handle */
    pobj->hHmgr = (HGDIOBJ)((ULONG_PTR)pobj->hHmgr | GDI_HANDLE_STOCK_MASK);

    /* Return the new handle */
    *phObj = pobj->hHmgr;

    /* Dereference the handle */
    GDIOBJ_vDereferenceObject(pobj);

    return TRUE;
}

BOOL
NTAPI
GDIOBJ_ConvertFromStockObj(HGDIOBJ *phObj)
{
    PENTRY pentry;
    POBJ pobj;

    /* Reference the handle entry */
    pentry = ENTRY_ReferenceEntryByHandle(*phObj, 0);
    if (!pentry)
    {
        DPRINT1("GDIOBJ: Requested handle 0x%p is not valid.\n", *phObj);
        return FALSE;
    }

    /* Update the entry */
    pentry->FullUnique &= ~GDI_ENTRY_STOCK_MASK;
    pentry->ObjectOwner.ulObj = PtrToUlong(PsGetCurrentProcessId());

    /* Get the pointer to the BASEOBJECT */
    pobj = pentry->einfo.pobj;

    /* Calculate the new handle */
    pobj->hHmgr = (HGDIOBJ)((ULONG_PTR)pobj->hHmgr & ~GDI_HANDLE_STOCK_MASK);

    /* Return the new handle */
    *phObj = pobj->hHmgr;

    /* Dereference the handle */
    GDIOBJ_vDereferenceObject(pobj);

    return TRUE;
}

POBJ NTAPI
GDIOBJ_AllocObjWithHandle(ULONG ObjectType, ULONG cjSize)
{
    POBJ pobj;
    FLONG fl = 0;
    UCHAR objt = (ObjectType >> 16) & 0xFF;

    if ((objt == GDIObjType_DC_TYPE && cjSize == sizeof(DC)) ||
        (objt == GDIObjType_PAL_TYPE && cjSize == sizeof(PALETTE)) ||
        (objt == GDIObjType_RGN_TYPE && cjSize == sizeof(REGION)) ||
        (objt == GDIObjType_SURF_TYPE && cjSize == sizeof(SURFACE)) ||
        (objt == GDIObjType_PATH_TYPE && cjSize == sizeof(PATH)))
    {
        fl |= BASEFLAG_LOOKASIDE;
    }

    pobj = GDIOBJ_AllocateObject(objt, cjSize, fl);
    if (!pobj)
    {
        return NULL;
    }

    if (!GDIOBJ_hInsertObject(pobj, GDI_OBJ_HMGR_POWNED))
    {
        GDIOBJ_vFreeObject(pobj);
        return NULL;
    }
    return pobj;
}

PVOID NTAPI
GDI_MapHandleTable(PEPROCESS pProcess)
{
    PVOID pvMappedView = NULL;
    NTSTATUS Status;
    LARGE_INTEGER liOffset;
    SIZE_T cjViewSize = sizeof(GDI_HANDLE_TABLE);

    liOffset.QuadPart = 0;

    ASSERT(gpvGdiHdlTblSection != NULL);
    ASSERT(pProcess != NULL);

    Status = MmMapViewOfSection(gpvGdiHdlTblSection,
                                pProcess,
                                &pvMappedView,
                                0,
                                0,
                                &liOffset,
                                &cjViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_READONLY);

    if (!NT_SUCCESS(Status))
        return NULL;

    return pvMappedView;
}

BOOL NTAPI
GDI_CleanupForProcess(struct _EPROCESS *Process)
{
    PENTRY pentry;
    ULONG ulIndex;
    DWORD dwProcessId;
    PPROCESSINFO ppi;

    DPRINT("CleanupForProcess prochandle %p Pid %p\n",
           Process, Process->UniqueProcessId);

    ASSERT(Process == PsGetCurrentProcess());

    /* Get the current process Id */
    dwProcessId = PtrToUlong(PsGetCurrentProcessId());

    /* Loop all handles in the handle table */
    for (ulIndex = RESERVE_ENTRIES_COUNT; ulIndex < gulFirstUnused; ulIndex++)
    {
        pentry = &gpentHmgr[ulIndex];

        /* Check if the object is owned by the process */
        if (pentry->ObjectOwner.ulObj == dwProcessId)
        {
            ASSERT(pentry->einfo.pobj->cExclusiveLock == 0);

            /* Reference the object and delete it */
            InterlockedIncrement((LONG*)&gpaulRefCount[ulIndex]);
            GDIOBJ_vDeleteObject(pentry->einfo.pobj);
        }
    }

#if DBG
    DbgGdiHTIntegrityCheck();
#endif

    ppi = PsGetCurrentProcessWin32Process();
    DPRINT("Completed cleanup for process %p\n", Process->UniqueProcessId);
    if (ppi->GDIHandleCount != 0)
    {
        DPRINT1("Leaking %d handles!\n", ppi->GDIHandleCount);
        ASSERT(FALSE);
    }

    /* Loop all handles in the handle table */
    for (ulIndex = RESERVE_ENTRIES_COUNT; ulIndex < gulFirstUnused; ulIndex++)
    {
        pentry = &gpentHmgr[ulIndex];

        /* Check if the object is owned by the process */
        if (pentry->ObjectOwner.ulObj == dwProcessId)
        {
            DPRINT1("Leaking object. Index=%lx, type=0x%x, refcount=%lx\n",
                    ulIndex, pentry->Objt, gpaulRefCount[ulIndex]);
            DBG_DUMP_EVENT_LIST(&pentry->einfo.pobj->slhLog);
            //DBG_CLEANUP_EVENT_LIST(&pentry->einfo.pobj->slhLog);
            ASSERT(FALSE);
        }
    }

    return TRUE;
}

/// HACK!
PGDI_POOL
GetBrushAttrPool(VOID)
{
    PPROCESSINFO ppi;

    ppi = PsGetCurrentProcessWin32Process();
    NT_ASSERT(ppi != NULL);

    return ppi->pPoolBrushAttr;
}

/* EOF */
