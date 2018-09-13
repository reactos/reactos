#include "priv.h"
#include "dochost.h"

#define DM_CACHEOLESERVER   DM_TRACE

#define HACK_CACHE_OBJECT_TOO
// #define HACK_LOCKRUNNING_TOO

class CClassHolder : IUnknown
{
protected:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    friend IUnknown* ClassHolder_Create(const CLSID* pclsid);

    CClassHolder(const CLSID* pclsid);
    ~CClassHolder();

    UINT _cRef;
    IClassFactory* _pcf;
    DWORD _dwAppHack;

#ifdef HACK_CACHE_OBJECT_TOO
    IUnknown* _punk;
#ifdef HACK_LOCKRUNNING_TOO
    IRunnableObject* _pro;
#endif
#endif // HACK_CACHE_OBJECT_TOO
};

HRESULT CClassHolder::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CClassHolder, IDiscardableBrowserProperty, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CClassHolder::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CClassHolder::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

CClassHolder::CClassHolder(const CLSID* pclsid) : _cRef(1)
{
    HRESULT hres;
    hres = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                    0, IID_IClassFactory, (LPVOID*)&_pcf);

    TraceMsg(DM_CACHEOLESERVER, "CCH::CCH Just called CoGetClassObject %x", hres);

    if (SUCCEEDED(hres)) {
        ::GetAppHackFlags(NULL, pclsid, &_dwAppHack);

        _pcf->LockServer(TRUE);

#ifdef HACK_CACHE_OBJECT_TOO
        hres = _pcf->CreateInstance(NULL, IID_IUnknown, (LPVOID*)&_punk);

        if ((_dwAppHack & BROWSERFLAG_INITNEWTOKEEP) && SUCCEEDED(hres)) {

            TraceMsg(TF_SHDAPPHACK, "CCH::CCH hack for Excel. Call InitNew to keep it running");

            //
            // This InitNew keeps Excel running
            //
            IPersistStorage* pps;
            HRESULT hresT;
            hresT = _punk->QueryInterface(IID_IPersistStorage, (LPVOID*)&pps);
            if (SUCCEEDED(hresT)) {
                IStorage* pstg;
                hresT = StgCreateDocfile(NULL,
                    STGM_DIRECT | STGM_CREATE | STGM_READWRITE
                    | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
                    0, &pstg);
                if (SUCCEEDED(hresT)) {
                    TraceMsg(DM_TRACE, "CCLH::ctor calling InitNew()");
                    pps->InitNew(pstg);
                    pstg->Release();
                } else {
                    TraceMsg(DM_TRACE, "CCLH::ctor StgCreateDocfile failed %x", hresT);
                }
                pps->Release();
            } else {
                TraceMsg(DM_TRACE, "CCLH::ctor QI to IPersistStorage failed %x", hresT);
            }
#ifdef HACK_LOCKRUNNING_TOO
            hres = _punk->QueryInterface(IID_IRunnableObject, (LPVOID*)&_pro);
            if (SUCCEEDED(hres)) {
                TraceMsg(DM_CACHEOLESERVER, "CCH::CCH This is runnable. Keep it running %x", _pro);
                OleRun(_pro);
                OleLockRunning(_pro, TRUE, TRUE);
            }
#endif
        }
#endif
    }
}

CClassHolder::~CClassHolder()
{
    if (_pcf) {
#ifdef HACK_CACHE_OBJECT_TOO
        if (_punk) {
#ifdef HACK_LOCKRUNNING_TOO
            if (_pro) {
                OleLockRunning(_pro, FALSE, TRUE);
                _pro->Release();
            }
#endif
            _punk->Release();
        }
#endif
        _pcf->LockServer(FALSE);
        ATOMICRELEASE(_pcf);
    }
}

IUnknown* ClassHolder_Create(const CLSID* pclsid)
{
    return new CClassHolder(pclsid);
}



