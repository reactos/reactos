//+---------------------------------------------------------------------
//
//  File:       iwatch.cxx
//
//  Contents:   Interface wrapper for method invocation traces
//
//----------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include <hlink.h>
#endif

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#include <docobj.h>
#endif

static ULONG g_ulSSN;
static ULONG g_ulNestingLevel = 0;
static TCHAR g_szNULL[] = _T("(null)");     // string output for null string ptr

#define OUTPUTSTR(s)(s ? (s) : g_szNULL)	// wsprintf chokes on null string ptrs

#define INDENT_NESTING \
    TraceTagEx((tagTrackItfVerbose, TAG_NONEWLINE, "%*s", g_ulNestingLevel*4, "")); \
    InterlockedIncrement((LONG *)&g_ulNestingLevel)

#define INDENT_NESTING2 \
    TraceTagEx((tagTrackItfVerbose, TAG_NONEWLINE, "%*s", g_ulNestingLevel*4, ""));

#undef TRETURN
#define TRETURN(hr) \
    {                                                    \
        HRESULT hr2 = hr;                                \
        InterlockedDecrement((LONG *)&g_ulNestingLevel); \
        INDENT_NESTING2;                                 \
        TraceTagEx((tagTrackItfVerbose, TAG_NONAME, "%hr", hr2)); \
        SRETURN(hr2);                                    \
    }

typedef void (*PFNVOID)();

#define TAG_FLAGS  (TAG_NONAME | TAG_NONEWLINE)


//+---------------------------------------------------------------
//
//  Function:   GetIIDName
//
//----------------------------------------------------------------


EXTERN_C const GUID IID_IDispatchEx;
EXTERN_C const GUID IID_IInternetHostSecurityManager;
EXTERN_C const GUID IID_IPersistHistory;
EXTERN_C const GUID IID_ITargetContainer;

static const char *
GetIIDName(REFIID iid)
{

#define CASE_IID(itf) if (IID_##itf == iid) return #itf;

    CASE_IID(IDispatchEx)
    CASE_IID(IInternetHostSecurityManager)
    CASE_IID(IPersistHistory)
    CASE_IID(ITargetContainer)
    CASE_IID(IAdviseSink)
    CASE_IID(IAdviseSink2)
    CASE_IID(IBindCtx)
    CASE_IID(IClassFactory)
    CASE_IID(IOleParentUndoUnit)
    CASE_IID(IConnectionPoint)
    CASE_IID(IConnectionPointContainer)
    CASE_IID(IDataAdviseHolder)
    CASE_IID(IDataObject)
    CASE_IID(IDebug)
    CASE_IID(IDebugStream)
    CASE_IID(IDispatch)
    CASE_IID(IDropSource)
    CASE_IID(IDropTarget)
    CASE_IID(IEnumCallback)
    CASE_IID(IEnumFORMATETC)
    CASE_IID(IEnumGeneric)
    CASE_IID(IEnumHolder)
    CASE_IID(IEnumMoniker)
    CASE_IID(IEnumOLEVERB)
    CASE_IID(IEnumSTATDATA)
    CASE_IID(IEnumString)
    CASE_IID(IEnumUnknown)
    CASE_IID(IExternalConnection)
    CASE_IID(ILockBytes)
    CASE_IID(IMarshal)
    CASE_IID(IOleCommandTarget)
    CASE_IID(IOleDocument)
    CASE_IID(IOleDocumentView)
    CASE_IID(IOleDocumentSite)
    CASE_IID(IOleAdviseHolder)
    CASE_IID(IOleCache)
    CASE_IID(IOleCache2)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleContainer)
    CASE_IID(IOleControl)
    CASE_IID(IOleControlSite)
    CASE_IID(IOleInPlaceActiveObject)
    CASE_IID(IOleInPlaceFrame)
    CASE_IID(IOleInPlaceObject)
    CASE_IID(IOleInPlaceObjectWindowless)
    CASE_IID(IOleInPlaceSite)
    CASE_IID(IOleInPlaceSiteWindowless)
    CASE_IID(IOleInPlaceUIWindow)
    CASE_IID(IOleItemContainer)
    CASE_IID(IOleLink)
    CASE_IID(IOleManager)
    CASE_IID(IOleObject)
    CASE_IID(IOlePresObj)
    CASE_IID(IOleWindow)
    CASE_IID(IParseDisplayName)
    CASE_IID(IPerPropertyBrowsing)
    CASE_IID(IPersist)
    CASE_IID(IPersistFile)
    CASE_IID(IPersistStorage)
    CASE_IID(IPersistStream)
    CASE_IID(IPersistStreamInit)
    CASE_IID(IPersistPropertyBag)
    CASE_IID(IProvideClassInfo)
    CASE_IID(IQuickActivate)
    CASE_IID(IRunnableObject)
    CASE_IID(IServiceProvider)
    CASE_IID(ISpecifyPropertyPages)
    CASE_IID(IStorage)
    CASE_IID(IStream)
    CASE_IID(IUnknown)
    CASE_IID(IOleUndoUnit)
    CASE_IID(IOleUndoManager)
    CASE_IID(IViewObject)
    CASE_IID(IViewObject2)
    CASE_IID(IViewObjectEx)
#undef CASE_IID

    // The following is not thread safe.  The only bad thing that can
    // happen is that we get bogus debug output. It's not worth fixing.

    static char ach[32];
    wsprintfA(ach, "%08x", iid.Data1);
    return ach;
}

//---------------------------------------------------------------
//
//  Class:      CTrack
//
//---------------------------------------------------------------

class CTrack
{
public:

    void * __cdecl operator new(size_t cb);
    void __cdecl operator delete(void *pv);

    CTrack(IUnknown *pUnk, REFIID iid, char *pch, BOOL fTrackOnQI);

    virtual ~CTrack();

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppv);

    ULONG       _ulRefs;
    ULONG       _ulSSN;
    char *      _pch;
    BOOL        _fTrackOnQI;
    IUnknown *  _pUnk;          // ASM code below relies on this being the last member in the class!
};


inline void * __cdecl 
CTrack::operator new(size_t cb)
{
    return DbgExPostAlloc(malloc(DbgExPreAlloc(cb))); 
}

inline void __cdecl 
CTrack::operator delete(void *pv)
{
    free(DbgExPreFree(pv)); 
    DbgExPostFree(); 
}

CTrack::CTrack(IUnknown *pUnk, REFIID iid, char *pch, BOOL fTrackOnQI)
{
    _ulRefs = 1;
    _pUnk = pUnk;
    _ulSSN = g_ulSSN++;
    _pch = pch;
    _fTrackOnQI = fTrackOnQI;

    DbgExMemSetName(this, "TI %s p=%08x ssn=%d id=%s", pch, _pUnk, _ulSSN, GetIIDName(iid));
}

CTrack::~CTrack()
{ 
	_pUnk->Release(); 
}

ULONG STDMETHODCALLTYPE 
CTrack::Release()
{
    if (InterlockedDecrement((long *)&_ulRefs) == 0)
    {
        delete this;
        return 0;
    }

    return _ulRefs;
}

ULONG STDMETHODCALLTYPE 
CTrack::AddRef()
{ 
    return InterlockedIncrement((long *)&_ulRefs); 
}

HRESULT STDMETHODCALLTYPE 
CTrack::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr;
    
    hr = _pUnk->QueryInterface(iid, ppv);
    if (_fTrackOnQI)
    {
        DbgExTrackItf(iid, _pch, TRUE, ppv);
    }
    return hr;
}

