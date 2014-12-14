/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/background.c
 * PURPOSE:         Background property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Alexey Minnekhanov (minlexx@rambler.ru)
 */

#include "desk.h"

#include <shellapi.h>

#define MAX_BACKGROUNDS     100

#define PLACEMENT_CENTER    0
#define PLACEMENT_STRETCH   1
#define PLACEMENT_TILE      2

/* The values in these macros are dependant on the
 * layout of the monitor image and they must be adjusted
 * if that image will be changed.
 */
#define MONITOR_LEFT        18
#define MONITOR_TOP         18
#define MONITOR_RIGHT       168
#define MONITOR_BOTTOM      128

#define MONITOR_WIDTH       (MONITOR_RIGHT-MONITOR_LEFT)
#define MONITOR_HEIGHT      (MONITOR_BOTTOM-MONITOR_TOP)

typedef struct
{
    BOOL bWallpaper; /* Is this background a wallpaper */

    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];

} BackgroundItem;

typedef struct _DATA
{
    BOOL bWallpaperChanged;
    BOOL bClrBackgroundChanged;

    BackgroundItem backgroundItems[MAX_BACKGROUNDS];

    PDIBITMAP pWallpaperBitmap;

    int placementSelection;
    int backgroundSelection;

    COLORREF custom_colors[16];

    int listViewItemCount;

    HBITMAP hBitmap;
    int cxSource;
    int cySource;
} DATA, *PDATA;

GLOBAL_DATA g_GlobalData;


