#include "pch.h"
#pragma hdrstop

#include "ownerdlg.h"
#include "ownerlst.h"
#include "resource.h"
#include "uihelp.h"
#include "uiutils.h"

const static DWORD rgFileOwnerDialogHelpIDs[] =
{
    IDC_CMB_OWNERDLG_OWNERS,    IDH_CMB_OWNERDLG_OWNERS,
    IDC_LV_OWNERDLG,            IDH_LV_OWNERDLG,
    IDC_BTN_OWNERDLG_DELETE,    IDH_BTN_OWNERDLG_DELETE,
    IDC_BTN_OWNERDLG_MOVETO,    IDH_BTN_OWNERDLG_MOVETO,
    IDC_BTN_OWNERDLG_TAKE,      IDH_BTN_OWNERDLG_TAKE,
    IDC_BTN_OWNERDLG_BROWSE,    IDH_BTN_OWNERDLG_BROWSE,
    IDC_EDIT_OWNERDLG_MOVETO,   IDH_EDIT_OWNERDLG_MOVETO,
    0,0
};


CFileOwnerDialog::CFileOwnerDialog(HINSTANCE hInstance,
    HWND hwndParent,
    LPCTSTR pszVolumeRoot,
    const CArray<IDiskQuotaUser *>& rgpOwners
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDlg(NULL),
        m_hwndLV(NULL),
        m_hwndOwnerCombo(NULL),
        m_hwndEditMoveTo(NULL),
        m_iLastColSorted(-1),
        m_bSortAscending(true),
        m_rgpOwners(rgpOwners),
        m_strVolumeRoot(pszVolumeRoot)
{

}



INT_PTR
CFileOwnerDialog::Run(
    void
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CFileOwnerDialog::Run")));
    return DialogBoxParam(m_hInstance,
                          MAKEINTRESOURCE(IDD_OWNERSANDFILES),
                          m_hwndParent,
                          DlgProc,
                          (LPARAM)this);
}


INT_PTR CALLBACK
CFileOwnerDialog::DlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Retrieve the dialog object's ptr from the window's userdata.
    // Place there in response to WM_INITDIALOG.
    //
    CFileOwnerDialog *pThis = (CFileOwnerDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    try
    {
        switch(uMsg)
        {
            case WM_INITDIALOG:
                //
                // Store "this" ptr in window's userdata.
                //
                SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
                pThis = (CFileOwnerDialog *)lParam;
                //
                // Save the HWND in our object.  We'll need it later.
                //
                pThis->m_hwndDlg = hwnd;
                return pThis->OnInitDialog(hwnd);

            case WM_DESTROY:
                return pThis->OnDestroy(hwnd);

            case WM_COMMAND:
                return pThis->OnCommand(hwnd, wParam, lParam);

            case WM_NOTIFY:
                return pThis->OnNotify(hwnd, wParam, lParam);

            case WM_CONTEXTMENU:
                return pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_HELP:
                WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, STR_DSKQUOUI_HELPFILE,
                            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) rgFileOwnerDialogHelpIDs);
                return TRUE;
        }
    }
    catch(CAllocException& me)
    {
        //
        // Announce any out-of-memory errors associated with running the dlg.
        //
        DiskQuotaMsgBox(GetDesktopWindow(),
                        IDS_OUTOFMEMORY,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);
    }

    return FALSE;
}


INT_PTR
CFileOwnerDialog::OnInitDialog(
    HWND hwnd
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("CFileOwnerDialog::OnInitDialog")));
    BOOL bResult = FALSE;

    //
    // Save HWNDs of controls we'll need later.
    //
    m_hwndLV         = GetDlgItem(hwnd, IDC_LV_OWNERDLG);
    m_hwndOwnerCombo = GetDlgItem(hwnd, IDC_CMB_OWNERDLG_OWNERS);
    m_hwndEditMoveTo = GetDlgItem(hwnd, IDC_EDIT_OWNERDLG_MOVETO);

    //
    // Build the list of owners and filenames on the volume.
    // This can take a while depending on how many owners
    // are in m_rgpOwners, the size of the volume and how many
    // files each owner owns.  First clear the owner list in case Run()
    // is being called multiple times on the same dialog object.
    //
    m_OwnerList.Clear();
    HRESULT hr = BuildFileOwnerList(m_strVolumeRoot,
                                    m_rgpOwners,
                                    &m_OwnerList);
    if (SUCCEEDED(hr))
    {
        //
        // Set the message in the top of the dialog.
        //
        CString s(m_hInstance, IDS_FMT_OWNERDLG_HEADER, m_OwnerList.OwnerCount());
        SetWindowText(GetDlgItem(hwnd, IDC_TXT_OWNERDLG_HEADER), s);

        //
        // Populate the listview and owner combo.
        //
        InitializeList(m_OwnerList, m_hwndLV);
        InitializeOwnerCombo(m_OwnerList, m_hwndOwnerCombo);
        bResult = TRUE;
    }

    return bResult;
}


INT_PTR
CFileOwnerDialog::OnDestroy(
    HWND hwnd
    )
{
    return TRUE;
}


