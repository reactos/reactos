/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *  Copyright 2000 Jason Mawdsley
 *  Copyright 2001, 2004 Michael Stefaniuc
 *  Copyright 2001 Charles Loep for CodeWeavers
 *  Copyright 2002 Dimitrie O. Paun
 *  Copyright 2009 Owen Rudge for CodeWeavers
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
 *
 *  TODO:
 *    - Add support for ILD_PRESERVEALPHA, ILD_SCALE, ILD_DPISCALE
 *    - Add support for ILS_GLOW, ILS_SHADOW
 *    - Thread-safe locking
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "comctl32.h"
#include "commoncontrols.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagelist);

#define MAX_OVERLAYIMAGE 15

#ifdef __REACTOS__
//The big bad reactos image list hack!
BOOL is_valid2(HIMAGELIST himl);
INT WINAPI Internal_ReplaceIcon (HIMAGELIST himl, INT nIndex, HICON hIcon);
BOOL WINAPI Internal_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp);
COLORREF WINAPI Internal_SetBkColor (HIMAGELIST himl, COLORREF clrBk);
BOOL WINAPI Internal_SetOverlayImage (HIMAGELIST himl, INT iImage, INT iOverlay);

#define ImageList_Add Internal_Add
#define ImageList_ReplaceIcon Internal_ReplaceIcon
#define ImageList_SetOverlayImage Internal_SetOverlayImage
#define ImageList_Replace Internal_Replace
#define ImageList_AddMasked Internal_AddMasked
#define ImageList_Remove Internal_Remove
#define ImageList_GetIcon Internal_GetIcon
#define ImageList_GetImageInfo Internal_GetImageInfo
#define ImageList_Copy Internal_Copy
#define ImageList_Merge Internal_Merge
#define ImageList_Duplicate Internal_Duplicate
#define ImageList_GetIconSize Internal_GetIconSize
#define ImageList_SetIconSize Internal_SetIconSize
#define ImageList_GetImageCount Internal_GetImageCount
#define ImageList_SetImageCount Internal_SetImageCount
#define ImageList_SetBkColor Internal_SetBkColor
#define ImageList_GetBkColor Internal_GetBkColor
#define ImageList_BeginDrag Internal_BeginDrag
#define ImageList_DrawIndirect Internal_DrawIndirect
#endif

struct _IMAGELIST
{
    IImageList2 IImageList2_iface;         /* 00: IImageList vtable */
    INT         cCurImage;                 /* 04: ImageCount */
    INT         cMaxImage;                 /* 08: maximages */
    INT         cGrow;                     /* 0C: cGrow */
    INT         cx;                        /* 10: cx */
    INT         cy;                        /* 14: cy */
    DWORD       x4;
    UINT        flags;                     /* 1C: flags */
    COLORREF    clrFg;                     /* 20: foreground color */
    COLORREF    clrBk;                     /* 24: background color */


    HBITMAP     hbmImage;                  /* 28: images Bitmap */
    HBITMAP     hbmMask;                   /* 2C: masks  Bitmap */
    HDC         hdcImage;                  /* 30: images MemDC  */
    HDC         hdcMask;                   /* 34: masks  MemDC  */
    INT         nOvlIdx[MAX_OVERLAYIMAGE]; /* 38: overlay images index */

    /* not yet found out */
    #ifdef __REACTOS__
    ULONG   usMagic;
    #endif
    HBRUSH  hbrBlend25;
    HBRUSH  hbrBlend50;
    INT     cInitial;
    UINT    uBitsPixel;
    char   *has_alpha;
    BOOL    color_table_set;

    LONG        ref;                       /* reference count */
};

#define IMAGELIST_MAGIC 0x53414D58
#ifdef __REACTOS__
#define IMAGELIST_MAGIC_DESTROYED 0x44454144
#endif

/* Header used by ImageList_Read() and ImageList_Write() */
#include "pshpack2.h"
typedef struct _ILHEAD
{
    USHORT	usMagic;
    USHORT	usVersion;
    WORD	cCurImage;
    WORD	cMaxImage;
    WORD	cGrow;
    WORD	cx;
    WORD	cy;
    COLORREF	bkcolor;
    WORD	flags;
    SHORT	ovls[4];
} ILHEAD;
#include "poppack.h"

/* internal image list data used for Drag & Drop operations */
typedef struct
{
    HWND	hwnd;
    HIMAGELIST	himl;
    HIMAGELIST	himlNoCursor;
    /* position of the drag image relative to the window */
    INT		x;
    INT		y;
    /* offset of the hotspot relative to the origin of the image */
    INT		dxHotspot;
    INT		dyHotspot;
    /* is the drag image visible */
    BOOL	bShow;
    /* saved background */
    HBITMAP	hbmBg;
} INTERNALDRAG;

static INTERNALDRAG InternalDrag = { 0, 0, 0, 0, 0, 0, 0, FALSE, 0 };

static inline HIMAGELIST impl_from_IImageList2(IImageList2 *iface)
{
    return CONTAINING_RECORD(iface, struct _IMAGELIST, IImageList2_iface);
}

static HBITMAP ImageList_CreateImage(HDC hdc, HIMAGELIST himl, UINT count);
static HRESULT ImageListImpl_CreateInstance(const IUnknown *pUnkOuter, REFIID iid, void** ppv);
static BOOL is_valid(HIMAGELIST himl);

/*
 * An imagelist with N images is tiled like this:
 *
 *   N/4 ->
 *
 * 4 048C..
 *   159D..
 * | 26AE.N
 * V 37BF.
 */

#define TILE_COUNT 4

static inline UINT imagelist_height( UINT count )
{
    return ((count + TILE_COUNT - 1)/TILE_COUNT);
}

static inline void imagelist_point_from_index( HIMAGELIST himl, UINT index, LPPOINT pt )
{
    pt->x = (index%TILE_COUNT) * himl->cx;
    pt->y = (index/TILE_COUNT) * himl->cy;
}

static inline void imagelist_get_bitmap_size( HIMAGELIST himl, UINT count, SIZE *sz )
{
    sz->cx = himl->cx * TILE_COUNT;
    sz->cy = imagelist_height( count ) * himl->cy;
}

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

/*
 * imagelist_copy_images()
 *
 * Copies a block of count images from offset src in the list to offset dest.
 * Images are copied a row at at time. Assumes hdcSrc and hdcDest are different.
 */
static inline void imagelist_copy_images( HIMAGELIST himl, HDC hdcSrc, HDC hdcDest,
                                          UINT src, UINT count, UINT dest )
{
    POINT ptSrc, ptDest;
    SIZE sz;
    UINT i;

    for ( i=0; i<TILE_COUNT; i++ )
    {
        imagelist_point_from_index( himl, src+i, &ptSrc );
        imagelist_point_from_index( himl, dest+i, &ptDest );
        sz.cx = himl->cx;
        sz.cy = himl->cy * imagelist_height( count - i );

        BitBlt( hdcDest, ptDest.x, ptDest.y, sz.cx, sz.cy,
                hdcSrc, ptSrc.x, ptSrc.y, SRCCOPY );
    }
}

static void add_dib_bits( HIMAGELIST himl, int pos, int count, int width, int height,
                          BITMAPINFO *info, BITMAPINFO *mask_info, DWORD *bits, BYTE *mask_bits )
{
    int i, j, n;
    POINT pt;
    int stride = info->bmiHeader.biWidth;
    int mask_stride = (info->bmiHeader.biWidth + 31) / 32 * 4;

    for (n = 0; n < count; n++)
    {
        BOOL has_alpha = FALSE;

        imagelist_point_from_index( himl, pos + n, &pt );

        /* check if bitmap has an alpha channel */
        for (i = 0; i < height && !has_alpha; i++)
            for (j = n * width; j < (n + 1) * width; j++)
                if ((has_alpha = ((bits[i * stride + j] & 0xff000000) != 0))) break;

        if (!has_alpha)  /* generate alpha channel from the mask */
        {
            for (i = 0; i < height; i++)
                for (j = n * width; j < (n + 1) * width; j++)
                    if (!mask_info || !((mask_bits[i * mask_stride + j / 8] << (j % 8)) & 0x80))
                        bits[i * stride + j] |= 0xff000000;
                    else
                        bits[i * stride + j] = 0;
        }
        else
        {
            himl->has_alpha[pos + n] = 1;

            if (mask_info && himl->hbmMask)  /* generate the mask from the alpha channel */
            {
                for (i = 0; i < height; i++)
                    for (j = n * width; j < (n + 1) * width; j++)
                        if ((bits[i * stride + j] >> 24) > 25) /* more than 10% alpha */
                            mask_bits[i * mask_stride + j / 8] &= ~(0x80 >> (j % 8));
                        else
                            mask_bits[i * mask_stride + j / 8] |= 0x80 >> (j % 8);
            }
        }
        StretchDIBits( himl->hdcImage, pt.x, pt.y, himl->cx, himl->cy,
                       n * width, 0, width, height, bits, info, DIB_RGB_COLORS, SRCCOPY );
        if (mask_info)
            StretchDIBits( himl->hdcMask, pt.x, pt.y, himl->cx, himl->cy,
                           n * width, 0, width, height, mask_bits, mask_info, DIB_RGB_COLORS, SRCCOPY );
    }
}

/* add images with an alpha channel when the image list is 32 bpp */
static BOOL add_with_alpha( HIMAGELIST himl, HDC hdc, int pos, int count,
                            int width, int height, HBITMAP hbmImage, HBITMAP hbmMask )
{
    BOOL ret = FALSE;
    BITMAP bm;
    BITMAPINFO *info, *mask_info = NULL;
    DWORD *bits = NULL;
    BYTE *mask_bits = NULL;
    DWORD mask_width;

    if (!GetObjectW( hbmImage, sizeof(bm), &bm )) return FALSE;

    /* if either the imagelist or the source bitmap don't have an alpha channel, bail out now */
    if (!himl->has_alpha) return FALSE;
    if (bm.bmBitsPixel != 32) return FALSE;

    SelectObject( hdc, hbmImage );
    mask_width = (bm.bmWidth + 31) / 32 * 4;

    if (!(info = heap_alloc( FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) goto done;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = bm.bmWidth;
    info->bmiHeader.biHeight = -height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = bm.bmWidth * height * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    if (!(bits = heap_alloc( info->bmiHeader.biSizeImage ))) goto done;
    if (!GetDIBits( hdc, hbmImage, 0, height, bits, info, DIB_RGB_COLORS )) goto done;

    if (hbmMask)
    {
        if (!(mask_info = heap_alloc( FIELD_OFFSET( BITMAPINFO, bmiColors[2] ))))
            goto done;
        mask_info->bmiHeader = info->bmiHeader;
        mask_info->bmiHeader.biBitCount = 1;
        mask_info->bmiHeader.biSizeImage = mask_width * height;
        if (!(mask_bits = heap_alloc_zero( mask_info->bmiHeader.biSizeImage )))
            goto done;
        if (!GetDIBits( hdc, hbmMask, 0, height, mask_bits, mask_info, DIB_RGB_COLORS )) goto done;
    }

    add_dib_bits( himl, pos, count, width, height, info, mask_info, bits, mask_bits );
    ret = TRUE;

done:
    heap_free( info );
    heap_free( mask_info );
    heap_free( bits );
    heap_free( mask_bits );
    return ret;
}

UINT WINAPI
ImageList_SetColorTable(HIMAGELIST himl, UINT uStartIndex, UINT cEntries, const RGBQUAD *prgb);

/*************************************************************************
 * IMAGELIST_InternalExpandBitmaps [Internal]
 *
 * Expands the bitmaps of an image list by the given number of images.
 *
 * PARAMS
 *     himl        [I] handle to image list
 *     nImageCount [I] number of images to add
 *
 * RETURNS
 *     nothing
 *
 * NOTES
 *     This function CANNOT be used to reduce the number of images.
 */
static void
IMAGELIST_InternalExpandBitmaps(HIMAGELIST himl, INT nImageCount)
{
    HDC     hdcBitmap;
    HBITMAP hbmNewBitmap, hbmNull;
    INT     nNewCount;
    SIZE    sz;

    TRACE("%p has allocated %d, max %d, grow %d images\n", himl, himl->cCurImage, himl->cMaxImage, himl->cGrow);

    if (himl->cCurImage + nImageCount < himl->cMaxImage)
        return;

    nNewCount = himl->cMaxImage + max(nImageCount, himl->cGrow) + 1;

    imagelist_get_bitmap_size(himl, nNewCount, &sz);

    TRACE("Create expanded bitmaps : himl=%p x=%d y=%d count=%d\n", himl, sz.cx, sz.cy, nNewCount);
    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap = ImageList_CreateImage(hdcBitmap, himl, nNewCount);

    if (hbmNewBitmap == 0)
        ERR("creating new image bitmap (x=%d y=%d)!\n", sz.cx, sz.cy);

    if (himl->cCurImage)
    {
        hbmNull = SelectObject (hdcBitmap, hbmNewBitmap);
        BitBlt (hdcBitmap, 0, 0, sz.cx, sz.cy,
                himl->hdcImage, 0, 0, SRCCOPY);
        SelectObject (hdcBitmap, hbmNull);
    }
    SelectObject (himl->hdcImage, hbmNewBitmap);
    DeleteObject (himl->hbmImage);
    himl->hbmImage = hbmNewBitmap;

    if (himl->flags & ILC_MASK)
    {
        hbmNewBitmap = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);

        if (hbmNewBitmap == 0)
            ERR("creating new mask bitmap!\n");

	if(himl->cCurImage)
	{
	    hbmNull = SelectObject (hdcBitmap, hbmNewBitmap);
	    BitBlt (hdcBitmap, 0, 0, sz.cx, sz.cy,
		    himl->hdcMask, 0, 0, SRCCOPY);
	    SelectObject (hdcBitmap, hbmNull);
	}
        SelectObject (himl->hdcMask, hbmNewBitmap);
        DeleteObject (himl->hbmMask);
        himl->hbmMask = hbmNewBitmap;
    }

    if (himl->has_alpha)
    {
        char *new_alpha = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, himl->has_alpha, nNewCount );
        if (new_alpha) himl->has_alpha = new_alpha;
        else
        {
            heap_free( himl->has_alpha );
            himl->has_alpha = NULL;
        }
    }

    himl->cMaxImage = nNewCount;

    DeleteDC (hdcBitmap);
}


/*************************************************************************
 * ImageList_Add [COMCTL32.@]
 *
 * Add an image or images to an image list.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     hbmImage [I] handle to image bitmap
 *     hbmMask  [I] handle to mask bitmap
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT WINAPI
ImageList_Add (HIMAGELIST himl,	HBITMAP hbmImage, HBITMAP hbmMask)
{
    HDC     hdcBitmap, hdcTemp = 0;
    INT     nFirstIndex, nImageCount, i;
    BITMAP  bmp;
    POINT   pt;

    TRACE("himl=%p hbmimage=%p hbmmask=%p\n", himl, hbmImage, hbmMask);
    if (!is_valid(himl))
        return -1;

    if (!GetObjectW(hbmImage, sizeof(BITMAP), &bmp))
        return -1;

    TRACE("himl %p, cCurImage %d, cMaxImage %d, cGrow %d, cx %d, cy %d\n",
          himl, himl->cCurImage, himl->cMaxImage, himl->cGrow, himl->cx, himl->cy);

    nImageCount = bmp.bmWidth / himl->cx;

    TRACE("%p has %d images (%d x %d) bpp %d\n", hbmImage, nImageCount, bmp.bmWidth, bmp.bmHeight,
          bmp.bmBitsPixel);

    IMAGELIST_InternalExpandBitmaps(himl, nImageCount);

    hdcBitmap = CreateCompatibleDC(0);

    SelectObject(hdcBitmap, hbmImage);

    if (add_with_alpha( himl, hdcBitmap, himl->cCurImage, nImageCount,
                        himl->cx, min( himl->cy, bmp.bmHeight), hbmImage, hbmMask ))
        goto done;

    if (himl->hbmMask)
    {
        hdcTemp = CreateCompatibleDC(0);
        SelectObject(hdcTemp, hbmMask);
    }

    if (himl->uBitsPixel <= 8 && bmp.bmBitsPixel <= 8 &&
        !himl->color_table_set && himl->cCurImage == 0)
    {
        RGBQUAD colors[256];
        UINT num = GetDIBColorTable( hdcBitmap, 0, 1 << bmp.bmBitsPixel, colors );
        if (num) ImageList_SetColorTable( himl, 0, num, colors );
    }

    for (i=0; i<nImageCount; i++)
    {
        imagelist_point_from_index( himl, himl->cCurImage + i, &pt );

        /* Copy result to the imagelist
        */
        BitBlt( himl->hdcImage, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcBitmap, i*himl->cx, 0, SRCCOPY );

        if (!himl->hbmMask)
             continue;

        BitBlt( himl->hdcMask, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcTemp, i*himl->cx, 0, SRCCOPY );

        /* Remove the background from the image
        */
        BitBlt( himl->hdcImage, pt.x, pt.y, himl->cx, bmp.bmHeight,
                himl->hdcMask, pt.x, pt.y, 0x220326 ); /* NOTSRCAND */
    }
    if (hdcTemp) DeleteDC(hdcTemp);

