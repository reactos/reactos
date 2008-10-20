/*
 * PROJECT:     ReactOS API Test GUI
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     browse dialog implementation
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include <precomp.h>

#define EXE_SEARCH_DIR L"\\Debug\\testexes\\*"
#define IL_MAIN 0
#define IL_TEST 1

#define HAS_NO_CHILD 0
#define HAS_CHILD 1


static INT
GetNumberOfExesInFolder(LPWSTR lpFolder)
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
GetListOfTestExes(PMAIN_WND_INFO pInfo)
{
    HANDLE hFind;
    WIN32_FIND_DATAW findFileData;
    WCHAR szExePath[MAX_PATH];
    LPWSTR ptr;
    INT numFiles = 0;
    INT len;

    len = GetCurrentDirectory(MAX_PATH, szExePath);
    if (!len) return 0;

    wcsncat(szExePath, EXE_SEARCH_DIR, MAX_PATH - (len + 1));

    numFiles = GetNumberOfExesInFolder(szExePath);
    if (!numFiles) return 0;

    pInfo->lpExeList = HeapAlloc(GetProcessHeap(),
                                 0,
                                 numFiles * (MAX_PATH * sizeof(WCHAR)));
    if (!pInfo->lpExeList)
        return 0;

    hFind = FindFirstFileW(szExePath,
                           &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        DisplayError(GetLastError());
        HeapFree(GetProcessHeap(), 0, pInfo->lpExeList);
        return 0;
    }

    /* remove the glob */
    ptr = wcschr(szExePath, L'*');
    if (ptr)
        *ptr = L'\0';

    /* don't modify our base pointer */
    ptr = pInfo->lpExeList;

    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            /* set the path */
            wcscpy(ptr, szExePath);

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
BuildTestItemData(LPWSTR lpExe,
                  LPWSTR lpRun)
{
    PTEST_ITEM pItem;

    pItem = (PTEST_ITEM)HeapAlloc(GetProcessHeap(),
                                  0,
                                  sizeof(TEST_ITEM));
    if (pItem)
    {
        if (lpExe)
            wcsncpy(pItem->szSelectedExe, lpExe, MAX_PATH);
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
    LPWSTR lpExePath;
    LPWSTR lpTestName;
    INT i;

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
                               (LPARAM)pTestItem,
                               IL_MAIN,
                               HAS_CHILD);

    for (i = 0; i < pInfo->numExes; i++)
    {
        HTREEITEM hParent;
        LPWSTR lpStr;

        lpExePath = pInfo->lpExeList + (MAX_PATH * i);

        lpTestName = wcsrchr(lpExePath, L'\\');
        if (lpTestName)
        {
            lpTestName++;

            lpStr = wcschr(lpTestName, L'_');
            if (lpStr)
            {
                //FIXME: Query the test name from the exe directly

                pTestItem = BuildTestItemData(lpExePath, lpTestName);

                hParent = InsertIntoTreeView(pInfo->hBrowseTV,
                                             hRoot,
                                             lpTestName,
                                             (LPARAM)pTestItem,
                                             IL_TEST,
                                             HAS_CHILD);
            }
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
    pInfo->numExes = GetListOfTestExes(pInfo);
    if (pInfo->numExes)
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

                    return TRUE;
                }

                case IDCANCEL:
                {
                    HeapFree(GetProcessHeap(), 0, pInfo->lpExeList);
                    pInfo->lpExeList = NULL;

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
