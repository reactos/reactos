#include "stdafx.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shsemip.h"

// Headers for the group list and group page base
#include "grpinfo.h"
#include "grppage.h"

#pragma hdrstop


//
// useful globals and macros used by everybody
//

DWORD g_dwWhichNet;
UINT g_uWizardIs = NAW_NETID; 

BOOL g_fRebootOnExit = FALSE;
BOOL g_fShownLastPage = FALSE;

// Stuff for creating a default autologon user
#define ITEMDATA_DEFAULTLOCALUSER   0xDEADBEEF

const WCHAR c_szAdminAccount[] = L"Administrator";


//
// The wizard pages we are adding
//

#define WIZDLG(name, dlgproc, dwFlags)   \
            { MAKEINTRESOURCE(IDD_PSW_##name##), dlgproc, MAKEINTRESOURCE(IDS_##name##), MAKEINTRESOURCE(IDS_##name##_SUB), dwFlags }

struct
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
}
pages[] =
{    
    WIZDLG(WELCOME,     _IntroDlgProc,       PSP_HIDEHEADER),
    WIZDLG(HOWUSE,      _HowUseDlgProc,      0),
    WIZDLG(WHICHNET,    _WhichNetDlgProc,    0),
    WIZDLG(DOMAININFO,  _DomainInfoDlgProc,  0),
    WIZDLG(USERINFO,    _UserInfoDlgProc,    0),
    WIZDLG(COMPINFO,    _CompInfoDlgProc,    0),
    WIZDLG(ADDUSER,     _AddUserDlgProc,     0),
    WIZDLG(PERMISSIONS, _PermissionsDlgProc, 0),
    WIZDLG(WORKGROUP,   _WorkgroupDlgProc,   0),
    WIZDLG(AUTOLOGON,   _AutoLogonDlgProc,   0),
    WIZDLG(DONE,        _DoneDlgProc,        PSP_HIDEHEADER), 
};

//
// Set the Wizard buttons for the dialog
//

void SetWizardButtons(HWND hwndPage, DWORD dwButtons)
{
    HWND hwndParent = GetParent(hwndPage);

    if ( g_uWizardIs != NAW_NETID )
    {
#if 0 
        EnableWindow(GetDlgItem(hwndParent,IDCANCEL),FALSE);
        ShowWindow(GetDlgItem(hwndParent,IDCANCEL),SW_HIDE);
#endif

        EnableWindow(GetDlgItem(hwndParent,IDHELP),FALSE);
        ShowWindow(GetDlgItem(hwndParent,IDHELP),SW_HIDE);
    }

    if ( g_fRebootOnExit ) 
    {
        TCHAR szBuffer[80];

        LoadString(GLOBAL_HINSTANCE, IDS_CLOSE, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hwndParent, IDCANCEL, szBuffer);
    }
    
    PropSheet_SetWizButtons(hwndParent, dwButtons);
}

// Utility fn's

// GetRegisteredOwner - retrieves the name of the registered owner
// of the machine
HRESULT GetRegisteredOwner(TCHAR* pszOwner, DWORD cchOwner)
{
    // REGKEY for Registered Owner (HKLM)
    static const TCHAR c_szRegOwnerKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
    static const TCHAR c_szRegOwnerVal[] = TEXT("RegisteredOwner");

    // Default to failure
    HRESULT hr = E_FAIL;
    LONG Err;
    
    HKEY hkey;
    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegOwnerKey, 0, KEY_QUERY_VALUE, &hkey);

    if (ERROR_SUCCESS == Err)
    {
        DWORD cbBuffer = cchOwner * sizeof(TCHAR);

        DWORD dwType;
        Err = RegQueryValueEx(hkey, c_szRegOwnerVal, 0, &dwType,
            (LPBYTE) pszOwner, &cbBuffer);

        if (ERROR_SUCCESS == Err)
        {
            hr = S_OK;
        }

        RegCloseKey(hkey);
    }

    return hr;
}

inline BOOL IsLegalNameCharacter(WCHAR ch)
{
    // We need to use illegal fat chars, says SBurns
    return (NULL == StrChr(ILLEGAL_FAT_CHARS, ch));
}

