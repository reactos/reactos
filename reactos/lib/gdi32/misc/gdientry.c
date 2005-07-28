/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * $Id:
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS GDI32
 * PURPOSE:          Gdi DirectX inteface
 * FILE:             lib/gdi32/misc/gdientry.c
 * PROGRAMER:        Magnus Olsen (magnus@greatlord.com)
 * REVISION HISTORY: 
 * NOTES:            
 */


#include "precomp.h"
static LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobalInternal;


/*
 * @implemented
 *
 * GDIEntry 1 
 */
BOOL 
STDCALL 
DdCreateDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                         HDC hdc)
{  
  HDC newHdc;
  /* check see if HDC is NULL or not  
     if it null we need create the DC */

  if (hdc != NULL) 
  {
    pDirectDrawGlobal->hDD = (ULONG_PTR)NtGdiDdCreateDirectDrawObject(hdc); 
    
    /* if hDD ==NULL */
    if (!pDirectDrawGlobal->hDD) 
    {
      return FALSE;
    }
    return TRUE;
  }

  /* The hdc was not null we need check see if we alread 
     have create a directdraw handler */

  /* if hDD !=NULL */
  if (pDirectDrawGlobalInternal->hDD)
  {
    /* we have create a directdraw handler already */

    pDirectDrawGlobal->hDD = pDirectDrawGlobalInternal->hDD;    
    return TRUE;
  }

  /* Now we create the DC */
  newHdc = CreateDC(L"DISPLAY\0", NULL, NULL, NULL);

  /* we are checking if we got a hdc or not */
  if ((ULONG_PTR)newHdc != pDirectDrawGlobalInternal->hDD)
  {
    pDirectDrawGlobalInternal->hDD = (ULONG_PTR) NtGdiDdCreateDirectDrawObject(newHdc);
    NtGdiDeleteDC(newHdc);
  }

   /* pDirectDrawGlobal->hDD = pDirectDrawGlobalInternal->hDD; ? */
   pDirectDrawGlobal->hDD = 0; /* ? */

  /* test see if we got a handler */
  if (!pDirectDrawGlobalInternal->hDD)
  {       
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 *
 * GDIEntry 3
 */
BOOL 
STDCALL 
DdDeleteDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal)
{
  /* if pDirectDrawGlobal->hDD == NULL and pDirectDrawGlobalInternal->hDD == NULL
     return false */

  if (!pDirectDrawGlobal->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
    return NtGdiDdDeleteDirectDrawObject((HANDLE)pDirectDrawGlobalInternal->hDD); 
  }

  return NtGdiDdDeleteDirectDrawObject((HANDLE)pDirectDrawGlobal->hDD); 	
}


/*
 * @implemented
 *
 * GDIEntry 5
 */
BOOL 
STDCALL 
DdDeleteSurfaceObject(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal)
{
  if (!pSurfaceLocal->hDDSurface)
  {
    return FALSE;
  }

  return NtGdiDdDeleteSurfaceObject((HANDLE)pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 6
 */
BOOL 
STDCALL 
DdResetVisrgn(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal, 
              HWND hWnd)
{
 return NtGdiDdResetVisrgn((HANDLE) pSurfaceLocal->hDDSurface, hWnd);
}

/*
 * @implemented
 *
 * GDIEntry 7
 */
HDC STDCALL DdGetDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
LPPALETTEENTRY pColorTable
)
{
	return NtGdiDdGetDC(pColorTable, (HANDLE) pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 8
 */
BOOL STDCALL DdReleaseDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
)
{
 return NtGdiDdReleaseDC((HANDLE) pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 10
 */
BOOL 
STDCALL 
DdReenableDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                           BOOL *pbNewMode)
{
 if (!pDirectDrawGlobal->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
    return NtGdiDdReenableDirectDrawObject((HANDLE)pDirectDrawGlobalInternal->hDD, pbNewMode); 
  }

  return NtGdiDdReenableDirectDrawObject((HANDLE)pDirectDrawGlobal->hDD, pbNewMode); 	
} 
