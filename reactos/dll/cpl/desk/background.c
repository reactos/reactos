/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/background.c
 * PURPOSE:         Background property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Alexey Minnekhanov (minlexx@rambler.ru)
 */

#include "desk.h"

#define MAX_BACKGROUNDS     100

#define PLACEMENT_CENTER    0
#define PLACEMENT_STRETCH   1
#define PLACEMENT_TILE      2

typedef struct
{
    BOOL bWallpaper; /* Is this background a wallpaper */

    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];

} BackgroundItem;

typedef struct _GLOBAL_DATA
{
    BackgroundItem backgroundItems[MAX_BACKGROUNDS];

    PDIBITMAP pWallpaperBitmap;

    int placementSelection;
    int backgroundSelection;

    COLORREF backgroundDesktopColor;
    COLORREF custom_colors[16];

    int listViewItemCount;

    HBITMAP hBitmap;
    int cxSource;
    int cySource;
} GLOBAL_DATA, *PGLOBAL_DATA;



/* Add the images in the C:\ReactOS directory and the current wallpaper if any */
static VOID
AddListViewItems(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
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
    DWORD bufferSize = sizeof(wallpaperFilename);
    DWORD varType = REG_SZ;
    LONG result;
    UINT i = 0;
    BackgroundItem *backgroundItem = NULL;
    TCHAR separators[] = TEXT(";");
    TCHAR *token;
    HWND hwndBackgroundList;
    TCHAR *p;

    hwndBackgroundList = GetDlgItem(hwndDlg, IDC_BACKGROUND_LIST);

    GetClientRect(hwndBackgroundList, &clientRect);

    /* Add a new column to the list */
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hwndBackgroundList, 0, &dummy);

    /* Add the "None" item */
    backgroundItem = &pGlobalData->backgroundItems[pGlobalData->listViewItemCount];
    backgroundItem->bWallpaper = FALSE;
    LoadString(hApplet,
               IDS_NONE,
               backgroundItem->szDisplayName,
               sizeof(backgroundItem->szDisplayName) / sizeof(TCHAR));

    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.state      = LVIS_SELECTED;
    listItem.pszText    = backgroundItem->szDisplayName;
    listItem.iImage     = -1;
    listItem.iItem      = pGlobalData->listViewItemCount;
    listItem.lParam     = pGlobalData->listViewItemCount;

    (void)ListView_InsertItem(hwndBackgroundList, &listItem);
    ListView_SetItemState(hwndBackgroundList, pGlobalData->listViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

    pGlobalData->listViewItemCount++;

    /* Add current wallpaper if any */
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);

    result = RegQueryValueEx(regKey, TEXT("Wallpaper"), 0, &varType, (LPBYTE)wallpaperFilename, &bufferSize);
    if ((result == ERROR_SUCCESS) && (_tcslen(wallpaperFilename) > 0))
    {
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

            backgroundItem = &pGlobalData->backgroundItems[pGlobalData->listViewItemCount];

            backgroundItem->bWallpaper = TRUE;

            _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
            p = _tcsrchr(backgroundItem->szDisplayName, _T('.'));
            if (p)
                *p = (TCHAR)0;
            _tcscpy(backgroundItem->szFilename, wallpaperFilename);

            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            listItem.state      = LVIS_SELECTED;
            listItem.pszText    = backgroundItem->szDisplayName;
            listItem.iImage     = sfi.iIcon;
            listItem.iItem      = pGlobalData->listViewItemCount;
            listItem.lParam     = pGlobalData->listViewItemCount;

            (void)ListView_InsertItem(hwndBackgroundList, &listItem);
            ListView_SetItemState(hwndBackgroundList, pGlobalData->listViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

            pGlobalData->listViewItemCount++;
        }
    }

    RegCloseKey(regKey);

    /* Add all the images in the C:\ReactOS directory. */

    LoadString(hApplet, IDS_SUPPORTED_EXT, szFileTypes, sizeof(szFileTypes) / sizeof(TCHAR));

    token = _tcstok(szFileTypes, separators);
    while (token != NULL)
    {
        GetWindowsDirectory(szSearchPath, MAX_PATH);
        _tcscat(szSearchPath, TEXT("\\"));
        _tcscat(szSearchPath, token);

        hFind = FindFirstFile(szSearchPath, &fd);
        while (hFind != INVALID_HANDLE_VALUE)
        {
            /* Don't add any hidden bitmaps */
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
            {
                TCHAR filename[MAX_PATH];

                GetWindowsDirectory(filename, MAX_PATH);

                _tcscat(filename, TEXT("\\"));
                _tcscat(filename, fd.cFileName);

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

                backgroundItem = &pGlobalData->backgroundItems[pGlobalData->listViewItemCount];

                backgroundItem->bWallpaper = TRUE;

                _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
                p = _tcsrchr(backgroundItem->szDisplayName, _T('.'));
                if (p)
                    *p = (TCHAR)0;
                _tcscpy(backgroundItem->szFilename, filename);

                ZeroMemory(&listItem, sizeof(LV_ITEM));
                listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
                listItem.pszText    = backgroundItem->szDisplayName;
                listItem.state      = 0;
                listItem.iImage     = sfi.iIcon;
                listItem.iItem      = pGlobalData->listViewItemCount;
                listItem.lParam     = pGlobalData->listViewItemCount;

                (void)ListView_InsertItem(hwndBackgroundList, &listItem);

                pGlobalData->listViewItemCount++;
            }

            if(!FindNextFile(hFind, &fd))
                hFind = INVALID_HANDLE_VALUE;
        }

        token = _tcstok(NULL, separators);
    }
}


