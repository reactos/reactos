#include "mslocusr.h"
#include "msluglob.h"

#include "resource.h"

#include <regentry.h>
#include "profiles.h"
#include <npmsg.h>

extern "C" void SHFlushSFCache(void);

void ReportUserError(HWND hwndParent, HRESULT hres)
{
    if (SUCCEEDED(hres))
        return;

    UINT idMsg;
    NLS_STR nlsSub(16);     /* long enough for any 32-bit number, any format */

    if ((((DWORD)hres) >> 16) == (FACILITY_WIN32 | 0x8000)) {
        UINT err = (hres & 0xffff);

        switch (err) {
        case ERROR_ACCESS_DENIED:       idMsg = IDS_E_ACCESSDENIED; break;
        case ERROR_NOT_AUTHENTICATED:   idMsg = IDS_ERROR_NOT_AUTHENTICATED; break;
        case ERROR_NO_SUCH_USER:        idMsg = IDS_ERROR_NO_SUCH_USER; break;
        case ERROR_USER_EXISTS:         idMsg = IDS_ERROR_USER_EXISTS; break;
        case ERROR_NOT_ENOUGH_MEMORY:   idMsg = IDS_ERROR_OUT_OF_MEMORY; break;
        case ERROR_BUSY:                idMsg = IDS_ERROR_BUSY; break;
        case ERROR_PATH_NOT_FOUND:      idMsg = IDS_ERROR_PATH_NOT_FOUND; break;
        case ERROR_BUFFER_OVERFLOW:     idMsg = IDS_ERROR_BUFFER_OVERFLOW; break;
        case IERR_CachingDisabled:      idMsg = IDS_IERR_CachingDisabled; break;
        case IERR_BadSig:               idMsg = IDS_IERR_BadSig; break;
        case IERR_CacheReadOnly :       idMsg = IDS_IERR_CacheReadOnly; break;
        case IERR_IncorrectUsername:    idMsg = IDS_IERR_IncorrectUsername; break;
        case IERR_CacheCorrupt:         idMsg = IDS_IERR_CacheCorrupt; break;
        case IERR_UsernameNotFound:     idMsg = IDS_IERR_UsernameNotFound; break;
        case IERR_CacheFull:            idMsg = IDS_IERR_CacheFull; break;
        case IERR_CacheAlreadyOpen:     idMsg = IDS_IERR_CacheAlreadyOpen; break;

        default:
            idMsg = IDS_UNKNOWN_ERROR;
            wsprintf(nlsSub.Party(), "%d", err);
            nlsSub.DonePartying();
            break;
        }
    }
    else {
        switch(hres) {
        case E_OUTOFMEMORY:             idMsg = IDS_ERROR_OUT_OF_MEMORY; break;
//        case E_ACCESSDENIED:            idMsg = IDS_E_ACCESSDENIED; break;

        default:
            idMsg = IDS_UNKNOWN_ERROR;
            wsprintf(nlsSub.Party(), "0x%x", hres);
            nlsSub.DonePartying();
            break;
        }
    }

    const NLS_STR *apnls[] = { &nlsSub, NULL };

    MsgBox(hwndParent, idMsg, MB_OK | MB_ICONSTOP, apnls);
}


const UINT MAX_WIZ_PAGES = 8;

#if 0   /* now in mslocusr.h */
class CWizData : public IUserProfileInit
{
public:
    HRESULT m_hresRatings;          /* result of VerifySupervisorPassword("") */
    BOOL m_fGoMultiWizard;          /* TRUE if this is the big go-multiuser wizard */
    NLS_STR m_nlsSupervisorPassword;
    NLS_STR m_nlsUsername;
    NLS_STR m_nlsUserPassword;
    IUserDatabase *m_pDB;
    IUser *m_pUserToClone;
    int m_idPrevPage;               /* ID of page before Finish */
    UINT m_cRef;
    DWORD m_fdwOriginalPerUserFolders;
    DWORD m_fdwNewPerUserFolders;
    DWORD m_fdwCloneFromDefault;
    BOOL m_fCreatingProfile;

    CWizData();
    ~CWizData();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP PreInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir);
    STDMETHODIMP PostInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir);
};
#endif


CWizData::CWizData()
    : m_nlsSupervisorPassword(),
      m_nlsUsername(),
      m_nlsUserPassword(),
      m_cRef(0),
      m_fdwOriginalPerUserFolders(0),
      m_fdwNewPerUserFolders(0),
      m_fdwCloneFromDefault(0),
      m_fCreatingProfile(FALSE)
{

}


CWizData::~CWizData()
{
    if (m_pUserToClone != NULL)
        m_pUserToClone->Release();
}


STDMETHODIMP CWizData::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IUserProfileInit)) {
        *ppvObj = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    *ppvObj = this;
    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CWizData::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CWizData::Release(void)
{
    ULONG cRef;

    cRef = --m_cRef;

    if (0L == m_cRef) {
        delete this;
    }

    return cRef;
}


void LimitCredentialLength(HWND hDlg, UINT idCtrl)
{
    SendDlgItemMessage(hDlg, idCtrl, EM_LIMITTEXT, (WPARAM)cchMaxUsername, 0);
}


void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPVOID pwd)
{
    if (ppsh->nPages < MAX_WIZ_PAGES)
    {
        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_DEFAULT;
        psp.hInstance   = ::hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc  = pfn;
        psp.lParam      = (LPARAM)pwd;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
            ppsh->nPages++;
    }
}  // AddPage


BOOL CALLBACK IntroDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                break;
            }

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


void GoToPage(HWND hDlg, int idPage)
{
    SetWindowLongPtr(hDlg, DWL_MSGRESULT, idPage);
}


inline void FailChangePage(HWND hDlg)
{
    GoToPage(hDlg, -1);
}


void InitWizDataPtr(HWND hDlg, LPARAM lParam)
{
    CWizData *pwd = (CWizData *)(((LPPROPSHEETPAGE)lParam)->lParam);
    SetWindowLongPtr(hDlg, DWL_USER, (LPARAM)pwd);
}


void InsertControlText(HWND hDlg, UINT idCtrl, const NLS_STR *pnlsInsert)
{
    int cchText = GetWindowTextLength(GetDlgItem(hDlg, IDC_MAIN_CAPTION)) + pnlsInsert->strlen() + 1;
    NLS_STR nlsTemp(MAX_RES_STR_LEN);
    if (nlsTemp.QueryError() == ERROR_SUCCESS) {
        GetDlgItemText(hDlg, idCtrl, nlsTemp.Party(), nlsTemp.QueryAllocSize());
        nlsTemp.DonePartying();
        const NLS_STR *apnls[] = { pnlsInsert, NULL };
        nlsTemp.InsertParams(apnls);
        if (nlsTemp.QueryError() == ERROR_SUCCESS)
            SetDlgItemText(hDlg, idCtrl, nlsTemp.QueryPch());
    }
}


