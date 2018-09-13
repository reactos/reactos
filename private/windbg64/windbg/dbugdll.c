/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbugdll.c

Abstract:

    This file contains the code for dealing with the Debugger Dll Dialog box

Author:

    Griffith Wm. Kadnier (v-griffk)

Environment:

    Win32 - User

--*/


#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"



PSTR pszSelectedTL;


void
AddDataToList(
    HWND hwndDlg, 
    CAll_TLs_WKSP * pCCopyOf_All_TLs_WrkSpc
    )
{
    Assert(pCCopyOf_All_TLs_WrkSpc);
    
    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);

    ///////////////////////////////////////////
    // Add the data to the list view
    ///////////////////////////////////////////
    ListView_DeleteAllItems(hwndList);
    
    //
    // Add the individual items
    //
    LV_ITEM         lvi;  
    
    // Initialize LV_ITEM members that are common to all items. 
    memset(&lvi, 0, sizeof(lvi));
    
    lvi.mask = LVIF_TEXT; // | LVIF_IMAGE | LVIF_STATE; 
    //lvi.lParam = 0;    // item data  
    //lvi.state = 0;     
    //lvi.stateMask = 0; 
    
    int nItems = 0;
    
    TListEntry<CIndiv_TL_WKSP *> * pContEntry = pCCopyOf_All_TLs_WrkSpc->m_listConts.FirstEntry();
    for (; pContEntry != pCCopyOf_All_TLs_WrkSpc->m_listConts.Stop(); 
        pContEntry = pContEntry->Flink, nItems++) {
        
        CIndiv_TL_WKSP * pIndTl = pContEntry->m_tData;
        AssertType(*pIndTl, CIndiv_TL_WKSP);
        
        //
        // Add name (main item)
        //
        lvi.iItem = nItems; 
        lvi.pszText = (LPSTR) pIndTl->m_pszRegistryName;
        lvi.iSubItem = 0;         
        
        // Index of the new item. 
        int nIdx = ListView_InsertItem(hwndList, &lvi);
        Assert(-1 != nIdx);       
        
        //
        // Add description (sub item)
        //
        ListView_SetItemText(hwndList, nIdx, 1, pIndTl->m_pszDescription);
        
        //
        // Add DLL (sub item)
        //
        ListView_SetItemText(hwndList, nIdx, 2, pIndTl->m_pszDll);
        
        //
        // Add params (sub item)
        //
        ListView_SetItemText(hwndList, nIdx, 3, pIndTl->m_pszParams);
        
        //
        // We perform the test because this routine is called on 2 occasions
        // during init and after we have edited an entry. If we have something 
        // selected we don't want to overwrite it.
        if (0 == GetWindowTextLength(GetDlgItem(hwndDlg, IDC_STEXT_SELECT) )) {
            // Display the short name of the selected transport layer.
            if (pszSelectedTL) {
                SetDlgItemText(hwndDlg, IDC_STEXT_SELECT, pszSelectedTL);
            }
        }
    }
 
    // Select the first item
    ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000F);
}

//
// Used primarily by DlgProc_TransportLayer, however, EditTransportLayersDlgProc
//  also uses it to check for duplicates.
//
static CAll_TLs_WKSP * g_pCCopyOf_All_TLs_WrkSpc = NULL;



