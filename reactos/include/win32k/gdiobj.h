/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

#include <ddk/ntddk.h>

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
/*@}*/

typedef PVOID PGDIOBJ;

typedef BOOL (FASTCALL *GDICLEANUPPROC)(PGDIOBJ Obj);

/*!
 * GDI object header. This is a part of any GDI object
*/
typedef struct _GDIOBJHDR
{
  DWORD dwCount; 		/* reference count for the object */
  HANDLE hProcessId;
  GDICLEANUPPROC CleanupProc;
  WORD wTableIndex;
  WORD Magic;
  const char* lockfile;
  int lockline;
/*  FAST_MUTEX Lock;*/
  DWORD LockTid;
  DWORD LockCount;
} GDIOBJHDR, *PGDIOBJHDR;

typedef struct _GDIMULTILOCK
{
  HGDIOBJ hObj;
  PGDIOBJ pObj;
  DWORD	ObjectType;
} GDIMULTILOCK, *PGDIMULTILOCK;

HGDIOBJ FASTCALL GDIOBJ_AllocObj(WORD Size, DWORD ObjectType, GDICLEANUPPROC CleanupProcPtr);
BOOL    STDCALL  GDIOBJ_FreeObj (HGDIOBJ Obj, DWORD ObjectType, DWORD Flag);
PGDIOBJ FASTCALL GDIOBJ_LockObj (HGDIOBJ Obj, DWORD ObjectType);
BOOL    FASTCALL GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj);
BOOL    FASTCALL GDIOBJ_UnlockObj (HGDIOBJ Obj, DWORD ObjectType);
BOOL    FASTCALL GDIOBJ_UnlockMultipleObj(PGDIMULTILOCK pList, INT nObj);
DWORD   FASTCALL GDIOBJ_GetObjectType(HGDIOBJ ObjectHandle);
BOOL    FASTCALL GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle);
void    FASTCALL GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS Owner);
void    FASTCALL GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL    FASTCALL GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj);

/* a couple macros for debugging GDIOBJ locking */
#define GDIOBJ_LockObj(obj,ty) GDIOBJ_LockObjDbg(__FILE__,__LINE__,obj,ty)
#define GDIOBJ_UnlockObj(obj,ty) GDIOBJ_UnlockObjDbg(__FILE__,__LINE__,obj,ty)

#ifdef GDIOBJ_LockObj
PGDIOBJ FASTCALL GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ Obj, DWORD ObjectType);
#endif /* GDIOBJ_LockObj */

#ifdef GDIOBJ_UnlockObj
BOOL FASTCALL GDIOBJ_UnlockObjDbg (const char* file, int line, HGDIOBJ Obj, DWORD ObjectType);
#endif /* GDIOBJ_UnlockObj */

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

#endif
