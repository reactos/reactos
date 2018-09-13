#ifndef __SHEMBED_H__
#define __SHEMBED_H__

#include "caggunk.h"
#include "cwndproc.h"

//=========================================================================
// CShellEmbedding class definition
//
// NOTE: I'm killing the embeddingness of this class since we
// never shipped a control marked for embedding. If we need it
// back we can easily inherit from CImpIPersistStorage instead
// of IPersist. If you do this, make sure dvoc.cpp explicitly
// returns failure for QI for IPersistStorage or Trident won't
// host it.
//
//=========================================================================
class CShellEmbedding
    : public IPersist
    , public IOleObject               // Embedding MUST
    , public IViewObject2             // Embedding MUST
    , public IDataObject              // for Word/Excel
    , public IOleInPlaceObject        // In-Place MUST
    , public IOleInPlaceActiveObject  // In-Place MUST
    , public IInternetSecurityMgrSite
    , public CAggregatedUnknown
    , protected CImpWndProc
{
public:
    // *** IUnknown ***
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj)
        { return CAggregatedUnknown::QueryInterface(riid, ppvObj); }
    virtual ULONG __stdcall AddRef(void)
        { return CAggregatedUnknown::AddRef(); }
    virtual ULONG __stdcall Release(void)
        { return CAggregatedUnknown::Release(); }

    // *** IPersist ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);

    // *** IViewObject ***
    virtual STDMETHODIMP Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
        const RECTL *, const RECTL *, BOOL (*)(ULONG_PTR), ULONG_PTR);
    virtual STDMETHODIMP GetColorSet(DWORD, LONG, void *, DVTARGETDEVICE *,
        HDC, LOGPALETTE **);
    virtual STDMETHODIMP Freeze(DWORD, LONG, void *, DWORD *);
    virtual STDMETHODIMP Unfreeze(DWORD);
    virtual STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink *);
    virtual STDMETHODIMP GetAdvise(DWORD *, DWORD *, IAdviseSink **);

    // *** IViewObject2 ***
    virtual STDMETHODIMP GetExtent(DWORD, LONG, DVTARGETDEVICE *, LPSIZEL);

    // *** IOleObject ***
    virtual HRESULT __stdcall SetClientSite(IOleClientSite *pClientSite);
    virtual HRESULT __stdcall GetClientSite(IOleClientSite **ppClientSite);
    virtual HRESULT __stdcall SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    virtual HRESULT __stdcall Close(DWORD dwSaveOption);
    virtual HRESULT __stdcall SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk);
    virtual HRESULT __stdcall GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    virtual HRESULT __stdcall InitFromData(IDataObject *pDataObject,BOOL fCreation,DWORD dwReserved);
    virtual HRESULT __stdcall GetClipboardData(DWORD dwReserved,IDataObject **ppDataObject);
    virtual HRESULT __stdcall DoVerb(LONG iVerb,LPMSG lpmsg,IOleClientSite *pActiveSite,LONG lindex,HWND hwndParent,LPCRECT lprcPosRect);
    virtual HRESULT __stdcall EnumVerbs(IEnumOLEVERB **ppEnumOleVerb);
    virtual HRESULT __stdcall Update(void);
    virtual HRESULT __stdcall IsUpToDate(void);
    virtual HRESULT __stdcall GetUserClassID(CLSID *pClsid);
    virtual HRESULT __stdcall GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType);
    virtual HRESULT __stdcall SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    virtual HRESULT __stdcall GetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    virtual HRESULT __stdcall Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT __stdcall Unadvise(DWORD dwConnection);
    virtual HRESULT __stdcall EnumAdvise(IEnumSTATDATA **ppenumAdvise);
    virtual HRESULT __stdcall GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);
    virtual HRESULT __stdcall SetColorScheme(LOGPALETTE *pLogpal);

    // *** IDataObject ***
    virtual HRESULT __stdcall GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall QueryGetData(FORMATETC *pformatetc);
    virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
    virtual HRESULT __stdcall SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT __stdcall DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT __stdcall DUnadvise(DWORD dwConnection);
    virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

    // *** IOleWindow ***
    virtual HRESULT __stdcall GetWindow(HWND * lphwnd);
    virtual HRESULT __stdcall ContextSensitiveHelp(BOOL fEnterMode);

    // *** IOleInPlaceObject ***
    virtual HRESULT __stdcall InPlaceDeactivate(void);
    virtual HRESULT __stdcall UIDeactivate(void);
    virtual HRESULT __stdcall SetObjectRects(LPCRECT lprcPosRect,
        LPCRECT lprcClipRect);
    virtual HRESULT __stdcall ReactivateAndUndo(void);

    // *** IOleInPlaceActiveObject ***
    virtual HRESULT __stdcall TranslateAccelerator(LPMSG lpmsg);
    virtual HRESULT __stdcall OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT __stdcall OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT __stdcall ResizeBorder(LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow);
    virtual HRESULT __stdcall EnableModeless(BOOL fEnable);

