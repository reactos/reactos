//***   inst.cpp -- 'instance' (CoCreate + initialization) mechanism
// SYNOPSIS
//  CInstClassFactory_Create    create 'stub loader' class factory
//  InstallBrowBandInst install BrowserBand instance into registry
//  InstallInstAndBag   install arbitrary instance into registry
//  - debug
//  DBCreateInitInst    create an
//
// DESCRIPTION
//  the 'instance' mechanism provides an easy way to create and initialize
//  a class from the registry (w/o writing any code).
//
//  an 'instance' consists of an INSTID (unique to the instance), a CLSID
//  (for the code), and an InitPropertyBag (to initialize the instance).
//
//  it is fully transparent to CoCreateInstance; that is, one can do a
//  CCI of an INSTID and it will create it and initialize it w/ the caller
//  none the wiser.  (actually there will be at least one tip-off, namely
//  that IPS::GetClassID on the instance will return the 'code' CLSID not
//  the 'instance' INSTID [which is as it should be, since this is exactly
//  how persistance works when one programmatically creates his own multiple
//  instances and then persists them. 
//
//  the INSTID is in the HKR/CLSID section of the registry (just like a
//  'normal' CLSID).  the code points to shdocvw.  when shdocvw hits the
//  failure case in its DllGetClassObject search, it looks for the magic
//  key 'HKCR/CLSID/{instid}/Instance'.  if it finds it, it knows it's
//  dealing w/ an INSTID, and builds a class factory 'stub loader' which
//  has sufficient information to find the 'code' CLSID and the 'init'
//  property bag.

#include "priv.h"
#include "stream.h"

extern "C" IClassFactory *CInstClassFactory_Create(const CLSID *pclsid);
HKEY GetClsidKey(LPTSTR pszClsid, DWORD grfMode);
HKEY GetInstKey(LPTSTR pszInst, DWORD grfMode);

// {
class CInstClassFactory : IClassFactory
{
public:
    //*** IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    //*** THISCLASS
    virtual STDMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvObj);
    virtual STDMETHODIMP LockServer(BOOL fLock);

protected:
    CInstClassFactory() { DllAddRef(); _cRef = 1; };
    ~CInstClassFactory();

    friend IClassFactory *CInstClassFactory_Create(const CLSID *pclsid);

    HRESULT Init(const CLSID *pclsid);

    ULONG   _cRef;
    HKEY    _hkey;  // hkey for instance info
};

//***   CInstClassFactory_Create --
// NOTES
//  called when class isn't in our sccls.c CCI table.  we see if it's an
// instance, and if so we make a stub for it that gives sufficient info
// for our CreateInstance to create and init it.
//
//  n.b. we keep the failure case as cheap as possible (just a regkey check,
// no object creation etc.).
//
IClassFactory *CInstClassFactory_Create(const CLSID *pclsid)
{
    CInstClassFactory *pcf = new CInstClassFactory;
    if (pcf && FAILED(pcf->Init(pclsid))) {
        delete pcf;
        pcf = NULL;
    }

    return pcf;
}

HRESULT CInstClassFactory::Init(const CLSID *pclsid)
{
    ASSERT(_hkey == NULL);  // only init me once please

    TCHAR szClass[GUIDSTR_MAX];

    // "CLSID/{instid}/Instance"
    SHStringFromGUID(*pclsid, szClass, GUIDSTR_MAX);
    _hkey = GetInstKey(szClass, STGM_READ);
    
    return _hkey ? S_OK : E_OUTOFMEMORY;
}

CInstClassFactory::~CInstClassFactory()
{
    if (_hkey)
        RegCloseKey(_hkey);

    DllRelease();
}

//***   CInstClassFactory::IUnknown::* {

ULONG CInstClassFactory::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CInstClassFactory::Release()
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CInstClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CInstClassFactory, IClassFactory), // IID_IClassFactory
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

// }


