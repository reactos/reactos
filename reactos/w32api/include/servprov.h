#ifndef _SERVPROV_H
#define _SERVPROV_H
#define _OLEIDL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXTERN_C const IID IID_IServiceProvider;
#define INTERFACE IServiceProvider
DECLARE_INTERFACE_(IServiceProvider,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(QueryService)(THIS_ REFGUID,REFIID,void**) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IServiceProvider_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IServiceProvider_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IServiceProvider_Release(T) (T)->lpVtbl->Release(T)
#define IServiceProvider_QueryService(T,a,b,c) (T)->lpVtbl->QueryService(T,a,b,c)
#endif

#ifdef __cplusplus
}
#endif
#endif
