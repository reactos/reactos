/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/systempage.c
 * PURPOSE:     System page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *                        2011      Gregor Schneider <Gregor.Schneider@reactos.org>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include <share.h>

#include "treeview.h"
#include "uxthemesupp.h"

#include "regutils.h"
#include "utils.h"


extern "C" {

LPCWSTR lpszSystemIni = L"%SystemRoot%\\system.ini"; // or: %windir%\\... ?
LPCWSTR lpszWinIni    = L"%SystemRoot%\\win.ini";    // or: %windir%\\... ?

}

static LPCWSTR szMSConfigTok = L";msconfig "; // Note the trailing whitespace
static const size_t MSConfigTokLen = 10;


extern "C" {

DWORD GetSystemIniActivation(VOID)
{
    DWORD dwSystemIni = 0;
    RegGetDWORDValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\state", L"system.ini", &dwSystemIni);
    return dwSystemIni;
}

DWORD GetWinIniActivation(VOID)
{
    DWORD dwWinIni = 0;
    RegGetDWORDValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSConfig\\state", L"win.ini", &dwWinIni);
    return dwWinIni;
}

}



static HWND hTree = NULL;
static WCHAR szSearchString[MAX_VALUE_NAME] = L"";
static BOOL bMatchExactText = FALSE;
static BOOL bSearchSense    = TRUE; // TRUE == down, FALSE == up.
static BOOL bCaseSensitive  = FALSE;

static void
ToLower(LPWSTR lpszString)
{
    if (!lpszString)
        return;

    while (*lpszString)
    {
        *lpszString = towlower(*lpszString);
        ++lpszString;
    }
}

INT_PTR CALLBACK
FindDialogWndProc(HWND hDlg,
                  UINT message,
                  WPARAM wParam,
                  LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hTree = (HWND)lParam;

            Button_SetCheck(GetDlgItem(hDlg, IDC_CBX_FIND_WHOLE_WORD_ONLY), (bMatchExactText ? BST_CHECKED : BST_UNCHECKED));
            Button_SetCheck(GetDlgItem(hDlg, IDC_RB_FIND_DOWN), (bSearchSense ? BST_CHECKED   : BST_UNCHECKED)); // TRUE == down, FALSE == up.
            Button_SetCheck(GetDlgItem(hDlg, IDC_RB_FIND_UP  ), (bSearchSense ? BST_UNCHECKED : BST_CHECKED  )); // TRUE == down, FALSE == up.
            Button_SetCheck(GetDlgItem(hDlg, IDC_CBX_FIND_MATCH_CASE), (bCaseSensitive ? BST_CHECKED : BST_UNCHECKED));

            Edit_SetText(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT), szSearchString);
            SetFocus(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT));
            Edit_SetSel(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT), 0, -1);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    TVITEMEXW tvItemEx;
                    HTREEITEM htiIterator;
                    WCHAR label[MAX_VALUE_NAME] = L"";
                    WCHAR szTemp[MAX_VALUE_NAME];

                    bMatchExactText = (Button_GetCheck(GetDlgItem(hDlg, IDC_CBX_FIND_WHOLE_WORD_ONLY)) == BST_CHECKED);
                    bSearchSense    = ((Button_GetCheck(GetDlgItem(hDlg, IDC_RB_FIND_DOWN)) == BST_CHECKED) &&
                                       (Button_GetCheck(GetDlgItem(hDlg, IDC_RB_FIND_UP  )) == BST_UNCHECKED)); // TRUE == down, FALSE == up.
                    bCaseSensitive  = (Button_GetCheck(GetDlgItem(hDlg, IDC_CBX_FIND_MATCH_CASE)) == BST_CHECKED);

                    Edit_GetText(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT), szSearchString, _ARRAYSIZE(szSearchString));
                    wcscpy(szTemp, szSearchString);
                    if (!bCaseSensitive)
                        ToLower(szTemp);

                    for (htiIterator = ((Button_GetCheck(GetDlgItem(hDlg, IDC_CBX_FIND_FROM_BEGINNING)) == BST_CHECKED) ? (bSearchSense ? TreeView_GetFirst(hTree)
                                                                                                                                        : TreeView_GetLast(hTree))
                                                                                                                        : (bSearchSense ? TreeView_GetNext(hTree, TreeView_GetSelection(hTree))
                                                                                                                                        : TreeView_GetPrev(hTree, TreeView_GetSelection(hTree))));
                         htiIterator ;
                         htiIterator = (bSearchSense ? TreeView_GetNext(hTree, htiIterator)
                                                     : TreeView_GetPrev(hTree, htiIterator)))
                    {
                        SecureZeroMemory(&tvItemEx, sizeof(tvItemEx));

                        tvItemEx.hItem = htiIterator; // Handle of the item to be retrieved
                        tvItemEx.mask  = TVIF_HANDLE | TVIF_TEXT;
                        tvItemEx.pszText    = label;
                        tvItemEx.cchTextMax = MAX_VALUE_NAME;
                        TreeView_GetItem(hTree, &tvItemEx);
                        if (!bCaseSensitive)
                            ToLower(label);

                        if (bMatchExactText ? (_tcscmp(label, szTemp) == 0) : !!_tcsstr(label, szTemp)) // <-- hackish. A arranger.
                        {
                            TreeView_SelectItem(hTree, htiIterator);
                            EndDialog(hDlg, LOWORD(wParam));
                            return TRUE;
                        }
                        //MessageBox(NULL, label, _T("Info"), MB_ICONINFORMATION | MB_OK);
                    }

                    // FIXME: Localize!
                    MessageBoxW(hDlg, L"No correspondence found.", szAppName, MB_ICONINFORMATION | MB_OK);
                    SetFocus(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT));
                    Edit_SetSel(GetDlgItem(hDlg, IDC_TXT_FIND_TEXT), 0, -1);
                    //EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;

                default:
                    //break;
                    return FALSE;
            }
        }
    }

    return FALSE;
}



