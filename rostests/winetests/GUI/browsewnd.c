/*
 * PROJECT:     ReactOS API Test GUI
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     browse dialog implementation
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

#define DLL_SEARCH_DIR L"\\Debug\\testlibs\\*"
#define IL_MAIN 0
#define IL_TEST 1

#define HAS_NO_CHILD 0
#define HAS_CHILD 1

typedef wchar_t *(__cdecl *DLLNAME)();
typedef int (_cdecl *MODULES)(char **);

static INT
GetNumberOfDllsInFolder(LPWSTR lpFolder)
{
    HANDLE hFind;
    WIN32_FIND_DATAW findFileData;
    INT numFiles = 0;

    hFind = FindFirstFileW(lpFolder,
                           &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DisplayError(GetLastError());
        return 0;
    }

    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            numFiles++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    return numFiles;
}

static INT
GetListOfTestDlls(PMAIN_WND_INFO pInfo)
{
    HANDLE hFind;
    WIN32_FIND_DATAW findFileData;
    WCHAR szDllPath[MAX_PATH];
    LPWSTR ptr;
    INT numFiles = 0;
    INT len;

    len = GetCurrentDirectory(MAX_PATH, szDllPath);
    if (!len) return 0;

    wcsncat(szDllPath, DLL_SEARCH_DIR, MAX_PATH - (len + 1));

    numFiles = GetNumberOfDllsInFolder(szDllPath);
    if (!numFiles) return 0;

    pInfo->lpDllList = HeapAlloc(GetProcessHeap(),
                                 0,
                                 numFiles * (MAX_PATH * sizeof(WCHAR)));
    if (!pInfo->lpDllList)
        return 0;

    hFind = FindFirstFileW(szDllPath,
                           &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DisplayError(GetLastError());
        HeapFree(GetProcessHeap(), 0, pInfo->lpDllList);
        return 0;
    }

    /* remove the glob */
    ptr = wcschr(szDllPath, L'*');
    if (ptr)
        *ptr = L'\0';

    /* don't mod our base pointer */
    ptr = pInfo->lpDllList;

    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            /* set the path */
            wcscpy(ptr, szDllPath);

            /* tag the file onto the path */
            len = MAX_PATH - (wcslen(ptr) + 1);
            wcsncat(ptr, findFileData.cFileName, len);

            /* move the pointer along by MAX_PATH */
            ptr += MAX_PATH;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    return numFiles;
}

static BOOL
NodeHasChild(PMAIN_WND_INFO pInfo,
             HTREEITEM hItem)
{
    TV_ITEM tvItem;

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_CHILDREN;

    (void)TreeView_GetItem(pInfo->hBrowseTV, &tvItem);

    return (tvItem.cChildren == 1);
}

static VOID
FreeItemTag(PMAIN_WND_INFO pInfo,
            HTREEITEM hItem)
{
    TV_ITEM tvItem;

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;

    (void)TreeView_GetItem(pInfo->hBrowseTV, &tvItem);

    HeapFree(GetProcessHeap(),
             0,
             (PTEST_ITEM)tvItem.lParam);
}

static VOID
TraverseTreeView(PMAIN_WND_INFO pInfo,
                 HTREEITEM hItem)
{
    while (NodeHasChild(pInfo, hItem))
    {
        HTREEITEM hChildItem;

        FreeItemTag(pInfo, hItem);

        hChildItem = TreeView_GetChild(pInfo->hBrowseTV,
                                       hItem);

        TraverseTreeView(pInfo,
                         hChildItem);

        hItem = TreeView_GetNextSibling(pInfo->hBrowseTV,
                                        hItem);
    }

    if (hItem)
    {
        /* loop the child items and free the tags */
        while (TRUE)
        {
            HTREEITEM hOldItem;

            FreeItemTag(pInfo, hItem);
            hOldItem = hItem;
            hItem = TreeView_GetNextSibling(pInfo->hBrowseTV,
                                            hItem);
            if (hItem == NULL)
            {
                hItem = hOldItem;
                break;
            }
        }

        hItem = TreeView_GetParent(pInfo->hBrowseTV,
                                   hItem);
    }
}

static HTREEITEM
InsertIntoTreeView(HWND hTreeView,
                   HTREEITEM hRoot,
                   LPWSTR lpLabel,
                   LPARAM Tag,
                   INT Image,
                   INT Child)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_CHILDREN | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = lstrlen(lpLabel);
    tvi.lParam = Tag;
    tvi.iImage = Image;
    tvi.iSelectedImage = Image;
    tvi.cChildren = Child;

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTreeView, &tvins);
}

static PTEST_ITEM
BuildTestItemData(LPWSTR lpDll,
                  LPWSTR lpRun)
{
    PTEST_ITEM pItem;

    pItem = (PTEST_ITEM)HeapAlloc(GetProcessHeap(),
                                  0,
                                  sizeof(TEST_ITEM));
    if (pItem)
    {
        if (lpDll)
            wcsncpy(pItem->szSelectedDll, lpDll, MAX_PATH);
        if (lpRun)
            wcsncpy(pItem->szRunString, lpRun, MAX_RUN_CMD);
    }

    return pItem;
}