HRESULT GetControlText(HWND hDlg, UINT idCtrl, NLS_STR *pnls)
{
    HWND hCtrl = GetDlgItem(hDlg, idCtrl);
    if (pnls->realloc(GetWindowTextLength(hCtrl) + 1)) {
        GetWindowText(hCtrl, pnls->Party(), pnls->QueryAllocSize());
        pnls->DonePartying();
        return NOERROR;
    }
    return E_OUTOFMEMORY;
}


BOOL CALLBACK EnterCAPWDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            break;

        case PSN_WIZNEXT:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                if (SUCCEEDED(GetControlText(hDlg, IDC_PASSWORD, &pwd->m_nlsSupervisorPassword))) {
                    if (VerifySupervisorPassword(pwd->m_nlsSupervisorPassword.QueryPch()) == S_FALSE) {
                        MsgBox(hDlg, IDS_BADPASSWORD, MB_OK | MB_ICONSTOP);
                        SetErrorFocus(hDlg, IDC_PASSWORD);
                        FailChangePage(hDlg);
                    }
                }
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        InitWizDataPtr(hDlg, lParam);
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


BOOL CALLBACK EnterUserPWDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }
            break;

        case PSN_WIZNEXT:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                if (SUCCEEDED(GetControlText(hDlg, IDC_PASSWORD, &pwd->m_nlsUserPassword))) {
                    HANDLE hPWL = NULL;
                    if (FAILED(::GetUserPasswordCache(pwd->m_nlsUsername.QueryPch(),
                                                      pwd->m_nlsUserPassword.QueryPch(),
                                                      &hPWL, TRUE))) {
                        MsgBox(hDlg, IDS_BADPASSWORD, MB_OK | MB_ICONSTOP);
                        SetErrorFocus(hDlg, IDC_PASSWORD);
                        FailChangePage(hDlg);
                    }
                    else {
                        if (FAILED(pwd->m_hresRatings))
                            pwd->m_nlsSupervisorPassword = pwd->m_nlsUserPassword;
                        pwd->m_idPrevPage = IDD_EnterUserPassword;
                        ::ClosePasswordCache(hPWL, TRUE);
                        int idNextPage;
                        if (pwd->m_fCreatingProfile)
                            idNextPage = IDD_ChooseFoldersWiz;
                        else
                            idNextPage = (pwd->m_fGoMultiWizard) ? IDD_FinishGoMulti : IDD_FinishAddUser;
                        GoToPage(hDlg, idNextPage);
                    }
                }
            }
            break;

        case PSN_WIZBACK:
            GoToPage(hDlg, IDD_EnterUsername);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
            InsertControlText(hDlg, IDC_MAIN_CAPTION, &pwd->m_nlsUsername);

            LimitCredentialLength(hDlg, IDC_PASSWORD);

            if (FAILED(pwd->m_hresRatings)) {
                NLS_STR nlsTemp(MAX_RES_STR_LEN);
                if (nlsTemp.QueryError() == ERROR_SUCCESS) {
                    nlsTemp.LoadString(IDS_RATINGS_PW_COMMENT);
                    if (nlsTemp.QueryError() == ERROR_SUCCESS)
                        SetDlgItemText(hDlg, IDC_RATINGS_PW_COMMENT, nlsTemp.QueryPch());
                }
            }
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


