/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.6 2000/06/16 07:22:39 jfilby Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/gdiobj.h>

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJHDR  NewObj;
  
  NewObj = ExAllocatePool(PagedPool, Size + sizeof (GDIOBJHDR)); // FIXME: Allocate with tag of MAGIC?
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

BOOL  GDIOBJ_FreeObject (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  ObjHdr;

  ObjHdr = (PGDIOBJHDR)(((PCHAR)Obj) - sizeof (GDIOBJHDR));
  if (ObjHdr->wMagic != Magic)
    {
       return FALSE;
    }
  ExFreePool (ObjHdr);
  return TRUE;
}

HGDIOBJ  GDIOBJ_PtrToHandle (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  if (Obj == NULL) return NULL;
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

  if (Obj == NULL) return NULL;

  objHeader = (PGDIOBJHDR) Obj;

  /*  FIXME: Lock object for duration  */

  if ((objHeader->wMagic != Magic) && (Magic != GO_MAGIC_DONTCARE))
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



