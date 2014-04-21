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

#include <user32.h>

#include <wine/debug.h>

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

static HDC screen_dc;

static const WCHAR DISPLAYW[] = {'D','I','S','P','L','A','Y',0};


static CRITICAL_SECTION IconCrst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &IconCrst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": IconCrst") }
};
static CRITICAL_SECTION IconCrst = { &critsect_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 *				CreateCursorIconHandle
 *
 * Creates a handle with everything in there
 */
static
HICON
CreateCursorIconHandle( PICONINFO IconInfo )
{
	HICON hIcon = NtUserxCreateEmptyCurObject(0);
	if(!hIcon)
		return NULL;

	NtUserSetCursorContents(hIcon, IconInfo);
	return hIcon;
}



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
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
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
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, DWORD *compr )
{
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
    ERR("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}


/*
 *  The following macro functions account for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL (*fnGetCIEntry)( LPCVOID dir, DWORD size, int n,
                              int *width, int *height, int *bits );

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and bit depth.
 */
static int CURSORICON_FindBestIcon( LPCVOID dir, DWORD size, fnGetCIEntry get_entry,
                                    int width, int height, int depth )
{
    int i, cx, cy, bits, bestEntry = -1;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
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
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
    {
        if(abs(width - cx) == iXDiff && abs(height - cy) == iYDiff)
        {
            iTempColorDiff = abs(depth - bits);
            if(iColorDiff > iTempColorDiff)
            {
                bestEntry = i;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    return bestEntry;
}

static BOOL CURSORICON_GetResIconEntry( LPCVOID dir, DWORD size, int n,
                                        int *width, int *height, int *bits )
{
    const CURSORICONDIR *resdir = dir;
    const ICONRESDIR *icon;

    if ( resdir->idCount <= n )
        return FALSE;
    if ((const char *)&resdir->idEntries[n + 1] - (const char *)dir > size)
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
static int CURSORICON_FindBestCursor( LPCVOID dir, DWORD size, fnGetCIEntry get_entry,
                                      int width, int height, int depth )
{
    int i, maxwidth, maxheight, cx, cy, bits, bestEntry = -1;

    /* Double height to account for AND and XOR masks */

    height *= 2;

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = 0;
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
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
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
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

static BOOL CURSORICON_GetResCursorEntry( LPCVOID dir, DWORD size, int n,
                                          int *width, int *height, int *bits )
{
    const CURSORICONDIR *resdir = dir;
    const CURSORRESDIR *cursor;

    if ( resdir->idCount <= n )
        return FALSE;
    if ((const char *)&resdir->idEntries[n + 1] - (const char *)dir > size)
        return FALSE;
    cursor = &resdir->idEntries[n].ResInfo.cursor;
    *width = cursor->wWidth;
    *height = cursor->wHeight;
    *bits = resdir->idEntries[n].wBitCount;
    return TRUE;
}

static CURSORICONDIRENTRY *CURSORICON_FindBestIconRes( CURSORICONDIR * dir, DWORD size,
                                                       int width, int height, int depth )
{
    int n;

    n = CURSORICON_FindBestIcon( dir, size, CURSORICON_GetResIconEntry,
                                 width, height, depth );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static CURSORICONDIRENTRY *CURSORICON_FindBestCursorRes( CURSORICONDIR *dir, DWORD size,
                                                         int width, int height, int depth )
{
    int n = CURSORICON_FindBestCursor( dir, size, CURSORICON_GetResCursorEntry,
                                   width, height, depth );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static BOOL CURSORICON_GetFileEntry( LPCVOID dir, DWORD size, int n,
                                     int *width, int *height, int *bits )
{
    const CURSORICONFILEDIR *filedir = dir;
    const CURSORICONFILEDIRENTRY *entry;
    const BITMAPINFOHEADER *info;

    if ( filedir->idCount <= n )
        return FALSE;
    if ((const char *)&filedir->idEntries[n + 1] - (const char *)dir > size)
        return FALSE;
    entry = &filedir->idEntries[n];
    info = (const BITMAPINFOHEADER *)((const char *)dir + entry->dwDIBOffset);
    if ((const char *)(info + 1) - (const char *)dir > size) return FALSE;
    *width = entry->bWidth;
    *height = entry->bHeight;
    *bits = info->biBitCount;
    return TRUE;
}

static CURSORICONFILEDIRENTRY *CURSORICON_FindBestCursorFile( CURSORICONFILEDIR *dir, DWORD size,
                                                              int width, int height, int depth )
{
    int n = CURSORICON_FindBestCursor( dir, size, CURSORICON_GetFileEntry,
                                       width, height, depth );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static CURSORICONFILEDIRENTRY *CURSORICON_FindBestIconFile( CURSORICONFILEDIR *dir, DWORD size,
                                                            int width, int height, int depth )
{
    int n = CURSORICON_FindBestIcon( dir, size, CURSORICON_GetFileEntry,
                                     width, height, depth );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

/***********************************************************************
 *          create_icon_bitmaps
 *
 * Create the color, mask and alpha bitmaps from the DIB info.
 */
static BOOL create_icon_bitmaps( const BITMAPINFO *bmi, int width, int height,
                                 HBITMAP *color, HBITMAP *mask )
{
    BOOL monochrome = is_dib_monochrome( bmi );
    unsigned int size = bitmap_info_size( bmi, DIB_RGB_COLORS );
    BITMAPINFO *info;
    void *color_bits, *mask_bits;
    BOOL ret = FALSE;
    HDC hdc = 0;

    if (!(info = HeapAlloc( GetProcessHeap(), 0, max( size, FIELD_OFFSET( BITMAPINFO, bmiColors[2] )))))
        return FALSE;
    if (!(hdc = CreateCompatibleDC(NULL))) goto done;

    memcpy( info, bmi, size );
    info->bmiHeader.biHeight /= 2;

    color_bits = (char *)bmi + size;
    mask_bits = (char *)color_bits +
        get_dib_width_bytes( bmi->bmiHeader.biWidth,
                             bmi->bmiHeader.biBitCount ) * abs(info->bmiHeader.biHeight);

    if (monochrome)
    {
        if (!(*mask = CreateBitmap( width, height * 2, 1, 1, NULL ))) goto done;
        *color = 0;

        /* copy color data into second half of mask bitmap */
        SelectObject( hdc, *mask );
        StretchDIBits( hdc, 0, height, width, height,
                       0, 0, info->bmiHeader.biWidth, info->bmiHeader.biHeight,
                       color_bits, info, DIB_RGB_COLORS, SRCCOPY );
    }
    else
    {
        if (!(*mask = CreateBitmap( width, height, 1, 1, NULL ))) goto done;
        if (!(*color = CreateBitmap( width, height, GetDeviceCaps(hdc, PLANES),
                                     GetDeviceCaps(hdc, BITSPIXEL), NULL )))
        {
            DeleteObject( *mask );
            goto done;
        }
        SelectObject( hdc, *color );
        StretchDIBits( hdc, 0, 0, width, height,
                       0, 0, info->bmiHeader.biWidth, info->bmiHeader.biHeight,
                       color_bits, info, DIB_RGB_COLORS, SRCCOPY );

        /* convert info to monochrome to copy the mask */
        info->bmiHeader.biBitCount = 1;
        if (info->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            RGBQUAD *rgb = info->bmiColors;

            info->bmiHeader.biClrUsed = info->bmiHeader.biClrImportant = 2;
            rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
            rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
            rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
        }
        else
        {
            RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)info) + 1);

            rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
            rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
        }
    }

    SelectObject( hdc, *mask );
    StretchDIBits( hdc, 0, 0, width, height,
                   0, 0, info->bmiHeader.biWidth, info->bmiHeader.biHeight,
                   mask_bits, info, DIB_RGB_COLORS, SRCCOPY );
    ret = TRUE;

done:
    if(hdc) DeleteDC( hdc );
    HeapFree( GetProcessHeap(), 0, info );
    return ret;
}

static HICON CURSORICON_CreateIconFromBMI( BITMAPINFO *bmi,
					   POINT hotspot, BOOL bIcon,
					   DWORD dwVersion,
					   INT width, INT height,
					   UINT cFlag )
{
    HBITMAP color = 0, mask = 0;
    BOOL do_stretch;
	ICONINFO IconInfo;

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

    if (!width) width = bmi->bmiHeader.biWidth;
    if (!height) height = bmi->bmiHeader.biHeight/2;
    do_stretch = (bmi->bmiHeader.biHeight/2 != height) ||
                 (bmi->bmiHeader.biWidth != width);

    /* Scale the hotspot */
    if (bIcon)
    {
        hotspot.x = width / 2;
        hotspot.y = height / 2;
    }
    else if (do_stretch)
    {
        hotspot.x = (hotspot.x * width) / bmi->bmiHeader.biWidth;
        hotspot.y = (hotspot.y * height) / (bmi->bmiHeader.biHeight / 2);
    }

    if (!screen_dc) screen_dc = CreateDCW( DISPLAYW, NULL, NULL, NULL );
    if (!screen_dc) return 0;

    if (!create_icon_bitmaps( bmi, width, height, &color, &mask )) return 0;

	IconInfo.xHotspot = hotspot.x;
	IconInfo.yHotspot = hotspot.y;
	IconInfo.fIcon = bIcon;
	IconInfo.hbmColor = color;
	IconInfo.hbmMask = mask;

	return CreateCursorIconHandle(&IconInfo);
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
    INT width, INT height, INT depth )
{
    HCURSOR cursor;
    ani_header header = {0};
    LPBYTE frame_bits = 0;
    POINT hotspot;
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
                                          bits + bits_size - icon_data,
                                          width, height, depth );

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
HICON WINAPI CreateIconFromResourceEx( PBYTE bits, DWORD cbSize,
                                       BOOL bIcon, DWORD dwVersion,
                                       int width, int height,
                                       UINT cFlag )
{
    POINT hotspot;
    BITMAPINFO *bmi;

    TRACE_(cursor)("%p (%u bytes), ver %08x, %ix%i %s %s\n",
                   bits, cbSize, dwVersion, width, height,
                   bIcon ? "icon" : "cursor", (cFlag & LR_MONOCHROME) ? "mono" : "" );

    if (bIcon)
    {
        hotspot.x = width / 2;
        hotspot.y = height / 2;
        bmi = (BITMAPINFO *)bits;
    }
    else /* get the hotspot */
    {
        SHORT *pt = (SHORT *)bits;
        hotspot.x = pt[0];
        hotspot.y = pt[1];
        bmi = (BITMAPINFO *)(pt + 2);
    }

    return CURSORICON_CreateIconFromBMI( bmi, hotspot, bIcon, dwVersion,
					 width, height, cFlag );
}


/**********************************************************************
 *		CreateIconFromResource (USER32.@)
 */
HICON WINAPI CreateIconFromResource( PBYTE bits, DWORD cbSize,
                                           BOOL bIcon, DWORD dwVersion)
{
    return CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, 0,0,0);
}


static HICON CURSORICON_LoadFromFile( LPCWSTR filename,
                             INT width, INT height, INT depth,
                             BOOL fCursor, UINT loadflags)
{
    CURSORICONFILEDIRENTRY *entry;
    CURSORICONFILEDIR *dir;
    DWORD filesize = 0;
    HICON hIcon = 0;
    LPBYTE bits;
    POINT hotspot;

    TRACE("loading %s\n", debugstr_w( filename ));

    bits = map_fileW( filename, &filesize );
    if (!bits)
        return hIcon;

    /* Check for .ani. */
    if (memcmp( bits, "RIFF", 4 ) == 0)
    {
        hIcon = CURSORICON_CreateIconFromANI( bits, filesize, width, height,
            depth );
        goto end;
    }

    dir = (CURSORICONFILEDIR*) bits;
    if ( filesize < sizeof(*dir) )
        goto end;

    if ( filesize < (sizeof(*dir) + sizeof(dir->idEntries[0])*(dir->idCount-1)) )
        goto end;

    if ( fCursor )
        entry = CURSORICON_FindBestCursorFile( dir, filesize, width, height, depth );
    else
        entry = CURSORICON_FindBestIconFile( dir, filesize, width, height, depth );

    if ( !entry )
        goto end;

    /* check that we don't run off the end of the file */
    if ( entry->dwDIBOffset > filesize )
        goto end;
    if ( entry->dwDIBOffset + entry->dwDIBSize > filesize )
        goto end;

    hotspot.x = entry->xHotspot;
    hotspot.y = entry->yHotspot;
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
                             INT width, INT height, INT depth,
                             BOOL fCursor, UINT loadflags)
{
    HANDLE handle = 0;
    HICON hIcon = 0;
    HRSRC hRsrc;
    //HRSRC hGroupRsrc;
    DWORD size;
    CURSORICONDIR *dir;
    CURSORICONDIRENTRY *dirEntry;
    LPBYTE bits;
    WORD wResId;
    DWORD dwBytesInRes;

    TRACE("%p, %s, %dx%d, depth %d, fCursor %d, flags 0x%04x\n",
          hInstance, debugstr_w(name), width, height, depth, fCursor, loadflags);

    if ( loadflags & LR_LOADFROMFILE )    /* Load from file */
        return CURSORICON_LoadFromFile( name, width, height, depth, fCursor, loadflags );

    if (!hInstance) hInstance = User32Instance;  /* Load OEM cursor/icon */

    /* don't cache 16-bit instances (FIXME: should never get 16-bit instances in the first place) */
    if ((ULONG_PTR)hInstance >> 16 == 0) loadflags &= ~LR_SHARED;

    /* Get directory resource ID */

    if (!(hRsrc = FindResourceW( hInstance, name,
                                 (LPWSTR)(fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON) )))
        return 0;
    //hGroupRsrc = hRsrc;

    /* Find the best entry in the directory */

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    if (!(dir = LockResource( handle ))) return 0;
    size = SizeofResource( hInstance, hRsrc );
    if (fCursor)
        dirEntry = CURSORICON_FindBestCursorRes( dir, size, width, height, depth );
    else
        dirEntry = CURSORICON_FindBestIconRes( dir, size, width, height, depth );
    if (!dirEntry) return 0;
    wResId = dirEntry->wResId;
    dwBytesInRes = dirEntry->dwBytesInRes;
    FreeResource( handle );

    /* Load the resource */

    if (!(hRsrc = FindResourceW(hInstance,MAKEINTRESOURCEW(wResId),
                                (LPWSTR)(fCursor ? RT_CURSOR : RT_ICON) ))) return 0;

    /* If shared icon, check whether it was already loaded */
    if (    (loadflags & LR_SHARED)
         && (hIcon = NtUserFindExistingCursorIcon( hInstance, hRsrc, 0, 0 ) ) != 0 )
        return hIcon;

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    bits = LockResource( handle );
    hIcon = CreateIconFromResourceEx( bits, dwBytesInRes,
                                      !fCursor, 0x00030000, width, height, loadflags);
    FreeResource( handle );

    /* If shared icon, add to icon cache */

      if (hIcon && 0 != (loadflags & LR_SHARED))
      {
#if 1
         NtUserSetCursorIconData((HICON)hIcon, NULL, NULL, hInstance, hRsrc,
                                 (HRSRC)NULL);
#else
         ICONINFO iconInfo;

         if(NtUserGetIconInfo(ResIcon, &iconInfo, NULL, NULL, NULL, FALSE))
            NtUserSetCursorIconData((HICON)hIcon, hinst, NULL, &iconInfo);
#endif
	  }

    return hIcon;
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
		TRACE("Copying from resource isn't implemented yet\n");
		hNew = CopyIcon(hIcon);

#if 0
        ICONCACHE* pIconCache = CURSORICON_FindCache(hIcon);

        /* Not Found in Cache, then do a straight copy
        */
        if(pIconCache == NULL)
        {
            hNew = CopyIcon( hIcon );
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
#endif
    }
    else hNew = CopyIcon( hIcon );
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
    ICONINFO info;
    HCURSOR hCursor;

    TRACE_(cursor)("%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);

    info.fIcon = FALSE;
    info.xHotspot = xHotSpot;
    info.yHotspot = yHotSpot;
    info.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, lpANDbits );
    info.hbmColor = CreateBitmap( nWidth, nHeight, 1, 1, lpXORbits );
    hCursor = CreateIconIndirect( &info );
    DeleteObject( info.hbmMask );
    DeleteObject( info.hbmColor );
    return hCursor;
}


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
    int       nWidth,     /* [in] the width of the provided bitmaps */
    int       nHeight,    /* [in] the height of the provided bitmaps */
    BYTE      bPlanes,    /* [in] the number of planes in the provided bitmaps */
    BYTE      bBitsPixel, /* [in] the number of bits per pixel of the lpXORbits bitmap */
    const BYTE*   lpANDbits,  /* [in] a monochrome bitmap representing the icon's mask */
    const BYTE*   lpXORbits)  /* [in] the icon's 'color' bitmap */
{
    ICONINFO iinfo;
    HICON hIcon;

    TRACE_(icon)("%dx%d, planes %d, bpp %d, xor %p, and %p\n",
                 nWidth, nHeight, bPlanes, bBitsPixel, lpXORbits, lpANDbits);

    iinfo.fIcon = TRUE;
    iinfo.xHotspot = nWidth / 2;
    iinfo.yHotspot = nHeight / 2;
    if (bPlanes * bBitsPixel > 1)
    {
        iinfo.hbmColor = CreateBitmap( nWidth, nHeight, bPlanes, bBitsPixel, lpXORbits );
        iinfo.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, lpANDbits );
    }
    else
    {
        iinfo.hbmMask = CreateBitmap( nWidth, nHeight * 2, 1, 1, lpANDbits );
        iinfo.hbmColor = NULL;
    }

    hIcon = CreateIconIndirect( &iinfo );

    DeleteObject( iinfo.hbmMask );
    if (iinfo.hbmColor) DeleteObject( iinfo.hbmColor );

    return hIcon;
}


/***********************************************************************
 *		CopyIcon (USER32.@)
 */
HICON WINAPI CopyIcon( HICON hIcon )
{
    HICON hRetIcon = NULL;
    ICONINFO IconInfo;

    if(GetIconInfo(hIcon, &IconInfo))
    {
        hRetIcon = CreateIconIndirect(&IconInfo);
        DeleteObject(IconInfo.hbmColor);
        DeleteObject(IconInfo.hbmMask);
    }

    return hRetIcon;
}


/***********************************************************************
 *		DestroyIcon (USER32.@)
 */
BOOL WINAPI DestroyIcon( HICON hIcon )
{
    TRACE_(icon)("%p\n", hIcon );

    return NtUserDestroyCursor(hIcon, 0);
}


/***********************************************************************
 *		DestroyCursor (USER32.@)
 */
BOOL WINAPI DestroyCursor( HCURSOR hCursor )
{
    if (GetCursor() == hCursor)
    {
        WARN_(cursor)("Destroying active cursor!\n" );
        return FALSE;
    }
    return DestroyIcon( hCursor );
}

/***********************************************************************
 *		DrawIcon (USER32.@)
 */
BOOL WINAPI DrawIcon( HDC hdc, INT x, INT y, HICON hIcon )
{
    return DrawIconEx( hdc, x, y, hIcon, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE );
}

/***********************************************************************
 *		ShowCursor (USER32.@)
 */
INT WINAPI DECLSPEC_HOTPATCH ShowCursor( BOOL bShow )
{
    return NtUserxShowCursor(bShow);
}

/***********************************************************************
 *		GetCursor (USER32.@)
 */
HCURSOR WINAPI GetCursor(void)
{
     return (HCURSOR)NtUserGetThreadState(THREADSTATE_GETCURSOR);
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

        const HDC hdc = GetDC(0);
        const int depth = (cFlag & LR_MONOCHROME) ?
            1 : GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        if( bIcon )
            entry = CURSORICON_FindBestIconRes( dir, ~0u, width, height, depth );
        else
            entry = CURSORICON_FindBestCursorRes( dir, ~0u, width, height, depth );

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
    return NtUserGetIconInfo(hIcon, iconinfo, 0, 0, 0, 0);
}

/* copy an icon bitmap, even when it can't be selected into a DC */
/* helper for CreateIconIndirect */
static void stretch_blt_icon( HDC hdc_dst, int dst_x, int dst_y, int dst_width, int dst_height,
                              HBITMAP src, int width, int height )
{
    HDC hdc = CreateCompatibleDC( 0 );

    if (!SelectObject( hdc, src ))  /* do it the hard way */
    {
        BITMAPINFO *info;
        void *bits;

        if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) return;
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = width;
        info->bmiHeader.biHeight = height;
        info->bmiHeader.biPlanes = GetDeviceCaps( hdc_dst, PLANES );
        info->bmiHeader.biBitCount = GetDeviceCaps( hdc_dst, BITSPIXEL );
        info->bmiHeader.biCompression = BI_RGB;
        info->bmiHeader.biSizeImage = height * get_dib_width_bytes( width, info->bmiHeader.biBitCount );
        info->bmiHeader.biXPelsPerMeter = 0;
        info->bmiHeader.biYPelsPerMeter = 0;
        info->bmiHeader.biClrUsed = 0;
        info->bmiHeader.biClrImportant = 0;
        bits = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
        if (bits && GetDIBits( hdc, src, 0, height, bits, info, DIB_RGB_COLORS ))
            StretchDIBits( hdc_dst, dst_x, dst_y, dst_width, dst_height,
                           0, 0, width, height, bits, info, DIB_RGB_COLORS, SRCCOPY );

        HeapFree( GetProcessHeap(), 0, bits );
        HeapFree( GetProcessHeap(), 0, info );
    }
    else StretchBlt( hdc_dst, dst_x, dst_y, dst_width, dst_height, hdc, 0, 0, width, height, SRCCOPY );

    DeleteDC( hdc );
}

/**********************************************************************
 *		CreateIconIndirect (USER32.@)
 */
HICON WINAPI CreateIconIndirect(PICONINFO iconinfo)
{
    BITMAP bmpXor, bmpAnd;
    HBITMAP color = 0, mask;
    int width, height;
    HDC hdc;
	ICONINFO iinfo;

    TRACE("color %p, mask %p, hotspot %ux%u, fIcon %d\n",
           iconinfo->hbmColor, iconinfo->hbmMask,
           iconinfo->xHotspot, iconinfo->yHotspot, iconinfo->fIcon);

    if (!iconinfo->hbmMask) return 0;

    GetObjectW( iconinfo->hbmMask, sizeof(bmpAnd), &bmpAnd );
    TRACE("mask: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
           bmpAnd.bmWidth, bmpAnd.bmHeight, bmpAnd.bmWidthBytes,
           bmpAnd.bmPlanes, bmpAnd.bmBitsPixel);

    if (iconinfo->hbmColor)
    {
        GetObjectW( iconinfo->hbmColor, sizeof(bmpXor), &bmpXor );
        TRACE("color: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
               bmpXor.bmWidth, bmpXor.bmHeight, bmpXor.bmWidthBytes,
               bmpXor.bmPlanes, bmpXor.bmBitsPixel);

        // the size of the mask bitmap always determines the icon size!
        width = bmpAnd.bmWidth;
        height = bmpAnd.bmHeight;
        if (bmpXor.bmPlanes * bmpXor.bmBitsPixel != 1 )
        {
            color = CreateBitmap( width, height, bmpXor.bmPlanes, bmpXor.bmBitsPixel, NULL );
            if(!color)
            {
                ERR("Unable to create color bitmap!\n");
		return NULL;
            }
            mask = CreateBitmap( width, height, 1, 1, NULL );
	    if(!mask)
	    {
               ERR("Unable to create mask bitmap!\n");
               DeleteObject(color);
               return NULL;
	    }
        }
        else 
	{
           mask = CreateBitmap( width, height * 2, 1, 1, NULL );
           if(!mask)
           {
              ERR("Unable to create mask bitmap!\n");
              return NULL;
           }
        }
    }
    else
    {
        width = bmpAnd.bmWidth;
        height = bmpAnd.bmHeight;
        mask = CreateBitmap( width, height, 1, 1, NULL );
    }

    hdc = CreateCompatibleDC( 0 );
    SelectObject( hdc, mask );
    stretch_blt_icon( hdc, 0, 0, width, height, iconinfo->hbmMask, bmpAnd.bmWidth, bmpAnd.bmHeight );

    if (color)
    {
        SelectObject( hdc, color );
        stretch_blt_icon( hdc, 0, 0, width, height, iconinfo->hbmColor, width, height );
    }
    else if (iconinfo->hbmColor)
    {
        stretch_blt_icon( hdc, 0, height, width, height, iconinfo->hbmColor, width, height );
    }
    else height /= 2;

    DeleteDC( hdc );

    iinfo.hbmColor = color ;
    iinfo.hbmMask = mask ;
    iinfo.fIcon = iconinfo->fIcon;
    if (iinfo.fIcon)
    {
        iinfo.xHotspot = width / 2;
        iinfo.yHotspot = height / 2;
    }
    else
    {
        iinfo.xHotspot = iconinfo->xHotspot;
        iinfo.yHotspot = iconinfo->yHotspot;
    }

	return CreateCursorIconHandle(&iinfo);
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
BOOL WINAPI DrawIconEx( HDC hdc, INT xLeft, INT yTop, HICON hIcon,
                            INT cxWidth, INT cyWidth, UINT istepIfAniCur,
                            HBRUSH hbrFlickerFreeDraw, UINT diFlags )
{
    return NtUserDrawIconEx(hdc, xLeft, yTop, hIcon, cxWidth, cyWidth,
                            istepIfAniCur, hbrFlickerFreeDraw, diFlags,
                            0, 0);
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
    DWORD compr_dummy, offbits = 0;
    INT bm_type;
    HDC screen_mem_dc = NULL;

    if (!(loadflags & LR_LOADFROMFILE))
    {
        if (!instance)
        {
            /* OEM bitmap: try to load the resource from user32.dll */
            instance = User32Instance;
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
        if (bmfh->bfType != 0x4d42 /* 'BM' */)
        {
            WARN("Invalid/unsupported bitmap format!\n");
            goto end_close;
        }
        if (bmfh->bfOffBits) offbits = bmfh->bfOffBits - sizeof(BITMAPFILEHEADER);
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

    if (bm_type == 0)
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)&scaled_info->bmiHeader;
        core->bcWidth = new_width;
        core->bcHeight = new_height;
    }
    else if (bm_type == 1)
    {
        /* Some sanity checks for BITMAPINFO (not applicable to BITMAPCOREINFO) */
        if (info->bmiHeader.biHeight > 65535 || info->bmiHeader.biWidth > 65535) {
            WARN("Broken BitmapInfoHeader!\n");
            goto end;
        }

        scaled_info->bmiHeader.biWidth = new_width;
        scaled_info->bmiHeader.biHeight = new_height;
    }
    else
        goto end;

    if (new_height < 0) new_height = -new_height;

    if (!screen_dc) screen_dc = CreateDCW( DISPLAYW, NULL, NULL, NULL );
    if (!(screen_mem_dc = CreateCompatibleDC( screen_dc ))) goto end;

    bits = (char *)info + (offbits ? offbits : size);

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
end_close:
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

    if (IS_INTRESOURCE(name))
        return LoadImageW(hinst, (LPCWSTR)name, type, desiredx, desiredy, loadflags);

    _SEH2_TRY
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
        u_name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, name, -1, u_name, len );
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        _SEH2_YIELD(return 0);
    }
    _SEH2_END
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
            return CURSORICON_Load(hinst, name, desiredx, desiredy,
                                   GetDeviceCaps(screen_dc, BITSPIXEL),
                                   FALSE, loadflags);
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

HCURSOR
CursorIconToCursor(HICON hIcon,
                   BOOL SemiTransparent)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetCursorPos(int X, int Y)
{
    return NtUserxSetCursorPos(X,Y);
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetCursorPos(LPPOINT lpPoint)
{
    BOOL res;
    /* Windows doesn't check if lpPoint == NULL, we do */
    if(!lpPoint)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    res = NtUserxGetCursorPos(lpPoint);

    return res;
}

/* INTERNAL ******************************************************************/

/* This callback routine is called directly after switching to gui mode */
NTSTATUS
WINAPI
User32SetupDefaultCursors(PVOID Arguments,
                          ULONG ArgumentLength)
{
    BOOL *DefaultCursor = (BOOL*)Arguments;
    HCURSOR hCursor; 

    if(*DefaultCursor)
    {
        /* set default cursor */
        hCursor = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        SetCursor(hCursor);
    }
    else
    {
        /* FIXME load system cursor scheme */
        SetCursor(0);
        hCursor = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        SetCursor(hCursor);
    }

    return(ZwCallbackReturn(&hCursor, sizeof(HCURSOR), STATUS_SUCCESS));
}

BOOL get_icon_size(HICON hIcon, SIZE *size)
{
    ICONINFO info;
    BITMAP bitmap;

    if (!GetIconInfo(hIcon, &info)) return FALSE;
    if (!GetObject(info.hbmMask, sizeof(bitmap), &bitmap)) return FALSE;

    size->cx = bitmap.bmWidth;
    size->cy = bitmap.bmHeight;

    /* Black and white icons store both the XOR and AND bitmap in hbmMask */
    if (!info.hbmColor)
    {
        size->cy /= 2;
    }
    else
    {
        DeleteObject(info.hbmColor);
    }

    DeleteObject(info.hbmMask);

    return TRUE;
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

HCURSOR
WINAPI
GetCursorFrameInfo(HCURSOR hCursor, DWORD reserved, DWORD istep, PINT rate_jiffies, DWORD *num_steps)
{
   return NtUserGetCursorFrameInfo(hCursor, istep, rate_jiffies, num_steps);
}