BOOL CALLBACK EnterUsernameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }
            break;

        case PSN_WIZNEXT:
            {
                int cchUsername = GetWindowTextLength(GetDlgItem(hDlg, IDC_USERNAME));
                if (!cchUsername) {
                    MsgBox(hDlg, IDS_BLANK_USERNAME, MB_OK | MB_ICONSTOP);
                    SetErrorFocus(hDlg, IDC_USERNAME, FALSE);
                    FailChangePage(hDlg);
                }
                else {
                    CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                    if (SUCCEEDED(GetControlText(hDlg, IDC_USERNAME, &pwd->m_nlsUsername))) {
                        /* If we're in the add-user wizard, fail if the user
                         * already exists.  In the go-multiuser wizard, we
                         * just use the info to determine whether to offer
                         * the folder-personalization page.
                         */
                        IUser *pUser = NULL;
                        if (SUCCEEDED(pwd->m_pDB->GetUser(pwd->m_nlsUsername.QueryPch(), &pUser))) {
                            pUser->Release();
                            if (!pwd->m_fGoMultiWizard) {
                                const NLS_STR *apnls[] = { &pwd->m_nlsUsername, NULL };
                                if (MsgBox(hDlg, IDS_USER_EXISTS, MB_OKCANCEL | MB_ICONSTOP, apnls) == IDCANCEL) {
                                    PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                                    break;
                                }
                                SetErrorFocus(hDlg, IDC_USERNAME, FALSE);
                                FailChangePage(hDlg);
                                break;
                            }
                            else {
                                pwd->m_fCreatingProfile = FALSE;
                            }
                        }
                        else {
                            pwd->m_fCreatingProfile = TRUE;
                        }

                        /* See if the user already has a PWL.  If not, next
                         * page is to enter and confirm a password.  If there
                         * is a PWL and its password is non-blank, next page
                         * is simply to enter the password.  If there's a PWL
                         * and its password is blank, next page is Finish.
                         */
                        int idNextPage;
                        HANDLE hPWL = NULL;
                        HRESULT hres = ::GetUserPasswordCache(pwd->m_nlsUsername.QueryPch(),
                                                              szNULL, &hPWL, FALSE);
                        if (SUCCEEDED(hres)) {
                            ::ClosePasswordCache(hPWL, TRUE);
                            pwd->m_idPrevPage = IDD_EnterUsername;
                            if (pwd->m_fCreatingProfile)
                                idNextPage = IDD_ChooseFoldersWiz;
                            else
                                idNextPage = (pwd->m_fGoMultiWizard) ? IDD_FinishGoMulti : IDD_FinishAddUser;
                        }
                        else if (hres == HRESULT_FROM_WIN32(IERR_IncorrectUsername)) {
                            idNextPage = IDD_EnterUserPassword;
                        }
                        else {
                            idNextPage = IDD_NewUserPassword;
                        }
                        GoToPage(hDlg, idNextPage);
                    }
                }
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

            LimitCredentialLength(hDlg, IDC_USERNAME);

            if (pwd->m_fGoMultiWizard) {
                NLS_STR nlsText(MAX_RES_STR_LEN);
                if (nlsText.QueryError() == ERROR_SUCCESS) {
                    nlsText.LoadString(IDS_ENTER_FIRST_USERNAME);
                    if (nlsText.QueryError() == ERROR_SUCCESS)
                        SetDlgItemText(hDlg, IDC_MAIN_CAPTION, nlsText.QueryPch());
                }
            }
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


void PickUserSelected(HWND hwndLB, int iItem)
{
    HWND hDlg = GetParent(hwndLB);

    if (iItem == LB_ERR) {
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
    }
    else {
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
    }
}


BOOL CALLBACK PickUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
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
                PickUserSelected((HWND)lParam, iItem);
            }
            break;

        case PSN_WIZNEXT:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                int iItem = (int)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETCURSEL, 0, 0);
                if (iItem != LB_ERR) {
                    if (pwd->m_pUserToClone != NULL)
                        pwd->m_pUserToClone->Release();
                    pwd->m_pUserToClone = (IUser *)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETITEMDATA, iItem, 0);
                    if (pwd->m_pUserToClone != NULL)
                        pwd->m_pUserToClone->AddRef();
                }
                else {
                    MsgBox(hDlg, IDS_PICK_USERNAME, MB_OK | MB_ICONSTOP);
                    FailChangePage(hDlg);
                }
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)(((LPPROPSHEETPAGE)lParam)->lParam);
            FillUserList(GetDlgItem(hDlg, IDC_USERNAME), pwd->m_pDB, NULL,
                         TRUE, PickUserSelected);
        }
        break;

    case WM_DESTROY:
        DestroyUserList(GetDlgItem(hDlg, IDC_USERNAME));
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_USERNAME && HIWORD(wParam) == LBN_SELCHANGE) {
            int iItem = (int)::SendDlgItemMessage(hDlg, IDC_USERNAME, LB_GETCURSEL, 0, 0);
            PickUserSelected((HWND)lParam, iItem);
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


BOOL CALLBACK NewPasswordDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }
            break;

        case PSN_WIZNEXT:
            {
                int cchPassword = GetWindowTextLength(GetDlgItem(hDlg, IDC_PASSWORD));

                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                GetControlText(hDlg, IDC_PASSWORD, &pwd->m_nlsUserPassword);

                BOOL fConfirmedOK = FALSE;
                int cchConfirm = GetWindowTextLength(GetDlgItem(hDlg, IDC_CONFIRM_PASSWORD));
                if (cchConfirm == cchPassword) {
                    NLS_STR nlsConfirm(cchConfirm+1);
                    if (SUCCEEDED(GetControlText(hDlg, IDC_CONFIRM_PASSWORD, &nlsConfirm))) {
                        if (!nlsConfirm.strcmp(pwd->m_nlsUserPassword))
                            fConfirmedOK = TRUE;
                    }
                }

                if (!fConfirmedOK) {
                    MsgBox(hDlg, IDS_NO_MATCH, MB_OK | MB_ICONSTOP);
                    SetDlgItemText(hDlg, IDC_PASSWORD, szNULL);
                    SetDlgItemText(hDlg, IDC_CONFIRM_PASSWORD, szNULL);
                    SetErrorFocus(hDlg, IDC_PASSWORD);
                    FailChangePage(hDlg);
                }
                else {
                    pwd->m_idPrevPage = IDD_NewUserPassword;
                    UINT idNextPage;
                    if (pwd->m_fCreatingProfile)
                        idNextPage = IDD_ChooseFoldersWiz;
                    else
                        idNextPage = pwd->m_fGoMultiWizard ? IDD_FinishGoMulti : IDD_FinishAddUser;
                    GoToPage(hDlg, idNextPage);
                }
            }
            break;

        case PSN_WIZBACK:
            GoToPage(hDlg, IDD_EnterUsername);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

            LimitCredentialLength(hDlg, IDC_PASSWORD);
            LimitCredentialLength(hDlg, IDC_CONFIRM_PASSWORD);

            if (pwd->m_fGoMultiWizard) {
                NLS_STR nlsText(MAX_RES_STR_LEN);
                if (nlsText.QueryError() == ERROR_SUCCESS) {
                    nlsText.LoadString(IDS_YOURNEWPASSWORD);
                    if (nlsText.QueryError() == ERROR_SUCCESS)
                        SetDlgItemText(hDlg, IDC_MAIN_CAPTION, nlsText.QueryPch());
                    if (FAILED(pwd->m_hresRatings)) {
                        nlsText.LoadString(IDS_RATINGS_PW_COMMENT);
                        if (nlsText.QueryError() == ERROR_SUCCESS)
                            SetDlgItemText(hDlg, IDC_RATINGS_PW_COMMENT, nlsText.QueryPch());
                    }
                }
            }
            else {
                InsertControlText(hDlg, IDC_MAIN_CAPTION, &pwd->m_nlsUsername);
            }
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


const TCHAR c_szExplorerUSFKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
const TCHAR c_szExplorerSFKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
const struct FolderDescriptor {
    UINT idsDirName;        /* Resource ID for directory name */
    LPCTSTR pszRegValue;    /* Name of reg value to set path in */
    LPCTSTR pszStaticName;  /* Static name for ProfileReconciliation subkey */
    LPCTSTR pszFiles;       /* Spec for which files should roam */
    DWORD dwAttribs;        /* Desired attributes */
    BOOL fSecondary : 1;    /* TRUE if subsidiary to the Start Menu */
    BOOL fDefaultInRoot : 1;/* TRUE if default is root of drive, not windir */
} aFolders[] = {

    /* NOTE: Keep the entries in the following table in the same order as the
     * corresponding FOLDER_XXXXXX bitflags in mslocusr.h.
     */

    { IDS_CSIDL_DESKTOP_L,  "Desktop",   "Desktop",   "*.*", FILE_ATTRIBUTE_DIRECTORY, 0, 0 },
    { IDS_CSIDL_NETHOOD_L,  "NetHood",   "NetHood",   "*.*", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN, 0, 0 },
    { IDS_CSIDL_RECENT_L,   "Recent",    "Recent",    "*.*", FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN, 0, 0 },
    { IDS_CSIDL_STARTMENU_L,"Start Menu","Start Menu","*.lnk,*.pif,*.url", FILE_ATTRIBUTE_DIRECTORY, 0, 0 },
    { IDS_CSIDL_PROGRAMS_L, "Programs",  "Programs",  "*.lnk,*.pif,*.url", FILE_ATTRIBUTE_DIRECTORY, 1, 0 },
    { IDS_CSIDL_STARTUP_L,  "Startup",   "Startup",   "*.lnk,*.pif,*.url", FILE_ATTRIBUTE_DIRECTORY, 1, 0 },
    { IDS_CSIDL_FAVORITES_L,"Favorites", "Favorites", "*.*", FILE_ATTRIBUTE_DIRECTORY, 0, 0 },
    { IDS_CSIDL_CACHE_L,    "Cache",     "Cache",     "",    FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM, 0, 0 },
    { IDS_CSIDL_PERSONAL_L, "Personal",  "Personal",  "*.*", FILE_ATTRIBUTE_DIRECTORY, 0, 1 },
};

