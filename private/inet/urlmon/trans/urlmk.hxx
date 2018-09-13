//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       urlmk.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-12-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef URLMK_HXX
#define URLMK_HXX


// These two structures are used to pass data from the inloader callbacks
// to the wndproc of the hidden window in the main thread.


class CUrlMon;

// Declaration of CUrlMon.  This class implements IMoniker.
// class declaration of CUrlMon. This class derives from IMoniker.
class CUrlMon : public IMoniker, public IROTData, public IMarshal
{
public:
    CUrlMon(LPWSTR szUrl);
    ~CUrlMon();

    LPWSTR  GetUrl() { return _pwzUrl; }


private:
    void DeleteUrl();
    HRESULT SetUrl(LPWSTR pwzUrl, LPWSTR pwzExtra = NULL);

    HRESULT IsRunningROT(IBindCtx *pbc, IMoniker *pmkToLeft,LPRUNNINGOBJECTTABLE *ppROT);
    BOOL IsUrlMoniker(IMoniker *pMk);
    STDMETHODIMP StartBinding(BOOL fBindToObject, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj);


public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IPerist methods ***
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // *** IPeristStream methods ***
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(IStream *pistm);
    STDMETHODIMP Save(IStream *pistm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IMoniker methods ***
    STDMETHODIMP BindToObject(IBindCtx *pbc, IMoniker *pmkToLeft,
                    REFIID riidResult, void **ppvResult);
    STDMETHODIMP BindToStorage(IBindCtx *pbc, IMoniker *pmkToLeft,
                    REFIID riid, void **ppvObj);
    STDMETHODIMP Reduce(IBindCtx *pbc, DWORD dwReduceHowFar,
                    IMoniker **ppmkToLeft, IMoniker **ppmkReduced);
    STDMETHODIMP ComposeWith(IMoniker *pmkRight, BOOL fOnlyIfNotGeneric,
                    IMoniker **ppmkComposite);
    STDMETHODIMP Enum(BOOL fForward, IEnumMoniker **ppenumMoniker);
    STDMETHODIMP IsEqual(IMoniker *pmkOtherMoniker);
    STDMETHODIMP Hash(DWORD *pdwHash);
    STDMETHODIMP IsRunning(IBindCtx *pbc, IMoniker *pmkToLeft,
                    IMoniker *pmkNewlyRunning);
    STDMETHODIMP GetTimeOfLastChange(IBindCtx *pbc, IMoniker *pmkToLeft,
                    FILETIME *pFileTime);
    STDMETHODIMP Inverse(IMoniker **ppmk);
    STDMETHODIMP CommonPrefixWith(IMoniker *pmkOther, IMoniker **ppmkPrefix);
    STDMETHODIMP RelativePathTo(IMoniker *pmkOther, IMoniker **ppmkRelPath);
    STDMETHODIMP GetDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft,
                    LPOLESTR *ppszDisplayName);
    STDMETHODIMP ParseDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft,
                    LPOLESTR pszDisplayName, ULONG *pchEaten,
                    IMoniker **ppmkOut);
    STDMETHODIMP IsSystemMoniker(DWORD *pdwMksys);

public:
    //
    // *** IMarshal methods ***
    STDMETHODIMP GetUnmarshalClass(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags, CLSID *pCid);
    STDMETHODIMP GetMarshalSizeMax(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags, DWORD *pSize);
    STDMETHODIMP MarshalInterface(IStream *pStm, REFIID riid, void *pvInteface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags);
    STDMETHODIMP UnmarshalInterface(IStream *pStm, REFIID riid, void **ppv);
    STDMETHODIMP ReleaseMarshalData(IStream *pStm);
    STDMETHODIMP DisconnectObject(DWORD dwReserved);

private:
    inline BOOL CanMarshalIID(REFIID riid);
    HRESULT ValidateMarshalParams(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext,DWORD mshlflags);

public:
    // *** IROTData methods ***
    STDMETHODIMP GetComparisonData(byte *pbData, ULONG cbMax, ULONG *pcbData);

private:
    CRefCount   _CRefs;    // refcount class
    LPWSTR      _pwzUrl;    // the url string
};



#endif  // URLMK_HXX
