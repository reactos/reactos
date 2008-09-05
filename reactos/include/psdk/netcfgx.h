#ifndef __NETCFGX_H__
#define __NETCFGX_H__

#undef  INTERFACE
#define INTERFACE   INetCfgLock
DECLARE_INTERFACE_(INetCfgLock, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,AcquireWriteLock)(THIS_ DWORD cmsTimeout, LPCWSTR pszwClientDescription, LPWSTR *ppszwClientDescription) PURE;
    STDMETHOD_(HRESULT,ReleaseWriteLock)(THIS) PURE;
    STDMETHOD_(HRESULT,IsWriteLocked)(THIS_ LPWSTR *ppszwClientDescription) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_INetCfgLock;

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgLock_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgLock_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetCfgLock_Release(p)                 (p)->lpVtbl->Release(p)
#define INetCfgLock_AcquireWriteLock(p,a,b,c)  (p)->lpVtbl->AcquireWriteLock(p,a,b,c)
#define INetCfgLock_ReleaseWriteLock(p)        (p)->lpVtbl->ReleaseWriteLock(p)
#define INetCfgLock_IsWriteLocked(p,a)         (p)->lpVtbl->IsWriteLocked(p,a)
#endif

typedef enum 
{
    NCRP_QUERY_PROPERTY_UI  = 1,
    NCRP_SHOW_PROPERTY_UI   = 2
}NCRP_FLAGS;


#undef  INTERFACE
#define INTERFACE   INetCfgComponent
DECLARE_INTERFACE_(INetCfgComponent, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,GetDisplayName)(THIS_ LPWSTR *ppszwDisplayName) PURE;
    STDMETHOD_(HRESULT,SetDisplayName)(THIS_ LPWSTR *ppszwDisplayName) PURE;
    STDMETHOD_(HRESULT,GetHelpText)(THIS_ LPWSTR *pszwHelpText) PURE;
    STDMETHOD_(HRESULT,GetId)(THIS_ LPWSTR *ppszwId) PURE;
    STDMETHOD_(HRESULT,GetCharacteristics)(THIS_ LPDWORD pdwCharacteristics) PURE;
    STDMETHOD_(HRESULT,GetInstanceGuid)(THIS_ GUID *pGuid) PURE;
    STDMETHOD_(HRESULT,GetPnpDevNodeId)(THIS_  LPWSTR *ppszwDevNodeId) PURE;
    STDMETHOD_(HRESULT,GetClassGuid)(THIS_  GUID *pGuid) PURE;
    STDMETHOD_(HRESULT,GetBindName)(THIS_ LPWSTR *ppszwBindName) PURE;
    STDMETHOD_(HRESULT,GetDeviceStatus)(THIS_ ULONG *pulStatus) PURE;
    STDMETHOD_(HRESULT,OpenParamKey)(THIS_ HKEY *phkey) PURE;
    STDMETHOD_(HRESULT,RaisePropertyUi)(THIS_ HWND hwndParent, DWORD dwFlags, IUnknown *punkContext) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponent_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponent_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetCfgComponent_Release(p)                 (p)->lpVtbl->Release(p)
#define INetCfgComponent_GetDisplayName(p,a)        (p)->lpVtbl->GetDisplayName(p,a)
#define INetCfgComponent_SetDisplayName(p,a)        (p)->lpVtbl->SetDisplayName(p,a)
#define INetCfgComponent_GetHelpText(p,a)           (p)->lpVtbl->GetHelpText(p,a)
#define INetCfgComponent_GetId(p,a)                 (p)->lpVtbl->GetId(p,a)
#define INetCfgComponent_GetCharacteristics(p,a)    (p)->lpVtbl->GetCharacteristics(p,a)
#define INetCfgComponent_GetInstanceGuid(p,a)       (p)->lpVtbl->GetInstanceGuid(p,a)
#define INetCfgComponent_GetPnpDevNodeId(p,a)       (p)->lpVtbl->GetPnpDevNodeId(p,a)
#define INetCfgComponent_GetClassGuid(p,a)          (p)->lpVtbl->GetClassGuid(p,a)
#define INetCfgComponent_GetBindName(p,a)           (p)->lpVtbl->GetBindName(p,a)
#define INetCfgComponent_GetDeviceStatus(p,a)       (p)->lpVtbl->GetDeviceStatus(p,a)
#define INetCfgComponent_OpenParamKey(p,a)          (p)->lpVtbl->OpenParamKey(p,a)
#define INetCfgComponent_RaisePropertyUi(p,a,b,c)   (p)->lpVtbl->RaisePropertyUi(p,a,b,c)
#endif

#undef  INTERFACE
#define INTERFACE   IEnumNetCfgComponent
DECLARE_INTERFACE_(IEnumNetCfgComponent, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Next)(THIS_ ULONG celt, INetCfgComponent **rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD_(HRESULT,Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD_(HRESULT,Reset) (THIS) PURE;
    STDMETHOD_(HRESULT,Clone) (THIS_ IEnumNetCfgComponent **ppenum) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumNetCfgComponent_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumNetCfgComponent_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IEnumNetCfgComponent_Release(p)                 (p)->lpVtbl->Release(p)
#define IEnumNetCfgComponent_Next(p,a,b,c)              (p)->lpVtbl->Next(p,a,b,c)
#define IEnumNetCfgComponent_Skip(p,a)                  (p)->lpVtbl->Skip(p,a)
#define IEnumNetCfgComponent_Reset(p)                   (p)->lpVtbl->Reset(p)
#define IEnumNetCfgComponent_Clone(p,a)                 (p)->lpVtbl->Clone(p,a)
#endif


#undef  INTERFACE
#define INTERFACE   INetCfg
DECLARE_INTERFACE_(INetCfg, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Initialize) (THIS_ PVOID pvReserved) PURE;
    STDMETHOD_(HRESULT,Uninitialize) (THIS) PURE;
    STDMETHOD_(HRESULT,Apply) (THIS) PURE;
    STDMETHOD_(HRESULT,Cancel) (THIS) PURE;
    STDMETHOD_(HRESULT,EnumComponents) (THIS_ const GUID *pguidClass, IEnumNetCfgComponent **ppenumComponent) PURE;
    STDMETHOD_(HRESULT,FindComponent) (THIS_ LPCWSTR pszwInfId, IEnumNetCfgComponent **ppenumComponent) PURE;
    STDMETHOD_(HRESULT,QueryNetCfgClass) (THIS_ const GUID *pguidClass, REFIID riid, void **ppvObject) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfg_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfg_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetCfg_Release(p)                 (p)->lpVtbl->Release(p)
#define INetCfg_Initialize(p,a)            (p)->lpVtbl->Initialize(p,a)
#define INetCfg_Uninitialize(p)            (p)->lpVtbl->Uninitialize(p)
#define INetCfg_Apply(p)                   (p)->lpVtbl->Apply(p)
#define INetCfg_Cancel(p)                  (p)->lpVtbl->Cancel(p)
#define INetCfg_EnumComponents(p,a,b)      (p)->lpVtbl->EnumComponents(p,a,b)
#define INetCfg_FindComponent(p,a,b)       (p)->lpVtbl->FindComponent(p,a,b)
#define INetCfg_QueryNetCfgClass(p,a,b,c)  (p)->lpVtbl->QueryNetCfgClass(p,a,b,c)
#endif

EXTERN_C const GUID CLSID_CNetCfg;
EXTERN_C const IID IID_INetCfg;

#endif
