/*
 * Regedit frame window
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

#include <commdlg.h>
#include <cderr.h>
#include <objsel.h>

#define FAVORITES_MENU_POSITION 3
static WCHAR s_szFavoritesRegKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\Favorites";
static BOOL bInMenuLoop = FALSE;        /* Tells us if we are in the menu loop */
extern WCHAR Suggestions[256];

static UINT ErrorBox(HWND hWnd, UINT Error)
{
    WCHAR buf[400];
    if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, Error, 0, buf, _countof(buf), NULL))
        *(UINT*)buf = L'?';
    MessageBoxW(hWnd, buf, NULL, MB_ICONSTOP);
    return Error;
}

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
    if (IsWindowVisible(hStatusBar))
    {
        RECT rt;

        SetupStatusBar(hWnd, TRUE);
        GetWindowRect(hStatusBar, &rt);
        prect->bottom -= rt.bottom - rt.top;
    }
    MoveWindow(g_pChildWnd->hWnd, prect->left, prect->top, prect->right, prect->bottom, TRUE);
}

static void resize_frame_client(HWND hWnd)
{
    RECT rect;

    GetClientRect(hWnd, &rect);
    resize_frame_rect(hWnd, &rect);
}

static void OnInitMenu(HWND hWnd)
{
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwIndex, cbValueName, cbValueData, dwType;
    WCHAR szValueName[256];
    BYTE abValueData[256];
    static int s_nFavoriteMenuSubPos = -1;
    HMENU hMenu;
    BOOL bDisplayedAny = FALSE;
    HTREEITEM hSelTreeItem;
    BOOL bCanAddFav;

    /* Find Favorites menu and clear it out */
    hMenu = GetSubMenu(GetMenu(hWnd), FAVORITES_MENU_POSITION);
    if (!hMenu)
        goto done;
    if (s_nFavoriteMenuSubPos < 0)
    {
        s_nFavoriteMenuSubPos = GetMenuItemCount(hMenu);
    }
    else
    {
        while(RemoveMenu(hMenu, s_nFavoriteMenuSubPos, MF_BYPOSITION)) ;
    }

    hSelTreeItem = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
    bCanAddFav = TreeView_GetParent(g_pChildWnd->hTreeWnd, hSelTreeItem) != NULL;
    EnableMenuItem(GetMenu(hWnd), ID_FAVOURITES_ADDTOFAVOURITES,
                   MF_BYCOMMAND | (bCanAddFav ? MF_ENABLED : MF_GRAYED));

    lResult = RegOpenKeyW(HKEY_CURRENT_USER, s_szFavoritesRegKey, &hKey);
    if (lResult != ERROR_SUCCESS)
        goto done;

    dwIndex = 0;
    do
    {
        cbValueName = ARRAY_SIZE(szValueName);
        cbValueData = sizeof(abValueData);
        lResult = RegEnumValueW(hKey, dwIndex, szValueName, &cbValueName, NULL, &dwType, abValueData, &cbValueData);
        if ((lResult == ERROR_SUCCESS) && (dwType == REG_SZ))
        {
            if (!bDisplayedAny)
            {
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                bDisplayedAny = TRUE;
            }
            AppendMenu(hMenu, 0, ID_FAVORITES_MIN + GetMenuItemCount(hMenu), szValueName);
        }
        dwIndex++;
    }
    while(lResult == ERROR_SUCCESS);

done:
    if (hKey)
        RegCloseKey(hKey);
}

static void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;
    UNREFERENCED_PARAMETER(hWnd);

    /* Update the status bar pane sizes */
    nParts = -1;
    SendMessageW(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
    bInMenuLoop = TRUE;
    SendMessageW(hStatusBar, SB_SETTEXTW, (WPARAM)0, (LPARAM)L"");
}

static void OnExitMenuLoop(HWND hWnd)
{
    bInMenuLoop = FALSE;
    /* Update the status bar pane sizes*/
    SetupStatusBar(hWnd, TRUE);
    UpdateStatusBar();
}

static void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    WCHAR str[100];

    str[0] = UNICODE_NULL;
    if (nFlags & MF_POPUP)
    {
        if (hSysMenu != GetMenu(hWnd))
        {
            if (nItemID == 2) nItemID = 5;
        }
    }
    if (LoadStringW(hInst, nItemID, str, 100))
    {
        /* load appropriate string*/
        LPWSTR lpsz = str;
        /* first newline terminates actual string*/
        lpsz = wcschr(lpsz, L'\n');
        if (lpsz != NULL)
            *lpsz = L'\0';
    }
    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)str);
}

void SetupStatusBar(HWND hWnd, BOOL bResize)
{
    RECT  rc;
    int nParts;
    GetClientRect(hWnd, &rc);
    nParts = rc.right;
    /*    nParts = -1;*/
    if (bResize)
        SendMessageW(hStatusBar, WM_SIZE, 0, 0);
    SendMessageW(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
}

void UpdateStatusBar(void)
{
    HKEY hKeyRoot;
    LPCWSTR pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);

    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)pszKeyPath);
}

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
    BOOL vis = IsWindowVisible(hchild);
    HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);

    CheckMenuItem(hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
    ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
    resize_frame_client(hWnd);
}

static BOOL CheckCommDlgError(HWND hWnd)
{
    DWORD dwErrorCode = CommDlgExtendedError();
    UNREFERENCED_PARAMETER(hWnd);
    switch (dwErrorCode)
    {
        case CDERR_DIALOGFAILURE:
            break;
        case CDERR_FINDRESFAILURE:
            break;
        case CDERR_NOHINSTANCE:
            break;
        case CDERR_INITIALIZATION:
            break;
        case CDERR_NOHOOK:
            break;
        case CDERR_LOCKRESFAILURE:
            break;
        case CDERR_NOTEMPLATE:
            break;
        case CDERR_LOADRESFAILURE:
            break;
        case CDERR_STRUCTSIZE:
            break;
        case CDERR_LOADSTRFAILURE:
            break;
        case FNERR_BUFFERTOOSMALL:
            break;
        case CDERR_MEMALLOCFAILURE:
            break;
        case FNERR_INVALIDFILENAME:
            break;
        case CDERR_MEMLOCKFAILURE:
            break;
        case FNERR_SUBCLASSFAILURE:
            break;
        default:
            break;
    }
    return TRUE;
}

