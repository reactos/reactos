#ifndef _EXPLORER_UNDOC__H
#define _EXPLORER_UNDOC__H

/*
 * Undocumented stuff
 */

/* IMenuDeskBar provides the band site toolbars menu */
static const CLSID CLSID_HACK_IShellBandSiteMenu = {0xECD4FC4E,0x521C,0x11D0,{0xB7,0x92,0x00,0xA0,0xC9,0x03,0x12,0xE1}};
#define CLSID_IShellBandSiteMenu CLSID_HACK_IShellBandSiteMenu

static const GUID IID_HACK_IBandSiteStreamCallback = {0xD1E7AFEA,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define IID_IBandSiteStreamCallback IID_HACK_IBandSiteStreamCallback

static const GUID CLSID_HACK_StartMenu = {0x4622AD11,0xFF23,0x11D0,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define CLSID_StartMenu CLSID_HACK_StartMenu
static const GUID CLSID_HACK_PersonalStartMenu = {0x3F6953F0,0x5359,0x47FC,{0xBD,0x99,0x9F,0x2C,0xB9,0x5A,0x62,0xFD}};
#define CLSID_PersonalStartMenu CLSID_HACK_PersonalStartMenu

static const GUID IID_HACK_IMenuBand = {0x568804CD,0xCBD7,0x11D0,{0x98,0x16,0x00,0xC0,0x4F,0xD9,0x19,0x72}};
#define IID_IMenuBand IID_HACK_IMenuBand

static const GUID IID_HACK_IStartMenuCallback = {0x4622AD10,0xFF23,0x11D0,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define IID_IStartMenuCallback IID_HACK_IStartMenuCallback

#define INTERFACE IStartMenuCallback
DECLARE_INTERFACE_(IStartMenuCallback,IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IStartMenuCallback ***/
    STDMETHOD_(HRESULT,Execute)(THIS_ IShellFolder*,LPCITEMIDLIST) PURE;
    STDMETHOD_(HRESULT,Unknown)(THIS_ PVOID,PVOID,PVOID,PVOID) PURE;
    STDMETHOD_(HRESULT,AppendMenu)(THIS_ HMENU*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IStartMenuCallback_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IStartMenuCallback_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IStartMenuCallback_Release(T) (T)->lpVtbl->Release(T)
#define IStartMenuCallback_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IStartMenuCallback_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IStartMenuCallback_Execute(T,a,b) (T)->lpVtbl->Execute(T,a,b)
#define IStartMenuCallback_Unknown(T,a,b,c,d) (T)->lpVtbl->Unknown(T,a,b,c,d)
#define IStartMenuCallback_AppendMenu(T,a) (T)->lpVtbl->AppendMenu(T,a)
#endif

#define INTERFACE IBandSiteStreamCallback
DECLARE_INTERFACE_(IBandSiteStreamCallback,IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IBandSiteStreamCallback ***/
    STDMETHOD_(HRESULT,OnLoad)(THIS_ IStream *pStm, REFIID riid, PVOID *pvObj) PURE;
    STDMETHOD_(HRESULT,OnSave)(THIS_ IUnknown *pUnk, IStream *pStm) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBandSiteStreamCallback_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBandSiteStreamCallback_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBandSiteStreamCallback_Release(T) (T)->lpVtbl->Release(T)
#define IBandSiteStreamCallback_OnLoad(T,a,b,c) (T)->lpVtbl->OnLoad(T,a,b,c)
#define IBandSiteStreamCallback_OnSave(T,a,b) (T)->lpVtbl->OnSave(T,a,b)
#endif

static const GUID IID_HACK_IWindowEventHandler = {0xEA5F2D61,0xE008,0x11CF,{0x99,0xCB,0x00,0xC0,0x4F,0xD6,0x44,0x97}};
#define IID_IWindowEventHandler IID_HACK_IWindowEventHandler

#define INTERFACE IWindowEventHandler
DECLARE_INTERFACE_(IWindowEventHandler,IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWindowEventHandler ***/
    STDMETHOD(ProcessMessage)(THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult) PURE;
    STDMETHOD(ContainsWindow)(THIS_ HWND hWnd) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IWindowEventHandler_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IWindowEventHandler_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IWindowEventHandler_Release(T) (T)->lpVtbl->Release(T)
#define IWindowEventHandler_ProcessMessage(T,a,b,c,d,e) (T)->lpVtbl->ProcessMessage(T,a,b,c,d,e)
#define IWindowEventHandler_ContainsWindow(T,a) (T)->lpVtbl->ContainsWindow(T,a)
#endif

#define INTERFACE IShellDesktopTray
DECLARE_INTERFACE_(IShellDesktopTray,IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellDesktopTray ***/
    STDMETHOD_(ULONG,GetState)(THIS) PURE;
    STDMETHOD(GetTrayWindow)(THIS_ HWND*) PURE;
    STDMETHOD(RegisterDesktopWindow)(THIS_ HWND) PURE;
    STDMETHOD(Unknown)(THIS_ DWORD,DWORD) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellDesktopTray_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellDesktopTray_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellDesktopTray_Release(T) (T)->lpVtbl->Release(T)
#define IShellDesktopTray_GetState(T) (T)->lpVtbl->GetState(T)
#define IShellDesktopTray_GetTrayWindow(T,a) (T)->lpVtbl->GetTrayWindow(T,a)
#define IShellDesktopTray_RegisterDesktopWindow(T,a) (T)->lpVtbl->RegisterDesktopWindow(T,a)
#define IShellDesktopTray_Unknown(T,a,b) (T)->lpVtbl->Unknown(T,a,b)
#endif

#if USE_API_SHCREATEDESKTOP != 0
#if 0
HANDLE WINAPI SHCreateDesktop(IShellDesktopTray*);
BOOL WINAPI SHDesktopMessageLoop(HANDLE);
#else
typedef HANDLE (WINAPI *PSHCreateDesktop)(IShellDesktopTray*);
static HANDLE __inline
SHCreateDesktop(IShellDesktopTray* sdt)
{
    static PSHCreateDesktop Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHELL32.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHCreateDesktop)GetProcAddress(hShlwapi, (LPCSTR)200);
        }
    }

    if (Func != NULL)
    {
        return Func(sdt);
    }

    MessageBox(NULL, TEXT("SHCreateDesktop not available"), NULL, 0);
    return NULL;
}

typedef BOOL (WINAPI *PSHDesktopMessageLoop)(HANDLE);
static BOOL __inline
SHDesktopMessageLoop(IN HANDLE hDesktop)
{
    static PSHDesktopMessageLoop Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHELL32.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHDesktopMessageLoop)GetProcAddress(hShlwapi, (LPCSTR)201);
        }
    }

    if (Func != NULL)
    {
        return Func(hDesktop);
    }

    MessageBox(NULL, TEXT("SHDesktopMessageLoop not available"), NULL, 0);
    return FALSE;
}
#endif

#endif /* USE_API_SHCREATEDESKTOP */

#define WM_GETISHELLBROWSER (WM_USER+7)
BOOL WINAPI SetShellWindow(HWND);
BOOL WINAPI SetShellWindowEx(HWND, HWND);
BOOL WINAPI RegisterShellHook(HWND, DWORD);
IStream* WINAPI SHGetViewStream(LPCITEMIDLIST, DWORD, LPCTSTR, LPCTSTR, LPCTSTR);
BOOL WINAPI SHIsEmptyStream(IStream*);

typedef struct tagCREATEMRULISTA
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTA, *LPCREATEMRULISTA;
typedef struct tagCREATEMRULISTW
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTW, *LPCREATEMRULISTW;

#define MRU_BINARY  0x1
#define MRU_CACHEWRITE  0x2

HANDLE WINAPI CreateMRUListW(LPCREATEMRULISTW);
HANDLE WINAPI CreateMRUListA(LPCREATEMRULISTA);
INT WINAPI AddMRUData(HANDLE,LPCVOID,DWORD);
INT WINAPI FindMRUData(HANDLE,LPCVOID,DWORD,LPINT);
VOID WINAPI FreeMRUList(HANDLE);

#define DC_NOSENDMSG 0x2000
BOOL WINAPI DrawCaptionTempA(HWND,HDC,const RECT*,HFONT,HICON,LPCSTR,UINT);
BOOL WINAPI DrawCaptionTempW(HWND,HDC,const RECT*,HFONT,HICON,LPCWSTR,UINT);

#ifdef UNICODE
typedef CREATEMRULISTW CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListW
#define DrawCaptionTemp DrawCaptionTempW
#else
typedef CREATEMRULISTA CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListA
#define DrawCaptionTemp DrawCaptionTempA
#endif

DEFINE_GUID(CLSID_RebarBandSite, 0xECD4FC4D, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
DEFINE_GUID(IID_IDeskBand, 0xEB0FE172, 0x1A3A, 0x11D0, 0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);

HRESULT WINAPI SHInvokeDefaultCommand(HWND,IShellFolder*,LPCITEMIDLIST);

HRESULT WINAPI SHPropertyBag_ReadPOINTL(IPropertyBag*,LPCWSTR,POINTL*);

#if 0
HRESULT WINAPI SHGetPerScreenResName(OUT LPWSTR lpResName,
                                     IN INT cchResName,
                                     IN DWORD dwReserved);
#else
typedef HRESULT (WINAPI *PSHGetPerScreenResName)(LPWSTR,INT,DWORD);
static HRESULT __inline
SHGetPerScreenResName(OUT LPWSTR lpResName,
                      IN INT cchResName,
                      IN DWORD dwReserved  OPTIONAL)
{
    static PSHGetPerScreenResName Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHLWAPI.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHGetPerScreenResName)GetProcAddress(hShlwapi, (LPCSTR)533);
        }
    }

    if (Func != NULL)
    {
        return Func(lpResName, cchResName, dwReserved);
    }

    MessageBox(NULL, TEXT("SHGetPerScreenResName not available"), NULL, 0);
    return E_NOTIMPL;
}
#endif

#if 0
HRESULT WINAPI SHPropertyBag_ReadStream(IPropertyBag*,LPCWSTR,IStream**);
#else
typedef HRESULT (WINAPI *PSHPropertyBag_ReadStream)(IPropertyBag*,LPCWSTR,IStream**);
static HRESULT __inline
SHPropertyBag_ReadStream(IN IPropertyBag *ppb,
                         IN LPCWSTR pszPropName,
                         OUT IStream **ppStream)
{
    static PSHPropertyBag_ReadStream Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHLWAPI.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHPropertyBag_ReadStream)GetProcAddress(hShlwapi, (LPCSTR)531);
        }
    }

    if (Func != NULL)
    {
        return Func(ppb, pszPropName, ppStream);
    }

    MessageBox(NULL, TEXT("SHPropertyBag_ReadStream not available"), NULL, 0);
    return E_NOTIMPL;
}
#endif

#endif /* _EXPLORER_UNDOC__H */
