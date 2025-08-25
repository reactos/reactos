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
IntExtEscape(
   HDC hdc,
    INT    iEsc,
    INT    cjIn,
    LPSTR  pjIn,
    INT    cjOut,
    LPSTR  pjOut )
{
    INT ret;
    PPDEVOBJ ppdev;
    SURFACE *psurf;
    KFLOATING_SAVE TempBuffer;

    if (hdc == NULL)
    {
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
       PDC pDC = DC_LockDc(hdc);
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

    ret = KeSaveFloatingPointState(&TempBuffer);
    ret = ppdev->DriverFunctions.Escape( &psurf->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
    KeRestoreFloatingPointState(&TempBuffer);


    SURFACE_ShareUnlockSurface(psurf);
    if (hdc == NULL)
    {
       EngReleaseSemaphore(ppdev->hsemDevLock);
    }
    PDEVOBJ_vRelease(ppdev);

    return ret;
}

INT
APIENTRY
IntNameExtEscape(
    PWCHAR pDriver,
    INT    iEsc,
    INT    cjIn,
    LPSTR  pjIn,
    INT    cjOut,
    LPSTR  pjOut )
{
    PPDEVOBJ ppdev;
    UNICODE_STRING usDriver;
    WCHAR awcBuffer[MAX_PATH];
    RtlInitEmptyUnicodeString(&usDriver, awcBuffer, sizeof(awcBuffer));
    RtlAppendUnicodeToString(&usDriver, L"\\SystemRoot\\System32\\");
    RtlAppendUnicodeToString(&usDriver, pDriver);

    ppdev = EngpGetPDEV(&usDriver);
    if ( ppdev && ppdev->DriverFunctions.Escape )
    {
       return ppdev->DriverFunctions.Escape( &ppdev->pSurface->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
    }
    return 0;
}

INT
APIENTRY
NtGdiExtEscape(
    _In_opt_ HDC hdc,
    _In_reads_opt_(cwcDriver) PWCHAR pDriver,
    _In_ INT cwcDriver,
    _In_ INT iEsc,
    _In_ INT cjIn,
    _In_reads_bytes_opt_(cjIn) LPSTR pjIn,
    _In_ INT cjOut,
    _Out_writes_bytes_opt_(cjOut) LPSTR pjOut )
{
    LPSTR SafeInData = NULL;
    LPSTR SafeOutData = NULL;
    PWCHAR psafeDriver = NULL;
    INT Ret = -1;
    if (pDriver)
    {
       /* FIXME : Get the pdev from its name */
       UNIMPLEMENTED;
       return -1;
    }
    if ( pDriver )
    {
        psafeDriver = ExAllocatePoolWithTag( PagedPool, (cwcDriver + 1) * sizeof(WCHAR), GDITAG_TEMP );
        RtlZeroMemory( psafeDriver, (cwcDriver + 1) * sizeof(WCHAR) );
    }

    if ( cjIn )
    {
       SafeInData = ExAllocatePoolWithTag( PagedPool, cjIn + 1, GDITAG_TEMP );
    }

    if ( cjOut )
    {
       SafeOutData = ExAllocatePoolWithTag( PagedPool, cjOut + 1, GDITAG_TEMP );
    }

    _SEH2_TRY
    {
        if ( psafeDriver )
        {
            ProbeForRead(pDriver, cwcDriver * sizeof(WCHAR), 1);
            RtlCopyMemory(psafeDriver, pDriver, cwcDriver * sizeof(WCHAR));
        }
        if ( SafeInData )
        {
            ProbeForRead(pjIn, cjIn, 1);
            RtlCopyMemory(SafeInData, pjIn, cjIn);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(goto Exit);
    }
    _SEH2_END;
     
    if ( SafeOutData )
    {
        _SEH2_TRY
        {
            ProbeForWrite(pjOut, cjOut, 1);
            RtlCopyMemory(pjOut, SafeOutData, cjOut);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = -1;
        }
        _SEH2_END;
    }


    if ( pDriver )
    {
        Ret = IntNameExtEscape( psafeDriver, iEsc, cjIn, SafeInData, cjOut, SafeOutData );
    }
    else
    {
      
            Ret = IntExtEscape( hdc, iEsc, cjIn, SafeInData, cjOut, SafeOutData );
    }

    if ( SafeOutData )
    {
        _SEH2_TRY
        {
            RtlCopyMemory(pjOut, SafeOutData, cjOut);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = -1;
        }
        _SEH2_END;
    }
Exit:
    if ( psafeDriver ) ExFreePoolWithTag ( psafeDriver, GDITAG_TEMP );
    if ( SafeInData ) ExFreePoolWithTag ( SafeInData, GDITAG_TEMP );
    if ( SafeOutData ) ExFreePoolWithTag ( SafeOutData, GDITAG_TEMP );

    return Ret;
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