static void
TreeView_SetBOOLCheck(HWND hTree, HTREEITEM htiItem, BOOL bState, BOOL bPropagateStateToParent)
{
    if (!hTree || !htiItem)
        return;

    TreeView_SetCheckState(hTree, htiItem, bState);

    /*
     * Add or remove the token for tree leaves only.
     */
    if (!TreeView_GetChild(hTree, htiItem))
    {
        /* 1- Retrieve properties */
        TVITEMEXW tvItemEx;
        SecureZeroMemory(&tvItemEx, sizeof(tvItemEx));

        tvItemEx.hItem = htiItem; // Handle of the item to be retrieved.
        tvItemEx.mask  = TVIF_HANDLE | TVIF_TEXT;
        WCHAR label[MAX_VALUE_NAME] = L"";
        tvItemEx.pszText    = label;
        tvItemEx.cchTextMax = MAX_VALUE_NAME;
        TreeView_GetItem(hTree, &tvItemEx);

        if (!bState)
        {
            /* 2- Add the token IF NEEDED */
            if ((wcslen(tvItemEx.pszText) < MSConfigTokLen) || (_wcsnicmp(tvItemEx.pszText, szMSConfigTok, MSConfigTokLen) != 0))
            {
                LPWSTR newLabel = (LPWSTR)MemAlloc(0, (_tcslen(tvItemEx.pszText) + MSConfigTokLen + 1) * sizeof(WCHAR));
                wcscpy(newLabel, szMSConfigTok);
                wcscat(newLabel, tvItemEx.pszText);
                tvItemEx.pszText = newLabel;

                TreeView_SetItem(hTree, &tvItemEx);

                MemFree(newLabel);
            }
        }
        else
        {
            /* 2- Remove the token IF NEEDED */
            if ((wcslen(tvItemEx.pszText) >= MSConfigTokLen) && (_wcsnicmp(tvItemEx.pszText, szMSConfigTok, MSConfigTokLen) == 0))
            {
                LPWSTR newLabel = (LPWSTR)MemAlloc(0, (_tcslen(tvItemEx.pszText) - MSConfigTokLen + 1) * sizeof(WCHAR));
                wcscpy(newLabel, tvItemEx.pszText + MSConfigTokLen);
                tvItemEx.pszText = newLabel;

                TreeView_SetItem(hTree, &tvItemEx);

                // TODO: if one finds tvItemEx.pszText == L"", one can
                // directly remove the item (cf. message TVN_ENDLABELEDIT).

                MemFree(newLabel);
            }
        }
    }
    ////////////////////////

    for (HTREEITEM htiIterator = TreeView_GetChild(hTree, htiItem) ; htiIterator ; htiIterator = TreeView_GetNextSibling(hTree, htiIterator))
        TreeView_SetBOOLCheck(hTree, htiIterator, bState, FALSE);

    if (bPropagateStateToParent)
        TreeView_PropagateStateOfItemToParent(hTree, htiItem);

    return;
}

