/*
 * PROJECT:         ReactOS user32.dll
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            dll/win32/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Jérôme Gardou (jerome.gardou@reactos.org)
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(cursor);
WINE_DECLARE_DEBUG_CHANNEL(icon);
//WINE_DECLARE_DEBUG_CHANNEL(resource);

/************* USER32 INTERNAL FUNCTIONS **********/

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

/************* IMPLEMENTATION CORE ****************/

static BOOL CURSORICON_GetIconInfoFromBMI(
    _Inout_ ICONINFO* pii,
    _In_    const BITMAPINFO *pbmi,
    _In_    int cxDesired,
    _In_    int cyDesired
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
    
    /* Fix the hotspot coords */
    if(!pii->fIcon)
    {
        if(cxDesired != pbmi->bmiHeader.biWidth)
            pii->xHotspot = (pii->xHotspot * cxDesired) / pbmi->bmiHeader.biWidth;
        if(cxDesired != (pbmi->bmiHeader.biHeight/2))
            pii->yHotspot = (pii->yHotspot * cyDesired * 2) / pbmi->bmiHeader.biHeight;
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
        
    pvColor = (const char*)pbmi + ubmiSize;
    pvMask = (const char*)pvColor +
        get_dib_image_size(pbmi->bmiHeader.biWidth, pbmiCopy->bmiHeader.biHeight, pbmi->bmiHeader.biBitCount );
    
    /* Set XOR bits */
    if(monochrome)
    {
        /* Create the 1bpp bitmap which will contain everything */
        pii->hbmColor = NULL;
        pii->hbmMask = CreateCompatibleBitmap(hdc, cxDesired, cyDesired * 2);
        if(!pii->hbmMask)
            goto done;
        hbmpOld = SelectObject(hdc, pii->hbmMask);
        if(!hbmpOld)
            goto done;
        
        if(!StretchDIBits(hdc, 0, cyDesired, cxDesired, cyDesired,
                          0, 0, pbmiCopy->bmiHeader.biWidth, pbmiCopy->bmiHeader.biHeight,
                          pvColor, pbmiCopy, DIB_RGB_COLORS, SRCCOPY))
            goto done;
    }
    else
    {
        /* Create the bitmap. It has to be compatible with the screen surface */
        pii->hbmColor = CreateCompatibleBitmap(hdcScreen, cxDesired, cyDesired);
        if(!pii->hbmColor)
            goto done;
        /* Create the 1bpp mask bitmap */
        pii->hbmMask = CreateCompatibleBitmap(hdc, cxDesired, cyDesired);
        if(!pii->hbmMask)
            goto done;
        hbmpOld = SelectObject(hdc, pii->hbmColor);
        if(!hbmpOld)
            goto done;
        if(!StretchDIBits(hdc, 0, 0, cxDesired, cyDesired,
                  0, 0, pbmiCopy->bmiHeader.biWidth, pbmiCopy->bmiHeader.biHeight,
                  pvColor, pbmiCopy, DIB_RGB_COLORS, SRCCOPY))
            goto done;
        
        /* Now convert the info to monochrome for the mask bits */
        pbmiCopy->bmiHeader.biBitCount = 1;
        /* Handle the CORE/INFO difference */
        if (pbmiCopy->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            RGBQUAD *rgb = pbmiCopy->bmiColors;

            pbmiCopy->bmiHeader.biClrUsed = pbmiCopy->bmiHeader.biClrImportant = 2;
            rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
            rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
            rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
        }
        else
        {
            RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)pbmiCopy) + 1);

            rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
            rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
        }
    }
    /* Set the mask bits */
    if(!SelectObject(hdc, pii->hbmMask))
        goto done;
    bResult = StretchDIBits(hdc, 0, 0, cxDesired, cyDesired,
                  0, 0, pbmiCopy->bmiHeader.biWidth, pbmiCopy->bmiHeader.biHeight,
                  pvMask, pbmiCopy, DIB_RGB_COLORS, SRCCOPY) != 0;
    
