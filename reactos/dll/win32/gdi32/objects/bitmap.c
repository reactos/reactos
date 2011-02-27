#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT FASTCALL DIB_BitmapInfoSize(const BITMAPINFO * info, WORD coloruse)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
               ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (colors > 256) colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

/*
 * Return the full scan size for a bitmap.
 *
 * Based on Wine, Utils.c and Windows Graphics Prog pg 595, SDK amvideo.h.
 */
UINT
FASTCALL
DIB_BitmapMaxBitsSize( PBITMAPINFO Info, UINT ScanLines )
{
    UINT MaxBits = 0;

    if (!Info) return 0;

    if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER)Info;
        MaxBits = Core->bcBitCount * Core->bcPlanes * Core->bcWidth;
    }
    else  /* assume BITMAPINFOHEADER */
    {
        if ((Info->bmiHeader.biCompression) && (Info->bmiHeader.biCompression != BI_BITFIELDS))
            return Info->bmiHeader.biSizeImage;
        // Planes are over looked by Yuan. I guess assumed always 1.
        MaxBits = Info->bmiHeader.biBitCount * Info->bmiHeader.biPlanes * Info->bmiHeader.biWidth;
    }
    MaxBits = ((MaxBits + 31) & ~31 ) / 8; // From Yuan, ScanLineSize = (Width * bitcount + 31)/32
    return (MaxBits * ScanLines);  // ret the full Size.
}

UINT
FASTCALL
DIB_BitmapBitsSize( CONST BITMAPINFO* Info )
{
    UINT Ret;

    if (!Info) return 0;

    if ( Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER)Info;
        Ret = Core->bcHeight *
              ((Core->bcWidth * Core->bcPlanes * Core->bcBitCount  + 31) & ~31 ) / 8;
    }
    else /* assume BITMAPINFOHEADER */
    {
        if ((Info->bmiHeader.biCompression) &&
                (Info->bmiHeader.biCompression != BI_BITFIELDS))
            return Info->bmiHeader.biSizeImage;
        // Make Height positive always....
        Ret = abs(Info->bmiHeader.biHeight) *
              ((Info->bmiHeader.biWidth * Info->bmiHeader.biPlanes * Info->bmiHeader.biBitCount + 31) & ~31 ) / 8;
    }
    return Ret;
}


/*
 * DIB_GetBitmapInfo is complete copy of wine cvs 2/9-2006
 * from file dib.c from gdi32.dll or orginal version
 * did not calc the info right for some headers.
 */
INT
WINAPI
DIB_GetBitmapInfo(const BITMAPINFOHEADER *header,
                  PLONG width,
                  PLONG height,
                  PWORD planes,
                  PWORD bpp,
                  PLONG compr,
                  PLONG size )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        *size   = 0;
        return 0;
    }

    if (header->biSize == sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        *size   = header->biSizeImage;
        return 1;
    }

    if (header->biSize == sizeof(BITMAPV4HEADER))
    {
        BITMAPV4HEADER *v4hdr = (BITMAPV4HEADER *)header;
        *width  = v4hdr->bV4Width;
        *height = v4hdr->bV4Height;
        *planes = v4hdr->bV4Planes;
        *bpp    = v4hdr->bV4BitCount;
        *compr  = v4hdr->bV4V4Compression;
        *size   = v4hdr->bV4SizeImage;
        return 4;
    }

    if (header->biSize == sizeof(BITMAPV5HEADER))
    {
        BITMAPV5HEADER *v5hdr = (BITMAPV5HEADER *)header;
        *width  = v5hdr->bV5Width;
        *height = v5hdr->bV5Height;
        *planes = v5hdr->bV5Planes;
        *bpp    = v5hdr->bV5BitCount;
        *compr  = v5hdr->bV5Compression;
        *size   = v5hdr->bV5SizeImage;
        return 5;
    }
    DPRINT("(%ld): wrong size for header\n", header->biSize );
    return -1;
}

