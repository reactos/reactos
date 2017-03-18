/*
 * Regedit settings
 *
 * Copyright (C) 2012 Edijs Kolesnikovics <terminedijs@yahoo.com>
 * Copyright (C) 2012 Grégori Macário Harbs <mysoft64bits at gmail dot com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "regedit.h"
#include <strsafe.h>

const WCHAR g_szGeneralRegKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit";

/* 
VV,VV,VV,VV,WA,WA,WA,WA,WB,WB,WB,WB,R1,R1,R1,R1
R2,R2,R2,R2,R3,R3,R3,R3,R4,R4,R4,r4,LL,LL,LL,LL
TT,TT,TT,TT,RR,RR,RR,RR,BB,BB,BB,BB,SS,SS,SS,SS
NN,NN,NN,NN,KK,KK,KK,KK,DD,DD,DD,DD,SB,SB,SB,SB

VV = Version or Sanity? WINDOWPLACEMENT? (2C?)
WA = (0=restored / 1=maximized)
WB = (1=restored / 3=maximized)
R1 = ???? \
R2 = ???? | either those are reserved unused or they will
R3 = ???? | have IP/INFO if connected to remote registry
R4 = ???? /
LL = Left position of window
TT = top position of window
RR = right position of window
BB = bottom position of window
SS = size of key tree view (splitter)
NN = size of 'name' column
KK = size of 'type' column (kind)
DD = size of 'data' coumn
SB = status bar (1=visible / 0=hidden)
*/

typedef struct
{
    WINDOWPLACEMENT tPlacement;
    int             TreeViewSize;
    int             NameColumnSize;
    int             TypeColumnSize;
    int             DataColumnSize;
    BOOL            StatusBarVisible;
} RegistryBinaryConfig;

extern void LoadSettings(void)
{
    HKEY hKey = NULL;
    WCHAR szBuffer[MAX_PATH];

    if (RegOpenKeyW(HKEY_CURRENT_USER, g_szGeneralRegKey, &hKey) == ERROR_SUCCESS)
    {
        RegistryBinaryConfig tConfig;
        DWORD iBufferSize = sizeof(tConfig);
        BOOL bVisible = FALSE;

        if (RegQueryValueExW(hKey, L"View", NULL, NULL, (LPBYTE)&tConfig, &iBufferSize) == ERROR_SUCCESS)
        {
            if (iBufferSize == sizeof(tConfig))
            {
                RECT rcTemp;

                /* Update status bar settings */
                CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND | (tConfig.StatusBarVisible ? MF_CHECKED : MF_UNCHECKED));
                ShowWindow(hStatusBar, (tConfig.StatusBarVisible ? SW_SHOW : SW_HIDE));

                /* Update listview column width */
                (void)ListView_SetColumnWidth(g_pChildWnd->hListWnd, 0, tConfig.NameColumnSize);
                (void)ListView_SetColumnWidth(g_pChildWnd->hListWnd, 1, tConfig.TypeColumnSize);
                (void)ListView_SetColumnWidth(g_pChildWnd->hListWnd, 2, tConfig.DataColumnSize);

                /* Update treeview (splitter) */
                GetClientRect(hFrameWnd, &rcTemp);
                g_pChildWnd->nSplitPos = tConfig.TreeViewSize;
                ResizeWnd(rcTemp.right, rcTemp.bottom);

                /* Apply program window settings */
                tConfig.tPlacement.length = sizeof(WINDOWPLACEMENT);
                bVisible = SetWindowPlacement(hFrameWnd, &tConfig.tPlacement);
            }
        }

        /* In case we fail to restore the window, or open the key, show normal */
        if (!bVisible)
            ShowWindow(hFrameWnd, SW_SHOWNORMAL);

        /* Restore key position */
        if (QueryStringValue(HKEY_CURRENT_USER, g_szGeneralRegKey, L"LastKey", szBuffer, COUNT_OF(szBuffer)) == ERROR_SUCCESS)
        {
            SelectNode(g_pChildWnd->hTreeWnd, szBuffer);
        }

        RegCloseKey(hKey);
    }
    else
    {
        /* Failed to open key, show normal */
        ShowWindow(hFrameWnd, SW_SHOWNORMAL);
    }
}

extern void SaveSettings(void)
{
    HKEY hKey = NULL;

    if (RegCreateKeyW(HKEY_CURRENT_USER, g_szGeneralRegKey, &hKey) == ERROR_SUCCESS)
    {
        RegistryBinaryConfig tConfig;
        DWORD iBufferSize = sizeof(tConfig);
        WCHAR szBuffer[MAX_PATH];
        LPCWSTR keyPath, rootName;
        HKEY hRootKey;

        /* Save key position */
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
        if (keyPath)
        {
            rootName = get_root_key_name(hRootKey);

            /* Load "My Computer" string and complete it */
            if (LoadStringW(hInst, IDS_MY_COMPUTER, szBuffer, COUNT_OF(szBuffer)) &&
                SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), L"\\")) &&
                SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), rootName)) &&
                SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), L"\\")) &&
                SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), keyPath)))
            {
                RegSetValueExW(hKey, L"LastKey", 0, REG_SZ, (LPBYTE)szBuffer, (DWORD)wcslen(szBuffer) * sizeof(WCHAR));
            }
        }

        /* Get statusbar settings */
        tConfig.StatusBarVisible = ((GetMenuState(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND) & MF_CHECKED) ? 1 : 0);

        /* Get splitter position */
        tConfig.TreeViewSize = g_pChildWnd->nSplitPos;

        /* Get list view column width*/
        tConfig.NameColumnSize = ListView_GetColumnWidth(g_pChildWnd->hListWnd, 0);
        tConfig.TypeColumnSize = ListView_GetColumnWidth(g_pChildWnd->hListWnd, 1);
        tConfig.DataColumnSize = ListView_GetColumnWidth(g_pChildWnd->hListWnd, 2);

        /* Get program window settings */
        tConfig.tPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hFrameWnd, &tConfig.tPlacement);

        /* Save all the data */
        RegSetValueExW(hKey, L"View", 0, REG_BINARY, (LPBYTE)&tConfig, iBufferSize);

        RegCloseKey(hKey);
    }
}
/* EOF */
