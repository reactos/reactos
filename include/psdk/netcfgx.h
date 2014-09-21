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
    STDMETHOD_(HRESULT,SetDisplayName)(THIS_ LPCWSTR ppszwDisplayName) PURE;
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

EXTERN_C const IID IID_INetCfgComponent;

#undef  INTERFACE
#define INTERFACE   INetCfgBindingInterface
DECLARE_INTERFACE_(INetCfgBindingInterface, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,GetName)(THIS_ LPWSTR *ppszwInterfaceName) PURE;
    STDMETHOD_(HRESULT,GetUpperComponent)(THIS_ INetCfgComponent **ppnccItem) PURE;
    STDMETHOD_(HRESULT,GetLowerComponent)(THIS_ INetCfgComponent **ppnccItem) PURE;
};
#undef INTERFACE


#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgBindingInterface_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgBindingInterface_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetCfgBindingInterface_Release(p)                 (p)->lpVtbl->Release(p)
#define INetCfgBindingInterface_GetName(p,a)               (p)->lpVtbl->GetName(p)
#define INetCfgBindingInterface_GetUpperComponent(p,a)     (p)->lpVtbl->GetUpperComponent(p)
#define INetCfgBindingInterface_GetLowerComponent(p,a)     (p)->lpVtbl->GetLowerComponent(p)
#endif

EXTERN_C const IID IID_INetCfgBindingInterface;


