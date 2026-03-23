#ifndef _NETCFGN_H__
#define _NETCFGN_H__

typedef enum
{
    NCRL_NDIS   = 1,
    NCRL_TDI    = 2
}NCPNP_RECONFIG_LAYER;

#undef  INTERFACE
#define INTERFACE   INetCfgPnpReconfigCallback
DECLARE_INTERFACE_(INetCfgPnpReconfigCallback, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,SendPnpReconfig)(THIS_ NCPNP_RECONFIG_LAYER Layer, LPCWSTR pszwUpper, LPCWSTR pszwLower, PVOID pvData, DWORD dwSizeOfData) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgPnpReconfigCallback_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgPnpReconfigCallback_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define INetCfgPnpReconfigCallback_Release(p)                        (p)->lpVtbl->Release(p)
#define INetCfgPnpReconfigCallback_SendPnpReconfig(p,a,b,c,d,e)      (p)->lpVtbl->SendPnpReconfig(p,a,b,c,d,e)
#endif

EXTERN_C const IID IID_INetCfgPnpReconfigCallback;


#undef  INTERFACE
#define INTERFACE   INetCfgComponentControl
DECLARE_INTERFACE_(INetCfgComponentControl, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Initialize) (THIS_ INetCfgComponent *pIComp, INetCfg *pINetCfg, BOOL fInstalling) PURE;
    STDMETHOD_(HRESULT,ApplyRegistryChanges) (THIS) PURE;
    STDMETHOD_(HRESULT,ApplyPnpChanges) (THIS_ INetCfgPnpReconfigCallback *pICallback) PURE;
    STDMETHOD_(HRESULT,CancelChanges) (THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentControl_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentControl_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define INetCfgComponentControl_Release(p)                        (p)->lpVtbl->Release(p)
#define INetCfgComponentControl_Initialize(p,a,b,c)               (p)->lpVtbl->Initialize(p,a,b,c)
#define INetCfgComponentControl_ApplyRegistryChanges(p)           (p)->lpVtbl->ApplyRegistryChanges(p)
#define INetCfgComponentControl_ApplyPnpChanges(p,a)              (p)->lpVtbl->ApplyRegistryChanges(p,a)
#define INetCfgComponentControl_CancelChanges(p)                  (p)->lpVtbl->CancelChanges(p)
#endif

EXTERN_C const IID IID_INetCfgComponentControl;

#undef  INTERFACE
#define INTERFACE   INetCfgComponentPropertyUi
DECLARE_INTERFACE_(INetCfgComponentPropertyUi, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,QueryPropertyUi)(THIS_ IUnknown *pUnkReserved) PURE;
    STDMETHOD_(HRESULT,SetContext)(THIS_ IUnknown *pUnkReserved) PURE;
    STDMETHOD_(HRESULT,MergePropPages)(THIS_ DWORD *pdwDefPages, BYTE **pahpspPrivate, UINT *pcPages, HWND hwndParent, LPCWSTR *pszStartPage) PURE;
    STDMETHOD_(HRESULT,ValidateProperties)(THIS_ HWND hwndSheet) PURE;
    STDMETHOD_(HRESULT,ApplyProperties)(THIS) PURE;
    STDMETHOD_(HRESULT,CancelProperties)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentPropertyUi_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentPropertyUi_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define INetCfgComponentPropertyUi_Release(p)                        (p)->lpVtbl->Release(p)
#define INetCfgComponentPropertyUi_QueryPropertyUi(p,a)              (p)->lpVtbl->QueryPropertyUi(p,a)
#define INetCfgComponentPropertyUi_SetContext(p,a)                   (p)->lpVtbl->SetContext(p,a)
#define INetCfgComponentPropertyUi_MergePropPages(p,a,b,c,d,e)       (p)->lpVtbl->MergePropPages(p,a,b,c,d,e)
#define INetCfgComponentPropertyUi_ValidateProperties(p,a)           (p)->lpVtbl->ValidateProperties(p,a)
#define INetCfgComponentPropertyUi_ApplyProperties(p)                (p)->lpVtbl->ApplyProperties(p)
#define INetCfgComponentPropertyUi_CancelProperties(p)               (p)->lpVtbl->CancelProperties(p)
#endif

EXTERN_C const IID IID_INetCfgComponentPropertyUi;

#undef  INTERFACE
#define INTERFACE   INetCfgComponentNotifyBinding
DECLARE_INTERFACE_(INetCfgComponentNotifyBinding, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,QueryBindingPath) (THIS_ DWORD dwChangeFlag, INetCfgBindingPath *pncbpItem) PURE;
    STDMETHOD_(HRESULT,NotifyBindingPath) (THIS_ DWORD dwChangeFlag, INetCfgBindingPath *pncbpItem) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentNotifyBinding_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentNotifyBinding_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define INetCfgComponentNotifyBinding_Release(p)                  (p)->lpVtbl->Release(p)
#define INetCfgComponentNotifyBinding_QueryBindingPath(p,a,b)     (p)->lpVtbl->QueryBindingPath(p,a,b)
#define INetCfgComponentNotifyBinding_NotifyBindingPath(p,a,b)    (p)->lpVtbl->NotifyBindingPath(p,a,b)
#endif

EXTERN_C const IID IID_INetCfgComponentNotifyBinding;

#undef  INTERFACE
#define INTERFACE   INetCfgComponentNotifyGlobal
DECLARE_INTERFACE_(INetCfgComponentNotifyGlobal, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,GetSupportedNotifications) (THIS_ DWORD *pdwNotifications) PURE;
    STDMETHOD_(HRESULT,SysQueryBindingPath) (THIS_ DWORD dwChangeFlag, INetCfgBindingPath *pncbpItem) PURE;
    STDMETHOD_(HRESULT,SysNotifyBindingPath) (THIS_ DWORD dwChangeFlag, INetCfgBindingPath *pncbpItem) PURE;
    STDMETHOD_(HRESULT,SysNotifyComponent) (THIS_ DWORD dwChangeFlag, INetCfgComponent *pnccItem) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentNotifyGlobal_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentNotifyGlobal_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define INetCfgComponentNotifyGlobal_Release(p)                     (p)->lpVtbl->Release(p)
#define INetCfgComponentNotifyGlobal_GetSupportedNotifications(p,a) (p)->lpVtbl->GetSupportedNotifications(p,a)
#define INetCfgComponentNotifyGlobal_SysQueryBindingPath(p,a,b)     (p)->lpVtbl->SysQueryBindingPath(p,a,b)
#define INetCfgComponentNotifyGlobal_SysNotifyBindingPath(p,a,b)    (p)->lpVtbl->SysNotifyBindingPath(p,a,b)
#define INetCfgComponentNotifyGlobal_SysNotifyComponent(p,a,b)      (p)->lpVtbl->SysNotifyComponent(p,a,b)
#endif

EXTERN_C const IID IID_INetCfgComponentNotifyGlobal;

#undef  INTERFACE
#define INTERFACE   INetCfgComponentSetup
DECLARE_INTERFACE_(INetCfgComponentSetup, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Install) (THIS_ DWORD dwSetupFlags) PURE;
    STDMETHOD_(HRESULT,Upgrade) (THIS_ DWORD dwSetupFlags, DWORD dwUpgradeFromBuildNo) PURE;
    STDMETHOD_(HRESULT,ReadAnswerFile) (THIS_ LPCWSTR pszwAnswerFile, LPCWSTR pszwAnswerSections) PURE;
    STDMETHOD_(HRESULT,Removing) (THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentControl_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentControl_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define INetCfgComponentControl_Release(p)                        (p)->lpVtbl->Release(p)
#define INetCfgComponentControl_Install(p,a)                      (p)->lpVtbl->Initialize(p,a)
#define INetCfgComponentControl_Upgrade(p,a,b)                    (p)->lpVtbl->Upgrade(p,a,b)
#define INetCfgComponentControl_ReadAnswerFile(p,a,b)             (p)->lpVtbl->ReadAnswerFile(p,a,b)
#define INetCfgComponentControl_Removing(p)                       (p)->lpVtbl->Removing(p)
#endif

EXTERN_C const IID IID_INetCfgComponentSetup;

#undef  INTERFACE
#define INTERFACE   INetLanConnectionUiInfo
DECLARE_INTERFACE_(INetLanConnectionUiInfo, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,GetDeviceGuid)(THIS_ GUID *pguid) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetLanConnectionUiInfo_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define INetLanConnectionUiInfo_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define INetLanConnectionUiInfo_Release(p)                        (p)->lpVtbl->Release(p)
#define INetLanConnectionUiInfo_GetDeviceGuid(p,a)                (p)->lpVtbl->GetDeviceGuid(p,a)
#endif

EXTERN_C const IID IID_INetLanConnectionUiInfo;

#endif
