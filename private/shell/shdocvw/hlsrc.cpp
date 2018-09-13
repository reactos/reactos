#include "priv.h"


class CMyHlinkSrc : public IHlinkSource
{
    friend HRESULT CMyHlinkSrc_CreateInstance(REFCLSID rclsid, DWORD grfContext, REFIID riid, LPVOID* ppvOut);
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IHlinkSource methods ***
    virtual STDMETHODIMP SetBrowseContext(
	 IHlinkBrowseContext *pihlbc);

    virtual STDMETHODIMP GetBrowseContext(
	 IHlinkBrowseContext **ppihlbc);

    virtual STDMETHODIMP Navigate(
	 DWORD grfHLNF,
	 LPCWSTR pwzJumpLocation);

    virtual STDMETHODIMP GetMoniker(
	 LPCWSTR pwzLocation,
	 DWORD dwAssign,
	 IMoniker **ppimkLocation);

    virtual STDMETHODIMP GetFriendlyName(
	 LPCWSTR pwzLocation,
	 LPWSTR *ppwzFriendlyName);

protected:
    CMyHlinkSrc();
    ~CMyHlinkSrc();

    UINT		_cRef;
    IUnknown*   	_punkInner;	// aggregated inner object
    IHlinkSource* 	 _phlsrc;	// cached IHlinkSource
    IHlinkBrowseContext* _phlbc;
};

CMyHlinkSrc::CMyHlinkSrc() : _cRef(1), _punkInner(NULL), _phlsrc(NULL), _phlbc(NULL)
{
    DllAddRef();
}

CMyHlinkSrc::~CMyHlinkSrc()
{
    DllRelease();
}

//
// This function returns an aggregated object
//
HRESULT CMyHlinkSrc_CreateInstance(REFCLSID rclsid, DWORD grfContext, REFIID riid, LPVOID* ppvOut)
{
    HRESULT hres = E_OUTOFMEMORY;
    *ppvOut = NULL;

    CMyHlinkSrc* phlsrcOuter = new CMyHlinkSrc();
    if (phlsrcOuter)
    {

	hres = CoCreateInstance(rclsid, phlsrcOuter, grfContext, IID_IUnknown,
			        (LPVOID*)&phlsrcOuter->_punkInner);
	if (SUCCEEDED(hres))
	{
	    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_CreateInstenace CoCreateSucceeded");
	    // Cache IHlinkSource of the inner object (if any).
	    HRESULT hresT = phlsrcOuter->_punkInner->QueryInterface(
				IID_IHlinkSource, (LPVOID*)&phlsrcOuter->_phlsrc);
	    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_CreateInstenace QI(IID_IHlinkSource) returned (%x)", hres);
	    if (SUCCEEDED(hresT)) {
		//
		// Decrement the reference count to avoid cycled reference.
		// See "The COM Programmer's Cookbook for detail.
		//
    		phlsrcOuter->Release();
	    }

	    hres = phlsrcOuter->QueryInterface(riid, ppvOut);
	}
	else
	{
	    TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_CreateInstenace CoCI failed (%x)", hres);
	}

	phlsrcOuter->Release();
    }

    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_CreateInstenace leaving");

    return hres;
}