done:
    DeleteDC(hdcBitmap);

    nFirstIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    return nFirstIndex;
}


/*************************************************************************
 * ImageList_AddIcon [COMCTL32.@]
 *
 * Adds an icon to an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     hIcon [I] handle to icon
 *
 * RETURNS
 *     Success: index of the new image
 *     Failure: -1
 */
#undef ImageList_AddIcon
INT WINAPI ImageList_AddIcon (HIMAGELIST himl, HICON hIcon)
{
    return ImageList_ReplaceIcon (himl, -1, hIcon);
}


/*************************************************************************
 * ImageList_AddMasked [COMCTL32.@]
 *
 * Adds an image or images to an image list and creates a mask from the
 * specified bitmap using the mask color.
 *
 * PARAMS
 *     himl    [I] handle to image list.
 *     hBitmap [I] handle to bitmap
 *     clrMask [I] mask color.
 *
 * RETURNS
 *     Success: Index of the first new image.
 *     Failure: -1
 */

INT WINAPI
ImageList_AddMasked (HIMAGELIST himl, HBITMAP hBitmap, COLORREF clrMask)
{
    HDC    hdcMask, hdcBitmap;
    INT    ret;
    BITMAP bmp;
    HBITMAP hMaskBitmap;
    COLORREF bkColor;

    TRACE("himl=%p hbitmap=%p clrmask=%x\n", himl, hBitmap, clrMask);
    if (!is_valid(himl))
        return -1;

    if (!GetObjectW(hBitmap, sizeof(BITMAP), &bmp))
        return -1;

    hdcBitmap = CreateCompatibleDC(0);
    SelectObject(hdcBitmap, hBitmap);

    /* Create a temp Mask so we can remove the background of the Image */
    hdcMask = CreateCompatibleDC(0);
    hMaskBitmap = CreateBitmap(bmp.bmWidth, bmp.bmHeight, 1, 1, NULL);
    SelectObject(hdcMask, hMaskBitmap);

    /* create monochrome image to the mask bitmap */
    bkColor = (clrMask != CLR_DEFAULT) ? clrMask : GetPixel (hdcBitmap, 0, 0);
    SetBkColor (hdcBitmap, bkColor);
    BitBlt (hdcMask, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcBitmap, 0, 0, SRCCOPY);

    /*
     * Remove the background from the image
     *
     * WINDOWS BUG ALERT!!!!!!
     *  The statement below should not be done in common practice
     *  but this is how ImageList_AddMasked works in Windows.
     *  It overwrites the original bitmap passed, this was discovered
     *  by using the same bitmap to iterate the different styles
     *  on windows where it failed (BUT ImageList_Add is OK)
     *  This is here in case some apps rely on this bug
     *
     *  Blt mode 0x220326 is NOTSRCAND
     */
    if (bmp.bmBitsPixel > 8)  /* NOTSRCAND can't work with palettes */
    {
        SetBkColor(hdcBitmap, RGB(255,255,255));
        BitBlt(hdcBitmap, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMask, 0, 0, 0x220326);
    }

    DeleteDC(hdcBitmap);
    DeleteDC(hdcMask);

    ret = ImageList_Add( himl, hBitmap, hMaskBitmap );

    DeleteObject(hMaskBitmap);
    return ret;
}


/*************************************************************************
 * ImageList_BeginDrag [COMCTL32.@]
 *
 * Creates a temporary image list that contains one image. It will be used
 * as a drag image.
 *
 * PARAMS
 *     himlTrack [I] handle to the source image list
 *     iTrack    [I] index of the drag image in the source image list
 *     dxHotspot [I] X position of the hot spot of the drag image
 *     dyHotspot [I] Y position of the hot spot of the drag image
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_BeginDrag (HIMAGELIST himlTrack, INT iTrack,
	             INT dxHotspot, INT dyHotspot)
{
    INT cx, cy;
    POINT src, dst;

    TRACE("(himlTrack=%p iTrack=%d dx=%d dy=%d)\n", himlTrack, iTrack,
	  dxHotspot, dyHotspot);

    if (!is_valid(himlTrack))
	return FALSE;

    if (iTrack >= himlTrack->cCurImage)
        return FALSE;

    if (InternalDrag.himl)
        return FALSE;

    cx = himlTrack->cx;
    cy = himlTrack->cy;

    InternalDrag.himlNoCursor = InternalDrag.himl = ImageList_Create (cx, cy, himlTrack->flags, 1, 1);
    if (InternalDrag.himl == NULL) {
        WARN("Error creating drag image list!\n");
        return FALSE;
    }

    InternalDrag.dxHotspot = dxHotspot;
    InternalDrag.dyHotspot = dyHotspot;

    /* copy image */
    imagelist_point_from_index(InternalDrag.himl, 0, &dst);
    imagelist_point_from_index(himlTrack, iTrack, &src);
    BitBlt(InternalDrag.himl->hdcImage, dst.x, dst.y, cx, cy, himlTrack->hdcImage, src.x, src.y,
            SRCCOPY);
    BitBlt(InternalDrag.himl->hdcMask, dst.x, dst.y, cx, cy, himlTrack->hdcMask, src.x, src.y,
            SRCCOPY);

    InternalDrag.himl->cCurImage = 1;

    return TRUE;
}


/*************************************************************************
 * ImageList_Copy [COMCTL32.@]
 *
 *  Copies an image of the source image list to an image of the
 *  destination image list. Images can be copied or swapped.
 *
 * PARAMS
 *     himlDst [I] handle to the destination image list
 *     iDst    [I] destination image index.
 *     himlSrc [I] handle to the source image list
 *     iSrc    [I] source image index
 *     uFlags  [I] flags for the copy operation
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Copying from one image list to another is possible. The original
 *     implementation just copies or swaps within one image list.
 *     Could this feature become a bug??? ;-)
 */

