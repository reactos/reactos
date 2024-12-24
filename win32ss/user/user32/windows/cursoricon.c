/*
 * PROJECT:         ReactOS user32.dll
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            win32ss/user/user32/windows/cursoricon.c
 * PURPOSE:         cursor and icons implementation
 * PROGRAMMER:      Jérôme Gardou (jerome.gardou@reactos.org)
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(cursor);
WINE_DECLARE_DEBUG_CHANNEL(icon);
//WINE_DECLARE_DEBUG_CHANNEL(resource);

/* We only use Wide string functions */
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE MAKEINTRESOURCEW

/************* USER32 INTERNAL FUNCTIONS **********/

VOID LoadSystemCursors(VOID)
{
   if (!gpsi->hIconSmWindows)
   {
       ERR("Loading System Cursors\n");
       NtUserSetSystemCursor(LoadImageW( 0, IDC_ARROW,       IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_NORMAL);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_IBEAM,       IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_IBEAM);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_WAIT,        IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_WAIT);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_CROSS,       IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_CROSS);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_UPARROW,     IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_UP);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_ICON,        IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_ICON);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZE,        IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZE);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZENWSE,    IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZENWSE);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZENESW,    IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZENESW);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZEWE,      IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZEWE);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZENS,      IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZENS);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_SIZEALL,     IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_SIZEALL);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_NO,          IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_NO);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_HAND,        IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_HAND);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_APPSTARTING, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_APPSTARTING);
       NtUserSetSystemCursor(LoadImageW( 0, IDC_HELP,        IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ), OCR_HELP);
   }
}

/* This callback routine is called directly after switching to gui mode */
NTSTATUS
WINAPI
User32SetupDefaultCursors(PVOID Arguments,
                          ULONG ArgumentLength)
{
    BOOL *DefaultCursor = (BOOL*)Arguments;
    HCURSOR hCursor;

    /* Load system cursors first */
    LoadSystemCursors();

    if(*DefaultCursor)
    {
        /* set default cursor */
        hCursor = LoadCursorW(0, IDC_ARROW);
        SetCursor(hCursor);
    }
    else
    {
        /* FIXME load system cursor scheme */
        SetCursor(0);
        hCursor = LoadCursorW(0, IDC_ARROW);
        SetCursor(hCursor);
    }

    return(ZwCallbackReturn(&hCursor, sizeof(HCURSOR), STATUS_SUCCESS));
}

BOOL get_icon_size(HICON hIcon, SIZE *size)
{
    return NtUserGetIconSize(hIcon, 0, &size->cx, &size->cy);
}

HCURSOR CursorIconToCursor(HICON hIcon, BOOL SemiTransparent)
{
    UNIMPLEMENTED;
    return NULL;
}

/************* IMPLEMENTATION HELPERS ******************/

static const WCHAR DISPLAYW[] = L"DISPLAY";

static void *map_fileW( LPCWSTR name, LPDWORD filesize )
{
    HANDLE hFile, hMapping;
    LPVOID ptr = NULL;

    hFile = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0 );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hMapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
        if (hMapping)
        {
            ptr = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle( hMapping );
            if (filesize)
                *filesize = GetFileSize( hFile, NULL );
        }
        CloseHandle( hFile );
    }
    return ptr;
}

static int get_dib_image_size( int width, int height, int depth )
{
    return (((width * depth + 31) / 8) & ~3) * abs( height );
}

static BOOL is_dib_monochrome( const BITMAPINFO* info )
{
    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const RGBTRIPLE *rgb = ((const BITMAPCOREINFO*)info)->bmciColors;

        if (((const BITMAPCOREINFO*)info)->bmciHeader.bcBitCount != 1) return FALSE;

        /* Check if the first color is black */
        if ((rgb->rgbtRed == 0) && (rgb->rgbtGreen == 0) && (rgb->rgbtBlue == 0))
        {
            rgb++;

            /* Check if the second color is white */
            return ((rgb->rgbtRed == 0xff) && (rgb->rgbtGreen == 0xff)
                 && (rgb->rgbtBlue == 0xff));
        }
        else return FALSE;
    }
    else  /* assume BITMAPINFOHEADER */
    {
        const RGBQUAD *rgb = info->bmiColors;

        if (info->bmiHeader.biBitCount != 1) return FALSE;

        /* Check if the first color is black */
        if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) &&
            (rgb->rgbBlue == 0) && (rgb->rgbReserved == 0))
        {
            rgb++;

            /* Check if the second color is white */
            return ((rgb->rgbRed == 0xff) && (rgb->rgbGreen == 0xff)
                 && (rgb->rgbBlue == 0xff) && (rgb->rgbReserved == 0));
        }
        else return FALSE;
    }
}

/* Return the size of the bitmap info structure including color table and
 * the bytes required for 3 DWORDS if this is a BI_BITFIELDS bmp. */
static int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
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
        if (colors > 256) /* buffer overflow otherwise */
                colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        /* Account for BI_BITFIELDS in BITMAPINFOHEADER(v1-v3) bmp's. The
         * 'max' selection using biSize below will exclude v4 & v5's. */
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        /* Test for BI_BITFIELDS format and either 16 or 32 bpp.
         * If so, account for the 3 DWORD masks (RGB Order).
         * BITMAPCOREHEADER tested above has no 16 or 32 bpp types.
         * See table "All of the possible pixel formats in a DIB"
         * at https://en.wikipedia.org/wiki/BMP_file_format. */
        if (info->bmiHeader.biSize >= sizeof(BITMAPV4HEADER) &&
            info->bmiHeader.biCompression == BI_BITFIELDS &&
            (info->bmiHeader.biBitCount == 16 || info->bmiHeader.biBitCount == 32))
        {
            size += 3 * sizeof(DWORD);  // BI_BITFIELDS
        }
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, DWORD *compr )
{
    #define CR 13
    #define LF 10
    #define EOFM 26 // DOS End Of File Marker
    #define HighBitDetect 0x89 // Byte with high bit set to test if not 7-bit
    /* wine's definition */
    static const BYTE png_sig_pattern[] = { HighBitDetect, 'P', 'N', 'G', CR, LF, EOFM, LF };
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        return 0;
    }
    else if (header->biSize == sizeof(BITMAPINFOHEADER) ||
             header->biSize == sizeof(BITMAPV4HEADER) ||
             header->biSize == sizeof(BITMAPV5HEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return 1;
    }
    if (memcmp(&header->biSize, png_sig_pattern, sizeof(png_sig_pattern)) == 0)
    {
        ERR("Cannot yet display PNG icons\n");
        /* for PNG format details see https://en.wikipedia.org/wiki/PNG */
    }
    else
    {
        ERR("Unknown/wrong size for header of 0x%x\n", header->biSize );
    }
    return -1;
}

/* copy an icon bitmap, even when it can't be selected into a DC */
/* helper for CreateIconIndirect */
static void stretch_blt_icon(HDC hdc_dst, int dst_width, int dst_height, HBITMAP src)
{
    HDC hdc = CreateCompatibleDC( 0 );
    BITMAP bm;
    HBITMAP hbmpPrev;

    GetObjectW(src, sizeof(bm), &bm);

    hbmpPrev = SelectObject(hdc, src);

    if (!hbmpPrev)  /* do it the hard way */
    {
        BITMAPINFO *info;
        void *bits;

        if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) return;
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = bm.bmWidth;
        info->bmiHeader.biHeight = bm.bmHeight;
        info->bmiHeader.biPlanes = GetDeviceCaps( hdc_dst, PLANES );
        info->bmiHeader.biBitCount = GetDeviceCaps( hdc_dst, BITSPIXEL );
        info->bmiHeader.biCompression = BI_RGB;
        info->bmiHeader.biSizeImage = get_dib_image_size( bm.bmWidth, bm.bmHeight, info->bmiHeader.biBitCount );
        info->bmiHeader.biXPelsPerMeter = 0;
        info->bmiHeader.biYPelsPerMeter = 0;
        info->bmiHeader.biClrUsed = 0;
        info->bmiHeader.biClrImportant = 0;
        bits = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
        if (bits && GetDIBits( hdc, src, 0, bm.bmHeight, bits, info, DIB_RGB_COLORS ))
            StretchDIBits( hdc_dst, 0, 0, dst_width, dst_height,
                           0, 0, bm.bmWidth, bm.bmHeight, bits, info, DIB_RGB_COLORS, SRCCOPY );

        HeapFree( GetProcessHeap(), 0, bits );
        HeapFree( GetProcessHeap(), 0, info );
    }
    else
    {
        StretchBlt( hdc_dst, 0, 0, dst_width, dst_height, hdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
        SelectObject(hdc, hbmpPrev);
    }

    DeleteDC( hdc );
}

/***********************************************************************
 *          bmi_has_alpha
 */
static BOOL bmi_has_alpha( const BITMAPINFO *info, const void *bits )
{
    int i;
    BOOL has_alpha = FALSE;
    const unsigned char *ptr = bits;

    if (info->bmiHeader.biBitCount != 32) return FALSE;
    for (i = 0; i < info->bmiHeader.biWidth * abs(info->bmiHeader.biHeight); i++, ptr += 4)
        if ((has_alpha = (ptr[3] != 0))) break;
    return has_alpha;
}

/***********************************************************************
 *          create_alpha_bitmap
 *
 * Create the alpha bitmap for a 32-bpp icon that has an alpha channel.
 */