/*
 * @implemented
 */
int
WINAPI
GdiGetBitmapBitsSize(BITMAPINFO *lpbmi)
{
    int retSize;

    if (lpbmi->bmiHeader.biSize == FIELD_OFFSET(BITMAPINFOHEADER, biPlanes))
    {
        /* Calc the bits Size and align it*/
        retSize = HIWORD(lpbmi->bmiHeader.biWidth) * ((LOWORD(lpbmi->bmiHeader.biWidth) *
                  LOWORD(lpbmi->bmiHeader.biHeight) * HIWORD(lpbmi->bmiHeader.biHeight) + 31)
                  & -32) / 8;
    }
    else
    {
        if ( (lpbmi->bmiHeader.biCompression == BI_BITFIELDS) ||
                (lpbmi->bmiHeader.biCompression == BI_RGB))
        {
            if (lpbmi->bmiHeader.biHeight >=0 )
            {
                /* Calc the bits Size and align it*/
                retSize = lpbmi->bmiHeader.biHeight * ((lpbmi->bmiHeader.biWidth *
                                                        lpbmi->bmiHeader.biPlanes * lpbmi->bmiHeader.biBitCount + 31) & -32) / 8;
            }
            else
            {
                /* Make height postiive if it negitve then calc the bits Size and align it*/
                retSize = (-lpbmi->bmiHeader.biHeight) * ((lpbmi->bmiHeader.biWidth *
                          lpbmi->bmiHeader.biPlanes * lpbmi->bmiHeader.biBitCount + 31) & -32) / 8;
            }
        }
        else
        {
            retSize = lpbmi->bmiHeader.biSizeImage;
        }
    }
    return retSize;
}

/*
 * @implemented
 */
HBITMAP WINAPI
CreateDIBSection(
    HDC hDC,
    CONST BITMAPINFO *BitmapInfo,
    UINT Usage,
    VOID **Bits,
    HANDLE hSection,
    DWORD dwOffset)
{
    PBITMAPINFO pConvertedInfo;
    UINT ConvertedInfoSize;
    HBITMAP hBitmap = NULL;
    PVOID  bmBits = NULL;

    pConvertedInfo = ConvertBitmapInfo(BitmapInfo, Usage,
                                       &ConvertedInfoSize, FALSE);
    if (pConvertedInfo)
    {
        // Verify header due to converted may == info.
        if ( pConvertedInfo->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) )
        {
            if ( pConvertedInfo->bmiHeader.biCompression == BI_JPEG ||
                    pConvertedInfo->bmiHeader.biCompression  == BI_PNG )
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return NULL;
            }
        }
        bmBits = Bits;
        hBitmap = NtGdiCreateDIBSection( hDC,
                                         hSection,
                                         dwOffset,
                                         pConvertedInfo,
                                         Usage,
                                         0,
                                         0,
                                         0,
                                         &bmBits);
        if (BitmapInfo != pConvertedInfo)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);

        if (!hBitmap)
        {
            bmBits = NULL;
        }
    }

    if (Bits) *Bits = bmBits;

    return hBitmap;
}


/*
 * @implemented
 */
BOOL
WINAPI
BitBlt(HDC hdcDest,      /* handle to destination DC */
       int nXOriginDest, /* x-coord of destination upper-left corner */
       int nYOriginDest, /* y-coord of destination upper-left corner */
       int nWidthDest,   /* width of destination rectangle */
       int nHeightDest,  /* height of destination rectangle */
       HDC hdcSrc,       /* handle to source DC */
       int nXSrc,        /* x-coordinate of source upper-left corner */
       int nYSrc,        /* y-coordinate of source upper-left corner */
       DWORD dwRop)      /* raster operation code */
{
    /* use patBlt for no source blt  Like windows does */
    if (!ROP_USES_SOURCE(dwRop))
    {
        return PatBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, dwRop);
    }

    return NtGdiBitBlt(hdcDest,
                       nXOriginDest,
                       nYOriginDest,
                       nWidthDest,
                       nHeightDest,
                       hdcSrc,
                       nXSrc,
                       nYSrc,
                       dwRop,
                       0,
                       0);
}