INT_PTR
CFileOwnerDialog::OnCommand(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bResult     = TRUE; // Assume not handled.
    WORD wID         = LOWORD(wParam);
    WORD wNotifyCode = HIWORD(wParam);
    HWND hCtl        = (HWND)lParam;

    switch(wID)
    {
        case IDCANCEL:
            EndDialog(hwnd, 0);
            bResult = FALSE;
            break;

        case IDC_CMB_OWNERDLG_OWNERS:
            if (CBN_SELCHANGE == wNotifyCode)
            {
                int iOwner = ComboBox_GetCurSel(m_hwndOwnerCombo);
                if (1 < m_OwnerList.OwnerCount())
                {
                    //
                    // Owner list contains more than one owner.  The combo
                    // contains a leading "All owners" entry.
                    //
                    iOwner--;
                }

                DBGASSERT((-1 <= iOwner));
                CAutoSetRedraw autoredraw(m_hwndLV, false);
                //
                // Only show "owner" column if user has selected "all owners" combo item.
                //
                CreateListColumns(m_hwndLV, -1 == iOwner);
                FillListView(m_OwnerList, m_hwndLV, iOwner);
            }
            bResult = FALSE;
            break;

        case IDC_BTN_OWNERDLG_BROWSE:
        {
            CString s;
            if (BrowseForFolder(hwnd, &s))
                SetWindowText(m_hwndEditMoveTo, s);
            break;
        }

        case IDC_BTN_OWNERDLG_DELETE:
            DeleteSelectedFiles(m_hwndLV);
            bResult = FALSE;
            break;

        case IDC_BTN_OWNERDLG_MOVETO:
        {
            CPath strDest;
            CPath strRoot;
            int cchEdit = Edit_GetTextLength(m_hwndEditMoveTo);
            Edit_GetText(m_hwndEditMoveTo,
                         strDest.GetBuffer(cchEdit + 1),
                         cchEdit + 1);
            strDest.ReleaseBuffer();
            strDest.Trim();
            strDest.GetRoot(&strRoot);

            if (IsSameVolume(strRoot, m_strVolumeRoot))
            {
                //
                // Don't let operator move files to a folder
                // on the same volume.
                //
                DiskQuotaMsgBox(m_hwndDlg,
                                IDS_ERROR_MOVETO_SAMEVOL,
                                IDS_TITLE_DISK_QUOTA,
                                MB_ICONINFORMATION | MB_OK);

                SetWindowText(m_hwndEditMoveTo, strDest);
                SetFocus(m_hwndEditMoveTo);
            }
            else
            {
                //
                // Eveything looks OK.  Try to move the files.
                //
                MoveSelectedFiles(m_hwndLV, strDest);
            }
            bResult = FALSE;
            break;
        }

        case IDC_BTN_OWNERDLG_TAKE:
        {
            HRESULT hr = TakeOwnershipOfSelectedFiles(m_hwndLV);
            if (FAILED(hr))
            {
                DBGERROR((TEXT("TakeOwnershipOfSelectedFiles failed with hr = 0x%08X"), hr));
            }
            break;
        }

        case IDC_EDIT_OWNERDLG_MOVETO:
            if (EN_UPDATE == wNotifyCode)
            {
                //
                // Disable the "Move" button if the destination edit field
                // is blank.
                //
                HWND hwnd    = GetDlgItem(m_hwndDlg, IDC_BTN_OWNERDLG_MOVETO);
                bool bEnable = ShouldEnableButton(IDC_BTN_OWNERDLG_MOVETO);
                if (bEnable != boolify(IsWindowEnabled(hwnd)))
                {
                    EnableWindow(hwnd, bEnable);
                }
            }
            break;

    }
    return bResult;
}


INT_PTR
CFileOwnerDialog::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem,
            UseWindowsHelp(idCtl) ? NULL : STR_DSKQUOUI_HELPFILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)((LPTSTR)rgFileOwnerDialogHelpIDs));

    return FALSE;
}

//
// Determine if one of the move/delete/take buttons should be enabled
// or disabled.
//
bool
CFileOwnerDialog::ShouldEnableButton(
    UINT idBtn
    )
{
    bool bEnable = true;
    int cLVSel = ListView_GetSelectedCount(m_hwndLV);
    switch(idBtn)
    {
        case IDC_BTN_OWNERDLG_DELETE:
        case IDC_BTN_OWNERDLG_TAKE:
            bEnable = 0 < cLVSel;
            break;

        case IDC_BTN_OWNERDLG_MOVETO:
            bEnable = false;
            if (0 < cLVSel)
            {
                CPath s;
                int cch = Edit_GetTextLength(m_hwndEditMoveTo);
                Edit_GetText(m_hwndEditMoveTo, s.GetBuffer(cch + 1), cch + 1);
                s.ReleaseBuffer();
                s.Trim();
                bEnable = 0 < s.Length();
            }
            break;

        default:
            break;
    }
    return bEnable;
}


INT_PTR
CFileOwnerDialog::OnNotify(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bResult = TRUE;
    LPNMHDR pnm  = (LPNMHDR)lParam;

    switch(pnm->code)
    {
        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)lParam);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)lParam);
            break;

        case LVN_ITEMCHANGED:
            OnLVN_ItemChanged((NM_LISTVIEW *)lParam);
            break;

        case LVN_KEYDOWN:
            OnLVN_KeyDown((NMLVKEYDOWN *)lParam);
            break;

        default:
            break;
    }

    return bResult;
}


void
CFileOwnerDialog::OnLVN_GetDispInfo(
    LV_DISPINFO *plvdi
    )
{
    static CPath strPath;
    static CString strOwner;

    COwnerListItemHandle hItem(plvdi->item.lParam);
    int iOwner = hItem.OwnerIndex();
    int iFile  = hItem.FileIndex();

    if (LVIF_TEXT & plvdi->item.mask)
    {
        switch(plvdi->item.iSubItem)
        {
            case iLVSUBITEM_FILE:
                m_OwnerList.GetFileName(iOwner, iFile, &strPath);
                plvdi->item.pszText = (LPTSTR)strPath.Cstr();
                break;

            case iLVSUBITEM_FOLDER:
                m_OwnerList.GetFolderName(iOwner, iFile, &strPath);
                plvdi->item.pszText = (LPTSTR)strPath.Cstr();
                break;

            case iLVSUBITEM_OWNER:
                m_OwnerList.GetOwnerName(iOwner, &strOwner);
                plvdi->item.pszText = (LPTSTR)strOwner.Cstr();
                break;
        }
    }

    if (LVIF_IMAGE & plvdi->item.mask)
    {
        //
        // Not displaying any images.  This is just a placeholder.
        // Should be optimized out by compiler.
        //
    }
}


