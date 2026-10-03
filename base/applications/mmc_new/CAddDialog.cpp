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
    m_Advanced = FALSE;
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

VOID
CAddDialog::InitCB(CWindow& comboBox)
{
    comboBox.SendMessage(CBEM_SETIMAGELIST, 0, (LPARAM)m_MainWnd->SnapinImageList());
}

VOID
CAddDialog::AppendComboBoxItem(CWindow& comboBox, CSnapinAlias *Alias, int iIndent)
{
    COMBOBOXEXITEMW Item;

    ZeroMemory(&Item, sizeof(COMBOBOXEXITEMW));

    Item.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT | CBEIF_LPARAM;
    Item.iItem = -1;
    Item.pszText = (LPWSTR)Alias->Snapin()->DisplayName().GetString();
    Item.cchTextMax = -1;
    Item.iImage = Alias->Snapin()->CacheEntry()->NormalImageIndex();
    Item.iSelectedImage = Alias->Snapin()->CacheEntry()->NormalImageIndex();
    Item.iIndent = iIndent;
    Item.lParam = (LPARAM)Alias;

    comboBox.SendMessage(CBEM_INSERTITEM, 0, (LPARAM)&Item);
}

VOID
CAddDialog::InsertSnapinAliasesRecursive(CSnapinAlias *ParentAlias, INT Indent)
{
    POSITION pos = ParentAlias->m_SubNodes.GetHeadPosition();
    while (pos)
    {
        CSnapinAlias *Alias = (CSnapinAlias*)ParentAlias->m_SubNodes.GetNext(pos);
        if (Alias)
        {
            Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, ParentAlias->hTreeItem);
            AppendComboBoxItem(m_ParentList, Alias, Indent);
            InsertSnapinAliasesRecursive(Alias, Indent + 1);
        }
    }
    m_Selected.Expand(ParentAlias->hTreeItem, TVE_EXPAND);
}


LRESULT
CAddDialog::OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(m_Console->m_hWnd);

    m_Available.Attach(GetDlgItem(IDC_ADD_LIST_AVAILABLE));
    m_Selected.Attach(GetDlgItem(IDC_ADD_LIST_SELECTED));
    m_BtnAdd.Attach(GetDlgItem(IDC_ADD_BUTTON_ADD));
    m_BtnRemove.Attach(GetDlgItem(IDC_ADD_BUTTON_REMOVE));
    m_BtnUp.Attach(GetDlgItem(IDC_ADD_BUTTON_UP));
    m_BtnDown.Attach(GetDlgItem(IDC_ADD_BUTTON_DOWN));
    m_Description.Attach(GetDlgItem(IDC_ADD_DESCRIPTION));
    m_SelectedText.Attach(GetDlgItem(IDC_ADD_TEXT_SELECTED));
    m_ParentText.Attach(GetDlgItem(IDC_ADD_TEXT_PARENT));
    m_ParentList.Attach(GetDlgItem(IDC_ADD_LIST_PARENT));

    m_ParentText.ShowWindow(SW_HIDE);
    m_ParentList.ShowWindow(SW_HIDE);

    InitLV(m_Available);
    InitTV(m_Selected);
    InitCB(m_ParentList);

    /* Get Snapins from the Cache */
    for (int i = 0; i < m_MainWnd->GetSnapinCacheCount(); i++)
    {
        InsertListItem(m_Available, m_MainWnd->GetSnapinCacheEntry(i));
    }

    /* Create the snapin alias tree */
    m_RootSnapin = m_MainWnd->GetRootSnapin();
    m_RootAlias = new CSnapinAlias(NULL, m_RootSnapin);
    m_ParentAlias = m_RootAlias;

    CreateSnapinAliases(m_RootAlias, m_RootSnapin);

    m_RootAlias->hTreeItem = InsertTreeItem(m_Selected, m_RootAlias, TVI_ROOT);

    AppendComboBoxItem(m_ParentList, m_RootAlias, 0);

    InsertSnapinAliasesRecursive(m_RootAlias, 1);
    m_Selected.Expand(m_RootAlias->hTreeItem, TVE_EXPAND);

    m_ParentList.SendMessage(CB_SETCURSEL, 0, 0);

    UpdateButtons();

    return TRUE;
}

