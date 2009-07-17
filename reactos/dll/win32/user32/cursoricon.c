/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 *           1996 Martin Von Loewis
 *           1997 Alex Korobka
 *           1998 Turchanov Sergey
 *           2007 Henri Verbeet
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

/*
 * Theory:
 *
 * Cursors and icons are stored in a global heap block, with the
 * following layout:
 *
 * CURSORICONINFO info;
 * BYTE[]         ANDbits;
 * BYTE[]         XORbits;
 *
 * The bits structures are in the format of a device-dependent bitmap.
 *
 * This layout is very sub-optimal, as the bitmap bits are stored in
 * the X client instead of in the server like other bitmaps; however,
 * some programs (notably Paint Brush) expect to be able to manipulate
 * the bits directly :-(
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);
WINE_DECLARE_DEBUG_CHANNEL(icon);
WINE_DECLARE_DEBUG_CHANNEL(resource);

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

#define CID_RESOURCE  0x0001
#define CID_WIN32     0x0004
#define CID_NONSHARED 0x0008

static RECT CURSOR_ClipRect;       /* Cursor clipping rect */

static HDC screen_dc;

static const WCHAR DISPLAYW[] = {'D','I','S','P','L','A','Y',0};

/**********************************************************************
 * ICONCACHE for cursors/icons loaded with LR_SHARED.
 *
 * FIXME: This should not be allocated on the system heap, but on a
 *        subsystem-global heap (i.e. one for all Win16 processes,
 *        and one for each Win32 process).
 */
typedef struct tagICONCACHE
{
    struct tagICONCACHE *next;

    HMODULE              hModule;
    HRSRC                hRsrc;
    HRSRC                hGroupRsrc;
    HICON                hIcon;

    INT                  count;

} ICONCACHE;

static ICONCACHE *IconAnchor = NULL;

static CRITICAL_SECTION IconCrst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &IconCrst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": IconCrst") }
};
static CRITICAL_SECTION IconCrst = { &critsect_debug, -1, 0, 0, 0, 0 };

static const WORD ICON_HOTSPOT = 0x4242;


/***********************************************************************
 *             map_fileW
 *
 * Helper function to map a file to memory:
 *  name			-	file name
 *  [RETURN] ptr		-	pointer to mapped file
 *  [RETURN] filesize           -       pointer size of file to be stored if not NULL
 */
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


/***********************************************************************
 *           get_bitmap_width_bytes
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB
 * data.
 */
