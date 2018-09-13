#include "mslocusr.h"
#include "msluglob.h"

#include "resource.h"

#include "profiles.h"
#include <npmsg.h>
#include <shellapi.h>

#include "contxids.h"


HRESULT GetLBItemText(HWND hDlg, UINT idCtrl, int iItem, NLS_STR *pnls)
{
    HWND hCtrl = GetDlgItem(hDlg, idCtrl);
    UINT cch = (UINT)SendMessage(hCtrl, LB_GETTEXTLEN, iItem, 0);
    if (pnls->realloc(cch + 1)) {
        SendMessage(hCtrl, LB_GETTEXT, iItem, (LPARAM)(LPSTR)pnls->Party());
        pnls->DonePartying();
        return NOERROR;
    }
    return E_OUTOFMEMORY;
}


void SetErrorFocus(HWND hDlg, UINT idCtrl, BOOL fClear /* = TRUE */)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    ::SetFocus(hCtrl);
    if (fClear)
        ::SetWindowText(hCtrl, "");
    else
        ::SendMessage(hCtrl, EM_SETSEL, 0, -1);
}


BOOL CALLBACK PasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_STATIC2,    IDH_RATINGS_SUPERVISOR_PASSWORD,
        IDC_PASSWORD,   IDH_RATINGS_SUPERVISOR_PASSWORD,
        0,0
    };

    CHAR pszPassword[MAX_PATH];
    HRESULT hRet;

    switch (uMsg) {
        case WM_INITDIALOG:
            {
                HWND hwndCheckbox = GetDlgItem(hDlg, IDC_CACHE_PASSWORD);

                IUserDatabase *pDB = (IUserDatabase *)lParam;
                SetWindowLongPtr(hDlg, DWL_USER, lParam);
                IUser *pCurrentUser;
                HRESULT hres;
                hres = pDB->GetCurrentUser(&pCurrentUser);
                if (SUCCEEDED(hres)) {
                    DWORD cbBuffer = sizeof(pszPassword);
                    hres = pCurrentUser->GetName(pszPassword, &cbBuffer);
                    if (SUCCEEDED(hres)) {
                        NLS_STR nlsName(STR_OWNERALLOC, pszPassword);
                        NLS_STR nlsTemp(MAX_RES_STR_LEN);
                        const NLS_STR *apnls[] = { &nlsName, NULL };
                        hres = HRESULT_FROM_WIN32(nlsTemp.LoadString(IDS_CACHE_PASSWORD, apnls));
                        if (SUCCEEDED(hres))
                            SetWindowText(hwndCheckbox, nlsTemp.QueryPch());
                    }
                    pCurrentUser->Release();
                }
                if (FAILED(hres)) {
                    ShowWindow(hwndCheckbox, SW_HIDE);
                    EnableWindow(hwndCheckbox, FALSE);
                }
                CheckDlgButton(hDlg, IDC_CACHE_PASSWORD, 0);
            }
            return TRUE;        /* we did not set the focus */

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDCANCEL:
                EndDialog(hDlg, FALSE);
                break;

            case IDOK:
                GetDlgItemText(hDlg, IDC_PASSWORD, pszPassword, sizeof(pszPassword));
                hRet = VerifySupervisorPassword(pszPassword);

                if (hRet == (NOERROR)) {
                    IUserDatabase *pDB = (IUserDatabase *)GetWindowLongPtr(hDlg, DWL_USER);
                    IUser *pCurrentUser;
                    if (SUCCEEDED(pDB->GetCurrentUser(&pCurrentUser))) {
                        if (IsDlgButtonChecked(hDlg, IDC_CACHE_PASSWORD)) {
                            pCurrentUser->SetSupervisorPrivilege(TRUE, pszPassword);
                        }
                        else {
                            pCurrentUser->MakeTempSupervisor(TRUE, pszPassword);
                        }
                        pCurrentUser->Release();
                    }

                    EndDialog(hDlg, TRUE);
                }
                else
                {
                    MsgBox(hDlg, IDS_BADPASSWORD, MB_OK | MB_ICONSTOP);
                    SetErrorFocus(hDlg, IDC_PASSWORD);
                }
                break;

            default:
                return FALSE;
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szRatingsHelpFile,
                    HELP_WM_HELP, (DWORD)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, ::szRatingsHelpFile, HELP_CONTEXTMENU,
                    (DWORD)(LPVOID)aIds);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ChangePasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,            IDH_OLD_PASSWORD,
        IDC_OLD_PASSWORD,       IDH_OLD_PASSWORD,
        IDC_STATIC2,            IDH_NEW_PASSWORD,
        IDC_PASSWORD,           IDH_NEW_PASSWORD,
        IDC_STATIC3,            IDH_CONFIRM_PASSWORD,
        IDC_CONFIRM_PASSWORD,   IDH_CONFIRM_PASSWORD,
        0,0
    };

    CHAR pszPassword[MAX_PATH];
    CHAR pszTempPassword[MAX_PATH];
    CHAR *p = NULL;
    HRESULT hRet;
    HWND hwndPassword;

    switch (uMsg) {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWL_USER, lParam);
            return TRUE;    /* we didn't set the focus */

        case WM_COMMAND:
            switch (LOWORD(wParam))  {
            case IDCANCEL:
                EndDialog(hDlg, FALSE);
                break;

            case IDOK:
                {
                    IUser *pUser = (IUser *)GetWindowLongPtr(hDlg, DWL_USER);

                    hwndPassword = ::GetDlgItem(hDlg, IDC_PASSWORD);
                    GetWindowText(hwndPassword, pszPassword, sizeof(pszPassword));
                    GetDlgItemText(hDlg, IDC_CONFIRM_PASSWORD, pszTempPassword, sizeof(pszTempPassword));

                    /* if they've typed just the first password but not the
                     * second, let Enter take them to the second field
                     */
                    if (*pszPassword && !*pszTempPassword && GetFocus() == hwndPassword) {
                        SetErrorFocus(hDlg, IDC_CONFIRM_PASSWORD);
                        break;
                    }

                    if (strcmpf(pszPassword, pszTempPassword))
                    {
                        MsgBox(hDlg, IDS_NO_MATCH, MB_OK | MB_ICONSTOP);
                        SetErrorFocus(hDlg, IDC_CONFIRM_PASSWORD);
                        break;
                    }

                    GetDlgItemText(hDlg, IDC_OLD_PASSWORD, pszTempPassword, sizeof(pszTempPassword));
                    
                    hRet = pUser->ChangePassword(pszTempPassword, pszPassword);
                    
                    if (SUCCEEDED(hRet))
                        EndDialog(hDlg, TRUE);
                    else
                    {
                        MsgBox(hDlg, IDS_BADPASSWORD, MB_OK | MB_ICONSTOP);
                        SetErrorFocus(hDlg, IDC_OLD_PASSWORD);
                    }
                    
                    break;
                }

            default:
                return FALSE;
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                    HELP_WM_HELP, (DWORD)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                    (DWORD)(LPVOID)aIds);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


