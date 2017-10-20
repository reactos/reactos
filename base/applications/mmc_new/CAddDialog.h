/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin selection dialog
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
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
    END_MSG_MAP()

private:
    CListView m_Available;
    CListView m_Selected;
    CWindow m_BtnAdd;
    CWindow m_BtnRemove;

    CComPtr<CConsoleWnd> m_Console;
    CSimpleArray<CSnapin> m_Snapins;

public:

    CAddDialog(CConsoleWnd* console)
    {
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
        listView.InsertColumn(0, (LPWSTR)module.GetString(), LVCFMT_LEFT, 120, 0);

        CAtlString vendor(MAKEINTRESOURCE(IDS_VENDOR));
        listView.InsertColumn(0, (LPWSTR)vendor.GetString(), LVCFMT_LEFT, 120, 1);
    }

    void InsertItem(CListView& listView, CSnapin* snapin)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.lParam = (LPARAM)snapin;
        lvi.pszText = (LPWSTR)snapin->Name().GetString();
        lvi.iItem = INT_MAX;
        lvi.iImage = 0;
        listView.InsertItem(&lvi);
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //m_pChildInfo = (PCONSOLE_CHILDFRM_WND)lParam;

        m_Available.Attach(GetDlgItem(IDC_LIST_AVAILABLE));
        m_Selected.Attach(GetDlgItem(IDC_LIST_SELECTED));
        m_BtnAdd.Attach(GetDlgItem(IDC_BUTTON_ADD));
        m_BtnRemove.Attach(GetDlgItem(IDC_BUTTON_REMOVE));

        ReadSnapins();

        InitLV(m_Available);
        InitLV(m_Selected);

        for (int n = 0; n < m_Snapins.GetSize(); ++n)
        {
            InsertItem(m_Available, &m_Snapins[n]);
        }

        UpdateButtons();
        return 0;
    }

    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        switch (wID)
        {
        case IDC_BUTTON_ADD:
        {
            //LVITEM lvi = { LVIF_PARAM };
            //lvi.iItem = ListView_GetNextItem(m_Available.m_hWnd, -1, LVNI_SELECTED);
            int iItem = m_Available.GetNextItem(-1, LVNI_SELECTED);
            CSnapin* snapin;
            if (iItem != -1 && (snapin = (CSnapin*)m_Available.GetItemData(iItem)))
            {
                snapin->OnAdd(m_Console)
                InsertItem(m_Selected, snapin);
            }
        }
        break;
        case IDC_BUTTON_REMOVE:
        {
            int iItem = m_Selected.GetNextItem(-1, LVNI_SELECTED);
            m_Selected.DeleteItem(iItem);
        }
        break;
        case IDOK:
        {
            int items = m_Selected.GetItemCount();
            for (int n = 0; n < items; ++n)
            {
                CSnapin* snapin;
                if ((snapin = (CSnapin*)m_Selected.GetItemData(n)))
                {
                    snapin->OnAccept(m_Console);
                }
            }
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
        UpdateButtons();
        return TRUE;
    }

    void UpdateButtons()
    {
        m_BtnAdd.EnableWindow(ListView_GetSelectedCount(m_Available.m_hWnd) > 0);
        m_BtnRemove.EnableWindow(ListView_GetSelectedCount(m_Selected.m_hWnd) > 0);
    }

    void ReadSnapins()
    {
        CRegKey snapins;
        if (ERROR_SUCCESS == snapins.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MMC\\SnapIns", KEY_READ))
        {
            WCHAR namebuf[MAX_PATH];
            DWORD index = 0, namelen = _countof(namebuf);
            while (ERROR_SUCCESS == snapins.EnumKey(index++, namebuf, &namelen))
            {
                CSnapin* snapin = CSnapin::Create(snapins, namebuf);
                if (snapin)
                {
                    m_Snapins.Add(*snapin);
                    delete snapin;
                }
                namelen = _countof(namebuf);
            }
        }
    }
};
