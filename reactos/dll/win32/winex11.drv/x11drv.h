/*
 * X11 driver definitions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
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

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdarg.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef HAVE_LIBXXSHM
# include <X11/extensions/XShm.h>
#endif /* defined(HAVE_LIBXXSHM) */

#define BOOL X_BOOL
#define BYTE X_BYTE
#define INT8 X_INT8
#define INT16 X_INT16
#define INT32 X_INT32
#define INT64 X_INT64
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#undef BOOL
#undef BYTE
#undef INT8
#undef INT16
#undef INT32
#undef INT64
#undef LONG64

#undef Status  /* avoid conflict with wintrnl.h */
typedef int Status;

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddrawi.h"
#include "wine/list.h"

#define MAX_PIXELFORMATS 8
#define MAX_DASHLEN 16

#define WINE_XDND_VERSION 4

extern void CDECL wine_tsx11_lock(void);
extern void CDECL wine_tsx11_unlock(void);

  /* X physical pen */
typedef struct
{
    int          style;
    int          endcap;
    int          linejoin;
    int          pixel;
    int          width;
    char         dashes[MAX_DASHLEN];
    int          dash_len;
    int          type;          /* GEOMETRIC || COSMETIC */
    int          ext;           /* extended pen - 1, otherwise - 0 */
} X_PHYSPEN;

  /* X physical brush */
typedef struct
{
    int          style;
    int          fillStyle;
    int          pixel;
    Pixmap       pixmap;
} X_PHYSBRUSH;

enum x11drv_shm_mode
{
    X11DRV_SHM_NONE = 0,
    X11DRV_SHM_PIXMAP,
    X11DRV_SHM_IMAGE,
};

typedef struct {
    int shift;
    int scale;
    int max;
} ChannelShift;

typedef struct
{
    ChannelShift physicalRed, physicalBlue, physicalGreen;
    ChannelShift logicalRed, logicalBlue, logicalGreen;
} ColorShifts;

  /* X physical bitmap */
typedef struct
{
    HBITMAP      hbitmap;
    Pixmap       pixmap;
    XID          glxpixmap;
    int          pixmap_depth;
    ColorShifts  pixmap_color_shifts;
    /* the following fields are only used for DIB section bitmaps */
    int          status, p_status;  /* mapping status */
    XImage      *image;             /* cached XImage */
    int         *colorMap;          /* color map info */
    int          nColorMap;
    BOOL         trueColor;
    BOOL         topdown;
    CRITICAL_SECTION lock;          /* GDI access lock */
    enum x11drv_shm_mode shm_mode;
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo shminfo;        /* shared memory segment info */
#endif
    struct list   entry;            /* Entry in global DIB list */
    BYTE         *base;             /* Base address */
    SIZE_T        size;             /* Size in bytes */
} X_PHYSBITMAP;

  /* X physical font */
typedef UINT	 X_PHYSFONT;

struct xrender_info;

  /* X physical device */
typedef struct
{
    HDC           hdc;
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    RECT          dc_rect;       /* DC rectangle relative to drawable */
    RECT          drawable_rect; /* Drawable rectangle relative to screen */
    HRGN          region;        /* Device region (visible region & clip region) */
    X_PHYSFONT    font;
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
    X_PHYSBITMAP *bitmap;       /* currently selected bitmap for memory DCs */
    BOOL          has_gdi_font; /* is current font a GDI font? */
    int           backgroundPixel;
    int           textPixel;
    int           depth;       /* bit depth of the DC */
    ColorShifts  *color_shifts; /* color shifts of the DC */
    int           exposures;   /* count of graphics exposures operations */
    int           current_pf;
    Drawable      gl_drawable;
    Pixmap        pixmap;      /* Pixmap for a GLXPixmap gl_drawable */
    int           gl_copy;
    struct xrender_info *xrender;
} X11DRV_PDEVICE;

struct bitblt_coords
{
    int  x;      /* original position and width */
    int  y;
    int  width;
    int  height;
    RECT visrect;   /* rectangle clipped to the visible part */
    DWORD layout;   /* DC layout */
};


extern X_PHYSBITMAP BITMAP_stock_phys_bitmap;  /* phys bitmap for the default stock bitmap */

/* Retrieve the GC used for bitmap operations */
extern GC get_bitmap_gc(int depth);

/* Wine driver X11 functions */

