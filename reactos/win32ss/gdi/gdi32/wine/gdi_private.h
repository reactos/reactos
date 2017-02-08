/*
 * GDI definitions
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

#ifndef __WINE_GDI_PRIVATE_H
#define __WINE_GDI_PRIVATE_H

#include <limits.h>
#include <math.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include <wine/gdi_driver.h>

/* Metafile defines */
#define META_EOF 0x0000
/* values of mtType in METAHEADER.  Note however that the disk image of a disk
   based metafile has mtType == 1 */
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2
#define MFHEADERSIZE (sizeof(METAHEADER))
#define MFVERSION 0x300

typedef struct {
    EMR   emr;
    INT   nBreakExtra;
    INT   nBreakCount;
} EMRSETTEXTJUSTIFICATION, *PEMRSETTEXTJUSTIFICATION;

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, HDC hdc );
    INT     (*pGetObjectA)( HGDIOBJ handle, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle );
    BOOL    (*pDeleteObject)( HGDIOBJ handle );
};

typedef struct tagWINEDC
{
    HDC          hdc;
    PHYSDEV      physDev;          /* current top of the physdev stack */
    LONG         refcount;         /* thread refcount */
    INT          saveLevel;
    struct gdi_physdev NullPhysDev;
    HFONT hFont;
    HBRUSH hBrush;
    HPEN hPen;
    HPALETTE hPalette;
} WINEDC, DC;

/* brush.c */
extern BOOL get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void **bits, UINT *usage ) DECLSPEC_HIDDEN;

/* dc.c */
extern DC *alloc_dc_ptr( WORD magic ) DECLSPEC_HIDDEN;
extern void free_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;
extern DC *get_dc_ptr( HDC hdc ) DECLSPEC_HIDDEN;
extern void release_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;

/* dib.c */
extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk ) DECLSPEC_HIDDEN;

/* gdiobj.c */
extern HGDIOBJ alloc_gdi_handle( void *obj, WORD type, const struct gdi_obj_funcs *funcs ) DECLSPEC_HIDDEN;
extern void *free_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern HGDIOBJ get_full_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern void *GDI_GetObjPtr( HGDIOBJ, WORD ) DECLSPEC_HIDDEN;
extern void GDI_ReleaseObj( HGDIOBJ ) DECLSPEC_HIDDEN;
extern void GDI_hdc_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;
extern void GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh) DECLSPEC_HIDDEN;
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCVOID filename, BOOL unicode ) DECLSPEC_HIDDEN;

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg) DECLSPEC_HIDDEN;
extern UINT WINAPI GDIRealizePalette( HDC hdc ) DECLSPEC_HIDDEN;

#define EMR_SETLINKEDUFI        119

/* Undocumented value for DIB's iUsage: Indicates a mono DIB w/o pal entries */
#define DIB_PAL_MONO 2

BOOL WINAPI SetVirtualResolution(HDC hdc, DWORD horz_res, DWORD vert_res, DWORD horz_size, DWORD vert_size);

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

/* only for use on sanitized BITMAPINFO structures */
static inline int get_dib_info_size( const BITMAPINFO *info, UINT coloruse )
{
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
    if (coloruse == DIB_PAL_COLORS)
        return sizeof(BITMAPINFOHEADER) + info->bmiHeader.biClrUsed * sizeof(WORD);
    return FIELD_OFFSET( BITMAPINFO, bmiColors[info->bmiHeader.biClrUsed] );
}

/* Special sauce for reactos */
#define GDIRealizePalette RealizePalette
#define GDISelectPalette SelectPalette

HGDIOBJ WINAPI GdiFixUpHandle(HGDIOBJ hGdiObj);
#define get_full_gdi_handle GdiFixUpHandle

extern void push_dc_driver_ros(PHYSDEV *dev, PHYSDEV physdev, const struct gdi_dc_funcs *funcs);
#define push_dc_driver push_dc_driver_ros

BOOL WINAPI SetWorldTransformForMetafile(HDC hdc, const XFORM *pxform);
#define SetWorldTransform SetWorldTransformForMetafile

#ifdef _M_ARM
#define DbgRaiseAssertionFailure() __emit(0xdefc)
#else
#define DbgRaiseAssertionFailure() __int2c()
#endif // _M_ARM

#undef ASSERT
#define ASSERT(x) if (!(x)) DbgRaiseAssertionFailure()

#endif /* __WINE_GDI_PRIVATE_H */