BOOL DoPasswordConfirm(HWND hwndParent, IUserDatabase *pDB)
{
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwndParent, (DLGPROC) PasswordDialogProc, (LPARAM)pDB);
}


const UINT MAX_PAGES = 1;

class CCPLData
{
public:
    IUserDatabase *m_pDB;
    LPCSTR m_pszNameToDelete;
};


void CPLUserSelected(HWND hwndLB, int iItem)
{
    HWND hDlg = GetParent(hwndLB);
    BOOL fEnableButtons = (iItem != LB_ERR);

    EnableWindow(GetDlgItem(hDlg, IDC_Delete), fEnableButtons);
    EnableWindow(GetDlgItem(hDlg, IDC_Clone), fEnableButtons);
    EnableWindow(GetDlgItem(hDlg, IDC_SetPassword), fEnableButtons);
    EnableWindow(GetDlgItem(hDlg, IDC_OpenProfileFolder), fEnableButtons);

    NLS_STR nlsTemp(MAX_RES_STR_LEN);

    if (fEnableButtons) {
        NLS_STR nlsName;
        if (SUCCEEDED(GetLBItemText(hDlg, IDC_USERNAME, iItem, &nlsName))) {
            const NLS_STR *apnls[] = { &nlsName, NULL };
            if (nlsTemp.LoadString(IDS_SETTINGS_FOR, apnls) != ERROR_SUCCESS)
                nlsTemp = szNULL;
        }
    }
    else {
        if (nlsTemp.LoadString(IDS_SELECTED_USER) != ERROR_SUCCESS)
            nlsTemp = szNULL;
    }
    if (nlsTemp.strlen())
        SetDlgItemText(hDlg, IDC_MAIN_CAPTION, nlsTemp.QueryPch());
}