WCHAR FileNameBuffer[MAX_PATH];

typedef struct
{
    UINT DisplayID;
    UINT FilterID;
} FILTERPAIR, *PFILTERPAIR;

void
BuildFilterStrings(WCHAR *Filter, PFILTERPAIR Pairs, int PairCount)
{
    int i, c;

    c = 0;
    for(i = 0; i < PairCount; i++)
    {
        c += LoadStringW(hInst, Pairs[i].DisplayID, &Filter[c], 255);
        Filter[++c] = L'\0';
        c += LoadStringW(hInst, Pairs[i].FilterID, &Filter[c], 255);
        Filter[++c] = L'\0';
    }
    Filter[++c] = L'\0';
}

static BOOL InitOpenFileName(HWND hWnd, OPENFILENAME* pofn, BOOL bSave)
{
    FILTERPAIR FilterPairs[5];
    static WCHAR Filter[1024];

    memset(pofn, 0, sizeof(OPENFILENAME));
    pofn->lStructSize = sizeof(OPENFILENAME);
    pofn->hwndOwner = hWnd;
    pofn->hInstance = hInst;

    /* create filter string */
    FilterPairs[0].DisplayID = IDS_FLT_REGFILES;
    FilterPairs[0].FilterID = IDS_FLT_REGFILES_FLT;
    FilterPairs[1].DisplayID = IDS_FLT_HIVFILES;
    FilterPairs[1].FilterID = IDS_FLT_HIVFILES_FLT;
    FilterPairs[2].DisplayID = IDS_FLT_REGEDIT4;
    FilterPairs[2].FilterID = IDS_FLT_REGEDIT4_FLT;
    if (bSave)
    {
        FilterPairs[3].DisplayID = IDS_FLT_TXTFILES;
        FilterPairs[3].FilterID = IDS_FLT_TXTFILES_FLT;
        FilterPairs[4].DisplayID = IDS_FLT_ALLFILES;
        FilterPairs[4].FilterID = IDS_FLT_ALLFILES_FLT;
    }
    else
    {
        FilterPairs[3].DisplayID = IDS_FLT_ALLFILES;
        FilterPairs[3].FilterID = IDS_FLT_ALLFILES_FLT;
    }

    BuildFilterStrings(Filter, FilterPairs, ARRAY_SIZE(FilterPairs) - !bSave);

    pofn->lpstrFilter = Filter;
    pofn->lpstrFile = FileNameBuffer;
    pofn->nMaxFile = _countof(FileNameBuffer);
    pofn->Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    pofn->lpstrDefExt = L"reg";
    if (bSave)
        pofn->Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    else
        pofn->Flags |= OFN_FILEMUSTEXIST;

    return TRUE;
}

#define LOADHIVE_KEYNAMELENGTH 128

static INT_PTR CALLBACK LoadHive_KeyNameInHookProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPWSTR sKey = NULL;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        sKey = (LPWSTR)lParam;
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            if(GetDlgItemTextW(hWndDlg, IDC_EDIT_KEY, sKey, LOADHIVE_KEYNAMELENGTH))
                return EndDialog(hWndDlg, -1);
            else
                return EndDialog(hWndDlg, 0);
        case IDCANCEL:
            return EndDialog(hWndDlg, 0);
        }
        break;
    }
    return FALSE;
}

static BOOL EnablePrivilege(LPCWSTR lpszPrivilegeName, LPCWSTR lpszSystemName, BOOL bEnablePrivilege)
{
    BOOL   bRet   = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES,
                         &hToken))
    {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

        if (LookupPrivilegeValueW(lpszSystemName,
                                  lpszPrivilegeName,
                                  &tp.Privileges[0].Luid))
        {
            bRet = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

            if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
                bRet = FALSE;
        }

        CloseHandle(hToken);
    }

    return bRet;
}

static BOOL LoadHive(HWND hWnd)
{
    OPENFILENAME ofn;
    WCHAR Caption[128];
    LPCWSTR pszKeyPath;
    WCHAR xPath[LOADHIVE_KEYNAMELENGTH];
    HKEY hRootKey;
    WCHAR Filter[1024];
    FILTERPAIR filter;
    /* get the item key to load the hive in */
    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
    /* initialize the "open file" dialog */
    InitOpenFileName(hWnd, &ofn, FALSE);
    /* build the "All Files" filter up */
    filter.DisplayID = IDS_FLT_ALLFILES;
    filter.FilterID = IDS_FLT_ALLFILES_FLT;
    BuildFilterStrings(Filter, &filter, 1);
    ofn.lpstrFilter = Filter;
    /* load and set the caption and flags for dialog */
    LoadStringW(hInst, IDS_LOAD_HIVE, Caption, ARRAY_SIZE(Caption));
    ofn.lpstrTitle = Caption;
    ofn.Flags |= OFN_ENABLESIZING;

    /* now load the hive */
    if (GetOpenFileName(&ofn))
    {
        if (DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_LOADHIVE), hWnd,
                            &LoadHive_KeyNameInHookProc, (LPARAM)xPath))
        {
            LONG regLoadResult;

            /* Enable the 'restore' privilege, load the hive, disable the privilege */
            EnablePrivilege(SE_RESTORE_NAME, NULL, TRUE);
            regLoadResult = RegLoadKeyW(hRootKey, xPath, ofn.lpstrFile);
            EnablePrivilege(SE_RESTORE_NAME, NULL, FALSE);

            if(regLoadResult == ERROR_SUCCESS)
            {
                /* refresh tree and list views */
                RefreshTreeView(g_pChildWnd->hTreeWnd);
                pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
                RefreshListView(g_pChildWnd->hListWnd, hRootKey, pszKeyPath, TRUE);
            }
            else
            {
                ErrorMessageBox(hWnd, Caption, regLoadResult);
                return FALSE;
            }
        }
    }
    else
    {
        CheckCommDlgError(hWnd);
    }
    return TRUE;
}

