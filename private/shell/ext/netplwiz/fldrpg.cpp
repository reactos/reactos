#include "stdafx.h"

#include <wininet.h>

#include "fldrpg.h"


enum {
    COL_FOLDER,
    COL_COMMENT,
    NUM_COLUMNS
};

INT_PTR CNetPlacesWizardPage2::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    default:
        break;
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage2::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_SELECTFOLDER_LIST);

    //
    // Limit the size of the edit controls
    //
    Edit_LimitText(GetDlgItem(hwnd, IDC_FOLDER_EDIT),
                   MAX_PATH - 1);
    // Trace(TRACE_LEVEL_FLOW, TEXT("Entering InitListView\n"));

    //
    // Use a do-while loop so we can break out of it
    // on errors (note:  it's while(0))
    //
    do
    {
        ListView_SetImageList(hwndList, 
                              m_pdata->himlSmall, 
                              LVSIL_SMALL);
            
        //
        // Create 2 listview columns.  If more are added, the column
        // width calculation needs to change.
        //
        _ASSERT(NUM_COLUMNS == 2);

        LV_COLUMN   lvc;
        RECT        rcLV;
        TCHAR       tszColumnLabel[MAX_CAPTION + 1];
        DWORD       dwCol;
 
        GetClientRect(hwndList, &rcLV);
        rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

        ZeroMemory(&lvc, sizeof(lvc));
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = rcLV.right / 3;
        lvc.pszText = tszColumnLabel;
 
        for (dwCol = 0; dwCol < NUM_COLUMNS; dwCol++) 
        { 
            lvc.iSubItem = dwCol; 

            LoadString(g_hInstance, IDS_FIRSTCOLUMN + dwCol, tszColumnLabel, 
                ARRAYSIZE(tszColumnLabel));

            //
            // Once the first column has been inserted, allocate the 
            // remaining width to the second column.
            //
            if (dwCol)
            {
                lvc.cx = rcLV.right - lvc.cx;
            }

            if (ListView_InsertColumn(hwndList, dwCol, &lvc) == -1)
            {
                _ASSERT(FALSE);
                break;
            }
        }

    } while (0);   
    
    m_fListValid = FALSE;

    return TRUE;
}

BOOL CNetPlacesWizardPage2::OnDestroy(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_SELECTFOLDER_LIST);
    
    if (hwndList != NULL)
        ListView_DeleteAllItems(hwndList);

    return TRUE;
}

