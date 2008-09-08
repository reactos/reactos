#ifndef _NETCFGN_H__
#define _NETCFGN_H__

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
#define INetLanConnectionUiInfo_GetDeviceGuid(p,a)                 (p)->lpVtbl->GetDeviceGuid(p,a)
#endif

EXTERN_C const IID IID_INetLanConnectionUiInfo;

#endif
