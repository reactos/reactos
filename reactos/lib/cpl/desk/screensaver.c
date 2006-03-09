/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/screensaver.c
 * PURPOSE:         Screen saver property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include <cpl.h>
#include <tchar.h>
#include "desk.h"

#define MAX_SCREENSAVERS 100

void AddListViewItems2();
void CheckRegScreenSaverIsSecure();

typedef struct {
    BOOL  bIsScreenSaver; /* Is this background a wallpaper */
    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];
} ScreenSaverItem;

int ImageListSelection       = 0;
ScreenSaverItem g_ScreenSaverItems[MAX_SCREENSAVERS];

HWND g_hScreenBackgroundPage          = NULL;
HWND g_hScreengroundList          = NULL;
HWND ControlScreenSaverIsSecure = NULL;

void ListViewItemAreChanged(int itemIndex)
{
    ScreenSaverItem *ScreenSaverItem = NULL;
    
    ImageListSelection = itemIndex;
    ScreenSaverItem = &g_ScreenSaverItems[ImageListSelection];
    
    PropSheet_Changed(GetParent(g_hScreenBackgroundPage), g_hScreenBackgroundPage);
}

INT_PTR
CALLBACK
ScreenSaverPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    g_hScreenBackgroundPage = hwndDlg;
    
    switch(uMsg) {
        case WM_INITDIALOG:
        {
            g_hScreengroundList = GetDlgItem(g_hScreenBackgroundPage, IDC_SCREENS_CHOICES);
            AddListViewItems2();
            CheckRegScreenSaverIsSecure();
        } break;
        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);
 
            switch(controlId) {
                case IDC_SCREENS_POWER_BUTTON: // Start Powercfg.Cpl
                {
                    if(command == BN_CLICKED)
                        WinExec("rundll32 shell32.dll,Control_RunDLL powercfg.cpl,,",SW_SHOWNORMAL);
                } break;
                case IDC_SCREENS_TESTSC: // Screensaver Preview
                {
                    if(command == BN_CLICKED)
                        MessageBox(NULL, TEXT("That button doesn't do anything yet"), TEXT("Whoops"), MB_OK);
                        
                } break;
                case IDC_SCREENS_DELETE: // Delete Screensaver
                {
                    if(command == BN_CLICKED) {
                        if (ImageListSelection == 0) // Can NOT delete None sry:-)
                           return FALSE;
                        DeleteFileW(g_ScreenSaverItems[ImageListSelection].szFilename);
                    }
                } break;
                case IDC_SCREENS_SETTINGS: // Screensaver Settings
                {
                    if(command == BN_CLICKED)
                        MessageBox(NULL, TEXT("That button doesn't do anything yet"), TEXT("Whoops"), MB_OK);
                } break;
                case IDC_SCREENS_USEPASSCHK: // Screensaver Is Secure
                {
                    if(command == BN_CLICKED)
                        MessageBox(NULL, TEXT("That button doesn't do anything yet"), TEXT("Whoops"), MB_OK);
                } break;
                case IDC_SCREENS_TIME: // Delay before show screensaver
                {
                } break;
                default:
                    break;
            } break;
        }
        case WM_NOTIFY:
        {
           LPNMHDR lpnm = (LPNMHDR)lParam;
                
           switch(lpnm->code) {   
             case PSN_APPLY:
             {
                return TRUE;
             } break;
             case LVN_ITEMCHANGED:
             {
                LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;
                if((nm->uNewState & LVIS_SELECTED) == 0)
                  return FALSE;
                ListViewItemAreChanged(nm->iItem);
                break;
             } break;
            default:
                break;
           }
        } break;
    }
    
    return FALSE;
}