HRESULT CInstClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;            // the usual optimism :-)
    *ppv = NULL;

    ASSERT(_hkey);          // o.w. shouldn't ever get here

    CRegStrPropBag *prspb = CRegStrPropBag_Create(_hkey, STGM_READ);

    if (prspb)
    {
        IUnknown* pUnk = NULL;

        TCHAR szClass[GUIDSTR_MAX];
        DWORD cbTmp = SIZEOF(szClass);

        // get object (vs. instance) CLSID and create it
        hr = prspb->QueryValueStr(TEXT("CLSID"), szClass, &cbTmp);

        if (SUCCEEDED(hr))
        {
            CLSID clsid;
            hr = SHCLSIDFromString(szClass, &clsid);

            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnk);

                if (SUCCEEDED(hr))
                {
                    // try to load from propertybag first

                    HRESULT hrPropBagLoading = E_FAIL;

                    hr = prspb->ChDir(TEXT("InitPropertyBag"));

                    if (SUCCEEDED(hr))
                    {
                        IPersistPropertyBag *pPerPBag;

                        hr = pUnk->QueryInterface(IID_IPersistPropertyBag, (void**) &pPerPBag);

                        if (SUCCEEDED(hr))
                        {
                            // Failure here is not fatal
                            hrPropBagLoading = pPerPBag->Load(SAFECAST(prspb, IPropertyBag*), NULL);
                        }

                        pPerPBag->Release();
                    }

                    // Did the property bag interface exist and did it load properly?
                    if ( FAILED(hr) || FAILED(hrPropBagLoading))
                    {
                        // No property bag interface or did not load suyccessfully, try stream

                        // Store this state temporarily, if stream fails too then we'll return the object
                        //  with this hr
                        HRESULT hrPropBag = hr;

                        IPersistStream* pPerStream = NULL;

                        hr = pUnk->QueryInterface(IID_IPersistStream, (void**) &pPerStream);

                        if (SUCCEEDED(hr))
                        {
                            // try to open the stream from the reg

                            hr = prspb->ChDir(TEXT("InitStream"));

                            IStream* pStream = NULL;

                            if (SUCCEEDED(hr))
                            {
                                pStream = SHOpenRegStream(_hkey, TEXT("InitStream"), NULL, STGM_READ);
                            }

                            if (SUCCEEDED(hr) && pStream)
                            {
                                hr = pPerStream->Load(pStream);

                                pStream->Release();
                            }
                            else
                                hr = E_FAIL;

                            pPerStream->Release();
                        }
                        else
                            hr = hrPropBag;
                    }
                }  
            }
        }
        prspb->Release();

        if (SUCCEEDED(hr))
        {
            hr = pUnk->QueryInterface(riid, ppv);

            pUnk->Release();
        }
    }

    return hr;
}

HRESULT CInstClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

// }

//***   misc helpers {

//***
//
HKEY GetClsidKey(LPTSTR pszInst, DWORD grfMode)
{
    TCHAR szRegName[MAX_PATH];      // "CLSID/{instid}" BUGBUG size?

    // "CLSID/{instid}"
    ASSERT(ARRAYSIZE(szRegName) >= 5 + 1 + GUIDSTR_MAX - 1 + 1);
    ASSERT(lstrlen(pszInst) == GUIDSTR_MAX - 1);
    wnsprintf(szRegName, ARRAYSIZE(szRegName), TEXT("CLSID\\%s"), pszInst);

    return Reg_CreateOpenKey(HKEY_CLASSES_ROOT, szRegName, grfMode);
}

//***
// NOTES
//  perf: failure case is cheap, only does a RegOpen, no object creation.
//  positions to the 'Instance' part, must 'ChDir' to get to InitXxx part.
HKEY GetInstKey(LPTSTR pszInst, DWORD grfMode)
{
    TCHAR szRegName[MAX_PATH];      // "CLSID/{instid}/Instance" BUGBUG size?

    // "CLSID/{instid}/Instance"
    ASSERT(ARRAYSIZE(szRegName) >= 5 + 1 + GUIDSTR_MAX - 1 + 1 + 8 + 1);
    ASSERT(lstrlen(pszInst) == GUIDSTR_MAX - 1);
    wnsprintf(szRegName, ARRAYSIZE(szRegName), TEXT("CLSID\\%s\\Instance"), pszInst);

    return Reg_CreateOpenKey(HKEY_CLASSES_ROOT, szRegName, grfMode);
}

// }

//***   client code (install and instance) {

extern CRegStrPropBag *InstallInstAndBag(LPTSTR pszInst, LPTSTR pszName, LPTSTR pszClass);