void ReInitUserList(HWND hDlg, CCPLData *pcpld)
{
    HWND hwndLB = GetDlgItem(hDlg, IDC_USERNAME);

    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0);
    DestroyUserList(hwndLB);
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    FillUserList(hwndLB, pcpld->m_pDB, NULL, FALSE, CPLUserSelected);
    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndLB, NULL, TRUE);
}


void DoCloneUser(HWND hDlg, CCPLData *pcpld, IUser *pUserToClone)
{
    DoAddUserWizard(hDlg, pcpld->m_pDB, FALSE, pUserToClone);

    ReInitUserList(hDlg, pcpld);
}


HRESULT DeleteProgressFunc(LPARAM lParam)
{
    CCPLData *pcpld = (CCPLData *)lParam;

    return pcpld->m_pDB->DeleteUser(pcpld->m_pszNameToDelete);
}


void DoDeleteUser(HWND hDlg, CCPLData *pcpld, int iItem)
{
    NLS_STR nlsName;
    if (FAILED(GetLBItemText(hDlg, IDC_USERNAME, iItem, &nlsName)))
        return;

    const NLS_STR *apnls[] = { &nlsName, NULL };
    if (MsgBox(hDlg, IDS_CONFIRM_DELETE_USER,
               MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
               apnls) == IDNO)
        return;

    pcpld->m_pszNameToDelete = nlsName.QueryPch();
    HRESULT hres = CallWithinProgressDialog(hDlg, IDD_DeleteProgress,
                                            DeleteProgressFunc, (LPARAM)pcpld);

    if (SUCCEEDED(hres)) {
        ReInitUserList(hDlg, pcpld);
    }
    else {
        ReportUserError(hDlg, hres);
    }
}


void DoSetPassword(HWND hDlg, CCPLData *pcpld, int iItem)
{
    /* Note, getting pUser this way does not automatically AddRef it */
    IUser *pUser = (IUser *)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETITEMDATA, iItem, 0);

    if (pUser != NULL) {
        pUser->AddRef();        /* extra AddRef for life of dialog */
        if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHANGE_PASSWORD), hDlg, (DLGPROC) ChangePasswordDialogProc, (LPARAM)pUser))
            MsgBox(hDlg, IDS_PASSWORD_CHANGED, MB_OK | MB_ICONINFORMATION);
        pUser->Release();       /* undo above AddRef */
    }
}