static VOID
InitBackgroundDialog(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    TCHAR szString[256];
    HKEY regKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;
    BITMAP bitmap;

    pGlobalData->backgroundDesktopColor = GetSysColor(COLOR_BACKGROUND);

    AddListViewItems(hwndDlg, pGlobalData);

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
        /* now the key must be created & opened and regKey points to opened key */
        /* On error result will not contain ERROR_SUCCESS. I don't know how to handle */
        /* this case :( */
    }

    result = RegQueryValueEx(regKey, TEXT("WallpaperStyle"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    if (result == ERROR_SUCCESS)
    {
        if (_ttoi(szBuffer) == 0)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_CENTER, 0);
            pGlobalData->placementSelection = PLACEMENT_CENTER;
        }

        if (_ttoi(szBuffer) == 2)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_STRETCH, 0);
            pGlobalData->placementSelection = PLACEMENT_STRETCH;
        }
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_CENTER, 0);
        pGlobalData->placementSelection = PLACEMENT_CENTER;
    }

    result = RegQueryValueEx(regKey, TEXT("TileWallpaper"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    if (result == ERROR_SUCCESS)
    {
        if (_ttoi(szBuffer) == 1)
        {
            SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_SETCURSEL, PLACEMENT_TILE, 0);
            pGlobalData->placementSelection = PLACEMENT_TILE;
        }
    }

    RegCloseKey(regKey);

    pGlobalData->hBitmap = (HBITMAP) LoadImage(hApplet, MAKEINTRESOURCE(IDC_MONITOR), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
    if (pGlobalData->hBitmap != NULL)
    {
        GetObject(pGlobalData->hBitmap, sizeof(BITMAP), &bitmap);

        pGlobalData->cxSource = bitmap.bmWidth;
        pGlobalData->cySource = bitmap.bmHeight;
    }
}


static VOID
OnColorButton(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
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
        DWORD cbData = sizeof(pGlobalData->custom_colors);
        res = RegQueryValueEx(hKey, TEXT("CustomColors"), NULL, &dwType,
            (LPBYTE)pGlobalData->custom_colors, &cbData);
        RegCloseKey(hKey);
        hKey = NULL;
    }

    /* Launch ChooseColor() dialog */

    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = hwndDlg;
    cc.hInstance = NULL;
    cc.rgbResult = pGlobalData->backgroundDesktopColor;
    cc.lpCustColors = pGlobalData->custom_colors;
    cc.Flags = CC_ANYCOLOR | /* Causes the dialog box to display all available colors in the set of basic colors.  */
               CC_FULLOPEN | /* opens dialog in full size */
               CC_RGBINIT ;  /* init chosen color by rgbResult value */
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;
    if (ChooseColor(&cc))
    {
        /* Save selected color to var */
        pGlobalData->backgroundDesktopColor = cc.rgbResult;

        /* Allpy buuton will be activated */
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

        /* Window will be updated :) */
        InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW), NULL, TRUE);

        /* Save custom colors to reg. To this moment key must be ceated already. See above */
        res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), 0,
            KEY_WRITE, &hKey);
        if (res == ERROR_SUCCESS)
        {
            /* Key opened */
            RegSetValueEx(hKey, TEXT("CustomColors"), 0, REG_BINARY,
                (const BYTE *)pGlobalData->custom_colors, sizeof(pGlobalData->custom_colors));
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

    lvfi.flags = LVFI_STRING; /* search item by EXACT string */
    lvfi.psz   = tszFileName; /* string to search */

    /* other items of this structure are not valid, besacuse flags are not set. */
    retVal = ListView_FindItem(hwndList, -1, &lvfi);
    if (retVal != -1)
        return TRUE; /* item found! */

    return FALSE; /* item not found. */
}


