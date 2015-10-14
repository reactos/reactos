/*
 * GDI Device Context functions
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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "wine/winternl.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };

static BOOL DC_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs dc_funcs =
{
    NULL,             /* pSelectObject */
    NULL,             /* pGetObjectA */
    NULL,             /* pGetObjectW */
    NULL,             /* pUnrealizeObject */
    DC_DeleteObject   /* pDeleteObject */
};


static inline DC *get_dc_obj( HDC hdc )
{
    DC *dc = GDI_GetObjPtr( hdc, 0 );
    if (!dc) return NULL;

    if ((dc->header.type != OBJ_DC) &&
        (dc->header.type != OBJ_MEMDC) &&
        (dc->header.type != OBJ_METADC) &&
        (dc->header.type != OBJ_ENHMETADC))
    {
        GDI_ReleaseObj( hdc );
        SetLastError( ERROR_INVALID_HANDLE );
        dc = NULL;
    }
    return dc;
}


/***********************************************************************
 *           alloc_dc_ptr
 */
DC *alloc_dc_ptr( const DC_FUNCTIONS *funcs, WORD magic )
{
    DC *dc;

    if (!(dc = HeapAlloc( GetProcessHeap(), 0, sizeof(*dc) ))) return NULL;

    dc->funcs               = funcs;
    dc->physDev             = NULL;
    dc->thread              = GetCurrentThreadId();
    dc->refcount            = 1;
    dc->dirty               = 0;
    dc->saveLevel           = 0;
    dc->saved_dc            = 0;
    dc->dwHookData          = 0;
    dc->hookProc            = NULL;
    dc->wndOrgX             = 0;
    dc->wndOrgY             = 0;
    dc->wndExtX             = 1;
    dc->wndExtY             = 1;
    dc->vportOrgX           = 0;
    dc->vportOrgY           = 0;
    dc->vportExtX           = 1;
    dc->vportExtY           = 1;
    dc->miterLimit          = 10.0f; /* 10.0 is the default, from MSDN */
    dc->flags               = 0;
    dc->layout              = 0;
    dc->hClipRgn            = 0;
    dc->hMetaRgn            = 0;
    dc->hMetaClipRgn        = 0;
    dc->hVisRgn             = 0;
    dc->hPen                = GDI_inc_ref_count( GetStockObject( BLACK_PEN ));
    dc->hBrush              = GDI_inc_ref_count( GetStockObject( WHITE_BRUSH ));
    dc->hFont               = GDI_inc_ref_count( GetStockObject( SYSTEM_FONT ));
    dc->hBitmap             = 0;
    dc->hDevice             = 0;
    dc->hPalette            = GetStockObject( DEFAULT_PALETTE );
    dc->gdiFont             = 0;
    dc->font_code_page      = CP_ACP;
    dc->ROPmode             = R2_COPYPEN;
    dc->polyFillMode        = ALTERNATE;
    dc->stretchBltMode      = BLACKONWHITE;
    dc->relAbsMode          = ABSOLUTE;
    dc->backgroundMode      = OPAQUE;
    dc->backgroundColor     = RGB( 255, 255, 255 );
    dc->dcBrushColor        = RGB( 255, 255, 255 );
    dc->dcPenColor          = RGB( 0, 0, 0 );
    dc->textColor           = RGB( 0, 0, 0 );
    dc->brushOrgX           = 0;
    dc->brushOrgY           = 0;
    dc->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    dc->charExtra           = 0;
    dc->breakExtra          = 0;
    dc->breakRem            = 0;
    dc->MapMode             = MM_TEXT;
    dc->GraphicsMode        = GM_COMPATIBLE;
    dc->pAbortProc          = NULL;
    dc->CursPosX            = 0;
    dc->CursPosY            = 0;
    dc->ArcDirection        = AD_COUNTERCLOCKWISE;
    dc->xformWorld2Wnd.eM11 = 1.0f;
    dc->xformWorld2Wnd.eM12 = 0.0f;
    dc->xformWorld2Wnd.eM21 = 0.0f;
    dc->xformWorld2Wnd.eM22 = 1.0f;
    dc->xformWorld2Wnd.eDx  = 0.0f;
    dc->xformWorld2Wnd.eDy  = 0.0f;
    dc->xformWorld2Vport    = dc->xformWorld2Wnd;
    dc->xformVport2World    = dc->xformWorld2Wnd;
    dc->vport2WorldValid    = TRUE;
    dc->BoundsRect.left     = 0;
    dc->BoundsRect.top      = 0;
    dc->BoundsRect.right    = 0;
    dc->BoundsRect.bottom   = 0;
    PATH_InitGdiPath(&dc->path);

    if (!(dc->hSelf = alloc_gdi_handle( &dc->header, magic, &dc_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, dc );
        dc = NULL;
    }
    return dc;
}



/***********************************************************************
 *           free_dc_ptr
 */
BOOL free_dc_ptr( DC *dc )
{
    assert( dc->refcount == 1 );
    if (free_gdi_handle( dc->hSelf ) != dc) return FALSE;  /* shouldn't happen */
    if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
    if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
    if (dc->hMetaClipRgn) DeleteObject( dc->hMetaClipRgn );
    if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
    PATH_DestroyGdiPath( &dc->path );
    return HeapFree( GetProcessHeap(), 0, dc );
}


/***********************************************************************
 *           get_dc_ptr
 *
 * Retrieve a DC pointer but release the GDI lock.
 */
DC *get_dc_ptr( HDC hdc )
{
    DC *dc = get_dc_obj( hdc );
    if (!dc) return NULL;

    if (!InterlockedCompareExchange( &dc->refcount, 1, 0 ))
    {
        dc->thread = GetCurrentThreadId();
    }
    else if (dc->thread != GetCurrentThreadId())
    {
        WARN( "dc %p belongs to thread %04x\n", hdc, dc->thread );
        GDI_ReleaseObj( hdc );
        return NULL;
    }
    else InterlockedIncrement( &dc->refcount );

    GDI_ReleaseObj( hdc );
    return dc;
}


/***********************************************************************
 *           release_dc_ptr
 */
void release_dc_ptr( DC *dc )
{
    LONG ref;

    dc->thread = 0;
    ref = InterlockedDecrement( &dc->refcount );
    assert( ref >= 0 );
    if (ref) dc->thread = GetCurrentThreadId();  /* we still own it */
}


/***********************************************************************
 *           update_dc
 *
 * Make sure the DC vis region is up to date.
 * This function may call up to USER so the GDI lock should _not_
 * be held when calling it.
 */
void update_dc( DC *dc )
{
    if (InterlockedExchange( &dc->dirty, 0 ) && dc->hookProc)
        dc->hookProc( dc->hSelf, DCHC_INVALIDVISRGN, dc->dwHookData, 0 );
}


/***********************************************************************
 *           DC_DeleteObject
 */
static BOOL DC_DeleteObject( HGDIOBJ handle )
{
    return DeleteDC( handle );
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    if (dc->funcs->pRealizeDefaultPalette) dc->funcs->pRealizeDefaultPalette( dc->physDev );
    SetTextColor( dc->hSelf, dc->textColor );
    SetBkColor( dc->hSelf, dc->backgroundColor );
    SelectObject( dc->hSelf, dc->hPen );
    SelectObject( dc->hSelf, dc->hBrush );
    SelectObject( dc->hSelf, dc->hFont );
    CLIPPING_UpdateGCRegion( dc );
    SetVirtualResolution( dc->hSelf, 0, 0, 0, 0 );
}


/***********************************************************************
 *           DC_InvertXform
 *
 * Computes the inverse of the transformation xformSrc and stores it to
 * xformDest. Returns TRUE if successful or FALSE if the xformSrc matrix
 * is singular.
 */
static BOOL DC_InvertXform( const XFORM *xformSrc, XFORM *xformDest )
{
    double determinant;

    determinant = xformSrc->eM11*xformSrc->eM22 -
        xformSrc->eM12*xformSrc->eM21;
    if (determinant > -1e-12 && determinant < 1e-12)
        return FALSE;

    xformDest->eM11 =  xformSrc->eM22 / determinant;
    xformDest->eM12 = -xformSrc->eM12 / determinant;
    xformDest->eM21 = -xformSrc->eM21 / determinant;
    xformDest->eM22 =  xformSrc->eM11 / determinant;
    xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 -
                       xformSrc->eDy * xformDest->eM21;
    xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 -
                       xformSrc->eDy * xformDest->eM22;

    return TRUE;
}

/* Construct a transformation to do the window-to-viewport conversion */
static void construct_window_to_viewport(DC *dc, XFORM *xform)
{
    double scaleX, scaleY;
    scaleX = (double)dc->vportExtX / (double)dc->wndExtX;
    scaleY = (double)dc->vportExtY / (double)dc->wndExtY;

    if (dc->layout & LAYOUT_RTL) scaleX = -scaleX;
    xform->eM11 = scaleX;
    xform->eM12 = 0.0;
    xform->eM21 = 0.0;
    xform->eM22 = scaleY;
    xform->eDx  = (double)dc->vportOrgX - scaleX * (double)dc->wndOrgX;
    xform->eDy  = (double)dc->vportOrgY - scaleY * (double)dc->wndOrgY;
    if (dc->layout & LAYOUT_RTL) xform->eDx = dc->vis_rect.right - dc->vis_rect.left - 1 - xform->eDx;
}

/***********************************************************************
 *           DC_UpdateXforms
 *
 * Updates the xformWorld2Vport, xformVport2World and vport2WorldValid
 * fields of the specified DC by creating a transformation that
 * represents the current mapping mode and combining it with the DC's
 * world transform. This function should be called whenever the
 * parameters associated with the mapping mode (window and viewport
 * extents and origins) or the world transform change.
 */
void DC_UpdateXforms( DC *dc )
{
    XFORM xformWnd2Vport, oldworld2vport;

    construct_window_to_viewport(dc, &xformWnd2Vport);

    oldworld2vport = dc->xformWorld2Vport;
    /* Combine with the world transformation */
    CombineTransform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );

    /* Reselect the font and pen back into the dc so that the size
       gets updated. */
    if (memcmp(&oldworld2vport, &dc->xformWorld2Vport, sizeof(oldworld2vport)) &&
        !GdiIsMetaFileDC(dc->hSelf))
    {
        SelectObject(dc->hSelf, GetCurrentObject(dc->hSelf, OBJ_FONT));
        SelectObject(dc->hSelf, GetCurrentObject(dc->hSelf, OBJ_PEN));
    }
}


