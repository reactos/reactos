/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGFIND.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        14 Jul 1994
*
*  Find routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regkey.h"
#include "regresid.h"
#include "reghelp.h"

#define SIZE_FINDSPEC                   (max(MAXKEYNAME, MAXVALUENAME_LENGTH))

TCHAR s_FindSpecification[SIZE_FINDSPEC] = { 0 };

#define FIND_EXACT                      0x00000001
#define FIND_KEYS                       0x00000002
#define FIND_VALUES                     0x00000004
#define FIND_DATA                       0x00000008

//  Initialized value is the default if we don't find last known state in the
//  registry.
DWORD g_FindFlags = FIND_KEYS | FIND_VALUES | FIND_DATA;

//  Global needed to monitor the find abort dialog status.
BOOL s_fContinueFind;

//
//  Reference data for the RegFind dialog.
//

typedef struct _REGFINDDATA {
    UINT LookForCount;
}   REGFINDDATA;

REGFINDDATA s_RegFindData;

//
//  Association between the items of the RegFind dialog and the find flags.
//

typedef struct _DLGITEMFINDFLAGASSOC {
    int DlgItem;
    DWORD Flag;
}   DLGITEMFINDFLAGASSOC;

const DLGITEMFINDFLAGASSOC s_DlgItemFindFlagAssoc[] = {
    IDC_WHOLEWORDONLY,      FIND_EXACT,
    IDC_FORKEYS,            FIND_KEYS,
    IDC_FORVALUES,          FIND_VALUES,
    IDC_FORDATA,            FIND_DATA
};

const DWORD s_RegFindHelpIDs[] = {
    IDC_FINDWHAT,      IDH_FIND_SEARCHTEXT,
    IDC_GROUPBOX,      IDH_REGEDIT_LOOK,
    IDC_FORKEYS,       IDH_REGEDIT_LOOK,
    IDC_FORVALUES,     IDH_REGEDIT_LOOK,
    IDC_FORDATA,       IDH_REGEDIT_LOOK,
    IDC_WHOLEWORDONLY, IDH_FIND_WHOLE,
    IDOK,              IDH_FIND_NEXT_BUTTON,

    0, 0
};

BOOL
PASCAL
FindCompare(
    LPTSTR lpString
    );

INT_PTR
PASCAL
RegFindDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
RegFind_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

VOID
PASCAL
RegFind_OnCommand(
    HWND hWnd,
    int DlgItem,
    HWND hControlWnd,
    UINT NotificationCode
    );

BOOL
PASCAL
RegFindAbortProc(
    HWND hRegFindAbortWnd
    );

INT_PTR
CALLBACK
RegFindAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

