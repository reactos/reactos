#include <precomp.h>

/* LoadLPK global variables */
HINSTANCE hLpk = NULL;
LPKETO LpkExtTextOut = NULL;
LPKGCP LpkGetCharacterPlacement = NULL;
LPKGTEP LpkGetTextExtentExPoint = NULL;

/**
 * @name CalculateColorTableSize
 *
 * Internal routine to calculate the number of color table entries.
 *
 * @param BitmapInfoHeader
 *        Input bitmap information header, can be any version of
 *        BITMAPINFOHEADER or BITMAPCOREHEADER.
 *
 * @param ColorSpec
 *        Pointer to variable which specifiing the color mode (DIB_RGB_COLORS
 *        or DIB_RGB_COLORS). On successful return this value is normalized
 *        according to the bitmap info.
 *
 * @param ColorTableSize
 *        On successful return this variable is filled with number of
 *        entries in color table for the image with specified parameters.
 *
 * @return
 *    TRUE if the input values together form a valid image, FALSE otherwise.
 */

BOOL WINAPI
CalculateColorTableSize(
    CONST BITMAPINFOHEADER *BitmapInfoHeader,
    UINT *ColorSpec,
    UINT *ColorTableSize)
{
    WORD BitCount;
    DWORD ClrUsed;
    DWORD Compression;

    /*
     * At first get some basic parameters from the passed BitmapInfoHeader
     * structure. It can have one of the following formats:
     * - BITMAPCOREHEADER (the oldest one with totally different layout
     *                     from the others)
     * - BITMAPINFOHEADER (the standard and most common header)
     * - BITMAPV4HEADER (extension of BITMAPINFOHEADER)
     * - BITMAPV5HEADER (extension of BITMAPV4HEADER)
     */

    if (BitmapInfoHeader->biSize == sizeof(BITMAPCOREHEADER))
    {
        BitCount = ((LPBITMAPCOREHEADER)BitmapInfoHeader)->bcBitCount;
        ClrUsed = 0;
        Compression = BI_RGB;
    }
    else
    {
        BitCount = BitmapInfoHeader->biBitCount;
        ClrUsed = BitmapInfoHeader->biClrUsed;
        Compression = BitmapInfoHeader->biCompression;
    }

    switch (Compression)
    {
    case BI_BITFIELDS:
        if (*ColorSpec == DIB_PAL_COLORS)
            *ColorSpec = DIB_RGB_COLORS;

        if (BitCount != 16 && BitCount != 32)
            return FALSE;

        /*
         * For BITMAPV4HEADER/BITMAPV5HEADER the masks are included in
         * the structure itself (bV4RedMask, bV4GreenMask, and bV4BlueMask).
         * For BITMAPINFOHEADER the color masks are stored in the palette.
         */

        if (BitmapInfoHeader->biSize > sizeof(BITMAPINFOHEADER))
            *ColorTableSize = 0;
        else
            *ColorTableSize = 3;

        return TRUE;

    case BI_RGB:
        switch (BitCount)
        {
        case 1:
            *ColorTableSize = ClrUsed ? min(ClrUsed, 2) : 2;
            return TRUE;

        case 4:
            *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
            return TRUE;

        case 8:
            *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
            return TRUE;

        default:
            if (*ColorSpec == DIB_PAL_COLORS)
                *ColorSpec = DIB_RGB_COLORS;
            if (BitCount != 16 && BitCount != 24 && BitCount != 32)
                return FALSE;
            *ColorTableSize = ClrUsed;
            return TRUE;
        }

    case BI_RLE4:
        if (BitCount == 4)
        {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
            return TRUE;
        }
        return FALSE;

    case BI_RLE8:
        if (BitCount == 8)
        {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
            return TRUE;
        }
        return FALSE;

    case BI_JPEG:
    case BI_PNG:
        *ColorTableSize = ClrUsed;
        return TRUE;

    default:
        return FALSE;
    }
}