static
HBITMAP
create_alpha_bitmap(
    _In_opt_  HBITMAP color,
    _In_opt_  BITMAPINFO *src_info,
    _In_opt_  const void *color_bits,
    _In_ LONG width,
    _In_ LONG height)
{
    HBITMAP alpha = NULL, hbmpOld;
    HDC hdc = NULL, hdcScreen;
    unsigned char *ptr;
    void *bits = NULL;
    ULONG size;

    hdcScreen = CreateDCW(DISPLAYW, NULL, NULL, NULL);
    if (!hdcScreen)
        return NULL;
    hdc = CreateCompatibleDC(hdcScreen);
    if (!hdc)
    {
        DeleteDC(hdcScreen);
        return NULL;
    }

    if (color)
    {
        BITMAP bm;
        BITMAPINFO *info = NULL;

        TRACE("Creating alpha bitmap from existing bitmap.\n");

        if (!GetObjectW( color, sizeof(bm), &bm ))
            goto done;
        if (bm.bmBitsPixel != 32)
            goto done;

        size = get_dib_image_size(bm.bmWidth, bm.bmHeight, 32);

        info = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(BITMAPINFO, bmiColors[256]));
        if(!info)
            goto done;
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = bm.bmWidth;
        info->bmiHeader.biHeight = -bm.bmHeight;
        info->bmiHeader.biPlanes = 1;
        info->bmiHeader.biBitCount = 32;
        info->bmiHeader.biCompression = BI_RGB;
        info->bmiHeader.biSizeImage = size;
        info->bmiHeader.biXPelsPerMeter = 0;
        info->bmiHeader.biYPelsPerMeter = 0;
        info->bmiHeader.biClrUsed = 0;
        info->bmiHeader.biClrImportant = 0;

        bits = HeapAlloc(GetProcessHeap(), 0, size);
        if(!bits)
        {
            HeapFree(GetProcessHeap(), 0, info);
            goto done;
        }
        if(!GetDIBits( hdc, color, 0, bm.bmHeight, bits, info, DIB_RGB_COLORS ))
        {
            HeapFree(GetProcessHeap(), 0, info);
            goto done;
        }
        if (!bmi_has_alpha( info, bits ))
        {
            HeapFree(GetProcessHeap(), 0, info);
            goto done;
        }

        /* pre-multiply by alpha */
        for (ptr = bits; ptr < ((BYTE*)bits + size); ptr += 4)
        {
            unsigned int alpha = ptr[3];
            ptr[0] = (ptr[0] * alpha) / 255;
            ptr[1] = (ptr[1] * alpha) / 255;
            ptr[2] = (ptr[2] * alpha) / 255;
        }

        /* Directly create a 32-bits DDB (thanks to undocumented CreateDIBitmap flag). */
        alpha = CreateDIBitmap(hdc, NULL, CBM_INIT | 2, bits, info, DIB_RGB_COLORS);

        HeapFree(GetProcessHeap(), 0, info);
    }
    else
    {
        WORD bpp;
        DWORD compr;
        LONG orig_width, orig_height;

        TRACE("Creating alpha bitmap from bitmap info.\n");

        if(!bmi_has_alpha(src_info, color_bits))
            goto done;

        if(!DIB_GetBitmapInfo(&src_info->bmiHeader, &orig_width, &orig_height, &bpp, &compr))
            goto done;
        if(bpp != 32)
            goto done;

        size = get_dib_image_size(orig_width, orig_height, bpp);
        bits = HeapAlloc(GetProcessHeap(), 0, size);
        if(!bits)
            goto done;
        CopyMemory(bits, color_bits, size);
        /* pre-multiply by alpha */
        for (ptr = bits; ptr < ((BYTE*)bits + size); ptr += 4)
        {
            unsigned int alpha = ptr[3];
            ptr[0] = (ptr[0] * alpha) / 255;
            ptr[1] = (ptr[1] * alpha) / 255;
            ptr[2] = (ptr[2] * alpha) / 255;
        }

        /* Create the bitmap. Set the bitmap info to have the right width and height */
        if(src_info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            ((BITMAPCOREHEADER*)&src_info->bmiHeader)->bcWidth = width;
            ((BITMAPCOREHEADER*)&src_info->bmiHeader)->bcHeight = height;
        }
        else
        {
            src_info->bmiHeader.biWidth = width;
            src_info->bmiHeader.biHeight = height;
        }
        /* Directly create a 32-bits DDB (thanks to undocumented CreateDIBitmap flag). */
        alpha = CreateDIBitmap(hdcScreen, NULL, 2, NULL, src_info, DIB_RGB_COLORS);
        /* Restore values */
        if(src_info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            ((BITMAPCOREHEADER*)&src_info->bmiHeader)->bcWidth = orig_width;
            ((BITMAPCOREHEADER*)&src_info->bmiHeader)->bcHeight = orig_height;
        }
        else
        {
            src_info->bmiHeader.biWidth = orig_width;
            src_info->bmiHeader.biHeight = orig_height;
        }
        if(!alpha)
            goto done;
        hbmpOld = SelectObject(hdc, alpha);
        if(!hbmpOld)
        {
            DeleteObject(alpha);
            alpha = NULL;
            goto done;
        }
        if(!StretchDIBits( hdc, 0, 0, width, height,
                   0, 0, orig_width, orig_height,
                   bits, src_info, DIB_RGB_COLORS, SRCCOPY ))
        {
            SelectObject(hdc, hbmpOld);
            hbmpOld = NULL;
            DeleteObject(alpha);
            alpha = NULL;
        }
        else
        {
            SelectObject(hdc, hbmpOld);
        }
    }

done:
    DeleteDC(hdcScreen);
    DeleteDC( hdc );
    if(bits) HeapFree(GetProcessHeap(), 0, bits);

    TRACE("Returning 0x%08x.\n", alpha);
    return alpha;
}

#include "pshpack1.h"

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

const CURSORICONFILEDIRENTRY*
get_best_icon_file_entry(
    _In_ const CURSORICONFILEDIR* dir,
    _In_ DWORD dwFileSize,
    _In_ int cxDesired,
    _In_ int cyDesired,
    _In_ BOOL bIcon,
    _In_ DWORD fuLoad
)
{
    CURSORICONDIR* fakeDir;
    CURSORICONDIRENTRY* fakeEntry;
    WORD i;
    const CURSORICONFILEDIRENTRY* entry;

    /* Check our file is what it claims to be */
    if ( dwFileSize < sizeof(*dir) )
        return NULL;

    if (dwFileSize < FIELD_OFFSET(CURSORICONFILEDIR, idEntries[dir->idCount]))
        return NULL;

    /*
     * Cute little hack:
     * We allocate a buffer, fake it as if it was a pointer to a resource in a module,
     * pass it to LookupIconIdFromDirectoryEx and get back the index we have to use
     */
    fakeDir = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(CURSORICONDIR, idEntries[dir->idCount]));
    if(!fakeDir)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    fakeDir->idReserved = 0;
    fakeDir->idType = dir->idType;
    fakeDir->idCount = dir->idCount;
    for(i = 0; i<dir->idCount; i++)
    {
        fakeEntry = &fakeDir->idEntries[i];
        entry = &dir->idEntries[i];
        /* Take this as an occasion to perform a size check */
        if ((entry->dwDIBOffset > dwFileSize)
                || ((entry->dwDIBOffset + entry->dwDIBSize) > dwFileSize))
        {
            ERR("Corrupted icon file?.\n");
            HeapFree(GetProcessHeap(), 0, fakeDir);
            return NULL;
        }
        /* File icon/cursors are not like resource ones */
        if(bIcon)
        {
            fakeEntry->ResInfo.icon.bWidth = entry->bWidth;
            fakeEntry->ResInfo.icon.bHeight = entry->bHeight;
            fakeEntry->ResInfo.icon.bColorCount = 0;
            fakeEntry->ResInfo.icon.bReserved = 0;
        }
        else
        {
            fakeEntry->ResInfo.cursor.wWidth = entry->bWidth;
            fakeEntry->ResInfo.cursor.wHeight = entry->bHeight;
        }
        /* Let's assume there's always one plane */
        fakeEntry->wPlanes = 1;
        /* We must get the bitcount from the BITMAPINFOHEADER itself */
        if (((BITMAPINFOHEADER *)((char *)dir + entry->dwDIBOffset))->biSize == sizeof(BITMAPCOREHEADER))
            fakeEntry->wBitCount = ((BITMAPCOREHEADER *)((char *)dir + entry->dwDIBOffset))->bcBitCount;
        else
            fakeEntry->wBitCount = ((BITMAPINFOHEADER *)((char *)dir + entry->dwDIBOffset))->biBitCount;
        fakeEntry->dwBytesInRes = entry->dwDIBSize;
        fakeEntry->wResId = i + 1;
    }

    /* Now call LookupIconIdFromResourceEx */
    i = LookupIconIdFromDirectoryEx((PBYTE)fakeDir, bIcon, cxDesired, cyDesired, fuLoad & LR_MONOCHROME);
    /* We don't need this anymore */
    HeapFree(GetProcessHeap(), 0, fakeDir);
    if(i == 0)
    {
        WARN("Unable to get a fit entry index.\n");
        return NULL;
    }

    /* We found it */
    return &dir->idEntries[i-1];
}

DWORD
get_best_icon_file_offset(
    _In_ const LPBYTE dir,
    _In_ DWORD dwFileSize,
    _In_ int cxDesired,
    _In_ int cyDesired,
    _In_ BOOL bIcon,
    _In_ DWORD fuLoad,
    _Out_ POINT *ptHotSpot
)
{
    const CURSORICONFILEDIRENTRY *entry;

    entry = get_best_icon_file_entry((CURSORICONFILEDIR *) dir, dwFileSize, cxDesired, cyDesired, bIcon, fuLoad);

    if(ptHotSpot)
    {
        ptHotSpot->x = entry->xHotspot;
        ptHotSpot->y = entry->yHotspot;
    }

    if(entry)
        return entry->dwDIBOffset;

    return 0;
}



/************* IMPLEMENTATION CORE ****************/

static BOOL CURSORICON_GetCursorDataFromBMI(
    _Inout_ CURSORDATA* pdata,
    _In_    const BITMAPINFO *pbmi
)
{
    UINT ubmiSize = bitmap_info_size(pbmi, DIB_RGB_COLORS);
    BOOL monochrome = is_dib_monochrome(pbmi);
    LONG width, height;
    WORD bpp;
    DWORD compr;
    int ibmpType;
    HDC hdc, hdcScreen;
    BITMAPINFO* pbmiCopy;
    HBITMAP hbmpOld = NULL;
    BOOL bResult = FALSE;
    const VOID *pvColor, *pvMask;

    ibmpType = DIB_GetBitmapInfo(&pbmi->bmiHeader, &width, &height, &bpp, &compr);
    /* Invalid data */
    if(ibmpType < 0)
        return FALSE;

    /* No compression for icons */
    if(compr != BI_RGB)
        return FALSE;

    /* If no dimensions were set, use the one from the icon */
    if(!pdata->cx) pdata->cx = width;
    if(!pdata->cy) pdata->cy = height < 0 ? -height/2 : height/2;

    /* Fix the hotspot coords */
    if(pdata->rt == (USHORT)((ULONG_PTR)RT_CURSOR))
    {
        if(pdata->cx != width)
            pdata->xHotspot = (pdata->xHotspot * pdata->cx) / width;
        if(pdata->cy != height/2)
            pdata->yHotspot = (pdata->yHotspot * pdata->cy * 2) / height;
    }
    else
    {
        pdata->xHotspot = pdata->cx/2;
        pdata->yHotspot = pdata->cy/2;
    }

    hdcScreen = CreateDCW(DISPLAYW, NULL, NULL, NULL);
    if(!hdcScreen)
        return FALSE;
    hdc = CreateCompatibleDC(hdcScreen);
    if(!hdc)
    {
        DeleteDC(hdcScreen);
        return FALSE;
    }

    pbmiCopy = HeapAlloc(GetProcessHeap(), 0, max(ubmiSize, FIELD_OFFSET(BITMAPINFO, bmiColors[3])));
    if(!pbmiCopy)
        goto done;
    RtlCopyMemory(pbmiCopy, pbmi, ubmiSize);

    /* In an icon/cursor, the BITMAPINFO holds twice the height */
    if(pbmiCopy->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        ((BITMAPCOREHEADER*)&pbmiCopy->bmiHeader)->bcHeight /= 2;
    else
        pbmiCopy->bmiHeader.biHeight /= 2;
    height /= 2;

    pvColor = (const char*)pbmi + ubmiSize;
    pvMask = (const char*)pvColor +
        get_dib_image_size(width, height, bpp );

    /* Set XOR bits */
    if(monochrome)
    {
        /* Create the 1bpp bitmap which will contain everything */
        pdata->hbmColor = NULL;
        pdata->hbmMask = CreateBitmap(pdata->cx, pdata->cy * 2, 1, 1, NULL);
        if(!pdata->hbmMask)
            goto done;
        hbmpOld = SelectObject(hdc, pdata->hbmMask);
        if(!hbmpOld)
            goto done;

        if(!StretchDIBits(hdc, 0, pdata->cy, pdata->cx, pdata->cy,
                          0, 0, width, height,
                          pvColor, pbmiCopy, DIB_RGB_COLORS, SRCCOPY))
            goto done;
        pdata->bpp = 1;
    }
    else
    {
        /* Create the bitmap. It has to be compatible with the screen surface */
        pdata->hbmColor = CreateCompatibleBitmap(hdcScreen, pdata->cx, pdata->cy);
        if(!pdata->hbmColor)
            goto done;
        /* Create the 1bpp mask bitmap */
        pdata->hbmMask = CreateBitmap(pdata->cx, pdata->cy, 1, 1, NULL);
        if(!pdata->hbmMask)
            goto done;
        hbmpOld = SelectObject(hdc, pdata->hbmColor);
        if(!hbmpOld)
            goto done;
        if(!StretchDIBits(hdc, 0, 0, pdata->cx, pdata->cy,
                  0, 0, width, height,
                  pvColor, pbmiCopy, DIB_RGB_COLORS, SRCCOPY))
            goto done;
        pdata->bpp = GetDeviceCaps(hdcScreen, BITSPIXEL);
        pdata->hbmAlpha = create_alpha_bitmap(NULL, pbmiCopy, pvColor, pdata->cx, pdata->cy);

        /* Now convert the info to monochrome for the mask bits */
        if (pbmiCopy->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            RGBQUAD *rgb = pbmiCopy->bmiColors;

            pbmiCopy->bmiHeader.biClrUsed = pbmiCopy->bmiHeader.biClrImportant = 2;
            rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
            rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
            rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
            pbmiCopy->bmiHeader.biBitCount = 1;
        }
        else
        {
            RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)pbmiCopy) + 1);

            rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
            rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
            ((BITMAPCOREHEADER*)&pbmiCopy->bmiHeader)->bcBitCount = 1;
        }
    }
    /* Set the mask bits */
    if(!SelectObject(hdc, pdata->hbmMask))
        goto done;
    bResult = StretchDIBits(hdc, 0, 0, pdata->cx, pdata->cy,
                  0, 0, width, height,
                  pvMask, pbmiCopy, DIB_RGB_COLORS, SRCCOPY) != 0;

