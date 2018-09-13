#include "priv.h"

class CMyHlink : public IHlink
{
public:
    // *** IUnknown methods ***
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG __stdcall AddRef(void) ;
    virtual ULONG __stdcall Release(void);

    // *** IOleWindow methods ***
    virtual HRESULT __stdcall SetHlinkSite(IHlinkSite *pihlSite);
    virtual HRESULT __stdcall GetHlinkSite(IHlinkSite **ppihlSite);
    virtual HRESULT __stdcall GetMonikerReference(
	 IMoniker **ppimk,
	 LPWSTR *ppwzLocation);
    virtual HRESULT __stdcall GetStringReference(LPWSTR *ppwzRefString);
    virtual HRESULT __stdcall GetFriendlyName(LPWSTR *ppwzFriendlyName);
    virtual HRESULT __stdcall Navigate(
	 IHlinkFrame *pihlFrame,
	 DWORD grfHLNF,
	 LPBC pbc,
	 DWORD dwbscCookie,
	 IBindStatusCallback *pibsc,
	 IHlinkBrowseContext *pihlbc);

protected:
    CMyHlink();
    ~CMyHlink();
    UINT	_cRef;
    IHlinkSite* _pihlSite;
    IMoniker*   _pmk;
    TCHAR	_szLocation[MAX_PATH];
};

CMyHlink::CMyHlink(IMoniker* pmk, LPCTSTR pszLocation, LPCSTR pszFriedlyName)
		    : _cRef(1), _pihlSite(NULL), _pmk(pmk)
{
    if (_pmk) {
	_pmk->AddRef();
    }
    if (pszLocation) {
	lstrcpy(_szLocation, pszLocation);
    } else {
	_szLocation[0] = '\0';
    }
}


CMyHlink::~CMyHlink()
{
    if (_pmk) {
	_pmk->Release();
    }

    if (_pihlSite) {
	_pihlSite->Release();
    }
}

HRESULT CMyHlink::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
}

ULONG CMyHlink::AddRef(void)
{
    return ++_cRef;
}

ULONG CMyHlink::Release(void)
{
    if (--_cRef > 0) {
	return _cRef;
    }

    delete this;
    return 0;
}

HRESULT CMyHlink::SetHlinkSite(IHlinkSite *pihlSite)
{
    if (_pihlSite) {
	_pihlSite->Release();
    }

    _pihlSite = pihlSite;

    if (_pihlSite) {
	_pihlSite->AddRefe();
    }

    return S_OK;
}

HRESULT CMyHlink::GetHlinkSite(IHlinkSite **ppihlSite)
{
    *ppihlSite = _pihlSite;
    if (_pihlSite) {
	_pihlSite->AddRefe();
    }

    return S_OK;
}

HRESULT CMyHlink::GetMonikerReference(
     IMoniker **ppimk,
     LPWSTR *ppwzLocation)
{
    if (ppimk) {
	*ppimk = _pmk;
	if (_pmk) {
	    _pmk->AddRef();
	}
    }

    // BUGBUG: Handle ppwszLocation as well!

    return S_OK;
}

HRESULT CMyHlink::GetStringReference(LPWSTR *ppwzRefString)
{
    // BUGBUG: Implete ment it later, if we ever need this.
    return E_NOTIMPL;
}

HRESULT CMyHlink::GetFriendlyName(LPWSTR *ppwzFriendlyName)
{

}

HRESULT CMyHlink::Navigate(
     IHlinkFrame *pihlFrame,
     DWORD grfHLNF,
     LPBC pbc,
     DWORD dwbscCookie,
     IBindStatusCallback *pibsc,
     IHlinkBrowseContext *pihlbc);