BOOL CALLBACK FoldersDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_CHECK_DESKTOP,      IDH_DESKTOP_NETHOOD,
        IDC_CHECK_STARTMENU,    IDH_START_MENU,
        IDC_CHECK_FAVORITES,    IDH_FAVORITES,
        IDC_CHECK_CACHE,        IDH_TEMP_FILES,
        IDC_CHECK_MYDOCS,       IDH_MY_DOCS,
        IDC_RADIO_EMPTY,        IDH_EMPTY_FOLDERS,
        IDC_RADIO_COPY,         IDH_EXISTING_FILES,
        0,0
    };

    switch(message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))  {
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        case IDOK:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

                pwd->m_fdwCloneFromDefault = IsDlgButtonChecked(hDlg, IDC_RADIO_EMPTY) ? 0 : 0xffffffff;
                pwd->m_fdwNewPerUserFolders = 0;
                if (IsDlgButtonChecked(hDlg, IDC_CHECK_DESKTOP))
                    pwd->m_fdwNewPerUserFolders |= FOLDER_DESKTOP | FOLDER_NETHOOD | FOLDER_RECENT;
                else
                    pwd->m_fdwNewPerUserFolders &= ~(FOLDER_DESKTOP | FOLDER_NETHOOD | FOLDER_RECENT);

                if (IsDlgButtonChecked(hDlg, IDC_CHECK_STARTMENU))
                    pwd->m_fdwNewPerUserFolders |= FOLDER_STARTMENU | FOLDER_PROGRAMS | FOLDER_STARTUP;
                else
                    pwd->m_fdwNewPerUserFolders &= ~(FOLDER_STARTMENU | FOLDER_PROGRAMS | FOLDER_STARTUP);

                if (IsDlgButtonChecked(hDlg, IDC_CHECK_FAVORITES))
                    pwd->m_fdwNewPerUserFolders |= FOLDER_FAVORITES;
                else
                    pwd->m_fdwNewPerUserFolders &= ~(FOLDER_FAVORITES);

                if (IsDlgButtonChecked(hDlg, IDC_CHECK_CACHE))
                    pwd->m_fdwNewPerUserFolders |= FOLDER_CACHE;
                else
                    pwd->m_fdwNewPerUserFolders &= ~(FOLDER_CACHE);

                if (IsDlgButtonChecked(hDlg, IDC_CHECK_MYDOCS))
                    pwd->m_fdwNewPerUserFolders |= FOLDER_MYDOCS;
                else
                    pwd->m_fdwNewPerUserFolders &= ~(FOLDER_MYDOCS);

                FinishChooseFolders(hDlg, pwd);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWL_USER, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

            InitFolderCheckboxes(hDlg, pwd);
            CheckRadioButton(hDlg, IDC_RADIO_COPY, IDC_RADIO_EMPTY, IDC_RADIO_COPY);
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                HELP_WM_HELP, (DWORD)(LPSTR)aIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                (DWORD)(LPVOID)aIds);
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


void DoOpenProfileFolder(HWND hDlg, CCPLData *pcpld, int iItem)
{
    /* Note, getting pUser this way does not automatically AddRef it */
    IUser *pUser = (IUser *)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETITEMDATA, iItem, 0);

#if 0   /* old code to launch Explorer on the user's profile dir */
    if (pUser != NULL) {
        CHAR szHomeDir[MAX_PATH];
        DWORD cbBuffer = sizeof(szHomeDir);

        if (SUCCEEDED(pUser->GetProfileDirectory(szHomeDir, &cbBuffer)) &&
            cbBuffer > 0) {
            TCHAR szArgs[MAX_PATH+4];
            lstrcpy(szArgs, "/e,");
            lstrcat(szArgs, szHomeDir);

            SHELLEXECUTEINFO ei;

            ei.lpFile          = "explorer.exe";
            ei.cbSize          = sizeof(SHELLEXECUTEINFO);
            ei.hwnd            = NULL;
            ei.lpVerb          = NULL;
            ei.lpParameters    = szArgs;
            ei.lpDirectory     = szHomeDir;
            ei.nShow           = SW_SHOWNORMAL;
            ei.fMask           = SEE_MASK_NOCLOSEPROCESS;

            if (ShellExecuteEx(&ei))
            {
                CloseHandle(ei.hProcess);
            }
        }
    }
#else

    if (pUser != NULL) {
        CWizData wd;
        wd.m_pDB = pcpld->m_pDB;
        wd.m_pUserToClone = pUser;
        pUser->AddRef();

        DialogBoxParam(::hInstance, MAKEINTRESOURCE(IDD_ChooseFolders), hDlg, 
                       FoldersDlgProc, (LPARAM)&wd);
    }

#endif
}


