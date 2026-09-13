/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin selection dialog
 * COPYRIGHT:   Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CAddDialog :
    public CDialogImpl<CAddDialog>
{
public:
    enum { IDD = IDD_DIALOG_ADD };

    BEGIN_MSG_MAP(CAddDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDCANCEL, OnCommand)
        COMMAND_ID_HANDLER(IDOK, OnCommand)
        COMMAND_ID_HANDLER(IDC_BUTTON_ADD, OnCommand)
        COMMAND_ID_HANDLER(IDC_BUTTON_REMOVE, OnCommand)

        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_CODE_HANDLER(TVN_SELCHANGED,  OnSelectionChanged)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnItemDblClicked)
    END_MSG_MAP()

private:
    CListView m_Available;
    CTreeView m_Selected;
    CWindow m_BtnAdd;
    CWindow m_BtnRemove;
    CWindow m_Description;

    CMainWnd *m_MainWnd;
    CComPtr<CConsoleWnd> m_Console;
    CAtlList<CSnapin*> m_Snapins;

public:
    CAddDialog(CMainWnd *MainWnd, CConsoleWnd* console)
    {
        m_MainWnd = MainWnd;
        m_Console = console;
    }

    ~CAddDialog()
    {
        m_Snapins.RemoveAll();
    }

    void InitLV(CListView& listView)
    {
        listView.DeleteAllItems();

        CAtlString module(MAKEINTRESOURCE(IDS_MODULE));
        listView.InsertColumn(0, (LPWSTR)module.GetString(), LVCFMT_LEFT, 130, 0);

        CAtlString vendor(MAKEINTRESOURCE(IDS_VENDOR));
        listView.InsertColumn(1, (LPWSTR)vendor.GetString(), LVCFMT_LEFT, 120, 1);

        listView.SetImageList(m_MainWnd->SnapinImageList(), LVSIL_SMALL);
    }

    void InsertListItem(CListView& listView, CSnapinCacheEntry *CacheEntry)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.lParam = (LPARAM)CacheEntry;
        lvi.pszText = (LPWSTR)CacheEntry->Name().GetString();
        lvi.iItem = INT_MAX;
        lvi.iImage = CacheEntry->NormalImageIndex();
        int item = listView.InsertItem(&lvi);

        listView.SetItemText(item, 1, (LPWSTR)CacheEntry->Provider().GetString());
    }

    void InitTV(CTreeView& treeView)
    {
        treeView.SetImageList(m_MainWnd->SnapinImageList(), TVSIL_NORMAL);
    }

    HTREEITEM InsertTreeItem(CTreeView& treeView, CSnapin *Snapin, HTREEITEM hInsertAfter)
    {
        TVINSERTSTRUCTW Insert;

        ZeroMemory(&Insert, sizeof(TVINSERTSTRUCTW));

        Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        Insert.item.pszText = (LPWSTR)Snapin->GetCacheEntry()->Name().GetString();
        Insert.item.iImage = Snapin->GetCacheEntry()->NormalImageIndex();
        Insert.item.iSelectedImage = Snapin->GetCacheEntry()->OpenImageIndex();
        Insert.item.lParam = (LPARAM)Snapin;

        Insert.hParent = NULL;
        Insert.hInsertAfter = hInsertAfter;

        return treeView.InsertItem(&Insert);
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CenterWindow(m_Console->m_hWnd);

        m_Available.Attach(GetDlgItem(IDC_LIST_AVAILABLE));
        m_Selected.Attach(GetDlgItem(IDC_LIST_SELECTED));
        m_BtnAdd.Attach(GetDlgItem(IDC_BUTTON_ADD));
        m_BtnRemove.Attach(GetDlgItem(IDC_BUTTON_REMOVE));
        m_Description.Attach(GetDlgItem(IDC_DESCRIPTION));

        InitLV(m_Available);
        InitTV(m_Selected);

        /* Get Snapins from the Cache */
        for (int i = 0; i < m_MainWnd->GetSnapinCacheCount(); i++)
        {
            InsertListItem(m_Available, m_MainWnd->GetSnapinCacheEntry(i));
        }

        UpdateButtons();

        return TRUE;
    }

    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        switch (wID)
        {
            case IDC_BUTTON_ADD:
            {
                int iItem = m_Available.GetNextItem(-1, LVNI_SELECTED);
                if (iItem != -1)
                {
                    CSnapinCacheEntry *CacheEntry = (CSnapinCacheEntry*)m_Available.GetItemData(iItem);
                    if (CacheEntry)
                    {
                        CSnapin *Snapin = new CSnapin(CacheEntry);
                        m_Snapins.AddTail(Snapin);
                        Snapin->OnAdd(m_Console);
                        InsertTreeItem(m_Selected, Snapin, NULL);
                    }
                }
            }
            break;

            case IDC_BUTTON_REMOVE:
            {
                HTREEITEM hTreeItem = m_Selected.GetSelection();
                if (hTreeItem != NULL)
                {
                    CSnapin *Snapin = (CSnapin *)m_Selected.GetItemData(hTreeItem);
                    if (Snapin)
                    {
                        m_Snapins.RemoveAt(m_Snapins.Find(Snapin));
                        m_Selected.DeleteItem(hTreeItem);
                        delete Snapin;
                    }
                }
            }
            break;

            case IDOK:
            {
#if 0
                int items = m_Selected.GetItemCount();
                for (int n = 0; n < items; ++n)
                {
                    CSnapin* snapin;
                    if ((snapin = (CSnapin*)m_Selected.GetItemData(n)))
                    {
                        snapin->OnAccept(m_Console);
                    }
                }
#endif
            }
            // save stuff
            case IDCANCEL:
                EndDialog(0);
                return 0;
        }

        UpdateButtons();

        return 0;
    }

    LRESULT OnItemChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        int iItem = m_Available.GetNextItem(-1, LVNI_SELECTED);
        CSnapinCacheEntry *CacheEntry;
        if (iItem != -1 && (CacheEntry = (CSnapinCacheEntry*)m_Available.GetItemData(iItem)))
        {
            m_Description.SetWindowText((LPWSTR)CacheEntry->Description().GetString());
        }
        else
        {
            m_Description.SetWindowText(NULL);
        }

        UpdateButtons();

        return TRUE;
    }

    LRESULT OnSelectionChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        UpdateButtons();
        return TRUE;
    }

    LRESULT OnItemDblClicked(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        if (hdr->hwndFrom == m_Available.m_hWnd)
        {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)hdr;
            if (lpnmitem->iItem != -1)
            {
                CSnapinCacheEntry* CacheEntry = (CSnapinCacheEntry*)m_Available.GetItemData(lpnmitem->iItem);
                if (CacheEntry)
                {
                    CSnapin *Snapin = new CSnapin(CacheEntry);
                    m_Snapins.AddTail(Snapin);
                    Snapin->OnAdd(m_Console);
                    InsertTreeItem(m_Selected, Snapin, NULL);
                }
            }
        }
        return TRUE;
    }

    void UpdateButtons()
    {
        m_BtnAdd.EnableWindow(ListView_GetSelectedCount(m_Available.m_hWnd) > 0);

        HTREEITEM hItem = m_Selected.GetNextItem(NULL, TVGN_CARET);
        m_BtnRemove.EnableWindow(hItem != NULL);
    }
};
