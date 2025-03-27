/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Drive selection dialog
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "cleanmgr.h"

class CSelectDriveDlg : public CDialogImpl<CSelectDriveDlg>
{
public:
    enum { IDD = IDD_SELECTDRIVE };

    BEGIN_MSG_MAP(CSelectDriveDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnEndDialog)
        COMMAND_ID_HANDLER(IDCANCEL, OnEndDialog)
    END_MSG_MAP()

    CSelectDriveDlg()
        :m_SelectedDrive(UNICODE_NULL)
    {
    }

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
    {
        // Try to find an existing instance of this dialog
        WCHAR buf[300];
        GetWindowTextW(buf, _countof(buf));
        for (HWND hNext = NULL, hFind; (hFind = ::FindWindowExW(NULL, hNext, NULL, buf)) != NULL; hNext = hFind)
        {
            if (hFind != *this && ::IsWindowVisible(hFind))
            {
                ::SetForegroundWindow(hFind);
                EndDialog(IDCANCEL);
                return FALSE;
            }
        }

        CWindow cbo = GetDlgItem(IDC_DRIVES);
        WCHAR VolumeNameBuffer[MAX_PATH + 1];
        CStringW Tmp;
        for (WCHAR Drive = 'A'; Drive <= 'Z'; ++Drive)
        {
            WCHAR RootPathName[] = { Drive,':','\\',0 };
            UINT Type = GetDriveTypeW(RootPathName);
            if (Type == DRIVE_FIXED)
            {
                GetVolumeInformationW(RootPathName, VolumeNameBuffer, _countof(VolumeNameBuffer), 0, 0, 0, 0, 0);
                Tmp.Format(L"%s (%.2s)", VolumeNameBuffer, RootPathName);

                int index = (int)cbo.SendMessage(CB_ADDSTRING, NULL, (LPARAM)Tmp.GetString());
                cbo.SendMessage(CB_SETITEMDATA, index, Drive);
            }
        }
        cbo.SendMessage(CB_SETCURSEL, 0);
        return 1;
    }
    LRESULT OnEndDialog(WORD, WORD wID, HWND, BOOL&)
    {
        CWindow cbo = GetDlgItem(IDC_DRIVES);
        m_SelectedDrive = (WCHAR)cbo.SendMessage(CB_GETITEMDATA, cbo.SendMessage(CB_GETCURSEL));
        EndDialog(wID);
        return 0;
    }

    WCHAR m_SelectedDrive;
};


void
SelectDrive(WCHAR &Drive)
{
    CSelectDriveDlg dlgSelectDrive;
    if (dlgSelectDrive.DoModal() == IDOK)
    {
        Drive = dlgSelectDrive.m_SelectedDrive;
    }
}
