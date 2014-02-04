
#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <uxtheme.h>
#include <strsafe.h>

#include <atlbase.h>
#include <atlcom.h>
#include <wine/debug.h>

#define shell32_hInstance 0
#define SMC_EXEC 4
extern "C" INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);
extern "C" HRESULT CMenuSite_Constructor(REFIID riid, void **ppv);

#define INTERFACE IExplorerHostCreator
DECLARE_INTERFACE_(IExplorerHostCreator,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IExplorerHostCreator ***/
    STDMETHOD_(HRESULT,CreateHost)(/*THIS,*/ const GUID* guid) PURE;
    STDMETHOD_(HRESULT,RunHost)(THIS) PURE;
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define IExplorerHostCreator_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define IExplorerHostCreator_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define IExplorerHostCreator_Release(p)                        (p)->lpVtbl->Release(p)
/*** IExplorerHostCreator methods ***/
#define IExplorerHostCreator_CreateHost(p,a)                   (p)->lpVtbl->CreateHost(p,a)
#define IExplorerHostCreator_RunHost(p)                          (p)->lpVtbl->RunHost(p)
#endif