INT_PTR
WINAPI
EditTransportLayersDlgProc(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++
Routine Description:
    Edit a transport layer

Arguments:
Return Value:
    See docs for "DialogBox"
--*/
{
    static DWORD HelpArray[]=
    {
        ID_CHDBDLL_NAME, IDH_DISPNAME,
        ID_CHDBDLL_DESC, IDH_DESC,
        ID_CHDBDLL_PATH, IDH_PATH,
        ID_CHDBDLL_PARAM, IDH_PARAMETERS,
        0, 0
    };
    static CIndiv_TL_WKSP * pTL = NULL;
    
    switch (uMsg) {        
    case WM_INITDIALOG:
        //
        // lParam point to a transport layer.
        
        Assert(lParam);
        pTL = (CIndiv_TL_WKSP *) lParam;
        AssertType(*pTL, CIndiv_TL_WKSP);
        
        SetDlgItemText(hwndDlg, ID_CHDBDLL_NAME, pTL->m_pszRegistryName);
        SetDlgItemText(hwndDlg, ID_CHDBDLL_DESC, pTL->m_pszDescription);
        SetDlgItemText(hwndDlg, ID_CHDBDLL_PATH, pTL->m_pszDll);
        SetDlgItemText(hwndDlg, ID_CHDBDLL_PARAM, pTL->m_pszParams);
        return TRUE;
        
    case WM_DESTROY:
        pTL = NULL;
        return TRUE;
        
    case WM_COMMAND:       
        switch (wParam) {
        case IDOK:
            {
                char szName[MAX_MSG_TXT];
                char szDesc[MAX_MSG_TXT];
                char szDll[MAX_MSG_TXT];
                char szParams[MAX_MSG_TXT];
                
                GetDlgItemText(hwndDlg, ID_CHDBDLL_NAME, szName, sizeof(szName));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_DESC, szDesc, sizeof(szDesc));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_PATH, szDll, sizeof(szDll));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_PARAM, szParams, sizeof(szParams));

                //
                // Verify that key is not empty, and unique
                //
                
                if (0 == strlen(szName)) {
                    WKSP_MsgBox(NULL, ERR_Empty_Shortname);
                    return TRUE;
                }

                // Are we adding or editing
                if (NULL == pTL->m_pszRegistryName) {
                    // If it doesn't have a registry name, then we are adding
                    // Make sure the name isn't duplicated.
                    if (g_pCCopyOf_All_TLs_WrkSpc->
                            m_listConts.Find(szName, WKSP_Generic_CmpRegName) ) {

                        WKSP_MsgBox(NULL, ERR_Not_Unique_Shortname, szName);
                        return TRUE;
                    }
                } else {
                    // If it does have a registry name, then we are editing
                    if (strcmp(szName, pTL->m_pszRegistryName)) {
                        // The name has changed, then make sure it isn't duplicated.
                        if (g_pCCopyOf_All_TLs_WrkSpc->
                                m_listConts.Find(szName, WKSP_Generic_CmpRegName) ) {

                            WKSP_MsgBox(NULL, ERR_Not_Unique_Shortname, szName);
                            return TRUE;
                        }
                    }                
                }

                pTL->SetRegistryName(szName);

                FREE_STR(pTL->m_pszDescription);
                pTL->m_pszDescription = _strdup(szDesc);
                
                FREE_STR(pTL->m_pszDll);
                pTL->m_pszDll = _strdup(szDll);
                
                FREE_STR(pTL->m_pszParams);
                pTL->m_pszParams = _strdup(szParams);

                EndDialog(hwndDlg, TRUE);
            }
            return TRUE;
            
        case IDCANCEL:
            EndDialog(hwndDlg, FALSE);
            return TRUE;
            
        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
                (ULONG_PTR)(LPVOID) HelpArray );
            return TRUE;
        
        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID) HelpArray );
            return TRUE;
        
        }
    }
    return FALSE;
}






