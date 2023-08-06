/*
 *    shell icon cache (SIC)
 *
 * Copyright 1998, 1999 Juergen Schmied
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/********************** THE ICON CACHE ********************************/

#define INVALID_INDEX -1

typedef struct
{
    LPWSTR sSourceFile;    /* file (not path!) containing the icon */
    DWORD dwSourceIndex;    /* index within the file, if it is a resoure ID it will be negated */
    DWORD dwListIndex;    /* index within the iconlist */
    DWORD dwFlags;        /* GIL_* flags */
    DWORD dwAccessTime;
} SIC_ENTRY, * LPSIC_ENTRY;

static HDPA        sic_hdpa = 0;

static HIMAGELIST ShellSmallIconList;
static HIMAGELIST ShellBigIconList;

namespace
{
extern CRITICAL_SECTION SHELL32_SicCS;
CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &SHELL32_SicCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": SHELL32_SicCS") }
};
CRITICAL_SECTION SHELL32_SicCS = { &critsect_debug, -1, 0, 0, 0, 0 };
}

/*****************************************************************************
 * SIC_CompareEntries
 *
 * NOTES
 *  Callback for DPA_Search
 */
static INT CALLBACK SIC_CompareEntries( LPVOID p1, LPVOID p2, LPARAM lparam)
{    LPSIC_ENTRY e1 = (LPSIC_ENTRY)p1, e2 = (LPSIC_ENTRY)p2;

    TRACE("%p %p %8lx\n", p1, p2, lparam);

    /* Icons in the cache are keyed by the name of the file they are
     * loaded from, their resource index and the fact if they have a shortcut
     * icon overlay or not.
     */
    /* first the faster one */
    if (e1->dwSourceIndex != e2->dwSourceIndex)
      return (e1->dwSourceIndex < e2->dwSourceIndex) ? -1 : 1;

    if ((e1->dwFlags & GIL_FORSHORTCUT) != (e2->dwFlags & GIL_FORSHORTCUT))
      return ((e1->dwFlags & GIL_FORSHORTCUT) < (e2->dwFlags & GIL_FORSHORTCUT)) ? -1 : 1;

    return wcsicmp(e1->sSourceFile,e2->sSourceFile);
}

/* declare SIC_LoadOverlayIcon() */
static int SIC_LoadOverlayIcon(int icon_idx);

/*****************************************************************************
 * SIC_OverlayShortcutImage            [internal]
 *
 * NOTES
 *  Creates a new icon as a copy of the passed-in icon, overlayed with a
 *  shortcut image.
 * FIXME: This should go to the ImageList implementation!
 */
