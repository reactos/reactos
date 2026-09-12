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

    CONSOLE_MODE m_consoleMode;

public:

    COptionsDialog(CMainWnd *MainWnd, CConsoleWnd* console)
    {
        m_MainWnd = MainWnd;
        m_Console = console;
        m_consoleMode = m_MainWnd->GetConsoleMode();
    }

    ~COptionsDialog()
    {
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

        CAtlString consoleTitle;
        m_Console->GetWindowText(consoleTitle);
        m_ConsoleName.SetWindowText(consoleTitle);

        m_ConsoleSymbol.EnableWindow(FALSE);

        CAtlString AuthorMode(MAKEINTRESOURCE(IDS_AUTHORMODE));
        m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)AuthorMode.GetString());

        CAtlString UserModeFull(MAKEINTRESOURCE(IDS_USERMODE_FULL));
        m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeFull.GetString());

        CAtlString UserModeMultiple(MAKEINTRESOURCE(IDS_USERMODE_MULTIPLE));
        m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeMultiple.GetString());

        CAtlString UserModeSingle(MAKEINTRESOURCE(IDS_USERMODE_SINGLE));
        m_ConsoleMode.SendMessage(CB_ADDSTRING, 0, (LPARAM)UserModeSingle.GetString());

        m_ConsoleMode.SendMessage(CB_SETCURSEL, m_consoleMode, 0);

        CAtlString ModeDescription(MAKEINTRESOURCE(IDS_AUTHORMODE_DESC + m_consoleMode));
        m_ConsoleDescription.SetWindowText(ModeDescription.GetString());

        m_ConsoleNoSave.EnableWindow(m_consoleMode != 0);
        m_ConsoleCustom.EnableWindow(m_consoleMode != 0);

        return 0;
    }

    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        switch (wID)
        {
            case IDC_CONSOLEMODE:
                {
                    m_consoleMode = (CONSOLE_MODE)m_ConsoleMode.SendMessage(CB_GETCURSEL, 0, 0);

                    CAtlString ModeDescription(MAKEINTRESOURCE(IDS_AUTHORMODE_DESC + m_consoleMode));
                    m_ConsoleDescription.SetWindowText(ModeDescription.GetString());
                    m_ConsoleNoSave.EnableWindow(m_consoleMode != 0);
                    m_ConsoleCustom.EnableWindow(m_consoleMode != 0);
                }
                return 0;

            case IDOK:
                {
                    CAtlString consoleTitle;
                    m_ConsoleName.GetWindowText(consoleTitle);
                    m_Console->SetWindowText(consoleTitle);

                    m_MainWnd->SetConsoleMode(m_consoleMode);
                    EndDialog(IDOK);
                }
                return 0;

            case IDCANCEL:
                EndDialog(IDCANCEL);
                return 0;
        }

        return 0;
    }
};
