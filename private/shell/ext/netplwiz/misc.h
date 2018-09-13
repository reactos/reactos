#ifndef MISC_H
#define MISC_H

HRESULT ValidateName(LPCTSTR pszName);

HRESULT ParseDisplayNameHelper(HWND hwnd, LPTSTR szPath, LPITEMIDLIST* ppidl);

void FetchText(HWND hWndDlg, UINT uID, LPTSTR lpBuffer, DWORD dwMaxSize);

INT FetchTextLength(HWND hWndDlg, UINT uID);

HRESULT AttemptLookupAccountName(LPCTSTR szUsername, PSID* ppsid,
    LPTSTR szDomain, DWORD* pcchDomain, SID_NAME_USE* psUse);

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...);

BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...);

void EnableControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, BOOL fEnable);

void MakeDomainUserString(LPCTSTR szDomain, LPCTSTR szUsername, LPTSTR szDomainUser, DWORD cchBuffer);
void DomainUserString_GetParts(LPCTSTR szDomainUser, LPTSTR szUser, DWORD cchUser, LPTSTR szDomain, DWORD cchDomain);

BOOL GetCurrentUserAndDomainName(LPTSTR UserName, LPDWORD cchUserName, LPTSTR DomainName, LPDWORD cchDomainName);

HRESULT IsUserLocalAdmin(HANDLE TokenHandle OPTIONAL, BOOL* pfIsAdmin);

BOOL IsComputerInDomain();

void OffsetControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, int dx, int dy);

void OffsetWindow(HWND hwnd, int dx, int dy);

HFONT GetIntroFont(HWND hwnd);

// Stuff for the callback for IShellPropSheetExt::AddPages
#define MAX_PROPSHEET_PAGES     10

struct ADDPROPSHEETDATA
{
    HPROPSHEETPAGE rgPages[MAX_PROPSHEET_PAGES];
    int nPages;
};

BOOL AddPropSheetPageCallback(HPROPSHEETPAGE hpsp, LPARAM lParam);

class CEnsureSingleInstance
{
public:
    CEnsureSingleInstance(LPCTSTR szCaption);
    ~CEnsureSingleInstance();

    BOOL ShouldExit() { return m_fShouldExit;}

private:
    BOOL m_fShouldExit;
    HANDLE m_hEvent;
};

// BrowseForUser
// S_OK = Username/Domain are Ok
// S_FALSE = User clicked cancel
// E_xxx = Error
HRESULT BrowseForUser(HWND hwndDlg, TCHAR* pszUser, DWORD cchUser,
              TCHAR* pszDomain, DWORD cchDomain);

int CALLBACK ShareBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

void RemoveControl(HWND hwnd, UINT idControl, UINT idNextControl, const UINT* prgMoveControls, DWORD cControls, BOOL fShrinkParent);
void MoveControls(HWND hwnd, const UINT* prgControls, DWORD cControls, int dx, int dy);

void EnableDomainForUPN(HWND hwndUsername, HWND hwndDomain);


#endif //!MISC_H