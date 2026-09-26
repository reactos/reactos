/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin selection dialog
 * COPYRIGHT:   Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

CAddDialog::CAddDialog(CMainWnd *MainWnd, CConsoleWnd* Console)
{
    m_MainWnd = MainWnd;
    m_Console = Console;
}

CAddDialog::~CAddDialog()
{
}

void
CAddDialog::InitLV(CListView& listView)
{
    listView.DeleteAllItems();

    CAtlString module(MAKEINTRESOURCE(IDS_MODULE));
    listView.InsertColumn(0, (LPWSTR)module.GetString(), LVCFMT_LEFT, 130, 0);

    CAtlString vendor(MAKEINTRESOURCE(IDS_VENDOR));
    listView.InsertColumn(1, (LPWSTR)vendor.GetString(), LVCFMT_LEFT, 120, 1);

    listView.SetImageList(m_MainWnd->SnapinImageList(), LVSIL_SMALL);
}

void
CAddDialog::InsertListItem(CListView& listView, CSnapinCacheEntry *CacheEntry)
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

void
CAddDialog::InitTV(CTreeView& treeView)
{
    treeView.SetImageList(m_MainWnd->SnapinImageList(), TVSIL_NORMAL);
}

HTREEITEM
CAddDialog::InsertTreeItem(CTreeView& treeView, CSnapinAlias *Alias, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    TVINSERTSTRUCTW Insert;

    ZeroMemory(&Insert, sizeof(TVINSERTSTRUCTW));

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.item.pszText = (LPWSTR)Alias->Snapin()->DisplayName().GetString();
    Insert.item.iImage = Alias->Snapin()->CacheEntry()->NormalImageIndex();
    Insert.item.iSelectedImage = Alias->Snapin()->CacheEntry()->OpenImageIndex();
    Insert.item.lParam = (LPARAM)Alias;

    Insert.hParent = hParent;
    Insert.hInsertAfter = hInsertAfter;

    return treeView.InsertItem(&Insert);
}

LRESULT
CAddDialog::OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

    /* Create the snapin alias tree */
    m_RootSnapin = m_MainWnd->GetRootSnapin();
    m_RootAlias = new CSnapinAlias(NULL, m_RootSnapin);

    CreateSnapinAliases(m_RootAlias, m_RootSnapin);

    m_RootAlias->hTreeItem = InsertTreeItem(m_Selected, m_RootAlias);
    m_Selected.Expand(m_RootAlias->hTreeItem, TVE_EXPAND);

    POSITION pos = m_RootAlias->m_SubNodes.GetHeadPosition();
    while (pos)
    {
        CSnapinAlias *Alias = (CSnapinAlias*)m_RootAlias->m_SubNodes.GetNext(pos);
        if (Alias)
        {
            Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, NULL /*m_RootAlias->hTreeItem*/);
            m_Selected.Expand(Alias->hTreeItem, TVE_EXPAND);
        }
    }

    UpdateButtons();

    return TRUE;
}

LRESULT
CAddDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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
                    CSnapinAlias *Alias = new CSnapinAlias(m_RootAlias, new CSnapin(CacheEntry));
                    m_RootAlias->m_SubNodes.AddTail(Alias);
//                    Alias->Snapin()->OnAdd(m_Console);
                    Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, NULL /*m_RootAlias->hTreeItem*/);
                    m_Selected.Expand(Alias->hTreeItem, TVE_EXPAND);
                }
            }
        }
        break;

        case IDC_BUTTON_REMOVE:
        {
            HTREEITEM hTreeItem = m_Selected.GetSelection();
            if (hTreeItem != NULL)
            {
                CSnapinAlias *Alias = (CSnapinAlias *)m_Selected.GetItemData(hTreeItem);
                if (Alias)
                {
                    Alias->Deleted();
                    m_Selected.DeleteItem(Alias->hTreeItem);
                }
            }
        }
        break;

        case IDOK:
        {
            ReconnectSnapins(m_RootAlias, m_RootSnapin);
            DeleteSnapinAliases(TRUE);
            EndDialog(IDOK);
        }
        return 0;

        case IDCANCEL:
        {
            DeleteSnapinAliases(FALSE);
            EndDialog(IDCANCEL);
        }
        return 0;
    }

    UpdateButtons();

    return 0;
}

