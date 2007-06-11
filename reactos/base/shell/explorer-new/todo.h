#ifndef __TODO_H
#define __TODO_H

/*
 * Stuff missing in our headers
 */

#define SM_REMOTECONTROL 0x2001

/* FIXME: Ugly hack!!! FIX ASAP! Move to uuid! */
static const GUID IID_HACK_IShellView2 = {0x88E39E80,0x3578,0x11CF,{0xAE,0x69,0x08,0x00,0x2B,0x2E,0x12,0x62}};
#define IID_IShellView2 IID_HACK_IShellView2
static const GUID IID_HACK_IShellView3 = {0xEC39FA88,0xF8AF,0x41CF,{0x84,0x21,0x38,0xBE,0xD2,0x8F,0x46,0x73}};
#define IID_IShellView3 IID_HACK_IShellView2
static const GUID VID_HACK_LargeIcons = {0x0057D0E0, 0x3573, 0x11CF, {0xAE, 0x69, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62}};
#define VID_LargeIcons VID_HACK_LargeIcons

static const GUID IID_HACK_IDeskBarClient = {0xEB0FE175, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBarClient IID_HACK_IDeskBarClient
static const GUID IID_HACK_IDeskBar = {0xEB0FE173, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBar IID_HACK_IDeskBar

static const GUID IID_HACK_IMenuPopup = {0xD1E7AFEB,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define IID_IMenuPopup IID_HACK_IMenuPopup
static const GUID IID_HACK_IBanneredBar = {0x596A9A94,0x013E,0x11D1,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define IID_IBanneredBar IID_HACK_IBanneredBar

static const GUID IID_HACK_IInitializeObject = {0x4622AD16,0xFF23,0x11D0,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define IID_IInitializeObject IID_HACK_IInitializeObject

static const GUID SID_HACK_SMenuPopup = {0xD1E7AFEB,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define SID_SMenuPopup SID_HACK_SMenuPopup



#ifdef COBJMACROS
#define IDockingWindow_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDockingWindow_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDockingWindow_Release(T) (T)->lpVtbl->Release(T)
#define IDockingWindow_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDockingWindow_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDockingWindow_ShowDW(T,a) (T)->lpVtbl->ShowDW(T,a)
#define IDockingWindow_CloseDW(T,a) (T)->lpVtbl->CloseDW(T,a)
#define IDockingWindow_ResizeBorderDW(T,a,b,c) (T)->lpVtbl->ResizeBorderDW(T,a,b,c)
#endif

#define INTERFACE IDeskBarClient
DECLARE_INTERFACE_(IDeskBarClient,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IDeskBarClient methods ***/
    STDMETHOD_(HRESULT,SetDeskBarSite)(THIS_ IUnknown*) PURE;
    STDMETHOD_(HRESULT,SetModeDBC)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,UIActivateDBC)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,GetSize)(THIS_ DWORD,LPRECT) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IDeskBarClient_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDeskBarClient_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDeskBarClient_Release(T) (T)->lpVtbl->Release(T)
#define IDeskBarClient_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDeskBarClient_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDeskBarClient_SetDeskBarSite(T,a) (T)->lpVtbl->SetDeskBarSite(T,a)
#define IDeskBarClient_SetModeDBC(T,a) (T)->lpVtbl->SetModeDBC(T,a)
#define IDeskBarClient_UIActivateDBC(T,a) (T)->lpVtbl->UIActivateDBC(T,a)
#define IDeskBarClient_GetSize(T,a,b) (T)->lpVtbl->GetSize(T,a,b)
#endif

#define DBIM_TITLE  0x10
#define DBIMF_VARIABLEHEIGHT    0x8
#define DBIMF_DEBOSSED  0x20
#define DBIF_VIEWMODE_VERTICAL  0x1

#include <pshpack8.h>
typedef struct tagDESKBANDINFO
{
    DWORD dwMask;
    POINTL ptMinSize;
    POINTL ptMaxSize;
    POINTL ptIntegral;
    POINTL ptActual;
    WCHAR wszTitle[256];
    DWORD dwModeFlags;
    COLORREF crBkgnd;
} DESKBANDINFO;
#include <poppack.h>

#define INTERFACE IDeskBand
DECLARE_INTERFACE_(IDeskBand,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IDockingWindow methods ***/
    STDMETHOD_(HRESULT,ShowDW)(THIS_ BOOL) PURE;
    STDMETHOD_(HRESULT,CloseDW)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,ResizeBoderDW)(THIS_ LPCRECT,IUnknown*,BOOL) PURE;
    /*** IDeskBand methods ***/
    STDMETHOD_(HRESULT,GetBandInfo)(THIS_ DWORD,DWORD,DESKBANDINFO*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IDeskBand_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDeskBand_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDeskBand_Release(T) (T)->lpVtbl->Release(T)
#define IDeskBand_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDeskBand_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDeskBand_ShowDW(T,a) (T)->lpVtbl->ShowDW(T,a)
#define IDeskBand_CloseDW(T,a) (T)->lpVtbl->CloseDW(T,a)
#define IDeskBand_ResizeBorderDW(T,a,b,c) (T)->lpVtbl->ResizeBorderDW(T,a,b,c)
#define IDeskBand_GetBandInfo(T,a,b,c) (T)->lpVtbl->GetBandInfo(T,a,b,c)
#endif

#define INTERFACE IDeskBar
DECLARE_INTERFACE_(IDeskBar,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IDeskBar methods ***/
    STDMETHOD_(HRESULT,SetClient)(THIS_ IUnknown*) PURE;
    STDMETHOD_(HRESULT,GetClient)(THIS_ IUnknown**) PURE;
    STDMETHOD_(HRESULT,OnPosRectChangeDB)(THIS_ RECT*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IDeskBar_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDeskBar_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDeskBar_Release(T) (T)->lpVtbl->Release(T)
#define IDeskBar_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDeskBar_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDeskBar_SetClient(T,a) (T)->lpVtbl->SetClient(T,a)
#define IDeskBar_GetClient(T,a) (T)->lpVtbl->GetClient(T,a)
#define IDeskBar_OnPosRectChangeDB(T,a) (T)->lpVtbl->OnPosRectChangeDB(T,a)
#endif

#include <pshpack8.h>
typedef struct
{
    DWORD dwMask;
    DWORD dwState;
    DWORD dwStyle;
} BANDSITEINFO;
#include <poppack.h>

#define BSIM_STATE  0x1
#define BSIM_STYLE  0x2

#define BSIS_AUTOGRIPPER    0x0
#define BSIS_NOGRIPPER  0x1
#define BSIS_ALWAYSGRIPPER  0x2
#define BSIS_LOCKED 0x100

#define BSSF_UNDELETEABLE   0x1000

DEFINE_GUID(IID_IBandSite, 0x4CF504B0, 0xDE96, 0x11D0, 0x8B, 0x3F, 0x00, 0xA0, 0xC9, 0x11, 0xE8, 0xE5);

#define INTERFACE IBandSite
DECLARE_INTERFACE_(IBandSite,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IBandSite methods ***/
    STDMETHOD_(HRESULT,AddBand) (THIS_ IUnknown *punk) PURE;
    STDMETHOD_(HRESULT,EnumBands) (THIS_ UINT uBand, DWORD *pdwBandID) PURE;
    STDMETHOD_(HRESULT,QueryBand) (THIS_ DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName) PURE;
    STDMETHOD_(HRESULT,SetBandState) (THIS_ DWORD dwBandID, DWORD dwMask, DWORD dwState) PURE;
    STDMETHOD_(HRESULT,RemoveBand) (THIS_ DWORD dwBandID) PURE;
    STDMETHOD_(HRESULT,GetBandObject) (THIS_ DWORD dwBandID, REFIID riid, VOID **ppv) PURE;
    STDMETHOD_(HRESULT,SetBandSiteInfo) (THIS_ const BANDSITEINFO *pbsinfo) PURE;
    STDMETHOD_(HRESULT,GetBandSiteInfo) (THIS_ BANDSITEINFO *pbsinfo) PURE;
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define IBandSite_QueryInterface(p,a,b)     (p)->lpVtbl->QueryInterface(p,a,b)
#define IBandSite_AddRef(p)                 (p)->lpVtbl->AddRef(p)
#define IBandSite_Release(p)                (p)->lpVtbl->Release(p)
/*** IBandSite methods ***/
#define IBandSite_AddBand(p,a)              (p)->lpVtbl->AddBand(p,a)
#define IBandSite_EnumBands(p,a,b)          (p)->lpVtbl->EnumBands(p,a,b)
#define IBandSite_QueryBand(p,a,b,c,d,e)    (p)->lpVtbl->QueryBand(p,a,b,c,d,e)
#define IBandSite_SetBandState(p,a,b,c)     (p)->lpVtbl->SetBandState(p,a,b,c)
#define IBandSite_RemoveBand(p,a)           (p)->lpVtbl->RemoveBand(p,a)
#define IBandSite_GetBandObject(p,a,b,c)    (p)->lpVtbl->GetBandObject(p,a,b,c)
#define IBandSite_SetBandSiteInfo(p,a)      (p)->lpVtbl->SetBandSiteInfo(p,a)
#define IBandSite_GetBandSiteInfo(p,a)      (p)->lpVtbl->GetBandSiteInfo(p,a)
#endif

#include <pshpack8.h>
typedef struct _SV2CVW2_PARAMS
{
    DWORD cbSize;
    IShellView *psvPrev;
    LPCFOLDERSETTINGS pfs;
    IShellBrowser *psbOwner;
    RECT *prcView;
    SHELLVIEWID const *pvid;
    HWND hwndView;
} SV2CVW2_PARAMS, *LPSV2CVW2_PARAMS;
#include <poppack.h>

typedef ITEMIDLIST ITEMID_CHILD;
#define PITEMID_CHILD LPITEMIDLIST
typedef const ITEMID_CHILD /* __unaligned */ *PCUITEMID_CHILD;

enum tagSV3CVW3
{
    SV3CVW3_DEFAULT = 0x0,
    SV3CVW3_NONINTERACTIVE = 0x1,
    SV3CVW3_FORCEVIEWMODE = 0x2,
    SV3CVW3_FORCEFOLDERFLAGS = 0x4
} ;
typedef DWORD SV3CVW3_FLAGS;
#define INTERFACE IShellView3
DECLARE_INTERFACE_(IShellView3,IShellView)
{

	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
	STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
	STDMETHOD(TranslateAccelerator) (THIS_ LPMSG) PURE;
#ifdef _FIX_ENABLEMODELESS_CONFLICT
	STDMETHOD(EnableModelessSV)(THIS_ BOOL) PURE;
#else
	STDMETHOD(EnableModeless)(THIS_ BOOL) PURE;
#endif
	STDMETHOD(UIActivate)(THIS_ UINT) PURE;
	STDMETHOD(Refresh) (THIS) PURE;
	STDMETHOD(CreateViewWindow)(THIS_ IShellView*,LPCFOLDERSETTINGS,LPSHELLBROWSER,RECT*,HWND*) PURE;
	STDMETHOD(DestroyViewWindow)(THIS) PURE;
	STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS) PURE;
	STDMETHOD(AddPropertySheetPages)(THIS_ DWORD,LPFNADDPROPSHEETPAGE,LPARAM) PURE;
	STDMETHOD(SaveViewState)(THIS) PURE;
	STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST,UINT) PURE;
	STDMETHOD(GetItemObject)(THIS_ UINT,REFIID,PVOID*) PURE;
	STDMETHOD(GetView)(THIS_ SHELLVIEWID*,ULONG) PURE;
	STDMETHOD(CreateViewWindow2)(THIS_ LPSV2CVW2_PARAMS) PURE;
	STDMETHOD(HandleRename)(THIS_ PCUITEMID_CHILD) PURE;
	STDMETHOD(SelectAndPositionItem)(THIS_ PCUITEMID_CHILD,UINT,POINT*) PURE;
	STDMETHOD(CreateViewWindow3)(THIS_ IShellBrowser*,IShellView*,SV3CVW3_FLAGS,FOLDERFLAGS,FOLDERFLAGS,FOLDERVIEWMODE,const SHELLVIEWID*,const RECT*,HWND*) PURE;
};
#undef INTERFACE
#ifdef COBJMACROS
#define IShellView3_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellView3_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellView3_Release(T) (T)->lpVtbl->Release(T)
#define IShellView3_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IShellView3_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IShellView3_TranslateAccelerator(T,a) (T)->lpVtbl->TranslateAccelerator(T,a)
#ifdef _FIX_ENABLEMODELESS_CONFLICT
#define IShellView3_EnableModeless(T,a) (T)->lpVtbl->EnableModelessSV(T,a)
#else
#define IShellView3_EnableModeless(T,a) (T)->lpVtbl->EnableModeless(T,a)
#endif
#define IShellView3_UIActivate(T,a) (T)->lpVtbl->UIActivate(T,a)
#define IShellView3_Refresh(T) (T)->lpVtbl->Refresh(T)
#define IShellView3_CreateViewWindow(T,a,b,c,d,e) (T)->lpVtbl->CreateViewWindow(T,a,b,c,d,e)
#define IShellView3_DestroyViewWindow(T) (T)->lpVtbl->DestroyViewWindow(T)
#define IShellView3_GetCurrentInfo(T,a) (T)->lpVtbl->GetCurrentInfo(T,a)
#define IShellView3_AddPropertySheetPages(T,a,b,c) (T)->lpVtbl->AddPropertySheetPages(T,a,b,c)
#define IShellView3_SaveViewState(T) (T)->lpVtbl->SaveViewState(T)
#define IShellView3_SelectItem(T,a,b) (T)->lpVtbl->SelectItem(T,a,b)
#define IShellView3_GetItemObject(T,a,b,c) (T)->lpVtbl->GetItemObject(T,a,b,c)
#define IShellView3_GetView(T,a,b) (T)->lpVtbl->GetView(T,a,b)
#define IShellView3_CreateViewWindow2(T,a) (T)->lpVtbl->CreateViewWindow2(T,a)
#define IShellView3_HandleRename(T,a) (T)->lpVtbl->HandleRename(T,a)
#define IShellView3_SelectAndPositionItem(T,a,b,c) (T)->lpVtbl->SelectAndPositionItem(T,a,b,c)
#define IShellView3_CreateViewWindow3(T,a,b,c,d,e,f,g,h,i) (T)->lpVtbl->CreateViewWindow3(T,a,b,c,d,e,f,g,h,i)
#endif

#define SHGVSPB_PERUSER 0x1
#define SHGVSPB_PERFOLDER   0x4
#define SHGVSPB_ROAM    0x00000020
#define SHGVSPB_NOAUTODEFAULTS  0x80000000
#define SHGVSPB_FOLDER  (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER)
#define SHGVSPB_FOLDERNODEFAULTS    (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER | SHGVSPB_NOAUTODEFAULTS)


/*
 * DeskBand Command IDs
 */
enum tagDESKBANDCID
{
    DBID_BANDINFOCHANGED = 0,
    DBID_SHOWONLY,
    DBID_MAXIMIZEBAND,
    DBID_PUSHCHEVRON,
    DBID_DELAYINIT,
    DBID_FINISHINIT,
    DBID_SETWINDOWTHEME,
    DBID_PERMITAUTOHIDE
};

#define DBC_SHOW    1
#define DBC_HIDE    0

static const GUID IID_HACK_IShellService = {0x5836FB00,0x8187,0x11CF,{0xA1,0x2B,0x00,0xAA,0x00,0x4A,0xE8,0x37}};
#define IID_IShellService IID_HACK_IShellService

#define INTERFACE IShellService
DECLARE_INTERFACE_(IShellService,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellService methods ***/
    STDMETHOD_(HRESULT,SetOwner)(THIS_ IUnknown*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellService_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellService_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellService_Release(T) (T)->lpVtbl->Release(T)
#define IShellService_SetOwner(T,a) (T)->lpVtbl->SetOwner(T,a)
#endif

#if 0
HRESULT WINAPI SHGetViewStatePropertyBag(LPCITEMIDLIST,LPCWSTR,DWORD,REFIID,PVOID*);/* FIXME: Parameter should be PCIDLIST_ABSOLUTE */
#else
typedef HRESULT (WINAPI *PSHGetViewStatePropertyBag)(LPCITEMIDLIST,LPCWSTR,DWORD,REFIID,PVOID*);
static HRESULT __inline
SHGetViewStatePropertyBag(IN LPCITEMIDLIST pidl,
                          IN LPCWSTR pszBagName,
                          IN DWORD dwFlags,
                          IN REFIID riid,
                          OUT PVOID* ppv)
{
    static PSHGetViewStatePropertyBag Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHLWAPI.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHGetViewStatePropertyBag)GetProcAddress(hShlwapi, "SHGetViewStatePropertyBag");
        }
    }

    if (Func != NULL)
    {
        return Func(pidl, pszBagName, dwFlags, riid, ppv);
    }

    MessageBox(NULL, TEXT("SHGetViewStatePropertyBag not available"), NULL, 0);
    return E_NOTIMPL;
}
#endif

#define PIDLIST_ABSOLUTE LPITEMIDLIST
PIDLIST_ABSOLUTE WINAPI SHCloneSpecialIDList(HWND hwnd, int csidl, BOOL fCreate);

enum
{
    BMICON_LARGE = 0,
    BMICON_SMALL
};
#define INTERFACE IBanneredBar
DECLARE_INTERFACE_(IBanneredBar,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IBanneredBar methods ***/
    STDMETHOD_(HRESULT,SetIconSize)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,GetIconSize)(THIS_ DWORD*) PURE;
    STDMETHOD_(HRESULT,SetBitmap)(THIS_ HBITMAP) PURE;
    STDMETHOD_(HRESULT,GetBitmap)(THIS_ HBITMAP*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBanneredBar_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBanneredBar_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBanneredBar_Release(T) (T)->lpVtbl->Release(T)
#define IBanneredBar_SetIconSize(T,a) (T)->lpVtbl->SetIconSize(T,a)
#define IBanneredBar_GetIconSize(T,a) (T)->lpVtbl->GetIconSize(T,a)
#define IBanneredBar_SetBitmap(T,a) (T)->lpVtbl->SetBitmap(T,a)
#define IBanneredBar_GetBitmap(T,a) (T)->lpVtbl->GetBitmap(T,a)
#endif

enum tagMENUPOPUPPOPUPFLAGS
{
    MPPF_SETFOCUS = 0x1,
    MPPF_INITIALSELECT = 0x2,
    MPPF_NOANIMATE = 0x4,
    MPPF_KEYBOARD = 0x10,
    MPPF_REPOSITION = 0x20,
    MPPF_FORCEZORDER = 0x40,
    MPPF_FINALSELECT = 0x80,
    MPPF_TOP = 0x20000000,
    MPPF_LEFT = 0x40000000,
    MPPF_RIGHT = 0x60000000,
    MPPF_BOTTOM = 0x80000000,
    MPPF_POS_MASK = 0xE0000000,
    MPPF_ALIGN_LEFT = 0x2000000,
    MPPF_ALIGN_RIGHT = 0x4000000,
};
typedef int MP_POPUPFLAGS;

#define INTERFACE IMenuPopup
DECLARE_INTERFACE_(IMenuPopup,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IDeskBar methods ***/
    STDMETHOD_(HRESULT,SetClient)(THIS_ IUnknown*) PURE;
    STDMETHOD_(HRESULT,GetClient)(THIS_ IUnknown**) PURE;
    STDMETHOD_(HRESULT,OnPosRectChangeDB)(THIS_ RECT*) PURE;
    /*** IMenuPopup methods ***/
    STDMETHOD_(HRESULT,Popup)(THIS_ POINTL*,RECTL*,MP_POPUPFLAGS) PURE;
    STDMETHOD_(HRESULT,OnSelect)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,SetSubMenu)(THIS_ IMenuPopup*,BOOL) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IMenuPopup_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IMenuPopup_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IMenuPopup_Release(T) (T)->lpVtbl->Release(T)
#define IMenuPopup_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IMenuPopup_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IMenuPopup_SetClient(T,a) (T)->lpVtbl->SetClient(T,a)
#define IMenuPopup_GetClient(T,a) (T)->lpVtbl->GetClient(T,a)
#define IMenuPopup_OnPosRectChangeDB(T,a) (T)->lpVtbl->OnPosRectChangeDB(T,a)
#define IMenuPopup_Popup(T,a,b,c) (T)->lpVtbl->Popup(T,a,b,c)
#define IMenuPopup_OnSelect(T,a) (T)->lpVtbl->OnSelect(T,a)
#define IMenuPopup_SetSubMenu(T,a,b) (T)->lpVtbl->SetSubMenu(T,a,b)
#endif

#define INTERFACE IMenuBand
DECLARE_INTERFACE_(IMenuBand,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMenuBand methods ***/
    STDMETHOD_(HRESULT,IsMenuMessage)(THIS_ MSG*) PURE;
    STDMETHOD_(HRESULT,TranslateMenuMessage)(THIS_ MSG*,LRESULT*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IMenuBand_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IMenuBand_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IMenuBand_Release(T) (T)->lpVtbl->Release(T)
#define IMenuBand_IsMenuMessage(T,a) (T)->lpVtbl->IsMenuMessage(T,a)
#define IMenuBand_TranslateMenuMessage(T,a,b) (T)->lpVtbl->TranslateMenuMessage(T,a,b)
#endif

#define INTERFACE IInitializeObject
DECLARE_INTERFACE_(IInitializeObject,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IInitializeObject methods ***/
    STDMETHOD_(HRESULT,Initialize)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IInitializeObject_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IInitializeObject_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IInitializeObject_Release(T) (T)->lpVtbl->Release(T)
#define IInitializeObject_Initialize(T) (T)->lpVtbl->Initialize(T)
#endif

#endif /* __TODO_H */
