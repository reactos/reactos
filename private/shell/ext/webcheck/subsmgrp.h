#include "offline.h"

HRESULT SubscriptionItemFromCookie(BOOL fCreateNew, const SUBSCRIPTIONCOOKIE *pCookie, 
                                   ISubscriptionItem **ppSubscriptionItem);

HRESULT AddUpdateSubscription(SUBSCRIPTIONCOOKIE *pCookie,
                              SUBSCRIPTIONITEMINFO *psii,
                              LPCWSTR pwszURL,
                              ULONG nProps,
                              const LPWSTR rgwszName[], 
                              VARIANT rgValue[]);

BOOL ItemKeyNameFromCookie(const SUBSCRIPTIONCOOKIE *pCookie, 
                           TCHAR *pszKeyName, 
                           DWORD cchKeyName);

BOOL OpenItemKey(const SUBSCRIPTIONCOOKIE *pCookie, BOOL fCreate, REGSAM samDesired, HKEY *phkey);

HRESULT GetInfoFromDataObject(IDataObject *pido,
                              TCHAR *pszPath, DWORD cchPath,
                              TCHAR *pszFriendlyName, DWORD cchFriendlyName,
                              TCHAR *pszURL, DWORD cchURL,
                              INIT_SRC_ENUM *peInitSrc);

HRESULT DoGetItemFromURL(LPCTSTR pszURL, ISubscriptionItem **ppSubscriptionItem);
HRESULT DoGetItemFromURLW(LPCWSTR pwszURL, ISubscriptionItem **ppSubscriptionItem);

HRESULT DoAbortItems( 
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);

HRESULT DoCreateSubscriptionItem( 
    /* [in] */  const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo,
    /* [out] */ SUBSCRIPTIONCOOKIE *pNewCookie,
    /* [out] */ ISubscriptionItem **ppSubscriptionItem);

HRESULT DoCloneSubscriptionItem(
    /* [in] */  ISubscriptionItem *pSubscriptionItem, 
    /* [out] */ SUBSCRIPTIONCOOKIE *pNewCookie,
    /* [out] */ ISubscriptionItem **ppSubscriptionItem);

HRESULT DoDeleteSubscriptionItem(
    /* [in] */ const SUBSCRIPTIONCOOKIE *pCookie,
    /* [in] */ BOOL fAbortItem);