static BOOL UnloadHive(HWND hWnd)
{
    WCHAR Caption[128];
    LPCWSTR pszKeyPath;
    HKEY hRootKey;
    LONG regUnloadResult;

    /* get the item key to unload */
    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
    /* load and set the caption and flags for dialog */
    LoadStringW(hInst, IDS_UNLOAD_HIVE, Caption, ARRAY_SIZE(Caption));

    /* Enable the 'restore' privilege, unload the hive, disable the privilege */
    EnablePrivilege(SE_RESTORE_NAME, NULL, TRUE);
    regUnloadResult = RegUnLoadKeyW(hRootKey, pszKeyPath);
    EnablePrivilege(SE_RESTORE_NAME, NULL, FALSE);

    if(regUnloadResult == ERROR_SUCCESS)
    {
        /* refresh tree and list views */
        RefreshTreeView(g_pChildWnd->hTreeWnd);
        pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
        RefreshListView(g_pChildWnd->hListWnd, hRootKey, pszKeyPath, TRUE);
    }
    else
    {
        ErrorMessageBox(hWnd, Caption, regUnloadResult);
        return FALSE;
    }
    return TRUE;
}

static BOOL ImportRegistryFile(HWND hWnd)
{
    BOOL bRet = FALSE;
    OPENFILENAME ofn;
    WCHAR Caption[128], szTitle[512], szText[512];
    HKEY hKeyRoot;
    LPCWSTR pszKeyPath;

    /* Figure out in which key path we are importing */
    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);

    InitOpenFileName(hWnd, &ofn, FALSE);
    LoadStringW(hInst, IDS_IMPORT_REG_FILE, Caption, ARRAY_SIZE(Caption));
    ofn.lpstrTitle = Caption;
    ofn.Flags |= OFN_ENABLESIZING;

    if (GetOpenFileName(&ofn))
    {
        /* Look at the extension of the file to determine its type */
        if (ofn.nFileExtension >= 1 &&
            _wcsicmp(ofn.lpstrFile + ofn.nFileExtension, L"reg") == 0) /* REGEDIT4 or Windows Registry Editor Version 5.00 */
        {
            /* Open the file */
            FILE* fp = _wfopen(ofn.lpstrFile, L"rb");

            /* Import it */
            if (fp == NULL || !import_registry_file(fp))
            {
                /* Error opening the file */
                LoadStringW(hInst, IDS_APP_TITLE, szTitle, ARRAY_SIZE(szTitle));
                LoadStringW(hInst, IDS_IMPORT_ERROR, szText, ARRAY_SIZE(szText));
                InfoMessageBox(hWnd, MB_OK | MB_ICONERROR, szTitle, szText, ofn.lpstrFile);
                bRet = FALSE;
            }
            else
            {
                /* Show successful import */
                LoadStringW(hInst, IDS_APP_TITLE, szTitle, ARRAY_SIZE(szTitle));
                LoadStringW(hInst, IDS_IMPORT_OK, szText, ARRAY_SIZE(szText));
                InfoMessageBox(hWnd, MB_OK | MB_ICONINFORMATION, szTitle, szText, ofn.lpstrFile);
                bRet = TRUE;
            }

            /* Close the file */
            if (fp) fclose(fp);
        }
        else /* Registry Hive Files */
        {
            LoadStringW(hInst, IDS_QUERY_IMPORT_HIVE_CAPTION, szTitle, ARRAY_SIZE(szTitle));
            LoadStringW(hInst, IDS_QUERY_IMPORT_HIVE_MSG, szText, ARRAY_SIZE(szText));

            /* Display a confirmation message */
            if (MessageBoxW(g_pChildWnd->hWnd, szText, szTitle, MB_ICONWARNING | MB_YESNO) == IDYES)
            {
                LONG lResult;
                HKEY hSubKey;

                /* Open the subkey */
                lResult = RegOpenKeyExW(hKeyRoot, pszKeyPath, 0, KEY_WRITE, &hSubKey);
                if (lResult == ERROR_SUCCESS)
                {
                    /* Enable the 'restore' privilege, restore the hive then disable the privilege */
                    EnablePrivilege(SE_RESTORE_NAME, NULL, TRUE);
                    lResult = RegRestoreKey(hSubKey, ofn.lpstrFile, REG_FORCE_RESTORE);
                    EnablePrivilege(SE_RESTORE_NAME, NULL, FALSE);

                    /* Flush the subkey and close it */
                    RegFlushKey(hSubKey);
                    RegCloseKey(hSubKey);
                }

                /* Set the return value */
                bRet = (lResult == ERROR_SUCCESS);

                /* Display error, if any */
                if (!bRet) ErrorMessageBox(hWnd, Caption, lResult);
            }
        }
    }
    else
    {
        CheckCommDlgError(hWnd);
    }

    /* refresh tree and list views */
    RefreshTreeView(g_pChildWnd->hTreeWnd);
    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, pszKeyPath, TRUE);

    return bRet;
}

static UINT_PTR CALLBACK ExportRegistryFile_OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndExportAll;
    HWND hwndExportBranch;
    HWND hwndExportBranchText;
    UINT_PTR iResult = 0;
    OPENFILENAME *pOfn;
    LPWSTR pszSelectedKey;
    OFNOTIFY *pOfnNotify;

    UNREFERENCED_PARAMETER(wParam);

    switch(uiMsg)
    {
    case WM_INITDIALOG:
        pOfn = (OPENFILENAME *) lParam;
        pszSelectedKey = (LPWSTR) pOfn->lCustData;

        hwndExportAll = GetDlgItem(hdlg, IDC_EXPORT_ALL);
        if (hwndExportAll)
            SendMessageW(hwndExportAll, BM_SETCHECK, pszSelectedKey ? BST_UNCHECKED : BST_CHECKED, 0);

        hwndExportBranch = GetDlgItem(hdlg, IDC_EXPORT_BRANCH);
        if (hwndExportBranch)
            SendMessageW(hwndExportBranch, BM_SETCHECK, pszSelectedKey ? BST_CHECKED : BST_UNCHECKED, 0);

        hwndExportBranchText = GetDlgItem(hdlg, IDC_EXPORT_BRANCH_TEXT);
        if (hwndExportBranchText)
            SetWindowTextW(hwndExportBranchText, pszSelectedKey);
        break;

    case WM_NOTIFY:
        if (((NMHDR *) lParam)->code == CDN_FILEOK)
        {
            pOfnNotify = (OFNOTIFY *) lParam;
            pszSelectedKey = (LPWSTR) pOfnNotify->lpOFN->lCustData;

            hwndExportBranch = GetDlgItem(hdlg, IDC_EXPORT_BRANCH);
            hwndExportBranchText = GetDlgItem(hdlg, IDC_EXPORT_BRANCH_TEXT);
            if (hwndExportBranch && hwndExportBranchText
                    && (SendMessageW(hwndExportBranch, BM_GETCHECK, 0, 0) == BST_CHECKED))
            {
                GetWindowTextW(hwndExportBranchText, pszSelectedKey, _MAX_PATH);
            }
            else if (pszSelectedKey)
            {
                pszSelectedKey[0] = L'\0';
            }
        }
        break;
    }
    return iResult;
}

