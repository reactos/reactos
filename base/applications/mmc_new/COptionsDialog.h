/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Console Options dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class COptionsDialog :
    public CDialogImpl<COptionsDialog>
{
public:
    enum { IDD = IDD_CONSOLEOPTIONS };

    BEGIN_MSG_MAP(COptionsDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDOK, OnCommand)
        COMMAND_ID_HANDLER(IDCANCEL, OnCommand)
        COMMAND_ID_HANDLER(IDC_CONSOLEMODE, OnCommand)

    END_MSG_MAP()

private:
    CWindow m_ConsoleIcon;
    CWindow m_ConsoleName;
    CWindow m_ConsoleSymbol;
    CWindow m_ConsoleMode;
    CWindow m_ConsoleDescription;
    CWindow m_ConsoleNoSave;
    CWindow m_ConsoleCustom;

    CMainWnd *m_MainWnd;
    CComPtr<CConsoleWnd> m_Console;

    DOCUMENT_MODE m_DocumentMode;
    BOOL m_LogicalReadOnly;
    BOOL m_PreventViewCustomization;

public:
    COptionsDialog(CMainWnd *MainWnd, CConsoleWnd* console);
    ~COptionsDialog();
    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};
