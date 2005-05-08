#ifndef _OLEIDL_H
#define _OLEIDL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <objfwd.h>

#define MK_ALT	32

typedef interface IParseDisplayName *LPPARSEDISPLAYNAME;
typedef interface IOleContainer *LPOLECONTAINER;
typedef interface IOleClientSite *LPOLECLIENTSITE;
typedef interface IOleObject *LPOLEOBJECT;
typedef interface IDropTarget *LPDROPTARGET;
typedef interface IDropSource *LPDROPSOURCE;
typedef interface IEnumOleUndoUnits *LPENUMOLEUNDOUNITS;
typedef interface IEnumOLEVERB *LPENUMOLEVERB;
typedef interface IOleWindow *LPOLEWINDOW;
typedef interface IOleInPlaceUIWindow *LPOLEINPLACEUIWINDOW;
typedef interface IOleInPlaceActiveObject *LPOLEINPLACEACTIVEOBJECT;
typedef interface IOleInPlaceFrame *LPOLEINPLACEFRAME;
typedef interface IOleAdviseHolder *LPOLEADVISEHOLDER;
typedef interface IViewObject *LPVIEWOBJECT;
typedef interface IViewObject2 *LPVIEWOBJECT2;
typedef interface IOleCache *LPOLECACHE;
typedef interface IOleCache2 *LPOLECACHE2;
typedef interface IOleCacheControl *LPOLECACHECONTROL;

typedef enum tagBINDSPEED
{
	BINDSPEED_INDEFINITE=1,
	BINDSPEED_MODERATE,
	BINDSPEED_IMMEDIATE
} BINDSPEED;
typedef enum tagOLEWHICHMK {
	OLEWHICHMK_CONTAINER=1,
	OLEWHICHMK_OBJREL,
	OLEWHICHMK_OBJFULL
} OLEWHICHMK;
typedef enum tagOLEGETMONIKER {
	OLEGETMONIKER_ONLYIFTHERE=1,
	OLEGETMONIKER_FORCEASSIGN,
	OLEGETMONIKER_UNASSIGN,
	OLEGETMONIKER_TEMPFORUSER
} OLEGETMONIKER;
typedef enum tagUSERCLASSTYPE {
	USERCLASSTYPE_FULL=1,
	USERCLASSTYPE_SHORT,
	USERCLASSTYPE_APPNAME
} USERCLASSTYPE;

#ifdef __GNUC__
__extension__  /* IS0 C says enums limited to range of int */ 
#endif
typedef enum tagDROPEFFECT {
	DROPEFFECT_NONE=0,
	DROPEFFECT_COPY=1,
	DROPEFFECT_MOVE=2,
	DROPEFFECT_LINK=4,
	DROPEFFECT_SCROLL=0x80000000
} DROPEFFECT;
typedef struct tagOleMenuGroupWidths {
	LONG width[6];
} OLEMENUGROUPWIDTHS,*LPOLEMENUGROUPWIDTHS;
typedef HGLOBAL HOLEMENU;
typedef enum tagOLECLOSE {
	OLECLOSE_SAVEIFDIRTY,
	OLECLOSE_NOSAVE,
	OLECLOSE_PROMPTSAVE
} OLECLOSE;
typedef struct tagOLEVERB {
	LONG lVerb;
	LPWSTR lpszVerbName;
	DWORD fuFlags;
	DWORD grfAttribs;
} OLEVERB,*LPOLEVERB;
typedef RECT BORDERWIDTHS;
typedef LPRECT LPBORDERWIDTHS;
typedef LPCRECT LPCBORDERWIDTHS;
typedef struct tagOIFI {
	UINT cb;
	BOOL fMDIApp;
	HWND hwndFrame;
	HACCEL haccel;
	UINT cAccelEntries;
}OLEINPLACEFRAMEINFO,*LPOLEINPLACEFRAMEINFO;

DECLARE_ENUMERATOR(OLEVERB);
typedef IEnumOLEVERB IEnumOleVerb;

EXTERN_C const IID IID_IParseDisplayName;
#undef INTERFACE
#define INTERFACE IParseDisplayName
DECLARE_INTERFACE_(IParseDisplayName,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ParseDisplayName)(THIS_ IBindCtx*,LPOLESTR,ULONG*,IMoniker**) PURE;
};

