/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin selection advanced dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once


class CAddAdvancedDialog :
    public CDialogImpl<CAddAdvancedDialog>
{
public:
    enum { IDD = IDD_ADVANCED };

    BEGIN_MSG_MAP(CAddAdvancedDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnCommand)
        COMMAND_ID_HANDLER(IDCANCEL, OnCommand)
    END_MSG_MAP()

private:
    CAddDialog *m_AddDialog;

public:

    CAddAdvancedDialog(CAddDialog *AddDialog)
    {
        m_AddDialog = AddDialog;
    }

    ~CAddAdvancedDialog()
    {
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};