static HICON SIC_OverlayShortcutImage(HICON SourceIcon, BOOL large)
{
    ICONINFO ShortcutIconInfo, TargetIconInfo;
    HICON ShortcutIcon = NULL, TargetIcon;
    BITMAP TargetBitmapInfo, ShortcutBitmapInfo;
    HDC ShortcutDC = NULL,
      TargetDC = NULL;
    HBITMAP OldShortcutBitmap = NULL,
      OldTargetBitmap = NULL;

    static int s_imgListIdx = -1;
    ZeroMemory(&ShortcutIconInfo, sizeof(ShortcutIconInfo));
    ZeroMemory(&TargetIconInfo, sizeof(TargetIconInfo));

    /* Get information about the source icon and shortcut overlay.
     * We will write over the source bitmaps to get the final ones */
    if (! GetIconInfo(SourceIcon, &TargetIconInfo))
        return NULL;

    /* Is it possible with the ImageList implementation? */
    if(!TargetIconInfo.hbmColor)
    {
        /* Maybe we'll support this at some point */
        FIXME("1bpp icon wants its overlay!\n");
        goto fail;
    }

    if(!GetObjectW(TargetIconInfo.hbmColor, sizeof(BITMAP), &TargetBitmapInfo))
    {
        goto fail;
    }

    /* search for the shortcut icon only once */
    if (s_imgListIdx == -1)
        s_imgListIdx = SIC_LoadOverlayIcon(- IDI_SHELL_SHORTCUT);
                           /* FIXME should use icon index 29 instead of the
                              resource id, but not all icons are present yet
                              so we can't use icon indices */

    if (s_imgListIdx != -1)
    {
        if (large)
            ShortcutIcon = ImageList_GetIcon(ShellBigIconList, s_imgListIdx, ILD_TRANSPARENT);
        else
            ShortcutIcon = ImageList_GetIcon(ShellSmallIconList, s_imgListIdx, ILD_TRANSPARENT);
    } else
        ShortcutIcon = NULL;

    if (!ShortcutIcon || !GetIconInfo(ShortcutIcon, &ShortcutIconInfo))
    {
        goto fail;
    }

    /* Is it possible with the ImageLists ? */
    if(!ShortcutIconInfo.hbmColor)
    {
        /* Maybe we'll support this at some point */
        FIXME("Should draw 1bpp overlay!\n");
        goto fail;
    }

    if(!GetObjectW(ShortcutIconInfo.hbmColor, sizeof(BITMAP), &ShortcutBitmapInfo))
    {
        goto fail;
    }

    /* Setup the masks */
    ShortcutDC = CreateCompatibleDC(NULL);
    if (NULL == ShortcutDC) goto fail;
    OldShortcutBitmap = (HBITMAP)SelectObject(ShortcutDC, ShortcutIconInfo.hbmMask);
    if (NULL == OldShortcutBitmap) goto fail;

    TargetDC = CreateCompatibleDC(NULL);
    if (NULL == TargetDC) goto fail;
    OldTargetBitmap = (HBITMAP)SelectObject(TargetDC, TargetIconInfo.hbmMask);
    if (NULL == OldTargetBitmap) goto fail;

    /* Create the complete mask by ANDing the source and shortcut masks.
     * NOTE: in an ImageList, all icons have the same dimensions */
    if (!BitBlt(TargetDC, 0, 0, ShortcutBitmapInfo.bmWidth, ShortcutBitmapInfo.bmHeight,
                ShortcutDC, 0, 0, SRCAND))
    {
      goto fail;
    }

    /*
     * We must remove or add the alpha component to the shortcut overlay:
     * If we don't, SRCCOPY will copy it to our resulting icon, resulting in a
     * partially transparent icons where it shouldn't be, and to an invisible icon
     * if the underlying icon don't have any alpha channel information. (16bpp only icon for instance).
     * But if the underlying icon has alpha channel information, then we must mark the overlay information
     * as opaque.
     * NOTE: This code sucks(tm) and should belong to the ImageList implementation.
     * NOTE2: there are better ways to do this.
     */
    if(ShortcutBitmapInfo.bmBitsPixel == 32)
    {
        BOOL add_alpha;
        BYTE buffer[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
        BITMAPINFO* lpbmi = (BITMAPINFO*)buffer;
        PVOID bits;
        PULONG pixel;
        INT i, j;

        /* Find if the source bitmap has an alpha channel */
        if(TargetBitmapInfo.bmBitsPixel != 32) add_alpha = FALSE;
        else
        {
            ZeroMemory(buffer, sizeof(buffer));
            lpbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            lpbmi->bmiHeader.biWidth = TargetBitmapInfo.bmWidth;
            lpbmi->bmiHeader.biHeight = TargetBitmapInfo.bmHeight;
            lpbmi->bmiHeader.biPlanes = 1;
            lpbmi->bmiHeader.biBitCount = 32;

            bits = HeapAlloc(GetProcessHeap(), 0, TargetBitmapInfo.bmHeight * TargetBitmapInfo.bmWidthBytes);

            if(!bits) goto fail;

            if(!GetDIBits(TargetDC, TargetIconInfo.hbmColor, 0, TargetBitmapInfo.bmHeight, bits, lpbmi, DIB_RGB_COLORS))
            {
                ERR("GetBIBits failed!\n");
                HeapFree(GetProcessHeap(), 0, bits);
                goto fail;
            }

            i = j = 0;
            pixel = (PULONG)bits;

            for(i=0; i<TargetBitmapInfo.bmHeight; i++)
            {
                for(j=0; j<TargetBitmapInfo.bmWidth; j++)
                {
                    add_alpha = (*pixel++ & 0xFF000000) != 0;
                    if(add_alpha) break;
                }
                if(add_alpha) break;
            }
            HeapFree(GetProcessHeap(), 0, bits);
        }

        /* Allocate the bits */
        bits = HeapAlloc(GetProcessHeap(), 0, ShortcutBitmapInfo.bmHeight*ShortcutBitmapInfo.bmWidthBytes);
        if(!bits) goto fail;

        ZeroMemory(buffer, sizeof(buffer));
        lpbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        lpbmi->bmiHeader.biWidth = ShortcutBitmapInfo.bmWidth;
        lpbmi->bmiHeader.biHeight = ShortcutBitmapInfo.bmHeight;
        lpbmi->bmiHeader.biPlanes = 1;
        lpbmi->bmiHeader.biBitCount = 32;

        if(!GetDIBits(TargetDC, ShortcutIconInfo.hbmColor, 0, ShortcutBitmapInfo.bmHeight, bits, lpbmi, DIB_RGB_COLORS))
        {
            ERR("GetBIBits failed!\n");
            HeapFree(GetProcessHeap(), 0, bits);
            goto fail;
        }

        pixel = (PULONG)bits;
        /* Remove alpha channel component or make it totally opaque */
        for(i=0; i<ShortcutBitmapInfo.bmHeight; i++)
        {
            for(j=0; j<ShortcutBitmapInfo.bmWidth; j++)
            {
                if(add_alpha) *pixel++ |= 0xFF000000;
                else *pixel++ &= 0x00FFFFFF;
            }
        }

        /* GetDIBits return BI_BITFIELDS with masks set to 0, and SetDIBits fails when masks are 0. The irony... */
        lpbmi->bmiHeader.biCompression = BI_RGB;

        /* Set the bits again */
        if(!SetDIBits(TargetDC, ShortcutIconInfo.hbmColor, 0, ShortcutBitmapInfo.bmHeight, bits, lpbmi, DIB_RGB_COLORS))
        {
            ERR("SetBIBits failed!, %lu\n", GetLastError());
            HeapFree(GetProcessHeap(), 0, bits);
            goto fail;
        }
        HeapFree(GetProcessHeap(), 0, bits);
    }

    /* Now do the copy. We overwrite the original icon data */
    if (NULL == SelectObject(ShortcutDC, ShortcutIconInfo.hbmColor) ||
        NULL == SelectObject(TargetDC, TargetIconInfo.hbmColor))
        goto fail;
    if (!MaskBlt(TargetDC, 0, 0, ShortcutBitmapInfo.bmWidth, ShortcutBitmapInfo.bmHeight,
                 ShortcutDC, 0, 0, ShortcutIconInfo.hbmMask, 0, 0,
                 MAKEROP4(0xAA0000, SRCCOPY)))
    {
        goto fail;
    }

    /* Clean up, we're not goto'ing to 'fail' after this so we can be lazy and not set
       handles to NULL */
    SelectObject(TargetDC, OldTargetBitmap);
    DeleteDC(TargetDC);
    SelectObject(ShortcutDC, OldShortcutBitmap);
    DeleteDC(ShortcutDC);

    /* Create the icon using the bitmaps prepared earlier */
    TargetIcon = CreateIconIndirect(&TargetIconInfo);

    /* CreateIconIndirect copies the bitmaps, so we can release our bitmaps now */
    DeleteObject(TargetIconInfo.hbmColor);
    DeleteObject(TargetIconInfo.hbmMask);
    /* Delete what GetIconInfo gave us */
    DeleteObject(ShortcutIconInfo.hbmColor);
    DeleteObject(ShortcutIconInfo.hbmMask);
    DestroyIcon(ShortcutIcon);

    return TargetIcon;

fail:
    /* Clean up scratch resources we created */
    if (NULL != OldTargetBitmap) SelectObject(TargetDC, OldTargetBitmap);
    if (NULL != TargetDC) DeleteDC(TargetDC);
    if (NULL != OldShortcutBitmap) SelectObject(ShortcutDC, OldShortcutBitmap);
    if (NULL != ShortcutDC) DeleteDC(ShortcutDC);
    if (NULL != TargetIconInfo.hbmColor) DeleteObject(TargetIconInfo.hbmColor);
    if (NULL != TargetIconInfo.hbmMask) DeleteObject(TargetIconInfo.hbmMask);
    if (NULL != ShortcutIconInfo.hbmColor) DeleteObject(ShortcutIconInfo.hbmColor);
    if (NULL != ShortcutIconInfo.hbmMask) DeleteObject(ShortcutIconInfo.hbmMask);
    if (NULL != ShortcutIcon) DestroyIcon(ShortcutIcon);

    return NULL;
}

/*****************************************************************************
 * SIC_IconAppend            [internal]
 *
 * NOTES
 *  appends an icon pair to the end of the cache
 */
static INT SIC_IconAppend (LPCWSTR sSourceFile, INT dwSourceIndex, HICON hSmallIcon, HICON hBigIcon, DWORD dwFlags)
{
    LPSIC_ENTRY lpsice;
    INT ret, index, index1, indexDPA;
    WCHAR path[MAX_PATH];
    TRACE("%s %i %p %p\n", debugstr_w(sSourceFile), dwSourceIndex, hSmallIcon ,hBigIcon);

    lpsice = (LPSIC_ENTRY) SHAlloc (sizeof (SIC_ENTRY));

    GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
    lpsice->sSourceFile = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, (wcslen(path)+1)*sizeof(WCHAR) );
    wcscpy( lpsice->sSourceFile, path );

    lpsice->dwSourceIndex = dwSourceIndex;
    lpsice->dwFlags = dwFlags;

    EnterCriticalSection(&SHELL32_SicCS);

    indexDPA = DPA_Search (sic_hdpa, lpsice, 0, SIC_CompareEntries, 0, DPAS_SORTED|DPAS_INSERTAFTER);
    indexDPA = DPA_InsertPtr(sic_hdpa, indexDPA, lpsice);
    if ( -1 == indexDPA )
    {
        ret = INVALID_INDEX;
        goto leave;
    }

    index = ImageList_AddIcon (ShellSmallIconList, hSmallIcon);
    index1= ImageList_AddIcon (ShellBigIconList, hBigIcon);

    /* Something went wrong when allocating a new image in the list. Abort. */
    if((index == -1) || (index1 == -1))
    {
        WARN("Something went wrong when adding the icon to the list: small - 0x%x, big - 0x%x.\n",
            index, index1);
        if(index != -1) ImageList_Remove(ShellSmallIconList, index);
        if(index1 != -1) ImageList_Remove(ShellBigIconList, index1);
        ret = INVALID_INDEX;
        goto leave;
    }

    if (index!=index1)
    {
        FIXME("iconlists out of sync 0x%x 0x%x\n", index, index1);
        /* What to do ???? */
    }
    lpsice->dwListIndex = index;
    ret = lpsice->dwListIndex;

leave:
    if(ret == INVALID_INDEX)
    {
        if(indexDPA != -1) DPA_DeletePtr(sic_hdpa, indexDPA);
        HeapFree(GetProcessHeap(), 0, lpsice->sSourceFile);
        SHFree(lpsice);
    }
    LeaveCriticalSection(&SHELL32_SicCS);
    return ret;
}
/****************************************************************************
 * SIC_LoadIcon                [internal]
 *
 * NOTES
 *  gets small/big icon by number from a file
 */