#ifdef COBJMACROS
#define IParseDisplayName_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IParseDisplayName_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IParseDisplayName_Release(T) (T)->lpVtbl->Release(T)
#define IParseDisplayName_ParseDisplayName(T,a,b,c,d) (T)->lpVtbl->ParseDisplayName(T,a,b,c,d)
#endif

EXTERN_C const IID IID_IOleContainer;
#undef INTERFACE
#define INTERFACE IOleContainer
DECLARE_INTERFACE_(IOleContainer,IParseDisplayName)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ParseDisplayName)(THIS_ IBindCtx*,LPOLESTR,ULONG*,IMoniker**) PURE;
	STDMETHOD(EnumObjects)(THIS_ DWORD,IEnumUnknown**) PURE;
	STDMETHOD(LockContainer)(THIS_ BOOL) PURE;
};

EXTERN_C const IID IID_IOleItemContainer;
#undef INTERFACE
#define INTERFACE IOleItemContainer
DECLARE_INTERFACE_(IOleItemContainer,IOleContainer)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ParseDisplayName)(THIS_ IBindCtx*,LPOLESTR,ULONG*,IMoniker**) PURE;
	STDMETHOD(EnumObjects)(THIS_ DWORD,IEnumUnknown**) PURE;
	STDMETHOD(LockContainer)(THIS_ BOOL) PURE;
	STDMETHOD(GetObject)(THIS_ LPOLESTR,DWORD,IBindCtx*,REFIID,void**) PURE;
	STDMETHOD(GetObjectStorage)(THIS_ LPOLESTR,IBindCtx*,REFIID,void**) PURE;
	STDMETHOD(IsRunning)(THIS_ LPOLESTR) PURE;
};

#ifdef COBJMACROS
#define IOleItemContainer_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleItemContainer_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleItemContainer_Release(T) (T)->lpVtbl->Release(T)
#define IOleItemContainer_ParseDisplayName(T,a,b,c,d) (T)->lpVtbl->ParseDisplayName(T,a,b,c,d)
#define IOleItemContainer_EnumObjects(T,a,b) (T)->lpVtbl->EnumObjects(T,a,b)
#define IOleItemContainer_LockContainer(T,a) (T)->lpVtbl->LockContainer(T,a)
#define IOleItemContainer_GetObject(T,a,b,c,d,e) (T)->lpVtbl->GetObject(T,a,b,c,d,e)
#define IOleItemContainer_GetObjectStorage(T,a,b,c,d) (T)->lpVtbl->GetObjectStorage(T,a,b,c,d)
#define IOleItemContainer_IsRunning(T,a) (T)->lpVtbl->IsRunning(T,a)
#endif

EXTERN_C const IID IID_IOleClientSite;
#undef INTERFACE
#define INTERFACE IOleClientSite
DECLARE_INTERFACE_(IOleClientSite,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SaveObject)(THIS) PURE;
	STDMETHOD(GetMoniker)(THIS_ DWORD,DWORD,LPMONIKER*) PURE;
	STDMETHOD(GetContainer)(THIS_ LPOLECONTAINER*) PURE;
	STDMETHOD(ShowObject)(THIS) PURE;
	STDMETHOD(OnShowWindow)(THIS_ BOOL) PURE;
	STDMETHOD(RequestNewObjectLayout)(THIS) PURE;
};

#ifdef COBJMACROS
#define IOleClientSite_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleClientSite_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleClientSite_Release(T) (T)->lpVtbl->Release(T)
#define IOleClientSite_SaveObject(T) (T)->lpVtbl->SaveObject(T)
#define IOleClientSite_GetMoniker(T,a,b,c) (T)->lpVtbl->GetMoniker(T,a,b,c)
#define IOleClientSite_GetContainer(T,a) (T)->lpVtbl->GetContainer(T,a)
#define IOleClientSite_ShowObject(T) (T)->lpVtbl->ShowObject(T)
#define IOleClientSite_OnShowWindow(T,a) (T)->lpVtbl->OnShowWindow(T,a)
#define IOleClientSite_RequestNewObjectLayout(T) (T)->lpVtbl->RequestNewObjectLayout(T)
#endif