int CALLBACK
CFileOwnerDialog::CompareLVItems(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    CFileOwnerDialog *pdlg = reinterpret_cast<CFileOwnerDialog *>(lParamSort);
    int diff = 0;
    try
    {
        COwnerListItemHandle h1(lParam1);
        COwnerListItemHandle h2(lParam2);
        int iOwner1 = h1.OwnerIndex();
        int iOwner2 = h2.OwnerIndex();
        int iFile1  = h1.FileIndex();
        int iFile2  = h2.FileIndex();
        static CPath s1, s2;

        //
        // This array controls the comparison column IDs used when
        // values for the selected column are equal.  These should
        // remain in order of the iLVSUBITEM_xxxxx enumeration with
        // respect to the first element in each row.
        //
        static const int rgColComp[3][3] = {
            { iLVSUBITEM_FILE,   iLVSUBITEM_FOLDER, iLVSUBITEM_OWNER  },
            { iLVSUBITEM_FOLDER, iLVSUBITEM_FILE,   iLVSUBITEM_OWNER  },
            { iLVSUBITEM_OWNER,  iLVSUBITEM_FILE,   iLVSUBITEM_FOLDER }
                                           };
        int iCompare = 0;
        while(0 == diff && iCompare < ARRAYSIZE(rgColComp))
        {
            switch(rgColComp[pdlg->m_iLastColSorted][iCompare++])
            {
                case iLVSUBITEM_FILE:
                    pdlg->m_OwnerList.GetFileName(iOwner1, iFile1, &s1);
                    pdlg->m_OwnerList.GetFileName(iOwner2, iFile2, &s2);
                    break;

                case iLVSUBITEM_FOLDER:
                    pdlg->m_OwnerList.GetFolderName(iOwner1, iFile1, &s1);
                    pdlg->m_OwnerList.GetFolderName(iOwner2, iFile2, &s2);
                    break;

                case iLVSUBITEM_OWNER:
                    //
                    // Can use CPath (s1 and s2) in place of CString arg since
                    // CPath is derived from CString.
                    //
                    pdlg->m_OwnerList.GetOwnerName(iOwner1, &s1);
                    pdlg->m_OwnerList.GetOwnerName(iOwner2, &s2);
                    break;

                default:
                    //
                    // If you hit this, you need to update this function
                    // to handle the new column you've added to the listview.
                    //
                    DBGASSERT((false));
                    break;
            }
            diff = s1.Compare(s2);
        }
        //
        // Don't need contents of static strings between function invocations.
        // The strings are static to avoid repeated construction/destruction.
        // It's only a minor optimization.
        //
        s1.Empty();
        s2.Empty();
    }
    catch(...)
    {
        //
        // Do nothing.  Just return diff "as is".
        // Don't want to throw an exception back into comctl32.
        //
    }
    return pdlg->m_bSortAscending ? diff : -1 * diff;
}


void
CFileOwnerDialog::OnLVN_ColumnClick(
    NM_LISTVIEW *pnmlv
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CFileOwnerDialog::OnLVN_ColumnClick")));

    if (m_iLastColSorted != pnmlv->iSubItem)
    {
        m_bSortAscending = true;
        m_iLastColSorted = pnmlv->iSubItem;
    }
    else
    {
        m_bSortAscending = !m_bSortAscending;
    }

    ListView_SortItems(m_hwndLV, CompareLVItems, LPARAM(this));
}


//
// Called whenever a listview item has changed state.
// I'm using this to update the "enabledness" of the
// dialog buttons.  If there's nothing selected in the listview,
// the move/delete/take buttons are disabled.
//
void
CFileOwnerDialog::OnLVN_ItemChanged(
    NM_LISTVIEW *pnmlv
    )
{
    static const int rgBtns[] = { IDC_BTN_OWNERDLG_DELETE,
                                  IDC_BTN_OWNERDLG_TAKE,
                                  IDC_BTN_OWNERDLG_MOVETO };

    //
    // LVN_ITEMCHANGED is sent multiple times when you move the
    // highlight bar in a listview.
    // Only run this code when the "focused" state bit is set
    // for the "new state".  This should be the last call in
    // the series.
    //
    if (LVIS_FOCUSED & pnmlv->uNewState)
    {
        for (int i = 0; i < ARRAYSIZE(rgBtns); i++)
        {
            HWND hwnd    = GetDlgItem(m_hwndDlg, rgBtns[i]);
            bool bEnable = ShouldEnableButton(rgBtns[i]);
            if (bEnable != boolify(IsWindowEnabled(hwnd)))
            {
                EnableWindow(hwnd, bEnable);
            }
        }
    }
}


void
CFileOwnerDialog::OnLVN_KeyDown(
    NMLVKEYDOWN *plvkd
    )
{
    if (VK_DELETE == plvkd->wVKey)
    {
        DeleteSelectedFiles(m_hwndLV);
        FocusOnSomethingInListview(m_hwndLV);
    }
}



void
CFileOwnerDialog::FocusOnSomethingInListview(
    HWND hwndLV
    )
{
    //
    // Focus on something.
    //
    int iFocus = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(hwndLV, iFocus, LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);
}


//
// Creates the listview columns and populates the listview
// with filenames.
//
void
CFileOwnerDialog::InitializeList(
    const COwnerList& fol,  // file & owner list
    HWND hwndList
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::InitializeList")));

    CreateListColumns(hwndList, 1 < m_OwnerList.OwnerCount());
    FillListView(fol, hwndList);
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
}