BOOL ExportRegistryFile(HWND hWnd)
{
    BOOL bRet = FALSE;
    OPENFILENAME ofn;
    WCHAR ExportKeyPath[_MAX_PATH] = {0};
    WCHAR Caption[128], szTitle[512], szText[512];
    HKEY hKeyRoot;
    LPCWSTR pszKeyPath;

    /* Figure out which key path we are exporting */
    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    GetKeyName(ExportKeyPath, ARRAY_SIZE(ExportKeyPath), hKeyRoot, pszKeyPath);

    InitOpenFileName(hWnd, &ofn, TRUE);
    LoadStringW(hInst, IDS_EXPORT_REG_FILE, Caption, ARRAY_SIZE(Caption));
    ofn.lpstrTitle = Caption;

    /* Only set the path if a key (not the root node) is selected */
    if (hKeyRoot != 0)
    {
        ofn.lCustData = (LPARAM) ExportKeyPath;
    }
    ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    ofn.lpfnHook = ExportRegistryFile_OFNHookProc;
    ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_EXPORTRANGE);
    if (GetSaveFileName(&ofn))
    {
        switch (ofn.nFilterIndex)
        {
            case 2: /* Registry Hive Files */
            {
                LONG lResult;
                HKEY hSubKey;

                /* Open the subkey */
                lResult = RegOpenKeyExW(hKeyRoot, pszKeyPath, 0, KEY_READ, &hSubKey);
                if (lResult == ERROR_SUCCESS)
                {
                    /* Enable the 'backup' privilege, save the hive then disable the privilege */
                    EnablePrivilege(SE_BACKUP_NAME, NULL, TRUE);
                    lResult = RegSaveKeyW(hSubKey, ofn.lpstrFile, NULL);
                    if (lResult == ERROR_ALREADY_EXISTS)
                    {
                        /*
                         * We are here, that means that we already said "yes" to the confirmation dialog.
                         * So we absolutely want to replace the hive file.
                         */
                        if (DeleteFileW(ofn.lpstrFile))
                        {
                            /* Try again */
                            lResult = RegSaveKeyW(hSubKey, ofn.lpstrFile, NULL);
                        }
                    }
                    EnablePrivilege(SE_BACKUP_NAME, NULL, FALSE);

                    if (lResult != ERROR_SUCCESS)
                    {
                        /*
                         * If we are here, it's because RegSaveKeyW has failed for any reason.
                         * The problem is that even if it has failed, it has created or
                         * replaced the exported hive file with a new empty file. We don't
                         * want to keep this file, so we delete it.
                         */
                        DeleteFileW(ofn.lpstrFile);
                    }

                    /* Close the subkey */
                    RegCloseKey(hSubKey);
                }

                /* Set the return value */
                bRet = (lResult == ERROR_SUCCESS);

                /* Display error, if any */
                if (!bRet) ErrorMessageBox(hWnd, Caption, lResult);

                break;
            }

            case 1:  /* Windows Registry Editor Version 5.00 */
            case 3:  /* REGEDIT4 */
            default: /* All files ==> use Windows Registry Editor Version 5.00 */
            {
                if (!export_registry_key(ofn.lpstrFile, ExportKeyPath,
                                         (ofn.nFilterIndex == 3 ? REG_FORMAT_4
                                                                : REG_FORMAT_5)))
                {
                    /* Error creating the file */
                    LoadStringW(hInst, IDS_APP_TITLE, szTitle, ARRAY_SIZE(szTitle));
                    LoadStringW(hInst, IDS_EXPORT_ERROR, szText, ARRAY_SIZE(szText));
                    InfoMessageBox(hWnd, MB_OK | MB_ICONERROR, szTitle, szText, ofn.lpstrFile);
                    bRet = FALSE;
                }
                else
                {
                    bRet = TRUE;
                }

                break;
            }

            case 4:  /* Text File */
            {
                bRet = txt_export_registry_key(ofn.lpstrFile, ExportKeyPath);
                if (!bRet)
                {
                    /* Error creating the file */
                    LoadStringW(hInst, IDS_APP_TITLE, szTitle, ARRAY_SIZE(szTitle));
                    LoadStringW(hInst, IDS_EXPORT_ERROR, szText, ARRAY_SIZE(szText));
                    InfoMessageBox(hWnd, MB_OK | MB_ICONERROR, szTitle, szText, ofn.lpstrFile);
                }
                break;
            }
        }
    }
    else
    {
        CheckCommDlgError(hWnd);
    }

    return bRet;
}

