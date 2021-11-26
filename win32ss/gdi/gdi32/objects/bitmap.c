#include <precomp.h>

#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

// From Yuan, ScanLineSize = (Width * bitcount + 31)/32
#define WIDTH_BYTES_ALIGN32(cx, bpp) ((((cx) * (bpp) + 31) & ~31) >> 3)

/*
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT
FASTCALL DIB_BitmapInfoSize(
    const BITMAPINFO * info,
    WORD coloruse,
    BOOL max)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *) info;
        size = sizeof(BITMAPCOREHEADER);
        if (core->bcBitCount <= 8)
        {
            colors = 1 << core->bcBitCount;
            if (coloruse == DIB_RGB_COLORS)
                size += colors * sizeof(RGBTRIPLE);
            else
                size += colors * sizeof(WORD);
        }
        return size;
    }
    else /* assume BITMAPINFOHEADER */
    {
        colors = max ? (1 << info->bmiHeader.biBitCount) : info->bmiHeader.biClrUsed;
        if (colors > 256)
            colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
            masks = 3;
        size = max(info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD));
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
DIB_BitmapMaxBitsSize(
    PBITMAPINFO Info,
    UINT ScanLines)
{
    UINT Ret;

    if (!Info)
        return 0;

    if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER) Info;
        Ret = WIDTH_BYTES_ALIGN32(Core->bcWidth * Core->bcPlanes,
                Core->bcBitCount) * ScanLines;
    }
    else /* assume BITMAPINFOHEADER */
    {
        if ((Info->bmiHeader.biCompression == BI_RGB) || (Info->bmiHeader.biCompression == BI_BITFIELDS))
        {
            Ret = WIDTH_BYTES_ALIGN32(
                    Info->bmiHeader.biWidth * Info->bmiHeader.biPlanes,
                    Info->bmiHeader.biBitCount) * ScanLines;
        }
        else
        {
            Ret = Info->bmiHeader.biSizeImage;
        }
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
DIB_GetBitmapInfo(
    const BITMAPINFOHEADER *header,
    PLONG width,
    PLONG height,
    PWORD planes,
    PWORD bpp,
    PLONG compr,
    PLONG size)
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *) header;
        *width = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp = core->bcBitCount;
        *compr = 0;
        *size = 0;
        return 0;
    }

    if (header->biSize == sizeof(BITMAPINFOHEADER))
    {
        *width = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp = header->biBitCount;
        *compr = header->biCompression;
        *size = header->biSizeImage;
        return 1;
    }

    if (header->biSize == sizeof(BITMAPV4HEADER))
    {
        BITMAPV4HEADER *v4hdr = (BITMAPV4HEADER *) header;
        *width = v4hdr->bV4Width;
        *height = v4hdr->bV4Height;
        *planes = v4hdr->bV4Planes;
        *bpp = v4hdr->bV4BitCount;
        *compr = v4hdr->bV4V4Compression;
        *size = v4hdr->bV4SizeImage;
        return 4;
    }

    if (header->biSize == sizeof(BITMAPV5HEADER))
    {
        BITMAPV5HEADER *v5hdr = (BITMAPV5HEADER *) header;
        *width = v5hdr->bV5Width;
        *height = v5hdr->bV5Height;
        *planes = v5hdr->bV5Planes;
        *bpp = v5hdr->bV5BitCount;
        *compr = v5hdr->bV5Compression;
        *size = v5hdr->bV5SizeImage;
        return 5;
    }
    DPRINT("(%lu): wrong size for header\n", header->biSize);
    return -1;
}

/*
 * @implemented
 */
int
WINAPI
GdiGetBitmapBitsSize(
    BITMAPINFO *lpbmi)
{
    UINT Ret;

    if (!lpbmi)
        return 0;

    if (lpbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER) lpbmi;
        Ret =
        WIDTH_BYTES_ALIGN32(Core->bcWidth * Core->bcPlanes,
                Core->bcBitCount) * Core->bcHeight;
    }
    else /* assume BITMAPINFOHEADER */
    {
        if (!(lpbmi->bmiHeader.biCompression) || (lpbmi->bmiHeader.biCompression == BI_BITFIELDS))
        {
            Ret = WIDTH_BYTES_ALIGN32(
                    lpbmi->bmiHeader.biWidth * lpbmi->bmiHeader.biPlanes,
                    lpbmi->bmiHeader.biBitCount) * abs(lpbmi->bmiHeader.biHeight);
        }
        else
        {
            Ret = lpbmi->bmiHeader.biSizeImage;
        }
    }
    return Ret;
}

