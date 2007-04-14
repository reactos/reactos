

#ifndef __REQUIRED_RPCNDR_H_VERSION__
  #define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
  #error this stub requires an updated version of <rpcndr.h>
#endif 

#ifndef COM_NO_WINDOWS_H
  #include "windows.h"
  #include "ole2.h"
#endif

#ifndef __dmodshow_h__
  #define __dmodshow_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
  #pragma once
#endif

#ifndef __IDMOWrapperFilter_FWD_DEFINED__
  #define __IDMOWrapperFilter_FWD_DEFINED__
  typedef interface IDMOWrapperFilter IDMOWrapperFilter;
#endif

#include "unknwn.h"
#include "objidl.h"
#include "mediaobj.h"

#ifdef __cplusplus
extern "C"{
#endif 

DEFINE_GUID(CLSID_DMOWrapperFilter, 0x94297043,0xBD82,0x4DFD,0xB0,0xDE,0x81,0x77,0x73,0x9C,0x6D,0x20);
DEFINE_GUID(CLSID_DMOFilterCategory,0xBCD5796C,0xBD52,0x4D30,0xAB,0x76,0x70,0xF9,0x75,0xB8,0x91,0x99);

extern RPC_IF_HANDLE __MIDL_itf_dmodshow_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dmodshow_0000_v0_0_s_ifspec;

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IDMOWrapperFilter_INTERFACE_DEFINED__
  #define __IDMOWrapperFilter_INTERFACE_DEFINED__
  EXTERN_C const IID IID_IDMOWrapperFilter;
  #if defined(__cplusplus) && !defined(CINTERFACE)
    MIDL_INTERFACE("52D6F586-9F0F-4824-8FC8-E32CA04930C2")
    IDMOWrapperFilter : public IUnknown
    {
      public:
      virtual HRESULT STDMETHODCALLTYPE Init(REFCLSID clsidDMO, REFCLSID catDMO) = 0;
    };
  #else
    typedef struct IDMOWrapperFilterVtbl
    {
      BEGIN_INTERFACE
      HRESULT ( STDMETHODCALLTYPE *QueryInterface )(IDMOWrapperFilter * This, REFIID riid, void **ppvObject);
      ULONG ( STDMETHODCALLTYPE *AddRef )(IDMOWrapperFilter * This);
      ULONG ( STDMETHODCALLTYPE *Release )(IDMOWrapperFilter * This);
      HRESULT ( STDMETHODCALLTYPE *Init )(IDMOWrapperFilter * This, REFCLSID clsidDMO, REFCLSID catDMO);
      END_INTERFACE
    } IDMOWrapperFilterVtbl;

    interface IDMOWrapperFilter
    {
      CONST_VTBL struct IDMOWrapperFilterVtbl *lpVtbl;
    };

    #ifdef COBJMACROS
      #define IDMOWrapperFilter_QueryInterface(This,riid,ppvObject) (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
      #define IDMOWrapperFilter_AddRef(This) (This)->lpVtbl -> AddRef(This)
      #define IDMOWrapperFilter_Release(This) (This)->lpVtbl -> Release(This)
      #define IDMOWrapperFilter_Init(This,clsidDMO,catDMO) (This)->lpVtbl -> Init(This,clsidDMO,catDMO)
    #endif
  #endif

HRESULT STDMETHODCALLTYPE
IDMOWrapperFilter_Init_Proxy( 
  IDMOWrapperFilter * This,
  REFCLSID clsidDMO,
  REFCLSID catDMO);


void __RPC_STUB
  IDMOWrapperFilter_Init_Stub(
  IRpcStubBuffer *This,
  IRpcChannelBuffer *_pRpcChannelBuffer,
  PRPC_MESSAGE _pRpcMessage,
  DWORD *_pdwStubPhase);
#endif

#ifdef __cplusplus
}
#endif
#endif