// Create a username from a source string by removing all non-alphabetic chars
HRESULT MungeUserName(TCHAR* pszSource, TCHAR* pszUsername, DWORD cchUsername)
{
    DWORD ichDest = 0;

    // Examine each char in the source string as long as there is room left
    // in the destination buffer for the next char and a null-terminator
    for (DWORD ichSrc = 0; ((TEXT('\0') != pszSource[ichSrc]) && (ichDest < (cchUsername-1))); ichSrc++)
    {
        TCHAR ch = pszSource[ichSrc];

        // Is character allowed?
        if (IsLegalNameCharacter(ch)) 
        {
            // it is, add it to the string
            pszUsername[ichDest ++] = ch;
        }
    }

    // Null-terminate
    if (cchUsername != 0)
    {
        pszUsername[ichDest] = TEXT('\0');

        // If we have room on the string for one more character before the null
        if (ichDest < (cchUsername-1))
        {
            // If the generated name matches the computer name
            TCHAR szComputer[MAX_COMPUTERNAME + 1];
            DWORD cchComputer = ARRAYSIZE(szComputer);

            if (GetComputerName(szComputer, &cchComputer))
            {
                if (0 == lstrcmpi(szComputer, pszUsername))
                {
                    // Yes, names are the same. Then we need to munge the name just a bit more so they are different
                    // Add a '1' to the end of the string
                    pszUsername[ichDest++] = TEXT('1');
                    pszUsername[ichDest] = TEXT('\0');
                }
            }    
        }
    }

    return (ichDest > 0) ? S_OK : E_FAIL;
}

HRESULT GetDefaultLocalUserName(TCHAR* szUser, DWORD cchUser)
{
    HRESULT hr = E_UNEXPECTED;

    // Name of the registered owner of this machine - we'll munge
    // a username from this
    TCHAR szRegisteredOwner[256];

    hr = GetRegisteredOwner(szRegisteredOwner, ARRAYSIZE(szRegisteredOwner));

    if (SUCCEEDED(hr))
    {
        // Allowing for only alphabetic characters, turn the registered
        // owners name into a username of no more than MAX_USER chars
        hr = MungeUserName(szRegisteredOwner, szUser, cchUser);
    }

    return hr;
}

BOOL GetAdministratorsGroupName(TCHAR* pszAdmins, DWORD cchAdmins)
{
    PSID psid;
    SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
    BOOL fSuccess = AllocateAndInitializeSid(
        &auth,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,
        &psid
        );

    if (fSuccess)
    {
        TCHAR szDomain[DNLEN + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);
        SID_NAME_USE sUse;

        fSuccess = LookupAccountSid(NULL, psid, pszAdmins, &cchAdmins, szDomain, &cchDomain, &sUse);

        FreeSid(psid);
    }

    return fSuccess;
}

// CreateLocalAdminUser - creates a local user account, puts this
// user into the administrators group.
HRESULT CreateLocalAdminUser(HWND hwnd, LPTSTR szUser, LPTSTR szPassword)
{
    HRESULT hr;

    // Create a user account
    // *********************
#ifdef DONT_JOIN
    hr = S_OK;
#else // !DONT_JOIN
    // Assume failure
    hr = E_FAIL;

    // Try to add this user to the system and put them in the administrators group
    USER_INFO_1 usri1;
    usri1.usri1_name = szUser;
    usri1.usri1_password = szPassword;
    usri1.usri1_password_age = 0;
    usri1.usri1_priv = USER_PRIV_USER;
    usri1.usri1_home_dir = L"";
    usri1.usri1_comment = L"";
    usri1.usri1_flags  = UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT;
    usri1.usri1_script_path = NULL;

    NET_API_STATUS status = NetUserAdd(NULL, 1, (LPBYTE) &usri1, NULL);

    if (NERR_Success == status)
    {
        // Set the user's full name (easier than using USER_INFO_2)
        USER_INFO_1011 usri1011;

        TCHAR szRegisteredOwner[256];

        if (SUCCEEDED(GetRegisteredOwner(szRegisteredOwner, ARRAYSIZE(szRegisteredOwner))))
        {
            usri1011.usri1011_full_name = szRegisteredOwner;
            NetUserSetInfo(NULL, szUser, 1011, (LPBYTE) &usri1011, NULL);
        }

        // Add to administrators group
        // ***************************

	TCHAR szAdmins[GNLEN + 1];
        if (GetAdministratorsGroupName(szAdmins, ARRAYSIZE(szAdmins)))
        {
            LOCALGROUP_MEMBERS_INFO_3 lgrmi3;
            lgrmi3.lgrmi3_domainandname = szUser;

            status = NetLocalGroupAddMembers(NULL, szAdmins, 3, 
                (LPBYTE) &lgrmi3, 1);

            if (NERR_Success != status)
            {
                // Failure - Roll-back
                // *******************

                // Delete this user since we couldn't add
                // them to administrators
                NetUserDel(NULL, szUser);
            }
        }
    }

    // Did we succeed? If not, show an error
    switch ( status )
    {
        // Success conditions
        case NERR_Success:
        case ERROR_MEMBER_IN_GROUP:
        case ERROR_MEMBER_IN_ALIAS:
        {
            hr = S_OK;
            break;
        }
        default:
        {
            // Unexpected error
            TCHAR szMessage[512];
            DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) status, 0, szMessage, ARRAYSIZE(szMessage), NULL);

            if (0 == dwFormatResult)
            {
                LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
            }

            ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_ERR_ADDUSER, MB_OK|MB_ICONERROR, szMessage);

            hr = E_FAIL;

            break;
        }
    }