#define BEGIN_TRACK(itf)\
class C##itf : public CTrack, public I##itf {\
public:\
    static IUnknown * Create(IUnknown *pUnk, char *pch, BOOL fTrackOnQI) { return new C##itf(pUnk, pch, fTrackOnQI); } \
    I##itf *_p; \
    C##itf(IUnknown *pUnk, char *pch, BOOL fTrackOnQI) : CTrack(pUnk, IID_I##itf, pch, fTrackOnQI), _p((I##itf *)pUnk) { } \
    ULONG STDMETHODCALLTYPE AddRef() { return CTrack::AddRef(); } \
    ULONG STDMETHODCALLTYPE Release() { return CTrack::Release(); } \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppv) { return CTrack::QueryInterface(iid, ppv); } \

#define END_TRACK() };

//---------------------------------------------------------------
//
//  Class:      CWrapArg
//
//---------------------------------------------------------------

class CWrapArg
{
public:
    CWrapArg(CTrack *pTrack, REFIID iid, void **ppv);
    ~CWrapArg();
    IUnknown ** _ppUnk;
};

CWrapArg::CWrapArg(CTrack *pTrack, REFIID iid, void **ppv)
{
    _ppUnk = (IUnknown **)ppv;
    if (*_ppUnk)
    {
        (*_ppUnk)->AddRef();
        DbgExTrackItf(iid, pTrack->_pch, TRUE, ppv);
    }
}

CWrapArg::~CWrapArg()
{
    if (*_ppUnk)
    {
        (*_ppUnk)->Release();
    }
}

//---------------------------------------------------------------
//
//  IOleObject
//
//---------------------------------------------------------------

BEGIN_TRACK(OleObject)

HRESULT STDMETHODCALLTYPE
SetClientSite(LPOLECLIENTSITE pClientSite)
{
    //CWrapArg arg(this, IID_IOleClientSite, (void **)&pClientSite);

    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetClientSite\n", _pch));
    TRETURN(_p->SetClientSite(pClientSite));
}

HRESULT STDMETHODCALLTYPE
GetClientSite(LPOLECLIENTSITE FAR* ppClientSite)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClientSite\n", _pch));
    TRETURN(_p->GetClientSite(ppClientSite));
}

HRESULT STDMETHODCALLTYPE
SetHostNames(LPCOLESTR szCntrApp, LPCOLESTR szCntrObj)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetHostNames(%ls,%ls)\n",
            _pch,
            OUTPUTSTR(szCntrApp),
            OUTPUTSTR(szCntrObj)));

    TRETURN(_p->SetHostNames(szCntrApp, szCntrObj));
}

HRESULT STDMETHODCALLTYPE
Close(DWORD reserved)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Close(%lu)\n", _pch, reserved));
    TRETURN(_p->Close(reserved));
}

HRESULT STDMETHODCALLTYPE
SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetMoniker\n", _pch));
    TRETURN(_p->SetMoniker(dwWhichMoniker, pmk));
}

HRESULT STDMETHODCALLTYPE
GetMoniker(DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER FAR* ppmk)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetMoniker\n", _pch));
    TRETURN(_p->GetMoniker(dwAssign, dwWhichMoniker, ppmk));
}

HRESULT STDMETHODCALLTYPE
InitFromData(LPDATAOBJECT pDataObject,
        BOOL fCreation,
        DWORD dwReserved)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InitFromData\n", _pch));
    TRETURN(_p->InitFromData(pDataObject, fCreation, dwReserved));
}

HRESULT STDMETHODCALLTYPE
GetClipboardData(DWORD dwReserved,
        LPDATAOBJECT FAR* ppDataObject)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClipboardData\n", _pch));
    TRETURN(_p->GetClipboardData(dwReserved, ppDataObject));
}

HRESULT STDMETHODCALLTYPE
DoVerb(LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPos)
{

    static char * apszVerbs[] = {
                                  "OLEIVERB_PRIMARY", //            (0L)
                                  "OLEIVERB_SHOW",    //            (-1L)
                                  "OLEIVERB_OPEN",    //            (-2L)
                                  "OLEIVERB_HIDE",    //            (-3L)
                                  "OLEIVERB_UIACTIVATE", //         (-4L)
                                  "OLEIVERB_INPLACEACTIVATE", //    (-5L)
                                  "OLEIVERB_DISCARDUNDOSTATE" //    (-6L)
                                };
    RECT rc = { 0 };

    INDENT_NESTING;

    if (lprcPos)
        rc = *lprcPos;

    if ((iVerb > 0) || (-iVerb > ARRAY_SIZE(apszVerbs)))
    {
        TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DoVerb(%ld,,,,,%s), pPos=(%d,%d,%d,%d)\n",
                    _pch, iVerb, ((lprcPos) ? "pPos" : "(null)"),
                    rc.left, rc.top, rc.right, rc.bottom
                    ));
    }
    else
    {
        TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DoVerb(%s,,,,,%s), pPos=(%d,%d,%d,%d)\n",
                    _pch, apszVerbs[-iVerb], ((lprcPos) ? "pPos" : NULL),
                    rc.left, rc.top, rc.right, rc.bottom
                    ));
    }

    TRETURN(_p->DoVerb(iVerb,
            lpmsg,
            pActiveSite,
            lindex,
            hwndParent,
            lprcPos));
}

HRESULT STDMETHODCALLTYPE
EnumVerbs(LPENUMOLEVERB FAR* ppenum)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnumVerbs\n", _pch));
    TRETURN(_p->EnumVerbs(ppenum));
}

HRESULT STDMETHODCALLTYPE
Update()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Update\n", _pch));
    TRETURN(_p->Update());
}

HRESULT STDMETHODCALLTYPE
IsUpToDate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsUpToDate\n", _pch));
    TRETURN(_p->IsUpToDate());
}

HRESULT STDMETHODCALLTYPE
GetUserClassID(CLSID FAR* pClsid)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetUserClassID\n", _pch));
    TRETURN(_p->GetUserClassID(pClsid));
}

HRESULT STDMETHODCALLTYPE
GetUserType(DWORD dwFormOfType, LPOLESTR FAR* ppch)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetUserType\n", _pch));
    TRETURN(_p->GetUserType(dwFormOfType, ppch));
}

HRESULT STDMETHODCALLTYPE
SetExtent(DWORD dwDrawAspect, SIZEL * lpsizel)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetExtent(%d, (cx=%d,cy=%d))\n",
                _pch, dwDrawAspect, lpsizel->cx, lpsizel->cy));
    TRETURN(_p->SetExtent(dwDrawAspect, lpsizel));
}

HRESULT STDMETHODCALLTYPE
GetExtent(DWORD dwDrawAspect, SIZEL * lpsizel)
{
    HRESULT hr;

    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetExtent\n", _pch));

    hr = _p->GetExtent(dwDrawAspect, lpsizel);

    InterlockedDecrement((LONG *)&g_ulNestingLevel);
    INDENT_NESTING2;
    TraceTagEx((tagTrackItfVerbose, TAG_NONAME, "%s::GetExtent Returning cx=%d,cy=%d, %hr.",
                _pch, lpsizel->cx, lpsizel->cy, hr));
    SRETURN(hr);
}

HRESULT STDMETHODCALLTYPE
Advise(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Advise\n", _pch));
    TRETURN(_p->Advise(pAdvSink, pdwConnection));
}