BOOL PrintRegistryHive(HWND hWnd, LPWSTR path)
{
#if 1
    PRINTDLG pd;
    UNREFERENCED_PARAMETER(path);

    ZeroMemory(&pd, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner   = hWnd;
    pd.hDevMode    = NULL;     /* Don't forget to free or store hDevMode*/
    pd.hDevNames   = NULL;     /* Don't forget to free or store hDevNames*/
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies     = 1;
    pd.nFromPage   = 0xFFFF;
    pd.nToPage     = 0xFFFF;
    pd.nMinPage    = 1;
    pd.nMaxPage    = 0xFFFF;
    if (PrintDlg(&pd))
    {
        /* GDI calls to render output. */
        DeleteDC(pd.hDC); /* Delete DC when done.*/
    }
#else
    HRESULT hResult;
    PRINTDLGEX pd;

    hResult = PrintDlgEx(&pd);
    if (hResult == S_OK)
    {
        switch (pd.dwResultAction)
        {
        case PD_RESULT_APPLY:
            /*The user clicked the Apply button and later clicked the Cancel button. This indicates that the user wants to apply the changes made in the property sheet, but does not yet want to print. The PRINTDLGEX structure contains the information specified by the user at the time the Apply button was clicked. */
            break;
        case PD_RESULT_CANCEL:
            /*The user clicked the Cancel button. The information in the PRINTDLGEX structure is unchanged. */
            break;
        case PD_RESULT_PRINT:
            /*The user clicked the Print button. The PRINTDLGEX structure contains the information specified by the user. */
            break;
        default:
            break;
        }
    }
    else
    {
        switch (hResult)
        {
        case E_OUTOFMEMORY:
            /*Insufficient memory. */
            break;
        case E_INVALIDARG:
            /* One or more arguments are invalid. */
            break;
        case E_POINTER:
            /*Invalid pointer. */
            break;
        case E_HANDLE:
            /*Invalid handle. */
            break;
        case E_FAIL:
            /*Unspecified error. */
            break;
        default:
            break;
        }
        return FALSE;
    }
#endif
    return TRUE;
}

static void ChooseFavorite(LPCWSTR pszFavorite)
{
    HKEY hKey = NULL;
    WCHAR szFavoritePath[512];
    DWORD cbData, dwType;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, s_szFavoritesRegKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        goto done;

    cbData = sizeof(szFavoritePath);
    memset(szFavoritePath, 0, sizeof(szFavoritePath));
    if (RegQueryValueExW(hKey, pszFavorite, NULL, &dwType, (LPBYTE) szFavoritePath, &cbData) != ERROR_SUCCESS)
        goto done;

    if (dwType == REG_SZ)
        SelectNode(g_pChildWnd->hTreeWnd, szFavoritePath);

done:
    if (hKey)
        RegCloseKey(hKey);
}

static LPWSTR GetItemFullPath(HTREEITEM hTI)
{
    HKEY hRoot;
    WCHAR rootname[MAX_PATH], *buffer;
    SIZE_T rootlen, subkeylen;
    LPCWSTR subkey = GetItemPath(g_pChildWnd->hTreeWnd, hTI, &hRoot);
    if (!subkey || !hRoot)
        return NULL;
    if (!GetKeyName(rootname, ARRAY_SIZE(rootname), hRoot, L""))
        return NULL;
    rootlen = lstrlenW(rootname) + 1; // + 1 for '\\'
    subkeylen = lstrlenW(subkey);
    buffer = (WCHAR*)malloc((rootlen + subkeylen + 1) * sizeof(WCHAR));
    if (buffer)
    {
        lstrcpyW(buffer, rootname);
        buffer[rootlen - 1] = '\\';
        lstrcpyW(buffer + rootlen, subkey);
    }
    return buffer;
}

static INT_PTR CALLBACK AddToFavoritesDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hName = GetDlgItem(hWnd, IDC_FAVORITENAME);
    WCHAR name[MAX_PATH];
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TVITEM tvi;
            tvi.mask = TVIF_HANDLE | TVIF_TEXT;
            tvi.hItem = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
            tvi.pszText = name;
            tvi.cchTextMax = _countof(name);
            if (!TreeView_GetItem(g_pChildWnd->hTreeWnd, &tvi))
                tvi.pszText[0] = UNICODE_NULL;
            SetWindowTextW(hName, tvi.pszText);
            SendMessageW(hName, EM_LIMITTEXT, _countof(name) - 1, 0);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    {
                        LPWSTR path;
                        HKEY hKey;
                        DWORD err;
                        if (!GetWindowTextW(hName, name, _countof(name)))
                        {
                            err = GetLastError();
                            goto failed;
                        }
                        path = GetItemFullPath(NULL);
                        if (!path)
                        {
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            goto failed;
                        }
                        err = RegCreateKeyExW(HKEY_CURRENT_USER, s_szFavoritesRegKey, 0,
                                              NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
                        if (err)
                            goto failed;
                        err = RegSetValueExW(hKey, name, 0, REG_SZ, (BYTE*)path, (lstrlenW(path) + 1) * sizeof(WCHAR));
                        RegCloseKey(hKey);
                        if (err) failed:
                            ErrorBox(hWnd, err);
                        free(path);
                        return EndDialog(hWnd, err);
                    }
                case IDCANCEL:
                    return EndDialog(hWnd, ERROR_CANCELLED);
                case IDC_FAVORITENAME:
                    if (HIWORD(wParam) == EN_UPDATE)
                        EnableWindow(GetDlgItem(hWnd, IDOK), GetWindowTextLengthW(hName) != 0);
                    break;
            }
            break;
    }
    return FALSE;
}

BOOL CopyKeyName(HWND hWnd, HKEY hRootKey, LPCWSTR keyName)
{
    BOOL bClipboardOpened = FALSE;
    BOOL bSuccess = FALSE;
    WCHAR szBuffer[512];
    HGLOBAL hGlobal;
    LPWSTR s;
    SIZE_T cbGlobal;

    if (!OpenClipboard(hWnd))
        goto done;
    bClipboardOpened = TRUE;

    if (!EmptyClipboard())
        goto done;

    if (!GetKeyName(szBuffer, ARRAY_SIZE(szBuffer), hRootKey, keyName))
        goto done;

    cbGlobal = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
    hGlobal = GlobalAlloc(GMEM_MOVEABLE, cbGlobal);
    if (!hGlobal)
        goto done;

    s = GlobalLock(hGlobal);
    StringCbCopyW(s, cbGlobal, szBuffer);
    GlobalUnlock(hGlobal);

    SetClipboardData(CF_UNICODETEXT, hGlobal);
    bSuccess = TRUE;

done:
    if (bClipboardOpened)
        CloseClipboard();
    return bSuccess;
}