extern BOOL CDECL X11DRV_AlphaBlend( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                                     INT widthDst, INT heightDst,
                                     X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                                     INT widthSrc, INT heightSrc, BLENDFUNCTION blendfn );
extern BOOL CDECL X11DRV_EnumDeviceFonts( X11DRV_PDEVICE *physDev, LPLOGFONTW plf,
                                          FONTENUMPROCW dfeproc, LPARAM lp );
extern LONG CDECL X11DRV_GetBitmapBits( HBITMAP hbitmap, void *bits, LONG count );
extern BOOL CDECL X11DRV_GetCharWidth( X11DRV_PDEVICE *physDev, UINT firstChar,
                                       UINT lastChar, LPINT buffer );
extern BOOL CDECL X11DRV_GetTextExtentExPoint( X11DRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                               INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size );
extern BOOL CDECL X11DRV_GetTextMetrics(X11DRV_PDEVICE *physDev, TEXTMETRICW *metrics);
extern BOOL CDECL X11DRV_StretchBlt( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                                     INT widthDst, INT heightDst,
                                     X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                                     INT widthSrc, INT heightSrc, DWORD rop );
extern BOOL CDECL X11DRV_LineTo( X11DRV_PDEVICE *physDev, INT x, INT y);
extern BOOL CDECL X11DRV_Arc( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
                              INT bottom, INT xstart, INT ystart, INT xend, INT yend );
extern BOOL CDECL X11DRV_Pie( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
                              INT bottom, INT xstart, INT ystart, INT xend,
                              INT yend );
extern BOOL CDECL X11DRV_Chord( X11DRV_PDEVICE *physDev, INT left, INT top,
                                INT right, INT bottom, INT xstart,
                                INT ystart, INT xend, INT yend );
extern BOOL CDECL X11DRV_Ellipse( X11DRV_PDEVICE *physDev, INT left, INT top,
                                  INT right, INT bottom );
extern BOOL CDECL X11DRV_Rectangle(X11DRV_PDEVICE *physDev, INT left, INT top,
                                   INT right, INT bottom);
extern BOOL CDECL X11DRV_RoundRect( X11DRV_PDEVICE *physDev, INT left, INT top,
                                    INT right, INT bottom, INT ell_width,
                                    INT ell_height );
extern COLORREF CDECL X11DRV_SetPixel( X11DRV_PDEVICE *physDev, INT x, INT y, COLORREF color );
extern COLORREF CDECL X11DRV_GetPixel( X11DRV_PDEVICE *physDev, INT x, INT y);
extern BOOL CDECL X11DRV_PaintRgn( X11DRV_PDEVICE *physDev, HRGN hrgn );
extern BOOL CDECL X11DRV_Polyline( X11DRV_PDEVICE *physDev,const POINT* pt,INT count);
extern BOOL CDECL X11DRV_Polygon( X11DRV_PDEVICE *physDev, const POINT* pt, INT count );
extern BOOL CDECL X11DRV_PolyPolygon( X11DRV_PDEVICE *physDev, const POINT* pt,
                                      const INT* counts, UINT polygons);
extern BOOL CDECL X11DRV_PolyPolyline( X11DRV_PDEVICE *physDev, const POINT* pt,
                                       const DWORD* counts, DWORD polylines);

extern COLORREF CDECL X11DRV_SetBkColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern COLORREF CDECL X11DRV_SetTextColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern BOOL CDECL X11DRV_ExtFloodFill( X11DRV_PDEVICE *physDev, INT x, INT y,
                                       COLORREF color, UINT fillType );
extern BOOL CDECL X11DRV_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y,
                                     UINT flags, const RECT *lprect,
                                     LPCWSTR str, UINT count, const INT *lpDx );
extern LONG CDECL X11DRV_SetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count );
extern void CDECL X11DRV_SetDeviceClipping( X11DRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn );
extern INT CDECL X11DRV_SetDIBitsToDevice( X11DRV_PDEVICE *physDev, INT xDest,
                                           INT yDest, DWORD cx, DWORD cy,
                                           INT xSrc, INT ySrc,
                                           UINT startscan, UINT lines,
                                           LPCVOID bits, const BITMAPINFO *info,
                                           UINT coloruse );
