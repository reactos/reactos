/*****************************************************************************
 *
 *    account.cpp -
 *
 *
 *****************************************************************************/

#include "priv.h"
#include "account.h"
#include "passwordapi.h"
#include <richedit.h>
#include <regapix.h>

#define SZ_REGKEY_SEPARATOR                 TEXT("\\")

// Server Level Login Attributes
#define ATTRIB_NONE                         0x00000000
#define ATTRIB_LOGIN_ANONYMOUSLY            0x00000001
#define ATTRIB_SAVE_USERNAME                0x00000002
#define ATTRIB_SAVE_PASSWORD                0x00000004

#define ATTRIB_DEFAULT                      (ATTRIB_LOGIN_ANONYMOUSLY | ATTRIB_SAVE_USERNAME)



/*****************************************************************************\
    FUNCTION: _GetAccountKey

    DESCRIPTION: 
\*****************************************************************************/
HRESULT CAccounts::_GetAccountKey(LPCTSTR pszServer, LPTSTR pszKey, DWORD cchKeySize)
{
    HRESULT hr = S_OK;

    StrCpyN(pszKey, SZ_REGKEY_FTPFOLDER_ACCOUNTS, cchKeySize);
    StrCatBuff(pszKey, pszServer, cchKeySize);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _GetUserAccountKey

    DESCRIPTION: 
\*****************************************************************************/
HRESULT CAccounts::_GetUserAccountKey(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszKey, DWORD cchKeySize)
{
    TCHAR szUserNameEscaped[MAX_PATH];
    HRESULT hr = _GetAccountKey(pszServer, pszKey, cchKeySize);

    EscapeString(pszUserName, szUserNameEscaped, ARRAYSIZE(szUserNameEscaped));
    StrCatBuff(pszKey, SZ_REGKEY_SEPARATOR, cchKeySize);
    StrCatBuff(pszKey, szUserNameEscaped, cchKeySize);

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetUserName

    DESCRIPTION: 
\*****************************************************************************/
HRESULT CAccounts::GetUserName(LPCTSTR pszServer, LPTSTR pszUserName, DWORD cchUserName)
{
    HRESULT hr = E_FAIL;
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];
    DWORD dwType = REG_SZ;
    DWORD cbSize = cchUserName * sizeof(TCHAR);

    hr = _GetAccountKey(pszServer, szKey, ARRAYSIZE(szKey));
    if (EVAL(SUCCEEDED(hr)))
    {
        if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, szKey, SZ_REGVALUE_DEFAULT_USER, &dwType, pszUserName, &cbSize))
            hr = E_FAIL;
    }

    return hr;
}




/*****************************************************************************\
    FUNCTION: _LoadLoginAttributes

    DESCRIPTION: 
\*****************************************************************************/
HRESULT CAccounts::_LoadLoginAttributes(DWORD * pdwLoginAttribs)
{
    HRESULT hr = E_FAIL;
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];
    DWORD dwType = REG_DWORD;
    DWORD cbSize = sizeof(*pdwLoginAttribs);

    // TODO: Walk the tree so these are read from the correct place.
    ASSERT(pdwLoginAttribs);
    hr = _GetAccountKey(m_pszServer, szKey, ARRAYSIZE(szKey));
    if (EVAL(SUCCEEDED(hr)))
    {
        // Do we also want to check on a per user basis?
        if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, szKey, SZ_REGKEY_LOGIN_ATTRIBS, &dwType, pdwLoginAttribs, &cbSize)) ||
            (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_FTPFOLDER_ACCOUNTS, SZ_REGKEY_LOGIN_ATTRIBS, &dwType, pdwLoginAttribs, &cbSize)))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}




/*****************************************************************************\
    FUNCTION: _SaveLoginAttributes

    DESCRIPTION: 
\*****************************************************************************/
HRESULT CAccounts::_SaveLoginAttributes(LPCTSTR pszServer, DWORD dwLoginAttribs)
{
    HRESULT hr = E_FAIL;
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];

    // TODO: Walk the tree so these are saved to the correct place.
    hr = _GetAccountKey(pszServer, szKey, ARRAYSIZE(szKey));
    if (EVAL(SUCCEEDED(hr)))
    {
        if (!EVAL(ERROR_SUCCESS == SHSetValue(HKEY_CURRENT_USER, szKey, SZ_REGKEY_LOGIN_ATTRIBS, REG_DWORD, &dwLoginAttribs, sizeof(dwLoginAttribs))) ||
            !EVAL(ERROR_SUCCESS == SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_FTPFOLDER_ACCOUNTS, SZ_REGKEY_LOGIN_ATTRIBS, REG_DWORD, &dwLoginAttribs, sizeof(dwLoginAttribs))))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetPassword

    DESCRIPTION: 
        Update m_pszUser with pszUserName and get the password if we are allowed
    to.  pszPassword is optional.