#endif //DONT_JOIN

    return hr;
}

/*-----------------------------------------------------------------------------
/ Intro page to the wizard.
/----------------------------------------------------------------------------*/

INT_PTR CALLBACK _IntroDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT);
                    return TRUE;              

                case PSN_WIZNEXT:
                {
                    switch ( g_uWizardIs )
                    {
                    case NAW_PSDOMAINJOINED:
                        WizardNext(hwnd, IDD_PSW_ADDUSER);
                        break;
                    case NAW_PSWORKGROUP:
                        WizardNext(hwnd, IDD_PSW_AUTOLOGON);
                        break;
                    default:
                        // Let the wizard go to the next page
                        break;
                    }
                        

                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}                                    


/*-----------------------------------------------------------------------------
/ Get the usage of the computer: corp vs home
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK _HowUseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CheckRadioButton(hwnd, IDC_NETWORKED, IDC_NOTNETWORKED, IDC_NETWORKED);
            return TRUE;
    
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                    WizardNext(hwnd, IDD_PSW_WELCOME);
                    return TRUE;

                case PSN_WIZNEXT:
                {                    
                    if ( IsDlgButtonChecked(hwnd, IDC_NETWORKED) == BST_CHECKED )
                    {
                        WizardNext(hwnd, IDD_PSW_WHICHNET);
                    }
                    else
                    {
                        g_dwWhichNet = IDC_NONE;

                        if ( SUCCEEDED(JoinDomain(hwnd, FALSE, DEFAULT_WORKGROUP, NULL)) )
                            WizardNext(hwnd, IDD_PSW_AUTOLOGON);
                        else
                            WizardNext(hwnd, -1);
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}



/*-----------------------------------------------------------------------------
/ Determine what sort of networking the user wants to use (domain, workgroup
/ or none).
/
/----------------------------------------------------------------------------*/