//***   InstallBrowBandInst -- install 'instance' of BrowBand
// ENTRY/EXIT
//  pszInst     instance clsid as '{...}'
//  pszName     friendly name (for title?)
//  pszClass    code     clsid as '{...}'
//  pszUrl      initial URL
HRESULT InstallBrowBandInst(LPTSTR pszInst, LPTSTR pszName, LPTSTR pszClass, LPTSTR pszUrl)
{
    CRegStrPropBag *prspb;
    HRESULT hr = E_FAIL;

    // instance and minimal prop bag
    prspb = InstallInstAndBag(pszInst, pszName, pszClass);
    if (prspb != NULL) {
        // additional properties
        hr = prspb->ChDir(TEXT("InitPropertyBag"));
        if (SUCCEEDED(hr))
            hr = prspb->SetValueStr(TEXT("Url"), pszUrl);

        prspb->Release();
    }

    return hr;
}

//***   InstallInstAndBag -- install an 'instance' and its minimal (req'd) bag
// ENTRY/EXIT
//  pszInst     instance clsid as '{...}'
//  pszName     friendly name (for title?)
//  pszClass    object   clsid as '{...}'
//  prspb       [out] prop bag for additional bag inits 
// NOTES
//  BUGBUG if the func fails part-way thru we leave around the CLSID entry
CRegStrPropBag *InstallInstAndBag(LPTSTR pszInst, LPTSTR pszName, LPTSTR pszClass)
{
    CRegStrPropBag *prspb = NULL;
    HKEY hkey = GetClsidKey(pszInst, STGM_WRITE);    // "CLSID/{instid}"
    if (hkey) {
        prspb = CRegStrPropBag_Create(hkey, STGM_WRITE);
        if (prspb) {
            int GetUnExpSystemDir(TCHAR *pszBuf, int cchBuf, BOOL fSlash);
            int i;
            TCHAR szExpSz[MAX_PATH];

            prspb->SetValueStr(TEXT(""), pszName); // BUGBUG here or in Instance?

            // %SystemRoot%/System32/shdocvw.dll or c:/windows/system/shdocvw.dll
            i = GetUnExpSystemDir(szExpSz, ARRAYSIZE(szExpSz), TRUE);
            StrCpyN(szExpSz + i, TEXT("shdocvw.dll"), ARRAYSIZE(szExpSz) - i);

            prspb->ChDir(TEXT("InProcServer32"));
            prspb->SetValueStrEx(TEXT(""), REG_EXPAND_SZ, szExpSz);
            prspb->SetValueStr(TEXT("ThreadingModel"), TEXT("Apartment"));

            prspb->SetRoot(hkey, STGM_WRITE);       // back up to ".." (hack)
            prspb->ChDir(TEXT("Instance"));
            prspb->SetValueStr(TEXT("CLSID"), pszClass);
        }

        RegCloseKey(hkey);  // (waited until now for ".." hack)
    }

    return prspb;
}

//***   GetUnExpSystem32 -- get winNT/win95 regkey for "windows/system" equiv
// ENTRY/EXIT
//  return      w95: "c:/windows/system/", NT: "%SystemRoot%/System32/"
//              failure: "" (relative path)
// NOTES
//  WARNING: doesn't handle buffer overflow
int GetUnExpSystemDir(TCHAR *pszBuf, int cchBuf, BOOL fSlash)
{
    static const TCHAR szSys32[] = TEXT("%SystemRoot%\\System32");
    int i;

#ifdef DEBUG
    pszBuf[cchBuf - 1] = 0;
#endif

    if (g_fRunningOnNT) {
        StrCpyN(pszBuf, szSys32, cchBuf);
        i = ARRAYSIZE(szSys32) - 1;
    }
    else {
        i = GetSystemDirectory(pszBuf, cchBuf);
        ASSERT(i <= cchBuf);
        if (i == 0) {
            ASSERT(0);
            // use relative path
            pszBuf[0] = 0;
            i = 0;
        }
    }

    if (fSlash && i != 0 && pszBuf[i - 1] != TEXT('\\')) {
        ASSERT(i + 1 < cchBuf);
        pszBuf[i++] = TEXT('\\');
        pszBuf[i] = 0;
    }
        
    ASSERT(pszBuf[cchBuf - 1] == 0);    // overflow?

    ASSERT(i == lstrlen(pszBuf));

    return i;
}

#ifdef DEBUG
//***   DBCreateInitInst -- create "instance" and initialize it
// DESCRIPTION
//  tmp for testing until we hook up to sccls.c etc.
HRESULT DBCreateInitInst(const CLSID *pclsid, REFIID riid, void **ppv)
{
    HRESULT hr;
    IClassFactory *pcf = CInstClassFactory_Create(pclsid);
    if (pcf) {
        hr = pcf->CreateInstance(NULL, riid, ppv);
        pcf->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}
#endif

// }
