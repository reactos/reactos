#ifndef _subitem_h
#define _subitem_h

HRESULT BlobToVariant(BYTE *pData, DWORD cbData, VARIANT *pVar, DWORD *pcbUsed);

class CEnumItemProperties : public IEnumItemProperties
{
public:
    CEnumItemProperties();
    HRESULT Initialize(const SUBSCRIPTIONCOOKIE *pCookie, ISubscriptionItem *psi);
    HRESULT CopyItem(ITEMPROP *pip, WCHAR *pwszName, VARIANT *pVar);
    HRESULT CopyRange(ULONG nStart, ULONG nCount, ITEMPROP *ppip, ULONG *pnCopied);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumItemProperties
    STDMETHODIMP Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ ITEMPROP *rgelt,
        /* [out] */ ULONG *pceltFetched);
    
    STDMETHODIMP Skip( 
        /* [in] */ ULONG celt);
    
    STDMETHODIMP Reset( void);
    
    STDMETHODIMP Clone( 
        /* [out] */ IEnumItemProperties **ppenum);
    
    STDMETHODIMP GetCount( 
        /* [out] */ ULONG *pnCount);

private:
    ~CEnumItemProperties();

    ULONG       m_cRef;
    ULONG       m_nCurrent;
    ULONG       m_nCount;

    ITEMPROP    *m_pItemProps;
};

class CSubscriptionItem : public ISubscriptionItem 
{
public:
    CSubscriptionItem(const SUBSCRIPTIONCOOKIE *pCookie, HKEY hkey);
    HRESULT Read(HKEY hkeyIn, const WCHAR *pwszValueName, BYTE *pData, DWORD dwDataSize);
    HRESULT ReadWithAlloc(HKEY hkeyIn, const WCHAR *pwszValueName, BYTE **ppData, DWORD *pdwDataSize);
    HRESULT Write(HKEY hkeyIn, const WCHAR *pwszValueName, BYTE *pData, DWORD dwDataSize);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISubscriptionItem
    STDMETHODIMP GetCookie(SUBSCRIPTIONCOOKIE *pCookie);
    STDMETHODIMP GetSubscriptionItemInfo( 
        /* [out] */ SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);
    
    STDMETHODIMP SetSubscriptionItemInfo( 
        /* [in] */ const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo);
    
    STDMETHODIMP ReadProperties( 
        ULONG nCount,
        /* [size_is][in] */ const LPCWSTR rgwszName[],
        /* [size_is][out] */ VARIANT rgValue[]);
    
    STDMETHODIMP WriteProperties( 
        ULONG nCount,
        /* [size_is][in] */ const LPCWSTR rgwszName[],
        /* [size_is][in] */ const VARIANT rgValue[]);
    
    STDMETHODIMP EnumProperties( 
        /* [out] */ IEnumItemProperties **ppEnumItemProperties);

    STDMETHODIMP NotifyChanged();

private:
    ~CSubscriptionItem();
    ULONG               m_cRef;
    SUBSCRIPTIONCOOKIE  m_Cookie;
    DWORD               m_dwFlags;
};

#endif // _subitem_h


