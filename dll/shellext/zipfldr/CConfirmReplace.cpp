/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Ask the user to replace a file
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

class CConfirmReplace : public CDialogImpl<CConfirmReplace>
{
private:
    CStringA m_Filename;
public:

    CConfirmReplace(const char* filename)
    {
        m_Filename = filename;
    }

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CenterWindow(GetParent());

        HICON hIcon = LoadIcon(NULL, IDI_EXCLAMATION);
        SendDlgItemMessage(IDC_EXCLAMATION_ICON, STM_SETICON, (WPARAM)hIcon);

        /* Our CString does not support FormatMessage yet */
        CStringA message(MAKEINTRESOURCE(IDS_OVERWRITEFILE_TEXT));
        CHeapPtr<CHAR, CLocalAllocator> formatted;

        DWORD_PTR args[2] =
        {
            (DWORD_PTR)m_Filename.GetString(),
            NULL
        };

        ::FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         message, 0, 0, (LPSTR)&formatted, 0, (va_list*)args);

        ::SetDlgItemTextA(m_hWnd, IDC_MESSAGE, formatted);
        return 0;
    }

    LRESULT OnButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

public:
    enum { IDD = IDD_CONFIRM_FILE_REPLACE };

    BEGIN_MSG_MAP(CConfirmReplace)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDYES, OnButton)
        COMMAND_ID_HANDLER(IDYESALL, OnButton)
        COMMAND_ID_HANDLER(IDNO, OnButton)
        COMMAND_ID_HANDLER(IDCANCEL, OnButton)
    END_MSG_MAP()
};


eZipConfirmResponse _CZipAskReplace(HWND hDlg, PCSTR FullPath)
{
    PCSTR Filename = PathFindFileNameA(FullPath);
    CConfirmReplace confirm(Filename);
    INT_PTR Result = confirm.DoModal(hDlg);
    switch (Result)
    {
    case IDYES: return eYes;
    case IDYESALL: return eYesToAll;
    default:
    case IDNO: return eNo;
    case IDCANCEL: return eCancel;
    }
}
