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
                                                ppdev->hPDev,
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
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return 0;
  }

  if (!pdc->ipfdDevMax) IntGetipfdDevMax(pdc);

  if ( BufSize < sizeof(PIXELFORMATDESCRIPTOR) ||
       PixelFormat < 1 ||
       PixelFormat > pdc->ipfdDevMax )
  {  
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
                                                ppdev->hPDev,
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
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  if (!pdc->ipfdDevMax) IntGetipfdDevMax(pdc);

  if ( ipfd < 1 ||
       ipfd > pdc->ipfdDevMax )
  {  
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     goto Exit;
  }

  UserEnterExclusive();
  hWnd = UserGethWnd(hdc, &pWndObj);
  UserLeave();

  if (!hWnd)
  {
     SetLastWin32Error(ERROR_INVALID_WINDOW_STYLE);
     goto Exit;
  }

  ppdev = pdc->ppdev;

  /*
      WndObj is needed so exit on NULL pointer.
   */
  if (pWndObj) pso = pWndObj->psoOwner;
  else
  {
     SetLastWin32Error(ERROR_INVALID_PIXEL_FORMAT);
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