void
CFileOwnerDialog::CreateListColumns(
    HWND hwndList,
    bool bShowOwner    // Default is true.
    )
{
    //
    // Clear out the listview and header.
    //
    ListView_DeleteAllItems(hwndList);
    HWND hwndHeader = ListView_GetHeader(hwndList);
    if (NULL != hwndHeader)
    {
        while(0 < Header_GetItemCount(hwndHeader))
            ListView_DeleteColumn(hwndList, 0);
    }

    //
    // Create the header titles.
    //
    CString strFile(m_hInstance,   IDS_OWNERDLG_HDR_FILE);
    CString strFolder(m_hInstance, IDS_OWNERDLG_HDR_FOLDER);
    CString strOwner(m_hInstance,  IDS_OWNERDLG_HDR_OWNER);

    //
    // BUGBUG:  Should probably allow for vertical scroll bar also.
    //
    RECT rcList;
    GetClientRect(hwndList, &rcList);
    int cxCol = (rcList.right - rcList.left) / (bShowOwner ? 3 : 2);

#define LVCOLMASK (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM)

    LV_COLUMN rgCols[] = {
         { LVCOLMASK, LVCFMT_LEFT, cxCol, strFile,   0, iLVSUBITEM_FILE   },
         { LVCOLMASK, LVCFMT_LEFT, cxCol, strFolder, 0, iLVSUBITEM_FOLDER },
         { LVCOLMASK, LVCFMT_LEFT, cxCol, strOwner,  0, iLVSUBITEM_OWNER  }
                         };
    //
    // Add the columns to the listview.
    //
    int cCols = bShowOwner ? ARRAYSIZE(rgCols) : ARRAYSIZE(rgCols) - 1;
    for (INT i = 0; i < cCols; i++)
    {
        if (-1 == ListView_InsertColumn(hwndList, i, &rgCols[i]))
        {
            DBGERROR((TEXT("CFileOwnerDialog::CreateListColumns failed to add column %d"), i));
        }
    }
}


void
CFileOwnerDialog::FillListView(
    const COwnerList& fol,  // file & owner list
    HWND hwndList,
    int iOwner              // default is -1 (all owners)
    )
{
    ListView_DeleteAllItems(hwndList);

    LV_ITEM item;
    item.iSubItem  = 0;
    item.mask      = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM;
    item.state     = 0;
    item.stateMask = 0;
    item.pszText   = LPSTR_TEXTCALLBACK;
    item.iImage    = I_IMAGECALLBACK;

    //
    // This prevents the listview from having to extend itself each time we
    // add an item.
    //
    ListView_SetItemCount(hwndList, fol.FileCount(iOwner));

    int iFirst = iOwner;
    int iLast  = iOwner;
    if (-1 == iOwner)
    {
        iFirst = 0;
        iLast  = fol.OwnerCount() - 1;
    }
    int iItem = 0;
    //
    // WARNING:  Reusing formal arg iOwner.  It's safe to do, but you
    //           should be aware that I'm doing it.
    //
    for (iOwner = iFirst; iOwner <= iLast; iOwner++)
    {
        int cFiles = fol.FileCount(iOwner);
        for (int iFile = 0; iFile < cFiles; iFile++)
        {
            if (!fol.IsFileDeleted(iOwner, iFile))
            {
                item.lParam = COwnerListItemHandle(iOwner, iFile);
                item.iItem  = iItem++;
                if (-1 == ListView_InsertItem(hwndList, &item))
                    DBGERROR((TEXT("Error adding LV item for owner %d, file %d"), iOwner, iFile));
            }
        }
    }
}


void
CFileOwnerDialog::InitializeOwnerCombo(
    const COwnerList& fol,  // file & owner list
    HWND hwndCombo
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::InitializeList")));

    int iSelected = ComboBox_GetCurSel(hwndCombo);
    ComboBox_ResetContent(hwndCombo);

    CString s, s2;
    int cOwners = fol.OwnerCount();
    if (1 < cOwners)
    {
        //
        // Add "all owners" entry.
        //
        s.Format(m_hInstance, IDS_FMT_ALLOWNERS, fol.FileCount());
        ComboBox_InsertString(hwndCombo, -1, s);
    }

    for (int iOwner = 0; iOwner < cOwners; iOwner++)
    {
        fol.GetOwnerName(iOwner, &s2);
        s.Format(m_hInstance, IDS_FMT_OWNER, s2.Cstr(), fol.FileCount(iOwner));
        ComboBox_InsertString(hwndCombo, -1, s);
    }

    ComboBox_SetCurSel(hwndCombo, CB_ERR != iSelected ? iSelected : 0);

    //
    // Set the max height of the owner combo
    //
    RECT rcCombo;
    GetClientRect(m_hwndOwnerCombo, &rcCombo);
    SetWindowPos(m_hwndOwnerCombo,
                 NULL,
                 0, 0,
                 rcCombo.right - rcCombo.left,
                 200,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
}


//
// Determine if two volume root strings refer to the same volume.
// With volume mount points, "C:\" and "D:\DriveC" could refer to the
// same physical volume.  To differentiate we need to examine the unique
// volume name GUID strings.
//
bool
CFileOwnerDialog::IsSameVolume(
    LPCTSTR pszRoot1,
    LPCTSTR pszRoot2
    )
{
    TCHAR szVolGUID1[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    bool bSameVolume = false;

    //
    // GetVolumeNameForVolumeMountPoint requires trailing backslash on paths.
    //
    lstrcpyn(szTemp, pszRoot1, ARRAYSIZE(szTemp));
    PathAddBackslash(szTemp);
    
    if (GetVolumeNameForVolumeMountPoint(szTemp, szVolGUID1, ARRAYSIZE(szVolGUID1)))
    {
        TCHAR szVolGUID2[MAX_PATH];
        lstrcpyn(szTemp, pszRoot2, ARRAYSIZE(szTemp));
        PathAddBackslash(szTemp);

        if (GetVolumeNameForVolumeMountPoint(szTemp, szVolGUID2, ARRAYSIZE(szVolGUID2)))
        {
            if (0 == lstrcmpi(szVolGUID1, szVolGUID2))
                bSameVolume = true;
        }
    }
    return bSameVolume;
}

//
// Let the user browse for a folder.
// The selected folder path is returned in *pstrFolder.
//
bool
CFileOwnerDialog::BrowseForFolder(
    HWND hwndParent,
    CString *pstrFolder
    )
{
    bool bResult = false;
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));

    CString strTitle(m_hInstance, IDS_BROWSEFORFOLDER);

    bi.hwndOwner      = hwndParent;
    bi.pidlRoot       = NULL;       // Start at desktop.
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = strTitle.Cstr();
    //
    // BUGBUG:  Setting the BIF_EDITBOX flag causes SHBrowseForFolder to invoke
    //          autocomplete through SHAutoComplete (in shlwapi).  SHAutoComplete
    //          loads browseui.dll to implement the autocomplete feature.  The bad
    //          part is that SHAutoComplete also unloads browseui.dll before it
    //          returns, resulting in calls to the unloaded WndProc.  I've notified
    //          ReinerF about this.  Turning off the BIF_EDITBOX bit prevents
    //          autocomplete from being used and thus prevents the problem.
    //          I want the edit box.  Turn it back on once they fix this bug.
    //
    //          brianau [1/30/97]
    //
    bi.ulFlags        = BIF_RETURNONLYFSDIRS; // | BIF_EDITBOX;
    bi.lpfn           = BrowseForFolderCallback;
    bi.lParam         = (LPARAM)pstrFolder;
    bi.iImage         = 0;

    bResult = boolify(SHBrowseForFolder(&bi));
    return bResult;
}