BOOL WINAPI
ImageList_Copy (HIMAGELIST himlDst, INT iDst,	HIMAGELIST himlSrc,
		INT iSrc, UINT uFlags)
{
    POINT ptSrc, ptDst;

    TRACE("himlDst=%p iDst=%d himlSrc=%p iSrc=%d\n", himlDst, iDst, himlSrc, iSrc);

    if (!is_valid(himlSrc) || !is_valid(himlDst))
	return FALSE;
    if ((iDst < 0) || (iDst >= himlDst->cCurImage))
	return FALSE;
    if ((iSrc < 0) || (iSrc >= himlSrc->cCurImage))
	return FALSE;

    imagelist_point_from_index( himlDst, iDst, &ptDst );
    imagelist_point_from_index( himlSrc, iSrc, &ptSrc );

    if (uFlags & ILCF_SWAP) {
        /* swap */
        HDC     hdcBmp;
        HBITMAP hbmTempImage, hbmTempMask;

        hdcBmp = CreateCompatibleDC (0);

        /* create temporary bitmaps */
        hbmTempImage = CreateBitmap (himlSrc->cx, himlSrc->cy, 1,
                                       himlSrc->uBitsPixel, NULL);
        hbmTempMask = CreateBitmap (himlSrc->cx, himlSrc->cy, 1,
				      1, NULL);

        /* copy (and stretch) destination to temporary bitmaps.(save) */
        /* image */
        SelectObject (hdcBmp, hbmTempImage);
        StretchBlt   (hdcBmp, 0, 0, himlSrc->cx, himlSrc->cy,
                      himlDst->hdcImage, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      SRCCOPY);
        /* mask */
        SelectObject (hdcBmp, hbmTempMask);
        StretchBlt   (hdcBmp, 0, 0, himlSrc->cx, himlSrc->cy,
                      himlDst->hdcMask, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      SRCCOPY);

        /* copy (and stretch) source to destination */
        /* image */
        StretchBlt   (himlDst->hdcImage, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      himlSrc->hdcImage, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
        /* mask */
        StretchBlt   (himlDst->hdcMask, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      himlSrc->hdcMask, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy (without stretching) temporary bitmaps to source (restore) */
        /* mask */
        BitBlt       (himlSrc->hdcMask, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      hdcBmp, 0, 0, SRCCOPY);

        /* image */
        BitBlt       (himlSrc->hdcImage, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      hdcBmp, 0, 0, SRCCOPY);
        /* delete temporary bitmaps */
        DeleteObject (hbmTempMask);
        DeleteObject (hbmTempImage);
        DeleteDC(hdcBmp);
    }
    else {
        /* copy image */
        StretchBlt   (himlDst->hdcImage, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      himlSrc->hdcImage, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);

        /* copy mask */
        StretchBlt   (himlDst->hdcMask, ptDst.x, ptDst.y, himlDst->cx, himlDst->cy,
                      himlSrc->hdcMask, ptSrc.x, ptSrc.y, himlSrc->cx, himlSrc->cy,
                      SRCCOPY);
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_Create [COMCTL32.@]
 *
 * Creates a new image list.
 *
 * PARAMS
 *     cx       [I] image height
 *     cy       [I] image width
 *     flags    [I] creation flags
 *     cInitial [I] initial number of images in the image list
 *     cGrow    [I] number of images by which image list grows
 *
 * RETURNS
 *     Success: Handle to the created image list
 *     Failure: NULL
 */
HIMAGELIST WINAPI
ImageList_Create (INT cx, INT cy, UINT flags,
		  INT cInitial, INT cGrow)
{
    HIMAGELIST himl;
    INT      nCount;
    HBITMAP  hbmTemp;
    UINT     ilc = (flags & 0xFE);
    static const WORD aBitBlend25[] =
        {0xAA, 0x00, 0x55, 0x00, 0xAA, 0x00, 0x55, 0x00};

    static const WORD aBitBlend50[] =
        {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};

    TRACE("(%d %d 0x%x %d %d)\n", cx, cy, flags, cInitial, cGrow);

    if (cx < 0 || cy < 0) return NULL;
    if (!((flags&ILC_COLORDDB) == ILC_COLORDDB) && (cx == 0 || cy == 0)) return NULL;

    /* Create the IImageList interface for the image list */
    if (FAILED(ImageListImpl_CreateInstance(NULL, &IID_IImageList, (void **)&himl)))
        return NULL;

    cGrow = (WORD)((max( cGrow, 1 ) + 3) & ~3);

    if (cGrow > 256)
    {
        /* Windows doesn't limit the size here, but X11 doesn't let us allocate such huge bitmaps */
        WARN( "grow %d too large, limiting to 256\n", cGrow );
        cGrow = 256;
    }

    himl->cx        = cx;
    himl->cy        = cy;
    himl->flags     = flags;
    himl->cMaxImage = cInitial + 1;
    himl->cInitial  = cInitial;
    himl->cGrow     = cGrow;
    himl->clrFg     = CLR_DEFAULT;
    himl->clrBk     = CLR_NONE;
    himl->color_table_set = FALSE;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    /* Create Image & Mask DCs */
    himl->hdcImage = CreateCompatibleDC (0);
    if (!himl->hdcImage)
        goto cleanup;
    if (himl->flags & ILC_MASK){
        himl->hdcMask = CreateCompatibleDC(0);
        if (!himl->hdcMask)
            goto cleanup;
    }

    /* Default to ILC_COLOR4 if none of the ILC_COLOR* flags are specified */
    if (ilc == ILC_COLOR)
    {
        ilc = ILC_COLOR4;
        himl->flags |= ILC_COLOR4;
    }

    if (ilc >= ILC_COLOR4 && ilc <= ILC_COLOR32)
        himl->uBitsPixel = ilc;
    else
        himl->uBitsPixel = (UINT)GetDeviceCaps (himl->hdcImage, BITSPIXEL);

    if (himl->cMaxImage > 0) {
        himl->hbmImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage);
	SelectObject(himl->hdcImage, himl->hbmImage);
    } else
        himl->hbmImage = 0;

    if ((himl->cMaxImage > 0) && (himl->flags & ILC_MASK)) {
        SIZE sz;

        imagelist_get_bitmap_size(himl, himl->cMaxImage, &sz);
        himl->hbmMask = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
        if (himl->hbmMask == 0) {
            ERR("Error creating mask bitmap!\n");
            goto cleanup;
        }
        SelectObject(himl->hdcMask, himl->hbmMask);
    }
    else
        himl->hbmMask = 0;

    if (ilc == ILC_COLOR32)
        himl->has_alpha = heap_alloc_zero( himl->cMaxImage );
    else
        himl->has_alpha = NULL;

    /* create blending brushes */
    hbmTemp = CreateBitmap (8, 8, 1, 1, aBitBlend25);
    himl->hbrBlend25 = CreatePatternBrush (hbmTemp);
    DeleteObject (hbmTemp);

    hbmTemp = CreateBitmap (8, 8, 1, 1, aBitBlend50);
    himl->hbrBlend50 = CreatePatternBrush (hbmTemp);
    DeleteObject (hbmTemp);

    TRACE("created imagelist %p\n", himl);
    return himl;

cleanup:
    ImageList_Destroy(himl);
    return NULL;
}


/*************************************************************************
 * ImageList_Destroy [COMCTL32.@]
 *
 * Destroys an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_Destroy (HIMAGELIST himl)
{
    if (!is_valid(himl))
	return FALSE;

    IImageList_Release((IImageList *) himl);
    return TRUE;
}


/*************************************************************************
 * ImageList_DragEnter [COMCTL32.@]
 *
 * Locks window update and displays the drag image at the given position.
 *
 * PARAMS
 *     hwndLock [I] handle of the window that owns the drag image.
 *     x        [I] X position of the drag image.
 *     y        [I] Y position of the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The position of the drag image is relative to the window, not
 *     the client area.
 */

BOOL WINAPI
ImageList_DragEnter (HWND hwndLock, INT x, INT y)
{
    TRACE("(hwnd=%p x=%d y=%d)\n", hwndLock, x, y);

    if (!is_valid(InternalDrag.himl))
	return FALSE;

    if (hwndLock)
	InternalDrag.hwnd = hwndLock;
    else
	InternalDrag.hwnd = GetDesktopWindow ();

    InternalDrag.x = x;
    InternalDrag.y = y;

    /* draw the drag image and save the background */
    return ImageList_DragShowNolock(TRUE);
}


/*************************************************************************
 * ImageList_DragLeave [COMCTL32.@]
 *
 * Unlocks window update and hides the drag image.
 *
 * PARAMS
 *     hwndLock [I] handle of the window that owns the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_DragLeave (HWND hwndLock)
{
    /* As we don't save drag info in the window this can lead to problems if
       an app does not supply the same window as DragEnter */
    /* if (hwndLock)
	InternalDrag.hwnd = hwndLock;
    else
	InternalDrag.hwnd = GetDesktopWindow (); */
    if(!hwndLock)
	hwndLock = GetDesktopWindow();
    if(InternalDrag.hwnd != hwndLock)
	FIXME("DragLeave hWnd != DragEnter hWnd\n");

    ImageList_DragShowNolock (FALSE);

    return TRUE;
}


/*************************************************************************
 * ImageList_InternalDragDraw [Internal]
 *
 * Draws the drag image.
 *
 * PARAMS
 *     hdc [I] device context to draw into.
 *     x   [I] X position of the drag image.
 *     y   [I] Y position of the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The position of the drag image is relative to the window, not
 *     the client area.
 *
 */

static inline void
ImageList_InternalDragDraw (HDC hdc, INT x, INT y)
{
    IMAGELISTDRAWPARAMS imldp;

    ZeroMemory (&imldp, sizeof(imldp));
    imldp.cbSize  = sizeof(imldp);
    imldp.himl    = InternalDrag.himl;
    imldp.i       = 0;
    imldp.hdcDst  = hdc;
    imldp.x       = x;
    imldp.y       = y;
    imldp.rgbBk   = CLR_DEFAULT;
    imldp.rgbFg   = CLR_DEFAULT;
    imldp.fStyle  = ILD_NORMAL;
    imldp.fState  = ILS_ALPHA;
    imldp.Frame   = 192;
    ImageList_DrawIndirect (&imldp);
}

/*************************************************************************
 * ImageList_DragMove [COMCTL32.@]
 *
 * Moves the drag image.
 *
 * PARAMS
 *     x [I] X position of the drag image.
 *     y [I] Y position of the drag image.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The position of the drag image is relative to the window, not
 *     the client area.
 */

BOOL WINAPI
ImageList_DragMove (INT x, INT y)
{
    TRACE("(x=%d y=%d)\n", x, y);

    if (!is_valid(InternalDrag.himl))
	return FALSE;

    /* draw/update the drag image */
    if (InternalDrag.bShow) {
	HDC hdcDrag;
	HDC hdcOffScreen;
	HDC hdcBg;
	HBITMAP hbmOffScreen;
	INT origNewX, origNewY;
	INT origOldX, origOldY;
	INT origRegX, origRegY;
	INT sizeRegX, sizeRegY;


	/* calculate the update region */
	origNewX = x - InternalDrag.dxHotspot;
	origNewY = y - InternalDrag.dyHotspot;
	origOldX = InternalDrag.x - InternalDrag.dxHotspot;
	origOldY = InternalDrag.y - InternalDrag.dyHotspot;
	origRegX = min(origNewX, origOldX);
	origRegY = min(origNewY, origOldY);
	sizeRegX = InternalDrag.himl->cx + abs(x - InternalDrag.x);
	sizeRegY = InternalDrag.himl->cy + abs(y - InternalDrag.y);

	hdcDrag = GetDCEx(InternalDrag.hwnd, 0,
   			  DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);
    	hdcOffScreen = CreateCompatibleDC(hdcDrag);
    	hdcBg = CreateCompatibleDC(hdcDrag);

	hbmOffScreen = CreateCompatibleBitmap(hdcDrag, sizeRegX, sizeRegY);
	SelectObject(hdcOffScreen, hbmOffScreen);
	SelectObject(hdcBg, InternalDrag.hbmBg);

	/* get the actual background of the update region */
	BitBlt(hdcOffScreen, 0, 0, sizeRegX, sizeRegY, hdcDrag,
	       origRegX, origRegY, SRCCOPY);
	/* erase the old image */
	BitBlt(hdcOffScreen, origOldX - origRegX, origOldY - origRegY,
	       InternalDrag.himl->cx, InternalDrag.himl->cy, hdcBg, 0, 0,
	       SRCCOPY);
	/* save the background */
	BitBlt(hdcBg, 0, 0, InternalDrag.himl->cx, InternalDrag.himl->cy,
	       hdcOffScreen, origNewX - origRegX, origNewY - origRegY, SRCCOPY);
	/* draw the image */
	ImageList_InternalDragDraw(hdcOffScreen, origNewX - origRegX, 
				   origNewY - origRegY);
	/* draw the update region to the screen */
	BitBlt(hdcDrag, origRegX, origRegY, sizeRegX, sizeRegY,
	       hdcOffScreen, 0, 0, SRCCOPY);

	DeleteDC(hdcBg);
	DeleteDC(hdcOffScreen);
	DeleteObject(hbmOffScreen);
	ReleaseDC(InternalDrag.hwnd, hdcDrag);
    }

    /* update the image position */
    InternalDrag.x = x;
    InternalDrag.y = y;

    return TRUE;
}


/*************************************************************************
 * ImageList_DragShowNolock [COMCTL32.@]
 *
 * Shows or hides the drag image.
 *
 * PARAMS
 *     bShow [I] TRUE shows the drag image, FALSE hides it.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_DragShowNolock (BOOL bShow)
{
    HDC hdcDrag;
    HDC hdcBg;
    INT x, y;

    if (!is_valid(InternalDrag.himl))
        return FALSE;
    
    TRACE("bShow=0x%X!\n", bShow);

    /* DragImage is already visible/hidden */
    if ((InternalDrag.bShow && bShow) || (!InternalDrag.bShow && !bShow)) {
	return FALSE;
    }

    /* position of the origin of the DragImage */
    x = InternalDrag.x - InternalDrag.dxHotspot;
    y = InternalDrag.y - InternalDrag.dyHotspot;

    hdcDrag = GetDCEx (InternalDrag.hwnd, 0,
			 DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);
    if (!hdcDrag) {
	return FALSE;
    }

    hdcBg = CreateCompatibleDC(hdcDrag);
    if (!InternalDrag.hbmBg) {
	InternalDrag.hbmBg = CreateCompatibleBitmap(hdcDrag,
		    InternalDrag.himl->cx, InternalDrag.himl->cy);
    }
    SelectObject(hdcBg, InternalDrag.hbmBg);

    if (bShow) {
	/* save the background */
	BitBlt(hdcBg, 0, 0, InternalDrag.himl->cx, InternalDrag.himl->cy,
	       hdcDrag, x, y, SRCCOPY);
	/* show the image */
	ImageList_InternalDragDraw(hdcDrag, x, y);
    } else {
	/* hide the image */
	BitBlt(hdcDrag, x, y, InternalDrag.himl->cx, InternalDrag.himl->cy,
	       hdcBg, 0, 0, SRCCOPY);
    }

    InternalDrag.bShow = !InternalDrag.bShow;

    DeleteDC(hdcBg);
    ReleaseDC (InternalDrag.hwnd, hdcDrag);
    return TRUE;
}


/*************************************************************************
 * ImageList_Draw [COMCTL32.@]
 *
 * Draws an image.
 *
 * PARAMS
 *     himl   [I] handle to image list
 *     i      [I] image index
 *     hdc    [I] handle to device context
 *     x      [I] x position
 *     y      [I] y position
 *     fStyle [I] drawing flags
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * SEE
 *     ImageList_DrawEx.
 */

BOOL WINAPI
ImageList_Draw (HIMAGELIST himl, INT i, HDC hdc, INT x, INT y, UINT fStyle)
{
    return ImageList_DrawEx (himl, i, hdc, x, y, 0, 0, 
		             CLR_DEFAULT, CLR_DEFAULT, fStyle);
}


/*************************************************************************
 * ImageList_DrawEx [COMCTL32.@]
 *
 * Draws an image and allows using extended drawing features.
 *
 * PARAMS
 *     himl   [I] handle to image list
 *     i      [I] image index
 *     hdc    [I] handle to device context
 *     x      [I] X position
 *     y      [I] Y position
 *     dx     [I] X offset
 *     dy     [I] Y offset
 *     rgbBk  [I] background color
 *     rgbFg  [I] foreground color
 *     fStyle [I] drawing flags
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Calls ImageList_DrawIndirect.
 *
 * SEE
 *     ImageList_DrawIndirect.
 */

BOOL WINAPI
ImageList_DrawEx (HIMAGELIST himl, INT i, HDC hdc, INT x, INT y,
		  INT dx, INT dy, COLORREF rgbBk, COLORREF rgbFg,
		  UINT fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    ZeroMemory (&imldp, sizeof(imldp));
    imldp.cbSize  = sizeof(imldp);
    imldp.himl    = himl;
    imldp.i       = i;
    imldp.hdcDst  = hdc;
    imldp.x       = x;
    imldp.y       = y;
    imldp.cx      = dx;
    imldp.cy      = dy;
    imldp.rgbBk   = rgbBk;
    imldp.rgbFg   = rgbFg;
    imldp.fStyle  = fStyle;

    return ImageList_DrawIndirect (&imldp);
}

#ifdef __REACTOS__
static BOOL alpha_blend_image( HIMAGELIST himl, HDC srce_dc, HDC srce_dcMask, HDC dest_dc, int dest_x, int dest_y,
#else
static BOOL alpha_blend_image( HIMAGELIST himl, HDC dest_dc, int dest_x, int dest_y,
#endif
                               int src_x, int src_y, int cx, int cy, BLENDFUNCTION func,
                               UINT style, COLORREF blend_col )
{
    BOOL ret = FALSE;
    HDC hdc;
    HBITMAP bmp = 0, mask = 0;
    BITMAPINFO *info;
    void *bits, *mask_bits;
    unsigned int *ptr;
    int i, j;

    if (!(hdc = CreateCompatibleDC( 0 ))) return FALSE;
    if (!(info = heap_alloc( FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) goto done;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = cx;
    info->bmiHeader.biHeight = cy;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = cx * cy * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
#ifdef __REACTOS__
    if (!(bmp = CreateDIBSection( srce_dc, info, DIB_RGB_COLORS, &bits, 0, 0 ))) goto done;
#else
    if (!(bmp = CreateDIBSection( himl->hdcImage, info, DIB_RGB_COLORS, &bits, 0, 0 ))) goto done;
#endif
    SelectObject( hdc, bmp );
#ifdef __REACTOS__
    if (!BitBlt(hdc, 0, 0, cx, cy, srce_dc, src_x, src_y, SRCCOPY))
    {
        TRACE("BitBlt failed\n");
        goto done;
    }
#else
    BitBlt( hdc, 0, 0, cx, cy, himl->hdcImage, src_x, src_y, SRCCOPY );
#endif

    if (blend_col != CLR_NONE)
    {
        BYTE r = GetRValue( blend_col );
        BYTE g = GetGValue( blend_col );
        BYTE b = GetBValue( blend_col );

        if (style & ILD_BLEND25)
        {
            for (i = 0, ptr = bits; i < cx * cy; i++, ptr++)
                *ptr = ((*ptr & 0xff000000) |
                        ((((*ptr & 0x00ff0000) * 3 + (r << 16)) / 4) & 0x00ff0000) |
                        ((((*ptr & 0x0000ff00) * 3 + (g << 8))  / 4) & 0x0000ff00) |
                        ((((*ptr & 0x000000ff) * 3 + (b << 0))  / 4) & 0x000000ff));
        }
        else if (style & ILD_BLEND50)
        {
            for (i = 0, ptr = bits; i < cx * cy; i++, ptr++)
                *ptr = ((*ptr & 0xff000000) |
                        ((((*ptr & 0x00ff0000) + (r << 16)) / 2) & 0x00ff0000) |
                        ((((*ptr & 0x0000ff00) + (g << 8))  / 2) & 0x0000ff00) |
                        ((((*ptr & 0x000000ff) + (b << 0))  / 2) & 0x000000ff));
        }
    }

    if (himl->has_alpha)  /* we already have an alpha channel in this case */
    {
        /* pre-multiply by the alpha channel */
        for (i = 0, ptr = bits; i < cx * cy; i++, ptr++)
        {
            DWORD alpha = *ptr >> 24;
            *ptr = ((*ptr & 0xff000000) |
                    (((*ptr & 0x00ff0000) * alpha / 255) & 0x00ff0000) |
                    (((*ptr & 0x0000ff00) * alpha / 255) & 0x0000ff00) |
                    (((*ptr & 0x000000ff) * alpha / 255)));
        }
    }
    else if (himl->hbmMask)
    {
        unsigned int width_bytes = (cx + 31) / 32 * 4;
        /* generate alpha channel from the mask */
        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * cy;
        info->bmiColors[0].rgbRed      = 0;
        info->bmiColors[0].rgbGreen    = 0;
        info->bmiColors[0].rgbBlue     = 0;
        info->bmiColors[0].rgbReserved = 0;
        info->bmiColors[1].rgbRed      = 0xff;
        info->bmiColors[1].rgbGreen    = 0xff;
        info->bmiColors[1].rgbBlue     = 0xff;
        info->bmiColors[1].rgbReserved = 0;
        if (!(mask = CreateDIBSection( srce_dcMask, info, DIB_RGB_COLORS, &mask_bits, 0, 0)))
        {
            TRACE("CreateDIBSection failed %i\n", GetLastError());
            goto done;
        }
        if (SelectObject(hdc, mask) == NULL)
        {
            TRACE("SelectObject failed %i\n", GetLastError());
            SelectObject(hdc, bmp);
            goto done;
        }
        if (!BitBlt( hdc, 0, 0, cx, cy, srce_dcMask, src_x, src_y, SRCCOPY))
        {
            TRACE("BitBlt failed %i\n", GetLastError());
            SelectObject(hdc, bmp);
            goto done;
        }
        if (SelectObject( hdc, bmp) == NULL)
        {
            TRACE("SelectObject failed %i\n", GetLastError());
            goto done;
        }
        for (i = 0, ptr = bits; i < cy; i++)
            for (j = 0; j < cx; j++, ptr++)
                if ((((BYTE *)mask_bits)[i * width_bytes + j / 8] << (j % 8)) & 0x80) *ptr = 0;
                else *ptr |= 0xff000000;
    }

    ret = GdiAlphaBlend( dest_dc, dest_x, dest_y, cx, cy, hdc, 0, 0, cx, cy, func );

done:
    DeleteDC( hdc );
    if (bmp) DeleteObject( bmp );
    if (mask) DeleteObject( mask );
    heap_free( info );
    return ret;
}

#ifdef __REACTOS__
BOOL saturate_image(HIMAGELIST himl, HDC dest_dc, int dest_x, int dest_y,
                    int src_x, int src_y, int cx, int cy, COLORREF rgbFg,
                    HDC *hdcImageListDC, HDC *hdcMaskListDC)
{
    HDC hdc = NULL, hdcMask = NULL;
    HBITMAP bmp = 0, bmpMask = 0;
    BITMAPINFO *info;

    unsigned int *ptr;
    void *bits;
    int i;

    /* create a dc and its device independent bitmap for doing the work,
       shamelessly copied from the alpha-blending function above */
    if (!(hdc = CreateCompatibleDC( 0 ))) return FALSE;
    if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) goto done;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = cx;
    info->bmiHeader.biHeight = cy;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = cx * cy * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    if (!(bmp = CreateDIBSection(himl->hdcImage, info, DIB_RGB_COLORS, &bits, 0, 0 ))) goto done;

    /* bind both surfaces */
    if (SelectObject(hdc, bmp) == NULL)
    {
        TRACE("SelectObject failed\n");
        goto done;
    }

    /* copy into our dc the section that covers just the icon we we're asked for */
    if (!BitBlt(hdc, 0, 0, cx, cy, himl->hdcImage, src_x, src_y, SRCCOPY))
    {
        TRACE("BitBlt failed!\n");
        goto done;
    }

    /* loop every pixel of the bitmap */
    for (i = 0, ptr = bits; i < cx * cy; i++, ptr++)
    {
        COLORREF orig_color = *ptr;

        /* calculate the effective luminance using the constants from here, adapted to the human eye:
           <http://bobpowell.net/grayscale.aspx> */
        float mixed_color = (GetRValue(orig_color) * .30 +
                             GetGValue(orig_color) * .59 +
                             GetBValue(orig_color) * .11);

        *ptr = RGBA(mixed_color, mixed_color, mixed_color, GetAValue(orig_color));
    }

    if (himl->hdcMask)
    {
        hdcMask = CreateCompatibleDC(NULL);
        bmpMask = CreateCompatibleBitmap(hdcMask, cx, cy);

        SelectObject(hdcMask, bmpMask);

        if (!BitBlt(hdcMask, 0, 0, cx, cy, himl->hdcMask, src_x, src_y, SRCCOPY))
        {
            ERR("BitBlt failed %i\n", GetLastError());
            DeleteDC(hdcMask);
            hdcMask = NULL;
            goto done;
        }
        TRACE("mask ok\n");
    }

done:

    if (bmp)
        DeleteObject(bmp);
    if (bmpMask)
        DeleteObject(bmpMask);

    if (info)
        HeapFree(GetProcessHeap(), 0, info);

    /* return the handle to our desaturated dc, that will substitute its original counterpart in the next calls */
    *hdcMaskListDC = hdcMask;
    *hdcImageListDC = hdc;
    return (hdc != NULL);
}
#endif /* __REACTOS__ */

/*************************************************************************
 * ImageList_DrawIndirect [COMCTL32.@]
 *
 * Draws an image using various parameters specified in pimldp.
 *
 * PARAMS
 *     pimldp [I] pointer to IMAGELISTDRAWPARAMS structure.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp)
{
    INT cx, cy, nOvlIdx;
    DWORD fState, dwRop;
    UINT fStyle;
    COLORREF oldImageBk, oldImageFg;
    HDC hImageDC, hImageListDC, hMaskListDC;
    HBITMAP hImageBmp, hOldImageBmp, hBlendMaskBmp;
    BOOL bIsTransparent, bBlend, bResult = FALSE, bMask;
    HIMAGELIST himl;
    HBRUSH hOldBrush;
    POINT pt;
    BOOL has_alpha;
#ifdef __REACTOS__
    HDC hdcSaturated = NULL, hdcSaturatedMask = NULL;
#endif

    if (!pimldp || !(himl = pimldp->himl)) return FALSE;
    if (!is_valid(himl)) return FALSE;
    if ((pimldp->i < 0) || (pimldp->i >= himl->cCurImage)) return FALSE;

    imagelist_point_from_index( himl, pimldp->i, &pt );
    pt.x += pimldp->xBitmap;
    pt.y += pimldp->yBitmap;

    fState = pimldp->cbSize < sizeof(IMAGELISTDRAWPARAMS) ? ILS_NORMAL : pimldp->fState;
    fStyle = pimldp->fStyle & ~ILD_OVERLAYMASK;
    cx = (pimldp->cx == 0) ? himl->cx : pimldp->cx;
    cy = (pimldp->cy == 0) ? himl->cy : pimldp->cy;

    bIsTransparent = (fStyle & ILD_TRANSPARENT);
    if( pimldp->rgbBk == CLR_NONE )
        bIsTransparent = TRUE;
    if( ( pimldp->rgbBk == CLR_DEFAULT ) && ( himl->clrBk == CLR_NONE ) )
        bIsTransparent = TRUE;
    bMask = (himl->flags & ILC_MASK) && (fStyle & ILD_MASK) ;
    bBlend = (fStyle & (ILD_BLEND25 | ILD_BLEND50) ) && !bMask;

    TRACE("himl(%p) hbmMask(%p) iImage(%d) x(%d) y(%d) cx(%d) cy(%d)\n",
          himl, himl->hbmMask, pimldp->i, pimldp->x, pimldp->y, cx, cy);

    /* we will use these DCs to access the images and masks in the ImageList */
    hImageListDC = himl->hdcImage;
    hMaskListDC  = himl->hdcMask;

    /* these will accumulate the image and mask for the image we're drawing */
    hImageDC = CreateCompatibleDC( pimldp->hdcDst );
    hImageBmp = CreateCompatibleBitmap( pimldp->hdcDst, cx, cy );
    hBlendMaskBmp = bBlend ? CreateBitmap(cx, cy, 1, 1, NULL) : 0;

    /* Create a compatible DC. */
    if (!hImageListDC || !hImageDC || !hImageBmp ||
	(bBlend && !hBlendMaskBmp) || (himl->hbmMask && !hMaskListDC))
	goto cleanup;
    
    hOldImageBmp = SelectObject(hImageDC, hImageBmp);
  
    /*
     * To obtain a transparent look, background color should be set
     * to white and foreground color to black when blitting the
     * monochrome mask.
     */
    oldImageFg = SetTextColor( hImageDC, RGB( 0, 0, 0 ) );
    oldImageBk = SetBkColor( hImageDC, RGB( 0xff, 0xff, 0xff ) );

#ifdef __REACTOS__
    /*
     * If the ILS_SATURATE bit is enabled we should multiply the
     * RGB colors of the original image by the contents of rgbFg.
     */
    if (fState & ILS_SATURATE)
    {
        if (saturate_image(himl, pimldp->hdcDst, pimldp->x, pimldp->y,
                           pt.x, pt.y, cx, cy, pimldp->rgbFg,
                           &hdcSaturated, &hdcSaturatedMask))
        {
            hImageListDC = hdcSaturated;
            hMaskListDC = hdcSaturatedMask;
            /* shitty way of getting subroutines to blit at the right place (top left corner),
               as our modified imagelist only contains a single image for performance reasons */
            pt.x = 0;
            pt.y = 0;
        }
    }
#endif

    has_alpha = (himl->has_alpha && himl->has_alpha[pimldp->i]);
    if (!bMask && (has_alpha || (fState & ILS_ALPHA)))
    {
        COLORREF colour, blend_col = CLR_NONE;
        BLENDFUNCTION func;

        if (bBlend)
        {
            blend_col = pimldp->rgbFg;
            if (blend_col == CLR_DEFAULT) blend_col = GetSysColor( COLOR_HIGHLIGHT );
            else if (blend_col == CLR_NONE) blend_col = GetTextColor( pimldp->hdcDst );
        }

        func.BlendOp = AC_SRC_OVER;
        func.BlendFlags = 0;
        func.SourceConstantAlpha = (fState & ILS_ALPHA) ? pimldp->Frame : 255;
        func.AlphaFormat = AC_SRC_ALPHA;

        if (bIsTransparent)
        {
#ifdef __REACTOS__
            bResult = alpha_blend_image( himl, hImageListDC, hMaskListDC, pimldp->hdcDst, pimldp->x, pimldp->y,
#else
            bResult = alpha_blend_image( himl, pimldp->hdcDst, pimldp->x, pimldp->y,
#endif
                                         pt.x, pt.y, cx, cy, func, fStyle, blend_col );
            goto end;
        }
        colour = pimldp->rgbBk;
        if (colour == CLR_DEFAULT) colour = himl->clrBk;
        if (colour == CLR_NONE) colour = GetBkColor( pimldp->hdcDst );

        hOldBrush = SelectObject (hImageDC, CreateSolidBrush (colour));
        PatBlt( hImageDC, 0, 0, cx, cy, PATCOPY );
#ifdef __REACTOS__
        alpha_blend_image( himl, hImageListDC, hMaskListDC, hImageDC, 0, 0, pt.x, pt.y, cx, cy, func, fStyle, blend_col );
#else
        alpha_blend_image( himl, hImageDC, 0, 0, pt.x, pt.y, cx, cy, func, fStyle, blend_col );
#endif
        DeleteObject (SelectObject (hImageDC, hOldBrush));
        bResult = BitBlt( pimldp->hdcDst, pimldp->x,  pimldp->y, cx, cy, hImageDC, 0, 0, SRCCOPY );
        goto end;
    }

    /*
     * Draw the initial image
     */
    if( bMask ) {
	if (himl->hbmMask) {
            hOldBrush = SelectObject (hImageDC, CreateSolidBrush (GetTextColor(pimldp->hdcDst)));
            PatBlt( hImageDC, 0, 0, cx, cy, PATCOPY );
            BitBlt(hImageDC, 0, 0, cx, cy, hMaskListDC, pt.x, pt.y, SRCPAINT);
            DeleteObject (SelectObject (hImageDC, hOldBrush));
            if( bIsTransparent )
            {
                BitBlt ( pimldp->hdcDst, pimldp->x,  pimldp->y, cx, cy, hImageDC, 0, 0, SRCAND);
                bResult = TRUE;
                goto end;
            }
	} else {
	    hOldBrush = SelectObject (hImageDC, GetStockObject(BLACK_BRUSH));
	    PatBlt( hImageDC, 0, 0, cx, cy, PATCOPY);
	    SelectObject(hImageDC, hOldBrush);
	}
    } else {
	/* blend the image with the needed solid background */
        COLORREF colour = RGB(0,0,0);

        if( !bIsTransparent )
        {
            colour = pimldp->rgbBk;
            if( colour == CLR_DEFAULT )
                colour = himl->clrBk;
            if( colour == CLR_NONE )
                colour = GetBkColor(pimldp->hdcDst);
        }

        hOldBrush = SelectObject (hImageDC, CreateSolidBrush (colour));
        PatBlt( hImageDC, 0, 0, cx, cy, PATCOPY );
        if (himl->hbmMask)
        {
            BitBlt( hImageDC, 0, 0, cx, cy, hMaskListDC, pt.x, pt.y, SRCAND );
            BitBlt( hImageDC, 0, 0, cx, cy, hImageListDC, pt.x, pt.y, SRCPAINT );
        }
        else
            BitBlt( hImageDC, 0, 0, cx, cy, hImageListDC, pt.x, pt.y, SRCCOPY);
        DeleteObject (SelectObject (hImageDC, hOldBrush));
    }

    /* Time for blending, if required */
    if (bBlend) {
	HBRUSH hBlendBrush;
        COLORREF clrBlend = pimldp->rgbFg;
	HDC hBlendMaskDC = hImageListDC;
	HBITMAP hOldBitmap;

	/* Create the blend Mask */
    	hOldBitmap = SelectObject(hBlendMaskDC, hBlendMaskBmp);
	hBlendBrush = fStyle & ILD_BLEND50 ? himl->hbrBlend50 : himl->hbrBlend25;
        hOldBrush = SelectObject(hBlendMaskDC, hBlendBrush);
    	PatBlt(hBlendMaskDC, 0, 0, cx, cy, PATCOPY);
    	SelectObject(hBlendMaskDC, hOldBrush);

    	/* Modify the blend mask if an Image Mask exist */
    	if(himl->hbmMask) {
	    BitBlt(hBlendMaskDC, 0, 0, cx, cy, hMaskListDC, pt.x, pt.y, 0x220326); /* NOTSRCAND */
	    BitBlt(hBlendMaskDC, 0, 0, cx, cy, hBlendMaskDC, 0, 0, NOTSRCCOPY);
	}

	/* now apply blend to the current image given the BlendMask */
        if (clrBlend == CLR_DEFAULT) clrBlend = GetSysColor (COLOR_HIGHLIGHT);
        else if (clrBlend == CLR_NONE) clrBlend = GetTextColor (pimldp->hdcDst);
	hOldBrush = SelectObject (hImageDC, CreateSolidBrush(clrBlend));
	BitBlt (hImageDC, 0, 0, cx, cy, hBlendMaskDC, 0, 0, 0xB8074A); /* PSDPxax */
	DeleteObject(SelectObject(hImageDC, hOldBrush));
	SelectObject(hBlendMaskDC, hOldBitmap);
    }

    /* Now do the overlay image, if any */
    nOvlIdx = (pimldp->fStyle & ILD_OVERLAYMASK) >> 8;
    if ( (nOvlIdx >= 1) && (nOvlIdx <= MAX_OVERLAYIMAGE)) {
	nOvlIdx = himl->nOvlIdx[nOvlIdx - 1];
	if ((nOvlIdx >= 0) && (nOvlIdx < himl->cCurImage)) {
            POINT ptOvl;
            imagelist_point_from_index( himl, nOvlIdx, &ptOvl );
            ptOvl.x += pimldp->xBitmap;
	    if (himl->hbmMask && !(fStyle & ILD_IMAGE))
		BitBlt (hImageDC, 0, 0, cx, cy, hMaskListDC, ptOvl.x, ptOvl.y, SRCAND);
	    BitBlt (hImageDC, 0, 0, cx, cy, hImageListDC, ptOvl.x, ptOvl.y, SRCPAINT);
	}
    }

#ifndef __REACTOS__
    if (fState & ILS_SATURATE) FIXME("ILS_SATURATE: unimplemented!\n");
#endif
    if (fState & ILS_GLOW) FIXME("ILS_GLOW: unimplemented!\n");
    if (fState & ILS_SHADOW) FIXME("ILS_SHADOW: unimplemented!\n");

    if (fStyle & ILD_PRESERVEALPHA) FIXME("ILD_PRESERVEALPHA: unimplemented!\n");
    if (fStyle & ILD_SCALE) FIXME("ILD_SCALE: unimplemented!\n");
    if (fStyle & ILD_DPISCALE) FIXME("ILD_DPISCALE: unimplemented!\n");

    /* now copy the image to the screen */
    dwRop = SRCCOPY;
    if (himl->hbmMask && bIsTransparent ) {
	COLORREF oldDstFg = SetTextColor(pimldp->hdcDst, RGB( 0, 0, 0 ) );
	COLORREF oldDstBk = SetBkColor(pimldp->hdcDst, RGB( 0xff, 0xff, 0xff ));
        BitBlt (pimldp->hdcDst, pimldp->x,  pimldp->y, cx, cy, hMaskListDC, pt.x, pt.y, SRCAND);
	SetBkColor(pimldp->hdcDst, oldDstBk);
	SetTextColor(pimldp->hdcDst, oldDstFg);
	dwRop = SRCPAINT;
    }
    if (fStyle & ILD_ROP) dwRop = pimldp->dwRop;
    BitBlt (pimldp->hdcDst, pimldp->x,  pimldp->y, cx, cy, hImageDC, 0, 0, dwRop);

    bResult = TRUE;
end:
    /* cleanup the mess */
    SetBkColor(hImageDC, oldImageBk);
    SetTextColor(hImageDC, oldImageFg);
    SelectObject(hImageDC, hOldImageBmp);
cleanup:
#ifdef __REACTOS__
    if (hdcSaturated)
        DeleteDC(hdcSaturated);
    if (hdcSaturatedMask)
        DeleteDC(hdcSaturatedMask);
#endif
    DeleteObject(hBlendMaskBmp);
    DeleteObject(hImageBmp);
    DeleteDC(hImageDC);

    return bResult;
}


/*************************************************************************
 * ImageList_Duplicate [COMCTL32.@]
 *
 * Duplicates an image list.
 *
 * PARAMS
 *     himlSrc [I] source image list handle
 *
 * RETURNS
 *     Success: Handle of duplicated image list.
 *     Failure: NULL
 */

HIMAGELIST WINAPI
ImageList_Duplicate (HIMAGELIST himlSrc)
{
    HIMAGELIST himlDst;

    if (!is_valid(himlSrc)) {
        ERR("Invalid image list handle!\n");
        return NULL;
    }

    himlDst = ImageList_Create (himlSrc->cx, himlSrc->cy, himlSrc->flags,
                                himlSrc->cCurImage, himlSrc->cGrow);

    if (himlDst)
    {
        SIZE sz;

        imagelist_get_bitmap_size(himlSrc, himlSrc->cCurImage, &sz);
        BitBlt (himlDst->hdcImage, 0, 0, sz.cx, sz.cy,
                himlSrc->hdcImage, 0, 0, SRCCOPY);

        if (himlDst->hbmMask)
            BitBlt (himlDst->hdcMask, 0, 0, sz.cx, sz.cy,
                    himlSrc->hdcMask, 0, 0, SRCCOPY);

	himlDst->cCurImage = himlSrc->cCurImage;
        if (himlSrc->has_alpha && himlDst->has_alpha)
            memcpy( himlDst->has_alpha, himlSrc->has_alpha, himlDst->cCurImage );
    }
    return himlDst;
}


/*************************************************************************
 * ImageList_EndDrag [COMCTL32.@]
 *
 * Finishes a drag operation.
 *
 * PARAMS
 *     no Parameters
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

VOID WINAPI
ImageList_EndDrag (void)
{
    /* cleanup the InternalDrag struct */
    InternalDrag.hwnd = 0;
    if (InternalDrag.himl != InternalDrag.himlNoCursor)
        ImageList_Destroy (InternalDrag.himlNoCursor);
    ImageList_Destroy (InternalDrag.himl);
    InternalDrag.himlNoCursor = InternalDrag.himl = 0;
    InternalDrag.x= 0;
    InternalDrag.y= 0;
    InternalDrag.dxHotspot = 0;
    InternalDrag.dyHotspot = 0;
    InternalDrag.bShow = FALSE;
    DeleteObject(InternalDrag.hbmBg);
    InternalDrag.hbmBg = 0;
}


/*************************************************************************
 * ImageList_GetBkColor [COMCTL32.@]
 *
 * Returns the background color of an image list.
 *
 * PARAMS
 *     himl [I] Image list handle.
 *
 * RETURNS
 *     Success: background color
 *     Failure: CLR_NONE
 */

COLORREF WINAPI
ImageList_GetBkColor (HIMAGELIST himl)
{
    return himl ? himl->clrBk : CLR_NONE;
}


/*************************************************************************
 * ImageList_GetDragImage [COMCTL32.@]
 *
 * Returns the handle to the internal drag image list.
 *
 * PARAMS
 *     ppt        [O] Pointer to the drag position. Can be NULL.
 *     pptHotspot [O] Pointer to the position of the hot spot. Can be NULL.
 *
 * RETURNS
 *     Success: Handle of the drag image list.
 *     Failure: NULL.
 */

HIMAGELIST WINAPI
ImageList_GetDragImage (POINT *ppt, POINT *pptHotspot)
{
    if (is_valid(InternalDrag.himl)) {
	if (ppt) {
	    ppt->x = InternalDrag.x;
	    ppt->y = InternalDrag.y;
	}
	if (pptHotspot) {
	    pptHotspot->x = InternalDrag.dxHotspot;
	    pptHotspot->y = InternalDrag.dyHotspot;
	}
        return (InternalDrag.himl);
    }

    return NULL;
}


/*************************************************************************
 * ImageList_GetFlags [COMCTL32.@]
 *
 * Gets the flags of the specified image list.
 *
 * PARAMS
 *     himl [I] Handle to image list
 *
 * RETURNS
 *     Image list flags.
 *
 * BUGS
 *    Stub.
 */

DWORD WINAPI
ImageList_GetFlags(HIMAGELIST himl)
{
    TRACE("%p\n", himl);
#ifdef __REACTOS__
    if(!is_valid2(himl))
        return 0;
    return himl->flags;
#else
    return is_valid(himl) ? himl->flags : 0;
#endif
}


/*************************************************************************
 * ImageList_GetIcon [COMCTL32.@]
 *
 * Creates an icon from a masked image of an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     i     [I] image index
 *     flags [I] drawing style flags
 *
 * RETURNS
 *     Success: icon handle
 *     Failure: NULL
 */

HICON WINAPI
ImageList_GetIcon (HIMAGELIST himl, INT i, UINT fStyle)
{
    ICONINFO ii;
    HICON hIcon;
    HBITMAP hOldDstBitmap;
    HDC hdcDst;
    POINT pt;

    TRACE("%p %d %d\n", himl, i, fStyle);
    if (!is_valid(himl) || (i < 0) || (i >= himl->cCurImage)) return NULL;

    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;

    /* create colour bitmap */
    hdcDst = GetDC(0);
    ii.hbmColor = CreateCompatibleBitmap(hdcDst, himl->cx, himl->cy);
    ReleaseDC(0, hdcDst);

    hdcDst = CreateCompatibleDC(0);

    imagelist_point_from_index( himl, i, &pt );

    /* draw mask*/
    ii.hbmMask  = CreateBitmap (himl->cx, himl->cy, 1, 1, NULL);
    hOldDstBitmap = SelectObject (hdcDst, ii.hbmMask);
    if (himl->hbmMask) {
        BitBlt (hdcDst, 0, 0, himl->cx, himl->cy,
                himl->hdcMask, pt.x, pt.y, SRCCOPY);
    }
    else
        PatBlt (hdcDst, 0, 0, himl->cx, himl->cy, BLACKNESS);

    /* draw image*/
    SelectObject (hdcDst, ii.hbmColor);
    BitBlt (hdcDst, 0, 0, himl->cx, himl->cy,
            himl->hdcImage, pt.x, pt.y, SRCCOPY);

    /*
     * CreateIconIndirect requires us to deselect the bitmaps from
     * the DCs before calling
     */
    SelectObject(hdcDst, hOldDstBitmap);

    hIcon = CreateIconIndirect (&ii);

    DeleteObject (ii.hbmMask);
    DeleteObject (ii.hbmColor);
    DeleteDC (hdcDst);

    return hIcon;
}


/*************************************************************************
 * ImageList_GetIconSize [COMCTL32.@]
 *
 * Retrieves the size of an image in an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
 *     cx   [O] pointer to the image width.
 *     cy   [O] pointer to the image height.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     All images in an image list have the same size.
 */

BOOL WINAPI
ImageList_GetIconSize (HIMAGELIST himl, INT *cx, INT *cy)
{
    if (!is_valid(himl) || !cx || !cy)
	return FALSE;

    *cx = himl->cx;
    *cy = himl->cy;

    return TRUE;
}


/*************************************************************************
 * ImageList_GetImageCount [COMCTL32.@]
 *
 * Returns the number of images in an image list.
 *
 * PARAMS
 *     himl [I] handle to image list
 *
 * RETURNS
 *     Success: Number of images.
 *     Failure: 0
 */

INT WINAPI
ImageList_GetImageCount (HIMAGELIST himl)
{
    if (!is_valid(himl))
	return 0;

    return himl->cCurImage;
}


/*************************************************************************
 * ImageList_GetImageInfo [COMCTL32.@]
 *
 * Returns information about an image in an image list.
 *
 * PARAMS
 *     himl       [I] handle to image list
 *     i          [I] image index
 *     pImageInfo [O] pointer to the image information
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_GetImageInfo (HIMAGELIST himl, INT i, IMAGEINFO *pImageInfo)
{
    POINT pt;

    if (!is_valid(himl) || (pImageInfo == NULL))
	return FALSE;
    if ((i < 0) || (i >= himl->cCurImage))
	return FALSE;

    pImageInfo->hbmImage = himl->hbmImage;
    pImageInfo->hbmMask  = himl->hbmMask;

    imagelist_point_from_index( himl, i, &pt );
    pImageInfo->rcImage.top    = pt.y;
    pImageInfo->rcImage.bottom = pt.y + himl->cy;
    pImageInfo->rcImage.left   = pt.x;
    pImageInfo->rcImage.right  = pt.x + himl->cx;

    return TRUE;
}


/*************************************************************************
 * ImageList_GetImageRect [COMCTL32.@]
 *
 * Retrieves the rectangle of the specified image in an image list.
 *
 * PARAMS
 *     himl   [I] handle to image list
 *     i      [I] image index
 *     lpRect [O] pointer to the image rectangle
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * NOTES
 *    This is an UNDOCUMENTED function!!!
 */

BOOL WINAPI
ImageList_GetImageRect (HIMAGELIST himl, INT i, LPRECT lpRect)
{
#ifdef __REACTOS__
    IMAGEINFO ImageInfo;
    if (!ImageList_GetImageInfo(himl, i, &ImageInfo))
        return FALSE;
    *lpRect = ImageInfo.rcImage;
#else
    POINT pt;

    if (!is_valid(himl) || (lpRect == NULL))
	return FALSE;
    if ((i < 0) || (i >= himl->cCurImage))
	return FALSE;

    imagelist_point_from_index( himl, i, &pt );
    lpRect->left   = pt.x;
    lpRect->top    = pt.y;
    lpRect->right  = pt.x + himl->cx;
    lpRect->bottom = pt.y + himl->cy;
#endif
    return TRUE;
}


/*************************************************************************
 * ImageList_LoadImage  [COMCTL32.@]
 * ImageList_LoadImageA [COMCTL32.@]
 *
 * Creates an image list from a bitmap, icon or cursor.
 *
 * See ImageList_LoadImageW.
 */

HIMAGELIST WINAPI
ImageList_LoadImageA (HINSTANCE hi, LPCSTR lpbmp, INT cx, INT cGrow,
			COLORREF clrMask, UINT uType, UINT uFlags)
{
    HIMAGELIST himl;
    LPWSTR lpbmpW;
    DWORD len;

    if (IS_INTRESOURCE(lpbmp))
        return ImageList_LoadImageW(hi, (LPCWSTR)lpbmp, cx, cGrow, clrMask,
                                    uType, uFlags);

    len = MultiByteToWideChar(CP_ACP, 0, lpbmp, -1, NULL, 0);
    lpbmpW = heap_alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpbmp, -1, lpbmpW, len);

    himl = ImageList_LoadImageW(hi, lpbmpW, cx, cGrow, clrMask, uType, uFlags);
    heap_free (lpbmpW);
    return himl;
}


/*************************************************************************
 * ImageList_LoadImageW [COMCTL32.@]
 *
 * Creates an image list from a bitmap, icon or cursor.
 *
 * PARAMS
 *     hi      [I] instance handle
 *     lpbmp   [I] name or id of the image
 *     cx      [I] width of each image
 *     cGrow   [I] number of images to expand
 *     clrMask [I] mask color
 *     uType   [I] type of image to load
 *     uFlags  [I] loading flags
 *
 * RETURNS
 *     Success: handle to the loaded image list
 *     Failure: NULL
 *
 * SEE
 *     LoadImage ()
 */

HIMAGELIST WINAPI
ImageList_LoadImageW (HINSTANCE hi, LPCWSTR lpbmp, INT cx, INT cGrow,
                      COLORREF clrMask, UINT uType, UINT uFlags)
{
    HIMAGELIST himl = NULL;
    HANDLE   handle;
    INT      nImageCount;

    handle = LoadImageW (hi, lpbmp, uType, 0, 0, uFlags);
    if (!handle) {
        WARN("Couldn't load image\n");
        return NULL;
    }

    if (uType == IMAGE_BITMAP) {
        DIBSECTION dib;
        UINT color;

        if (GetObjectW (handle, sizeof(dib), &dib) == sizeof(BITMAP)) color = ILC_COLOR;
        else color = dib.dsBm.bmBitsPixel;

        /* To match windows behavior, if cx is set to zero and
         the flag DI_DEFAULTSIZE is specified, cx becomes the
         system metric value for icons. If the flag is not specified
         the function sets the size to the height of the bitmap */
        if (cx == 0)
        {
            if (uFlags & DI_DEFAULTSIZE)
                cx = GetSystemMetrics (SM_CXICON);
            else
                cx = dib.dsBm.bmHeight;
        }

        nImageCount = dib.dsBm.bmWidth / cx;

        if (clrMask != CLR_NONE) color |= ILC_MASK;
        himl = ImageList_Create (cx, dib.dsBm.bmHeight, color, nImageCount, cGrow);
        if (!himl) {
            DeleteObject (handle);
            return NULL;
        }
        ImageList_AddMasked (himl, handle, clrMask);
    }
    else if ((uType == IMAGE_ICON) || (uType == IMAGE_CURSOR)) {
        ICONINFO ii;
        BITMAP bmp;

        GetIconInfo (handle, &ii);
        GetObjectW (ii.hbmColor, sizeof(BITMAP), &bmp);
        himl = ImageList_Create (bmp.bmWidth, bmp.bmHeight,
                                 ILC_MASK | ILC_COLOR, 1, cGrow);
        if (!himl) {
            DeleteObject (ii.hbmColor);
            DeleteObject (ii.hbmMask);
            DeleteObject (handle);
            return NULL;
        }
        ImageList_Add (himl, ii.hbmColor, ii.hbmMask);
        DeleteObject (ii.hbmColor);
        DeleteObject (ii.hbmMask);
    }

    DeleteObject (handle);

    return himl;
}


/*************************************************************************
 * ImageList_Merge [COMCTL32.@]
 *
 * Create an image list containing a merged image from two image lists.
 *
 * PARAMS
 *     himl1 [I] handle to first image list
 *     i1    [I] first image index
 *     himl2 [I] handle to second image list
 *     i2    [I] second image index
 *     dx    [I] X offset of the second image relative to the first.
 *     dy    [I] Y offset of the second image relative to the first.
 *
 * RETURNS
 *     Success: The newly created image list. It contains a single image
 *              consisting of the second image merged with the first.
 *     Failure: NULL, if either himl1 or himl2 is invalid.
 *
 * NOTES
 *   - The returned image list should be deleted by the caller using
 *     ImageList_Destroy() when it is no longer required.
 *   - If either i1 or i2 is not a valid image index, they will be treated
 *     as blank images.
 */
HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT i1, HIMAGELIST himl2, INT i2,
		 INT dx, INT dy)
{
    HIMAGELIST himlDst = NULL;
    INT      cxDst, cyDst;
    INT      xOff1, yOff1, xOff2, yOff2;
    POINT    pt1, pt2;
    INT      newFlags;

    TRACE("(himl1=%p i1=%d himl2=%p i2=%d dx=%d dy=%d)\n", himl1, i1, himl2,
	   i2, dx, dy);

    if (!is_valid(himl1) || !is_valid(himl2))
	return NULL;

    if (dx > 0) {
        cxDst = max (himl1->cx, dx + himl2->cx);
        xOff1 = 0;
        xOff2 = dx;
    }
    else if (dx < 0) {
        cxDst = max (himl2->cx, himl1->cx - dx);
        xOff1 = -dx;
        xOff2 = 0;
    }
    else {
        cxDst = max (himl1->cx, himl2->cx);
        xOff1 = 0;
        xOff2 = 0;
    }

    if (dy > 0) {
        cyDst = max (himl1->cy, dy + himl2->cy);
        yOff1 = 0;
        yOff2 = dy;
    }
    else if (dy < 0) {
        cyDst = max (himl2->cy, himl1->cy - dy);
        yOff1 = -dy;
        yOff2 = 0;
    }
    else {
        cyDst = max (himl1->cy, himl2->cy);
        yOff1 = 0;
        yOff2 = 0;
    }

    newFlags = (himl1->flags > himl2->flags ? himl1->flags : himl2->flags) & ILC_COLORDDB;
    if (newFlags == ILC_COLORDDB && (himl1->flags & ILC_COLORDDB) == ILC_COLOR16)
        newFlags = ILC_COLOR16; /* this is what native (at least v5) does, don't know why */
    himlDst = ImageList_Create (cxDst, cyDst, ILC_MASK | newFlags, 1, 1);

    if (himlDst)
    {
        imagelist_point_from_index( himl1, i1, &pt1 );
        imagelist_point_from_index( himl2, i2, &pt2 );

        /* copy image */
        BitBlt (himlDst->hdcImage, 0, 0, cxDst, cyDst, himl1->hdcImage, 0, 0, BLACKNESS);
        if (i1 >= 0 && i1 < himl1->cCurImage)
            BitBlt (himlDst->hdcImage, xOff1, yOff1, himl1->cx, himl1->cy, himl1->hdcImage, pt1.x, pt1.y, SRCCOPY);
        if (i2 >= 0 && i2 < himl2->cCurImage)
        {
            if (himl2->flags & ILC_MASK)
            {
                BitBlt (himlDst->hdcImage, xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcMask , pt2.x, pt2.y, SRCAND);
                BitBlt (himlDst->hdcImage, xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcImage, pt2.x, pt2.y, SRCPAINT);
            }
            else
                BitBlt (himlDst->hdcImage, xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcImage, pt2.x, pt2.y, SRCCOPY);
        }

        /* copy mask */
        BitBlt (himlDst->hdcMask, 0, 0, cxDst, cyDst, himl1->hdcMask, 0, 0, WHITENESS);
        if (i1 >= 0 && i1 < himl1->cCurImage)
            BitBlt (himlDst->hdcMask,  xOff1, yOff1, himl1->cx, himl1->cy, himl1->hdcMask,  pt1.x, pt1.y, SRCCOPY);
        if (i2 >= 0 && i2 < himl2->cCurImage)
            BitBlt (himlDst->hdcMask,  xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcMask,  pt2.x, pt2.y, SRCAND);

	himlDst->cCurImage = 1;
    }

    return himlDst;
}