void CheckRegScreenSaverIsSecure()
{
    HKEY hKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;
    
    ControlScreenSaverIsSecure = GetDlgItem(g_hScreenBackgroundPage, IDC_SCREENS_USEPASSCHK);

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &hKey);
    result = RegQueryValueEx(hKey, TEXT("ScreenSaverIsSecure"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    if(result == ERROR_SUCCESS)
        if(_ttoi(szBuffer) == 1) {
          SendMessage(ControlScreenSaverIsSecure, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
          goto End;
        }
    SendMessage(ControlScreenSaverIsSecure, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
End:
    RegCloseKey(hKey);
}

/* Add the bitmaps in the C:\ReactOS directory and the current wallpaper if any */
void AddListViewItems2()
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
    HIMAGELIST g_hScreenShellImageList    = NULL;
    TCHAR wallpaperFilename[MAX_PATH];
    DWORD bufferSize = sizeof(wallpaperFilename);
    DWORD varType = REG_SZ;
    LONG result;
    UINT i = 0;
    int g_ScreenlistViewItemCount         = 0;
    ScreenSaverItem *ScreenSaverItem = NULL;
    
    GetClientRect(g_hScreengroundList, &clientRect);
    
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    
    ListView_InsertColumn(g_hScreengroundList, 0, &dummy);

    /* Add the "None" item */
    ScreenSaverItem = &g_ScreenSaverItems[g_ScreenlistViewItemCount];
    
    ScreenSaverItem->bIsScreenSaver = FALSE;

    LoadString(hApplet,
               IDS_NONE,
               ScreenSaverItem->szDisplayName,
               sizeof(ScreenSaverItem->szDisplayName) / sizeof(TCHAR));
    
    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.state      = LVIS_SELECTED;
    listItem.pszText    = ScreenSaverItem->szDisplayName;
    listItem.iImage     = -1;
    listItem.iItem      = g_ScreenlistViewItemCount;
    listItem.lParam     = g_ScreenlistViewItemCount;
    
    ListView_InsertItem(g_hScreengroundList, &listItem);
    ListView_SetItemState(g_hScreengroundList, g_ScreenlistViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

    g_ScreenlistViewItemCount++;

    /* Add current screensaver if any */
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    
    result = RegQueryValueEx(regKey, TEXT("SCRNSAVE.EXE"), 0, &varType, (LPBYTE)wallpaperFilename, &bufferSize);
    
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
                g_hScreenShellImageList = himl;
                ListView_SetImageList(g_hScreengroundList, himl, LVSIL_SMALL);
            }

            ScreenSaverItem = &g_ScreenSaverItems[g_ScreenlistViewItemCount];
            
            ScreenSaverItem->bIsScreenSaver = TRUE;

            _tcscpy(ScreenSaverItem->szDisplayName, sfi.szDisplayName);
            _tcscpy(ScreenSaverItem->szFilename, wallpaperFilename);

            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            listItem.state      = LVIS_SELECTED;
            listItem.pszText    = ScreenSaverItem->szDisplayName;
            listItem.iImage     = sfi.iIcon;
            listItem.iItem      = g_ScreenlistViewItemCount;
            listItem.lParam     = g_ScreenlistViewItemCount;

            ListView_InsertItem(g_hScreengroundList, &listItem);
            ListView_SetItemState(g_hScreengroundList, g_ScreenlistViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

            g_ScreenlistViewItemCount++;
        }
    }
    
    RegCloseKey(regKey);

    /* Add all the screensavers in the C:\ReactOS\System32 directory. */

    GetSystemDirectory(szSearchPath, MAX_PATH);
    _tcscat(szSearchPath, TEXT("\\*.scr"));
    
    hFind = FindFirstFile(szSearchPath, &fd);
    while(hFind != INVALID_HANDLE_VALUE)
    {
        /* Don't add any hidden screensavers */
        if((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
        {
            TCHAR filename[MAX_PATH];
            
            GetSystemDirectory(filename, MAX_PATH);

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
                g_hScreenShellImageList = himl;
                ListView_SetImageList(g_hScreengroundList, himl, LVSIL_SMALL);
            }

            ScreenSaverItem = &g_ScreenSaverItems[g_ScreenlistViewItemCount];

            ScreenSaverItem->bIsScreenSaver = TRUE;
            
            _tcscpy(ScreenSaverItem->szDisplayName, sfi.szDisplayName);
            _tcscpy(ScreenSaverItem->szFilename, filename);

            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            listItem.pszText    = ScreenSaverItem->szDisplayName;
            listItem.state      = 0;
            listItem.iImage     = sfi.iIcon;
            listItem.iItem      = g_ScreenlistViewItemCount;
            listItem.lParam     = g_ScreenlistViewItemCount;
            
            ListView_InsertItem(g_hScreengroundList, &listItem);
            
            g_ScreenlistViewItemCount++;
        }
        
        if(!FindNextFile(hFind, &fd))
            hFind = INVALID_HANDLE_VALUE;
    }
}