/*******************************************************************************
*
*  RegEdit_OnCommandFindNext
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCommandFindNext(
    HWND hWnd,
    BOOL fForceDialog
    )
{

    BOOL fSearchedToEnd;
    HWND hFocusWnd;
    LV_ITEM LVItem;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD Type;
    DWORD cbValueData;
    TV_ITEM TVItem;
    TCHAR KeyName[MAXKEYNAME];
    HWND hRegFindAbortWnd;
    HTREEITEM hTempTreeItem;
    UINT ExpandCounter;
    HKEY hRootKey;
    HKEY hKey;
    DWORD EnumIndex;
    DWORD cbValueName;
    BOOL fFoundMatch;
    TCHAR BestValueName[MAXVALUENAME_LENGTH];
    LPBYTE lpValueData;
    LV_FINDINFO LVFindInfo;

    fSearchedToEnd = FALSE;
    hFocusWnd = NULL;
    hRegFindAbortWnd = NULL;

    //
    //  Check if we're to show the find dialog.  This is either due to the user
    //  explicitly choosing the "Find" menu item or causing a "Find Next" with
    //  the search specification being uninitialized.
    //

    if (fForceDialog || s_FindSpecification[0] == 0) {

        if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_REGFIND), hWnd,
            RegFindDlgProc) != IDOK)
            return;

    }

    RegEdit_SetWaitCursor(TRUE);

    //
    //  Check if we're trying to finding either value names or data.  If so,
    //  then the next match might be part of the current ValueList.
    //

    if (g_FindFlags & (FIND_VALUES | FIND_DATA)) {

        LVItem.iItem = ListView_GetNextItem(g_RegEditData.hValueListWnd, -1,
            LVNI_FOCUSED);
        LVItem.iSubItem = 0;
        LVItem.mask = LVIF_TEXT;
        LVItem.pszText = ValueName;
        LVItem.cchTextMax = sizeof(ValueName)/sizeof(TCHAR);

        //
        //  Walk over all of the rest of the value names attempting to find a
        //  match.
        //

        while ((LVItem.iItem = ListView_GetNextItem(g_RegEditData.hValueListWnd,
            LVItem.iItem, LVNI_ALL)) != -1) {

            ListView_GetItem(g_RegEditData.hValueListWnd, &LVItem);

            //
            //  Check if this value name meets our search specification.  We'll
            //  assume that this value name still exists.
            //

            if ((g_FindFlags & FIND_VALUES) && FindCompare(ValueName))
                goto SelectListItem;

            //
            //  Check if this value data meets our search specification.  We'll
            //  have to go back to the registry to determine this.
            //

            if (g_FindFlags & FIND_DATA) {

                cbValueData = sizeof(g_ValueDataBuffer);

                if (RegQueryValueEx(g_RegEditData.hCurrentSelectionKey,
                    ValueName, NULL, &Type, g_ValueDataBuffer, &cbValueData) ==
                    ERROR_SUCCESS && IsRegStringType(Type)) {

                    if (FindCompare((PTSTR)g_ValueDataBuffer))
                        goto SelectListItem;

                }

            }

        }

    }

    //
    //  Searching the registry (especially with this code!) is a lengthy
    //  operation, so we must provide a way for the user to cancel the
    //  operation.
    //

    s_fContinueFind = TRUE;

    if ((hRegFindAbortWnd = CreateDialog(g_hInstance,
        MAKEINTRESOURCE(IDD_REGFINDABORT), hWnd, RegFindAbortDlgProc)) !=
        NULL) {

        EnableWindow(hWnd, FALSE);

        //
        //  Major hack:  The following code sequence relies heavily on the
        //  TreeView to maintain the state of the find process.  Even though I'm
        //  inserting and deleting non-visible tree items, the TreeView
        //  currently flickers despite this.
        //
        //  So, we set this internal flag and turn off the TreeView's redraw
        //  flag.  Whenever we get a WM_PAINT message for our main window, we
        //  temporarily "let" it redraw itself then and only then.  That way,
        //  the user can move the modeless abort dialog or switch away and back
        //  and still have the TreeView look normal.
        //
        //  Yes, it's difficult at this time to fix the TreeView's paint logic.
        //

        g_RegEditData.fProcessingFind = TRUE;
        SetWindowRedraw(g_RegEditData.hKeyTreeWnd, FALSE);

    }

    //
    //  Either the user wasn't trying to find value names or data or else no
    //  matches were found.  This means that we must move on to the next branch
    //  of the registry.
    //
    //  We first walk into the children of the current branch, then the
    //  siblings, and finally pop back through the parent.
    //
    //  We use the information already in the KeyTree pane as much as possible.
    //

    ExpandCounter = 0;
    fFoundMatch = FALSE;
    BestValueName[0] = '\0';
    lpValueData = (g_FindFlags & FIND_DATA) ? g_ValueDataBuffer : NULL;

    TVItem.mask = TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN;
    TVItem.pszText = KeyName;
    TVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

    TVItem.hItem = TreeView_GetSelection(g_RegEditData.hKeyTreeWnd);
    TreeView_GetItem(g_RegEditData.hKeyTreeWnd, &TVItem);

    while (TRUE) {

        //
        //  Check if we should cancel the find operation.  If so, restore our
        //  initial state and exit.
        //

        if (!RegFindAbortProc(hRegFindAbortWnd)) {

            if (ExpandCounter) {

                hTempTreeItem = TVItem.hItem;

                do {

                    hTempTreeItem =
                        TreeView_GetParent(g_RegEditData.hKeyTreeWnd,
                        hTempTreeItem);

                }   while (--ExpandCounter);

                TreeView_Expand(g_RegEditData.hKeyTreeWnd, hTempTreeItem,
                    TVE_COLLAPSE | TVE_COLLAPSERESET);

            }

            goto DismissRegFindAbortWnd;

        }

        //
        //  Does this branch have any children?  This would have been determined
        //  when the tree item was built by the routine KeyTree_ExpandBranch.
        //

        if (TVItem.cChildren) {

            //
            //  The branch may have children, but it may not have been expanded
            //  yet.
            //

            if ((hTempTreeItem = TreeView_GetChild(g_RegEditData.hKeyTreeWnd,
                TVItem.hItem)) == NULL) {

                if (!KeyTree_ExpandBranch(g_RegEditData.hKeyTreeWnd,
                    TVItem.hItem))
                    goto SkipToSibling;

                if ((hTempTreeItem = TreeView_GetChild(g_RegEditData.hKeyTreeWnd,
                    TVItem.hItem)) == NULL)
                    goto SkipToSibling;

                ExpandCounter++;

            }

            TVItem.hItem = hTempTreeItem;

        }

        //
        //  The branch doesn't have any children, so we'll move on to the next
        //  sibling of the current branch.  If none exists, then try finding
        //  the next sibling of the parent branch, and so on.
        //

        else {

SkipToSibling:
            while (TRUE) {

                if ((hTempTreeItem =
                    TreeView_GetNextSibling(g_RegEditData.hKeyTreeWnd,
                    TVItem.hItem)) != NULL) {

                    TVItem.hItem = hTempTreeItem;
                    break;

                }

                //
                //  If no more parents exist, then we've finished searching the
                //  tree.  We're outta here!
                //

                if ((TVItem.hItem =
                    TreeView_GetParent(g_RegEditData.hKeyTreeWnd,
                    TVItem.hItem)) == NULL) {

                    fSearchedToEnd = TRUE;

                    goto DismissRegFindAbortWnd;

                }

                if (ExpandCounter) {

                    ExpandCounter--;

                    TreeView_Expand(g_RegEditData.hKeyTreeWnd, TVItem.hItem,
                        TVE_COLLAPSE | TVE_COLLAPSERESET);

                }

            }

        }

        //
        //  If we made it this far, then we're at the next branch of the
        //  registry to evaluate.
        //

        TreeView_GetItem(g_RegEditData.hKeyTreeWnd, &TVItem);

        //
        //  Check if we're trying to find keys.
        //

        if (g_FindFlags & FIND_KEYS) {

            if (FindCompare(KeyName))
                goto SelectTreeItem;

        }

        //
        //  Check if we're trying to find value names or data.
        //

        if (g_FindFlags & (FIND_VALUES | FIND_DATA)) {

            //
            //  Try to open the registry at the new current branch.
            //

            hRootKey = KeyTree_BuildKeyPath(g_RegEditData.hKeyTreeWnd,
                TVItem.hItem, KeyName, BKP_TOSUBKEY);

            if(hRootKey && RegOpenKeyEx(hRootKey,KeyName,0,KEY_QUERY_VALUE,&hKey) ==
                ERROR_SUCCESS) {

                //
                //  Here's the simple case-- we're trying to find an exact match
                //  for a value name.  We can just use the registry API to do
                //  this for us!
                //

                if ((g_FindFlags & (FIND_VALUES | FIND_DATA | FIND_EXACT)) ==
                    (FIND_VALUES | FIND_EXACT)) {

                    if (RegQueryValueEx(hKey, s_FindSpecification, NULL, NULL,
                        NULL, NULL) == ERROR_SUCCESS) {

                        lstrcpy(BestValueName, s_FindSpecification);
                        fFoundMatch = TRUE;

                    }

                }

                //
                //  Bummer... we need to walk through all of the registry
                //  value/data pairs for this key to try to find a match.  Even
                //  worse, we have to look at _all_ of the entries, not just the
                //  first hit... we must display the first alphabetically
                //  matching entry!
                //

                else {

                    EnumIndex = 0;

                    while (TRUE) {

                        cbValueName = sizeof(ValueName)/sizeof(TCHAR);
                        cbValueData = sizeof(g_ValueDataBuffer);

                        if (RegEnumValue(hKey, EnumIndex++, ValueName,
                            &cbValueName, NULL, &Type, lpValueData,
                            &cbValueData) != ERROR_SUCCESS)
                            break;

                        if (((g_FindFlags & FIND_VALUES) &&
                            FindCompare(ValueName)) ||
                            ((g_FindFlags & FIND_DATA) && IsRegStringType(Type) &&
                            FindCompare((PTSTR)g_ValueDataBuffer))) {

                            //
                            //  We've got to check if we've found a "better"
                            //  value name to display-- one that's at the top of
                            //  the sorted list.
                            //

                            if (fFoundMatch) {

                                if (lstrcmpi(BestValueName, ValueName) > 0)
                                    lstrcpy(BestValueName, ValueName);

                            }

                            else {

                                lstrcpy(BestValueName, ValueName);
                                fFoundMatch = TRUE;

                            }

                        }

                    }

                }

                RegCloseKey(hKey);

                if (fFoundMatch)
                    goto SelectTreeItem;

            }

        }

    }

SelectTreeItem:
    TreeView_EnsureVisible(g_RegEditData.hKeyTreeWnd, TVItem.hItem);
    TreeView_SelectItem(g_RegEditData.hKeyTreeWnd, TVItem.hItem);

    if (!fFoundMatch)
        hFocusWnd = g_RegEditData.hKeyTreeWnd;

    else {

        //
        //  Right now, the TreeView_SelectItem above will cause the ValueListWnd
        //  to update, but only after a short time delay.  We want the list
        //  immediately updated, so force the timer to go off now.
        //

        RegEdit_OnSelChangedTimer(hWnd);

        if (BestValueName[0] == 0)
            LVItem.iItem = 0;

        else {

            LVFindInfo.flags = LVFI_STRING;
            LVFindInfo.psz = BestValueName;

            LVItem.iItem = ListView_FindItem(g_RegEditData.hValueListWnd,
                -1, &LVFindInfo);

        }

SelectListItem:
        ListView_SetItemState(g_RegEditData.hValueListWnd, -1, 0,
            LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(g_RegEditData.hValueListWnd, LVItem.iItem,
            LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_RegEditData.hValueListWnd, LVItem.iItem,
            FALSE);

        hFocusWnd = g_RegEditData.hValueListWnd;

    }

DismissRegFindAbortWnd:
    RegEdit_SetWaitCursor(FALSE);

    if (hRegFindAbortWnd != NULL) {

        g_RegEditData.fProcessingFind = FALSE;
        SetWindowRedraw(g_RegEditData.hKeyTreeWnd, TRUE);

        EnableWindow(hWnd, TRUE);
        DestroyWindow(hRegFindAbortWnd);

    }

    if (hFocusWnd != NULL)
        SetFocus(hFocusWnd);

    if (fSearchedToEnd)
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_SEARCHEDTOEND),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONINFORMATION | MB_OK);

}

/*******************************************************************************
*
*  FindCompare
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
FindCompare(
    LPTSTR lpString
    )
{

    if (g_FindFlags & FIND_EXACT)
        return lstrcmpi(lpString, s_FindSpecification) == 0;

    else
        return StrStrI(lpString, s_FindSpecification) != NULL;

}

/*******************************************************************************
*
*  RegFindDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR
PASCAL
RegFindDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        HANDLE_MSG(hWnd, WM_INITDIALOG, RegFind_OnInitDialog);
        HANDLE_MSG(hWnd, WM_COMMAND, RegFind_OnCommand);

        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) s_RegFindHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) s_RegFindHelpIDs);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  RegFind_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
RegFind_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    UINT Counter;
    int DlgItem;

    //
    //  Initialize the "Find What" edit control.
    //

    SendDlgItemMessage(hWnd, IDC_FINDWHAT, EM_SETLIMITTEXT,
        SIZE_FINDSPEC, 0);
    SetDlgItemText(hWnd, IDC_FINDWHAT, s_FindSpecification);

    //
    //  Initialize the checkboxes based on the state of the global find flags.
    //

    s_RegFindData.LookForCount = 0;

    for (Counter = 0; Counter < sizeof(s_DlgItemFindFlagAssoc) /
        sizeof(DLGITEMFINDFLAGASSOC); Counter++) {

        if (g_FindFlags & s_DlgItemFindFlagAssoc[Counter].Flag) {

            DlgItem = s_DlgItemFindFlagAssoc[Counter].DlgItem;

            CheckDlgButton(hWnd, DlgItem, TRUE);

            if (DlgItem >= IDC_FORKEYS && DlgItem <= IDC_FORDATA)
                s_RegFindData.LookForCount++;

        }

    }

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);
    UNREFERENCED_PARAMETER(lParam);

}

/*******************************************************************************
*
*  RegFind_OnCommand
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegFind_OnCommand(
    HWND hWnd,
    int DlgItem,
    HWND hControlWnd,
    UINT NotificationCode
    )
{

    UINT Counter;

    if (DlgItem >= IDC_FORKEYS && DlgItem <= IDC_FORDATA) {

        if (NotificationCode == BN_CLICKED) {

            IsDlgButtonChecked(hWnd, DlgItem) ? s_RegFindData.LookForCount++ :
                s_RegFindData.LookForCount--;

            goto EnableFindNextButton;

        }

    }

    else {

        switch (DlgItem) {

            case IDC_FINDWHAT:
                if (NotificationCode == EN_CHANGE) {

EnableFindNextButton:
                    EnableWindow(GetDlgItem(hWnd, IDOK),
                        s_RegFindData.LookForCount > 0 &&
                        SendDlgItemMessage(hWnd, IDC_FINDWHAT,
                        WM_GETTEXTLENGTH, 0, 0) != 0);

                }
                break;

            case IDOK:
                GetDlgItemText(hWnd, IDC_FINDWHAT, s_FindSpecification,
                    sizeof(s_FindSpecification)/sizeof(TCHAR));

                for (Counter = 0; Counter < sizeof(s_DlgItemFindFlagAssoc) /
                    sizeof(DLGITEMFINDFLAGASSOC); Counter++) {

                    if (IsDlgButtonChecked(hWnd,
                        s_DlgItemFindFlagAssoc[Counter].DlgItem))
                        g_FindFlags |= s_DlgItemFindFlagAssoc[Counter].Flag;
                    else
                        g_FindFlags &= ~s_DlgItemFindFlagAssoc[Counter].Flag;

                }

                //  FALL THROUGH

            case IDCANCEL:
                EndDialog(hWnd, DlgItem);
                break;

        }

    }

}

/*******************************************************************************
*
*  RegFindAbortProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     (returns), TRUE to continue the find, else FALSE to cancel.
*
*******************************************************************************/

BOOL
PASCAL
RegFindAbortProc(
    HWND hRegFindAbortWnd
    )
{

    while (s_fContinueFind && MessagePump(hRegFindAbortWnd))
        ;

    return s_fContinueFind;

}

/*******************************************************************************
*
*  RegAbortDlgProc
*
*  DESCRIPTION:
*     Callback procedure for the RegAbort dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegAbort window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

INT_PTR
CALLBACK
RegFindAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        case WM_INITDIALOG:
            break;

        case WM_CLOSE:
        case WM_COMMAND:
            s_fContinueFind = FALSE;
            break;

        default:
            return FALSE;

    }

    return TRUE;

}