static int get_bitmap_width_bytes( int width, int bpp )
{
    switch(bpp)
    {
    case 1:
        return 2 * ((width+15) / 16);
    case 4:
        return 2 * ((width+3) / 4);
    case 24:
        width *= 3;
        /* fall through */
    case 8:
        return width + (width & 1);
    case 16:
    case 15:
        return width * 2;
    case 32:
        return width * 4;
    default:
        WARN("Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}


/***********************************************************************
 *          get_dib_width_bytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 */
static int get_dib_width_bytes( int width, int depth )
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
    }
    return 4 * words;
}


/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
{
    int colors, masks = 0;

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
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        return sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) + colors *
               ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *          is_dib_monochrome
 *
 * Returns whether a DIB can be converted to a monochrome DDB.
 *
 * A DIB can be converted if its color table contains only black and
 * white. Black must be the first color in the color table.
 *
 * Note : If the first color in the color table is white followed by
 *        black, we can't convert it to a monochrome DDB with
 *        SetDIBits, because black and white would be inverted.
 */
static BOOL is_dib_monochrome( const BITMAPINFO* info )
{
    if (info->bmiHeader.biBitCount != 1) return FALSE;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const RGBTRIPLE *rgb = ((const BITMAPCOREINFO*)info)->bmciColors;

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

/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER,
 * 4 for V4HEADER, 5 for V5HEADER, -1 for error.
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, DWORD *compr )
{
    if (header->biSize == sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return 1;
    }
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        return 0;
    }
    if (header->biSize == sizeof(BITMAPV4HEADER))
    {
        const BITMAPV4HEADER *v4hdr = (const BITMAPV4HEADER *)header;
        *width  = v4hdr->bV4Width;
        *height = v4hdr->bV4Height;
        *bpp    = v4hdr->bV4BitCount;
        *compr  = v4hdr->bV4V4Compression;
        return 4;
    }
    if (header->biSize == sizeof(BITMAPV5HEADER))
    {
        const BITMAPV5HEADER *v5hdr = (const BITMAPV5HEADER *)header;
        *width  = v5hdr->bV5Width;
        *height = v5hdr->bV5Height;
        *bpp    = v5hdr->bV5BitCount;
        *compr  = v5hdr->bV5Compression;
        return 5;
    }
    ERR("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}

/**********************************************************************
 *	    CURSORICON_FindSharedIcon
 */
static HICON CURSORICON_FindSharedIcon( HMODULE hModule, HRSRC hRsrc )
{
    HICON hIcon = 0;
    ICONCACHE *ptr;

    EnterCriticalSection( &IconCrst );

    for ( ptr = IconAnchor; ptr; ptr = ptr->next )
        if ( ptr->hModule == hModule && ptr->hRsrc == hRsrc )
        {
            ptr->count++;
            hIcon = ptr->hIcon;
            break;
        }

    LeaveCriticalSection( &IconCrst );

    return hIcon;
}

/*************************************************************************
 * CURSORICON_FindCache
 *
 * Given a handle, find the corresponding cache element
 *
 * PARAMS
 *      Handle     [I] handle to an Image
 *
 * RETURNS
 *     Success: The cache entry
 *     Failure: NULL
 *
 */
static ICONCACHE* CURSORICON_FindCache(HICON hIcon)
{
    ICONCACHE *ptr;
    ICONCACHE *pRet=NULL;
    BOOL IsFound = FALSE;

    EnterCriticalSection( &IconCrst );

    for (ptr = IconAnchor; ptr != NULL && !IsFound; ptr = ptr->next)
    {
        if ( hIcon == ptr->hIcon )
        {
            IsFound = TRUE;
            pRet = ptr;
        }
    }

    LeaveCriticalSection( &IconCrst );

    return pRet;
}

/**********************************************************************
 *	    CURSORICON_AddSharedIcon
 */
static void CURSORICON_AddSharedIcon( HMODULE hModule, HRSRC hRsrc, HRSRC hGroupRsrc, HICON hIcon )
{
    ICONCACHE *ptr = HeapAlloc( GetProcessHeap(), 0, sizeof(ICONCACHE) );
    if ( !ptr ) return;

    ptr->hModule = hModule;
    ptr->hRsrc   = hRsrc;
    ptr->hIcon  = hIcon;
    ptr->hGroupRsrc = hGroupRsrc;
    ptr->count   = 1;

    EnterCriticalSection( &IconCrst );
    ptr->next    = IconAnchor;
    IconAnchor   = ptr;
    LeaveCriticalSection( &IconCrst );
}

/**********************************************************************
 *	    CURSORICON_DelSharedIcon
 */
static INT CURSORICON_DelSharedIcon( HICON hIcon )
{
    INT count = -1;
    ICONCACHE *ptr;

    EnterCriticalSection( &IconCrst );

    for ( ptr = IconAnchor; ptr; ptr = ptr->next )
        if ( ptr->hIcon == hIcon )
        {
            if ( ptr->count > 0 ) ptr->count--;
            count = ptr->count;
            break;
        }

    LeaveCriticalSection( &IconCrst );

    return count;
}

/**********************************************************************
 *	    CURSORICON_FreeModuleIcons
 */
void CURSORICON_FreeModuleIcons( HMODULE16 hMod16 )
{
    ICONCACHE **ptr = &IconAnchor;
    HMODULE hModule = HMODULE_32(GetExePtr( hMod16 ));

    EnterCriticalSection( &IconCrst );

    while ( *ptr )
    {
        if ( (*ptr)->hModule == hModule )
        {
            ICONCACHE *freePtr = *ptr;
            *ptr = freePtr->next;

            GlobalFree16(HICON_16(freePtr->hIcon));
            HeapFree( GetProcessHeap(), 0, freePtr );
            continue;
        }
        ptr = &(*ptr)->next;
    }

    LeaveCriticalSection( &IconCrst );
}

/*
 *  The following macro functions account for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL (*fnGetCIEntry)( LPVOID dir, int n,
                              int *width, int *height, int *bits );

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and number of colors.
 */
static int CURSORICON_FindBestIcon( LPVOID dir, fnGetCIEntry get_entry,
                                    int width, int height, int colors )
{
    int i, cx, cy, bits, bestEntry = -1;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        iTempXDiff = abs(width - cx);
        iTempYDiff = abs(height - cy);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
    }

    /* Find Best Colors for Best Fit */
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        if(abs(width - cx) == iXDiff && abs(height - cy) == iYDiff)
        {
            iTempColorDiff = abs(colors - (1<<bits));
            if(iColorDiff > iTempColorDiff)
            {
                bestEntry = i;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    return bestEntry;
}

static BOOL CURSORICON_GetResIconEntry( LPVOID dir, int n,
                                        int *width, int *height, int *bits )
{
    CURSORICONDIR *resdir = dir;
    ICONRESDIR *icon;

    if ( resdir->idCount <= n )
        return FALSE;
    icon = &resdir->idEntries[n].ResInfo.icon;
    *width = icon->bWidth;
    *height = icon->bHeight;
    *bits = resdir->idEntries[n].wBitCount;
    return TRUE;
}

/**********************************************************************
 *	    CURSORICON_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 *
 * FIXME: parameter 'color' ignored.
 */
static int CURSORICON_FindBestCursor( LPVOID dir, fnGetCIEntry get_entry,
                                      int width, int height, int color )
{
    int i, maxwidth, maxheight, cx, cy, bits, bestEntry = -1;

    /* Double height to account for AND and XOR masks */

    height *= 2;

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = 0;
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        if ((cx <= width) && (cy <= height) &&
            (cx > maxwidth) && (cy > maxheight))
        {
            bestEntry = i;
            maxwidth  = cx;
            maxheight = cy;
        }
    }
    if (bestEntry != -1) return bestEntry;

    /* Now find the smallest one larger than the requested size */

    maxwidth = maxheight = 255;
    for ( i = 0; get_entry( dir, i, &cx, &cy, &bits ); i++ )
    {
        if (((cx < maxwidth) && (cy < maxheight)) || (bestEntry == -1))
        {
            bestEntry = i;
            maxwidth  = cx;
            maxheight = cy;
        }
    }

    return bestEntry;
}

static BOOL CURSORICON_GetResCursorEntry( LPVOID dir, int n,
                                          int *width, int *height, int *bits )
{
    CURSORICONDIR *resdir = dir;
    CURSORDIR *cursor;

    if ( resdir->idCount <= n )
        return FALSE;
    cursor = &resdir->idEntries[n].ResInfo.cursor;
    *width = cursor->wWidth;
    *height = cursor->wHeight;
    *bits = resdir->idEntries[n].wBitCount;
    return TRUE;
}

static CURSORICONDIRENTRY *CURSORICON_FindBestIconRes( CURSORICONDIR * dir,
                                      int width, int height, int colors )
{
    int n;

    n = CURSORICON_FindBestIcon( dir, CURSORICON_GetResIconEntry,
                                 width, height, colors );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static CURSORICONDIRENTRY *CURSORICON_FindBestCursorRes( CURSORICONDIR *dir,
                                      int width, int height, int color )
{
    int n = CURSORICON_FindBestCursor( dir, CURSORICON_GetResCursorEntry,
                                   width, height, color );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static BOOL CURSORICON_GetFileEntry( LPVOID dir, int n,
                                     int *width, int *height, int *bits )
{
    CURSORICONFILEDIR *filedir = dir;
    CURSORICONFILEDIRENTRY *entry;

    if ( filedir->idCount <= n )
        return FALSE;
    entry = &filedir->idEntries[n];
    *width = entry->bWidth;
    *height = entry->bHeight;
    *bits = entry->bColorCount;
    return TRUE;
}

static CURSORICONFILEDIRENTRY *CURSORICON_FindBestCursorFile( CURSORICONFILEDIR *dir,
                                      int width, int height, int color )
{
    int n = CURSORICON_FindBestCursor( dir, CURSORICON_GetFileEntry,
                                       width, height, color );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static CURSORICONFILEDIRENTRY *CURSORICON_FindBestIconFile( CURSORICONFILEDIR *dir,
                                      int width, int height, int color )
{
    int n = CURSORICON_FindBestIcon( dir, CURSORICON_GetFileEntry,
                                     width, height, color );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static HICON CURSORICON_CreateIconFromBMI( BITMAPINFO *bmi,
					   POINT16 hotspot, BOOL bIcon,
					   DWORD dwVersion,
					   INT width, INT height,
					   UINT cFlag )
{
    HGLOBAL16 hObj;
    static HDC hdcMem;
    int sizeAnd, sizeXor;
    HBITMAP hAndBits = 0, hXorBits = 0; /* error condition for later */
    BITMAP bmpXor, bmpAnd;
    BOOL DoStretch;
    INT size;

    if (dwVersion == 0x00020000)
    {
        FIXME_(cursor)("\t2.xx resources are not supported\n");
        return 0;
    }

    /* Check bitmap header */

    if ( (bmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) &&
         (bmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)  ||
          bmi->bmiHeader.biCompression != BI_RGB) )
    {
          WARN_(cursor)("\tinvalid resource bitmap header.\n");
          return 0;
    }

    size = bitmap_info_size( bmi, DIB_RGB_COLORS );

    if (!width) width = bmi->bmiHeader.biWidth;
    if (!height) height = bmi->bmiHeader.biHeight/2;
    DoStretch = (bmi->bmiHeader.biHeight/2 != height) ||
      (bmi->bmiHeader.biWidth != width);

    /* Scale the hotspot */
    if (DoStretch && hotspot.x != ICON_HOTSPOT && hotspot.y != ICON_HOTSPOT)
    {
        hotspot.x = (hotspot.x * width) / bmi->bmiHeader.biWidth;
        hotspot.y = (hotspot.y * height) / (bmi->bmiHeader.biHeight / 2);
    }

    if (!screen_dc) screen_dc = CreateDCW( DISPLAYW, NULL, NULL, NULL );
    if (screen_dc)
    {
        BITMAPINFO* pInfo;

        /* Make sure we have room for the monochrome bitmap later on.
         * Note that BITMAPINFOINFO and BITMAPCOREHEADER are the same
         * up to and including the biBitCount. In-memory icon resource
         * format is as follows:
         *
         *   BITMAPINFOHEADER   icHeader  // DIB header
         *   RGBQUAD         icColors[]   // Color table
         *   BYTE            icXOR[]      // DIB bits for XOR mask
         *   BYTE            icAND[]      // DIB bits for AND mask
         */

        if ((pInfo = HeapAlloc( GetProcessHeap(), 0,
                                max(size, sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD)))))
        {
            memcpy( pInfo, bmi, size );
            pInfo->bmiHeader.biHeight /= 2;

            /* Create the XOR bitmap */

            if (DoStretch) {
                hXorBits = CreateCompatibleBitmap(screen_dc, width, height);
                if(hXorBits)
                {
                HBITMAP hOld;
                BOOL res = FALSE;

                if (!hdcMem) hdcMem = CreateCompatibleDC(screen_dc);
                if (hdcMem) {
                    hOld = SelectObject(hdcMem, hXorBits);
                    res = StretchDIBits(hdcMem, 0, 0, width, height, 0, 0,
                                        bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight/2,
                                        (char*)bmi + size, pInfo, DIB_RGB_COLORS, SRCCOPY);
                    SelectObject(hdcMem, hOld);
                }
                if (!res) { DeleteObject(hXorBits); hXorBits = 0; }
              }
            } else {
              if (is_dib_monochrome(bmi)) {
                  hXorBits = CreateBitmap(width, height, 1, 1, NULL);
                  SetDIBits(screen_dc, hXorBits, 0, height,
                     (char*)bmi + size, pInfo, DIB_RGB_COLORS);
              }
              else
                  hXorBits = CreateDIBitmap(screen_dc, &pInfo->bmiHeader,
                     CBM_INIT, (char*)bmi + size, pInfo, DIB_RGB_COLORS); 
            }

            if( hXorBits )
            {
                char* xbits = (char *)bmi + size +
                    get_dib_width_bytes( bmi->bmiHeader.biWidth,
                                         bmi->bmiHeader.biBitCount ) * abs( bmi->bmiHeader.biHeight ) / 2;

                pInfo->bmiHeader.biBitCount = 1;
                if (pInfo->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
                {
                    RGBQUAD *rgb = pInfo->bmiColors;

                    pInfo->bmiHeader.biClrUsed = pInfo->bmiHeader.biClrImportant = 2;
                    rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
                    rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
                    rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
                }
                else
                {
                    RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)pInfo) + 1);

                    rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
                    rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
                }

                /* Create the AND bitmap */

            if (DoStretch) {
              if ((hAndBits = CreateBitmap(width, height, 1, 1, NULL))) {
                HBITMAP hOld;
                BOOL res = FALSE;

                if (!hdcMem) hdcMem = CreateCompatibleDC(screen_dc);
                if (hdcMem) {
                    hOld = SelectObject(hdcMem, hAndBits);
                    res = StretchDIBits(hdcMem, 0, 0, width, height, 0, 0,
                                        pInfo->bmiHeader.biWidth, pInfo->bmiHeader.biHeight,
                                        xbits, pInfo, DIB_RGB_COLORS, SRCCOPY);
                    SelectObject(hdcMem, hOld);
                }
                if (!res) { DeleteObject(hAndBits); hAndBits = 0; }
              }
            } else {
              hAndBits = CreateBitmap(width, height, 1, 1, NULL);

              if (hAndBits) SetDIBits(screen_dc, hAndBits, 0, height,
                             xbits, pInfo, DIB_RGB_COLORS);

            }
                if( !hAndBits ) DeleteObject( hXorBits );
            }
            HeapFree( GetProcessHeap(), 0, pInfo );
        }
    }

    if( !hXorBits || !hAndBits )
    {
        WARN_(cursor)("\tunable to create an icon bitmap.\n");
        return 0;
    }

    /* Now create the CURSORICONINFO structure */
    GetObjectA( hXorBits, sizeof(bmpXor), &bmpXor );
    GetObjectA( hAndBits, sizeof(bmpAnd), &bmpAnd );
    sizeXor = bmpXor.bmHeight * bmpXor.bmWidthBytes;
    sizeAnd = bmpAnd.bmHeight * bmpAnd.bmWidthBytes;

    hObj = GlobalAlloc16( GMEM_MOVEABLE,
                     sizeof(CURSORICONINFO) + sizeXor + sizeAnd );
    if (hObj)
    {
        CURSORICONINFO *info;

        info = GlobalLock16( hObj );
        info->ptHotSpot.x   = hotspot.x;
        info->ptHotSpot.y   = hotspot.y;
        info->nWidth        = bmpXor.bmWidth;
        info->nHeight       = bmpXor.bmHeight;
        info->nWidthBytes   = bmpXor.bmWidthBytes;
        info->bPlanes       = bmpXor.bmPlanes;
        info->bBitsPerPixel = bmpXor.bmBitsPixel;

        /* Transfer the bitmap bits to the CURSORICONINFO structure */

        GetBitmapBits( hAndBits, sizeAnd, info + 1 );
        GetBitmapBits( hXorBits, sizeXor, (char *)(info + 1) + sizeAnd );
        GlobalUnlock16( hObj );
    }

    DeleteObject( hAndBits );
    DeleteObject( hXorBits );
    return HICON_32(hObj);
}


/**********************************************************************
 *          .ANI cursor support
 */
#define RIFF_FOURCC( c0, c1, c2, c3 ) \
        ( (DWORD)(BYTE)(c0) | ( (DWORD)(BYTE)(c1) << 8 ) | \
        ( (DWORD)(BYTE)(c2) << 16 ) | ( (DWORD)(BYTE)(c3) << 24 ) )

#define ANI_RIFF_ID RIFF_FOURCC('R', 'I', 'F', 'F')
#define ANI_LIST_ID RIFF_FOURCC('L', 'I', 'S', 'T')
#define ANI_ACON_ID RIFF_FOURCC('A', 'C', 'O', 'N')
#define ANI_anih_ID RIFF_FOURCC('a', 'n', 'i', 'h')
#define ANI_seq__ID RIFF_FOURCC('s', 'e', 'q', ' ')
#define ANI_fram_ID RIFF_FOURCC('f', 'r', 'a', 'm')

#define ANI_FLAG_ICON       0x1
#define ANI_FLAG_SEQUENCE   0x2

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


/*
 * RIFF:
 * DWORD "RIFF"
 * DWORD size
 * DWORD riff_id
 * BYTE[] data
 *
 * LIST:
 * DWORD "LIST"
 * DWORD size
 * DWORD list_id
 * BYTE[] data
 *
 * CHUNK:
 * DWORD chunk_id
 * DWORD size
 * BYTE[] data
 */
static void riff_find_chunk( DWORD chunk_id, DWORD chunk_type, const riff_chunk_t *parent_chunk, riff_chunk_t *chunk )
{
    const unsigned char *ptr = parent_chunk->data;
    const unsigned char *end = parent_chunk->data + (parent_chunk->data_size - (2 * sizeof(DWORD)));

    if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) end -= sizeof(DWORD);

    while (ptr < end)
    {
        if ((!chunk_type && *(DWORD *)ptr == chunk_id )
                || (chunk_type && *(DWORD *)ptr == chunk_type && *((DWORD *)ptr + 2) == chunk_id ))
        {
            ptr += sizeof(DWORD);
            chunk->data_size = *(DWORD *)ptr;
            ptr += sizeof(DWORD);
            if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) ptr += sizeof(DWORD);
            chunk->data = ptr;

            return;
        }

        ptr += sizeof(DWORD);
        ptr += *(DWORD *)ptr;
        ptr += sizeof(DWORD);
    }
}


