/********************************************************
 secdlg.cpp

  User Manager security dialog implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "secdlg.h"

#include "misc.h"


INT_PTR CSecurityCheckDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CSecurityCheckDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CSecurityCheckDlg::OnInitDialog");
    BOOL fIsLocalAdmin;

    // First we must check if the current user is a local administrator; if this is
    // the case, our dialog doesn't even display
    
    if (SUCCEEDED(IsUserLocalAdmin(NULL, &fIsLocalAdmin)))
    {
        if (fIsLocalAdmin)
        {
            // We want to continue and launch the applet (don't display the security check dlg)
            EndDialog(hwnd, IDOK);
        }
    }
    else
    {
        TraceMsg("IsUserLocalAdmin failed");
        EndDialog(hwnd, IDCANCEL);
    }

    // Set the "can't launch User Options" message
    TCHAR szUsername[MAX_USER + 1];
    DWORD cchUsername = ARRAYSIZE(szUsername);

    TCHAR szDomain[MAX_DOMAIN + 1];
    DWORD cchDomain = ARRAYSIZE(szDomain);
    if (GetCurrentUserAndDomainName(szUsername, &cchUsername, szDomain,
        &cchDomain))
    {
        TCHAR szDomainAndUsername[MAX_DOMAIN + MAX_USER + 2];

        MakeDomainUserString(szDomain, szUsername, szDomainAndUsername, 
            ARRAYSIZE(szDomainAndUsername));

        TCHAR szMessage[256];

        if (FormatMessageString(IDS_USR_CANTRUNCPL_FORMAT, szMessage, ARRAYSIZE(szMessage), szDomainAndUsername))
        {
            SetWindowText(GetDlgItem(hwnd, IDC_CANTRUNCPL_STATIC), szMessage);
        }

        TCHAR szAdministrator[MAX_USER + 1];

        LoadString(g_hInstance, IDS_ADMINISTRATOR, szAdministrator,
            ARRAYSIZE(szAdministrator));

        SetWindowText(GetDlgItem(hwnd, IDC_USER), szAdministrator);

        TCHAR szMachine[MAX_COMPUTERNAME + 1];
        
        DWORD dwSize = ARRAYSIZE(szMachine);
        ::GetComputerName(szMachine, &dwSize);

        SetWindowText(GetDlgItem(hwnd, IDC_DOMAIN), szMachine);
    }

    // Limit the text in the edit fields
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    Edit_LimitText(hwndUsername, MAX_USER);

    HWND hwndDomain = GetDlgItem(hwnd, IDC_DOMAIN);
    Edit_LimitText(hwndDomain, MAX_DOMAIN);

    HWND hwndPassword = GetDlgItem(hwnd, IDC_PASSWORD);
    Edit_LimitText(hwndPassword, MAX_PASSWORD);

    if (!IsComputerInDomain())
    {
        // Don't need domain box
        EnableWindow(hwndDomain, FALSE);
        ShowWindow(hwndDomain, SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_DOMAIN_STATIC), SW_HIDE);

        // Move up the OK/Cancel buttons and text and shrink the dialog
        RECT rcDomain;
        GetWindowRect(hwndDomain, &rcDomain);

        RECT rcPassword;
        GetWindowRect(hwndPassword, &rcPassword);
        
        int dy = (rcPassword.top - rcDomain.top);
        // dy is negative 

        OffsetWindow(GetDlgItem(hwnd, IDOK), 0, dy);
        OffsetWindow(GetDlgItem(hwnd, IDCANCEL), 0, dy);
        OffsetWindow(GetDlgItem(hwnd, IDC_PASSWORD_STATIC), 0, dy);

        RECT rcDialog;
        GetWindowRect(hwnd, &rcDialog);

        rcDialog.bottom += dy;  

        MoveWindow(hwnd, rcDialog.left, rcDialog.top, rcDialog.right-rcDialog.left,
            rcDialog.bottom-rcDialog.top, FALSE);
    }

    TraceLeaveValue(TRUE);
}

BOOL CSecurityCheckDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CSecurityCheckDlg::OnCommand");
    BOOL fReturn = FALSE;

    switch (id)
    {
    case IDOK:
        if (SUCCEEDED(RelaunchAsUser(hwnd)))
        {
            EndDialog(hwnd, IDCANCEL);
        }
        fReturn = TRUE;
        break;

    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        fReturn = TRUE;
        break;
    }

    TraceLeaveValue(fReturn);
}

HRESULT CSecurityCheckDlg::RelaunchAsUser(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CSecurityCheckDlg::RelaunchAsUser");
    USES_CONVERSION;
    HRESULT hr = E_FAIL;

    TCHAR szUsername[MAX_USER + 1];
    FetchText(hwnd, IDC_USER, szUsername, ARRAYSIZE(szUsername));

    TCHAR szDomain[MAX_DOMAIN + 1];
    FetchText(hwnd, IDC_DOMAIN, szDomain, ARRAYSIZE(szDomain));

    // If the user didn't type a domain
    if (szDomain[0] == TEXT('\0'))
    {
        // Use this machine as the domain
        DWORD cchComputername = ARRAYSIZE(szDomain);
        ::GetComputerName(szDomain, &cchComputername);
    }

    TCHAR szPassword[MAX_PASSWORD + 1];
    GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), szPassword, ARRAYSIZE(szPassword));
    
    // Now relaunch ourselves with this information
    STARTUPINFO startupinfo = {0};
    startupinfo.cb = sizeof (startupinfo);

    WCHAR c_szCommandLineFormat[] = L"rundll32.exe netplwiz.dll,UsersRunDll %s";

    // Put the "real" user name in the command-line so that we know what user is
    // actually logged on to the machine even though we are re-launching in a different
    // user context
    WCHAR szCommandLine[ARRAYSIZE(c_szCommandLineFormat) + MAX_DOMAIN + MAX_USER + 2];
    wnsprintf(szCommandLine, ARRAYSIZE(szCommandLine), c_szCommandLineFormat, m_pszDomainUser);

    PROCESS_INFORMATION process_information;
    if (CreateProcessWithLogonW(szUsername, szDomain, szPassword, 0, NULL,
        szCommandLine, 0, NULL, NULL, &startupinfo, &process_information))
    {
        hr = S_OK;
        CloseHandle(process_information.hProcess);
        CloseHandle(process_information.hThread);
    }
    else
    {
        DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_USR_CANTOPENCPLASUSER_ERROR, 
            MB_OK | MB_ICONERROR);
    }
 
    TraceLeaveResult(hr);
}
