/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/print.c
 * PURPOSE:         Print functions
 * PROGRAMMER:
 */

#include <win32k.h>

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
    EngSetLastError(ERROR_INVALID_HANDLE);
    return SP_ERROR;
  }

  /* TODO: FIXME: Don't pass umode buffer to an Int function */
  ret = IntGdiEscape(dc, Escape, InSize, InData, OutData);

  DC_UnlockDc( dc );
  return ret;
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
   SURFACE *psurf;
   INT Result;

   if ((dc->ppdev->DriverFunctions.Escape == NULL) ||
       (dc->dclevel.pSurface == NULL))
   {
      Result = 0;
   }
   else
   {
      DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);
      psurf = dc->dclevel.pSurface;

      Result = dc->ppdev->DriverFunctions.Escape(
         &psurf->SurfObj,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData );

      DC_vFinishBlit(dc, NULL);
   }

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
      EngSetLastError(ERROR_INVALID_HANDLE);
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

      SafeInData = ExAllocatePoolWithTag ( PagedPool, InSize, GDITAG_TEMP );
      if ( !SafeInData )
      {
         DC_UnlockDc(pDC);
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }

      _SEH2_TRY
      {
        /* Pointers were already probed! */
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
         ExFreePoolWithTag ( SafeInData, GDITAG_TEMP );
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

      SafeOutData = ExAllocatePoolWithTag ( PagedPool, OutSize, GDITAG_TEMP );
      if ( !SafeOutData )
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
freeout:
         if ( SafeInData )
            ExFreePoolWithTag ( SafeInData, GDITAG_TEMP );
         DC_UnlockDc(pDC);
         return -1;
      }
   }

   Result = IntGdiExtEscape ( pDC, Escape, InSize, SafeInData, OutSize, SafeOutData );

   DC_UnlockDc(pDC);

   if ( SafeInData )
      ExFreePoolWithTag ( SafeInData ,GDITAG_TEMP );

   if ( SafeOutData )
   {
      _SEH2_TRY
      {
        /* Pointers were already probed! */
        RtlCopyMemory(UnsafeOutData,
                      SafeOutData,
                      OutSize);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      ExFreePoolWithTag ( SafeOutData, GDITAG_TEMP );
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