/*
 * @implemented
 */
BOOL WINAPI
StretchBlt(
    HDC hdcDest,      /* handle to destination DC */
    int nXOriginDest, /* x-coord of destination upper-left corner */
    int nYOriginDest, /* y-coord of destination upper-left corner */
    int nWidthDest,   /* width of destination rectangle */
    int nHeightDest,  /* height of destination rectangle */
    HDC hdcSrc,       /* handle to source DC */
    int nXOriginSrc,  /* x-coord of source upper-left corner */
    int nYOriginSrc,  /* y-coord of source upper-left corner */
    int nWidthSrc,    /* width of source rectangle */
    int nHeightSrc,   /* height of source rectangle */
    DWORD dwRop)      /* raster operation code */

{
    if ((nWidthDest != nWidthSrc) || (nHeightDest != nHeightSrc))
    {
        return NtGdiStretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                               nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc,
                               nWidthSrc, nHeightSrc, dwRop, 0);
    }

    return NtGdiBitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                       nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, dwRop, 0, 0);
}

/*
 * @unimplemented
 */
HBITMAP WINAPI
CreateBitmap(INT  Width,
             INT  Height,
             UINT  Planes,
             UINT  BitsPixel,
             CONST VOID* pUnsafeBits)
{
    /* FIXME some part should be done in user mode */
    if (Width && Height)
    {
        return NtGdiCreateBitmap(Width, Height, Planes, BitsPixel, (LPBYTE) pUnsafeBits);
    }
    else
    {
        /* Return 1x1 bitmap */
        return GetStockObject(DEFAULT_BITMAP);
    }
}

/*
 * @implemented
 */
