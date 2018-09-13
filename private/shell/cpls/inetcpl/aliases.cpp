/****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1994
 *  All rights reserved
 *
 ***************************************************************************/

#ifdef UNIX_FEATURE_ALIAS

#undef UNICODE

#include "inetcplp.h"
#include "shalias.h"

#include "mluisupp.h"

STDAPI RefreshGlobalAliasList();

#define  GETALIASLIST(hDlg)     ((LPALIASINFO )GetWindowLong(hDlg, DWL_USER))->aliasList
#define  GETALIASDELLIST(hDlg)  ((LPALIASINFO )GetWindowLong(hDlg, DWL_USER))->aliasDelList

BOOL CALLBACK AlEditDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID WINAPI InitAliasListStyle(HWND hwndLV, DWORD dwView);

static TCHAR g_szAliasKey[]     = TEXT("Software\\Microsoft\\Internet Explorer\\Unix\\Alias");

// InitListViewImageLists - creates image lists for a list view. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndLV - handle to the list view control. 
BOOL WINAPI InitAliasListImageLists(HWND hwndLV)     
{ 
    HICON hiconItem;        // icon for list view items 
    HIMAGELIST himlLarge;   // image list for icon view 
    HIMAGELIST himlSmall;   // image list for other views  

    // Create the full-sized and small icon image lists. 
    himlLarge = ImageList_Create(GetSystemMetrics(SM_CXICON), 
        GetSystemMetrics(SM_CYICON), TRUE, 1, 1); 
    himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), TRUE, 1, 1);  
    
    // Add an icon to each image list. 
    // note that IDI_WALLET has to live in inetcplc.rc because
    // it's used by a localizable dialog, hence the MLGetHinst()
    hiconItem = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_WALLET));
    ImageList_AddIcon(himlLarge, hiconItem); 
    ImageList_AddIcon(himlSmall, hiconItem);     
    DeleteObject(hiconItem);  
    
    // Assign the image lists to the list view control. 
    ListView_SetImageList(hwndLV, himlLarge, LVSIL_NORMAL); 
    ListView_SetImageList(hwndLV, himlSmall, LVSIL_SMALL);     

    return TRUE;     
} 

    
// InitListViewItems - adds items and subitems to a list view. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndLV - handle to the list view control. 
// pfData - text file containing list view items with columns 
//          separated by semicolons. 
BOOL WINAPI InitAliasListItems(HWND hwndLV, HDPA aliasList)     
{ 
    PSTR pszEnd;
    int iItem;
    int iSubItem;
    LVITEM lvi;  
    
    // Initialize LVITEM members that are common to all items. 
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE; 
    lvi.state = 0;     lvi.stateMask = 0; 
    lvi.pszText = LPSTR_TEXTCALLBACK;   // app. maintains text 
    lvi.iImage = 0;                     // image list index  
    
    int aliasCount = DPA_GetPtrCount( aliasList );

    for (int i = 0; i< aliasCount; i++)
    { 
        CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasList, i );

        // Initialize item-specific LVITEM members.         
        lvi.iItem = i; 
        lvi.iSubItem = 0;
        lvi.lParam = (LPARAM) NULL;    // item data  
        // Add the item.       
        ListView_InsertItem(hwndLV, &lvi);  

        // Initialize item-specific LVITEM members.         
        ListView_SetItemText(hwndLV, i, 0, (TCHAR*)GetAliasName(ptr));  
        ListView_SetItemText(hwndLV, i, 1, (TCHAR*)GetAliasUrl(ptr));  
    }      
    
    return TRUE;
}  


