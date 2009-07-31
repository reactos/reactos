#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

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
//////////////////////////////////////////////////////////////////////////////
  PPAGED_LOOKASIDE_LIST LookasideLists;

  ULONG           FirstFree;
  ULONG           FirstUnused;

} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

extern PGDI_HANDLE_TABLE GdiHandleTable;

typedef struct _BASEOBJECT
{
  HGDIOBJ     hHmgr;
  ULONG       ulShareCount;
  USHORT      cExclusiveLock;
  USHORT      BaseFlags;
  PTHREADINFO Tid;
} BASEOBJECT, *PBASEOBJECT;

typedef BOOL (APIENTRY *GDICLEANUPPROC)(PVOID ObjectBody);

#if 0
extern BOOLEAN GDIOBJ_Init(void);
extern HGDIOBJ alloc_gdi_handle( GDIOBJHDR *obj, SHORT type);
extern void *free_gdi_handle( HGDIOBJ handle );
extern void *GDI_GetObjPtr( HGDIOBJ, SHORT );
extern void GDI_ReleaseObj( HGDIOBJ );
extern HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle );
extern BOOL GDI_dec_ref_count( HGDIOBJ handle );
#endif

enum BASEFLAGS
{
    BASEFLAG_LOOKASIDE = 0x80,

    /* ReactOS specific: */
    BASEFLAG_READY_TO_DIE = 0x1000
};

PGDI_HANDLE_TABLE APIENTRY GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject);
PVOID       APIENTRY GDI_MapHandleTable(PSECTION_OBJECT SectionObject, PEPROCESS Process);

BOOL        APIENTRY GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle);
BOOL        APIENTRY GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS Owner);
BOOL        APIENTRY GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL        APIENTRY GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj);
BOOL        APIENTRY GDIOBJ_ValidateHandle(HGDIOBJ hObj, ULONG ObjectType);
PBASEOBJECT APIENTRY GDIOBJ_AllocObj(UCHAR ObjectType);
PBASEOBJECT APIENTRY GDIOBJ_AllocObjWithHandle(ULONG ObjectType);
VOID        APIENTRY GDIOBJ_FreeObj (PBASEOBJECT pObj, UCHAR ObjectType);
BOOL        APIENTRY GDIOBJ_FreeObjByHandle (HGDIOBJ hObj, DWORD ObjectType);
PVOID       APIENTRY GDIOBJ_LockObj (HGDIOBJ hObj, DWORD ObjectType);
ULONG       FASTCALL GDIOBJ_UnlockObjByPtr(PBASEOBJECT Object);

PVOID       APIENTRY GDIOBJ_ShareLockObj (HGDIOBJ hObj, DWORD ObjectType);

/* Inlines */
ULONG
FORCEINLINE
GDIOBJ_ShareUnlockObjByPtr(PBASEOBJECT Object)
{
    HGDIOBJ hobj = Object->hHmgr;
    USHORT flags = Object->BaseFlags;
    INT cLocks = InterlockedDecrement((PLONG)&Object->ulShareCount);
    ASSERT(cLocks >= 0);
    if ((flags & BASEFLAG_READY_TO_DIE) && (cLocks == 0))
    {
        GDIOBJ_FreeObjByHandle(hobj, GDI_OBJECT_TYPE_DONTCARE);
    }
    return cLocks;
}

/* Handle mapping */
VOID NTAPI GDI_InitHandleMapping();
VOID NTAPI GDI_AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser);
HGDIOBJ NTAPI GDI_MapUserHandle(HGDIOBJ hUser);
VOID NTAPI GDI_RemoveHandleMapping(HGDIOBJ hUser);

#endif