/* helper for ImageList_Read, see comments below */
static void *read_bitmap(IStream *pstm, BITMAPINFO *bmi)
{
    BITMAPFILEHEADER	bmfh;
    int bitsperpixel, palspace;
    void *bits;

    if (FAILED(IStream_Read ( pstm, &bmfh, sizeof(bmfh), NULL)))
        return NULL;

    if (bmfh.bfType != (('M'<<8)|'B'))
        return NULL;

    if (FAILED(IStream_Read ( pstm, &bmi->bmiHeader, sizeof(bmi->bmiHeader), NULL)))
        return NULL;

    if ((bmi->bmiHeader.biSize != sizeof(bmi->bmiHeader)))
        return NULL;

    TRACE("width %u, height %u, planes %u, bpp %u\n",
          bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
          bmi->bmiHeader.biPlanes, bmi->bmiHeader.biBitCount);

    bitsperpixel = bmi->bmiHeader.biPlanes * bmi->bmiHeader.biBitCount;
    if (bitsperpixel<=8)
        palspace = (1<<bitsperpixel)*sizeof(RGBQUAD);
    else
        palspace = 0;

    bmi->bmiHeader.biSizeImage = get_dib_image_size( bmi );

    /* read the palette right after the end of the bitmapinfoheader */
    if (palspace && FAILED(IStream_Read(pstm, bmi->bmiColors, palspace, NULL)))
        return NULL;

    bits = heap_alloc_zero(bmi->bmiHeader.biSizeImage);
    if (!bits) return NULL;

    if (FAILED(IStream_Read(pstm, bits, bmi->bmiHeader.biSizeImage, NULL)))
    {
        heap_free(bits);
        return NULL;
    }
    return bits;
}