static INT SIC_LoadIcon (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags)
{
    HICON hiconLarge=0;
    HICON hiconSmall=0;
    UINT ret;

    PrivateExtractIconsW(sSourceFile, dwSourceIndex, 32, 32, &hiconLarge, NULL, 1, LR_COPYFROMRESOURCE);
    PrivateExtractIconsW(sSourceFile, dwSourceIndex, 16, 16, &hiconSmall, NULL, 1, LR_COPYFROMRESOURCE);

    if ( !hiconLarge ||  !hiconSmall)
    {
        WARN("failure loading icon %i from %s (%p %p)\n", dwSourceIndex, debugstr_w(sSourceFile), hiconLarge, hiconSmall);
        if(hiconLarge) DestroyIcon(hiconLarge);
        if(hiconSmall) DestroyIcon(hiconSmall);
        return INVALID_INDEX;
    }

    if (0 != (dwFlags & GIL_FORSHORTCUT))
    {
        HICON hiconLargeShortcut = SIC_OverlayShortcutImage(hiconLarge, TRUE);
        HICON hiconSmallShortcut = SIC_OverlayShortcutImage(hiconSmall, FALSE);
        if (NULL != hiconLargeShortcut && NULL != hiconSmallShortcut)
        {
            DestroyIcon(hiconLarge);
            DestroyIcon(hiconSmall);
            hiconLarge = hiconLargeShortcut;
            hiconSmall = hiconSmallShortcut;
        }
        else
        {
            WARN("Failed to create shortcut overlayed icons\n");
            if (NULL != hiconLargeShortcut) DestroyIcon(hiconLargeShortcut);
            if (NULL != hiconSmallShortcut) DestroyIcon(hiconSmallShortcut);
            dwFlags &= ~ GIL_FORSHORTCUT;
        }
    }

    ret = SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge, dwFlags);
    DestroyIcon(hiconLarge);
    DestroyIcon(hiconSmall);
    return ret;
}
/*****************************************************************************
 * SIC_GetIconIndex            [internal]
 *
 * Parameters
 *    sSourceFile    [IN]    filename of file containing the icon
 *    index        [IN]    index/resID (negated) in this file
 *
 * NOTES
 *  look in the cache for a proper icon. if not available the icon is taken
 *  from the file and cached
 */
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags )
{
    SIC_ENTRY sice;
    INT ret, index = INVALID_INDEX;
    WCHAR path[MAX_PATH];

    TRACE("%s %i\n", debugstr_w(sSourceFile), dwSourceIndex);

    GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
    sice.sSourceFile = path;
    sice.dwSourceIndex = dwSourceIndex;
    sice.dwFlags = dwFlags;

    if (!sic_hdpa)
        SIC_Initialize();

    EnterCriticalSection(&SHELL32_SicCS);

    if (NULL != DPA_GetPtr (sic_hdpa, 0))
    {
      /* search linear from position 0*/
      index = DPA_Search (sic_hdpa, &sice, 0, SIC_CompareEntries, 0, DPAS_SORTED);
    }

    if ( INVALID_INDEX == index )
    {
          ret = SIC_LoadIcon (sSourceFile, dwSourceIndex, dwFlags);
    }
    else
    {
      TRACE("-- found\n");
      ret = ((LPSIC_ENTRY)DPA_GetPtr(sic_hdpa, index))->dwListIndex;
    }

    LeaveCriticalSection(&SHELL32_SicCS);
    return ret;
}