HRESULT STDMETHODCALLTYPE
Unadvise(DWORD dwConnection)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Unadvise\n", _pch));
    TRETURN(_p->Unadvise(dwConnection));
}

HRESULT STDMETHODCALLTYPE
EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnumAdvise\n", _pch));
    TRETURN(_p->EnumAdvise(ppenumAdvise));
}

HRESULT STDMETHODCALLTYPE
GetMiscStatus(DWORD dwAspect, DWORD FAR* pdwStatus)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetMiscStatus\n", _pch));
    TRETURN(_p->GetMiscStatus(dwAspect, pdwStatus));
}

HRESULT STDMETHODCALLTYPE
SetColorScheme(LPLOGPALETTE lpLogpal)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetColorScheme\n", _pch));
    TRETURN(_p->SetColorScheme(lpLogpal));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleClientSite
//
//---------------------------------------------------------------

BEGIN_TRACK(OleClientSite)

HRESULT STDMETHODCALLTYPE
SaveObject()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SaveObject\n", _pch));
    TRETURN(_p->SaveObject());
}

HRESULT STDMETHODCALLTYPE
GetMoniker(DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER FAR* ppmk)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetMoniker\n", _pch));
    TRETURN(_p->GetMoniker(dwAssign, dwWhichMoniker, ppmk));
}

HRESULT STDMETHODCALLTYPE
GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetContainer\n", _pch));
    TRETURN(_p->GetContainer(ppContainer));
}

HRESULT STDMETHODCALLTYPE
ShowObject()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ShowObject\n", _pch));
    TRETURN(_p->ShowObject());
}

HRESULT STDMETHODCALLTYPE
OnShowWindow(BOOL fShow)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnShowWindow\n", _pch));
    TRETURN(_p->OnShowWindow(fShow));
}

HRESULT STDMETHODCALLTYPE
RequestNewObjectLayout ()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::RequestNewObjectLayout\n", _pch));
    TRETURN(_p->RequestNewObjectLayout());
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleInPlaceSiteWindowless
//
//---------------------------------------------------------------

BEGIN_TRACK(OleInPlaceSiteWindowless)

HRESULT STDMETHODCALLTYPE
GetWindow(HWND * lphwnd)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindow\n", _pch));
    TRETURN(_p->GetWindow(lphwnd));
}

HRESULT STDMETHODCALLTYPE
ContextSensitiveHelp(BOOL fEnterMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ContextSensitiveHelp\n", _pch));
    TRETURN(_p->ContextSensitiveHelp(fEnterMode));
}

HRESULT STDMETHODCALLTYPE
CanInPlaceActivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::CanInPlaceActivate\n", _pch));
    TRETURN(_p->CanInPlaceActivate());
}

HRESULT STDMETHODCALLTYPE
OnInPlaceActivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnInPlaceActivate\n", _pch));
    TRETURN(_p->OnInPlaceActivate());
}

HRESULT STDMETHODCALLTYPE
OnInPlaceActivateEx(BOOL *pf, DWORD dw)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnInPlaceActivate\n", _pch));
    TRETURN(_p->OnInPlaceActivateEx(pf, dw));
}

HRESULT STDMETHODCALLTYPE
RequestUIActivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::RequestUIActivate\n", _pch));
    TRETURN(_p->RequestUIActivate());
}

HRESULT STDMETHODCALLTYPE
OnUIActivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnUIActivate\n", _pch));
    TRETURN(_p->OnUIActivate());
}

HRESULT STDMETHODCALLTYPE
GetWindowContext(
                    LPOLEINPLACEFRAME *    lplpFrame,
                    LPOLEINPLACEUIWINDOW * lplpDoc,
                    LPRECT                 lprcPosRect,
                    LPRECT                 lprcClipRect,
                    LPOLEINPLACEFRAMEINFO  lpFrameInfo)
{
    HRESULT hr;

    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindowContext\n", _pch));

    hr = _p->GetWindowContext(
                    lplpFrame,
                    lplpDoc,
                    lprcPosRect,
                    lprcClipRect,
                    lpFrameInfo);

    InterlockedDecrement((LONG *)&g_ulNestingLevel);
    INDENT_NESTING2;
    if (lprcPosRect && lprcClipRect)
    {
        TraceTagEx((tagTrackItfVerbose, TAG_NONAME, "%s::GetWindowContext Returning "
                    "Pos=(%d,%d,%d,%d), Clip=(%d,%d,%d,%d) %hr.",
                    _pch,
                    lprcPosRect->left, lprcPosRect->top,
                    lprcPosRect->right, lprcPosRect->bottom,
                    lprcClipRect->left, lprcClipRect->top,
                    lprcClipRect->right, lprcClipRect->bottom,
                    hr));
    }
    else
    {
        TraceTagEx((tagTrackItfVerbose, TAG_NONAME, "%s::GetWindowContext Returning "
                    "Pos=(null), Clip=(null) %hr.", _pch, hr));
    }

    SRETURN(hr);
}

HRESULT STDMETHODCALLTYPE
Scroll(SIZE extent)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Scroll\n", _pch));
    TRETURN(_p->Scroll(extent));
}

HRESULT STDMETHODCALLTYPE
OnUIDeactivate(BOOL fUndoable)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnUIDeactivate\n", _pch));
    TRETURN(_p->OnUIDeactivate(fUndoable));
}

HRESULT STDMETHODCALLTYPE
OnInPlaceDeactivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnInPlaceDeactivate\n", _pch));
    TRETURN(_p->OnInPlaceDeactivate());
}

HRESULT STDMETHODCALLTYPE
OnInPlaceDeactivateEx(BOOL f)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnInPlaceDeactivateEx\n", _pch));
    TRETURN(_p->OnInPlaceDeactivateEx(f));
}

HRESULT STDMETHODCALLTYPE
DiscardUndoState()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DiscardUndoState\n", _pch));
    TRETURN(_p->DiscardUndoState());
}

HRESULT STDMETHODCALLTYPE
DeactivateAndUndo()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DeactivateAndUndo\n", _pch));
    TRETURN(_p->DeactivateAndUndo());
}

HRESULT STDMETHODCALLTYPE
OnPosRectChange(LPCRECT lprcPosRect)
{
    INDENT_NESTING;
    if (lprcPosRect)
    {
        TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnPosRectChange -- "
                    "Pos=(%d,%d,%d,%d)\n",
                    _pch,
                    lprcPosRect->left, lprcPosRect->top,
                    lprcPosRect->right, lprcPosRect->bottom
                    ));
    }
    else
    {
        TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnPosRectChange -- "
                    "Pos=(null)\n", _pch));
    }
    TRETURN(_p->OnPosRectChange(lprcPosRect));
}

HRESULT STDMETHODCALLTYPE
CanWindowlessActivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::CanWindowlessActivate\n", _pch));
    TRETURN(_p->CanWindowlessActivate());
}

HRESULT STDMETHODCALLTYPE
GetCapture()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetCapture\n", _pch));
    TRETURN(_p->GetCapture());
}

HRESULT STDMETHODCALLTYPE
SetCapture(BOOL fCapture)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetCapture\n", _pch));
    TRETURN(_p->SetCapture(fCapture));
}

HRESULT STDMETHODCALLTYPE
GetFocus()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetFocus\n", _pch));
    TRETURN(_p->GetFocus());
}

HRESULT STDMETHODCALLTYPE
SetFocus(BOOL fFocus)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetFocus\n", _pch));
    TRETURN(_p->SetFocus(fFocus));
}

