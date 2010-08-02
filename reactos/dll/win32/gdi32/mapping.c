/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);


/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
static void MAPPING_FixIsotropic( DC * dc )
{
    double xdim = fabs((double)dc->vportExtX * dc->virtual_size.cx /
                  (dc->virtual_res.cx * dc->wndExtX));
    double ydim = fabs((double)dc->vportExtY * dc->virtual_size.cy /
                  (dc->virtual_res.cy * dc->wndExtY));

    if (xdim > ydim)
    {
        INT mincx = (dc->vportExtX >= 0) ? 1 : -1;
        dc->vportExtX = floor(dc->vportExtX * ydim / xdim + 0.5);
        if (!dc->vportExtX) dc->vportExtX = mincx;
    }
    else
    {
        INT mincy = (dc->vportExtY >= 0) ? 1 : -1;
        dc->vportExtY = floor(dc->vportExtY * xdim / ydim + 0.5);
        if (!dc->vportExtY) dc->vportExtY = mincy;
    }
}


/***********************************************************************
 *           DPtoLP    (GDI32.@)
 */
BOOL WINAPI DPtoLP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (dc->vport2WorldValid)
    {
        while (count--)
        {
            double x = points->x;
            double y = points->y;
            points->x = floor( x * dc->xformVport2World.eM11 +
                               y * dc->xformVport2World.eM21 +
                               dc->xformVport2World.eDx + 0.5 );
            points->y = floor( x * dc->xformVport2World.eM12 +
                               y * dc->xformVport2World.eM22 +
                               dc->xformVport2World.eDy + 0.5 );
            points++;
        }
    }
    release_dc_ptr( dc );
    return (count < 0);
}


/***********************************************************************
 *           LPtoDP    (GDI32.@)
 */