static VOID
PopulateTreeView(PMAIN_WND_INFO pInfo)
{
    HTREEITEM hRoot;
    HIMAGELIST hImgList;
    PTEST_ITEM pTestItem;
    DLLNAME GetTestName;
    MODULES GetModulesInTest;
    HMODULE hDll;
    LPWSTR lpDllPath;
    LPWSTR lpTestName;
    INT RootImage, i;

    pInfo->hBrowseTV = GetDlgItem(pInfo->hBrowseDlg, IDC_TREEVIEW);

    (void)TreeView_DeleteAllItems(pInfo->hBrowseTV);

    hImgList = InitImageList(IDI_ICON,
                             IDI_TESTS,
                             16,
                             16);
    if (!hImgList) return;

    (void)TreeView_SetImageList(pInfo->hBrowseTV,
                                hImgList,
                                TVSIL_NORMAL);

    pTestItem = BuildTestItemData(L"", L"Full");

    /* insert the root item into the tree */
    hRoot = InsertIntoTreeView(pInfo->hBrowseTV,
                               NULL,
                               L"Full",
                               pTestItem,
                               IL_MAIN,
                               HAS_CHILD);

    for (i = 0; i < pInfo->numDlls; i++)
    {
        lpDllPath = pInfo->lpDllList + (MAX_PATH * i);

        hDll = LoadLibraryW(lpDllPath);
        if (hDll)
        {
            GetTestName = (DLLNAME)GetProcAddress(hDll, "GetTestName");
            if (GetTestName)
            {
                HTREEITEM hParent;
                LPSTR lpModules, ptr;
                LPWSTR lpModW;
                INT numMods;

                lpTestName = GetTestName();

                pTestItem = BuildTestItemData(lpDllPath, lpTestName);

                hParent = InsertIntoTreeView(pInfo->hBrowseTV,
                                             hRoot,
                                             lpTestName,
                                             pTestItem,
                                             IL_TEST,
                                             HAS_CHILD);
                if (hParent)
                {
                    /* Get the list of modules a dll offers. This is returned as list of
                     * Ansi null-terminated strings, terminated with an empty string (double null) */
                    GetModulesInTest = (MODULES)GetProcAddress(hDll, "GetModulesInTest");
                    if ((numMods = GetModulesInTest(&lpModules)))
                    {
                        ptr = lpModules;
                        while (numMods && *ptr != '\0')
                        {
                            /* convert the string to unicode */
                            if (AnsiToUnicode(ptr, &lpModW))
                            {
                                WCHAR szRunCmd[MAX_RUN_CMD];

                                _snwprintf(szRunCmd, MAX_RUN_CMD, L"%s:%s", lpTestName, lpModW);
                                pTestItem = BuildTestItemData(lpDllPath, szRunCmd);

                                InsertIntoTreeView(pInfo->hBrowseTV,
                                                   hParent,
                                                   lpModW,
                                                   pTestItem,
                                                   IL_TEST,
                                                   HAS_NO_CHILD);

                                HeapFree(GetProcessHeap(), 0, lpModW);
                            }

                            /* move onto next string */
                            while (*(ptr++) != '\0')
                                ;

                            numMods--;
                        }

                        HeapFree(GetProcessHeap(), 0, lpModules);
                    }
                }
            }

            FreeLibrary(hDll);
        }
    }

    if (hRoot)
    {
        TreeView_Expand(pInfo->hBrowseTV,
                        hRoot,
                        TVE_EXPAND);
    }
}

static VOID
PopulateTestList(PMAIN_WND_INFO pInfo)
{
    pInfo->numDlls = GetListOfTestDlls(pInfo);
    if (pInfo->numDlls)
    {
        PopulateTreeView(pInfo);
    }
}

static BOOL
OnInitBrowseDialog(HWND hDlg,
                   LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

    pInfo = (PMAIN_WND_INFO)lParam;

    pInfo->hBrowseDlg = hDlg;

    SetWindowLongPtr(hDlg,
                     GWLP_USERDATA,
                     (LONG_PTR)pInfo);

    PopulateTestList(pInfo);

    return TRUE;
}

BOOL CALLBACK
BrowseDlgProc(HWND hDlg,
              UINT Message,
              WPARAM wParam,
              LPARAM lParam)
{
    PMAIN_WND_INFO pInfo;

    /* Get the window context */
    pInfo = (PMAIN_WND_INFO)GetWindowLongPtr(hDlg,
                                             GWLP_USERDATA);
    if (pInfo == NULL && Message != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_INITDIALOG:
            return OnInitBrowseDialog(hDlg, lParam);

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    TV_ITEM tvItem;

                    tvItem.hItem = TreeView_GetSelection(pInfo->hBrowseTV);
                    tvItem.mask = TVIF_PARAM;

                    if (TreeView_GetItem(pInfo->hBrowseTV, &tvItem))
                    {
                        PTEST_ITEM pItem;

                        pItem = (PTEST_ITEM)tvItem.lParam;
                        if (pItem)
                            CopyMemory(&pInfo->SelectedTest, pItem, sizeof(TEST_ITEM));

                        EndDialog(hDlg,
                                  LOWORD(wParam));
                    }
                    else
                    {
                        DisplayMessage(L"Please select an item");
                    }

                    break;
                }

                case IDCANCEL:
                {
                    HeapFree(GetProcessHeap(), 0, pInfo->lpDllList);
                    pInfo->lpDllList = NULL;

                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
            }

            break;
        }

        case WM_DESTROY:
        {
            HTREEITEM hItem = TreeView_GetRoot(pInfo->hBrowseTV);

            TraverseTreeView(pInfo, hItem);

            pInfo->hBrowseDlg = NULL;

            break;
        }

HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}
