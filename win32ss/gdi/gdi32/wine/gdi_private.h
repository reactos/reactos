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

#ifndef _NTGDITYP_
typedef enum GDILoObjType
{
    GDILoObjType_LO_BRUSH_TYPE = 0x100000,
    GDILoObjType_LO_DC_TYPE = 0x10000,
    GDILoObjType_LO_BITMAP_TYPE = 0x50000,
    GDILoObjType_LO_PALETTE_TYPE = 0x80000,
    GDILoObjType_LO_FONT_TYPE = 0xa0000,
    GDILoObjType_LO_REGION_TYPE = 0x40000,
    GDILoObjType_LO_ICMLCS_TYPE = 0x90000,
    GDILoObjType_LO_CLIENTOBJ_TYPE = 0x60000,
    GDILoObjType_LO_UMPD_TYPE = 0x110000,
    GDILoObjType_LO_META_TYPE = 0x150000,
    GDILoObjType_LO_ALTDC_TYPE = 0x210000,
    GDILoObjType_LO_PEN_TYPE = 0x300000,
    GDILoObjType_LO_EXTPEN_TYPE = 0x500000,
    GDILoObjType_LO_DIBSECTION_TYPE = 0x250000,
    GDILoObjType_LO_METAFILE16_TYPE = 0x260000,
    GDILoObjType_LO_METAFILE_TYPE = 0x460000,
    GDILoObjType_LO_METADC16_TYPE = 0x660000
} GDILOOBJTYPE, *PGDILOOBJTYPE;
#endif

#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_GET_TYPE(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK)

HRGN APIENTRY NtGdiPathToRegion(_In_ HDC hdc);
HDC APIENTRY NtGdiCreateMetafileDC(_In_ HDC hdc);
#define GdiWorldSpaceToDeviceSpace  0x204
BOOL APIENTRY NtGdiGetTransform(_In_ HDC hdc,_In_ DWORD iXform, _Out_ LPXFORM pxf);
/* Get/SetBounds/Rect support. */
#define DCB_WINDOWMGR 0x8000 /* Queries the Windows bounding rectangle instead of the application's */
BOOL WINAPI GetBoundsRectAlt(HDC hdc,LPRECT prc,UINT flags);
BOOL WINAPI SetBoundsRectAlt(HDC hdc,LPRECT prc,UINT flags);

HGDIOBJ WINAPI GdiCreateClientObj(_In_ PVOID pvObject, _In_ GDILOOBJTYPE eObjType);
PVOID WINAPI GdiGetClientObjLink(_In_ HGDIOBJ hobj);
PVOID WINAPI GdiDeleteClientObj(_In_ HGDIOBJ hobj);

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

typedef struct tagEMRESCAPE {
        EMR emr;
        INT iEsc;
        INT cjIn;
        BYTE Data[1];
} EMRESCAPE, *PEMRESCAPE, EMRNAMEDESCAPE, *PEMRNAMEDESCAPE;

INT WINAPI NamedEscape(HDC,PWCHAR,INT,INT,LPSTR,INT,LPSTR);

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, HDC hdc );
    INT     (*pGetObjectA)( HGDIOBJ handle, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle );
    BOOL    (*pDeleteObject)( HGDIOBJ handle );
};

/* DC_ATTR LCD Types */
#define LDC_LDC           0x00000001
#define LDC_EMFLDC        0x00000002

typedef struct emf *PEMF;

typedef struct tagWINEDC
{
    HDC          hdc;
    ULONG        Flags;
    INT          iType;
    PEMF         emf; /* Pointer to ENHMETAFILE structure */
    LPWSTR       pwszPort;
    ABORTPROC    pAbortProc;
    DWORD        CallBackTick;
    HANDLE       hPrinter;
    PDEVMODEW    pdm;
    PVOID        pUMPDev;
    PVOID        pUMdhpdev;
    PVOID        UFIHashTable[3];
    ULONG        ufi[2];
    PVOID        pvEMFSpoolData;
    ULONG        cjSize;
    LIST_ENTRY   leRecords;
    ULONG        DevCaps[36];
    HBRUSH       hBrush;
    HPEN         hPen;
    ////
    INT          save_level;
    RECTL        emf_bounds;
} WINEDC, DC;

static inline BOOL is_meta_dc( HDC hdc )
{
    return GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE;
}


/* brush.c */
extern BOOL get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void *bits, UINT *usage ) DECLSPEC_HIDDEN;

/* dc.c */
extern DC *alloc_dc_ptr( WORD magic ) DECLSPEC_HIDDEN;
extern void free_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;
extern DC *get_dc_ptr( HDC hdc ) DECLSPEC_HIDDEN;
extern void release_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;

/* dib.c */
extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, DWORD filesize, BOOL on_disk ) DECLSPEC_HIDDEN;

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