//
// Callback called by SHBrowseForFolder.  Writes selected folder path
// to CString object who's pointer is passed in lpData arg.
//
int
CFileOwnerDialog::BrowseForFolderCallback(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::BrowseForFolderCallback")));
    CString *pstrFolder = (CString *)lpData;

    if (BFFM_SELCHANGED == uMsg)
    {
        SHGetPathFromIDList((LPCITEMIDLIST)lParam, pstrFolder->GetBuffer(MAX_PATH));
        pstrFolder->ReleaseBuffer();
    }
    return 0;
}


//
// Builds a double-nul terminated list of file paths from the listview
// along with an array of "item handle" objects that acts as a cross-
// reference between the list items, items in the listview and items
// in the file owner list.  Each handle contains an owner index and
// file index into the file owner list.  Each handle is also the value
// stored as the lParam in the listview items.
// Both pList and prgItemHandles arguments are optional.  Although,
// calling with neither non-null is sort of useless.
//
void
CFileOwnerDialog::BuildListOfSelectedFiles(
    HWND hwndLV,
    DblNulTermList *pList,
    CArray<COwnerListItemHandle> *prgItemHandles
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::BuildListOfSelectedFiles")));
    int iItem = -1;
    CPath strPath;
    LV_ITEM item;

    if (NULL != prgItemHandles)
        prgItemHandles->Clear();
    while(-1 != (iItem = ListView_GetNextItem(hwndLV, iItem, LVNI_SELECTED)))
    {
        item.iSubItem = 0;
        item.iItem    = iItem;
        item.mask     = LVIF_PARAM;
        if (-1 != ListView_GetItem(hwndLV, &item))
        {
            COwnerListItemHandle hItem(item.lParam);
            m_OwnerList.GetFileFullPath(hItem.OwnerIndex(),
                                        hItem.FileIndex(),
                                        &strPath);
            if (pList)
                pList->AddString(strPath);
            if (prgItemHandles)
                prgItemHandles->Append(hItem);
        }
    }
}

//
// Given an item "handle", find it's entry in the listview.
//
int
CFileOwnerDialog::FindItemFromHandle(
    HWND hwndLV,
    const COwnerListItemHandle& handle
    )
{
    LV_FINDINFO lvfi;
    lvfi.flags  = LVFI_PARAM;
    lvfi.lParam = handle;
    return ListView_FindItem(hwndLV, -1, &lvfi);
}


//
// Scans an array of item handles and removes all corresponding
// items from the listview.
//
void
CFileOwnerDialog::RemoveListViewItems(
    HWND hwndLV,
    const CArray<COwnerListItemHandle>& rgItemHandles
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::RemoveListViewItems")));
    LV_ITEM item;
    CPath strPath;

    CAutoSetRedraw autoredraw(hwndLV, false);
    int cHandles = rgItemHandles.Count();
    for (int iHandle = 0; iHandle < cHandles; iHandle++)
    {
        COwnerListItemHandle handle = rgItemHandles[iHandle];
        int iItem = FindItemFromHandle(hwndLV, handle);
        if (-1 != iItem)
        {
            int iOwner = handle.OwnerIndex();
            int iFile  = handle.FileIndex();
            m_OwnerList.GetFileFullPath(iOwner, iFile, &strPath);

            if ((DWORD)-1 == GetFileAttributes(strPath))
            {
                //
                // File doesn't exist any more.
                // Delete from the listview.
                // Mark it as "deleted" in the ownerlist container.
                //
                ListView_DeleteItem(hwndLV, iItem);
                m_OwnerList.MarkFileDeleted(iOwner, iFile);
                DBGPRINT((DM_VIEW, DL_LOW, TEXT("Removed item %d \"%s\""),
                         iItem, strPath.Cstr()));
            }
        }
    }
    //
    // Refresh the owner combo to update the file counts.
    //
    InitializeOwnerCombo(m_OwnerList, m_hwndOwnerCombo);
}


//
// Delete the files selected in the listview.
// Files deleted are removed from the listview.
//
void
CFileOwnerDialog::DeleteSelectedFiles(
    HWND hwndLV
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::DeleteSelectedFiles")));
    DblNulTermList list(1024);  // 1024 is the buffer growth size in chars.
    CArray<COwnerListItemHandle> rgItemHandles;

    BuildListOfSelectedFiles(hwndLV, &list, &rgItemHandles);
    if (0 < list.Count())
    {
        SHFILEOPSTRUCT fo;
        fo.hwnd   = m_hwndDlg;
        fo.wFunc  = FO_DELETE;
        fo.pFrom  = list;
        fo.pTo    = NULL;
        fo.fFlags = 0;

        if (0 != SHFileOperation(&fo))
        {
            DBGERROR((TEXT("SHFileOperation [FO_DELETE] failed")));
        }
        //
        // Remove listview items if their files were really deleted.
        //
        RemoveListViewItems(hwndLV, rgItemHandles);
    }
}


