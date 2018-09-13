//
// Private channel manager include file.
//

#undef  INTERFACE
#define INTERFACE   IChannelMgrPriv

DECLARE_INTERFACE_(IChannelMgrPriv, IUnknown)
{
    typedef enum _tagCHANNELFOLDERLOCATION { CF_CHANNEL, CF_SOFTWAREUPDATE } CHANNELFOLDERLOCATION;

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IChannelMgrPriv ***
    STDMETHOD(GetBaseChannelPath) (THIS_ LPSTR pszPath, int cch) PURE;
    STDMETHOD(InvalidateCdfCache) (THIS) PURE;
    STDMETHOD(PreUpdateChannelImage) (THIS_ LPCSTR pszPath, LPSTR pszHashItem,
                                      int* piIndex, UINT* puFlags,
                                      int* piImageIndex) PURE;
    STDMETHOD(UpdateChannelImage) (THIS_ LPCWSTR pszHashItem, int iIndex,
                                   UINT uFlags, int iImageIndex) PURE;
    STDMETHOD(GetChannelFolderPath) (THIS_ LPSTR pszPath, int cch, CHANNELFOLDERLOCATION cflChannel) PURE;
    STDMETHOD(GetChannelFolder) (THIS_ LPITEMIDLIST* ppidl, CHANNELFOLDERLOCATION cflChannel) PURE;
    STDMETHOD(DownloadMinCDF) (THIS_ HWND hwnd, LPCWSTR pwszURL, LPWSTR pwszTitle, 
                               DWORD cchTitle, SUBSCRIPTIONINFO *pSubInfo, BOOL *pfIsSoftware) PURE;
    STDMETHOD(ShowChannel) (THIS_ IWebBrowser2 *pWebBrowser2, LPWSTR pwszURL, HWND hwnd) PURE;
    STDMETHOD(IsChannelInstalled) (THIS_ LPCWSTR pwszURL) PURE;
    STDMETHOD(IsChannelPreinstalled) (THIS_ LPCWSTR pwszURL, BSTR * bstrFile) PURE;
    STDMETHOD(RemovePreinstalledMapping) (THIS_ LPCWSTR pwszURL) PURE;
    STDMETHOD(SetupPreinstalledMapping) (THIS_ LPCWSTR pwszURL, LPCWSTR pwszFile) PURE;

    // WARNING!  BEFORE CALLING THE AddAndSubscribe METHOD YOU MUST DETECT
    // THE CDFVIEW VERSION BECAUSE IE 4.00 WILL CRASH IF YOU TRY
    // TO CALL IT

    //  pSubscriptionMgr can be NULL
    STDMETHOD(AddAndSubscribe) (THIS_ HWND hwnd, LPCWSTR pwszURL, 
                                ISubscriptionMgr *pSubscriptionMgr) PURE;
};

#undef  INTERFACE
#define INTERFACE   IChannelMgrPriv2
DECLARE_INTERFACE_(IChannelMgrPriv2, IChannelMgrPriv)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IChannelMgrPriv ***
    STDMETHOD(GetBaseChannelPath) (THIS_ LPSTR pszPath, int cch) PURE;
    STDMETHOD(InvalidateCdfCache) (THIS) PURE;
    STDMETHOD(PreUpdateChannelImage) (THIS_ LPCSTR pszPath, LPSTR pszHashItem,
                                      int* piIndex, UINT* puFlags,
                                      int* piImageIndex) PURE;
    STDMETHOD(UpdateChannelImage) (THIS_ LPCWSTR pszHashItem, int iIndex,
                                   UINT uFlags, int iImageIndex) PURE;
    STDMETHOD(GetChannelFolderPath) (THIS_ LPSTR pszPath, int cch, CHANNELFOLDERLOCATION cflChannel) PURE;
    STDMETHOD(GetChannelFolder) (THIS_ LPITEMIDLIST* ppidl, CHANNELFOLDERLOCATION cflChannel) PURE;
    STDMETHOD(DownloadMinCDF) (THIS_ HWND hwnd, LPCWSTR pwszURL, LPWSTR pwszTitle, 
                               DWORD cchTitle, SUBSCRIPTIONINFO *pSubInfo, BOOL *pfIsSoftware) PURE;
    STDMETHOD(ShowChannel) (THIS_ IWebBrowser2 *pWebBrowser2, LPWSTR pwszURL, HWND hwnd) PURE;
    STDMETHOD(IsChannelInstalled) (THIS_ LPCWSTR pwszURL) PURE;
    STDMETHOD(IsChannelPreinstalled) (THIS_ LPCWSTR pwszURL, BSTR * bstrFile) PURE;
    STDMETHOD(RemovePreinstalledMapping) (THIS_ LPCWSTR pwszURL) PURE;
    STDMETHOD(SetupPreinstalledMapping) (THIS_ LPCWSTR pwszURL, LPCWSTR pwszFile) PURE;

    // WARNING!  BEFORE CALLING THE AddAndSubscribe METHOD YOU MUST DETECT
    // THE CDFVIEW VERSION BECAUSE IE 4.00 WILL CRASH IF YOU TRY
    // TO CALL IT

    //  pSubscriptionMgr can be NULL
    STDMETHOD(AddAndSubscribe) (THIS_ HWND hwnd, LPCWSTR pwszURL, 
                                ISubscriptionMgr *pSubscriptionMgr) PURE;

    // *** IChannelMgrPriv2 ***
    STDMETHOD(WriteScreenSaverURL) (THIS_ LPCWSTR pwszURL, LPCWSTR pwszScreenSaverURL) PURE;
    STDMETHOD(RefreshScreenSaverURLs) (THIS) PURE;
};