LRESULT
CAddDialog::OnItemChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled)
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

LRESULT
CAddDialog::OnSelectionChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled)
{
    UpdateButtons();
    return TRUE;
}

LRESULT
CAddDialog::OnItemDblClicked(INT uCode, LPNMHDR hdr, BOOL& bHandled)
{
    if (hdr->hwndFrom == m_Available.m_hWnd)
    {
        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)hdr;
        if (lpnmitem->iItem != -1)
        {
            CSnapinCacheEntry* CacheEntry = (CSnapinCacheEntry*)m_Available.GetItemData(lpnmitem->iItem);
            if (CacheEntry)
            {
                CSnapinAlias *Alias = new CSnapinAlias(m_RootAlias, new CSnapin(CacheEntry));
                m_RootAlias->m_SubNodes.AddTail(Alias);
//                Alias->Snapin()->OnAdd(m_Console);
                Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, NULL /*m_RootAlias->hTreeItem*/);
                m_Selected.Expand(Alias->hTreeItem, TVE_EXPAND);
            }
        }
    }
    return TRUE;
}

void
CAddDialog::UpdateButtons()
{
    m_BtnAdd.EnableWindow(ListView_GetSelectedCount(m_Available.m_hWnd) > 0);

    CSnapinAlias *ParentAlias = NULL;
    HTREEITEM hTreeItem = m_Selected.GetNextItem(NULL, TVGN_CARET);
    if (hTreeItem)
    {
        CSnapinAlias *Alias = (CSnapinAlias *)m_Selected.GetItemData(hTreeItem);
        if (Alias)
            ParentAlias = Alias->Parent();
    }

    m_BtnRemove.EnableWindow((hTreeItem != NULL) && (ParentAlias != NULL));
}

void
CAddDialog::CreateSnapinAliases(CSnapinAlias *ParentAlias, CSnapin *ParentSnapin)
{
    POSITION pos = ParentSnapin->m_SubNodes.GetHeadPosition();
    while (pos)
    {
        CSnapin *Snapin = (CSnapin*)ParentSnapin->m_SubNodes.GetNext(pos);
        if (Snapin)
        {
            CSnapinAlias *Alias = new CSnapinAlias(ParentAlias, Snapin);
            ParentAlias->m_SubNodes.AddTail(Alias);
            CreateSnapinAliases(Alias, Snapin);
        }
    }
}

void
CAddDialog::ReconnectSnapins(CSnapinAlias *ParentAlias, CSnapin *ParentSnapin)
{
    ParentSnapin->m_SubNodes.RemoveAll();

    POSITION pos = ParentAlias->m_SubNodes.GetHeadPosition();
    while (pos)
    {
        CSnapinAlias *Alias = (CSnapinAlias*)ParentAlias->m_SubNodes.GetNext(pos);
        if (Alias)
        {
            if (!Alias->IsDeleted())
            {
                ParentSnapin->m_SubNodes.AddTail(Alias->Snapin());
                ReconnectSnapins(Alias, Alias->Snapin());
            }
        }
    }
}

void
CAddDialog::DeleteSnapinAliasesRecursive(CSnapinAlias *ParentAlias, BOOL RemoveDeletedSnapins)
{
    POSITION pos = ParentAlias->m_SubNodes.GetHeadPosition();
    while (pos)
    {
        CSnapinAlias *Alias = (CSnapinAlias*)ParentAlias->m_SubNodes.GetNext(pos);
        if (Alias)
        {
            DeleteSnapinAliasesRecursive(Alias, RemoveDeletedSnapins);
            Alias->m_SubNodes.RemoveAll();
            if (RemoveDeletedSnapins && (Alias->IsDeleted()))
                delete Alias->Snapin();
            delete Alias;
        }
    }
}

void
CAddDialog::DeleteSnapinAliases(BOOL RemoveDeletedSnapins)
{
    if (m_RootAlias)
    {
        DeleteSnapinAliasesRecursive(m_RootAlias, RemoveDeletedSnapins);
        m_RootAlias->m_SubNodes.RemoveAll();
        delete m_RootAlias;
        m_RootAlias = NULL;
    }
}
