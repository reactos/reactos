/********************************************************
 advpg.cpp

  User Manager Advanced Property Page Implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "advpg.h"

// Functions to read/write the "require CAD" value from the register

// Relevant regkeys/regvals
#define REGKEY_WINLOGON         \
 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")

#define REGKEY_WINLOGON_POLICY  \
 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")

#define REGVAL_DISABLE_CAD      TEXT("DisableCAD")


void ReadRequireCad(BOOL* pfRequireCad, BOOL* pfSetInPolicy)
// Returns whether or not users are required to press C-A-D before
// logging in. Also returns whether or not this value was set in
// policy. If it is set by policy, may as well disable the check-box
// in the UI.
{
    TraceEnter(TRACE_USR_CORE, "::ReadRequireCad");

    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    BOOL fDisableCad;
    NT_PRODUCT_TYPE nttype;

    *pfRequireCad = TRUE;
    *pfSetInPolicy = FALSE; 

    RtlGetNtProductType(&nttype);

    // By default, don't require CAD for workstations not
    // on a domain only
    if ((NtProductWinNt == nttype) && !IsComputerInDomain())
    {
        *pfRequireCad = FALSE;
    }

    // Read the setting from the machine preferences
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON, 0, 
        KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(fDisableCad);

        if (ERROR_SUCCESS == RegQueryValueEx (hkey, REGVAL_DISABLE_CAD, NULL, &dwType,
                        (LPBYTE) &fDisableCad, &dwSize))
        {
            *pfRequireCad = !fDisableCad;
        }

        RegCloseKey (hkey);
    }

    // Check if C-A-D is disabled via policy

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON_POLICY, 0, KEY_READ,
                     &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(fDisableCad);

        if (ERROR_SUCCESS == RegQueryValueEx (hkey, REGVAL_DISABLE_CAD, NULL, &dwType,
                            (LPBYTE) &fDisableCad, &dwSize))
        {
            *pfRequireCad = !fDisableCad;
            *pfSetInPolicy = TRUE;
        }

        RegCloseKey (hkey);
    }

    TraceLeaveVoid();
}

void WriteRequireCad(BOOL fRequireCad)
// Write the require C-A-D to log on preference to the registry
// This setting can be overridden by press C-A-D policy if applicable
{
    TraceEnter(TRACE_USR_CORE, "::WriteRequireCad");

    HKEY hkey;
    DWORD dwDisp;
    BOOL fDisableCad = !fRequireCad;

    if (ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON, 0, 
        NULL, 0, KEY_WRITE, NULL, &hkey, &dwDisp))
    {
        RegSetValueEx(hkey, REGVAL_DISABLE_CAD, 0, REG_DWORD,
                        (LPBYTE) &fDisableCad, sizeof(fDisableCad));

        RegCloseKey (hkey);
    }

    TraceLeaveVoid();
}

// Advanced Page Implementation

// Certificate Manager static functions and delay-load stuff
class CCertificateApi
{
public:
    static BOOL Manager(HWND hwnd);
    static BOOL Wizard(HWND hwnd);

private:
    static BOOL         m_fFailed;
    static HINSTANCE    m_hInstCryptUI;
};

BOOL CCertificateApi::m_fFailed = FALSE;
HINSTANCE CCertificateApi::m_hInstCryptUI = NULL;

// Certificate manager API wrapper implementation

// CCertificateApi::Manager - launch certificate manager
typedef BOOL (WINAPI *PFNCRYPTUIDLGCERTMGR)(IN PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr);
BOOL CCertificateApi::Manager(HWND hwnd)
{
    static PFNCRYPTUIDLGCERTMGR pCryptUIDlgCertMgr = NULL;

    if ((m_hInstCryptUI == NULL) && (!m_fFailed))
    {
        m_hInstCryptUI = LoadLibrary(TEXT("cryptui.dll"));
    }

    if (m_hInstCryptUI != NULL)
    {
        pCryptUIDlgCertMgr = (PFNCRYPTUIDLGCERTMGR)
            GetProcAddress(m_hInstCryptUI, "CryptUIDlgCertMgr");
    }

    if (pCryptUIDlgCertMgr)
    {
        CRYPTUI_CERT_MGR_STRUCT ccm = {0};
        ccm.dwSize = sizeof(ccm);
        ccm.hwndParent = hwnd;
        pCryptUIDlgCertMgr(&ccm);
    }
    else
    {
        m_fFailed = TRUE;
    }

    return (!m_fFailed);
}


// CCertificateApi::Wizard - launch the enrollment wizard
typedef BOOL (WINAPI *PFNCRYPTUIWIZCERTREQUEST)(IN DWORD dwFlags,
    IN OPTIONAL HWND, IN OPTIONAL LPCWSTR pwszWizardTitle,
    IN PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
    OUT OPTIONAL PCCERT_CONTEXT *ppCertContext, 
    OUT OPTIONAL DWORD *pCAdwStatus);
BOOL CCertificateApi::Wizard(HWND hwnd)
{
    static PFNCRYPTUIWIZCERTREQUEST pCryptUIWizCertRequest = NULL;

    if ((m_hInstCryptUI == NULL) && (!m_fFailed))
    {
        m_hInstCryptUI = LoadLibrary(TEXT("cryptui.dll"));
    }

    if (m_hInstCryptUI != NULL)
    {
        pCryptUIWizCertRequest = (PFNCRYPTUIWIZCERTREQUEST)
            GetProcAddress(m_hInstCryptUI, "CryptUIWizCertRequest");
    }

    if (pCryptUIWizCertRequest)
    {
        CRYPTUI_WIZ_CERT_REQUEST_INFO           CertRequestInfo = {0}; 
        CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW        CertRequestPvkNew = {0};

        CertRequestInfo.dwSize=sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO);
        CertRequestInfo.dwPurpose=CRYPTUI_WIZ_CERT_ENROLL;
        CertRequestInfo.dwPvkChoice=CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
        CertRequestInfo.pPvkNew=&CertRequestPvkNew;
    
        CertRequestPvkNew.dwSize=sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW); 

        // This can take a while!
        SetCursor(LoadCursor(NULL, IDC_WAIT));

        pCryptUIWizCertRequest(
                        0,            
                        hwnd,       
                        NULL,    
                        &CertRequestInfo, 
                        NULL,     
                        NULL);  
    }
    else
    {
        m_fFailed = TRUE;
    }
    
    return (!m_fFailed);
}

static const DWORD rgHelpIds[] = 
{
    IDC_CERTWIZARD_BUTTON,      IDH_NEW_CERTIFICATE_BUTTON,
    IDC_CERTIFICATE_BUTTON,     IDH_CERTIFICATES_BUTTON,
    IDC_ADVANCED_BUTTON,        IDH_ADVANCED_BUTTON,
    IDC_BOOT_ICON,              IDH_SECUREBOOT_CHECK,
    IDC_BOOT_TEXT,              IDH_SECUREBOOT_CHECK,
    IDC_REQUIRECAD,             IDH_SECUREBOOT_CHECK,
    IDC_CERT_ICON,              (DWORD) -1,
    IDC_CERT_TEXT,              (DWORD) -1,
    0, 0
};

INT_PTR CAdvancedPropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        case WM_HELP: return OnHelp(hwndDlg, (LPHELPINFO) lParam);
        case WM_CONTEXTMENU: return OnContextMenu((HWND) wParam);
    }
    
    return FALSE;
}

BOOL CAdvancedPropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CAdvancedPropertyPage::OnNotify");
    
    BOOL fReturn = FALSE;

    switch (pnmh->code)
    {
    case PSN_APPLY:
        {
            HWND hwndCheck = GetDlgItem(hwnd, IDC_REQUIRECAD);
            TraceAssert(hwndCheck);
            
            BOOL fRequireCad = (BST_CHECKED == Button_GetCheck(hwndCheck));

            // See if a change is really necessary
            BOOL fOldRequireCad;
            BOOL fDummy;

            ReadRequireCad(&fOldRequireCad, &fDummy);

            if (fRequireCad != fOldRequireCad)
            {
                WriteRequireCad(fRequireCad);
                // m_fRebootRequired = TRUE;
                // Uncomment the line above if it ever becomes necessary to reboot the machine - it isn't now.
            }

            // xxx->lParam == 0 means Ok as opposed to Apply
            if ((((PSHNOTIFY*) pnmh)->lParam) && m_fRebootRequired) 
            {
                PropSheet_RebootSystem(GetParent(hwnd));
            }

            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            fReturn = TRUE;
        }
        break;
    }

    TraceLeaveValue(fReturn);
}


BOOL CAdvancedPropertyPage::OnHelp(HWND hwnd, LPHELPINFO pHelpInfo)
{
    TraceEnter(TRACE_USR_CORE, "CAdvancedPropertyPage::OnHelp");

    WinHelp((HWND) pHelpInfo->hItemHandle, m_pData->GetHelpfilePath(), 
        HELP_WM_HELP, (ULONG_PTR) (LPTSTR) rgHelpIds);

    TraceLeaveValue(TRUE);
}

BOOL CAdvancedPropertyPage::OnContextMenu(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CAdvancedPropertyPage::OnContextMenu");

    WinHelp(hwnd, m_pData->GetHelpfilePath(), 
        HELP_CONTEXTMENU, (ULONG_PTR) (LPTSTR) rgHelpIds);

    TraceLeaveValue(TRUE);
}

BOOL CAdvancedPropertyPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CAdvancedPropertyPage::OnInitDialog");

    // Do the required mucking for the Require C-A-D checkbox...
    // Read the setting for the require CAD checkbox
    BOOL fRequireCad;
    BOOL fSetInPolicy;

    ReadRequireCad(&fRequireCad, &fSetInPolicy);

    HWND hwndCheck = GetDlgItem(hwnd, IDC_REQUIRECAD);

    TraceAssert(hwndCheck);

    // Disable the check if set in policy
    EnableWindow(hwndCheck, !fSetInPolicy);

    // Set the check accordingly
    Button_SetCheck(hwndCheck, 
        fRequireCad ? BST_CHECKED : BST_UNCHECKED);
    
    TraceLeaveValue(TRUE);
}

BOOL CAdvancedPropertyPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CAdvancedPropertyPage::OnCommand");
        
    switch (id)
    {
    case IDC_CERTIFICATE_BUTTON:
        {
            CCertificateApi::Manager(hwnd);
        }
        break;

    case IDC_CERTWIZARD_BUTTON:
        {
            CCertificateApi::Wizard(hwnd);
        }
        break;

    case IDC_ADVANCED_BUTTON:
        {
            // Launch the MMC local user manager
            STARTUPINFO startupinfo = {0};
            startupinfo.cb = sizeof (startupinfo);

            PROCESS_INFORMATION process_information;

            static const TCHAR szMMCCommandLine[] = 
                TEXT("mmc.exe %systemroot%\\system32\\lusrmgr.msc computername=localmachine");
            
            TCHAR szExpandedCommandLine[MAX_PATH];

            if (ExpandEnvironmentStrings(szMMCCommandLine, szExpandedCommandLine, 
                ARRAYSIZE(szExpandedCommandLine)) > 0)
            {
                if (CreateProcess(NULL, szExpandedCommandLine, NULL, NULL, FALSE, 0, NULL, NULL,
                    &startupinfo, &process_information))
                {
                    CloseHandle(process_information.hProcess);
                    CloseHandle(process_information.hThread);
                }
            }
        }
        break;

    case IDC_REQUIRECAD:
        {
            TraceAssert(GetParent(hwnd));
            PropSheet_Changed(GetParent(hwnd), hwnd);
        }
    }

    TraceLeaveValue(FALSE);
}