/**
 * @name ConvertBitmapInfo
 *
 * Internal routine to convert a user-passed BITMAPINFO structure into
 * unified BITMAPINFO structure.
 *
 * @param BitmapInfo
 *        Input bitmap info, can be any version of BITMAPINFO or
 *        BITMAPCOREINFO.
 * @param ColorSpec
 *        Specifies whether the bmiColors member of the BITMAPINFO structure
 *        contains a valid color table and, if so, whether the entries in
 *        this color table contain explicit red, green, blue (DIB_RGB_COLORS)
 *        values or palette indexes (DIB_PAL_COLORS).
 * @param BitmapInfoSize
 *        On successful return contains the size of the returned BITMAPINFO
 *        structure. If FollowedByData is TRUE the size includes the number
 *        of bytes occupied by the image data.
 * @param FollowedByData
 *        Specifies if the BITMAPINFO header is immediately followed
 *        by the actual bitmap data (eg. as passed to CreateDIBPatternBrush).
 *
 * @return
 *    Either the original BitmapInfo or newly allocated structure is
 *    returned. For the later case the caller is responsible for freeing the
 *    memory using RtlFreeHeap with the current process heap.
 *
 * @example
 *    PBITMAPINFO NewBitmapInfo;
 *    UINT NewBitmapInfoSize;
 *
 *    NewBitmapInfo = ConvertBitmapInfo(OldBitmapInfo, DIB_RGB_COLORS,
 *                                      &NewBitmapInfoSize, FALSE);
 *    if (NewBitmapInfo)
 *    {
 *       <do something with the bitmap info>
 *       if (NewBitmapInfo != OldBitmapInfo)
 *          RtlFreeHeap(RtlGetProcessHeap(), 0, NewBitmapInfo);
 *    }
 */

