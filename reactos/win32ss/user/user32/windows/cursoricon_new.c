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
//WINE_DECLARE_DEBUG_CHANNEL(icon);
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
    HDC hdc, hdcScreen;
    BITMAPINFO* pbmiCopy;
    HBITMAP hbmpOld = NULL;
    BOOL bResult = FALSE;
    const VOID *pvColor, *pvMask;
    
    /* Check for invalid data */
    if ( (pbmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER) &&
          pbmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER))  ||
        pbmi->bmiHeader.biCompression != BI_RGB )
    {
          WARN("Invalid resource bitmap header.\n");
          return FALSE;
    }
    
    /* Fix the hotspot coords */
    if(cxDesired != pbmi->bmiHeader.biWidth)
        pii->xHotspot = (pii->xHotspot * cxDesired) / pbmi->bmiHeader.biWidth;
    if(cxDesired != (pbmi->bmiHeader.biHeight/2))
        pii->yHotspot = (pii->yHotspot * cyDesired * 2) / pbmi->bmiHeader.biHeight;
    
    hdcScreen = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
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
HANDLE
BITMAP_LoadImageW(
  _In_opt_  HINSTANCE hinst,
  _In_      LPCWSTR lpszName,
  _In_      int cxDesired,
  _In_      int cyDesired,
  _In_      UINT fuLoad
)
{
    UNIMPLEMENTED;
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
    HRSRC hrsrc, hrsrc2;
    HANDLE handle, hCurIcon;
    CURSORICONDIR* dir;
    WORD wResId;
    LPBYTE bits;
    ICONINFO ii;
    BOOL bStatus;
    
    if(fuLoad & LR_LOADFROMFILE)
    {
        UNIMPLEMENTED;
        return NULL;
    }
    
    /* Check if caller wants OEM icons */
    if(!hinst)
        hinst = User32Instance;
    
    /* Find resource ID */
    hrsrc = FindResourceW(
        hinst,
        lpszName,
        (LPWSTR)(bIcon ? RT_GROUP_ICON : RT_GROUP_CURSOR));
    
    /* We let FindResource, LoadResource, etc. call SetLastError */
    if(!hrsrc)
        return NULL;
    
    /* Fix width/height */
    if(fuLoad & LR_DEFAULTSIZE)
    {
        if(!cxDesired) cxDesired = GetSystemMetrics(bIcon ? SM_CXICON : SM_CXCURSOR);
        if(!cyDesired) cyDesired = GetSystemMetrics(bIcon ? SM_CYICON : SM_CYCURSOR);
    }
    
    /* If LR_SHARED is set, we must check for the cache */
    hCurIcon = NtUserFindExistingCursorIcon(hinst, hrsrc, cxDesired, cyDesired);
    if(hCurIcon)
        return hCurIcon;
    
    handle = LoadResource(hinst, hrsrc);
    if(!handle)
        return NULL;
    
    dir = LockResource(handle);
        if(!dir) return NULL;
    
    /* For now, take the first entry */
    wResId = dir->idEntries[0].wResId;
    FreeResource(handle);
    
    /* Get the relevant resource pointer */
    hrsrc2 = FindResourceW(
        hinst,
        MAKEINTRESOURCEW(wResId),
        (LPWSTR)(bIcon ? RT_ICON : RT_CURSOR));
    if(!hrsrc2)
        return NULL;
    
    handle = LoadResource(hinst, hrsrc2);
    if(!handle)
        return NULL;
    
    bits = LockResource(handle);
    if(!bits)
    {
        FreeResource(handle);
        return NULL;
    }
    
    /* Get the hospot */
    if(bIcon)
    {
        ii.xHotspot = cxDesired/2;
        ii.yHotspot = cyDesired/2;
    }
    else
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
        return NULL;
    
    /* Create the handle */
    hCurIcon = NtUserxCreateEmptyCurObject(bIcon ? 0 : 1);
    if(!hCurIcon)
        return NULL;
    
    /* Tell win32k */
    if(fuLoad & LR_SHARED)
        bStatus = NtUserSetCursorIconData(hCurIcon, hinst, hrsrc, &ii);
    else
        bStatus = NtUserSetCursorIconData(hCurIcon, NULL, NULL, &ii);
    
    if(!bStatus)
    {
        NtUserDestroyCursor(hCurIcon, TRUE);
        hCurIcon = NULL;
    }
    
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);
    
    return hCurIcon;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI GetIconInfo(
  _In_   HICON hIcon,
  _Out_  PICONINFO piconinfo
)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI DestroyIcon(
  _In_  HICON hIcon
)
{
    UNIMPLEMENTED;
    return FALSE;
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
    UNIMPLEMENTED;
    return 0;
}

int WINAPI LookupIconIdFromDirectoryEx(
  _In_  PBYTE presbits,
  _In_  BOOL fIcon,
  _In_  int cxDesired,
  _In_  int cyDesired,
  _In_  UINT Flags
)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return NULL;
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
    UNIMPLEMENTED;
    return NULL;
}

HICON WINAPI CreateIconIndirect(
  _In_  PICONINFO piconinfo
)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return NULL;
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
    UNIMPLEMENTED;
    return FALSE;
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
    UNIMPLEMENTED;
    return FALSE;
}