/*
 * @implemented
 */
HBITMAP
WINAPI
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
    PVOID bmBits = NULL;

    pConvertedInfo = ConvertBitmapInfo(BitmapInfo, Usage, &ConvertedInfoSize,
    FALSE);

    if (pConvertedInfo)
    {
        // Verify header due to converted may == info.
        if (pConvertedInfo->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
        {
            if (pConvertedInfo->bmiHeader.biCompression == BI_JPEG
                || pConvertedInfo->bmiHeader.biCompression == BI_PNG)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return NULL;
            }
        }
        bmBits = Bits;
        hBitmap = NtGdiCreateDIBSection(hDC, hSection, dwOffset, pConvertedInfo, Usage,
            ConvertedInfoSize, 0, // fl
            0, // dwColorSpace
            &bmBits);

        if (BitmapInfo != pConvertedInfo)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);

        if (!hBitmap)
        {
            bmBits = NULL;
        }
    }

    if (Bits)
        *Bits = bmBits;

    return hBitmap;
}


/*
 * @implemented
 */
HBITMAP
WINAPI
CreateBitmap(
    INT Width,
    INT Height,
    UINT Planes,
    UINT BitsPixel,
    CONST VOID* pUnsafeBits)
{
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
HBITMAP
WINAPI
CreateBitmapIndirect(
    const BITMAP *pbm)
{
    HBITMAP bitmap = NULL;

    /* Note windows xp/2003 does not check if pbm is NULL or not */
    if ((pbm->bmWidthBytes != 0) && (!(pbm->bmWidthBytes & 1)))

    {
        bitmap = CreateBitmap(pbm->bmWidth, pbm->bmHeight, pbm->bmPlanes, pbm->bmBitsPixel,
            pbm->bmBits);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bitmap;
}

HBITMAP
WINAPI
CreateDiscardableBitmap(
    HDC hDC,
    INT Width,
    INT Height)
{
    return CreateCompatibleBitmap(hDC, Width, Height);
}

HBITMAP
WINAPI
CreateCompatibleBitmap(
    HDC hDC,
    INT Width,
    INT Height)
{
    PDC_ATTR pDc_Attr;

    if (!GdiGetHandleUserData(hDC, GDI_OBJECT_TYPE_DC, (PVOID) & pDc_Attr))
        return NULL;

    if (!Width || !Height)
        return GetStockObject(DEFAULT_BITMAP);

    if (!(pDc_Attr->ulDirty_ & DC_DIBSECTION))
    {
        return NtGdiCreateCompatibleBitmap(hDC, Width, Height);
    }
    else
    {
        HBITMAP hBmp = NULL;
        struct
        {
            BITMAP bitmap;
            BITMAPINFOHEADER bmih;
            RGBQUAD rgbquad[256];
        } buffer;
        DIBSECTION* pDIBs = (DIBSECTION*) &buffer;
        BITMAPINFO* pbmi = (BITMAPINFO*) &buffer.bmih;

        hBmp = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_BITMAP);

        if (GetObjectA(hBmp, sizeof(DIBSECTION), pDIBs) != sizeof(DIBSECTION))
            return NULL;

        if (pDIBs->dsBm.bmBitsPixel <= 8)
            GetDIBColorTable(hDC, 0, 256, buffer.rgbquad);

        pDIBs->dsBmih.biWidth = Width;
        pDIBs->dsBmih.biHeight = Height;

        return CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS, NULL, NULL, 0);
    }
    return NULL;
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
    UINT cjBmpScanSize;
    UINT cjInfoSize;

    if (!hDC || !GdiValidateHandle((HGDIOBJ) hDC) || !lpbmi)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    cjBmpScanSize = DIB_BitmapMaxBitsSize(lpbmi, cScanLines);
    /* Caller must provide maximum size possible */
    cjInfoSize = DIB_BitmapInfoSize(lpbmi, uUsage, TRUE);

    if (lpvBits)
    {
        if (lpbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
        {
            if (lpbmi->bmiHeader.biCompression == BI_JPEG
                || lpbmi->bmiHeader.biCompression == BI_PNG)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
    }

    return NtGdiGetDIBitsInternal(hDC, hbmp, uStartScan, cScanLines, lpvBits, lpbmi, uUsage,
        cjBmpScanSize, cjInfoSize);
}

/*
 * @implemented
 */
HBITMAP
WINAPI
CreateDIBitmap(
    HDC hDC,
    const BITMAPINFOHEADER *Header,
    DWORD Init,
    LPCVOID Bits,
    const BITMAPINFO *Data,
    UINT ColorUse)
{
    LONG Width, Height, Compression, DibSize;
    WORD Planes, BitsPerPixel;
//  PDC_ATTR pDc_Attr;
    UINT cjBmpScanSize = 0;
    HBITMAP hBitmap = NULL;
    PBITMAPINFO pbmiConverted;
    UINT cjInfoSize;

    /* Convert the BITMAPINFO if it is a COREINFO */
    pbmiConverted = ConvertBitmapInfo(Data, ColorUse, &cjInfoSize, FALSE);

    /* Check for CBM_CREATDIB */
    if (Init & CBM_CREATDIB)
    {
        if (cjInfoSize == 0)
        {
            goto Exit;
        }
        else if (Init & CBM_INIT)
        {
            if (Bits == NULL)
            {
                goto Exit;
            }
        }
        else
        {
            Bits = NULL;
        }

        /* CBM_CREATDIB needs Data. */
        if (pbmiConverted == NULL)
        {
            DPRINT1("CBM_CREATDIB needs a BITMAPINFO!\n");
            goto Exit;
        }

        /* It only works with PAL or RGB */
        if (ColorUse > DIB_PAL_COLORS)
        {
            DPRINT1("Invalid ColorUse: %lu\n", ColorUse);
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            goto Exit;
        }

        /* Use the header from the data */
        Header = &Data->bmiHeader;
    }
    else
    {
        if (Init & CBM_INIT)
        {
            if (Bits != NULL)
            {
                if (cjInfoSize == 0)
                {
                    goto Exit;
                }
            }
            else
            {
                Init &= ~CBM_INIT;
            }
        }
    }

    /* Header is required */
    if (!Header)
    {
        DPRINT1("Header is NULL\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    /* Get the bitmap format and dimensions */
    if (DIB_GetBitmapInfo(Header, &Width, &Height, &Planes, &BitsPerPixel, &Compression, &DibSize) == -1)
    {
        DPRINT1("DIB_GetBitmapInfo failed!\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    /* Check if the Compr is incompatible */
    if ((Compression == BI_JPEG) || (Compression == BI_PNG))
    {
        DPRINT1("Invalid compression: %lu!\n", Compression);
        goto Exit;
    }

    /* Only DIB_RGB_COLORS (0), DIB_PAL_COLORS (1) and 2 are valid. */
    if (ColorUse > DIB_PAL_COLORS + 1)
    {
        DPRINT1("Invalid compression: %lu!\n", Compression);
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    /* If some Bits are given, only DIB_PAL_COLORS and DIB_RGB_COLORS are valid */
    if (Bits && (ColorUse > DIB_PAL_COLORS))
    {
        DPRINT1("Invalid ColorUse: %lu\n", ColorUse);
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    /* Negative width is not allowed */
    if (Width < 0)
    {
        DPRINT1("Negative width: %li\n", Width);
        goto Exit;
    }

    /* Top-down DIBs have a negative height. */
    Height = abs(Height);

// For Icm support.
// GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))

    cjBmpScanSize = GdiGetBitmapBitsSize(pbmiConverted);

    DPRINT("pBMI %p, Size bpp %u, dibsize %d, Conv %u, BSS %u\n",
           Data, BitsPerPixel, DibSize, cjInfoSize, cjBmpScanSize);

    if (!Width || !Height)
    {
        hBitmap = GetStockObject(DEFAULT_BITMAP);
    }
    else
    {
        hBitmap = NtGdiCreateDIBitmapInternal(hDC,
                                              Width,
                                              Height,
                                              Init,
                                              (LPBYTE)Bits,
                                              (LPBITMAPINFO)pbmiConverted,
                                              ColorUse,
                                              cjInfoSize,
                                              cjBmpScanSize,
                                              0, 0);
    }

Exit:
    /* Cleanup converted BITMAPINFO */
    if ((pbmiConverted != NULL) && (pbmiConverted != Data))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, pbmiConverted);
    }

    return hBitmap;
}

/*
 * @implemented
 */
INT
WINAPI
SetDIBits(
    HDC hDC,
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

    if (!lpvBits || (GDI_HANDLE_GET_TYPE(hBitmap) != GDI_OBJECT_TYPE_BITMAP))
        return 0;

    if (lpbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
    {
        if (lpbmi->bmiHeader.biCompression == BI_JPEG
            || lpbmi->bmiHeader.biCompression == BI_PNG)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    hDCc = NtGdiGetDCforBitmap(hBitmap); // hDC can be NULL, so, get it from the bitmap.
    SavehDC = hDCc;
    if (!hDCc) // No DC associated with bitmap, Clone or Create one.
    {
        nhDC = CreateCompatibleDC(hDC);
        if (!nhDC)
            return 0;
        newDC = TRUE;
        SavehDC = nhDC;
    }
    else if (!SaveDC(hDCc))
        return 0;

    hOldBitmap = SelectObject(SavehDC, hBitmap);

    if (hOldBitmap)
    {
        if (hDC)
            hPal = SelectPalette(SavehDC, (HPALETTE) GetCurrentObject(hDC, OBJ_PAL), FALSE);

        if (lpbmi->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
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

        LinesCopied = SetDIBitsToDevice(SavehDC, 0, 0, dwWidth, dwHeight, 0, 0, uStartScan,
            cScanLines, (void *) lpvBits, (LPBITMAPINFO) lpbmi, fuColorUse);

        if (hDC)
            SelectPalette(SavehDC, hPal, FALSE);

        SelectObject(SavehDC, hOldBitmap);
    }

    if (newDC)
        DeleteDC(SavehDC);
    else
        RestoreDC(SavehDC, -1);

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
    BOOL Hit = FALSE;
    PVOID pvSafeBits = (PVOID) Bits;

    if (!ScanLines || !lpbmi || !Bits)
        return 0;

    if (ColorUse && ColorUse != DIB_PAL_COLORS && ColorUse != DIB_PAL_COLORS + 1)
        return 0;

    pConvertedInfo = ConvertBitmapInfo(lpbmi, ColorUse, &ConvertedInfoSize, FALSE);
    if (!pConvertedInfo)
        return 0;

    HANDLE_METADC(INT,
                  SetDIBitsToDevice,
                  0,
                  hdc,
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

    // Handle the "Special Case"!
    {
        PLDC pldc;
        ULONG hType = GDI_HANDLE_GET_TYPE(hdc);
        if (hType != GDILoObjType_LO_DC_TYPE && hType != GDILoObjType_LO_METADC16_TYPE)
        {
            pldc = GdiGetLDC(hdc);
            if (pldc)
            {
                if (pldc->Flags & LDC_STARTPAGE) StartPage(hdc);

                if (pldc->Flags & LDC_SAPCALLBACK) GdiSAPCallback(pldc);

                if (pldc->Flags & LDC_KILL_DOCUMENT)
                {
                    LinesCopied = 0;
                    goto Exit;
                }
            }
            else
            {
                SetLastError(ERROR_INVALID_HANDLE);
                LinesCopied = 0;
                goto Exit;
            }
        }
    }

    if ((pConvertedInfo->bmiHeader.biCompression == BI_RLE8) ||
            (pConvertedInfo->bmiHeader.biCompression == BI_RLE4))
    {
        /* For compressed data, we must set the whole thing */
        StartScan = 0;
        ScanLines = pConvertedInfo->bmiHeader.biHeight;
    }

    cjBmpScanSize = DIB_BitmapMaxBitsSize((LPBITMAPINFO) lpbmi, ScanLines);

    pvSafeBits = RtlAllocateHeap(GetProcessHeap(), 0, cjBmpScanSize);
    if (pvSafeBits)
    {
        _SEH2_TRY
        {
            RtlCopyMemory(pvSafeBits, Bits, cjBmpScanSize);
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
            DPRINT1("SetDIBitsToDevice fail to read BitMapInfo: %p or Bits: %p & Size: %u\n",
                pConvertedInfo, Bits, cjBmpScanSize);
        }
        DPRINT("SetDIBitsToDevice Allocate Bits %u!!!\n", cjBmpScanSize);
    }

    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID) & pDc_Attr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*
     if ( !pDc_Attr || // DC is Public
     ColorUse == DIB_PAL_COLORS ||
     ((pConvertedInfo->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) &&
     (pConvertedInfo->bmiHeader.biCompression == BI_JPEG ||
     pConvertedInfo->bmiHeader.biCompression  == BI_PNG )) )*/
    {
        LinesCopied = NtGdiSetDIBitsToDeviceInternal(hdc, XDest, YDest, Width, Height, XSrc, YSrc,
            StartScan, ScanLines, (LPBYTE) pvSafeBits, (LPBITMAPINFO) pConvertedInfo, ColorUse,
            cjBmpScanSize, ConvertedInfoSize,
            TRUE,
            NULL);
    }
Exit:
    if (Bits != pvSafeBits)
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
StretchDIBits(
    HDC hdc,
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

    DPRINT("StretchDIBits %p : %p : %u\n", lpBits, lpBitsInfo, iUsage);

    HANDLE_METADC( int,
                   StretchDIBits,
                   0,
                   hdc,
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
                   dwRop );

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return 0;

    pConvertedInfo = ConvertBitmapInfo(lpBitsInfo, iUsage, &ConvertedInfoSize, FALSE);
    if (!pConvertedInfo)
    {
        return 0;
    }

    cjBmpScanSize = GdiGetBitmapBitsSize((BITMAPINFO *) pConvertedInfo);

    if (lpBits)
    {
        pvSafeBits = RtlAllocateHeap(GetProcessHeap(), 0, cjBmpScanSize);
        if (pvSafeBits)
        {
            _SEH2_TRY
            {
                RtlCopyMemory(pvSafeBits, lpBits, cjBmpScanSize);
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
                DPRINT1("StretchDIBits fail to read BitMapInfo: %p or Bits: %p & Size: %u\n",
                    pConvertedInfo, lpBits, cjBmpScanSize);
            }
            DPRINT("StretchDIBits Allocate Bits %u!!!\n", cjBmpScanSize);
        }
    }

    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID) & pDc_Attr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*
     if ( !pDc_Attr ||
     iUsage == DIB_PAL_COLORS ||
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
                                                  (DWORD) iUsage,
                                                  dwRop,
                                                  ConvertedInfoSize,
                                                  cjBmpScanSize,
                                                  NULL );
    }
    if (pvSafeBits)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pvSafeBits);
    if (lpBitsInfo != pConvertedInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);

    return LinesCopied;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetBitmapAttributes(HBITMAP hbm)
{
    if ( GDI_HANDLE_IS_STOCKOBJ(hbm) )
    {
        return SC_BB_STOCKOBJ;
    }
    return 0;
}

/*
 * @implemented
 */
HBITMAP
WINAPI
SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
    if ( dwFlags & ~SC_BB_STOCKOBJ )
    {
        return NULL;
    }
    return NtGdiSetBitmapAttributes( hbm, dwFlags );
}

/*
 * @implemented
 */
HBITMAP
WINAPI
ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
    if ( dwFlags & ~SC_BB_STOCKOBJ )
    {
        return NULL;
    }
    return NtGdiClearBitmapAttributes( hbm, dwFlags );;
}


