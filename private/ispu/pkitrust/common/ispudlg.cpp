//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ispudlg.cpp
//
//  Contents:   Microsoft Internet Security Office Helper
//
//  History:    14-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "ispudlg.hxx"

INT_PTR CALLBACK UIMessageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProcessingDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

ISPUdlg_::ISPUdlg_(HWND hWndParent, HINSTANCE hInst, DWORD dwDialogId)
{
    m_dwDialogId        = dwDialogId;
    m_hInst             = hInst;
    m_hWndParent        = hWndParent;
    m_hWndMe            = NULL;
    m_hrResult          = E_NOTIMPL;
    m_hDlgProcessing    = NULL;
}

ISPUdlg_::~ISPUdlg_(void)
{
    if (m_hDlgProcessing)
    {
        DestroyWindow(m_hDlgProcessing);
    }
}

HRESULT ISPUdlg_::Invoke(void)
{

    if (DialogBoxParam(m_hInst, MAKEINTRESOURCE(m_dwDialogId), m_hWndParent,
                       UIMessageProc, (LPARAM)this) == (-1))
    {
        return(HRESULT_FROM_WIN32(GetLastError()));
    }

    return(m_hrResult);
}

void ISPUdlg_::ShowError(HWND hWnd, DWORD dwStringId, DWORD dwTitleId)
{
    char    szTitle[MAX_PATH + 1];
    char    szErr[MAX_PATH + 1];

    LoadStringA(m_hInst, dwTitleId, &szTitle[0], MAX_PATH);
    LoadStringA(m_hInst, dwStringId, &szErr[0], MAX_PATH);

    MessageBeep(MB_ICONEXCLAMATION);

    MessageBox((hWnd) ? hWnd : m_hWndParent, &szErr[0], &szTitle[0],
                MB_OK | MB_ICONERROR);
}

void ISPUdlg_::StartShowProcessing(DWORD dwDialogId, DWORD dwTextControlId, DWORD dwStringId)
{
    char    szText[MAX_PATH + 1];

    if (m_hDlgProcessing)
    {
        DestroyWindow(m_hDlgProcessing);
    }

    szText[0] = NULL;
    LoadStringA(m_hInst, dwStringId, &szText[0], MAX_PATH);

    m_hDlgProcessing = CreateDialog(m_hInst, MAKEINTRESOURCE(dwDialogId), m_hWndParent,
                                    ProcessingDialogProc);

    this->Center(m_hDlgProcessing);

    ShowWindow(m_hDlgProcessing, SW_SHOW);

    SetDlgItemText(m_hDlgProcessing, dwTextControlId, &szText[0]);
}

void ISPUdlg_::ChangeShowProcessing(DWORD dwTextControlId, DWORD dwStirngId)
{
    if (!(m_hDlgProcessing))
    {
        return;
    }

    char    szText[MAX_PATH + 1];

    szText[0] = NULL;
    LoadStringA(m_hInst, dwStirngId, &szText[0], MAX_PATH);

    SetDlgItemText(m_hDlgProcessing, dwTextControlId, &szText[0]);
}

void ISPUdlg_::EndShowProcessing(void)
{
    if (m_hDlgProcessing)
    {
        DestroyWindow(m_hDlgProcessing);
        m_hDlgProcessing = NULL;
    }
}


BOOL ISPUdlg_::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
                m_hWndMe = hWnd;

                this->Center();

                return(this->OnInitDialog(hWnd, wParam, lParam));

        case WM_COMMAND:
            return(this->OnCommand(hWnd, uMsg, wParam, lParam));

        case WM_CLOSE:
            return(this->OnCancel(hWnd));

        case WM_HELP:
            return(this->OnHelp(hWnd, wParam, lParam));

        default:
            return(FALSE);
    }

    return(TRUE);
}

BOOL ISPUdlg_::OnOK(HWND hWnd)
{
    EndDialog(hWnd, (int)m_hrResult);

    return(TRUE);
}

BOOL ISPUdlg_::OnCancel(HWND hWnd)
{
    EndDialog(hWnd, (int)m_hrResult);

    return(TRUE);
}