/*****************************************************************************
 * SIC_Initialize            [internal]
 */
BOOL SIC_Initialize(void)
{
    HICON hSm = NULL, hLg = NULL;
    INT cx_small, cy_small;
    INT cx_large, cy_large;
    HDC hDC;
    INT bpp;
    DWORD ilMask;
    BOOL result = FALSE;

    TRACE("Entered SIC_Initialize\n");

    if (sic_hdpa)
    {
        TRACE("Icon cache already initialized\n");
        return TRUE;
    }

    sic_hdpa = DPA_Create(16);
    if (!sic_hdpa)
    {
        return FALSE;
    }

    hDC = CreateICW(L"DISPLAY", NULL, NULL, NULL);
    if (!hDC)
    {
        ERR("Failed to create information context (error %d)\n", GetLastError());
        goto end;
    }

    bpp = GetDeviceCaps(hDC, BITSPIXEL);
    DeleteDC(hDC);

    if (bpp <= 4)
        ilMask = ILC_COLOR4;
    else if (bpp <= 8)
        ilMask = ILC_COLOR8;
    else if (bpp <= 16)
        ilMask = ILC_COLOR16;
    else if (bpp <= 24)
        ilMask = ILC_COLOR24;
    else if (bpp <= 32)
        ilMask = ILC_COLOR32;
    else
        ilMask = ILC_COLOR;

    ilMask |= ILC_MASK;

    cx_small = GetSystemMetrics(SM_CXSMICON);
    cy_small = GetSystemMetrics(SM_CYSMICON);
    cx_large = GetSystemMetrics(SM_CXICON);
    cy_large = GetSystemMetrics(SM_CYICON);

    ShellSmallIconList = ImageList_Create(cx_small,
                                          cy_small,
                                          ilMask,
                                          100,
                                          100);
    if (!ShellSmallIconList)
    {
        ERR("Failed to create the small icon list.\n");
        goto end;
    }

    ShellBigIconList = ImageList_Create(cx_large,
                                        cy_large,
                                        ilMask,
                                        100,
                                        100);
    if (!ShellBigIconList)
    {
        ERR("Failed to create the big icon list.\n");
        goto end;
    }

    /* Load the document icon, which is used as the default if an icon isn't found. */
    hSm = (HICON)LoadImageW(shell32_hInstance,
                            MAKEINTRESOURCEW(IDI_SHELL_DOCUMENT),
                            IMAGE_ICON,
                            cx_small,
                            cy_small,
                            LR_SHARED | LR_DEFAULTCOLOR);
    if (!hSm)
    {
        ERR("Failed to load small IDI_SHELL_DOCUMENT icon!\n");
        goto end;
    }

    hLg = (HICON)LoadImageW(shell32_hInstance,
                            MAKEINTRESOURCEW(IDI_SHELL_DOCUMENT),
                            IMAGE_ICON,
                            cx_large,
                            cy_large,
                            LR_SHARED | LR_DEFAULTCOLOR);
    if (!hLg)
    {
        ERR("Failed to load large IDI_SHELL_DOCUMENT icon!\n");
        goto end;
    }

    if(SIC_IconAppend(swShell32Name, IDI_SHELL_DOCUMENT-1, hSm, hLg, 0) == INVALID_INDEX)
    {
        ERR("Failed to add IDI_SHELL_DOCUMENT icon to cache.\n");
        goto end;
    }
    if(SIC_IconAppend(swShell32Name, -IDI_SHELL_DOCUMENT, hSm, hLg, 0) == INVALID_INDEX)
    {
        ERR("Failed to add IDI_SHELL_DOCUMENT icon to cache.\n");
        goto end;
    }

    /* Everything went fine */
    result = TRUE;

end:
    /* The image list keeps a copy of the icons, we must destroy them */
    if(hSm) DestroyIcon(hSm);
    if(hLg) DestroyIcon(hLg);

    /* Clean everything if something went wrong */
    if(!result)
    {
        if(sic_hdpa) DPA_Destroy(sic_hdpa);
        if(ShellSmallIconList) ImageList_Destroy(ShellSmallIconList);
        if(ShellBigIconList) ImageList_Destroy(ShellSmallIconList);
        sic_hdpa = NULL;
        ShellSmallIconList = NULL;
        ShellBigIconList = NULL;
    }

    TRACE("hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

    return result;
}

/*************************************************************************
 * SIC_Destroy
 *
 * frees the cache
 */
static INT CALLBACK sic_free( LPVOID ptr, LPVOID lparam )
{
    HeapFree(GetProcessHeap(), 0, ((LPSIC_ENTRY)ptr)->sSourceFile);
    SHFree(ptr);
    return TRUE;
}

void SIC_Destroy(void)
{
    TRACE("\n");

    EnterCriticalSection(&SHELL32_SicCS);

    if (sic_hdpa) DPA_DestroyCallback(sic_hdpa, sic_free, NULL );

    sic_hdpa = NULL;
    ImageList_Destroy(ShellSmallIconList);
    ShellSmallIconList = 0;
    ImageList_Destroy(ShellBigIconList);
    ShellBigIconList = 0;

    LeaveCriticalSection(&SHELL32_SicCS);
    //DeleteCriticalSection(&SHELL32_SicCS); //static
}

/*****************************************************************************
 * SIC_LoadOverlayIcon            [internal]
 *
 * Load a shell overlay icon and return its icon cache index.
 */
static int SIC_LoadOverlayIcon(int icon_idx)
{
    WCHAR buffer[1024], wszIdx[8];
    HKEY hKeyShellIcons;
    LPCWSTR iconPath;
    int iconIdx;

    iconPath = swShell32Name;    /* default: load icon from shell32.dll */
    iconIdx = icon_idx;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons",
                      0, KEY_READ, &hKeyShellIcons) == ERROR_SUCCESS)
    {
        DWORD count = sizeof(buffer);

        swprintf(wszIdx, L"%d", icon_idx);

        /* read icon path and index */
        if (RegQueryValueExW(hKeyShellIcons, wszIdx, NULL, NULL, (LPBYTE)buffer, &count) == ERROR_SUCCESS)
        {
            LPWSTR p = wcschr(buffer, ',');

            if (p)
                *p++ = 0;

            iconPath = buffer;
            iconIdx = _wtoi(p);
        }

        RegCloseKey(hKeyShellIcons);
    }

    if (!sic_hdpa)
        SIC_Initialize();

    return SIC_LoadIcon(iconPath, iconIdx, 0);
}