/* Add the images in the C:\ReactOS directory and the current wallpaper if any */
static VOID
AddListViewItems(HWND hwndDlg, PDATA pData)
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szSearchPath[MAX_PATH];
    TCHAR szFileTypes[MAX_PATH];
    LV_ITEM listItem;
    LV_COLUMN dummy;
    RECT clientRect;
    HKEY regKey;
    SHFILEINFO sfi;
    HIMAGELIST himl;
    TCHAR wallpaperFilename[MAX_PATH];
    TCHAR originalWallpaper[MAX_PATH];
    DWORD bufferSize = sizeof(wallpaperFilename);
    TCHAR buffer[MAX_PATH];
    DWORD varType = REG_SZ;
    LONG result;
    UINT i = 0;
    BackgroundItem *backgroundItem = NULL;
    TCHAR separators[] = TEXT(";");
    TCHAR *token;
    HWND hwndBackgroundList;
    TCHAR *p;
    HRESULT hr;

    hwndBackgroundList = GetDlgItem(hwndDlg, IDC_BACKGROUND_LIST);

    GetClientRect(hwndBackgroundList, &clientRect);

    /* Add a new column to the list */
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hwndBackgroundList, 0, &dummy);

    /* Add the "None" item */
    backgroundItem = &pData->backgroundItems[pData->listViewItemCount];
    backgroundItem->bWallpaper = FALSE;
    LoadString(hApplet,
               IDS_NONE,
               backgroundItem->szDisplayName,
               sizeof(backgroundItem->szDisplayName) / sizeof(TCHAR));

    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.state      = 0;
    listItem.pszText    = backgroundItem->szDisplayName;
    listItem.iImage     = -1;
    listItem.iItem      = pData->listViewItemCount;
    listItem.lParam     = pData->listViewItemCount;

    (void)ListView_InsertItem(hwndBackgroundList, &listItem);
    ListView_SetItemState(hwndBackgroundList, 
                          pData->listViewItemCount,
                          LVIS_SELECTED,
                          LVIS_SELECTED);

    pData->listViewItemCount++;

    /* Add current wallpaper if any */
    result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    if (result == ERROR_SUCCESS)
    {
        result = RegQueryValueEx(regKey, TEXT("Wallpaper"), 0, &varType, (LPBYTE)wallpaperFilename, &bufferSize);
        if ((result == ERROR_SUCCESS) && (_tcslen(wallpaperFilename) > 0))
        {
            bufferSize = sizeof(originalWallpaper);
            result = RegQueryValueEx(regKey, TEXT("OriginalWallpaper"), 0, &varType, (LPBYTE)originalWallpaper, &bufferSize);

            /* If Wallpaper and OriginalWallpaper are the same, try to retrieve ConvertedWallpaper and use it instead of Wallpaper */
            if ((result == ERROR_SUCCESS) && (_tcslen(originalWallpaper) > 0) && (_tcsicmp(wallpaperFilename, originalWallpaper) == 0))
            {
                bufferSize = sizeof(originalWallpaper);
                result = RegQueryValueEx(regKey, TEXT("ConvertedWallpaper"), 0, &varType, (LPBYTE)originalWallpaper, &bufferSize);

                if ((result == ERROR_SUCCESS) && (_tcslen(originalWallpaper) > 0))
                {
                    hr = StringCbCopy(wallpaperFilename, sizeof(wallpaperFilename), originalWallpaper);
                }

                if (FAILED(hr))
                {
                    RegCloseKey(regKey);
                    return;
                }
            }

            /* Allow environment variables in file name */
            if (ExpandEnvironmentStrings(wallpaperFilename, buffer, MAX_PATH))
            {
                hr = StringCbCopy(wallpaperFilename, sizeof(wallpaperFilename), buffer);
                if (FAILED(hr))
                {
                    RegCloseKey(regKey);
                    return;
                }
            }

            himl = (HIMAGELIST)SHGetFileInfo(wallpaperFilename,
                                             0,
                                             &sfi,
                                             sizeof(sfi),
                                             SHGFI_SYSICONINDEX | SHGFI_SMALLICON |
                                             SHGFI_DISPLAYNAME);

            if (himl != NULL)
            {
                if (i++ == 0)
                {
                    (void)ListView_SetImageList(hwndBackgroundList, himl, LVSIL_SMALL);
                }

                backgroundItem = &pData->backgroundItems[pData->listViewItemCount];

                backgroundItem->bWallpaper = TRUE;

                hr = StringCbCopy(backgroundItem->szDisplayName, sizeof(backgroundItem->szDisplayName), sfi.szDisplayName);
                if (FAILED(hr))
                {
                    RegCloseKey(regKey);
                    return;
                }

                p = _tcsrchr(backgroundItem->szDisplayName, _T('.'));
                if (p)
                    *p = (TCHAR)0;

                hr = StringCbCopy(backgroundItem->szFilename, sizeof(backgroundItem->szFilename), wallpaperFilename);
                if (FAILED(hr))
                {
                    RegCloseKey(regKey);
                    return;
                }

                ZeroMemory(&listItem, sizeof(LV_ITEM));
                listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
                listItem.state      = 0;
                listItem.pszText    = backgroundItem->szDisplayName;
                listItem.iImage     = sfi.iIcon;
                listItem.iItem      = pData->listViewItemCount;
                listItem.lParam     = pData->listViewItemCount;

                (void)ListView_InsertItem(hwndBackgroundList, &listItem);
                ListView_SetItemState(hwndBackgroundList,
                                      pData->listViewItemCount,
                                      LVIS_SELECTED,
                                      LVIS_SELECTED);

                pData->listViewItemCount++;
            }
        }

        RegCloseKey(regKey);
    }

    /* Add all the images in the C:\ReactOS directory. */

    LoadString(hApplet, IDS_SUPPORTED_EXT, szFileTypes, sizeof(szFileTypes) / sizeof(TCHAR));

    token = _tcstok(szFileTypes, separators);
    while (token != NULL)
    {
        GetWindowsDirectory(szSearchPath, MAX_PATH);
        
        hr = StringCbCat(szSearchPath, sizeof(szSearchPath), TEXT("\\"));
        if (FAILED(hr))
            return;
        hr = StringCbCat(szSearchPath, sizeof(szSearchPath), token);
        if (FAILED(hr))
            return;

        hFind = FindFirstFile(szSearchPath, &fd);
        while (hFind != INVALID_HANDLE_VALUE)
        {
            TCHAR filename[MAX_PATH];

            GetWindowsDirectory(filename, MAX_PATH);

            hr = StringCbCat(filename, sizeof(filename), TEXT("\\"));
            if (FAILED(hr))
            {
                FindClose(hFind);
                return;
            }
            hr = StringCbCat(filename, sizeof(filename), fd.cFileName);
            if (FAILED(hr))
            {
                FindClose(hFind);
                return;
            }

            /* Don't add any hidden bitmaps. Also don't add current wallpaper once more. */
            if (((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) && (_tcsicmp(wallpaperFilename, filename) != 0))
            {
                himl = (HIMAGELIST)SHGetFileInfo(filename,
                                                0,
                                                &sfi,
                                                sizeof(sfi),
                                                SHGFI_SYSICONINDEX | SHGFI_SMALLICON |
                                                SHGFI_DISPLAYNAME);

                if (himl == NULL)
                    break;

                if (i++ == 0)
                {
                    (void)ListView_SetImageList(hwndBackgroundList, himl, LVSIL_SMALL);
                }

                backgroundItem = &pData->backgroundItems[pData->listViewItemCount];

                backgroundItem->bWallpaper = TRUE;

                hr = StringCbCopy(backgroundItem->szDisplayName, sizeof(backgroundItem->szDisplayName), sfi.szDisplayName);
                if (FAILED(hr))
                {
                    FindClose(hFind);
                    return;
                }
                p = _tcsrchr(backgroundItem->szDisplayName, _T('.'));
                if (p)
                    *p = (TCHAR)0;
                hr = StringCbCopy(backgroundItem->szFilename, sizeof(backgroundItem->szFilename), filename);
                if (FAILED(hr))
                {
                    FindClose(hFind);
                    return;
                }

                ZeroMemory(&listItem, sizeof(LV_ITEM));
                listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
                listItem.pszText    = backgroundItem->szDisplayName;
                listItem.state      = 0;
                listItem.iImage     = sfi.iIcon;
                listItem.iItem      = pData->listViewItemCount;
                listItem.lParam     = pData->listViewItemCount;

                (void)ListView_InsertItem(hwndBackgroundList, &listItem);

                pData->listViewItemCount++;
            }

            if(!FindNextFile(hFind, &fd))
                break;
        }

        token = _tcstok(NULL, separators);
        FindClose(hFind);
    }
}