HRESULT STDMETHODCALLTYPE
OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnDefWindowMessage\n", _pch));
    TRETURN(_p->OnDefWindowMessage(msg, wParam, lParam, plResult));
}

HRESULT STDMETHODCALLTYPE
GetDC(LPCRECT prc, DWORD dwFlags, HDC * phDC)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetFocus\n", _pch));
    TRETURN(_p->GetDC(prc, dwFlags, phDC));
}

HRESULT STDMETHODCALLTYPE
ReleaseDC(HDC hdc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ReleaseDC\n", _pch));
    TRETURN(_p->ReleaseDC(hdc));
}

HRESULT STDMETHODCALLTYPE
InvalidateRect(LPCRECT prc, BOOL fErase)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InvalidateRect\n", _pch));
    TRETURN(_p->InvalidateRect(prc, fErase));
}

HRESULT STDMETHODCALLTYPE
InvalidateRgn(HRGN hrgn, BOOL fErase)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InvalidateRgn\n", _pch));
    TRETURN(_p->InvalidateRgn(hrgn, fErase));
}

HRESULT STDMETHODCALLTYPE
ScrollRect(int dx, int dy, LPCRECT lprcScroll, LPCRECT lprcClip)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ScrollRect\n", _pch));
    TRETURN(_p->ScrollRect(dx, dy, lprcScroll, lprcClip));
}

HRESULT STDMETHODCALLTYPE
AdjustRect(LPRECT prc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::AdjustRect\n", _pch));
    TRETURN(_p->AdjustRect(prc));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IDataObject
//
//---------------------------------------------------------------

BEGIN_TRACK(DataObject)

HRESULT STDMETHODCALLTYPE
GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetData\n", _pch));
    TRETURN(_p->GetData(pformatetc, pmedium));
}

HRESULT STDMETHODCALLTYPE
GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetDataHere\n", _pch));
    TRETURN(_p->GetDataHere(pformatetc, pmedium));
}

HRESULT STDMETHODCALLTYPE
QueryGetData(LPFORMATETC pformatetc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::QueryGetData\n", _pch));
    TRETURN(_p->QueryGetData(pformatetc));
}

HRESULT STDMETHODCALLTYPE
GetCanonicalFormatEtc(LPFORMATETC pformatetc,
        LPFORMATETC pformatetcOut)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetCanonicalFormat\n", _pch));
    TRETURN(_p->GetCanonicalFormatEtc(pformatetc, pformatetcOut));
}

HRESULT STDMETHODCALLTYPE
SetData(LPFORMATETC pformatetc,
        STGMEDIUM FAR *pmedium, BOOL fRelease)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetData\n", _pch));
    TRETURN(_p->SetData(pformatetc, pmedium, fRelease));
}

HRESULT STDMETHODCALLTYPE
EnumFormatEtc(DWORD dwDirection,
        LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnumFormatEtc\n", _pch));
    TRETURN(_p->EnumFormatEtc(dwDirection, ppenumFormatEtc));
}

HRESULT STDMETHODCALLTYPE
DAdvise(FORMATETC FAR* pFormatetc, DWORD advf,
        LPADVISESINK pAdvSink, DWORD FAR* pdwConnection)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DAdvise\n", _pch));
    TRETURN(_p->DAdvise(pFormatetc, advf, pAdvSink, pdwConnection));
}

HRESULT STDMETHODCALLTYPE
DUnadvise(DWORD dwConnection)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::DUnadvise\n", _pch));
    TRETURN(_p->DUnadvise(dwConnection));
}

HRESULT STDMETHODCALLTYPE
EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnumDAdvise\n", _pch));
    TRETURN(_p->EnumDAdvise(ppenumAdvise));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IViewObject
//
//---------------------------------------------------------------

BEGIN_TRACK(ViewObject)

HRESULT STDMETHODCALLTYPE
Draw(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (ULONG_PTR), ULONG_PTR dwContinue)
{
    INDENT_NESTING;
    RECTL  rc = { 0 }, rcBounds = { 0 };

    if (lprectl)
        rc = *lprectl;

    if (lprcWBounds)
        rcBounds = *lprcWBounds;

    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Draw -- rc=(%d,%d,%d,%d), "
                "rcBounds=(%d,%d,%d,%d)\n", _pch,
                rc.left, rc.top, rc.right, rc.bottom,
                rcBounds.left, rcBounds.top, rcBounds.right, rcBounds.bottom));

    TRETURN(_p->Draw(dwDrawAspect,
            lindex,
            pvAspect,
            ptd,
            hicTargetDev,
            hdcDraw,
            lprectl,
            lprcWBounds,
            pfnContinue,
            dwContinue));
}

HRESULT STDMETHODCALLTYPE
GetColorSet(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        LPLOGPALETTE FAR* ppColorSet)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetColorSet\n", _pch));
    TRETURN(_p->GetColorSet(dwDrawAspect,
            lindex,
            pvAspect,
            ptd,
            hicTargetDev,
            ppColorSet));
}

HRESULT STDMETHODCALLTYPE
Freeze(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DWORD FAR* pdwFreeze)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Freeze\n", _pch));
    TRETURN(_p->Freeze(dwDrawAspect, lindex, pvAspect, pdwFreeze));
}

HRESULT STDMETHODCALLTYPE
Unfreeze(DWORD dwFreeze)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Unfreeze\n", _pch));
    TRETURN(_p->Unfreeze(dwFreeze));
}

HRESULT STDMETHODCALLTYPE
SetAdvise(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetAdvise\n", _pch));
    TRETURN(_p->SetAdvise(aspects, advf, pAdvSink));
}

HRESULT STDMETHODCALLTYPE
GetAdvise(DWORD FAR* pAspects,
        DWORD FAR* pAdvf,
        LPADVISESINK FAR* ppAdvSink)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetAdvise\n", _pch));
    TRETURN(_p->GetAdvise(pAspects, pAdvf, ppAdvSink));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IPersistStorage
//
//---------------------------------------------------------------

BEGIN_TRACK(PersistStorage)


HRESULT STDMETHODCALLTYPE
GetClassID(LPCLSID lpClassID)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClassID\n", _pch));
    TRETURN(_p->GetClassID(lpClassID));
}

HRESULT STDMETHODCALLTYPE
IsDirty()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsDirty\n", _pch));
    TRETURN(_p->IsDirty());
}

HRESULT STDMETHODCALLTYPE
InitNew(LPSTORAGE pStg)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InitNew\n", _pch));
    TRETURN(_p->InitNew(pStg));
}

HRESULT STDMETHODCALLTYPE
Load(LPSTORAGE pStg)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Load(stg)\n", _pch));
    TRETURN(_p->Load(pStg));
}

HRESULT STDMETHODCALLTYPE
Save(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Save(stg,)\n", _pch));
    TRETURN(_p->Save(pStg, fSameAsLoad));
}

HRESULT STDMETHODCALLTYPE
SaveCompleted(LPSTORAGE pStg)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SaveCompleted(stg)\n", _pch));
    TRETURN(_p->SaveCompleted(pStg));
}

HRESULT STDMETHODCALLTYPE
HandsOffStorage()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::HandsOffStorage\n", _pch));
    TRETURN(_p->HandsOffStorage());
}

END_TRACK()

//---------------------------------------------------------------
//
//  IPersistFile
//
//---------------------------------------------------------------

BEGIN_TRACK(PersistFile)