done:
    DeleteDC(hdcScreen);
    if(hbmpOld) SelectObject(hdc, hbmpOld);
    DeleteDC(hdc);
    if(pbmiCopy) HeapFree(GetProcessHeap(), 0, pbmiCopy);
    /* Clean up in case of failure */
    if(!bResult)
    {
        if(pii->hbmMask) DeleteObject(pii->hbmMask);
        if(pii->hbmColor) DeleteObject(pii->hbmColor);
    }
    return bResult;
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
    const VOID* pvMapping;
    DWORD dwOffset = 0;
    HGLOBAL hgRsrc;
    int iBMISize;
    PVOID pvBits;
    HDC hdcScreen = NULL;
    HDC hdc = NULL;
    HBITMAP hbmpRet, hbmpOld;

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
        hrsrc = FindResourceW(hinst, lpszName, (LPWSTR)RT_BITMAP);
        if(!hrsrc)
            return NULL;
        hgRsrc = LoadResource(hinst, hrsrc);
        if(!hgRsrc)
            return NULL;
        pbmi = LockResource(hgRsrc);
        if(!pbmi)
            return NULL;
    }
    
    /* See if we must scale the bitmap */
    iBMISize = bitmap_info_size(pbmi, DIB_RGB_COLORS);
    
    /* Get a pointer to the image data */
    pvBits = (char*)pbmi + (dwOffset ? dwOffset : iBMISize);

    /* Create a copy of the info describing the bitmap in the file */
    pbmiCopy = HeapAlloc(GetProcessHeap(), 0, iBMISize);
    if(!pbmiCopy)
        goto end;
    CopyMemory(pbmiCopy, pbmi, iBMISize);
    
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
            if(cxDesired == 0)
                cxDesired = pbmch->bcWidth;
            if(cyDesired == 0)
                cyDesired == pbmch->bcHeight;
            else if(pbmch->bcHeight < 0)
                cyDesired = -cyDesired;

            pbmch->bcWidth = cxDesired;
            pbmch->bcHeight = cyDesired;
        }
        else
        {
            if ((pbmi->bmiHeader.biHeight > 65535) || (pbmi->bmiHeader.biWidth > 65535)) {
                WARN("Broken BITMAPINFO!\n");
                goto end;
            }
            
            if(cxDesired == 0)
                cxDesired = pbmi->bmiHeader.biWidth;
            if(cyDesired == 0)
                cyDesired = pbmi->bmiHeader.biHeight;
            else if(pbmi->bmiHeader.biHeight < 0)
                cyDesired = -cyDesired;

            pbmiScaled->bmiHeader.biWidth = cxDesired;
            pbmiScaled->bmiHeader.biHeight = cyDesired;
            /* No compression for DIB sections */
            if(fuLoad & LR_CREATEDIBSECTION)
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
                           0, 0, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biWidth,
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
    if (fuLoad & LR_LOADFROMFILE)
        UnmapViewOfFile( pvMapping );
    else
        FreeResource(hgRsrc);

    return hbmpRet;
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
    CURSORICONDIR* fakeDir;
    CURSORICONDIRENTRY* fakeEntry;
    CURSORICONFILEDIRENTRY *entry;
    CURSORICONFILEDIR *dir;
    DWORD filesize = 0;
    LPBYTE bits;
    HANDLE hRet = NULL;
    WORD i;
    ICONINFO ii;

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
    if ( filesize < sizeof(*dir) )
        goto end;

    if ( filesize < (sizeof(*dir) + sizeof(dir->idEntries[0])*(dir->idCount-1)) )
        goto end;
    
    /* 
     * Cute little hack:
     * We allocate a buffer, fake it as if it was a pointer to a resource in a module,
     * pass it to LookupIconIdFromDirectoryEx and get back the index we have to use
     */
    fakeDir = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(CURSORICONDIR, idEntries[dir->idCount]));
    if(!fakeDir)
        goto end;
    fakeDir->idReserved = 0;
    fakeDir->idType = dir->idType;
    fakeDir->idCount = dir->idCount;
    for(i = 0; i<dir->idCount; i++)
    {
        fakeEntry = &fakeDir->idEntries[i];
        entry = &dir->idEntries[i];
        /* Take this as an occasion to perform a size check */
        if((entry->dwDIBOffset + entry->dwDIBSize) > filesize)
            goto end;
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
        goto end;
    }
    
    /* Get our entry */
    entry = &dir->idEntries[i-1];
    /* A bit of preparation */
    ii.xHotspot = entry->xHotspot;
    ii.yHotspot = entry->yHotspot;
    ii.fIcon = bIcon;
    
    /* Do the dance */
    if(!CURSORICON_GetIconInfoFromBMI(&ii, (BITMAPINFO*)&bits[entry->dwDIBOffset], cxDesired, cyDesired))
        goto end;
    
    /* Create the icon. NOTE: there's no LR_SHARED icons if they are created from file */
    hRet = CreateIconIndirect(&ii);
    
    /* Clean up */
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);
    