/*************************************************************************
 * ImageList_Read [COMCTL32.@]
 *
 * Reads an image list from a stream.
 *
 * PARAMS
 *     pstm [I] pointer to a stream
 *
 * RETURNS
 *     Success: handle to image list
 *     Failure: NULL
 *
 * The format is like this:
 *	ILHEAD			ilheadstruct;
 *
 * for the color image part:
 *	BITMAPFILEHEADER	bmfh;
 *	BITMAPINFOHEADER	bmih;
 * only if it has a palette:
 *	RGBQUAD		rgbs[nr_of_paletted_colors];
 *
 *	BYTE			colorbits[imagesize];
 *
 * the following only if the ILC_MASK bit is set in ILHEAD.ilFlags:
 *	BITMAPFILEHEADER	bmfh_mask;
 *	BITMAPINFOHEADER	bmih_mask;
 * only if it has a palette (it usually does not):
 *	RGBQUAD		rgbs[nr_of_paletted_colors];
 *
 *	BYTE			maskbits[imagesize];
 */
HIMAGELIST WINAPI ImageList_Read(IStream *pstm)
{
    char image_buf[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
    char mask_buf[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
    BITMAPINFO *image_info = (BITMAPINFO *)image_buf;
    BITMAPINFO *mask_info = (BITMAPINFO *)mask_buf;
    void *image_bits, *mask_bits = NULL;
    ILHEAD	ilHead;
    HIMAGELIST	himl;
    unsigned int i;

    TRACE("%p\n", pstm);

    if (FAILED(IStream_Read (pstm, &ilHead, sizeof(ILHEAD), NULL)))
	return NULL;
    if (ilHead.usMagic != (('L' << 8) | 'I'))
	return NULL;
    if (ilHead.usVersion != 0x101) /* probably version? */
	return NULL;

    TRACE("cx %u, cy %u, flags 0x%04x, cCurImage %u, cMaxImage %u\n",
          ilHead.cx, ilHead.cy, ilHead.flags, ilHead.cCurImage, ilHead.cMaxImage);

    himl = ImageList_Create(ilHead.cx, ilHead.cy, ilHead.flags, ilHead.cMaxImage, ilHead.cGrow);
    if (!himl)
	return NULL;

    if (!(image_bits = read_bitmap(pstm, image_info)))
    {
	WARN("failed to read bitmap from stream\n");
	return NULL;
    }
    if (ilHead.flags & ILC_MASK)
    {
        if (!(mask_bits = read_bitmap(pstm, mask_info)))
        {
            WARN("failed to read mask bitmap from stream\n");
	    return NULL;
	}
    }
    else mask_info = NULL;

    if (himl->has_alpha && image_info->bmiHeader.biBitCount == 32)
    {
        DWORD *ptr = image_bits;
        BYTE *mask_ptr = mask_bits;
        int stride = himl->cy * image_info->bmiHeader.biWidth;

        if (image_info->bmiHeader.biHeight > 0)  /* bottom-up */
        {
            ptr += image_info->bmiHeader.biHeight * image_info->bmiHeader.biWidth - stride;
            mask_ptr += (image_info->bmiHeader.biHeight * image_info->bmiHeader.biWidth - stride) / 8;
            stride = -stride;
            image_info->bmiHeader.biHeight = himl->cy;
        }
        else image_info->bmiHeader.biHeight = -himl->cy;

        for (i = 0; i < ilHead.cCurImage; i += TILE_COUNT)
        {
            add_dib_bits( himl, i, min( ilHead.cCurImage - i, TILE_COUNT ),
                          himl->cx, himl->cy, image_info, mask_info, ptr, mask_ptr );
            ptr += stride;
            mask_ptr += stride / 8;
        }
    }
    else
    {
        StretchDIBits( himl->hdcImage, 0, 0, image_info->bmiHeader.biWidth, image_info->bmiHeader.biHeight,
                       0, 0, image_info->bmiHeader.biWidth, image_info->bmiHeader.biHeight,
                       image_bits, image_info, DIB_RGB_COLORS, SRCCOPY);
        if (mask_info)
            StretchDIBits( himl->hdcMask, 0, 0, mask_info->bmiHeader.biWidth, mask_info->bmiHeader.biHeight,
                           0, 0, mask_info->bmiHeader.biWidth, mask_info->bmiHeader.biHeight,
                           mask_bits, mask_info, DIB_RGB_COLORS, SRCCOPY);
    }
    heap_free( image_bits );
    heap_free( mask_bits );

    himl->cCurImage = ilHead.cCurImage;
    himl->cMaxImage = ilHead.cMaxImage;

    ImageList_SetBkColor(himl,ilHead.bkcolor);
    for (i=0;i<4;i++)
	ImageList_SetOverlayImage(himl,ilHead.ovls[i],i+1);
    return himl;
}


/*************************************************************************
 * ImageList_Remove [COMCTL32.@]
 *
 * Removes an image from an image list
 *
 * PARAMS
 *     himl [I] image list handle
 *     i    [I] image index
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * FIXME: as the image list storage test shows, native comctl32 simply shifts
 * images without creating a new bitmap.
 */
BOOL WINAPI
ImageList_Remove (HIMAGELIST himl, INT i)
{
    HBITMAP hbmNewImage, hbmNewMask;
    HDC     hdcBmp;
    SIZE    sz;

    TRACE("(himl=%p i=%d)\n", himl, i);

    if (!is_valid(himl)) {
        ERR("Invalid image list handle!\n");
        return FALSE;
    }

    if ((i < -1) || (i >= himl->cCurImage)) {
        TRACE("index out of range! %d\n", i);
        return FALSE;
    }

    if (i == -1) {
        INT nCount;

        /* remove all */
	if (himl->cCurImage == 0) {
	    /* remove all on empty ImageList is allowed */
	    TRACE("remove all on empty ImageList!\n");
	    return TRUE;
	}

        himl->cMaxImage = himl->cGrow;
        himl->cCurImage = 0;
        for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
             himl->nOvlIdx[nCount] = -1;

        if (himl->has_alpha)
        {
            heap_free( himl->has_alpha );
            himl->has_alpha = heap_alloc_zero( himl->cMaxImage );
        }

        hbmNewImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage);
        SelectObject (himl->hdcImage, hbmNewImage);
        DeleteObject (himl->hbmImage);
        himl->hbmImage = hbmNewImage;

        if (himl->hbmMask) {

            imagelist_get_bitmap_size(himl, himl->cMaxImage, &sz);
            hbmNewMask = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
            SelectObject (himl->hdcMask, hbmNewMask);
            DeleteObject (himl->hbmMask);
            himl->hbmMask = hbmNewMask;
        }
    }
    else {
        /* delete one image */
        TRACE("Remove single image! %d\n", i);

        /* create new bitmap(s) */
        TRACE(" - Number of images: %d / %d (Old/New)\n",
                 himl->cCurImage, himl->cCurImage - 1);

        hbmNewImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage);

        imagelist_get_bitmap_size(himl, himl->cMaxImage, &sz );
        if (himl->hbmMask)
            hbmNewMask = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
        else
            hbmNewMask = 0;  /* Just to keep compiler happy! */

        hdcBmp = CreateCompatibleDC (0);

        /* copy all images and masks prior to the "removed" image */
        if (i > 0) {
            TRACE("Pre image copy: Copy %d images\n", i);

            SelectObject (hdcBmp, hbmNewImage);
            imagelist_copy_images( himl, himl->hdcImage, hdcBmp, 0, i, 0 );

            if (himl->hbmMask) {
                SelectObject (hdcBmp, hbmNewMask);
                imagelist_copy_images( himl, himl->hdcMask, hdcBmp, 0, i, 0 );
            }
        }

        /* copy all images and masks behind the removed image */
        if (i < himl->cCurImage - 1) {
            TRACE("Post image copy!\n");

            SelectObject (hdcBmp, hbmNewImage);
            imagelist_copy_images( himl, himl->hdcImage, hdcBmp, i + 1,
                                   (himl->cCurImage - i), i );

            if (himl->hbmMask) {
                SelectObject (hdcBmp, hbmNewMask);
                imagelist_copy_images( himl, himl->hdcMask, hdcBmp, i + 1,
                                       (himl->cCurImage - i), i );
            }
        }

        DeleteDC (hdcBmp);

        /* delete old images and insert new ones */
        SelectObject (himl->hdcImage, hbmNewImage);
        DeleteObject (himl->hbmImage);
        himl->hbmImage = hbmNewImage;
        if (himl->hbmMask) {
            SelectObject (himl->hdcMask, hbmNewMask);
            DeleteObject (himl->hbmMask);
            himl->hbmMask = hbmNewMask;
        }

        himl->cCurImage--;
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_Replace [COMCTL32.@]
 *
 * Replaces an image in an image list with a new image.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     i        [I] image index
 *     hbmImage [I] handle to image bitmap
 *     hbmMask  [I] handle to mask bitmap. Can be NULL.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_Replace (HIMAGELIST himl, INT i, HBITMAP hbmImage,
		   HBITMAP hbmMask)
{
    HDC hdcImage;
    BITMAP bmp;
    POINT pt;

    TRACE("%p %d %p %p\n", himl, i, hbmImage, hbmMask);

    if (!is_valid(himl)) {
        ERR("Invalid image list handle!\n");
        return FALSE;
    }

    if ((i >= himl->cMaxImage) || (i < 0)) {
        ERR("Invalid image index!\n");
        return FALSE;
    }

    if (!GetObjectW(hbmImage, sizeof(BITMAP), &bmp))
        return FALSE;

    hdcImage = CreateCompatibleDC (0);

    /* Replace Image */
    SelectObject (hdcImage, hbmImage);

    if (add_with_alpha( himl, hdcImage, i, 1, bmp.bmWidth, bmp.bmHeight, hbmImage, hbmMask ))
        goto done;

    imagelist_point_from_index(himl, i, &pt);
    StretchBlt (himl->hdcImage, pt.x, pt.y, himl->cx, himl->cy,
                  hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    if (himl->hbmMask)
    {
        HDC hdcTemp;
        HBITMAP hOldBitmapTemp;

        hdcTemp   = CreateCompatibleDC(0);
        hOldBitmapTemp = SelectObject(hdcTemp, hbmMask);

        StretchBlt (himl->hdcMask, pt.x, pt.y, himl->cx, himl->cy,
                      hdcTemp, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        SelectObject(hdcTemp, hOldBitmapTemp);
        DeleteDC(hdcTemp);

        /* Remove the background from the image
        */
        BitBlt (himl->hdcImage, pt.x, pt.y, bmp.bmWidth, bmp.bmHeight,
                himl->hdcMask, pt.x, pt.y, 0x220326); /* NOTSRCAND */
    }

done:
    DeleteDC (hdcImage);

    return TRUE;
}


/*************************************************************************
 * ImageList_ReplaceIcon [COMCTL32.@]
 *
 * Replaces an image in an image list using an icon.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     i     [I] image index
 *     hIcon [I] handle to icon
 *
 * RETURNS
 *     Success: index of the replaced image
 *     Failure: -1
 */

INT WINAPI
ImageList_ReplaceIcon (HIMAGELIST himl, INT nIndex, HICON hIcon)
{
    HICON   hBestFitIcon;
    ICONINFO  ii;
    BITMAP  bmp;
    BOOL    ret;
    POINT   pt;

    TRACE("(%p %d %p)\n", himl, nIndex, hIcon);

    if (!is_valid(himl)) {
        ERR("invalid image list\n");
        return -1;
    }
    if ((nIndex >= himl->cMaxImage) || (nIndex < -1)) {
        ERR("invalid image index %d / %d\n", nIndex, himl->cMaxImage);
        return -1;
    }

    hBestFitIcon = CopyImage(
        hIcon, IMAGE_ICON,
        himl->cx, himl->cy,
        LR_COPYFROMRESOURCE);
    /* the above will fail if the icon wasn't loaded from a resource, so try
     * again without LR_COPYFROMRESOURCE flag */
    if (!hBestFitIcon)
        hBestFitIcon = CopyImage(
            hIcon, IMAGE_ICON,
            himl->cx, himl->cy,
            0);
    if (!hBestFitIcon)
        return -1;

    if (nIndex == -1) {
        if (himl->cCurImage + 1 >= himl->cMaxImage)
            IMAGELIST_InternalExpandBitmaps(himl, 1);

        nIndex = himl->cCurImage;
        himl->cCurImage++;
    }

    if (himl->has_alpha && GetIconInfo (hBestFitIcon, &ii))
    {
        HDC hdcImage = CreateCompatibleDC( 0 );
        GetObjectW (ii.hbmMask, sizeof(BITMAP), &bmp);

        if (!ii.hbmColor)
        {
            UINT height = bmp.bmHeight / 2;
            HDC hdcMask = CreateCompatibleDC( 0 );
            HBITMAP color = CreateBitmap( bmp.bmWidth, height, 1, 1, NULL );
            SelectObject( hdcImage, color );
            SelectObject( hdcMask, ii.hbmMask );
            BitBlt( hdcImage, 0, 0, bmp.bmWidth, height, hdcMask, 0, height, SRCCOPY );
            ret = add_with_alpha( himl, hdcImage, nIndex, 1, bmp.bmWidth, height, color, ii.hbmMask );
            DeleteDC( hdcMask );
            DeleteObject( color );
        }
        else ret = add_with_alpha( himl, hdcImage, nIndex, 1, bmp.bmWidth, bmp.bmHeight,
                                   ii.hbmColor, ii.hbmMask );

        DeleteDC( hdcImage );
        DeleteObject (ii.hbmMask);
        if (ii.hbmColor) DeleteObject (ii.hbmColor);
        if (ret) goto done;
    }

    imagelist_point_from_index(himl, nIndex, &pt);

    if (himl->hbmMask)
    {
        DrawIconEx( himl->hdcImage, pt.x, pt.y, hBestFitIcon, himl->cx, himl->cy, 0, 0, DI_IMAGE );
        PatBlt( himl->hdcMask, pt.x, pt.y, himl->cx, himl->cy, WHITENESS );
        DrawIconEx( himl->hdcMask, pt.x, pt.y, hBestFitIcon, himl->cx, himl->cy, 0, 0, DI_MASK );
    }
    else
    {
        COLORREF color = himl->clrBk != CLR_NONE ? himl->clrBk : comctl32_color.clrWindow;
        HBRUSH brush = CreateSolidBrush( GetNearestColor( himl->hdcImage, color ));

        SelectObject( himl->hdcImage, brush );
        PatBlt( himl->hdcImage, pt.x, pt.y, himl->cx, himl->cy, PATCOPY );
        SelectObject( himl->hdcImage, GetStockObject(BLACK_BRUSH) );
        DeleteObject( brush );
        DrawIconEx( himl->hdcImage, pt.x, pt.y, hBestFitIcon, himl->cx, himl->cy, 0, 0, DI_NORMAL );
    }

done:
    DestroyIcon(hBestFitIcon);

    TRACE("Insert index = %d, himl->cCurImage = %d\n", nIndex, himl->cCurImage);
    return nIndex;
}


/*************************************************************************
 * ImageList_SetBkColor [COMCTL32.@]
 *
 * Sets the background color of an image list.
 *
 * PARAMS
 *     himl  [I] handle to image list
 *     clrBk [I] background color
 *
 * RETURNS
 *     Success: previous background color
 *     Failure: CLR_NONE
 */

COLORREF WINAPI
ImageList_SetBkColor (HIMAGELIST himl, COLORREF clrBk)
{
    COLORREF clrOldBk;

    if (!is_valid(himl))
	return CLR_NONE;

    clrOldBk = himl->clrBk;
    himl->clrBk = clrBk;
    return clrOldBk;
}


/*************************************************************************
 * ImageList_SetDragCursorImage [COMCTL32.@]
 *
 * Combines the specified image with the current drag image
 *
 * PARAMS
 *     himlDrag  [I] handle to drag image list
 *     iDrag     [I] drag image index
 *     dxHotspot [I] X position of the hot spot
 *     dyHotspot [I] Y position of the hot spot
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *   - The names dxHotspot, dyHotspot are misleading because they have nothing
 *     to do with a hotspot but are only the offset of the origin of the new
 *     image relative to the origin of the old image.
 *
 *   - When this function is called and the drag image is visible, a
 *     short flickering occurs but this matches the Win9x behavior. It is
 *     possible to fix the flickering using code like in ImageList_DragMove.
 */

BOOL WINAPI
ImageList_SetDragCursorImage (HIMAGELIST himlDrag, INT iDrag,
			      INT dxHotspot, INT dyHotspot)
{
    HIMAGELIST himlTemp;
    BOOL visible;

    if (!is_valid(InternalDrag.himl) || !is_valid(himlDrag))
	return FALSE;

    TRACE(" dxH=%d dyH=%d nX=%d nY=%d\n",
	   dxHotspot, dyHotspot, InternalDrag.dxHotspot, InternalDrag.dyHotspot);

    visible = InternalDrag.bShow;

    himlTemp = ImageList_Merge (InternalDrag.himlNoCursor, 0, himlDrag, iDrag,
                                dxHotspot, dyHotspot);

    if (visible) {
	/* hide the drag image */
	ImageList_DragShowNolock(FALSE);
    }
    if ((InternalDrag.himl->cx != himlTemp->cx) ||
	   (InternalDrag.himl->cy != himlTemp->cy)) {
	/* the size of the drag image changed, invalidate the buffer */
	DeleteObject(InternalDrag.hbmBg);
	InternalDrag.hbmBg = 0;
    }

    if (InternalDrag.himl != InternalDrag.himlNoCursor)
        ImageList_Destroy (InternalDrag.himl);
    InternalDrag.himl = himlTemp;

    if (visible) {
	/* show the drag image */
	ImageList_DragShowNolock(TRUE);
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_SetFilter [COMCTL32.@]
 *
 * Sets a filter (or does something completely different)!!???
 * It removes 12 Bytes from the stack (3 Parameters).
 *
 * PARAMS
 *     himl     [I] SHOULD be a handle to image list
 *     i        [I] COULD be an index?
 *     dwFilter [I] ???
 *
 * RETURNS
 *     Success: TRUE ???
 *     Failure: FALSE ???
 *
 * BUGS
 *     This is an UNDOCUMENTED function!!!!
 *     empty stub.
 */

BOOL WINAPI
ImageList_SetFilter (HIMAGELIST himl, INT i, DWORD dwFilter)
{
    FIXME("(%p 0x%x 0x%x):empty stub!\n", himl, i, dwFilter);

    return FALSE;
}


/*************************************************************************
 * ImageList_SetFlags [COMCTL32.@]
 *
 * Sets the image list flags.
 *
 * PARAMS
 *     himl  [I] Handle to image list
 *     flags [I] Flags to set
 *
 * RETURNS
 *     Old flags?
 *
 * BUGS
 *    Stub.
 */

DWORD WINAPI
ImageList_SetFlags(HIMAGELIST himl, DWORD flags)
{
    FIXME("(%p %08x):empty stub\n", himl, flags);
    return 0;
}


/*************************************************************************
 * ImageList_SetIconSize [COMCTL32.@]
 *
 * Sets the image size of the bitmap and deletes all images.
 *
 * PARAMS
 *     himl [I] handle to image list
 *     cx   [I] image width
 *     cy   [I] image height
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetIconSize (HIMAGELIST himl, INT cx, INT cy)
{
    INT nCount;
    HBITMAP hbmNew;

    if (!is_valid(himl))
	return FALSE;

    /* remove all images */
    himl->cMaxImage = himl->cInitial + 1;
    himl->cCurImage = 0;
    himl->cx        = cx;
    himl->cy        = cy;

    /* initialize overlay mask indices */
    for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
        himl->nOvlIdx[nCount] = -1;

    hbmNew = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage);
    SelectObject (himl->hdcImage, hbmNew);
    DeleteObject (himl->hbmImage);
    himl->hbmImage = hbmNew;

    if (himl->hbmMask) {
        SIZE sz;
        imagelist_get_bitmap_size(himl, himl->cMaxImage, &sz);
        hbmNew = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
        SelectObject (himl->hdcMask, hbmNew);
        DeleteObject (himl->hbmMask);
        himl->hbmMask = hbmNew;
    }

    return TRUE;
}


/*************************************************************************
 * ImageList_SetImageCount [COMCTL32.@]
 *
 * Resizes an image list to the specified number of images.
 *
 * PARAMS
 *     himl        [I] handle to image list
 *     iImageCount [I] number of images in the image list
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetImageCount (HIMAGELIST himl, UINT iImageCount)
{
    HDC     hdcBitmap;
    HBITMAP hbmNewBitmap, hbmOld;
    INT     nNewCount, nCopyCount;

    TRACE("%p %d\n",himl,iImageCount);

    if (!is_valid(himl))
	return FALSE;

    nNewCount = iImageCount + 1;
    nCopyCount = min(himl->cCurImage, iImageCount);

    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap = ImageList_CreateImage(hdcBitmap, himl, nNewCount);

    if (hbmNewBitmap != 0)
    {
        hbmOld = SelectObject (hdcBitmap, hbmNewBitmap);
        imagelist_copy_images( himl, himl->hdcImage, hdcBitmap, 0, nCopyCount, 0 );
        SelectObject (hdcBitmap, hbmOld);

	/* FIXME: delete 'empty' image space? */

        SelectObject (himl->hdcImage, hbmNewBitmap);
	DeleteObject (himl->hbmImage);
	himl->hbmImage = hbmNewBitmap;
    }
    else
	ERR("Could not create new image bitmap!\n");

    if (himl->hbmMask)
    {
        SIZE sz;
        imagelist_get_bitmap_size( himl, nNewCount, &sz );
        hbmNewBitmap = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
        if (hbmNewBitmap != 0)
        {
            hbmOld = SelectObject (hdcBitmap, hbmNewBitmap);
            imagelist_copy_images( himl, himl->hdcMask, hdcBitmap, 0, nCopyCount, 0 );
            SelectObject (hdcBitmap, hbmOld);

	    /* FIXME: delete 'empty' image space? */

            SelectObject (himl->hdcMask, hbmNewBitmap);
            DeleteObject (himl->hbmMask);
            himl->hbmMask = hbmNewBitmap;
        }
        else
            ERR("Could not create new mask bitmap!\n");
    }

    DeleteDC (hdcBitmap);

    if (himl->has_alpha)
    {
        char *new_alpha = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, himl->has_alpha, nNewCount );
        if (new_alpha) himl->has_alpha = new_alpha;
        else
        {
            heap_free( himl->has_alpha );
            himl->has_alpha = NULL;
        }
    }

    /* Update max image count and current image count */
    himl->cMaxImage = nNewCount;
    himl->cCurImage = iImageCount;

    return TRUE;
}


/*************************************************************************
 * ImageList_SetOverlayImage [COMCTL32.@]
 *
 * Assigns an overlay mask index to an existing image in an image list.
 *
 * PARAMS
 *     himl     [I] handle to image list
 *     iImage   [I] image index
 *     iOverlay [I] overlay mask index
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
ImageList_SetOverlayImage (HIMAGELIST himl, INT iImage, INT iOverlay)
{
    if (!is_valid(himl))
	return FALSE;
    if ((iOverlay < 1) || (iOverlay > MAX_OVERLAYIMAGE))
	return FALSE;
    if ((iImage!=-1) && ((iImage < 0) || (iImage > himl->cCurImage)))
	return FALSE;
    himl->nOvlIdx[iOverlay - 1] = iImage;
    return TRUE;
}



/* helper for ImageList_Write - write bitmap to pstm
 * currently everything is written as 24 bit RGB, except masks
 */
static BOOL _write_bitmap(HBITMAP hBitmap, IStream *pstm)
{
    LPBITMAPFILEHEADER bmfh;
    LPBITMAPINFOHEADER bmih;
    LPBYTE data = NULL, lpBits;
    BITMAP bm;
    INT bitCount, sizeImage, offBits, totalSize;
    HDC xdc;
    BOOL result = FALSE;

    if (!GetObjectW(hBitmap, sizeof(BITMAP), &bm))
        return FALSE;

    bitCount = bm.bmBitsPixel;
    sizeImage = get_dib_stride(bm.bmWidth, bitCount) * bm.bmHeight;

    totalSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if(bitCount <= 8)
	totalSize += (1 << bitCount) * sizeof(RGBQUAD);
    offBits = totalSize;
    totalSize += sizeImage;

    data = heap_alloc_zero(totalSize);
    bmfh = (LPBITMAPFILEHEADER)data;
    bmih = (LPBITMAPINFOHEADER)(data + sizeof(BITMAPFILEHEADER));
    lpBits = data + offBits;

    /* setup BITMAPFILEHEADER */
    bmfh->bfType      = (('M' << 8) | 'B');
    bmfh->bfSize      = offBits;
    bmfh->bfReserved1 = 0;
    bmfh->bfReserved2 = 0;
    bmfh->bfOffBits   = offBits;

    /* setup BITMAPINFOHEADER */
    bmih->biSize          = sizeof(BITMAPINFOHEADER);
    bmih->biWidth         = bm.bmWidth;
    bmih->biHeight        = bm.bmHeight;
    bmih->biPlanes        = 1;
    bmih->biBitCount      = bitCount;
    bmih->biCompression   = BI_RGB;
    bmih->biSizeImage     = sizeImage;
    bmih->biXPelsPerMeter = 0;
    bmih->biYPelsPerMeter = 0;
    bmih->biClrUsed       = 0;
    bmih->biClrImportant  = 0;

    xdc = GetDC(0);
    result = GetDIBits(xdc, hBitmap, 0, bm.bmHeight, lpBits, (BITMAPINFO *)bmih, DIB_RGB_COLORS) == bm.bmHeight;
    ReleaseDC(0, xdc);
    if (!result)
	goto failed;

    TRACE("width %u, height %u, planes %u, bpp %u\n",
          bmih->biWidth, bmih->biHeight,
          bmih->biPlanes, bmih->biBitCount);

    if(FAILED(IStream_Write(pstm, data, totalSize, NULL)))
	goto failed;

    result = TRUE;

failed:
    heap_free(data);

    return result;
}


/*************************************************************************
 * ImageList_Write [COMCTL32.@]
 *
 * Writes an image list to a stream.
 *
 * PARAMS
 *     himl [I] handle to image list
 *     pstm [O] Pointer to a stream.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * BUGS
 *     probably.
 */

BOOL WINAPI ImageList_Write(HIMAGELIST himl, IStream *pstm)
{
    ILHEAD ilHead;
    int i;

    TRACE("%p %p\n", himl, pstm);

    if (!is_valid(himl))
	return FALSE;

    ilHead.usMagic   = (('L' << 8) | 'I');
    ilHead.usVersion = 0x101;
    ilHead.cCurImage = himl->cCurImage;
    ilHead.cMaxImage = himl->cMaxImage;
    ilHead.cGrow     = himl->cGrow;
    ilHead.cx        = himl->cx;
    ilHead.cy        = himl->cy;
    ilHead.bkcolor   = himl->clrBk;
    ilHead.flags     = himl->flags;
    for(i = 0; i < 4; i++) {
	ilHead.ovls[i] = himl->nOvlIdx[i];
    }

    TRACE("cx %u, cy %u, flags 0x04%x, cCurImage %u, cMaxImage %u\n",
          ilHead.cx, ilHead.cy, ilHead.flags, ilHead.cCurImage, ilHead.cMaxImage);

    if(FAILED(IStream_Write(pstm, &ilHead, sizeof(ILHEAD), NULL)))
	return FALSE;

    /* write the bitmap */
    if(!_write_bitmap(himl->hbmImage, pstm))
	return FALSE;

    /* write the mask if we have one */
    if(himl->flags & ILC_MASK) {
	if(!_write_bitmap(himl->hbmMask, pstm))
	    return FALSE;
    }

    return TRUE;
}


static HBITMAP ImageList_CreateImage(HDC hdc, HIMAGELIST himl, UINT count)
{
    HBITMAP hbmNewBitmap;
    UINT ilc = (himl->flags & 0xFE);
    SIZE sz;

    imagelist_get_bitmap_size( himl, count, &sz );

    if ((ilc >= ILC_COLOR4 && ilc <= ILC_COLOR32) || ilc == ILC_COLOR)
    {
        char buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
        BITMAPINFO *bmi = (BITMAPINFO *)buffer;

        TRACE("Creating DIBSection %d x %d, %d Bits per Pixel\n",
              sz.cx, sz.cy, himl->uBitsPixel);

        memset( buffer, 0, sizeof(buffer) );
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = sz.cx;
	bmi->bmiHeader.biHeight = sz.cy;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = himl->uBitsPixel;
	bmi->bmiHeader.biCompression = BI_RGB;

	if (himl->uBitsPixel <= ILC_COLOR8)
	{
            if (!himl->color_table_set)
            {
                /* retrieve the default color map */
                HBITMAP tmp = CreateBitmap( 1, 1, 1, 1, NULL );
                GetDIBits( hdc, tmp, 0, 0, NULL, bmi, DIB_RGB_COLORS );
                DeleteObject( tmp );
                if (ilc == ILC_COLOR4)
                {
                    RGBQUAD tmp;
                    tmp = bmi->bmiColors[7];
                    bmi->bmiColors[7] = bmi->bmiColors[8];
                    bmi->bmiColors[8] = tmp;
                }
            }
            else
            {
                GetDIBColorTable(himl->hdcImage, 0, 1 << himl->uBitsPixel, bmi->bmiColors);
            }
        }
	hbmNewBitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, 0, 0);
    }
    else /*if (ilc == ILC_COLORDDB)*/
    {
        TRACE("Creating Bitmap: %d Bits per Pixel\n", himl->uBitsPixel);

        hbmNewBitmap = CreateBitmap (sz.cx, sz.cy, 1, himl->uBitsPixel, NULL);
    }
    TRACE("returning %p\n", hbmNewBitmap);
    return hbmNewBitmap;
}