INT_PTR CALLBACK _WhichNetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CheckRadioButton(hwnd, IDC_DOMAIN, IDC_WORKGROUP, IDC_DOMAIN);
            return TRUE;
    
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                    WizardNext(hwnd, IDD_PSW_HOWUSE);
                    return TRUE;

                case PSN_WIZNEXT:
                {                    
                    if ( IsDlgButtonChecked(hwnd, IDC_DOMAIN) == BST_CHECKED )
                    {
                        g_dwWhichNet = IDC_DOMAIN;
                        WizardNext(hwnd, IDD_PSW_DOMAININFO);
                    }
                    else
                    {
                        g_dwWhichNet = IDC_WORKGROUP;
                        WizardNext(hwnd, IDD_PSW_WORKGROUP);
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ Allow the user to change the workgroup name.
/----------------------------------------------------------------------------*/

BOOL _WorkgroupBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;

    if ( !FetchTextLength(hwnd, IDC_WORKGROUP) )
        dwButtons &= ~PSWIZB_NEXT;

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;
}

INT_PTR CALLBACK _WorkgroupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            Edit_LimitText(GetDlgItem(hwnd, IDC_WORKGROUP), MAX_COMPUTERNAME);
            SetDlgItemText(hwnd, IDC_WORKGROUP, DEFAULT_WORKGROUP);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    return _WorkgroupBtnState(hwnd);

                case PSN_WIZBACK:
                    WizardNext(hwnd, IDD_PSW_WHICHNET);
                    return TRUE;

                case PSN_WIZNEXT:
                {
                    WCHAR szWorkgroup[MAX_WORKGROUP+1];

                    FetchText(hwnd, IDC_WORKGROUP, szWorkgroup, ARRAYSIZE(szWorkgroup));

                    if ( SUCCEEDED(JoinDomain(hwnd, FALSE, szWorkgroup, NULL)) )
                    {
                        ClearAutoLogon();
                        WizardNext(hwnd, IDD_PSW_DONE);
                    }
                    else
                    {
                        WizardNext(hwnd, -1);
                    }

                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            if ( HIWORD(wParam) == EN_CHANGE )
                return _WorkgroupBtnState(hwnd);

            break;
        }
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ Handle the auto logon page
/----------------------------------------------------------------------------*/

VOID _SetAutoLogonCtrls(HWND hwnd)
{
    BOOL fDisable = IsDlgButtonChecked(hwnd, IDC_NOAUTOLOGON) == BST_CHECKED;

    EnableWindow(GetDlgItem(hwnd, IDC_USERS), !fDisable);
    EnableWindow(GetDlgItem(hwnd, IDC_USERSLABEL), !fDisable);
    EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), !fDisable);
    EnableWindow(GetDlgItem(hwnd, IDC_PASSWORDLABEL), !fDisable);
    EnableWindow(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), !fDisable);
    EnableWindow(GetDlgItem(hwnd, IDC_CONFIRMPASSWORDLABEL), !fDisable);
}

BOOL _InitAutoLogonDlg(HWND hwnd)
{
    HWND hwndUsers = GetDlgItem(hwnd, IDC_USERS);
    NET_API_STATUS status;
    USER_INFO_1 *pbuff;
    DWORD dwResume = 0;
    DWORD dwCount, dwEntries;

    //
    // set the auto logon state
    //

    CheckRadioButton(hwnd, IDC_NOAUTOLOGON, IDC_AUTOLOGON, (g_uWizardIs == NAW_PSWORKGROUP) ? IDC_AUTOLOGON : IDC_NOAUTOLOGON);
    _SetAutoLogonCtrls(hwnd);


    // 
    // enumerate the local users of this machine and add them to the 
    // combo box.
    //

    do 
    {
        status = NetUserEnum(NULL, 1, FILTER_NORMAL_ACCOUNT, (LPBYTE *)&pbuff, 8192, &dwCount, &dwEntries, &dwResume);
        if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
        {
            DWORD i;

            for ( i = 0 ; i < dwCount ; i++ )
            {
                if ( !(pbuff[i].usri1_flags & UF_ACCOUNTDISABLE) )
                    ComboBox_AddString(hwndUsers, pbuff[i].usri1_name);
            }

            NetApiBufferFree(pbuff);
        }
    }
    while ( dwCount != dwEntries );

    // Select the first item, although we may override this with the default local user in a sec.
    ComboBox_SetCurSel(hwndUsers, 0);

    // If required, add the special default local user entry
    if (g_uWizardIs == NAW_PSWORKGROUP)
    {
        // Add a special default local user string - user isn't actually created
        // until Next is clicked.
        TCHAR szUser[MAX_USER + 1];
        if (SUCCEEDED(GetDefaultLocalUserName(szUser, ARRAYSIZE(szUser))))
        {
            // Don't add this user if he already exists. This is a weird situation since
            // we only expect to run at first boot, but handle it anyway.
            int iItem = ComboBox_FindStringExact(hwndUsers, -1, szUser);
            
            if (iItem == CB_ERR)
            {
                iItem = ComboBox_AddString(hwndUsers, szUser);

                if (iItem != CB_ERR)
                {
                    // Mark this item as the default local user guy.
                    ComboBox_SetItemData(hwndUsers, iItem, ITEMDATA_DEFAULTLOCALUSER);
                }
            }

            if (iItem != CB_ERR)
            {
                // Select it, its the default!
                ComboBox_SetCurSel(hwndUsers, iItem);
            }
        }
    }

    //
    // limit the length of the password fields accordingly
    //

    Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), PWLEN);
    Edit_LimitText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), PWLEN);

    
    // 
    // enable/disable buttons accordingly
    //
    _SetAutoLogonCtrls(hwnd);

    return TRUE;
}