/* Format of comment record added by GetWinMetaFileBits */
#include <pshpack2.h>
typedef struct
{
    DWORD magic;        /* WMFC */
    DWORD comment_type; /* Always 0x00000001 */
    DWORD version;      /* Always 0x00010000 */
    WORD checksum;
    DWORD flags;        /* Always 0 */
    DWORD num_chunks;
    DWORD chunk_size;
    DWORD remaining_size;
    DWORD emf_size;
    BYTE emf_data[1];
} emf_in_wmf_comment;
#include <poppack.h>

#define WMFC_MAGIC 0x43464d57
/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg) DECLSPEC_HIDDEN;
extern UINT WINAPI GDIRealizePalette( HDC hdc ) DECLSPEC_HIDDEN;

DWORD WINAPI GetDCDWord(_In_ HDC hdc,_In_ UINT u,_In_ DWORD dwError);

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

#define GdiWorldSpaceToDeviceSpace  0x204
BOOL APIENTRY NtGdiGetTransform( _In_ HDC hdc, _In_ DWORD iXform, _Out_ LPXFORM pxf);

/* Special sauce for reactos */
#define GDIRealizePalette RealizePalette
#define GDISelectPalette SelectPalette

HGDIOBJ WINAPI GdiFixUpHandle(HGDIOBJ hGdiObj);
#define get_full_gdi_handle GdiFixUpHandle

#if 0
BOOL WINAPI SetWorldTransformForMetafile(HDC hdc, const XFORM *pxform);
#define SetWorldTransform SetWorldTransformForMetafile
#endif
#ifdef _M_ARM
#define DbgRaiseAssertionFailure() __emit(0xdefc)
#else
#define DbgRaiseAssertionFailure() __int2c()
#endif // _M_ARM

#undef ASSERT
#define ASSERT(x) if (!(x)) DbgRaiseAssertionFailure()

BOOL EMFDRV_LineTo( WINEDC *dc, INT x, INT y );
BOOL EMFDRV_RoundRect( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT ell_width, INT ell_height );
BOOL EMFDRV_ArcChordPie( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend, DWORD type );
BOOL EMFDRV_Arc( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend );
BOOL EMFDRV_ArcTo( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend );
BOOL EMFDRV_Pie( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend );
BOOL EMFDRV_Chord( WINEDC *dc, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend );
BOOL EMFDRV_Ellipse( WINEDC *dc, INT left, INT top, INT right, INT bottom );
BOOL EMFDRV_Rectangle( WINEDC *dc, INT left, INT top, INT right, INT bottom );
COLORREF EMFDRV_SetPixel( WINEDC *dc, INT x, INT y, COLORREF color );
BOOL EMFDRV_PolylineTo( WINEDC *dc, const POINT *pt, INT count );
BOOL EMFDRV_PolyBezier( WINEDC *dc, const POINT *pts, DWORD count );
BOOL EMFDRV_PolyBezierTo( WINEDC *dc, const POINT *pts, DWORD count );
BOOL EMFDRV_PolyPolyline( WINEDC *dc, const POINT *pt, const DWORD *counts, UINT polys );
BOOL EMFDRV_PolyPolygon( WINEDC *dc, const POINT *pt, const INT *counts, UINT polys );
BOOL EMFDRV_PolyDraw( WINEDC *dc, const POINT *pts, const BYTE *types, DWORD count );
BOOL EMFDRV_FillRgn( WINEDC *dc, HRGN hrgn, HBRUSH hbrush );
BOOL EMFDRV_FrameRgn( WINEDC *dc, HRGN hrgn, HBRUSH hbrush, INT width, INT height );
BOOL EMFDRV_InvertRgn( WINEDC *dc, HRGN hrgn );
BOOL EMFDRV_ExtTextOut( WINEDC *dc, INT x, INT y, UINT flags, const RECT *lprect,LPCWSTR str, UINT count, const INT *lpDx );
BOOL EMFDRV_GradientFill( WINEDC *dc, TRIVERTEX *vert_array, ULONG nvert, void *grad_array, ULONG ngrad, ULONG mode );
BOOL EMFDRV_FillPath( WINEDC *dc );
BOOL EMFDRV_StrokeAndFillPath( WINEDC *dc );
BOOL EMFDRV_StrokePath( WINEDC *dc );
BOOL EMFDRV_AlphaBlend( WINEDC *dc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,HDC dc_src, INT x_src, INT y_src, INT width_src, INT height_src, BLENDFUNCTION func );
BOOL EMFDRV_PatBlt( WINEDC *dc, INT left, INT top, INT width, INT height, DWORD rop );
INT EMFDRV_StretchDIBits( WINEDC *dc, INT x_dst, INT y_dst, INT width_dst,INT height_dst, INT x_src, INT y_src, INT width_src, INT height_src, const void *bits, BITMAPINFO *info, UINT wUsage, DWORD dwRop );
INT EMFDRV_SetDIBitsToDevice( WINEDC *dc, INT x_dst, INT y_dst, DWORD width, DWORD height, INT x_src, INT y_src, UINT startscan, UINT lines, const void *bits, BITMAPINFO *info, UINT usage );
HBITMAP EMFDRV_SelectBitmap( WINEDC *dc, HBITMAP hbitmap );


#endif /* __WINE_GDI_PRIVATE_H */

