#include "stdafx.h"

#include "advanced.h"

BOOL CAdvancedDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    default:
        break;
    }

    return FALSE;
}

BOOL CAdvancedDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Create the message for the advanced dialog that informs the user
    // of the purpose of the dialog, etc.

    TCHAR szUsername[UNLEN + 1];
    TCHAR szDomainname[UNCLEN + 1];
    TCHAR szMessage[UNLEN + UNCLEN + MAX_STATIC + 1];
    TCHAR szFormat[MAX_STATIC + 1];
    TCHAR szUserAndDomain[UNLEN + UNCLEN + 1];

    // Load the current user name
    ULONG cchUsername = ARRAYSIZE(szUsername);
    ULONG cchDomainname = ARRAYSIZE(szDomainname);
    if (GetCurrentUserAndDomainName(szUsername, &cchUsername, szDomainname, &cchDomainname))
    {
        // Create the server\username form
        wsprintf(szUserAndDomain, TEXT("%s\\%s"), szDomainname, szUsername);

        // Create the message
        LoadString(g_hInstance, IDS_CONNECTASUSER_MESSAGE, szFormat, ARRAYSIZE(szFormat));
        _sntprintf(szMessage, ARRAYSIZE(szMessage), szFormat, m_pdata->szResourceName, 
            szUserAndDomain);

        SetDlgItemText(hwnd, IDC_LOGGEDIN_STATIC, szMessage);
    }

    // Load the current alternate user name into the text box
    SetDlgItemText(hwnd, IDC_USER, m_pdata->szUsername);

    Edit_LimitText(GetDlgItem(hwnd, IDC_USER), UNLEN);

    // If there is no DC, disable "find user" function
    if (!GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) )
    {
        // DS isn't available, disable browse
        EnableWindow(GetDlgItem(hwnd, IDC_FINDUSER_BUTTON), FALSE);
    }

    // Load the current alternate user password into the text box
    SetDlgItemText(hwnd, IDC_PASSWORD, m_pdata->szPassword);

    Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), PWLEN);

    // Set the reconnect check box if needed
    Button_SetCheck(GetDlgItem(hwnd, IDC_COMPLETION_RECONNECT_CHECK),
                    (m_pdata->fReconnect ? BST_CHECKED : BST_UNCHECKED));

    return TRUE;
}

BOOL CAdvancedDialog::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_FINDUSER_BUTTON:
        // User wants to look up a username
        FindUser(hwnd, IDC_USER);
        return TRUE;
    case IDOK:
        // User has click OK, save the username they've entered
        GetDlgItemText(hwnd, IDC_USER, m_pdata->szUsername, 
            ARRAYSIZE(m_pdata->szUsername));

        GetDlgItemText(hwnd, IDC_PASSWORD, m_pdata->szPassword,
            ARRAYSIZE(m_pdata->szPassword));

        // See if the user wants to reconnect to the share on startup
        m_pdata->fReconnect = IsDlgButtonChecked(hwnd, IDC_COMPLETION_RECONNECT_CHECK);

        // Fall through
    case IDCANCEL:
        EndDialog(hwnd, id);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void CAdvancedDialog::FindUser(HWND hwndDlg, UINT uiTextLocation)
// This routine activates the appropriate Object Picker to allow
// the user to select a user
// uiTextLocation  -- The resource ID of the Edit control where the selected 
//                    object should be printed 
{
    static const TCHAR c_szObjectSid[] = TEXT("ObjectSid");
    static const TCHAR* aszAttributes[] = { c_szObjectSid, };

    HRESULT             hr;
    PDSSELECTIONLIST    pDsSelectionList;

    GETUSERGROUPSELECTIONINFO gui = {0};

    gui.cbSize = sizeof(gui);
    gui.ptzComputerName = NULL;

    gui.flDsObjectPicker =
          DSOP_SCOPE_DIRECTORY
       |  DSOP_SCOPE_DOMAIN_TREE
       |  DSOP_SCOPE_EXTERNAL_TRUSTED_DOMAINS
       |  DSOP_RESTRICT_NAMES_TO_KNOWN_DOMAINS;
    gui.flStartingScope = DSOP_SCOPE_SPECIFIED_DOMAIN;
    gui.flUserGroupObjectPickerOtherDomains =
          UGOP_USERS;

    gui.cRequestedAttributes = ARRAYSIZE(aszAttributes);
    gui.aptzRequestedAttributes = aszAttributes;
    gui.ppDsSelList = &pDsSelectionList;

    gui.hwndParent = hwndDlg;

    //
    // List users and groups, with this computer as the focus
    //
    hr = GetUserGroupSelection(&gui);
    //
    // Note: If the object picker dialog is cancelled, it returns S_FALSE,
    // so we must check for that as well. Technically, we could just check 
    // (hr == S_OK), but this way, we're safe if COM adds more success codes.
    // Also, note that the NULL check for pDsSelectionList is a workaround
    // for a bug in the Object Picker where it returns S_OK with no selection.
    //
    if (SUCCEEDED(hr) && hr != S_FALSE && pDsSelectionList)
    {
        // Fill in the edit box with the user name
        LPVARIANT pvarSid = pDsSelectionList->aDsSelection[0].pvarOtherAttributes;
        if (NULL != pvarSid && (VT_ARRAY | VT_UI1) == V_VT(pvarSid))
        {
            PSID pSid; // = (PSID)(V_ARRAY(pvarSid)->pvData);

            if (SUCCEEDED(SafeArrayAccessData(V_ARRAY(pvarSid), &pSid)))
            {
                TCHAR szDomain[DNLEN + 1];
                TCHAR szUser[UNLEN + 1];
                DWORD cchDomain = ARRAYSIZE(szDomain);
                DWORD cchUser = ARRAYSIZE(szUser);
                SID_NAME_USE sUse;

                // Lookup the domain and user name for this SID
                BOOL fSuccess = LookupAccountSid(NULL, pSid, szUser, &cchUser, 
                    szDomain, &cchDomain, &sUse);

                if (fSuccess)
                {
                    TCHAR szDomainUser[UNLEN + DNLEN + 2];
                    wnsprintf(szDomainUser, ARRAYSIZE(szDomainUser), TEXT("%s\\%s"),
                        szDomain, szUser);

                    SetDlgItemText(hwndDlg, uiTextLocation, szDomainUser);
                }
                else
                {
                    // Lookupaccountsid failed
                }

                SafeArrayUnaccessData(V_ARRAY(pvarSid));
            }
        }
        else
        {
            TraceMsg("SID wasn't returned from GetUserGroupSelection");
        }
    }
    else
    {
        // MPRUI_LOG0(TRACE, "Object picker was closed with no selection made\n");
    }

    if (pDsSelectionList)
        FreeDsSelectionList(pDsSelectionList);
}