//
// Move the selected files to a new location.
// Moved files are removed from the listview.
//
void
CFileOwnerDialog::MoveSelectedFiles(
    HWND hwndLV,
    LPCTSTR pszDest
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::DeleteSelectedFiles")));
    DblNulTermList list(1024);  // 1024 is the buffer growth size in chars.
    CArray<COwnerListItemHandle> rgItemHandles;

    BuildListOfSelectedFiles(hwndLV, &list, &rgItemHandles);
    if (0 < list.Count())
    {
        CPath strDest(pszDest);
        if (1 == list.Count())
        {
            //
            // If we have only a single file we MUST create a fully-qualified
            // path to the destination file.  Oddities in the shell's move/copy
            // engine won't let us pass merely a destination folder in the
            // case where that folder doesn't exist.  If we give the full path
            // including filename we'll get the "folder doesn't exist, create
            // now?" messagebox as we would expect.  If we're moving multiple
            // files the shell accepts a single directory path.
            //
            LPCTSTR psz;
            DblNulTermListIter iter(list);
            if (iter.Next(&psz))
            {
                CPath strSrc(psz);           // Copy the source
                CPath strFile;               
                strSrc.GetFileSpec(&strFile);// Extract the filename.
                strDest.Append(strFile);     // Append to the dest path.
            }
        }
            
        SHFILEOPSTRUCT fo;
        fo.hwnd   = m_hwndDlg;
        fo.wFunc  = FO_MOVE;
        fo.pFrom  = list;
        fo.pTo    = strDest;
        fo.fFlags = FOF_RENAMEONCOLLISION;

        if (0 != SHFileOperation(&fo))
        {
            DBGERROR((TEXT("SHFileOperation [FO_MOVE] failed")));
        }
        //
        // Remove listview items if their file was really deleted.
        //
        RemoveListViewItems(hwndLV, rgItemHandles);
    }
}