/*************************************************************************
 * ImageList_SetColorTable [COMCTL32.@]
 *
 * Sets the color table of an image list.
 *
 * PARAMS
 *     himl        [I] Handle to the image list.
 *     uStartIndex [I] The first index to set.
 *     cEntries    [I] Number of entries to set.
 *     prgb        [I] New color information for color table for the image list.
 *
 * RETURNS
 *     Success: Number of entries in the table that were set.
 *     Failure: Zero.
 *
 * SEE
 *     ImageList_Create(), SetDIBColorTable()
 */

UINT WINAPI
ImageList_SetColorTable(HIMAGELIST himl, UINT uStartIndex, UINT cEntries, const RGBQUAD *prgb)
{
#ifdef __REACTOS__
    if(!is_valid2(himl))
        return 0;
#endif
    TRACE("(%p, %d, %d, %p)\n", himl, uStartIndex, cEntries, prgb);
    himl->color_table_set = TRUE;
    return SetDIBColorTable(himl->hdcImage, uStartIndex, cEntries, prgb);
}

/*************************************************************************
 * ImageList_CoCreateInstance [COMCTL32.@]
 *
 * Creates a new imagelist instance and returns an interface pointer to it.
 *
 * PARAMS
 *     rclsid      [I] A reference to the CLSID (CLSID_ImageList).
 *     punkOuter   [I] Pointer to IUnknown interface for aggregation, if desired
 *     riid        [I] Identifier of the requested interface.
 *     ppv         [O] Returns the address of the pointer requested, or NULL.
 *
 * RETURNS
 *     Success: S_OK.
 *     Failure: Error value.
 */
