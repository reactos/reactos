#ifndef __WINE_ICOM_H
#define __WINE_ICOM_H

//#if defined(ICOM_MSVTABLE_COMPAT) && (!defined(__cplusplus) || defined(CINTERFACE))
//#define ICOM_MSVTABLE_COMPAT_FIELDS long dummyRTTI1,dummyRTTI2;
//#define ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE 0,0,
//#else
# define ICOM_MSVTABLE_COMPAT_FIELDS
# define ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
//#endif

#define ICOM_DEFINE(iface,ibase) DECLARE_INTERFACE_(iface,ibase) { iface##_METHODS };
#define ICOM_VTABLE(iface)       iface##Vtbl
#define ICOM_VFIELD(iface)       ICOM_VTABLE(iface)* lpVtbl
#define ICOM_THIS(impl,iface)    impl* const This=(impl*)(iface)
#define ICOM_THIS_MULTI(impl,field,iface)  impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#define IUnknown_METHODS \
    ICOM_MSVTABLE_COMPAT_FIELDS \
    /*** IUnknown methods ***/ \
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE;

#endif /* __WINE_ICOM_H */