BOOL WINAPI LPtoDP( HDC hdc, LPPOINT points, INT count )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    while (count--)
    {
        double x = points->x;
        double y = points->y;
        points->x = floor( x * dc->xformWorld2Vport.eM11 +
                           y * dc->xformWorld2Vport.eM21 +
                           dc->xformWorld2Vport.eDx + 0.5 );
        points->y = floor( x * dc->xformWorld2Vport.eM12 +
                           y * dc->xformWorld2Vport.eM22 +
                           dc->xformWorld2Vport.eDy + 0.5 );
        points++;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           SetMapMode    (GDI32.@)
 */
INT WINAPI SetMapMode( HDC hdc, INT mode )
{
    INT ret;
    INT horzSize, vertSize, horzRes, vertRes;

    DC * dc = get_dc_ptr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetMapMode)
    {
        if((ret = dc->funcs->pSetMapMode( dc->physDev, mode )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }

    TRACE("%p %d\n", hdc, mode );

    ret = dc->MapMode;

    if (mode == dc->MapMode && (mode == MM_ISOTROPIC || mode == MM_ANISOTROPIC))
        goto done;

    horzSize = dc->virtual_size.cx;
    vertSize = dc->virtual_size.cy;
    horzRes  = dc->virtual_res.cx;
    vertRes  = dc->virtual_res.cy;
    switch(mode)
    {
    case MM_TEXT:
        dc->wndExtX   = 1;
        dc->wndExtY   = 1;
        dc->vportExtX = 1;
        dc->vportExtY = 1;
        break;
    case MM_LOMETRIC:
    case MM_ISOTROPIC:
        dc->wndExtX   = horzSize * 10;
        dc->wndExtY   = vertSize * 10;
        dc->vportExtX = horzRes;
        dc->vportExtY = -vertRes;
        break;
    case MM_HIMETRIC:
        dc->wndExtX   = horzSize * 100;
        dc->wndExtY   = vertSize * 100;
        dc->vportExtX = horzRes;
        dc->vportExtY = -vertRes;
        break;
    case MM_LOENGLISH:
        dc->wndExtX   = MulDiv(1000, horzSize, 254);
        dc->wndExtY   = MulDiv(1000, vertSize, 254);
        dc->vportExtX = horzRes;
        dc->vportExtY = -vertRes;
        break;
    case MM_HIENGLISH:
        dc->wndExtX   = MulDiv(10000, horzSize, 254);
        dc->wndExtY   = MulDiv(10000, vertSize, 254);
        dc->vportExtX = horzRes;
        dc->vportExtY = -vertRes;
        break;
    case MM_TWIPS:
        dc->wndExtX   = MulDiv(14400, horzSize, 254);
        dc->wndExtY   = MulDiv(14400, vertSize, 254);
        dc->vportExtX = horzRes;
        dc->vportExtY = -vertRes;
        break;
    case MM_ANISOTROPIC:
        break;
    default:
        goto done;
    }
    /* RTL layout is always MM_ANISOTROPIC */
    if (!(dc->layout & LAYOUT_RTL)) dc->MapMode = mode;
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx    (GDI32.@)
 */
BOOL WINAPI SetViewportExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportExt)
    {
        if((ret = dc->funcs->pSetViewportExt( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = x;
    dc->vportExtY = y;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI SetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetViewportOrg)
    {
        if((ret = dc->funcs->pSetViewportOrg( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (pt)
    {
        pt->x = dc->vportOrgX;
	pt->y = dc->vportOrgY;
    }
    dc->vportOrgX = x;
    dc->vportOrgY = y;
    DC_UpdateXforms( dc );

 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx    (GDI32.@)
 */
BOOL WINAPI SetWindowExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowExt)
    {
        if((ret = dc->funcs->pSetWindowExt( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!x || !y)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = x;
    dc->wndExtY = y;
    /* The API docs say that you should call SetWindowExtEx before
       SetViewportExtEx. This advice does not imply that Windows
       doesn't ensure the isotropic mapping after SetWindowExtEx! */
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI SetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetWindowOrg)
    {
        if((ret = dc->funcs->pSetWindowOrg( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (pt)
    {
        pt->x = dc->wndOrgX;
	pt->y = dc->wndOrgY;
    }
    dc->wndOrgX = x;
    dc->wndOrgY = y;
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetViewportOrgEx( HDC hdc, INT x, INT y, LPPOINT pt)
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetViewportOrg)
    {
        if((ret = dc->funcs->pOffsetViewportOrg( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (pt)
    {
        pt->x = dc->vportOrgX;
	pt->y = dc->vportOrgY;
    }
    dc->vportOrgX += x;
    dc->vportOrgY += y;
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetWindowOrgEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pOffsetWindowOrg)
    {
        if((ret = dc->funcs->pOffsetWindowOrg( dc->physDev, x, y )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (pt)
    {
        pt->x = dc->wndOrgX;
	pt->y = dc->wndOrgY;
    }
    dc->wndOrgX += x;
    dc->wndOrgY += y;
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleViewportExtEx( HDC hdc, INT xNum, INT xDenom,
                                    INT yNum, INT yDenom, LPSIZE size )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleViewportExt)
    {
        if((ret = dc->funcs->pScaleViewportExt( dc->physDev, xNum, xDenom, yNum, yDenom )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (size)
    {
	size->cx = dc->vportExtX;
	size->cy = dc->vportExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !yNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->vportExtX = (dc->vportExtX * xNum) / xDenom;
    dc->vportExtY = (dc->vportExtY * yNum) / yDenom;
    if (dc->vportExtX == 0) dc->vportExtX = 1;
    if (dc->vportExtY == 0) dc->vportExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleWindowExtEx( HDC hdc, INT xNum, INT xDenom,
                                  INT yNum, INT yDenom, LPSIZE size )
{
    INT ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pScaleWindowExt)
    {
        if((ret = dc->funcs->pScaleWindowExt( dc->physDev, xNum, xDenom, yNum, yDenom )) != TRUE)
	{
	    if(ret == GDI_NO_MORE_WORK)
	        ret = TRUE;
	    goto done;
	}
    }
    if (size)
    {
	size->cx = dc->wndExtX;
	size->cy = dc->wndExtY;
    }
    if ((dc->MapMode != MM_ISOTROPIC) && (dc->MapMode != MM_ANISOTROPIC))
	goto done;
    if (!xNum || !xDenom || !xNum || !yDenom)
    {
        ret = FALSE;
        goto done;
    }
    dc->wndExtX = (dc->wndExtX * xNum) / xDenom;
    dc->wndExtY = (dc->wndExtY * yNum) / yDenom;
    if (dc->wndExtX == 0) dc->wndExtX = 1;
    if (dc->wndExtY == 0) dc->wndExtY = 1;
    if (dc->MapMode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           SetVirtualResolution   (GDI32.@)
 *
 * Undocumented on msdn.
 *
 * Changes the values of screen size in pixels and millimeters used by
 * the mapping mode functions.
 *
 * PARAMS
 *     hdc       [I] Device context
 *     horz_res  [I] Width in pixels  (equivalent to HORZRES device cap).
 *     vert_res  [I] Height in pixels (equivalent to VERTRES device cap).
 *     horz_size [I] Width in mm      (equivalent to HORZSIZE device cap).
 *     vert_size [I] Height in mm     (equivalent to VERTSIZE device cap).
 *
 * RETURNS
 *    TRUE if successful.
 *    FALSE if any (but not all) of the last four params are zero.
 *
 * NOTES
 *    This doesn't change the values returned by GetDeviceCaps, just the
 *    scaling of the mapping modes.
 *
 *    Calling with the last four params equal to zero sets the values
 *    back to their defaults obtained by calls to GetDeviceCaps.
 */
BOOL WINAPI SetVirtualResolution(HDC hdc, DWORD horz_res, DWORD vert_res,
                                 DWORD horz_size, DWORD vert_size)
{
    DC * dc;
    TRACE("(%p %d %d %d %d)\n", hdc, horz_res, vert_res, horz_size, vert_size);

    if(horz_res == 0 && vert_res == 0 && horz_size == 0 && vert_size == 0)
    {
        horz_res  = GetDeviceCaps(hdc, HORZRES);
        vert_res  = GetDeviceCaps(hdc, VERTRES);
        horz_size = GetDeviceCaps(hdc, HORZSIZE);
        vert_size = GetDeviceCaps(hdc, VERTSIZE);
    }
    else if(horz_res == 0 || vert_res == 0 || horz_size == 0 || vert_size == 0)
        return FALSE;

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    dc->virtual_res.cx  = horz_res;
    dc->virtual_res.cy  = vert_res;
    dc->virtual_size.cx = horz_size;
    dc->virtual_size.cy = vert_size;

    release_dc_ptr( dc );
    return TRUE;
}
