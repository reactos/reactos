/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.2 1999/09/10 21:17:07 rex Exp $
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