/***********************************************************************
 *           save_dc_state
 */
INT save_dc_state( HDC hdc )
{
    DC * newdc, * dc;
    INT ret;

    if (!(dc = get_dc_ptr( hdc ))) return 0;
    if (!(newdc = HeapAlloc( GetProcessHeap(), 0, sizeof(*newdc ))))
    {
      release_dc_ptr( dc );
      return 0;
    }

    newdc->flags            = dc->flags | DC_SAVED;
    newdc->layout           = dc->layout;
    newdc->hPen             = dc->hPen;
    newdc->hBrush           = dc->hBrush;
    newdc->hFont            = dc->hFont;
    newdc->hBitmap          = dc->hBitmap;
    newdc->hDevice          = dc->hDevice;
    newdc->hPalette         = dc->hPalette;
    newdc->ROPmode          = dc->ROPmode;
    newdc->polyFillMode     = dc->polyFillMode;
    newdc->stretchBltMode   = dc->stretchBltMode;
    newdc->relAbsMode       = dc->relAbsMode;
    newdc->backgroundMode   = dc->backgroundMode;
    newdc->backgroundColor  = dc->backgroundColor;
    newdc->textColor        = dc->textColor;
    newdc->dcBrushColor     = dc->dcBrushColor;
    newdc->dcPenColor       = dc->dcPenColor;
    newdc->brushOrgX        = dc->brushOrgX;
    newdc->brushOrgY        = dc->brushOrgY;
    newdc->textAlign        = dc->textAlign;
    newdc->charExtra        = dc->charExtra;
    newdc->breakExtra       = dc->breakExtra;
    newdc->breakRem         = dc->breakRem;
    newdc->MapMode          = dc->MapMode;
    newdc->GraphicsMode     = dc->GraphicsMode;
    newdc->CursPosX         = dc->CursPosX;
    newdc->CursPosY         = dc->CursPosY;
    newdc->ArcDirection     = dc->ArcDirection;
    newdc->xformWorld2Wnd   = dc->xformWorld2Wnd;
    newdc->xformWorld2Vport = dc->xformWorld2Vport;
    newdc->xformVport2World = dc->xformVport2World;
    newdc->vport2WorldValid = dc->vport2WorldValid;
    newdc->wndOrgX          = dc->wndOrgX;
    newdc->wndOrgY          = dc->wndOrgY;
    newdc->wndExtX          = dc->wndExtX;
    newdc->wndExtY          = dc->wndExtY;
    newdc->vportOrgX        = dc->vportOrgX;
    newdc->vportOrgY        = dc->vportOrgY;
    newdc->vportExtX        = dc->vportExtX;
    newdc->vportExtY        = dc->vportExtY;
    newdc->virtual_res      = dc->virtual_res;
    newdc->virtual_size     = dc->virtual_size;
    newdc->BoundsRect       = dc->BoundsRect;
    newdc->gdiFont          = dc->gdiFont;

    newdc->thread    = GetCurrentThreadId();
    newdc->refcount  = 1;
    newdc->saveLevel = 0;
    newdc->saved_dc  = 0;

    PATH_InitGdiPath( &newdc->path );

    newdc->pAbortProc = NULL;
    newdc->hookProc   = NULL;

    if (!(newdc->hSelf = alloc_gdi_handle( &newdc->header, dc->header.type, &dc_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, newdc );
        release_dc_ptr( dc );
        return 0;
    }

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    newdc->hVisRgn      = 0;
    newdc->hClipRgn     = 0;
    newdc->hMetaRgn     = 0;
    newdc->hMetaClipRgn = 0;
    if (dc->hClipRgn)
    {
        newdc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( newdc->hClipRgn, dc->hClipRgn, 0, RGN_COPY );
    }
    if (dc->hMetaRgn)
    {
        newdc->hMetaRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( newdc->hMetaRgn, dc->hMetaRgn, 0, RGN_COPY );
    }
    /* don't bother recomputing hMetaClipRgn, we'll do that in SetDCState */

    if (!PATH_AssignGdiPath( &newdc->path, &dc->path ))
    {
        release_dc_ptr( dc );
        free_dc_ptr( newdc );
	return 0;
    }

    newdc->saved_dc = dc->saved_dc;
    dc->saved_dc = newdc->hSelf;
    ret = ++dc->saveLevel;
    release_dc_ptr( newdc );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           restore_dc_state
 */
BOOL restore_dc_state( HDC hdc, INT level )
{
    HDC hdcs, first_dcs;
    DC *dc, *dcs;
    INT save_level;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    /* find the state level to restore */

    if (abs(level) > dc->saveLevel || level == 0)
    {
        release_dc_ptr( dc );
        return FALSE;
    }
    if (level < 0) level = dc->saveLevel + level + 1;
    first_dcs = dc->saved_dc;
    for (hdcs = first_dcs, save_level = dc->saveLevel; save_level > level; save_level--)
    {
	if (!(dcs = get_dc_ptr( hdcs )))
	{
            release_dc_ptr( dc );
            return FALSE;
	}
        hdcs = dcs->saved_dc;
        release_dc_ptr( dcs );
    }

    /* restore the state */

    if (!(dcs = get_dc_ptr( hdcs )))
    {
        release_dc_ptr( dc );
        return FALSE;
    }
    if (!PATH_AssignGdiPath( &dc->path, &dcs->path ))
    {
        release_dc_ptr( dcs );
        release_dc_ptr( dc );
        return FALSE;
    }

    dc->flags            = dcs->flags & ~DC_SAVED;
    dc->layout           = dcs->layout;
    dc->hDevice          = dcs->hDevice;
    dc->ROPmode          = dcs->ROPmode;
    dc->polyFillMode     = dcs->polyFillMode;
    dc->stretchBltMode   = dcs->stretchBltMode;
    dc->relAbsMode       = dcs->relAbsMode;
    dc->backgroundMode   = dcs->backgroundMode;
    dc->backgroundColor  = dcs->backgroundColor;
    dc->textColor        = dcs->textColor;
    dc->dcBrushColor     = dcs->dcBrushColor;
    dc->dcPenColor       = dcs->dcPenColor;
    dc->brushOrgX        = dcs->brushOrgX;
    dc->brushOrgY        = dcs->brushOrgY;
    dc->textAlign        = dcs->textAlign;
    dc->charExtra        = dcs->charExtra;
    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->MapMode          = dcs->MapMode;
    dc->GraphicsMode     = dcs->GraphicsMode;
    dc->CursPosX         = dcs->CursPosX;
    dc->CursPosY         = dcs->CursPosY;
    dc->ArcDirection     = dcs->ArcDirection;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;
    dc->BoundsRect       = dcs->BoundsRect;

    dc->wndOrgX          = dcs->wndOrgX;
    dc->wndOrgY          = dcs->wndOrgY;
    dc->wndExtX          = dcs->wndExtX;
    dc->wndExtY          = dcs->wndExtY;
    dc->vportOrgX        = dcs->vportOrgX;
    dc->vportOrgY        = dcs->vportOrgY;
    dc->vportExtX        = dcs->vportExtX;
    dc->vportExtY        = dcs->vportExtY;
    dc->virtual_res      = dcs->virtual_res;
    dc->virtual_size     = dcs->virtual_size;

    if (dcs->hClipRgn)
    {
        if (!dc->hClipRgn) dc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hClipRgn, dcs->hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
        dc->hClipRgn = 0;
    }
    if (dcs->hMetaRgn)
    {
        if (!dc->hMetaRgn) dc->hMetaRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hMetaRgn, dcs->hMetaRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
        dc->hMetaRgn = 0;
    }
    DC_UpdateXforms( dc );
    CLIPPING_UpdateGCRegion( dc );

    SelectObject( hdc, dcs->hBitmap );
    SelectObject( hdc, dcs->hBrush );
    SelectObject( hdc, dcs->hFont );
    SelectObject( hdc, dcs->hPen );
    SetBkColor( hdc, dcs->backgroundColor);
    SetTextColor( hdc, dcs->textColor);
    GDISelectPalette( hdc, dcs->hPalette, FALSE );

    dc->saved_dc  = dcs->saved_dc;
    dcs->saved_dc = 0;
    dc->saveLevel = save_level - 1;

    release_dc_ptr( dcs );

    /* now destroy all the saved DCs */

    while (first_dcs)
    {
	if (!(dcs = get_dc_ptr( first_dcs ))) break;
        hdcs = dcs->saved_dc;
        free_dc_ptr( dcs );
        first_dcs = hdcs;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           SaveDC    (GDI32.@)
 */
INT WINAPI SaveDC( HDC hdc )
{
    DC * dc;
    INT ret;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if(dc->funcs->pSaveDC)
        ret = dc->funcs->pSaveDC( dc->physDev );
    else
        ret = save_dc_state( hdc );

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           RestoreDC    (GDI32.@)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    DC *dc;
    BOOL success;

    TRACE("%p %d\n", hdc, level );
    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    update_dc( dc );

    if(dc->funcs->pRestoreDC)
        success = dc->funcs->pRestoreDC( dc->physDev, level );
    else
        success = restore_dc_state( hdc, level );

    release_dc_ptr( dc );
    return success;
}


/***********************************************************************
 *           CreateDCW    (GDI32.@)
 */
HDC WINAPI CreateDCW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                      const DEVMODEW *initData )
{
    HDC hdc;
    DC * dc;
    const DC_FUNCTIONS *funcs;
    WCHAR buf[300];

    GDI_CheckNotLock();

    if (!device || !DRIVER_GetDriverName( device, buf, 300 ))
    {
        if (!driver)
        {
            ERR( "no device found for %s\n", debugstr_w(device) );
            return 0;
        }
        strcpyW(buf, driver);
    }

    if (!(funcs = DRIVER_load_driver( buf )))
    {
        ERR( "no driver found for %s\n", debugstr_w(buf) );
        return 0;
    }
    if (!(dc = alloc_dc_ptr( funcs, OBJ_DC ))) goto error;
    hdc = dc->hSelf;

    dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));
    if (!(dc->hVisRgn = CreateRectRgn( 0, 0, 1, 1 ))) goto error;

    TRACE("(driver=%s, device=%s, output=%s): returning %p\n",
          debugstr_w(driver), debugstr_w(device), debugstr_w(output), dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( hdc, &dc->physDev, buf, device, output, initData ))
    {
        WARN("creation aborted by device\n" );
        goto error;
    }

    dc->vis_rect.left   = 0;
    dc->vis_rect.top    = 0;
    dc->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
    dc->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );
    SetRectRgn(dc->hVisRgn, dc->vis_rect.left, dc->vis_rect.top, dc->vis_rect.right, dc->vis_rect.bottom);

    DC_InitDC( dc );
    release_dc_ptr( dc );
    return hdc;

error:
    if (dc) free_dc_ptr( dc );
    return 0;
}