static VOID
InitBackgroundDialog(HWND hwndDlg, PDATA pData)
{
    TCHAR szString[256];
    HKEY regKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;
    BITMAP bitmap;

    AddListViewItems(hwndDlg, pData);

    LoadString(hApplet, IDS_CENTER, szString, sizeof(szString) / sizeof(TCHAR));
    SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_INSERTSTRING, PLACEMENT_CENTER, (LPARAM)szString);

    LoadString(hApplet, IDS_STRETCH, szString, sizeof(szString) / sizeof(TCHAR));
    SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_INSERTSTRING, PLACEMENT_STRETCH, (LPARAM)szString);

    LoadString(hApplet, IDS_TILE, szString, sizeof(szString) / sizeof(TCHAR));
    SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_INSERTSTRING, PLACEMENT_TILE, (LPARAM)szString);

    /* Load the default settings from the registry */
    result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    if (result != ERROR_SUCCESS)
    {
        /* reg key open failed; maybe it does not exist? create it! */
        DWORD dwDisposition = 0;
        result = RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, NULL, 0, KEY_ALL_ACCESS, NULL,
            &regKey, &dwDisposition );
        /* Now the key must be created & opened and regKey points to opened key */
        /* On error result will not contain ERROR_SUCCESS. I don't know how to handle */
        /* this case :( */
    }

    result = RegQueryValueEx(regKey, TEXT("WallpaperStyle"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    if (result == ERROR_SUCCESS)
    {
        if (_ttoi(szBuffer) == 0)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_CENTER, 0);
            pData->placementSelection = PLACEMENT_CENTER;
        }

        if (_ttoi(szBuffer) == 2)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_STRETCH, 0);
            pData->placementSelection = PLACEMENT_STRETCH;
        }
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_CENTER, 0);
        pData->placementSelection = PLACEMENT_CENTER;
    }

    result = RegQueryValueEx(regKey, TEXT("TileWallpaper"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    if (result == ERROR_SUCCESS)
    {
        if (_ttoi(szBuffer) == 1)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_TILE, 0);
            pData->placementSelection = PLACEMENT_TILE;
        }
    }

    RegCloseKey(regKey);

    pData->hBitmap = (HBITMAP) LoadImage(hApplet, MAKEINTRESOURCE(IDC_MONITOR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    if (pData->hBitmap != NULL)
    {
        GetObject(pData->hBitmap, sizeof(BITMAP), &bitmap);

        pData->cxSource = bitmap.bmWidth;
        pData->cySource = bitmap.bmHeight;
    }
}


static VOID
OnColorButton(HWND hwndDlg, PDATA pData)
{
    /* Load custom colors from Registry */
    HKEY hKey = NULL;
    LONG res = ERROR_SUCCESS;
    CHOOSECOLOR cc;

    res = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), 0, NULL, 0,
        KEY_ALL_ACCESS, NULL, &hKey, NULL);
    /* Now the key is either created or opened existing, if res == ERROR_SUCCESS */
    if (res == ERROR_SUCCESS)
    {
        /* Key opened */
        DWORD dwType = REG_BINARY;
        DWORD cbData = sizeof(pData->custom_colors);
        res = RegQueryValueEx(hKey, TEXT("CustomColors"), NULL, &dwType,
            (LPBYTE)pData->custom_colors, &cbData);
        RegCloseKey(hKey);
        hKey = NULL;
    }

    /* Launch ChooseColor() dialog */

    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = hwndDlg;
    cc.hInstance = NULL;
    cc.rgbResult = g_GlobalData.desktop_color;
    cc.lpCustColors = pData->custom_colors;
    cc.Flags = CC_ANYCOLOR | /* Causes the dialog box to display all available colors in the set of basic colors.  */
               CC_FULLOPEN | /* opens dialog in full size */
               CC_RGBINIT ;  /* init chosen color by rgbResult value */
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;
    if (ChooseColor(&cc))
    {
        /* Save selected color to var */
        g_GlobalData.desktop_color = cc.rgbResult;
        pData->bClrBackgroundChanged = TRUE;

        /* Apply button will be activated */
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

        /* Window will be updated :) */
        InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW), NULL, TRUE);

        /* Save custom colors to reg. To this moment key must be created already. See above */
        res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), 0,
            KEY_WRITE, &hKey);
        if (res == ERROR_SUCCESS)
        {
            /* Key opened */
            RegSetValueEx(hKey, TEXT("CustomColors"), 0, REG_BINARY,
                (const BYTE *)pData->custom_colors, sizeof(pData->custom_colors));
            RegCloseKey(hKey);
            hKey = NULL;
        }
    }
}