EXTERN_C const IID IID_IOleObject;
#undef INTERFACE
#define INTERFACE IOleObject
DECLARE_INTERFACE_(IOleObject,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SetClientSite)(THIS_ LPOLECLIENTSITE) PURE;
	STDMETHOD(GetClientSite)(THIS_ LPOLECLIENTSITE*) PURE;
	STDMETHOD(SetHostNames)(THIS_ LPCOLESTR,LPCOLESTR) PURE;
	STDMETHOD(Close)(THIS_ DWORD) PURE;
	STDMETHOD(SetMoniker)(THIS_ DWORD,LPMONIKER) PURE;
	STDMETHOD(GetMoniker)(THIS_ DWORD,DWORD,LPMONIKER*) PURE;
	STDMETHOD(InitFromData)(THIS_ LPDATAOBJECT,BOOL,DWORD) PURE;
	STDMETHOD(GetClipboardData)(THIS_ DWORD,LPDATAOBJECT*) PURE;
	STDMETHOD(DoVerb)(THIS_ LONG,LPMSG,LPOLECLIENTSITE,LONG,HWND,LPCRECT) PURE;
	STDMETHOD(EnumVerbs)(THIS_ LPENUMOLEVERB*) PURE;
	STDMETHOD(Update)(THIS) PURE;
	STDMETHOD(IsUpToDate)(THIS) PURE;
	STDMETHOD(GetUserClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(GetUserType)(THIS_ DWORD,LPOLESTR*) PURE;
	STDMETHOD(SetExtent)(THIS_ DWORD,SIZEL*) PURE;
	STDMETHOD(GetExtent)(THIS_ DWORD,SIZEL*) PURE;
	STDMETHOD(Advise)(THIS_ LPADVISESINK,PDWORD) PURE;
	STDMETHOD(Unadvise)(THIS_ DWORD) PURE;
	STDMETHOD(EnumAdvise)(THIS_ LPENUMSTATDATA*) PURE;
	STDMETHOD(GetMiscStatus)(THIS_ DWORD,PDWORD) PURE;
	STDMETHOD(SetColorScheme)(THIS_ LPLOGPALETTE) PURE;
};

#ifdef COBJMACROS
#define IOleObject_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleObject_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleObject_Release(T) (T)->lpVtbl->Release(T)
#define IOleObject_SetClientSite(T,a) (T)->lpVtbl->SetClientSite(T,a)
#define IOleObject_GetClientSite(T,a) (T)->lpVtbl->GetClientSite(T,a)
#define IOleObject_SetHostNames(T,a,b) (T)->lpVtbl->SetHostNames(T,a,b)
#define IOleObject_Close(T,a) (T)->lpVtbl->Close(T,a)
#define IOleObject_SetMoniker(T,a,b) (T)->lpVtbl->SetMoniker(T,a,b)
#define IOleObject_GetMoniker(T,a,b,c) (T)->lpVtbl->GetMoniker(T,a,b,c)
#define IOleObject_InitFromData(T,a,b,c) (T)->lpVtbl->InitFromData(T,a,b,c)
#define IOleObject_GetClipboardData(T,a,b) (T)->lpVtbl->GetClipboardData(T,a,b)
#define IOleObject_DoVerb(T,a,b,c,d,e,f) (T)->lpVtbl->DoVerb(T,a,b,c,d,e,f)
#define IOleObject_EnumVerbs(T,a) (T)->lpVtbl->EnumVerbs(T,a)
#define IOleObject_Update(T) (T)->lpVtbl->Update(T)
#define IOleObject_IsUpToDate(T) (T)->lpVtbl->IsUpToDate(T)
#define IOleObject_GetUserClassID(T,a) (T)->lpVtbl->GetUserClassID(T,a)
#define IOleObject_GetUserType(T,a,b) (T)->lpVtbl->GetUserType(T,a,b)
#define IOleObject_SetExtent(T,a,b) (T)->lpVtbl->SetExtent(T,a,b)
#define IOleObject_GetExtent(T,a,b) (T)->lpVtbl->GetExtent(T,a,b)
#define IOleObject_Advise(T,a,b) (T)->lpVtbl->Advise(T,a,b)
#define IOleObject_Unadvise(T,a) (T)->lpVtbl->Unadvise(T,a)
#define IOleObject_EnumAdvise(T,a) (T)->lpVtbl->EnumAdvise(T,a)
#define IOleObject_GetMiscStatus(T,a,b) (T)->lpVtbl->GetMiscStatus(T,a,b)
#define IOleObject_SetColorScheme(T,a) (T)->lpVtbl->SetColorScheme(T,a)
#endif