\*****************************************************************************/
HRESULT CAccounts::GetPassword(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszPassword, DWORD cchPassword)
{
    HRESULT hr = E_NOTIMPL;
    DWORD dwLogAttribs = 0;

    Str_SetPtr((LPTSTR *) &m_pszServer, pszServer);
    Str_SetPtr((LPTSTR *) &m_pszUser, pszUserName);
    _LoadLoginAttributes(&dwLogAttribs);
    hr = _LoadDefaultPassword((dwLogAttribs & ATTRIB_SAVE_PASSWORD));
    if (pszPassword)
    {
        pszPassword[0] = 0;  // Incase this password isn't stored yet.
        if (SUCCEEDED(hr))
            StrCpyN(pszPassword, m_pszPassword, cchPassword);
    }

    return hr;
}



/*****************************************************************************\
    FUNCTION: _GetPassword

    DESCRIPTION: 
        Always get a password even if persist is off.
\*****************************************************************************/
HRESULT CAccounts::_GetPassword(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszPassword, DWORD cchPassword)
{
    HRESULT hr = E_NOTIMPL;

    pszPassword[0] = 0;  // Incase this password isn't stored yet.

#ifdef FEATURE_SAVE_PASSWORD
    TCHAR wzKey[MAX_URL_STRING];

    wnsprintfW(wzKey, ARRAYSIZE(wzKey), L"ftp://%ls@%ls", pszUserName, pszServer);
    hr = GetCachedCredentials(wzKey, pszPassword, cchPassword);
#endif // FEATURE_SAVE_PASSWORD

    return hr;
}