/***********************************************************************
 *           CreateDCA    (GDI32.@)
 */
HDC WINAPI CreateDCA( LPCSTR driver, LPCSTR device, LPCSTR output,
                      const DEVMODEA *initData )
{
    UNICODE_STRING driverW, deviceW, outputW;
    DEVMODEW *initDataW;
    HDC ret;

    if (driver) RtlCreateUnicodeStringFromAsciiz(&driverW, driver);
    else driverW.Buffer = NULL;

    if (device) RtlCreateUnicodeStringFromAsciiz(&deviceW, device);
    else deviceW.Buffer = NULL;

    if (output) RtlCreateUnicodeStringFromAsciiz(&outputW, output);
    else outputW.Buffer = NULL;

    initDataW = NULL;
    if (initData)
    {
        /* don't convert initData for DISPLAY driver, it's not used */
        if (!driverW.Buffer || strcmpiW( driverW.Buffer, displayW ))
            initDataW = GdiConvertToDevmodeW(initData);
    }

    ret = CreateDCW( driverW.Buffer, deviceW.Buffer, outputW.Buffer, initDataW );

    RtlFreeUnicodeString(&driverW);
    RtlFreeUnicodeString(&deviceW);
    RtlFreeUnicodeString(&outputW);
    HeapFree(GetProcessHeap(), 0, initDataW);
    return ret;
}