INT_PTR CALLBACK _AutoLogonDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return _InitAutoLogonDlg(hwnd);

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDC_AUTOLOGON:
                case IDC_NOAUTOLOGON:
                    _SetAutoLogonCtrls(hwnd);
                    break;
            } 
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_QUERYINITIALFOCUS:
                    // Set focus to the password control if we are going to try to do the autologon thing
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 
                        (LONG_PTR) ((g_uWizardIs == NAW_PSWORKGROUP) ? GetDlgItem(hwnd, IDC_PASSWORD) : 0)
                        );

                    return TRUE;

                case PSN_SETACTIVE:            
                    SetWizardButtons(hwnd, PSWIZB_BACK|PSWIZB_NEXT);
                    return TRUE;              

                case PSN_WIZBACK:
                {
                    switch ( g_dwWhichNet )
                    {
                        case IDC_WORKGROUP:
                            WizardNext(hwnd, IDD_PSW_WORKGROUP);
                            break;

                        case IDC_NONE:
                            WizardNext(hwnd, IDD_PSW_HOWUSE);
                            break;
                    }
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    WCHAR szUser[MAX_USER + 1] = { 0 };
                    WCHAR szPassword[MAX_PASSWORD + 1] = { 0 };
                    WCHAR szConfirmPwd[MAX_PASSWORD + 1] = { 0 };

                    if ( IsDlgButtonChecked(hwnd, IDC_NOAUTOLOGON) == BST_CHECKED )
                    {
                        //
                        // clear the auto admin logon
                        //

                        ClearAutoLogon();
                    }
                    else
                    {
                        //
                        // read the two passwords, check they match, if then don't tell
                        // the user clear them and prompt again.  if they do then
                        // we can continue.
                        //

                        HWND hwndUsers = GetDlgItem(hwnd, IDC_USERS);

                        ComboBox_GetText(hwndUsers, szUser, ARRAYSIZE(szUser));
                        FetchText(hwnd, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));
                        FetchText(hwnd, IDC_CONFIRMPASSWORD, szConfirmPwd, ARRAYSIZE(szConfirmPwd));
                
                        if ( StrCmpW(szPassword, szConfirmPwd) )
                        {
                            ShellMessageBox(GLOBAL_HINSTANCE, hwnd,
                                            MAKEINTRESOURCE(IDS_ERR_PWDNOMATCH),
                                            MAKEINTRESOURCE(IDS_AUTOLOGON),
                                            MB_OK|MB_ICONWARNING);

                            SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
                            SetDlgItemText(hwnd, IDC_PASSWORD, L"");
                            SetDlgItemText(hwnd, IDC_CONFIRMPASSWORD, L"");

                            WizardNext(hwnd, -1);
                        }
                        else
                        {
                            // The passwords match - create the default local user if appropriate
                            
                            // Is this the default boy?
                            HRESULT hr = S_OK;
                            int iItem = ComboBox_GetCurSel(hwndUsers);

                            if (iItem != CB_ERR)
                            {
                                if (ITEMDATA_DEFAULTLOCALUSER == ComboBox_GetItemData(hwndUsers, iItem))
                                {
                                    hr = CreateLocalAdminUser(hwnd, szUser, szPassword);
                                }
                            }

                            if (SUCCEEDED(hr))
                            {
                                SetAutoLogon(szUser, szPassword);

                                WizardNext(hwnd, IDD_PSW_DONE);
                            }
                            else
                            {
                                WizardNext(hwnd, -1);
                            }
                        }
                    }
                                       
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ The wizard is complete, so lets allow them to back up if they want,
/ otherwise "Next" becomes "Finish" and we exit.
/----------------------------------------------------------------------------*/