HRESULT STDMETHODCALLTYPE
GetClassID(LPCLSID lpClassID)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClassID\n", _pch));
    TRETURN(_p->GetClassID(lpClassID));
}

HRESULT STDMETHODCALLTYPE
IsDirty()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsDirty\n", _pch));
    TRETURN(_p->IsDirty());
}

HRESULT STDMETHODCALLTYPE
Load(LPCOLESTR lpszFileName, DWORD grfMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Load(%ls,)\n", _pch, OUTPUTSTR(lpszFileName)));
    TRETURN(_p->Load(lpszFileName, grfMode));
}

HRESULT STDMETHODCALLTYPE
Save(LPCOLESTR lpszFileName, BOOL fRemember)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Save(%ls,)\n", _pch, OUTPUTSTR(lpszFileName)));
    TRETURN(_p->Save(lpszFileName, fRemember));
}

HRESULT STDMETHODCALLTYPE
SaveCompleted(LPCOLESTR lpszFileName)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SaveCompleted(%ls)\n", _pch, OUTPUTSTR(lpszFileName)));
    TRETURN(_p->SaveCompleted(lpszFileName));
}

HRESULT STDMETHODCALLTYPE
GetCurFile (LPOLESTR FAR * ppstrFile)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetCurFile\n", _pch));
    TRETURN(_p->GetCurFile(ppstrFile));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IPersistStream
//
//---------------------------------------------------------------

BEGIN_TRACK(PersistStream)

HRESULT STDMETHODCALLTYPE
GetClassID(LPCLSID lpClassID)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClassID\n", _pch));
    TRETURN(_p->GetClassID(lpClassID));
}

HRESULT STDMETHODCALLTYPE
IsDirty()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsDirty\n", _pch));
    TRETURN(_p->IsDirty());
}

HRESULT STDMETHODCALLTYPE
Load(IStream * pStrm)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Load(strm)\n", _pch));
    TRETURN(_p->Load(pStrm));
}

HRESULT STDMETHODCALLTYPE
Save(IStream * pStrm, BOOL fClearDirty)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Save(strm,)\n", _pch));
    TRETURN(_p->Save(pStrm, fClearDirty));
}

HRESULT STDMETHODCALLTYPE
GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetSizeMax\n", _pch));
    TRETURN(_p->GetSizeMax(pcbSize));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IPersistStreamInit
//
//---------------------------------------------------------------

BEGIN_TRACK(PersistStreamInit)

HRESULT STDMETHODCALLTYPE
GetClassID(LPCLSID lpClassID)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClassID\n", _pch));
    TRETURN(_p->GetClassID(lpClassID));
}

HRESULT STDMETHODCALLTYPE
IsDirty()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsDirty\n", _pch));
    TRETURN(_p->IsDirty());
}

HRESULT STDMETHODCALLTYPE
Load(IStream * pStrm)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Load(strm)\n", _pch));
    TRETURN(_p->Load(pStrm));
}

HRESULT STDMETHODCALLTYPE
Save(IStream * pStrm, BOOL fClearDirty)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Save(strm,)\n", _pch));
    TRETURN(_p->Save(pStrm, fClearDirty));
}

HRESULT STDMETHODCALLTYPE
GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetSizeMax\n", _pch));
    TRETURN(_p->GetSizeMax(pcbSize));
}

HRESULT STDMETHODCALLTYPE
InitNew()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InitNew [Stream]\n", _pch));
    TRETURN(_p->InitNew());
}

END_TRACK()

//---------------------------------------------------------------
//
//  IPersistPropertyBag
//
//---------------------------------------------------------------

BEGIN_TRACK(PersistPropertyBag)

HRESULT STDMETHODCALLTYPE
GetClassID(LPCLSID lpClassID)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetClassID\n", _pch));
    TRETURN(_p->GetClassID(lpClassID));
}

HRESULT STDMETHODCALLTYPE
InitNew()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InitNew [PropertyBag]\n", _pch));
    TRETURN(_p->InitNew());
}

HRESULT STDMETHODCALLTYPE
Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrLog)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Load(propbag)\n", _pch));
    TRETURN(_p->Load(pPropBag, pErrLog));
}

HRESULT STDMETHODCALLTYPE
Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Save(propbag,)\n", _pch));
    TRETURN(_p->Save(pPropBag, fClearDirty, fSaveAllProperties));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleInPlaceObject
//
//---------------------------------------------------------------

BEGIN_TRACK(OleInPlaceObject)


HRESULT STDMETHODCALLTYPE
GetWindow(HWND FAR* lphwnd)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindow\n", _pch));
    TRETURN(_p->GetWindow(lphwnd));
}

HRESULT STDMETHODCALLTYPE
ContextSensitiveHelp(BOOL fEnterMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ContextSensitiveHelp\n", _pch));
    TRETURN(_p->ContextSensitiveHelp(fEnterMode));
}

HRESULT STDMETHODCALLTYPE
InPlaceDeactivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InPlaceDeactivate\n", _pch));
    TRETURN(_p->InPlaceDeactivate());
}

HRESULT STDMETHODCALLTYPE
UIDeactivate()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::UIDeactivate\n", _pch));
    TRETURN(_p->UIDeactivate());
}

HRESULT STDMETHODCALLTYPE
SetObjectRects(LPCRECT lprcPos, LPCRECT lprcVisRect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetObjectRects(pPos,pClip) Pos=(%d,%d,%d,%d), Clip=(%d,%d,%d,%d)\n",
                _pch,
                lprcPos->left, lprcPos->top, lprcPos->right, lprcPos->bottom,
                lprcVisRect->left, lprcVisRect->top, lprcVisRect->right, lprcVisRect->bottom
                ));
    TRETURN(_p->SetObjectRects(lprcPos, lprcVisRect));
}

HRESULT STDMETHODCALLTYPE
ReactivateAndUndo()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ReactivateAndUndo\n", _pch));
    TRETURN(_p->ReactivateAndUndo());
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleControl
//
//---------------------------------------------------------------

BEGIN_TRACK(OleControl)

HRESULT STDMETHODCALLTYPE
GetControlInfo(CONTROLINFO * pCI)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetControlInfo\n", _pch));
    TRETURN(_p->GetControlInfo(pCI));
}

HRESULT STDMETHODCALLTYPE
OnMnemonic(LPMSG pMsg)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnMnemonic\n", _pch));
    TRETURN(_p->OnMnemonic(pMsg));
}

HRESULT STDMETHODCALLTYPE
OnAmbientPropertyChange(DISPID dispid)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnAmbientPropertyChange\n", _pch));
    TRETURN(_p->OnAmbientPropertyChange(dispid));
}

HRESULT STDMETHODCALLTYPE
FreezeEvents(BOOL fFreeze)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::FreezeEvents(%s)\n", _pch,
               ((fFreeze) ? "TRUE" : "FALSE")));
    TRETURN(_p->FreezeEvents(fFreeze));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleInPlaceActiveObject
//
//---------------------------------------------------------------

BEGIN_TRACK(OleInPlaceActiveObject)

HRESULT STDMETHODCALLTYPE
GetWindow(HWND FAR* lphwnd)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindow\n", _pch));
    TRETURN(_p->GetWindow(lphwnd));
}

HRESULT STDMETHODCALLTYPE
ContextSensitiveHelp(BOOL fEnterMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ContextSensitiveHelp\n", _pch));
    TRETURN(_p->ContextSensitiveHelp(fEnterMode));
}

HRESULT STDMETHODCALLTYPE
TranslateAccelerator(LPMSG lpmsg)
{
    return _p->TranslateAccelerator(lpmsg);
}