const struct FolderDescriptor fdChannels =
    { IDS_CSIDL_CHANNELS_L, NULL, "Channel Preservation Key", "*.*", FILE_ATTRIBUTE_DIRECTORY, 0, 0 };

void InitFolderCheckboxes(HWND hDlg, CWizData *pwd)
{
    IUser *pUserToClone;

    pwd->m_fdwOriginalPerUserFolders = 0;

    if (pwd->m_pUserToClone != NULL) {
        pUserToClone = pwd->m_pUserToClone;
        pUserToClone->AddRef();
    }
    else {
        pwd->m_pDB->GetSpecialUser(GSU_DEFAULT, &pUserToClone);
    }

    HKEY hkeyUser;
    if (pUserToClone != NULL && SUCCEEDED(pUserToClone->LoadProfile(&hkeyUser))) {
        HKEY hkeyProfRec, hkeyProfRec2;
        if (RegOpenKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation",
                         0, KEY_READ, &hkeyProfRec) != ERROR_SUCCESS)
            hkeyProfRec = NULL;
        if (RegOpenKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\SecondaryProfileReconciliation",
                         0, KEY_READ, &hkeyProfRec2) != ERROR_SUCCESS)
            hkeyProfRec2 = NULL;

        for (UINT iFolder = 0; iFolder < ARRAYSIZE(aFolders); iFolder++) {
            HKEY hkeyTemp;
            HKEY hkeyParent = aFolders[iFolder].fSecondary ? hkeyProfRec2 : hkeyProfRec;

            if (hkeyParent != NULL &&
                RegOpenKeyEx(hkeyParent,
                             aFolders[iFolder].pszStaticName,
                             0, KEY_READ, &hkeyTemp) == ERROR_SUCCESS) {
                RegCloseKey(hkeyTemp);
                pwd->m_fdwOriginalPerUserFolders |= 1 << iFolder;
            }
            /* else bit is already clear */
        }

        if (hkeyProfRec != NULL)
            RegCloseKey(hkeyProfRec);
        if (hkeyProfRec2 != NULL)
            RegCloseKey(hkeyProfRec2);

        pUserToClone->UnloadProfile(hkeyUser);
    }

    if (pUserToClone != NULL)
        pUserToClone->Release();

    CheckDlgButton(hDlg, IDC_CHECK_DESKTOP,
                   (pwd->m_fdwOriginalPerUserFolders & 
                    (FOLDER_DESKTOP | FOLDER_NETHOOD | FOLDER_RECENT)) ? 1 : 0);
    CheckDlgButton(hDlg, IDC_CHECK_STARTMENU,
                   (pwd->m_fdwOriginalPerUserFolders & 
                    (FOLDER_STARTMENU | FOLDER_PROGRAMS | FOLDER_STARTUP)) ? 1 : 0);
    CheckDlgButton(hDlg, IDC_CHECK_FAVORITES,
                   (pwd->m_fdwOriginalPerUserFolders & FOLDER_FAVORITES) ? 1 : 0);
    CheckDlgButton(hDlg, IDC_CHECK_CACHE,
                   (pwd->m_fdwOriginalPerUserFolders & FOLDER_CACHE) ? 1 : 0);
    CheckDlgButton(hDlg, IDC_CHECK_MYDOCS,
                   (pwd->m_fdwOriginalPerUserFolders & FOLDER_MYDOCS) ? 1 : 0);
}


BOOL CALLBACK ChooseFoldersDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

                InitFolderCheckboxes(hDlg, pwd);
            }
            break;

        case PSN_WIZNEXT:
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

                pwd->m_idPrevPage = IDD_ChooseFoldersWiz;
                GoToPage(hDlg, pwd->m_fGoMultiWizard ? IDD_FinishGoMulti : IDD_FinishAddUser);
            }
            break;

        case PSN_WIZBACK:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                GoToPage(hDlg, pwd->m_idPrevPage);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);

            CheckRadioButton(hDlg, IDC_RADIO_COPY, IDC_RADIO_EMPTY, IDC_RADIO_COPY);
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


void GetWindowsRootPath(LPSTR pszBuffer, UINT cchBuffer)
{
    GetWindowsDirectory(pszBuffer, cchBuffer);

    LPSTR pszEnd = NULL;
    if (*pszBuffer == '\\' && *(pszBuffer+1) == '\\') {
        pszEnd = ::strchrf(pszBuffer+2, '\\');
        if (pszEnd != NULL) {
            pszEnd = ::strchrf(pszEnd+1, '\\');
            if (pszEnd != NULL)
                pszEnd++;
        }
    }
    else {
        LPSTR pszNext = CharNext(pszBuffer);
        if (*pszNext == ':' && *(pszNext+1) == '\\')
            pszEnd = pszNext + 2;
    }
    if (pszEnd != NULL)
        *pszEnd = '\0';
    else
        AddBackslash(pszBuffer);
}