static VOID
OnBrowseButton(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];
    TCHAR fileTitle[256];
    TCHAR filter[MAX_PATH];
    BackgroundItem *backgroundItem = NULL;
    SHFILEINFO sfi;
    LV_ITEM listItem;
    HWND hwndBackgroundList;

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
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        /* Check if there is already a entry that holds this filename */
        if (CheckListViewFilenameExists(hwndBackgroundList, ofn.lpstrFileTitle) == TRUE)
            return;

        if (pGlobalData->listViewItemCount > (MAX_BACKGROUNDS - 1))
            return;

        SHGetFileInfo(filename,
                      0,
                      &sfi,
                      sizeof(sfi),
                      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);

        backgroundItem = &pGlobalData->backgroundItems[pGlobalData->listViewItemCount];

        backgroundItem->bWallpaper = TRUE;

        _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
        _tcscpy(backgroundItem->szFilename, filename);

        ZeroMemory(&listItem, sizeof(LV_ITEM));
        listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        listItem.state      = 0;
        listItem.pszText    = backgroundItem->szDisplayName;
        listItem.iImage     = sfi.iIcon;
        listItem.iItem      = pGlobalData->listViewItemCount;
        listItem.lParam     = pGlobalData->listViewItemCount;

        (void)ListView_InsertItem(hwndBackgroundList, &listItem);

        pGlobalData->listViewItemCount++;
    }
}


static VOID
ListViewItemChanged(HWND hwndDlg, PGLOBAL_DATA pGlobalData, int itemIndex)
{
    BackgroundItem *backgroundItem = NULL;

    pGlobalData->backgroundSelection = itemIndex;
    backgroundItem = &pGlobalData->backgroundItems[pGlobalData->backgroundSelection];

    if (pGlobalData->pWallpaperBitmap != NULL)
    {
        DibFreeImage(pGlobalData->pWallpaperBitmap);
        pGlobalData->pWallpaperBitmap = NULL;
    }

    if (backgroundItem->bWallpaper == TRUE)
    {
        pGlobalData->pWallpaperBitmap = DibLoadImage(backgroundItem->szFilename);

        if (pGlobalData->pWallpaperBitmap == NULL)
            return;
    }

    InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW),
                   NULL, TRUE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR_BUTTON),
                 (backgroundItem->bWallpaper == FALSE ? TRUE : FALSE));
    EnableWindow(GetDlgItem(hwndDlg, IDC_PLACEMENT_COMBO),
                 backgroundItem->bWallpaper);

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
}