//
// Get the SID to use for taking ownership of files.
// First try to get the first group SID with the SE_GROUP_OWNER attribute.
// If none found, use the operator's account SID.  The SID is in a
// dynamic buffer attached to the ptrSid autoptr argument.
//
HRESULT
CFileOwnerDialog::GetOwnershipSid(
    array_autoptr<BYTE> *ptrSid
    )
{
    HRESULT hr  = NOERROR;
    DWORD dwErr = 0;

    //
    // Get the token handle. First try the thread token then the process
    // token.  If these fail we return early.  No sense in continuing
    // on if we can't get a user token.
    //
    CWin32Handle hToken;
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_READ,
                         TRUE,
                         hToken.HandlePtr()))
    {
        if (ERROR_NO_TOKEN == GetLastError())
        {
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_READ,
                                  hToken.HandlePtr()))
            {
                dwErr = GetLastError();
                DBGERROR((TEXT("Error %d opening process token"), dwErr));
                return HRESULT_FROM_WIN32(dwErr);
            }
        }
        else
        {
            dwErr = GetLastError();
            DBGERROR((TEXT("Error %d opening thread token"), dwErr));
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    //
    // Get the required size of the group token information buffer.
    //
    array_autoptr<BYTE> ptrTokenInfo;
    DWORD cbTokenInfo = 0;

    if (!GetTokenInformation(hToken,
                             TokenGroups,
                             NULL,
                             cbTokenInfo,
                             &cbTokenInfo))
    {
        dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwErr)
        {
            ptrTokenInfo = new BYTE[cbTokenInfo];
        }
        else
        {
            dwErr = GetLastError();
            DBGERROR((TEXT("Error %d getting TokenGroups info [for size]"), dwErr));
            hr = HRESULT_FROM_WIN32(hr);
        }
    }

    //
    // Get the group token information.
    //
    if (SUCCEEDED(hr))
    {
        if (!GetTokenInformation(hToken,
                                 TokenGroups,
                                 ptrTokenInfo.get(),
                                 cbTokenInfo,
                                 &cbTokenInfo))
        {
            dwErr = GetLastError();
            DBGERROR((TEXT("Error %d getting TokenGroups info"), dwErr));
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        else
        {
            //
            // Extract the first SID with the GROUP_OWNER bit set.
            //
            TOKEN_GROUPS *ptg = (TOKEN_GROUPS *)ptrTokenInfo.get();
            DBGASSERT((NULL != ptg));
            for (DWORD i = 0; i < ptg->GroupCount; i++)
            {
                SID_AND_ATTRIBUTES *psa = (SID_AND_ATTRIBUTES *)&ptg->Groups[i];
                DBGASSERT((NULL != psa));
                if (SE_GROUP_OWNER & psa->Attributes)
                {
                    int cbSid = GetLengthSid(psa->Sid);
                    *ptrSid = new BYTE[cbSid];
                    CopySid(cbSid, ptrSid->get(), psa->Sid);
                    hr = NOERROR;
                    break;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        //
        // Didn't find a SID from the group information.
        // Use the operator's SID.
        //
        cbTokenInfo = 0;
        if (!GetTokenInformation(hToken,
                                 TokenUser,
                                 NULL,
                                 cbTokenInfo,
                                 &cbTokenInfo))
        {
            dwErr = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == dwErr)
            {
                ptrTokenInfo = new BYTE[cbTokenInfo];
            }
            else
            {
                dwErr = GetLastError();
                DBGERROR((TEXT("Error %d getting TokenUser info [for size]"), dwErr));
                hr = HRESULT_FROM_WIN32(hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            //
            // Get the user token information.
            //
            if (!GetTokenInformation(hToken,
                                     TokenUser,
                                     ptrTokenInfo.get(),
                                     cbTokenInfo,
                                     &cbTokenInfo))
            {
                dwErr = GetLastError();
                DBGERROR((TEXT("Error %d getting TokenUser info"), dwErr));
                hr = HRESULT_FROM_WIN32(dwErr);
            }
            else
            {
                SID_AND_ATTRIBUTES *psa = (SID_AND_ATTRIBUTES *)ptrTokenInfo.get();
                DBGASSERT((NULL != psa));
                int cbSid = GetLengthSid(psa->Sid);
                *ptrSid = new BYTE[cbSid];
                CopySid(cbSid, ptrSid->get(), psa->Sid);
                hr = NOERROR;
            }
        }
    }
    if (SUCCEEDED(hr) && NULL != ptrSid->get() && !IsValidSid(ptrSid->get()))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }
    return hr;
}


//
// Transfers ownership of selected files in the listview to the
// currently logged-on user.
//
HRESULT
CFileOwnerDialog::TakeOwnershipOfSelectedFiles(
    HWND hwndLV
    )
{
    HRESULT hr = NOERROR;
    DWORD dwErr = 0;
    CArray<COwnerListItemHandle> rgItemHandles;

    BuildListOfSelectedFiles(hwndLV, NULL, &rgItemHandles);
    if (0 == rgItemHandles.Count())
        return S_OK;

    array_autoptr<BYTE> ptrSid;
    hr = GetOwnershipSid(&ptrSid);
    if (FAILED(hr))
        return hr;

    CPath strFile;
    int cHandles = rgItemHandles.Count();
    for (int i = 0; i < cHandles; i++)
    {
        COwnerListItemHandle handle = rgItemHandles[i];
        int iItem = FindItemFromHandle(hwndLV, handle);
        if (-1 != iItem)
        {
            SECURITY_DESCRIPTOR sd;
            if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            {
                int iOwner = handle.OwnerIndex();
                int iFile  = handle.FileIndex();
                m_OwnerList.GetFileFullPath(iOwner, iFile, &strFile);
                if (SetSecurityDescriptorOwner(&sd, ptrSid.get(), FALSE))
                {
                    if (SetFileSecurity(strFile, OWNER_SECURITY_INFORMATION, &sd))
                    {
                        ListView_DeleteItem(hwndLV, iItem);
                        m_OwnerList.MarkFileDeleted(iOwner, iFile);
                    }
                    else
                    {
                        dwErr = GetLastError();
                        DBGERROR((TEXT("Error %d setting new owner for \"%s\""),
                                 dwErr, strFile.Cstr()));
                        hr = HRESULT_FROM_WIN32(dwErr);
                    }
                }
                else
                {
                    dwErr = GetLastError();
                    DBGERROR((TEXT("Error %d setting security descriptor owner"), dwErr));
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }
            else
            {
                dwErr = GetLastError();
                DBGERROR((TEXT("Error %d initing security descriptor"), GetLastError()));
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }
        else
        {
            DBGERROR((TEXT("Can't find listview item for owner %d, file %d"),
                     handle.OwnerIndex(), handle.FileIndex()));
        }
    }
    //
    // Refresh the owner combo with the new file counts.
    //
    InitializeOwnerCombo(m_OwnerList, m_hwndOwnerCombo);
    return hr;
}



//
// The original code for listing files owned by a user was
// contributed by MarkZ.  I made some minor modifications
// to fit it into the diskquota project and make it more
// exception safe.
//
inline VOID *
Add2Ptr(VOID *pv, ULONG cb)
{
    return((BYTE *) pv + cb);
}

inline ULONG
QuadAlign( ULONG Value )
{
    return (Value + 7) & ~7;
}


//
// Add files owned by a particular user on a particular volume.
//
HRESULT
CFileOwnerDialog::AddFilesToOwnerList(
    LPCTSTR pszVolumeRoot,
    HANDLE hVolumeRoot,
    IDiskQuotaUser *pOwner,
    COwnerList *pOwnerList
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::AddFilesToOwnerList")));
    DBGASSERT((NULL != hVolumeRoot));
    DBGASSERT((NULL != pOwner));
    DBGASSERT((NULL != pOwnerList));

    struct
    {
        ULONG Restart;
        BYTE Sid[MAX_SID_LEN];
    }FsCtlInput;

    NTSTATUS status = ERROR_SUCCESS;

    //
    // Get owner's SID.
    //
    HRESULT hr = pOwner->GetSid(FsCtlInput.Sid, sizeof(FsCtlInput.Sid));
    if (FAILED(hr))
    {
        DBGERROR((TEXT("IDiskQuotaUser::GetSid failed with hr = 0x%08X"), hr));
        return hr;
    }

    //
    // Add the owner to the owner-file list.
    //
    int iOwner = pOwnerList->AddOwner(pOwner);

    IO_STATUS_BLOCK iosb;
    FsCtlInput.Restart = 1;
    BYTE Output[1024];
    bool bPathIsRemote = false;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    //
    // Determine if the volume is a remote device.  This will affect
    // our handling of the paths returned by NtQueryInformationFile.
    //
    status = NtQueryVolumeInformationFile(
                    hVolumeRoot,
                    &iosb,
                    &DeviceInfo,
                    sizeof(DeviceInfo),
                    FileFsDeviceInformation);
                    
    if (NT_SUCCESS(status))
    {
        bPathIsRemote = (FILE_REMOTE_DEVICE == DeviceInfo.Characteristics);
    }

    while (true)
    {
        status = NtFsControlFile(hVolumeRoot,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &iosb,
                                 FSCTL_FIND_FILES_BY_SID,
                                 &FsCtlInput,
                                 sizeof(FsCtlInput),
                                 Output,
                                 sizeof(Output));

        FsCtlInput.Restart = 0;

        if (!NT_SUCCESS(status) && STATUS_BUFFER_OVERFLOW != status)
        {
            DBGERROR((TEXT("NtFsControlFile failed with status 0x%08X"), status));
            return HRESULT_FROM_NT(status);
        }

        if (0 == iosb.Information)
        {
            //
            // No more data.
            //
            break;
        }

        PFILE_NAME_INFORMATION pFileNameInfo = (PFILE_NAME_INFORMATION)Output;

        while ((PBYTE)pFileNameInfo < Output + iosb.Information)
        {
            ULONG Length = sizeof(FILE_NAME_INFORMATION) - sizeof(WCHAR) +
                           pFileNameInfo->FileNameLength;

            CNtHandle hChild;
            WCHAR szChild[MAX_PATH];

            RtlMoveMemory(szChild, pFileNameInfo->FileName, pFileNameInfo->FileNameLength);
            szChild[pFileNameInfo->FileNameLength / sizeof(WCHAR)] = L'\0';
            status = OpenNtObject(szChild,
                                  hVolumeRoot,
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                                  FILE_READ_ATTRIBUTES,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  FILE_OPEN,
                                  hChild.HandlePtr());

            if (!NT_SUCCESS(status))
            {
                DBGERROR((TEXT("Unable to open file \"%s\".  Status = 0x%08X"),
                         szChild, status));
            }
            else
            {
                //
                // Is the file a directory?  We don't include directories.
                //
                IO_STATUS_BLOCK iosb2;
                FILE_BASIC_INFORMATION fbi;
                status = NtQueryInformationFile(hChild,
                                                &iosb2,
                                                &fbi,
                                                sizeof(fbi),
                                                FileBasicInformation);
                if (!NT_SUCCESS(status))
                {
                    DBGERROR((TEXT("NtQueryInformationFile failed with status 0x%08X for \"%s\""),
                              status, szChild));
                }
                else if (0 == (FILE_ATTRIBUTE_DIRECTORY & fbi.FileAttributes))
                {
                    //
                    // Get the file's name (full path).
                    //
                    WCHAR szFile[MAX_PATH + 10];
                    status = NtQueryInformationFile(hChild,
                                                    &iosb2,
                                                    szFile,
                                                    sizeof(szFile),
                                                    FileNameInformation);

                    if (!NT_SUCCESS(status))
                    {
                        DBGERROR((TEXT("NtQueryInformation file failed with status 0x%08X for \"%s\""),
                                  status, szChild));
                    }
                    else
                    {
                        PFILE_NAME_INFORMATION pfn = (PFILE_NAME_INFORMATION)szFile;
                        pfn->FileName[pfn->FileNameLength / sizeof(WCHAR)] = L'\0';
                        CPath path;

                        //
                        // If the path is remote, NtQueryInformationFile returns
                        // a string like this:
                        //
                        //  \server\share\dir1\dir2\file.ext
                        //
                        // If the path is local, NtQueryInformationFile returns
                        // a string like this:
                        //
                        //  \dir1\dir2\file.ext
                        //
                        // For remote paths we merely prepend a '\' to create a
                        // valid UNC path.  For local paths we prepend the local
                        // drive specification.
                        //
                        if (bPathIsRemote)
                        {
                            path = L"\\";
                            path += CString(pfn->FileName);
                        }
                        else
                        {
                            path = pszVolumeRoot;
                            path.Append(pfn->FileName);
                        }

                        DBGPRINT((DM_VIEW, DL_LOW, TEXT("Adding \"%s\""), path.Cstr()));
                        pOwnerList->AddFile(iOwner, path);
                    }
                }
            }
            hChild.Close();

            pFileNameInfo =
                (PFILE_NAME_INFORMATION) Add2Ptr(pFileNameInfo, QuadAlign(Length));
        }
    }
    return NOERROR;
}


//
// Build a list of files owned by a set of users on a particular volume.
// pszVolumeRoot is the volume root directory (i.e. "C:\").
// rgpOwners is an array of user object pointers, one for each owner.
// pOwnerList is the container where the resulting filenames are placed.
// Calls AddFilesToOwnerList() for each owner in rgpOwners.
//
HRESULT
CFileOwnerDialog::BuildFileOwnerList(
    LPCTSTR pszVolumeRoot,
    const CArray<IDiskQuotaUser *>& rgpOwners,
    COwnerList *pOwnerList
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CFileOwnerDialog::BuildFileOwnerList")));
    HRESULT hr = NOERROR;
    CNtHandle hVolumeRoot;
    NTSTATUS status = OpenNtObject(pszVolumeRoot,
                                   NULL,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   FILE_READ_ATTRIBUTES,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_OPEN,
                                   hVolumeRoot.HandlePtr());

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_NT(status);

    int cOwners = rgpOwners.Count();
    for (int i = 0; i < cOwners; i++)
    {
        hr = AddFilesToOwnerList(pszVolumeRoot, hVolumeRoot, rgpOwners[i], pOwnerList);
    }
    return hr;
}


//
// MarkZ had this function in his original implementation so I just
// kept it.  I did need to fix a bug in the original code.  He was
// calling RtlFreeHeap() on str.Buffer for all cases.  This is was
// not applicable in the RtlInitUnicodeString() case where the
// unicode string is merely bound to the pszFile argument.
//
NTSTATUS
CFileOwnerDialog::OpenNtObject (
    LPCWSTR pszFile,
    HANDLE RelatedObject,
    ULONG CreateOptions,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    HANDLE *ph)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING str;
    IO_STATUS_BLOCK isb;
    bool bFreeString = false;

    if (NULL == RelatedObject)
    {
        RtlDosPathNameToNtPathName_U(pszFile, &str, NULL, NULL);
        bFreeString = true;
    }
    else
    {
        //
        // This just attaches pszFile to the rtl string.
        // We don't free it.
        //
        RtlInitUnicodeString(&str, pszFile);
    }

    InitializeObjectAttributes(&oa,
                               &str,
                               OBJ_CASE_INSENSITIVE,
                               RelatedObject,
                               NULL);

    status = NtCreateFile(ph,
                          DesiredAccess | SYNCHRONIZE,
                          &oa,
                          &isb,
                          NULL,                   // pallocationsize (none!)
                          FILE_ATTRIBUTE_NORMAL,
                          ShareAccess,
                          CreateDisposition,
                          CreateOptions,
                          NULL,                   // EA buffer (none!)
                          0);

    if (bFreeString)
        RtlFreeHeap(RtlProcessHeap(), 0, str.Buffer);
    return(status);
}