HRESULT STDMETHODCALLTYPE
OnFrameWindowActivate(BOOL fActivate)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnFrameWindowActivate\n", _pch));
    TRETURN(_p->OnFrameWindowActivate(fActivate));
}

HRESULT STDMETHODCALLTYPE
OnDocWindowActivate(BOOL fActivate)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnDocWindowActivate\n", _pch));
    TRETURN(_p->OnDocWindowActivate(fActivate));
}

HRESULT STDMETHODCALLTYPE
ResizeBorder(LPCRECT lprc,
        LPOLEINPLACEUIWINDOW pUIWindow,
        BOOL fFrameWindow)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ResizeBorder\n", _pch));
    TRETURN(_p->ResizeBorder(lprc, pUIWindow, fFrameWindow));
}

HRESULT STDMETHODCALLTYPE
EnableModeless(BOOL fEnable)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnableModeless\n", _pch));
    TRETURN(_p->EnableModeless(fEnable));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleInPlaceFrame
//
//---------------------------------------------------------------

BEGIN_TRACK(OleInPlaceFrame)

HRESULT STDMETHODCALLTYPE
GetWindow(HWND FAR* lphwnd)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindow\n", _pch));
    TRETURN(_p->GetWindow(lphwnd));
}

HRESULT STDMETHODCALLTYPE
ContextSensitiveHelp(BOOL fEnterMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ContextSensitiveHelp\n", _pch));
    TRETURN(_p->ContextSensitiveHelp(fEnterMode));
}

HRESULT STDMETHODCALLTYPE
GetBorder(LPRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetBorder\n", _pch));
    TRETURN(_p->GetBorder(lprect));
}

HRESULT STDMETHODCALLTYPE
RequestBorderSpace(LPCRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::RequestBorderSpace\n", _pch));
    TRETURN(_p->RequestBorderSpace(lprect));
}

HRESULT STDMETHODCALLTYPE
SetBorderSpace(LPCRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetBorderSpace\n", _pch));
    TRETURN(_p->SetBorderSpace(lprect));
}

HRESULT STDMETHODCALLTYPE
SetActiveObject(LPOLEINPLACEACTIVEOBJECT lpActiveObject,
            LPCOLESTR lpszObjName)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetActiveObject\n", _pch));
    TRETURN(_p->SetActiveObject(lpActiveObject, lpszObjName));
}

HRESULT STDMETHODCALLTYPE
InsertMenus(HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::InsertMenus\n", _pch));
    TRETURN(_p->InsertMenus(hmenuShared, lpMenuWidths));
}

HRESULT STDMETHODCALLTYPE
SetMenu(HMENU hmenuShared,
        HOLEMENU holemenu,
        HWND hwndActiveObject)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetMenu\n", _pch));
    TRETURN(_p->SetMenu(hmenuShared, holemenu, hwndActiveObject));
}

HRESULT STDMETHODCALLTYPE
RemoveMenus(HMENU hmenuShared)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::RemoveMenus\n", _pch));
    TRETURN(_p->RemoveMenus(hmenuShared));
}

HRESULT STDMETHODCALLTYPE
SetStatusText(LPCOLESTR lpszStatusText)
{
    return _p->SetStatusText(lpszStatusText);
}

HRESULT STDMETHODCALLTYPE
EnableModeless(BOOL fEnable)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnableModeless\n", _pch));
    TRETURN(_p->EnableModeless(fEnable));
}

HRESULT STDMETHODCALLTYPE
TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    return _p->TranslateAccelerator(lpmsg, wID);
}

END_TRACK()

//---------------------------------------------------------------
//
//  IOleInPlaceUIWindow
//
//---------------------------------------------------------------

BEGIN_TRACK(OleInPlaceUIWindow)

HRESULT STDMETHODCALLTYPE
GetWindow(HWND FAR* lphwnd)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetWindow\n", _pch));
    TRETURN(_p->GetWindow(lphwnd));
}

HRESULT STDMETHODCALLTYPE
ContextSensitiveHelp(BOOL fEnterMode)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::ContextSensitiveHelp\n", _pch));
    TRETURN(_p->ContextSensitiveHelp(fEnterMode));
}

HRESULT STDMETHODCALLTYPE
GetBorder(LPRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetBorder\n", _pch));
    TRETURN(_p->GetBorder(lprect));
}

HRESULT STDMETHODCALLTYPE
RequestBorderSpace(LPCRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::RequestBorderSpace\n", _pch));
    TRETURN(_p->RequestBorderSpace(lprect));
}

HRESULT STDMETHODCALLTYPE
SetBorderSpace(LPCRECT lprect)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetBorderSpace\n", _pch));
    TRETURN(_p->SetBorderSpace(lprect));
}

HRESULT STDMETHODCALLTYPE
SetActiveObject(LPOLEINPLACEACTIVEOBJECT lpActiveObject,
            LPCOLESTR lpszObjName)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetActiveObject\n", _pch));
    TRETURN(_p->SetActiveObject(lpActiveObject, lpszObjName));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IAdviseSink
//
//---------------------------------------------------------------

BEGIN_TRACK(AdviseSink)

void STDMETHODCALLTYPE
OnDataChange(FORMATETC FAR* pFormatetc,
        STGMEDIUM FAR* pStgmed)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnDataChange\n", _pch));
    _p->OnDataChange(pFormatetc, pStgmed);
    InterlockedDecrement((LONG *)&g_ulNestingLevel);
}

void STDMETHODCALLTYPE
OnViewChange(DWORD dwAspects, LONG lindex)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnViewChange\n", _pch));
    _p->OnViewChange(dwAspects, lindex);
    InterlockedDecrement((LONG *)&g_ulNestingLevel);
}

void STDMETHODCALLTYPE
OnRename(LPMONIKER pmk)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnRename\n", _pch));
    _p->OnRename(pmk);
    InterlockedDecrement((LONG *)&g_ulNestingLevel);
}

void STDMETHODCALLTYPE
OnSave()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnSave\n", _pch));
    _p->OnSave();
    InterlockedDecrement((LONG *)&g_ulNestingLevel);
}

void STDMETHODCALLTYPE
OnClose()
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::OnClose\n", _pch));
    _p->OnClose();
    InterlockedDecrement((LONG *)&g_ulNestingLevel);
}

END_TRACK()


//---------------------------------------------------------------
//
//  IOleItemContainer
//
//---------------------------------------------------------------

BEGIN_TRACK(OleItemContainer)


HRESULT STDMETHODCALLTYPE
ParseDisplayName(LPBC pbc,
        LPOLESTR lpszDisplayName,
        ULONG FAR* pchEaten,
        LPMONIKER FAR* ppmkOut)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS,
                   "%s::ParseDisplayName(,%ls,,)\n",
                   _pch,
                   OUTPUTSTR(lpszDisplayName)));

    TRETURN(_p->ParseDisplayName(pbc, lpszDisplayName, pchEaten, ppmkOut));
}

HRESULT STDMETHODCALLTYPE
EnumObjects(DWORD grfFlags,
        LPENUMUNKNOWN FAR* ppenumUnknown)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::EnumObjects\n", _pch));
    TRETURN(_p->EnumObjects(grfFlags, ppenumUnknown));
}

HRESULT STDMETHODCALLTYPE
LockContainer(BOOL fLock)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::LockContainer\n", _pch));
    TRETURN(_p->LockContainer(fLock));
}