extern BOOL CDECL X11DRV_GetDeviceGammaRamp( X11DRV_PDEVICE *physDev, LPVOID ramp );
extern BOOL CDECL X11DRV_SetDeviceGammaRamp( X11DRV_PDEVICE *physDev, LPVOID ramp );

/* OpenGL / X11 driver functions */
extern int CDECL X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
		                      const PIXELFORMATDESCRIPTOR *pppfd);
extern int CDECL X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
		                        int iPixelFormat, UINT nBytes,
					PIXELFORMATDESCRIPTOR *ppfd);
extern int CDECL X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev);
extern BOOL CDECL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev);
extern void X11DRV_OpenGL_Cleanup(void);

/* X11 driver internal functions */

extern void X11DRV_Xcursor_Init(void);
extern void X11DRV_BITMAP_Init(void);
extern void X11DRV_FONT_Init( int log_pixels_x, int log_pixels_y );

extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse );
extern XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth );
extern void X11DRV_DIB_DestroyXImage( XImage *image );
extern HGLOBAL X11DRV_DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp);
extern HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc);
extern Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc );
extern X_PHYSBITMAP *X11DRV_get_phys_bitmap( HBITMAP hbitmap );
extern X_PHYSBITMAP *X11DRV_init_phys_bitmap( HBITMAP hbitmap );
extern Pixmap X11DRV_get_pixmap( HBITMAP hbitmap );

extern RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp );

extern BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors );
extern BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev );
extern BOOL X11DRV_SetupGCForText( X11DRV_PDEVICE *physDev );
extern INT X11DRV_XWStoDS( X11DRV_PDEVICE *physDev, INT width );
extern INT X11DRV_YWStoDS( X11DRV_PDEVICE *physDev, INT height );

extern const int X11DRV_XROPfunction[];

extern void _XInitImageFuncPtrs(XImage *);

extern int client_side_with_core;
extern int client_side_with_render;
extern int client_side_antialias_with_core;
extern int client_side_antialias_with_render;
extern int using_client_side_fonts;
extern void X11DRV_XRender_Init(void);
extern void X11DRV_XRender_Finalize(void);
extern BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE*, HFONT);
extern void X11DRV_XRender_SetDeviceClipping(X11DRV_PDEVICE *physDev, const RGNDATA *data);
extern void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE*);
extern void X11DRV_XRender_CopyBrush(X11DRV_PDEVICE *physDev, X_PHYSBITMAP *physBitmap, int width, int height);
extern BOOL X11DRV_XRender_ExtTextOut(X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				      const RECT *lprect, LPCWSTR wstr,
				      UINT count, const INT *lpDx);
extern BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib);
BOOL X11DRV_XRender_GetSrcAreaStretch(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      Pixmap pixmap, GC gc,
                                      const struct bitblt_coords *src, const struct bitblt_coords *dst );
extern void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev);
extern BOOL XRender_AlphaBlend( X11DRV_PDEVICE *devDst, X11DRV_PDEVICE *devSrc,
                                struct bitblt_coords *dst, struct bitblt_coords *src,
                                BLENDFUNCTION blendfn );

extern Drawable get_glxdrawable(X11DRV_PDEVICE *physDev);
extern BOOL destroy_glxpixmap(Display *display, XID glxpixmap);

/* IME support */
extern void IME_UnregisterClasses(void);
extern void IME_SetOpenStatus(BOOL fOpen, BOOL force);
extern INT IME_GetCursorPos(void);
extern void IME_SetCursorPos(DWORD pos);
extern void IME_UpdateAssociation(HWND focus);
extern BOOL IME_SetCompositionString(DWORD dwIndex, LPCVOID lpComp,
                                     DWORD dwCompLen, LPCVOID lpRead,
                                     DWORD dwReadLen);
extern void IME_SetResultString(LPWSTR lpResult, DWORD dwResultlen);

extern void X11DRV_XDND_EnterEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_PositionEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_DropEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_LeaveEvent( HWND hWnd, XClientMessageEvent *event );

/* exported dib functions for now */

/* DIB Section sync state */
enum { DIB_Status_None, DIB_Status_InSync, DIB_Status_GdiMod, DIB_Status_AppMod };

