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

BackgroundItem g_backgroundItems[MAX_BACKGROUNDS];

DIBitmap *g_pWallpaperBitmap    = NULL;

int g_placementSelection        = 0;
int g_backgroundSelection       = 0;

/* this holds current selection of background color */
COLORREF g_backgroundDesktopColor = 0;
/* this holds selection of custom colors in dialog box */
COLORREF custom_colors[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

int g_listViewItemCount         = 0;

HWND g_hBackgroundPage          = NULL;
HWND g_hBackgroundList          = NULL;
HWND g_hBackgroundPreview       = NULL;

HWND g_hPlacementCombo          = NULL;
HWND g_hColorButton             = NULL;

HIMAGELIST g_hShellImageList    = NULL;

static HBITMAP hBitmap = NULL;
static int cxSource, cySource;

/* Add the images in the C:\ReactOS directory and the current wallpaper if any */
void AddListViewItems()
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

    GetClientRect(g_hBackgroundList, &clientRect);
    
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    
    (void)ListView_InsertColumn(g_hBackgroundList, 0, &dummy);
    
    /* Add the "None" item */
    backgroundItem = &g_backgroundItems[g_listViewItemCount];   
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
    listItem.iItem      = g_listViewItemCount;
    listItem.lParam     = g_listViewItemCount;
    
    (void)ListView_InsertItem(g_hBackgroundList, &listItem);
    ListView_SetItemState(g_hBackgroundList, g_listViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

    g_listViewItemCount++;

    /* Add current wallpaper if any */
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    result = RegQueryValueEx(regKey, TEXT("Wallpaper"), 0, &varType, (LPBYTE)wallpaperFilename, &bufferSize);
    
    if((result == ERROR_SUCCESS) && (_tcslen(wallpaperFilename) > 0))
    {
        himl = (HIMAGELIST)SHGetFileInfo(wallpaperFilename,
                                         0,
                                         &sfi,
                                         sizeof(sfi),
                                         SHGFI_SYSICONINDEX | SHGFI_SMALLICON |
                                         SHGFI_DISPLAYNAME);

        if(himl != NULL)
        {
            if(i++ == 0)
            {
                g_hShellImageList = himl;
                (void)ListView_SetImageList(g_hBackgroundList, himl, LVSIL_SMALL);
            }

            backgroundItem = &g_backgroundItems[g_listViewItemCount];
            
            backgroundItem->bWallpaper = TRUE;

            _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
            _tcscpy(backgroundItem->szFilename, wallpaperFilename);

            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            listItem.state      = LVIS_SELECTED;
            listItem.pszText    = backgroundItem->szDisplayName;
            listItem.iImage     = sfi.iIcon;
            listItem.iItem      = g_listViewItemCount;
            listItem.lParam     = g_listViewItemCount;

            (void)ListView_InsertItem(g_hBackgroundList, &listItem);
            ListView_SetItemState(g_hBackgroundList, g_listViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

            g_listViewItemCount++;
        }
    }
    
    RegCloseKey(regKey);

    /* Add all the images in the C:\ReactOS directory. */

    LoadString(hApplet, IDS_SUPPORTED_EXT, szFileTypes, sizeof(szFileTypes) / sizeof(TCHAR));
	

    token = _tcstok ( szFileTypes, separators );
    while ( token != NULL )
    {
        GetWindowsDirectory(szSearchPath, MAX_PATH);
        _tcscat(szSearchPath, TEXT("\\"));
        _tcscat(szSearchPath, token);
	    
        hFind = FindFirstFile(szSearchPath, &fd);
        while(hFind != INVALID_HANDLE_VALUE)
        {
            /* Don't add any hidden bitmaps */
            if((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
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

                if(himl == NULL) break;
	            
                if(i++ == 0)
                {
                    g_hShellImageList = himl;
                    (void)ListView_SetImageList(g_hBackgroundList, himl, LVSIL_SMALL);
                }

                backgroundItem = &g_backgroundItems[g_listViewItemCount];

                backgroundItem->bWallpaper = TRUE;
	            
                _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
                _tcscpy(backgroundItem->szFilename, filename);

                ZeroMemory(&listItem, sizeof(LV_ITEM));
                listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
                listItem.pszText    = backgroundItem->szDisplayName;
                listItem.state      = 0;
                listItem.iImage     = sfi.iIcon;
                listItem.iItem      = g_listViewItemCount;
                listItem.lParam     = g_listViewItemCount;
	            
                (void)ListView_InsertItem(g_hBackgroundList, &listItem);
	            
                g_listViewItemCount++;
            }
	        
            if(!FindNextFile(hFind, &fd))
                hFind = INVALID_HANDLE_VALUE;
        }

        token = _tcstok ( NULL, separators );
    }
}

void InitBackgroundDialog()
{
    TCHAR szString[256];
    HKEY regKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;
    BITMAP bitmap;
    
    g_backgroundDesktopColor = GetSysColor( COLOR_BACKGROUND );
	g_hBackgroundList       = GetDlgItem(g_hBackgroundPage, IDC_BACKGROUND_LIST);
    g_hBackgroundPreview    = GetDlgItem(g_hBackgroundPage, IDC_BACKGROUND_PREVIEW);
    g_hPlacementCombo       = GetDlgItem(g_hBackgroundPage, IDC_PLACEMENT_COMBO);
    g_hColorButton          = GetDlgItem(g_hBackgroundPage, IDC_COLOR_BUTTON);

    AddListViewItems();
    
    LoadString(hApplet, IDS_CENTER, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_CENTER, (LPARAM)szString);

    LoadString(hApplet, IDS_STRETCH, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_STRETCH, (LPARAM)szString);

    LoadString(hApplet, IDS_TILE, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_TILE, (LPARAM)szString);

    /* Load the default settings from the registry */
    result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
	if( result != ERROR_SUCCESS )
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

    if(result == ERROR_SUCCESS)
    {
        if(_ttoi(szBuffer) == 0)
        {
            SendMessage(g_hPlacementCombo, CB_SETCURSEL, PLACEMENT_CENTER, 0);
            g_placementSelection = PLACEMENT_CENTER;
        }
        
        if(_ttoi(szBuffer) == 2)
        {
            SendMessage(g_hPlacementCombo, CB_SETCURSEL, PLACEMENT_STRETCH, 0);
            g_placementSelection = PLACEMENT_STRETCH;
        }
    }
    else
    {
        SendMessage(g_hPlacementCombo, CB_SETCURSEL, PLACEMENT_CENTER, 0);
        g_placementSelection = PLACEMENT_CENTER;
    }
    
    result = RegQueryValueEx(regKey, TEXT("TileWallpaper"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);

    if(result == ERROR_SUCCESS)
    {
        if(_ttoi(szBuffer) == 1)
        {
            SendMessage(g_hPlacementCombo, CB_SETCURSEL, PLACEMENT_TILE, 0);
            g_placementSelection = PLACEMENT_TILE;
        }
    }

    RegCloseKey(regKey);

    hBitmap = LoadImage(hApplet, MAKEINTRESOURCE(IDC_MONITOR), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
    if (hBitmap != NULL)
    {
        GetObject(hBitmap, sizeof(BITMAP), &bitmap);

        cxSource = bitmap.bmWidth;
        cySource = bitmap.bmHeight;
    }
}

void OnColorButton()
{
    /* Load custom colors from Registry */
    HKEY hKey = NULL;
    LONG res = ERROR_SUCCESS;
    res = RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), 0, NULL, 0,
        KEY_ALL_ACCESS, NULL, &hKey, NULL );
    /* Now the key is either created or opened existing, if res == ERROR_SUCCESS */
    if( res == ERROR_SUCCESS )
    {
        /* Key opened */
        DWORD dwType = REG_BINARY;
        DWORD cbData = sizeof(custom_colors);
        res = RegQueryValueEx( hKey, TEXT("CustomColors"), NULL, &dwType, 
            (LPBYTE)custom_colors, &cbData );
        RegCloseKey( hKey );
        hKey = NULL;
    }
	
    /* Launch ChooseColor() dialog */
    CHOOSECOLOR cc;
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = g_hBackgroundPage;
    cc.hInstance = NULL;
    cc.rgbResult = g_backgroundDesktopColor;
    cc.lpCustColors = custom_colors;
    cc.Flags = CC_ANYCOLOR | /* Causes the dialog box to display all available colors in the set of basic colors.  */
               CC_FULLOPEN | /* opens dialog in full size */
               CC_RGBINIT ;  /* init chosen color by rgbResult value */
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;
    if( ChooseColor( &cc ) )
    {
        /* Save selected color to var */
        g_backgroundDesktopColor = cc.rgbResult;
        /* Allpy buuton will be activated */
        PropSheet_Changed( GetParent( g_hBackgroundPage ), g_hBackgroundPage );
        /* Window will be updated :) */
        InvalidateRect(g_hBackgroundPreview, NULL, TRUE);
        /* Save custom colors to reg. To this moment key must be ceated already. See above */
        res = RegOpenKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), 0,
            KEY_WRITE, &hKey );
        if( res == ERROR_SUCCESS )
        {
            /* Key opened */
            RegSetValueEx( hKey, TEXT("CustomColors"), 0, REG_BINARY, 
                (const BYTE *)custom_colors, sizeof(custom_colors) );
            RegCloseKey( hKey );
            hKey = NULL;
        }
    }
}

BOOL CheckListViewFilenameExists(HWND hWndList, LPCTSTR tszFileName)
{
    /* ListView_FindItem() Macro: Searches for a list-view item with the specified   *
     * characteristics. Returns the index of the item if successful, or -1 otherwise */
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_STRING; /* search item by EXACT string */
    lvfi.psz   = tszFileName; /* string to search */
    /* other items of this structure are not valid, besacuse flags are not set. */
    int retVal = ListView_FindItem( hWndList, -1, &lvfi );
    if( retVal != -1 ) return TRUE; /* item found! */
    return FALSE; /* item not found. */
}

void OnBrowseButton()
{
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];
    TCHAR fileTitle[256];
    TCHAR filter[MAX_PATH];
    BackgroundItem *backgroundItem = NULL;
    SHFILEINFO sfi;
    LV_ITEM listItem;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = g_hBackgroundPage;
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

    if(GetOpenFileName(&ofn) == TRUE)
    {
        /* Check if there is already a entry that holds this filename */
        if(CheckListViewFilenameExists(g_hBackgroundList, ofn.lpstrFileTitle) == TRUE)
            return;
        
        if(g_listViewItemCount > (MAX_BACKGROUNDS - 1))
            return;
        
        SHGetFileInfo(filename,
                      0,
                      &sfi,
                      sizeof(sfi),
                      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);
        
        backgroundItem = &g_backgroundItems[g_listViewItemCount];
        
        backgroundItem->bWallpaper = TRUE;
        
        _tcscpy(backgroundItem->szDisplayName, sfi.szDisplayName);
        _tcscpy(backgroundItem->szFilename, filename);
        
        ZeroMemory(&listItem, sizeof(LV_ITEM));
        listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        listItem.state      = 0;
        listItem.pszText    = backgroundItem->szDisplayName;
        listItem.iImage     = sfi.iIcon;
        listItem.iItem      = g_listViewItemCount;
        listItem.lParam     = g_listViewItemCount;
        
        (void)ListView_InsertItem(g_hBackgroundList, &listItem);
        
        g_listViewItemCount++;
    }
}