HRESULT WINAPI
ImageList_CoCreateInstance (REFCLSID rclsid, const IUnknown *punkOuter, REFIID riid, void **ppv)
{
    TRACE("(%s,%p,%s,%p)\n", debugstr_guid(rclsid), punkOuter, debugstr_guid(riid), ppv);

    if (!IsEqualCLSID(&CLSID_ImageList, rclsid))
        return E_NOINTERFACE;

    return ImageListImpl_CreateInstance(punkOuter, riid, ppv);
}


/*************************************************************************
 * IImageList implementation
 */

static HRESULT WINAPI ImageListImpl_QueryInterface(IImageList2 *iface,
    REFIID iid, void **ppv)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IImageList, iid) ||
        IsEqualIID(&IID_IImageList2, iid))
    {
        *ppv = &imgl->IImageList2_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IImageList2_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ImageListImpl_AddRef(IImageList2 *iface)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    ULONG ref = InterlockedIncrement(&imgl->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);
    return ref;
}

static ULONG WINAPI ImageListImpl_Release(IImageList2 *iface)
{
    HIMAGELIST This = impl_from_IImageList2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        /* delete image bitmaps */
        if (This->hbmImage) DeleteObject (This->hbmImage);
        if (This->hbmMask)  DeleteObject (This->hbmMask);

        /* delete image & mask DCs */
        if (This->hdcImage) DeleteDC (This->hdcImage);
        if (This->hdcMask)  DeleteDC (This->hdcMask);

        /* delete blending brushes */
        if (This->hbrBlend25) DeleteObject (This->hbrBlend25);
        if (This->hbrBlend50) DeleteObject (This->hbrBlend50);

#ifdef __REACTOS__
        This->usMagic = IMAGELIST_MAGIC_DESTROYED;
#endif
        This->IImageList2_iface.lpVtbl = NULL;
        heap_free(This->has_alpha);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ImageListImpl_Add(IImageList2 *iface, HBITMAP hbmImage,
    HBITMAP hbmMask, int *pi)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    int ret;

    if (!pi)
        return E_FAIL;

    ret = ImageList_Add(imgl, hbmImage, hbmMask);

    if (ret == -1)
        return E_FAIL;

    *pi = ret;
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_ReplaceIcon(IImageList2 *iface, int i,
    HICON hicon, int *pi)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    int ret;

    if (!pi)
        return E_FAIL;

    ret = ImageList_ReplaceIcon(imgl, i, hicon);

    if (ret == -1)
        return E_FAIL;

    *pi = ret;
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_SetOverlayImage(IImageList2 *iface,
    int iImage, int iOverlay)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_SetOverlayImage(imgl, iImage, iOverlay) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_Replace(IImageList2 *iface, int i,
    HBITMAP hbmImage, HBITMAP hbmMask)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_Replace(imgl, i, hbmImage, hbmMask) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_AddMasked(IImageList2 *iface, HBITMAP hbmImage,
    COLORREF crMask, int *pi)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    int ret;

    if (!pi)
        return E_FAIL;

    ret = ImageList_AddMasked(imgl, hbmImage, crMask);

    if (ret == -1)
        return E_FAIL;

    *pi = ret;
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_Draw(IImageList2 *iface,
    IMAGELISTDRAWPARAMS *pimldp)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    HIMAGELIST old_himl;
    int ret;

    /* As far as I can tell, Windows simply ignores the contents of pimldp->himl
       so we shall simulate the same */
    old_himl = pimldp->himl;
    pimldp->himl = imgl;

    ret = ImageList_DrawIndirect(pimldp);

    pimldp->himl = old_himl;
    return ret ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI ImageListImpl_Remove(IImageList2 *iface, int i)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return (ImageList_Remove(imgl, i) == 0) ? E_INVALIDARG : S_OK;
}

static HRESULT WINAPI ImageListImpl_GetIcon(IImageList2 *iface, int i, UINT flags,
    HICON *picon)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    HICON hIcon;

    if (!picon)
        return E_FAIL;

    hIcon = ImageList_GetIcon(imgl, i, flags);

    if (hIcon == NULL)
        return E_FAIL;

    *picon = hIcon;
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_GetImageInfo(IImageList2 *iface, int i,
    IMAGEINFO *pImageInfo)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_GetImageInfo(imgl, i, pImageInfo) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_Copy(IImageList2 *iface, int dst_index,
    IUnknown *unk_src, int src_index, UINT flags)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    IImageList *src = NULL;
    HRESULT ret;

    if (!unk_src)
        return E_FAIL;

#ifdef __REACTOS__
    /* Make sure that the second image list uses the same implementation with the first */
    if (!is_valid2((HIMAGELIST)unk_src))
        return E_FAIL;
#endif

    /* TODO: Add test for IID_ImageList2 too */
    if (FAILED(IUnknown_QueryInterface(unk_src, &IID_IImageList,
            (void **) &src)))
        return E_FAIL;

    if (ImageList_Copy(imgl, dst_index, (HIMAGELIST) src, src_index, flags))
        ret = S_OK;
    else
        ret = E_FAIL;

    IImageList_Release(src);
    return ret;
}

static HRESULT WINAPI ImageListImpl_Merge(IImageList2 *iface, int i1,
    IUnknown *punk2, int i2, int dx, int dy, REFIID riid, void **ppv)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    IImageList *iml2 = NULL;
    HIMAGELIST merged;
    HRESULT ret = E_FAIL;

    TRACE("(%p)->(%d %p %d %d %d %s %p)\n", iface, i1, punk2, i2, dx, dy, debugstr_guid(riid), ppv);

#ifdef __REACTOS__
    /* Make sure that the second image list uses the same implementation with the first */
    if (!is_valid2((HIMAGELIST)punk2))
        return E_FAIL;
#endif

    /* TODO: Add test for IID_ImageList2 too */
    if (FAILED(IUnknown_QueryInterface(punk2, &IID_IImageList,
            (void **) &iml2)))
        return E_FAIL;

    merged = ImageList_Merge(imgl, i1, (HIMAGELIST) iml2, i2, dx, dy);

    /* Get the interface for the new image list */
    if (merged)
    {
        ret = HIMAGELIST_QueryInterface(merged, riid, ppv);
        ImageList_Destroy(merged);
    }

    IImageList_Release(iml2);
    return ret;
}

static HRESULT WINAPI ImageListImpl_Clone(IImageList2 *iface, REFIID riid, void **ppv)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    HIMAGELIST clone;
    HRESULT ret = E_FAIL;

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    clone = ImageList_Duplicate(imgl);

    /* Get the interface for the new image list */
    if (clone)
    {
        ret = HIMAGELIST_QueryInterface(clone, riid, ppv);
        ImageList_Destroy(clone);
    }

    return ret;
}

static HRESULT WINAPI ImageListImpl_GetImageRect(IImageList2 *iface, int i,
    RECT *prc)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    IMAGEINFO info;

    if (!prc)
        return E_FAIL;

    if (!ImageList_GetImageInfo(imgl, i, &info))
        return E_FAIL;

    *prc = info.rcImage;

    return S_OK;
}