typedef struct {
    void (*Convert_5x5_asis)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes);
    void (*Convert_555_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_555_to_565_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_555_to_565_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_555_to_888_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_555_to_888_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_555_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_555_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_5x5_to_any0888)(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   WORD rsrc, WORD gsrc, WORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_565_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_565_to_555_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_565_to_555_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_565_to_888_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_565_to_888_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_565_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_565_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_888_asis)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes);
    void (*Convert_888_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_888_to_555_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_888_to_555_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_888_to_565_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_888_to_565_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_888_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_888_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_rgb888_to_any0888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_bgr888_to_any0888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_0888_asis)(int width, int height,
                              const void* srcbits, int srclinebytes,
                              void* dstbits, int dstlinebytes);
    void (*Convert_0888_reverse)(int width, int height,
                                 const void* srcbits, int srclinebytes,
                                 void* dstbits, int dstlinebytes);
    void (*Convert_0888_any)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             DWORD rsrc, DWORD gsrc, DWORD bsrc,
                             void* dstbits, int dstlinebytes,
                             DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_0888_to_555_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_555_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_565_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_565_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_5x5)(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   WORD rdst, WORD gdst, WORD bdst);
    void (*Convert_0888_to_888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_rgb888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_bgr888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes);
} dib_conversions;

extern const dib_conversions dib_normal, dib_src_byteswap, dib_dst_byteswap;

extern INT X11DRV_DIB_MaskToShift(DWORD mask);
extern INT X11DRV_CoerceDIBSection(X11DRV_PDEVICE *physDev,INT);
extern INT X11DRV_LockDIBSection(X11DRV_PDEVICE *physDev,INT);
extern void X11DRV_UnlockDIBSection(X11DRV_PDEVICE *physDev,BOOL);

extern void X11DRV_DIB_DeleteDIBSection(X_PHYSBITMAP *physBitmap, DIBSECTION *dib);
extern void X11DRV_DIB_CopyDIBSection(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      DWORD xSrc, DWORD ySrc, DWORD xDest, DWORD yDest,
                                      DWORD width, DWORD height);
struct _DCICMD;
extern INT X11DRV_DCICommand(INT cbInput, const struct _DCICMD *lpCmd, LPVOID lpOutData);

/**************************************************************************
 * X11 GDI driver
 */

extern void X11DRV_GDI_Finalize(void);

extern Display *gdi_display;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define X11DRV_PALETTE_WHITESET 0x2000

extern Colormap X11DRV_PALETTE_PaletteXColormap;
extern UINT16 X11DRV_PALETTE_PaletteFlags;

extern int *X11DRV_PALETTE_PaletteToXPixel;
extern int *X11DRV_PALETTE_XPixelToPalette;
extern ColorShifts X11DRV_PALETTE_default_shifts;

extern int X11DRV_PALETTE_mapEGAPixel[16];

extern int X11DRV_PALETTE_Init(void);
extern void X11DRV_PALETTE_Cleanup(void);
extern BOOL X11DRV_IsSolidColor(COLORREF color);

extern COLORREF X11DRV_PALETTE_ToLogical(X11DRV_PDEVICE *physDev, int pixel);
extern int X11DRV_PALETTE_ToPhysical(X11DRV_PDEVICE *physDev, COLORREF color);
extern COLORREF X11DRV_PALETTE_GetColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern int X11DRV_PALETTE_LookupPixel(ColorShifts *shifts, COLORREF color);
extern void X11DRV_PALETTE_ComputeColorShifts(ColorShifts *shifts, unsigned long redMask, unsigned long greenMask, unsigned long blueMask);

extern unsigned int depth_to_bpp( unsigned int depth );

/* GDI escapes */

#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,      /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,     /* get current drawable for a DC */
    X11DRV_GET_FONT,         /* get current X font for a DC */
    X11DRV_SET_DRAWABLE,     /* set current drawable for a DC */
    X11DRV_START_EXPOSURES,  /* start graphics exposures */
    X11DRV_END_EXPOSURES,    /* end graphics exposures */
    X11DRV_GET_DCE,          /* no longer used */
    X11DRV_SET_DCE,          /* no longer used */
    X11DRV_GET_GLX_DRAWABLE, /* get current glx drawable for a DC */
    X11DRV_SYNC_PIXMAP,      /* sync the dibsection to its pixmap */
    X11DRV_FLUSH_GL_DRAWABLE /* flush changes made to the gl drawable */
};

struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    RECT                     drawable_rect;/* Drawable rectangle relative to screen */
    XID                      fbconfig_id;  /* fbconfig id used by the GL drawable */
    Drawable                 gl_drawable;  /* GL drawable */
    Pixmap                   pixmap;       /* Pixmap for a GLXPixmap gl_drawable */
    int                      gl_copy;      /* whether the GL contents need explicit copying */
};

/**************************************************************************
 * X11 USER driver
 */

struct x11drv_thread_data
{
    Display *display;
    XEvent  *current_event;        /* event currently being processed */
    Window   grab_window;          /* window that currently grabs the mouse */
    HWND     last_focus;           /* last window that had focus */
    XIM      xim;                  /* input method */
    HWND     last_xic_hwnd;        /* last xic window */
    XFontSet font_set;             /* international text drawing font set */
    Window   selection_wnd;        /* window used for selection interactions */
    HKL      kbd_layout;           /* active keyboard layout */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void);
extern DWORD thread_data_tls_index;

static inline struct x11drv_thread_data *x11drv_thread_data(void)
{
    return TlsGetValue( thread_data_tls_index );
}

/* retrieve the thread display, or NULL if not created yet */
static inline Display *thread_display(void)
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    if (!data) return NULL;
    return data->display;
}

/* retrieve the thread display, creating it if needed */
static inline Display *thread_init_display(void)
{
    return x11drv_init_thread_data()->display;
}

static inline size_t get_property_size( int format, unsigned long count )
{
    /* format==32 means long, which can be 64 bits... */
    if (format == 32) return count * sizeof(long);
    return count * (format / 8);
}

extern Visual *visual;
extern Window root_window;
extern unsigned int screen_width;
extern unsigned int screen_height;
extern unsigned int screen_bpp;
extern unsigned int screen_depth;
extern RECT virtual_screen_rect;
extern unsigned int text_caps;
extern int dxgrab;
extern int use_xkb;
extern int use_take_focus;
extern int use_primary_selection;
extern int use_system_cursors;
extern int show_systray;
extern int usexcomposite;
extern int managed_mode;
extern int decorated_mode;
extern int private_color_map;
extern int primary_monitor;
extern int copy_default_colors;
extern int alloc_system_colors;
extern int xrender_error_base;
extern HMODULE x11drv_module;

extern BYTE key_state_table[256];
extern POINT cursor_pos;

/* atoms */

enum x11drv_atoms
{
    FIRST_XATOM = XA_LAST_PREDEFINED + 1,
    XATOM_CLIPBOARD = FIRST_XATOM,
    XATOM_COMPOUND_TEXT,
    XATOM_INCR,
    XATOM_MANAGER,
    XATOM_MULTIPLE,
    XATOM_SELECTION_DATA,
    XATOM_TARGETS,
    XATOM_TEXT,
    XATOM_UTF8_STRING,
    XATOM_RAW_ASCENT,
    XATOM_RAW_DESCENT,
    XATOM_RAW_CAP_HEIGHT,
    XATOM_WM_PROTOCOLS,
    XATOM_WM_DELETE_WINDOW,
    XATOM_WM_STATE,
    XATOM_WM_TAKE_FOCUS,
    XATOM_DndProtocol,
    XATOM_DndSelection,
    XATOM__ICC_PROFILE,
    XATOM__MOTIF_WM_HINTS,
    XATOM__NET_STARTUP_INFO_BEGIN,
    XATOM__NET_STARTUP_INFO,
    XATOM__NET_SUPPORTED,
    XATOM__NET_SYSTEM_TRAY_OPCODE,
    XATOM__NET_SYSTEM_TRAY_S0,
    XATOM__NET_WM_ICON,
    XATOM__NET_WM_MOVERESIZE,
    XATOM__NET_WM_NAME,
    XATOM__NET_WM_PID,
    XATOM__NET_WM_PING,
    XATOM__NET_WM_STATE,
    XATOM__NET_WM_STATE_ABOVE,
    XATOM__NET_WM_STATE_FULLSCREEN,
    XATOM__NET_WM_STATE_MAXIMIZED_HORZ,
    XATOM__NET_WM_STATE_MAXIMIZED_VERT,
    XATOM__NET_WM_STATE_SKIP_PAGER,
    XATOM__NET_WM_STATE_SKIP_TASKBAR,
    XATOM__NET_WM_USER_TIME,
    XATOM__NET_WM_USER_TIME_WINDOW,
    XATOM__NET_WM_WINDOW_OPACITY,
    XATOM__NET_WM_WINDOW_TYPE,
    XATOM__NET_WM_WINDOW_TYPE_DIALOG,
    XATOM__NET_WM_WINDOW_TYPE_NORMAL,
    XATOM__NET_WM_WINDOW_TYPE_UTILITY,
    XATOM__NET_WORKAREA,
    XATOM__XEMBED,
    XATOM__XEMBED_INFO,
    XATOM_XdndAware,
    XATOM_XdndEnter,
    XATOM_XdndPosition,
    XATOM_XdndStatus,
    XATOM_XdndLeave,
    XATOM_XdndFinished,
    XATOM_XdndDrop,
    XATOM_XdndActionCopy,
    XATOM_XdndActionMove,
    XATOM_XdndActionLink,
    XATOM_XdndActionAsk,
    XATOM_XdndActionPrivate,
    XATOM_XdndSelection,
    XATOM_XdndTarget,
    XATOM_XdndTypeList,
    XATOM_HTML_Format,
    XATOM_WCF_DIB,
    XATOM_image_gif,
    XATOM_image_jpeg,
    XATOM_image_png,
    XATOM_text_html,
    XATOM_text_plain,
    XATOM_text_rtf,
    XATOM_text_richtext,
    XATOM_text_uri_list,
    NB_XATOMS
};