/*
 * .ANI layout:
 *
 * RIFF:'ACON'                  RIFF chunk
 *     |- CHUNK:'anih'          Header
 *     |- CHUNK:'seq '          Sequence information (optional)
 *     \- LIST:'fram'           Frame list
 *            |- CHUNK:icon     Cursor frames
 *            |- CHUNK:icon
 *            |- ...
 *            \- CHUNK:icon
 */
static HCURSOR CURSORICON_CreateIconFromANI( const LPBYTE bits, DWORD bits_size,
    INT width, INT height, INT colors )
{
    HCURSOR cursor;
    ani_header header = {0};
    LPBYTE frame_bits = 0;
    POINT16 hotspot;
    CURSORICONFILEDIRENTRY *entry;

    riff_chunk_t root_chunk = { bits_size, bits };
    riff_chunk_t ACON_chunk = {0};
    riff_chunk_t anih_chunk = {0};
    riff_chunk_t fram_chunk = {0};
    const unsigned char *icon_data;

    TRACE("bits %p, bits_size %d\n", bits, bits_size);

    if (!bits) return 0;

    riff_find_chunk( ANI_ACON_ID, ANI_RIFF_ID, &root_chunk, &ACON_chunk );
    if (!ACON_chunk.data)
    {
        ERR("Failed to get root chunk.\n");
        return 0;
    }

    riff_find_chunk( ANI_anih_ID, 0, &ACON_chunk, &anih_chunk );
    if (!anih_chunk.data)
    {
        ERR("Failed to get 'anih' chunk.\n");
        return 0;
    }
    memcpy( &header, anih_chunk.data, sizeof(header) );
    dump_ani_header( &header );

    riff_find_chunk( ANI_fram_ID, ANI_LIST_ID, &ACON_chunk, &fram_chunk );
    if (!fram_chunk.data)
    {
        ERR("Failed to get icon list.\n");
        return 0;
    }

    /* FIXME: For now, just load the first frame.  Before we can load all the
     * frames, we need to write the needed code in wineserver, etc. to handle
     * cursors.  Once this code is written, we can extend it to support .ani
     * cursors and then update user32 and winex11.drv to load all frames.
     *
     * Hopefully this will at least make some games (C&C3, etc.) more playable
     * in the meantime.
     */
    FIXME("Loading all frames for .ani cursors not implemented.\n");
    icon_data = fram_chunk.data + (2 * sizeof(DWORD));

    entry = CURSORICON_FindBestIconFile( (CURSORICONFILEDIR *) icon_data,
        width, height, colors );

    frame_bits = HeapAlloc( GetProcessHeap(), 0, entry->dwDIBSize );
    memcpy( frame_bits, icon_data + entry->dwDIBOffset, entry->dwDIBSize );

    if (!header.width || !header.height)
    {
        header.width = entry->bWidth;
        header.height = entry->bHeight;
    }

    hotspot.x = entry->xHotspot;
    hotspot.y = entry->yHotspot;

    cursor = CURSORICON_CreateIconFromBMI( (BITMAPINFO *) frame_bits, hotspot,
        FALSE, 0x00030000, header.width, header.height, 0 );

    HeapFree( GetProcessHeap(), 0, frame_bits );

    return cursor;
}


/**********************************************************************
 *		CreateIconFromResourceEx (USER32.@)
 *
 * FIXME: Convert to mono when cFlag is LR_MONOCHROME. Do something
 *        with cbSize parameter as well.
 */
HICON WINAPI CreateIconFromResourceEx( LPBYTE bits, UINT cbSize,
                                       BOOL bIcon, DWORD dwVersion,
                                       INT width, INT height,
                                       UINT cFlag )
{
    POINT16 hotspot;
    BITMAPINFO *bmi;

    hotspot.x = ICON_HOTSPOT;
    hotspot.y = ICON_HOTSPOT;

    TRACE_(cursor)("%p (%u bytes), ver %08x, %ix%i %s %s\n",
                   bits, cbSize, dwVersion, width, height,
                                  bIcon ? "icon" : "cursor", (cFlag & LR_MONOCHROME) ? "mono" : "" );

    if (bIcon)
        bmi = (BITMAPINFO *)bits;
    else /* get the hotspot */
    {
        POINT16 *pt = (POINT16 *)bits;
        hotspot = *pt;
        bmi = (BITMAPINFO *)(pt + 1);
    }

    return CURSORICON_CreateIconFromBMI( bmi, hotspot, bIcon, dwVersion,
					 width, height, cFlag );
}


/**********************************************************************
 *		CreateIconFromResource (USER32.@)
 */
HICON WINAPI CreateIconFromResource( LPBYTE bits, UINT cbSize,
                                           BOOL bIcon, DWORD dwVersion)
{
    return CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, 0,0,0);
}


static HICON CURSORICON_LoadFromFile( LPCWSTR filename,
                             INT width, INT height, INT colors,
                             BOOL fCursor, UINT loadflags)
{
    CURSORICONFILEDIRENTRY *entry;
    CURSORICONFILEDIR *dir;
    DWORD filesize = 0;
    HICON hIcon = 0;
    LPBYTE bits;
    POINT16 hotspot;

    TRACE("loading %s\n", debugstr_w( filename ));

    bits = map_fileW( filename, &filesize );
    if (!bits)
        return hIcon;

    /* Check for .ani. */
    if (memcmp( bits, "RIFF", 4 ) == 0)
    {
        hIcon = CURSORICON_CreateIconFromANI( bits, filesize, width, height,
            colors );
        goto end;
    }

    dir = (CURSORICONFILEDIR*) bits;
    if ( filesize < sizeof(*dir) )
        goto end;

    if ( filesize < (sizeof(*dir) + sizeof(dir->idEntries[0])*(dir->idCount-1)) )
        goto end;

    if ( fCursor )
        entry = CURSORICON_FindBestCursorFile( dir, width, height, colors );
    else
        entry = CURSORICON_FindBestIconFile( dir, width, height, colors );

    if ( !entry )
        goto end;

    /* check that we don't run off the end of the file */
    if ( entry->dwDIBOffset > filesize )
        goto end;
    if ( entry->dwDIBOffset + entry->dwDIBSize > filesize )
        goto end;

    /* Set the actual hotspot for cursors and ICON_HOTSPOT for icons. */
    if ( fCursor )
    {
        hotspot.x = entry->xHotspot;
        hotspot.y = entry->yHotspot;
    }
    else
    {
        hotspot.x = ICON_HOTSPOT;
        hotspot.y = ICON_HOTSPOT;
    }
    hIcon = CURSORICON_CreateIconFromBMI( (BITMAPINFO *)&bits[entry->dwDIBOffset],
					  hotspot, !fCursor, 0x00030000,
					  width, height, loadflags );
end:
    TRACE("loaded %s -> %p\n", debugstr_w( filename ), hIcon );
    UnmapViewOfFile( bits );
    return hIcon;
}

/**********************************************************************
 *          CURSORICON_Load
 *
 * Load a cursor or icon from resource or file.
 */
static HICON CURSORICON_Load(HINSTANCE hInstance, LPCWSTR name,
                             INT width, INT height, INT colors,
                             BOOL fCursor, UINT loadflags)
{
    HANDLE handle = 0;
    HICON hIcon = 0;
    HRSRC hRsrc, hGroupRsrc;
    CURSORICONDIR *dir;
    CURSORICONDIRENTRY *dirEntry;
    LPBYTE bits;
    WORD wResId;
    DWORD dwBytesInRes;

    TRACE("%p, %s, %dx%d, colors %d, fCursor %d, flags 0x%04x\n",
          hInstance, debugstr_w(name), width, height, colors, fCursor, loadflags);

    if ( loadflags & LR_LOADFROMFILE )    /* Load from file */
        return CURSORICON_LoadFromFile( name, width, height, colors, fCursor, loadflags );

    if (!hInstance) hInstance = user32_module;  /* Load OEM cursor/icon */

    /* Normalize hInstance (must be uniquely represented for icon cache) */

    if (!HIWORD( hInstance ))
        hInstance = HINSTANCE_32(GetExePtr( HINSTANCE_16(hInstance) ));

    /* Get directory resource ID */

    if (!(hRsrc = FindResourceW( hInstance, name,
                                 (LPWSTR)(fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON) )))
        return 0;
    hGroupRsrc = hRsrc;

    /* Find the best entry in the directory */

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    if (!(dir = LockResource( handle ))) return 0;
    if (fCursor)
        dirEntry = CURSORICON_FindBestCursorRes( dir, width, height, colors );
    else
        dirEntry = CURSORICON_FindBestIconRes( dir, width, height, colors );
    if (!dirEntry) return 0;
    wResId = dirEntry->wResId;
    dwBytesInRes = dirEntry->dwBytesInRes;
    FreeResource( handle );

    /* Load the resource */

    if (!(hRsrc = FindResourceW(hInstance,MAKEINTRESOURCEW(wResId),
                                (LPWSTR)(fCursor ? RT_CURSOR : RT_ICON) ))) return 0;

    /* If shared icon, check whether it was already loaded */
    if (    (loadflags & LR_SHARED)
         && (hIcon = CURSORICON_FindSharedIcon( hInstance, hRsrc ) ) != 0 )
        return hIcon;

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    bits = LockResource( handle );
    hIcon = CreateIconFromResourceEx( bits, dwBytesInRes,
                                      !fCursor, 0x00030000, width, height, loadflags);
    FreeResource( handle );

    /* If shared icon, add to icon cache */

    if ( hIcon && (loadflags & LR_SHARED) )
        CURSORICON_AddSharedIcon( hInstance, hRsrc, hGroupRsrc, hIcon );

    return hIcon;
}

/***********************************************************************
 *           CURSORICON_Copy
 *
 * Make a copy of a cursor or icon.
 */
static HICON CURSORICON_Copy( HINSTANCE16 hInst16, HICON hIcon )
{
    char *ptrOld, *ptrNew;
    int size;
    HICON16 hOld = HICON_16(hIcon);
    HICON16 hNew;

    if (!(ptrOld = GlobalLock16( hOld ))) return 0;
    if (hInst16 && !(hInst16 = GetExePtr( hInst16 ))) return 0;
    size = GlobalSize16( hOld );
    hNew = GlobalAlloc16( GMEM_MOVEABLE, size );
    FarSetOwner16( hNew, hInst16 );
    ptrNew = GlobalLock16( hNew );
    memcpy( ptrNew, ptrOld, size );
    GlobalUnlock16( hOld );
    GlobalUnlock16( hNew );
    return HICON_32(hNew);
}