void AddProfRecKey(HKEY hkeyUser, HKEY hkeyProfRec, const FolderDescriptor *pFolder,
                   BOOL fCloneFromDefault, LPCSTR pszProfileDir)
{
    TCHAR szDefaultPath[MAX_PATH];

    if (pFolder->fDefaultInRoot)
        GetWindowsRootPath(szDefaultPath, ARRAYSIZE(szDefaultPath));
    else
        ::strcpyf(szDefaultPath, TEXT("*windir\\"));

    LPTSTR pszEnd = szDefaultPath + ::strlenf(szDefaultPath);
    LoadString(::hInstance, pFolder->idsDirName,
               pszEnd, ARRAYSIZE(szDefaultPath) - (pszEnd - szDefaultPath));
    LPTSTR pszLastComponent = ::strrchrf(pszEnd, '\\');
    if (pszLastComponent == NULL)
        pszLastComponent = pszEnd;
    else
        pszLastComponent++;

    HKEY hSubKey;

    LONG err = RegCreateKeyEx(hkeyProfRec, pFolder->pszStaticName, 0, NULL, REG_OPTION_NON_VOLATILE, 
                              KEY_WRITE, NULL, &hSubKey, NULL);
    if (err == ERROR_SUCCESS) {
        err = RegSetValueEx(hSubKey, "CentralFile", 0, REG_SZ,
                            (LPBYTE)pszLastComponent, ::strlenf(pszLastComponent)+1);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, "LocalFile", 0, REG_SZ, (LPBYTE)pszEnd,
                                ::strlenf(pszEnd)+1);
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, "Name", 0, REG_SZ, (LPBYTE)pFolder->pszFiles,
                                ::strlenf(pFolder->pszFiles) + 1);

        if (fCloneFromDefault && err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, "DefaultDir", 0, REG_SZ, (LPBYTE)szDefaultPath,
                                ::strlenf(szDefaultPath) + 1);

        DWORD dwOne = 1;
        if (err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, "MustBeRelative", 0, REG_DWORD, (LPBYTE)&dwOne,
                                sizeof(dwOne));
        if (fCloneFromDefault && err == ERROR_SUCCESS)
            err = RegSetValueEx(hSubKey, "Default", 0, REG_DWORD, (LPBYTE)&dwOne,
                                sizeof(dwOne));

        if (pFolder->pszRegValue != NULL) {
            if (err == ERROR_SUCCESS)
                err = RegSetValueEx(hSubKey, "RegKey", 0, REG_SZ, (LPBYTE)c_szExplorerUSFKey,
                                    ::strlenf(c_szExplorerUSFKey) + 1);
            if (err == ERROR_SUCCESS)
                err = RegSetValueEx(hSubKey, "RegValue", 0, REG_SZ, (LPBYTE)pFolder->pszRegValue,
                                    ::strlenf(pFolder->pszRegValue) + 1);
        }

        if (err == ERROR_SUCCESS && pFolder->fSecondary) {
            err = RegSetValueEx(hSubKey, "ParentKey", 0, REG_SZ, (LPBYTE)"Start Menu", 11);
        }

        RegCloseKey(hSubKey);
    }

    /* And if we're not adding a clone-from-default profrec key, we know that
     * no profile code is going to create the directory, so we'd better do it
     * ourselves, and set the path in the registry.
     */

    if (!fCloneFromDefault) {
        NLS_STR nlsTemp(MAX_PATH);
        if (nlsTemp.QueryError() == ERROR_SUCCESS) {
            nlsTemp = pszProfileDir;
            AddBackslash(nlsTemp);
            nlsTemp.strcat(pszEnd);

            HKEY hkeyExplorer;
            if (RegOpenKeyEx(hkeyUser, c_szExplorerSFKey, 0,
                             KEY_READ | KEY_WRITE, &hkeyExplorer) == ERROR_SUCCESS) {
                RegSetValueEx(hkeyExplorer, pFolder->pszRegValue,
                              0, REG_SZ, (LPBYTE)nlsTemp.QueryPch(),
                              nlsTemp.strlen()+1);
                RegCloseKey(hkeyExplorer);
            }
            if (RegOpenKeyEx(hkeyUser, c_szExplorerUSFKey, 0,
                             KEY_READ | KEY_WRITE, &hkeyExplorer) == ERROR_SUCCESS) {
                RegSetValueEx(hkeyExplorer, pFolder->pszRegValue,
                              0, REG_SZ, (LPBYTE)nlsTemp.QueryPch(),
                              nlsTemp.strlen()+1);
                RegCloseKey(hkeyExplorer);
            }
            CreateDirectoryPath(nlsTemp.QueryPch());
        }
    }
}


/* CWizData::PreInitProfile is called back by IUserDatabase::Install or
 * ::AddUser after the new user's profile has been created but before
 * directory reconciliation takes place.  This is our chance to add
 * roaming keys for any folders that we want to be per-user and initialized
 * from the defaults, and remove roaming keys for other folders we know about.
 */
STDMETHODIMP CWizData::PreInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir)
{
    HKEY hkeyProfRec, hkeyProfRec2;
    DWORD dwDisp;
    if (RegCreateKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation",
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkeyProfRec, &dwDisp) != ERROR_SUCCESS)
        hkeyProfRec = NULL;
    if (RegCreateKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\SecondaryProfileReconciliation",
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkeyProfRec2, &dwDisp) != ERROR_SUCCESS)
        hkeyProfRec2 = NULL;

    m_fChannelHack = FALSE;

    DWORD fdwFlag = 1;

    for (UINT iFolder = 0;
         iFolder < ARRAYSIZE(aFolders);
         iFolder++, fdwFlag <<= 1) {

        BOOL fWasPerUser = (m_fdwOriginalPerUserFolders & fdwFlag);
        BOOL fMakePerUser = (m_fdwNewPerUserFolders & fdwFlag);
        BOOL fCloneFromDefault = (m_fdwCloneFromDefault & fdwFlag);

        /* If the folder was shared and is staying that way, do nothing. */
        if (!fWasPerUser && !fMakePerUser)
            continue;

        /* If the folder was per-user and is staying that way, do nothing,
         * UNLESS we're creating a new profile and the user chose the start-
         * out-empty option.  In that case we want to make sure to kill the
         * profrec keys until PostInitProfile.
         */
        if (fWasPerUser && fMakePerUser && (!m_fCreatingProfile || fCloneFromDefault))
            continue;

        HKEY hkeyParent = aFolders[iFolder].fSecondary ? hkeyProfRec2 : hkeyProfRec;

        /* If the user is making a folder per-user when it wasn't, and they
         * want this folder cloned from the default, add a profrec key now.
         */
        if (fMakePerUser && fCloneFromDefault) {
            AddProfRecKey(hkeyUser, hkeyParent, &aFolders[iFolder], TRUE, pszProfileDir);
        }

        /* If the user is making a folder shared, or they want this per-user
         * folder to start out empty, delete the profrec key now.  In the
         * latter case, we will add it for roaming purposes during
         * PostInitProfile.
         */
        if (!fMakePerUser || !fCloneFromDefault) {

            RegDeleteKey(hkeyParent, aFolders[iFolder].pszStaticName);

            /* If we're making a folder shared and we're not cloning the
             * default profile, then the profile has a per-user directory
             * path in it which we need to clear out.
             *
             * We know that we need to delete the value from User Shell Folders,
             * and set the default path under Shell Folders.
             */

            if (!fMakePerUser && m_pUserToClone != NULL) {

                TCHAR szDefaultPath[MAX_PATH+1];

                if (aFolders[iFolder].fDefaultInRoot) {
                    GetWindowsRootPath(szDefaultPath, ARRAYSIZE(szDefaultPath));
                }
                else {
                    GetWindowsDirectory(szDefaultPath, ARRAYSIZE(szDefaultPath));
                    AddBackslash(szDefaultPath);
                }
                LPTSTR pszEnd = szDefaultPath + ::strlenf(szDefaultPath);

                LoadString(::hInstance, aFolders[iFolder].idsDirName,
                           pszEnd, ARRAYSIZE(szDefaultPath) - (pszEnd - szDefaultPath));

                HKEY hkeyExplorer;

                if (RegOpenKeyEx(hkeyUser, c_szExplorerUSFKey, 0,
                                 KEY_READ | KEY_WRITE, &hkeyExplorer) == ERROR_SUCCESS) {
                    if (aFolders[iFolder].fDefaultInRoot) {
                        RegSetValueEx(hkeyExplorer, aFolders[iFolder].pszRegValue,
                                      0, REG_SZ, (LPBYTE)szDefaultPath,
                                      ::strlenf(szDefaultPath)+1);
                    }
                    else {
                        RegDeleteValue(hkeyExplorer, aFolders[iFolder].pszRegValue);
                    }
                    RegCloseKey(hkeyExplorer);
                }

                if (RegOpenKeyEx(hkeyUser, c_szExplorerSFKey, 0,
                                 KEY_READ | KEY_WRITE, &hkeyExplorer) == ERROR_SUCCESS) {
                    RegSetValueEx(hkeyExplorer, aFolders[iFolder].pszRegValue,
                                  0, REG_SZ, (LPBYTE)szDefaultPath,
                                  ::strlenf(szDefaultPath)+1);
                    RegCloseKey(hkeyExplorer);
                }
            }
        }

        /* Special code for start-out-empty folders:  Some of them need to
         * start out with certain crucial files, not totally empty.
         */

        if (fMakePerUser &&
            (!fWasPerUser || m_fCreatingProfile) &&
            !fCloneFromDefault) {

            /* Special hack for channels: If the user wants Favorites to be per-user,
             * but chooses to start it out empty, they get no channels either, because
             * Channels is a subdirectory of Favorites.  So, for that case only,
             * we force in a clone-from-default profrec key for Channels, which we'll
             * delete in PostInit.
             */

            if (fdwFlag == FOLDER_FAVORITES) {
                AddProfRecKey(hkeyUser, hkeyProfRec, &fdChannels, TRUE, pszProfileDir);
                m_fChannelHack = TRUE;
            }
        }
    }

    if (hkeyProfRec != NULL)
        RegCloseKey(hkeyProfRec);
    if (hkeyProfRec2 != NULL)
        RegCloseKey(hkeyProfRec2);

    return S_OK;
}