done:
    DeleteDC(hdcScreen);
    if(hbmpOld) SelectObject(hdc, hbmpOld);
    DeleteDC(hdc);
    if(pbmiCopy) HeapFree(GetProcessHeap(), 0, pbmiCopy);
    /* Clean up in case of failure */
    if(!bResult)
    {
        if(pdata->hbmMask) DeleteObject(pdata->hbmMask);
        if(pdata->hbmColor) DeleteObject(pdata->hbmColor);
        if(pdata->hbmAlpha) DeleteObject(pdata->hbmAlpha);
    }
    return bResult;
}

static BOOL CURSORICON_GetCursorDataFromIconInfo(
  _Out_ CURSORDATA* pCursorData,
  _In_  ICONINFO* pIconInfo
)
{
    BITMAP bm;

    ZeroMemory(pCursorData, sizeof(*pCursorData));
    if(pIconInfo->hbmColor)
    {
        /* We must convert the color bitmap to screen format */
        HDC hdcScreen, hdcMem;
        HBITMAP hbmpPrev;

        /* The mask dictates its dimensions */
        if (!GetObject(pIconInfo->hbmMask, sizeof(bm), &bm))
            return FALSE;
        hdcScreen = CreateDCW(DISPLAYW, NULL, NULL, NULL);
        if(!hdcScreen)
            return FALSE;
        hdcMem = CreateCompatibleDC(hdcScreen);
        if(!hdcMem)
        {
            DeleteDC(hdcScreen);
            return FALSE;
        }
        pCursorData->hbmColor = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
        DeleteDC(hdcScreen);
        if (!pCursorData->hbmColor)
        {
            DeleteDC(hdcMem);
            return FALSE;
        }
        hbmpPrev = SelectObject(hdcMem, pCursorData->hbmColor);
        if (!hbmpPrev)
        {
            DeleteDC(hdcMem);
            DeleteObject(pCursorData->hbmColor);
            return FALSE;
        }
        stretch_blt_icon( hdcMem, bm.bmWidth, bm.bmHeight, pIconInfo->hbmColor);
        SelectObject(hdcMem, hbmpPrev);
        DeleteDC(hdcMem);
    }
    pCursorData->hbmMask = CopyImage(pIconInfo->hbmMask, IMAGE_BITMAP, 0, 0, LR_MONOCHROME);
    if(!pCursorData->hbmMask)
        return FALSE;

    /* Now, fill some information */
    pCursorData->rt = (USHORT)((ULONG_PTR)(pIconInfo->fIcon ? RT_ICON : RT_CURSOR));
    if(pCursorData->hbmColor)
    {
        GetObject(pCursorData->hbmColor, sizeof(bm), &bm);
        pCursorData->bpp = bm.bmBitsPixel;
        pCursorData->cx = bm.bmWidth;
        pCursorData->cy = bm.bmHeight;
        if(pCursorData->bpp == 32)
            pCursorData->hbmAlpha = create_alpha_bitmap(pCursorData->hbmColor, NULL, NULL, 0, 0);
    }
    else
    {
        GetObject(pCursorData->hbmMask, sizeof(bm), &bm);
        pCursorData->bpp = 1;
        pCursorData->cx = bm.bmWidth;
        pCursorData->cy = bm.bmHeight/2;
    }

    if(pIconInfo->fIcon)
    {
        pCursorData->xHotspot = pCursorData->cx/2;
        pCursorData->yHotspot = pCursorData->cy/2;
    }
    else
    {
        pCursorData->xHotspot = pIconInfo->xHotspot;
        pCursorData->yHotspot = pIconInfo->yHotspot;
    }

    return TRUE;
}


#define RIFF_FOURCC( c0, c1, c2, c3 ) \
        ( (DWORD)(BYTE)(c0) | ( (DWORD)(BYTE)(c1) << 8 ) | \
        ( (DWORD)(BYTE)(c2) << 16 ) | ( (DWORD)(BYTE)(c3) << 24 ) )

#define ANI_RIFF_ID RIFF_FOURCC('R', 'I', 'F', 'F')
#define ANI_LIST_ID RIFF_FOURCC('L', 'I', 'S', 'T')
#define ANI_ACON_ID RIFF_FOURCC('A', 'C', 'O', 'N')
#define ANI_anih_ID RIFF_FOURCC('a', 'n', 'i', 'h')
#define ANI_seq__ID RIFF_FOURCC('s', 'e', 'q', ' ')
#define ANI_fram_ID RIFF_FOURCC('f', 'r', 'a', 'm')
#define ANI_rate_ID RIFF_FOURCC('r', 'a', 't', 'e')

#define ANI_FLAG_ICON       0x1
#define ANI_FLAG_SEQUENCE   0x2

#include <pshpack1.h>
typedef struct {
    DWORD header_size;
    DWORD num_frames;
    DWORD num_steps;
    DWORD width;
    DWORD height;
    DWORD bpp;
    DWORD num_planes;
    DWORD display_rate;
    DWORD flags;
} ani_header;

typedef struct {
    DWORD           data_size;
    const unsigned char   *data;
} riff_chunk_t;
#include <poppack.h>

static void dump_ani_header( const ani_header *header )
{
    TRACE("     header size: %d\n", header->header_size);
    TRACE("          frames: %d\n", header->num_frames);
    TRACE("           steps: %d\n", header->num_steps);
    TRACE("           width: %d\n", header->width);
    TRACE("          height: %d\n", header->height);
    TRACE("             bpp: %d\n", header->bpp);
    TRACE("          planes: %d\n", header->num_planes);
    TRACE("    display rate: %d\n", header->display_rate);
    TRACE("           flags: 0x%08x\n", header->flags);
}

/* Find an animated cursor chunk, given its type and ID */
static void riff_find_chunk( DWORD chunk_id, DWORD chunk_type, const riff_chunk_t *parent_chunk, riff_chunk_t *chunk )
{
    const unsigned char *ptr = parent_chunk->data;
    const unsigned char *end = parent_chunk->data + (parent_chunk->data_size - (2 * sizeof(DWORD)));

    if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) end -= sizeof(DWORD);

    while (ptr < end)
    {
        if ((!chunk_type && *(const DWORD *)ptr == chunk_id )
                || (chunk_type && *(const DWORD *)ptr == chunk_type && *((const DWORD *)ptr + 2) == chunk_id ))
        {
            ptr += sizeof(DWORD);
            chunk->data_size = (*(const DWORD *)ptr + 1) & ~1;
            ptr += sizeof(DWORD);
            if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) ptr += sizeof(DWORD);
            chunk->data = ptr;

            return;
        }

        ptr += sizeof(DWORD);
        ptr += (*(const DWORD *)ptr + 1) & ~1;
        ptr += sizeof(DWORD);
    }
}