/*************************************************************************
 * Shell_GetImageLists            [SHELL32.71]
 *
 * PARAMETERS
 *  imglist[1|2] [OUT] pointer which receives imagelist handles
 *
 */
BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList)
{
    TRACE("(%p,%p)\n",lpBigList,lpSmallList);

    if (!sic_hdpa)
        SIC_Initialize();

    if (lpBigList)
        *lpBigList = ShellBigIconList;

    if (lpSmallList)
        *lpSmallList = ShellSmallIconList;

    return TRUE;
}
/*************************************************************************
 * PidlToSicIndex            [INTERNAL]
 *
 * PARAMETERS
 *    sh    [IN]    IShellFolder
 *    pidl    [IN]
 *    bBigIcon [IN]
 *    uFlags    [IN]    GIL_*
 *    pIndex    [OUT]    index within the SIC
 *
 */
BOOL PidlToSicIndex (
    IShellFolder * sh,
    LPCITEMIDLIST pidl,
    BOOL bBigIcon,
    UINT uFlags,
    int * pIndex)
{
    CComPtr<IExtractIconW>        ei;
    WCHAR        szIconFile[MAX_PATH];    /* file containing the icon */
    INT        iSourceIndex;        /* index or resID(negated) in this file */
    BOOL        ret = FALSE;
    UINT        dwFlags = 0;
    int        iShortcutDefaultIndex = INVALID_INDEX;

    TRACE("sf=%p pidl=%p %s\n", sh, pidl, bBigIcon?"Big":"Small");

    if (!sic_hdpa)
        SIC_Initialize();

    if (SUCCEEDED (sh->GetUIObjectOf(0, 1, &pidl, IID_NULL_PPV_ARG(IExtractIconW, &ei))))
    {
      if (SUCCEEDED(ei->GetIconLocation(uFlags &~ GIL_FORSHORTCUT, szIconFile, MAX_PATH, &iSourceIndex, &dwFlags)))
      {
        *pIndex = SIC_GetIconIndex(szIconFile, iSourceIndex, uFlags);
        ret = TRUE;
      }
    }

    if (INVALID_INDEX == *pIndex)    /* default icon when failed */
    {
      if (0 == (uFlags & GIL_FORSHORTCUT))
      {
        *pIndex = 0;
      }
      else
      {
        if (INVALID_INDEX == iShortcutDefaultIndex)
        {
          iShortcutDefaultIndex = SIC_LoadIcon(swShell32Name, 0, GIL_FORSHORTCUT);
        }
        *pIndex = (INVALID_INDEX != iShortcutDefaultIndex ? iShortcutDefaultIndex : 0);
      }
    }

    return ret;

}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex    [SHELL32.77]
 *
 * PARAMETERS
 *    sh    [IN]        pointer to an instance of IShellFolder
 *    pidl    [IN]
 *    pIndex    [OUT][OPTIONAL]    SIC index for big icon
 *
 */