static BOOL CreateNewValue(HKEY hRootKey, LPCWSTR pszKeyPath, DWORD dwType)
{
    WCHAR szNewValueFormat[128];
    WCHAR szNewValue[128];
    int iIndex = 1;
    BYTE data[128];
    DWORD dwExistingType, cbData;
    LONG lResult;
    HKEY hKey;
    LVFINDINFO lvfi;

    if (RegOpenKeyExW(hRootKey, pszKeyPath, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &hKey) != ERROR_SUCCESS)
        return FALSE;

    LoadStringW(hInst, IDS_NEW_VALUE, szNewValueFormat, ARRAY_SIZE(szNewValueFormat));

    do
    {
        wsprintf(szNewValue, szNewValueFormat, iIndex++);
        cbData = sizeof(data);
        lResult = RegQueryValueExW(hKey, szNewValue, NULL, &dwExistingType, data, &cbData);
    }
    while(lResult == ERROR_SUCCESS);

    switch(dwType)
    {
        case REG_DWORD:
            cbData = sizeof(DWORD);
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
            cbData = sizeof(WCHAR);
            break;
        case REG_MULTI_SZ:
            /*
             * WARNING: An empty multi-string has only one null char.
             * Indeed, multi-strings are built in the following form:
             * str1\0str2\0...strN\0\0
             * where each strI\0 is a null-terminated string, and it
             * ends with a terminating empty string.
             * Therefore an empty multi-string contains only the terminating
             * empty string, that is, one null char.
             */
            cbData = sizeof(WCHAR);
            break;
        case REG_QWORD: /* REG_QWORD_LITTLE_ENDIAN */
            cbData = sizeof(DWORDLONG); // == sizeof(DWORD) * 2;
            break;
        default:
            cbData = 0;
            break;
    }
    memset(data, 0, cbData);
    lResult = RegSetValueExW(hKey, szNewValue, 0, dwType, data, cbData);
    RegCloseKey(hKey);
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RefreshListView(g_pChildWnd->hListWnd, hRootKey, pszKeyPath, TRUE);

    /* locate the newly added value, and get ready to rename it */
    memset(&lvfi, 0, sizeof(lvfi));
    lvfi.flags = LVFI_STRING;
    lvfi.psz = szNewValue;
    iIndex = ListView_FindItem(g_pChildWnd->hListWnd, -1, &lvfi);
    if (iIndex >= 0)
    {
        ListView_SetItemState(g_pChildWnd->hListWnd, iIndex,
                              LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_pChildWnd->hListWnd, iIndex, FALSE);
        (void)ListView_EditLabel(g_pChildWnd->hListWnd, iIndex);
    }

    return TRUE;
}

static HRESULT
InitializeRemoteRegistryPicker(OUT IDsObjectPicker **pDsObjectPicker)
{
    HRESULT hRet;

    *pDsObjectPicker = NULL;

    hRet = CoCreateInstance(&CLSID_DsObjectPicker,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            &IID_IDsObjectPicker,
                            (LPVOID*)pDsObjectPicker);
    if (SUCCEEDED(hRet))
    {
        DSOP_INIT_INFO InitInfo;
        static DSOP_SCOPE_INIT_INFO Scopes[] =
        {
            {
                sizeof(DSOP_SCOPE_INIT_INFO),
                DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE |
                DSOP_SCOPE_TYPE_GLOBAL_CATALOG | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
                DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN | DSOP_SCOPE_TYPE_WORKGROUP |
                DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,
                0,
                {
                    {
                        DSOP_FILTER_COMPUTERS,
                        0,
                        0
                    },
                    DSOP_DOWNLEVEL_FILTER_COMPUTERS
                },
                NULL,
                NULL,
                S_OK
            },
        };

        InitInfo.cbSize = sizeof(InitInfo);
        InitInfo.pwzTargetComputer = NULL;
        InitInfo.cDsScopeInfos = ARRAY_SIZE(Scopes);
        InitInfo.aDsScopeInfos = Scopes;
        InitInfo.flOptions = 0;
        InitInfo.cAttributesToFetch = 0;
        InitInfo.apwzAttributeNames = NULL;

        hRet = (*pDsObjectPicker)->lpVtbl->Initialize(*pDsObjectPicker,
                &InitInfo);

        if (FAILED(hRet))
        {
            /* delete the object picker in case initialization failed! */
            (*pDsObjectPicker)->lpVtbl->Release(*pDsObjectPicker);
        }
    }

    return hRet;
}

static HRESULT
InvokeRemoteRegistryPickerDialog(IN IDsObjectPicker *pDsObjectPicker,
                                 IN HWND hwndParent  OPTIONAL,
                                 OUT LPWSTR lpBuffer,
                                 IN UINT uSize)
{
    IDataObject *pdo = NULL;
    HRESULT hRet;

    hRet = pDsObjectPicker->lpVtbl->InvokeDialog(pDsObjectPicker,
            hwndParent,
            &pdo);
    if (hRet == S_OK)
    {
        STGMEDIUM stm;
        FORMATETC fe;

        fe.cfFormat = (CLIPFORMAT) RegisterClipboardFormatW(CFSTR_DSOP_DS_SELECTION_LIST);
        fe.ptd = NULL;
        fe.dwAspect = DVASPECT_CONTENT;
        fe.lindex = -1;
        fe.tymed = TYMED_HGLOBAL;

        hRet = pdo->lpVtbl->GetData(pdo,
                                    &fe,
                                    &stm);
        if (SUCCEEDED(hRet))
        {
            PDS_SELECTION_LIST SelectionList = (PDS_SELECTION_LIST)GlobalLock(stm.hGlobal);
            if (SelectionList != NULL)
            {
                if (SelectionList->cItems == 1)
                {
                    size_t nlen = wcslen(SelectionList->aDsSelection[0].pwzName);
                    if (nlen >= uSize)
                    {
                        nlen = uSize - 1;
                    }

                    memcpy(lpBuffer,
                           SelectionList->aDsSelection[0].pwzName,
                           nlen * sizeof(WCHAR));

                    lpBuffer[nlen] = L'\0';
                }

                GlobalUnlock(stm.hGlobal);
            }

            ReleaseStgMedium(&stm);
        }

        pdo->lpVtbl->Release(pdo);
    }

    return hRet;
}

static VOID
FreeObjectPicker(IN IDsObjectPicker *pDsObjectPicker)
{
    pDsObjectPicker->lpVtbl->Release(pDsObjectPicker);
}

/**
 * PURPOSE: Processes WM_COMMAND messages for the main frame window.
 */