INT_PTR
DlgProc_TransportLayer(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
Routine Description:
    This routine contains the window procedure for transport dll selection
    dialog

    Editing of transport layers is not allowed when doing kernel debugging or
    the debugging of kernel and user mode dumps.

Arguments:
Return Value:
    See docs for "DialogBox"
--*/
{
    // fKernelDebugger cover kernel and kernel dumps
    BOOL bTL_EditingAllowed = !g_contWorkspace_WkSp.m_bKernelDebugger &&
                              !g_contWorkspace_WkSp.m_bUserCrashDump;

    static DWORD HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT,
       IDC_STEXT_SELECT, IDH_TLSELECT,
       IDC_BUT_SELECT, IDH_TLSELECT,
       IDC_BUT_ADD, IDH_TLADD,
       IDC_BUT_EDIT, IDH_TLEDIT,
       IDC_BUT_DELETE, IDH_TLDEL,
       0, 0
    };

    static DWORD NOEDIT_HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT_DISABLED,
       IDC_STEXT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_ADD, IDH_TRANSPORT_DISABLED,
       IDC_BUT_EDIT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_DELETE, IDH_TRANSPORT_DISABLED,
       0, 0
    };

    PDWORD pdwHelpArray = bTL_EditingAllowed ? HelpArray : NOEDIT_HelpArray;

    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_BUT_EDIT);
    HWND hwndDelete = GetDlgItem(hwndDlg, IDC_BUT_DELETE);
    HWND hwndSelect = GetDlgItem(hwndDlg, IDC_BUT_SELECT);

    switch (uMsg) {
        
    default:
        return FALSE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                "windbg.hlp",
                HELP_CONTEXTMENU,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;

    case WM_INITDIALOG:
        if (!bTL_EditingAllowed) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_LIST1), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_SELECT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_STEXT_SELECT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_ADD), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUT_DELETE), FALSE);
        }

        // This section sets up the dialog and initializes the fields.        
        
        // Make a copy of the registry, and screw with the copy.
        Assert(NULL == g_pCCopyOf_All_TLs_WrkSpc);
        g_pCCopyOf_All_TLs_WrkSpc = new CAll_TLs_WKSP();
        
        g_pCCopyOf_All_TLs_WrkSpc->m_bDynamicList = TRUE;
        g_pCCopyOf_All_TLs_WrkSpc->Duplicate(g_dynacontAll_TLs_WkSp);

        Assert(NULL == pszSelectedTL);
        Assert(g_contWorkspace_WkSp.m_pszSelectedTL);
        pszSelectedTL = _strdup(g_contWorkspace_WkSp.m_pszSelectedTL);
        
        //
        // Set the extended style
        //
        ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);       
        
        ///////////////////////////////////////////
        // Setup the column header for the list view
        ///////////////////////////////////////////
        
        //
        // Add Column headers
        //            
        {
            char            szColHdr[MAX_MSG_TXT];
            LV_COLUMN       lvc;
            
            // Initialize the LV_COLUMN structure. 
            lvc.mask = lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 100;
            lvc.pszText = szColHdr;  
            
            // Add the 1st column hdr
            Dbg(LoadString(g_hInst, SYS_TRANSPORT_LAYER_COL_HDR1, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 0;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 2nd col hdr
            Dbg(LoadString(g_hInst, SYS_TRANSPORT_LAYER_COL_HDR2, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 1;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 3rd col hdr
            Dbg(LoadString(g_hInst, SYS_TRANSPORT_LAYER_COL_HDR3, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 2;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 4th col hdr
            Dbg(LoadString(g_hInst, SYS_TRANSPORT_LAYER_COL_HDR4, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 3;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
        }            
        
        //
        // Add the actual data to the list
        //
        AddDataToList(hwndDlg, g_pCCopyOf_All_TLs_WrkSpc);
        return TRUE;

    case WM_COMMAND:
        if (bTL_EditingAllowed) {
            WORD wNotifyCode = HIWORD(wParam);  // notification code 
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;       // handle of control 
            
            switch (wID) {
                
            default:
                return FALSE;
                
            case IDC_BUT_RESET:
                {
                    // Reset it to the original contents
                    CAll_TLs_WKSP * pAll_TLs_Tmp = new CAll_TLs_WKSP();
                    
                    if (pAll_TLs_Tmp) {
                        g_pCCopyOf_All_TLs_WrkSpc->Duplicate(*pAll_TLs_Tmp);
                        
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOf_All_TLs_WrkSpc);
                        
                        BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                        EnableWindow(hwndEdit, !bEmpty);
                        EnableWindow(hwndDelete, !bEmpty);
                        EnableWindow(hwndSelect, !bEmpty);
                        
                        delete pAll_TLs_Tmp;
                    }
                }
                return FALSE;
                
            case IDC_BUT_SELECT:
                if (BN_CLICKED == wNotifyCode) {                   
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    Assert(-1 != nCurSel);
                    
                    if (nCurSel >= 0) {
                        char sz[MAX_PATH];
                        
                        ListView_GetItemText(hwndList, nCurSel, 0, sz, sizeof(sz));
                        
                        SetDlgItemText(hwndDlg, IDC_STEXT_SELECT, sz);
                        
                        FREE_STR(pszSelectedTL);
                        pszSelectedTL = _strdup(sz);
                    }
                }
                return FALSE;
                
            case IDC_BUT_DELETE:
                if (BN_CLICKED == wNotifyCode) {                   
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    Assert(-1 != nCurSel);
                    
                    char szTransName[MAX_MSG_TXT]; // Transport name
                    ListView_GetItemText(hwndList, nCurSel, 0, szTransName, sizeof(szTransName));
                    
                    char szMsg[MAX_VAR_MSG_TXT];
                    {
                        char szQuestion[MAX_MSG_TXT]; // Question confirming deletion
                        
                        Dbg(LoadString(g_hInst, DBG_Deleting_DLL, szQuestion, sizeof(szQuestion)));
                        
                        wsprintf(szMsg, szQuestion, szTransName);
                    }
                    
                    char szTitle[MAX_MSG_TXT];
                    Dbg(LoadString(g_hInst, DLG_Deleting_DLL_Title, szTitle, sizeof(szTitle)));
                    strcat(szTitle, "'");
                    strcat(szTitle, szTransName);
                    strcat(szTitle, "'");
                    
                    if (MessageBox(hwndDlg, szMsg, szTitle, MB_OKCANCEL | MB_TASKMODAL) == IDOK) {
                        // Did we erase the selected transport layer
                        if (pszSelectedTL && !strcmp(szTransName, pszSelectedTL)) {
                            
                            // Erased
                            FREE_STR(pszSelectedTL);
                        }
                        
                        // Remove the deleted TL from the reg list.
                        {
                            TListEntry<CIndiv_TL_WKSP *> * pContEntry =
                                g_pCCopyOf_All_TLs_WrkSpc->
                                m_listConts.Find(szTransName, WKSP_Generic_CmpRegName);
                            
                            Assert(pContEntry);
                            delete pContEntry->m_tData;
                            delete pContEntry;
                        }
                        
                        Dbg(ListView_DeleteItem(hwndList, nCurSel));
                        
                        // Select the first item, make sure something is always selected.
                        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000F);
                        
                        BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                        EnableWindow(hwndEdit, !bEmpty);
                        EnableWindow(hwndSelect, !bEmpty);
                        EnableWindow(hwndDelete, !bEmpty);
                        
                        SetDlgItemText(hwndDlg, IDC_STEXT_SELECT, pszSelectedTL);
                    }
                }
                return FALSE;
                
            case IDC_BUT_EDIT:
                if (BN_CLICKED == wNotifyCode) {
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    Assert(-1 != nCurSel);
                    
                    char szTransName[MAX_MSG_TXT]; // Transport name
                    ListView_GetItemText(hwndList, nCurSel, 0, szTransName, sizeof(szTransName));
                    
                    TListEntry<CIndiv_TL_WKSP *> * pContEntry =
                        g_pCCopyOf_All_TLs_WrkSpc->
                        m_listConts.Find(szTransName, WKSP_Generic_CmpRegName);
                    
                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DLG_TRANSPORTLAYER), hwndDlg,
                        EditTransportLayersDlgProc, (LPARAM)pContEntry->m_tData)) {
                        
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOf_All_TLs_WrkSpc);
                    }
                    
                    BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                    EnableWindow(hwndEdit, !bEmpty);
                    EnableWindow(hwndDelete, !bEmpty);
                    EnableWindow(hwndSelect, !bEmpty);
                }                
                return FALSE;
                
            case IDC_BUT_ADD:
                if (BN_CLICKED == wNotifyCode) {
                    CIndiv_TL_WKSP * pTL = new CIndiv_TL_WKSP;
                    
                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DLG_TRANSPORTLAYER), hwndDlg,
                        EditTransportLayersDlgProc, (LPARAM)pTL)) {
                        
                        pTL->SetParent(g_pCCopyOf_All_TLs_WrkSpc);
                        g_pCCopyOf_All_TLs_WrkSpc->AddToContainerList(pTL);
                        
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOf_All_TLs_WrkSpc);
                        
                        BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                        EnableWindow(hwndEdit, !bEmpty);
                        EnableWindow(hwndDelete, !bEmpty);
                        EnableWindow(hwndSelect, !bEmpty);
                    } else {
                        delete pTL;
                    }
                }                
                return FALSE;
            }
        }
        return FALSE;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code) {
        default:
            return FALSE;

        case NM_DBLCLK:
           if (bTL_EditingAllowed) {
                LPNMHDR lpnmh = (LPNMHDR) lParam;
                
                switch (lpnmh->idFrom) {
                case IDC_LIST1:
                    // Dbl clicking will work the same as a pressing the "select" button.
                    // Let the Select button process the dbl click
                    PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_BUT_SELECT, BN_CLICKED),
                        (LPARAM) lpnmh->hwndFrom);
                    break;
                }
            }
            return FALSE;

        case PSN_KILLACTIVE:
            // Error checking: must select a TL, and that TL must exist
            if (!ValidateAllTransportLayers(hwndDlg, g_pCCopyOf_All_TLs_WrkSpc)) {
                MessageBeep(MB_ICONEXCLAMATION);
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
            }
            
            if (NULL == pszSelectedTL) {
                
                PSTR pszTitle = WKSP_DynaLoadString(g_hInst, SYS_Warning);
                WKSP_MsgBox(pszTitle, ERR_Please_Select_A_TL);
                free(pszTitle);

                MessageBeep(MB_ICONEXCLAMATION);
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
                
            } else if (NULL == g_pCCopyOf_All_TLs_WrkSpc->
                m_listConts.Find(pszSelectedTL, WKSP_Generic_CmpRegName)) {
                
                PSTR pszTitle = WKSP_DynaLoadString(g_hInst, SYS_Warning);
                WKSP_MsgBox(pszTitle, ERR_Transport_Doesnt_Exist, pszSelectedTL);
                free(pszTitle);

                MessageBeep(MB_ICONEXCLAMATION);
                SetWindowLong(hwndDlg, DWLP_MSGRESULT, TRUE);
            }
            
            SetWindowLong(hwndDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;

        case PSN_APPLY:
            g_dynacontAll_TLs_WkSp.Duplicate(*g_pCCopyOf_All_TLs_WrkSpc);
            g_dynacontAll_TLs_WkSp.Save(FALSE, FALSE);
            
            Assert(SetTransportLayer(pszSelectedTL, g_pCCopyOf_All_TLs_WrkSpc));
            return TRUE;
        }
        break;

    case WM_DESTROY:
        delete g_pCCopyOf_All_TLs_WrkSpc;
        g_pCCopyOf_All_TLs_WrkSpc = NULL;
                
        if (pszSelectedTL) {
            free(pszSelectedTL);
            pszSelectedTL = NULL;
        }
        return FALSE;
    }
}