/***********************************************************************
 *           CreateICA    (GDI32.@)
 */
HDC WINAPI CreateICA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, initData );
}


/***********************************************************************
 *           CreateICW    (GDI32.@)
 */
HDC WINAPI CreateICW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                          const DEVMODEW* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCW( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC   (GDI32.@)
 */
HDC WINAPI CreateCompatibleDC( HDC hdc )
{
    DC *dc, *origDC;
    HDC ret;
    const DC_FUNCTIONS *funcs = NULL;
    PHYSDEV physDev = NULL;

    GDI_CheckNotLock();

    if (hdc)
    {
        if (!(origDC = get_dc_ptr( hdc ))) return 0;
        if (GetObjectType( hdc ) == OBJ_DC)
        {
            funcs = origDC->funcs;
            physDev = origDC->physDev;
        }
        release_dc_ptr( origDC );
    }

    if (!funcs && !(funcs = DRIVER_get_display_driver())) return 0;

    if (!(dc = alloc_dc_ptr( funcs, OBJ_MEMDC ))) goto error;

    TRACE("(%p): returning %p\n", hdc, dc->hSelf );

    dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));
    dc->vis_rect.left   = 0;
    dc->vis_rect.top    = 0;
    dc->vis_rect.right  = 1;
    dc->vis_rect.bottom = 1;
    if (!(dc->hVisRgn = CreateRectRgn( 0, 0, 1, 1 ))) goto error;   /* default bitmap is 1x1 */

    /* Copy the driver-specific physical device info into
     * the new DC. The driver may use this read-only info
     * while creating the compatible DC below. */
    dc->physDev = physDev;
    ret = dc->hSelf;

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc->hSelf, &dc->physDev, NULL, NULL, NULL, NULL ))
    {
        WARN("creation aborted by device\n");
        goto error;
    }

    DC_InitDC( dc );
    release_dc_ptr( dc );
    return ret;

error:
    if (dc) free_dc_ptr( dc );
    return 0;
}