EXTERN_C const IID IID_IOleWindow;
#undef INTERFACE
#define INTERFACE IOleWindow
DECLARE_INTERFACE_(IOleWindow,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
};

#ifdef COBJMACROS
#define IOleWindow_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleWindow_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleWindow_Release(T) (T)->lpVtbl->Release(T)
#define IOleWindow_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IOleWindow_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#endif

EXTERN_C const IID IID_IOleInPlaceUIWindow;
#undef INTERFACE
#define INTERFACE IOleInPlaceUIWindow
DECLARE_INTERFACE_(IOleInPlaceUIWindow,IOleWindow)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(GetBorder)(THIS_ LPRECT) PURE;
	STDMETHOD(RequestBorderSpace)(THIS_ LPCBORDERWIDTHS) PURE;
	STDMETHOD(SetBorderSpace)(THIS_ LPCBORDERWIDTHS) PURE;
	STDMETHOD(SetActiveObject)(THIS_ LPOLEINPLACEACTIVEOBJECT,LPCOLESTR) PURE;
};

EXTERN_C const IID IID_IOleInPlaceObject;
#undef INTERFACE
#define INTERFACE IOleInPlaceObject
DECLARE_INTERFACE_(IOleInPlaceObject,IOleWindow)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(InPlaceDeactivate)(THIS) PURE;
	STDMETHOD(UIDeactivate)(THIS) PURE;
	STDMETHOD(SetObjectRects)(THIS_ LPCRECT,LPCRECT) PURE;
	STDMETHOD(ReactivateAndUndo)(THIS) PURE;
};

EXTERN_C const IID IID_IOleInPlaceActiveObject;
#undef INTERFACE
#define INTERFACE IOleInPlaceActiveObject
DECLARE_INTERFACE_(IOleInPlaceActiveObject,IOleWindow)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG) PURE;
	STDMETHOD(OnFrameWindowActivate)(THIS_ BOOL) PURE;
	STDMETHOD(OnDocWindowActivate)(THIS_ BOOL) PURE;
	STDMETHOD(ResizeBorder)(THIS_ LPCRECT,LPOLEINPLACEUIWINDOW,BOOL) PURE;
	STDMETHOD(EnableModeless)(THIS_ BOOL) PURE;
};

EXTERN_C const IID IID_IOleInPlaceFrame;
#undef INTERFACE
#define INTERFACE IOleInPlaceFrame
DECLARE_INTERFACE_(IOleInPlaceFrame,IOleInPlaceUIWindow)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(GetBorder)(THIS_ LPRECT) PURE;
	STDMETHOD(RequestBorderSpace)(THIS_ LPCBORDERWIDTHS) PURE;
	STDMETHOD(SetBorderSpace)(THIS_ LPCBORDERWIDTHS) PURE;
	STDMETHOD(SetActiveObject)(THIS_ LPOLEINPLACEACTIVEOBJECT,LPCOLESTR) PURE;
	STDMETHOD(InsertMenus)(THIS_ HMENU,LPOLEMENUGROUPWIDTHS) PURE;
	STDMETHOD(SetMenu)(THIS_ HMENU,HOLEMENU,HWND) PURE;
	STDMETHOD(RemoveMenus)(THIS_ HMENU) PURE;
	STDMETHOD(SetStatusText)(THIS_ LPCOLESTR) PURE;
	STDMETHOD(EnableModeless)(THIS_ BOOL) PURE;
	STDMETHOD(TranslateAccelerator)(THIS_ LPMSG,WORD) PURE;
};