static void
LoadIniFile(HWND hTree, LPCWSTR lpszIniFile)
{
    // Ouverture en lecture (sans création de fichier si celui-ci n'esistait pas déjà)
    // d'un flux en mode texte, avec permission de lecture seule.
    DWORD dwNumOfChars = ExpandEnvironmentStringsW(lpszIniFile, NULL, 0);
    LPWSTR lpszFileName = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszIniFile, lpszFileName, dwNumOfChars);

    FILE* ini_file = _wfsopen(lpszFileName, L"rt", _SH_DENYWR); // r+t <-- read write text ; rt <-- read text
    MemFree(lpszFileName);

    if (!ini_file)
        return; // error

    WCHAR szLine[MAX_VALUE_NAME] = L"";
    TVINSERTSTRUCT tvis;
    HTREEITEM hParent = TVI_ROOT;
    BOOL bIsSection = FALSE;
    LPWSTR lpsz1 = NULL;
    LPWSTR lpsz2 = NULL;

    while (!feof(ini_file) && fgetws(szLine, _ARRAYSIZE(szLine), ini_file))
    {
        /* Skip hypothetical starting spaces or newline characters */
        lpsz1 = szLine;
        while (*lpsz1 == L' ' || *lpsz1 == L'\r' || *lpsz1 == L'\n')
            ++lpsz1;

        /* Skip empty lines */
        if (!*lpsz1)
            continue;

        /* Find the last newline character (if exists) and replace it by the NULL terminator */
        lpsz2 = lpsz1;
        while (*lpsz2)
        {
            if (*lpsz2 == L'\r' || *lpsz2 == L'\n')
            {
                *lpsz2 = L'\0';
                break;
            }

            ++lpsz2;
        }

        /* Check for new sections. They should be parent of ROOT. */
        if (*lpsz1 == L'[')
        {
            bIsSection = TRUE;
            hParent    = TVI_ROOT;
        }

        SecureZeroMemory(&tvis, sizeof(tvis));
        tvis.hParent        = hParent;
        tvis.hInsertAfter   = TVI_LAST;
        tvis.itemex.mask    = TVIF_TEXT; // TVIF_HANDLE | TVIF_TEXT;
        tvis.itemex.pszText = lpsz1;
        tvis.itemex.hItem   = TreeView_InsertItem(hTree, &tvis);

        /* The special ";msconfig " token disables the line */
        if (!bIsSection && _wcsnicmp(lpsz1, szMSConfigTok, MSConfigTokLen) == 0)
            TreeView_SetBOOLCheck(hTree, tvis.itemex.hItem, FALSE, TRUE);
        else
            TreeView_SetBOOLCheck(hTree, tvis.itemex.hItem, TRUE, TRUE);

        /*
         * Now, all the elements will be children of this section,
         * until we create a new one.
         */
        if (bIsSection)
        {
            bIsSection = FALSE;
            hParent    = tvis.itemex.hItem;
        }
    }

    fclose(ini_file);
    return;

    //// Test code for the TreeView ////
    /*
    HTREEITEM hItem[16];

    hItem[0] = InsertItem(hTree, _T("B"),TVI_ROOT,TVI_LAST);
    hItem[1] = InsertItem(hTree, _T("C"),TVI_ROOT,TVI_LAST);
    hItem[2] = InsertItem(hTree, _T("A"),TVI_ROOT,TVI_LAST);
    hItem[3] = InsertItem(hTree, _T("D"),TVI_ROOT,TVI_LAST);
        hItem[4] = InsertItem(hTree, _T("D-1"),hItem[3] ,TVI_LAST);
        hItem[5] = InsertItem(hTree, _T("D-2"),hItem[3] ,TVI_LAST);
            hItem[9] = InsertItem(hTree, _T("D-2-1"),hItem[5],TVI_LAST);
        hItem[6] = InsertItem(hTree, _T("D-3"),hItem[3] ,TVI_LAST);
            hItem[7] = InsertItem(hTree, _T("D-3-1"),hItem[6],TVI_LAST);
                hItem[10] = InsertItem(hTree, _T("D-3-1-1"),hItem[7],TVI_LAST);
                hItem[11] = InsertItem(hTree, _T("D-3-1-2"),hItem[7],TVI_LAST);
                hItem[12] = InsertItem(hTree, _T("D-3-1-3"),hItem[7],TVI_LAST);
                hItem[13] = InsertItem(hTree, _T("D-3-1-4"),hItem[7],TVI_LAST);
                hItem[14] = InsertItem(hTree, _T("D-3-1-5"),hItem[7],TVI_LAST);
                hItem[15] = InsertItem(hTree, _T("D-3-1-6"),hItem[7],TVI_LAST);
    hItem[13] = InsertItem(hTree, _T("E"),TVI_ROOT,TVI_LAST);
    */
    ////////////////////////////////////

}