LPBITMAPINFO WINAPI
ConvertBitmapInfo(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT *BitmapInfoSize,
    BOOL FollowedByData)
{
    LPBITMAPINFO NewBitmapInfo = (LPBITMAPINFO)BitmapInfo;
    LPBITMAPCOREINFO CoreBitmapInfo = (LPBITMAPCOREINFO)BitmapInfo;
    DWORD Size = 0;
    ULONG DataSize = 0;
    UINT PaletteEntryCount = 0;

    /*
     * At first check if the passed BitmapInfo structure has valid size. It
     * can have one of these headers: BITMAPCOREHEADER, BITMAPINFOHEADER,
     * BITMAPV4HEADER or BITMAPV5HEADER (see CalculateColorTableSize for
     * description).
     */

    if ( !BitmapInfo ||
            (BitmapInfo->bmiHeader.biSize != sizeof(BITMAPCOREHEADER) &&
             (BitmapInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER) ||
              BitmapInfo->bmiHeader.biSize > sizeof(BITMAPV5HEADER))))
    {
        return NULL;
    }

    /*
     * Now calculate the color table size. Also if the bitmap info contains
     * invalid color information it's rejected here.
     */

    if (!CalculateColorTableSize(&BitmapInfo->bmiHeader, &ColorSpec,
                                 &PaletteEntryCount))
    {
        return NULL;
    }

    /*
     * Calculate the size of image data if applicable. We must be careful
     * to do proper aligning on line ends.
     */

    if (FollowedByData)
    {
        DataSize = GdiGetBitmapBitsSize((PBITMAPINFO)BitmapInfo );
    }

    /*
     * If BitmapInfo was originally BITMAPCOREINFO then we need to convert
     * it to the standard BITMAPINFO layout.
     */

    if (BitmapInfo->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        Size = sizeof(BITMAPINFOHEADER);
        if (ColorSpec == DIB_RGB_COLORS)
            Size += PaletteEntryCount * sizeof(RGBQUAD);
        else
            Size += PaletteEntryCount * sizeof(USHORT);
        Size += DataSize;

        NewBitmapInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (NewBitmapInfo == NULL)
        {
            return NULL;
        }

        NewBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        NewBitmapInfo->bmiHeader.biWidth = CoreBitmapInfo->bmciHeader.bcWidth;
        NewBitmapInfo->bmiHeader.biHeight = CoreBitmapInfo->bmciHeader.bcHeight;
        NewBitmapInfo->bmiHeader.biPlanes = CoreBitmapInfo->bmciHeader.bcPlanes;
        NewBitmapInfo->bmiHeader.biBitCount = CoreBitmapInfo->bmciHeader.bcBitCount;
        NewBitmapInfo->bmiHeader.biCompression = BI_RGB;
        NewBitmapInfo->bmiHeader.biSizeImage = 0;
        NewBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
        NewBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
        NewBitmapInfo->bmiHeader.biClrUsed = 0;
        NewBitmapInfo->bmiHeader.biClrImportant = 0;

        if (PaletteEntryCount != 0)
        {
            if (ColorSpec == DIB_RGB_COLORS)
            {
                ULONG Index;

                for (Index = 0; Index < PaletteEntryCount; Index++)
                {
                    NewBitmapInfo->bmiColors[Index].rgbRed =
                        CoreBitmapInfo->bmciColors[Index].rgbtRed;
                    NewBitmapInfo->bmiColors[Index].rgbGreen =
                        CoreBitmapInfo->bmciColors[Index].rgbtGreen;
                    NewBitmapInfo->bmiColors[Index].rgbBlue =
                        CoreBitmapInfo->bmciColors[Index].rgbtBlue;
                    NewBitmapInfo->bmiColors[Index].rgbReserved = 0;
                }
            }
            else
            {
                RtlCopyMemory(NewBitmapInfo->bmiColors,
                              CoreBitmapInfo->bmciColors,
                              PaletteEntryCount * sizeof(USHORT));
            }
        }

        if (FollowedByData)
        {
            ULONG_PTR NewDataPtr, OldDataPtr;

            if (ColorSpec == DIB_RGB_COLORS)
            {
                NewDataPtr = (ULONG_PTR)(NewBitmapInfo->bmiColors +
                                         PaletteEntryCount);
                OldDataPtr = (ULONG_PTR)(CoreBitmapInfo->bmciColors +
                                         PaletteEntryCount);
            }
            else
            {
                NewDataPtr = (ULONG_PTR)(NewBitmapInfo->bmiColors) +
                             PaletteEntryCount * sizeof(USHORT);
                OldDataPtr = (ULONG_PTR)(CoreBitmapInfo->bmciColors) +
                             PaletteEntryCount * sizeof(USHORT);
            }

            RtlCopyMemory((PVOID)NewDataPtr, (PVOID)OldDataPtr, DataSize);
        }
    }
    else
    {
        /* Verify some data validity */
        switch (BitmapInfo->bmiHeader.biCompression)
        {
            case BI_RLE8:
                if (BitmapInfo->bmiHeader.biBitCount != 8)
                    return NULL;
                if (BitmapInfo->bmiHeader.biHeight < 0)
                    return NULL;
                break;
            case BI_RLE4:
                if (BitmapInfo->bmiHeader.biBitCount != 4)
                    return NULL;
                if (BitmapInfo->bmiHeader.biHeight < 0)
                    return NULL;
                break;
            default:
                break;
        }

        /* Non "standard" formats must have a valid size set */
        if ((BitmapInfo->bmiHeader.biCompression != BI_RGB) &&
                (BitmapInfo->bmiHeader.biCompression != BI_BITFIELDS))
        {
            if (BitmapInfo->bmiHeader.biSizeImage == 0)
                return NULL;
        }
    }

    Size = NewBitmapInfo->bmiHeader.biSize;
    if (ColorSpec == DIB_RGB_COLORS)
        Size += PaletteEntryCount * sizeof(RGBQUAD);
    else
        Size += PaletteEntryCount * sizeof(USHORT);
    Size += DataSize;
    *BitmapInfoSize = Size;

    return NewBitmapInfo;
}