static VOID
DrawBackgroundPreview(LPDRAWITEMSTRUCT draw, PGLOBAL_DATA pGlobalData)
{
    float scaleX;
    float scaleY;
    int scaledWidth;
    int scaledHeight;
    int posX;
    int posY;
    HBRUSH hBrush;
    int x;
    int y;

    if (pGlobalData->backgroundItems[pGlobalData->backgroundSelection].bWallpaper == FALSE)
    {
        /* update desktop background color image */
        hBrush = CreateSolidBrush(pGlobalData->backgroundDesktopColor);
        FillRect(draw->hDC, &draw->rcItem, hBrush);
        DeleteObject(hBrush);
        return;
    }

    if (pGlobalData->pWallpaperBitmap == NULL)
        return;

    scaleX = ((float)GetSystemMetrics(SM_CXSCREEN) - 1) / (float)draw->rcItem.right;
    scaleY = ((float)GetSystemMetrics(SM_CYSCREEN) - 1) / (float)draw->rcItem.bottom;

    scaledWidth = pGlobalData->pWallpaperBitmap->width / scaleX;
    scaledHeight = pGlobalData->pWallpaperBitmap->height / scaleY;

    posX = (draw->rcItem.right / 2) - (scaledWidth / 2);
    posY = (draw->rcItem.bottom / 2) - (scaledHeight / 2);

    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_BACKGROUND));

    SetStretchBltMode(draw->hDC, COLORONCOLOR);

    switch (pGlobalData->placementSelection)
    {
        case PLACEMENT_CENTER:
            StretchDIBits(draw->hDC,
                          posX,
                          posY,
                          scaledWidth,
                          scaledHeight,
                          0,
                          0,
                          pGlobalData->pWallpaperBitmap->width,
                          pGlobalData->pWallpaperBitmap->height,
                          pGlobalData->pWallpaperBitmap->bits,
                          pGlobalData->pWallpaperBitmap->info,
                          DIB_RGB_COLORS,
                          SRCCOPY);
            break;

        case PLACEMENT_STRETCH:
            StretchDIBits(draw->hDC,
                          0,
                          0,
                          draw->rcItem.right,
                          draw->rcItem.bottom,
                          0,
                          0,
                          pGlobalData->pWallpaperBitmap->width,
                          pGlobalData->pWallpaperBitmap->height,
                          pGlobalData->pWallpaperBitmap->bits,
                          pGlobalData->pWallpaperBitmap->info,
                          DIB_RGB_COLORS,
                          SRCCOPY);
            break;

        case PLACEMENT_TILE:
            for (y = 0; y < draw->rcItem.bottom; y += scaledHeight)
            {
                for (x = 0; x < draw->rcItem.right; x += scaledWidth)
                {
                    StretchDIBits(draw->hDC,
                                  x,
                                  y,
                                  scaledWidth,
                                  scaledHeight,
                                  0,
                                  0,
                                  pGlobalData->pWallpaperBitmap->width,
                                  pGlobalData->pWallpaperBitmap->height,
                                  pGlobalData->pWallpaperBitmap->bits,
                                  pGlobalData->pWallpaperBitmap->info,
                                  DIB_RGB_COLORS,
                                  SRCCOPY);
                }
            }
            break;
    }
}


static VOID
SetWallpaper(PGLOBAL_DATA pGlobalData)
{
    HKEY regKey;

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);

    if (pGlobalData->placementSelection == PLACEMENT_TILE)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("1"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }

    if (pGlobalData->placementSelection == PLACEMENT_CENTER)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }

    if (pGlobalData->placementSelection == PLACEMENT_STRETCH)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("2"), sizeof(TCHAR) * 2);
    }

    RegCloseKey(regKey);

    if (pGlobalData->backgroundItems[pGlobalData->backgroundSelection].bWallpaper == TRUE)
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER,
                             0,
                             pGlobalData->backgroundItems[pGlobalData->backgroundSelection].szFilename,
                             SPIF_UPDATEINIFILE);
    }
    else
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*) TEXT(""), SPIF_UPDATEINIFILE);
    }
}


