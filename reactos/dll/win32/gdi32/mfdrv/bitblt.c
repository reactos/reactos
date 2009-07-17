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
BOOL  CDECL MFDRV_PatBlt( PHYSDEV dev, INT left, INT top, INT width, INT height, DWORD rop )
{
    MFDRV_MetaParam6( dev, META_PATBLT, left, top, width, height, HIWORD(rop), LOWORD(rop) );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_BitBlt
 */
BOOL  CDECL MFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst, INT width, INT height,
                          PHYSDEV devSrc, INT xSrc, INT ySrc, DWORD rop )
{
    return MFDRV_StretchBlt(devDst, xDst, yDst, width, height, devSrc,
                            xSrc, ySrc, width, height, rop);
}



/***********************************************************************
 *           MFDRV_StretchBlt
 * this function contains TWO ways for processing StretchBlt in metafiles,
 * decide between rdFunction values  META_STRETCHBLT or META_DIBSTRETCHBLT
 * via #define STRETCH_VIA_DIB
 */
#define STRETCH_VIA_DIB

BOOL  CDECL MFDRV_StretchBlt( PHYSDEV devDst, INT xDst, INT yDst, INT widthDst,
                              INT heightDst, PHYSDEV devSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BITMAP BM;
    METAFILEDRV_PDEVICE *physDevSrc = (METAFILEDRV_PDEVICE *)devSrc;
#ifdef STRETCH_VIA_DIB
    LPBITMAPINFOHEADER lpBMI;
    WORD nBPP;
#endif
    HBITMAP hBitmap = GetCurrentObject(physDevSrc->hdc, OBJ_BITMAP);

    if (GetObjectW(hBitmap, sizeof(BITMAP), &BM) != sizeof(BITMAP))
    {
        WARN("bad bitmap object %p passed for hdc %p\n", hBitmap, physDevSrc->hdc);
        return FALSE;
    }
#ifdef STRETCH_VIA_DIB
    nBPP = BM.bmPlanes * BM.bmBitsPixel;
    if(nBPP > 8) nBPP = 24; /* FIXME Can't get 16bpp to work for some reason */
    len = sizeof(METARECORD) + 10 * sizeof(INT16)
            + sizeof(BITMAPINFOHEADER) + (nBPP <= 8 ? 1 << nBPP: 0) * sizeof(RGBQUAD)
              + DIB_GetDIBWidthBytes(BM.bmWidth, nBPP) * BM.bmHeight;
    if (!(mr = HeapAlloc( GetProcessHeap(), 0, len)))
	return FALSE;
    mr->rdFunction = META_DIBSTRETCHBLT;
    lpBMI=(LPBITMAPINFOHEADER)(mr->rdParm+10);
    lpBMI->biSize      = sizeof(BITMAPINFOHEADER);
    lpBMI->biWidth     = BM.bmWidth;
    lpBMI->biHeight    = BM.bmHeight;
    lpBMI->biPlanes    = 1;
    lpBMI->biBitCount  = nBPP;
    lpBMI->biSizeImage = DIB_GetDIBWidthBytes(BM.bmWidth, nBPP) * lpBMI->biHeight;
    lpBMI->biClrUsed   = nBPP <= 8 ? 1 << nBPP : 0;
    lpBMI->biCompression = BI_RGB;
    lpBMI->biXPelsPerMeter = MulDiv(GetDeviceCaps(physDevSrc->hdc,LOGPIXELSX),3937,100);
    lpBMI->biYPelsPerMeter = MulDiv(GetDeviceCaps(physDevSrc->hdc,LOGPIXELSY),3937,100);
    lpBMI->biClrImportant  = 0;                          /* 1 meter  = 39.37 inch */

    TRACE("MF_StretchBltViaDIB->len = %d  rop=%x  PixYPM=%d Caps=%d\n",
	  len,rop,lpBMI->biYPelsPerMeter,GetDeviceCaps(physDevSrc->hdc, LOGPIXELSY));

    if (GetDIBits(physDevSrc->hdc, hBitmap, 0, (UINT)lpBMI->biHeight,
                  (LPSTR)lpBMI + bitmap_info_size( (BITMAPINFO *)lpBMI,
                                                     DIB_RGB_COLORS ),
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
      *(mr->rdParm + 2) = heightSrc;
      *(mr->rdParm + 3) = widthSrc;
      *(mr->rdParm + 4) = ySrc;
      *(mr->rdParm + 5) = xSrc;
      *(mr->rdParm + 6) = heightDst;
      *(mr->rdParm + 7) = widthDst;
      *(mr->rdParm + 8) = yDst;
      *(mr->rdParm + 9) = xDst;
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
INT  CDECL MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                                INT heightSrc, const void *bits,
                                const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DWORD len, infosize, imagesize;
    METARECORD *mr;

    infosize = bitmap_info_size(info, wUsage);
    imagesize = DIB_GetDIBImageBytes( info->bmiHeader.biWidth,
				      info->bmiHeader.biHeight,
				      info->bmiHeader.biBitCount );

    len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + imagesize;
    mr = HeapAlloc( GetProcessHeap(), 0, len );
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
    memcpy(mr->rdParm + 11 + infosize / 2, bits, imagesize);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return heightSrc;
}


/***********************************************************************
 *           MFDRV_SetDIBitsToDeivce
 */
INT  CDECL MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDst, INT yDst, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                                    UINT lines, LPCVOID bits, const BITMAPINFO *info,
                                    UINT coloruse )

{
    DWORD len, infosize, imagesize;
    METARECORD *mr;

    infosize = bitmap_info_size(info, coloruse);
    imagesize = DIB_GetDIBImageBytes( info->bmiHeader.biWidth,
				      info->bmiHeader.biHeight,
				      info->bmiHeader.biBitCount );

    len = sizeof(METARECORD) + 8 * sizeof(WORD) + infosize + imagesize;
    mr = HeapAlloc( GetProcessHeap(), 0, len );
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
    memcpy(mr->rdParm + 9 + infosize / 2, bits, imagesize);
    MFDRV_WriteRecord( dev, mr, mr->rdSize * 2 );
    HeapFree( GetProcessHeap(), 0, mr );
    return lines;
}