#ifdef COBJMACROS
#define IOleInPlaceFrame_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleInPlaceFrame_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleInPlaceFrame_Release(T) (T)->lpVtbl->Release(T)
#define IOleInPlaceFrame_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IOleInPlaceFrame_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IOleInPlaceFrame_GetBorder(T,a) (T)->lpVtbl->GetBorder(T,a)
#define IOleInPlaceFrame_RequestBorderSpace(T,a) (T)->lpVtbl->RequestBorderSpace(T,a)
#define IOleInPlaceFrame_SetBorderSpace(T,a) (T)->lpVtbl->SetBorderSpace(T,a)
#define IOleInPlaceFrame_SetActiveObject(T,a,b) (T)->lpVtbl->SetActiveObject(T,a,b)
#define IOleInPlaceFrame_InsertMenus(T,a,b) (T)->lpVtbl->InsertMenus(T,a,b)
#define IOleInPlaceFrame_SetMenu(T,a,b,c) (T)->lpVtbl->SetMenu(T,a,b,c)
#define IOleInPlaceFrame_RemoveMenus(T,a) (T)->lpVtbl->RemoveMenus(T,a)
#define IOleInPlaceFrame_SetStatusText(T,a) (T)->lpVtbl->SetStatusText(T,a)
#define IOleInPlaceFrame_EnableModeless(T,a) (T)->lpVtbl->EnableModeless(T,a)
#define IOleInPlaceFrame_TranslateAccelerator(T,a,b) (T)->lpVtbl->TranslateAccelerator(T,a,b)
#endif

EXTERN_C const IID IID_IOleInPlaceSite;
#undef INTERFACE
#define INTERFACE IOleInPlaceSite
DECLARE_INTERFACE_(IOleInPlaceSite,IOleWindow)
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
};

EXTERN_C const IID IID_IOleAdviseHolder;
#undef INTERFACE
#define INTERFACE IOleAdviseHolder
DECLARE_INTERFACE_(IOleAdviseHolder,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Advise)(THIS_ LPADVISESINK,PDWORD) PURE;
	STDMETHOD(Unadvise)(THIS_ DWORD) PURE;
	STDMETHOD(EnumAdvise)(THIS_ LPENUMSTATDATA*) PURE;
	STDMETHOD(SendOnRename)(THIS_ LPMONIKER) PURE;
	STDMETHOD(SendOnSave)(THIS) PURE;
	STDMETHOD(SendOnClose)(THIS) PURE;
};

#ifdef COBJMACROS
#define IOleAdviseHolder_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleAdviseHolder_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleAdviseHolder_Release(T) (T)->lpVtbl->Release(T)
#define IOleAdviseHolder_Advise(T,a,b) (T)->lpVtbl->Advise(T,a,b)
#define IOleAdviseHolder_Unadvise(T,a) (T)->lpVtbl->Unadvise(T,a)
#define IOleAdviseHolder_EnumAdvise(T,a) (T)->lpVtbl->EnumAdvise(T,a)
#define IOleAdviseHolder_SendOnRename(T,a) (T)->lpVtbl->SendOnRename(T,a)
#define IOleAdviseHolder_SendOnSave(T) (T)->lpVtbl->SendOnSave(T)
#define IOleAdviseHolder_SendOnClose(T) (T)->lpVtbl->SendOnClose(T)
#endif

EXTERN_C const IID IID_IDropSource;
#undef INTERFACE
#define INTERFACE IDropSource
DECLARE_INTERFACE_(IDropSource,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(QueryContinueDrag)(THIS_ BOOL,DWORD) PURE;
	STDMETHOD(GiveFeedback)(THIS_ DWORD) PURE;
};

#ifdef COBJMACROS
#define IDropSource_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDropSource_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDropSource_Release(T) (T)->lpVtbl->Release(T)
#define IDropSource_QueryContinueDrag(T,a,b) (T)->lpVtbl->QueryContinueDrag(T,a,b)
#define IDropSource_GiveFeedback(T,a) (T)->lpVtbl->GiveFeedback(T,a)
#endif

EXTERN_C const IID IID_IDropTarget;
#undef INTERFACE
#define INTERFACE IDropTarget
DECLARE_INTERFACE_(IDropTarget,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(DragEnter)(THIS_ LPDATAOBJECT,DWORD,POINTL,PDWORD) PURE;
	STDMETHOD(DragOver)(THIS_ DWORD,POINTL,PDWORD) PURE;
	STDMETHOD(DragLeave)(THIS) PURE;
	STDMETHOD(Drop)(THIS_ LPDATAOBJECT,DWORD,POINTL,PDWORD) PURE;
};

