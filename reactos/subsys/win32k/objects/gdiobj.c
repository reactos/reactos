/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.3 1999/10/29 01:58:20 rex Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/gdiobj.h>

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJHDR  NewObj;
  
  NewObj = ExAllocatePool(NonPagedPool, Size);
  if (NewObj == NULL)
    {
      return  NULL;
    }

  RtlZeroMemory(NewObj, Size);
  NewObj->wMagic = Magic;
#if 0
  KeInitializeSpinlock(&NewObj->Lock);
#endif
  
  return  NewObj;
}

HGDIOBJ  GDIOBJ_PtrToHandle (PGDIOBJ Obj, WORD Magic)
{
  if (((PGDIOBJHDR)Obj)->wMagic != Magic)
    {
      return  0;
    }
  
  return  (HGDIOBJ) Obj;
}

PGDIOBJ  GDIOBJ_HandleToPtr (HGDIOBJ Obj, WORD Magic)
{
  /*  FIXME: Lock object for duration  */
  if (((PGDIOBJHDR)Obj)->wMagic != Magic)
    {
      return  0;
    }

  return  (PGDIOBJ) Obj;
}

BOOL  GDIOBJ_LockObject (HGDIOBJ Obj)
{
  /* FIXME: write this  */
  return  TRUE;
}

BOOL  GDIOBJ_UnlockObject (HGDIOBJ Obj)
{
  /* FIXME: write this  */
  return  TRUE;
}

