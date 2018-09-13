//
// Globals and helper macros
//

#define GetDlgItemTextLength(hwnd, id)              \
            GetWindowTextLength(GetDlgItem(hwnd, id))

#define WizardNext(hwnd, to)                        \
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)to)


#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)


//
// IDC_ of networking option chosen, used for flow control
//

extern DWORD g_dwWhichNet;                      
extern UINT g_uWizardIs;

extern BOOL g_fRebootOnExit;        // reboot the mc on exit

extern const WCHAR c_szWinLogon[];                              
extern const WCHAR c_szAutoLogon[];
extern const WCHAR c_szDisableCAD[];                              
extern const WCHAR c_szDefUserName[]; 
extern const WCHAR c_szDefDomainName[];
extern const WCHAR c_szDefPassword[];                               
extern const WCHAR c_szAdminAccount[];

extern BOOL g_fWizardCreatedRASConnection;

//
// default workgroup name
// BUGBUG: read from resource?
//

#define DEFAULT_WORKGROUP   L"WORKGROUP"


//
// structure used to pass around credential information
//

typedef struct
{
    LPWSTR pszUser;
    INT cchUser;
    LPWSTR pszDomain;
    INT cchDomain;
    LPWSTR pszPassword;
    INT cchPassword;
} CREDINFO, * LPCREDINFO;


//
// dlgproc's and functions
//

void SetWizardButtons(HWND hwndPage, DWORD dwButtons);


INT_PTR CALLBACK _IntroDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _HowUseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _DomainInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _WhichNetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _UserInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _CompInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _AddUserDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _PermissionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _WorkgroupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _AutoLogonDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT JoinDomain(HWND hwnd, BOOL fDomain, LPCWSTR pDomain, CREDINFO* pci);

VOID SetAutoLogon(LPCWSTR pszUserName, LPCWSTR pszPassword);
VOID SetDefAccount(LPCWSTR pszUserName, LPCWSTR pszDomain);

int PropertySheetIcon(LPCPROPSHEETHEADER ppsh, LPCTSTR pszIcon);