protected:
    CShellEmbedding(IUnknown* punkOuter, LPCOBJECTINFO poi, const OLEVERB* pverbs=NULL);
    virtual ~CShellEmbedding();
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);

    virtual void _OnSetClientSite(void);    // called when we actually get a client site

    // Activation related -- this is the normal order these funcs get called
    HRESULT _DoActivateChange(IOleClientSite* pActiveSite, UINT uState, BOOL fForce); // figures out what to do
    virtual HRESULT _OnActivateChange(IOleClientSite* pActiveSite, UINT uState);// calls below
    virtual void _OnInPlaceActivate(void);      // called when we actually go in-place-active
    virtual void _OnUIActivate(void);           // called when we actually go ui-active
    virtual void _OnUIDeactivate(void);         // called when we actually go ui-deactive
    virtual void _OnInPlaceDeactivate(void);    // called when we actually deactivate

    // Window related
    virtual LRESULT v_WndProc(HWND, UINT, WPARAM, LPARAM);
    void _RegisterWindowClass(void);

    // Helper functions for subclasses
    HRESULT _CreateWindowOrSetParent(IOleWindow* pwin);
    HDC _OleStdCreateDC(DVTARGETDEVICE *ptd);

    void _ViewChange(DWORD dwAspect, LONG lindex);
    void _SendAdvise(UINT uCode);

    BOOL _ShouldDraw(LONG lindex);

    IOleClientSite*     _pcli;
    IOleClientSite*     _pcliHold;  // Save a pointer to our client site if we're DoVerbed after Close
    IAdviseSink*        _padv;
    DWORD               _advf;      // ADVF_ flags (p.166 OLE spec)
    DWORD               _asp;       // DVASPECT
    IStorage*           _pstg;
    SIZE                _size;
    SIZEL               _sizeHIM;       // HIMETRIC SetExtent size -- we pretty much ignore this.
    LPCOBJECTINFO       _pObjectInfo;   // pointer into global object array

    // BUGBUG: Load's OLE
    IOleAdviseHolder*   _poah;
    IDataAdviseHolder*  _pdah;

    // In-Place object specific
    RECT                _rcPos;
    RECT                _rcClip;
    IOleInPlaceSite*    _pipsite;
    IOleInPlaceFrame*   _pipframe;
    IOleInPlaceUIWindow* _pipui;
    OLEINPLACEFRAMEINFO _finfo;
    HWND                _hwndChild;
    const OLEVERB*      _pverbs;
    BOOL                _fDirty:1;
    BOOL                _fOpen:1;
    BOOL                _fUsingWindowRgn:1;
    UINT                _nActivate;
};

// Activation defines
#define OC_DEACTIVE         0
#define OC_INPLACEACTIVE    1
#define OC_UIACTIVE         2

//
//Copied from polyline.h in Inside OLE 2nd edition
//
//Code for CShellEmbedding::_SendAdvise
//......Code.....................Method called in CShellEmbedding::_SendAdvise
#define OBJECTCODE_SAVED       0 //IOleAdviseHolder::SendOnSave
#define OBJECTCODE_CLOSED      1 //IOleAdviseHolder::SendOnClose
#define OBJECTCODE_RENAMED     2 //IOleAdviseHolder::SendOnRename
#define OBJECTCODE_SAVEOBJECT  3 //IOleClientSite::SaveObject
#define OBJECTCODE_DATACHANGED 4 //IDataAdviseHolder::SendOnDataChange
#define OBJECTCODE_SHOWWINDOW  5 //IOleClientSite::OnShowWindow(TRUE)
#define OBJECTCODE_HIDEWINDOW  6 //IOleClientSite::OnShowWindow(FALSE)
#define OBJECTCODE_SHOWOBJECT  7 //IOleClientSite::ShowObject
#define OBJECTCODE_VIEWCHANGED 8 //IAdviseSink::OnViewChange

// A helper function in shembed.c
void PixelsToMetric(SIZEL* psize);
void MetricToPixels(SIZEL* psize);

//=========================================================================
// CSVVerb class definition
//=========================================================================
class CSVVerb : public IEnumOLEVERB
{
public:
    // *** IUnknown ***
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG __stdcall AddRef(void) ;
    virtual ULONG __stdcall Release(void);

    // *** IEnumOLEVERB ***
    virtual /* [local] */ HRESULT __stdcall Next(
        /* [in] */ ULONG celt,
        /* [out] */ LPOLEVERB rgelt,
        /* [out] */ ULONG *pceltFetched);

    virtual HRESULT __stdcall Skip(
        /* [in] */ ULONG celt);

    virtual HRESULT __stdcall Reset( void);

    virtual HRESULT __stdcall Clone(
        /* [out] */ IEnumOLEVERB **ppenum);

    CSVVerb(const OLEVERB* pverbs) : _cRef(1), _iCur(0), _pverbs(pverbs) {}

protected:
    UINT _cRef;
    UINT _iCur;
    const OLEVERB* _pverbs;
};


#endif // __SHEMBED_H__
