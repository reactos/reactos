//
// API to install a channel by creating a system folder in the channel directory
//
// Julian Jiggins (julianj), 4th May, 1997
//

typedef enum _tagSUBSCRIPTIONACTION {
    SUBSACTION_SUBSCRIBEONLY,
    SUBSACTION_ADDADDITIONALCOMPONENTS
} SUBSCRIPTIONACTION;

//
// Flags used by SusbcribeToCDF
//

#define     STC_CHANNEL             0x00000001
#define     STC_DESKTOPCOMPONENT    0x00000002
#define     STC_ALL                 0xffffffff

#define     GUID_STR_LEN            80


EXTERN_C STDAPI_(void) OpenChannel
(
    HWND hwndParent,
    HINSTANCE hinst,
    LPSTR pszCmdLine,
    int nShow
);

EXTERN_C STDAPI_(void) Subscribe
(
    HWND hwndParent,
    HINSTANCE hinst,
    LPSTR pszCmdLine,
    int nShow
);

EXTERN_C STDAPI ParseDesktopComponent
(
    HWND hwndOwner,
    LPWSTR wszURL,                                             
    COMPONENT* pInfo
);

EXTERN_C STDAPI SubscribeToCDF
(
    HWND hwndOwner,
    LPWSTR wszURL,
    DWORD dwFlags
);

HRESULT AddChannel
(
    LPCTSTR pszName, 
    LPCTSTR pszURL, 
    LPCTSTR pszLogo, 
    LPCTSTR pszWideLogo, 
    LPCTSTR pszIcon,
    XMLDOCTYPE xdt
);

HRESULT DeleteChannel
(
    LPTSTR pszName
);

HRESULT OpenChannelHelper
(
    LPWSTR wszURL,
    HWND hwndOwner
);

HRESULT NavigateBrowser
(
    IWebBrowser2* pIWebBrowser2,
    LPWSTR wszURL,
    HWND hwnd
);

HRESULT NavigateChannelPane(
    IWebBrowser2* pIWebBrowser2,
    LPCWSTR pwszName
);

BOOL SubscriptionHelper
(
    IXMLDocument *pIXMLDocument,
    HWND hwnd,
    SUBSCRIPTIONTYPE st,
    SUBSCRIPTIONACTION sa,
    LPCWSTR pszwURL,
    XMLDOCTYPE xdt,
    BSTR* pbstrSubscribedURL
);

BOOL SubscribeToURL
(
    ISubscriptionMgr* pISubscriptionMgr,
    BSTR bstrURL,
    BSTR bstrName,
    SUBSCRIPTIONINFO* psi,
    HWND hwnd,
    SUBSCRIPTIONTYPE st,
    BOOL bIsSoftware
);

HRESULT AddDesktopComponent
(
    COMPONENT* pInfo
);

HRESULT ShowChannelPane
(
    IWebBrowser2* pIWebBrowser2
);

int Channel_CreateDirectory
(
    LPCTSTR pszPath
);

HRESULT Channel_GetBasePath(
    LPTSTR pszPath,
    int cch
);

HRESULT Channel_GetFolder
(
    LPTSTR pszPath,
    XMLDOCTYPE xdt
);

BSTR Channel_GetFullPath
(
    LPCWSTR pwszName
);


DWORD CountChannels(void);

HRESULT Channel_CreateSpecialFolder(
    LPCTSTR pszPath,    // path to folder to create
    LPCTSTR pszURL,     // url for webview
    LPCTSTR pszLogo,    // [optional] path to logo
    LPCTSTR pszWideLogo,// [optional] path to wide logo
    LPCTSTR pszIcon,    // [optional] path to icon file
    int     nIconIndex  // index to icon in above file
    );

BOOL InitVARIANTFromPidl(VARIANT* pvar, LPCITEMIDLIST pidl);

HRESULT Channel_CreateILFromPath(LPCTSTR pszPath, LPITEMIDLIST* ppidl);

HRESULT Channel_CreateChannelFolder( XMLDOCTYPE xdt );

#ifndef UNICODE
int MyPathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);
#endif
//HRESULT Channel_WriteNotificationPath(LPCTSTR pszURL, LPCTSTR pszPath);

HRESULT UpdateImage(LPCTSTR pszPath);

HRESULT PreUpdateChannelImage(
    LPCTSTR pszPath,
    LPTSTR pszHashItem,
    int* piIndex,
    UINT* puFlags,
    int* piImageIndex
);

void UpdateChannelImage(
    LPCWSTR pszHashItem,
    int iIndex,
    UINT uFlags,
    int iImageIndex
);

BOOL Channel_IsInstalled(
    LPCWSTR pszURL
);


LPOLESTR Channel_GetChannelPanePath(
    LPCWSTR pszURL
);

void Channel_SendUpdateNotifications(
    LPCWSTR pwszURL
);

// check the pre-load cache to see if the URL is a default installed one.
BOOL Channel_CheckURLMapping(
    LPCWSTR wszURL
);

HRESULT Channel_WriteScreenSaverURL(
    LPCWSTR pszURL,
    LPCWSTR pszScreenSaverURL
);

HRESULT Channel_RefreshScreenSaverURLs();

HRESULT Channel_GetAndWriteScreenSaverURL(
    LPCTSTR pszURL, 
    LPCTSTR pszDesktopINI
);