/* CWizData::PostInitProfile is called by IUserDatabase::Install or ::AddUser
 * after the user's folders have all been created and initialized.  Here we
 * add profrec keys for any folders which we want to be per-user but don't
 * want initialized from the default.
 */
STDMETHODIMP CWizData::PostInitProfile(HKEY hkeyUser, LPCSTR pszProfileDir)
{
    HKEY hkeyProfRec, hkeyProfRec2;
    DWORD dwDisp;
    if (RegCreateKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation",
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkeyProfRec, &dwDisp) != ERROR_SUCCESS)
        hkeyProfRec = NULL;
    if (RegCreateKeyEx(hkeyUser, "Software\\Microsoft\\Windows\\CurrentVersion\\SecondaryProfileReconciliation",
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkeyProfRec2, &dwDisp) != ERROR_SUCCESS)
        hkeyProfRec2 = NULL;

    DWORD fdwFlag = 1;
    for (UINT iFolder = 0;
         iFolder < ARRAYSIZE(aFolders);
         iFolder++, fdwFlag <<= 1) {

        HKEY hkeyParent = aFolders[iFolder].fSecondary ? hkeyProfRec2 : hkeyProfRec;

        if (m_fdwNewPerUserFolders & fdwFlag) {
            /* If the user is making a folder per-user when it wasn't or making a
             * folder per-user in a new profile, and they want this folder to start 
             * out empty, add a profrec key now.
             */
            if ((!(m_fdwOriginalPerUserFolders & fdwFlag) || m_fCreatingProfile) &&
                !(m_fdwCloneFromDefault & fdwFlag)) {
                AddProfRecKey(hkeyUser, hkeyParent, &aFolders[iFolder], FALSE, pszProfileDir);
            }

            /* If the folder is per-user and is supposed to have special
             * attributes, make sure it has them.
             */
            if (aFolders[iFolder].dwAttribs != FILE_ATTRIBUTE_DIRECTORY) {
                RegEntry re(aFolders[iFolder].pszStaticName, hkeyParent);
                NLS_STR nlsTemp(MAX_PATH);
                if (re.GetError() == ERROR_SUCCESS && nlsTemp.QueryError() == ERROR_SUCCESS) {
                    GetSetRegistryPath(hkeyUser, re, &nlsTemp, FALSE);
                    if (nlsTemp.strlen()) {
                        ::SetFileAttributes(nlsTemp.QueryPch(), aFolders[iFolder].dwAttribs);
                    }
                }
            }
        }
    }

    /* If we added a hack for channels, undo that hack now that we're done
     * initializing the profile.
     */
    if (m_fChannelHack)
        RegDeleteKey(hkeyProfRec, fdChannels.pszStaticName);

    if (hkeyProfRec != NULL)
        RegCloseKey(hkeyProfRec);
    if (hkeyProfRec2 != NULL)
        RegCloseKey(hkeyProfRec2);

    return S_OK;
}


/* Following is actually for the CPL version of the choose-folders dialog */
HRESULT ChooseFoldersProgressFunc(LPARAM lParam)
{
    CWizData *pwd = (CWizData *)lParam;
    HKEY hkeyUser;

    if (pwd->m_pUserToClone != NULL &&
        SUCCEEDED(pwd->m_pUserToClone->LoadProfile(&hkeyUser))) {

        TCHAR szProfileDir[MAX_PATH];
        DWORD cbBuffer = sizeof(szProfileDir);
        pwd->m_pUserToClone->GetProfileDirectory(szProfileDir, &cbBuffer);

        pwd->PreInitProfile(hkeyUser, szProfileDir);

        NLS_STR nlsPath(szProfileDir);
        AddBackslash(nlsPath);

        DWORD fdwFlag = 1;
        for (UINT iFolder = 0;
             iFolder < ARRAYSIZE(aFolders);
             iFolder++, fdwFlag <<= 1) {

            /* Do reconciliation if we want to per-user-ize a folder that 
             * wasn't per-user before and we want to clone it from default.
             */
            if (!(pwd->m_fdwOriginalPerUserFolders & fdwFlag) &&
                (pwd->m_fdwNewPerUserFolders & fdwFlag) &&
                (pwd->m_fdwCloneFromDefault & fdwFlag)) {
                DefaultReconcileKey(hkeyUser, nlsPath,
                                    aFolders[iFolder].pszStaticName,
                                    aFolders[iFolder].fSecondary);
            }
        }

        /* Process the initialization hack for Channels, if it exists. */
        if (pwd->m_fChannelHack)
            DefaultReconcileKey(hkeyUser, nlsPath, fdChannels.pszStaticName,
                                fdChannels.fSecondary);

        pwd->PostInitProfile(hkeyUser, szProfileDir);

        pwd->m_pUserToClone->UnloadProfile(hkeyUser);

        SHFlushSFCache();

        return S_OK;
    }
    return E_FAIL;
}