static BOOL CURSORICON_GetCursorDataFromANI(
    _Inout_ CURSORDATA* pCurData,
    _In_    const BYTE *pData,
    _In_    DWORD dwDataSize,
    _In_    DWORD fuLoad
)
{
    UINT i;
    const ani_header *pHeader;
    riff_chunk_t root_chunk = { dwDataSize, pData };
    riff_chunk_t ACON_chunk = {0};
    riff_chunk_t anih_chunk = {0};
    riff_chunk_t fram_chunk = {0};
    riff_chunk_t rate_chunk = {0};
    riff_chunk_t seq_chunk = {0};
    const unsigned char *icon_chunk;
    const unsigned char *icon_data;

    /* Find the root chunk */
    riff_find_chunk( ANI_ACON_ID, ANI_RIFF_ID, &root_chunk, &ACON_chunk );
    if (!ACON_chunk.data)
    {
        ERR("Failed to get root chunk.\n");
        return FALSE;
    }

    /* Find the header chunk */
    riff_find_chunk( ANI_anih_ID, 0, &ACON_chunk, &anih_chunk );
    if (!ACON_chunk.data)
    {
        ERR("Failed to get header chunk.\n");
        return FALSE;
    }
    pHeader = (ani_header*)anih_chunk.data;
    dump_ani_header(pHeader);

    /* Set up the master data */
    pCurData->CURSORF_flags |= CURSORF_ACON;
    pCurData->cpcur = pHeader->num_frames;
    pCurData->cicur = pHeader->num_steps;
    pCurData->iicur = pHeader->display_rate;

    /* Get the sequences */
    if (pHeader->flags & ANI_FLAG_SEQUENCE)
    {
        riff_find_chunk( ANI_seq__ID, 0, &ACON_chunk, &seq_chunk );
        if (!seq_chunk.data)
        {
            ERR("No sequence data although the flag is set!\n");
            return FALSE;
        }
    }

    /* Get the frame rates */
    riff_find_chunk( ANI_rate_ID, 0, &ACON_chunk, &rate_chunk );
    if (rate_chunk.data)
        pCurData->ajifRate = (INT*)rate_chunk.data;

    /* Get the frames chunk */
    riff_find_chunk( ANI_fram_ID, ANI_LIST_ID, &ACON_chunk, &fram_chunk );
    if (!fram_chunk.data)
    {
        ERR("Failed to get icon list.\n");
        return 0;
    }
    icon_chunk = fram_chunk.data;
    icon_data = fram_chunk.data + (2 * sizeof(DWORD));

    if(pHeader->num_frames > 1)
    {
        /* Allocate frame descriptors, step indices and rates */
        pCurData->aspcur = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            pHeader->num_frames * sizeof(CURSORDATA) + pHeader->num_steps * (sizeof(DWORD) + sizeof(INT)));
        if(!pCurData->aspcur)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        pCurData->aicur = (DWORD*)(pCurData->aspcur + pHeader->num_frames);
        pCurData->ajifRate = (INT*)(pCurData->aicur + pHeader->num_steps);
    }

    for(i=0; i < pHeader->num_frames; i++)
    {
        CURSORDATA* pFrameData;
        const DWORD chunk_size = *(const DWORD *)(icon_chunk + sizeof(DWORD));
        const BITMAPINFO* pbmi;

        if(pHeader->num_frames > 1)
            pFrameData = &pCurData->aspcur[i];
        else
            pFrameData = pCurData;

        pFrameData->rt = pCurData->rt;

        if (pHeader->flags & ANI_FLAG_ICON)
        {
            /* The chunks describe an icon file */
            const CURSORICONFILEDIRENTRY* pDirEntry = get_best_icon_file_entry(
                (const CURSORICONFILEDIR *) icon_data,
                chunk_size,
                pCurData->cx,
                pCurData->cy,
                TRUE,
                fuLoad);
            if(!pDirEntry)
            {
                ERR("Unable to find the right file entry for frame %d.\n", i);
                goto error;
            }
            pFrameData->xHotspot = pDirEntry->xHotspot;
            pFrameData->yHotspot = pDirEntry->yHotspot;
            if(!pHeader->width || !pHeader->height)
            {
                pFrameData->cx = pDirEntry->bWidth;
                pFrameData->cy = pDirEntry->bHeight;
            }
            else
            {
                pFrameData->cx = pHeader->width;
                pFrameData->cy = pHeader->height;
            }
            pbmi = (const BITMAPINFO *) (icon_data + pDirEntry->dwDIBOffset);
        }
        else
        {
            /* The chunks just describe bitmaps */
            pbmi = (const BITMAPINFO *)icon_data;
            pFrameData->xHotspot = pFrameData->yHotspot = 0;
        }

        /* Do the real work */
        CURSORICON_GetCursorDataFromBMI(pFrameData, pbmi);

        if(pHeader->num_frames > 1)
            pFrameData->CURSORF_flags |= CURSORF_ACONFRAME;
        else
            pFrameData->CURSORF_flags &= ~CURSORF_ACON;


        /* Next frame */
        icon_chunk += chunk_size + (2 * sizeof(DWORD));
        icon_data = icon_chunk + (2 * sizeof(DWORD));
    }

    if(pHeader->num_frames <= 1)
        return TRUE;

    if(rate_chunk.data)
        CopyMemory(pCurData->ajifRate, rate_chunk.data, pHeader->num_steps * sizeof(INT));
    else
    {
        for(i=0; i < pHeader->num_steps; i++)
            pCurData->ajifRate[i] = pHeader->display_rate;
    }

    if (pHeader->flags & ANI_FLAG_SEQUENCE)
    {
        CopyMemory(pCurData->aicur, seq_chunk.data, pHeader->num_steps * sizeof(DWORD));
    }
    else
    {
        for(i=0; i < pHeader->num_steps; i++)
            pCurData->aicur[i] = i;
    }

    return TRUE;

error:
    HeapFree(GetProcessHeap(), 0, pCurData->aspcur);
    ZeroMemory(pCurData, sizeof(CURSORDATA));
    return FALSE;
}


static
HBITMAP
BITMAP_LoadImageW(
  _In_opt_  HINSTANCE hinst,
  _In_      LPCWSTR lpszName,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad
)
{
    const BITMAPINFO* pbmi;
    BITMAPINFO* pbmiScaled = NULL;
    BITMAPINFO* pbmiCopy = NULL;
    const VOID* pvMapping = NULL;
    DWORD dwOffset = 0;
    HGLOBAL hgRsrc = NULL;
    int iBMISize;
    PVOID pvBits;
    HDC hdcScreen = NULL;
    HDC hdc = NULL;
    HBITMAP hbmpOld, hbmpRet = NULL;
    LONG width, height;
    WORD bpp;
    DWORD compr, ResSize = 0;

    /* Map the bitmap info */
    if(fuLoad & LR_LOADFROMFILE)
    {
        const BITMAPFILEHEADER* pbmfh;

        pvMapping = map_fileW(lpszName, NULL);
        if(!pvMapping)
            return NULL;
        pbmfh = pvMapping;
        if (pbmfh->bfType != 0x4d42 /* 'BM' */)
        {
            WARN("Invalid/unsupported bitmap format!\n");
            goto end;
        }
        pbmi = (const BITMAPINFO*)(pbmfh + 1);

        /* Get the image bits */
        if(pbmfh->bfOffBits)
            dwOffset = pbmfh->bfOffBits - sizeof(BITMAPFILEHEADER);
    }
    else
    {
        HRSRC hrsrc;

        /* Caller wants an OEM bitmap */
        if(!hinst)
            hinst = User32Instance;
        hrsrc = FindResourceW(hinst, lpszName, RT_BITMAP);
        if(!hrsrc)
            return NULL;
        hgRsrc = LoadResource(hinst, hrsrc);
        if(!hgRsrc)
            return NULL;
        pbmi = LockResource(hgRsrc);
        if(!pbmi)
            return NULL;
        ResSize = SizeofResource(hinst, hrsrc);
    }

    /* Fix up values */
    if(DIB_GetBitmapInfo(&pbmi->bmiHeader, &width, &height, &bpp, &compr) == -1)
        goto end;
    if((width > 65535) || (height > 65535))
        goto end;
    if(cxDesired == 0)
        cxDesired = width;
    if(cyDesired == 0)
        cyDesired = height;
    else if(height < 0)
        cyDesired = -cyDesired;

    iBMISize = bitmap_info_size(pbmi, DIB_RGB_COLORS);

    /* Get a pointer to the image data */
    pvBits = (char*)pbmi + (dwOffset ? dwOffset : iBMISize);

    /* Create a copy of the info describing the bitmap in the file */
    pbmiCopy = HeapAlloc(GetProcessHeap(), 0, iBMISize);
    if(!pbmiCopy)
        goto end;
    CopyMemory(pbmiCopy, pbmi, iBMISize);

    TRACE("Size Image %d, Size Header %d, ResSize %d\n",
        pbmiCopy->bmiHeader.biSizeImage, pbmiCopy->bmiHeader.biSize, ResSize);

    /* HACK: If this is a binutils' windres.exe compiled 16 or 32 bpp bitmap
     * using BI_BITFIELDS, then a bug causes it to fail to include
     * the bytes for the bitfields. So, we have to substract out the
     * size of the bitfields previously included from bitmap_info_size. */
    if (compr == BI_BITFIELDS && (bpp == 16 || bpp == 32) &&
        pbmiCopy->bmiHeader.biSizeImage + pbmiCopy->bmiHeader.biSize == ResSize)
    {
        /* GCC pointer to the image data has 12 less bytes than MSVC */
        pvBits = (char*)pvBits - 12;
        WARN("Found GCC Resource Compiled 16-bpp or 32-bpp error\n");
    }

    /* Fix it up, if needed */
    if(fuLoad & (LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS))
    {
        WORD bpp, incr, numColors;
        char* pbmiColors;
        RGBTRIPLE* ptr;
        COLORREF crWindow, cr3DShadow, cr3DFace, cr3DLight;
        BYTE pixel = *((BYTE*)pvBits);
        UINT i;

        if(pbmiCopy->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            bpp = ((BITMAPCOREHEADER*)&pbmiCopy->bmiHeader)->bcBitCount;
            numColors = 1 << bpp;
            /* BITMAPCOREINFO holds RGBTRIPLEs */
            incr = 3;
        }
        else
        {
            bpp = pbmiCopy->bmiHeader.biBitCount;
            /* BITMAPINFOHEADER holds RGBQUADs */
            incr = 4;
            numColors = pbmiCopy->bmiHeader.biClrUsed;
            if(numColors > 256) numColors = 256;
            if (!numColors && (bpp <= 8)) numColors = 1 << bpp;
        }

        if(bpp > 8)
            goto create_bitmap;

        pbmiColors = (char*)pbmiCopy + pbmiCopy->bmiHeader.biSize;

        /* Get the relevant colors */
        crWindow = GetSysColor(COLOR_WINDOW);
        cr3DShadow = GetSysColor(COLOR_3DSHADOW);
        cr3DFace = GetSysColor(COLOR_3DFACE);
        cr3DLight = GetSysColor(COLOR_3DLIGHT);

        /* Fix the transparent palette entry */
        if(fuLoad & LR_LOADTRANSPARENT)
        {
            switch(bpp)
            {
                case 1: pixel >>= 7; break;
                case 4: pixel >>= 4; break;
                case 8: break;
                default:
                    FIXME("Unhandled bit depth %d.\n", bpp);
                    goto create_bitmap;
            }

            if(pixel >= numColors)
            {
                ERR("Wrong pixel passed in.\n");
                goto create_bitmap;
            }

            /* If both flags are set, we must use COLOR_3DFACE */
            if(fuLoad & LR_LOADMAP3DCOLORS) crWindow = cr3DFace;

            /* Define the color */
            ptr = (RGBTRIPLE*)(pbmiColors + pixel*incr);
            ptr->rgbtBlue = GetBValue(crWindow);
            ptr->rgbtGreen = GetGValue(crWindow);
            ptr->rgbtRed = GetRValue(crWindow);
            goto create_bitmap;
        }

        /* If we are here, then LR_LOADMAP3DCOLORS is set without LR_TRANSPARENT */
        for(i = 0; i<numColors; i++)
        {
            ptr = (RGBTRIPLE*)(pbmiColors + i*incr);
            if((ptr->rgbtBlue == ptr->rgbtRed) && (ptr->rgbtBlue == ptr->rgbtGreen))
            {
                if(ptr->rgbtBlue == 128)
                {
                    ptr->rgbtBlue = GetBValue(cr3DShadow);
                    ptr->rgbtGreen = GetGValue(cr3DShadow);
                    ptr->rgbtRed = GetRValue(cr3DShadow);
                }
                if(ptr->rgbtBlue == 192)
                {
                    ptr->rgbtBlue = GetBValue(cr3DFace);
                    ptr->rgbtGreen = GetGValue(cr3DFace);
                    ptr->rgbtRed = GetRValue(cr3DFace);
                }
                if(ptr->rgbtBlue == 223)
                {
                    ptr->rgbtBlue = GetBValue(cr3DLight);
                    ptr->rgbtGreen = GetGValue(cr3DLight);
                    ptr->rgbtRed = GetRValue(cr3DLight);
                }
            }
        }
    }

create_bitmap:
    if(fuLoad & LR_CREATEDIBSECTION)
    {
        /* Allocate the BMI describing the new bitmap */
        pbmiScaled = HeapAlloc(GetProcessHeap(), 0, iBMISize);
        if(!pbmiScaled)
            goto end;
        CopyMemory(pbmiScaled, pbmiCopy, iBMISize);

        /* Fix it up */
        if(pbmiScaled->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            BITMAPCOREHEADER* pbmch = (BITMAPCOREHEADER*)&pbmiScaled->bmiHeader;
            pbmch->bcWidth = cxDesired;
            pbmch->bcHeight = cyDesired;
        }
        else
        {
            pbmiScaled->bmiHeader.biWidth = cxDesired;
            pbmiScaled->bmiHeader.biHeight = cyDesired;
            /* No compression for DIB sections */
            pbmiScaled->bmiHeader.biCompression = BI_RGB;
        }
    }

    /* Top-down image */
    if(cyDesired < 0) cyDesired = -cyDesired;

    /* We need a device context */
    hdcScreen = CreateDCW(DISPLAYW, NULL, NULL, NULL);
    if(!hdcScreen)
        goto end;
    hdc = CreateCompatibleDC(hdcScreen);
    if(!hdc)
        goto end;

    /* Now create the bitmap */
    if(fuLoad & LR_CREATEDIBSECTION)
        hbmpRet = CreateDIBSection(hdc, pbmiScaled, DIB_RGB_COLORS, NULL, 0, 0);
    else
    {
        if(is_dib_monochrome(pbmiCopy) || (fuLoad & LR_MONOCHROME))
            hbmpRet = CreateBitmap(cxDesired, cyDesired, 1, 1, NULL);
        else
            hbmpRet = CreateCompatibleBitmap(hdcScreen, cxDesired, cyDesired);
    }

    if(!hbmpRet)
        goto end;

    hbmpOld = SelectObject(hdc, hbmpRet);
    if(!hbmpOld)
        goto end;
    if(!StretchDIBits(hdc, 0, 0, cxDesired, cyDesired,
                           0, 0, width, height,
                           pvBits, pbmiCopy, DIB_RGB_COLORS, SRCCOPY))
    {
        ERR("StretchDIBits failed!.\n");
        SelectObject(hdc, hbmpOld);
        DeleteObject(hbmpRet);
        hbmpRet = NULL;
        goto end;
    }

    SelectObject(hdc, hbmpOld);

end:
    if(hdcScreen)
        DeleteDC(hdcScreen);
    if(hdc)
        DeleteDC(hdc);
    if(pbmiScaled)
        HeapFree(GetProcessHeap(), 0, pbmiScaled);
    if(pbmiCopy)
        HeapFree(GetProcessHeap(), 0, pbmiCopy);
    if (pvMapping)
        UnmapViewOfFile( pvMapping );
    if(hgRsrc)
        FreeResource(hgRsrc);

    return hbmpRet;
}


