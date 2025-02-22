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
   LPVOID   SafeInData = NULL;
   LPVOID   SafeOutData = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   INT      Result;
   PPDEVOBJ ppdev;
   PSURFACE psurf;

   if (hDC == NULL)
   {
      if (pDriver)
      {
         /* FIXME : Get the pdev from its name */
         UNIMPLEMENTED;
         return -1;
      }

      ppdev = EngpGetPDEV(NULL);
      if (!ppdev)
      {
         EngSetLastError(ERROR_BAD_DEVICE);
         return -1;
      }

      /* We're using the primary surface of the pdev. Lock it */
      EngAcquireSemaphore(ppdev->hsemDevLock);

      psurf = ppdev->pSurface;
      if (!psurf)
      {
         EngReleaseSemaphore(ppdev->hsemDevLock);
         PDEVOBJ_vRelease(ppdev);
         return 0;
      }
      SURFACE_ShareLockByPointer(psurf);
   }
   else
   {
      PDC pDC = DC_LockDc(hDC);
      if ( pDC == NULL )
      {
         EngSetLastError(ERROR_INVALID_HANDLE);
         return -1;
      }

      /* Get the PDEV from the DC */
      ppdev = pDC->ppdev;
      PDEVOBJ_vReference(ppdev);

      /* Check if we have a surface */
      psurf = pDC->dclevel.pSurface;
      if (!psurf)
      {
         DC_UnlockDc(pDC);
         PDEVOBJ_vRelease(ppdev);
         return 0;
      }
      SURFACE_ShareLockByPointer(psurf);

      /* We're done with the DC */
      DC_UnlockDc(pDC);
   }

   /* See if we actually have a driver function to call */
   if (ppdev->DriverFunctions.Escape == NULL)
   {
      Result = 0;
      goto Exit;
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
         Result = -1;
         goto Exit;
      }

      SafeInData = ExAllocatePoolWithTag ( PagedPool, InSize, GDITAG_TEMP );
      if ( !SafeInData )
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         Result = -1;
         goto Exit;
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
         SetLastNtError(Status);
         Result = -1;
         goto Exit;
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
         Result = -1;
         goto Exit;
      }

      SafeOutData = ExAllocatePoolWithTag ( PagedPool, OutSize, GDITAG_TEMP );
      if ( !SafeOutData )
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         Result = -1;
         goto Exit;
      }
   }

   /* Finally call the driver */
   Result = ppdev->DriverFunctions.Escape(
         &psurf->SurfObj,
         Escape,
         InSize,
         SafeInData,
         OutSize,
         SafeOutData );

Exit:
   if (hDC == NULL)
   {
      EngReleaseSemaphore(ppdev->hsemDevLock);
   }
   SURFACE_ShareUnlockSurface(psurf);
   PDEVOBJ_vRelease(ppdev);

   if ( SafeInData )
   {
      ExFreePoolWithTag ( SafeInData ,GDITAG_TEMP );
   }

   if ( SafeOutData )
   {
      if (Result > 0)
      {
         _SEH2_TRY
         {
            /* Pointers were already probed! */
            RtlCopyMemory(UnsafeOutData, SafeOutData, OutSize);
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            Status = _SEH2_GetExceptionCode();
         }
         _SEH2_END;

         if ( !NT_SUCCESS(Status) )
         {
            SetLastNtError(Status);
            Result = -1;
         }
      }

      ExFreePoolWithTag ( SafeOutData, GDITAG_TEMP );
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