BOOL CALLBACK UserCPLDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_USERNAME,           IDH_USERS_LIST,
        IDC_Add,                IDH_NEW_USER,
        IDC_Delete,             IDH_REMOVE_USER,
        IDC_Clone,              IDH_COPY_USER,
        IDC_SetPassword,        IDH_SET_PASSWORD,
        IDC_OpenProfileFolder,  IDH_CHANGE_DESKTOP,
        0,0
    };

    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                int iItem = (int)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETCURSEL, 0, 0);
                CPLUserSelected((HWND)lParam, iItem);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            PropSheet_CancelToClose(GetParent(hDlg));
            InitWizDataPtr(hDlg, lParam);
            CCPLData *pcpld = (CCPLData *)(((LPPROPSHEETPAGE)lParam)->lParam);
            FillUserList(GetDlgItem(hDlg, IDC_USERNAME), pcpld->m_pDB, NULL,
                         FALSE, CPLUserSelected);
        }
        break;

    case WM_DESTROY:
        DestroyUserList(GetDlgItem(hDlg, IDC_USERNAME));
        break;

    case WM_COMMAND:
        {
            CCPLData *pcpld = (CCPLData *)GetWindowLongPtr(hDlg, DWL_USER);
            int iItem = (int)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETCURSEL, 0, 0);

            switch (LOWORD(wParam)) {
            case IDC_USERNAME:
                if (HIWORD(wParam) == LBN_SELCHANGE) {
                    CPLUserSelected((HWND)lParam, iItem);
                }
                break;

            case IDC_Add:
                DoCloneUser(hDlg, pcpld, NULL);
                break;

            case IDC_Clone:
                {
                    if (iItem != LB_ERR) {
                        IUser *pUser = (IUser *)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETITEMDATA, iItem, 0);
                        DoCloneUser(hDlg, pcpld, pUser);
                    }
                }
                break;

            case IDC_Delete:
                DoDeleteUser(hDlg, pcpld, iItem);
                break;

            case IDC_SetPassword:
                DoSetPassword(hDlg, pcpld, iItem);
                break;

            case IDC_OpenProfileFolder:
                DoOpenProfileFolder(hDlg, pcpld, iItem);
                break;

            default:
                return FALSE;
            }   /* switch */
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                HELP_WM_HELP, (DWORD)(LPSTR)aIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                (DWORD)(LPVOID)aIds);
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


STDMETHODIMP CLUDatabase::UserCPL(HWND hwndParent)
{
    if (ProfileUIRestricted()) {
        ReportRestrictionError(hwndParent);
        return E_ACCESSDENIED;
    }

    CCPLData cpld;
    cpld.m_pDB = this;

    if (!UseUserProfiles() || FAILED(VerifySupervisorPassword(szNULL))) {
        return InstallWizard(hwndParent);
    }
    else {
        BOOL fContinue = TRUE;

        if (IsCurrentUserSupervisor(this) != S_OK) {
            fContinue = DoPasswordConfirm(hwndParent, this);
        }

        if (fContinue) {
            LPPROPSHEETHEADER ppsh;

            // Allocate the property sheet header
            //
            if ((ppsh = (LPPROPSHEETHEADER)LocalAlloc(LMEM_FIXED, sizeof(PROPSHEETHEADER)+
                        (MAX_PAGES * sizeof(HPROPSHEETPAGE)))) != NULL)
            {
                ppsh->dwSize     = sizeof(*ppsh);
                ppsh->dwFlags    = PSH_NOAPPLYNOW;
                ppsh->hwndParent = hwndParent;
                ppsh->hInstance  = ::hInstance;
                ppsh->pszCaption = (LPSTR)IDS_MSGTITLE;
                ppsh->nPages     = 0;
                ppsh->nStartPage = 0;
                ppsh->phpage     = (HPROPSHEETPAGE *)(ppsh+1);

                AddPage(ppsh, IDD_Users, UserCPLDlgProc, &cpld);

                PropertySheet(ppsh);

                LocalFree((HLOCAL)ppsh);
            }
        }
    }

    return NOERROR;
}


void DoUserCPL(HWND hwndParent)
{
    IUserDatabase *pDB = NULL;
    if (FAILED(::CreateUserDatabase(IID_IUserDatabase, (void **)&pDB)))
        return;

    pDB->UserCPL(hwndParent);

    pDB->Release();
}


extern "C" void UserCPL(HWND hwndParent, HINSTANCE hinstEXE, LPSTR pszCmdLine, int nCmdShow)
{
    DoUserCPL(hwndParent);
}