/*****************************************************************************\
    FUNCTION: _UserChangeSelect

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_UserChangeSelect(HWND hDlg, BOOL fSelectChange)
{
    HRESULT hr = S_OK;
    TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
    HWND hwndComboBox = GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME);

    // SelectChange requires we get the text thru ComboBox_GetLBText because
    // it's not in GetWindowText yet.  KILLFOCUS requires we get it from
    // GetWindowText because nothing is selected.
    szUser[0] = 0;
    if (fSelectChange)
    {
        if (ARRAYSIZE(szUser) > ComboBox_GetLBTextLen(hwndComboBox, ComboBox_GetCurSel(hwndComboBox)))
            ComboBox_GetLBText(hwndComboBox, ComboBox_GetCurSel(hwndComboBox), szUser);
    }
    else
        GetWindowText(hwndComboBox, szUser, ARRAYSIZE(szUser));

    if (szUser[0])
    {
        GetPassword(m_pszServer, szUser, NULL, 0);
        SetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG1), m_pszPassword);
    }

    return hr;
}



/*****************************************************************************\
    FUNCTION: _SaveUserName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_SaveUserName(HWND hDlg)
{
    HRESULT hr = S_OK;
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];
    TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];

    GetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME), szUser, ARRAYSIZE(szUser));
    Str_SetPtr((LPTSTR *) &m_pszUser, szUser);

    // Always save the user name
    hr = _GetAccountKey(m_pszServer, szKey, ARRAYSIZE(szKey));
    if (EVAL(SUCCEEDED(hr)))
    {
        if (!EVAL(ERROR_SUCCESS == SHSetValue(HKEY_CURRENT_USER, szKey, SZ_REGVALUE_DEFAULT_USER, REG_SZ, szUser, (lstrlen(szUser) + 1) * sizeof(TCHAR))))
            hr = E_FAIL;

        hr = _GetUserAccountKey(m_pszServer, m_pszUser, szKey, ARRAYSIZE(szKey));
        if (EVAL(SUCCEEDED(hr)))
            SHSetValue(HKEY_CURRENT_USER, szKey, TEXT(""), REG_SZ, TEXT(""), sizeof(TEXT("")));
    }

    return hr;
}



/*****************************************************************************\
    FUNCTION: _SavePassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_SavePassword(HWND hDlg, LPCTSTR pszUser, BOOL fPersist)
{
    HRESULT hr = S_OK;
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
    TCHAR wzKey[MAX_URL_STRING];

    GetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG1), szPassword, ARRAYSIZE(szPassword));
    Str_SetPtr((LPTSTR *) &m_pszPassword, szPassword);

#ifdef FEATURE_SAVE_PASSWORD
    if (fPersist)
    {
        wnsprintfW(wzKey, ARRAYSIZE(wzKey), L"ftp://%ls@%ls", pszUser, m_pszServer);
        hr = SetCachedCredentials(wzKey, szPassword);
    }
#endif // FEATURE_SAVE_PASSWORD

    return hr;
}


/*****************************************************************************\
    FUNCTION: _SetLoginType

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_SetLoginType(HWND hDlg, BOOL fLoginAnnonymously)
{
    ////// The "Annonymous" section
    // Set the Radio Button
    CheckDlgButton(hDlg, IDC_LOGINDLG_ANONYMOUS_CBOX, (fLoginAnnonymously ? BST_CHECKED : BST_UNCHECKED));

    // Disable or Enable applicable items
    if (fLoginAnnonymously)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME_ANON), SW_SHOW);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG1), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2), SW_SHOW);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_LABEL_DLG1), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_LABEL_DLG2), SW_SHOW);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_NOTES_DLG1), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_NOTES_DLG2), SW_SHOW);

        // Hide "Save Password" in Anonymous mode.
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_SAVE_PASSWORD), SW_HIDE);
    }
    else
    {
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME_ANON), SW_HIDE);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG1), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2), SW_HIDE);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_LABEL_DLG1), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_LABEL_DLG2), SW_HIDE);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_NOTES_DLG1), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_NOTES_DLG2), SW_HIDE);

        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_SAVE_PASSWORD), SW_SHOW);
    }

    if (fLoginAnnonymously) // Select all the text.
    {
        int iStart = 0;
        int iEnd = -1;

        SendMessage(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2), EM_GETSEL, (WPARAM) &iStart, (LPARAM) &iEnd);
    }

    SetFocus(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2));
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _PopulateUserNameDropDown

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_PopulateUserNameDropDown(HWND hDlg, LPCTSTR pszServer)
{
    HRESULT hr = S_OK;
    HWND hwndUserComboBox = GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME);

    if (EVAL(hwndUserComboBox))
    {
        TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];
        TCHAR szDefaultUser[INTERNET_MAX_USER_NAME_LENGTH];

        szDefaultUser[0] = 0;
        hr = _GetAccountKey(pszServer, szKey, ARRAYSIZE(szKey));
        if (EVAL(SUCCEEDED(hr)))
        {
            HKEY hKey;

            SendMessage(hwndUserComboBox, CB_RESETCONTENT, 0, 0);      // Empty the contents.
            if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, szKey, &hKey))
            {
                TCHAR szUser[INTERNET_MAX_USER_NAME_LENGTH];
                DWORD dwIndex = 0;

                while (ERROR_SUCCESS == RegEnumKey(hKey, dwIndex++, szUser, sizeof(szUser)))
                {
                    UnEscapeString(NULL, szUser, ARRAYSIZE(szUser));
                    SendMessage(hwndUserComboBox, CB_ADDSTRING, NULL, (LPARAM) szUser);
                }

                RegCloseKey(hKey);
            }

            SendMessage(hwndUserComboBox, CB_SETCURSEL, 0, 0);
        }

        if (!m_pszUser[0])
            GetUserName(pszServer, szDefaultUser, ARRAYSIZE(szDefaultUser));

        if (CB_ERR == SendMessage(hwndUserComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM) (m_pszUser ? m_pszUser : szDefaultUser)))
            SendMessage(hwndUserComboBox, CB_ADDSTRING, NULL, (LPARAM) (m_pszUser ? m_pszUser : szDefaultUser));

        SetWindowText(hwndUserComboBox, (m_pszUser ? m_pszUser : szDefaultUser));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _LoadDefaultPassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_LoadDefaultPassword(BOOL fLoadPersisted)
{
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
    HRESULT hr = S_FALSE;

    if (fLoadPersisted)
        hr = _GetPassword(m_pszServer, m_pszUser, szPassword, ARRAYSIZE(szPassword));
    else
        szPassword[0] = 0;

    Str_SetPtr((LPTSTR *) &m_pszPassword, szPassword);
    return hr;
}


/*****************************************************************************\
    FUNCTION: _LoadMessage

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_LoadMessage(HWND hDlg)
{
    // if it's allowed, we need to load the anonymous email.  This needs to be
    // be hard coded in English because that's how FTP works.
    SetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_USERNAME_ANON), TEXT("Anonymous"));

    if (LOGINFLAGS_ANON_LOGINJUSTFAILED & m_dwLoginFlags)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_NORMAL), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_USERREJECT), SW_HIDE);
    }
    else if (LOGINFLAGS_USER_LOGINJUSTFAILED & m_dwLoginFlags)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_ANONREJECT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_NORMAL), SW_HIDE);
    }
    else
    {
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_ANONREJECT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_LOGINDLG_MESSAGE_USERREJECT), SW_HIDE);
    }

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _LoadEMailName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_LoadEMailName(HWND hDlg)
{
    TCHAR szEmailName[MAX_PATH];
    DWORD dwType = REG_SZ;
    DWORD cbSize = sizeof(szEmailName);

    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS, SZ_REGKEY_EMAIL_NAME, &dwType, szEmailName, &cbSize))
        SetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2), szEmailName);

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _SaveEMailName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::_SaveEMailName(HWND hDlg)
{
    HRESULT hr = E_FAIL;
    TCHAR szEmailName[MAX_PATH];

    if (GetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG2), szEmailName, ARRAYSIZE(szEmailName)))
    {
        if (ERROR_SUCCESS == SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS, SZ_REGKEY_EMAIL_NAME, REG_SZ, szEmailName, (lstrlen(szEmailName) + 1) * sizeof(TCHAR)))
            hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _InitDialog

    DESCRIPTION:
\*****************************************************************************/
BOOL CAccounts::_InitDialog(HWND hDlg)
{
    DWORD dwLogAttribs = ATTRIB_SAVE_USERNAME;
    BOOL fSucceeded = SetProp(hDlg, SZ_ACCOUNT_PROP, this);
    ASSERT(fSucceeded);

    // Init the dialog controls
    _LoadMessage(hDlg);     // Load Message
    _LoadLoginAttributes(&dwLogAttribs);
    if (m_dwLoginFlags & LOGINFLAGS_ANON_ISDEFAULT) // Do we want to login anonymously?
        dwLogAttribs |= ATTRIB_LOGIN_ANONYMOUSLY;   // Yes.

    CheckDlgButton(hDlg, IDC_LOGINDLG_ANONYMOUS_CBOX, (m_dwLoginFlags & LOGINFLAGS_ANON_ISDEFAULT));
    SetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_FTPSERVER), m_pszServer);

    _SetLoginType(hDlg, ATTRIB_LOGIN_ANONYMOUSLY & dwLogAttribs);

    _LoadEMailName(hDlg);
    _PopulateUserNameDropDown(hDlg, m_pszServer);
    _LoadDefaultPassword((dwLogAttribs & ATTRIB_SAVE_PASSWORD));
    SetWindowText(GetDlgItem(hDlg, IDC_LOGINDLG_PASSWORD_DLG1), m_pszPassword);

