/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

/* Public GDI Object/Handle definitions */
#include <win32k/ntgdihdl.h>

typedef struct _GDI_HANDLE_TABLE
{
  /* the table must be located at the beginning of this structure so it can be
     properly mapped! */
  GDI_TABLE_ENTRY Entries[GDI_HANDLE_COUNT];

  PPAGED_LOOKASIDE_LIST LookasideLists;

  SLIST_HEADER FreeEntriesHead;
  SLIST_ENTRY FreeEntries[((GDI_HANDLE_COUNT * sizeof(GDI_TABLE_ENTRY)) << 3) /
                          (sizeof(SLIST_ENTRY) << 3)];
} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

extern PGDI_HANDLE_TABLE GdiHandleTable;

typedef PVOID PGDIOBJ;

typedef BOOL (INTERNAL_CALL *GDICLEANUPPROC)(PVOID ObjectBody);

/*!
 * GDI object header. This is a part of any GDI object
*/
typedef struct _GDIOBJHDR
{
  PETHREAD LockingThread; /* only assigned if a thread is holding the lock! */
  ULONG Locks;
#ifdef GDI_DEBUG
  const char* createdfile;
  int createdline;
  const char* lockfile;
  int lockline;
#endif
} GDIOBJHDR, *PGDIOBJHDR;

//
// Every GDI Object must have this standard type of header.
// It's for thread locking.
// This header is standalone, used only in gdiobj.c.
//
typedef struct _GDIOBJEMPTYHDR
{
  HGDIOBJ     hHmgr;
  PVOID       pvEntry;
  ULONG       lucExcLock;
  ULONG       Tid;
} GDIOBJEMPTYHDR, *PGDIOBJEMPTYHDR;

BOOL    INTERNAL_CALL GDIOBJ_OwnedByCurrentProcess(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ ObjectHandle);
void    INTERNAL_CALL GDIOBJ_SetOwnership(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ ObjectHandle, PEPROCESS Owner);
void    INTERNAL_CALL GDIOBJ_CopyOwnership(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL    INTERNAL_CALL GDIOBJ_ConvertToStockObj(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ *hObj);
VOID    INTERNAL_CALL GDIOBJ_UnlockObjByPtr(PGDI_HANDLE_TABLE HandleTable, PGDIOBJ Object);

#define GDIOBJ_GetObjectType(Handle) \
  GDI_HANDLE_GET_TYPE(Handle)

#ifdef GDI_DEBUG

/* a couple macros for debugging GDIOBJ locking */
#define GDIOBJ_AllocObj(ht,ty) GDIOBJ_AllocObjDbg(ht,__FILE__,__LINE__,ty)
#define GDIOBJ_FreeObj(ht,obj,ty) GDIOBJ_FreeObjDbg(ht,__FILE__,__LINE__,obj,ty)
#define GDIOBJ_LockObj(ht,obj,ty) GDIOBJ_LockObjDbg(ht,__FILE__,__LINE__,obj,ty)
#define GDIOBJ_ShareLockObj(ht,obj,ty) GDIOBJ_ShareLockObjDbg(ht,__FILE__,__LINE__,obj,ty)

HGDIOBJ INTERNAL_CALL GDIOBJ_AllocObjDbg(PGDI_HANDLE_TABLE HandleTable, const char* file, int line, ULONG ObjectType);
BOOL    INTERNAL_CALL GDIOBJ_FreeObjDbg (PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_LockObjDbg (PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_ShareLockObjDbg (PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);

#else /* !GDI_DEBUG */

HGDIOBJ INTERNAL_CALL GDIOBJ_AllocObj(PGDI_HANDLE_TABLE HandleTable, ULONG ObjectType);
BOOL    INTERNAL_CALL GDIOBJ_FreeObj (PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_LockObj (PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_ShareLockObj (PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType);

#endif /* GDI_DEBUG */

PVOID   INTERNAL_CALL GDI_MapHandleTable(PSECTION_OBJECT SectionObject, PEPROCESS Process);

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

BOOL FASTCALL  NtGdiDeleteObject(HGDIOBJ hObject);

#endif