INT_PTR CALLBACK _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    TCHAR szBuffer[MAX_PATH];

                    //
                    // change the closing prompt if we are supposed to be
                    //

                    LoadString(GLOBAL_HINSTANCE, 
                               g_fRebootOnExit ? IDS_NETWIZFINISHREBOOT:IDS_NETWIZFINISH, 
                               szBuffer, ARRAYSIZE(szBuffer));
            
                    SetDlgItemText(hwnd, IDC_FINISHSTATIC, szBuffer);
                    SetWizardButtons(hwnd, PSWIZB_BACK|PSWIZB_FINISH);

                    g_fShownLastPage = TRUE;                    // show the last page of the wizard

                    return TRUE;
                }

                case PSN_WIZBACK:
                {
                    switch ( g_dwWhichNet )
                    {
                        case IDC_DOMAIN:
                            WizardNext(hwnd, IDD_PSW_ADDUSER);
                            break;

                        case IDC_WORKGROUP:
                            WizardNext(hwnd, IDD_PSW_WORKGROUP);
                            break;

                        case IDC_NONE:
                            WizardNext(hwnd, IDD_PSW_AUTOLOGON);
                            break;
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ Main entry point used to invoke the wizard.
/----------------------------------------------------------------------------*/

static WNDPROC _oldDlgWndProc;

LRESULT CALLBACK _WizardSubWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // on WM_WINDOWPOSCHANGING and the window is moving then lets centre it onto the
    // desktop window.  unfortunately setting the DS_CENTER bit doesn't buy us anything
    // as the wizard is resized after creation.
    //

    if ( uMsg == WM_WINDOWPOSCHANGING )
    {
        LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
        RECT rcDlg, rcDesktop;

        GetWindowRect(hwnd, &rcDlg);
        GetWindowRect(GetDesktopWindow(), &rcDesktop);

        lpwp->x = ((rcDesktop.right-rcDesktop.left)-(rcDlg.right-rcDlg.left))/2;
        lpwp->y = ((rcDesktop.bottom-rcDesktop.top)-(rcDlg.bottom-rcDlg.top))/2;
        lpwp->flags &= ~SWP_NOMOVE;
    }

    return _oldDlgWndProc(hwnd, uMsg, wParam, lParam);        
}

int CALLBACK _PropSheetCB(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    switch ( uMsg )
    {
        //
        // in pre-create lets set the window styles accorindlgy
        //      - remove the context menu and system menu
        //

        case PSCB_PRECREATE:
        {
            DLGTEMPLATE *pdlgtmp = (DLGTEMPLATE*)lParam;
            pdlgtmp->style &= ~(DS_CONTEXTHELP|WS_SYSMENU);
            break;
        }

        //
        // we now have a dialog, so lets sub class it so we can stop it being
        // move around.
        //

        case PSCB_INITIALIZED:
        {
            if ( g_uWizardIs != NAW_NETID )
                _oldDlgWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)_WizardSubWndProc);

            break;
        }
    }

    return FALSE;
}

// The global group page base object used by the domain wizard
// defined in domain.cpp
extern CGroupPageBase* g_pGroupPageBase;