// InitListViewColumns - adds columns to a list view control. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndLV - handle to the list view control. 
BOOL WINAPI InitAliasListColumns(HWND hwndLV)     
{ 
    TCHAR g_achTemp[256];         // temporary buffer     
    LVCOLUMN lvc; 
    int iCol;      
    
    // Initialize the LVCOLUMN structure. 
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    lvc.fmt = LVCFMT_LEFT;     
    lvc.pszText = g_achTemp;  
    
    // Add the columns.     
    for (iCol = 0; iCol < ALIASLIST_COLUMNS; iCol++) 
    { 
        lvc.iSubItem = iCol; 
        lvc.cx = 100 + (iCol*150);     
        MLLoadString(IDS_FIRSTCOLUMN + iCol, 
                g_achTemp, sizeof(g_achTemp)); 
        if (ListView_InsertColumn(hwndLV, iCol, &lvc) == -1) 
            return FALSE;     
    } 

    return TRUE;
}

// SetView - sets a list view's window style to change the view. 
// hwndLV - handle to the list view control. 
// dwView - value specifying a view style.      
VOID WINAPI InitAliasListStyle(HWND hwndLV, DWORD dwView)     
{ 
    // Get the current window style. 
    DWORD dwStyle = ListView_GetExtendedListViewStyle(hwndLV);  

    ListView_SetExtendedListViewStyle( hwndLV, (dwStyle|dwView) );
    // SetWindowLong(hwndLV, GWL_EXSTYLE, (dwStyle | dwView)); 
} 

// AliasDel - deletes alias from active list and moves it to the
//            del list to be delete later. 
// hDlg - handle of the propertysheet dialog.
BOOL WINAPI AliasDel( HWND hDlg )
{
    int index = 0, iItem = 0;
    HWND lb           = GetDlgItem( hDlg, IDC_ALIAS_LIST );
    HDPA aliasList    = GETALIASLIST(hDlg);
    HDPA aliasDelList = GETALIASDELLIST(hDlg);
    BOOL fAsked       = FALSE;
    BOOL fChanged     = FALSE;
    int  count        = ListView_GetItemCount(lb);

    // Get the selection from the Listview and remove it from the
    // active alias list, add it to the aliaslist to be deleted.
    while( (iItem = ListView_GetNextItem( lb, -1, LVNI_SELECTED ) ) != -1 )
    {
        TCHAR str[MAX_URL_STRING]; *str = TEXT('\0');

        if( !fAsked )
        {
            TCHAR question[MAX_PATH];

            wsprintf( question, "Are you Sure you want to delete the selected items?");
            if( MessageBox( GetParent(hDlg), question, TEXT("Delete Alias"), MB_YESNO ) != IDYES )
                return FALSE;

            fAsked = TRUE;
        }

        // if( !ListView_GetCheckState(lb, iItem) ) continue;

        ListView_GetItemText(lb, iItem, 0, str, MAX_URL_STRING );
        if(*str)
        {
            if( (index = FindAliasIndex(aliasList, str) ) != -1 )
            {
                 CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasList, index );
                 if( ptr )
                 {
                    CAlias *pAlias = (CAlias *)DPA_DeletePtr( aliasList, index );

                    // Add to List of deleted entries
                    DPA_InsertPtr( aliasDelList, 0x7FFF, pAlias );
                    ListView_DeleteItem(lb, iItem);
                    fChanged = TRUE;
                    LocalFree( str );
                } 
            }
        }
    }

    if( fChanged )
    {
        InitAliasDialog( hDlg, NULL, FALSE ); 
        PropSheet_Changed(GetParent(hDlg),hDlg);
    }

    return TRUE;
}