static
HANDLE
CURSORICON_LoadFromFileW(
  _In_      LPCWSTR lpszName,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad,
  _In_      BOOL bIcon
)
{
    const CURSORICONFILEDIRENTRY *entry;
    const CURSORICONFILEDIR *dir;
    DWORD filesize = 0;
    LPBYTE bits;
    HANDLE hCurIcon = NULL;
    CURSORDATA cursorData;

    TRACE("loading %s\n", debugstr_w( lpszName ));

    bits = map_fileW( lpszName, &filesize );
    if (!bits)
        return NULL;

    /* Check for .ani. */
    if (memcmp( bits, "RIFF", 4 ) == 0)
    {
        UNIMPLEMENTED;
        goto end;
    }

    dir = (CURSORICONFILEDIR*) bits;
    entry = get_best_icon_file_entry(dir, filesize, cxDesired, cyDesired, bIcon, fuLoad);
    if(!entry)
        goto end;

    /* Fix dimensions */
    if(!cxDesired) cxDesired = entry->bWidth;
    if(!cyDesired) cyDesired = entry->bHeight;
    /* A bit of preparation */
    ZeroMemory(&cursorData, sizeof(cursorData));
    if(!bIcon)
    {
        cursorData.xHotspot = entry->xHotspot;
        cursorData.yHotspot = entry->yHotspot;
    }
    cursorData.rt = (USHORT)((ULONG_PTR)(bIcon ? RT_ICON : RT_CURSOR));

    /* Do the dance */
    if(!CURSORICON_GetCursorDataFromBMI(&cursorData, (BITMAPINFO*)(&bits[entry->dwDIBOffset])))
        {
            ERR("Failing File is \n    '%S'.\n", lpszName);
            goto end;
        }

    hCurIcon = NtUserxCreateEmptyCurObject(FALSE);
    if(!hCurIcon)
        goto end;

    /* Tell win32k */
    if(!NtUserSetCursorIconData(hCurIcon, NULL, NULL, &cursorData))
    {
        NtUserDestroyCursor(hCurIcon, TRUE);
        goto end_error;
    }

end:
    UnmapViewOfFile(bits);
    return hCurIcon;

    /* Clean up */
end_error:
    DeleteObject(cursorData.hbmMask);
    if(cursorData.hbmColor) DeleteObject(cursorData.hbmColor);
    if(cursorData.hbmAlpha) DeleteObject(cursorData.hbmAlpha);
    UnmapViewOfFile(bits);

    return NULL;
}

static
HANDLE
CURSORICON_LoadImageW(
  _In_opt_  HINSTANCE hinst,
  _In_      LPCWSTR lpszName,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad,
  _In_      BOOL bIcon
)
{
    HRSRC hrsrc;
    HANDLE handle, hCurIcon = NULL;
    CURSORICONDIR* dir;
    WORD wResId;
    LPBYTE bits;
    CURSORDATA cursorData;
    BOOL bStatus;
    UNICODE_STRING ustrRsrc;
    UNICODE_STRING ustrModule = {0, 0, NULL};

    /* Fix width/height */
    if(fuLoad & LR_DEFAULTSIZE)
    {
        if(!cxDesired) cxDesired = GetSystemMetrics(bIcon ? SM_CXICON : SM_CXCURSOR);
        if(!cyDesired) cyDesired = GetSystemMetrics(bIcon ? SM_CYICON : SM_CYCURSOR);
    }

    if(fuLoad & LR_LOADFROMFILE)
    {
        return CURSORICON_LoadFromFileW(lpszName, cxDesired, cyDesired, fuLoad, bIcon);
    }

    /* Check if caller wants OEM icons */
    if(!hinst)
        hinst = User32Instance;

    if(lpszName)
    {
        /* Prepare the resource name string */
        if(IS_INTRESOURCE(lpszName))
        {
            ustrRsrc.Buffer = (LPWSTR)lpszName;
            ustrRsrc.Length = 0;
            ustrRsrc.MaximumLength = 0;
        }
        else
            RtlInitUnicodeString(&ustrRsrc, lpszName);
    }

    if(LDR_IS_RESOURCE(hinst))
    {
        /* We don't have a real module for GetModuleFileName, construct a fake name instead.
         * GetIconInfoEx reveals the name used by Windows. */
        LPCWSTR fakeNameFmt = sizeof(void*) > 4 ? L"\x01%016IX" : L"\x01%08IX";
        ustrModule.MaximumLength = 18 * sizeof(WCHAR);
        ustrModule.Buffer = HeapAlloc(GetProcessHeap(), 0, ustrModule.MaximumLength);
        if (!ustrModule.Buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        ustrModule.Length = wsprintfW(ustrModule.Buffer, fakeNameFmt, hinst) * sizeof(WCHAR);
    }
    else if(hinst)
    {
        DWORD size = MAX_PATH;
        /* Get the module name string */
        while (TRUE)
        {
            DWORD ret;
            ustrModule.Buffer = HeapAlloc(GetProcessHeap(), 0, size*sizeof(WCHAR));
            if (!ustrModule.Buffer)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return NULL;
            }
            ret = GetModuleFileNameW(hinst, ustrModule.Buffer, size);
            if(ret == 0)
            {
                HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
                return NULL;
            }

            /* This API is completely broken... */
            if (ret == size)
            {
                HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
                size *= 2;
                continue;
            }

            ustrModule.Buffer[ret] = UNICODE_NULL;
            ustrModule.Length = ret * sizeof(WCHAR);
            ustrModule.MaximumLength = size * sizeof(WCHAR);
            break;
        }
    }

    if(fuLoad & LR_SHARED)
    {
        FINDEXISTINGCURICONPARAM param;

        TRACE("Checking for an LR_SHARED cursor/icon.\n");
        /* Ask win32k */
        param.bIcon = bIcon;
        param.cx = cxDesired;
        param.cy = cyDesired;
        hCurIcon = NtUserFindExistingCursorIcon(&ustrModule, &ustrRsrc, &param);
        if(hCurIcon)
        {
            /* Woohoo, got it! */
            TRACE("MATCH! %p\n",hCurIcon);
            HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
            return hCurIcon;
        }
    }

    /* Find resource ID */
    hrsrc = FindResourceW(
        hinst,
        lpszName,
        bIcon ? RT_GROUP_ICON : RT_GROUP_CURSOR);

    /* We let FindResource, LoadResource, etc. call SetLastError */
    if(!hrsrc)
        goto done;

    handle = LoadResource(hinst, hrsrc);
    if(!handle)
        goto done;

    dir = LockResource(handle);
    if(!dir)
        goto done;

    wResId = LookupIconIdFromDirectoryEx((PBYTE)dir, bIcon, cxDesired, cyDesired, fuLoad);
    FreeResource(handle);

    /* Get the relevant resource pointer */
    hrsrc = FindResourceW(
        hinst,
        MAKEINTRESOURCEW(wResId),
        bIcon ? RT_ICON : RT_CURSOR);
    if(!hrsrc)
        goto done;

    handle = LoadResource(hinst, hrsrc);
    if(!handle)
        goto done;

    bits = LockResource(handle);
    if(!bits)
    {
        FreeResource(handle);
        goto done;
    }

    ZeroMemory(&cursorData, sizeof(cursorData));

    /* This is from resource */
    cursorData.CURSORF_flags = CURSORF_FROMRESOURCE;

    if(dir->idType == 2)
    {
        /* idType == 2 for cursor resources */
        SHORT* ptr = (SHORT*)bits;
        cursorData.xHotspot = ptr[0];
        cursorData.yHotspot = ptr[1];
        bits += 2*sizeof(SHORT);
    }
    cursorData.cx = cxDesired;
    cursorData.cy = cyDesired;
    cursorData.rt = (USHORT)((ULONG_PTR)(bIcon ? RT_ICON : RT_CURSOR));

    /* Get the bitmaps */
    bStatus = CURSORICON_GetCursorDataFromBMI(
        &cursorData,
        (BITMAPINFO*)bits);

    FreeResource( handle );

    if(!bStatus)
        goto done;

    /* Create the handle */
    hCurIcon = NtUserxCreateEmptyCurObject(FALSE);
    if(!hCurIcon)
    {
        goto end_error;
    }

    if(fuLoad & LR_SHARED)
    {
        cursorData.CURSORF_flags |= CURSORF_LRSHARED;
    }

    /* Tell win32k */
    bStatus = NtUserSetCursorIconData(hCurIcon, hinst ? &ustrModule : NULL, lpszName ? &ustrRsrc : NULL, &cursorData);

    if(!bStatus)
    {
        NtUserDestroyCursor(hCurIcon, TRUE);
        goto end_error;
    }

done:
    if(ustrModule.Buffer)
        HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
    return hCurIcon;

end_error:
    if(ustrModule.Buffer)
        HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
    DeleteObject(cursorData.hbmMask);
    if(cursorData.hbmColor) DeleteObject(cursorData.hbmColor);
    if(cursorData.hbmAlpha) DeleteObject(cursorData.hbmAlpha);

    return NULL;
}