#ifdef COBJMACROS
#define IDropTarget_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDropTarget_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDropTarget_Release(T) (T)->lpVtbl->Release(T)
#define IDropTarget_DragEnter(T,a,b,c,d) (T)->lpVtbl->DragEnter(T,a,b,c,d)
#define IDropTarget_DragOver(T,a,b,c) (T)->lpVtbl->DragOver(T,a,b,c)
#define IDropTarget_DragLeave(T) (T)->lpVtbl->DragLeave(T)
#define IDropTarget_Drop(T,a,b,c,d) (T)->lpVtbl->Drop(T,a,b,c,d)
#endif

typedef BOOL(CALLBACK *__IView_pfncont)(DWORD);
EXTERN_C const IID IID_IViewObject;
#undef INTERFACE
#define INTERFACE IViewObject
DECLARE_INTERFACE_(IViewObject,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Draw)(THIS_ DWORD,LONG,PVOID,DVTARGETDEVICE*,HDC,HDC,LPCRECTL,LPCRECTL,__IView_pfncont pfnContinue,DWORD) PURE;
	STDMETHOD(GetColorSet)(THIS_ DWORD,LONG,PVOID,DVTARGETDEVICE*,HDC,LPLOGPALETTE*) PURE;
	STDMETHOD(Freeze)(THIS_ DWORD,LONG,PVOID,PDWORD) PURE;
	STDMETHOD(Unfreeze)(THIS_ DWORD) PURE;
	STDMETHOD(SetAdvise)(THIS_ DWORD,DWORD,IAdviseSink*) PURE;
	STDMETHOD(GetAdvise)(THIS_ PDWORD,PDWORD,IAdviseSink**) PURE;
};

#ifdef COBJMACROS
#define IViewObject_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IViewObject_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IViewObject_Release(T) (T)->lpVtbl->Release(T)
#define IViewObject_Draw(T,a,b,c,d,e,f,g,h,i,j) (T)->lpVtbl->Draw(T,a,b,c,d,e,f,g,h,i,j)
#define IViewObject_GetColorSet(T,a,b,c,d,e,f) (T)->lpVtbl->GetColorSet(T,a,b,c,d,e,f)
#define IViewObject_Freeze(T,a,b,c,d) (T)->lpVtbl->Freeze(T,a,b,c,d)
#define IViewObject_Unfreeze(T,a) (T)->lpVtbl->Unfreeze(T,a)
#define IViewObject_SetAdvise(T,a,b,c) (T)->lpVtbl->SetAdvise(T,a,b,c)
#define IViewObject_GetAdvise(T,a,b,c) (T)->lpVtbl->GetAdvise(T,a,b,c)
#endif

EXTERN_C const IID IID_IViewObject2;
#undef INTERFACE
#define INTERFACE IViewObject2
DECLARE_INTERFACE_(IViewObject2,IViewObject)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Draw)(THIS_ DWORD,LONG,PVOID,DVTARGETDEVICE*,HDC,HDC,LPCRECTL,LPCRECTL,__IView_pfncont pfnContinue,DWORD) PURE;
	STDMETHOD(GetColorSet)(THIS_ DWORD,LONG,PVOID,DVTARGETDEVICE*,HDC,LPLOGPALETTE*) PURE;
	STDMETHOD(Freeze)(THIS_ DWORD,LONG,PVOID,PDWORD) PURE;
	STDMETHOD(Unfreeze)(THIS_ DWORD) PURE;
	STDMETHOD(SetAdvise)(THIS_ DWORD,DWORD,IAdviseSink*) PURE;
	STDMETHOD(GetAdvise)(THIS_ PDWORD,PDWORD,IAdviseSink**) PURE;
	STDMETHOD(GetExtent)(THIS_ DWORD,LONG,DVTARGETDEVICE*,LPSIZEL) PURE;
};

