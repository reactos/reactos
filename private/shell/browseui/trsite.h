#ifndef _TRSITE_H
#define _TRSITE_H

class CTransitionSite;
class CTransitionSitePropertyBag;

/////////////////////////////////////////////////////////////////////////////
// Typedefs and Structs
/////////////////////////////////////////////////////////////////////////////
enum TransitionEvent    // Transition Event type
{
    teFirstEvent        = 0,

    teSiteEnter         = teFirstEvent,
    tePageEnter,
    teSiteExit,
    tePageExit,

    teNumEvents,    // NOTE: Must follow last event!

    teUserDefault       = teNumEvents
};

struct TRANSITIONINFO   // Transition Event information
{
    CLSID                           clsid;
    CTransitionSitePropertyBag *    pPropBag;
};

struct NAMEVALUE
{
    WCHAR * pwszName;
    VARIANT varValue;
};

/////////////////////////////////////////////////////////////////////////////
// CTransitionSite
/////////////////////////////////////////////////////////////////////////////
class CTransitionSite : public IViewFilter,
                        public IViewFilterSite,
                        public IAdviseSink,
                        public IDispatch
{
// Construction/Destruction
public:
    CTransitionSite(IShellBrowser * pcont);
    ~CTransitionSite();

    HRESULT _SetTransitionInfo(TransitionEvent te, TRANSITIONINFO * pti);

    HRESULT _ApplyTransition(BOOL bSiteChange);
    HRESULT _StartTransition();
    HRESULT _StopTransition();

    HRESULT _UpdateEventList();

    enum TRSTATE
    {
        TRSTATE_NONE            = 0,
        TRSTATE_INITIALIZING    = 1,
        TRSTATE_STARTPAINTING   = 2,
        TRSTATE_PAINTING        = 3
    };

    TRSTATE         _uState;
    IShellView *    _psvNew;        // Valid only while we are playing
    IViewObject *   _pvoNew;
    BOOL            _fViewIsVisible;
    HWND            _hwndViewNew;
    IViewFilter *   _pTransition;
    IDispatch *     _pDispTransition;
    DWORD           _dwTransitionSink;

// Data
private:
    IShellBrowser *     _pContainer;    // CBaseBrowser container of parent
    IViewFilterSite *   _pSite;

    TRANSITIONINFO *    _ptiCurrent;
    TRANSITIONINFO      _tiEventInfo[teNumEvents];

// Internal methods
private:
    HRESULT _LoadTransition();
    HRESULT _InitWait();
    HRESULT _OnComplete();

// Interfaces
public:
    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);
    
    // IViewFilter
    STDMETHOD(SetSource)        (IViewFilter *pFilter);
    STDMETHOD(GetSource)        (IViewFilter **ppFilter);
    STDMETHOD(SetSite)          (IViewFilterSite *pSink);
    STDMETHOD(GetSite)          (IViewFilterSite **ppSink);
    STDMETHOD(SetPosition)      (LPCRECT prc);
    STDMETHOD(Draw)             (HDC hdc, LPCRECT prc);
    STDMETHOD(GetStatusBits)    (DWORD *pdwFlags);
    
    // IViewFilterSite
    STDMETHOD(GetDC)                (LPCRECT prc, DWORD dwFlags, HDC *phdc);
    STDMETHOD(ReleaseDC)            (HDC hdc);
    STDMETHOD(InvalidateRect)       (LPCRECT prc, BOOL fErase);
    STDMETHOD(InvalidateRgn)        (HRGN hrgn, BOOL fErase);
    STDMETHOD(OnStatusBitsChange)   (DWORD dwFlags);

    // IAdviseSink
    STDMETHOD_(void, OnDataChange)  (FORMATETC * pFormatetc, STGMEDIUM * pStgmed) {}
    STDMETHOD_(void, OnViewChange)  (DWORD dwAspect, LONG lindex);
    STDMETHOD_(void, OnRename)      (IMoniker * pmk) {}
    STDMETHOD_(void, OnSave)        () {}
    STDMETHOD_(void, OnClose)       () {}

    // IDispatch
    STDMETHOD(GetTypeInfoCount) (UINT * pctinfo) { return E_NOTIMPL; }
    STDMETHOD(GetTypeInfo)      (UINT itinfo, LCID lcid, ITypeInfo ** pptinfo) { return E_NOTIMPL; }
    STDMETHOD(GetIDsOfNames)    (REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid) { return E_NOTIMPL; }
    STDMETHOD(Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
};

/////////////////////////////////////////////////////////////////////////////
// CTransitionSitePropertyBag
/////////////////////////////////////////////////////////////////////////////
class CTransitionSitePropertyBag : public IPropertyBag
{
// Construction/Destruction
public:
    CTransitionSitePropertyBag();
    virtual ~CTransitionSitePropertyBag();

    HRESULT _AddProperty(WCHAR * wszPropName, VARIANT * pvarValue);

// Data
protected:
    UINT    _cRef;
    HDPA    _hdpaProperties;

// Implementation
protected:
    static int _DPA_FreeProperties(LPVOID pv, LPVOID pData);

// Interfaces
public:
    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    // IPropertyBag
    STDMETHOD(Read) (LPCOLESTR pszPropName, VARIANT * pVar, IErrorLog * pErrorLog);
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT * pVar)
    { return E_NOTIMPL; }
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
HRESULT CLSIDFromTransitionName(LPCTSTR pszName, LPCLSID clsidName);
BOOL    ParseTransitionInfo(WCHAR * pwz, TRANSITIONINFO * pti);

#define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)

#endif  // _TRSITE_H