HBITMAP WINAPI
CreateBitmapIndirect(const BITMAP *pbm)
{
    HBITMAP bitmap = NULL;

    /* Note windows xp/2003 does not check if pbm is NULL or not */
    if ( (pbm->bmWidthBytes != 0) &&
            (!(pbm->bmWidthBytes & 1)) )

    {

        bitmap = CreateBitmap(pbm->bmWidth,
                              pbm->bmHeight,
                              pbm->bmPlanes,
                              pbm->bmBitsPixel,
                              pbm->bmBits);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bitmap;
}

HBITMAP WINAPI
CreateDiscardableBitmap(
    HDC  hDC,
    INT  Width,
    INT  Height)
{
    return  CreateCompatibleBitmap(hDC, Width, Height);
}


HBITMAP WINAPI
CreateCompatibleBitmap(
    HDC  hDC,
    INT  Width,
    INT  Height)
{
    PDC_ATTR pDc_Attr;
    HBITMAP hBmp = NULL;
    DIBSECTION dibs;

    if (!GdiGetHandleUserData(hDC, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
        return NULL;

    if ( !Width || !Height )
        return GetStockObject(DEFAULT_BITMAP);

    if (!(pDc_Attr->ulDirty_ & DC_DIBSECTION))
    {
        return  NtGdiCreateCompatibleBitmap(hDC, Width, Height);
    }

    hBmp = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_BITMAP);

    if ( GetObjectA(hBmp, sizeof(DIBSECTION), &dibs) != sizeof(DIBSECTION) )
        return NULL;

    if ( dibs.dsBm.bmBitsPixel <= 8 )
        GetDIBColorTable(hDC, 0, 256, (RGBQUAD *)&dibs.dsBitfields);

    dibs.dsBmih.biWidth = Width;
    dibs.dsBmih.biHeight = Height;

    return CreateDIBSection(hDC, (CONST BITMAPINFO *)&dibs.dsBmih, 0, NULL, NULL, 0);
}

void
ConvertBackBitmapInfo(PBITMAPINFO pbmi, PBITMAPINFO pbmiConverted)
{
    /* Check if we converted from PBITMAPCOREINFO */
    if (pbmiConverted != pbmi)
    {
        PBITMAPCOREINFO pbci = (PBITMAPCOREINFO)pbmi;

        /* Convert back header */
        pbci->bmciHeader.biSize = sizeof(BITMAPCOREHEADER);
        pbci->bmciHeader.bcWidth = pbmiConverted->bmiHeader.biWidth;
        pbci->bmciHeader.bcHeight = pbmiConverted->bmiHeader.biHeight;
        pbci->bmciHeader.bcPlanes = 1;
        pbci->bmciHeader.bcBitCount = pbmiConverted->bmiHeader.biBitCount;

        /* Convert back colors */
        for (i = 0; i < pbmiConverted->bmiHeader.biClrUsed; i++)
        {
            pbci->bmciColors.rgbtRed = pbmiConverted->bmiColors.rgbRed;
            pbci->bmciColors.rgbtGreen = pbmiConverted->bmiColors.rgbGreen;
            pbci->bmciColors.rgbtBlue = pbmiConverted->bmiColors.rgbBlue;
        }

        /* Free memory */
        RtlFreeHeap(RtlGetProcessHeap(), 0, pbmiConverted);
    }
}

INT
WINAPI
GetDIBits(
    HDC hDC,
    HBITMAP hbmp,
    UINT uStartScan,
    UINT cScanLines,
    LPVOID lpvBits,
    LPBITMAPINFO lpbmi,
    UINT uUsage)
{
    PBITMAPINFO pbmiConverted;
    UINT cjBmpScanSize, cjInfoSize;
    INT iResult;

    if (!hDC || !GdiIsHandleValid((HGDIOBJ)hDC) || !lpbmi)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Convert BITMAPINFO to a proper format */
    pbmiConverted = ConvertBitmapInfo(lpbmi, uUsage, &cjInfoSize, FALSE);
    if (!pbmiConverted)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    cjBmpScanSize = DIB_BitmapMaxBitsSize(lpbmi, cScanLines);

    if (lpvBits)
    {
        if (lpbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
        {
            if (lpbmi->bmiHeader.biCompression == BI_JPEG ||
                lpbmi->bmiHeader.biCompression == BI_PNG)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
    }

    iResult = NtGdiGetDIBitsInternal(hDC,
                                     hbmp,
                                     uStartScan,
                                     cScanLines,
                                     lpvBits,
                                     pbmiConverted,
                                     uUsage,
                                     cjBmpScanSize,
                                     cjInfoSize);

    ConvertBackBitmapInfo(lpbmi, pbmiConverted);

    return iResult;
}


/*
 * @implemented
 */
HBITMAP
WINAPI
CreateDIBitmap( HDC hDC,
                const BITMAPINFOHEADER *Header,
                DWORD Init,
                LPCVOID Bits,
                const BITMAPINFO *Data,
                UINT ColorUse)
{
    LONG width, height, compr, dibsize;
    WORD planes, bpp;
//  PDC_ATTR pDc_Attr;
    UINT InfoSize = 0;
    UINT cjBmpScanSize = 0;
    HBITMAP hBmp;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Header) return 0;

    if (DIB_GetBitmapInfo(Header, &width, &height, &planes, &bpp, &compr, &dibsize) == -1)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

// For Icm support.
// GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))

    if(Data)
    {
        _SEH2_TRY
        {
            cjBmpScanSize = DIB_BitmapBitsSize(Data);
            CalculateColorTableSize(&Data->bmiHeader, &ColorUse, &InfoSize);
            InfoSize += Data->bmiHeader.biSize;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
    }

    if(!NT_SUCCESS(Status))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    DPRINT("pBMI %x, Size bpp %d, dibsize %d, Conv %d, BSS %d\n", Data,bpp,dibsize,InfoSize,cjBmpScanSize);

    if ( !width || !height )
        hBmp = GetStockObject(DEFAULT_BITMAP);
    else
    {
        hBmp = NtGdiCreateDIBitmapInternal(hDC,
                                           width,
                                           height,
                                           Init,
                                           (LPBYTE)Bits,
                                           (LPBITMAPINFO)Data,
                                           ColorUse,
                                           InfoSize,
                                           cjBmpScanSize,
                                           0,
                                           0);
    }
    return hBmp;
}

#if 0 // FIXME!!! This is a victim of the Win32k Initialization BUG!!!!!
/*
 * @implemented
 */
INT
WINAPI
SetDIBits(HDC hDC,
          HBITMAP hBitmap,
          UINT uStartScan,
          UINT cScanLines,
          CONST VOID *lpvBits,
          CONST BITMAPINFO *lpbmi,
          UINT fuColorUse)
{
    HDC hDCc, SavehDC, nhDC;
    DWORD dwWidth, dwHeight;
    HGDIOBJ hOldBitmap;
    HPALETTE hPal = NULL;
    INT LinesCopied = 0;
    BOOL newDC = FALSE;

    if ( !lpvBits || (GDI_HANDLE_GET_TYPE(hBitmap) != GDI_OBJECT_TYPE_BITMAP) )
        return 0;

    if ( lpbmi )
    {
        if ( lpbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) )
        {
            if ( lpbmi->bmiHeader.biCompression == BI_JPEG || lpbmi->bmiHeader.biCompression == BI_PNG )
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
    }

    hDCc = NtGdiGetDCforBitmap(hBitmap);
    SavehDC = hDCc;
    if ( !hDCc )
    {
        nhDC = CreateCompatibleDC(hDC);
        if ( !nhDC ) return 0;
        newDC = TRUE;
        SavehDC = nhDC;
    }
    else if ( !SaveDC(hDCc) )
        return 0;

    hOldBitmap = SelectObject(SavehDC, hBitmap);

    if ( hOldBitmap )
    {
        if ( hDC )
            hPal = SelectPalette(SavehDC, (HPALETTE)GetDCObject(hDC, GDI_OBJECT_TYPE_PALETTE), FALSE);

        if ( lpbmi->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
        {
            PBITMAPCOREINFO pbci = (PBITMAPCOREINFO) lpbmi;
            dwWidth = pbci->bmciHeader.bcWidth;
            dwHeight = pbci->bmciHeader.bcHeight;
        }
        else
        {
            dwWidth = lpbmi->bmiHeader.biWidth;
            dwHeight = abs(lpbmi->bmiHeader.biHeight);
        }

        LinesCopied = SetDIBitsToDevice(SavehDC,
                                        0,
                                        0,
                                        dwWidth,
                                        dwHeight,
                                        0,
                                        0,
                                        uStartScan,
                                        cScanLines,
                                        (void *)lpvBits,
                                        (LPBITMAPINFO)lpbmi,
                                        fuColorUse);

        if ( hDC ) SelectPalette(SavehDC, hPal, FALSE);

        SelectObject(SavehDC, hOldBitmap);
    }

    if ( newDC )
        DeleteDC(SavehDC);
    else
        RestoreDC(SavehDC, -1);

    return LinesCopied;
}
#endif

INT
WINAPI
SetDIBits(HDC hdc,
          HBITMAP hbmp,
          UINT uStartScan,
          UINT cScanLines,
          CONST VOID *lpvBits,
          CONST BITMAPINFO *lpbmi,
          UINT fuColorUse)
{
    PBITMAPINFO pConvertedInfo;
    UINT ConvertedInfoSize;
    INT LinesCopied = 0;
    UINT cjBmpScanSize = 0;
    PVOID pvSafeBits = (PVOID)lpvBits;

// This needs to be almost the sames as SetDIBitsToDevice

    if ( !cScanLines || !lpbmi || !lpvBits || (GDI_HANDLE_GET_TYPE(hbmp) != GDI_OBJECT_TYPE_BITMAP))
        return 0;

    if ( fuColorUse && fuColorUse != DIB_PAL_COLORS && fuColorUse != DIB_PAL_COLORS+1 )
        return 0;

    pConvertedInfo = ConvertBitmapInfo(lpbmi, fuColorUse,
                                       &ConvertedInfoSize, FALSE);
    if (!pConvertedInfo)
        return 0;

    cjBmpScanSize = DIB_BitmapMaxBitsSize((LPBITMAPINFO)lpbmi, cScanLines);

    if ( lpvBits )
    {
        pvSafeBits = RtlAllocateHeap(GetProcessHeap(), 0, cjBmpScanSize);
        if (pvSafeBits)
            RtlCopyMemory( pvSafeBits, lpvBits, cjBmpScanSize);
    }

    LinesCopied = NtGdiSetDIBits( hdc,
                                  hbmp,
                                  uStartScan,
                                  cScanLines,
                                  pvSafeBits,
                                  pConvertedInfo,
                                  fuColorUse);

    if ( lpvBits != pvSafeBits)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pvSafeBits);
    if (lpbmi != pConvertedInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
    return LinesCopied;
}

/*
 * @implemented
 *
 */
INT
WINAPI
SetDIBitsToDevice(
    HDC hdc,
    int XDest,
    int YDest,
    DWORD Width,
    DWORD Height,
    int XSrc,
    int YSrc,
    UINT StartScan,
    UINT ScanLines,
    CONST VOID *Bits,
    CONST BITMAPINFO *lpbmi,
    UINT ColorUse)
{
    PDC_ATTR pDc_Attr;
    PBITMAPINFO pConvertedInfo;
    UINT ConvertedInfoSize;
    INT LinesCopied = 0;
    UINT cjBmpScanSize = 0;
    PVOID pvSafeBits = (PVOID)Bits;

    if ( !ScanLines || !lpbmi || !Bits )
        return 0;

    if ( ColorUse && ColorUse != DIB_PAL_COLORS && ColorUse != DIB_PAL_COLORS+1 )
        return 0;

    pConvertedInfo = ConvertBitmapInfo(lpbmi, ColorUse,
                                       &ConvertedInfoSize, FALSE);
    if (!pConvertedInfo)
        return 0;

#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetDIBitsToDevice( hdc,
                                            XDest,
                                            YDest,
                                            Width,
                                            Height,
                                            XSrc,
                                            YSrc,
                                            StartScan,
                                            ScanLines,
                                            Bits,
                                            lpbmi,
                                            ColorUse);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return 0;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetDIBitsToDevice(hdc,
                                                XDest,
                                                YDest,
                                                Width,
                                                Height,
                                                XSrc,
                                                YSrc,
                                                StartScan,
                                                ScanLines,
                                                Bits,
                                                lpbmi,
                                                ColorUse);
            }
            return 0;
        }
    }
#endif
    cjBmpScanSize = DIB_BitmapMaxBitsSize((LPBITMAPINFO)lpbmi, ScanLines);

    if ( Bits )
    {
        pvSafeBits = RtlAllocateHeap(GetProcessHeap(), 0, cjBmpScanSize);
        if (pvSafeBits)
            RtlCopyMemory( pvSafeBits, Bits, cjBmpScanSize);
    }

    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*
      if ( !pDc_Attr ||
           ((pConvertedInfo->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) &&
           (pConvertedInfo->bmiHeader.biCompression == BI_JPEG ||
            pConvertedInfo->bmiHeader.biCompression  == BI_PNG )) )*/
    {
        LinesCopied = NtGdiSetDIBitsToDeviceInternal( hdc,
                      XDest,
                      YDest,
                      Width,
                      Height,
                      XSrc,
                      YSrc,
                      StartScan,
                      ScanLines,
                      (LPBYTE)pvSafeBits,
                      (LPBITMAPINFO)pConvertedInfo,
                      ColorUse,
                      cjBmpScanSize,
                      ConvertedInfoSize,
                      TRUE,
                      NULL);
    }
    if ( Bits != pvSafeBits)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pvSafeBits);
    if (lpbmi != pConvertedInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);

    return LinesCopied;
}