static void
WriteIniFile(HWND hTree, LPCWSTR lpszIniFile)
{
    // Ouverture en écriture (avec création de fichier si celui-ci n'esistait pas déjà)
    // d'un flux en mode texte, avec permission de lecture seule.
#if 0
    DWORD dwNumOfChars = ExpandEnvironmentStringsW(lpszIniFile, NULL, 0);
    LPWSTR lpszFileName = MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszIniFile, lpszFileName, dwNumOfChars);
#else
    // HACK: delete these following lines when the program will be ready.
    DWORD dwNumOfChars = ExpandEnvironmentStringsW(lpszIniFile, NULL, 0) + 11;
    LPWSTR lpszFileName = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszIniFile, lpszFileName, dwNumOfChars);
    wcscat(lpszFileName, L"__tests.ini");
    // END HACK.
#endif

    FILE* ini_file = _wfsopen(lpszFileName, L"wt", _SH_DENYRW); // w+t <-- write read text ; wt <-- write text
    MemFree(lpszFileName);

    if (!ini_file)
        return; // error


    TVITEMEXW tvItemEx;
    WCHAR label[MAX_VALUE_NAME] = L"";
    // WCHAR szLine[MAX_VALUE_NAME] = L"";

    // for (HTREEITEM htiIterator = TreeView_GetRoot(hTree) ; htiIterator ; htiIterator = TreeView_GetNextSibling(hTree, htiIterator))
    for (HTREEITEM htiIterator = TreeView_GetFirst(hTree) ; htiIterator ; htiIterator = TreeView_GetNext(hTree, htiIterator))
    {
        SecureZeroMemory(&tvItemEx, sizeof(tvItemEx));

        tvItemEx.hItem = htiIterator; // Handle of the item to be retrieved.
        tvItemEx.mask  = TVIF_HANDLE | TVIF_TEXT;
        tvItemEx.pszText    = label;
        tvItemEx.cchTextMax = MAX_VALUE_NAME;
        TreeView_GetItem(hTree, &tvItemEx);

        // Write into the file.
        wcscat(label, L"\n");
        fputws(label, ini_file);
    }

    fclose(ini_file);
    return;
}

static void
Update_Btn_States(HWND hDlg)
{
    HWND hTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

    HTREEITEM hti     = TreeView_GetSelection(hTree);
    HTREEITEM htiPrev = TreeView_GetPrevSibling(hTree, hti);
    HTREEITEM htiNext = TreeView_GetNextSibling(hTree, hti);

    //
    // "Up" / "Down" buttons.
    //
    if (htiPrev)
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_UP), TRUE);
    else
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_UP), FALSE);

    if (htiNext)
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DOWN), TRUE);
    else
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DOWN), FALSE);

    //
    // "Enable" / "Disable" buttons.
    //
    UINT uCheckState = TreeView_GetCheckState(hTree, hti);
    if (uCheckState == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE) , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE), FALSE);
    }
    else if (uCheckState == 1)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE) , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE), TRUE);
    }
    else if (uCheckState == 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE) , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE) , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE), FALSE);
    }

    //
    // "Enable all" / "Disable all" buttons.
    //
    UINT uRootCheckState = TreeView_GetRealSubtreeState(hTree, TVI_ROOT);
    if (uRootCheckState == 0)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE_ALL) , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE_ALL), FALSE);
    }
    else if (uRootCheckState == 1)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE_ALL) , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE_ALL), TRUE);
    }
    else if (uRootCheckState == 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE_ALL) , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE_ALL), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_ENABLE_ALL) , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DISABLE_ALL), FALSE);
    }

    //
    // "Search" / "Edit" / "Delete" buttons.
    //
    if (TreeView_GetRoot(hTree))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_FIND)  , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_EDIT)  , TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DELETE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_FIND)  , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_EDIT)  , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SYSTEM_DELETE), FALSE);
    }

    return;
}