end:
    UnmapViewOfFile(bits);
    return hRet;
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
    ICONINFO ii;
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
    
    if(fuLoad & LR_SHARED)
    {
        DWORD size = MAX_PATH;
        
        TRACE("Checking for an LR_SHARED cursor/icon.\n");
        /* Prepare the resource name string */
        if(IS_INTRESOURCE(lpszName))
        {
            ustrRsrc.Buffer = (LPWSTR)lpszName;
            ustrRsrc.Length = 0;
            ustrRsrc.MaximumLength = 0;
        }
        else
            RtlInitUnicodeString(&ustrRsrc, lpszName);
        
        /* Prepare the module name string */
        ustrModule.Buffer = HeapAlloc(GetProcessHeap(), 0, size*sizeof(WCHAR));
        /* Get it */
        do
        {
            DWORD ret = GetModuleFileName(hinst, ustrModule.Buffer, size);
            if(ret == 0)
            {
                HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
                return NULL;
            }
            if(ret < size)
            {
                ustrModule.Length = ret;
                ustrModule.MaximumLength = size;
                break;
            }
            size *= 2;
            ustrModule.Buffer = HeapReAlloc(GetProcessHeap(), 0, ustrModule.Buffer, size*sizeof(WCHAR));
        } while(TRUE);
        
        /* Ask win32k */
        hCurIcon = NtUserFindExistingCursorIcon(&ustrModule, &ustrRsrc, cxDesired, cyDesired);
        if(hCurIcon)
        {
            /* Woohoo, got it! */
            TRACE("MATCH!\n");
            HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
            return hCurIcon;
        }
    }

    /* Find resource ID */
    hrsrc = FindResourceW(
        hinst,
        lpszName,
        (LPWSTR)(bIcon ? RT_GROUP_ICON : RT_GROUP_CURSOR));
    
    /* We let FindResource, LoadResource, etc. call SetLastError */
    if(!hrsrc)
        goto done;
    
    handle = LoadResource(hinst, hrsrc);
    if(!handle)
        goto done;
    
    dir = LockResource(handle);
    if(!dir)
        goto done;
    
    wResId = LookupIconIdFromDirectoryEx((PBYTE)dir, bIcon, cxDesired, cyDesired, fuLoad & LR_MONOCHROME);
    FreeResource(handle);
    
    /* Get the relevant resource pointer */
    hrsrc = FindResourceW(
        hinst,
        MAKEINTRESOURCEW(wResId),
        (LPWSTR)(bIcon ? RT_ICON : RT_CURSOR));
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
    
    /* Get the hotspot */
    if(bIcon)
    {
        ii.xHotspot = cxDesired/2;
        ii.yHotspot = cyDesired/2;
    }
    if(!bIcon)
    {
        SHORT* ptr = (SHORT*)bits;
        ii.xHotspot = ptr[0];
        ii.yHotspot = ptr[1];
        bits += 2*sizeof(SHORT);
    }
    ii.fIcon = bIcon;
    
    /* Get the bitmaps */
    bStatus = CURSORICON_GetIconInfoFromBMI(
        &ii,
        (BITMAPINFO*)bits,
        cxDesired,
        cyDesired);
    
    FreeResource( handle );
    
    if(!bStatus)
        goto done;
    
    /* Create the handle */
    hCurIcon = NtUserxCreateEmptyCurObject(bIcon ? 0 : 1);
    if(!hCurIcon)
    {
        DeleteObject(ii.hbmMask);
        if(ii.hbmColor) DeleteObject(ii.hbmColor);
        goto done;
    }
    
    /* Tell win32k */
    if(fuLoad & LR_SHARED)
        bStatus = NtUserSetCursorIconData(hCurIcon, &ustrModule, &ustrRsrc, &ii);
    else
        bStatus = NtUserSetCursorIconData(hCurIcon, NULL, NULL, &ii);
    
    if(!bStatus)
    {
        NtUserDestroyCursor(hCurIcon, TRUE);
        hCurIcon = NULL;
    }
    
    DeleteObject(ii.hbmMask);
    if(ii.hbmColor) DeleteObject(ii.hbmColor);