/***********************************************************************
 *           DeleteDC    (GDI32.@)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    DC * dc;

    TRACE("%p\n", hdc );

    GDI_CheckNotLock();

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (dc->refcount != 1)
    {
        FIXME( "not deleting busy DC %p refcount %u\n", dc->hSelf, dc->refcount );
        release_dc_ptr( dc );
        return FALSE;
    }

    /* Call hook procedure to check whether is it OK to delete this DC */
    if (dc->hookProc && !dc->hookProc( hdc, DCHC_DELETEDC, dc->dwHookData, 0 ))
    {
        release_dc_ptr( dc );
        return TRUE;
    }

    while (dc->saveLevel)
    {
        DC * dcs;
        HDC hdcs = dc->saved_dc;
        if (!(dcs = get_dc_ptr( hdcs ))) break;
        dc->saved_dc = dcs->saved_dc;
        dc->saveLevel--;
        free_dc_ptr( dcs );
    }

    if (!(dc->flags & DC_SAVED))
    {
	SelectObject( hdc, GetStockObject(BLACK_PEN) );
	SelectObject( hdc, GetStockObject(WHITE_BRUSH) );
	SelectObject( hdc, GetStockObject(SYSTEM_FONT) );
        SelectObject( hdc, GetStockObject(DEFAULT_BITMAP) );
        if (dc->funcs->pDeleteDC) dc->funcs->pDeleteDC(dc->physDev);
        dc->physDev = NULL;
    }

    free_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           ResetDCW    (GDI32.@)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    DC *dc;
    HDC ret = hdc;

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pResetDC)
        {
            ret = dc->funcs->pResetDC( dc->physDev, devmode );
            if (ret)  /* reset the visible region */
            {
                dc->dirty = 0;
                dc->vis_rect.left   = 0;
                dc->vis_rect.top    = 0;
                dc->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
                dc->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );
                SetRectRgn( dc->hVisRgn, dc->vis_rect.left, dc->vis_rect.top,
                            dc->vis_rect.right, dc->vis_rect.bottom );
                CLIPPING_UpdateGCRegion( dc );
            }
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           ResetDCA    (GDI32.@)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    DEVMODEW *devmodeW;
    HDC ret;

    if (devmode) devmodeW = GdiConvertToDevmodeW(devmode);
    else devmodeW = NULL;

    ret = ResetDCW(hdc, devmodeW);

    HeapFree(GetProcessHeap(), 0, devmodeW);
    return ret;
}


/***********************************************************************
 *           GetDeviceCaps    (GDI32.@)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
{
    DC *dc;
    INT ret = 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pGetDeviceCaps) ret = dc->funcs->pGetDeviceCaps( dc->physDev, cap );
        else switch(cap)  /* return meaningful values for some entries */
        {
        case HORZRES:     ret = 640; break;
        case VERTRES:     ret = 480; break;
        case BITSPIXEL:   ret = 1; break;
        case PLANES:      ret = 1; break;
        case NUMCOLORS:   ret = 2; break;
        case ASPECTX:     ret = 36; break;
        case ASPECTY:     ret = 36; break;
        case ASPECTXY:    ret = 51; break;
        case LOGPIXELSX:  ret = 72; break;
        case LOGPIXELSY:  ret = 72; break;
        case SIZEPALETTE: ret = 2; break;
        case TEXTCAPS:
            ret = (TC_OP_CHARACTER | TC_OP_STROKE | TC_CP_STROKE |
                   TC_CR_ANY | TC_SF_X_YINDEP | TC_SA_DOUBLE | TC_SA_INTEGER |
                   TC_SA_CONTIN | TC_UA_ABLE | TC_SO_ABLE | TC_RA_ABLE | TC_VA_ABLE);
            break;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetBkColor (GDI32.@)
 */
COLORREF WINAPI GetBkColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->backgroundColor;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetBkColor    (GDI32.@)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = get_dc_ptr( hdc );

    TRACE("hdc=%p color=0x%08x\n", hdc, color);

    if (!dc) return CLR_INVALID;
    oldColor = dc->backgroundColor;
    if (dc->funcs->pSetBkColor)
    {
        color = dc->funcs->pSetBkColor(dc->physDev, color);
        if (color == CLR_INVALID)  /* don't change it */
        {
            color = oldColor;
            oldColor = CLR_INVALID;
        }
    }
    dc->backgroundColor = color;
    release_dc_ptr( dc );
    return oldColor;
}


/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
COLORREF WINAPI GetTextColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->textColor;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextColor    (GDI32.@)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = get_dc_ptr( hdc );

    TRACE(" hdc=%p color=0x%08x\n", hdc, color);

    if (!dc) return CLR_INVALID;
    oldColor = dc->textColor;
    if (dc->funcs->pSetTextColor)
    {
        color = dc->funcs->pSetTextColor(dc->physDev, color);
        if (color == CLR_INVALID)  /* don't change it */
        {
            color = oldColor;
            oldColor = CLR_INVALID;
        }
    }
    dc->textColor = color;
    release_dc_ptr( dc );
    return oldColor;
}


/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
UINT WINAPI GetTextAlign( HDC hdc )
{
    UINT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->textAlign;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextAlign    (GDI32.@)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    UINT ret;
    DC *dc = get_dc_ptr( hdc );

    TRACE("hdc=%p align=%d\n", hdc, align);

    if (!dc) return 0x0;
    ret = dc->textAlign;
    if (dc->funcs->pSetTextAlign)
        if (!dc->funcs->pSetTextAlign(dc->physDev, align))
            ret = GDI_ERROR;
    if (ret != GDI_ERROR)
	dc->textAlign = align;
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.@)
 */
