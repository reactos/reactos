#ifndef _OCIDL_H
#define _OCIDL_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <ole2.h>

typedef interface IErrorLog *LPERRORLOG;
typedef interface IPropertyBag *LPPROPERTYBAG;
typedef interface IPropertyBag2 *LPPROPERTYBAG2;
typedef interface IEnumConnections *LPENUMCONNECTIONS;
typedef interface IConnectionPoint *LPCONNECTIONPOINT;
typedef interface IEnumConnectionPoints *LPENUMCONNECTIONPOINTS;
typedef interface IPropertyPageSite *LPPROPERTYPAGESITE;
typedef interface IFont *LPFONT;
typedef interface IFontDisp *LPFONTDISP;
typedef interface IOleUndoManager *LPOLEUNDOMANAGER;

#ifndef OLE2ANSI
typedef TEXTMETRICW TEXTMETRICOLE;
#else
typedef TEXTMETRIC TEXTMETRICOLE;
#endif
typedef TEXTMETRICOLE *LPTEXTMETRICOLE;
typedef DWORD OLE_COLOR;
typedef UINT OLE_HANDLE;
typedef long OLE_XPOS_HIMETRIC;
typedef long OLE_YPOS_HIMETRIC;
typedef long OLE_XSIZE_HIMETRIC;
typedef long OLE_YSIZE_HIMETRIC;

typedef enum tagREADYSTATE {
	READYSTATE_UNINITIALIZED = 0,
	READYSTATE_LOADING = 1,
	READYSTATE_LOADED = 2,
	READYSTATE_INTERACTIVE = 3,
	READYSTATE_COMPLETE = 4
} READYSTATE;
typedef enum tagPROPBAG2_TYPE {
	PROPBAG2_TYPE_UNDEFINED	= 0,
	PROPBAG2_TYPE_DATA = 1,
	PROPBAG2_TYPE_URL = 2,
	PROPBAG2_TYPE_OBJECT = 3,
	PROPBAG2_TYPE_STREAM = 4,
	PROPBAG2_TYPE_STORAGE = 5,
	PROPBAG2_TYPE_MONIKER = 6
} PROPBAG2_TYPE;
typedef struct tagPROPBAG2
{
	DWORD dwType;
	VARTYPE vt;
	CLIPFORMAT cfType;
	DWORD dwHint;
	LPOLESTR pstrName;
	CLSID clsid;
} PROPBAG2;
enum tagQACONTAINERFLAGS
{
	QACONTAINER_SHOWHATCHING = 1,
	QACONTAINER_SHOWGRABHANDLES = 2,
	QACONTAINER_USERMODE = 4,
	QACONTAINER_DISPLAYASDEFAULT = 8,
	QACONTAINER_UIDEAD = 16,
	QACONTAINER_AUTOCLIP = 32,
	QACONTAINER_MESSAGEREFLECT = 64,
	QACONTAINER_SUPPORTSMNEMONICS = 128
} QACONTAINERFLAGS;
typedef struct tagQACONTAINER
{
	ULONG cbSize;
	interface IOleClientSite *pClientSite;
	interface IAdviseSinkEx *pAdviseSink;
	interface IPropertyNotifySink *pPropertyNotifySink;
	IUnknown *pUnkEventSink;
	DWORD dwAmbientFlags;
	OLE_COLOR colorFore;
	OLE_COLOR colorBack;
	interface IFont *pFont;
	interface IOleUndoManager *pUndoMgr;
	DWORD dwAppearance;
	LONG lcid;
	HPALETTE hpal;
	interface IBindHost *pBindHost;
	interface IOleControlSite *pOleControlSite;
	interface IServiceProvider *pServiceProvider;
} QACONTAINER;
typedef struct tagQACONTROL
{
	ULONG cbSize;
	DWORD dwMiscStatus;
	DWORD dwViewStatus;
	DWORD dwEventCookie;
	DWORD dwPropNotifyCookie;
	DWORD dwPointerActivationPolicy;
} QACONTROL;
typedef struct tagPOINTF {
	float x;
	float y;
} POINTF,*LPPOINTF;
typedef struct tagCONTROLINFO {
	ULONG cb;
	HACCEL hAccel;
	USHORT cAccel;
	DWORD dwFlags;
} CONTROLINFO,*LPCONTROLINFO;
typedef struct tagCONNECTDATA {
	LPUNKNOWN pUnk;
	DWORD dwCookie;
} CONNECTDATA,*LPCONNECTDATA;
typedef struct tagLICINFO {
	long cbLicInfo;
	BOOL fRuntimeKeyAvail;
	BOOL fLicVerified;
} LICINFO,*LPLICINFO;
typedef struct tagCAUUID {
	ULONG cElems;
	GUID *pElems;
} CAUUID,*LPCAUUID;
typedef struct tagCALPOLESTR {
	ULONG cElems;
	LPOLESTR *pElems;
} CALPOLESTR,*LPCALPOLESTR;
typedef struct tagCADWORD {
	ULONG cElems;
	DWORD *pElems;
} CADWORD,*LPCADWORD;
typedef struct tagPROPPAGEINFO {
	ULONG cb;
	LPOLESTR pszTitle;
	SIZE size;
	LPOLESTR pszDocString;
	LPOLESTR pszHelpFile;
	DWORD dwHelpContext;
} PROPPAGEINFO,*LPPROPPAGEINFO;

