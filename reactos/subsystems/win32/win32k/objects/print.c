/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

INT
STDCALL
NtGdiAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
FASTCALL
IntGdiEscape(PDC    dc,
             INT    Escape,
             INT    InSize,
             LPCSTR InData,
             LPVOID OutData)
{
  if (Escape == QUERYESCSUPPORT)
    return FALSE;

  UNIMPLEMENTED;
  return SP_ERROR;
}

INT
STDCALL
NtGdiEscape(HDC  hDC,
            INT  Escape,
            INT  InSize,
            LPCSTR  InData,
            LPVOID  OutData)
{
  PDC dc;
  INT ret;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return SP_ERROR;
  }

  /* TODO FIXME - don't pass umode buffer to an Int function */
  ret = IntGdiEscape(dc, Escape, InSize, InData, OutData);

  DC_UnlockDc( dc );
  return ret;
}

INT
STDCALL
IntEngExtEscape(
   SURFOBJ *Surface,
   INT      Escape,
   INT      InSize,
   LPVOID   InData,
   INT      OutSize,
   LPVOID   OutData)
{
   if (Escape == QUERYESCSUPPORT)
      return FALSE;

   DPRINT1("IntEngExtEscape is unimplemented. - Keep going and have a nice day\n");
   return -1;
}

INT
STDCALL
IntGdiExtEscape(
   PDC    dc,
   INT    Escape,
   INT    InSize,
   LPCSTR InData,
   INT    OutSize,
   LPSTR  OutData)
{
   BITMAPOBJ *BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   INT Result;

   /* FIXME - Handle BitmapObj == NULL !!!!!! */

   if ( NULL == dc->DriverFunctions.Escape )
   {
      Result = IntEngExtEscape(
         &BitmapObj->SurfObj,
         Escape,
         InSize,
         (PVOID)((ULONG_PTR)InData),
         OutSize,
         (PVOID)OutData);
   }
   else
   {
      Result = dc->DriverFunctions.Escape(
         &BitmapObj->SurfObj,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData );
   }
   BITMAPOBJ_UnlockBitmap(BitmapObj);

   return Result;
}

INT
STDCALL
NtGdiExtEscape(
   HDC    hDC,
   IN OPTIONAL PWCHAR pDriver,
   IN INT nDriver,
   INT    Escape,
   INT    InSize,
   OPTIONAL LPSTR UnsafeInData,
   INT    OutSize,
   OPTIONAL LPSTR  UnsafeOutData)
{
   PDC      pDC;
   LPVOID   SafeInData = NULL;
   LPVOID   SafeOutData = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   INT      Result;

   if (hDC == 0)
   {
       hDC = (HDC)UserGetWindowDC(NULL);
   }

   pDC = DC_LockDc(hDC);
   if ( pDC == NULL )
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return -1;
   }
   if ( pDC->IsIC )
   {
      DC_UnlockDc(pDC);
      return 0;
   }

   if ( InSize && UnsafeInData )
   {
      _SEH_TRY
      {
        ProbeForRead(UnsafeInData,
                     InSize,
                     1);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if (!NT_SUCCESS(Status))
      {
        DC_UnlockDc(pDC);
        SetLastNtError(Status);
        return -1;
      }

      SafeInData = ExAllocatePoolWithTag ( PagedPool, InSize, TAG_PRINT );
      if ( !SafeInData )
      {
         DC_UnlockDc(pDC);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }

      _SEH_TRY
      {
        /* pointers were already probed! */
        RtlCopyMemory(SafeInData,
                      UnsafeInData,
                      InSize);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if ( !NT_SUCCESS(Status) )
      {
         ExFreePool ( SafeInData );
         DC_UnlockDc(pDC);
         SetLastNtError(Status);
         return -1;
      }
   }

   if ( OutSize && UnsafeOutData )
   {
      _SEH_TRY
      {
        ProbeForWrite(UnsafeOutData,
                      OutSize,
                      1);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if (!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        goto freeout;
      }

      SafeOutData = ExAllocatePoolWithTag ( PagedPool, OutSize, TAG_PRINT );
      if ( !SafeOutData )
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
freeout:
         if ( SafeInData )
            ExFreePool ( SafeInData );
         DC_UnlockDc(pDC);
         return -1;
      }
   }

   Result = IntGdiExtEscape ( pDC, Escape, InSize, SafeInData, OutSize, SafeOutData );

   DC_UnlockDc(pDC);

   if ( SafeInData )
      ExFreePool ( SafeInData );

   if ( SafeOutData )
   {
      _SEH_TRY
      {
        /* pointers were already probed! */
        RtlCopyMemory(UnsafeOutData,
                      SafeOutData,
                      OutSize);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      ExFreePool ( SafeOutData );
      if ( !NT_SUCCESS(Status) )
      {
         SetLastNtError(Status);
         return -1;
      }
   }

   return Result;
}

INT
APIENTRY
NtGdiStartDoc(
    IN HDC hdc,
    IN DOCINFOW *pdi,
    OUT BOOL *pbBanding,
    IN INT iJob)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}
/* EOF */