BOOL CNetPlacesWizardPage2::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code) 
    {
    case PSN_SETACTIVE:
        {
            CWaitCursor     waitcursor;
            TCHAR           tszStaticText[MAX_PATH + MAX_STATIC + 1];
            TCHAR           tszFormatString[MAX_STATIC + 1];
            DWORD           dwError;
            HWND            hwndList = GetDlgItem(hwnd, IDC_SELECTFOLDER_LIST);

            ZeroMemory(tszStaticText,  sizeof(tszStaticText));
            ZeroMemory(tszFormatString, sizeof(tszFormatString));

            //
            // Load in the static text
            //
            FormatMessageString(IDS_SELECTFOLDER_STATIC, tszStaticText, ARRAYSIZE(tszStaticText), m_pdata->netplace.GetResourceName());

            dwError = SetDlgItemText(hwnd, IDC_SELECTFOLDER_STATIC, tszStaticText);

            if (!m_fListValid)
            {
                if (!AddShareNamesToList(hwndList))
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idWelcomePage);
                    return -1;
                }
            
                //
                // Focus on the first item in the ListView
                //
                ListView_SetItemState(hwndList, 0, LVIS_FOCUSED, LVIS_FOCUSED);

            }

            // Set wizard buttons depending on whether an item is selected in
            // the listbox

            if (ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) == -1)
            {
                PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK);
            }
            else
            {
                PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
            }

            return TRUE;
        }

    case PSN_WIZBACK:
        {
            //
            // Get rid of whatever's in the ListView
            //
            ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_SELECTFOLDER_LIST));

            m_fListValid = FALSE;
        
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idWelcomePage);
            return -1;
        }

    case PSN_WIZNEXT:
        {
            HWND hwndList = GetDlgItem(hwnd, IDC_SELECTFOLDER_LIST);
            int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

            if (iItem != -1)
            {
                GetFolderChoice(hwndList, iItem);

                // Set m_fListValid to TRUE so we don't refill the ListView
                // if the user presses Back from the next page
    
                m_fListValid = TRUE;
        
                // Set the wizard to display page 3 (completion)
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idCompletionPage);
            }
            else
            {
                // Some weird error occured; don't proceed to the completion page
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, m_pdata->idFoldersPage);
            }

            return -1;
        }

    case LVN_ITEMCHANGED:
        {
            LPNMLISTVIEW pnmhList = (LPNMLISTVIEW) pnmh;

            if (pnmhList->uChanged & LVIF_STATE)
            {
                if (pnmhList->uNewState & LVIS_SELECTED)
                {
                    // User has selected a different folder in the ListView,
                    // so activate the Next button
                    PropSheet_SetWizButtons(GetParent(hwnd),
                                            PSWIZB_BACK | PSWIZB_NEXT);
                }
                else
                {

                    // There is now nothing selected in the ListView, so
                    // disable the Next button.
                    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK);
                }
            }
            return FALSE;
        }

    case NM_DBLCLK:
        {
            LPNMLISTVIEW pnmhList = (LPNMLISTVIEW) pnmh;

            if (-1 != pnmhList->iItem)
            {
                // Act like we pressed next
                PropSheet_PressButton(GetParent(hwnd), PSBTN_NEXT);
            }

            return TRUE;
        }
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage2::AddShareNamesToList(HWND hwndList)
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering EnumerateMachineFolders\n"));

    BOOL fSuccess = FALSE;

    DWORD       dwRetVal;
    NETRESOURCE nr = {0};
    HANDLE      hEnum;
    TCHAR szResourceName[MAX_PATH + 1];

    lstrcpyn(szResourceName, m_pdata->netplace.GetResourceName(), ARRAYSIZE(szResourceName));

    nr.dwType           = RESOURCETYPE_DISK;         // Disk resources only
    nr.lpRemoteName     = szResourceName;            // Remote machine name

    //
    // Open a resource enumeration
    //
    if (NO_ERROR == WNetOpenEnum(RESOURCE_GLOBALNET,  // Resources on the network
                     RESOURCETYPE_DISK,   // Disk resources only
                     0,                   // List all resources
                     &nr,
                     &hEnum))
    {

        DWORD           dwIndex;
        DWORD           dwCount     = 0xffffffff;
        BYTE            lpResources[4096];
        DWORD           dwSize = sizeof(lpResources);
        LV_ITEM         lvi;
        UINT            i;
        LPNETRESOURCE   lpnr;

    
        lvi.mask        = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem       = COL_FOLDER;
        lvi.iSubItem    = 0;
        lvi.iImage      = Shell_GetCachedImageIndex(c_szShell32Dll, II_FOLDER, 0); // EIRESID(IDI_FOLDER), 0);

        //
        // Enumerate the network resources until there are none left
        //
        while ((dwRetVal = WNetEnumResource(hEnum,
                                            &dwCount,
                                            lpResources,
                                            &dwSize))
                    == NO_ERROR)
        {
            lpnr = (LPNETRESOURCE) lpResources;

            //
            // dwCount contains the number of resources read in
            //
            for (i = 0; i < dwCount; i++)
            {
                //
                // Traverse the resource name and strip off the server name
                //
                lvi.pszText = StrChrW(lpnr[i].lpRemoteName + 2, L'\\');
                TraceAssert(lvi.pszText);

                lvi.pszText++;

                //
                // Add the folder to the ListView
                //
                if ((dwIndex = ListView_InsertItem(hwndList, &lvi)) == -1)
                {                
                    WNetCloseEnum(hEnum);
                    TraceAssert(FALSE);
                    return FALSE;
                }
            
                //
                // Add the comment to the ListView
                //
                ListView_SetItemText(hwndList, 
                                     dwIndex, 
                                     COL_COMMENT, 
                                     lpnr[i].lpComment);
            }
    
            //
            // Reset dwCount so WNetEnumResource reads in as much as possible
            //
            dwCount = 0xffffffff;
        }
    }

    //
    // We stopped enumerating -- it should only be 
    // because we enumerated everything
    //
    switch (dwRetVal)
    {
    case ERROR_INVALID_ACCESS:
        // A server with the provided name could not be found
        DisplayFormatMessage(hwndList, IDS_ERR_CAPTION, IDS_INVALID_REMOTE_PATH,
            MB_OK|MB_ICONINFORMATION, m_pdata->netplace.GetResourceName());
        break;
    default:
        // This may be a normal situation or an error
        fSuccess = (dwRetVal == ERROR_NO_MORE_ITEMS) && (ListView_GetItemCount(hwndList) > 0);
        if (!fSuccess)
        {
            DisplayFormatMessage(hwndList, IDS_ERR_CAPTION, IDS_NOFOLDERS_STATIC, 
                MB_OK|MB_ICONINFORMATION, m_pdata->netplace.GetResourceName());
        }
        break;            
    }

    WNetCloseEnum(hEnum);
    return fSuccess;
}

void CNetPlacesWizardPage2::GetFolderChoice(HWND hwndList, int iItem)
{
    BOOL fEnableFolderEdit = FALSE;
    BOOL fEnableBrowse = FALSE;
    BOOL fEnableNext = FALSE;
    // Trace(TRACE_LEVEL_FLOW, TEXT("Entering WizardSelectFolderPage_ProcessChoice\n"));

    TCHAR   tszFolder[MAX_PATH - CNLEN];
    DWORD   dwError;
    LVITEM  lvi;

    ZeroMemory(tszFolder, sizeof(tszFolder));

    lvi.mask        = LVIF_TEXT;
    lvi.iItem       = iItem;
    lvi.iSubItem    = 0;
    lvi.pszText     = tszFolder;
    lvi.cchTextMax  = MAX_PATH - CNLEN - 1;

    dwError = ListView_GetItem(hwndList, &lvi);

    TCHAR szTemp[MAX_PATH + 1];

    dwError = wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s\\%s"), m_pdata->netplace.GetResourceName(), tszFolder);

    if (0 != dwError)
    {
        m_pdata->netplace.SetResourceName(GetParent(hwndList), szTemp);
    }
}