void ListViewItemChanged(int itemIndex)
{
    BackgroundItem *backgroundItem = NULL;
    
    g_backgroundSelection = itemIndex;
    backgroundItem = &g_backgroundItems[g_backgroundSelection];
    
    if(g_pWallpaperBitmap != NULL)
    {
        DibFreeImage(g_pWallpaperBitmap);
        g_pWallpaperBitmap = NULL;
    }
    
    if(backgroundItem->bWallpaper == TRUE)
    {   
        g_pWallpaperBitmap = DibLoadImage(backgroundItem->szFilename);
        
        if(g_pWallpaperBitmap == NULL)
        {
            return;
        }
    }
    
    InvalidateRect(g_hBackgroundPreview, NULL, TRUE);
    
    EnableWindow(g_hColorButton,    (backgroundItem->bWallpaper == FALSE ? TRUE : FALSE));
    EnableWindow(g_hPlacementCombo, backgroundItem->bWallpaper);
    
    PropSheet_Changed(GetParent(g_hBackgroundPage), g_hBackgroundPage);
}

void DrawBackgroundPreview(LPDRAWITEMSTRUCT draw)
{
    float scaleX;
    float scaleY;
    int scaledWidth;
    int scaledHeight;
    int posX;
    int posY;

    if(g_backgroundItems[g_backgroundSelection].bWallpaper == FALSE)
    {
        /* update desktop background color image */
    	HBRUSH hBrush = CreateSolidBrush( g_backgroundDesktopColor );
        FillRect(draw->hDC, &draw->rcItem, hBrush );
        DeleteObject( hBrush );
        return;
    }
    
    if(g_pWallpaperBitmap == NULL)
        return;

    scaleX = ((float)GetSystemMetrics(SM_CXSCREEN) - 1) / (float)draw->rcItem.right;
    scaleY = ((float)GetSystemMetrics(SM_CYSCREEN) - 1) / (float)draw->rcItem.bottom;

    scaledWidth = g_pWallpaperBitmap->width / scaleX;
    scaledHeight = g_pWallpaperBitmap->height / scaleY;
    
    posX = (draw->rcItem.right / 2) - (scaledWidth / 2);
    posY = (draw->rcItem.bottom / 2) - (scaledHeight / 2);
    
    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_BACKGROUND));
    
    SetStretchBltMode(draw->hDC, COLORONCOLOR);
    
    if(g_placementSelection == PLACEMENT_CENTER)
    {
        StretchDIBits(draw->hDC,
                      posX,
                      posY,
                      scaledWidth,
                      scaledHeight,
                      0,
                      0,
                      g_pWallpaperBitmap->width,
                      g_pWallpaperBitmap->height,
                      g_pWallpaperBitmap->bits,
                      g_pWallpaperBitmap->info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
    }
    
    if(g_placementSelection == PLACEMENT_STRETCH)
    {
        StretchDIBits(draw->hDC,
                      0,
                      0,
                      draw->rcItem.right,
                      draw->rcItem.bottom,
                      0,
                      0,
                      g_pWallpaperBitmap->width,
                      g_pWallpaperBitmap->height,
                      g_pWallpaperBitmap->bits,
                      g_pWallpaperBitmap->info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
    }

    if(g_placementSelection == PLACEMENT_TILE)
    {
        int x;
        int y;
        
        for(y = 0; y < draw->rcItem.bottom; y += scaledHeight)
        {
            for(x = 0; x < draw->rcItem.right; x += scaledWidth)
            {
                StretchDIBits(draw->hDC,
                              x,
                              y,
                              scaledWidth,
                              scaledHeight,
                              0,
                              0,
                              g_pWallpaperBitmap->width,
                              g_pWallpaperBitmap->height,
                              g_pWallpaperBitmap->bits,
                              g_pWallpaperBitmap->info,
                              DIB_RGB_COLORS,
                              SRCCOPY);
            }
        }
    }
}

void SetWallpaper()
{
    HKEY regKey;
    
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);

    if(g_placementSelection == PLACEMENT_TILE)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("1"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }
    
    if(g_placementSelection == PLACEMENT_CENTER)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
    }

    if(g_placementSelection == PLACEMENT_STRETCH)
    {
        RegSetValueEx(regKey, TEXT("TileWallpaper"), 0, REG_SZ, (BYTE *)TEXT("0"), sizeof(TCHAR) * 2);
        RegSetValueEx(regKey, TEXT("WallpaperStyle"), 0, REG_SZ, (BYTE *)TEXT("2"), sizeof(TCHAR) * 2);
    }
    
    RegCloseKey(regKey);
    
    if(g_backgroundItems[g_backgroundSelection].bWallpaper == TRUE)
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER,
                             0,
                             g_backgroundItems[g_backgroundSelection].szFilename,
                             SPIF_UPDATEINIFILE);
    }
    else
    {   
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, TEXT(""), SPIF_UPDATEINIFILE);
    }
}