//
// Subscription manager
//
class CSubscriptionMgr : public IShellPropSheetExt,
                         public IShellExtInit,
                         public ISubscriptionMgr2,
                         public ISubscriptionMgrPriv
{
friend INT_PTR CALLBACK SummarizeDesktopSubscriptionDlgProc(HWND, UINT, WPARAM, LPARAM);
friend POOEBuf Summary_GetBuf(HWND hdlg);

protected:
    long        m_cRef;       
    LPMYPIDL    _pidl;
    SUBSCRIPTIONCOOKIE  m_cookie;
    TCHAR       m_pszURL[INTERNET_MAX_URL_LENGTH];
    TCHAR       m_pszFriendly[MAX_PATH];
    TCHAR       m_pszPath[MAX_PATH];
    POOEBuf     m_pBuf;
    IUnknown    * m_pUIHelper;
    UINT            m_nPages;
    INIT_SRC_ENUM   m_eInitSrc;
    SUBSCRIPTIONTYPE    m_oldType;

private:
    ~CSubscriptionMgr();

public:
    CSubscriptionMgr();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellExtInit members
    STDMETHODIMP         Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHODIMP         AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP         ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam);

    // ISubscriptionMgr
    STDMETHODIMP         DeleteSubscription(LPCWSTR pURL, HWND hwnd);
    STDMETHODIMP         IsSubscribed(LPCWSTR pURL, BOOL *);
    STDMETHODIMP         GetDefaultInfo(SUBSCRIPTIONTYPE subType, SUBSCRIPTIONINFO *pInfo);
    STDMETHODIMP         GetSubscriptionInfo(LPCWSTR    pwszURL,
                                            SUBSCRIPTIONINFO *pInfo);
    STDMETHODIMP         ShowSubscriptionProperties(LPCWSTR pURL, HWND hwnd);
    STDMETHODIMP         CreateSubscription(HWND hwnd,
                                            LPCWSTR pwszURL,
                                            LPCWSTR pwszFriendlyName,
                                            DWORD dwFlags,
                                            SUBSCRIPTIONTYPE subsType,
                                            SUBSCRIPTIONINFO *pInfo);
    STDMETHODIMP         UpdateSubscription(LPCWSTR pwszURL);
    STDMETHODIMP         UpdateAll();

    //  ISubscriptionMgr2
    STDMETHODIMP         GetItemFromURL(LPCWSTR pwszURL,
                                        ISubscriptionItem **ppSubscriptionItem);
    STDMETHODIMP         GetItemFromCookie(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
                                           ISubscriptionItem **ppSubscriptionItem);
    STDMETHODIMP         GetSubscriptionRunState(DWORD dwNumCookies,
                                                 const SUBSCRIPTIONCOOKIE *pCookies,
                                                 DWORD *pdwRunState);
    STDMETHODIMP         EnumSubscriptions(DWORD dwFlags,
                                           IEnumSubscription **ppEnumSubscriptions);
    STDMETHODIMP         UpdateItems(DWORD dwFlags,
                                     DWORD dwNumCookies,
                                     const SUBSCRIPTIONCOOKIE *pCookies);
    STDMETHODIMP         AbortItems(DWORD dwNumCookies,
                                    const SUBSCRIPTIONCOOKIE *pCookies);
    STDMETHODIMP         AbortAll();

    // ISubscriptionMgrPriv
    STDMETHODIMP        CreateSubscriptionItem(const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo,
                                               SUBSCRIPTIONCOOKIE *pNewCookie,
                                               ISubscriptionItem **ppSubscriptionItem);
    STDMETHODIMP        CloneSubscriptionItem(ISubscriptionItem *pSubscriptionItem, 
                                              SUBSCRIPTIONCOOKIE *pNewCookie,
                                              ISubscriptionItem **ppSubscriptionItem);
    STDMETHODIMP        DeleteSubscriptionItem(const SUBSCRIPTIONCOOKIE *pCookie);

    STDMETHODIMP        RemovePages(HWND hdlg);
    STDMETHODIMP        SaveSubscription();
    STDMETHODIMP        URLChange(LPCWSTR pwszNewURL);


    HRESULT              CountSubscriptions(SUBSCRIPTIONTYPE subType, PDWORD pdwCount);

protected:
    void                 ChangeSubscriptionValues(OOEBuf *pCurrent, SUBSCRIPTIONINFO *pNew);
//    HRESULT              ResyncData(HWND);
    BOOL                 IsValidSubscriptionInfo(SUBSCRIPTIONTYPE subType, SUBSCRIPTIONINFO *pSI);

    //helpers for CreateSubscription -- not exported via ISubscriptionMgr
    STDMETHODIMP         CreateSubscriptionNoSummary(HWND hwnd,
                                            LPCWSTR pwszURL,
                                            LPCWSTR pwszFriendlyName,
                                            DWORD dwFlags,
                                            SUBSCRIPTIONTYPE subsType,
                                            SUBSCRIPTIONINFO *pInfo);
    STDMETHODIMP         CreateDesktopSubscription(HWND hwnd,
                                            LPCWSTR pwszURL,
                                            LPCWSTR pwszFriendlyName,
                                            DWORD dwFlags,
                                            SUBSCRIPTIONTYPE subsType,
                                            SUBSCRIPTIONINFO *pInfo);
};

class CEnumSubscription : public IEnumSubscription
{
public:
    CEnumSubscription();
    HRESULT Initialize(DWORD dwFlags);
    HRESULT CopyRange(ULONG nStart, ULONG nCount, SUBSCRIPTIONCOOKIE *pCookies, ULONG *pnCopied);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumSubscription
    STDMETHODIMP Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ SUBSCRIPTIONCOOKIE *rgelt,
        /* [out] */ ULONG *pceltFetched);
    
    STDMETHODIMP Skip( 
        /* [in] */ ULONG celt);
    
    STDMETHODIMP Reset( void);
    
    STDMETHODIMP Clone( 
        /* [out] */ IEnumSubscription **ppenum);
    
    STDMETHODIMP GetCount( 
        /* [out] */ ULONG *pnCount);

private:
    ~CEnumSubscription();

    ULONG   m_nCurrent;
    ULONG   m_nCount;
    ULONG   m_cRef;

    SUBSCRIPTIONCOOKIE *m_pCookies;
};