HRESULT STDMETHODCALLTYPE
GetObject(LPOLESTR lpszItem,
        DWORD dwSpeedNeeded,
        LPBINDCTX pbc,
        REFIID iid, 
        LPVOID FAR* ppvObject)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetObject(%ls,,,,)\n", _pch, OUTPUTSTR(lpszItem)));
    TRETURN(_p->GetObject(lpszItem, dwSpeedNeeded, pbc, iid,  ppvObject));
}

HRESULT STDMETHODCALLTYPE
GetObjectStorage(LPOLESTR lpszItem,
        LPBINDCTX pbc,
        REFIID iid, 
        LPVOID FAR* ppvStorage)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetStorage(%ls,,,)\n", _pch, OUTPUTSTR(lpszItem)));
    TRETURN(_p->GetObjectStorage(lpszItem, pbc, iid,  ppvStorage));
}

HRESULT STDMETHODCALLTYPE
IsRunning(LPOLESTR lpszItem)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::IsRunning(%ls)\n", _pch, OUTPUTSTR(lpszItem)));
    TRETURN(_p->IsRunning(lpszItem));
}

END_TRACK()


//---------------------------------------------------------------
//
//  IOleCommandTarget
//
//---------------------------------------------------------------

BEGIN_TRACK(OleCommandTarget)

HRESULT STDMETHODCALLTYPE
QueryStatus(
            const GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext)
{
    INDENT_NESTING;

    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::QueryStatus(group %s, [",
        _pch, pguidCmdGroup ? "non-null" : "NULL"));
    ULONG i;
    for (i=0; i<cCmds; i++)
        TraceTagEx((tagTrackItfVerbose, TAG_FLAGS,
                       " %d",rgCmds[i].cmdID));
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, " ])\n"));

    TRETURN(_p->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext));
}

HRESULT STDMETHODCALLTYPE
Exec(
            const GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Exec(group %s, cmd %d, opts %x)\n",
        _pch, pguidCmdGroup ? "non-null" : "NULL", nCmdID, nCmdexecopt));

    TRETURN(_p->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IHlinkTarget
//
//---------------------------------------------------------------

BEGIN_TRACK(HlinkTarget)

HRESULT STDMETHODCALLTYPE
SetBrowseContext(IHlinkBrowseContext *pihlbc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::SetBrowseContext\n", _pch));
    TRETURN(_p->SetBrowseContext(pihlbc));
}

HRESULT STDMETHODCALLTYPE
GetBrowseContext(IHlinkBrowseContext **ppihlbc)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetBrowseContext\n", _pch));
    TRETURN(_p->GetBrowseContext(ppihlbc));
}

HRESULT STDMETHODCALLTYPE
Navigate(DWORD grfHLNF, LPCWSTR wzJumpLocation)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::Navigate(grfHLNF=%x, location=%ls)\n", _pch,
        grfHLNF, OUTPUTSTR(wzJumpLocation)));
    TRETURN(_p->Navigate(grfHLNF, wzJumpLocation));
}

HRESULT STDMETHODCALLTYPE
GetMoniker(LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetMoniker(dwAssign=%x, location=%ls)\n", _pch,
        dwAssign, OUTPUTSTR(wzLocation)));
    TRETURN(_p->GetMoniker(wzLocation, dwAssign, ppimkLocation));
}

HRESULT STDMETHODCALLTYPE
GetFriendlyName(LPCWSTR wzLocation, LPWSTR *pwzFriendlyName)
{
    INDENT_NESTING;
    TraceTagEx((tagTrackItfVerbose, TAG_FLAGS, "%s::GetFriendlyName(location=%ls)\n", _pch,
        OUTPUTSTR(wzLocation)));
    TRETURN(_p->GetFriendlyName(wzLocation, pwzFriendlyName));
}

END_TRACK()

//---------------------------------------------------------------
//
//  IUnknown
//
//---------------------------------------------------------------

#if defined(_M_IX86)

class CUnknownTrack : public CTrack
{   
public:

    CUnknownTrack(IUnknown *p, REFIID iid, char *pch, BOOL fTrackOnQI);
    void *_apfnVTbl;
};

#define THUNK_IMPL(n)\
void __declspec(naked) TrackThunk##n()\
{                                           \
    /* this = thisArg                   */  \
    __asm mov eax, [esp + 4]                \
    /* pUnkObject = this->_p            */  \
    __asm mov ecx, [eax - 4]                \
    /* thisArg = pvObject               */  \
    __asm mov [esp + 4], ecx                \
    /* vtbl = punkObject->vtbl          */  \
    __asm mov ecx, [ecx]                    \
    /* pfn = vtbl[n]                    */  \
    __asm mov ecx, [ecx + (4 * n)]          \
    /* jump....                         */  \
    __asm jmp ecx                           \
}


#define THUNK_ADDR(n) &TrackThunk##n,

static ULONG STDMETHODCALLTYPE
UnknownTrackAddRef(void *pv)
{
    return ((CUnknownTrack *)((BYTE *)pv - offsetof(CUnknownTrack, _apfnVTbl)))->AddRef();
}

static ULONG STDMETHODCALLTYPE
UnknownTrackRelease(void *pv)
{
    return ((CUnknownTrack *)((BYTE *)pv - offsetof(CUnknownTrack, _apfnVTbl)))->Release();
}

static HRESULT STDMETHODCALLTYPE
UnknownTrackQueryInterface(void *pv, REFIID iid, void **ppv)
{
    return ((CUnknownTrack *)((BYTE *)pv - offsetof(CUnknownTrack, _apfnVTbl)))->QueryInterface(iid, ppv);
}