static HRESULT WINAPI ImageListImpl_GetIconSize(IImageList2 *iface, int *cx,
    int *cy)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_GetIconSize(imgl, cx, cy) ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI ImageListImpl_SetIconSize(IImageList2 *iface, int cx,
    int cy)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_SetIconSize(imgl, cx, cy) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_GetImageCount(IImageList2 *iface, int *pi)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    *pi = ImageList_GetImageCount(imgl);
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_SetImageCount(IImageList2 *iface, UINT count)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_SetImageCount(imgl, count) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_SetBkColor(IImageList2 *iface, COLORREF clrBk,
    COLORREF *pclr)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    *pclr = ImageList_SetBkColor(imgl, clrBk);
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_GetBkColor(IImageList2 *iface, COLORREF *pclr)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    *pclr = ImageList_GetBkColor(imgl);
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_BeginDrag(IImageList2 *iface, int iTrack,
    int dxHotspot, int dyHotspot)
{
    HIMAGELIST imgl = impl_from_IImageList2(iface);
    return ImageList_BeginDrag(imgl, iTrack, dxHotspot, dyHotspot) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_EndDrag(IImageList2 *iface)
{
    ImageList_EndDrag();
    return S_OK;
}

static HRESULT WINAPI ImageListImpl_DragEnter(IImageList2 *iface, HWND hwndLock,
    int x, int y)
{
    return ImageList_DragEnter(hwndLock, x, y) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_DragLeave(IImageList2 *iface, HWND hwndLock)
{
    return ImageList_DragLeave(hwndLock) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_DragMove(IImageList2 *iface, int x, int y)
{
    return ImageList_DragMove(x, y) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_SetDragCursorImage(IImageList2 *iface,
    IUnknown *punk, int iDrag, int dxHotspot, int dyHotspot)
{
    IImageList *iml2 = NULL;
    BOOL ret;

    if (!punk)
        return E_FAIL;

    /* TODO: Add test for IID_ImageList2 too */
    if (FAILED(IUnknown_QueryInterface(punk, &IID_IImageList,
            (void **) &iml2)))
        return E_FAIL;

    ret = ImageList_SetDragCursorImage((HIMAGELIST) iml2, iDrag, dxHotspot,
        dyHotspot);

    IImageList_Release(iml2);

    return ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_DragShowNolock(IImageList2 *iface, BOOL fShow)
{
    return ImageList_DragShowNolock(fShow) ? S_OK : E_FAIL;
}

static HRESULT WINAPI ImageListImpl_GetDragImage(IImageList2 *iface, POINT *ppt,
    POINT *pptHotspot, REFIID riid, PVOID *ppv)
{
    HRESULT ret = E_FAIL;
    HIMAGELIST hNew;

    if (!ppv)
        return E_FAIL;

    hNew = ImageList_GetDragImage(ppt, pptHotspot);

    /* Get the interface for the new image list */
    if (hNew)
    {
        IImageList *idrag = (IImageList*)hNew;

        ret = HIMAGELIST_QueryInterface(hNew, riid, ppv);
        IImageList_Release(idrag);
    }

    return ret;
}

static HRESULT WINAPI ImageListImpl_GetItemFlags(IImageList2 *iface, int i,
    DWORD *dwFlags)
{
    FIXME("STUB: %p %d %p\n", iface, i, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_GetOverlayImage(IImageList2 *iface, int iOverlay,
    int *piIndex)
{
    HIMAGELIST This = impl_from_IImageList2(iface);
    int i;

    if ((iOverlay < 0) || (iOverlay > This->cCurImage))
        return E_FAIL;

    for (i = 0; i < MAX_OVERLAYIMAGE; i++)
    {
        if (This->nOvlIdx[i] == iOverlay)
        {
            *piIndex = i + 1;
            return S_OK;
        }
    }

    return E_FAIL;
}

static HRESULT WINAPI ImageListImpl_Resize(IImageList2 *iface, INT cx, INT cy)
{
    FIXME("(%p)->(%d %d): stub\n", iface, cx, cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_GetOriginalSize(IImageList2 *iface, INT image, DWORD flags, INT *cx, INT *cy)
{
    FIXME("(%p)->(%d %x %p %p): stub\n", iface, image, flags, cx, cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_SetOriginalSize(IImageList2 *iface, INT image, INT cx, INT cy)
{
    FIXME("(%p)->(%d %d %d): stub\n", iface, image, cx, cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_SetCallback(IImageList2 *iface, IUnknown *callback)
{
    FIXME("(%p)->(%p): stub\n", iface, callback);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_GetCallback(IImageList2 *iface, REFIID riid, void **ppv)
{
    FIXME("(%p)->(%s %p): stub\n", iface, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_ForceImagePresent(IImageList2 *iface, INT image, DWORD flags)
{
    FIXME("(%p)->(%d %x): stub\n", iface, image, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_DiscardImages(IImageList2 *iface, INT first_image, INT last_image, DWORD flags)
{
    FIXME("(%p)->(%d %d %x): stub\n", iface, first_image, last_image, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_PreloadImages(IImageList2 *iface, IMAGELISTDRAWPARAMS *params)
{
    FIXME("(%p)->(%p): stub\n", iface, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_GetStatistics(IImageList2 *iface, IMAGELISTSTATS *stats)
{
    FIXME("(%p)->(%p): stub\n", iface, stats);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_Initialize(IImageList2 *iface, INT cx, INT cy, UINT flags, INT initial, INT grow)
{
    FIXME("(%p)->(%d %d %d %d %d): stub\n", iface, cx, cy, flags, initial, grow);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_Replace2(IImageList2 *iface, INT i, HBITMAP image, HBITMAP mask, IUnknown *unk, DWORD flags)
{
    FIXME("(%p)->(%d %p %p %p %x): stub\n", iface, i, image, mask, unk, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImageListImpl_ReplaceFromImageList(IImageList2 *iface, INT i, IImageList *imagelist, INT src,
    IUnknown *unk, DWORD flags)
{
    FIXME("(%p)->(%d %p %d %p %x): stub\n", iface, i, imagelist, src, unk, flags);
    return E_NOTIMPL;
}

static const IImageList2Vtbl ImageListImpl_Vtbl = {
    ImageListImpl_QueryInterface,
    ImageListImpl_AddRef,
    ImageListImpl_Release,
    ImageListImpl_Add,
    ImageListImpl_ReplaceIcon,
    ImageListImpl_SetOverlayImage,
    ImageListImpl_Replace,
    ImageListImpl_AddMasked,
    ImageListImpl_Draw,
    ImageListImpl_Remove,
    ImageListImpl_GetIcon,
    ImageListImpl_GetImageInfo,
    ImageListImpl_Copy,
    ImageListImpl_Merge,
    ImageListImpl_Clone,
    ImageListImpl_GetImageRect,
    ImageListImpl_GetIconSize,
    ImageListImpl_SetIconSize,
    ImageListImpl_GetImageCount,
    ImageListImpl_SetImageCount,
    ImageListImpl_SetBkColor,
    ImageListImpl_GetBkColor,
    ImageListImpl_BeginDrag,
    ImageListImpl_EndDrag,
    ImageListImpl_DragEnter,
    ImageListImpl_DragLeave,
    ImageListImpl_DragMove,
    ImageListImpl_SetDragCursorImage,
    ImageListImpl_DragShowNolock,
    ImageListImpl_GetDragImage,
    ImageListImpl_GetItemFlags,
    ImageListImpl_GetOverlayImage,
    ImageListImpl_Resize,
    ImageListImpl_GetOriginalSize,
    ImageListImpl_SetOriginalSize,
    ImageListImpl_SetCallback,
    ImageListImpl_GetCallback,
    ImageListImpl_ForceImagePresent,
    ImageListImpl_DiscardImages,
    ImageListImpl_PreloadImages,
    ImageListImpl_GetStatistics,
    ImageListImpl_Initialize,
    ImageListImpl_Replace2,
    ImageListImpl_ReplaceFromImageList
};

static BOOL is_valid(HIMAGELIST himl)
{
    BOOL valid;
    __TRY
    {
    #ifdef __REACTOS__
        valid = himl && himl->usMagic == IMAGELIST_MAGIC;
        if (!valid && himl && himl->usMagic == IMAGELIST_MAGIC_DESTROYED)
        {
            ERR("Imagelist no longer valid: 0x%p\n", himl);
        }
    #else
        valid = himl && himl->IImageList2_iface.lpVtbl == &ImageListImpl_Vtbl;
    #endif

    }
    __EXCEPT_PAGE_FAULT
    {
        valid = FALSE;
    }
    __ENDTRY
    return valid;
}

/*************************************************************************
 * HIMAGELIST_QueryInterface [COMCTL32.@]
 *
 * Returns a pointer to an IImageList or IImageList2 object for the given
 * HIMAGELIST.
 *
 * PARAMS
 *     himl        [I] Image list handle.
 *     riid        [I] Identifier of the requested interface.
 *     ppv         [O] Returns the address of the pointer requested, or NULL.
 *
 * RETURNS
 *     Success: S_OK.
 *     Failure: Error value.
 */
HRESULT WINAPI
HIMAGELIST_QueryInterface (HIMAGELIST himl, REFIID riid, void **ppv)
{
    TRACE("(%p,%s,%p)\n", himl, debugstr_guid(riid), ppv);
    return IImageList2_QueryInterface((IImageList2 *) himl, riid, ppv);
}

static HRESULT ImageListImpl_CreateInstance(const IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    HIMAGELIST This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = heap_alloc_zero(sizeof(struct _IMAGELIST));
    if (!This) return E_OUTOFMEMORY;

    This->IImageList2_iface.lpVtbl = &ImageListImpl_Vtbl;
#ifdef __REACTOS__
    This->usMagic = IMAGELIST_MAGIC;
#endif
    This->ref = 1;

    ret = IImageList2_QueryInterface(&This->IImageList2_iface, iid, ppv);
    IImageList2_Release(&This->IImageList2_iface);

    return ret;
}



#ifdef __REACTOS__
//The big bad reactos image list hack!
#undef ImageList_Add
#undef ImageList_ReplaceIcon
#undef ImageList_SetOverlayImage
#undef ImageList_Replace
#undef ImageList_AddMasked
#undef ImageList_Remove
#undef ImageList_GetIcon
#undef ImageList_GetImageInfo
#undef ImageList_Copy
#undef ImageList_Merge
#undef ImageList_Duplicate
#undef ImageList_GetIconSize
#undef ImageList_SetIconSize
#undef ImageList_GetImageCount
#undef ImageList_SetImageCount
#undef ImageList_SetBkColor
#undef ImageList_GetBkColor
#undef ImageList_BeginDrag
#undef ImageList_DrawIndirect

static inline IImageList2* IImageList2_from_impl(HIMAGELIST himl)
{
    if (is_valid(himl))
    {
        return &himl->IImageList2_iface;
    }
    return NULL;
}

BOOL is_valid2(HIMAGELIST himl)
{
    BOOL valid;
    __TRY
    {
        valid = himl && 
                himl->IImageList2_iface.lpVtbl == &ImageListImpl_Vtbl && 
                himl->usMagic == IMAGELIST_MAGIC;
        if (!valid && himl &&
            himl->usMagic == IMAGELIST_MAGIC_DESTROYED)
        {
            ERR("Imagelist no longer valid: 0x%p\n", himl);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        valid = FALSE;
    }
    __ENDTRY
    return valid;
}

INT WINAPI
ImageList_Add (HIMAGELIST himl,	HBITMAP hbmImage, HBITMAP hbmMask)
{
    int res;
    HRESULT hr;
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return -1;

    hr = piml->lpVtbl->Add(piml, hbmImage, hbmMask, &res);
    if (FAILED(hr))
        return -1;

    return res;
}

INT WINAPI
ImageList_ReplaceIcon (HIMAGELIST himl, INT nIndex, HICON hIcon)
{
    int res;
    HRESULT hr;
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return -1;

    hr = piml->lpVtbl->ReplaceIcon(piml, nIndex, hIcon, &res);
    if (FAILED(hr))
        return -1;

    return res;
}

BOOL WINAPI
ImageList_SetOverlayImage (HIMAGELIST himl, INT iImage, INT iOverlay)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->SetOverlayImage(piml, iImage, iOverlay) == S_OK) ? TRUE : FALSE;
}

BOOL WINAPI
ImageList_Replace (HIMAGELIST himl, INT i, HBITMAP hbmImage,
		   HBITMAP hbmMask)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->Replace(piml, i, hbmImage, hbmMask) == S_OK) ? TRUE : FALSE;
}

INT WINAPI
ImageList_AddMasked (HIMAGELIST himl, HBITMAP hBitmap, COLORREF clrMask)
{
    int res;
    IImageList2* piml = IImageList2_from_impl(himl);
    HRESULT hr;
    if (!piml)
        return -1;

    hr = piml->lpVtbl->AddMasked(piml, hBitmap, clrMask, &res);
    if (FAILED(hr))
        return -1;

    return res;
}

BOOL WINAPI
ImageList_Remove (HIMAGELIST himl, INT i)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->Remove(piml, i) == S_OK) ? TRUE : FALSE;
}

HICON WINAPI
ImageList_GetIcon (HIMAGELIST himl, INT i, UINT fStyle)
{
    HICON res;
    IImageList2* piml = IImageList2_from_impl(himl);
    HRESULT hr;
    if (!piml)
        return NULL;

    hr = piml->lpVtbl->GetIcon(piml, i, fStyle, &res);
    if (FAILED(hr))
        return NULL;

    return res;
}

BOOL WINAPI
ImageList_GetImageInfo (HIMAGELIST himl, INT i, IMAGEINFO *pImageInfo)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->GetImageInfo(piml, i, pImageInfo) == S_OK) ? TRUE : FALSE;
}

BOOL WINAPI
ImageList_Copy (HIMAGELIST himlDst, INT iDst,	HIMAGELIST himlSrc,
		INT iSrc, UINT uFlags)
{
    IImageList2 *pimlDst, *pimlSrc;
    pimlDst = IImageList2_from_impl(himlDst);
    pimlSrc = IImageList2_from_impl(himlSrc);
    if (!pimlDst || !pimlSrc)
        return FALSE;

    return (pimlDst->lpVtbl->Copy(pimlDst, iDst, (IUnknown*)pimlSrc, iSrc, uFlags) == S_OK) ? TRUE : FALSE;
}

HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT i1, HIMAGELIST himl2, INT i2,
		 INT dx, INT dy)
{
    HRESULT hr;
    IImageList2 *piml1, *piml2, *pimlMerged;
    piml1 = IImageList2_from_impl(himl1);
    piml2 = IImageList2_from_impl(himl2);
    if (!piml1 || !piml2)
        return NULL;

    hr = piml1->lpVtbl->Merge(piml1, i1, (IUnknown*)piml2, i2, dx, dy, &IID_IImageList2, (void**)&pimlMerged);
    if (FAILED(hr))
        return NULL;

    return (HIMAGELIST)pimlMerged;
}

HIMAGELIST WINAPI
ImageList_Duplicate (HIMAGELIST himlSrc)
{
    HRESULT hr;
    IImageList2 *piml, *pimlCloned;
    piml = IImageList2_from_impl(himlSrc);
    if (!piml)
        return FALSE;

    hr = piml->lpVtbl->Clone(piml, &IID_IImageList2, (void**)&pimlCloned);
    if (FAILED(hr))
        return NULL;

    return (HIMAGELIST)pimlCloned;
}

BOOL WINAPI
ImageList_GetIconSize (HIMAGELIST himl, INT *cx, INT *cy)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->GetIconSize(piml, cx, cy) == S_OK) ? TRUE : FALSE;
}

BOOL WINAPI
ImageList_SetIconSize (HIMAGELIST himl, INT cx, INT cy)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->SetIconSize(piml, cx, cy) == S_OK) ? TRUE : FALSE;
}

INT WINAPI
ImageList_GetImageCount (HIMAGELIST himl)
{
    int res;
    HRESULT hr;
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return 0;

    hr = piml->lpVtbl->GetImageCount(piml, &res);
    if (FAILED(hr))
        return 0;

    return res;
}

BOOL WINAPI
ImageList_SetImageCount (HIMAGELIST himl, UINT iImageCount)
{
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->SetImageCount(piml, iImageCount) == S_OK) ? TRUE : FALSE;
}

COLORREF WINAPI
ImageList_SetBkColor (HIMAGELIST himl, COLORREF clrBk)
{
    COLORREF res;
    HRESULT hr;
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return CLR_NONE;

    hr = piml->lpVtbl->SetBkColor(piml, clrBk, &res);
    if (FAILED(hr))
        return CLR_NONE;

    return res;
}

COLORREF WINAPI
ImageList_GetBkColor (HIMAGELIST himl)
{
    COLORREF res;
    HRESULT hr;
    IImageList2* piml = IImageList2_from_impl(himl);
    if (!piml)
        return CLR_NONE;

    hr = piml->lpVtbl->GetBkColor(piml, &res);
    if (FAILED(hr))
        return CLR_NONE;

    return res;
}

BOOL WINAPI
ImageList_BeginDrag (HIMAGELIST himlTrack, INT iTrack,
	             INT dxHotspot, INT dyHotspot)
{
    IImageList2* piml = IImageList2_from_impl(himlTrack);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->BeginDrag(piml, iTrack, dxHotspot, dyHotspot) == S_OK) ? TRUE : FALSE;
}

BOOL WINAPI
ImageList_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp)
{
    IImageList2* piml;

    if (!pimldp)
        return FALSE;

    piml = IImageList2_from_impl(pimldp->himl);
    if (!piml)
        return FALSE;

    return (piml->lpVtbl->Draw(piml, pimldp) == S_OK) ? TRUE : FALSE;
}

#endif
