/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin selection advanced dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include <precomp.h>


LRESULT
CAddAdvancedDialog::OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(m_AddDialog->m_hWnd);
    CheckDlgButton(IDC_ADVANCED_CHECK, m_AddDialog->GetAdvanced() ? BST_CHECKED : BST_UNCHECKED);
    return 0;
}

LRESULT
CAddAdvancedDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (wID)
    {
        case IDOK:
        {
            m_AddDialog->SetAdvanced((IsDlgButtonChecked(IDC_ADVANCED_CHECK) == BST_CHECKED) ? TRUE : FALSE);
            EndDialog(IDOK);
        }
        return 0;

        case IDCANCEL:
        {
            EndDialog(IDCANCEL);
        }
        return 0;
    }

    return 0;
}
