/*
 *  GDI object common header definition
 *
 */

#pragma once

#define GDI_OBJECT_STACK_LEVELS 20

/* The first 10 entries are never used in windows, they are empty */
static const unsigned RESERVE_ENTRIES_COUNT = 10;

typedef struct _GDI_HANDLE_TABLE
{
/* The table must be located at the beginning of this structure so it can be
 * properly mapped!
 */
//////////////////////////////////////////////////////////////////////////////
  GDI_TABLE_ENTRY Entries[GDI_HANDLE_COUNT];
  DEVCAPS         DevCaps;                 // Device Capabilities.
  FLONG           flDeviceUniq;            // Device settings uniqueness.
  PVOID           pvLangPack;              // Language Pack.
  CFONT           cfPublic[GDI_CFONT_MAX]; // Public Fonts.
  DWORD           dwCFCount;


} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

extern PGDI_HANDLE_TABLE GdiHandleTable;

typedef PVOID PGDIOBJ;

typedef VOID (NTAPI *GDICLEANUPPROC)(PVOID ObjectBody);
typedef VOID (NTAPI *GDIOBJDELETEPROC)(PVOID ObjectBody);

/* Every GDI Object must have this standard type of header.
 * It's for thread locking. */
typedef struct _BASEOBJECT
{
    HGDIOBJ hHmgr;
    union {
        ULONG ulShareCount; /* For objects without a handle */
        DWORD dwThreadId;   /* Exclusive lock owner */
    };
    USHORT cExclusiveLock;
    USHORT BaseFlags;
    EX_PUSH_LOCK pushlock;
#if DBG_ENABLE_GDIOBJ_BACKTRACES
    PVOID apvBackTrace[GDI_OBJECT_STACK_LEVELS];
#endif
#if DBG_ENABLE_EVENT_LOGGING
    SLIST_HEADER slhLog;
#endif
} BASEOBJECT, *POBJ;

enum BASEFLAGS
{
    BASEFLAG_LOOKASIDE = 0x80,

    /* ReactOS specific: */
    BASEFLAG_READY_TO_DIE = 0x1000
};

typedef struct _CLIENTOBJ
{
    BASEOBJECT BaseObject;
} CLIENTOBJ, *PCLIENTOBJ;

enum _GDIOBJLAGS
{
    GDIOBJFLAG_DEFAULT    = 0x00,
    GDIOBJFLAG_IGNOREPID  = 0x01,
    GDIOBJFLAG_IGNORELOCK = 0x02
};

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitGdiHandleTable(VOID);

BOOL
NTAPI
GreIsHandleValid(
    HGDIOBJ hobj);

BOOL
NTAPI
GreDeleteObject(
    HGDIOBJ hObject);

ULONG
NTAPI
GreGetObjectOwner(
    HGDIOBJ hobj);

BOOL
NTAPI
GreSetObjectOwner(
    HGDIOBJ hobj,
    ULONG ulOwner);

BOOL
NTAPI
GreSetObjectOwnerEx(
    HGDIOBJ hobj,
    ULONG ulOwner,
    ULONG Flags);

INT
NTAPI
GreGetObject(
    IN HGDIOBJ hobj,
    IN INT cbCount,
    OUT PVOID pvBuffer);

POBJ
NTAPI
GDIOBJ_AllocateObject(
    UCHAR objt,
    ULONG cjSize,
    FLONG fl);

VOID
NTAPI
GDIOBJ_vDeleteObject(
    POBJ pobj);

POBJ
NTAPI
GDIOBJ_ReferenceObjectByHandle(
    HGDIOBJ hobj,
    UCHAR objt);

VOID
NTAPI
GDIOBJ_vReferenceObjectByPointer(
    POBJ pobj);

VOID
NTAPI
GDIOBJ_vDereferenceObject(
    POBJ pobj);

PGDIOBJ
NTAPI
GDIOBJ_LockObject(
    HGDIOBJ hobj,
    UCHAR objt);

PGDIOBJ
NTAPI
GDIOBJ_TryLockObject(
    HGDIOBJ hobj,
    UCHAR objt);

VOID
NTAPI
GDIOBJ_vUnlockObject(
    POBJ pobj);

VOID
NTAPI
GDIOBJ_vSetObjectOwner(
    POBJ pobj,
    ULONG ulOwner);

BOOL
NTAPI
GDIOBJ_bLockMultipleObjects(
    ULONG ulCount,
    HGDIOBJ* ahObj,
    PGDIOBJ* apObj,
    UCHAR objt);

HGDIOBJ
NTAPI
GDIOBJ_hInsertObject(
    POBJ pobj,
    ULONG ulOwner);

VOID
NTAPI
GDIOBJ_vFreeObject(
    POBJ pobj);

VOID
NTAPI
GDIOBJ_vSetObjectAttr(
    POBJ pobj,
    PVOID pvObjAttr);

PVOID
NTAPI
GDIOBJ_pvGetObjectAttr(
    POBJ pobj);

BOOL    NTAPI GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj);
BOOL    NTAPI GDIOBJ_ConvertFromStockObj(HGDIOBJ *phObj);
POBJ    NTAPI GDIOBJ_AllocObjWithHandle(ULONG ObjectType, ULONG cjSize);
PGDIOBJ NTAPI GDIOBJ_ShareLockObj(HGDIOBJ hObj, DWORD ObjectType);
PVOID   NTAPI GDI_MapHandleTable(PEPROCESS Process);