static
HBITMAP
BITMAP_CopyImage(
  _In_  HBITMAP hnd,
  _In_  int desiredx,
  _In_  int desiredy,
  _In_  UINT flags
)
{
    HBITMAP res = NULL;
    DIBSECTION ds;
    int objSize;
    BITMAPINFO * bi;

    objSize = GetObjectW( hnd, sizeof(ds), &ds );
    if (!objSize) return 0;
    if ((desiredx < 0) || (desiredy < 0)) return 0;

    if (flags & LR_COPYFROMRESOURCE)
    {
        FIXME("The flag LR_COPYFROMRESOURCE is not implemented for bitmaps\n");
    }

    if (flags & LR_COPYRETURNORG)
    {
        FIXME("The flag LR_COPYRETURNORG is not implemented for bitmaps\n");
    }

    if (desiredx == 0) desiredx = ds.dsBm.bmWidth;
    if (desiredy == 0) desiredy = ds.dsBm.bmHeight;

    /* Allocate memory for a BITMAPINFOHEADER structure and a
       color table. The maximum number of colors in a color table
       is 256 which corresponds to a bitmap with depth 8.
       Bitmaps with higher depths don't have color tables. */
    bi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!bi) return 0;

    bi->bmiHeader.biSize        = sizeof(bi->bmiHeader);
    bi->bmiHeader.biPlanes      = ds.dsBm.bmPlanes;
    bi->bmiHeader.biBitCount    = ds.dsBm.bmBitsPixel;
    bi->bmiHeader.biCompression = BI_RGB;

    if (flags & LR_CREATEDIBSECTION)
    {
        /* Create a DIB section. LR_MONOCHROME is ignored */
        void * bits;
        HDC dc = CreateCompatibleDC(NULL);

        if (objSize == sizeof(DIBSECTION))
        {
            /* The source bitmap is a DIB.
               Get its attributes to create an exact copy */
            memcpy(bi, &ds.dsBmih, sizeof(BITMAPINFOHEADER));
        }

        bi->bmiHeader.biWidth  = desiredx;
        bi->bmiHeader.biHeight = desiredy;

        /* Get the color table or the color masks */
        GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);

        res = CreateDIBSection(dc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
        DeleteDC(dc);
    }
    else
    {
        /* Create a device-dependent bitmap */

        BOOL monochrome = (flags & LR_MONOCHROME);

        if (objSize == sizeof(DIBSECTION))
        {
            /* The source bitmap is a DIB section.
               Get its attributes */
            HDC dc = CreateCompatibleDC(NULL);
            bi->bmiHeader.biWidth  = ds.dsBm.bmWidth;
            bi->bmiHeader.biHeight = ds.dsBm.bmHeight;
            GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
            DeleteDC(dc);

            if (!monochrome && ds.dsBm.bmBitsPixel == 1)
            {
                /* Look if the colors of the DIB are black and white */

                monochrome =
                      (bi->bmiColors[0].rgbRed == 0xff
                    && bi->bmiColors[0].rgbGreen == 0xff
                    && bi->bmiColors[0].rgbBlue == 0xff
                    && bi->bmiColors[0].rgbReserved == 0
                    && bi->bmiColors[1].rgbRed == 0
                    && bi->bmiColors[1].rgbGreen == 0
                    && bi->bmiColors[1].rgbBlue == 0
                    && bi->bmiColors[1].rgbReserved == 0)
                    ||
                      (bi->bmiColors[0].rgbRed == 0
                    && bi->bmiColors[0].rgbGreen == 0
                    && bi->bmiColors[0].rgbBlue == 0
                    && bi->bmiColors[0].rgbReserved == 0
                    && bi->bmiColors[1].rgbRed == 0xff
                    && bi->bmiColors[1].rgbGreen == 0xff
                    && bi->bmiColors[1].rgbBlue == 0xff
                    && bi->bmiColors[1].rgbReserved == 0);
            }
        }
        else if (!monochrome)
        {
            monochrome = ds.dsBm.bmBitsPixel == 1;
        }

        if (monochrome)
        {
            res = CreateBitmap(desiredx, desiredy, 1, 1, NULL);
        }
        else
        {
            HDC screenDC = GetDC(NULL);
            res = CreateCompatibleBitmap(screenDC, desiredx, desiredy);
            ReleaseDC(NULL, screenDC);
        }
    }

    if (res)
    {
        /* Only copy the bitmap if it's a DIB section or if it's
           compatible to the screen */
        BOOL copyContents;

        if (objSize == sizeof(DIBSECTION))
        {
            copyContents = TRUE;
        }
        else
        {
            HDC screenDC = GetDC(NULL);
            int screen_depth = GetDeviceCaps(screenDC, BITSPIXEL);
            ReleaseDC(NULL, screenDC);

            copyContents = (ds.dsBm.bmBitsPixel == 1 || ds.dsBm.bmBitsPixel == screen_depth);
        }

        if (copyContents)
        {
            /* The source bitmap may already be selected in a device context,
               use GetDIBits/StretchDIBits and not StretchBlt  */

            HDC dc;
            void * bits;

            dc = CreateCompatibleDC(NULL);

            bi->bmiHeader.biWidth = ds.dsBm.bmWidth;
            bi->bmiHeader.biHeight = ds.dsBm.bmHeight;
            bi->bmiHeader.biSizeImage = 0;
            bi->bmiHeader.biClrUsed = 0;
            bi->bmiHeader.biClrImportant = 0;

            /* Fill in biSizeImage */
            GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
            bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bi->bmiHeader.biSizeImage);

            if (bits)
            {
                HBITMAP oldBmp;

                /* Get the image bits of the source bitmap */
                GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, bits, bi, DIB_RGB_COLORS);

                /* Copy it to the destination bitmap */
                oldBmp = SelectObject(dc, res);
                StretchDIBits(dc, 0, 0, desiredx, desiredy,
                              0, 0, ds.dsBm.bmWidth, ds.dsBm.bmHeight,
                              bits, bi, DIB_RGB_COLORS, SRCCOPY);
                SelectObject(dc, oldBmp);

                HeapFree(GetProcessHeap(), 0, bits);
            }

            DeleteDC(dc);
        }

        if (flags & LR_COPYDELETEORG)
        {
            DeleteObject(hnd);
        }
    }
    HeapFree(GetProcessHeap(), 0, bi);
    return res;
}

static
HICON
CURSORICON_CopyImage(
  _In_  HICON hicon,
  _In_  BOOL  bIcon,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT fuFlags
)
{
    HICON ret = NULL;
    ICONINFO ii;
    CURSORDATA CursorData;

    if (fuFlags & LR_COPYFROMRESOURCE)
    {
        /* Get the icon module/resource names */
        UNICODE_STRING ustrModule;
        UNICODE_STRING ustrRsrc;
        HMODULE hModule;

        ustrModule.MaximumLength = 0;
        ustrRsrc.MaximumLength = 0;

        /* Get the buffer size */
        if (!NtUserGetIconInfo(hicon, NULL, &ustrModule, &ustrRsrc, NULL, FALSE))
        {
            return NULL;
        }

        ustrModule.Buffer = HeapAlloc(GetProcessHeap(), 0, ustrModule.MaximumLength);
        if (!ustrModule.Buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }

        if (ustrRsrc.MaximumLength)
        {
            ustrRsrc.Buffer = HeapAlloc(GetProcessHeap(), 0, ustrRsrc.MaximumLength);
            if (!ustrRsrc.Buffer)
            {
                HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return NULL;
            }
        }

        if (!NtUserGetIconInfo(hicon, NULL, &ustrModule, &ustrRsrc, NULL, FALSE))
        {
            HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
            if (!IS_INTRESOURCE(ustrRsrc.Buffer))
                HeapFree(GetProcessHeap(), 0, ustrRsrc.Buffer);
            return NULL;
        }

        /* NULL-terminate our strings */
        ustrModule.Buffer[ustrModule.Length/sizeof(WCHAR)] = UNICODE_NULL;
        if (!IS_INTRESOURCE(ustrRsrc.Buffer))
            ustrRsrc.Buffer[ustrRsrc.Length/sizeof(WCHAR)] = UNICODE_NULL;

        TRACE("Got module %wZ, resource %p (%S).\n", &ustrModule,
            ustrRsrc.Buffer, IS_INTRESOURCE(ustrRsrc.Buffer) ? L"" : ustrRsrc.Buffer);

        /* Get the module handle or load the module */
        hModule = LoadLibraryExW(ustrModule.Buffer, NULL, /* NT6+: LOAD_LIBRARY_AS_IMAGE_RESOURCE | */ LOAD_LIBRARY_AS_DATAFILE);
        if (!hModule)
        {
            DWORD err = GetLastError();
            ERR("Unable to load/use module '%wZ' in process %lu, error %lu.\n", &ustrModule, GetCurrentProcessId(), err);
            SetLastError(ERROR_INVALID_PARAMETER);
            goto leave;
        }

        /* Call the relevant function */
        ret = CURSORICON_LoadImageW(
            hModule,
            ustrRsrc.Buffer,
            cxDesired,
            cyDesired,
            fuFlags & (LR_DEFAULTSIZE | LR_SHARED),
            bIcon);

        FreeLibrary(hModule);

        /* If we're here, that means that the passed icon is shared. Don't destroy it, even if LR_COPYDELETEORG is specified */
    leave:
        HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
        if (!IS_INTRESOURCE(ustrRsrc.Buffer))
            HeapFree(GetProcessHeap(), 0, ustrRsrc.Buffer);

        TRACE("Returning 0x%08x.\n", ret);

        return ret;
    }

    /* This is a regular copy */
    if (fuFlags & ~(LR_COPYDELETEORG | LR_SHARED))
        FIXME("Unimplemented flags: 0x%08x\n", fuFlags);

    if (!GetIconInfo(hicon, &ii))
    {
        ERR("GetIconInfo failed.\n");
        return NULL;
    }

    /* This is CreateIconIndirect with the LR_SHARED coat added */
    if  (!CURSORICON_GetCursorDataFromIconInfo(&CursorData, &ii))
        goto Leave;

    if (fuFlags & LR_SHARED)
        CursorData.CURSORF_flags |= CURSORF_LRSHARED;

    ret = NtUserxCreateEmptyCurObject(FALSE);
    if (!ret)
        goto Leave;

    if (!NtUserSetCursorIconData(ret, NULL, NULL, &CursorData))
    {
        NtUserDestroyCursor(ret, TRUE);
        goto Leave;
    }

Leave:
    DeleteObject(ii.hbmMask);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);

    if (ret && (fuFlags & LR_COPYDELETEORG))
        DestroyIcon(hicon);

    return ret;
}

NTSTATUS WINAPI
User32CallCopyImageFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PCOPYIMAGE_CALLBACK_ARGUMENTS Common;
  HANDLE Result;
  Common = (PCOPYIMAGE_CALLBACK_ARGUMENTS) Arguments;

  Result = CopyImage(Common->hImage,
                     Common->uType,
                     Common->cxDesired,
                     Common->cyDesired,
                     Common->fuFlags);

  return ZwCallbackReturn(&Result, sizeof(HANDLE), STATUS_SUCCESS);
}


/************* PUBLIC FUNCTIONS *******************/

#define COPYIMAGE_VALID_FLAGS ( \
    LR_SHARED | LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS | 0x800 | \
    LR_VGACOLOR | LR_LOADREALSIZE | LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADFROMFILE | \
    LR_COPYDELETEORG | LR_COPYRETURNORG | LR_COLOR | LR_MONOCHROME \
)

