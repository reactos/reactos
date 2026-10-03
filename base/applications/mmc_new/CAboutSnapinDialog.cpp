/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     About Snapin dialog
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include <precomp.h>


LRESULT
CAboutSnapinDialog::OnInitDialog(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CenterWindow(m_MainWnd->m_hWnd);

    m_SnapinIcon.Attach(GetDlgItem(IDC_ABOUT_ICON));
    m_SnapinInfo.Attach(GetDlgItem(IDC_ABOUT_INFO));
    m_Description.Attach(GetDlgItem(IDC_ABOUT_DESCRIPTION));

    m_SnapinIcon.SendMessage(STM_SETIMAGE, IMAGE_ICON, (LPARAM)m_Snapin->CacheEntry()->Icon());

    CAtlString Info = m_Snapin->Name();
    Info.Append(L"\r\n");
    Info.Append(m_Snapin->CacheEntry()->Provider().GetString());
    if (!m_Snapin->CacheEntry()->Version().IsEmpty())
    {
        CAtlString VersionString(MAKEINTRESOURCE(IDS_ABOUT_VERSION));

        Info.Append(L"\r\n");
        Info.Append(VersionString.GetString()/*L"Version: "*/);
        Info.Append(m_Snapin->CacheEntry()->Version().GetString());
    }
    m_SnapinInfo.SetWindowText(Info.GetString());

    m_Description.SetWindowText((LPWSTR)m_Snapin->CacheEntry()->Description().GetString());

    return 0;
}

LRESULT
CAboutSnapinDialog::OnClose(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EndDialog(IDOK);
    return 0;
}

LRESULT
CAboutSnapinDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (wID)
    {
        case IDOK:
        {
            EndDialog(IDOK);
        }
        return 0;
    }

    return 0;
}