#ifdef FEATURE_SAVE_PASSWORD
    if (S_OK == InitCredentialPersist())
        CheckDlgButton(hDlg, IDC_LOGINDLG_SAVE_PASSWORD, (dwLogAttribs & ATTRIB_SAVE_PASSWORD));
    else
        EnableWindow(GetDlgItem(hDlg, IDC_LOGINDLG_SAVE_PASSWORD), FALSE);
#endif // FEATURE_SAVE_PASSWORD

    return TRUE;
}

/*****************************************************************************\
    FUNCTION: _SaveDialogData

    DESCRIPTION:
\*****************************************************************************/
BOOL CAccounts::_SaveDialogData(HWND hDlg)
{
    DWORD dwLogAttribs = ATTRIB_NONE;
    if (IsDlgButtonChecked(hDlg, IDC_LOGINDLG_ANONYMOUS_CBOX))
        m_dwLoginFlags |= LOGINFLAGS_ANON_ISDEFAULT;
    else
        m_dwLoginFlags &= ~LOGINFLAGS_ANON_ISDEFAULT;

    // Alway save user name
    dwLogAttribs |= ATTRIB_SAVE_USERNAME;

    _SaveUserName(hDlg);
#ifdef FEATURE_SAVE_PASSWORD
    if (IsDlgButtonChecked(hDlg, IDC_LOGINDLG_SAVE_PASSWORD))
        dwLogAttribs |= ATTRIB_SAVE_PASSWORD;

    if (!(m_dwLoginFlags & LOGINFLAGS_ANON_ISDEFAULT))
        _SavePassword(hDlg, m_pszUser, (dwLogAttribs & ATTRIB_SAVE_PASSWORD));
#endif // FEATURE_SAVE_PASSWORD

    _SaveLoginAttributes(m_pszServer, dwLogAttribs);

    // Init the dialog controls
    _SaveEMailName(hDlg);

    return TRUE;
}


