/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.4 1999/11/17 20:54:05 rex Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/gdiobj.h>

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJHDR  NewObj;
  
  NewObj = ExAllocatePool(PagedPool, Size + sizeof (GDIOBJHDR));
  if (NewObj == NULL)
    {
      return  NULL;
    }

  RtlZeroMemory(NewObj, Size + sizeof (GDIOBJHDR));
  NewObj->wMagic = Magic;
#if 0
  KeInitializeSpinlock(&NewObj->Lock);
#endif
  
  return  (PGDIOBJ)(((PCHAR) NewObj) + sizeof (GDIOBJHDR));
}

HGDIOBJ  GDIOBJ_PtrToHandle (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  objHeader = (PGDIOBJHDR) (((PCHAR)Obj) - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
    {
      return  0;
    }
  
  return  (HGDIOBJ) objHeader;
}

PGDIOBJ  GDIOBJ_HandleToPtr (HGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  objHeader = (PGDIOBJHDR) Obj;
  
  /*  FIXME: Lock object for duration  */

  if (objHeader->wMagic != Magic)
    {
      return  0;
    }

  return  (PGDIOBJ) (((PCHAR)Obj) + sizeof (GDIOBJHDR));
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

HGDIOBJ  GDIOBJ_GetNextObject (HGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  objHeader = (PGDIOBJHDR) ((PCHAR) Obj - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
    {
      return 0;
    }

  return objHeader->hNext;
}

HGDIOBJ  GDIOBJ_SetNextObject (HGDIOBJ Obj, WORD Magic, HGDIOBJ NextObj)
{
  PGDIOBJHDR  objHeader;
  HGDIOBJ  oldNext;
  
  /* FIXME: should we lock/unlock the object here? */
  objHeader = (PGDIOBJHDR) ((PCHAR) Obj - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
    {
      return  0;
    }
  oldNext = objHeader->hNext;
  objHeader->hNext = NextObj;
  
  return  oldNext;
}