int WINAPI SHMapPIDLToSystemImageListIndex(
    IShellFolder *sh,
    LPCITEMIDLIST pidl,
    int *pIndex)
{
    int Index;
    UINT uGilFlags = 0;

    TRACE("(SF=%p,pidl=%p,%p)\n",sh,pidl,pIndex);
    pdump(pidl);

    if (SHELL_IsShortcut(pidl))
        uGilFlags |= GIL_FORSHORTCUT;

    if (pIndex)
        if (!PidlToSicIndex ( sh, pidl, 1, uGilFlags, pIndex))
            *pIndex = -1;

    if (!PidlToSicIndex ( sh, pidl, 0, uGilFlags, &Index))
        return -1;

    return Index;
}

/*************************************************************************
 * SHMapIDListToImageListIndexAsync  [SHELL32.148]
 */
EXTERN_C HRESULT WINAPI SHMapIDListToImageListIndexAsync(IShellTaskScheduler *pts, IShellFolder *psf,
                                                LPCITEMIDLIST pidl, UINT flags,
                                                PFNASYNCICONTASKBALLBACK pfn, void *pvData, void *pvHint,
                                                int *piIndex, int *piIndexSel)
{
    FIXME("(%p, %p, %p, 0x%08x, %p, %p, %p, %p, %p)\n",
            pts, psf, pidl, flags, pfn, pvData, pvHint, piIndex, piIndexSel);
    return E_FAIL;
}

