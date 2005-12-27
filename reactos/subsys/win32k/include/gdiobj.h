/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

/* Public GDI Object/Handle definitions */
#include <win32k/ntgdihdl.h>

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

PVOID   INTERNAL_CALL GDI_MapHandleTable(PEPROCESS Process);

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

#endif