/*
 * @unimplemented
 */
int
WINAPI
StretchDIBits(HDC hdc,
              int XDest,
              int YDest,
              int nDestWidth,
              int nDestHeight,
              int XSrc,
              int YSrc,
              int nSrcWidth,
              int nSrcHeight,
              CONST VOID *lpBits,
              CONST BITMAPINFO *lpBitsInfo,
              UINT iUsage,
              DWORD dwRop)

{
    PDC_ATTR pDc_Attr;
    PBITMAPINFO pConvertedInfo = NULL;
    UINT ConvertedInfoSize = 0;
    INT LinesCopied = 0;
    UINT cjBmpScanSize = 0;
    PVOID pvSafeBits = NULL;
    BOOL Hit = FALSE;

    DPRINT("StretchDIBits %x : %x : %d\n", lpBits, lpBitsInfo, iUsage);
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_StretchBlt( hdc,
                                     XDest,
                                     YDest,
                                     nDestWidth,
                                     nDestHeight,
                                     XSrc,
                                     YSrc,
                                     nSrcWidth,
                                     nSrcHeight,
                                     lpBits,
                                     lpBitsInfo,
                                     iUsage,
                                     dwRop);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return 0;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_StretchBlt(hdc,
                                         XDest,
                                         YDest,
                                         nDestWidth,
                                         nDestHeight,
                                         XSrc,
                                         YSrc,
                                         nSrcWidth,
                                         nSrcHeight,
                                         lpBits,
                                         lpBitsInfo,
                                         iUsage,
                                         dwRop);
            }
            return 0;
        }
    }