/*************************************************************************
 * Shell_GetCachedImageIndex        [SHELL32.72]
 *
 */
INT WINAPI Shell_GetCachedImageIndexA(LPCSTR szPath, INT nIndex, UINT bSimulateDoc)
{
    INT ret, len;
    LPWSTR szTemp;

    WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_a(szPath), nIndex, bSimulateDoc);

    len = MultiByteToWideChar( CP_ACP, 0, szPath, -1, NULL, 0 );
    szTemp = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, szPath, -1, szTemp, len );

    ret = SIC_GetIconIndex( szTemp, nIndex, 0 );

    HeapFree( GetProcessHeap(), 0, szTemp );

    return ret;
}

INT WINAPI Shell_GetCachedImageIndexW(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc)
{
    WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_w(szPath), nIndex, bSimulateDoc);

    return SIC_GetIconIndex(szPath, nIndex, 0);
}

EXTERN_C INT WINAPI Shell_GetCachedImageIndexAW(LPCVOID szPath, INT nIndex, BOOL bSimulateDoc)
{    if( SHELL_OsIsUnicode())
      return Shell_GetCachedImageIndexW((LPCWSTR)szPath, nIndex, bSimulateDoc);
    return Shell_GetCachedImageIndexA((LPCSTR)szPath, nIndex, bSimulateDoc);
}

EXTERN_C INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc)
{
    return Shell_GetCachedImageIndexAW(szPath, nIndex, bSimulateDoc);
}

/*************************************************************************
 * ExtractIconExW            [SHELL32.@]
 * RETURNS
 *  0 no icon found (or the file is not valid)
 *  or number of icons extracted
 */
UINT WINAPI ExtractIconExW(LPCWSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
{
    UINT ret = 0;

    /* get entry point of undocumented function PrivateExtractIconExW() in user32 */
#if defined(__CYGWIN__) || defined (__MINGW32__) || defined(_MSC_VER)
    static UINT (WINAPI*PrivateExtractIconExW)(LPCWSTR,int,HICON*,HICON*,UINT) = NULL;

    if (!PrivateExtractIconExW) {
        HMODULE hUser32 = GetModuleHandleA("user32");
        PrivateExtractIconExW = (UINT(WINAPI*)(LPCWSTR,int,HICON*,HICON*,UINT)) GetProcAddress(hUser32, "PrivateExtractIconExW");

        if (!PrivateExtractIconExW)
        return ret;
    }
#endif

    TRACE("%s %i %p %p %i\n", debugstr_w(lpszFile), nIconIndex, phiconLarge, phiconSmall, nIcons);
    ret = PrivateExtractIconExW(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);

    /* PrivateExtractIconExW() may return -1 if the provided file is not a valid PE image file or the said
     * file couldn't be found. The behaviour is correct although ExtractIconExW() only returns the successfully
     * extracted icons from a file. In such scenario, simply return 0.
    */
    if (ret == 0xFFFFFFFF)
    {
        WARN("Invalid file or couldn't be found - %s\n", debugstr_w(lpszFile));
        ret = 0;
    }

    return ret;
}

/*************************************************************************
 * ExtractIconExA            [SHELL32.@]
 */
UINT WINAPI ExtractIconExA(LPCSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
{
    UINT ret = 0;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, NULL, 0);
    LPWSTR lpwstrFile = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    TRACE("%s %i %p %p %i\n", lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);

    if (lpwstrFile)
    {
        MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, lpwstrFile, len);
        ret = ExtractIconExW(lpwstrFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
        HeapFree(GetProcessHeap(), 0, lpwstrFile);
    }
    return ret;
}

/*************************************************************************
 *                ExtractAssociatedIconA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hIcon = NULL;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpIconPath, -1, NULL, 0);
    /* Note that we need to allocate MAX_PATH, since we are supposed to fill
     * the correct executable if there is no icon in lpIconPath directly.
     * lpIconPath itself is supposed to be large enough, so make sure lpIconPathW
     * is large enough too. Yes, I am puking too.
     */
    LPWSTR lpIconPathW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));

    TRACE("%p %s %p\n", hInst, debugstr_a(lpIconPath), lpiIcon);

    if (lpIconPathW)
    {
        MultiByteToWideChar(CP_ACP, 0, lpIconPath, -1, lpIconPathW, len);
        hIcon = ExtractAssociatedIconW(hInst, lpIconPathW, lpiIcon);
        WideCharToMultiByte(CP_ACP, 0, lpIconPathW, -1, lpIconPath, MAX_PATH , NULL, NULL);
        HeapFree(GetProcessHeap(), 0, lpIconPathW);
    }
    return hIcon;
}