// AliasEdit - Called in response to the Edit button pressed.
// hDlg - Handle to the property sheet 
BOOL WINAPI AliasEdit( HWND hDlg )
{
    CAlias * ptr = GetCurrentAlias( hDlg );
    HDPA aliasDelList = GETALIASDELLIST(hDlg);

    if( ptr )
    {
        CAlias *ptrOld = (CAlias *)CreateAlias( (LPTSTR)GetAliasName(ptr) );
        ALIASEDITINFO aliasEditInfo = { GETALIASLIST(hDlg), ptr, hDlg, EDIT_ALIAS };
        if(MLDialogBoxParamWrap( MLGetHinst(), MAKEINTRESOURCE(IDD_ALIAS_EDIT), hDlg, AlEditDlgProc, (LPARAM)&aliasEditInfo ) == 2 )
        {
            // Add old alias to del list if alias name changes.
            LPCTSTR aliasNew = GetAliasName(ptr);
            LPCTSTR aliasOld = GetAliasName(ptrOld);

            if( StrCmp( aliasNew, aliasOld) )
                DPA_InsertPtr( aliasDelList, 0x7FFF, ptrOld );
            else
                DestroyAlias( ptrOld );
            
            InitAliasDialog( hDlg, ptr, FALSE ); 
            PropSheet_Changed(GetParent(hDlg),hDlg);
        }
    }

    return TRUE;
}


// AliasEdit - Called in response to the Add button pressed.
// hDlg - Handle to the property sheet 
BOOL WINAPI AliasAdd( HWND hDlg)
{
    CAlias * ptr = (CAlias *)CreateAlias( TEXT("") );

    if ( ptr )
    {
        ALIASEDITINFO aliasEditInfo = { GETALIASLIST(hDlg), ptr, hDlg, ADD_ALIAS };
        if(MLDialogBoxParamWrap( MLGetHinst(), MAKEINTRESOURCE(IDD_ALIAS_EDIT), hDlg, AlEditDlgProc, (LPARAM)&aliasEditInfo ) == 2)
        {
            InitAliasDialog( hDlg, ptr, FALSE ); 
            PropSheet_Changed(GetParent(hDlg),hDlg);
        }
        DestroyAlias(ptr);
    }

    return TRUE;
}

// GetCurrentAlias - returns currently selected alis from the listview
// Returns - Selected alias
// hDlg - handle to the property sheet.
CAlias * GetCurrentAlias( HWND hDlg )
{
    int index = 0, iItem = 0;
    HDPA aliasList = GETALIASLIST( hDlg );
    HWND lb   = GetDlgItem( hDlg, IDC_ALIAS_LIST );

    if( ListView_GetSelectedCount(lb) == 1  && 
      ( (iItem = ListView_GetNextItem( lb, -1, LVNI_SELECTED ) ) != -1 ) )
    {
        TCHAR str[MAX_URL_STRING]; *str = TEXT('\0');
        ListView_GetItemText(lb, iItem, 0, str, MAX_URL_STRING );
        if(*str)
        {
            if( (index = FindAliasIndex(aliasList, str) ) != -1 )
            {
                CAlias * ptr = (CAlias *)DPA_FastGetPtr( aliasList, index );
                return ptr;
            }
        }
    }
    return NULL;
}

// InitAliasDialog - Initalizes the aliases dialog 
// Returns - TRUE if succeeded/FALSE if failed.
// hDlg - handle to the property sheet.
// fFullInit - Init listview columns/styles/etc
BOOL FAR PASCAL InitAliasDialog(HWND hDlg, CAlias * current, BOOL fFullInit)
{
    HRESULT  hr = E_FAIL;
    HKEY     hKey;
    HWND     listBox = GetDlgItem( hDlg, IDC_ALIAS_LIST );
    TCHAR *  displayString;

    // Allocate memory for a structure which will hold all the info
    // gathered from this page
    //
    LPALIASINFO pgti = (LPALIASINFO)GetWindowLong(hDlg, DWL_USER);
    pgti->fInternalChange = FALSE;

    SendMessage( listBox, LVM_DELETEALLITEMS, 0, 0L );

    // Initailize ListView
    if( fFullInit )
    {
        SendDlgItemMessage( hDlg, IDC_ALIAS_EDIT, EM_LIMITTEXT, 255, 0 );
        SendDlgItemMessage( hDlg, IDC_URL_EDIT, EM_LIMITTEXT, MAX_URL_STRING-1, 0 );
        // InitAliasListStyle(listBox, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT );
        InitAliasListStyle(listBox, LVS_EX_FULLROWSELECT );
        InitAliasListImageLists(listBox);     
        InitAliasListColumns(listBox);     
    }

    InitAliasListItems(listBox, GETALIASLIST(hDlg));     
        
    return TRUE;
}

