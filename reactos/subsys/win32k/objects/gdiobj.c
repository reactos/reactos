
#include <win32k/gdiobj.h>

HANDLE  GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJ  NewObj;
  
  NewObj = ExAllocatePool(NonPagedPool, Size);
  if (NewDC == NULL)
    {
      return  NULL;
    }

  RtlZeroMemory(NewObj, Size);
  NewObj->wMagic = Magic;
  KeInitializeSpinlock(&NewObj->Lock);
  
  return  NewObj;
}

