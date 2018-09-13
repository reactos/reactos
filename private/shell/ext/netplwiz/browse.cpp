#include "stdafx.h"

#include "browse.h"

INT_PTR CNoDsBrowseDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_SETCURSOR, OnSetCursor);
    default:
        break;
    }

    return FALSE;
}

BOOL CNoDsBrowseDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_SERVER_LIST);

    // Set the image list for the
    ListView_SetImageList(hwndList, m_pdata->himlSmall, LVSIL_SMALL);
        
    // Create 2 listview columns.  If more are added, the column
    // width calculation needs to change.

    LV_COLUMN   lvc = {0};
    RECT        rcListView;
    TCHAR       szColumnLabel[MAX_CAPTION];

    GetClientRect(hwndList, &rcListView);
    rcListView.right -= GetSystemMetrics(SM_CXVSCROLL);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = szColumnLabel;

    // Insert the first column
    lvc.cx = rcListView.right / 2;
    lvc.iSubItem = 0;
    LoadString(g_hInstance, IDS_SERVERLIST_FIRSTCOLUMN, szColumnLabel, 
        ARRAYSIZE(szColumnLabel));

    ListView_InsertColumn(hwndList, 0, &lvc);

    // Insert the second column
    lvc.cx = rcListView.right - lvc.cx;
    lvc.iSubItem = 1;
    LoadString(g_hInstance, IDS_SERVERLIST_SECONDCOLUMN, szColumnLabel, 
        ARRAYSIZE(szColumnLabel));

    ListView_InsertColumn(hwndList, 1, &lvc);

    // Fill the listview
    DWORD dwThread;
    m_hwndList = hwndList;
    HANDLE hThread = CreateThread(NULL, NULL, CNoDsBrowseDialog::AddServerNamesThread, 
        (LPVOID) this, 0, &dwThread);

    // Delete our reference to this thread
    if (hThread != NULL)
    {
        CloseHandle(hThread);
    }

    EnableOKButton(hwnd, FALSE);

    return TRUE;
}

DWORD WINAPI CNoDsBrowseDialog::AddServerNamesThread(LPVOID lpParam)
{
    // Make sure library stays in memory for the duration of this thread
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(g_hInstance, szPath, ARRAYSIZE(szPath));
    HMODULE hMod = LoadLibrary(szPath);
    if (hMod != NULL)
    {
        CNoDsBrowseDialog* pthis = (CNoDsBrowseDialog*) lpParam;

        pthis->m_fShowWaitCursor = TRUE;
        pthis->AddServerNamesToList(pthis->m_hwndList);
        pthis->m_fShowWaitCursor = FALSE;

        FreeLibraryAndExitThread(hMod, 0);
    }
    else
    {
        // Error; couldn't lock library in memory!
    }
    return 0;
}


BOOL CNoDsBrowseDialog::AddServerNamesToList(HWND hwndList)
{
    DWORD       dwRetVal;
    HANDLE      hEnum;

    //
    // Open a resource enumeration
    //
    if (WNetOpenEnum(RESOURCE_CONTEXT,           // Resources in this machine's context
                     RESOURCETYPE_DISK,          // Disk resources only
                     RESOURCEUSAGE_CONTAINER,  // List all resources
                     NULL,
                     &hEnum)
                     
            != NO_ERROR)
    {
        return FALSE;
    }

    // Be able to read 20 net resources at a time
    NETRESOURCE     rgnr[20];
    DWORD           dwIndex;
    DWORD           dwCount     = 0xffffffff;
    DWORD           dwSize      = sizeof(rgnr);
    LV_ITEM         lvi = {0};
    UINT            i;

    lvi.mask        = LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem       = 0;
    lvi.iSubItem    = 0;
    lvi.iImage      = Shell_GetCachedImageIndex(c_szShell32Dll, II_SERVER, 0); // EIRESID(IDI_SERVER), 0);

    // Enumerate the network resources until there are none left
    while (IsWindow(hwndList) && 
        (dwRetVal = WNetEnumResource(hEnum, &dwCount, rgnr, &dwSize) == NO_ERROR))
    {
        // dwCount contains the number of resources read in
        for (i = 0; (i < dwCount && IsWindow(hwndList)); i++)
        {
            // Only display servers
            if ((rgnr[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) && IsWindow(hwndList))
             
            {
                if (lstrlen(rgnr[i].lpRemoteName) >= 3)
                {
                    // From the server name "\\xxxx", just take "xxxx"
                    lvi.pszText = &(rgnr[i].lpRemoteName[2]);

                    // Add the folder to the ListView
                    dwIndex = ListView_InsertItem(hwndList, &lvi);

                    // Add the comment to the ListView
                    ListView_SetItemText(hwndList, 
                                         dwIndex, 
                                         1, // Second column
                                         rgnr[i].lpComment);
                }
            }
        }
    
        //
        // Reset dwCount so WNetEnumResource reads in as much as possible
        //
        dwCount = 0xffffffff;
    }

    WNetCloseEnum(hEnum);

    //
    // We stopped enumerating -- it should only be 
    // because we enumerated everything
    //

    if (dwRetVal != ERROR_NO_MORE_ITEMS)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CNoDsBrowseDialog::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        {
            HWND hwndList = GetDlgItem(hwnd, IDC_SERVER_LIST);
            OnOK(hwnd, ListView_GetNextItem(hwndList, -1, LVNI_SELECTED));
        }
        
        // Fall through
    case IDCANCEL:

        EndDialog(hwnd, id);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

BOOL CNoDsBrowseDialog::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        {
            LPNMLISTVIEW pnmhList = (LPNMLISTVIEW) pnmh;

            if (pnmhList->uChanged & LVIF_STATE)
            {
                if (pnmhList->uNewState & LVIS_SELECTED)
                {
                    EnableOKButton(hwnd, TRUE);
                }
                else
                {
                    EnableOKButton(hwnd, FALSE);
                }
            }
            return FALSE;
        }
    case NM_DBLCLK:
        OnOK(hwnd, ((LPNMLISTVIEW) pnmh)->iItem);
        EndDialog(hwnd, IDOK);
        break;

    default:
        break;
    }
    return FALSE;
}

BOOL CNoDsBrowseDialog::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    // Show the background wait cursor if applicable
    if (m_fShowWaitCursor)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
        return TRUE;
    }

    // Otherwise trigger default processing
    return FALSE;
}

void CNoDsBrowseDialog::EnableOKButton(HWND hwnd, BOOL fEnable)
{
    HWND hwndDlg = GetDlgItem(hwnd, IDOK);
    EnableWindow(hwndDlg, fEnable);
}

void CNoDsBrowseDialog::OnOK(HWND hwnd, int iItem)
{
    // Fill the supplied buffer with the currently selected item
    LVITEM lvitem = {0};
    HWND hwndList = GetDlgItem(hwnd, IDC_SERVER_LIST);

    lvitem.mask = LVIF_TEXT;
    lvitem.pszText = m_pszBuffer;
    lvitem.cchTextMax = m_cchBuffer;
    lvitem.iItem = iItem;

    if (-1 != lvitem.iItem)
    {
        ListView_GetItem(hwndList, &lvitem);
    }    
}