static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HKEY hKeyRoot = 0, hKey = 0;
    LPCWSTR keyPath;
    LPCWSTR valueName;
    BOOL result = TRUE;
    REGSAM regsam = KEY_READ;
    int item;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(message);

    switch (LOWORD(wParam))
    {
    case ID_REGISTRY_LOADHIVE:
        LoadHive(hWnd);
        return TRUE;
    case ID_REGISTRY_UNLOADHIVE:
        UnloadHive(hWnd);
        return TRUE;
    case ID_REGISTRY_IMPORTREGISTRYFILE:
        ImportRegistryFile(hWnd);
        return TRUE;
    case ID_REGISTRY_EXPORTREGISTRYFILE:
        ExportRegistryFile(hWnd);
        return TRUE;
    case ID_REGISTRY_CONNECTNETWORKREGISTRY:
    {
        IDsObjectPicker *ObjectPicker;
        WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        HRESULT hRet;

        hRet = CoInitialize(NULL);
        if (SUCCEEDED(hRet))
        {
            hRet = InitializeRemoteRegistryPicker(&ObjectPicker);
            if (SUCCEEDED(hRet))
            {
                hRet = InvokeRemoteRegistryPickerDialog(ObjectPicker,
                                                        hWnd,
                                                        szComputerName,
                                                        ARRAY_SIZE(szComputerName));
                if (hRet == S_OK)
                {
                    // FIXME - connect to the registry
                }

                FreeObjectPicker(ObjectPicker);
            }

            CoUninitialize();
        }

        return TRUE;
    }
    case ID_REGISTRY_DISCONNECTNETWORKREGISTRY:
        return TRUE;
    case ID_REGISTRY_PRINT:
        PrintRegistryHive(hWnd, L"");
        return TRUE;
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        return TRUE;
    case ID_VIEW_STATUSBAR:
        toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        return TRUE;
    case ID_FAVOURITES_ADDTOFAVOURITES:
        DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ADDFAVORITES), hWnd, AddToFavoritesDlgProc);
        return TRUE;
    case ID_HELP_HELPTOPICS:
        WinHelpW(hWnd, L"regedit", HELP_FINDER, 0);
        return TRUE;
    case ID_HELP_ABOUT:
        ShowAboutBox(hWnd);
        return TRUE;
    case ID_VIEW_SPLIT:
    {
        RECT rt;
        POINT pt, pts;
        GetClientRect(g_pChildWnd->hWnd, &rt);
        pt.x = rt.left + g_pChildWnd->nSplitPos;
        pt.y = (rt.bottom / 2);
        pts = pt;
        if(ClientToScreen(g_pChildWnd->hWnd, &pts))
        {
            SetCursorPos(pts.x, pts.y);
            SetCursor(LoadCursorW(0, IDC_SIZEWE));
            SendMessageW(g_pChildWnd->hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
        }
        return TRUE;
    }
    case ID_EDIT_RENAME:
    case ID_EDIT_MODIFY:
    case ID_EDIT_MODIFY_BIN:
    case ID_EDIT_DELETE:
        regsam |= KEY_WRITE;
        break;
    }

    keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    valueName = GetValueName(g_pChildWnd->hListWnd, -1);
    if (keyPath)
    {
        if (RegOpenKeyExW(hKeyRoot, keyPath, 0, regsam, &hKey) != ERROR_SUCCESS)
            hKey = 0;
    }

    switch (LOWORD(wParam))
    {
    case ID_EDIT_MODIFY:
        if (valueName && ModifyValue(hWnd, hKey, valueName, FALSE))
            RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, FALSE);
        break;
    case ID_EDIT_MODIFY_BIN:
        if (valueName && ModifyValue(hWnd, hKey, valueName, TRUE))
            RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, FALSE);
        break;
    case ID_EDIT_RENAME:
        if (GetFocus() == g_pChildWnd->hListWnd)
        {
            if(ListView_GetSelectedCount(g_pChildWnd->hListWnd) == 1)
            {
                item = ListView_GetNextItem(g_pChildWnd->hListWnd, -1, LVNI_SELECTED);
                if(item > -1)
                {
                    (void)ListView_EditLabel(g_pChildWnd->hListWnd, item);
                }
            }
        }
        else if (GetFocus() == g_pChildWnd->hTreeWnd)
        {
            /* Get focused entry of treeview (if any) */
            HTREEITEM hItem = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
            if (hItem != NULL)
                (void)TreeView_EditLabel(g_pChildWnd->hTreeWnd, hItem);
        }
        break;
    case ID_EDIT_DELETE:
    {
        if (GetFocus() == g_pChildWnd->hListWnd && hKey)
        {
            UINT nSelected = ListView_GetSelectedCount(g_pChildWnd->hListWnd);
            if(nSelected >= 1)
            {
                WCHAR msg[128], caption[128];
                LoadStringW(hInst, IDS_QUERY_DELETE_CONFIRM, caption, ARRAY_SIZE(caption));
                LoadStringW(hInst, (nSelected == 1 ? IDS_QUERY_DELETE_ONE : IDS_QUERY_DELETE_MORE), msg, ARRAY_SIZE(msg));
                if(MessageBoxW(g_pChildWnd->hWnd, msg, caption, MB_ICONQUESTION | MB_YESNO) == IDYES)
                {
                    int ni, errs;

                    item = -1;
                    errs = 0;
                    while((ni = ListView_GetNextItem(g_pChildWnd->hListWnd, item, LVNI_SELECTED)) > -1)
                    {
                        valueName = GetValueName(g_pChildWnd->hListWnd, item);
                        if(RegDeleteValueW(hKey, valueName) != ERROR_SUCCESS)
                        {
                            errs++;
                        }
                        item = ni;
                    }

                    RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, FALSE);
                    if(errs > 0)
                    {
                        LoadStringW(hInst, IDS_ERR_DELVAL_CAPTION, caption, ARRAY_SIZE(caption));
                        LoadStringW(hInst, IDS_ERR_DELETEVALUE, msg, ARRAY_SIZE(msg));
                        MessageBoxW(g_pChildWnd->hWnd, msg, caption, MB_ICONSTOP);
                    }
                }
            }
        }
        else if (GetFocus() == g_pChildWnd->hTreeWnd)
        {
            if (keyPath == NULL || *keyPath == UNICODE_NULL)
            {
                MessageBeep(MB_ICONHAND);
            }
            else if (DeleteKey(hWnd, hKeyRoot, keyPath))
            {
                DeleteNode(g_pChildWnd->hTreeWnd, 0);
                RefreshTreeView(g_pChildWnd->hTreeWnd);
            }
        }
        break;
    }
    case ID_EDIT_NEW_STRINGVALUE:
        CreateNewValue(hKeyRoot, keyPath, REG_SZ);
        break;
    case ID_EDIT_NEW_BINARYVALUE:
        CreateNewValue(hKeyRoot, keyPath, REG_BINARY);
        break;
    case ID_EDIT_NEW_DWORDVALUE:
        CreateNewValue(hKeyRoot, keyPath, REG_DWORD);
        break;
    case ID_EDIT_NEW_MULTISTRINGVALUE:
        CreateNewValue(hKeyRoot, keyPath, REG_MULTI_SZ);
        break;
    case ID_EDIT_NEW_EXPANDABLESTRINGVALUE:
        CreateNewValue(hKeyRoot, keyPath, REG_EXPAND_SZ);
        break;
    case ID_EDIT_FIND:
        FindDialog(hWnd);
        break;
    case ID_EDIT_FINDNEXT:
        FindNextMessageBox(hWnd);
        break;
    case ID_EDIT_COPYKEYNAME:
        CopyKeyName(hWnd, hKeyRoot, keyPath);
        break;
    case ID_EDIT_PERMISSIONS:
        RegKeyEditPermissions(hWnd, hKeyRoot, NULL, keyPath);
        break;
    case ID_VIEW_REFRESH:
        RefreshTreeView(g_pChildWnd->hTreeWnd);
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, TRUE);
        break;
        //case ID_OPTIONS_TOOLBAR:
        //    toggle_child(hWnd, LOWORD(wParam), hToolBar);
        //    break;
    case ID_EDIT_NEW_KEY:
        CreateNewKey(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd));
        break;
    case ID_TREE_EXPANDBRANCH:
        TreeView_Expand(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), TVE_EXPAND);
        break;
    case ID_TREE_COLLAPSEBRANCH:
        TreeView_Expand(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), TVE_COLLAPSE);
        break;
    case ID_TREE_RENAME:
        SetFocus(g_pChildWnd->hTreeWnd);
        TreeView_EditLabel(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd));
        break;
    case ID_TREE_DELETE:
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), &hKeyRoot);
        if (keyPath == 0 || *keyPath == 0)
            MessageBeep(MB_ICONHAND);
        else if (DeleteKey(hWnd, hKeyRoot, keyPath))
            DeleteNode(g_pChildWnd->hTreeWnd, 0);
        break;
    case ID_TREE_EXPORT:
        ExportRegistryFile(g_pChildWnd->hTreeWnd);
        break;
    case ID_TREE_PERMISSIONS:
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), &hKeyRoot);
        RegKeyEditPermissions(hWnd, hKeyRoot, NULL, keyPath);
        break;
    case ID_SWITCH_PANELS:
        {
            BOOL bShiftDown = GetKeyState(VK_SHIFT) < 0;
            HWND hwndItem = GetNextDlgTabItem(g_pChildWnd->hWnd, GetFocus(), bShiftDown);
            if (hwndItem == g_pChildWnd->hAddressBarWnd)
                PostMessageW(hwndItem, EM_SETSEL, 0, -1);
            SetFocus(hwndItem);
        }
        break;
    case ID_ADDRESS_FOCUS:
        SendMessageW(g_pChildWnd->hAddressBarWnd, EM_SETSEL, 0, -1);
        SetFocus(g_pChildWnd->hAddressBarWnd);
        break;
    default:
        if ((LOWORD(wParam) >= ID_FAVORITES_MIN) && (LOWORD(wParam) <= ID_FAVORITES_MAX))
        {
            HMENU hMenu;
            MENUITEMINFOW mii;
            WCHAR szFavorite[512];

            hMenu = GetSubMenu(GetMenu(hWnd), FAVORITES_MENU_POSITION);

            memset(&mii, 0, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szFavorite;
            mii.cch = ARRAY_SIZE(szFavorite);

            if (GetMenuItemInfo(hMenu, LOWORD(wParam) - ID_FAVORITES_MIN, TRUE, &mii))
            {
                ChooseFavorite(szFavorite);
            }
        }
        else if ((LOWORD(wParam) >= ID_TREE_SUGGESTION_MIN) && (LOWORD(wParam) <= ID_TREE_SUGGESTION_MAX))
        {
            WORD wID = LOWORD(wParam);
            LPCWSTR s = Suggestions;
            while(wID > ID_TREE_SUGGESTION_MIN)
            {
                if (*s)
                    s += wcslen(s) + 1;
                wID--;
            }
            SelectNode(g_pChildWnd->hTreeWnd, s);
        }
        else
        {
            result = FALSE;
        }
        break;
    }

    if(hKey)
        RegCloseKey(hKey);
    return result;
}