/*****************************************************************************\
    FUNCTION: _OnCommand

    DESCRIPTION:
\*****************************************************************************/
LRESULT CAccounts::_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    UINT idc = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idc)
    {
    case IDOK:
        if (m_hDlg == hDlg)  // (IDOK)
        {
            _SaveDialogData(hDlg);
            EndDialog(hDlg, TRUE);
            lResult = 1;
        }
        break;

    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        lResult = 1;
        break;

    case IDC_LOGINDLG_ANONYMOUS_CBOX:
        _SetLoginType(hDlg, IsDlgButtonChecked(hDlg, IDC_LOGINDLG_ANONYMOUS_CBOX));
        lResult = 1;
        break;

    case IDC_LOGINDLG_USERNAME:
        {
            UINT uCmd = GET_WM_COMMAND_CMD(wParam, lParam);

            switch (uCmd)
            {
            case CBN_SELCHANGE:
            case CBN_KILLFOCUS:
                _UserChangeSelect(hDlg, (CBN_SELCHANGE == uCmd));
                lResult = 1;
                break;
            }
        }
        break;
    }

    return lResult;
}


/*****************************************************************************\
    FUNCTION: _LoginDialogProc

    DESCRIPTION:
\*****************************************************************************/
INT_PTR CALLBACK CAccounts::_LoginDialogProc(HWND hDlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (wm)
    {
    case WM_INITDIALOG:
        {
            CAccounts * pThis = (CAccounts *) lParam;
            pThis->m_hDlg = hDlg;
            lResult = pThis->_InitDialog(hDlg);
            break;
        }

    case WM_COMMAND:
        {
            CAccounts * pThis = (CAccounts *)GetProp(hDlg, SZ_ACCOUNT_PROP);

            if (EVAL(pThis))
                lResult = pThis->_OnCommand(hDlg, wParam, lParam);
            break;
        }
    }

    return lResult;
}


/*****************************************************************************\
    FUNCTION: GetAccountUrl

    DESCRIPTION:
\*****************************************************************************/
HRESULT CAccounts::DisplayLoginDialog(HWND hwnd, DWORD dwLoginFlags, LPCTSTR pszServer, LPTSTR pszUserName, DWORD cchUserNameSize, LPTSTR pszPassword, DWORD cchPasswordSize)
{
    HRESULT hr;

    ASSERT(hwnd && pszServer[0]);
    if (TEXT('\0') == pszUserName[0])
        hr = GetUserName(pszServer, pszUserName, cchUserNameSize);

    Str_SetPtr((LPTSTR *) &m_pszServer, pszServer);
    Str_SetPtr((LPTSTR *) &m_pszUser, pszUserName);
    Str_SetPtr((LPTSTR *) &m_pszPassword, pszPassword);

    m_dwLoginFlags = dwLoginFlags;
    if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_LOGINDLG), hwnd, _LoginDialogProc, (LPARAM)this))
    {
        StrCpyN(pszUserName, ((m_dwLoginFlags & LOGINFLAGS_ANON_ISDEFAULT) ? TEXT("") : m_pszUser), cchUserNameSize);
        StrCpyN(pszPassword, ((m_dwLoginFlags & LOGINFLAGS_ANON_ISDEFAULT) ? TEXT("") : m_pszPassword), cchPasswordSize);
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    return hr;
}




/****************************************************\
    Constructor
\****************************************************/
CAccounts::CAccounts()
{
    DllAddRef();

    // NOTE: We may be put on the stack, so we will not
    //    automatically have our member variables inited.
    m_pszServer = 0;
    m_pszUser = 0;
    m_pszPassword = 0;

    LEAK_ADDREF(LEAK_CAccount);
}


/****************************************************\
    Destructor
\****************************************************/
CAccounts::~CAccounts()
{
    Str_SetPtr((LPTSTR *) &m_pszServer, NULL);
    Str_SetPtr((LPTSTR *) &m_pszUser, NULL);
    Str_SetPtr((LPTSTR *) &m_pszPassword, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CAccount);
}