/*
 * ListView_FindItem() Macro: Searches for a list-view item with the specified
 * characteristics. Returns the index of the item if successful, or -1 otherwise
 */
static BOOL
CheckListViewFilenameExists(HWND hwndList, LPCTSTR tszFileName)
{
    LVFINDINFO lvfi;
    int retVal;

    lvfi.flags = LVFI_STRING; /* Search item by EXACT string */
    lvfi.psz   = tszFileName; /* String to search */

    /* Other items of this structure are not valid, besacuse flags are not set. */
    retVal = ListView_FindItem(hwndList, -1, &lvfi);
    if (retVal != -1)
        return TRUE; /* item found! */

    return FALSE; /* item not found. */
}


static VOID
OnBrowseButton(HWND hwndDlg, PDATA pData)
{
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];
    TCHAR fileTitle[256];
    TCHAR filter[MAX_PATH];
    BackgroundItem *backgroundItem = NULL;
    SHFILEINFO sfi;
    LV_ITEM listItem;
    HWND hwndBackgroundList;
    TCHAR *p;
    HRESULT hr;

    hwndBackgroundList = GetDlgItem(hwndDlg, IDC_BACKGROUND_LIST);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFile = filename;

    LoadString(hApplet, IDS_BACKGROUND_COMDLG_FILTER, filter, sizeof(filter) / sizeof(TCHAR));

    /* Set lpstrFile[0] to '\0' so that GetOpenFileName does not
     * use the contents of szFile to initialize itself */
    ofn.lpstrFile[0] = TEXT('\0');
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = fileTitle;
    ofn.nMaxFileTitle = 256;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        /* Check if there is already a entry that holds this filename */
        if (CheckListViewFilenameExists(hwndBackgroundList, ofn.lpstrFileTitle) == TRUE)
            return;

        if (pData->listViewItemCount > (MAX_BACKGROUNDS - 1))
            return;

        SHGetFileInfo(filename,
                      0,
                      &sfi,
                      sizeof(sfi),
                      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);

        backgroundItem = &pData->backgroundItems[pData->listViewItemCount];

        backgroundItem->bWallpaper = TRUE;

        hr = StringCbCopy(backgroundItem->szDisplayName, sizeof(backgroundItem->szDisplayName), sfi.szDisplayName);
        if (FAILED(hr))
            return;
        p = _tcsrchr(backgroundItem->szDisplayName, _T('.'));
        if (p)
            *p = (TCHAR)0;
        hr = StringCbCopy(backgroundItem->szFilename, sizeof(backgroundItem->szFilename), filename);
        if (FAILED(hr))
            return;

        ZeroMemory(&listItem, sizeof(LV_ITEM));
        listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        listItem.state      = 0;
        listItem.pszText    = backgroundItem->szDisplayName;
        listItem.iImage     = sfi.iIcon;
        listItem.iItem      = pData->listViewItemCount;
        listItem.lParam     = pData->listViewItemCount;

        (void)ListView_InsertItem(hwndBackgroundList, &listItem);
        ListView_SetItemState(hwndBackgroundList,
                              pData->listViewItemCount,
                              LVIS_SELECTED,
                              LVIS_SELECTED);
        SendMessage(hwndBackgroundList, WM_VSCROLL, SB_BOTTOM, 0);

        pData->listViewItemCount++;
    }
}