/*************************************************************************
 *                ExtractAssociatedIconW (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hIcon = NULL;
    WORD wDummyIcon = 0;

    TRACE("%p %s %p\n", hInst, debugstr_w(lpIconPath), lpiIcon);

    if(lpiIcon == NULL)
        lpiIcon = &wDummyIcon;

    hIcon = ExtractIconW(hInst, lpIconPath, *lpiIcon);

    if( hIcon < (HICON)2 )
    { if( hIcon == (HICON)1 ) /* no icons found in given file */
      { WCHAR tempPath[MAX_PATH];
        HINSTANCE uRet = FindExecutableW(lpIconPath,NULL,tempPath);

        if( uRet > (HINSTANCE)32 && tempPath[0] )
        { wcscpy(lpIconPath,tempPath);
          hIcon = ExtractIconW(hInst, lpIconPath, *lpiIcon);
          if( hIcon > (HICON)2 )
            return hIcon;
        }
      }

      if( hIcon == (HICON)1 )
        *lpiIcon = 2;   /* MSDOS icon - we found .exe but no icons in it */
      else
        *lpiIcon = 6;   /* generic icon - found nothing */

      if (GetModuleFileNameW(hInst, lpIconPath, MAX_PATH))
        hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(*lpiIcon));
    }
    return hIcon;
}

/*************************************************************************
 *                ExtractAssociatedIconExW (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
EXTERN_C HICON WINAPI ExtractAssociatedIconExW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIconIdx, LPWORD lpiIconId)
{
  FIXME("%p %s %p %p): stub\n", hInst, debugstr_w(lpIconPath), lpiIconIdx, lpiIconId);
  return 0;
}

/*************************************************************************
 *                ExtractAssociatedIconExA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
EXTERN_C HICON WINAPI ExtractAssociatedIconExA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIconIdx, LPWORD lpiIconId)
{
  HICON ret;
  INT len = MultiByteToWideChar( CP_ACP, 0, lpIconPath, -1, NULL, 0 );
  LPWSTR lpwstrFile = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

  TRACE("%p %s %p %p)\n", hInst, lpIconPath, lpiIconIdx, lpiIconId);

  MultiByteToWideChar( CP_ACP, 0, lpIconPath, -1, lpwstrFile, len );
  ret = ExtractAssociatedIconExW(hInst, lpwstrFile, lpiIconIdx, lpiIconId);
  HeapFree(GetProcessHeap(), 0, lpwstrFile);
  return ret;
}


/****************************************************************************
 * SHDefExtractIconW        [SHELL32.@]
 */
HRESULT WINAPI SHDefExtractIconW(LPCWSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
    UINT ret;
    HICON hIcons[2];
    WARN("%s %d 0x%08x %p %p %d, semi-stub\n", debugstr_w(pszIconFile), iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);

    ret = PrivateExtractIconsW(pszIconFile, iIndex, nIconSize, nIconSize, hIcons, NULL, 2, LR_DEFAULTCOLOR);
    /* FIXME: deal with uFlags parameter which contains GIL_ flags */
    if (ret == 0xFFFFFFFF)
      return E_FAIL;
    if (ret > 0) {
      if (phiconLarge)
        *phiconLarge = hIcons[0];
      else
        DestroyIcon(hIcons[0]);
      if (phiconSmall)
        *phiconSmall = hIcons[1];
      else
        DestroyIcon(hIcons[1]);
      return S_OK;
    }
    return S_FALSE;
}

/****************************************************************************
 * SHDefExtractIconA        [SHELL32.@]
 */
HRESULT WINAPI SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
  HRESULT ret;
  INT len = MultiByteToWideChar(CP_ACP, 0, pszIconFile, -1, NULL, 0);
  LPWSTR lpwstrFile = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

  TRACE("%s %d 0x%08x %p %p %d\n", pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);

  MultiByteToWideChar(CP_ACP, 0, pszIconFile, -1, lpwstrFile, len);
  ret = SHDefExtractIconW(lpwstrFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
  HeapFree(GetProcessHeap(), 0, lpwstrFile);
  return ret;
}

/****************************************************************************
 * SHGetIconOverlayIndexA    [SHELL32.@]
 *
 * Returns the index of the overlay icon in the system image list.
 */
EXTERN_C INT WINAPI SHGetIconOverlayIndexA(LPCSTR pszIconPath, INT iIconIndex)
{
  FIXME("%s, %d\n", debugstr_a(pszIconPath), iIconIndex);

  return -1;
}

/****************************************************************************
 * SHGetIconOverlayIndexW    [SHELL32.@]
 *
 * Returns the index of the overlay icon in the system image list.
 */
EXTERN_C INT WINAPI SHGetIconOverlayIndexW(LPCWSTR pszIconPath, INT iIconIndex)
{
  FIXME("%s, %d\n", debugstr_w(pszIconPath), iIconIndex);

  return -1;
}
