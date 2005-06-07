/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

#include <ddk/ntddk.h>

/* base address where the handle table is mapped to */
#define GDI_HANDLE_TABLE_BASE_ADDRESS (0x400000)

/* gdi handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x4000

#define GDI_GLOBAL_PROCESS (0x0)

#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000

#define GDI_HANDLE_REUSECNT_SHIFT 24

#define GDI_HANDLE_CREATE(i, t)    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) & GDI_HANDLE_TYPE_MASK)))
#define GDI_HANDLE_GET_INDEX(h)    (((ULONG_PTR)(h)) & GDI_HANDLE_INDEX_MASK)
#define GDI_HANDLE_GET_TYPE(h)     (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK)
#define GDI_HANDLE_IS_TYPE(h, t)   ((t) == (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK))
#define GDI_HANDLE_IS_STOCKOBJ(h)  (0 != (((ULONG_PTR)(h)) & GDI_HANDLE_STOCK_MASK))
#define GDI_HANDLE_SET_STOCKOBJ(h) ((h) = (HANDLE)(((ULONG_PTR)(h)) | GDI_HANDLE_STOCK_MASK))


/*! \defgroup GDI object types
 *
 *  GDI object types
 *
 */
/*@{*/
#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_METADC      0x00710000
#define GDI_OBJECT_TYPE_METAFILE    0x00720000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00730000
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/** Not really an object type. Forces GDI_FreeObj to be silent. */
#define GDI_OBJECT_TYPE_SILENT      0x80000000
/*@}*/

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

BOOL    INTERNAL_CALL GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle);
void    INTERNAL_CALL GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS Owner);
void    INTERNAL_CALL GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL    INTERNAL_CALL GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj);
VOID    INTERNAL_CALL GDIOBJ_UnlockObjByPtr(PGDIOBJ Object);

#define GDIOBJ_GetObjectType(Handle) \
  GDI_HANDLE_GET_TYPE(Handle)

#ifdef GDI_DEBUG

/* a couple macros for debugging GDIOBJ locking */
#define GDIOBJ_AllocObj(ty) GDIOBJ_AllocObjDbg(__FILE__,__LINE__,ty)
#define GDIOBJ_FreeObj(obj,ty) GDIOBJ_FreeObjDbg(__FILE__,__LINE__,obj,ty)
#define GDIOBJ_LockObj(obj,ty) GDIOBJ_LockObjDbg(__FILE__,__LINE__,obj,ty)
#define GDIOBJ_ShareLockObj(obj,ty) GDIOBJ_ShareLockObjDbg(__FILE__,__LINE__,obj,ty)

HGDIOBJ INTERNAL_CALL GDIOBJ_AllocObjDbg(const char* file, int line, ULONG ObjectType);
BOOL    INTERNAL_CALL GDIOBJ_FreeObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_ShareLockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType);

#else /* !GDI_DEBUG */

HGDIOBJ INTERNAL_CALL GDIOBJ_AllocObj(ULONG ObjectType);
BOOL    INTERNAL_CALL GDIOBJ_FreeObj (HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_LockObj (HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_ShareLockObj (HGDIOBJ hObj, DWORD ObjectType);

#endif /* GDI_DEBUG */

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

PVOID   INTERNAL_CALL GDI_MapHandleTable(PEPROCESS Process);

#endif