static VOID
ListViewItemChanged(HWND hwndDlg, PDATA pData, int itemIndex)
{
    BackgroundItem *backgroundItem = NULL;

    pData->backgroundSelection = itemIndex;
    backgroundItem = &pData->backgroundItems[pData->backgroundSelection];

    if (pData->pWallpaperBitmap != NULL)
    {
        DibFreeImage(pData->pWallpaperBitmap);
        pData->pWallpaperBitmap = NULL;
    }

    if (backgroundItem->bWallpaper == TRUE)
    {
        pData->pWallpaperBitmap = DibLoadImage(backgroundItem->szFilename);

        if (pData->pWallpaperBitmap == NULL)
            return;
    }

    pData->bWallpaperChanged = TRUE;

    InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW),
                   NULL, TRUE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_PLACEMENT_COMBO),
                 backgroundItem->bWallpaper);

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
}


static VOID
DrawBackgroundPreview(LPDRAWITEMSTRUCT draw, PDATA pData)
{
    float scaleX;
    float scaleY;
    int scaledWidth;
    int scaledHeight;
    int posX, desX;
    int posY, desY;
    HBRUSH hBrush;
    int x;
    int y;
    HDC hDC;
    HGDIOBJ hOldObj;
    RECT rcItem = {
        MONITOR_LEFT,
        MONITOR_TOP,
        MONITOR_RIGHT,
        MONITOR_BOTTOM
    };

    hDC = CreateCompatibleDC(draw->hDC);
    hOldObj = SelectObject(hDC, pData->hBitmap);

    if (pData->backgroundItems[pData->backgroundSelection].bWallpaper == FALSE)
    {
        /* Update desktop background color image */
        hBrush = CreateSolidBrush(g_GlobalData.desktop_color);
        FillRect(hDC, &rcItem, hBrush);
        DeleteObject(hBrush);
    }
    else
    if (pData->pWallpaperBitmap != NULL)
    {
        scaleX = ((float)GetSystemMetrics(SM_CXSCREEN) - 1) / (float)MONITOR_WIDTH;
        scaleY = ((float)GetSystemMetrics(SM_CYSCREEN) - 1) / (float)MONITOR_HEIGHT;

        scaledWidth = (int)(pData->pWallpaperBitmap->width / scaleX);
        scaledHeight = (int)(pData->pWallpaperBitmap->height / scaleY);

        FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_BACKGROUND));

        SetStretchBltMode(hDC, COLORONCOLOR);

        switch (pData->placementSelection)
        {
            case PLACEMENT_CENTER:
                posX = (MONITOR_WIDTH - scaledWidth + 1) / 2;
                posY = (MONITOR_HEIGHT - scaledHeight + 1) / 2;
                desX = 0;
                desY = 0;

                if (posX < 0) { desX = -posX / 2; posX = 0; }
                if (posY < 0) { desY = -posY / 2; posY = 0; }

                if (scaledWidth > MONITOR_WIDTH)
                    scaledWidth = MONITOR_WIDTH;

                if (scaledHeight > MONITOR_HEIGHT)
                    scaledHeight = MONITOR_HEIGHT;

                StretchDIBits(hDC,
                              MONITOR_LEFT+posX,
                              MONITOR_TOP+posY,
                              scaledWidth,
                              scaledHeight,
                              desX,
                              desY,
                              pData->pWallpaperBitmap->width - (int)(desX * scaleX),
                              pData->pWallpaperBitmap->height - (int)(desY * scaleY),
                              pData->pWallpaperBitmap->bits,
                              pData->pWallpaperBitmap->info,
                              DIB_RGB_COLORS,
                              SRCCOPY);
                break;

            case PLACEMENT_STRETCH:
                StretchDIBits(hDC,
                              MONITOR_LEFT,
                              MONITOR_TOP,
                              MONITOR_WIDTH,
                              MONITOR_HEIGHT,
                              0,
                              0,
                              pData->pWallpaperBitmap->width,
                              pData->pWallpaperBitmap->height,
                              pData->pWallpaperBitmap->bits,
                              pData->pWallpaperBitmap->info,
                              DIB_RGB_COLORS,
                              SRCCOPY);
                break;

            case PLACEMENT_TILE:
                for (y = 0; y < MONITOR_HEIGHT; y += scaledHeight)
                {
                    for (x = 0; x < MONITOR_WIDTH; x += scaledWidth)
                    {
                        if ((MONITOR_WIDTH-x) >= scaledWidth)
                            posX = scaledWidth;
                        else
                            posX = MONITOR_WIDTH-x;


                        if ((MONITOR_HEIGHT-y) >= scaledHeight)
                            posY = scaledHeight;
                        else
                            posY = MONITOR_HEIGHT-y;

                        StretchDIBits(hDC,
                                      MONITOR_LEFT + x,
                                      MONITOR_TOP + y,
                                      posX,
                                      posY,
                                      0,
                                      0,
                                      pData->pWallpaperBitmap->width * posX / scaledWidth,
                                      pData->pWallpaperBitmap->height * posY / scaledHeight,
                                      pData->pWallpaperBitmap->bits,
                                      pData->pWallpaperBitmap->info,
                                      DIB_RGB_COLORS,
                                      SRCCOPY);
                    }

                }

                break;
        }
    }

    GdiTransparentBlt(draw->hDC,
                      draw->rcItem.left, draw->rcItem.top,
                      draw->rcItem.right-draw->rcItem.left+1,
                      draw->rcItem.bottom-draw->rcItem.top+1,
                      hDC,
                      0, 0,
                      pData->cxSource, pData->cySource,
                      0xFF00FF);

    SelectObject(hDC, hOldObj);
    DeleteDC(hDC);
}


