#include_next <unknwn.h>

#ifndef __WINE_UNKNWN_H
#define __WINE_UNKNWN_H

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IUnknown_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IUnknown_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IUnknown_Release(p) (p)->lpVtbl->Release(p)

DEFINE_GUID(IID_IClassFactory, 0x00000001, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

/*** IUnknown methods ***/
#define IClassFactory_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IClassFactory_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IClassFactory_Release(p) (p)->lpVtbl->Release(p)
/*** IClassFactory methods ***/
#define IClassFactory_CreateInstance(p,a,b,c) (p)->lpVtbl->CreateInstance(p,a,b,c)
#define IClassFactory_LockServer(p,a) (p)->lpVtbl->LockServer(p,a)

#define IUnknown_METHODS \
    /*** IUnknown methods ***/ \
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE;

#endif  /* __WINE_UNKNWN_H */
