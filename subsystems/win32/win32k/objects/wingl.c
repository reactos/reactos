/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/wingl.c
 * PURPOSE:         WinGL API
 * PROGRAMMER:      
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static
INT
FASTCALL
IntGetipfdDevMax(PDC pdc)
{
  INT Ret = 0;
  PPDEVOBJ ppdev = pdc->ppdev;

  if (ppdev->flFlags & PDEV_META_DEVICE)
  {
     return 0;
  }

  if (ppdev->DriverFunctions.DescribePixelFormat)
  {
     Ret = ppdev->DriverFunctions.DescribePixelFormat(
                                                ppdev->dhpdev,
                                                1,
                                                0,
                                                NULL);
  }

  if (Ret) pdc->ipfdDevMax = Ret;

  return Ret;
}


INT
APIENTRY
NtGdiDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             LPPIXELFORMATDESCRIPTOR  pfd)
{
  PDC pdc;
  PPDEVOBJ ppdev;
  INT Ret = 0;
  PIXELFORMATDESCRIPTOR pfdSafe;
  NTSTATUS Status = STATUS_SUCCESS;

  if (!BufSize) return 0;

  pdc = DC_LockDc(hDC);
  if (!pdc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  if (!pdc->ipfdDevMax) IntGetipfdDevMax(pdc);

  if ( BufSize < sizeof(PIXELFORMATDESCRIPTOR) ||
       PixelFormat < 1 ||
       PixelFormat > pdc->ipfdDevMax )
  {  
     EngSetLastError(ERROR_INVALID_PARAMETER);
     goto Exit;
  }

  ppdev = pdc->ppdev;

  if (ppdev->flFlags & PDEV_META_DEVICE)
  {
     UNIMPLEMENTED;
     goto Exit;
  }

  if (ppdev->DriverFunctions.DescribePixelFormat)
  {
     Ret = ppdev->DriverFunctions.DescribePixelFormat(
                                                ppdev->dhpdev,
                                                PixelFormat,
                                                sizeof(PIXELFORMATDESCRIPTOR),
                                                &pfdSafe);
  }

  _SEH2_TRY
  {
     ProbeForWrite( pfd,
                    sizeof(PIXELFORMATDESCRIPTOR),
                    1);
     RtlCopyMemory(&pfdSafe, pfd, sizeof(PIXELFORMATDESCRIPTOR));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;

  if (!NT_SUCCESS(Status)) SetLastNtError(Status);

Exit:
  DC_UnlockDc(pdc);
  return Ret;
}


BOOL
APIENTRY
NtGdiSetPixelFormat(
    IN HDC hdc,
    IN INT ipfd)
{
  PDC pdc;
  PPDEVOBJ ppdev;
  HWND hWnd;
  PWNDOBJ pWndObj;
  SURFOBJ *pso = NULL;
  BOOL Ret = FALSE;

  pdc = DC_LockDc(hdc);
  if (!pdc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  if (!pdc->ipfdDevMax) IntGetipfdDevMax(pdc);

  if ( ipfd < 1 ||
       ipfd > pdc->ipfdDevMax )
  {  
     EngSetLastError(ERROR_INVALID_PARAMETER);
     goto Exit;
  }

  UserEnterExclusive();
  hWnd = UserGethWnd(hdc, &pWndObj);
  UserLeave();

  if (!hWnd)
  {
     EngSetLastError(ERROR_INVALID_WINDOW_STYLE);
     goto Exit;
  }

  ppdev = pdc->ppdev;

  /*
      WndObj is needed so exit on NULL pointer.
   */
  if (pWndObj) pso = pWndObj->psoOwner;
  else
  {
     EngSetLastError(ERROR_INVALID_PIXEL_FORMAT);
     goto Exit;
  }

  if (ppdev->flFlags & PDEV_META_DEVICE)
  {
     UNIMPLEMENTED;
     goto Exit;
  }

  if (ppdev->DriverFunctions.SetPixelFormat)
  {
     Ret = ppdev->DriverFunctions.SetPixelFormat(
                                                pso,
                                                ipfd,
                                                hWnd);
  }

Exit:
  DC_UnlockDc(pdc);
  return Ret;
}

BOOL
APIENTRY
NtGdiSwapBuffers(HDC  hDC)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