EXTERN_C const IID IID_IOleControl;
#define INTERFACE IOleControl
DECLARE_INTERFACE_(IOleControl,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetControlInfo)(THIS_ LPCONTROLINFO) PURE;
	STDMETHOD(OnMnemonic)(THIS_ LPMSG) PURE;
	STDMETHOD(OnAmbientPropertyChange)(THIS_ DISPID) PURE;
	STDMETHOD(FreezeEvents)(THIS_ BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleControlSite;
#define INTERFACE IOleControlSite
DECLARE_INTERFACE_(IOleControlSite,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(OnControlInfoChanged)(THIS) PURE;
	STDMETHOD(LockInPlaceActive)(THIS_ BOOL) PURE;
	STDMETHOD(GetExtendedControl)(THIS_ LPDISPATCH*) PURE;
	STDMETHOD(TransformCoords)(THIS_ POINTL*,POINTF*,DWORD) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG,DWORD) PURE;
	STDMETHOD(OnFocus)(THIS_ BOOL) PURE;
	STDMETHOD(ShowPropertyFrame)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IOleControlSite_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleControlSite_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleControlSite_Release(T) (T)->lpVtbl->Release(T)
#define IOleControlSite_OnControlInfoChanged(T) (T)->lpVtbl->OnControlInfoChanged(T)
#define IOleControlSite_LockInPlaceActive(T,a) (T)->lpVtbl->LockInPlaceActive(T,a)
#define IOleControlSite_GetExtendedControl(T,a) (T)->lpVtbl->GetExtendedControl(T,a)
#define IOleControlSite_TransformCoords(T,a,b,c) (T)->lpVtbl->TransformCoords(T,a,b,c)
#define IOleControlSite_TranslateAccelerator(T,a,b) (T)->lpVtbl->TranslateAccelerator(T,a,b)
#define IOleControlSite_OnFocus(T,a) (T)->lpVtbl->OnFocus(T,a)
#define IOleControlSite_ShowPropertyFrame(T) (T)->lpVtbl->ShowPropertyFrame(T)
#endif

EXTERN_C const IID IID_ISimpleFrameSite;
#define INTERFACE ISimpleFrameSite
DECLARE_INTERFACE_(ISimpleFrameSite,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(PreMessageFilter)(THIS_ HWND,UINT,WPARAM,LPARAM,LRESULT*,PDWORD) PURE;
	STDMETHOD(PostMessageFilter)(THIS_ HWND,UINT,WPARAM,LPARAM,LRESULT*,DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IErrorLog;
#define INTERFACE IErrorLog
DECLARE_INTERFACE_(IErrorLog,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(AddError)(THIS_ LPCOLESTR,LPEXCEPINFO) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyBag;
#define INTERFACE IPropertyBag
DECLARE_INTERFACE_(IPropertyBag,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Read)(THIS_ LPCOLESTR,LPVARIANT,LPERRORLOG) PURE;
	STDMETHOD(Write)(THIS_ LPCOLESTR,LPVARIANT) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyBag2;
#define INTERFACE IPropertyBag2
DECLARE_INTERFACE_(IPropertyBag2,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Read)(THIS_ ULONG,PROPBAG2*,LPERRORLOG,VARIANT*,HRESULT*) PURE;
	STDMETHOD(Write)(THIS_ ULONG,PROPBAG2*,VARIANT*) PURE;
	STDMETHOD(CountProperties)(THIS_ ULONG*) PURE;
	STDMETHOD(GetPropertyInfo)(THIS_ ULONG,ULONG,PROPBAG2*,ULONG*) PURE;
	STDMETHOD(LoadObject)(THIS_ LPCOLESTR,DWORD,IUnknown*,LPERRORLOG) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersistPropertyBag;
#define INTERFACE IPersistPropertyBag
DECLARE_INTERFACE_(IPersistPropertyBag,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(InitNew)(THIS) PURE;
	STDMETHOD(Load)(THIS_ LPPROPERTYBAG,LPERRORLOG) PURE;
	STDMETHOD(Save)(THIS_ LPPROPERTYBAG,BOOL,BOOL) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IPersistPropertyBag_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IPersistPropertyBag_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IPersistPropertyBag_Release(T) (T)->lpVtbl->Release(T)
#define IPersistPropertyBag_GetClassID(T,a) (T)->lpVtbl->GetClassID(T,a)
#define IPersistPropertyBag_InitNew(T) (T)->lpVtbl->InitNew(T)
#define IPersistPropertyBag_Load(T,a,b) (T)->lpVtbl->Load(T,a,b)
#define IPersistPropertyBag_Save(T,a,b,c) (T)->lpVtbl->Save(T,a,b,c)
#endif

EXTERN_C const IID IID_IPersistPropertyBag2;
#define INTERFACE IPersistPropertyBag2
DECLARE_INTERFACE_(IPersistPropertyBag2,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(InitNew)(THIS) PURE;
	STDMETHOD(Load)(THIS_ LPPROPERTYBAG2,LPERRORLOG) PURE;
	STDMETHOD(Save)(THIS_ LPPROPERTYBAG2,BOOL,BOOL) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IPersistPropertyBag2_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IPersistPropertyBag2_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IPersistPropertyBag2_Release(T) (T)->lpVtbl->Release(T)
#define IPersistPropertyBag2_GetClassID(T,a) (T)->lpVtbl->GetClassID(T,a)
#define IPersistPropertyBag2_InitNew(T) (T)->lpVtbl->InitNew(T)
#define IPersistPropertyBag2_Load(T,a,b) (T)->lpVtbl->Load(T,a,b)
#define IPersistPropertyBag2_Save(T,a,b,c) (T)->lpVtbl->Save(T,a,b,c)
#define IPersistPropertyBag2_IsDirty(T) (T)->lpVtbl->IsDirty(T)
#endif

EXTERN_C const IID IID_IPersistStreamInit;
#define INTERFACE IPersistStreamInit
DECLARE_INTERFACE_(IPersistStreamInit,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ LPSTREAM) PURE;
	STDMETHOD(Save)(THIS_ LPSTREAM,BOOL) PURE;
	STDMETHOD(GetSizeMax)(THIS_ PULARGE_INTEGER) PURE;
	STDMETHOD(InitNew)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersistMemory;
#define INTERFACE IPersistMemory
DECLARE_INTERFACE_(IPersistMemory,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ PVOID,ULONG) PURE;
	STDMETHOD(Save)(THIS_ PVOID,BOOL,ULONG) PURE;
	STDMETHOD(GetSizeMax)(THIS_ PULONG) PURE;
	STDMETHOD(InitNew)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyNotifySink;
#define INTERFACE IPropertyNotifySink
DECLARE_INTERFACE_(IPropertyNotifySink,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(OnChanged)(THIS_ DISPID) PURE;
	STDMETHOD(OnRequestEdit)(THIS_ DISPID) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IPropertyNotifySink_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IPropertyNotifySink_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IPropertyNotifySink_Release(T) (T)->lpVtbl->Release(T)
#define IPropertyNotifySink_OnChanged(T,a) (T)->lpVtbl->OnChanged(T,a)
#define IPropertyNotifySink_OnRequestEdit(T,a) (T)->lpVtbl->OnRequestEdit(T,a)
#endif

EXTERN_C const IID IID_IProvideClassInfo;
#define INTERFACE IProvideClassInfo
DECLARE_INTERFACE_(IProvideClassInfo,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassInfo)(THIS_ LPTYPEINFO*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IProvideClassInfo2;
#define INTERFACE IProvideClassInfo2
DECLARE_INTERFACE_(IProvideClassInfo2,IProvideClassInfo)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassInfo)(THIS_ LPTYPEINFO*) PURE;
	STDMETHOD(GetGUID)(THIS_ DWORD,GUID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IConnectionPointContainer;
#define INTERFACE IConnectionPointContainer
DECLARE_INTERFACE_(IConnectionPointContainer,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(EnumConnectionPoints)(THIS_ LPENUMCONNECTIONPOINTS*) PURE;
	STDMETHOD(FindConnectionPoint)(THIS_ REFIID,LPCONNECTIONPOINT*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IConnectionPointContainer_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IConnectionPointContainer_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IConnectionPointContainer_Release(T) (T)->lpVtbl->Release(T)
#define IConnectionPointContainer_EnumConnectionPoints(T,a) (T)->lpVtbl->EnumConnectionPoints(T,a)
#define IConnectionPointContainer_FindConnectionPoint(T,a,b) (T)->lpVtbl->FindConnectionPoint(T,a,b)
#endif

EXTERN_C const IID IID_IEnumConnectionPoints;
#define INTERFACE IEnumConnectionPoints
DECLARE_INTERFACE_(IEnumConnectionPoints,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Next)(THIS_ ULONG,LPCONNECTIONPOINT*,ULONG*) PURE;
	STDMETHOD(Skip)(THIS_ ULONG) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMCONNECTIONPOINTS*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IConnectionPoint;
#define INTERFACE IConnectionPoint
DECLARE_INTERFACE_(IConnectionPoint,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetConnectionInterface)(THIS_ IID*) PURE;
	STDMETHOD(GetConnectionPointContainer)(THIS_ IConnectionPointContainer**) PURE;
	STDMETHOD(Advise)(THIS_ LPUNKNOWN,PDWORD) PURE;
	STDMETHOD(Unadvise)(THIS_ DWORD) PURE;
	STDMETHOD(EnumConnections)(THIS_ LPENUMCONNECTIONS*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IConnectionPoint_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IConnectionPoint_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IConnectionPoint_Release(T) (T)->lpVtbl->Release(T)
#define IConnectionPoint_GetConnectionInterface(T,a) (T)->lpVtbl->GetConnectionInterface(T,a)
#define IConnectionPoint_GetConnectionPointContainer(T,a) (T)->lpVtbl->GetConnectionPointContainer(T,a)
#define IConnectionPoint_Advise(T,a,b) (T)->lpVtbl->Advise(T,a,b)
#define IConnectionPoint_Unadvise(T,a) (T)->lpVtbl->Unadvise(T,a)
#define IConnectionPoint_EnumConnections(T,a) (T)->lpVtbl->EnumConnections(T,a)
#endif

EXTERN_C const IID IID_IEnumConnections;
#define INTERFACE IEnumConnections
DECLARE_INTERFACE_(IEnumConnections,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Next)(THIS_ ULONG,LPCONNECTDATA,PULONG) PURE;
	STDMETHOD(Skip)(THIS_ ULONG) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMCONNECTIONS*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IEnumConnections_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IEnumConnections_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IEnumConnections_Release(T) (T)->lpVtbl->Release(T)
#define IEnumConnections_Next(T,a,b,c) (T)->lpVtbl->Next(T,a,b,c)
#define IEnumConnections_Skip(T,a) (T)->lpVtbl->Skip(T,a)
#define IEnumConnections_Reset(T) (T)->lpVtbl->Reset(T)
#define IEnumConnections_Clone(T,a) (T)->lpVtbl->Clone(T,a)
#endif

EXTERN_C const IID IID_IClassFactory2;
#define INTERFACE IClassFactory2
DECLARE_INTERFACE_(IClassFactory2,IClassFactory)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(CreateInstance)(THIS_ LPUNKNOWN,REFIID,PVOID*) PURE;
	STDMETHOD(LockServer)(THIS_ BOOL) PURE;
	STDMETHOD(GetLicInfo)(THIS_ LPLICINFO) PURE;
	STDMETHOD(RequestLicKey)(THIS_ DWORD,BSTR*) PURE;
	STDMETHOD(CreateInstanceLic)(THIS_ LPUNKNOWN,LPUNKNOWN,REFIID,BSTR,PVOID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_ISpecifyPropertyPages;
#define INTERFACE ISpecifyPropertyPages
DECLARE_INTERFACE_(ISpecifyPropertyPages,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetPages)(THIS_ CAUUID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPerPropertyBrowsing;
#define INTERFACE IPerPropertyBrowsing
DECLARE_INTERFACE_(IPerPropertyBrowsing,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDisplayString)(THIS_ DISPID,BSTR*) PURE;
	STDMETHOD(MapPropertyToPage)(THIS_ DISPID,LPCLSID) PURE;
	STDMETHOD(GetPredefinedStrings)(THIS_ DISPID,CALPOLESTR*,CADWORD*) PURE;
	STDMETHOD(GetPredefinedValue)(THIS_ DISPID,DWORD,VARIANT*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyPageSite;
#define INTERFACE IPropertyPageSite
DECLARE_INTERFACE_(IPropertyPageSite,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(OnStatusChange)(THIS_ DWORD) PURE;
	STDMETHOD(GetLocaleID)(THIS_ LCID*) PURE;
	STDMETHOD(GetPageContainer)(THIS_ LPUNKNOWN*) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyPage;
#define INTERFACE IPropertyPage
DECLARE_INTERFACE_(IPropertyPage,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SetPageSite)(THIS_ LPPROPERTYPAGESITE) PURE;
	STDMETHOD(Activate)(THIS_ HWND,LPCRECT,BOOL) PURE;
	STDMETHOD(Deactivate)(THIS) PURE;
	STDMETHOD(GetPageInfo)(THIS_ LPPROPPAGEINFO) PURE;
	STDMETHOD(SetObjects)(THIS_ ULONG,LPUNKNOWN*) PURE;
	STDMETHOD(Show)(THIS_ UINT) PURE;
	STDMETHOD(Move)(THIS_ LPCRECT) PURE;
	STDMETHOD(IsPageDirty)(THIS) PURE;
	STDMETHOD(Apply)(THIS) PURE;
	STDMETHOD(Help)(THIS_ LPCOLESTR) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyPage2;
#define INTERFACE IPropertyPage2
DECLARE_INTERFACE_(IPropertyPage2,IPropertyPage)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SetPageSite)(THIS_ LPPROPERTYPAGESITE) PURE;
	STDMETHOD(Activate)(THIS_ HWND,LPCRECT,BOOL) PURE;
	STDMETHOD(Deactivate)(THIS) PURE;
	STDMETHOD(GetPageInfo)(THIS_ LPPROPPAGEINFO) PURE;
	STDMETHOD(SetObjects)(THIS_ ULONG,LPUNKNOWN*) PURE;
	STDMETHOD(Show)(THIS_ UINT) PURE;
	STDMETHOD(Move)(THIS_ LPCRECT) PURE;
	STDMETHOD(IsPageDirty)(THIS) PURE;
	STDMETHOD(Apply)(THIS) PURE;
	STDMETHOD(Help)(THIS_ LPCOLESTR) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG) PURE;
	STDMETHOD(EditProperty)(THIS_ DISPID) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IFont;
#define INTERFACE IFont
DECLARE_INTERFACE_(IFont,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(get_Name)(THIS_ BSTR*) PURE;
	STDMETHOD(put_Name)(THIS_ BSTR) PURE;
	STDMETHOD(get_Size)(THIS_ CY*) PURE;
	STDMETHOD(put_Size)(THIS_ CY) PURE;
	STDMETHOD(get_Bold)(THIS_ BOOL*) PURE;
	STDMETHOD(put_Bold)(THIS_ BOOL) PURE;
	STDMETHOD(get_Italic)(THIS_ BOOL*) PURE;
	STDMETHOD(put_Italic)(THIS_ BOOL) PURE;
	STDMETHOD(get_Underline)(THIS_ BOOL*) PURE;
	STDMETHOD(put_Underline)(THIS_ BOOL) PURE;
	STDMETHOD(get_Strikethrough)(THIS_ BOOL*) PURE;
	STDMETHOD(put_Strikethrough)(THIS_ BOOL) PURE;
	STDMETHOD(get_Weight)(THIS_ short*) PURE;
	STDMETHOD(put_Weight)(THIS_ short) PURE;
	STDMETHOD(get_Charset)(THIS_ short*) PURE;
	STDMETHOD(put_Charset)(THIS_ short) PURE;
	STDMETHOD(get_hFont)(THIS_ HFONT*) PURE;
	STDMETHOD(Clone)(THIS_ IFont**) PURE;
	STDMETHOD(IsEqual)(THIS_ IFont*) PURE;
	STDMETHOD(SetRatio)(THIS_ long,long) PURE;
	STDMETHOD(QueryTextMetrics)(THIS_ LPTEXTMETRICOLE) PURE;
	STDMETHOD(AddRefHfont)(THIS_ HFONT) PURE;
	STDMETHOD(ReleaseHfont)(THIS_ HFONT) PURE;
	STDMETHOD(SetHdc)(THIS_ HDC) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IFontDisp;
#define INTERFACE IFontDisp
DECLARE_INTERFACE_(IFontDisp,IDispatch)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetTypeInfoCount)(THIS_ UINT*) PURE;
	STDMETHOD(GetTypeInfo)(THIS_ UINT,LCID,LPTYPEINFO*) PURE;
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID,LPOLESTR*,UINT,LCID,DISPID*) PURE;
	STDMETHOD(Invoke)(THIS_ DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPicture;
#define INTERFACE IPicture
DECLARE_INTERFACE_(IPicture,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(get_Handle)(THIS_ OLE_HANDLE*) PURE;
	STDMETHOD(get_hPal)(THIS_ OLE_HANDLE*) PURE;
	STDMETHOD(get_Type)(THIS_ short*) PURE;
	STDMETHOD(get_Width)(THIS_ OLE_XSIZE_HIMETRIC*) PURE;
	STDMETHOD(get_Height)(THIS_ OLE_YSIZE_HIMETRIC*) PURE;
	STDMETHOD(Render)(THIS_ HDC,long,long,long,long,OLE_XPOS_HIMETRIC,OLE_YPOS_HIMETRIC,OLE_XSIZE_HIMETRIC,OLE_YSIZE_HIMETRIC,LPCRECT) PURE;
	STDMETHOD(set_hPal)(THIS_ OLE_HANDLE) PURE;
	STDMETHOD(get_CurDC)(THIS_ HDC*) PURE;
	STDMETHOD(SelectPicture)(THIS_ HDC,HDC*,OLE_HANDLE*) PURE;
	STDMETHOD(get_KeepOriginalFormat)(THIS_ BOOL*) PURE;
	STDMETHOD(put_KeepOriginalFormat)(THIS_ BOOL) PURE;
	STDMETHOD(PictureChanged)(THIS) PURE;
	STDMETHOD(SaveAsFile)(THIS_ LPSTREAM,BOOL,LONG*) PURE;
	STDMETHOD(get_Attributes)(THIS_ PDWORD) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IPicture_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IPicture_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IPicture_Release(p) (p)->lpVtbl->Release(p)
#define IPicture_get_Handle(p,a) (p)->lpVtbl->get_Handle(p,a)
#define IPicture_get_hPal(p,a) (p)->lpVtbl->get_hPal(p,a)
#define IPicture_get_Type(p,a) (p)->lpVtbl->get_Type(p,a)
#define IPicture_get_Width(p,a) (p)->lpVtbl->get_Width(p,a)
#define IPicture_get_Height(p,a) (p)->lpVtbl->get_Height(p,a)
#define IPicture_Render(p,a,b,c,d,e,f,g,h,i,j) (p)->lpVtbl->Render(p,a,b,c,d,e,f,g,h,i,j)
#define IPicture_set_hPal(p,a) (p)->lpVtbl->set_hPal(p,a)
#define IPicture_get_CurDC(p,a) (p)->lpVtbl->get_CurDC(p,a)
#define IPicture_SelectPicture(p,a,b,c) (p)->lpVtbl->SelectPicture(p,a,b,c)
#define IPicture_get_KeepOriginalFormat(p,a) (p)->lpVtbl->get_KeepOriginalFormat(p,a)
#define IPicture_put_KeepOriginalFormat(p,a) (p)->lpVtbl->put_KeepOriginalFormat(p,a)
#define IPicture_PictureChanged(p) (p)->lpVtbl->PictureChanged(p)
#define IPicture_SaveAsFile(p,a,b,c) (p)->lpVtbl->SaveAsFile(p,a,b,c)
#define IPicture_get_Attributes(p,a) (p)->lpVtbl->get_Attributes(p,a)
#endif

EXTERN_C const IID IID_IPictureDisp;
#define INTERFACE IPictureDisp
DECLARE_INTERFACE_(IPictureDisp,IDispatch)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetTypeInfoCount)(THIS_ UINT*) PURE;
	STDMETHOD(GetTypeInfo)(THIS_ UINT,LCID,LPTYPEINFO*) PURE;
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID,LPOLESTR*,UINT,LCID,DISPID*) PURE;
	STDMETHOD(Invoke)(THIS_ DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleInPlaceSiteEx;
#define INTERFACE IOleInPlaceSiteEx
DECLARE_INTERFACE_(IOleInPlaceSiteEx,IOleInPlaceSite)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(CanInPlaceActivate)(THIS) PURE;
	STDMETHOD(OnInPlaceActivate)(THIS) PURE;
	STDMETHOD(OnUIActivate)(THIS) PURE;
	STDMETHOD(GetWindowContext)(THIS_ IOleInPlaceFrame**,IOleInPlaceUIWindow**,LPRECT,LPRECT,LPOLEINPLACEFRAMEINFO) PURE;
	STDMETHOD(Scroll)(THIS_ SIZE) PURE;
	STDMETHOD(OnUIDeactivate)(THIS_ BOOL) PURE;
	STDMETHOD(OnInPlaceDeactivate)(THIS) PURE;
	STDMETHOD(DiscardUndoState)(THIS) PURE;
	STDMETHOD(DeactivateAndUndo)(THIS) PURE;
	STDMETHOD(OnPosRectChange)(THIS_ LPCRECT) PURE;
	STDMETHOD(OnInPlaceActivateEx)(THIS_ BOOL*,DWORD) PURE;
	STDMETHOD(OnInPlaceDeactivateEx)(THIS_ BOOL) PURE;
	STDMETHOD(RequestUIActivate)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IObjectWithSite;
#define INTERFACE IObjectWithSite
DECLARE_INTERFACE_(IObjectWithSite,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SetSite)(THIS_ IUnknown*) PURE;
	STDMETHOD(GetSite)(THIS_ REFIID, void**) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleInPlaceSiteWindowless;
#define INTERFACE IOleInPlaceSiteWindowless
DECLARE_INTERFACE_(IOleInPlaceSiteWindowless,IOleInPlaceSiteEx)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(CanInPlaceActivate)(THIS) PURE;
	STDMETHOD(OnInPlaceActivate)(THIS) PURE;
	STDMETHOD(OnUIActivate)(THIS) PURE;
	STDMETHOD(GetWindowContext)(THIS_ IOleInPlaceFrame**,IOleInPlaceUIWindow**,LPRECT,LPRECT,LPOLEINPLACEFRAMEINFO) PURE;
	STDMETHOD(Scroll)(THIS_ SIZE) PURE;
	STDMETHOD(OnUIDeactivate)(THIS_ BOOL) PURE;
	STDMETHOD(OnInPlaceDeactivate)(THIS) PURE;
	STDMETHOD(DiscardUndoState)(THIS) PURE;
	STDMETHOD(DeactivateAndUndo)(THIS) PURE;
	STDMETHOD(OnPosRectChange)(THIS_ LPCRECT) PURE;

	STDMETHOD(OnInPlaceActivateEx)(THIS_ BOOL*,DWORD) PURE;
	STDMETHOD(OnInPlaceDeactivateEx)(THIS_ BOOL) PURE;
	STDMETHOD(RequestUIActivate)(THIS) PURE;

	STDMETHOD(CanWindowlessActivate)(THIS) PURE;
	STDMETHOD(GetCapture)(THIS) PURE;
	STDMETHOD(SetCapture)(THIS_ BOOL) PURE;
	STDMETHOD(GetFocus)(THIS) PURE;
	STDMETHOD(SetFocus)(THIS_ BOOL) PURE;
	STDMETHOD(GetDC)(THIS_ LPCRECT,DWORD,HDC*) PURE;
	STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
	STDMETHOD(InvalidateRect)(THIS_ LPCRECT,BOOL) PURE;
	STDMETHOD(InvalidateRgn)(THIS_ HRGN,BOOL) PURE;
	STDMETHOD(ScrollRect)(THIS_ INT,INT,LPCRECT,LPCRECT) PURE;
	STDMETHOD(AdjustRect)(THIS_ LPCRECT) PURE;
	STDMETHOD(OnDefWindowMessage)(THIS_ UINT,WPARAM,LPARAM,LRESULT*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IAdviseSinkEx;
#define INTERFACE IAdviseSinkEx
DECLARE_INTERFACE_(IAdviseSinkEx,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC*,STGMEDIUM*) PURE;
	STDMETHOD_(void,OnViewChange)(THIS_ DWORD,LONG) PURE;
	STDMETHOD_(void,OnRename)(THIS_ IMoniker*) PURE;
	STDMETHOD_(void,OnSave)(THIS) PURE;
	STDMETHOD_(void,OnClose)(THIS) PURE;
	STDMETHOD(OnViewStatusChange)(THIS_ DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPointerInactive;
#define INTERFACE IPointerInactive
DECLARE_INTERFACE_(IPointerInactive,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetActivationPolicy)(THIS_ DWORD*) PURE;
	STDMETHOD(OnInactiveMouseMove)(THIS_ LPCRECT,LONG,LONG,DWORD) PURE;
	STDMETHOD(OnInactiveSetCursor)(THIS_ LPCRECT,LONG,LONG,DWORD,BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleUndoUnit;
#define INTERFACE IOleUndoUnit
DECLARE_INTERFACE_(IOleUndoUnit,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Do)(THIS_ LPOLEUNDOMANAGER) PURE;
	STDMETHOD(GetDescription)(THIS_ BSTR*) PURE;
	STDMETHOD(GetUnitType)(THIS_ CLSID*,LONG*) PURE;
	STDMETHOD(OnNextAdd)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleParentUndoUnit;
#define INTERFACE IOleParentUndoUnit
DECLARE_INTERFACE_(IOleParentUndoUnit,IOleUndoUnit)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Do)(THIS_ LPOLEUNDOMANAGER) PURE;
	STDMETHOD(GetDescription)(THIS_ BSTR*) PURE;
	STDMETHOD(GetUnitType)(THIS_ CLSID*,LONG*) PURE;
	STDMETHOD(OnNextAdd)(THIS) PURE;
	STDMETHOD(Open)(THIS_ IOleParentUndoUnit*) PURE;
	STDMETHOD(Close)(THIS_ IOleParentUndoUnit*,BOOL) PURE;
	STDMETHOD(Add)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(FindUnit)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(GetParentState)(THIS_ DWORD*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IEnumOleUndoUnits;
#define INTERFACE IEnumOleUndoUnits
DECLARE_INTERFACE_(IEnumOleUndoUnits,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Next)(THIS_ ULONG,IOleUndoUnit**,ULONG*) PURE;        
	STDMETHOD(Skip)(THIS_ ULONG) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ IEnumOleUndoUnits**) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IOleUndoManager;
#define INTERFACE IOleUndoManager
DECLARE_INTERFACE_(IOleUndoManager,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Open)(THIS_ IOleParentUndoUnit*) PURE;
	STDMETHOD(Close)(THIS_ IOleParentUndoUnit*,BOOL) PURE;
	STDMETHOD(Add)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(GetOpenParentState)(THIS_ DWORD*) PURE;
	STDMETHOD(DiscardFrom)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(UndoTo)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(RedoTo)(THIS_ IOleUndoUnit*) PURE;
	STDMETHOD(EnumUndoable)(THIS_ IEnumOleUndoUnits**) PURE;
	STDMETHOD(EnumRedoable)(THIS_ IEnumOleUndoUnits**) PURE;
	STDMETHOD(GetLastUndoDescription)(THIS_ BSTR*) PURE;
	STDMETHOD(GetLastRedoDescription)(THIS_ BSTR*) PURE;
	STDMETHOD(Enable)(THIS_ BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IQuickActivate;
#define INTERFACE IQuickActivate
DECLARE_INTERFACE_(IQuickActivate,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(QuickActivate)(THIS_ QACONTAINER*,QACONTROL*) PURE;
	STDMETHOD(SetContentExtent)(THIS_ LPSIZEL) PURE;
	STDMETHOD(GetContentExtent)(THIS_ LPSIZEL) PURE;
};
#undef INTERFACE

#ifdef __cplusplus
}
#endif
#endif
