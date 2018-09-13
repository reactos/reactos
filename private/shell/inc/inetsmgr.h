
#if !defined(_INETSMGR_H_)
#define _INETSMGR_H_

// whoever inherits this class must implement _IsSafeSite
class CInternetSecurityMgrImpl : public IInternetSecurityManager
{
public:
    // *** IUnknown ***
    // (client must provide!)
    
    // *** IInternetSecurityManager methods ***
    virtual STDMETHODIMP MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwReserved)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP GetSecurityId(LPCWSTR pwszUrl, BYTE* pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                  BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
    {
        HRESULT hres = INET_E_DEFAULT_ACTION;

        if (_IsSafeUrl(pwszUrl))
        {
            if (cbPolicy >= SIZEOF(DWORD))
            {
                *(DWORD *)pPolicy = URLPOLICY_ALLOW;
                hres = S_OK;
            }
            else
                hres = S_FALSE;
        }

        return hres;
    }
    virtual STDMETHODIMP QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy,
                                   BYTE *pContext, DWORD cbContext, DWORD dwReserved)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP SetSecuritySite(IInternetSecurityMgrSite *pSite)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP GetSecuritySite(IInternetSecurityMgrSite **ppSite)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
    {
        return INET_E_DEFAULT_ACTION;
    }
    virtual STDMETHODIMP GetZoneMappings(DWORD dwZone, IEnumString **ppEnumString, DWORD dwFlags)
    {
        return INET_E_DEFAULT_ACTION;
    }

protected:
    virtual BOOL _IsSafeUrl(LPCWSTR pwszUrl) = 0;
};

#endif