#undef  INTERFACE
#define INTERFACE   IEnumNetCfgBindingInterface
DECLARE_INTERFACE_(IEnumNetCfgBindingInterface, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Next)(THIS_ ULONG celt, INetCfgBindingInterface **rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD_(HRESULT,Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD_(HRESULT,Reset) (THIS) PURE;
    STDMETHOD_(HRESULT,Clone) (THIS_ IEnumNetCfgBindingInterface **ppenum) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumNetCfgBindingInterface_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumNetCfgBindingInterface_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IEnumNetCfgBindingInterface_Release(p)                    (p)->lpVtbl->Release(p)
#define IEnumNetCfgBindingInterface_Next(p,a,b,c)                   (p)->lpVtbl->Next(p,a,b,c)
#define IEnumNetCfgBindingInterface_Skip(p,a)                       (p)->lpVtbl->Skip(p,a)
#define IEnumNetCfgBindingInterface_Reset(p)                        (p)->lpVtbl->Reset(p)
#define IEnumNetCfgBindingInterface_Clone(p,a)                      (p)->lpVtbl->Clone(p,a)
#endif

EXTERN_C const IID IID_IEnumNetCfgBindingInterface;

#undef  INTERFACE
#define INTERFACE   INetCfgBindingPath
DECLARE_INTERFACE_(INetCfgBindingPath, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,IsSamePathAs)(THIS_ INetCfgBindingPath *pPath) PURE;
    STDMETHOD_(HRESULT,IsSubPathOf)(THIS_ INetCfgBindingPath *pPath) PURE;
    STDMETHOD_(HRESULT,IsEnabled) (THIS) PURE;
    STDMETHOD_(HRESULT,Enable) (THIS_ BOOL fEnable) PURE;
    STDMETHOD_(HRESULT,GetPathToken) (THIS_ LPWSTR *ppszwPathToken) PURE;
    STDMETHOD_(HRESULT,GetOwner)(THIS_ INetCfgComponent **ppComponent) PURE;
    STDMETHOD_(HRESULT,GetDepth)(THIS_ ULONG *pcInterfaces) PURE;
    STDMETHOD_(HRESULT,EnumBindingInterfaces)(THIS_ IEnumNetCfgBindingInterface **ppenumInterface) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgBindingPath_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgBindingPath_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define INetCfgBindingPath_Release(p)                    (p)->lpVtbl->Release(p)
#define INetCfgBindingPath_IsSamePathAs(p,a)             (p)->lpVtbl->IsSamePathAs(p,a)
#define INetCfgBindingPath_IsSubPathOf(p,a)              (p)->lpVtbl->IsSubPathOf(p,a)
#define INetCfgBindingPath_IsEnabled(p)                  (p)->lpVtbl->IsEnabled(p)
#define INetCfgBindingPath_Enable(p,a)                   (p)->lpVtbl->Enable(p,a)
#define INetCfgBindingPath_GetPathToken(p,a)             (p)->lpVtbl->GetPathToken(p,a)
#define INetCfgBindingPath_GetOwner(p,a)                 (p)->lpVtbl->GetOwner(p,a)
#define INetCfgBindingPath_GetDepth(p,a)                 (p)->lpVtbl->GetDepth(p,a)
#define INetCfgBindingPath_EnumBindingInterfaces(p,a)    (p)->lpVtbl->EnumBindingInterfaces(p,a)
#endif

EXTERN_C const IID IID_INetCfgBindingPath;

#undef  INTERFACE
#define INTERFACE   IEnumNetCfgBindingPath
DECLARE_INTERFACE_(IEnumNetCfgBindingPath, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Next)(THIS_ ULONG celt, INetCfgBindingPath **rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD_(HRESULT,Skip)(THIS_ ULONG celt)  PURE;
    STDMETHOD_(HRESULT,Reset)(THIS)  PURE;
    STDMETHOD_(HRESULT,Clone)(THIS_ IEnumNetCfgBindingPath **ppenum)  PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IEnumNetCfgBindingPath_QueryInterface(p,a,b)           (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumNetCfgBindingPath_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IEnumNetCfgBindingPath_Release(p)                      (p)->lpVtbl->Release(p)
#define IEnumNetCfgBindingPath_Next(p,a,b,c)                   (p)->lpVtbl->Next(p,a,b,c)
#define IEnumNetCfgBindingPath_Skip(p,a)                       (p)->lpVtbl->Skip(p,a)
#define IEnumNetCfgBindingPath_Reset(p)                        (p)->lpVtbl->Reset(p)
#define IEnumNetCfgBindingPath_Clone(p,a)                      (p)->lpVtbl->Clone(p,a)
#endif

EXTERN_C const IID IID_IEnumNetCfgBindingPath;

typedef enum
{
    NCF_LOWER = 0x1,
    NCF_UPPER = 0x2
}SUPPORTS_BINDING_INTERFACE_FLAGS;

typedef enum
{
    EBP_ABOVE   = 0x1,
    EBP_BELOW   = 0x2
}ENUM_BINDING_PATHS_FLAGS;

#undef  INTERFACE
#define INTERFACE   INetCfgComponentBindings
DECLARE_INTERFACE_(INetCfgComponentBindings, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,BindTo)(THIS_ INetCfgComponent *pnccItem) PURE;
    STDMETHOD_(HRESULT,UnbindFrom)(THIS_ INetCfgComponent *pnccItem) PURE;
    STDMETHOD_(HRESULT,SupportsBindingInterface)(THIS_ DWORD dwFlags, LPCWSTR pszwInterfaceName) PURE;
    STDMETHOD_(HRESULT,IsBoundTo)(THIS_ INetCfgComponent *pnccItem) PURE;
    STDMETHOD_(HRESULT,IsBindableTo)(THIS_ INetCfgComponent *pnccItem) PURE;
    STDMETHOD_(HRESULT,EnumBindingPaths)(THIS_ DWORD dwFlags, IEnumNetCfgBindingPath **ppIEnum) PURE;
    STDMETHOD_(HRESULT,MoveBefore)(THIS_ DWORD dwFlags, INetCfgBindingPath *pncbItemSrc, INetCfgBindingPath *pncbItemDest) PURE;
    STDMETHOD_(HRESULT,MoveAfter)(THIS_ DWORD dwFlags, INetCfgBindingPath *pncbItemSrc, INetCfgBindingPath *pncbItemDest) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentBindings_QueryInterface(p,a,b)           (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentBindings_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define INetCfgComponentBindings_Release(p)                      (p)->lpVtbl->Release(p)
#define INetCfgComponentBindings_BindTo(p,a)                     (p)->lpVtbl->BindTo(p,a)
#define INetCfgComponentBindings_UnbindFrom(p,a)                 (p)->lpVtbl->UnbindFrom(p,a)
#define INetCfgComponentBindings_SupportsBindingInterface(p,a,b) (p)->lpVtbl->UnbindFrom(p,a,b)
#define INetCfgComponentBindings_IsBoundTo(p,a)                  (p)->lpVtbl->IsBoundTo(p,a)
#define INetCfgComponentBindings_IsBindableTo(p,a)               (p)->lpVtbl->IsBindableTo(p,a)
#define INetCfgComponentBindings_EnumBindingPaths(p,a,b)         (p)->lpVtbl->EnumBindingPaths(p,a,b)
#define INetCfgComponentBindings_MoveBefore(p,a,b,c)             (p)->lpVtbl->MoveBefore(p,a,b,c)
#define INetCfgComponentBindings_MoveAfter(p,a,b,c)              (p)->lpVtbl->MoveAfter(p,a,b,c)
#endif

EXTERN_C const IID IID_INetCfgComponentBindings;

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

EXTERN_C const IID IID_IEnumNetCfgComponent;

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
    STDMETHOD_(HRESULT,FindComponent) (THIS_ LPCWSTR pszwInfId, INetCfgComponent **ppenumComponent) PURE;
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

#define NETCFG_E_ALREADY_INITIALIZED                 0x8004A020
#define NETCFG_E_NOT_INITIALIZED                     0x8004A021
#define NETCFG_E_IN_USE                              0x8004A022
#define NETCFG_E_NO_WRITE_LOCK                       0x8004A024
#define NETCFG_E_NEED_REBOOT                         0x8004A025
#define NETCFG_E_ACTIVE_RAS_CONNECTIONS              0x8004A026
#define NETCFG_E_ADAPTER_NOT_FOUND                   0x8004A027
#define NETCFG_E_COMPONENT_REMOVED_PENDING_REBOOT    0x8004A028
#define NETCFG_E_MAX_FILTER_LIMIT                    0x8004A029
#define NETCFG_S_REBOOT                              0x8004A020
#define NETCFG_S_DISABLE_QUERY                       0x8004A022
#define NETCFG_S_STILL_REFERENCED                    0x8004A023
#define NETCFG_S_CAUSED_SETUP_CHANGE                 0x8004A024
#define NETCFG_S_COMMIT_NOW                          0x8004A025


#endif