static VOID
SetWallpaper(PDATA pData)
{
    HKEY regKey;

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);

    if (pData->placementSelection == PLACEMENT_TILE)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("1"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }

    if (pData->placementSelection == PLACEMENT_CENTER)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }

    if (pData->placementSelection == PLACEMENT_STRETCH)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("2"), sizeof(TCHAR) * 2);
    }

    RegCloseKey(regKey);

    if (pData->backgroundItems[pData->backgroundSelection].bWallpaper == TRUE)
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER,
                             0,
                             pData->backgroundItems[pData->backgroundSelection].szFilename,
                             SPIF_UPDATEINIFILE);
    }
    else
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*) TEXT(""), SPIF_UPDATEINIFILE);
    }
}


/* Change system color */
static VOID
SetDesktopBackColor(HWND hwndDlg, DATA *pData)
{
    INT iElement = COLOR_BACKGROUND;
    HKEY hKey;
    LONG result;
    TCHAR clText[16];
    BYTE red, green, blue;
    DWORD dwDispostion;

    if( !SetSysColors( 1, &iElement, &g_GlobalData.desktop_color ) )
        MessageBox(hwndDlg, TEXT("SetSysColor() failed!"), /* these error texts can need internationalization? */
            TEXT("Error!"), MB_ICONSTOP );

    result = RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Colors"), 0, NULL, 0,
        KEY_ALL_ACCESS, NULL, &hKey, &dwDispostion );
    if (result != ERROR_SUCCESS)
    {
        red   = GetRValue(g_GlobalData.desktop_color);
        green = GetGValue(g_GlobalData.desktop_color);
        blue  = GetBValue(g_GlobalData.desktop_color);
        /* Format string to be set to registry */
        StringCbPrintf(clText, sizeof(clText), TEXT("%d %d %d"), red, green, blue);
        RegSetValueEx(hKey, TEXT("Background"), 0, REG_SZ, (BYTE *)clText,
                      (lstrlen(clText) + 1) * sizeof(TCHAR));
        RegCloseKey(hKey);
    }
}

