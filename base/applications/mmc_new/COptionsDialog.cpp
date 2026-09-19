/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console Options dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

COptionsDialog::COptionsDialog(CMainWnd *MainWnd, CConsoleWnd* console)
{
    m_MainWnd = MainWnd;
    m_Console = console;
    m_DocumentMode = m_MainWnd->GetDocumentMode();
    m_LogicalReadOnly = m_MainWnd->GetLogicalReadOnly();
    m_PreventViewCustomization = m_MainWnd->GetPreventViewCustomization();
}

COptionsDialog::~COptionsDialog()
{
}

LRESULT
COptionsDialog::OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(m_Console->m_hWnd);

//        m_ConsoleIcon.Attach(GetDlgItem(IDC_CONSOLEICON));
    m_ConsoleName.Attach(GetDlgItem(IDC_CONSOLENAME));
    m_ConsoleSymbol.Attach(GetDlgItem(IDC_CONSOLESYMBOL));
    m_ConsoleMode.Attach(GetDlgItem(IDC_CONSOLEMODE));
    m_ConsoleDescription.Attach(GetDlgItem(IDC_CONSOLEDESCRIPTION));
    m_ConsoleNoSave.Attach(GetDlgItem(IDC_CONSOLENOSAVE));
    m_ConsoleCustom.Attach(GetDlgItem(IDC_CONSOLECUSTOM));

//        m_ConsoleIcon.SetIcon();

    m_ConsoleName.SetWindowText(m_MainWnd->GetConsoleTitle()->GetString());

    m_ConsoleSymbol.EnableWindow(FALSE);

    CAtlString AuthorMode(MAKEINTRESOURCE(IDS_AUTHORMODE));
    m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)AuthorMode.GetString());

    CAtlString UserModeFull(MAKEINTRESOURCE(IDS_USERMODE_FULL));
    m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeFull.GetString());

    CAtlString UserModeMultiple(MAKEINTRESOURCE(IDS_USERMODE_MULTIPLE));
    m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeMultiple.GetString());

    CAtlString UserModeSingle(MAKEINTRESOURCE(IDS_USERMODE_SINGLE));
    m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeSingle.GetString());

    m_ConsoleMode.SendMessage(CB_SETCURSEL, m_DocumentMode, 0);

    CAtlString ModeDescription(MAKEINTRESOURCE(IDS_AUTHORMODE_DESC + m_DocumentMode));
    m_ConsoleDescription.SetWindowText(ModeDescription.GetString());

    m_ConsoleNoSave.EnableWindow(m_DocumentMode != DocumentMode_Author);
    if (m_DocumentMode != DocumentMode_Author)
        CheckDlgButton(IDC_CONSOLENOSAVE, m_LogicalReadOnly ? BST_UNCHECKED : BST_CHECKED);

    m_ConsoleCustom.EnableWindow(m_DocumentMode != DocumentMode_Author);
    if (m_DocumentMode != DocumentMode_Author)
        CheckDlgButton(IDC_CONSOLECUSTOM, m_PreventViewCustomization ? BST_UNCHECKED : BST_CHECKED);

    return 0;
}

LRESULT
COptionsDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (wID)
    {
        case IDC_CONSOLEMODE:
            {
                m_DocumentMode = (DOCUMENT_MODE)m_ConsoleMode.SendMessage(CB_GETCURSEL, 0, 0);

                CAtlString ModeDescription(MAKEINTRESOURCE(IDS_AUTHORMODE_DESC + m_DocumentMode));
                m_ConsoleDescription.SetWindowText(ModeDescription.GetString());
                m_ConsoleNoSave.EnableWindow(m_DocumentMode != DocumentMode_Author);
                if (m_DocumentMode == DocumentMode_Author)
                    CheckDlgButton(IDC_CONSOLENOSAVE, BST_UNCHECKED);
                m_ConsoleCustom.EnableWindow(m_DocumentMode != DocumentMode_Author);
                if (m_DocumentMode == DocumentMode_Author)
                    CheckDlgButton(IDC_CONSOLECUSTOM, BST_UNCHECKED);
            }
            return 0;

        case IDOK:
            {
                CAtlString consoleTitle;
                m_ConsoleName.GetWindowText(consoleTitle);
                m_MainWnd->SetConsoleTitle(consoleTitle);

                m_MainWnd->SetDocumentMode(m_DocumentMode);
                if (m_DocumentMode == DocumentMode_Author)
                {
                    m_MainWnd->SetLogicalReadOnly(FALSE);
                    m_MainWnd->SetPreventViewCustomization(FALSE);
                }
                else
                {
                    m_MainWnd->SetLogicalReadOnly((IsDlgButtonChecked(IDC_CONSOLENOSAVE) == BST_CHECKED) ? FALSE : TRUE);
                    m_MainWnd->SetPreventViewCustomization((IsDlgButtonChecked(IDC_CONSOLECUSTOM) == BST_CHECKED) ? FALSE : TRUE);
                }

                EndDialog(IDOK);
            }
            return 0;

        case IDCANCEL:
            EndDialog(IDCANCEL);
            return 0;
    }

    return 0;
}
