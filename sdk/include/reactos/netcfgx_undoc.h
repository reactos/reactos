#ifndef __NETCFGX_UNDOC_H__
#define __NETCFGX_UNDOC_H__

#undef  INTERFACE
#define INTERFACE INetCfgComponentPrivate
DECLARE_INTERFACE_(INetCfgComponentPrivate, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Unknown1)(THIS_ DWORD dwParam1, DWORD dwParam2) PURE;
    /* ??? */
};
#undef INTERFACE

EXTERN_C const IID IID_INetCfgComponentPrivate;

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetCfgComponentPrivate_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define INetCfgComponentPrivate_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define INetCfgComponentPrivate_Release(p)            (p)->lpVtbl->Release(p)
#define INetCfgComponentPrivate_Unknown1(p,a,b)       (p)->lpVtbl->Unknown1(p,a,b)
/* ??? */
#endif

#endif /* __NETCFGX_UNDOC_H__ */