/*************************************************************************
 * CURSORICON_ExtCopy
 *
 * Copies an Image from the Cache if LR_COPYFROMRESOURCE is specified
 *
 * PARAMS
 *      Handle     [I] handle to an Image
 *      nType      [I] Type of Handle (IMAGE_CURSOR | IMAGE_ICON)
 *      iDesiredCX [I] The Desired width of the Image
 *      iDesiredCY [I] The desired height of the Image
 *      nFlags     [I] The flags from CopyImage
 *
 * RETURNS
 *     Success: The new handle of the Image
 *
 * NOTES
 *     LR_COPYDELETEORG and LR_MONOCHROME are currently not implemented.
 *     LR_MONOCHROME should be implemented by CreateIconFromResourceEx.
 *     LR_COPYFROMRESOURCE will only work if the Image is in the Cache.
 *
 *
 */

static HICON CURSORICON_ExtCopy(HICON hIcon, UINT nType,
                                INT iDesiredCX, INT iDesiredCY,
                                UINT nFlags)
{
    HICON hNew=0;

    TRACE_(icon)("hIcon %p, nType %u, iDesiredCX %i, iDesiredCY %i, nFlags %u\n",
                 hIcon, nType, iDesiredCX, iDesiredCY, nFlags);

    if(hIcon == 0)
    {
        return 0;
    }

    /* Best Fit or Monochrome */
    if( (nFlags & LR_COPYFROMRESOURCE
        && (iDesiredCX > 0 || iDesiredCY > 0))
        || nFlags & LR_MONOCHROME)
    {
        ICONCACHE* pIconCache = CURSORICON_FindCache(hIcon);

        /* Not Found in Cache, then do a straight copy
        */
        if(pIconCache == NULL)
        {
            hNew = CURSORICON_Copy(0, hIcon);
            if(nFlags & LR_COPYFROMRESOURCE)
            {
                TRACE_(icon)("LR_COPYFROMRESOURCE: Failed to load from cache\n");
            }
        }
        else
        {
            int iTargetCY = iDesiredCY, iTargetCX = iDesiredCX;
            LPBYTE pBits;
            HANDLE hMem;
            HRSRC hRsrc;
            DWORD dwBytesInRes;
            WORD wResId;
            CURSORICONDIR *pDir;
            CURSORICONDIRENTRY *pDirEntry;
            BOOL bIsIcon = (nType == IMAGE_ICON);

            /* Completing iDesiredCX CY for Monochrome Bitmaps if needed
            */
            if(((nFlags & LR_MONOCHROME) && !(nFlags & LR_COPYFROMRESOURCE))
                || (iDesiredCX == 0 && iDesiredCY == 0))
            {
                iDesiredCY = GetSystemMetrics(bIsIcon ?
                    SM_CYICON : SM_CYCURSOR);
                iDesiredCX = GetSystemMetrics(bIsIcon ?
                    SM_CXICON : SM_CXCURSOR);
            }

            /* Retrieve the CURSORICONDIRENTRY
            */
            if (!(hMem = LoadResource( pIconCache->hModule ,
                            pIconCache->hGroupRsrc)))
            {
                return 0;
            }
            if (!(pDir = LockResource( hMem )))
            {
                return 0;
            }

            /* Find Best Fit
            */
            if(bIsIcon)
            {
                pDirEntry = CURSORICON_FindBestIconRes(
                                pDir, iDesiredCX, iDesiredCY, 256 );
            }
            else
            {
                pDirEntry = CURSORICON_FindBestCursorRes(
                                pDir, iDesiredCX, iDesiredCY, 1);
            }

            wResId = pDirEntry->wResId;
            dwBytesInRes = pDirEntry->dwBytesInRes;
            FreeResource(hMem);

            TRACE_(icon)("ResID %u, BytesInRes %u, Width %d, Height %d DX %d, DY %d\n",
                wResId, dwBytesInRes,  pDirEntry->ResInfo.icon.bWidth,
                pDirEntry->ResInfo.icon.bHeight, iDesiredCX, iDesiredCY);

            /* Get the Best Fit
            */
            if (!(hRsrc = FindResourceW(pIconCache->hModule ,
                MAKEINTRESOURCEW(wResId), (LPWSTR)(bIsIcon ? RT_ICON : RT_CURSOR))))
            {
                return 0;
            }
            if (!(hMem = LoadResource( pIconCache->hModule , hRsrc )))
            {
                return 0;
            }

            pBits = LockResource( hMem );

            if(nFlags & LR_DEFAULTSIZE)
            {
                iTargetCY = GetSystemMetrics(SM_CYICON);
                iTargetCX = GetSystemMetrics(SM_CXICON);
            }

            /* Create a New Icon with the proper dimension
            */
            hNew = CreateIconFromResourceEx( pBits, dwBytesInRes,
                       bIsIcon, 0x00030000, iTargetCX, iTargetCY, nFlags);
            FreeResource(hMem);
        }
    }
    else hNew = CURSORICON_Copy(0, hIcon);
    return hNew;
}


/***********************************************************************
 *		CreateCursor (USER32.@)
 */
