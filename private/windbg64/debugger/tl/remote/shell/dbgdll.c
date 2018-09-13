/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbgdll.c

Abstract:

    This file contains the source code for the dialog to select the
    various different transport layers

Author:

    Wesley Witt (wesw) 1-Nov-1993

Environment:

    User mode WIN32


--*/

#include "precomp.h"
#pragma hdrstop


extern CHAR       szHelpFileName[];


void
AddDataToList(
    HWND hwndDlg, 
    CWindbgrm_RM_WKSP * pCCopyOfRM_Windg_WrkSpc
    )
{
    Assert(pCCopyOfRM_Windg_WrkSpc);
    
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
    
    TListEntry<CIndiv_TL_RM_WKSP *> * pContEntry = pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.m_listConts.FirstEntry();
    for (; pContEntry != pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.m_listConts.Stop(); 
        pContEntry = pContEntry->Flink, nItems++) {
        
        CIndiv_TL_RM_WKSP * pIndTl = pContEntry->m_tData;
        AssertType(*pIndTl, CIndiv_TL_RM_WKSP);
        
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
            if (pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL) {
                SetDlgItemText(hwndDlg, IDC_STEXT_SELECT, 
                    pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL);
            }
        }
    }
 
    // Select the first item
    ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000F);
}

//
// Used primarily by TransportLayersDlgProc, however, EditTransportLayersDlgProc
//  also uses it to check for duplicates.
//
static CWindbgrm_RM_WKSP * g_pCCopyOfRM_Windg_WrkSpc = NULL;