// AliasApply - This function is called in response to  pressing the apply/ok
//              button on the property sheet dialog.
void AliasApply(HWND hDlg)
{
    HDPA aliasDelList = GETALIASDELLIST(hDlg);
    HDPA aliasList    = GETALIASLIST(hDlg);

    ASSERT(aliasList);

    if( aliasDelList )
    {
        int count = DPA_GetPtrCount( aliasDelList );
        
        for(int i=count-1; i>=0; i--)
        {
            CAlias * pAlias = (CAlias *)DPA_DeletePtr( aliasDelList, i );
            if(pAlias) 
            {
                pAlias->Delete();
                DestroyAlias(pAlias);
            }
        }
    }

    // Save the currently changed aliases
    SaveAliases( aliasList );

    // Refresh Global Alias List.
    RefreshGlobalAliasList();
}

// AliasDlgProc - Alias PropertySheet dialog Proc
// Returns BOOL
// hDlg - Handle to the property sheet window
// wParam, lParam - Word/Long param
BOOL CALLBACK AliasDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // get our tab info structure
    LPALIASINFO pgti;

    if (uMsg == WM_INITDIALOG)
    {
        // Allocate memory for a structure which will hold all the info
        // gathered from this page
        //
        LPALIASINFO pgti = (LPALIASINFO)LocalAlloc(LPTR, sizeof(tagALIASINFO));
        if (!pgti)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }

        pgti->hDlg = hDlg;

        pgti->fInternalChange = FALSE;
        SetWindowLong(hDlg, DWL_USER, (LPARAM)pgti);
        
        if((pgti->aliasList = DPA_Create(4)) != (HDPA)NULL ) 
        {
            pgti->aliasDelList = DPA_Create(4);
            LoadAliases( pgti->aliasList );

            // Initailize dialog 
            if( InitAliasDialog(hDlg, NULL, TRUE) ) 
            {
                return TRUE;
            }
            else
            {
                TCHAR szTitle[MAX_PATH];
                MLLoadString(IDS_ERROR_REGISTRY_TITLE, szTitle, sizeof(szTitle));
                MessageBox( GetParent(hDlg), TEXT("Cannot read aliases from registry."), szTitle, MB_OK ); 
                return FALSE;
            }
        }
        else
            return FALSE;
    }
    else
        pgti = (LPALIASINFO)GetWindowLong(hDlg, DWL_USER);

    if (!pgti)
        return FALSE;

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            switch (lpnm->code)
            {
                case NM_DBLCLK:
                    if(lpnm->idFrom == IDC_ALIAS_LIST)
                          AliasEdit( pgti->hDlg ); 
                    break;
                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    return TRUE;

                case PSN_APPLY:
                    AliasApply(hDlg);
                    break;
            }
            break;
        }

        case WM_COMMAND:
            { 
                if(HIWORD(wParam) == BN_CLICKED)
                {
                     switch (LOWORD(wParam))
                     { 
                         case IDC_ALIAS_ADD:
                            AliasAdd( pgti->hDlg ); break;
                         case IDC_ALIAS_EDIT:
                             AliasEdit( pgti->hDlg ); break;
                         case IDC_ALIAS_DEL:
                            AliasDel( pgti->hDlg ); break;
                     }
                }
            }
            break;

        case WM_DESTROY:
            // Delete registry information
            if( pgti->aliasList )
            {
                FreeAliases(pgti->aliasList);
                DPA_Destroy(pgti->aliasList);
            }

            if( pgti->aliasDelList )
            {
                FreeAliases(pgti->aliasDelList);
                DPA_Destroy(pgti->aliasDelList);
            }

            if (pgti)
                LocalFree(pgti);

            SetWindowLong(hDlg, DWL_USER, (LONG)NULL);  // make sure we don't re-enter
            break;

    }
    return FALSE;
}