extern Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM];
extern Atom systray_atom;

#define x11drv_atom(name) (X11DRV_Atoms[XATOM_##name - FIRST_XATOM])

/* X11 event driver */

typedef void (*x11drv_event_handler)( HWND hwnd, XEvent *event );

extern void X11DRV_register_event_handler( int type, x11drv_event_handler handler );

extern void X11DRV_ButtonPress( HWND hwnd, XEvent *event );
extern void X11DRV_ButtonRelease( HWND hwnd, XEvent *event );
extern void X11DRV_MotionNotify( HWND hwnd, XEvent *event );
extern void X11DRV_EnterNotify( HWND hwnd, XEvent *event );
extern void X11DRV_KeyEvent( HWND hwnd, XEvent *event );
extern void X11DRV_KeymapNotify( HWND hwnd, XEvent *event );
extern void X11DRV_DestroyNotify( HWND hwnd, XEvent *event );
extern void X11DRV_SelectionRequest( HWND hWnd, XEvent *event );
extern void X11DRV_SelectionClear( HWND hWnd, XEvent *event );
extern void X11DRV_MappingNotify( HWND hWnd, XEvent *event );

extern DWORD EVENT_x11_time_to_win32_time(Time time);

/* X11 driver private messages, must be in the range 0x80001000..0x80001fff */
enum x11drv_window_messages
{
    WM_X11DRV_ACQUIRE_SELECTION = 0x80001000,
    WM_X11DRV_SET_WIN_FORMAT,
    WM_X11DRV_SET_WIN_REGION,
    WM_X11DRV_RESIZE_DESKTOP,
    WM_X11DRV_SET_CURSOR
};

/* _NET_WM_STATE properties that we keep track of */
enum x11drv_net_wm_state
{
    NET_WM_STATE_FULLSCREEN,
    NET_WM_STATE_ABOVE,
    NET_WM_STATE_MAXIMIZED,
    NET_WM_STATE_SKIP_PAGER,
    NET_WM_STATE_SKIP_TASKBAR,
    NB_NET_WM_STATES
};

/* x11drv private window data */
struct x11drv_win_data
{
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      client_window;  /* X window for the client area */
    Window      icon_window;    /* X window for the icon */
    Colormap    colormap;       /* Colormap for this window */
    VisualID    visualid;       /* visual id of the client window */
    XID         fbconfig_id;    /* fbconfig id for the GL drawable this hwnd uses */
    Drawable    gl_drawable;    /* Optional GL drawable for rendering the client area */
    Pixmap      pixmap;         /* Base pixmap for if gl_drawable is a GLXPixmap */
    RECT        window_rect;    /* USER window rectangle relative to parent */
    RECT        whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT        client_rect;    /* client area relative to parent */
    XIC         xic;            /* X input context */
    HCURSOR     cursor;         /* current cursor */
    XWMHints   *wm_hints;       /* window manager hints */
    BOOL        managed : 1;    /* is window managed? */
    BOOL        mapped : 1;     /* is window mapped? (in either normal or iconic state) */
    BOOL        iconic : 1;     /* is window in iconic state? */
    BOOL        embedded : 1;   /* is window an XEMBED client? */
    BOOL        shaped : 1;     /* is window using a custom region shape? */
    int         wm_state;       /* current value of the WM_STATE property */
    DWORD       net_wm_state;   /* bit mask of active x11drv_net_wm_state values */
    Window      embedder;       /* window id of embedder */
    unsigned long configure_serial; /* serial number of last configure request */
    HBITMAP     hWMIconBitmap;
    HBITMAP     hWMIconMask;
};