INT_PTR
CALLBACK
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
    static CIndiv_TL_RM_WKSP * pTL = NULL;
    
    switch (uMsg) {        
    case WM_INITDIALOG:
        //
        // lParam point to a transport layer.
        
        Assert(lParam);
        pTL = (CIndiv_TL_RM_WKSP *) lParam;
        AssertType(*pTL, CIndiv_TL_RM_WKSP);
        
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
                char szName[MAX_MSG_TXT], szDesc[MAX_MSG_TXT], szDll[MAX_MSG_TXT], szParams[MAX_MSG_TXT];
                
                GetDlgItemText(hwndDlg, ID_CHDBDLL_NAME, szName, sizeof(szName));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_DESC, szDesc, sizeof(szDesc));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_PATH, szDll, sizeof(szDll));
                GetDlgItemText(hwndDlg, ID_CHDBDLL_PARAM, szParams, sizeof(szParams));

                //
                // Verify that key is not empty, and unique
                //
                
                if (0 == strlen(szName)) {
                    WKSP_MsgBox(NULL, IDS_ERR_Empty_Shortname);
                    return TRUE;
                }

                // Are we adding or editing
                if (NULL == pTL->m_pszRegistryName) {
                    // If it doesn't have a registry name, then we are adding
                    // Make sure the name isn't duplicated.
                    if (g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.
                            m_listConts.Find(szName, WKSP_Generic_CmpRegName) ) {

                        WKSP_MsgBox(NULL, IDS_ERR_Not_Unique_Shortname, szName);
                        return TRUE;
                    }
                } else {
                    // If it does have a registry name, then we are editing
                    if (strcmp(szName, pTL->m_pszRegistryName)) {
                        // The name has changed, then make sure it isn't duplicated.
                        if (g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.
                                m_listConts.Find(szName, WKSP_Generic_CmpRegName) ) {

                            WKSP_MsgBox(NULL, IDS_ERR_Not_Unique_Shortname, szName);
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
            
        case ID_HELP:            
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR
CALLBACK
TransportLayersDlgProc(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++
Routine Description:
    This routine contains the window procedure for transport dll selection
    dialog

Arguments:
Return Value:
    See docs for "DialogBox"
--*/
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_BUT_EDIT);
    HWND hwndDelete = GetDlgItem(hwndDlg, IDC_BUT_DELETE);
    HWND hwndSelect = GetDlgItem(hwndDlg, IDC_BUT_SELECT);
    
    switch (uMsg) {
    case WM_INITDIALOG:
        // This section sets up the dialog and initializes the fields.        
        
        // Make a copy of the registry, and screw with the registry.
        Assert(NULL == g_pCCopyOfRM_Windg_WrkSpc);
        g_pCCopyOfRM_Windg_WrkSpc = new CWindbgrm_RM_WKSP();
        Assert(g_pCCopyOfRM_Windg_WrkSpc);
        g_pCCopyOfRM_Windg_WrkSpc->Duplicate(g_RM_Windbgrm_WkSp);
        
        
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
            Assert(LoadString(g_hInst, IDS_SYS_TRANSPORT_LAYER_COL_HDR1, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 0;
            Assert(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 2nd col hdr
            Assert(LoadString(g_hInst, IDS_SYS_TRANSPORT_LAYER_COL_HDR2, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 1;
            Assert(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 3rd col hdr
            Assert(LoadString(g_hInst, IDS_SYS_TRANSPORT_LAYER_COL_HDR3, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 2;
            Assert(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
            
            // Add the 4th col hdr
            Assert(LoadString(g_hInst, IDS_SYS_TRANSPORT_LAYER_COL_HDR4, 
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 3;
            Assert(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);
        }            
        
        //
        // Add the actual data to the list
        //
        AddDataToList(hwndDlg, g_pCCopyOfRM_Windg_WrkSpc);
        return TRUE;
        
    case WM_COMMAND:        
        {
            WORD wNotifyCode = HIWORD(wParam);  // notification code 
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;       // handle of control 
            
            switch (wID) {

            case IDC_BUT_RESET:
                {
                    // Reset it to the original contents
                    CWindbgrm_RM_WKSP * pWkSpTmp = new CWindbgrm_RM_WKSP();
                
                    if (pWkSpTmp) {
                        g_pCCopyOfRM_Windg_WrkSpc->Duplicate(*pWkSpTmp);
    
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOfRM_Windg_WrkSpc);
                    
                        BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                        EnableWindow(hwndEdit, !bEmpty);
                        EnableWindow(hwndDelete, !bEmpty);
                        EnableWindow(hwndSelect, !bEmpty);

                        delete pWkSpTmp;
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
                        
                        FREE_STR(g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL);
                        g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL = _strdup(sz);
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
                        
                        Dbg(LoadString(g_hInst, IDS_DBG_Deleting_DLL, szQuestion, sizeof(szQuestion)));
                        
                        wsprintf(szMsg, szQuestion, szTransName);
                    }
                    
                    char szTitle[MAX_MSG_TXT];
                    Dbg(LoadString(g_hInst, IDS_DBG_Deleting_DLL_Title, szTitle, sizeof(szTitle)));
                    strcat(szTitle, "'");
                    strcat(szTitle, szTransName);
                    strcat(szTitle, "'");
                    
                    if (MessageBox(hwndDlg, szMsg, szTitle, MB_OKCANCEL | MB_TASKMODAL) == IDOK) {
                        // Did we erase the selected transport layer
                        if (g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL &&
                            !strcmp(szTransName, g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL)) {
                            
                            // Erased
                            FREE_STR(g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL);
                        }
                        
                        // Remove the deleted TL from the reg list.
                        {
                            TListEntry<CIndiv_TL_RM_WKSP *> * pContEntry =
                                g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.
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
                        
                        SetDlgItemText(hwndDlg, IDC_STEXT_SELECT, 
                            g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL);
                    }
                }
                return FALSE;
                
            case IDC_BUT_EDIT:
                if (BN_CLICKED == wNotifyCode) {
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    Assert(-1 != nCurSel);
                    
                    char szTransName[MAX_MSG_TXT]; // Transport name
                    ListView_GetItemText(hwndList, nCurSel, 0, szTransName, sizeof(szTransName));
                    
                    TListEntry<CIndiv_TL_RM_WKSP *> * pContEntry =
                        g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.
                        m_listConts.Find(szTransName, WKSP_Generic_CmpRegName);
                    
                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(DLG_EDIT_TL), hwndDlg,
                        EditTransportLayersDlgProc, (LPARAM)pContEntry->m_tData)) {
                        
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOfRM_Windg_WrkSpc);
                    }
                    
                    BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                    EnableWindow(hwndEdit, !bEmpty);
                    EnableWindow(hwndDelete, !bEmpty);
                    EnableWindow(hwndSelect, !bEmpty);
                }                
                return FALSE;
                
            case IDC_BUT_ADD:
                if (BN_CLICKED == wNotifyCode) {
                    CIndiv_TL_RM_WKSP * pTL = new CIndiv_TL_RM_WKSP;
                    
                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(DLG_EDIT_TL), hwndDlg,
                        EditTransportLayersDlgProc, (LPARAM)pTL)) {
                        
                        pTL->SetParent(&(g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs));
                        g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.AddToContainerList(pTL);
                        
                        // Data has been changed, repopulate the list.
                        AddDataToList(hwndDlg, g_pCCopyOfRM_Windg_WrkSpc);
                        
                        BOOL bEmpty = ListView_GetItemCount(hwndList) <= 0;
                        EnableWindow(hwndEdit, !bEmpty);
                        EnableWindow(hwndDelete, !bEmpty);
                        EnableWindow(hwndSelect, !bEmpty);
                    } else {
                        delete pTL;
                    }
                }                
                return FALSE;
                
#if 0                
            case ID_ADVANCED:
                DialogBoxParam( GetModuleHandle( NULL ),
                    MAKEINTRESOURCE(DLG_KERNELDBG),
                    hwndDlg,
                    DlgKernelDbg,
                    (LPARAM) &rgDbt[idx].KdParams
                    );
                SetDefaultButton(hwndDlg, IDOK);
                return(TRUE);
#endif
                
                
            case IDOK:
                // Error checking: must select a TL, and that TL must exist
                if (NULL == g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL) {

                    char szTitle[MAX_MSG_TXT];

                    Dbg(LoadString(g_hInst, IDS_Sys_Warning, szTitle, sizeof(szTitle)));
                    WKSP_MsgBox(szTitle, IDS_ERR_No_TL_Selected);
                    return FALSE;

                } else if (NULL == g_pCCopyOfRM_Windg_WrkSpc->m_dynacont_All_TLs.
                    m_listConts.Find(g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL, WKSP_Generic_CmpRegName)) {

                    char szTitle[MAX_MSG_TXT];

                    Dbg(LoadString(g_hInst, IDS_Sys_Warning, szTitle, sizeof(szTitle)));
                    WKSP_MsgBox(szTitle, IDS_ERR_Transport_Doesnt_Exist, g_pCCopyOfRM_Windg_WrkSpc->m_pszSelectedTL);
                    return FALSE;
                }


                g_RM_Windbgrm_WkSp.Duplicate(*g_pCCopyOfRM_Windg_WrkSpc);
                g_RM_Windbgrm_WkSp.Save(FALSE, FALSE);
                if (pszTlName) {
                    free(pszTlName);
                    pszTlName = NULL;
                }
                pszTlName = _strdup(g_RM_Windbgrm_WkSp.m_pszSelectedTL);
                EndDialog(hwndDlg, TRUE); // TRUE says "Accept the Changes"
                return FALSE;
                
            case IDCANCEL:
                EndDialog(hwndDlg, FALSE); // FALSE says "Discard the Changes"
                return FALSE;
                
            case ID_HELP:
                WinHelp(hwndDlg, szHelpFileName, HELP_CONTEXT, ID_TRANSPORT_HELP);
                return FALSE;
                
            default:
                return FALSE;
            }
        } // WM_COMMAND
        
    case WM_DESTROY:
        delete g_pCCopyOfRM_Windg_WrkSpc;
        g_pCCopyOfRM_Windg_WrkSpc = NULL;
        return FALSE;
        
    default:
        return FALSE;
    }
}