void ISPUdlg_::Center(HWND hWnd2Center)
{
    RECT    rcDlg;
    RECT    rcArea;
    RECT    rcCenter;
    HWND    hWndParent;
    HWND    hWndCenter;
    DWORD   dwStyle;
    int     w_Dlg;
    int     h_Dlg;
    int     xLeft;
    int     yTop;

    if (!(hWnd2Center))
    {
        hWnd2Center = m_hWndMe;
    }

    GetWindowRect(hWnd2Center, &rcDlg);

    dwStyle = (DWORD)GetWindowLong(hWnd2Center, GWL_STYLE);

    if (dwStyle & WS_CHILD)
    {
        hWndCenter = GetParent(hWnd2Center);

        hWndParent = GetParent(hWnd2Center);

        GetClientRect(hWndParent, &rcArea);
        GetClientRect(hWndCenter, &rcCenter);
        MapWindowPoints(hWndCenter, hWndParent, (POINT *)&rcCenter, 2);
    }
    else
    {
        hWndCenter = GetWindow(hWnd2Center, GW_OWNER);

        if (hWndCenter)
        {
            dwStyle = (DWORD)GetWindowLong(hWndCenter, GWL_STYLE);

            if (!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
            {
                hWndCenter = NULL;
            }
        }

        SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);

        if (hWndCenter)
        {
            GetWindowRect(hWndCenter, &rcCenter);
        }
        else
        {
            rcCenter = rcArea;
        }

    }

    w_Dlg   = rcDlg.right - rcDlg.left;
    h_Dlg   = rcDlg.bottom - rcDlg.top;

    xLeft   = (rcCenter.left + rcCenter.right) / 2 - w_Dlg / 2;
    yTop    = (rcCenter.top + rcCenter.bottom) / 2 - h_Dlg / 2;

    if (xLeft < rcArea.left)
    {
        xLeft = rcArea.left;
    }
    else if ((xLeft + w_Dlg) > rcArea.right)
    {
        xLeft = rcArea.right - w_Dlg;
    }

    if (yTop < rcArea.top)
    {
        yTop = rcArea.top;
    }
    else if ((yTop + h_Dlg) > rcArea.bottom)
    {
        yTop = rcArea.bottom - h_Dlg;
    }

    SetWindowPos(hWnd2Center, NULL, xLeft, yTop, -1, -1, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void ISPUdlg_::SetItemText(DWORD dwControlId, WCHAR *pwszText)
{
    DWORD   cbsz;
    char    *psz;

    cbsz = (DWORD)WideCharToMultiByte(0, 0, pwszText, wcslen(pwszText) + 1, NULL, 0, NULL, NULL);

    if (cbsz < 1)
    {
        return;
    }

    if (!(psz = new char[cbsz + 1]))
    {
        return;
    }
    psz[0] = NULL;

    WideCharToMultiByte(0, 0, pwszText, wcslen(pwszText) + 1, psz, cbsz, NULL, NULL);

    SetDlgItemText(m_hWndMe, (UINT)dwControlId, psz);

    delete psz;
}

BOOL ISPUdlg_::GetItemText(DWORD dwControlId, WCHAR **ppwszText)
{
    DWORD   cbsz;
    char    *psz;

    *ppwszText = NULL;

    cbsz = (DWORD)SendDlgItemMessage(m_hWndMe, (UINT)dwControlId, WM_GETTEXTLENGTH, 0, 0);

    if (cbsz < 1)
    {
        return(FALSE);
    }

    if (!(psz = new char[cbsz + 1]))
    {
        return(FALSE);
    }

    psz[0] = NULL;

    GetDlgItemText(m_hWndMe, (UINT)dwControlId, psz, cbsz + 1);

    if (!(*ppwszText = new WCHAR[cbsz + 1]))
    {
        delete psz;
        return(FALSE);
    }

    MultiByteToWideChar(0, 0, psz, -1, *ppwszText, cbsz + 1);

    delete psz;

    return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
////
////    local
////

INT_PTR CALLBACK UIMessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ISPUdlg_ *pUI;

    if (uMsg == WM_INITDIALOG)
    {
        pUI = (ISPUdlg_ *)lParam;

        SetWindowLongPtr(hWnd, DWLP_USER, (INT_PTR)lParam);
    }
    else
    {
        pUI = (ISPUdlg_ *)GetWindowLongPtr(hWnd, DWLP_USER);
    }

    if (!(pUI))
    {
        return(FALSE);
    }

    return(pUI->OnMessage(hWnd, uMsg, wParam, lParam));
}

INT_PTR CALLBACK ProcessingDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return(TRUE);
    }

    return(FALSE);
}

