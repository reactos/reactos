////////////////////////////////////////////////////////////////////////////////
//
// chanmgri.h 
//
//   The class definition for the CChannelMgr
//
//   History:
//
//       4/30/97  julianj   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CHANMGRI_H_

#define _CHANMGRI_H_

//
// The class definition for the Channel Manager
//

class CChannelMgr : public IChannelMgr,
                    public IChannelMgrPriv2,
#ifdef UNICODE
                    public ICopyHookA,
#endif
                    public ICopyHook
{
    //
    // Methods
    //
public:

    //
    // Constructor
    //
    CChannelMgr(void);                           

    //
    // IUnknown
    //
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IChannelMgr methods
    //
    STDMETHODIMP AddChannelShortcut(CHANNELSHORTCUTINFO *pChannelInfo);
    STDMETHODIMP DeleteChannelShortcut(BSTR strTitle);
    STDMETHODIMP AddCategory(CHANNELCATEGORYINFO *pCategoryInfo);
    STDMETHODIMP DeleteCategory(BSTR strTitle);
    STDMETHODIMP EnumChannels(DWORD dwEnumFlags, LPCWSTR pszURL,
                              IEnumChannels** pIEnumChannels);

    //
    // IChannelMgrPrive methods.
    //
    STDMETHODIMP GetBaseChannelPath(LPSTR pszPath, int cch);
    STDMETHODIMP InvalidateCdfCache(void);

    STDMETHODIMP PreUpdateChannelImage(LPCSTR pszPath,
                                       LPSTR pszHashItem,
                                       int* piIndex,
                                       UINT* puFlags,
                                       int* piImageIndex);

    STDMETHODIMP UpdateChannelImage(LPCWSTR pszHashItem,
                                    int iIndex,
                                    UINT uFlags,
                                    int iImageIndex);

    STDMETHODIMP GetChannelFolderPath (LPSTR pszPath, int cch,
                                       enum _tagCHANNELFOLDERLOCATION chLoc);
    STDMETHODIMP GetChannelFolder (LPITEMIDLIST* ppidl,
                                   enum _tagCHANNELFOLDERLOCATION chLoc);

    STDMETHODIMP DownloadMinCDF(HWND hwnd, LPCWSTR pwszURL, LPWSTR pwszTitle, 
                                DWORD cchTitle, SUBSCRIPTIONINFO *pSubInfo,
                                BOOL *pfIsSoftware);
    STDMETHODIMP ShowChannel(IWebBrowser2 *pWebBrowser2, LPWSTR pwszURL, HWND hwnd);
    STDMETHODIMP IsChannelInstalled(LPCWSTR pwszURL);
    STDMETHODIMP IsChannelPreinstalled(LPCWSTR pwszURL, BSTR * bstrFile); 
    STDMETHODIMP RemovePreinstalledMapping(LPCWSTR pwszURL);
    STDMETHODIMP SetupPreinstalledMapping(LPCWSTR pwszURL, LPCWSTR pwszFile);
    STDMETHODIMP AddAndSubscribe(HWND hwnd, LPCWSTR pwszURL, 
                                 ISubscriptionMgr *pSubscriptionMgr);


    STDMETHODIMP WriteScreenSaverURL(LPCWSTR pwszURL, LPCWSTR pwszScreenSaverURL);
    STDMETHODIMP RefreshScreenSaverURLs();

    //
    // ICopyHook method
    //
    STDMETHODIMP_(UINT) CopyCallback(
        HWND hwnd,          
        UINT wFunc,         
        UINT wFlags,        
        LPCTSTR pszSrcFile,  
        DWORD dwSrcAttribs, 
        LPCTSTR pszDestFile, 
        DWORD dwDestAttribs 
    );
#ifdef UNICODE
    STDMETHODIMP_(UINT) CopyCallback(
        HWND hwnd,          
        UINT wFunc,         
        UINT wFlags,        
        LPCSTR pszSrcFile,  
        DWORD  dwSrcAttribs, 
        LPCSTR pszDestFile, 
        DWORD  dwDestAttribs
    );
#endif

    //  Helpers
    STDMETHODIMP AddAndSubscribeEx2(HWND hwnd, LPCWSTR pwszURL, 
                                    ISubscriptionMgr *pSubscriptionMgr, 
                                    BOOL bAlwaysSubscribe);

private:
    //
    // Destructor
    //
    ~CChannelMgr(void);

    //
    // Member variables.
    //
private:

    ULONG           m_cRef;
};

#endif // _CHANMGRI_H_