BOOL CALLBACK AlEditDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAlias * pAlias;
    LPALIASEDITINFO pAliasInfo;

    if (uMsg == WM_INITDIALOG)
    {
        TCHAR achTemp[256];
        pAliasInfo = (LPALIASEDITINFO)lParam;
        pAlias = pAliasInfo->alias;
        
        if( !lParam ) 
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }

        SendDlgItemMessage( hDlg, IDC_ALIAS_EDIT, WM_SETTEXT, 0,  (LPARAM)GetAliasName(pAlias));
        SendDlgItemMessage( hDlg, IDC_URL_EDIT, WM_SETTEXT, 0,  (LPARAM)GetAliasUrl(pAlias));

        if( pAliasInfo->dwFlags & EDIT_ALIAS )
        {
            // EnableWindow( GetDlgItem(hDlg, IDC_ALIAS_EDIT ), FALSE );
            MLLoadString(IDS_TITLE_ALIASEDIT, 
                achTemp, sizeof(achTemp)); 
            SendMessage( hDlg, WM_SETTEXT, 0, (LPARAM)achTemp); 
        }
        else
        {
            MLLoadString(IDS_TITLE_ALIASADD, 
                achTemp, sizeof(achTemp)); 
            SendMessage( hDlg, WM_SETTEXT, 0, (LPARAM)achTemp); 
        }

        SetWindowLong(hDlg, DWL_USER, (LPARAM)pAliasInfo);
        EnableWindow( GetDlgItem(hDlg, IDOK), FALSE );
    }
    else
        pAliasInfo = (LPALIASEDITINFO)GetWindowLong(hDlg, DWL_USER);

    if (!pAlias)
        return FALSE;

    switch (uMsg)
    {
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_ALIAS_EDIT:
                case IDC_URL_EDIT:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) 
                    {
                        EnableWindow( GetDlgItem(hDlg, IDOK), TRUE );
                    }
                    break;
                case IDOK:
                {
                    if( pAliasInfo )
                    {
                        TCHAR alias[MAX_URL_STRING];
                        TCHAR szurl[MAX_URL_STRING];
                        SendDlgItemMessage( hDlg, IDC_ALIAS_EDIT, WM_GETTEXT, MAX_URL_STRING, (LPARAM)alias );
                        SendDlgItemMessage( hDlg, IDC_URL_EDIT, WM_GETTEXT, MAX_URL_STRING, (LPARAM)szurl );

                        EatSpaces( alias );
                        
                        if( !*alias ) 
                        {
                            EndDialog( hDlg, 1 );
                            break;
                        }

                        if( pAliasInfo->dwFlags & ADD_ALIAS  && *alias)
                        {
                            if(AddAliasToList( pAliasInfo->aliasList, alias, szurl, hDlg ))
                                EndDialog( hDlg, 2);
                        }
                        else if( pAliasInfo->dwFlags & EDIT_ALIAS )
                        {
                            CAlias * ptr = pAliasInfo->alias;
                            if( StrCmp(GetAliasName(ptr), alias) )
                                if(FindAliasIndex( pAliasInfo->aliasList, alias ) != -1)
                                {
                                    MessageBox( hDlg, 
                                        TEXT("Alias with same name already exists"), 
                                        TEXT("Edit Alias"), 
                                        MB_OK|MB_ICONSTOP );
                                    break;
                                }
                            SetAliasInfo(ptr, alias, szurl);
                            EndDialog( hDlg, 2);
                        }
                        break;
                    }        
                    else
                        EndDialog( hDlg, 1 );
                    break;
                }
                case IDCANCEL:
                {
                    EndDialog( hDlg, 1 );
                }
            }
            break;

        case WM_DESTROY:
            SetWindowLong(hDlg, DWL_USER, (LONG)NULL);  
            break;

    }
    return FALSE;
}

#endif /* UNIX_FEATURE_ALIAS */