void FinishChooseFolders(HWND hDlg, CWizData *pwd)
{
    if (SUCCEEDED(CallWithinProgressDialog(hDlg, IDD_CreateProgress,    
                                           ChooseFoldersProgressFunc, (LPARAM)pwd)))
        EndDialog(hDlg, TRUE);
}


HRESULT InstallProgressFunc(LPARAM lParam)
{
    CWizData *pwd = (CWizData *)lParam;

    return pwd->m_pDB->Install(pwd->m_nlsUsername.QueryPch(),
                               pwd->m_nlsUserPassword.QueryPch(),
                               pwd->m_nlsSupervisorPassword.QueryPch(),
                               pwd);
}


BOOL FinishGoMulti(HWND hDlg, CWizData *pwd)
{
    DWORD dwExitCode = 0xffffffff;  /* not a valid EWX_ value */

    /* If user profiles aren't enabled, enable them.  Using them requires
     * logging off.
     */
    if (!UseUserProfiles()) {
        dwExitCode = EWX_LOGOFF;
        EnableProfiles();
    }

    /* If there is no primary logon provider, install our logon dialog as
     * the primary.  Using this requires rebooting the system.
     */
    if (InstallLogonDialog()) {
        dwExitCode = EWX_REBOOT;
    }

    pwd->m_nlsUserPassword.strupr();
    HRESULT hres = CallWithinProgressDialog(hDlg, IDD_CreateProgress,
                                            InstallProgressFunc, (LPARAM)pwd);
    if (SUCCEEDED(hres)) {
        /* Set the new username as the default one to log on as. */
        HKEY hkeyLogon;
        if (OpenLogonKey(&hkeyLogon) == ERROR_SUCCESS) {
            pwd->m_nlsUsername.ToOEM();
            RegSetValueEx(hkeyLogon, ::szUsername, 0, REG_SZ,
                          (LPBYTE)pwd->m_nlsUsername.QueryPch(),
                          pwd->m_nlsUsername.strlen() + 1);
            pwd->m_nlsUsername.ToAnsi();
            RegCloseKey(hkeyLogon);
        }
        if ((dwExitCode != 0xffffffff) &&
            MsgBox(hDlg, IDS_GO_MULTI_RESTART, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            ::ExitWindowsEx(dwExitCode, 0);
            ::ExitProcess(0);
        }
        return TRUE;
    }

    ReportUserError(hDlg, hres);
    return FALSE;
}


HRESULT AddUserProgressFunc(LPARAM lParam)
{
    CWizData *pwd = (CWizData *)lParam;

    return pwd->m_pDB->AddUser(pwd->m_nlsUsername.QueryPch(),
                               pwd->m_pUserToClone, pwd, &pwd->m_pNewUser);
}


BOOL FinishAddUser(HWND hDlg, CWizData *pwd)
{
    pwd->m_nlsUserPassword.strupr();

    pwd->m_pNewUser = NULL;
    HRESULT hres = CallWithinProgressDialog(hDlg, IDD_CreateProgress,
                                            AddUserProgressFunc, (LPARAM)pwd);
    if (SUCCEEDED(hres)) {
        hres = pwd->m_pNewUser->ChangePassword(szNULL, pwd->m_nlsUserPassword.QueryPch());
        pwd->m_pNewUser->Release();
        pwd->m_pNewUser = NULL;
    }
    else {
        ReportUserError(hDlg, hres);
    }

    return SUCCEEDED(hres);
}


BOOL CALLBACK FinishDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
            }
            break;

        case PSN_WIZFINISH:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                BOOL fOK = pwd->m_fGoMultiWizard ? FinishGoMulti(hDlg, pwd) : FinishAddUser(hDlg, pwd);
                if (!fOK)
                    FailChangePage(hDlg);
                else
                    return FALSE;       /* destroy wizard */
            }
            break;

        case PSN_WIZBACK:
            {
                CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
                GoToPage(hDlg, pwd->m_idPrevPage);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        {
            InitWizDataPtr(hDlg, lParam);
            CWizData *pwd = (CWizData *)GetWindowLongPtr(hDlg, DWL_USER);
            if (!pwd->m_fGoMultiWizard) {
                InsertControlText(hDlg, IDC_MAIN_CAPTION, &pwd->m_nlsUsername);
                InsertControlText(hDlg, IDC_MAIN_CAPTION2, &pwd->m_nlsUsername);
            }
        }
        break;

    default:
        return FALSE;

    } // end of switch on message
    return TRUE;
}


STDMETHODIMP CLUDatabase::InstallWizard(HWND hwndParent)
{
#if 0
    if (UseUserProfiles()) {
        MsgBox(hwndParent, IDS_PROFILES_ALREADY_ENABLED, MB_OK | MB_ICONINFORMATION);
        return E_FAIL;
    }
#endif

    if (ProfileUIRestricted()) {
        ReportRestrictionError(hwndParent);
        return E_ACCESSDENIED;
    }

    CWizData wd;
    wd.m_hresRatings = VerifySupervisorPassword(szNULL);
    wd.m_fGoMultiWizard = TRUE;
    wd.m_pDB = this;
    AddRef();           /* just in case */
    wd.m_pUserToClone = NULL;

    LPPROPSHEETHEADER ppsh;

    // Allocate the property sheet header
    //
    if ((ppsh = (LPPROPSHEETHEADER)LocalAlloc(LMEM_FIXED, sizeof(PROPSHEETHEADER)+
                (MAX_WIZ_PAGES * sizeof(HPROPSHEETPAGE)))) != NULL)
    {
        ppsh->dwSize     = sizeof(*ppsh);
        ppsh->dwFlags    = PSH_WIZARD;
        ppsh->hwndParent = hwndParent;
        ppsh->hInstance  = ::hInstance;
        ppsh->pszCaption = NULL;
        ppsh->nPages     = 0;
        ppsh->nStartPage = 0;
        ppsh->phpage     = (HPROPSHEETPAGE *)(ppsh+1);

        /* Add the pages for the wizard.  Note that we have two pages to
         * prompt for the user's account password -- one to enter, the other
         * to enter and confirm.  The code in EnterUsernameDlgProc jumps to
         * the right password page depending on whether the user has a PWL.
         * The code in NewPasswordDlgProc knows to jump ahead to the finish
         * page.
         *
         * If you add more pages here, be sure to update the code as necessary
         * so the sequence is correct.
         */
        AddPage(ppsh, IDD_EnableProfilesIntro, IntroDlgProc, &wd);
        if (wd.m_hresRatings == S_FALSE)
            AddPage(ppsh, IDD_EnterCAPassword, EnterCAPWDlgProc, &wd);
        AddPage(ppsh, IDD_EnterUsername, EnterUsernameDlgProc, &wd);
        AddPage(ppsh, IDD_NewUserPassword, NewPasswordDlgProc, &wd);
        AddPage(ppsh, IDD_EnterUserPassword, EnterUserPWDlgProc, &wd);
        AddPage(ppsh, IDD_ChooseFoldersWiz, ChooseFoldersDlgProc, &wd);
        AddPage(ppsh, IDD_FinishGoMulti, FinishDlgProc, &wd);

        PropertySheet(ppsh);

        LocalFree((HLOCAL)ppsh);
    }

    Release();  /* undo above AddRef() */

    return S_OK;
}