HRESULT CMyHlinkSrc::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IUnknown*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IHlinkSource))
    {
	//
	// If the inner object supports IHlinkSource, return it;
	// otherwise, return our own.
	//
	TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc::QueryInterface IID_IHlinkSource called");
        *ppvObj = _phlsrc ? _phlsrc : (IHlinkSource*)this;
        _cRef++;
        return S_OK;
    }
    else if (_punkInner)
    {
	//
	// Delegate QI down to the inner object. This technique is
	// called "Blind QueryInterfcae" in the COM Programmer's Cookbook.
	// This book says, we shouldn't use this technique unless we modify
	// any behavior of other interfaces. In this case, we don't modify
	// any behavior and it's safe to use this technique.
	//
	// TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc::QueryInterface delegating QI to inner object");
	return _punkInner->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CMyHlinkSrc::AddRef(void)
{
    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc::AddRef new _cRef is %d", _cRef+1);
    return ++_cRef;
}

ULONG CMyHlinkSrc::Release(void)
{
    if (--_cRef > 0) {
	// TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc::Release new _cRef is %d", _cRef);
	return _cRef;
    }

    TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc::Release deleting this object ----- (YES!)");

    if (_phlbc) {
	_phlbc->Release();
    }

    _cRef = 1;	// guard (to be recursively hit this code)
    if (_phlsrc) {
	AddRef();		// balance the ref. count
	_phlsrc->Release();	// release the cached interface
    }

    if (_punkInner) {
	_punkInner->Release();
    }

    ASSERT(_cRef == 1);
    delete this;
    return 0;
}

// *** IHlinkSource methods ***
HRESULT CMyHlinkSrc::SetBrowseContext(
     IHlinkBrowseContext *pihlbc)
{
    if (_phlbc) {
	_phlbc->Release();
    }

    _phlbc = pihlbc;
    if (_phlbc) {
	_phlbc->AddRef();
    }

    return S_OK;
}

HRESULT CMyHlinkSrc::GetBrowseContext(
     IHlinkBrowseContext **ppihlbc)
{
    *ppihlbc = _phlbc;

    if (_phlbc) {
	_phlbc->AddRef();
    }

    return S_OK;
}

HRESULT CMyHlinkSrc::Navigate(
     DWORD grfHLNF,
     LPCWSTR pwzJumpLocation)
{
    IOleDocumentView* pmsov = NULL;
    HRESULT hres = _punkInner->QueryInterface(IID_IOleDocumentView, (LPVOID*)&pmsov);
    if (SUCCEEDED(hres)) {
	hres = pmsov->UIActivate(TRUE);
	TraceMsg(DM_TRACE, "sdv TR CHS::Navigate pmsov->UIActivate() returned %x", hres);
	if (SUCCEEDED(hres)) {
	    // HlinkOnNavigate
	}
	pmsov->Release();
    } else {
	TraceMsg(DM_TRACE, "sdv TR CHS::Navigate _punkInner->QI(IID_Mso) failed");
    }

    // BUGBUG: Implement it later
    return S_OK;
}

HRESULT CMyHlinkSrc::GetMoniker(
     LPCWSTR pwzLocation,
     DWORD dwAssign,
     IMoniker **ppimkLocation)
{
    // BUGBUG: Implement it later
    return E_NOTIMPL;
}

HRESULT CMyHlinkSrc::GetFriendlyName(
     LPCWSTR pwzLocation,
     LPWSTR *ppwzFriendlyName)
{
    // BUGBUG: Implement it later
    return E_NOTIMPL;
}

//
// Almost identical copy of OleCreate, which allows us to pass
// the punkOuter.
//
HRESULT CMyHlinkSrc_OleCreate(CLSID rclsid, REFIID riid, DWORD renderOpt,
		   FORMATETC* pFormatEtc, IOleClientSite* pclient,
		   IStorage* pstg, LPVOID* ppvOut)
{
    HRESULT hres;
    *ppvOut = NULL;	// assume error

    IUnknown* punk;
    hres = CMyHlinkSrc_CreateInstance(rclsid, CLSCTX_INPROC, IID_IUnknown, (LPVOID*)&punk);
    if (SUCCEEDED(hres))
    {
	// Artificial one-time loop, which allows us to easily
	// handle error cases by saying "if (FAILED(hres)) break;"
	do {
	    // Call IPersistStorage::InitNew
	    IPersistStorage* ppstg;
	    hres = punk->QueryInterface(IID_IPersistStorage, (LPVOID*)&ppstg);
	    if (FAILED(hres))
		break;
	    hres = ppstg->InitNew(pstg);
	    ppstg->Release();
	    if (FAILED(hres))
		break;

	    // Call IOleObject::SetClientSite
	    IOleObject* pole;
	    hres = punk->QueryInterface(IID_IOleObject, (LPVOID*)&pole);
	    if (FAILED(hres))
		break;
	    hres = pole->SetClientSite(pclient);
	    pole->Release();
	    if (FAILED(hres))
		break;

	    hres = punk->QueryInterface(riid, ppvOut);
	} while (0);

	punk->Release();
    }
    return hres;
}

//
// Almost identical copy of OleLoad, which allows us to pass
// the punkOuter.
//
HRESULT CMyHlinkSrc_OleLoad(IStorage* pstg, REFIID riid,
		            IOleClientSite* pclient, LPVOID* ppvOut)
{
    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_OleLoad called");

    HRESULT hres;
    *ppvOut = NULL;	// assume error

    STATSTG statstg;
    hres = pstg->Stat(&statstg, STATFLAG_NONAME);
    if (SUCCEEDED(hres))
    {
	IUnknown* punk;
	hres = CMyHlinkSrc_CreateInstance(statstg.clsid, CLSCTX_INPROC, IID_IUnknown, (LPVOID*)&punk);
	if (SUCCEEDED(hres))
	{
	    // Artificial one-time loop, which allows us to easily
	    // handle error cases by saying "if (FAILED(hres)) break;"
	    do {
		// TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_OladLoad calling IPS::Load");
		// Call IPersistStorage::Load
		IPersistStorage* ppstg;
		hres = punk->QueryInterface(IID_IPersistStorage, (LPVOID*)&ppstg);
		if (FAILED(hres))
		    break;
		hres = ppstg->Load(pstg);
		ppstg->Release();
		if (FAILED(hres))
		    break;

		// TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_OladLoad calling IOO::SetClientSite");
		// Call IOleObject::SetClientSite
		IOleObject* pole;
		hres = punk->QueryInterface(IID_IOleObject, (LPVOID*)&pole);
		if (FAILED(hres))
		    break;
		hres = pole->SetClientSite(pclient);
		pole->Release();
		if (FAILED(hres))
		    break;

		// TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_OladLoad calling IUnk::QI");
		hres = punk->QueryInterface(riid, ppvOut);
	    } while (0);

	    punk->Release();
	}
    }

    // TraceMsg(DM_TRACE, "sdv TR CMyHlinkSrc_OleLoad is leaving");

    return hres;
}



