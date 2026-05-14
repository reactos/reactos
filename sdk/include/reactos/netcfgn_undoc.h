#ifndef __NETCFGN_UNDOC_H__
#define __NETCFGN_UNDOC_H__

typedef struct _TCPIP_PROPERTIES
{
    DWORD dwDhcp;
    PWSTR pszIpAddress;
    PWSTR pszSubnetMask;
    PWSTR pszParameters;
} TCPIP_PROPERTIES, *PTCPIP_PROPERTIES;

#undef  INTERFACE
#define INTERFACE ITcpipProperties
DECLARE_INTERFACE_(ITcpipProperties, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Unknown1)(THIS_ GUID *pAdapterName, PTCPIP_PROPERTIES *ppProperties) PURE;
    /* ??? */
};
#undef INTERFACE

EXTERN_C const IID IID_ITcpipProperties;

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ITcpipProperties_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITcpipProperties_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ITcpipProperties_Release(p)            (p)->lpVtbl->Release(p)
#define ITcpipProperties_Unknown1(p,a,b)       (p)->lpVtbl->Unknown1(p,a,b)
/* ??? */
#endif

#endif /* __NETCFGX_UNDOC_H__ */