HANDLE WINAPI CopyImage(
  _In_  HANDLE hImage,
  _In_  UINT uType,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT fuFlags
)
{
    TRACE("hImage=%p, uType=%u, cxDesired=%d, cyDesired=%d, fuFlags=%x\n",
        hImage, uType, cxDesired, cyDesired, fuFlags);

    if (fuFlags & ~COPYIMAGE_VALID_FLAGS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    switch(uType)
    {
        case IMAGE_BITMAP:
            if (!hImage)
            {
                SetLastError(ERROR_INVALID_HANDLE);
                break;
            }
            return BITMAP_CopyImage(hImage, cxDesired, cyDesired, fuFlags);
        case IMAGE_CURSOR:
        case IMAGE_ICON:
        {
            HANDLE handle;
            if (!hImage)
            {
                SetLastError(ERROR_INVALID_CURSOR_HANDLE);
                break;
            }
            handle = CURSORICON_CopyImage(hImage, uType == IMAGE_ICON, cxDesired, cyDesired, fuFlags);
            if (!handle && (fuFlags & LR_COPYFROMRESOURCE))
            {
                /* Test if the hImage is the same size as what we want by getting
                 * its BITMAP and comparing its dimensions to the desired size. */
                BITMAP bm;

                ICONINFO iconinfo = { 0 };
                if (!GetIconInfo(hImage, &iconinfo))
                {
                    ERR("GetIconInfo Failed. hImage %p\n", hImage);
                    return NULL;
                }
                if (!GetObject(iconinfo.hbmColor, sizeof(bm), &bm))
                {
                    ERR("GetObject Failed. iconinfo %p\n", iconinfo);
                    return NULL;
                }

                DeleteObject(iconinfo.hbmMask);
                DeleteObject(iconinfo.hbmColor);

                /* If the images are the same size remove LF_COPYFROMRESOURCE and try again */
                if (cxDesired == bm.bmWidth && cyDesired == bm.bmHeight)
                {
                    handle = CURSORICON_CopyImage(hImage, uType == IMAGE_ICON, cxDesired,
                                                  cyDesired, (fuFlags & ~LR_COPYFROMRESOURCE));
                }
            }
            return handle;
        }
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
    }
    return NULL;
}

HICON WINAPI CopyIcon(
  _In_  HICON hIcon
)
{
    return CURSORICON_CopyImage(hIcon, FALSE, 0, 0, 0);
}

BOOL WINAPI DrawIcon(
  _In_  HDC hDC,
  _In_  int X,
  _In_  int Y,
  _In_  HICON hIcon
)
{
    return DrawIconEx(hDC, X, Y, hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
}

BOOL WINAPI DrawIconEx(
  _In_      HDC hdc,
  _In_      int xLeft,
  _In_      int yTop,
  _In_      HICON hIcon,
  _In_      int cxWidth,
  _In_      int cyWidth,
  _In_      UINT istepIfAniCur,
  _In_opt_  HBRUSH hbrFlickerFreeDraw,
  _In_      UINT diFlags
)
{
    return NtUserDrawIconEx(hdc, xLeft, yTop, hIcon, cxWidth, cyWidth,
                            istepIfAniCur, hbrFlickerFreeDraw, diFlags,
                            0, 0);
}

BOOL WINAPI GetIconInfo(
  _In_   HICON hIcon,
  _Out_  PICONINFO piconinfo
)
{
    return NtUserGetIconInfo(hIcon, piconinfo, NULL, NULL, NULL, FALSE);
}

BOOL WINAPI DestroyIcon(
  _In_  HICON hIcon
)
{
    return NtUserDestroyCursor(hIcon, FALSE);
}

HICON WINAPI LoadIconA(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCSTR lpIconName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(lpIconName));

    return LoadImageA(hInstance,
        lpIconName,
        IMAGE_ICON,
        0,
        0,
        LR_SHARED | LR_DEFAULTSIZE );
}

HICON WINAPI LoadIconW(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCWSTR lpIconName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(lpIconName));

    return LoadImageW(hInstance,
        lpIconName,
        IMAGE_ICON,
        0,
        0,
        LR_SHARED | LR_DEFAULTSIZE );
}

HCURSOR WINAPI LoadCursorA(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCSTR    lpCursorName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(lpCursorName));

    return LoadImageA(hInstance,
        lpCursorName,
        IMAGE_CURSOR,
        0,
        0,
        LR_SHARED | LR_DEFAULTSIZE );
}

HCURSOR WINAPI LoadCursorW(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCWSTR   lpCursorName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(lpCursorName));

    return LoadImageW(hInstance,
        lpCursorName,
        IMAGE_CURSOR,
        0,
        0,
        LR_SHARED | LR_DEFAULTSIZE );
}

HCURSOR WINAPI LoadCursorFromFileA(
  _In_  LPCSTR lpFileName
)
{
    TRACE("%s\n", debugstr_a(lpFileName));

    return LoadImageA(NULL,
        lpFileName,
        IMAGE_CURSOR,
        0,
        0,
        LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

HCURSOR WINAPI LoadCursorFromFileW(
  _In_  LPCWSTR lpFileName
)
{
    TRACE("%s\n", debugstr_w(lpFileName));

    return LoadImageW(NULL,
        lpFileName,
        IMAGE_CURSOR,
        0,
        0,
        LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

HBITMAP WINAPI LoadBitmapA(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCSTR lpBitmapName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(lpBitmapName));

    return LoadImageA(hInstance,
        lpBitmapName,
        IMAGE_BITMAP,
        0,
        0,
        0);
}

HBITMAP WINAPI LoadBitmapW(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCWSTR lpBitmapName
)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(lpBitmapName));

    return LoadImageW(hInstance,
        lpBitmapName,
        IMAGE_BITMAP,
        0,
        0,
        0);
}

HANDLE WINAPI LoadImageA(
  _In_opt_  HINSTANCE hinst,
  _In_      LPCSTR lpszName,
  _In_      UINT uType,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad
)
{
    HANDLE res;
    LPWSTR u_name;
    DWORD len;

    if (IS_INTRESOURCE(lpszName))
        return LoadImageW(hinst, (LPCWSTR)lpszName, uType, cxDesired, cyDesired, fuLoad);

    len = MultiByteToWideChar( CP_ACP, 0, lpszName, -1, NULL, 0 );
    u_name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpszName, -1, u_name, len );

    res = LoadImageW(hinst, u_name, uType, cxDesired, cyDesired, fuLoad);
    HeapFree(GetProcessHeap(), 0, u_name);
    return res;
}

HANDLE WINAPI LoadImageW(
  _In_opt_  HINSTANCE hinst,
  _In_      LPCWSTR lpszName,
  _In_      UINT uType,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad
)
{
    TRACE("hinst 0x%p, name %s, uType 0x%08x, cxDesired %d, cyDesired %d, fuLoad 0x%08x.\n",
        hinst, debugstr_w(lpszName), uType, cxDesired, cyDesired, fuLoad);
    /* Redirect to each implementation */
    switch(uType)
    {
        case IMAGE_BITMAP:
            return BITMAP_LoadImageW(hinst, lpszName, cxDesired, cyDesired, fuLoad);
        case IMAGE_CURSOR:
        case IMAGE_ICON:
            return CURSORICON_LoadImageW(hinst, lpszName, cxDesired, cyDesired, fuLoad, uType == IMAGE_ICON);
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
    }
    return NULL;
}

int WINAPI LookupIconIdFromDirectory(
  _In_  PBYTE presbits,
  _In_  BOOL fIcon
)
{
    return LookupIconIdFromDirectoryEx( presbits, fIcon,
           fIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
           fIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), fIcon ? 0 : LR_MONOCHROME );
}

int WINAPI LookupIconIdFromDirectoryEx(
  _In_  PBYTE presbits,
  _In_  BOOL fIcon,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT Flags
)
{
    WORD bppDesired;
    CURSORICONDIR* dir = (CURSORICONDIR*)presbits;
    CURSORICONDIRENTRY* entry;
    int i, numMatch = 0, iIndex = -1;
    WORD width, height, BitCount = 0;
    BOOL notPaletted = FALSE;
    ULONG bestScore = 0xFFFFFFFF, score;

    TRACE("%p, %x, %i, %i, %x.\n", presbits, fIcon, cxDesired, cyDesired, Flags);

    if(!(dir && !dir->idReserved && (dir->idType & 3)))
    {
        WARN("Invalid resource.\n");
        return 0;
    }

    if(Flags & LR_MONOCHROME)
        bppDesired = 1;
    else
    {
        HDC icScreen;
        icScreen = CreateICW(DISPLAYW, NULL, NULL, NULL);
        if(!icScreen)
            return FALSE;

        bppDesired = GetDeviceCaps(icScreen, BITSPIXEL);
        DeleteDC(icScreen);
    }

    if(!cxDesired)
        cxDesired = Flags & LR_DEFAULTSIZE ? GetSystemMetrics(fIcon ? SM_CXICON : SM_CXCURSOR) : 256;
    if(!cyDesired)
        cyDesired = Flags & LR_DEFAULTSIZE ? GetSystemMetrics(fIcon ? SM_CYICON : SM_CYCURSOR) : 256;

    /* Find the best match for the desired size */
    for(i = 0; i < dir->idCount; i++)
    {
        entry = &dir->idEntries[i];
        width = fIcon ? entry->ResInfo.icon.bWidth : entry->ResInfo.cursor.wWidth;
        /* Height is twice as big in cursor resources */
        height = fIcon ? entry->ResInfo.icon.bHeight : entry->ResInfo.cursor.wHeight/2;
        /* 0 represents 256 */
        if(!width) width = 256;
        if(!height) height = 256;
        /* Calculate the "score" (lower is better) */
        score = 2*(abs(width - cxDesired) + abs(height - cyDesired));
        if( score > bestScore)
            continue;
        /* Bigger than requested lowers the score */
        if(width > cxDesired)
            score -= width - cxDesired;
        if(height > cyDesired)
            score -= height - cyDesired;
        if(score > bestScore)
            continue;
        if(score == bestScore)
        {
            if(entry->wBitCount > BitCount)
                BitCount = entry->wBitCount;
            numMatch++;
            continue;
        }
        iIndex = i;
        numMatch = 1;
        bestScore = score;
        BitCount = entry->wBitCount;
    }

    if(numMatch == 1)
    {
        /* Only one entry fits the asked dimensions */
        return dir->idEntries[iIndex].wResId;
    }

    /* Avoid paletted icons on non-paletted device */
    if (bppDesired > 8 && BitCount > 8)
        notPaletted = TRUE;

    BitCount = 0;
    iIndex = -1;
    /* Now find the entry with the best depth */
    for(i = 0; i < dir->idCount; i++)
    {
        entry = &dir->idEntries[i];
        width = fIcon ? entry->ResInfo.icon.bWidth : entry->ResInfo.cursor.wWidth;
        height = fIcon ? entry->ResInfo.icon.bHeight : entry->ResInfo.cursor.wHeight/2;
        /* 0 represents 256 */
        if(!width) width = 256;
        if(!height) height = 256;
        /* Check if this is the best match we had */
        score = 2*(abs(width - cxDesired) + abs(height - cyDesired));
        if(width > cxDesired)
            score -= width - cxDesired;
        if(height > cyDesired)
            score -= height - cyDesired;
        if(score != bestScore)
            continue;
        /* Exact match? */
        if(entry->wBitCount == bppDesired)
            return entry->wResId;
        /* We take the highest possible but smaller  than the display depth */
        if((entry->wBitCount > BitCount) && (entry->wBitCount < bppDesired))
        {
            /* Avoid paletted icons on non paletted devices */
            if ((entry->wBitCount <= 8) && notPaletted)
                continue;
            iIndex = i;
            BitCount = entry->wBitCount;
        }
    }

    if(iIndex >= 0)
        return dir->idEntries[iIndex].wResId;

    /* No inferior or equal depth available. Get the smallest bigger one */
    BitCount = 0xFFFF;
    iIndex = -1;
    for(i = 0; i < dir->idCount; i++)
    {
        entry = &dir->idEntries[i];
        width = fIcon ? entry->ResInfo.icon.bWidth : entry->ResInfo.cursor.wWidth;
        height = fIcon ? entry->ResInfo.icon.bHeight : entry->ResInfo.cursor.wHeight/2;
        /* 0 represents 256 */
        if(!width) width = 256;
        if(!height) height = 256;
        /* Check if this is the best match we had */
        score = 2*(abs(width - cxDesired) + abs(height - cyDesired));
        if(width > cxDesired)
            score -= width - cxDesired;
        if(height > cyDesired)
            score -= height - cyDesired;
        if(score != bestScore)
            continue;
        /* Check the bit depth */
        if(entry->wBitCount < BitCount)
        {
            if((entry->wBitCount <= 8) && notPaletted)
                continue;
            iIndex = i;
            BitCount = entry->wBitCount;
        }
    }
    if (iIndex >= 0)
        return dir->idEntries[iIndex].wResId;

    return 0;
}