INT_PTR CALLBACK
BackgroundPageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PDATA pData;

    pData = (PDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pData = (DATA*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pData);
            InitBackgroundDialog(hwndDlg, pData);
            break;

        case WM_COMMAND:
            {
                DWORD controlId = LOWORD(wParam);
                DWORD command   = HIWORD(wParam);

                switch (controlId)
                {
                    case IDC_COLOR_BUTTON:
                        if (command == BN_CLICKED)
                            OnColorButton(hwndDlg, pData);
                        break;

                    case IDC_BROWSE_BUTTON:
                        if (command == BN_CLICKED)
                            OnBrowseButton(hwndDlg, pData);
                        break;

                    case IDC_PLACEMENT_COMBO:
                        if (command == CBN_SELCHANGE)
                        {
                            pData->placementSelection = (int)SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_GETCURSEL, 0, 0);

                            InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW), NULL, TRUE);

                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }
                        break;
                }
            } break;

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT drawItem;
                drawItem = (LPDRAWITEMSTRUCT)lParam;

                if (drawItem->CtlID == IDC_BACKGROUND_PREVIEW)
                {
                    DrawBackgroundPreview(drawItem, pData);
                }

            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch(lpnm->code)
                {
                    case PSN_APPLY:
                        if(pData->bWallpaperChanged)
                            SetWallpaper(pData);
                        if(pData->bClrBackgroundChanged)
                            SetDesktopBackColor(hwndDlg, pData);
                        SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)_T(""));
                        return TRUE;

                    case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;

                            if ((nm->uNewState & LVIS_SELECTED) == 0)
                                return FALSE;

                            ListViewItemChanged(hwndDlg, pData, nm->iItem);
                        }
                        break;
                }
            }
            break;

        case WM_DESTROY:
            if (pData->pWallpaperBitmap != NULL)
                DibFreeImage(pData->pWallpaperBitmap);

            DeleteObject(pData->hBitmap);
            HeapFree(GetProcessHeap(), 0, pData);
            break;
    }

    return FALSE;
}