extern struct x11drv_win_data *X11DRV_get_win_data( HWND hwnd );
extern struct x11drv_win_data *X11DRV_create_win_data( HWND hwnd );
extern Window X11DRV_get_whole_window( HWND hwnd );
extern XIC X11DRV_get_ic( HWND hwnd );

extern int pixelformat_from_fbconfig_id( XID fbconfig_id );
extern XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id );
extern void mark_drawable_dirty( Drawable old, Drawable new );
extern Drawable create_glxpixmap( Display *display, XVisualInfo *vis, Pixmap parent );
extern void flush_gl_drawable( X11DRV_PDEVICE *physDev );

extern void wait_for_withdrawn_state( Display *display, struct x11drv_win_data *data, BOOL set );
extern void update_user_time( Time time );
extern void update_net_wm_states( Display *display, struct x11drv_win_data *data );
extern void make_window_embedded( Display *display, struct x11drv_win_data *data );
extern void change_systray_owner( Display *display, Window systray_window );
extern HWND create_foreign_window( Display *display, Window window );

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* X context to associate a hwnd to an X window */
extern XContext winContext;

extern void X11DRV_InitClipboard(void);
extern int CDECL X11DRV_AcquireClipboard(HWND hWndClipWindow);
extern void X11DRV_Clipboard_Cleanup(void);
extern void X11DRV_ResetSelectionOwner(void);
extern void CDECL X11DRV_SetFocus( HWND hwnd );
extern void set_window_cursor( HWND hwnd, HCURSOR handle );
extern BOOL CDECL X11DRV_ClipCursor( LPCRECT clip );
extern void X11DRV_InitKeyboard( Display *display );
extern void X11DRV_send_keyboard_input( WORD wVk, WORD wScan, DWORD dwFlags, DWORD time,
                                        DWORD dwExtraInfo, UINT injected_flags );
extern void X11DRV_send_mouse_input( HWND top_hwnd, HWND cursor_hwnd, DWORD flags, DWORD x, DWORD y,
                                     DWORD data, DWORD time, DWORD extra_info, UINT injected_flags );
extern DWORD CDECL X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                       DWORD mask, DWORD flags );

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg );
extern int X11DRV_check_error(void);
extern void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect );
extern void xinerama_init( unsigned int width, unsigned int height );

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height );
extern void X11DRV_resize_desktop(unsigned int width, unsigned int height);
extern void X11DRV_Settings_AddDepthModes(void);
extern void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq);
extern int X11DRV_Settings_CreateDriver(LPDDHALINFO info);
extern LPDDHALMODEINFO X11DRV_Settings_CreateModes(unsigned int max_modes, int reserve_depths);
unsigned int X11DRV_Settings_GetModeCount(void);
void X11DRV_Settings_Init(void);
LPDDHALMODEINFO X11DRV_Settings_SetHandlers(const char *name,
                                            int (*pNewGCM)(void),
                                            LONG (*pNewSCM)(int),
                                            unsigned int nmodes,
                                            int reserve_depths);

extern void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr, LPVIDMEM fb_mem);

/* XIM support */
extern BOOL X11DRV_InitXIM( const char *input_style ) DECLSPEC_HIDDEN;
extern XIC X11DRV_CreateIC(XIM xim, struct x11drv_win_data *data) DECLSPEC_HIDDEN;
extern void X11DRV_SetupXIM(void) DECLSPEC_HIDDEN;
extern void X11DRV_XIMLookupChars( const char *str, DWORD count ) DECLSPEC_HIDDEN;
extern void X11DRV_ForceXIMReset(HWND hwnd) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetPreeditState(HWND hwnd, BOOL fOpen);

/* FIXME: private functions imported from user32 */
extern LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode );

#define XEMBED_MAPPED  (1 << 0)

#endif  /* __WINE_X11DRV_H */