done:
    if(ustrModule.Buffer)
        HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
    return hCurIcon;
}

static
HBITMAP
BITMAP_CopyImage(
  _In_  HBITMAP hbmp,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT fuFlags
)
{
    HBITMAP res = NULL;
    DIBSECTION ds;
    int objSize;
    BITMAPINFO * bi;

    objSize = GetObjectW( hbmp, sizeof(ds), &ds );
    if (!objSize) return 0;
    if ((cxDesired < 0) || (cyDesired < 0)) return 0;

    if (fuFlags & LR_COPYFROMRESOURCE)
    {
        FIXME("The flag LR_COPYFROMRESOURCE is not implemented for bitmaps\n");
    }
    
    if (fuFlags & LR_COPYRETURNORG)
    {
        FIXME("The flag LR_COPYRETURNORG is not implemented for bitmaps\n");
    }

    if (cxDesired == 0) cxDesired = ds.dsBm.bmWidth;
    if (cyDesired == 0) cyDesired = ds.dsBm.bmHeight;

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

    if (fuFlags & LR_CREATEDIBSECTION)
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
        GetDIBits(dc, hbmp, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);

        bi->bmiHeader.biWidth  = cxDesired;
        bi->bmiHeader.biHeight = cyDesired;
        bi->bmiHeader.biSizeImage = 0;

        res = CreateDIBSection(dc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
        DeleteDC(dc);
    }
    else
    {
        /* Create a device-dependent bitmap */

        BOOL monochrome = (fuFlags & LR_MONOCHROME);

        if (objSize == sizeof(DIBSECTION))
        {
            /* The source bitmap is a DIB section.
               Get its attributes */
            HDC dc = CreateCompatibleDC(NULL);
            bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
            bi->bmiHeader.biBitCount = ds.dsBm.bmBitsPixel;
            GetDIBits(dc, hbmp, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
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
            res = CreateBitmap(cxDesired, cyDesired, 1, 1, NULL);
        }
        else
        {
            HDC screenDC = GetDC(NULL);
            res = CreateCompatibleBitmap(screenDC, cxDesired, cyDesired);
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
            GetDIBits(dc, hbmp, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
            bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bi->bmiHeader.biSizeImage);

            if (bits)
            {
                HBITMAP oldBmp;

                /* Get the image bits of the source bitmap */
                GetDIBits(dc, hbmp, 0, ds.dsBm.bmHeight, bits, bi, DIB_RGB_COLORS);

                /* Copy it to the destination bitmap */
                oldBmp = SelectObject(dc, res);
                StretchDIBits(dc, 0, 0, cxDesired, cyDesired,
                              0, 0, ds.dsBm.bmWidth, ds.dsBm.bmHeight,
                              bits, bi, DIB_RGB_COLORS, SRCCOPY);
                SelectObject(dc, oldBmp);

                HeapFree(GetProcessHeap(), 0, bits);
            }

            DeleteDC(dc);
        }

        if (fuFlags & LR_COPYDELETEORG)
        {
            DeleteObject(hbmp);
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
    
    if(fuFlags & LR_COPYFROMRESOURCE)
    {
        /* Get the icon module/resource names */
        UNICODE_STRING ustrModule;
        UNICODE_STRING ustrRsrc;
        PVOID pvBuf;
        HMODULE hModule;
        
        ustrModule.MaximumLength = MAX_PATH;
        ustrRsrc.MaximumLength = 256;
        
        ustrModule.Buffer = HeapAlloc(GetProcessHeap(), 0, ustrModule.MaximumLength * sizeof(WCHAR));
        if(!ustrModule.Buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        /* Keep track of the buffer for the resource, NtUserGetIconInfo might overwrite it */
        pvBuf = HeapAlloc(GetProcessHeap(), 0, 256 * sizeof(WCHAR));
        if(!pvBuf)
        {
            HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        ustrRsrc.Buffer = pvBuf;
        
        do
        {
            if(!NtUserGetIconInfo(hicon, NULL, &ustrModule, &ustrRsrc, NULL, FALSE))
            {
                HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
                HeapFree(GetProcessHeap(), 0, pvBuf);
                return NULL;
            }
            
            if((ustrModule.Length < ustrModule.MaximumLength) && (ustrRsrc.Length < ustrRsrc.MaximumLength))
            {
                /* Buffers were big enough */
                break;
            }
            
            /* Find which buffer were too small */
            if(ustrModule.Length == ustrModule.MaximumLength)
            {
                PWSTR newBuffer;
                ustrModule.MaximumLength *= 2;
                newBuffer = HeapReAlloc(GetProcessHeap(), 0, ustrModule.Buffer, ustrModule.MaximumLength * sizeof(WCHAR));
                if(!ustrModule.Buffer)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto leave;
                }
                ustrModule.Buffer = newBuffer;
            }
            
            if(ustrRsrc.Length == ustrRsrc.MaximumLength)
            {
                ustrRsrc.MaximumLength *= 2;
                pvBuf = HeapReAlloc(GetProcessHeap(), 0, ustrRsrc.Buffer, ustrRsrc.MaximumLength * sizeof(WCHAR));
                if(!pvBuf)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto leave;
                }
                ustrRsrc.Buffer = pvBuf;
            }
        } while(TRUE);
        
        /* NULL-terminate our strings */
        ustrModule.Buffer[ustrModule.Length] = 0;
        if(!IS_INTRESOURCE(ustrRsrc.Buffer))
            ustrRsrc.Buffer[ustrRsrc.Length] = 0;
        
        /* Get the module handle */
        if(!GetModuleHandleExW(0, ustrModule.Buffer, &hModule))
        {
            /* This hould never happen */
            SetLastError(ERROR_INVALID_PARAMETER);
            goto leave;
        }
        
        /* Call the relevant function */
        ret = CURSORICON_LoadImageW(hModule, ustrRsrc.Buffer, cxDesired, cyDesired, bIcon, fuFlags & LR_DEFAULTSIZE);
        
        FreeLibrary(hModule);
        
        /* If we're here, that means that the passed icon is shared. Don't destroy it, even if LR_COPYDELETEORG is specified */
    leave:
        HeapFree(GetProcessHeap(), 0, ustrModule.Buffer);
        HeapFree(GetProcessHeap(), 0, pvBuf);
        
        return ret;
    }
    
    /* This is a regular copy */
    if(fuFlags & ~LR_COPYDELETEORG)
        FIXME("Unimplemented flags: 0x%08x\n", fuFlags);
    
    if(!GetIconInfo(hicon, &ii))
    {
        ERR("GetIconInfo failed.\n");
        return NULL;
    }
    
    ret = CreateIconIndirect(&ii);
    
    DeleteObject(ii.hbmMask);
    if(ii.hbmColor) DeleteObject(ii.hbmColor);
    
    if(ret && (fuFlags & LR_COPYDELETEORG))
        DestroyIcon(hicon);
    
    return hicon;
}

/************* PUBLIC FUNCTIONS *******************/

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
    switch(uType)
    {
        case IMAGE_BITMAP:
            return BITMAP_CopyImage(hImage, cxDesired, cyDesired, fuFlags);
        case IMAGE_CURSOR:
        case IMAGE_ICON:
            return CURSORICON_CopyImage(hImage, uType == IMAGE_ICON, cxDesired, cyDesired, fuFlags);
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
    UNIMPLEMENTED;
    return NULL;
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
    int i, numMatch, iIndex = -1;
    WORD width, height;
    USHORT bitcount;
    ULONG cxyDiff, cxyDiffTmp;
    
    TRACE("%p, %x, %i, %i, %x.\n", presbits, fIcon, cxDesired, cyDesired, Flags);
    
    if(!(dir && !dir->idReserved && (dir->idType & 3)))
    {
        WARN("Invalid resource.\n");
        return 0;
    }
    
    /* idType == 2 is for cursors, 1 for icons */
    /*if(fIcon)
    {
        if(dir->idType == 2)
        {
            WARN("An icon was asked for a cursor resource.\n");
            return 0;
        }
    }
    else if(dir->idType == 1)
    {
        WARN("A cursor was asked for an icon resource.\n");
        return 0;
    }*/
    
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

    /* Find the best match for the desired size */
    cxyDiff = 0xFFFFFFFF;
    numMatch = 0;
    for(i = 0; i < dir->idCount; i++)
    {
        entry = &dir->idEntries[i];
        width = fIcon ? entry->ResInfo.icon.bWidth : entry->ResInfo.cursor.wWidth;
        /* Height is twice as big in cursor resources */
        height = fIcon ? entry->ResInfo.icon.bHeight : entry->ResInfo.cursor.wHeight/2;
        /* 0 represents 256 */
        if(!width) width = 256;
        if(!height) height = 256;
        /* Let it be a 1-norm */
        cxyDiffTmp = max(abs(width - cxDesired), abs(height - cyDesired));
        if( cxyDiffTmp > cxyDiff)
            continue;
        if( cxyDiffTmp == cxyDiff)
        {
            numMatch++;
            continue;
        }
        iIndex = i;
        numMatch = 1;
        cxyDiff = cxyDiffTmp;
    }
    
    if(numMatch == 1)
    {
        /* Only one entry fit the asked dimensions */
        return dir->idEntries[iIndex].wResId;
    }
    
    bitcount = 0;
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
        cxyDiffTmp = max(abs(width - cxDesired), abs(height - cyDesired));
        if(cxyDiffTmp != cxyDiff)
            continue;
        /* Exact match? */
        if(entry->wBitCount == bppDesired)
            return entry->wResId;
        /* We take the highest possible but smaller  than the display depth */
        if((entry->wBitCount > bitcount) && (entry->wBitCount < bppDesired))
        {
            iIndex = i;
            bitcount = entry->wBitCount;
        }
    }
    
    if(iIndex >= 0)
        return dir->idEntries[iIndex].wResId;
    
    /* No inferior or equal depth available. Get the smallest one */
    bitcount = 0x7FFF;
    for(i = 0; i < dir->idCount; i++)
    {
        entry = &dir->idEntries[i];
        width = fIcon ? entry->ResInfo.icon.bWidth : entry->ResInfo.cursor.wWidth;
        height = fIcon ? entry->ResInfo.icon.bHeight : entry->ResInfo.cursor.wHeight/2;
        /* 0 represents 256 */
        if(!width) width = 256;
        if(!height) height = 256;
        /* Check if this is the best match we had */
        cxyDiffTmp = max(abs(width - cxDesired), abs(height - cyDesired));
        if(cxyDiffTmp != cxyDiff)
            continue;
        /* Check the bit depth */
        if(entry->wBitCount < bitcount)
        {
            iIndex = i;
            bitcount = entry->wBitCount;
        }
    }
    
    return dir->idEntries[iIndex].wResId;
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
    return CreateIconFromResourceEx( presbits, dwResSize, fIcon, dwVer, 0,0,0);
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
    ICONINFO ii;
    HICON hIcon;
    
    if(uFlags)
        FIXME("uFlags 0x%08x ignored.\n", uFlags);
    
    if(fIcon)
    {
        ii.xHotspot = cxDesired/2;
        ii.yHotspot = cyDesired/2;
    }
    else
    {
        WORD* pt = (WORD*)pbIconBits;
        ii.xHotspot = *pt++;
        ii.yHotspot = *pt++;
        pbIconBits = (PBYTE)pt;
    }
    ii.fIcon = fIcon;
    
    if(!CURSORICON_GetIconInfoFromBMI(&ii, (BITMAPINFO*)pbIconBits, cxDesired, cyDesired))
        return NULL;
    
    hIcon = CreateIconIndirect(&ii);
    
    /* Clean up */
    DeleteObject(ii.hbmMask);
    if(ii.hbmColor) DeleteObject(ii.hbmColor);
    
    return hIcon;
}

HICON WINAPI CreateIconIndirect(
  _In_  PICONINFO piconinfo
)
{
    /* As simple as creating a handle, and let win32k deal with the bitmaps */
    HICON hiconRet;
    
    TRACE("%p.\n", piconinfo);
    
    hiconRet = NtUserxCreateEmptyCurObject(piconinfo->fIcon ? 0 : 1);
    if(!hiconRet)
        return NULL;
    
    if(!NtUserSetCursorIconData(hiconRet, NULL, NULL, piconinfo))
    {
        NtUserDestroyCursor(hiconRet, FALSE);
        hiconRet = NULL;
    }
    
    TRACE("Returning 0x%08x.\n", hiconRet);
    
    return hiconRet;
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
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI SetCursorPos(
  _In_  int X,
  _In_  int Y
)
{
    UNIMPLEMENTED;
    return FALSE;
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
    UNIMPLEMENTED;
    return -1;
}

HCURSOR WINAPI GetCursor(void)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL WINAPI DestroyCursor(
  _In_  HCURSOR hCursor
)
{
    return NtUserDestroyCursor(hCursor, FALSE);
}