VOID
WINAPI
LogFontA2W(LPLOGFONTW pW, CONST LOGFONTA *pA)
{
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, pA->f, len, pW->f, len )
#define COPYN(f) pW->f = pA->f

    COPYN(lfHeight);
    COPYN(lfWidth);
    COPYN(lfEscapement);
    COPYN(lfOrientation);
    COPYN(lfWeight);
    COPYN(lfItalic);
    COPYN(lfUnderline);
    COPYN(lfStrikeOut);
    COPYN(lfCharSet);
    COPYN(lfOutPrecision);
    COPYN(lfClipPrecision);
    COPYN(lfQuality);
    COPYN(lfPitchAndFamily);
    COPYS(lfFaceName,LF_FACESIZE);
    pW->lfFaceName[LF_FACESIZE - 1] = '\0';

#undef COPYN
#undef COPYS
}

VOID
WINAPI
LogFontW2A(LPLOGFONTA pA, CONST LOGFONTW *pW)
{
#define COPYS(f,len) WideCharToMultiByte ( CP_THREAD_ACP, 0, pW->f, len, pA->f, len, NULL, NULL )
#define COPYN(f) pA->f = pW->f

    COPYN(lfHeight);
    COPYN(lfWidth);
    COPYN(lfEscapement);
    COPYN(lfOrientation);
    COPYN(lfWeight);
    COPYN(lfItalic);
    COPYN(lfUnderline);
    COPYN(lfStrikeOut);
    COPYN(lfCharSet);
    COPYN(lfOutPrecision);
    COPYN(lfClipPrecision);
    COPYN(lfQuality);
    COPYN(lfPitchAndFamily);
    COPYS(lfFaceName,LF_FACESIZE);
    pA->lfFaceName[LF_FACESIZE - 1] = '\0';

#undef COPYN
#undef COPYS
}

VOID
WINAPI
EnumLogFontExW2A( LPENUMLOGFONTEXA fontA, CONST ENUMLOGFONTEXW *fontW )
{
    LogFontW2A( (LPLOGFONTA)fontA, (CONST LOGFONTW *)fontW );

    WideCharToMultiByte( CP_THREAD_ACP, 0, fontW->elfFullName, -1,
                         (LPSTR) fontA->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    fontA->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_THREAD_ACP, 0, fontW->elfStyle, -1,
                         (LPSTR) fontA->elfStyle, LF_FACESIZE, NULL, NULL );
    fontA->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_THREAD_ACP, 0, fontW->elfScript, -1,
                         (LPSTR) fontA->elfScript, LF_FACESIZE, NULL, NULL );
    fontA->elfScript[LF_FACESIZE-1] = '\0';
}

/*
* LPK.DLL loader function
*
* Returns TRUE if a valid parameter was passed and loading was successful,
* returns FALSE otherwise.
*/
BOOL WINAPI LoadLPK(INT LpkFunctionID)
{
    if(!hLpk) // Check if the DLL is already loaded
        hLpk = LoadLibraryW(L"lpk.dll");

    if (hLpk)
    {
        switch (LpkFunctionID)
        {
            case LPK_INIT:
                return TRUE;

            case LPK_ETO:
                if (!LpkExtTextOut) // Check if the function is already loaded
                    LpkExtTextOut = (LPKETO) GetProcAddress(hLpk, "LpkExtTextOut");

                if (!LpkExtTextOut)
                {
                    FreeLibrary(hLpk);
                    return FALSE;
                }

                return TRUE;

            case LPK_GCP:
                if (!LpkGetCharacterPlacement) // Check if the function is already loaded
                    LpkGetCharacterPlacement = (LPKGCP) GetProcAddress(hLpk, "LpkGetCharacterPlacement");

                if (!LpkGetCharacterPlacement)
                {
                    FreeLibrary(hLpk);
                    return FALSE;
                }

                return TRUE;

            case LPK_GTEP:
                if (!LpkGetTextExtentExPoint) // Check if the function is already loaded
                    LpkGetTextExtentExPoint = (LPKGTEP) GetProcAddress(hLpk, "LpkGetTextExtentExPoint");

                if (!LpkGetTextExtentExPoint)
                {
                    FreeLibrary(hLpk);
                    return FALSE;
                }

                return TRUE;

            default:
                FreeLibrary(hLpk);
                return FALSE;
        }
    }

    else
        return FALSE;
}