#ifdef COBJMACROS
#define IViewObject2_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IViewObject2_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IViewObject2_Release(T) (T)->lpVtbl->Release(T)
#define IViewObject2_Draw(T,a,b,c,d,e,f,g,h,i,j) (T)->lpVtbl->Draw(T,a,b,c,d,e,f,g,h,i,j)
#define IViewObject2_GetColorSet(T,a,b,c,d,e,f) (T)->lpVtbl->GetColorSet(T,a,b,c,d,e,f)
#define IViewObject2_Freeze(T,a,b,c,d) (T)->lpVtbl->Freeze(T,a,b,c,d)
#define IViewObject2_Unfreeze(T,a) (T)->lpVtbl->Unfreeze(T,a)
#define IViewObject2_SetAdvise(T,a,b,c) (T)->lpVtbl->SetAdvise(T,a,b,c)
#define IViewObject2_GetAdvise(T,a,b,c) (T)->lpVtbl->GetAdvise(T,a,b,c)
#define IViewObject2_GetExtent(T,a,b,c,d) (T)->lpVtbl->GetExtent(T,a,b,c,d)
#endif

EXTERN_C const IID IID_IOleCache;
#undef INTERFACE
#define INTERFACE IOleCache
DECLARE_INTERFACE_(IOleCache,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Cache)(THIS_ FORMATETC*,DWORD,DWORD*) PURE;
	STDMETHOD(Uncache)(THIS_ DWORD) PURE; 
	STDMETHOD(EnumCache)(THIS_ IEnumSTATDATA**); 
	STDMETHOD(InitCache)(THIS_ LPDATAOBJECT); 
	STDMETHOD(SetData)(THIS_ FORMATETC*,STGMEDIUM*,BOOL); 
};

#ifdef COBJMACROS
#define IOleCache_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleCache_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleCache_Release(T) (T)->lpVtbl->Release(T)
#define IOleCache_Cache(T,a,b,c) (T)->lpVtbl->Cache(T,a,b,c)
#define IOleCache_Uncache(T,a) (T)->lpVtbl->Uncache(T,a)
#define IOleCache_EnumCache(T,a) (T)->lpVtbl->EnumCache(T,a)
#define IOleCache_InitCache(T,a) (T)->lpVtbl->InitCache(T,a)
#define IOleCache_SetData(T,a,b,c) (T)->lpVtbl->SetData(T,a,b,c)
#endif

EXTERN_C const IID IID_IOleCache2;
#undef INTERFACE
#define INTERFACE IOleCache2
DECLARE_INTERFACE_(IOleCache2,IOleCache)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Cache)(THIS_ FORMATETC*,DWORD,DWORD*) PURE;
	STDMETHOD(Uncache)(THIS_ DWORD) PURE; 
	STDMETHOD(EnumCache)(THIS_ IEnumSTATDATA**) PURE; 
	STDMETHOD(InitCache)(THIS_ LPDATAOBJECT) PURE; 
	STDMETHOD(SetData)(THIS_ FORMATETC*,STGMEDIUM*,BOOL) PURE; 
	STDMETHOD(UpdateCache)(THIS_ LPDATAOBJECT,DWORD,LPVOID) PURE; 
	STDMETHOD(DiscardCache)(THIS_ DWORD) PURE;
};

#ifdef COBJMACROS
#define IOleCache2_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IOleCache2_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IOleCache2_Release(T) (T)->lpVtbl->Release(T)
#define IOleCache2_Cache(T,a,b,c) (T)->lpVtbl->Cache(T,a,b,c)
#define IOleCache2_Uncache(T,a) (T)->lpVtbl->Uncache(T,a)
#define IOleCache2_EnumCache(T,a) (T)->lpVtbl->EnumCache(T,a)
#define IOleCache2_InitCache(T,a) (T)->lpVtbl->InitCache(T,a)
#define IOleCache2_SetData(T,a,b,c) (T)->lpVtbl->SetData(T,a,b,c)
#define IOleCache2_UpdateCache(T,a,b,c) (T)->lpVtbl->UpdateCache(T,a,b,c)
#define IOleCache2_DiscardCache(T,a) (T)->lpVtbl->DiscardCache(T,a)
#endif

EXTERN_C const IID IID_IOleCacheControl;
#undef INTERFACE
#define INTERFACE IOleCacheControl
DECLARE_INTERFACE_(IOleCacheControl,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(OnRun)(THIS_ LPDATAOBJECT) PURE;
	STDMETHOD(OnStop)(THIS) PURE;
};

#ifdef __cplusplus
}
#endif
#endif

