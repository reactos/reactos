/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Ask the user for a password
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

class CZipPassword : public CDialogImpl<CZipPassword>
{
private:
    CStringW m_Filename;
    CStringA* m_pPassword;
public:
    CZipPassword(PCWSTR filename, CStringA* Password)
        :m_pPassword(Password)
    {
        if (filename != NULL)
            m_Filename = filename;
    }

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CenterWindow(GetParent());

        /* No filename, so this is the question before starting to extract */
        if (m_Filename.IsEmpty())
        {
            CStringW message(MAKEINTRESOURCEW(IDS_PASSWORD_ZIP_TEXT));
            ::SetDlgItemTextW(m_hWnd, IDC_MESSAGE, message);
            ::ShowWindow(GetDlgItem(IDSKIP), SW_HIDE);
        }
        else
        {
            CStringW message;
            message.FormatMessage(IDS_PASSWORD_FILE_TEXT, m_Filename.GetString());
            ::SetDlgItemTextW(m_hWnd, IDC_MESSAGE, message);
        }
        return TRUE;
    }

    LRESULT OnButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (wID == IDOK)
        {
            HWND item = GetDlgItem(IDC_PASSWORD);
            int len = ::GetWindowTextLengthA(item);
            len = ::GetDlgItemTextA(m_hWnd, IDC_PASSWORD, m_pPassword->GetBuffer(len+1), len+1);
            m_pPassword->ReleaseBuffer(len);
        }
        EndDialog(wID);
        return 0;
    }

public:
    enum { IDD = IDD_PASSWORD };

    BEGIN_MSG_MAP(CZipPassword)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnButton)
        COMMAND_ID_HANDLER(IDSKIP, OnButton)
        COMMAND_ID_HANDLER(IDCANCEL, OnButton)
    END_MSG_MAP()
};

eZipPasswordResponse _CZipAskPassword(HWND hDlg, PCWSTR filename, CStringA& Password)
{
    if (filename)
        filename = PathFindFileNameW(filename);
    CZipPassword password(filename, &Password);
    INT_PTR Result = password.DoModal(hDlg);
    switch (Result)
    {
    case IDOK: return eAccept;
    case IDSKIP: return eSkip;
    default:
    case IDCANCEL: return eAbort;
    }
}