/**
 * PURPOSE: Processes messages for the main frame window
 *
 * WM_COMMAND - process the application menu
 * WM_DESTROY - post a quit message and return
 */
LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    switch (message)
    {
    case WM_CREATE:
        // For now, the Help dialog item is disabled because of lacking of HTML Help support
        EnableMenuItem(GetMenu(hWnd), ID_HELP_HELPTOPICS, MF_BYCOMMAND | MF_GRAYED);
        GetClientRect(hWnd, &rc);
        CreateWindowExW(0, szChildClass, NULL,
                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                        hWnd, (HMENU)0, hInst, 0);
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam))
            return DefWindowProcW(hWnd, message, wParam, lParam);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE && g_pChildWnd)
            SetFocus(g_pChildWnd->hWnd);
        break;
    case WM_SIZE:
        resize_frame_client(hWnd);
        break;
    case WM_INITMENU:
        OnInitMenu(hWnd);
        break;
    case WM_ENTERMENULOOP:
        OnEnterMenuLoop(hWnd);
        break;
    case WM_EXITMENULOOP:
        OnExitMenuLoop(hWnd);
        break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    case WM_SYSCOLORCHANGE:
        /* Forward WM_SYSCOLORCHANGE to common controls */
        SendMessageW(g_pChildWnd->hListWnd, WM_SYSCOLORCHANGE, 0, 0);
        SendMessageW(g_pChildWnd->hTreeWnd, WM_SYSCOLORCHANGE, 0, 0);
        break;
    case WM_DESTROY:
        WinHelpW(hWnd, L"regedit", HELP_QUIT, 0);
        SaveSettings();
        PostQuitMessage(0);
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