HICON WINAPI CreateIcon(
  _In_opt_  HINSTANCE hInstance,
  _In_      int nWidth,
  _In_      int nHeight,
  _In_      BYTE cPlanes,
  _In_      BYTE cBitsPixel,
  _In_      const BYTE *lpbANDbits,
  _In_      const BYTE *lpbXORbits
)
{
    ICONINFO iinfo;
    HICON hIcon;

    TRACE_(icon)("%dx%d, planes %d, bpp %d, xor %p, and %p\n",
                 nWidth, nHeight, cPlanes, cBitsPixel, lpbXORbits, lpbANDbits);

    iinfo.fIcon = TRUE;
    iinfo.xHotspot = nWidth / 2;
    iinfo.yHotspot = nHeight / 2;
    if (cPlanes * cBitsPixel > 1)
    {
        iinfo.hbmColor = CreateBitmap( nWidth, nHeight, cPlanes, cBitsPixel, lpbXORbits );
        iinfo.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, lpbANDbits );
    }
    else
    {
        iinfo.hbmMask = CreateBitmap( nWidth, nHeight * 2, 1, 1, lpbANDbits );
        iinfo.hbmColor = NULL;
    }

    hIcon = CreateIconIndirect( &iinfo );

    DeleteObject( iinfo.hbmMask );
    if (iinfo.hbmColor) DeleteObject( iinfo.hbmColor );

    return hIcon;
}

HICON WINAPI CreateIconFromResource(
  _In_  PBYTE presbits,
  _In_  DWORD dwResSize,
  _In_  BOOL fIcon,
  _In_  DWORD dwVer
)
{
    return CreateIconFromResourceEx( presbits, dwResSize, fIcon, dwVer, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
}

HICON WINAPI CreateIconFromResourceEx(
  _In_  PBYTE pbIconBits,
  _In_  DWORD cbIconBits,
  _In_  BOOL fIcon,
  _In_  DWORD dwVersion,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT uFlags
)
{
    CURSORDATA cursorData;
    HICON hIcon;
    BOOL isAnimated;

    TRACE("%p, %lu, %lu, %lu, %i, %i, %lu.\n", pbIconBits, cbIconBits, fIcon, dwVersion, cxDesired, cyDesired, uFlags);

    if(uFlags & LR_DEFAULTSIZE)
    {
        if(!cxDesired) cxDesired = GetSystemMetrics(fIcon ? SM_CXICON : SM_CXCURSOR);
        if(!cyDesired) cyDesired = GetSystemMetrics(fIcon ? SM_CYICON : SM_CYCURSOR);
    }

    ZeroMemory(&cursorData, sizeof(cursorData));
    cursorData.cx = cxDesired;
    cursorData.cy = cyDesired;
    cursorData.rt = (USHORT)((ULONG_PTR)(fIcon ? RT_ICON : RT_CURSOR));

    /* Convert to win32k-ready data */
    if(!memcmp(pbIconBits, "RIFF", 4))
    {
        if(!CURSORICON_GetCursorDataFromANI(&cursorData, pbIconBits, cbIconBits, uFlags))
        {
            ERR("Could not get cursor data from .ani.\n");
            return NULL;
        }
        isAnimated = !!(cursorData.CURSORF_flags & CURSORF_ACON);
    }
    else
    {
        /* It is possible to pass Icon Directories to this API */
        int wResId = LookupIconIdFromDirectoryEx(pbIconBits, fIcon, cxDesired, cyDesired, uFlags);
        HANDLE ResHandle = NULL;
#ifdef __REACTOS__
        if (wResId && (pbIconBits[4] != sizeof(BITMAPINFOHEADER)))
#else
        if(wResId)
#endif
        {
            HINSTANCE hinst;
            HRSRC hrsrc;
            CURSORICONDIR* pCurIconDir = (CURSORICONDIR*)pbIconBits;

            TRACE("Pointer points to a directory structure.\n");

            /* So this is a pointer to an icon directory structure. Find the module */
            if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    (LPCWSTR)pbIconBits,
                    &hinst))
            {
                return NULL;
            }

            /* Check we were given the right type of resource */
            if((fIcon && pCurIconDir->idType == 2) || (!fIcon && pCurIconDir->idType == 1))
            {
                WARN("Got a %s directory pointer, but called for a %s\n", fIcon ? "cursor" : "icon", fIcon ? "icon" : "cursor");
                return NULL;
            }

            /* Get the relevant resource pointer */
            hrsrc = FindResourceW(
                hinst,
                MAKEINTRESOURCEW(wResId),
                fIcon ? RT_ICON : RT_CURSOR);
            if (!hrsrc)
                return NULL;

            ResHandle = LoadResource(hinst, hrsrc);
            if (!ResHandle)
                return NULL;

            pbIconBits = LockResource(ResHandle);
            if (!pbIconBits)
            {
                FreeResource(ResHandle);
                return NULL;
            }
        }
        if(!fIcon)
        {
            WORD* pt = (WORD*)pbIconBits;
            cursorData.xHotspot = *pt++;
            cursorData.yHotspot = *pt++;
            pbIconBits = (PBYTE)pt;
        }

        if (!CURSORICON_GetCursorDataFromBMI(&cursorData, (BITMAPINFO*)pbIconBits))
        {
            ERR("Couldn't fill the CURSORDATA structure.\n");
            if (ResHandle)
                FreeResource(ResHandle);
            return NULL;
        }
        if (ResHandle)
            FreeResource(ResHandle);
        isAnimated = FALSE;
    }

    if (uFlags & LR_SHARED)
        cursorData.CURSORF_flags |= CURSORF_LRSHARED;

    hIcon = NtUserxCreateEmptyCurObject(isAnimated);
    if (!hIcon)
        goto end_error;

    if(!NtUserSetCursorIconData(hIcon, NULL, NULL, &cursorData))
    {
        ERR("NtUserSetCursorIconData failed.\n");
        NtUserDestroyCursor(hIcon, TRUE);
        goto end_error;
    }

    if(isAnimated)
        HeapFree(GetProcessHeap(), 0, cursorData.aspcur);

    return hIcon;

    /* Clean up */
end_error:
    if(isAnimated)
        HeapFree(GetProcessHeap(), 0, cursorData.aspcur);
    DeleteObject(cursorData.hbmMask);
    if(cursorData.hbmColor) DeleteObject(cursorData.hbmColor);
    if(cursorData.hbmAlpha) DeleteObject(cursorData.hbmAlpha);

    return NULL;
}

HICON WINAPI CreateIconIndirect(
  _In_  PICONINFO piconinfo
)
{
    /* As simple as creating a handle, and let win32k deal with the bitmaps */
    HICON hiconRet;
    CURSORDATA cursorData;

    TRACE("%p.\n", piconinfo);

    ZeroMemory(&cursorData, sizeof(cursorData));

    if(!CURSORICON_GetCursorDataFromIconInfo(&cursorData, piconinfo))
        return NULL;

    hiconRet = NtUserxCreateEmptyCurObject(FALSE);
    if(!hiconRet)
        goto end_error;

    if(!NtUserSetCursorIconData(hiconRet, NULL, NULL, &cursorData))
    {
        NtUserDestroyCursor(hiconRet, FALSE);
        goto end_error;
    }

    TRACE("Returning 0x%08x.\n", hiconRet);

    return hiconRet;

end_error:
    /* Clean up */
    DeleteObject(cursorData.hbmMask);
    if(cursorData.hbmColor) DeleteObject(cursorData.hbmColor);
    if(cursorData.hbmAlpha) DeleteObject(cursorData.hbmAlpha);

    return NULL;
}

HCURSOR WINAPI CreateCursor(
  _In_opt_  HINSTANCE hInst,
  _In_      int xHotSpot,
  _In_      int yHotSpot,
  _In_      int nWidth,
  _In_      int nHeight,
  _In_      const VOID *pvANDPlane,
  _In_      const VOID *pvXORPlane
)
{
    ICONINFO info;
    HCURSOR hCursor;

    TRACE_(cursor)("%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, pvXORPlane, pvANDPlane);

    info.fIcon = FALSE;
    info.xHotspot = xHotSpot;
    info.yHotspot = yHotSpot;
    info.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, pvANDPlane );
    info.hbmColor = CreateBitmap( nWidth, nHeight, 1, 1, pvXORPlane );
    hCursor = CreateIconIndirect( &info );
    DeleteObject( info.hbmMask );
    DeleteObject( info.hbmColor );
    return hCursor;
}

BOOL WINAPI SetSystemCursor(
  _In_  HCURSOR hcur,
  _In_  DWORD id
)
{
    if (hcur == NULL)
    {
       hcur = LoadImageW(NULL, MAKEINTRESOURCEW(id), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
       if (hcur == NULL)
       {
          return FALSE;
       }
    }
    return NtUserSetSystemCursor(hcur,id);
}

BOOL WINAPI SetCursorPos(
  _In_  int X,
  _In_  int Y
)
{
    return NtUserxSetCursorPos(X,Y);
}

BOOL WINAPI GetCursorPos(
  _Out_  LPPOINT lpPoint
)
{
    return NtUserxGetCursorPos(lpPoint);
}

int WINAPI ShowCursor(
  _In_  BOOL bShow
)
{
    return NtUserxShowCursor(bShow);
}

HCURSOR WINAPI GetCursor(void)
{
    return (HCURSOR)NtUserGetThreadState(THREADSTATE_GETCURSOR);
}

BOOL WINAPI DestroyCursor(
  _In_  HCURSOR hCursor
)
{
    return NtUserDestroyCursor(hCursor, FALSE);
}

HCURSOR
WINAPI
GetCursorFrameInfo(HCURSOR hCursor, DWORD reserved, DWORD istep, PINT rate_jiffies, DWORD *num_steps)
{
   return NtUserGetCursorFrameInfo(hCursor, istep, rate_jiffies, num_steps);
}
