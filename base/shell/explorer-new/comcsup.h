#pragma once

/*
 ******************************************************************************
 * This header is for easier generation of IUnknown interfaces for inherited  *
 * classes and for casts from the interface to the implementation and vice    *
 * versa.                                                                     *
 ******************************************************************************
 */

/* Generates a Iiface::AddRef() method that forwards to Iimpl::AddRef() */
#define METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(iface,impl) impl##Impl_##iface##_AddRef
#define METHOD_IUNKNOWN_INHERITED_ADDREF(iface,impl) \
static ULONG STDMETHODCALLTYPE \
impl##Impl_##iface##_AddRef(IN OUT iface *ifc) { \
    impl##Impl *This = impl##Impl_from_##iface (ifc); \
    impl *baseiface = impl##_from_##impl##Impl(This); \
    return impl##Impl_AddRef(baseiface); \
}

/* Generates a Iiface::Release() method that forwards to Iimpl::Release() */
#define METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(iface,impl) impl##Impl_##iface##_Release
#define METHOD_IUNKNOWN_INHERITED_RELEASE(iface,impl) \
static ULONG STDMETHODCALLTYPE \
impl##Impl_##iface##_Release(IN OUT iface *ifc) { \
    impl##Impl *This = impl##Impl_from_##iface (ifc); \
    impl *baseiface = impl##_from_##impl##Impl(This); \
    return impl##Impl_AddRef(baseiface); \
}

/* Generates a Iiface::QueryInterface() method that forwards to Iimpl::QueryInterface() */
#define METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(iface,impl) impl##Impl_##iface##_QueryInterface
#define METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(iface,impl) \
static HRESULT STDMETHODCALLTYPE \
impl##Impl_##iface##_QueryInterface(IN OUT iface *ifc, IN REFIID riid, OUT VOID **ppvObject) { \
    impl##Impl *This = impl##Impl_from_##iface (ifc); \
    impl *baseiface = impl##_from_##impl##Impl(This); \
    return impl##Impl_QueryInterface(baseiface, riid, ppvObject); \
}

/* Generates a Ixxx_from_IxxxImpl() and a IxxxImpl_from_Ixxx() inline function */
#define IMPL_CASTS(iface,impl,vtbl) \
static __inline iface * \
iface##_from_##impl##Impl (impl##Impl *This) { \
    return (iface *)&This->vtbl; \
} \
static __inline impl##Impl * \
impl##Impl_from_##iface (iface *ifc) { \
    return (impl##Impl *)((ULONG_PTR)ifc - FIELD_OFFSET(impl##Impl, vtbl)); \
}