BOOL WINAPI GetDCOrgEx( HDC hDC, LPPOINT lpp )
{
    DC * dc;

    if (!lpp) return FALSE;
    if (!(dc = get_dc_ptr( hDC ))) return FALSE;
    lpp->x = dc->vis_rect.left;
    lpp->y = dc->vis_rect.top;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->GraphicsMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetGraphicsMode    (GDI32.@)
 */
INT WINAPI SetGraphicsMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    if (!dc) return 0;
    if ((mode > 0) && (mode <= GM_LAST))
    {
        ret = dc->GraphicsMode;
        dc->GraphicsMode = mode;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->ArcDirection;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetArcDirection    (GDI32.@)
 */
INT WINAPI SetArcDirection( HDC hdc, INT nDirection )
{
    DC * dc;
    INT nOldDirection = 0;

    if (nDirection!=AD_COUNTERCLOCKWISE && nDirection!=AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pSetArcDirection)
        {
            dc->funcs->pSetArcDirection(dc->physDev, nDirection);
        }
        nOldDirection = dc->ArcDirection;
        dc->ArcDirection = nDirection;
        release_dc_ptr( dc );
    }
    return nOldDirection;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.@)
 */
BOOL WINAPI GetWorldTransform( HDC hdc, LPXFORM xform )
{
    DC * dc;
    if (!xform) return FALSE;
    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    *xform = dc->xformWorld2Wnd;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetTransform    (GDI32.@)
 *
 * Undocumented
 *
 * Returns one of the co-ordinate space transforms
 *
 * PARAMS
 *    hdc   [I] Device context.
 *    which [I] Which xform to return:
 *                  0x203 World -> Page transform (that set by SetWorldTransform).
 *                  0x304 Page -> Device transform (the mapping mode transform).
 *                  0x204 World -> Device transform (the combination of the above two).
 *                  0x402 Device -> World transform (the inversion of the above).
 *    xform [O] The xform.
 *
 */
BOOL WINAPI GetTransform( HDC hdc, DWORD which, XFORM *xform )
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    switch(which)
    {
    case 0x203:
        *xform = dc->xformWorld2Wnd;
        break;

    case 0x304:
        construct_window_to_viewport(dc, xform);
        break;

    case 0x204:
        *xform = dc->xformWorld2Vport;
        break;

    case 0x402:
        *xform = dc->xformVport2World;
        break;

    default:
        FIXME("Unknown code %x\n", which);
        ret = FALSE;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.@)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    BOOL ret = FALSE;
    DC *dc;

    if (!xform) return FALSE;

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

    TRACE("eM11 %f eM12 %f eM21 %f eM22 %f eDx %f eDy %f\n",
        xform->eM11, xform->eM12, xform->eM21, xform->eM22, xform->eDx, xform->eDy);

    /* The transform must conform to (eM11 * eM22 != eM12 * eM21) requirement */
    if (xform->eM11 * xform->eM22 == xform->eM12 * xform->eM21) goto done;

    if (dc->funcs->pSetWorldTransform)
    {
        ret = dc->funcs->pSetWorldTransform(dc->physDev, xform);
        if (!ret) goto done;
    }

    dc->xformWorld2Wnd = *xform;
    DC_UpdateXforms( dc );
    ret = TRUE;
 done:
    release_dc_ptr( dc );
    return ret;
}


/****************************************************************************
 * ModifyWorldTransform [GDI32.@]
 * Modifies the world transformation for a device context.
 *
 * PARAMS
 *    hdc   [I] Handle to device context
 *    xform [I] XFORM structure that will be used to modify the world
 *              transformation
 *    iMode [I] Specifies in what way to modify the world transformation
 *              Possible values:
 *              MWT_IDENTITY
 *                 Resets the world transformation to the identity matrix.
 *                 The parameter xform is ignored.
 *              MWT_LEFTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the left.
 *              MWT_RIGHTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the right.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform,
    DWORD iMode )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    /* Check for illegal parameters */
    if (!dc) return FALSE;
    if (!xform && iMode != MWT_IDENTITY) goto done;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

    if (dc->funcs->pModifyWorldTransform)
    {
        ret = dc->funcs->pModifyWorldTransform(dc->physDev, xform, iMode);
        if (!ret) goto done;
    }

    switch (iMode)
    {
        case MWT_IDENTITY:
	    dc->xformWorld2Wnd.eM11 = 1.0f;
	    dc->xformWorld2Wnd.eM12 = 0.0f;
	    dc->xformWorld2Wnd.eM21 = 0.0f;
	    dc->xformWorld2Wnd.eM22 = 1.0f;
	    dc->xformWorld2Wnd.eDx  = 0.0f;
	    dc->xformWorld2Wnd.eDy  = 0.0f;
	    break;
        case MWT_LEFTMULTIPLY:
	    CombineTransform( &dc->xformWorld2Wnd, xform,
	        &dc->xformWorld2Wnd );
	    break;
	case MWT_RIGHTMULTIPLY:
	    CombineTransform( &dc->xformWorld2Wnd, &dc->xformWorld2Wnd,
	        xform );
	    break;
        default:
            goto done;
    }

    DC_UpdateXforms( dc );
    ret = TRUE;
 done:
    release_dc_ptr( dc );
    return ret;
}


/****************************************************************************
 * CombineTransform [GDI32.@]
 * Combines two transformation matrices.
 *
 * PARAMS
 *    xformResult [O] Stores the result of combining the two matrices
 *    xform1      [I] Specifies the first matrix to apply
 *    xform2      [I] Specifies the second matrix to apply
 *
 * REMARKS
 *    The same matrix can be passed in for more than one of the parameters.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI CombineTransform( LPXFORM xformResult, const XFORM *xform1,
    const XFORM *xform2 )
{
    XFORM xformTemp;

    /* Check for illegal parameters */
    if (!xformResult || !xform1 || !xform2)
        return FALSE;

    /* Create the result in a temporary XFORM, since xformResult may be
     * equal to xform1 or xform2 */
    xformTemp.eM11 = xform1->eM11 * xform2->eM11 +
                     xform1->eM12 * xform2->eM21;
    xformTemp.eM12 = xform1->eM11 * xform2->eM12 +
                     xform1->eM12 * xform2->eM22;
    xformTemp.eM21 = xform1->eM21 * xform2->eM11 +
                     xform1->eM22 * xform2->eM21;
    xformTemp.eM22 = xform1->eM21 * xform2->eM12 +
                     xform1->eM22 * xform2->eM22;
    xformTemp.eDx  = xform1->eDx  * xform2->eM11 +
                     xform1->eDy  * xform2->eM21 +
                     xform2->eDx;
    xformTemp.eDy  = xform1->eDx  * xform2->eM12 +
                     xform1->eDy  * xform2->eM22 +
                     xform2->eDy;

    /* Copy the result to xformResult */
    *xformResult = xformTemp;

    return TRUE;
}


/***********************************************************************
 *           SetDCHook   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
BOOL WINAPI SetDCHook( HDC hdc, DCHOOKPROC hookProc, DWORD_PTR dwHookData )
{
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    if (!(dc->flags & DC_SAVED))
    {
        dc->dwHookData = dwHookData;
        dc->hookProc = hookProc;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
DWORD_PTR WINAPI GetDCHook( HDC hdc, DCHOOKPROC *proc )
{
    DC *dc = get_dc_ptr( hdc );
    DWORD_PTR ret;

    if (!dc) return 0;
    if (proc) *proc = dc->hookProc;
    ret = dc->dwHookData;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetHookFlags   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
WORD WINAPI SetHookFlags( HDC hdc, WORD flags )
{
    DC *dc = get_dc_obj( hdc );  /* not get_dc_ptr, this needs to work from any thread */
    LONG ret = 0;

    if (!dc) return 0;

    /* "Undocumented Windows" info is slightly confusing. */

    TRACE("hDC %p, flags %04x\n",hdc,flags);

    if (flags & DCHF_INVALIDATEVISRGN)
        ret = InterlockedExchange( &dc->dirty, 1 );
    else if (flags & DCHF_VALIDATEVISRGN || !flags)
        ret = InterlockedExchange( &dc->dirty, 0 );

    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *           SetICMMode    (GDI32.@)
 */