LRESULT
CAddDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (wID)
    {
        case IDC_ADD_BUTTON_ADD:
        {
            int iItem = m_Available.GetNextItem(-1, LVNI_SELECTED);
            if (iItem != -1)
            {
                CSnapinCacheEntry *CacheEntry = (CSnapinCacheEntry*)m_Available.GetItemData(iItem);
                if (CacheEntry)
                {
                    CSnapinAlias *Alias = new CSnapinAlias(m_ParentAlias, new CSnapin(CacheEntry));
                    m_ParentAlias->m_SubNodes.AddTail(Alias);
//                    Alias->Snapin()->OnAdd(m_Console);
                    Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, m_ParentAlias->hTreeItem);
                    m_Selected.Expand(m_ParentAlias->hTreeItem, TVE_EXPAND);
                }
            }
        }
        break;

        case IDC_ADD_BUTTON_REMOVE:
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

        case IDC_ADD_BUTTON_UP:
        {
            HTREEITEM hSelectItem = m_Selected.GetSelection();
            if (hSelectItem != NULL)
            {
                HTREEITEM hParentItem = m_Selected.GetNextItem(hSelectItem, TVGN_PARENT);
                if (hParentItem != NULL)
                {
                    CSnapinAlias *ParentAlias = (CSnapinAlias *)m_Selected.GetItemData(hParentItem);
                    CSnapinAlias *Alias = (CSnapinAlias *)m_Selected.GetItemData(hSelectItem);
                    if (ParentAlias && Alias)
                    {
                        HTREEITEM hPrevItem = m_Selected.GetNextItem(hSelectItem, TVGN_PREVIOUS);
                        if (hPrevItem)
                        {
                            hPrevItem = m_Selected.GetNextItem(hPrevItem, TVGN_PREVIOUS);
                            if (hPrevItem == NULL)
                                hPrevItem = TVI_FIRST;

                            POSITION pos = ParentAlias->m_SubNodes.Find(Alias);
                            POSITION prev = pos;
                            ParentAlias->m_SubNodes.GetPrev(prev);
                            ParentAlias->m_SubNodes.SwapElements(pos, prev);

                            m_Selected.DeleteItem(hSelectItem);
                            Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, hParentItem, hPrevItem);
                            m_Selected.SelectItem(Alias->hTreeItem);
                        }
                    }
                }
            }
        }
        break;

        case IDC_ADD_BUTTON_DOWN:
        {
            HTREEITEM hSelectItem = m_Selected.GetSelection();
            if (hSelectItem != NULL)
            {
                HTREEITEM hParentItem = m_Selected.GetNextItem(hSelectItem, TVGN_PARENT);
                if (hParentItem != NULL)
                {
                    CSnapinAlias *ParentAlias = (CSnapinAlias *)m_Selected.GetItemData(hParentItem);
                    CSnapinAlias *Alias = (CSnapinAlias *)m_Selected.GetItemData(hSelectItem);
                    if (ParentAlias && Alias)
                    {
                        HTREEITEM hNextItem = m_Selected.GetNextItem(hSelectItem, TVGN_NEXT);
                        if (hNextItem)
                        {
                            POSITION pos = ParentAlias->m_SubNodes.Find(Alias);
                            POSITION next = pos;
                            ParentAlias->m_SubNodes.GetNext(next);
                            ParentAlias->m_SubNodes.SwapElements(pos, next);

                            m_Selected.DeleteItem(hSelectItem);
                            Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, hParentItem, hNextItem);
                            m_Selected.SelectItem(Alias->hTreeItem);
                        }
                    }
                }
            }
        }
        break;

        case IDC_ADD_ADVANCED:
        {
            CAddAdvancedDialog AdvancedDialog(this);
            if (AdvancedDialog.DoModal(m_hWnd, (LPARAM)0) == IDOK)
            {
                RECT rc;
                int Offset = 50;

//                Offset = HIWORD(::GetDialogBaseUnits()) * 20;

                ::GetWindowRect(m_Selected.m_hWnd, &rc);
                ::MapWindowPoints(HWND_DESKTOP, this->m_hWnd, (LPPOINT)&rc, 2);
                if (m_Advanced)
                    rc.top += Offset;
                else
                    rc.top -= Offset;

                ::MoveWindow(m_Selected.m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

                ::GetWindowRect(m_SelectedText.m_hWnd, &rc);
                ::MapWindowPoints(HWND_DESKTOP, this->m_hWnd, (LPPOINT)&rc, 2);
                if (m_Advanced)
                {
                    rc.top += Offset;
                    rc.bottom += Offset;
                }
                else
                {
                    rc.top -= Offset;
                    rc.bottom -= Offset;
                }

                ::MoveWindow(m_SelectedText.m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

                if (m_Advanced)
                {
                    m_ParentText.ShowWindow(SW_SHOW);
                    m_ParentList.ShowWindow(SW_SHOW);
                }
                else
                {
                    m_ParentText.ShowWindow(SW_HIDE);
                    m_ParentList.ShowWindow(SW_HIDE);
                }
            }
        }
        break;

        case IDC_ADD_LIST_PARENT:
        {
            INT iItem = SendMessage(m_ParentList, CB_GETCURSEL, 0, 0);
            if (iItem != CB_ERR)
            {
                m_ParentAlias = (CSnapinAlias*)SendMessage(m_ParentList, CB_GETITEMDATA, iItem, 0);
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
                CSnapinAlias *Alias = new CSnapinAlias(m_ParentAlias, new CSnapin(CacheEntry));
                m_ParentAlias->m_SubNodes.AddTail(Alias);
//                Alias->Snapin()->OnAdd(m_Console);
                Alias->hTreeItem = InsertTreeItem(m_Selected, Alias, m_ParentAlias->hTreeItem);
                m_Selected.Expand(m_ParentAlias->hTreeItem, TVE_EXPAND);
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

    HTREEITEM hPrevItem = NULL, hNextItem = NULL;
    HTREEITEM hSelectItem = m_Selected.GetSelection();
    if (hSelectItem)
    {
        hPrevItem = m_Selected.GetNextItem(hSelectItem, TVGN_PREVIOUS);
        hNextItem = m_Selected.GetNextItem(hSelectItem, TVGN_NEXT);
    }

    m_BtnUp.EnableWindow(hPrevItem != NULL);
    m_BtnDown.EnableWindow(hNextItem != NULL);
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

BOOL
CAddDialog::GetAdvanced()
{
    return m_Advanced;
}

VOID
CAddDialog::SetAdvanced(BOOL Advanced)
{
    m_Advanced = Advanced;
}