STDAPI NetAccessWizard(HWND hwnd, UINT uType, BOOL *pfReboot)
{
    PROPSHEETHEADER psh = { 0 };
    HPROPSHEETPAGE rghpage[ARRAYSIZE(pages)];
    INT cPages = 0;
    INITCOMMONCONTROLSEX iccex = { 0 };

    iccex.dwSize = sizeof (iccex);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccex);

    //
    // decode the type and set our state accordingly
    //

    switch ( uType )
    {
        case NAW_NETID:
            break;

        case NAW_PSWORKGROUP:
        case NAW_PSDOMAINJOINFAILED:
            g_dwWhichNet = IDC_NONE;
            g_uWizardIs = uType;
            break;

        case NAW_PSDOMAINJOINED:
            g_dwWhichNet = IDC_DOMAIN;
            g_uWizardIs = uType;
            break;

        default:
            return E_INVALIDARG;
    }

    //
    // build the pages for the wizard.
    //

    for ( cPages = 0 ; cPages < ARRAYSIZE(pages) ; cPages++ )
    {                           
        PROPSHEETPAGE psp = { 0 };
        WCHAR szBuffer[MAX_PATH] = { 0 };

        psp.dwSize = SIZEOF(PROPSHEETPAGE);
        psp.hInstance = GLOBAL_HINSTANCE;
        psp.lParam = cPages;
        psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | 
                            PSP_USEHEADERSUBTITLE | pages[cPages].dwFlags;
        psp.pszTemplate = pages[cPages].idPage;
        psp.pfnDlgProc = pages[cPages].pDlgProc;
        psp.pszTitle = MAKEINTRESOURCE(IDS_NETWIZCAPTION);
        psp.pszHeaderTitle = pages[cPages].pHeading;
        psp.pszHeaderSubTitle = pages[cPages].pSubHeading;

        rghpage[cPages] = CreatePropertySheetPage(&psp);
    }

    //
    // wizard pages are ready, so lets display the wizard.
    //

    psh.dwSize = SIZEOF(PROPSHEETHEADER);
    psh.hwndParent = hwnd;
    psh.hInstance = GLOBAL_HINSTANCE;
    psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | 
                            PSH_STRETCHWATERMARK | PSH_HEADER | PSH_USECALLBACK;
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_PSW_BANNER);
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_PSW_WATERMARK);
    psh.nPages = cPages;
    psh.phpage = rghpage;
    psh.pfnCallback = _PropSheetCB;

    // Create the global CGroupPageBase object if necessary
    CGroupInfoList grouplist;
    if (SUCCEEDED(grouplist.Initialize()))
    {
        g_pGroupPageBase = new CGroupPageBase(NULL, &grouplist);

        if (NULL != g_pGroupPageBase)
        {
            PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_PSW));
            delete g_pGroupPageBase;
        }
    }

    //
    // Hang up the all RAS connections if the wizard created one. It is assumed that no non-wizard connections will
    // exist at this time. 90% of the time, they've just changed their domain membership anyway to they will
    // be just about to reboot. Hanging up all connections MAY cause trouble if: There were existing connections
    // before the pre-logon wizard started AND the user cancelled after making connections with the wizard but before
    // changing their domain. There are no situations where this currently happens.
    //

    if (g_fWizardCreatedRASConnection)
    {
        RASCONN* prgrasconn = (RASCONN*) LocalAlloc(0, sizeof(RASCONN));

        if (NULL != prgrasconn)
        {
            prgrasconn[0].dwSize = sizeof(RASCONN);

            DWORD cb = sizeof(RASCONN);
            DWORD nConn = 0;

            DWORD dwSuccess = RasEnumConnections(prgrasconn, &cb, &nConn);

            if (ERROR_BUFFER_TOO_SMALL == dwSuccess)
            {
                LocalFree(prgrasconn);
                prgrasconn = (RASCONN*) LocalAlloc(0, cb);

                if (NULL != prgrasconn)
                {
                    prgrasconn[0].dwSize = sizeof(RASCONN);
                    dwSuccess = RasEnumConnections(prgrasconn, &cb, &nConn);
                }
            }

            if (0 == dwSuccess)
            {
                // Make sure we have one and only one connection before hanging up
                for (DWORD i = 0; i < nConn; i ++)
                {
                    RasHangUp(prgrasconn[i].hrasconn);
                }
            }

            LocalFree(prgrasconn);
        }
    }

    //
    // restart the machine if we need to, eg: the domain changed
    //

    if ( pfReboot )
        *pfReboot = g_fRebootOnExit;

    //
    // if this is coming from setup, then lets display the message
    //

    if ( g_fRebootOnExit && !g_fShownLastPage && (g_uWizardIs != NAW_NETID) )
    {
        ShellMessageBox(GLOBAL_HINSTANCE, 
                        hwnd,
                        MAKEINTRESOURCE(IDS_RESTARTREQUIRED), MAKEINTRESOURCE(IDS_NETWIZCAPTION),
                        MB_OK);        
    }
    
    return S_OK;
}

void APIENTRY NetAccWizRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    UINT uType = 0;

    if ( *pszCmdLine )
        uType = StrToIntA(pszCmdLine);

    NetAccessWizard(hwndStub, uType, NULL);
}
