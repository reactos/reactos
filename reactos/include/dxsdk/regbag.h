

#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires never version of <rpcndr.h>
#endif

#ifndef COM_NO_WINDOWS_H
  #include "windows.h"
  #include "ole2.h"
#endif

#ifndef __regbag_h__
#define __regbag_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
  #pragma once
#endif

#ifndef __ICreatePropBagOnRegKey_FWD_DEFINED__
  #define __ICreatePropBagOnRegKey_FWD_DEFINED__
  typedef interface ICreatePropBagOnRegKey ICreatePropBagOnRegKey;
#endif

#include "objidl.h"
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#pragma once


extern RPC_IF_HANDLE __MIDL_itf_regbag_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_regbag_0000_v0_0_s_ifspec;

#ifndef __ICreatePropBagOnRegKey_INTERFACE_DEFINED__
  #define __ICreatePropBagOnRegKey_INTERFACE_DEFINED__
  EXTERN_C const IID IID_ICreatePropBagOnRegKey;
  #if defined(__cplusplus) && !defined(CINTERFACE)
       MIDL_INTERFACE("8A674B48-1F63-11D3-B64C-00C04F79498E")
       ICreatePropBagOnRegKey : public IUnknown
       {
          public:
          virtual HRESULT STDMETHODCALLTYPE Create(HKEY hkey, LPCOLESTR subkey, DWORD ulOptions, DWORD samDesired,
                                                   REFIID iid, LPVOID *ppBag) = 0;
    };
#else

  typedef struct ICreatePropBagOnRegKeyVtbl
  {
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(ICreatePropBagOnRegKey * This, REFIID riid, void **ppvObject);
    ULONG ( STDMETHODCALLTYPE *AddRef )(ICreatePropBagOnRegKey * This);
    ULONG ( STDMETHODCALLTYPE *Release )(ICreatePropBagOnRegKey * This);
    HRESULT ( STDMETHODCALLTYPE *Create )(ICreatePropBagOnRegKey * This, HKEY hkey, LPCOLESTR subkey,
                                          DWORD ulOptions, DWORD samDesired, REFIID iid, LPVOID *ppBag);
    END_INTERFACE
  } ICreatePropBagOnRegKeyVtbl;

  interface ICreatePropBagOnRegKey
  {
    CONST_VTBL struct ICreatePropBagOnRegKeyVtbl *lpVtbl;
  };

  #ifdef COBJMACROS
    #define ICreatePropBagOnRegKey_QueryInterface(This,riid,ppvObject) (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
    #define ICreatePropBagOnRegKey_AddRef(This) (This)->lpVtbl -> AddRef(This)
    #define ICreatePropBagOnRegKey_Release(This) (This)->lpVtbl -> Release(This)
    #define ICreatePropBagOnRegKey_Create(This,hkey,subkey,ulOptions,samDesired,iid,ppBag) (This)->lpVtbl -> Create(This,hkey,subkey,ulOptions,samDesired,iid,ppBag)
  #endif
#endif


void __RPC_STUB 
ICreatePropBagOnRegKey_Create_Stub(
  IRpcStubBuffer *This,
  IRpcChannelBuffer *_pRpcChannelBuffer,
  PRPC_MESSAGE _pRpcMessage,
  DWORD *_pdwStubPhase);

HRESULT STDMETHODCALLTYPE
ICreatePropBagOnRegKey_Create_Proxy(
  ICreatePropBagOnRegKey * This,
  HKEY hkey,
  LPCOLESTR subkey,
  DWORD ulOptions,
  DWORD samDesired,
  REFIID iid,
  LPVOID *ppBag);

#endif 
#ifdef __cplusplus
}
#endif

#endif
