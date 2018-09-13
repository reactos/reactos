//  File:       zonemgr.h
//
//  Contents:   This file defines the class that implements the base IInternetZoneManager
//
//  Classes:    CUrlZoneManager
//
//  Functions:
//
//  History: 
//
//----------------------------------------------------------------------------

#ifndef _ZONEMGR_H_
#define _ZONEMGR_H_

class CUrlZoneManager : public IInternetZoneManager
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);



    // IInternetZoneManager overrides
       
    STDMETHODIMP GetZoneAttributes( 
        /* [in] */ DWORD dwZone,
        /* [unique][out][in] */ ZONEATTRIBUTES *pZoneAttributes);
    
    STDMETHODIMP SetZoneAttributes( 
        /* [in] */ DWORD dwZone,
        /* [in] */ ZONEATTRIBUTES *pZoneAttributes);
    
    STDMETHODIMP GetZoneCustomPolicy( 
        /* [in] */ DWORD dwZone,
        /* [in] */ REFGUID guidKey,
        /* [size_is][size_is][out] */ BYTE **ppPolicy,
        /* [out] */ DWORD *pcbPolicy,
        /* [in] */ URLZONEREG urlZoneReg);
    
    STDMETHODIMP SetZoneCustomPolicy( 
        /* [in] */ DWORD dwZone,
        /* [in] */ REFGUID guidKey,
        /* [size_is][in] */ BYTE *pPolicy,
        /* [in] */ DWORD cbPolicy,
        /* [in] */ URLZONEREG urlZoneReg);
    
    STDMETHODIMP GetZoneActionPolicy( 
        /* [in] */ DWORD dwZone,
        /* [in] */ DWORD dwAction,
        /* [size_is][out] */ BYTE *pPolicy,
        /* [in] */ DWORD cbPolicy,
        /* [in] */ URLZONEREG urlZoneReg);
    
    STDMETHODIMP SetZoneActionPolicy( 
        /* [in] */ DWORD dwZone,
        /* [in] */ DWORD dwAction,
        /* [size_is][in] */ BYTE *pPolicy,
        /* [in] */ DWORD cbPolicy,
        /* [in] */ URLZONEREG urlZoneReg);
    
    STDMETHODIMP PromptAction( 
        /* [in] */ DWORD dwAction,
        /* [in] */ HWND hwndParent,
        /* [in] */ LPCWSTR pwszUrl,
        /* [in] */ LPCWSTR pwszText,
        /* [in] */ DWORD dwPromptFlags);
    
    STDMETHODIMP LogAction( 
        /* [in] */ DWORD dwAction,
        /* [in] */ LPCWSTR pwszUrl,
        /* [in] */ LPCWSTR pwszText,
        /* [in] */ DWORD dwLogFlags);
    
    STDMETHODIMP CreateZoneEnumerator( 
        /* [out] */ DWORD *pdwEnum,
        /* [out] */ DWORD *pdwCount,
        /* [in] */ DWORD dwFlags);
    
    STDMETHODIMP GetZoneAt( 
        /* [in] */ DWORD dwEnum,
        /* [in] */ DWORD dwIndex,
        /* [out] */ DWORD *pdwZone);
    
    STDMETHODIMP DestroyZoneEnumerator( 
        /* [in] */ DWORD dwEnum);
    
    STDMETHODIMP CopyTemplatePoliciesToZone( 
        /* [in] */ DWORD dwTemplate,
        /* [in] */ DWORD dwZone,
        /* [in] */ DWORD dwReserved);
    

public:
    CUrlZoneManager(IUnknown *pUnkOuter, IUnknown** ppUnkInner );
    virtual ~CUrlZoneManager();
    virtual BOOL Initialize();  

    static inline BOOL Cleanup ( )
    {   delete s_pRegZoneContainer ;  
        if ( s_bcsectInit ) DeleteCriticalSection(&s_csect) ; 
        return TRUE;
    }



    static CRITICAL_SECTION s_csect;
    static BOOL s_bcsectInit;

// Aggregation and RefCount support.
protected:
    CRefCount m_ref;
        
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        ~CPrivUnknown() {}
        CPrivUnknown() : m_ref () {}

    private:
        CRefCount   m_ref;          // the total refcount of this object
    };

    friend class CPrivUnknown;
    CPrivUnknown m_Unknown;

    IUnknown*   m_pUnkOuter;

    STDMETHODIMP_(ULONG) PrivAddRef()
    {
        return m_Unknown.AddRef();
    }
    STDMETHODIMP_(ULONG) PrivRelease()
    {
        return m_Unknown.Release();
    }


protected:
    static CRegZoneContainer* s_pRegZoneContainer;
    static inline CRegZone * GetRegZoneById(DWORD dw) 
        { return s_pRegZoneContainer->GetRegZoneById(dw); }

private:
    IServiceProvider *m_pSP;    
};

#endif // _ZONEMGR_H_