HCURSOR WINAPI CreateCursor( HINSTANCE hInstance,
                                 INT xHotSpot, INT yHotSpot,
                                 INT nWidth, INT nHeight,
                                 LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(cursor)("%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);

    info.ptHotSpot.x = xHotSpot;
    info.ptHotSpot.y = yHotSpot;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = 1;
    info.bBitsPerPixel = 1;

    return HICON_32(CreateCursorIconIndirect16(0, &info, lpANDbits, lpXORbits));
}


/***********************************************************************
 *		CreateIcon (USER.407)
 */
#ifndef __REACTOS__
HICON16 WINAPI CreateIcon16( HINSTANCE16 hInstance, INT16 nWidth,
                             INT16 nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(icon)("%dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);

    info.ptHotSpot.x = ICON_HOTSPOT;
    info.ptHotSpot.y = ICON_HOTSPOT;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = bPlanes;
    info.bBitsPerPixel = bBitsPixel;

    return CreateCursorIconIndirect16( hInstance, &info, lpANDbits, lpXORbits );
}
#endif


/***********************************************************************
 *		CreateIcon (USER32.@)
 *
 *  Creates an icon based on the specified bitmaps. The bitmaps must be
 *  provided in a device dependent format and will be resized to
 *  (SM_CXICON,SM_CYICON) and depth converted to match the screen's color
 *  depth. The provided bitmaps must be top-down bitmaps.
 *  Although Windows does not support 15bpp(*) this API must support it
 *  for Winelib applications.
 *
 *  (*) Windows does not support 15bpp but it supports the 555 RGB 16bpp
 *      format!
 *
 * RETURNS
 *  Success: handle to an icon
 *  Failure: NULL
 *
 * FIXME: Do we need to resize the bitmaps?
 */
HICON WINAPI CreateIcon(
    HINSTANCE hInstance,  /* [in] the application's hInstance */
    INT       nWidth,     /* [in] the width of the provided bitmaps */
    INT       nHeight,    /* [in] the height of the provided bitmaps */
    BYTE      bPlanes,    /* [in] the number of planes in the provided bitmaps */
    BYTE      bBitsPixel, /* [in] the number of bits per pixel of the lpXORbits bitmap */
    LPCVOID   lpANDbits,  /* [in] a monochrome bitmap representing the icon's mask */
    LPCVOID   lpXORbits)  /* [in] the icon's 'color' bitmap */
{
    ICONINFO iinfo;
    HICON hIcon;

    TRACE_(icon)("%dx%d, planes %d, bpp %d, xor %p, and %p\n",
                 nWidth, nHeight, bPlanes, bBitsPixel, lpXORbits, lpANDbits);

    iinfo.fIcon = TRUE;
    iinfo.xHotspot = ICON_HOTSPOT;
    iinfo.yHotspot = ICON_HOTSPOT;
    iinfo.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, lpANDbits );
    iinfo.hbmColor = CreateBitmap( nWidth, nHeight, bPlanes, bBitsPixel, lpXORbits );

    hIcon = CreateIconIndirect( &iinfo );

    DeleteObject( iinfo.hbmMask );
    DeleteObject( iinfo.hbmColor );

    return hIcon;
}


/***********************************************************************
 *		CreateCursorIconIndirect (USER.408)
 */
HGLOBAL16 WINAPI CreateCursorIconIndirect16( HINSTANCE16 hInstance,
                                           CURSORICONINFO *info,
                                           LPCVOID lpANDbits,
                                           LPCVOID lpXORbits )
{
    HGLOBAL16 handle;
    char *ptr;
    int sizeAnd, sizeXor;

    hInstance = GetExePtr( hInstance );  /* Make it a module handle */
    if (!lpXORbits || !lpANDbits || info->bPlanes != 1) return 0;
    info->nWidthBytes = get_bitmap_width_bytes(info->nWidth,info->bBitsPerPixel);
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
        return 0;
    FarSetOwner16( handle, hInstance );
    ptr = GlobalLock16( handle );
    memcpy( ptr, info, sizeof(*info) );
    memcpy( ptr + sizeof(CURSORICONINFO), lpANDbits, sizeAnd );
    memcpy( ptr + sizeof(CURSORICONINFO) + sizeAnd, lpXORbits, sizeXor );
    GlobalUnlock16( handle );
    return handle;
}


/***********************************************************************
 *		CopyIcon (USER.368)
 */
HICON16 WINAPI CopyIcon16( HINSTANCE16 hInstance, HICON16 hIcon )
{
    TRACE_(icon)("%04x %04x\n", hInstance, hIcon );
    return HICON_16(CURSORICON_Copy(hInstance, HICON_32(hIcon)));
}


/***********************************************************************
 *		CopyIcon (USER32.@)
 */
HICON WINAPI CopyIcon( HICON hIcon )
{
    TRACE_(icon)("%p\n", hIcon );
    return CURSORICON_Copy( 0, hIcon );
}


/***********************************************************************
 *		CopyCursor (USER.369)
 */
HCURSOR16 WINAPI CopyCursor16( HINSTANCE16 hInstance, HCURSOR16 hCursor )
{
    TRACE_(cursor)("%04x %04x\n", hInstance, hCursor );
    return HICON_16(CURSORICON_Copy(hInstance, HCURSOR_32(hCursor)));
}

/**********************************************************************
 *		DestroyIcon32 (USER.610)
 *
 * This routine is actually exported from Win95 USER under the name
 * DestroyIcon32 ...  The behaviour implemented here should mimic
 * the Win95 one exactly, especially the return values, which
 * depend on the setting of various flags.
 */
WORD WINAPI DestroyIcon32( HGLOBAL16 handle, UINT16 flags )
{
    WORD retv;

    TRACE_(icon)("(%04x, %04x)\n", handle, flags );

    /* Check whether destroying active cursor */

    if ( get_user_thread_info()->cursor == HICON_32(handle) )
    {
        WARN_(cursor)("Destroying active cursor!\n" );
        return FALSE;
    }

    /* Try shared cursor/icon first */

    if ( !(flags & CID_NONSHARED) )
    {
        INT count = CURSORICON_DelSharedIcon(HICON_32(handle));

        if ( count != -1 )
            return (flags & CID_WIN32)? TRUE : (count == 0);

        /* FIXME: OEM cursors/icons should be recognized */
    }

    /* Now assume non-shared cursor/icon */

    retv = GlobalFree16( handle );
    return (flags & CID_RESOURCE)? retv : TRUE;
}

/***********************************************************************
 *		DestroyIcon (USER32.@)
 */
BOOL WINAPI DestroyIcon( HICON hIcon )
{
    return DestroyIcon32(HICON_16(hIcon), CID_WIN32);
}


/***********************************************************************
 *		DestroyCursor (USER32.@)
 */
BOOL WINAPI DestroyCursor( HCURSOR hCursor )
{
    return DestroyIcon32(HCURSOR_16(hCursor), CID_WIN32);
}

/***********************************************************************
 *      bitmap_has_alpha_channel
 *
 * Analyses bits bitmap to determine if alpha data is present.
 *
 * PARAMS
 *      bpp          [I] The bits-per-pixel of the bitmap
 *      bitmapBits   [I] A pointer to the bitmap data
 *      bitmapLength [I] The length of the bitmap in bytes
 *
 * RETURNS
 *      TRUE if an alpha channel is discovered, FALSE
 *
 * NOTE
 *      Windows' behaviour is that if the icon bitmap is 32-bit and at
 *      least one pixel has a non-zero alpha, then the bitmap is a
 *      treated as having an alpha channel transparentcy. Otherwise,
 *      it's treated as being completely opaque.
 *
 */
static BOOL bitmap_has_alpha_channel( int bpp, unsigned char *bitmapBits,
                                      unsigned int bitmapLength )
{
    /* Detect an alpha channel by looking for non-zero alpha pixels */
    if(bpp == 32)
    {
        unsigned int offset;
        for(offset = 3; offset < bitmapLength; offset += 4)
        {
            if(bitmapBits[offset] != 0)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *          premultiply_alpha_channel
 *
 * Premultiplies the color channels of a 32-bit bitmap by the alpha
 * channel. This is a necessary step that must be carried out on
 * the image before it is passed to GdiAlphaBlend
 *
 * PARAMS
 *      destBitmap   [I] The destination bitmap buffer
 *      srcBitmap    [I] The source bitmap buffer
 *      bitmapLength [I] The length of the bitmap in bytes
 *
 */
static void premultiply_alpha_channel( unsigned char *destBitmap,
                                       unsigned char *srcBitmap,
                                       unsigned int bitmapLength )
{
    unsigned char *destPixel = destBitmap;
    unsigned char *srcPixel = srcBitmap;

    while(destPixel < destBitmap + bitmapLength)
    {
        unsigned char alpha = srcPixel[3];
        *(destPixel++) = *(srcPixel++) * alpha / 255;
        *(destPixel++) = *(srcPixel++) * alpha / 255;
        *(destPixel++) = *(srcPixel++) * alpha / 255;
        *(destPixel++) = *(srcPixel++);
    }
}

/***********************************************************************
 *		DrawIcon (USER32.@)
 */
BOOL WINAPI DrawIcon( HDC hdc, INT x, INT y, HICON hIcon )
{
    CURSORICONINFO *ptr;
    HDC hMemDC;
    HBITMAP hXorBits = NULL, hAndBits = NULL, hBitTemp = NULL;
    COLORREF oldFg, oldBg;
    unsigned char *xorBitmapBits;
    unsigned int dibLength;

    TRACE("%p, (%d,%d), %p\n", hdc, x, y, hIcon);

    if (!(ptr = GlobalLock16(HICON_16(hIcon)))) return FALSE;
    if (!(hMemDC = CreateCompatibleDC( hdc ))) return FALSE;

    dibLength = ptr->nHeight * get_bitmap_width_bytes(
        ptr->nWidth, ptr->bBitsPerPixel);

    xorBitmapBits = (unsigned char *)(ptr + 1) + ptr->nHeight *
                    get_bitmap_width_bytes(ptr->nWidth, 1);

    oldFg = SetTextColor( hdc, RGB(0,0,0) );
    oldBg = SetBkColor( hdc, RGB(255,255,255) );

    if(bitmap_has_alpha_channel(ptr->bBitsPerPixel, xorBitmapBits, dibLength))
    {
        BITMAPINFOHEADER bmih;
        unsigned char *dibBits;

        memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biWidth = ptr->nWidth;
        bmih.biHeight = -ptr->nHeight;
        bmih.biPlanes = ptr->bPlanes;
        bmih.biBitCount = 32;
        bmih.biCompression = BI_RGB;

        hXorBits = CreateDIBSection(hdc, (BITMAPINFO*)&bmih, DIB_RGB_COLORS,
                                    (void*)&dibBits, NULL, 0);

        if (hXorBits && dibBits)
        {
            BLENDFUNCTION pixelblend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

            /* Do the alpha blending render */
            premultiply_alpha_channel(dibBits, xorBitmapBits, dibLength);
            hBitTemp = SelectObject( hMemDC, hXorBits );
            GdiAlphaBlend(hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC,
                            0, 0, ptr->nWidth, ptr->nHeight, pixelblend);
            SelectObject( hMemDC, hBitTemp );
        }
    }
    else
    {
        hAndBits = CreateBitmap( ptr->nWidth, ptr->nHeight, 1, 1, ptr + 1 );
        hXorBits = CreateBitmap( ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                               ptr->bBitsPerPixel, xorBitmapBits);

        if (hXorBits && hAndBits)
        {
            hBitTemp = SelectObject( hMemDC, hAndBits );
            BitBlt( hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0, SRCAND );
            SelectObject( hMemDC, hXorBits );
            BitBlt(hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0,SRCINVERT);
            SelectObject( hMemDC, hBitTemp );
        }
    }

    DeleteDC( hMemDC );
    if (hXorBits) DeleteObject( hXorBits );
    if (hAndBits) DeleteObject( hAndBits );
    GlobalUnlock16(HICON_16(hIcon));
    SetTextColor( hdc, oldFg );
    SetBkColor( hdc, oldBg );
    return TRUE;
}

/***********************************************************************
 *		DumpIcon (USER.459)
 */
DWORD WINAPI DumpIcon16( SEGPTR pInfo, WORD *lpLen,
                       SEGPTR *lpXorBits, SEGPTR *lpAndBits )
{
    CURSORICONINFO *info = MapSL( pInfo );
    int sizeAnd, sizeXor;

    if (!info) return 0;
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    if (lpAndBits) *lpAndBits = pInfo + sizeof(CURSORICONINFO);
    if (lpXorBits) *lpXorBits = pInfo + sizeof(CURSORICONINFO) + sizeAnd;
    if (lpLen) *lpLen = sizeof(CURSORICONINFO) + sizeAnd + sizeXor;
    return MAKELONG( sizeXor, sizeXor );
}


/***********************************************************************
 *		SetCursor (USER32.@)
 *
 * Set the cursor shape.
 *
 * RETURNS
 *	A handle to the previous cursor shape.
 */
HCURSOR WINAPI SetCursor( HCURSOR hCursor /* [in] Handle of cursor to show */ )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    HCURSOR hOldCursor;

    if (hCursor == thread_info->cursor) return hCursor;  /* No change */
    TRACE("%p\n", hCursor);
    hOldCursor = thread_info->cursor;
    thread_info->cursor = hCursor;
    /* Change the cursor shape only if it is visible */
    if (thread_info->cursor_count >= 0)
    {
        USER_Driver->pSetCursor(GlobalLock16(HCURSOR_16(hCursor)));
        GlobalUnlock16(HCURSOR_16(hCursor));
    }
    return hOldCursor;
}

/***********************************************************************
 *		ShowCursor (USER32.@)
 */
INT WINAPI ShowCursor( BOOL bShow )
{
    struct user_thread_info *thread_info = get_user_thread_info();

    TRACE("%d, count=%d\n", bShow, thread_info->cursor_count );

    if (bShow)
    {
        if (++thread_info->cursor_count == 0) /* Show it */
        {
            USER_Driver->pSetCursor(GlobalLock16(HCURSOR_16(thread_info->cursor)));
            GlobalUnlock16(HCURSOR_16(thread_info->cursor));
        }
    }
    else
    {
        if (--thread_info->cursor_count == -1) /* Hide it */
            USER_Driver->pSetCursor( NULL );
    }
    return thread_info->cursor_count;
}

/***********************************************************************
 *		GetCursor (USER32.@)
 */
HCURSOR WINAPI GetCursor(void)
{
    return get_user_thread_info()->cursor;
}


/***********************************************************************
 *		ClipCursor (USER32.@)
 */
BOOL WINAPI ClipCursor( const RECT *rect )
{
    RECT virt;

    SetRect( &virt, 0, 0, GetSystemMetrics( SM_CXVIRTUALSCREEN ),
                          GetSystemMetrics( SM_CYVIRTUALSCREEN ) );
    OffsetRect( &virt, GetSystemMetrics( SM_XVIRTUALSCREEN ),
                       GetSystemMetrics( SM_YVIRTUALSCREEN ) );

    TRACE( "Clipping to: %s was: %s screen: %s\n", wine_dbgstr_rect(rect),
           wine_dbgstr_rect(&CURSOR_ClipRect), wine_dbgstr_rect(&virt) );

    if (!IntersectRect( &CURSOR_ClipRect, &virt, rect ))
        CURSOR_ClipRect = virt;

    USER_Driver->pClipCursor( rect );
    return TRUE;
}


/***********************************************************************
 *		GetClipCursor (USER32.@)
 */
BOOL WINAPI GetClipCursor( RECT *rect )
{
    /* If this is first time - initialize the rect */
    if (IsRectEmpty( &CURSOR_ClipRect )) ClipCursor( NULL );

    return CopyRect( rect, &CURSOR_ClipRect );
}


/***********************************************************************
 *		SetSystemCursor (USER32.@)
 */
BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{
    FIXME("(%p,%08x),stub!\n",  hcur, id);
    return TRUE;
}


/**********************************************************************
 *		LookupIconIdFromDirectoryEx (USER.364)
 *
 * FIXME: exact parameter sizes
 */
INT16 WINAPI LookupIconIdFromDirectoryEx16( LPBYTE dir, BOOL16 bIcon,
                                            INT16 width, INT16 height, UINT16 cFlag )
{
    return LookupIconIdFromDirectoryEx( dir, bIcon, width, height, cFlag );
}

/**********************************************************************
 *		LookupIconIdFromDirectoryEx (USER32.@)
 */
INT WINAPI LookupIconIdFromDirectoryEx( LPBYTE xdir, BOOL bIcon,
             INT width, INT height, UINT cFlag )
{
    CURSORICONDIR       *dir = (CURSORICONDIR*)xdir;
    UINT retVal = 0;
    if( dir && !dir->idReserved && (dir->idType & 3) )
    {
        CURSORICONDIRENTRY* entry;
        HDC hdc;
        UINT palEnts;
        int colors;
        hdc = GetDC(0);
        palEnts = GetSystemPaletteEntries(hdc, 0, 0, NULL);
        if (palEnts == 0)
            palEnts = 256;
        colors = (cFlag & LR_MONOCHROME) ? 2 : palEnts;

        ReleaseDC(0, hdc);

        if( bIcon )
            entry = CURSORICON_FindBestIconRes( dir, width, height, colors );
        else
            entry = CURSORICON_FindBestCursorRes( dir, width, height, colors );

        if( entry ) retVal = entry->wResId;
    }
    else WARN_(cursor)("invalid resource directory\n");
    return retVal;
}

/**********************************************************************
 *              LookupIconIdFromDirectory (USER32.@)
 */
INT WINAPI LookupIconIdFromDirectory( LPBYTE dir, BOOL bIcon )
{
    return LookupIconIdFromDirectoryEx( dir, bIcon,
           bIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
           bIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), bIcon ? 0 : LR_MONOCHROME );
}

/**********************************************************************
 *              GetIconID (USER.455)
 */
#ifndef __REACTOS__
WORD WINAPI GetIconID16( HGLOBAL16 hResource, DWORD resType )
{
    LPBYTE lpDir = GlobalLock16(hResource);

    TRACE_(cursor)("hRes=%04x, entries=%i\n",
                    hResource, lpDir ? ((CURSORICONDIR*)lpDir)->idCount : 0);

    switch(resType)
    {
        case RT_CURSOR:
             return (WORD)LookupIconIdFromDirectoryEx16( lpDir, FALSE,
                          GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_MONOCHROME );
        case RT_ICON:
             return (WORD)LookupIconIdFromDirectoryEx16( lpDir, TRUE,
                          GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0 );
        default:
             WARN_(cursor)("invalid res type %d\n", resType );
    }
    return 0;
}

/**********************************************************************
 *              LoadCursorIconHandler (USER.336)
 *
 * Supposed to load resources of Windows 2.x applications.
 */
HGLOBAL16 WINAPI LoadCursorIconHandler16( HGLOBAL16 hResource, HMODULE16 hModule, HRSRC16 hRsrc )
{
    FIXME_(cursor)("(%04x,%04x,%04x): old 2.x resources are not supported!\n",
          hResource, hModule, hRsrc);
    return 0;
}

/**********************************************************************
 *              LoadIconHandler (USER.456)
 */
HICON16 WINAPI LoadIconHandler16( HGLOBAL16 hResource, BOOL16 bNew )
{
    LPBYTE bits = LockResource16( hResource );

    TRACE_(cursor)("hRes=%04x\n",hResource);

    return HICON_16(CreateIconFromResourceEx( bits, 0, TRUE,
                      bNew ? 0x00030000 : 0x00020000, 0, 0, LR_DEFAULTCOLOR));
}
#endif
/***********************************************************************
 *              LoadCursorW (USER32.@)
 */
HCURSOR WINAPI LoadCursorW(HINSTANCE hInstance, LPCWSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(name));

    return LoadImageW( hInstance, name, IMAGE_CURSOR, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorA (USER32.@)
 */
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(name));

    return LoadImageA( hInstance, name, IMAGE_CURSOR, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorFromFileW (USER32.@)
 */
HCURSOR WINAPI LoadCursorFromFileW (LPCWSTR name)
{
    TRACE("%s\n", debugstr_w(name));

    return LoadImageW( 0, name, IMAGE_CURSOR, 0, 0,
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorFromFileA (USER32.@)
 */
HCURSOR WINAPI LoadCursorFromFileA (LPCSTR name)
{
    TRACE("%s\n", debugstr_a(name));

    return LoadImageA( 0, name, IMAGE_CURSOR, 0, 0,
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadIconW (USER32.@)
 */
HICON WINAPI LoadIconW(HINSTANCE hInstance, LPCWSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(name));

    return LoadImageW( hInstance, name, IMAGE_ICON, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *              LoadIconA (USER32.@)
 */
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(name));

    return LoadImageA( hInstance, name, IMAGE_ICON, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/**********************************************************************
 *              GetIconInfo (USER32.@)
 */
BOOL WINAPI GetIconInfo(HICON hIcon, PICONINFO iconinfo)
{
    CURSORICONINFO *ciconinfo;
    INT height;

    ciconinfo = GlobalLock16(HICON_16(hIcon));
    if (!ciconinfo)
        return FALSE;

    TRACE("%p => %dx%d, %d bpp\n", hIcon,
          ciconinfo->nWidth, ciconinfo->nHeight, ciconinfo->bBitsPerPixel);

    if ( (ciconinfo->ptHotSpot.x == ICON_HOTSPOT) &&
         (ciconinfo->ptHotSpot.y == ICON_HOTSPOT) )
    {
      iconinfo->fIcon    = TRUE;
      iconinfo->xHotspot = ciconinfo->nWidth / 2;
      iconinfo->yHotspot = ciconinfo->nHeight / 2;
    }
    else
    {
      iconinfo->fIcon    = FALSE;
      iconinfo->xHotspot = ciconinfo->ptHotSpot.x;
      iconinfo->yHotspot = ciconinfo->ptHotSpot.y;
    }

    height = ciconinfo->nHeight;

    if (ciconinfo->bBitsPerPixel > 1)
    {
        iconinfo->hbmColor = CreateBitmap( ciconinfo->nWidth, ciconinfo->nHeight,
                                ciconinfo->bPlanes, ciconinfo->bBitsPerPixel,
                                (char *)(ciconinfo + 1)
                                + ciconinfo->nHeight *
                                get_bitmap_width_bytes (ciconinfo->nWidth,1) );
    }
    else
    {
        iconinfo->hbmColor = 0;
        height *= 2;
    }

    iconinfo->hbmMask = CreateBitmap ( ciconinfo->nWidth, height,
                                1, 1, ciconinfo + 1);

    GlobalUnlock16(HICON_16(hIcon));

    return TRUE;
}

/**********************************************************************
 *		CreateIconIndirect (USER32.@)
 */
HICON WINAPI CreateIconIndirect(PICONINFO iconinfo)
{
    DIBSECTION bmpXor;
    BITMAP bmpAnd;
    HICON16 hObj;
    int xor_objsize = 0, sizeXor = 0, sizeAnd, planes, bpp;

    TRACE("color %p, mask %p, hotspot %ux%u, fIcon %d\n",
           iconinfo->hbmColor, iconinfo->hbmMask,
           iconinfo->xHotspot, iconinfo->yHotspot, iconinfo->fIcon);

    if (!iconinfo->hbmMask) return 0;

    planes = GetDeviceCaps( screen_dc, PLANES );
    bpp = GetDeviceCaps( screen_dc, BITSPIXEL );

    if (iconinfo->hbmColor)
    {
        xor_objsize = GetObjectW( iconinfo->hbmColor, sizeof(bmpXor), &bmpXor );
        TRACE("color: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
               bmpXor.dsBm.bmWidth, bmpXor.dsBm.bmHeight, bmpXor.dsBm.bmWidthBytes,
               bmpXor.dsBm.bmPlanes, bmpXor.dsBm.bmBitsPixel);
        /* we can use either depth 1 or screen depth for xor bitmap */
        if (bmpXor.dsBm.bmPlanes == 1 && bmpXor.dsBm.bmBitsPixel == 1) planes = bpp = 1;
        sizeXor = bmpXor.dsBm.bmHeight * planes * get_bitmap_width_bytes( bmpXor.dsBm.bmWidth, bpp );
    }
    GetObjectW( iconinfo->hbmMask, sizeof(bmpAnd), &bmpAnd );
    TRACE("mask: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
           bmpAnd.bmWidth, bmpAnd.bmHeight, bmpAnd.bmWidthBytes,
           bmpAnd.bmPlanes, bmpAnd.bmBitsPixel);

    sizeAnd = bmpAnd.bmHeight * get_bitmap_width_bytes(bmpAnd.bmWidth, 1);

    hObj = GlobalAlloc16( GMEM_MOVEABLE,
                          sizeof(CURSORICONINFO) + sizeXor + sizeAnd );
    if (hObj)
    {
        CURSORICONINFO *info;

        info = GlobalLock16( hObj );

        /* If we are creating an icon, the hotspot is unused */
        if (iconinfo->fIcon)
        {
            info->ptHotSpot.x   = ICON_HOTSPOT;
            info->ptHotSpot.y   = ICON_HOTSPOT;
        }
        else
        {
            info->ptHotSpot.x   = iconinfo->xHotspot;
            info->ptHotSpot.y   = iconinfo->yHotspot;
        }

        if (iconinfo->hbmColor)
        {
            info->nWidth        = bmpXor.dsBm.bmWidth;
            info->nHeight       = bmpXor.dsBm.bmHeight;
            info->nWidthBytes   = bmpXor.dsBm.bmWidthBytes;
            info->bPlanes       = planes;
            info->bBitsPerPixel = bpp;
        }
        else
        {
            info->nWidth        = bmpAnd.bmWidth;
            info->nHeight       = bmpAnd.bmHeight / 2;
            info->nWidthBytes   = get_bitmap_width_bytes(bmpAnd.bmWidth, 1);
            info->bPlanes       = 1;
            info->bBitsPerPixel = 1;
        }

        /* Transfer the bitmap bits to the CURSORICONINFO structure */

        /* Some apps pass a color bitmap as a mask, convert it to b/w */
        if (bmpAnd.bmBitsPixel == 1)
        {
            GetBitmapBits( iconinfo->hbmMask, sizeAnd, info + 1 );
        }
        else
        {
            HDC hdc, hdc_mem;
            HBITMAP hbmp_old, hbmp_mem_old, hbmp_mono;

            hdc = GetDC( 0 );
            hdc_mem = CreateCompatibleDC( hdc );

            hbmp_mono = CreateBitmap( bmpAnd.bmWidth, bmpAnd.bmHeight, 1, 1, NULL );

            hbmp_old = SelectObject( hdc, iconinfo->hbmMask );
            hbmp_mem_old = SelectObject( hdc_mem, hbmp_mono );

            BitBlt( hdc_mem, 0, 0, bmpAnd.bmWidth, bmpAnd.bmHeight, hdc, 0, 0, SRCCOPY );

            SelectObject( hdc, hbmp_old );
            SelectObject( hdc_mem, hbmp_mem_old );

            DeleteDC( hdc_mem );
            ReleaseDC( 0, hdc );

            GetBitmapBits( hbmp_mono, sizeAnd, info + 1 );
            DeleteObject( hbmp_mono );
        }

        if (iconinfo->hbmColor)
        {
            char *dst_bits = (char*)(info + 1) + sizeAnd;

            if (bmpXor.dsBm.bmPlanes == planes && bmpXor.dsBm.bmBitsPixel == bpp)
                GetBitmapBits( iconinfo->hbmColor, sizeXor, dst_bits );
            else
            {
                BITMAPINFO bminfo;
                int dib_width = get_dib_width_bytes( info->nWidth, info->bBitsPerPixel );
                int bitmap_width = get_bitmap_width_bytes( info->nWidth, info->bBitsPerPixel );

                bminfo.bmiHeader.biSize = sizeof(bminfo);
                bminfo.bmiHeader.biWidth = info->nWidth;
                bminfo.bmiHeader.biHeight = info->nHeight;
                bminfo.bmiHeader.biPlanes = info->bPlanes;
                bminfo.bmiHeader.biBitCount = info->bBitsPerPixel;
                bminfo.bmiHeader.biCompression = BI_RGB;
                bminfo.bmiHeader.biSizeImage = info->nHeight * dib_width;
                bminfo.bmiHeader.biXPelsPerMeter = 0;
                bminfo.bmiHeader.biYPelsPerMeter = 0;
                bminfo.bmiHeader.biClrUsed = 0;
                bminfo.bmiHeader.biClrImportant = 0;

                /* swap lines for dib sections */
                if (xor_objsize == sizeof(DIBSECTION))
                    bminfo.bmiHeader.biHeight = -bminfo.bmiHeader.biHeight;

                if (dib_width != bitmap_width)  /* need to fixup alignment */
                {
                    char *src_bits = HeapAlloc( GetProcessHeap(), 0, bminfo.bmiHeader.biSizeImage );

                    if (src_bits && GetDIBits( screen_dc, iconinfo->hbmColor, 0, info->nHeight,
                                               src_bits, &bminfo, DIB_RGB_COLORS ))
                    {
                        int y;
                        for (y = 0; y < info->nHeight; y++)
                            memcpy( dst_bits + y * bitmap_width, src_bits + y * dib_width, bitmap_width );
                    }
                    HeapFree( GetProcessHeap(), 0, src_bits );
                }
                else
                    GetDIBits( screen_dc, iconinfo->hbmColor, 0, info->nHeight,
                               dst_bits, &bminfo, DIB_RGB_COLORS );
            }
        }
        GlobalUnlock16( hObj );
    }
    return HICON_32(hObj);
}

/******************************************************************************
 *		DrawIconEx (USER32.@) Draws an icon or cursor on device context
 *
 * NOTES
 *    Why is this using SM_CXICON instead of SM_CXCURSOR?
 *
 * PARAMS
 *    hdc     [I] Handle to device context
 *    x0      [I] X coordinate of upper left corner
 *    y0      [I] Y coordinate of upper left corner
 *    hIcon   [I] Handle to icon to draw
 *    cxWidth [I] Width of icon
 *    cyWidth [I] Height of icon
 *    istep   [I] Index of frame in animated cursor
 *    hbr     [I] Handle to background brush
 *    flags   [I] Icon-drawing flags
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DrawIconEx( HDC hdc, INT x0, INT y0, HICON hIcon,
                            INT cxWidth, INT cyWidth, UINT istep,
                            HBRUSH hbr, UINT flags )
{
    CURSORICONINFO *ptr;
    HDC hDC_off = 0, hMemDC;
    BOOL result = FALSE, DoOffscreen;
    HBITMAP hB_off = 0, hOld = 0;
    unsigned char *xorBitmapBits;
    unsigned int xorLength;
    BOOL has_alpha = FALSE;

    TRACE_(icon)("(hdc=%p,pos=%d.%d,hicon=%p,extend=%d.%d,istep=%d,br=%p,flags=0x%08x)\n",
                 hdc,x0,y0,hIcon,cxWidth,cyWidth,istep,hbr,flags );

    if (!(ptr = GlobalLock16(HICON_16(hIcon)))) return FALSE;
    if (!(hMemDC = CreateCompatibleDC( hdc ))) return FALSE;

    if (istep)
        FIXME_(icon)("Ignoring istep=%d\n", istep);
    if (flags & DI_NOMIRROR)
        FIXME_(icon)("Ignoring flag DI_NOMIRROR\n");

    xorLength = ptr->nHeight * get_bitmap_width_bytes(
        ptr->nWidth, ptr->bBitsPerPixel);
    xorBitmapBits = (unsigned char *)(ptr + 1) + ptr->nHeight *
                    get_bitmap_width_bytes(ptr->nWidth, 1);

    if (flags & DI_IMAGE)
        has_alpha = bitmap_has_alpha_channel(
            ptr->bBitsPerPixel, xorBitmapBits, xorLength);

    /* Calculate the size of the destination image.  */
    if (cxWidth == 0)
    {
        if (flags & DI_DEFAULTSIZE)
            cxWidth = GetSystemMetrics (SM_CXICON);
        else
            cxWidth = ptr->nWidth;
    }
    if (cyWidth == 0)
    {
        if (flags & DI_DEFAULTSIZE)
            cyWidth = GetSystemMetrics (SM_CYICON);
        else
            cyWidth = ptr->nHeight;
    }

    DoOffscreen = (GetObjectType( hbr ) == OBJ_BRUSH);

    if (DoOffscreen) {
        RECT r;

        r.left = 0;
        r.top = 0;
        r.right = cxWidth;
        r.bottom = cxWidth;

        hDC_off = CreateCompatibleDC(hdc);
        hB_off = CreateCompatibleBitmap(hdc, cxWidth, cyWidth);
        if (hDC_off && hB_off) {
            hOld = SelectObject(hDC_off, hB_off);
            FillRect(hDC_off, &r, hbr);
        }
    }

    if (hMemDC && (!DoOffscreen || (hDC_off && hB_off)))
    {
        HBITMAP hBitTemp;
        HBITMAP hXorBits = NULL, hAndBits = NULL;
        COLORREF  oldFg, oldBg;
        INT     nStretchMode;

        nStretchMode = SetStretchBltMode (hdc, STRETCH_DELETESCANS);

        oldFg = SetTextColor( hdc, RGB(0,0,0) );
        oldBg = SetBkColor( hdc, RGB(255,255,255) );

        if (((flags & DI_MASK) && !(flags & DI_IMAGE)) ||
            ((flags & DI_MASK) && !has_alpha))
        {
            hAndBits = CreateBitmap ( ptr->nWidth, ptr->nHeight, 1, 1, ptr + 1 );
            if (hAndBits)
            {
                hBitTemp = SelectObject( hMemDC, hAndBits );
                if (DoOffscreen)
                    StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
                                hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCAND);
                else
                    StretchBlt (hdc, x0, y0, cxWidth, cyWidth,
                                hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCAND);
                SelectObject( hMemDC, hBitTemp );
            }
        }

        if (flags & DI_IMAGE)
        {
            BITMAPINFOHEADER bmih;
            unsigned char *dibBits;

            memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
            bmih.biSize = sizeof(BITMAPINFOHEADER);
            bmih.biWidth = ptr->nWidth;
            bmih.biHeight = -ptr->nHeight;
            bmih.biPlanes = ptr->bPlanes;
            bmih.biBitCount = ptr->bBitsPerPixel;
            bmih.biCompression = BI_RGB;

            hXorBits = CreateDIBSection(hdc, (BITMAPINFO*)&bmih, DIB_RGB_COLORS,
                                        (void*)&dibBits, NULL, 0);

            if (hXorBits && dibBits)
            {
                if(has_alpha)
                {
                    BLENDFUNCTION pixelblend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

                    /* Do the alpha blending render */
                    premultiply_alpha_channel(dibBits, xorBitmapBits, xorLength);
                    hBitTemp = SelectObject( hMemDC, hXorBits );

                    if (DoOffscreen)
                        GdiAlphaBlend(hDC_off, 0, 0, cxWidth, cyWidth, hMemDC,
                                        0, 0, ptr->nWidth, ptr->nHeight, pixelblend);
                    else
                        GdiAlphaBlend(hdc, x0, y0, cxWidth, cyWidth, hMemDC,
                                        0, 0, ptr->nWidth, ptr->nHeight, pixelblend);

                    SelectObject( hMemDC, hBitTemp );
                }
                else
                {
                    memcpy(dibBits, xorBitmapBits, xorLength);
                    hBitTemp = SelectObject( hMemDC, hXorBits );
                    if (DoOffscreen)
                        StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
                                    hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCPAINT);
                    else
                        StretchBlt (hdc, x0, y0, cxWidth, cyWidth,
                                    hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCPAINT);
                    SelectObject( hMemDC, hBitTemp );
                }

                DeleteObject( hXorBits );
            }
        }

        result = TRUE;

        SetTextColor( hdc, oldFg );
        SetBkColor( hdc, oldBg );

        if (hAndBits) DeleteObject( hAndBits );
        SetStretchBltMode (hdc, nStretchMode);
        if (DoOffscreen) {
            BitBlt(hdc, x0, y0, cxWidth, cyWidth, hDC_off, 0, 0, SRCCOPY);
            SelectObject(hDC_off, hOld);
        }
    }
    if (hMemDC) DeleteDC( hMemDC );
    if (hDC_off) DeleteDC(hDC_off);
    if (hB_off) DeleteObject(hB_off);
    GlobalUnlock16(HICON_16(hIcon));
    return result;
}

/***********************************************************************
 *           DIB_FixColorsToLoadflags
 *
 * Change color table entries when LR_LOADTRANSPARENT or LR_LOADMAP3DCOLORS
 * are in loadflags
 */
static void DIB_FixColorsToLoadflags(BITMAPINFO * bmi, UINT loadflags, BYTE pix)
{
    int colors;
    COLORREF c_W, c_S, c_F, c_L, c_C;
    int incr,i;
    RGBQUAD *ptr;
    int bitmap_type;
    LONG width;
    LONG height;
    WORD bpp;
    DWORD compr;

    if (((bitmap_type = DIB_GetBitmapInfo((BITMAPINFOHEADER*) bmi, &width, &height, &bpp, &compr)) == -1))
    {
        WARN_(resource)("Invalid bitmap\n");
        return;
    }

    if (bpp > 8) return;

    if (bitmap_type == 0) /* BITMAPCOREHEADER */
    {
        incr = 3;
        colors = 1 << bpp;
    }
    else
    {
        incr = 4;
        colors = bmi->bmiHeader.biClrUsed;
        if (colors > 256) colors = 256;
        if (!colors && (bpp <= 8)) colors = 1 << bpp;
    }

    c_W = GetSysColor(COLOR_WINDOW);
    c_S = GetSysColor(COLOR_3DSHADOW);
    c_F = GetSysColor(COLOR_3DFACE);
    c_L = GetSysColor(COLOR_3DLIGHT);

    if (loadflags & LR_LOADTRANSPARENT) {
        switch (bpp) {
        case 1: pix = pix >> 7; break;
        case 4: pix = pix >> 4; break;
        case 8: break;
        default:
            WARN_(resource)("(%d): Unsupported depth\n", bpp);
            return;
        }
        if (pix >= colors) {
            WARN_(resource)("pixel has color index greater than biClrUsed!\n");
            return;
        }
        if (loadflags & LR_LOADMAP3DCOLORS) c_W = c_F;
        ptr = (RGBQUAD*)((char*)bmi->bmiColors+pix*incr);
        ptr->rgbBlue = GetBValue(c_W);
        ptr->rgbGreen = GetGValue(c_W);
        ptr->rgbRed = GetRValue(c_W);
    }
    if (loadflags & LR_LOADMAP3DCOLORS)
        for (i=0; i<colors; i++) {
            ptr = (RGBQUAD*)((char*)bmi->bmiColors+i*incr);
            c_C = RGB(ptr->rgbRed, ptr->rgbGreen, ptr->rgbBlue);
            if (c_C == RGB(128, 128, 128)) {
                ptr->rgbRed = GetRValue(c_S);
                ptr->rgbGreen = GetGValue(c_S);
                ptr->rgbBlue = GetBValue(c_S);
            } else if (c_C == RGB(192, 192, 192)) {
                ptr->rgbRed = GetRValue(c_F);
                ptr->rgbGreen = GetGValue(c_F);
                ptr->rgbBlue = GetBValue(c_F);
            } else if (c_C == RGB(223, 223, 223)) {
                ptr->rgbRed = GetRValue(c_L);
                ptr->rgbGreen = GetGValue(c_L);
                ptr->rgbBlue = GetBValue(c_L);
            }
        }
}


/**********************************************************************
 *       BITMAP_Load
 */
static HBITMAP BITMAP_Load( HINSTANCE instance, LPCWSTR name,
                            INT desiredx, INT desiredy, UINT loadflags )
{
    HBITMAP hbitmap = 0, orig_bm;
    HRSRC hRsrc;
    HGLOBAL handle;
    char *ptr = NULL;
    BITMAPINFO *info, *fix_info = NULL, *scaled_info = NULL;
    int size;
    BYTE pix;
    char *bits;
    LONG width, height, new_width, new_height;
    WORD bpp_dummy;
    DWORD compr_dummy;
    INT bm_type;
    HDC screen_mem_dc = NULL;

    if (!(loadflags & LR_LOADFROMFILE))
    {
        if (!instance)
        {
            /* OEM bitmap: try to load the resource from user32.dll */
            instance = user32_module;
        }

        if (!(hRsrc = FindResourceW( instance, name, (LPWSTR)RT_BITMAP ))) return 0;
        if (!(handle = LoadResource( instance, hRsrc ))) return 0;

        if ((info = LockResource( handle )) == NULL) return 0;
    }
    else
    {
        BITMAPFILEHEADER * bmfh;

        if (!(ptr = map_fileW( name, NULL ))) return 0;
        info = (BITMAPINFO *)(ptr + sizeof(BITMAPFILEHEADER));
        bmfh = (BITMAPFILEHEADER *)ptr;
        if (!(  bmfh->bfType == 0x4d42 /* 'BM' */ &&
                bmfh->bfReserved1 == 0 &&
                bmfh->bfReserved2 == 0))
        {
            WARN("Invalid/unsupported bitmap format!\n");
            UnmapViewOfFile( ptr );
            return 0;
        }
    }

    size = bitmap_info_size(info, DIB_RGB_COLORS);
    fix_info = HeapAlloc(GetProcessHeap(), 0, size);
    scaled_info = HeapAlloc(GetProcessHeap(), 0, size);

    if (!fix_info || !scaled_info) goto end;
    memcpy(fix_info, info, size);

    pix = *((LPBYTE)info + size);
    DIB_FixColorsToLoadflags(fix_info, loadflags, pix);

    memcpy(scaled_info, fix_info, size);
    bm_type = DIB_GetBitmapInfo( &fix_info->bmiHeader, &width, &height,
                                 &bpp_dummy, &compr_dummy);
    if(desiredx != 0)
        new_width = desiredx;
    else
        new_width = width;

    if(desiredy != 0)
        new_height = height > 0 ? desiredy : -desiredy;
    else
        new_height = height;

    if(bm_type == 0)
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)&scaled_info->bmiHeader;
        core->bcWidth = new_width;
        core->bcHeight = new_height;
    }
    else
    {
        scaled_info->bmiHeader.biWidth = new_width;
        scaled_info->bmiHeader.biHeight = new_height;
    }

    if (new_height < 0) new_height = -new_height;

    if (!screen_dc) screen_dc = CreateDCW( DISPLAYW, NULL, NULL, NULL );
    if (!(screen_mem_dc = CreateCompatibleDC( screen_dc ))) goto end;

    bits = (char *)info + size;

    if (loadflags & LR_CREATEDIBSECTION)
    {
        scaled_info->bmiHeader.biCompression = 0; /* DIBSection can't be compressed */
        hbitmap = CreateDIBSection(screen_dc, scaled_info, DIB_RGB_COLORS, NULL, 0, 0);
    }
    else
    {
        if (is_dib_monochrome(fix_info))
            hbitmap = CreateBitmap(new_width, new_height, 1, 1, NULL);
        else
            hbitmap = CreateCompatibleBitmap(screen_dc, new_width, new_height);        
    }

    orig_bm = SelectObject(screen_mem_dc, hbitmap);
    StretchDIBits(screen_mem_dc, 0, 0, new_width, new_height, 0, 0, width, height, bits, fix_info, DIB_RGB_COLORS, SRCCOPY);
    SelectObject(screen_mem_dc, orig_bm);

end:
    if (screen_mem_dc) DeleteDC(screen_mem_dc);
    HeapFree(GetProcessHeap(), 0, scaled_info);
    HeapFree(GetProcessHeap(), 0, fix_info);
    if (loadflags & LR_LOADFROMFILE) UnmapViewOfFile( ptr );

    return hbitmap;
}

/**********************************************************************
 *		LoadImageA (USER32.@)
 *
 * See LoadImageW.
 */
HANDLE WINAPI LoadImageA( HINSTANCE hinst, LPCSTR name, UINT type,
                              INT desiredx, INT desiredy, UINT loadflags)
{
    HANDLE res;
    LPWSTR u_name;

    if (!HIWORD(name))
        return LoadImageW(hinst, (LPCWSTR)name, type, desiredx, desiredy, loadflags);

    __TRY {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
        u_name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, name, -1, u_name, len );
    }
    __EXCEPT_PAGE_FAULT {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    res = LoadImageW(hinst, u_name, type, desiredx, desiredy, loadflags);
    HeapFree(GetProcessHeap(), 0, u_name);
    return res;
}


/******************************************************************************
 *		LoadImageW (USER32.@) Loads an icon, cursor, or bitmap
 *
 * PARAMS
 *    hinst     [I] Handle of instance that contains image
 *    name      [I] Name of image
 *    type      [I] Type of image
 *    desiredx  [I] Desired width
 *    desiredy  [I] Desired height
 *    loadflags [I] Load flags
 *
 * RETURNS
 *    Success: Handle to newly loaded image
 *    Failure: NULL
 *
 * FIXME: Implementation lacks some features, see LR_ defines in winuser.h
 */
HANDLE WINAPI LoadImageW( HINSTANCE hinst, LPCWSTR name, UINT type,
                INT desiredx, INT desiredy, UINT loadflags )
{
    TRACE_(resource)("(%p,%s,%d,%d,%d,0x%08x)\n",
                     hinst,debugstr_w(name),type,desiredx,desiredy,loadflags);

    if (loadflags & LR_DEFAULTSIZE) {
        if (type == IMAGE_ICON) {
            if (!desiredx) desiredx = GetSystemMetrics(SM_CXICON);
            if (!desiredy) desiredy = GetSystemMetrics(SM_CYICON);
        } else if (type == IMAGE_CURSOR) {
            if (!desiredx) desiredx = GetSystemMetrics(SM_CXCURSOR);
            if (!desiredy) desiredy = GetSystemMetrics(SM_CYCURSOR);
        }
    }
    if (loadflags & LR_LOADFROMFILE) loadflags &= ~LR_SHARED;
    switch (type) {
    case IMAGE_BITMAP:
        return BITMAP_Load( hinst, name, desiredx, desiredy, loadflags );

    case IMAGE_ICON:
        if (!screen_dc) screen_dc = CreateDCW( DISPLAYW, NULL, NULL, NULL );
        if (screen_dc)
        {
            UINT palEnts = GetSystemPaletteEntries(screen_dc, 0, 0, NULL);
            if (palEnts == 0) palEnts = 256;
            return CURSORICON_Load(hinst, name, desiredx, desiredy,
                                   palEnts, FALSE, loadflags);
        }
        break;

    case IMAGE_CURSOR:
        return CURSORICON_Load(hinst, name, desiredx, desiredy,
                               1, TRUE, loadflags);
    }
    return 0;
}

/******************************************************************************
 *		CopyImage (USER32.@) Creates new image and copies attributes to it
 *
 * PARAMS
 *    hnd      [I] Handle to image to copy
 *    type     [I] Type of image to copy
 *    desiredx [I] Desired width of new image
 *    desiredy [I] Desired height of new image
 *    flags    [I] Copy flags
 *
 * RETURNS
 *    Success: Handle to newly created image
 *    Failure: NULL
 *
 * BUGS
 *    Only Windows NT 4.0 supports the LR_COPYRETURNORG flag for bitmaps,
 *    all other versions (95/2000/XP have been tested) ignore it.
 *
 * NOTES
 *    If LR_CREATEDIBSECTION is absent, the copy will be monochrome for
 *    a monochrome source bitmap or if LR_MONOCHROME is present, otherwise
 *    the copy will have the same depth as the screen.
 *    The content of the image will only be copied if the bit depth of the
 *    original image is compatible with the bit depth of the screen, or
 *    if the source is a DIB section.
 *    The LR_MONOCHROME flag is ignored if LR_CREATEDIBSECTION is present.
 */
HANDLE WINAPI CopyImage( HANDLE hnd, UINT type, INT desiredx,
                             INT desiredy, UINT flags )
{
    TRACE("hnd=%p, type=%u, desiredx=%d, desiredy=%d, flags=%x\n",
          hnd, type, desiredx, desiredy, flags);

    switch (type)
    {
        case IMAGE_BITMAP:
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

                /* Get the color table or the color masks */
                GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);

                bi->bmiHeader.biWidth  = desiredx;
                bi->bmiHeader.biHeight = desiredy;
                bi->bmiHeader.biSizeImage = 0;

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
                    bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
                    bi->bmiHeader.biBitCount = ds.dsBm.bmBitsPixel;
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
        case IMAGE_ICON:
                return CURSORICON_ExtCopy(hnd,type, desiredx, desiredy, flags);
        case IMAGE_CURSOR:
                /* Should call CURSORICON_ExtCopy but more testing
                 * needs to be done before we change this
                 */
                if (flags) FIXME("Flags are ignored\n");
                return CopyCursor(hnd);
    }
    return 0;
}


/******************************************************************************
 *		LoadBitmapW (USER32.@) Loads bitmap from the executable file
 *
 * RETURNS
 *    Success: Handle to specified bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI LoadBitmapW(
    HINSTANCE instance, /* [in] Handle to application instance */
    LPCWSTR name)         /* [in] Address of bitmap resource name */
{
    return LoadImageW( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}

/**********************************************************************
 *		LoadBitmapA (USER32.@)
 *
 * See LoadBitmapW.
 */
HBITMAP WINAPI LoadBitmapA( HINSTANCE instance, LPCSTR name )
{
    return LoadImageA( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}