#define THUNK_IT(x) \
THUNK_##x(3)   THUNK_##x(4)   THUNK_##x(5)   THUNK_##x(6)   THUNK_##x(7)   THUNK_##x(8)   THUNK_##x(9)   THUNK_##x(10)  THUNK_##x(11)  THUNK_##x(12)  THUNK_##x(13)  \
THUNK_##x(14)  THUNK_##x(15)  THUNK_##x(16)  THUNK_##x(17)  THUNK_##x(18)  THUNK_##x(19)  THUNK_##x(20)  THUNK_##x(21)  THUNK_##x(22)  THUNK_##x(23)  THUNK_##x(24)  \
THUNK_##x(25)  THUNK_##x(26)  THUNK_##x(27)  THUNK_##x(28)  THUNK_##x(29)  THUNK_##x(30)  THUNK_##x(31)  THUNK_##x(32)  THUNK_##x(33)  THUNK_##x(34)  THUNK_##x(35)  \
THUNK_##x(36)  THUNK_##x(37)  THUNK_##x(38)  THUNK_##x(39)  THUNK_##x(40)  THUNK_##x(41)  THUNK_##x(42)  THUNK_##x(43)  THUNK_##x(44)  THUNK_##x(45)  THUNK_##x(46)  \
THUNK_##x(47)  THUNK_##x(48)  THUNK_##x(49)  THUNK_##x(50)  THUNK_##x(51)  THUNK_##x(52)  THUNK_##x(53)  THUNK_##x(54)  THUNK_##x(55)  THUNK_##x(56)  THUNK_##x(57)  \
THUNK_##x(58)  THUNK_##x(59)  THUNK_##x(60)  THUNK_##x(61)  THUNK_##x(62)  THUNK_##x(63)  THUNK_##x(64)  THUNK_##x(65)  THUNK_##x(66)  THUNK_##x(67)  THUNK_##x(68)  \
THUNK_##x(69)  THUNK_##x(70)  THUNK_##x(71)  THUNK_##x(72)  THUNK_##x(73)  THUNK_##x(74)  THUNK_##x(75)  THUNK_##x(76)  THUNK_##x(77)  THUNK_##x(78)  THUNK_##x(79)  \
THUNK_##x(80)  THUNK_##x(81)  THUNK_##x(82)  THUNK_##x(83)  THUNK_##x(84)  THUNK_##x(85)  THUNK_##x(86)  THUNK_##x(87)  THUNK_##x(88)  THUNK_##x(89)  THUNK_##x(90)  \
THUNK_##x(91)  THUNK_##x(92)  THUNK_##x(93)  THUNK_##x(94)  THUNK_##x(95)  THUNK_##x(96)  THUNK_##x(97)  THUNK_##x(98)  THUNK_##x(99)  THUNK_##x(100) THUNK_##x(101) \
THUNK_##x(102) THUNK_##x(103) THUNK_##x(104) THUNK_##x(105) THUNK_##x(106) THUNK_##x(107) THUNK_##x(108) THUNK_##x(109) THUNK_##x(110) THUNK_##x(111) THUNK_##x(112) \
THUNK_##x(113) THUNK_##x(114) THUNK_##x(115) THUNK_##x(116) THUNK_##x(117) THUNK_##x(118) THUNK_##x(119) THUNK_##x(120) THUNK_##x(121) THUNK_##x(122) THUNK_##x(123) \
THUNK_##x(124) THUNK_##x(125) THUNK_##x(126) THUNK_##x(127) THUNK_##x(128) THUNK_##x(129) THUNK_##x(130) THUNK_##x(131) THUNK_##x(132) THUNK_##x(133) THUNK_##x(134) \
THUNK_##x(135) THUNK_##x(136) THUNK_##x(137) THUNK_##x(138) THUNK_##x(139) THUNK_##x(140) THUNK_##x(141) THUNK_##x(142) THUNK_##x(143) THUNK_##x(144) THUNK_##x(145) \
THUNK_##x(146) THUNK_##x(147) THUNK_##x(148) THUNK_##x(149) THUNK_##x(150) THUNK_##x(151) THUNK_##x(152) THUNK_##x(153) THUNK_##x(154) THUNK_##x(155) THUNK_##x(156) \
THUNK_##x(157) THUNK_##x(158) THUNK_##x(159) THUNK_##x(160) THUNK_##x(161) THUNK_##x(162) THUNK_##x(163) THUNK_##x(164) THUNK_##x(165) THUNK_##x(166) THUNK_##x(167) \
THUNK_##x(168) THUNK_##x(169) THUNK_##x(170) THUNK_##x(171) THUNK_##x(172) THUNK_##x(173) THUNK_##x(174) THUNK_##x(175) THUNK_##x(176) THUNK_##x(177) THUNK_##x(178) \
THUNK_##x(179) THUNK_##x(180) THUNK_##x(181) THUNK_##x(182) THUNK_##x(183) THUNK_##x(184) THUNK_##x(185) THUNK_##x(186) THUNK_##x(187) THUNK_##x(188) THUNK_##x(189) \
THUNK_##x(190) THUNK_##x(191) THUNK_##x(192) THUNK_##x(193) THUNK_##x(194) THUNK_##x(195) THUNK_##x(196) THUNK_##x(197) THUNK_##x(198) THUNK_##x(199)

THUNK_IT(IMPL)

static void (*s_apfnVtbl[])() =
{
    PFNVOID(&UnknownTrackQueryInterface ),
    PFNVOID(&UnknownTrackAddRef ),
    PFNVOID(&UnknownTrackRelease ),
    THUNK_IT(ADDR)
};

CUnknownTrack::CUnknownTrack(IUnknown *p, REFIID iid, char *pch, BOOL fTrackOnQI) : 
    CTrack(p, iid, pch, fTrackOnQI), 
    _apfnVTbl(s_apfnVtbl) 
{ 
}

#endif

//+---------------------------------------------------------------
//
//  g_aIIDtoFN
//
//----------------------------------------------------------------

static struct 
{
    const IID *pIID;
    IUnknown * (*pfnCreate)(IUnknown *, char *, BOOL);
}
g_aIIDtoFN[] = 
{
    { &IID_IOleObject, COleObject::Create }, 
    { &IID_IParseDisplayName, COleItemContainer::Create }, 
    { &IID_IOleContainer, COleItemContainer::Create }, 
    { &IID_IOleItemContainer, COleItemContainer::Create }, 
    { &IID_IOleClientSite, COleClientSite::Create }, 
    { &IID_IOleInPlaceSite, COleInPlaceSiteWindowless::Create }, 
    { &IID_IOleInPlaceSiteWindowless, COleInPlaceSiteWindowless::Create }, 
    { &IID_IDataObject, CDataObject::Create }, 
    { &IID_IViewObject, CViewObject::Create }, 
    { &IID_IPersist, CPersistStorage::Create }, 
    { &IID_IPersistStorage, CPersistStorage::Create }, 
    { &IID_IPersistFile, CPersistFile::Create }, 
    { &IID_IPersistStream, CPersistStream::Create }, 
    { &IID_IPersistStreamInit, CPersistStreamInit::Create }, 
    { &IID_IPersistPropertyBag, CPersistPropertyBag::Create }, 
    { &IID_IOleInPlaceObject, COleInPlaceObject::Create }, 
    { &IID_IOleControl, COleControl::Create }, 
	{ &IID_IOleInPlaceActiveObject, COleInPlaceActiveObject::Create }, 
	{ &IID_IOleInPlaceFrame, COleInPlaceFrame::Create }, 
	{ &IID_IOleInPlaceUIWindow, COleInPlaceUIWindow::Create }, 
	{ &IID_IOleCommandTarget, COleCommandTarget::Create }, 
	{ &IID_IHlinkTarget, CHlinkTarget::Create }, 
};

//+---------------------------------------------------------------
//
//  Function:   DbgExTrackItf
//
//  Synopsis:   Wraps an interface pointer for the purpose of
//              tracing the method calls on that pointer
//
//  Arguments:  iid The interface of the pointer
//              pch A string prefix to use in the trace output
//              ppv  The pointer to the interface. Updated on return.
//
//----------------------------------------------------------------

void WINAPI
DbgExTrackItf(REFIID iid, char * pch, BOOL fTrackOnQI, void **ppv)
{
    
    if (     
           *ppv == NULL
        || !(DbgExIsTagEnabled(tagTrackItf) || DbgExIsTagEnabled(tagTrackItfVerbose)) 
        || iid == IID_IUnknown 
#if defined(_M_IX86)
        || (void *)s_apfnVtbl == **(void ***)ppv
#endif
        )
    {
        return;
    }

    for (int i = ARRAY_SIZE(g_aIIDtoFN); --i >= 0; )
    {
        if (iid == *g_aIIDtoFN[i].pIID)
            break;
    }

    if (i < 0)
    {
#if defined(_M_IX86)
        CUnknownTrack * pUnknownTrack = new CUnknownTrack((IUnknown *)*ppv, iid, pch, fTrackOnQI);
        if (pUnknownTrack)
            *ppv = &pUnknownTrack->_apfnVTbl;
#endif
    }
    else
    {
        void *pv = g_aIIDtoFN[i].pfnCreate((IUnknown *)*ppv, pch, fTrackOnQI);
        if (pv)
            *ppv = pv;
    }
}
