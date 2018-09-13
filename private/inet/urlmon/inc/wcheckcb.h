#ifndef __CONTROL_REFRESH_CALLBACK__
#define __CONTROL_REFRESH_CALLBACK__

#include <webcheck.h>
#include <objidl.h>
#include <wininet.h>

#define CALLBACK_OBJ_CLSID "{5DFE9E81-46E4-11d0-94E8-00AA0059CE02}"
extern const CLSID CLSID_ControlRefreshCallback;


/******************************************************************************
   Class factory for callback object
******************************************************************************/
STDMETHODIMP CreateCallbackClassFactory(IClassFactory** ppCF);

class CCallbackObjFactory : public IClassFactory
{
public:

    // constructor
    CCallbackObjFactory();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory Methods
    STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv);
    STDMETHODIMP LockServer(BOOL fLock);

private:

    // destructor
    ~CCallbackObjFactory();

    // data members
    UINT   m_cRef;         // object refcount
    UINT   m_cLocks;        // dll lock refcount
};


/******************************************************************************
   Callback object class
******************************************************************************/
class CControlRefreshCallback : public IPersistStream,
                                public IWebCheckAdviseSink
{

public:

    // constructor
    CControlRefreshCallback();

    // passing information to this callback object
    STDMETHODIMP SetInfo(REFCLSID rclsidControl, LPCWSTR lpwszURL);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IPersistStream Methods
    STDMETHODIMP GetClassID(CLSID* pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream* pStm);
    STDMETHODIMP Save(IStream* pStm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);

    // IWebCheckAdviseSink Methods
    STDMETHODIMP UpdateBegin(long lCookie, SCODE scReason, BSTR lpURL);
    STDMETHODIMP UpdateEnd(long lCookie, SCODE scReason);
    STDMETHODIMP UpdateProgress(long lCookie, long lCurrent, long lMax);

protected:

    // Update flag in registry to indicate a new version of control
    // has arrived
//    HRESULT UpdateControlInCacheFlag(SCODE scReason) const;
    HRESULT DownloadControl() const;

protected:

    // destructor
    ~CControlRefreshCallback();

    // ref. count
    UINT m_cRef;

    // clsid of control this callback obj deals with
    CLSID m_clsidCtrl;

    // URL of control this callback obj deals with
    WCHAR m_wszURL[INTERNET_MAX_URL_LENGTH];
};

#endif