void SetDesktopBackColor()
{
    /* Change system color */
    INT iElement = COLOR_BACKGROUND;
    if( !SetSysColors( 1, &iElement, &g_backgroundDesktopColor ) )
        MessageBox( g_hBackgroundPage, TEXT("SetSysColor() failed!"), /* these error texts can need internationalization? */
            TEXT("Error!"), MB_ICONSTOP );
    /* Write color to registry key: HKEY_CURRENT_USER\Control Panel\Colors\Background */
    HKEY hKey = NULL;
    LONG result = ERROR_SUCCESS;
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
    TCHAR clText[16] = {0};
    DWORD red   = GetRValue( g_backgroundDesktopColor );
    DWORD green = GetGValue( g_backgroundDesktopColor );
    DWORD blue  = GetBValue( g_backgroundDesktopColor );
    wsprintf( clText, TEXT("%d %d %d"), red, green, blue ); /* format string to be set to registry */
    RegSetValueEx( hKey, TEXT("Background"), 0, REG_SZ, (BYTE *)clText, lstrlen( clText )*sizeof(TCHAR) + sizeof(TCHAR) );
    RegCloseKey( hKey );
}

INT_PTR CALLBACK BackgroundPageProc(HWND hwndDlg,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    g_hBackgroundPage = hwndDlg;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            {
                InitBackgroundDialog();
            } break;
        
        case WM_COMMAND:
            {              
                DWORD controlId = LOWORD(wParam);
                DWORD command   = HIWORD(wParam);
                
                switch(controlId)
                {
                    case IDC_COLOR_BUTTON:
                        {
                            if(command == BN_CLICKED)
                                OnColorButton();
                            
                        } break;
                    
                    case IDC_BROWSE_BUTTON:
                        {
                            if(command == BN_CLICKED)
                                OnBrowseButton();
                            
                        } break;
                    
                    case IDC_PLACEMENT_COMBO:
                        {
                            if(command == CBN_SELCHANGE)
                            {
                                g_placementSelection = (int)SendMessage(g_hPlacementCombo, CB_GETCURSEL, 0, 0);
                                
                                InvalidateRect(g_hBackgroundPreview, NULL, TRUE);
                                
                                PropSheet_Changed(GetParent(g_hBackgroundPage), g_hBackgroundPage);
                            }
                            
                        } break;
                }
            } break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc, hdcMem;
       
                hdc = BeginPaint(hwndDlg, &ps);
 
                hdcMem = CreateCompatibleDC(hdc);
                SelectObject(hdcMem, hBitmap);

                TransparentBlt(hdc, 98, 0, cxSource, cySource, hdcMem, 0, 0, cxSource, cySource, 0xFF80FF);

                DeleteDC(hdcMem);
                EndPaint(hwndDlg, &ps);

            } break;

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT drawItem;
                drawItem = (LPDRAWITEMSTRUCT)lParam;

                if(drawItem->CtlID == IDC_BACKGROUND_PREVIEW)
                {
                    DrawBackgroundPreview(drawItem);
                }

            } break;
        
        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;
                
                switch(lpnm->code)
                {   
                    case PSN_APPLY:
                        {
                            SetWallpaper();
                            SetDesktopBackColor();
                            return TRUE;
                        } break;

                    case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;
                            
                            if((nm->uNewState & LVIS_SELECTED) == 0)
                                return FALSE;
                            
                            ListViewItemChanged(nm->iItem);

                        } break;
                    
                    default:
                        break;
                }
                
            } break;

        case WM_DESTROY:
            {
                if(g_pWallpaperBitmap != NULL)
                    DibFreeImage(g_pWallpaperBitmap);

                DeleteObject(hBitmap);

            } break;
    }
    
    return FALSE;
}

