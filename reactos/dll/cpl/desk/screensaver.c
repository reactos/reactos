/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/screensaver.c
 * PURPOSE:         Screen saver property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"


#define MAX_SCREENSAVERS 100

typedef struct
{
    BOOL  bIsScreenSaver; /* Is this background a wallpaper */
    TCHAR szFilename[MAX_PATH];
    TCHAR szDisplayName[256];
} ScreenSaverItem;


typedef struct _GLOBAL_DATA
{
    ScreenSaverItem g_ScreenSaverItems[MAX_SCREENSAVERS];
    int ImageListSelection;
    HMENU g_hPopupMenu;
} GLOBAL_DATA, *PGLOBAL_DATA;



static VOID
ListViewItemAreChanged(HWND hwndDlg, PGLOBAL_DATA pGlobalData, int itemIndex)
{
    BOOL bEnable;
    LV_ITEM lvItem;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = itemIndex;
    lvItem.iSubItem = 0;

    if (!ListView_GetItem(GetDlgItem(hwndDlg, IDC_SCREENS_CHOICES), &lvItem))
        return;

    pGlobalData->ImageListSelection = lvItem.lParam;

    bEnable = (lvItem.lParam != 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_SETTINGS), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TESTSC), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_USEPASSCHK), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIMEDELAY), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SCREENS_TIME), bEnable);
}