HRESULT DoAddUserWizard(HWND hwndParent, IUserDatabase *pDB,
                        BOOL fPickUserPage, IUser *pUserToClone)
{
    CWizData wd;
    wd.m_hresRatings = VerifySupervisorPassword(szNULL);
    wd.m_fGoMultiWizard = FALSE;
    wd.m_pDB = pDB;
    pDB->AddRef();      /* just in case */
    wd.m_pUserToClone = pUserToClone;
    if (wd.m_pUserToClone != NULL)
        wd.m_pUserToClone->AddRef();

    LPPROPSHEETHEADER ppsh;

    // Allocate the property sheet header
    //
    if ((ppsh = (LPPROPSHEETHEADER)LocalAlloc(LMEM_FIXED, sizeof(PROPSHEETHEADER)+
                (MAX_WIZ_PAGES * sizeof(HPROPSHEETPAGE)))) != NULL)
    {
        ppsh->dwSize     = sizeof(*ppsh);
        ppsh->dwFlags    = PSH_WIZARD;
        ppsh->hwndParent = hwndParent;
        ppsh->hInstance  = ::hInstance;
        ppsh->pszCaption = NULL;
        ppsh->nPages     = 0;
        ppsh->nStartPage = 0;
        ppsh->phpage     = (HPROPSHEETPAGE *)(ppsh+1);

        AddPage(ppsh, IDD_AddUserIntro, IntroDlgProc, &wd);

        if (IsCurrentUserSupervisor(pDB) != S_OK)
        {
            AddPage(ppsh, IDD_EnterCAPassword, EnterCAPWDlgProc, &wd);
        }

        if (fPickUserPage)
            AddPage(ppsh, IDD_PickUser, PickUserDlgProc, &wd);
        AddPage(ppsh, IDD_EnterUsername, EnterUsernameDlgProc, &wd);
        AddPage(ppsh, IDD_NewUserPassword, NewPasswordDlgProc, &wd);
        AddPage(ppsh, IDD_EnterUserPassword, EnterUserPWDlgProc, &wd);
        AddPage(ppsh, IDD_ChooseFoldersWiz, ChooseFoldersDlgProc, &wd);
        AddPage(ppsh, IDD_FinishAddUser, FinishDlgProc, &wd);

        PropertySheet(ppsh);

        LocalFree((HLOCAL)ppsh);
    }

    pDB->Release(); /* undo above AddRef() */

    return S_OK;
}


STDMETHODIMP CLUDatabase::AddUserWizard(HWND hwndParent)
{
    if (ProfileUIRestricted()) {
        ReportRestrictionError(hwndParent);
        return E_ACCESSDENIED;
    }

    return DoAddUserWizard(hwndParent, this, TRUE, NULL);
}


extern "C" void InstallWizard(HWND hwndParent, HINSTANCE hinstEXE, LPSTR pszCmdLine, int nCmdShow)
{
    IUserDatabase *pDB;
    if (SUCCEEDED(::CreateUserDatabase(IID_IUserDatabase, (void **)&pDB))) {
        pDB->InstallWizard(hwndParent);
        pDB->Release();
    }
}


extern "C" void AddUserWizard(HWND hwndParent, HINSTANCE hinstEXE, LPSTR pszCmdLine, int nCmdShow)
{
    IUserDatabase *pDB;
    if (SUCCEEDED(::CreateUserDatabase(IID_IUserDatabase, (void **)&pDB))) {
        pDB->AddUserWizard(hwndParent);
        pDB->Release();
    }
}


struct ProgressDlgData
{
    PFNPROGRESS pfn;
    LPARAM lParam;
    HRESULT hres;
};


void CallProgressFunc(HWND hDlg)
{
    ProgressDlgData *ppdd = (ProgressDlgData *)GetWindowLongPtr(hDlg, DWL_USER);

    ppdd->hres = ppdd->pfn(ppdd->lParam);

    EndDialog(hDlg, FALSE);
}


const UINT WM_CALL_FUNC = WM_USER + 100;

BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWL_USER, lParam);

            /* Defer function call to a posted message so the dialog manager
             * will show our dialog.
             *
             * If PostMessage fails, at least still call the function anyway.
             */
            if (!PostMessage(hDlg, WM_CALL_FUNC, 0, 0)) {
                CallProgressFunc(hDlg);
            }
        }

        return TRUE;        /* we didn't set the focus */

    case WM_CALL_FUNC:
        CallProgressFunc(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


HRESULT CallWithinProgressDialog(HWND hwndOwner, UINT idResource, PFNPROGRESS pfn,
                                 LPARAM lParam)
{
    ProgressDlgData pdd = { pfn, lParam, E_FAIL };

    DialogBoxParam(::hInstance, MAKEINTRESOURCE(idResource), hwndOwner,
                   ProgressDlgProc, (LPARAM)&pdd);

    return pdd.hres;
}


BOOL ProfileUIRestricted(void)
{
    RegEntry rePolicy("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");

    if (rePolicy.GetError() == ERROR_SUCCESS) {
        if (rePolicy.GetNumber("NoProfilePage") != 0)
            return TRUE;
    }
    return FALSE;
}


void ReportRestrictionError(HWND hwndOwner)
{
    MsgBox(hwndOwner, IDS_PROFILE_POLICY, MB_OK | MB_ICONSTOP);
}