#endif
    pConvertedInfo = ConvertBitmapInfo(lpBitsInfo, iUsage,
                                       &ConvertedInfoSize, FALSE);
    if (!pConvertedInfo)
    {
        return 0;
    }

    cjBmpScanSize = DIB_BitmapBitsSize((LPBITMAPINFO)pConvertedInfo);

    if ( lpBits )
    {
        pvSafeBits = RtlAllocateHeap(GetProcessHeap(), 0, cjBmpScanSize);
        if (pvSafeBits)
        {
            _SEH2_TRY
            {
                RtlCopyMemory( pvSafeBits, lpBits, cjBmpScanSize );
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Hit = TRUE;
            }
            _SEH2_END

            if (Hit)
            {
                // We don't die, we continue on with a allocated safe pointer to kernel
                // space.....
                DPRINT1("StretchDIBits fail to read BitMapInfo: %x or Bits: %x & Size: %d\n",pConvertedInfo,lpBits,cjBmpScanSize);
            }
            DPRINT("StretchDIBits Allocate Bits %d!!!\n", cjBmpScanSize);
        }
    }

    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*
      if ( !pDc_Attr ||
           ((pConvertedInfo->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) &&
           (pConvertedInfo->bmiHeader.biCompression == BI_JPEG ||
            pConvertedInfo->bmiHeader.biCompression  == BI_PNG )) )*/
    {
        LinesCopied = NtGdiStretchDIBitsInternal( hdc,
                      XDest,
                      YDest,
                      nDestWidth,
                      nDestHeight,
                      XSrc,
                      YSrc,
                      nSrcWidth,
                      nSrcHeight,
                      pvSafeBits,
                      pConvertedInfo,
                      (DWORD)iUsage,
                      dwRop,
                      ConvertedInfoSize,
                      cjBmpScanSize,
                      NULL);
    }
    if ( pvSafeBits )
        RtlFreeHeap(RtlGetProcessHeap(), 0, pvSafeBits);
    if (lpBitsInfo != pConvertedInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);

    return LinesCopied;
}

