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

    CSnapin *m_RootSnapin;
    CSnapinAlias *m_RootAlias;

    void CreateSnapinAliases(CSnapinAlias *ParentAlias, CSnapin *ParentSnapin);
    void ReconnectSnapins(CSnapinAlias *ParentAlias, CSnapin *ParentSnapin);
    void DeleteSnapinAliasesRecursive(CSnapinAlias *ParentAlias, BOOL RemoveDeletedSnapins);
    void DeleteSnapinAliases(BOOL RemoveDeletedSnapins);

public:
    CAddDialog(CMainWnd *MainWnd, CConsoleWnd* console);
    ~CAddDialog();
    void InitLV(CListView& listView);
    void InsertListItem(CListView& listView, CSnapinCacheEntry *CacheEntry);
    void InitTV(CTreeView& treeView);
    HTREEITEM InsertTreeItem(CTreeView& treeView, CSnapinAlias *Alias, HTREEITEM hParent = NULL, HTREEITEM hInsertAfter = TVI_LAST);
    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnItemChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnSelectionChanged(INT uCode, LPNMHDR hdr, BOOL& bHandled);
    LRESULT OnItemDblClicked(INT uCode, LPNMHDR hdr, BOOL& bHandled);
    void UpdateButtons();
};