INT WINAPI SetICMMode(HDC hdc, INT iEnableICM)
{
/*FIXME:  Assume that ICM is always off, and cannot be turned on */
    if (iEnableICM == ICM_OFF) return ICM_OFF;
    if (iEnableICM == ICM_ON) return 0;
    if (iEnableICM == ICM_QUERY) return ICM_OFF;
    return 0;
}

/***********************************************************************
 *           GetDeviceGammaRamp    (GDI32.@)
 */
BOOL WINAPI GetDeviceGammaRamp(HDC hDC, LPVOID ptr)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hDC );

    if( dc )
    {
	if (dc->funcs->pGetDeviceGammaRamp)
	    ret = dc->funcs->pGetDeviceGammaRamp(dc->physDev, ptr);
	release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           SetDeviceGammaRamp    (GDI32.@)
 */
BOOL WINAPI SetDeviceGammaRamp(HDC hDC, LPVOID ptr)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hDC );

    if( dc )
    {
	if (dc->funcs->pSetDeviceGammaRamp)
	    ret = dc->funcs->pSetDeviceGammaRamp(dc->physDev, ptr);
	release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           GetColorSpace    (GDI32.@)
 */
HCOLORSPACE WINAPI GetColorSpace(HDC hdc)
{
/*FIXME    Need to to whatever GetColorSpace actually does */
    return 0;
}

/***********************************************************************
 *           CreateColorSpaceA    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceA( LPLOGCOLORSPACEA lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0;
}

/***********************************************************************
 *           CreateColorSpaceW    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceW( LPLOGCOLORSPACEW lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0;
}

/***********************************************************************
 *           DeleteColorSpace     (GDI32.@)
 */
BOOL WINAPI DeleteColorSpace( HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );

  return TRUE;
}

/***********************************************************************
 *           SetColorSpace     (GDI32.@)
 */
HCOLORSPACE WINAPI SetColorSpace( HDC hDC, HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );

  return hColorSpace;
}

/***********************************************************************
 *           GetBoundsRect    (GDI32.@)
 */