INT_PTR
CommonWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

            //
            // Initialize the styles.
            //
            TreeView_Set3StateCheck(hSystemTree);
            SetWindowTheme(hSystemTree, L"Explorer", NULL);

            TreeView_SetIndent(hSystemTree, TreeView_GetIndent(hSystemTree) + 2);

            /* Load data */
            LoadIniFile(hSystemTree, (LPCWSTR)((LPPROPSHEETPAGE)lParam)->lParam);

            /* Select the first item */
            TreeView_SelectItem(hSystemTree, TreeView_GetRoot(hSystemTree)); // Is it really necessary?
            SetFocus(hSystemTree);

            Update_Btn_States(hDlg);

            return TRUE;
        }

        case WM_DESTROY:
        {
            TreeView_Cleanup(GetDlgItem(hDlg, IDC_SYSTEM_TREE));
            return FALSE;
        }

        case WM_COMMAND:
        {
            HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

            switch (LOWORD(wParam))
            {
                case IDC_BTN_SYSTEM_UP:
                {
                    TreeView_UpItem(hSystemTree, TreeView_GetSelection(hSystemTree));
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    return TRUE;
                }

                case IDC_BTN_SYSTEM_DOWN:
                {
                    TreeView_DownItem(hSystemTree, TreeView_GetSelection(hSystemTree));
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    return TRUE;
                }

                case IDC_BTN_SYSTEM_ENABLE:
                {
                    HTREEITEM hItem = TreeView_GetSelection(hSystemTree);
                    TreeView_SetBOOLCheck(hSystemTree, hItem, TRUE, TRUE);
                    TreeView_SelectItem(hSystemTree, hItem);
                    Update_Btn_States(hDlg);

                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    return TRUE;
                }

                case IDC_BTN_SYSTEM_ENABLE_ALL:
                {
                    for (HTREEITEM htiIterator = TreeView_GetRoot(hSystemTree) ; htiIterator ; htiIterator = TreeView_GetNextSibling(hSystemTree, htiIterator))
                        TreeView_SetBOOLCheck(hSystemTree, htiIterator, TRUE, TRUE);

                    Update_Btn_States(hDlg);

                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    return TRUE;
                }

                case IDC_BTN_SYSTEM_DISABLE:
                {
                    HTREEITEM hItem = TreeView_GetSelection(hSystemTree);
                    TreeView_SetBOOLCheck(hSystemTree, hItem, FALSE, TRUE);
                    TreeView_SelectItem(hSystemTree, hItem);

                    Update_Btn_States(hDlg);

                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    return TRUE;
                }

                case IDC_BTN_SYSTEM_DISABLE_ALL:
                {
                    for (HTREEITEM htiIterator = TreeView_GetRoot(hSystemTree) ; htiIterator ; htiIterator = TreeView_GetNextSibling(hSystemTree, htiIterator))
                        TreeView_SetBOOLCheck(hSystemTree, htiIterator, FALSE, TRUE);

                    Update_Btn_States(hDlg);

                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    return TRUE;
                }

                case IDC_BTN_SYSTEM_FIND:
                {
                    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_FIND_DIALOG), hDlg /* hMainWnd */, FindDialogWndProc, (LPARAM)hSystemTree);
                    return TRUE;
                }

                case IDC_BTN_SYSTEM_NEW:
                {
                    HTREEITEM hInsertAfter = TreeView_GetSelection(hSystemTree);
                    HTREEITEM hNewItem = InsertItem(hSystemTree, L"", TreeView_GetParent(hSystemTree, hInsertAfter), hInsertAfter);
                    TreeView_EditLabel(hSystemTree, hNewItem);
                    TreeView_SelectItem(hSystemTree, hNewItem);

                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    return TRUE;
                }

                case IDC_BTN_SYSTEM_EDIT:
                {
                    TreeView_EditLabel(hSystemTree, TreeView_GetSelection(hSystemTree));
                    return TRUE;
                }

                case IDC_BTN_SYSTEM_DELETE:
                {
                    TreeView_DeleteItem(hSystemTree, TreeView_GetSelection(hSystemTree));
                    Update_Btn_States(hDlg);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    return TRUE;
                }

                default:
                    return FALSE;
            }
            // return FALSE;
        }

        case UM_CHECKSTATECHANGE:
        {
            HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

            /* Retrieve the new checked state of the item and handle the notification */
            HTREEITEM hItemChanged = (HTREEITEM)lParam;

            //
            // State before   |   State after
            // -------------------------------
            // 0 (unchecked)  |   1 (checked)
            // 1 (checked)    |   0 (unchecked)
            // 2 (grayed)     |   1 (checked) --> this case corresponds to the former
            //                |                   with 0 == 2 mod 2.
            //
            UINT uiCheckState = TreeView_GetCheckState(hSystemTree, hItemChanged) % 2;
            TreeView_SetBOOLCheck(hSystemTree, hItemChanged, uiCheckState ? FALSE : TRUE, TRUE);
            TreeView_SelectItem(hSystemTree, hItemChanged);
            Update_Btn_States(hDlg);

            PropSheet_Changed(GetParent(hDlg), hDlg);

            return TRUE;
        }

        case WM_NOTIFY:
        {
            if (((LPNMHDR)lParam)->idFrom == IDC_SYSTEM_TREE)
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case NM_CLICK:
                    {
                        HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

                        DWORD         dwpos = GetMessagePos();
                        TVHITTESTINFO ht    = {};
                        ht.pt.x = GET_X_LPARAM(dwpos);
                        ht.pt.y = GET_Y_LPARAM(dwpos);
                        MapWindowPoints(HWND_DESKTOP /*NULL*/, hSystemTree, &ht.pt, 1);

                        TreeView_HitTest(hSystemTree, &ht);

                        if (TVHT_ONITEMSTATEICON & ht.flags)
                        {
                            PostMessage(hDlg, UM_CHECKSTATECHANGE, 0, (LPARAM)ht.hItem);

                            // Disable default behaviour. Needed for the UM_CHECKSTATECHANGE
                            // custom notification to work as expected.
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }
                        /*
                        else
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                        */

                        return TRUE;
                    }

                    case TVN_KEYDOWN:
                    {
                        if (((LPNMTVKEYDOWN)lParam)->wVKey == VK_SPACE)
                        {
                            HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

                            HTREEITEM hti = TreeView_GetSelection(hSystemTree);

                            // Hack the tree item state. This is needed because whether or not
                            // TRUE is being returned, the default implementation of SysTreeView32
                            // is always being processed for a key down !
                            if (GetWindowLongPtr(hSystemTree, GWL_STYLE) & TVS_CHECKBOXES)
                            {
                                TreeView_SetItemState(hSystemTree, hti, INDEXTOSTATEIMAGEMASK(TreeView_GetCheckState(hSystemTree, hti)), TVIS_STATEIMAGEMASK);
                            }

                            PostMessage(hDlg, UM_CHECKSTATECHANGE, 0, (LPARAM)hti);

                            // Disable default behaviour. Needed for the UM_CHECKSTATECHANGE
                            // custom notification to work as expected.
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }

                        return TRUE;
                    }

                    case TVN_ENDLABELEDIT:
                    {
                        HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

                        /*
                         * Ehh yes, we have to deal with a "dialog proc", which is quite different from a "window proc":
                         *
                         * (excerpt from: MSDN library http://msdn.microsoft.com/en-us/library/ms645469(VS.85).aspx)
                         *
                         * Return Value
                         * ============
                         * INT_PTR
                         *
                         * Typically, the dialog box procedure should return TRUE if it processed the message, and FALSE if it did not.
                         * If the dialog box procedure returns FALSE, the dialog manager performs the default dialog operation in response
                         * to the message.
                         *
                         * If the dialog box procedure processes a message that requires a specific return value, the dialog box procedure
                         * should set the desired return value by calling SetWindowLong(hwndDlg, DWLP_MSGRESULT, lResult) immediately before
                         * returning TRUE. Note that you must call SetWindowLong immediately before returning TRUE; doing so earlier may result
                         * in the DWLP_MSGRESULT value being overwritten by a nested dialog box message.
                         *
                         * [...]
                         *
                         * Remarks
                         * =======
                         * You should use the dialog box procedure only if you use the dialog box class for the dialog box. This is the default
                         * class and is used when no explicit class is specified in the dialog box template. Although the dialog box procedure
                         * is similar to a window procedure, it must not call the DefWindowProc function to process unwanted messages. Unwanted
                         * messages are processed internally by the dialog box window procedure.
                         *
                         */

                        // A arranger un peu ???? Certainement.
                        TVITEMW truc = ((LPNMTVDISPINFO)lParam)->item;
                        if (truc.pszText)
                        {
                            if (!*truc.pszText)
                                TreeView_DeleteItem(hSystemTree, truc.hItem);

                            PropSheet_Changed(GetParent(hDlg), hDlg);

                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }
                        else
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);

                        Update_Btn_States(hDlg);
                        return TRUE;
                    }

                    case TVN_SELCHANGED:
                        Update_Btn_States(hDlg);
                        return TRUE;

                    default:
                        return FALSE;
                }
            }
            else
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                    {
                        // TODO: Enum the items.
                        // HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);
                        //
                        PropSheet_CancelToClose(GetParent(hDlg));

                        /* TODO: see :
                         *
                         * dll/win32/devmgr/advprop.c:                PropSheet_RebootSystem(hwndDlg);
                         * include/psdk/prsht.h:#define PropSheet_RebootSystem(d) SendMessage(d,PSM_REBOOTSYSTEM,0,0)
                         *
                         * dll/shellext/deskadp/deskadp.c:            PropSheet_RestartWindows(GetParent(This->hwndDlg));
                         * dll/shellext/deskmon/deskmon.c:            PropSheet_RestartWindows(GetParent(This->hwndDlg));
                         * include/psdk/prsht.h:#define PropSheet_RestartWindows(d) SendMessage(d,PSM_RESTARTWINDOWS,0,0)
                         *
                         * for their usage.
                         */
                        PropSheet_RebootSystem(GetParent(hDlg));
                        //PropSheet_RestartWindows(GetParent(hDlg));

                        WriteIniFile(GetDlgItem(hDlg, IDC_SYSTEM_TREE), (LPCWSTR)wParam);

                        // Since there are nothing to modify, applying modifications
                        // cannot return any error.
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                        //PropSheet_UnChanged(GetParent(hDlg) /*hMainWnd*/, hDlg);
                        return TRUE;
                    }

                    case PSN_HELP:
                    {
                        MessageBox(hDlg, _T("Help not implemented yet!"), _T("Help"), MB_ICONINFORMATION | MB_OK);
                        return TRUE;
                    }

                    case PSN_KILLACTIVE: // Is going to lose activation.
                    {
                        // Changes are always valid of course.
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                        return TRUE;
                    }

                    case PSN_QUERYCANCEL:
                    {
                        // Allows cancellation since there are nothing to cancel...
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                        return TRUE;
                    }

                    case PSN_QUERYINITIALFOCUS:
                    {
                        HWND hSystemTree = GetDlgItem(hDlg, IDC_SYSTEM_TREE);

                        // Give the focus on and select the first item.
                        ListView_SetItemState(hSystemTree, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)hSystemTree);
                        return TRUE;
                    }

                    //
                    // DO NOT TOUCH THESE NEXT MESSAGES, THEY ARE OK LIKE THIS...
                    //
                    case PSN_RESET: // Perform final cleaning, called before WM_DESTROY.
                        return TRUE;

                    case PSN_SETACTIVE: // Is going to gain activation.
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                        return TRUE;
                    }

                    default:
                        break;
                }
            }

            return FALSE;
        }

        default:
            return FALSE;
    }

    // return FALSE;
}


extern "C" {

INT_PTR CALLBACK
SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPCWSTR lpszIniFile = NULL;

    if (message == WM_INITDIALOG)
        lpszIniFile = (LPCWSTR)((LPPROPSHEETPAGE)lParam)->lParam;

    if ( (message == WM_NOTIFY) && (((LPNMHDR)lParam)->code == PSN_APPLY) )
        wParam = (WPARAM)lpszIniFile;

    return CommonWndProc(hDlg, message, wParam, lParam);
}

INT_PTR CALLBACK
WinPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPCWSTR lpszIniFile = NULL;

    if (message == WM_INITDIALOG)
        lpszIniFile = (LPCWSTR)((LPPROPSHEETPAGE)lParam)->lParam;

    if ( (message == WM_NOTIFY) && (((LPNMHDR)lParam)->code == PSN_APPLY) )
        wParam = (WPARAM)lpszIniFile;

    return CommonWndProc(hDlg, message, wParam, lParam);
}

}
