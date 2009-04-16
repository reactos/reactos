/*
 *  ImageList implementation
 *
 *  Copyright 1998 Eric Kohl
 *  Copyright 2000 Jason Mawdsley
 *  Copyright 2001, 2004 Michael Stefaniuc
 *  Copyright 2001 Charles Loep for CodeWeavers
 *  Copyright 2002 Dimitrie O. Paun
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
 * NOTE
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 12, 2002, by Dimitrie O. Paun.
 *
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 *  TODO:
 *    - Add support for ILD_PRESERVEALPHA, ILD_SCALE, ILD_DPISCALE
 *    - Add support for ILS_GLOW, ILS_SHADOW, ILS_SATURATE, ILS_ALPHA
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
#include "imagelist.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagelist);


#define MAX_OVERLAYIMAGE 15

/* internal image list data used for Drag & Drop operations */
typedef struct
{
    HWND	hwnd;
    HIMAGELIST	himl;
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

static INTERNALDRAG InternalDrag = { 0, 0, 0, 0, 0, 0, FALSE, 0 };

static HBITMAP ImageList_CreateImage(HDC hdc, HIMAGELIST himl, UINT count, UINT width);

static inline BOOL is_valid(HIMAGELIST himl)
{
    return himl && himl->magic == IMAGELIST_MAGIC;
}

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

static inline void imagelist_get_bitmap_size( HIMAGELIST himl, UINT count, UINT cx, SIZE *sz )
{
    sz->cx = cx * TILE_COUNT;
    sz->cy = imagelist_height( count ) * himl->cy;
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
IMAGELIST_InternalExpandBitmaps (HIMAGELIST himl, INT nImageCount, INT cy)
{
    HDC     hdcBitmap;
    HBITMAP hbmNewBitmap, hbmNull;
    INT     nNewCount;
    SIZE    sz;

    if ((himl->cCurImage + nImageCount <= himl->cMaxImage)
        && (himl->cy >= cy))
	return;

    nNewCount = himl->cCurImage + nImageCount + himl->cGrow;

    imagelist_get_bitmap_size(himl, nNewCount, himl->cx, &sz);

    TRACE("Create expanded bitmaps : himl=%p x=%d y=%d count=%d\n", himl, sz.cx, cy, nNewCount);
    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap = ImageList_CreateImage(hdcBitmap, himl, nNewCount, himl->cx);

    if (hbmNewBitmap == 0)
        ERR("creating new image bitmap (x=%d y=%d)!\n", sz.cx, cy);

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
    HDC     hdcBitmap, hdcTemp;
    INT     nFirstIndex, nImageCount, i;
    BITMAP  bmp;
    HBITMAP hOldBitmap, hOldBitmapTemp;
    POINT   pt;

    TRACE("himl=%p hbmimage=%p hbmmask=%p\n", himl, hbmImage, hbmMask);
    if (!is_valid(himl))
        return -1;

    if (!GetObjectW(hbmImage, sizeof(BITMAP), &bmp))
        return -1;

    nImageCount = bmp.bmWidth / himl->cx;

    IMAGELIST_InternalExpandBitmaps (himl, nImageCount, bmp.bmHeight);

    hdcBitmap = CreateCompatibleDC(0);

    hOldBitmap = SelectObject(hdcBitmap, hbmImage);

    for (i=0; i<nImageCount; i++)
    {
        imagelist_point_from_index( himl, himl->cCurImage + i, &pt );

        /* Copy result to the imagelist
        */
        BitBlt( himl->hdcImage, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcBitmap, i*himl->cx, 0, SRCCOPY );

        if (!himl->hbmMask)
             continue;

        hdcTemp   = CreateCompatibleDC(0);
        hOldBitmapTemp = SelectObject(hdcTemp, hbmMask);

        BitBlt( himl->hdcMask, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcTemp, i*himl->cx, 0, SRCCOPY );

        SelectObject(hdcTemp, hOldBitmapTemp);
        DeleteDC(hdcTemp);

        /* Remove the background from the image
        */
        BitBlt( himl->hdcImage, pt.x, pt.y, himl->cx, bmp.bmHeight,
                himl->hdcMask, pt.x, pt.y, 0x220326 ); /* NOTSRCAND */
    }

    SelectObject(hdcBitmap, hOldBitmap);
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
    INT    i, nIndex, nImageCount;
    BITMAP bmp;
    HBITMAP hOldBitmap;
    HBITMAP hMaskBitmap=0;
    COLORREF bkColor;
    POINT  pt;

    TRACE("himl=%p hbitmap=%p clrmask=%x\n", himl, hBitmap, clrMask);
    if (!is_valid(himl))
        return -1;

    if (!GetObjectW(hBitmap, sizeof(BITMAP), &bmp))
        return -1;

    if (himl->cx > 0)
	nImageCount = bmp.bmWidth / himl->cx;
    else
	nImageCount = 0;

    IMAGELIST_InternalExpandBitmaps (himl, nImageCount, bmp.bmHeight);

    nIndex = himl->cCurImage;
    himl->cCurImage += nImageCount;

    hdcBitmap = CreateCompatibleDC(0);
    hOldBitmap = SelectObject(hdcBitmap, hBitmap);

    /* Create a temp Mask so we can remove the background of the Image */
    hdcMask = CreateCompatibleDC(0);
    hMaskBitmap = CreateBitmap(bmp.bmWidth, bmp.bmHeight, 1, 1, NULL);
    SelectObject(hdcMask, hMaskBitmap);

    /* create monochrome image to the mask bitmap */
    bkColor = (clrMask != CLR_DEFAULT) ? clrMask : GetPixel (hdcBitmap, 0, 0);
    SetBkColor (hdcBitmap, bkColor);
    BitBlt (hdcMask, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcBitmap, 0, 0, SRCCOPY);

    SetBkColor(hdcBitmap, RGB(255,255,255));

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
    BitBlt(hdcBitmap, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMask, 0, 0, 0x220326);

    /* Copy result to the imagelist */
    for (i=0; i<nImageCount; i++)
    {
        imagelist_point_from_index( himl, nIndex + i, &pt );
        BitBlt(himl->hdcImage, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcBitmap, i*himl->cx, 0, SRCCOPY);
        BitBlt(himl->hdcMask, pt.x, pt.y, himl->cx, bmp.bmHeight,
                hdcMask, i*himl->cx, 0, SRCCOPY);
    }

    /* Clean up */
    SelectObject(hdcBitmap, hOldBitmap);
    DeleteDC(hdcBitmap);
    DeleteObject(hMaskBitmap);
    DeleteDC(hdcMask);

    return nIndex;
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

    TRACE("(himlTrack=%p iTrack=%d dx=%d dy=%d)\n", himlTrack, iTrack,
	  dxHotspot, dyHotspot);

    if (!is_valid(himlTrack))
	return FALSE;

    if (InternalDrag.himl)
        ImageList_EndDrag ();

    cx = himlTrack->cx;
    cy = himlTrack->cy;

    InternalDrag.himl = ImageList_Create (cx, cy, himlTrack->flags, 1, 1);
    if (InternalDrag.himl == NULL) {
        WARN("Error creating drag image list!\n");
        return FALSE;
    }

    InternalDrag.dxHotspot = dxHotspot;
    InternalDrag.dyHotspot = dyHotspot;

    /* copy image */
    BitBlt (InternalDrag.himl->hdcImage, 0, 0, cx, cy, himlTrack->hdcImage, iTrack * cx, 0, SRCCOPY);

    /* copy mask */
    BitBlt (InternalDrag.himl->hdcMask, 0, 0, cx, cy, himlTrack->hdcMask, iTrack * cx, 0, SRCCOPY);

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

    himl = Alloc (sizeof(struct _IMAGELIST));
    if (!himl)
        return NULL;

    cGrow = (cGrow < 4) ? 4 : (cGrow + 3) & ~3;

    himl->magic     = IMAGELIST_MAGIC;
    himl->cx        = cx;
    himl->cy        = cy;
    himl->flags     = flags;
    himl->cMaxImage = cInitial + 1;
    himl->cInitial  = cInitial;
    himl->cGrow     = cGrow;
    himl->clrFg     = CLR_DEFAULT;
    himl->clrBk     = CLR_NONE;

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
        ilc = ILC_COLOR4;

    if (ilc >= ILC_COLOR4 && ilc <= ILC_COLOR32)
        himl->uBitsPixel = ilc;
    else
        himl->uBitsPixel = (UINT)GetDeviceCaps (himl->hdcImage, BITSPIXEL);

    if (himl->cMaxImage > 0) {
        himl->hbmImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage, cx);
	SelectObject(himl->hdcImage, himl->hbmImage);
    } else
        himl->hbmImage = 0;

    if ((himl->cMaxImage > 0) && (himl->flags & ILC_MASK)) {
        SIZE sz;

        imagelist_get_bitmap_size(himl, himl->cMaxImage, himl->cx, &sz);
        himl->hbmMask = CreateBitmap (sz.cx, sz.cy, 1, 1, NULL);
        if (himl->hbmMask == 0) {
            ERR("Error creating mask bitmap!\n");
            goto cleanup;
        }
        SelectObject(himl->hdcMask, himl->hbmMask);
    }
    else
        himl->hbmMask = 0;

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
    if (himl) ImageList_Destroy(himl);
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

    /* delete image bitmaps */
    if (himl->hbmImage)
        DeleteObject (himl->hbmImage);
    if (himl->hbmMask)
        DeleteObject (himl->hbmMask);

    /* delete image & mask DCs */
    if (himl->hdcImage)
        DeleteDC(himl->hdcImage);
    if (himl->hdcMask)
        DeleteDC(himl->hdcMask);

    /* delete blending brushes */
    if (himl->hbrBlend25)
        DeleteObject (himl->hbrBlend25);
    if (himl->hbrBlend50)
        DeleteObject (himl->hbrBlend50);

    ZeroMemory(himl, sizeof(*himl));
    Free (himl);

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
    if (!ImageList_DragShowNolock(TRUE)) {
	return FALSE;
    }

    return TRUE;
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
    imldp.hdcDst  = hdc,
    imldp.x       = x;
    imldp.y       = y;
    imldp.rgbBk   = CLR_DEFAULT;
    imldp.rgbFg   = CLR_DEFAULT;
    imldp.fStyle  = ILD_NORMAL;
    imldp.fState  = ILS_ALPHA;
    imldp.Frame   = 128;

    /* FIXME: instead of using the alpha blending, we should
     * create a 50% mask, and draw it semitransparantly that way */
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
 *
 * BUGS
 *     The drag image should be drawn semitransparent.
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
 *
 * BUGS
 *     The drag image should be drawn semitransparent.
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
 * Draws an image and allows to use extended drawing features.
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
    imldp.hdcDst  = hdc,
    imldp.x       = x;
    imldp.y       = y;
    imldp.cx      = dx;
    imldp.cy      = dy;
    imldp.rgbBk   = rgbBk;
    imldp.rgbFg   = rgbFg;
    imldp.fStyle  = fStyle;

    return ImageList_DrawIndirect (&imldp);
}


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
    POINT pt;

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
     * to white and foreground color to black when blting the
     * monochrome mask.
     */
    oldImageFg = SetTextColor( hImageDC, RGB( 0, 0, 0 ) );
    oldImageBk = SetBkColor( hImageDC, RGB( 0xff, 0xff, 0xff ) );

    /*
     * Draw the initial image
     */
    if( bMask ) {
	if (himl->hbmMask) {
            HBRUSH hOldBrush;
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
	    HBRUSH hOldBrush = SelectObject (hImageDC, GetStockObject(BLACK_BRUSH));
	    PatBlt( hImageDC, 0, 0, cx, cy, PATCOPY);
	    SelectObject(hImageDC, hOldBrush);
	}
    } else {
	/* blend the image with the needed solid background */
        COLORREF colour = RGB(0,0,0);
        HBRUSH hOldBrush;

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
	HBRUSH hBlendBrush, hOldBrush;
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

    if (fState & ILS_SATURATE) FIXME("ILS_SATURATE: unimplemented!\n");
    if (fState & ILS_GLOW) FIXME("ILS_GLOW: unimplemented!\n");
    if (fState & ILS_SHADOW) FIXME("ILS_SHADOW: unimplemented!\n");
    if (fState & ILS_ALPHA) FIXME("ILS_ALPHA: unimplemented!\n");

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
                                himlSrc->cInitial, himlSrc->cGrow);

    if (himlDst)
    {
        SIZE sz;

        imagelist_get_bitmap_size(himlSrc, himlSrc->cCurImage, himlSrc->cx, &sz);
        BitBlt (himlDst->hdcImage, 0, 0, sz.cx, sz.cy,
                himlSrc->hdcImage, 0, 0, SRCCOPY);

        if (himlDst->hbmMask)
            BitBlt (himlDst->hdcMask, 0, 0, sz.cx, sz.cy,
                    himlSrc->hdcMask, 0, 0, SRCCOPY);

	himlDst->cCurImage = himlSrc->cCurImage;
	himlDst->cMaxImage = himlSrc->cMaxImage;
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
    ImageList_Destroy (InternalDrag.himl);
    InternalDrag.himl = 0;
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

    return is_valid(himl) ? himl->flags : 0;
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
    if (!is_valid(himl))
	return FALSE;
    if ((himl->cx <= 0) || (himl->cy <= 0))
	return FALSE;

    if (cx)
	*cx = himl->cx;
    if (cy)
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

    if (!HIWORD(lpbmp))
        return ImageList_LoadImageW(hi, (LPCWSTR)lpbmp, cx, cGrow, clrMask,
                                    uType, uFlags);

    len = MultiByteToWideChar(CP_ACP, 0, lpbmp, -1, NULL, 0);
    lpbmpW = Alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpbmp, -1, lpbmpW, len);

    himl = ImageList_LoadImageW(hi, lpbmpW, cx, cGrow, clrMask, uType, uFlags);
    Free (lpbmpW);
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
        BITMAP bmp;
        GetObjectW (handle, sizeof(BITMAP), &bmp);

        /* To match windows behavior, if cx is set to zero and
         the flag DI_DEFAULTSIZE is specified, cx becomes the
         system metric value for icons. If the flag is not specified
         the function sets the size to the height of the bitmap */
        if (cx == 0)
        {
            if (uFlags & DI_DEFAULTSIZE)
                cx = GetSystemMetrics (SM_CXICON);
            else
                cx = bmp.bmHeight;
        }

        nImageCount = bmp.bmWidth / cx;

        himl = ImageList_Create (cx, bmp.bmHeight, ILC_MASK | ILC_COLOR,
                                 nImageCount, cGrow);
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
 *     Failure: NULL, if either himl1 or himl2 are invalid.
 *
 * NOTES
 *   - The returned image list should be deleted by the caller using
 *     ImageList_Destroy() when it is no longer required.
 *   - If either i1 or i2 are not valid image indices they will be treated
 *     as a blank image.
 */
HIMAGELIST WINAPI
ImageList_Merge (HIMAGELIST himl1, INT i1, HIMAGELIST himl2, INT i2,
		 INT dx, INT dy)
{
    HIMAGELIST himlDst = NULL;
    INT      cxDst, cyDst;
    INT      xOff1, yOff1, xOff2, yOff2;
    POINT    pt1, pt2;

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

    himlDst = ImageList_Create (cxDst, cyDst, ILC_MASK | ILC_COLOR, 1, 1);

    if (himlDst)
    {
        imagelist_point_from_index( himl1, i1, &pt1 );
        imagelist_point_from_index( himl1, i2, &pt2 );

        /* copy image */
        BitBlt (himlDst->hdcImage, 0, 0, cxDst, cyDst, himl1->hdcImage, 0, 0, BLACKNESS);
        if (i1 >= 0 && i1 < himl1->cCurImage)
            BitBlt (himlDst->hdcImage, xOff1, yOff1, himl1->cx, himl1->cy, himl1->hdcImage, pt1.x, pt1.y, SRCCOPY);
        if (i2 >= 0 && i2 < himl2->cCurImage)
        {
            BitBlt (himlDst->hdcImage, xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcMask , pt2.x, pt2.y, SRCAND);
            BitBlt (himlDst->hdcImage, xOff2, yOff2, himl2->cx, himl2->cy, himl2->hdcImage, pt2.x, pt2.y, SRCPAINT);
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


/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 */
static int DIB_GetDIBWidthBytes( int width, int depth )
{
    int words;

    switch(depth)
    {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;

    default:
        WARN("(%d): Unsupported depth\n", depth );
        /* fall through */
    case 32:
        words = width;
        break;
    }
    return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 */
static int DIB_GetDIBImageBytes( int width, int height, int depth )
{
    return DIB_GetDIBWidthBytes( width, depth ) * abs( height );
}


/* helper for ImageList_Read, see comments below */
static BOOL _read_bitmap(HDC hdcIml, LPSTREAM pstm)
{
    BITMAPFILEHEADER	bmfh;
    int bitsperpixel, palspace;
    char bmi_buf[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
    LPBITMAPINFO bmi = (LPBITMAPINFO)bmi_buf;
    int                result = FALSE;
    LPBYTE             bits = NULL;

    if (FAILED(IStream_Read ( pstm, &bmfh, sizeof(bmfh), NULL)))
        return FALSE;

    if (bmfh.bfType != (('M'<<8)|'B'))
        return FALSE;

    if (FAILED(IStream_Read ( pstm, &bmi->bmiHeader, sizeof(bmi->bmiHeader), NULL)))
        return FALSE;

    if ((bmi->bmiHeader.biSize != sizeof(bmi->bmiHeader)))
        return FALSE;

    TRACE("width %u, height %u, planes %u, bpp %u\n",
          bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
          bmi->bmiHeader.biPlanes, bmi->bmiHeader.biBitCount);

    bitsperpixel = bmi->bmiHeader.biPlanes * bmi->bmiHeader.biBitCount;
    if (bitsperpixel<=8)
        palspace = (1<<bitsperpixel)*sizeof(RGBQUAD);
    else
        palspace = 0;

    bmi->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, bitsperpixel);

    /* read the palette right after the end of the bitmapinfoheader */
    if (palspace && FAILED(IStream_Read(pstm, bmi->bmiColors, palspace, NULL)))
	goto error;

    bits = Alloc(bmi->bmiHeader.biSizeImage);
    if (!bits)
        goto error;
    if (FAILED(IStream_Read(pstm, bits, bmi->bmiHeader.biSizeImage, NULL)))
        goto error;

    if (!StretchDIBits(hdcIml, 0, 0, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
                  0, 0, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
                  bits, bmi, DIB_RGB_COLORS, SRCCOPY))
        goto error;
    result = TRUE;

error:
    Free(bits);
    return result;
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
HIMAGELIST WINAPI ImageList_Read (LPSTREAM pstm)
{
    ILHEAD	ilHead;
    HIMAGELIST	himl;
    int		i;

    TRACE("%p\n", pstm);

    if (FAILED(IStream_Read (pstm, &ilHead, sizeof(ILHEAD), NULL)))
	return NULL;
    if (ilHead.usMagic != (('L' << 8) | 'I'))
	return NULL;
    if (ilHead.usVersion != 0x101) /* probably version? */
	return NULL;

    TRACE("cx %u, cy %u, flags 0x%04x, cCurImage %u, cMaxImage %u\n",
          ilHead.cx, ilHead.cy, ilHead.flags, ilHead.cCurImage, ilHead.cMaxImage);

    himl = ImageList_Create(ilHead.cx, ilHead.cy, ilHead.flags, ilHead.cCurImage, ilHead.cMaxImage);
    if (!himl)
	return NULL;

    if (!_read_bitmap(himl->hdcImage, pstm))
    {
	WARN("failed to read bitmap from stream\n");
	return NULL;
    }
    if (ilHead.flags & ILC_MASK)
    {
        if (!_read_bitmap(himl->hdcMask, pstm))
        {
            WARN("failed to read mask bitmap from stream\n");
	    return NULL;
	}
    }

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

        himl->cMaxImage = himl->cInitial + himl->cGrow - 1;
        himl->cCurImage = 0;
        for (nCount = 0; nCount < MAX_OVERLAYIMAGE; nCount++)
             himl->nOvlIdx[nCount] = -1;

        hbmNewImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage, himl->cx);
        SelectObject (himl->hdcImage, hbmNewImage);
        DeleteObject (himl->hbmImage);
        himl->hbmImage = hbmNewImage;

        if (himl->hbmMask) {

            imagelist_get_bitmap_size(himl, himl->cMaxImage, himl->cx, &sz);
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

        hbmNewImage = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage, himl->cx);

        imagelist_get_bitmap_size(himl, himl->cMaxImage, himl->cx, &sz );
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
    HBITMAP hOldBitmap;
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
    hOldBitmap = SelectObject (hdcImage, hbmImage);

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

    SelectObject (hdcImage, hOldBitmap);
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
    HDC     hdcImage;
    HICON   hBestFitIcon;
    HBITMAP hbmOldSrc;
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

    ret = GetIconInfo (hBestFitIcon, &ii);
    if (!ret) {
        DestroyIcon(hBestFitIcon);
        return -1;
    }

    ret = GetObjectW (ii.hbmMask, sizeof(BITMAP), &bmp);
    if (!ret) {
        ERR("couldn't get mask bitmap info\n");
        if (ii.hbmColor)
            DeleteObject (ii.hbmColor);
        if (ii.hbmMask)
            DeleteObject (ii.hbmMask);
        DestroyIcon(hBestFitIcon);
        return -1;
    }

    if (nIndex == -1) {
        if (himl->cCurImage + 1 > himl->cMaxImage)
            IMAGELIST_InternalExpandBitmaps (himl, 1, 0);

        nIndex = himl->cCurImage;
        himl->cCurImage++;
    }

    hdcImage = CreateCompatibleDC (0);
    TRACE("hdcImage=%p\n", hdcImage);
    if (hdcImage == 0)
	ERR("invalid hdcImage!\n");

    imagelist_point_from_index(himl, nIndex, &pt);

    SetTextColor(himl->hdcImage, RGB(0,0,0));
    SetBkColor  (himl->hdcImage, RGB(255,255,255));

    if (ii.hbmColor)
    {
        hbmOldSrc = SelectObject (hdcImage, ii.hbmColor);
        StretchBlt (himl->hdcImage, pt.x, pt.y, himl->cx, himl->cy,
                    hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        if (himl->hbmMask)
        {
            SelectObject (hdcImage, ii.hbmMask);
            StretchBlt (himl->hdcMask, pt.x, pt.y, himl->cx, himl->cy,
                        hdcImage, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        }
    }
    else
    {
        UINT height = bmp.bmHeight / 2;
        hbmOldSrc = SelectObject (hdcImage, ii.hbmMask);
        StretchBlt (himl->hdcImage, pt.x, pt.y, himl->cx, himl->cy,
                    hdcImage, 0, height, bmp.bmWidth, height, SRCCOPY);
        if (himl->hbmMask)
            StretchBlt (himl->hdcMask, pt.x, pt.y, himl->cx, himl->cy,
                        hdcImage, 0, 0, bmp.bmWidth, height, SRCCOPY);
    }

    SelectObject (hdcImage, hbmOldSrc);

    DestroyIcon(hBestFitIcon);
    if (hdcImage)
	DeleteDC (hdcImage);
    if (ii.hbmColor)
	DeleteObject (ii.hbmColor);
    if (ii.hbmMask)
	DeleteObject (ii.hbmMask);

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

    himlTemp = ImageList_Merge (InternalDrag.himl, 0, himlDrag, iDrag,
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

    hbmNew = ImageList_CreateImage(himl->hdcImage, himl, himl->cMaxImage, himl->cx);
    SelectObject (himl->hdcImage, hbmNew);
    DeleteObject (himl->hbmImage);
    himl->hbmImage = hbmNew;

    if (himl->hbmMask) {
        SIZE sz;
        imagelist_get_bitmap_size(himl, himl->cMaxImage, himl->cx, &sz);
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
    if (himl->cMaxImage > iImageCount)
    {
        himl->cCurImage = iImageCount;
        /* TODO: shrink the bitmap when cMaxImage-cCurImage>cGrow ? */
	return TRUE;
    }

    nNewCount = iImageCount + himl->cGrow;
    nCopyCount = min(himl->cCurImage, iImageCount);

    hdcBitmap = CreateCompatibleDC (0);

    hbmNewBitmap = ImageList_CreateImage(hdcBitmap, himl, nNewCount, himl->cx);

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
	ERR("Could not create new image bitmap !\n");

    if (himl->hbmMask)
    {
        SIZE sz;
        imagelist_get_bitmap_size( himl, nNewCount, himl->cx, &sz );
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
static BOOL
_write_bitmap(HBITMAP hBitmap, LPSTREAM pstm)
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

    bitCount = bm.bmBitsPixel == 1 ? 1 : 24;
    sizeImage = DIB_GetDIBImageBytes(bm.bmWidth, bm.bmHeight, bitCount);

    totalSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if(bitCount != 24)
	totalSize += (1 << bitCount) * sizeof(RGBQUAD);
    offBits = totalSize;
    totalSize += sizeImage;

    data = Alloc(totalSize);
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

    if(bitCount == 1) {
        /* Hack. */
	LPBITMAPINFO inf = (LPBITMAPINFO)bmih;
	inf->bmiColors[0].rgbRed = inf->bmiColors[0].rgbGreen = inf->bmiColors[0].rgbBlue = 0;
	inf->bmiColors[1].rgbRed = inf->bmiColors[1].rgbGreen = inf->bmiColors[1].rgbBlue = 0xff;
    }

    if(FAILED(IStream_Write(pstm, data, totalSize, NULL)))
	goto failed;

    result = TRUE;

failed:
    Free(data);

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

BOOL WINAPI
ImageList_Write (HIMAGELIST himl, LPSTREAM pstm)
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


static HBITMAP ImageList_CreateImage(HDC hdc, HIMAGELIST himl, UINT count, UINT width)
{
    HBITMAP hbmNewBitmap;
    UINT ilc = (himl->flags & 0xFE);
    SIZE sz;

    imagelist_get_bitmap_size( himl, count, width, &sz );

    if ((ilc >= ILC_COLOR4 && ilc <= ILC_COLOR32) || ilc == ILC_COLOR)
    {
        VOID* bits;
        BITMAPINFO *bmi;

        TRACE("Creating DIBSection %d x %d, %d Bits per Pixel\n",
              sz.cx, sz.cy, himl->uBitsPixel);

	if (himl->uBitsPixel <= ILC_COLOR8)
	{
	    LPPALETTEENTRY pal;
	    ULONG i, colors;
	    BYTE temp;

	    colors = 1 << himl->uBitsPixel;
	    bmi = Alloc(sizeof(BITMAPINFOHEADER) +
	                    sizeof(PALETTEENTRY) * colors);

	    pal = (LPPALETTEENTRY)bmi->bmiColors;
	    GetPaletteEntries(GetStockObject(DEFAULT_PALETTE), 0, colors, pal);

	    /* Swap colors returned by GetPaletteEntries so we can use them for
	     * CreateDIBSection call. */
	    for (i = 0; i < colors; i++)
	    {
	        temp = pal[i].peBlue;
	        bmi->bmiColors[i].rgbRed = pal[i].peRed;
	        bmi->bmiColors[i].rgbBlue = temp;
	    }
	}
	else
	{
	    bmi = Alloc(sizeof(BITMAPINFOHEADER));
	}

	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = sz.cx;
	bmi->bmiHeader.biHeight = sz.cy;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = himl->uBitsPixel;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	hbmNewBitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &bits, 0, 0);

	Free (bmi);
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
ImageList_SetColorTable (HIMAGELIST himl, UINT uStartIndex, UINT cEntries, CONST RGBQUAD * prgb)
{
    return SetDIBColorTable(himl->hdcImage, uStartIndex, cEntries, prgb);
}