UINT WINAPI GetBoundsRect(HDC hdc, LPRECT rect, UINT flags)
{
    UINT ret;
    DC *dc = get_dc_ptr( hdc );

    if ( !dc ) return 0;

    if (rect)
    {
        *rect = dc->BoundsRect;
        ret = ((dc->flags & DC_BOUNDS_SET) ? DCB_SET : DCB_RESET);
    }
    else
        ret = 0;

    if (flags & DCB_RESET)
    {
        dc->BoundsRect.left   = 0;
        dc->BoundsRect.top    = 0;
        dc->BoundsRect.right  = 0;
        dc->BoundsRect.bottom = 0;
        dc->flags &= ~DC_BOUNDS_SET;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetBoundsRect    (GDI32.@)
 */
UINT WINAPI SetBoundsRect(HDC hdc, const RECT* rect, UINT flags)
{
    UINT ret;
    DC *dc;

    if ((flags & DCB_ENABLE) && (flags & DCB_DISABLE)) return 0;
    if (!(dc = get_dc_ptr( hdc ))) return 0;

    ret = ((dc->flags & DC_BOUNDS_ENABLE) ? DCB_ENABLE : DCB_DISABLE) |
          ((dc->flags & DC_BOUNDS_SET) ? DCB_SET : DCB_RESET);

    if (flags & DCB_RESET)
    {
        dc->BoundsRect.left   = 0;
        dc->BoundsRect.top    = 0;
        dc->BoundsRect.right  = 0;
        dc->BoundsRect.bottom = 0;
        dc->flags &= ~DC_BOUNDS_SET;
    }

    if ((flags & DCB_ACCUMULATE) && rect && rect->left < rect->right && rect->top < rect->bottom)
    {
        if (dc->flags & DC_BOUNDS_SET)
        {
            dc->BoundsRect.left   = min( dc->BoundsRect.left, rect->left );
            dc->BoundsRect.top    = min( dc->BoundsRect.top, rect->top );
            dc->BoundsRect.right  = max( dc->BoundsRect.right, rect->right );
            dc->BoundsRect.bottom = max( dc->BoundsRect.bottom, rect->bottom );
        }
        else
        {
            dc->BoundsRect = *rect;
            dc->flags |= DC_BOUNDS_SET;
        }
    }

    if (flags & DCB_ENABLE) dc->flags |= DC_BOUNDS_ENABLE;
    if (flags & DCB_DISABLE) dc->flags &= ~DC_BOUNDS_ENABLE;

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetRelAbs		(GDI32.@)
 */
INT WINAPI GetRelAbs( HDC hdc, DWORD dwIgnore )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->relAbsMode;
        release_dc_ptr( dc );
    }
    return ret;
}




/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
INT WINAPI GetBkMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->backgroundMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
INT WINAPI SetBkMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > BKMODE_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;

    ret = dc->backgroundMode;
    if (dc->funcs->pSetBkMode)
        if (!dc->funcs->pSetBkMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->backgroundMode = mode;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
INT WINAPI GetROP2( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->ROPmode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
INT WINAPI SetROP2( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode < R2_BLACK) || (mode > R2_WHITE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    ret = dc->ROPmode;
    if (dc->funcs->pSetROP2)
        if (!dc->funcs->pSetROP2( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->ROPmode = mode;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
INT WINAPI SetRelAbs( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode != ABSOLUTE) && (mode != RELATIVE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    if (dc->funcs->pSetRelAbs)
        ret = dc->funcs->pSetRelAbs( dc->physDev, mode );
    else
    {
        ret = dc->relAbsMode;
        dc->relAbsMode = mode;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetPolyFillMode (GDI32.@)
 */
INT WINAPI GetPolyFillMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->polyFillMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
INT WINAPI SetPolyFillMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > POLYFILL_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    ret = dc->polyFillMode;
    if (dc->funcs->pSetPolyFillMode)
        if (!dc->funcs->pSetPolyFillMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->polyFillMode = mode;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
INT WINAPI GetStretchBltMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->stretchBltMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
INT WINAPI SetStretchBltMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > MAXSTRETCHBLTMODE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    ret = dc->stretchBltMode;
    if (dc->funcs->pSetStretchBltMode)
        if (!dc->funcs->pSetStretchBltMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->stretchBltMode = mode;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetMapMode (GDI32.@)
 */
INT WINAPI GetMapMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->MapMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetBrushOrgEx (GDI32.@)
 */
BOOL WINAPI GetBrushOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->brushOrgX;
    pt->y = dc->brushOrgY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
BOOL WINAPI GetCurrentPositionEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->CursPosX;
    pt->y = dc->CursPosY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportExtEx (GDI32.@)
 */
BOOL WINAPI GetViewportExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    size->cx = dc->vportExtX;
    size->cy = dc->vportExtY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportOrgEx (GDI32.@)
 */
BOOL WINAPI GetViewportOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->vportOrgX;
    pt->y = dc->vportOrgY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowExtEx (GDI32.@)
 */
BOOL WINAPI GetWindowExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    size->cx = dc->wndExtX;
    size->cy = dc->wndExtY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowOrgEx (GDI32.@)
 */
BOOL WINAPI GetWindowOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->wndOrgX;
    pt->y = dc->wndOrgY;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetLayout    (GDI32.@)
 *
 * Gets left->right or right->left text layout flags of a dc.
 *
 */
DWORD WINAPI GetLayout(HDC hdc)
{
    DWORD layout = GDI_ERROR;

    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        layout = dc->layout;
        release_dc_ptr( dc );
    }

    TRACE("hdc : %p, layout : %08x\n", hdc, layout);

    return layout;
}

/***********************************************************************
 *           SetLayout    (GDI32.@)
 *
 * Sets left->right or right->left text layout flags of a dc.
 *
 */
DWORD WINAPI SetLayout(HDC hdc, DWORD layout)
{
    DWORD oldlayout = GDI_ERROR;

    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        oldlayout = dc->layout;
        dc->layout = layout;
        if (layout != oldlayout)
        {
            if (layout & LAYOUT_RTL) dc->MapMode = MM_ANISOTROPIC;
            DC_UpdateXforms( dc );
        }
        release_dc_ptr( dc );
    }

    TRACE("hdc : %p, old layout : %08x, new layout : %08x\n", hdc, oldlayout, layout);

    return oldlayout;
}

/***********************************************************************
 *           GetDCBrushColor    (GDI32.@)
 *
 * Retrieves the current brush color for the specified device
 * context (DC).
 *
 */
COLORREF WINAPI GetDCBrushColor(HDC hdc)
{
    DC *dc;
    COLORREF dcBrushColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        dcBrushColor = dc->dcBrushColor;
	release_dc_ptr( dc );
    }

    return dcBrushColor;
}

/***********************************************************************
 *           SetDCBrushColor    (GDI32.@)
 *
 * Sets the current device context (DC) brush color to the specified
 * color value. If the device cannot represent the specified color
 * value, the color is set to the nearest physical color.
 *
 */
COLORREF WINAPI SetDCBrushColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (dc->funcs->pSetDCBrushColor)
            crColor = dc->funcs->pSetDCBrushColor( dc->physDev, crColor );
        else if (dc->hBrush == GetStockObject( DC_BRUSH ))
        {
            /* If DC_BRUSH is selected, update driver pen color */
            HBRUSH hBrush = CreateSolidBrush( crColor );
            dc->funcs->pSelectBrush( dc->physDev, hBrush );
	    DeleteObject( hBrush );
	}

        if (crColor != CLR_INVALID)
        {
            oldClr = dc->dcBrushColor;
            dc->dcBrushColor = crColor;
        }

        release_dc_ptr( dc );
    }

    return oldClr;
}

/***********************************************************************
 *           GetDCPenColor    (GDI32.@)
 *
 * Retrieves the current pen color for the specified device
 * context (DC).
 *
 */
COLORREF WINAPI GetDCPenColor(HDC hdc)
{
    DC *dc;
    COLORREF dcPenColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        dcPenColor = dc->dcPenColor;
	release_dc_ptr( dc );
    }

    return dcPenColor;
}

/***********************************************************************
 *           SetDCPenColor    (GDI32.@)
 *
 * Sets the current device context (DC) pen color to the specified
 * color value. If the device cannot represent the specified color
 * value, the color is set to the nearest physical color.
 *
 */
COLORREF WINAPI SetDCPenColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (dc->funcs->pSetDCPenColor)
            crColor = dc->funcs->pSetDCPenColor( dc->physDev, crColor );
        else if (dc->hPen == GetStockObject( DC_PEN ))
        {
            /* If DC_PEN is selected, update the driver pen color */
            LOGPEN logpen = { PS_SOLID, { 0, 0 }, crColor };
            HPEN hPen = CreatePenIndirect( &logpen );
            dc->funcs->pSelectPen( dc->physDev, hPen );
	    DeleteObject( hPen );
	}

        if (crColor != CLR_INVALID)
        {
            oldClr = dc->dcPenColor;
            dc->dcPenColor = crColor;
        }

        release_dc_ptr( dc );
    }

    return oldClr;
}

/***********************************************************************
 *           CancelDC    (GDI32.@)
 */
BOOL WINAPI CancelDC(HDC hdc)
{
    FIXME("stub\n");
    return TRUE;
}

/*******************************************************************
 *      GetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI GetMiterLimit(HDC hdc, PFLOAT peLimit)
{
    BOOL bRet = FALSE;
    DC *dc;

    TRACE("(%p,%p)\n", hdc, peLimit);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (peLimit)
            *peLimit = dc->miterLimit;

        release_dc_ptr( dc );
        bRet = TRUE;
    }
    return bRet;
}

/*******************************************************************
 *      SetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI SetMiterLimit(HDC hdc, FLOAT eNewLimit, PFLOAT peOldLimit)
{
    BOOL bRet = FALSE;
    DC *dc;

    TRACE("(%p,%f,%p)\n", hdc, eNewLimit, peOldLimit);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (peOldLimit)
            *peOldLimit = dc->miterLimit;
        dc->miterLimit = eNewLimit;
        release_dc_ptr( dc );
        bRet = TRUE;
    }
    return bRet;
}

/*******************************************************************
 *      GdiIsMetaPrintDC [GDI32.@]
 */
BOOL WINAPI GdiIsMetaPrintDC(HDC hdc)
{
    FIXME("%p\n", hdc);
    return FALSE;
}

/*******************************************************************
 *      GdiIsMetaFileDC [GDI32.@]
 */
BOOL WINAPI GdiIsMetaFileDC(HDC hdc)
{
    TRACE("%p\n", hdc);

    switch( GetObjectType( hdc ) )
    {
    case OBJ_METADC:
    case OBJ_ENHMETADC:
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************
 *      GdiIsPlayMetafileDC [GDI32.@]
 */
BOOL WINAPI GdiIsPlayMetafileDC(HDC hdc)
{
    FIXME("%p\n", hdc);
    return FALSE;
}
