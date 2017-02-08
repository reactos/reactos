/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
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

#include <string.h>

#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

/***********************************************************************
 *           MFDRV_PatBlt
 */
BOOL MFDRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    MFDRV_MetaParam6( dev, META_PATBLT, dst->log_x, dst->log_y, dst->log_width, dst->log_height,
                      HIWORD(rop), LOWORD(rop) );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_StretchBlt
 * this function contains TWO ways for processing StretchBlt in metafiles,
 * decide between rdFunction values  META_STRETCHBLT or META_DIBSTRETCHBLT
 * via #define STRETCH_VIA_DIB
 */
#define STRETCH_VIA_DIB

BOOL MFDRV_StretchBlt( PHYSDEV devDst, struct bitblt_coords *dst,
                       PHYSDEV devSrc, struct bitblt_coords *src, DWORD rop )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BITMAP BM;
#ifdef STRETCH_VIA_DIB
    LPBITMAPINFOHEADER lpBMI;
    WORD nBPP;
#endif
    HBITMAP hBitmap = GetCurrentObject(devSrc->hdc, OBJ_BITMAP);

    if (devSrc->funcs == devDst->funcs) return FALSE;  /* can't use a metafile DC as source */

    if (GetObjectW(hBitmap, sizeof(BITMAP), &BM) != sizeof(BITMAP))
    {
        WARN("bad bitmap object %p passed for hdc %p\n", hBitmap, devSrc->hdc);
        return FALSE;
    }
#ifdef STRETCH_VIA_DIB
    nBPP = BM.bmPlanes * BM.bmBitsPixel;
    if(nBPP > 8) nBPP = 24; /* FIXME Can't get 16bpp to work for some reason */
    len = sizeof(METARECORD) + 10 * sizeof(INT16)
            + sizeof(BITMAPINFOHEADER) + (nBPP <= 8 ? 1 << nBPP: 0) * sizeof(RGBQUAD)
              + get_dib_stride( BM.bmWidth, nBPP ) * BM.bmHeight;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, len)))
	return FALSE;
    mr->rdFunction = META_DIBSTRETCHBLT;
    lpBMI=(LPBITMAPINFOHEADER)(mr->rdParm+10);
    lpBMI->biSize      = sizeof(BITMAPINFOHEADER);
    lpBMI->biWidth     = BM.bmWidth;
    lpBMI->biHeight    = BM.bmHeight;
    lpBMI->biPlanes    = 1;
    lpBMI->biBitCount  = nBPP;
    lpBMI->biSizeImage = get_dib_image_size( (BITMAPINFO *)lpBMI );
    lpBMI->biClrUsed   = nBPP <= 8 ? 1 << nBPP : 0;
    lpBMI->biCompression = BI_RGB;
    lpBMI->biXPelsPerMeter = MulDiv(GetDeviceCaps(devSrc->hdc,LOGPIXELSX),3937,100);
    lpBMI->biYPelsPerMeter = MulDiv(GetDeviceCaps(devSrc->hdc,LOGPIXELSY),3937,100);
    lpBMI->biClrImportant  = 0;                          /* 1 meter  = 39.37 inch */

    TRACE("MF_StretchBltViaDIB->len = %d  rop=%x  PixYPM=%d Caps=%d\n",
	  len,rop,lpBMI->biYPelsPerMeter,GetDeviceCaps(devSrc->hdc, LOGPIXELSY));

    if (GetDIBits(devSrc->hdc, hBitmap, 0, (UINT)lpBMI->biHeight,
                  (LPSTR)lpBMI + get_dib_info_size( (BITMAPINFO *)lpBMI, DIB_RGB_COLORS ),
                  (LPBITMAPINFO)lpBMI, DIB_RGB_COLORS))
#else
    len = sizeof(METARECORD) + 15 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, len )))
	return FALSE;
    mr->rdFunction = META_STRETCHBLT;
    *(mr->rdParm +10) = BM.bmWidth;
    *(mr->rdParm +11) = BM.bmHeight;
    *(mr->rdParm +12) = BM.bmWidthBytes;
    *(mr->rdParm +13) = BM.bmPlanes;
    *(mr->rdParm +14) = BM.bmBitsPixel;
    TRACE("len = %ld  rop=%lx\n", len, rop);
    if (GetBitmapBits( hBitmap, BM.bmWidthBytes * BM.bmHeight, mr->rdParm + 15))
#endif
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParm) = LOWORD(rop);
      *(mr->rdParm + 1) = HIWORD(rop);
      *(mr->rdParm + 2) = src->log_height;
      *(mr->rdParm + 3) = src->log_width;
      *(mr->rdParm + 4) = src->log_y;
      *(mr->rdParm + 5) = src->log_x;
      *(mr->rdParm + 6) = dst->log_height;
      *(mr->rdParm + 7) = dst->log_width;
      *(mr->rdParm + 8) = dst->log_y;
      *(mr->rdParm + 9) = dst->log_x;
      ret = MFDRV_WriteRecord( devDst, mr, mr->rdSize * 2);
    }
    else
        ret = FALSE;
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}


/***********************************************************************
 *           MFDRV_StretchDIBits
 */
INT MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                         INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                         INT heightSrc, const void *bits,
                         BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DWORD infosize = get_dib_info_size(info, wUsage);
    DWORD len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr = HeapAlloc( GetProcessHeap(), 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_STRETCHDIB;
    mr->rdParm[0] = LOWORD(dwRop);
    mr->rdParm[1] = HIWORD(dwRop);
    mr->rdParm[2] = wUsage;
    mr->rdParm[3] = (INT16)heightSrc;
    mr->rdParm[4] = (INT16)widthSrc;
    mr->rdParm[5] = (INT16)ySrc;
    mr->rdParm[6] = (INT16)xSrc;
    mr->rdParm[7] = (INT16)heightDst;
    mr->rdParm[8] = (INT16)widthDst;
    mr->rdParm[9] = (INT16)yDst;
    mr->rdParm[10] = (INT16)xDst;
    memcpy(mr->rdParm + 11, info, infosize);
    memcpy(mr->rdParm + 11 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return heightSrc;
}


/***********************************************************************
 *           MFDRV_SetDIBitsToDeivce
 */
INT MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDst, INT yDst, DWORD cx,
                             DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                             UINT lines, LPCVOID bits, BITMAPINFO *info, UINT coloruse )

{
    DWORD infosize = get_dib_info_size(info, coloruse);
    DWORD len = sizeof(METARECORD) + 8 * sizeof(WORD) + infosize + info->bmiHeader.biSizeImage;
    METARECORD *mr = HeapAlloc( GetProcessHeap(), 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_SETDIBTODEV;
    mr->rdParm[0] = coloruse;
    mr->rdParm[1] = lines;
    mr->rdParm[2] = startscan;
    mr->rdParm[3] = (INT16)ySrc;
    mr->rdParm[4] = (INT16)xSrc;
    mr->rdParm[5] = (INT16)cy;
    mr->rdParm[6] = (INT16)cx;
    mr->rdParm[7] = (INT16)yDst;
    mr->rdParm[8] = (INT16)xDst;
    memcpy(mr->rdParm + 9, info, infosize);
    memcpy(mr->rdParm + 9 + infosize / 2, bits, info->bmiHeader.biSizeImage);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return lines;
}
