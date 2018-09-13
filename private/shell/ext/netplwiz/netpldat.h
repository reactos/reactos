#ifndef NETPLDAT_H_INCLUDED
#define NETPLDAT_H_INCLUDED

#define NETPLACE_MAX_RESOURCE_NAME (MAX(MAX_PATH, INTERNET_MAX_URL_LENGTH))
#define NETPLACE_MAX_FRIENDLY_NAME (MAX_PATH)

class CNetworkPlaceData
{
public:
    CNetworkPlaceData();
    ~CNetworkPlaceData();

    HRESULT SetResourceName(HWND hwnd, LPCTSTR szResourceName);
    LPCTSTR GetResourceName();

    HRESULT SetFriendlyName(LPCTSTR szFriendlyName);
    LPCTSTR GetFriendlyName();

    HRESULT SetFtpUserPassword(LPCTSTR szUser, LPCTSTR szPassword);

    HRESULT CreatePlace(HWND hwnd);
    HRESULT Cancel();

    HRESULT ConnectToUnc(HWND hwnd);

    typedef enum _PLACETYPE
    {
        PLACETYPE_NONE,
        PLACETYPE_WEBFOLDERURL,
        PLACETYPE_UNCSERVER,
        PLACETYPE_UNCSHARE,
        PLACETYPE_FTP
    } PLACETYPE;

    PLACETYPE GetType() {return m_placetype;}

private:
    HRESULT AddWebFolder_Begin(HWND hwnd, BOOL* pfCustomMessageDisplayed, LPTSTR pszDisplayName, DWORD cchDisplayName);
    HRESULT AddWebFolder_Finish(HWND hwnd, BOOL* pfCustomMessageDisplayed);
    HRESULT AddWebFolder_Cancel();

    HRESULT AddUnc(HWND hwnd, BOOL* pfCustomMessageDisplayed);
    HRESULT AddShortcut(HWND hwnd, BOOL* pfCustomMessageDisplayed);
    HRESULT AddFfpFolder(HWND hwnd, BOOL* pfCustomMessageDisplayed);

    HRESULT CreateDefaultFriendlyName(HWND hwnd);
    HRESULT CheckFriendlyName();

    BOOL IsFriendlyNameTaken();

    HRESULT GetWebFoldersFolder(IShellFolder** ppfolder);
    HRESULT BrowseToPidl(LPCITEMIDLIST pidl);
    HRESULT GetMyNetPlacesItemPidl(HWND hwnd, LPTSTR pszItemName, LPITEMIDLIST* ppidlOut);

    TCHAR m_szResourceName[NETPLACE_MAX_RESOURCE_NAME + 1];
    TCHAR m_szFriendlyName[NETPLACE_MAX_FRIENDLY_NAME + 1];

    // Web folder in process of being created - is valid after a call to AddWebFolder_Begin and
    // before a call to AddWebFolder_Cancel or a successful call to AddWebFolder_Finish.
    LPITEMIDLIST m_pidlNewWebFolder;

    PLACETYPE m_placetype;

    HRESULT SetPlaceType();
    BOOL IsUncServer();
    BOOL IsUncShare();
    BOOL IsFtp();
    BOOL IsWebFolderUrl();

};

struct NETPLACESWIZARDDATA
{
    // The network place object. Manages the creation of the place
    CNetworkPlaceData netplace;

    // Array of dialog template resource ID's - this makes it easier to
    // specify different templates for different wizard types
    UINT idWelcomePage;
    UINT idFoldersPage;
    UINT idPasswordPage;
    UINT idCompletionPage;

    // Does the user need to be shown the folders page
    BOOL fShowFoldersPage;

    // Does the user need to be shown the FTP User login page
    BOOL fShowFTPUserPage;

    // Are we using the small wizard (only last page?)
    BOOL fSmallWizard;

    // Image lists returned from the shell - used to display
    // standard icons
    HIMAGELIST himlSmall;
    HIMAGELIST himlLarge;
        
    // Value to return from the wizard
    DWORD dwReturn;
};

#endif //!NETPLDAT_H_INCLUDED