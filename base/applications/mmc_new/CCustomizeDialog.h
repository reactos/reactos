/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     View options dialog
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CCustomizeDialog :
    public CDialogImpl<CCustomizeDialog>
{
public:
    enum { IDD = IDD_CUSTOMIZE };

    BEGIN_MSG_MAP(CCustomizeDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDOK, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_SCOPEPANE, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_MENUS, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_TOOLBAR, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_STATUSBAR, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_DESCRIPTIONBAR, OnCommand)
        COMMAND_ID_HANDLER(IDC_CUSTOMIZE_ACTIONSPANE, OnCommand)

    END_MSG_MAP()

private:
    CMainWnd *m_MainWnd;
    CComPtr<CConsoleWnd> m_Console;

public:

    CCustomizeDialog(CMainWnd *MainWnd, CConsoleWnd* console)
    {
        m_MainWnd = MainWnd;
        m_Console = console;
    }

    ~CCustomizeDialog()
    {
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CenterWindow(m_Console->m_hWnd);

        CheckDlgButton(IDC_CUSTOMIZE_SCOPEPANE, m_Console->IsTreeViewVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_CUSTOMIZE_MENUS, m_MainWnd->AreStandardMenusVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_CUSTOMIZE_TOOLBAR, m_MainWnd->IsToolBarVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_CUSTOMIZE_STATUSBAR, m_Console->IsStatusBarVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_CUSTOMIZE_DESCRIPTIONBAR, m_Console->IsDescriptionBarVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_CUSTOMIZE_ACTIONSPANE, m_Console->IsActionsPaneVisible() ? BST_CHECKED : BST_UNCHECKED);

        ::EnableWindow(GetDlgItem(IDC_CUSTOMIZE_MENUS), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CUSTOMIZE_TABS), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CUSTOMIZE_SNAPIN_MENUS), FALSE);
        ::EnableWindow(GetDlgItem(IDC_CUSTOMIZE_SNAPIN_TOOLBARS), FALSE);

        return 0;
    }

    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        switch (wID)
        {
            case IDC_CUSTOMIZE_SCOPEPANE:
                m_Console->SetTreeViewVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_SCOPEPANE) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDC_CUSTOMIZE_MENUS:
                m_MainWnd->SetStandardMenusVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_MENUS) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDC_CUSTOMIZE_TOOLBAR:
                m_MainWnd->SetToolBarVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_TOOLBAR) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDC_CUSTOMIZE_STATUSBAR:
                m_Console->SetStatusBarVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_STATUSBAR) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDC_CUSTOMIZE_DESCRIPTIONBAR:
                m_Console->SetDescriptionBarVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_DESCRIPTIONBAR) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDC_CUSTOMIZE_ACTIONSPANE:
                m_Console->SetActionsPaneVisible((IsDlgButtonChecked(IDC_CUSTOMIZE_ACTIONSPANE) == BST_CHECKED) ? TRUE : FALSE);
                return 0;

            case IDOK:
                EndDialog(IDOK);
                return 0;
        }

        return 0;
    }
};
