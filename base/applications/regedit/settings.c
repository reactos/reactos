/*
 * Regedit settings
 *
 * Copyright 2012 Edijs Kolesnikovics <terminedijs@yahoo.com>
 * Copyright 2012 Gr�gori Mac�rio Harbs <mysoft64bits@gmail.com>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

const WCHAR g_szGeneralRegKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit";
DECLSPEC_IMPORT ULONG WINAPIV DbgPrint(PCCH Format,...);

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
        if (QueryStringValue(HKEY_CURRENT_USER, g_szGeneralRegKey, L"LastKey", szBuffer, ARRAY_SIZE(szBuffer)) == ERROR_SUCCESS)
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
        WCHAR szBuffer[MAX_PATH]; /* FIXME: a complete registry path can be longer than that */
        LPCWSTR keyPath, rootName;
        HKEY hRootKey;

        /* Save key position */
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
        rootName = get_root_key_name(hRootKey);

        /* Load "My Computer" string and complete it */
        if (LoadStringW(hInst, IDS_MY_COMPUTER, szBuffer, ARRAY_SIZE(szBuffer)) &&
            SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), L"\\")) &&
            SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), rootName)) &&
            SUCCEEDED(StringCbCatW(szBuffer, sizeof(szBuffer), L"\\")))
        {
            HRESULT hr = S_OK;
            if (keyPath)
                hr = StringCbCatW(szBuffer, sizeof(szBuffer), keyPath);
            if (SUCCEEDED(hr))
                RegSetValueExW(hKey, L"LastKey", 0, REG_SZ, (LPBYTE)szBuffer, (DWORD)wcslen(szBuffer) * sizeof(WCHAR));
            else
                DbgPrint("err: (%s:%d): Buffer not big enough for '%S + %S'\n", __FILE__, __LINE__, rootName, keyPath);
        }
        else
        {
            DbgPrint("err: (%s:%d): Buffer not big enough for '%S'\n", __FILE__, __LINE__, rootName);
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
