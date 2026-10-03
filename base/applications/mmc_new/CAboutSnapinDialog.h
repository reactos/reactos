/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     About Snapin dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl
 */

#pragma once


class CAboutSnapinDialog :
    public CDialogImpl<CAboutSnapinDialog>
{
public:
    enum { IDD = IDD_ABOUT_SNAPIN };

    BEGIN_MSG_MAP(CAddAdvancedDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDOK, OnCommand)
    END_MSG_MAP()

private:
    CMainWnd* m_MainWnd;
    CSnapin *m_Snapin;

    CWindow m_SnapinIcon;
    CWindow m_SnapinInfo;
    CWindow m_Description;

public:

    CAboutSnapinDialog(CMainWnd* MainWnd, CSnapin *Snapin)
    {
        m_MainWnd = MainWnd;
        m_Snapin = Snapin;
    }

    ~CAboutSnapinDialog()
    {
    }

    LRESULT OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};
