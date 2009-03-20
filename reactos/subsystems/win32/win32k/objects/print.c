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
APIENTRY
NtGdiAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
APIENTRY
NtGdiEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
IntGdiExtEscape(
   PDC    dc,
   INT    Escape,
   INT    InSize,
   LPCSTR InData,
   INT    OutSize,
   LPSTR  OutData)
{
   SURFACE *psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
   INT Result;

   /* FIXME - Handle psurf == NULL !!!!!! */

   if ( NULL == ((GDIDEVICE *)dc->ppdev)->DriverFunctions.Escape )
   {
      Result = IntEngExtEscape(
         &psurf->SurfObj,
         Escape,
         InSize,
         (PVOID)((ULONG_PTR)InData),
         OutSize,
         (PVOID)OutData);
   }
   else
   {
      Result = ((GDIDEVICE *)dc->ppdev)->DriverFunctions.Escape(
         &psurf->SurfObj,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData );
   }
   SURFACE_UnlockSurface(psurf);

   return Result;
}

INT
APIENTRY
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
       hDC = UserGetWindowDC(NULL);
   }

   pDC = DC_LockDc(hDC);
   if ( pDC == NULL )
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return -1;
   }
   if ( pDC->dctype == DC_TYPE_INFO)
   {
      DC_UnlockDc(pDC);
      return 0;
   }

   if ( InSize && UnsafeInData )
   {
      _SEH2_TRY
      {
        ProbeForRead(UnsafeInData,
                     InSize,
                     1);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

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

      _SEH2_TRY
      {
        /* pointers were already probed! */
        RtlCopyMemory(SafeInData,
                      UnsafeInData,
                      InSize);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      if ( !NT_SUCCESS(Status) )
      {
         ExFreePoolWithTag ( SafeInData, TAG_PRINT );
         DC_UnlockDc(pDC);
         SetLastNtError(Status);
         return -1;
      }
   }

   if ( OutSize && UnsafeOutData )
   {
      _SEH2_TRY
      {
        ProbeForWrite(UnsafeOutData,
                      OutSize,
                      1);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

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
            ExFreePoolWithTag ( SafeInData, TAG_PRINT );
         DC_UnlockDc(pDC);
         return -1;
      }
   }

   Result = IntGdiExtEscape ( pDC, Escape, InSize, SafeInData, OutSize, SafeOutData );

   DC_UnlockDc(pDC);

   if ( SafeInData )
      ExFreePoolWithTag ( SafeInData ,TAG_PRINT );

   if ( SafeOutData )
   {
      _SEH2_TRY
      {
        /* pointers were already probed! */
        RtlCopyMemory(UnsafeOutData,
                      SafeOutData,
                      OutSize);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      ExFreePoolWithTag ( SafeOutData, TAG_PRINT );
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
APIENTRY
NtGdiStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}
/* EOF */