/* Change system color */
static VOID
SetDesktopBackColor(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    INT iElement = COLOR_BACKGROUND;
    HKEY hKey;
    LONG result;
    TCHAR clText[16];
    DWORD red, green, blue;

    if( !SetSysColors( 1, &iElement, &pGlobalData->backgroundDesktopColor ) )
        MessageBox(hwndDlg, TEXT("SetSysColor() failed!"), /* these error texts can need internationalization? */
            TEXT("Error!"), MB_ICONSTOP );
    /* Write color to registry key: HKEY_CURRENT_USER\Control Panel\Colors\Background */
    hKey = NULL;
    result = ERROR_SUCCESS;
    result = RegOpenKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Colors"), 0, KEY_WRITE, &hKey );
    if( result != ERROR_SUCCESS )
    {
        /* Key open failed; maybe it does not exist? create it! */
        result = RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Colors"), 0, NULL, 0,
            KEY_ALL_ACCESS, NULL, &hKey, NULL );
        /* Now key must be created and opened and hKey must point at newly created key */
        /* On error result will not contain ERROR_SUCCESS. I don't know how to handle */
        /* this case :( */
    }
    red   = GetRValue(pGlobalData->backgroundDesktopColor);
    green = GetGValue(pGlobalData->backgroundDesktopColor);
    blue  = GetBValue(pGlobalData->backgroundDesktopColor);
    _stprintf(clText, TEXT("%d %d %d"), red, green, blue ); /* format string to be set to registry */
    RegSetValueEx(hKey, TEXT("Background"), 0, REG_SZ, (BYTE *)clText, lstrlen( clText )*sizeof(TCHAR) + sizeof(TCHAR) );
    RegCloseKey(hKey);
}


INT_PTR CALLBACK
BackgroundPageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (GLOBAL_DATA*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);
            InitBackgroundDialog(hwndDlg, pGlobalData);
            break;

        case WM_COMMAND:
            {
                DWORD controlId = LOWORD(wParam);
                DWORD command   = HIWORD(wParam);

                switch (controlId)
                {
                    case IDC_COLOR_BUTTON:
                        if (command == BN_CLICKED)
                            OnColorButton(hwndDlg, pGlobalData);
                        break;

                    case IDC_BROWSE_BUTTON:
                        if (command == BN_CLICKED)
                            OnBrowseButton(hwndDlg, pGlobalData);
                        break;

                    case IDC_PLACEMENT_COMBO:
                        if (command == CBN_SELCHANGE)
                        {
                            pGlobalData->placementSelection = (int)SendDlgItemMessage(hwndDlg, IDC_PLACEMENT_COMBO, CB_GETCURSEL, 0, 0);

                            InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUND_PREVIEW), NULL, TRUE);

                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }
                        break;
                }
            } break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc, hdcMem;

                hdc = BeginPaint(hwndDlg, &ps);

                hdcMem = CreateCompatibleDC(hdc);
                SelectObject(hdcMem, pGlobalData->hBitmap);
/*
                TransparentBlt(hdc, 98, 0,
                               pGlobalData->cxSource, pGlobalData->cySource, hdcMem, 0, 0,
                               pGlobalData->cxSource, pGlobalData->cySource, 0xFF80FF);
*/
                DeleteDC(hdcMem);
                EndPaint(hwndDlg, &ps);
            }
            break;

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT drawItem;
                drawItem = (LPDRAWITEMSTRUCT)lParam;

                if (drawItem->CtlID == IDC_BACKGROUND_PREVIEW)
                {
                    DrawBackgroundPreview(drawItem, pGlobalData);
                }

            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch(lpnm->code)
                {
                    case PSN_APPLY:
                        SetWallpaper(pGlobalData);
                        SetDesktopBackColor(hwndDlg, pGlobalData);
                        return TRUE;

                    case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;

                            if ((nm->uNewState & LVIS_SELECTED) == 0)
                                return FALSE;

                            ListViewItemChanged(hwndDlg, pGlobalData, nm->iItem);

                        } break;
                }
            }
            break;

        case WM_DESTROY:
            if (pGlobalData->pWallpaperBitmap != NULL)
                DibFreeImage(pGlobalData->pWallpaperBitmap);

            DeleteObject(pGlobalData->hBitmap);
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}

