/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/background.c
 * PURPOSE:         Background property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cpl.h>
#include <tchar.h>

#include "resource.h"

#include "desk.h"
#include "dibitmap.h"

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

int g_listViewItemCount         = 0;

HWND g_hBackgroundPage          = NULL;
HWND g_hBackgroundList          = NULL;
HWND g_hBackgroundPreview       = NULL;

HWND g_hPlacementCombo          = NULL;
HWND g_hColorButton             = NULL;

HIMAGELIST g_hShellImageList    = NULL;

/* Add the bitmaps in the C:\ReactOS directory and the current wallpaper if any */
void AddListViewItems()
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szSearchPath[MAX_PATH];
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

    GetClientRect(g_hBackgroundList, &clientRect);
    
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    
    ListView_InsertColumn(g_hBackgroundList, 0, &dummy);

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
    
    ListView_InsertItem(g_hBackgroundList, &listItem);
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
                ListView_SetImageList(g_hBackgroundList, himl, LVSIL_SMALL);
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

            ListView_InsertItem(g_hBackgroundList, &listItem);
            ListView_SetItemState(g_hBackgroundList, g_listViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

            g_listViewItemCount++;
        }
    }
    
    RegCloseKey(regKey);

    /* Add all the bitmaps in the C:\ReactOS directory. */

    GetWindowsDirectory(szSearchPath, MAX_PATH);
    _tcscat(szSearchPath, TEXT("\\*.bmp"));
    
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

            if(himl == NULL)
            {
                break;
            }
            
            if(i++ == 0)
            {
                g_hShellImageList = himl;
                ListView_SetImageList(g_hBackgroundList, himl, LVSIL_SMALL);
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
            
            ListView_InsertItem(g_hBackgroundList, &listItem);
            
            g_listViewItemCount++;
        }
        
        if(!FindNextFile(hFind, &fd))
            hFind = INVALID_HANDLE_VALUE;
    }
}

void InitBackgroundDialog()
{
    g_hBackgroundList       = GetDlgItem(g_hBackgroundPage, IDC_BACKGROUND_LIST);
    g_hBackgroundPreview    = GetDlgItem(g_hBackgroundPage, IDC_BACKGROUND_PREVIEW);
    g_hPlacementCombo       = GetDlgItem(g_hBackgroundPage, IDC_PLACEMENT_COMBO);
    g_hColorButton          = GetDlgItem(g_hBackgroundPage, IDC_COLOR_BUTTON);

    AddListViewItems();
    
    TCHAR szString[256];
    
    LoadString(hApplet, IDS_CENTER, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_CENTER, (LPARAM)szString);

    LoadString(hApplet, IDS_STRETCH, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_STRETCH, (LPARAM)szString);

    LoadString(hApplet, IDS_TILE, szString, sizeof(szString) / sizeof(TCHAR));
    SendMessage(g_hPlacementCombo, CB_INSERTSTRING, PLACEMENT_TILE, (LPARAM)szString);

    /* Load the default settings from the registry */
    HKEY regKey;
    
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;
    
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    
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
}

void OnColorButton()
{
    MessageBox(NULL, TEXT("That button doesn't do anything yet"), TEXT("Whoops"), MB_OK);
}

BOOL CheckListBoxFilename(HWND list, TCHAR *filename)
{
    return FALSE;
}

void OnBrowseButton()
{
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];
    TCHAR fileTitle[256];
    BackgroundItem *backgroundItem = NULL;
        
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = g_hBackgroundPage;
    ofn.lpstrFile = filename;
        
    /* Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
     * use the contents of szFile to initialize itself */
    ofn.lpstrFile[0] = TEXT('\0');
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = TEXT("Bitmap Files (*.bmp)\0*.bmp\0");
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = fileTitle;
    ofn.nMaxFileTitle = 256;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if(GetOpenFileName(&ofn) == TRUE)
    {   
        /* Check if there is already a entry that holds this filename */
        if(CheckListBoxFilename(g_hBackgroundList, filename) == TRUE)
            return;
        
        SHFILEINFO sfi;
        LV_ITEM listItem;
        
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
        
        ListView_InsertItem(g_hBackgroundList, &listItem);
        
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
    
    EnableWindow(g_hColorButton, (backgroundItem->bWallpaper == FALSE ? TRUE : FALSE));
    EnableWindow(g_hPlacementCombo, backgroundItem->bWallpaper);
    
    PropSheet_Changed(GetParent(g_hBackgroundPage), g_hBackgroundPage);
}

void DrawBackgroundPreview(LPDRAWITEMSTRUCT draw)
{
    if(g_backgroundItems[g_backgroundSelection].bWallpaper == FALSE)
    {
        FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_BACKGROUND));
        return;
    }
    
    if(g_pWallpaperBitmap == NULL)
        return;

    float scaleX = ((float)GetSystemMetrics(SM_CXSCREEN) - 1) / (float)draw->rcItem.right;
    float scaleY = ((float)GetSystemMetrics(SM_CYSCREEN) - 1) / (float)draw->rcItem.bottom;

    int scaledWidth = g_pWallpaperBitmap->width / scaleX;
    int scaledHeight = g_pWallpaperBitmap->height / scaleY;
    
    int posX = (draw->rcItem.right / 2) - (scaledWidth / 2);
    int posY = (draw->rcItem.bottom / 2) - (scaledHeight / 2);
    
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
                                g_placementSelection = SendMessage(g_hPlacementCombo, CB_GETCURSEL, 0, 0);
                                
                                InvalidateRect(g_hBackgroundPreview, NULL, TRUE);
                                
                                PropSheet_Changed(GetParent(g_hBackgroundPage), g_hBackgroundPage);
                            }
                            
                        } break;
                }
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

            } break;
    }
    
    return FALSE;
}