static VOID
ScreensaverConfig(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    /*
       /p:<hwnd>  Run in preview 
       /s         Run normal
       /c:<hwnd>  Run configuration, hwnd is handle of calling window
       /a         Run change password
    */

    WCHAR szCmdline[2048];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (pGlobalData->ImageListSelection < 1)
        return;

    swprintf(szCmdline, L"%s /c:%u",
             pGlobalData->g_ScreenSaverItems[pGlobalData->ImageListSelection].szFilename,
             hwndDlg);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if(CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


static VOID
ScreensaverPreview(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    /*
       /p:<hwnd>  Run in preview
       /s         Run normal
       /c:<hwnd>  Run configuration, hwnd is handle of calling window
       /a         Run change password
    */

    WCHAR szCmdline[2048];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    if (pGlobalData->ImageListSelection < 1)
        return;

    swprintf(szCmdline, L"%s /p",
             pGlobalData->g_ScreenSaverItems[pGlobalData->ImageListSelection].szFilename);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if(CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


static VOID
ScreensaverDelete(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    SHFILEOPSTRUCT fos;

    if (pGlobalData->ImageListSelection < 1) // Can NOT delete anything :-)
        return;

    fos.hwnd = hwndDlg;
    fos.wFunc = FO_DELETE;
    fos.fFlags = 0;
    fos.pFrom = pGlobalData->g_ScreenSaverItems[pGlobalData->ImageListSelection].szFilename;

    SHFileOperationW(&fos);
}


static VOID
CheckRegScreenSaverIsSecure(HWND hwndDlg)
{
    HKEY hKey;
    TCHAR szBuffer[2];
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG result;

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &hKey);
    result = RegQueryValueEx(hKey, TEXT("ScreenSaverIsSecure"), 0, &varType, (LPBYTE)szBuffer, &bufferSize);
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS)
    {
        if (_ttoi(szBuffer) == 1)
        {
            SendDlgItemMessage(hwndDlg, IDC_SCREENS_USEPASSCHK, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
            return;
        }
    }

    SendDlgItemMessage(hwndDlg, IDC_SCREENS_USEPASSCHK, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
}


static VOID
AddListViewItems(HWND hwndDlg, PGLOBAL_DATA pGlobalData)
{
    HWND hwndScreenSaverList = GetDlgItem(hwndDlg, IDC_SCREENS_CHOICES);
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR szSearchPath[MAX_PATH];
    LV_ITEM listItem;
    LV_COLUMN dummy;
    RECT clientRect;
    //HKEY regKey;
    SHFILEINFO sfi;
    HIMAGELIST himl;
    HIMAGELIST hScreenShellImageList    = NULL;
    //TCHAR wallpaperFilename[MAX_PATH];
    //DWORD bufferSize = sizeof(wallpaperFilename);
    //DWORD varType = REG_SZ;
    //LONG result;
    UINT i = 0;
    int ScreenlistViewItemCount = 0;
    ScreenSaverItem *ScreenSaverItem = NULL;
    HANDLE hModule = NULL;

    GetClientRect(hwndScreenSaverList, &clientRect);

    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);

    (void)ListView_InsertColumn(hwndScreenSaverList, 0, &dummy);

    /* Add the "None" item */
    ScreenSaverItem = &pGlobalData->g_ScreenSaverItems[ScreenlistViewItemCount];

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
    listItem.iItem      = ScreenlistViewItemCount;
    listItem.lParam     = ScreenlistViewItemCount;

    (void)ListView_InsertItem(hwndScreenSaverList, &listItem);
    ListView_SetItemState(hwndScreenSaverList, ScreenlistViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

    ScreenlistViewItemCount++;

    /* Add current screensaver if any */
/*
    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);

    result = RegQueryValueEx(regKey, TEXT("SCRNSAVE.EXE"), 0, &varType, (LPBYTE)wallpaperFilename, &bufferSize);

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
                g_hScreenShellImageList = himl;
                (void)ListView_SetImageList(g_hScreengroundList, himl, LVSIL_SMALL);
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

            (void)ListView_InsertItem(g_hScreengroundList, &listItem);
            ListView_SetItemState(g_hScreengroundList, g_ScreenlistViewItemCount, LVIS_SELECTED, LVIS_SELECTED);

            g_ScreenlistViewItemCount++;
        }
    }



    RegCloseKey(regKey);
*/

    /* Add all the screensavers in the C:\ReactOS\System32 directory. */

    GetSystemDirectory(szSearchPath, MAX_PATH);
    _tcscat(szSearchPath, TEXT("\\*.scr"));

    hFind = FindFirstFile(szSearchPath, &fd);
    while (hFind != INVALID_HANDLE_VALUE)
    {
        /* Don't add any hidden screensavers */
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
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

            if (himl == NULL)
            {
                break;
            }

            if (i++ == 0)
            {
                hScreenShellImageList = himl;
                (VOID)ListView_SetImageList(hwndScreenSaverList, himl, LVSIL_SMALL);
            }

            ScreenSaverItem = &pGlobalData->g_ScreenSaverItems[ScreenlistViewItemCount];

            ScreenSaverItem->bIsScreenSaver = TRUE;

            hModule = LoadLibraryEx(filename, NULL, DONT_RESOLVE_DLL_REFERENCES);
            if (hModule)
            {
               LoadString(hModule,
                          1,
                          ScreenSaverItem->szDisplayName,
                          sizeof(ScreenSaverItem->szDisplayName) / sizeof(TCHAR));
               FreeLibrary(hModule);
            }
            else
            {
               _tcscpy(ScreenSaverItem->szDisplayName, sfi.szDisplayName);
            }


            _tcscpy(ScreenSaverItem->szFilename, filename);

            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            listItem.pszText    = ScreenSaverItem->szDisplayName;
            listItem.state      = 0;
            listItem.iImage     = sfi.iIcon;
            listItem.iItem      = ScreenlistViewItemCount;
            listItem.lParam     = ScreenlistViewItemCount;

            (VOID)ListView_InsertItem(hwndScreenSaverList, &listItem);

            ScreenlistViewItemCount++;
        }

        if (!FindNextFile(hFind, &fd))
            hFind = INVALID_HANDLE_VALUE;
    }
}


static VOID
SetScreenSaver(PGLOBAL_DATA pGlobalData)
{
    HKEY regKey;

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &regKey);
    RegSetValueEx(regKey, TEXT("SCRNSAVE.EXE"), 0, REG_SZ,
                  (BYTE *)pGlobalData->g_ScreenSaverItems[pGlobalData->ImageListSelection].szFilename,
                  _tcslen(pGlobalData->g_ScreenSaverItems[pGlobalData->ImageListSelection].szFilename) * sizeof(TCHAR));

    RegCloseKey(regKey);

    //if(g_backgroundItems[g_backgroundSelection].bWallpaper == TRUE)
    //{
    //    SystemParametersInfo(SPI_SETDESKWALLPAPER,
    //                         0,
    //                         g_backgroundItems[g_backgroundSelection].szFilename,
    //                         SPIF_UPDATEINIFILE);
    //}
    //else
    //{
    //    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, TEXT(""), SPIF_UPDATEINIFILE);
    //}
}


INT_PTR CALLBACK
ScreenSaverPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            pGlobalData->ImageListSelection = -1;

            SendDlgItemMessage(hwndDlg, IDC_SCREENS_TIME, UDM_SETRANGE, 0, MAKELONG ((short) 240, (short) 0));
            AddListViewItems(hwndDlg, pGlobalData);

            pGlobalData->g_hPopupMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDR_POPUP_MENU));
            pGlobalData->g_hPopupMenu = GetSubMenu(pGlobalData->g_hPopupMenu, 0);

            CheckRegScreenSaverIsSecure(hwndDlg);
            break;

        case WM_DESTROY:
            DestroyMenu(pGlobalData->g_hPopupMenu);
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;

        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);

            switch (controlId)
            {
                case IDC_SCREENS_POWER_BUTTON: // Start Powercfg.Cpl
                    if (command == BN_CLICKED)
                        WinExec("rundll32 shell32.dll,Control_RunDLL powercfg.cpl,,",SW_SHOWNORMAL);
                    break;

                case IDC_SCREENS_TESTSC: // Screensaver Preview
                    if(command == BN_CLICKED)
                        ScreensaverPreview(hwndDlg, pGlobalData);
                    break;

                case ID_MENU_PREVIEW:
                    ScreensaverPreview(hwndDlg, pGlobalData);
                    break;

                case ID_MENU_CONFIG:
                    ScreensaverConfig(hwndDlg, pGlobalData);
                    break;

                case ID_MENU_DELETE: // Delete Screensaver
                    if (command == BN_CLICKED)
                        ScreensaverDelete(hwndDlg, pGlobalData);
                    break;

                case IDC_SCREENS_SETTINGS: // Screensaver Settings
                    if (command == BN_CLICKED)
                        ScreensaverConfig(hwndDlg, pGlobalData);
                    break;

                case IDC_SCREENS_USEPASSCHK: // Screensaver Is Secure
                    if (command == BN_CLICKED)
                        MessageBox(NULL, TEXT("That button doesn't do anything yet"), TEXT("Whoops"), MB_OK);
                    break;

                case IDC_SCREENS_TIME: // Delay before show screensaver
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            LPNMITEMACTIVATE nmia = (LPNMITEMACTIVATE) lParam;
            RECT rc;

            switch(lpnm->code)
            {
                case PSN_APPLY:
                    SetScreenSaver(pGlobalData);
                    return TRUE;

                case NM_RCLICK:
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_SCREENS_CHOICES), &rc);
                    TrackPopupMenuEx(pGlobalData->g_hPopupMenu, TPM_RIGHTBUTTON,
                                     rc.left + nmia->ptAction.x, rc.top + nmia->ptAction.y, hwndDlg, NULL);
                    break;

                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;
                    if ((nm->uNewState & LVIS_SELECTED) == 0)
                        return FALSE;
                    ListViewItemAreChanged(hwndDlg, pGlobalData, nm->iItem);
                    break;
                }

                default:
                    break;
            }
        }
        break;
    }

    return FALSE;
}
