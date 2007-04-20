/* This file contains the proxy/stub code for core COM interfaces.

   It is usually generated directly by MIDL, however this file has
   been tweaked since then to account for slight differences in the way
   gcc and MSVC++ compile it. In particular, in some functions REFIIDs
   declared on the stack have been converted to plain IID* in order to eliminate
   the constness of the REFIID type, ensuring that the zero initializer is not
   discarded.

   Therefore, please do not regenerate this file.
*/

/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jan 07 22:24:52 2003
 */
/* Compiler settings for oaidl.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
/*@@MIDL_FILE_HEADING(  ) */


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "oaidl.h"

#define TYPE_FORMAT_STRING_SIZE   1907                              
#define PROC_FORMAT_STRING_SIZE   495                               

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


/* Standard interface: __MIDL_itf_oaidl_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: IOleAutomationTypes, ver. 1.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


static const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE IDispatch_GetTypeInfoCount_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pctinfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!pctinfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            *pctinfo = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pctinfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IDispatch_GetTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    UINT _M0;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT __RPC_FAR *pctinfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pctinfo = 0;
    RpcTryFinally
        {
        pctinfo = &_M0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IDispatch*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeInfoCount((IDispatch *) ((CStdStubBuffer *)This)->pvServerObject,pctinfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pctinfo;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IDispatch_GetTypeInfo_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = iTInfo;
            _StubMsg.Buffer += sizeof(UINT);
            
            *( LCID __RPC_FAR * )_StubMsg.Buffer = lcid;
            _StubMsg.Buffer += sizeof(LCID);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[6] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTInfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IDispatch_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M1;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT iTInfo;
    LCID lcid;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTInfo = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[6] );
        
        iTInfo = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        lcid = *( LCID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(LCID);
        
        ppTInfo = &_M1;
        _M1 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IDispatch*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeInfo(
               (IDispatch *) ((CStdStubBuffer *)This)->pvServerObject,
               iTInfo,
               lcid,
               ppTInfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTInfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTInfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IDispatch_GetIDsOfNames_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        if(!riid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!rgszNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!rgDispId)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U + 7U + 7U + 7U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)riid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            _StubMsg.MaxCount = cNames;
            
            NdrConformantArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                          (unsigned char __RPC_FAR *)rgszNames,
                                          (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[54] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)riid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            _StubMsg.MaxCount = cNames;
            
            NdrConformantArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                        (unsigned char __RPC_FAR *)rgszNames,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[54] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = cNames;
            _StubMsg.Buffer += sizeof(UINT);
            
            *( LCID __RPC_FAR * )_StubMsg.Buffer = lcid;
            _StubMsg.Buffer += sizeof(LCID);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[16] );
            
            NdrConformantArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                          (unsigned char __RPC_FAR * __RPC_FAR *)&rgDispId,
                                          (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[88],
                                          (unsigned char)0 );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = cNames;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[84],
                         ( void __RPC_FAR * )rgDispId);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IDispatch_GetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT cNames;
    LCID lcid;
    DISPID __RPC_FAR *rgDispId;
    LPOLESTR __RPC_FAR *rgszNames;
    IID* riid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    rgszNames = 0;
    rgDispId = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[16] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&riid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        NdrConformantArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&rgszNames,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[54],
                                      (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        cNames = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);

        
        lcid = *( LCID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(LCID);
        
        rgDispId = NdrAllocate(&_StubMsg,cNames * 4);
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IDispatch*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetIDsOfNames(
                 (IDispatch *) ((CStdStubBuffer *)This)->pvServerObject,
                 riid,
                 rgszNames,
                 cNames,
                 lcid,
                 rgDispId);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 7U;
        _StubMsg.MaxCount = cNames;
        
        NdrConformantArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR *)rgDispId,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[88] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = cNames;
        
        NdrConformantArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                    (unsigned char __RPC_FAR *)rgDispId,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[88] );
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = cNames;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)rgszNames,
                        &__MIDL_TypeFormatString.Format[50] );
        
        if ( rgDispId )
            _StubMsg.pfnFree( rgDispId );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IDispatch_RemoteInvoke_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *pArgErr,
    /* [in] */ UINT cVarRef,
    /* [size_is][in] */ UINT __RPC_FAR *rgVarRefIdx,
    /* [size_is][out][in] */ VARIANTARG __RPC_FAR *rgVarRef)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarResult)
        {
        MIDL_memset(
               pVarResult,
               0,
               sizeof( VARIANT  ));
        }
    if(pExcepInfo)
        {
        MIDL_memset(
               pExcepInfo,
               0,
               sizeof( EXCEPINFO  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!riid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDispParams)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarResult)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pExcepInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pArgErr)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!rgVarRefIdx)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!rgVarRef)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 11U + 7U + 4U + 11U + 7U + 7U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)riid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR *)pDispParams,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1080] );
            
            _StubMsg.MaxCount = cVarRef;
            
            NdrConformantArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                          (unsigned char __RPC_FAR *)rgVarRefIdx,
                                          (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1170] );
            
            _StubMsg.MaxCount = cVarRef;
            
            NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)rgVarRef,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *( DISPID __RPC_FAR * )_StubMsg.Buffer = dispIdMember;
            _StubMsg.Buffer += sizeof(DISPID);
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)riid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            *( LCID __RPC_FAR * )_StubMsg.Buffer = lcid;
            _StubMsg.Buffer += sizeof(LCID);
            
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = dwFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                      (unsigned char __RPC_FAR *)pDispParams,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1080] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = cVarRef;
            _StubMsg.Buffer += sizeof(UINT);
            
            _StubMsg.MaxCount = cVarRef;
            
            NdrConformantArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                        (unsigned char __RPC_FAR *)rgVarRefIdx,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1170] );
            
            _StubMsg.MaxCount = cVarRef;
            
            NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)rgVarRef,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[34] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarResult,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pExcepInfo,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1138],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pArgErr = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&rgVarRef,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184],
                                       (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarResult);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1120],
                         ( void __RPC_FAR * )pExcepInfo);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pArgErr);
        _StubMsg.MaxCount = cVarRef;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1180],
                         ( void __RPC_FAR * )rgVarRef);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IDispatch_RemoteInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M6;
    UINT _M7;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    EXCEPINFO _pExcepInfoM;
    UINT cVarRef;
    DISPID dispIdMember;
    DWORD dwFlags;
    LCID lcid;
    UINT __RPC_FAR *pArgErr;
    DISPPARAMS __RPC_FAR *pDispParams;
    EXCEPINFO __RPC_FAR *pExcepInfo;
    VARIANT __RPC_FAR *pVarResult;
    VARIANTARG __RPC_FAR *rgVarRef;
    UINT __RPC_FAR *rgVarRefIdx;
    IID* riid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pDispParams = 0;
    pVarResult = 0;
    pExcepInfo = 0;
    pArgErr = 0;
    rgVarRefIdx = 0;
    rgVarRef = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[34] );
        
        dispIdMember = *( DISPID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DISPID);
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&riid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        lcid = *( LCID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(LCID);
        
        dwFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR * __RPC_FAR *)&pDispParams,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1080],
                                    (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        cVarRef = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        NdrConformantArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&rgVarRefIdx,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1170],
                                      (unsigned char)0 );
        
        NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&rgVarRef,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184],
                                   (unsigned char)0 );
        
        pVarResult = &_M6;
        MIDL_memset(
               pVarResult,
               0,
               sizeof( VARIANT  ));
        pExcepInfo = &_pExcepInfoM;
        pArgErr = &_M7;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = IDispatch_Invoke_Stub(
                                (IDispatch *) ((CStdStubBuffer *)This)->pvServerObject,
                                dispIdMember,
                                riid,
                                lcid,
                                dwFlags,
                                pDispParams,
                                pVarResult,
                                pExcepInfo,
                                pArgErr,
                                cVarRef,
                                rgVarRefIdx,
                                rgVarRef);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 7U + 11U + 7U + 7U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarResult,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pExcepInfo,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1138] );
        
        _StubMsg.MaxCount = cVarRef;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)rgVarRef,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarResult,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pExcepInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1138] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pArgErr;
        _StubMsg.Buffer += sizeof(UINT);
        
        _StubMsg.MaxCount = cVarRef;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)rgVarRef,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1184] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDispParams,
                        &__MIDL_TypeFormatString.Format[98] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarResult,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pExcepInfo,
                        &__MIDL_TypeFormatString.Format[1120] );
        
        _StubMsg.MaxCount = cVarRef;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)rgVarRef,
                        &__MIDL_TypeFormatString.Format[1180] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(7) _IDispatchProxyVtbl = 
{
    { &IID_IDispatch },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        IDispatch_GetTypeInfoCount_Proxy ,
        IDispatch_GetTypeInfo_Proxy ,
        IDispatch_GetIDsOfNames_Proxy ,
        IDispatch_Invoke_Proxy
    }
};


static const PRPC_STUB_FUNCTION IDispatch_table[] =
{
    IDispatch_GetTypeInfoCount_Stub,
    IDispatch_GetTypeInfo_Stub,
    IDispatch_GetIDsOfNames_Stub,
    IDispatch_RemoteInvoke_Stub
};

static const CInterfaceStubVtbl _IDispatchStubVtbl =
{
    {
        &IID_IDispatch,
        0,
        7,
        &IDispatch_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: IEnumVARIANT, ver. 0.0,
   GUID={0x00020404,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumVARIANT_RemoteNext_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
    /* [out] */ ULONG __RPC_FAR *pCeltFetched)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(rgVar)
        {
        MIDL_memset(
               rgVar,
               0,
               celt * sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!rgVar)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pCeltFetched)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = celt;
            _StubMsg.Buffer += sizeof(ULONG);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[72] );
            
            NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&rgVar,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1206],
                                       (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pCeltFetched = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(ULONG);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = celt;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = _StubMsg.MaxCount;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1202],
                         ( void __RPC_FAR * )rgVar);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pCeltFetched);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IEnumVARIANT_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ULONG _M11;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG celt;
    ULONG __RPC_FAR *pCeltFetched;
    VARIANT __RPC_FAR *rgVar;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    rgVar = 0;
    pCeltFetched = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[72] );
        
        celt = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        rgVar = NdrAllocate(&_StubMsg,celt * 16);
        pCeltFetched = &_M11;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = IEnumVARIANT_Next_Stub(
                                 (IEnumVARIANT *) ((CStdStubBuffer *)This)->pvServerObject,
                                 celt,
                                 rgVar,
                                 pCeltFetched);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 12U + 7U + 7U;
        _StubMsg.MaxCount = celt;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pCeltFetched ? *pCeltFetched : 0;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)rgVar,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1206] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = celt;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pCeltFetched ? *pCeltFetched : 0;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)rgVar,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1206] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( ULONG __RPC_FAR * )_StubMsg.Buffer = *pCeltFetched;
        _StubMsg.Buffer += sizeof(ULONG);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = celt;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pCeltFetched ? *pCeltFetched : 0;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)rgVar,
                        &__MIDL_TypeFormatString.Format[1202] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Skip_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = celt;
            _StubMsg.Buffer += sizeof(ULONG);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[84] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IEnumVARIANT_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG celt;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[84] );
        
        celt = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IEnumVARIANT*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Skip((IEnumVARIANT *) ((CStdStubBuffer *)This)->pvServerObject,celt);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Reset_Proxy( 
    IEnumVARIANT __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
 
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IEnumVARIANT_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IEnumVARIANT*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Reset((IEnumVARIANT *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Clone_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppEnum)
        {
        MIDL_memset(
               ppEnum,
               0,
               sizeof( IEnumVARIANT __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!ppEnum)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[90] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppEnum,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1224],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1224],
                         ( void __RPC_FAR * )ppEnum);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IEnumVARIANT_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    IEnumVARIANT __RPC_FAR *_M12;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppEnum = 0;
    RpcTryFinally
        {
        ppEnum = &_M12;
        _M12 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IEnumVARIANT*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Clone((IEnumVARIANT *) ((CStdStubBuffer *)This)->pvServerObject,ppEnum);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppEnum,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1224] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppEnum,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1224] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppEnum,
                        &__MIDL_TypeFormatString.Format[1224] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(7) _IEnumVARIANTProxyVtbl = 
{
    { &IID_IEnumVARIANT },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        IEnumVARIANT_Next_Proxy ,
        IEnumVARIANT_Skip_Proxy ,
        IEnumVARIANT_Reset_Proxy ,
        IEnumVARIANT_Clone_Proxy
    }
};


static const PRPC_STUB_FUNCTION IEnumVARIANT_table[] =
{
    IEnumVARIANT_RemoteNext_Stub,
    IEnumVARIANT_Skip_Stub,
    IEnumVARIANT_Reset_Stub,
    IEnumVARIANT_Clone_Stub
};

static const CInterfaceStubVtbl _IEnumVARIANTStubVtbl =
{
    {
        &IID_IEnumVARIANT,
        0,
        7,
        &IEnumVARIANT_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeComp, ver. 0.0,
   GUID={0x00020403,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_RemoteBind_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [in] */ WORD wFlags,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [out] */ DESCKIND __RPC_FAR *pDescKind,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTypeComp,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    if(ppFuncDesc)
        {
        *ppFuncDesc = 0;
        }
    if(ppVarDesc)
        {
        *ppVarDesc = 0;
        }
    if(ppTypeComp)
        {
        MIDL_memset(
               ppTypeComp,
               0,
               sizeof( ITypeComp __RPC_FAR *__RPC_FAR * ));
        }
    if(pDummy)
        {
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!szName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDescKind)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppFuncDesc)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppVarDesc)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppTypeComp)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDummy)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U + 10U + 4U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szName,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szName,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = lHashVal;
            _StubMsg.Buffer += sizeof(ULONG);
            
            *( WORD __RPC_FAR * )_StubMsg.Buffer = wFlags;
            _StubMsg.Buffer += sizeof(WORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[96] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&pDescKind,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250],
                                  (unsigned char)0 );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppFuncDesc,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254],
                                  (unsigned char)0 );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppVarDesc,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464],
                                  (unsigned char)0 );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTypeComp,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540],
                                  (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pDummy,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1568],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTInfo);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1250],
                         ( void __RPC_FAR * )pDescKind);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1254],
                         ( void __RPC_FAR * )ppFuncDesc);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1464],
                         ( void __RPC_FAR * )ppVarDesc);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1540],
                         ( void __RPC_FAR * )ppTypeComp);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1562],
                         ( void __RPC_FAR * )pDummy);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeComp_RemoteBind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M15;
    DESCKIND _M16;
    LPFUNCDESC _M17;
    LPVARDESC _M18;
    ITypeComp __RPC_FAR *_M19;
    CLEANLOCALSTORAGE _M20;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG lHashVal;
    DESCKIND __RPC_FAR *pDescKind;
    CLEANLOCALSTORAGE __RPC_FAR *pDummy;
    LPFUNCDESC __RPC_FAR *ppFuncDesc;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    ITypeComp __RPC_FAR *__RPC_FAR *ppTypeComp;
    LPVARDESC __RPC_FAR *ppVarDesc;
    LPOLESTR szName;
    WORD wFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szName = 0;
    ppTInfo = 0;
    pDescKind = 0;
    ppFuncDesc = 0;
    ppVarDesc = 0;
    ppTypeComp = 0;
    pDummy = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[96] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szName,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        lHashVal = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        wFlags = *( WORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(WORD);
        
        ppTInfo = &_M15;
        _M15 = 0;
        pDescKind = &_M16;
        ppFuncDesc = &_M17;
        _M17 = 0;
        ppVarDesc = &_M18;
        _M18 = 0;
        ppTypeComp = &_M19;
        _M19 = 0;
        pDummy = &_M20;
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeComp_Bind_Stub(
                              (ITypeComp *) ((CStdStubBuffer *)This)->pvServerObject,
                              szName,
                              lHashVal,
                              wFlags,
                              ppTInfo,
                              pDescKind,
                              ppFuncDesc,
                              ppVarDesc,
                              ppTypeComp,
                              pDummy);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U + 4U + 15U + 0U + 11U + 7U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTInfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppFuncDesc,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppVarDesc,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTypeComp,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTInfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)pDescKind,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppFuncDesc,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppVarDesc,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTypeComp,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pDummy,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1568] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppFuncDesc,
                        &__MIDL_TypeFormatString.Format[1254] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppVarDesc,
                        &__MIDL_TypeFormatString.Format[1464] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTypeComp,
                        &__MIDL_TypeFormatString.Format[1540] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDummy,
                        &__MIDL_TypeFormatString.Format[1562] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_RemoteBindType_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!szName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U + 10U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szName,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szName,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = lHashVal;
            _StubMsg.Buffer += sizeof(ULONG);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[130] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTInfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeComp_RemoteBindType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M23;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG lHashVal;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    LPOLESTR szName;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szName = 0;
    ppTInfo = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[130] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szName,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        lHashVal = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        ppTInfo = &_M23;
        _M23 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeComp_BindType_Stub(
                                  (ITypeComp *) ((CStdStubBuffer *)This)->pvServerObject,
                                  szName,
                                  lHashVal,
                                  ppTInfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTInfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTInfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(5) _ITypeCompProxyVtbl = 
{
    { &IID_ITypeComp },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeComp_Bind_Proxy ,
        ITypeComp_BindType_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeComp_table[] =
{
    ITypeComp_RemoteBind_Stub,
    ITypeComp_RemoteBindType_Stub
};

static const CInterfaceStubVtbl _ITypeCompStubVtbl =
{
    {
        &IID_ITypeComp,
        0,
        5,
        &ITypeComp_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeInfo, ver. 0.0,
   GUID={0x00020401,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ LPTYPEATTR __RPC_FAR *ppTypeAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTypeAttr)
        {
        *ppTypeAttr = 0;
        }
    if(pDummy)
        {
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!ppTypeAttr)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDummy)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[142] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTypeAttr,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1578],
                                  (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pDummy,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1644],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1578],
                         ( void __RPC_FAR * )ppTypeAttr);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1638],
                         ( void __RPC_FAR * )pDummy);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetTypeAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    LPTYPEATTR _M24;
    CLEANLOCALSTORAGE _M25;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CLEANLOCALSTORAGE __RPC_FAR *pDummy;
    LPTYPEATTR __RPC_FAR *ppTypeAttr;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTypeAttr = 0;
    pDummy = 0;
    RpcTryFinally
        {
        ppTypeAttr = &_M24;
        _M24 = 0;
        pDummy = &_M25;
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetTypeAttr_Stub(
                                     (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                     ppTypeAttr,
                                     pDummy);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 7U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTypeAttr,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1578] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTypeAttr,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1578] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pDummy,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1644] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTypeAttr,
                        &__MIDL_TypeFormatString.Format[1578] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDummy,
                        &__MIDL_TypeFormatString.Format[1638] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeComp_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTComp)
        {
        MIDL_memset(
               ppTComp,
               0,
               sizeof( ITypeComp __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!ppTComp)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[152] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTComp,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1540],
                         ( void __RPC_FAR * )ppTComp);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_GetTypeComp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeComp __RPC_FAR *_M26;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ITypeComp __RPC_FAR *__RPC_FAR *ppTComp;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTComp = 0;
    RpcTryFinally
        {
        ppTComp = &_M26;
        _M26 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeComp((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,ppTComp);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTComp,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTComp,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTComp,
                        &__MIDL_TypeFormatString.Format[1540] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppFuncDesc)
        {
        *ppFuncDesc = 0;
        }
    if(pDummy)
        {
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        if(!ppFuncDesc)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDummy)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[158] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppFuncDesc,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254],
                                  (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pDummy,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1660],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1254],
                         ( void __RPC_FAR * )ppFuncDesc);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1654],
                         ( void __RPC_FAR * )pDummy);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    LPFUNCDESC _M27;
    CLEANLOCALSTORAGE _M28;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    CLEANLOCALSTORAGE __RPC_FAR *pDummy;
    LPFUNCDESC __RPC_FAR *ppFuncDesc;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppFuncDesc = 0;
    pDummy = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[158] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        ppFuncDesc = &_M27;
        _M27 = 0;
        pDummy = &_M28;
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetFuncDesc_Stub(
                                     (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                     index,
                                     ppFuncDesc,
                                     pDummy);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 7U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppFuncDesc,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppFuncDesc,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1254] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pDummy,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1660] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppFuncDesc,
                        &__MIDL_TypeFormatString.Format[1254] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDummy,
                        &__MIDL_TypeFormatString.Format[1654] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppVarDesc)
        {
        *ppVarDesc = 0;
        }
    if(pDummy)
        {
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!ppVarDesc)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDummy)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[170] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppVarDesc,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464],
                                  (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pDummy,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1676],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1464],
                         ( void __RPC_FAR * )ppVarDesc);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1670],
                         ( void __RPC_FAR * )pDummy);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    LPVARDESC _M29;
    CLEANLOCALSTORAGE _M30;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    CLEANLOCALSTORAGE __RPC_FAR *pDummy;
    LPVARDESC __RPC_FAR *ppVarDesc;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppVarDesc = 0;
    pDummy = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[170] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        ppVarDesc = &_M29;
        _M29 = 0;
        pDummy = &_M30;
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetVarDesc_Stub(
                                    (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                    index,
                                    ppVarDesc,
                                    pDummy);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 7U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppVarDesc,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppVarDesc,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1464] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pDummy,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1676] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppVarDesc,
                        &__MIDL_TypeFormatString.Format[1464] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDummy,
                        &__MIDL_TypeFormatString.Format[1670] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetNames_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
    /* [in] */ UINT cMaxNames,
    /* [out] */ UINT __RPC_FAR *pcNames)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(rgBstrNames)
        {
        MIDL_memset(
               rgBstrNames,
               0,
               cMaxNames * sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      7);
        
        
        
        if(!rgBstrNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pcNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            *( UINT __RPC_FAR * )_StubMsg.Buffer = cMaxNames;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[182] );
            
            NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&rgBstrNames,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1690],
                                       (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pcNames = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = cMaxNames;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = _StubMsg.MaxCount;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1686],
                         ( void __RPC_FAR * )rgBstrNames);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pcNames);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    UINT _M34;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT cMaxNames;
    MEMBERID memid;
    UINT __RPC_FAR *pcNames;
    BSTR __RPC_FAR *rgBstrNames;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    rgBstrNames = 0;
    pcNames = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[182] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        cMaxNames = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        rgBstrNames = NdrAllocate(&_StubMsg,cMaxNames * 4);
        pcNames = &_M34;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetNames_Stub(
                                  (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                  memid,
                                  rgBstrNames,
                                  cMaxNames,
                                  pcNames);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 12U + 7U + 7U;
        _StubMsg.MaxCount = cMaxNames;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcNames ? *pcNames : 0;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)rgBstrNames,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1690] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = cMaxNames;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcNames ? *pcNames : 0;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)rgBstrNames,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1690] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pcNames;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = cMaxNames;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcNames ? *pcNames : 0;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)rgBstrNames,
                        &__MIDL_TypeFormatString.Format[1686] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo_GetRefTypeOfImplType_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ HREFTYPE __RPC_FAR *pRefType)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      8);
        
        
        
        if(!pRefType)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
            
            *pRefType = *( HREFTYPE __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HREFTYPE);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pRefType);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_GetRefTypeOfImplType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HREFTYPE _M35;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    HREFTYPE __RPC_FAR *pRefType;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pRefType = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pRefType = &_M35;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetRefTypeOfImplType(
                        (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                        index,
                        pRefType);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HREFTYPE __RPC_FAR * )_StubMsg.Buffer = *pRefType;
        _StubMsg.Buffer += sizeof(HREFTYPE);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo_GetImplTypeFlags_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ INT __RPC_FAR *pImplTypeFlags)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      9);
        
        
        
        if(!pImplTypeFlags)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
            
            *pImplTypeFlags = *( INT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(INT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pImplTypeFlags);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_GetImplTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    INT _M36;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    INT __RPC_FAR *pImplTypeFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pImplTypeFlags = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pImplTypeFlags = &_M36;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetImplTypeFlags(
                    (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                    index,
                    pImplTypeFlags);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( INT __RPC_FAR * )_StubMsg.Buffer = *pImplTypeFlags;
        _StubMsg.Buffer += sizeof(INT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalGetIDsOfNames_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      10);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalGetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetIDsOfNames_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalInvoke_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      11);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_Invoke_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetDocumentation_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrName)
        {
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        }
    if(pBstrDocString)
        {
        MIDL_memset(
               pBstrDocString,
               0,
               sizeof( BSTR  ));
        }
    if(pBstrHelpFile)
        {
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      12);
        
        
        
        if(!pBstrName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrDocString)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pdwHelpContext)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrHelpFile)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = refPtrFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[204] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrDocString,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pdwHelpContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrHelpFile,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrName);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrDocString);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pdwHelpContext);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrHelpFile);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetDocumentation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M37;
    BSTR _M38;
    DWORD _M39;
    BSTR _M40;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    MEMBERID memid;
    BSTR __RPC_FAR *pBstrDocString;
    BSTR __RPC_FAR *pBstrHelpFile;
    BSTR __RPC_FAR *pBstrName;
    DWORD __RPC_FAR *pdwHelpContext;
    DWORD refPtrFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrName = 0;
    pBstrDocString = 0;
    pdwHelpContext = 0;
    pBstrHelpFile = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[204] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        refPtrFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        pBstrName = &_M37;
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        pBstrDocString = &_M38;
        MIDL_memset(
               pBstrDocString,
               0,
               sizeof( BSTR  ));
        pdwHelpContext = &_M39;
        pBstrHelpFile = &_M40;
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetDocumentation_Stub(
                                          (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                          memid,
                                          refPtrFlags,
                                          pBstrName,
                                          pBstrDocString,
                                          pdwHelpContext,
                                          pBstrHelpFile);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 15U + 11U + 11U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrDocString,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrHelpFile,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrDocString,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( DWORD __RPC_FAR * )_StubMsg.Buffer = *pdwHelpContext;
        _StubMsg.Buffer += sizeof(DWORD);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrHelpFile,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrDocString,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrHelpFile,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetDllEntry_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrDllName,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ WORD __RPC_FAR *pwOrdinal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrDllName)
        {
        MIDL_memset(
               pBstrDllName,
               0,
               sizeof( BSTR  ));
        }
    if(pBstrName)
        {
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      13);
        
        
        
        if(!pBstrDllName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pwOrdinal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            NdrSimpleTypeMarshall(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( unsigned char __RPC_FAR * )&invKind,
                         14);
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = refPtrFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[226] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrDllName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
            *pwOrdinal = *( WORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(WORD);
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrDllName);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrName);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1712],
                         ( void __RPC_FAR * )pwOrdinal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetDllEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M41;
    BSTR _M42;
    WORD _M43;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    INVOKEKIND invKind;
    MEMBERID memid;
    BSTR __RPC_FAR *pBstrDllName;
    BSTR __RPC_FAR *pBstrName;
    WORD __RPC_FAR *pwOrdinal;
    DWORD refPtrFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrDllName = 0;
    pBstrName = 0;
    pwOrdinal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[226] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        NdrSimpleTypeUnmarshall(
                           ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                           ( unsigned char __RPC_FAR * )&invKind,
                           14);
        refPtrFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        pBstrDllName = &_M41;
        MIDL_memset(
               pBstrDllName,
               0,
               sizeof( BSTR  ));
        pBstrName = &_M42;
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        pwOrdinal = &_M43;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetDllEntry_Stub(
                                     (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                     memid,
                                     invKind,
                                     refPtrFlags,
                                     pBstrDllName,
                                     pBstrName,
                                     pwOrdinal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 15U + 5U + 10U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrDllName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrDllName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
        *( WORD __RPC_FAR * )_StubMsg.Buffer = *pwOrdinal;
        _StubMsg.Buffer += sizeof(WORD);
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrDllName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo_GetRefTypeInfo_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ HREFTYPE hRefType,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      14);
        
        
        
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( HREFTYPE __RPC_FAR * )_StubMsg.Buffer = hRefType;
            _StubMsg.Buffer += sizeof(HREFTYPE);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[246] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTInfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_GetRefTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M44;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    HREFTYPE hRefType;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTInfo = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[246] );
        
        hRefType = *( HREFTYPE __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(HREFTYPE);
        
        ppTInfo = &_M44;
        _M44 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetRefTypeInfo(
                  (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                  hRefType,
                  ppTInfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTInfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTInfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalAddressOfMember_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      15);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalAddressOfMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_AddressOfMember_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteCreateInstance_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppvObj)
        {
        MIDL_memset(
               ppvObj,
               0,
               sizeof( IUnknown __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      16);
        
        
        
        if(!riid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppvObj)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)riid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)riid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[254] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppvObj,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1716],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1716],
                         ( void __RPC_FAR * )ppvObj);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteCreateInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    IUnknown __RPC_FAR *__RPC_FAR *_M45;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    IUnknown __RPC_FAR *__RPC_FAR *ppvObj;
    IID* riid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppvObj = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[254] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&riid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        ppvObj = (void *)&_M45;
        _M45 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_CreateInstance_Stub(
                                        (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                        riid,
                                        ppvObj);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppvObj,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1716] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppvObj,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1716] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppvObj,
                        &__MIDL_TypeFormatString.Format[1716] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo_GetMops_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [out] */ BSTR __RPC_FAR *pBstrMops)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrMops)
        {
        MIDL_memset(
               pBstrMops,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      17);
        
        
        
        if(!pBstrMops)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[264] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrMops,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrMops);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_GetMops_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M46;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    MEMBERID memid;
    BSTR __RPC_FAR *pBstrMops;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrMops = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[264] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        pBstrMops = &_M46;
        MIDL_memset(
               pBstrMops,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetMops(
           (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
           memid,
           pBstrMops);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrMops,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrMops,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrMops,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetContainingTypeLib_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
    /* [out] */ UINT __RPC_FAR *pIndex)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTLib)
        {
        MIDL_memset(
               ppTLib,
               0,
               sizeof( ITypeLib __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      18);
        
        
        
        if(!ppTLib)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pIndex)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[272] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTLib,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1726],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pIndex = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1726],
                         ( void __RPC_FAR * )ppTLib);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pIndex);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_RemoteGetContainingTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeLib __RPC_FAR *_M47;
    UINT _M48;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT __RPC_FAR *pIndex;
    ITypeLib __RPC_FAR *__RPC_FAR *ppTLib;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTLib = 0;
    pIndex = 0;
    RpcTryFinally
        {
        ppTLib = &_M47;
        _M47 = 0;
        pIndex = &_M48;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_GetContainingTypeLib_Stub(
                                              (ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject,
                                              ppTLib,
                                              pIndex);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTLib,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1726] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTLib,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1726] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pIndex;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTLib,
                        &__MIDL_TypeFormatString.Format[1726] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      19);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalReleaseTypeAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_ReleaseTypeAttr_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      20);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalReleaseFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_ReleaseFuncDesc_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      21);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo_LocalReleaseVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo_ReleaseVarDesc_Stub((ITypeInfo *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(22) _ITypeInfoProxyVtbl = 
{
    { &IID_ITypeInfo },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeInfo_GetTypeAttr_Proxy ,
        ITypeInfo_GetTypeComp_Proxy ,
        ITypeInfo_GetFuncDesc_Proxy ,
        ITypeInfo_GetVarDesc_Proxy ,
        ITypeInfo_GetNames_Proxy ,
        ITypeInfo_GetRefTypeOfImplType_Proxy ,
        ITypeInfo_GetImplTypeFlags_Proxy ,
        ITypeInfo_GetIDsOfNames_Proxy ,
        ITypeInfo_Invoke_Proxy ,
        ITypeInfo_GetDocumentation_Proxy ,
        ITypeInfo_GetDllEntry_Proxy ,
        ITypeInfo_GetRefTypeInfo_Proxy ,
        ITypeInfo_AddressOfMember_Proxy ,
        ITypeInfo_CreateInstance_Proxy ,
        ITypeInfo_GetMops_Proxy ,
        ITypeInfo_GetContainingTypeLib_Proxy ,
        ITypeInfo_ReleaseTypeAttr_Proxy ,
        ITypeInfo_ReleaseFuncDesc_Proxy ,
        ITypeInfo_ReleaseVarDesc_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeInfo_table[] =
{
    ITypeInfo_RemoteGetTypeAttr_Stub,
    ITypeInfo_GetTypeComp_Stub,
    ITypeInfo_RemoteGetFuncDesc_Stub,
    ITypeInfo_RemoteGetVarDesc_Stub,
    ITypeInfo_RemoteGetNames_Stub,
    ITypeInfo_GetRefTypeOfImplType_Stub,
    ITypeInfo_GetImplTypeFlags_Stub,
    ITypeInfo_LocalGetIDsOfNames_Stub,
    ITypeInfo_LocalInvoke_Stub,
    ITypeInfo_RemoteGetDocumentation_Stub,
    ITypeInfo_RemoteGetDllEntry_Stub,
    ITypeInfo_GetRefTypeInfo_Stub,
    ITypeInfo_LocalAddressOfMember_Stub,
    ITypeInfo_RemoteCreateInstance_Stub,
    ITypeInfo_GetMops_Stub,
    ITypeInfo_RemoteGetContainingTypeLib_Stub,
    ITypeInfo_LocalReleaseTypeAttr_Stub,
    ITypeInfo_LocalReleaseFuncDesc_Stub,
    ITypeInfo_LocalReleaseVarDesc_Stub
};

static const CInterfaceStubVtbl _ITypeInfoStubVtbl =
{
    {
        &IID_ITypeInfo,
        0,
        22,
        &ITypeInfo_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeInfo2, ver. 0.0,
   GUID={0x00020412,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetTypeKind_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ TYPEKIND __RPC_FAR *pTypeKind)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      22);
        
        
        
        if(!pTypeKind)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[282] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&pTypeKind,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250],
                                  (unsigned char)0 );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1250],
                         ( void __RPC_FAR * )pTypeKind);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetTypeKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    TYPEKIND _M49;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    TYPEKIND __RPC_FAR *pTypeKind;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pTypeKind = 0;
    RpcTryFinally
        {
        pTypeKind = &_M49;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeKind((ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,pTypeKind);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)pTypeKind,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250] );
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetTypeFlags_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pTypeFlags)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      23);
        
        
        
        if(!pTypeFlags)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            *pTypeFlags = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(ULONG);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pTypeFlags);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ULONG _M50;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG __RPC_FAR *pTypeFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pTypeFlags = 0;
    RpcTryFinally
        {
        pTypeFlags = &_M50;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeFlags((ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,pTypeFlags);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( ULONG __RPC_FAR * )_StubMsg.Buffer = *pTypeFlags;
        _StubMsg.Buffer += sizeof(ULONG);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetFuncIndexOfMemId_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [out] */ UINT __RPC_FAR *pFuncIndex)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      24);
        
        
        
        if(!pFuncIndex)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            NdrSimpleTypeMarshall(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( unsigned char __RPC_FAR * )&invKind,
                         14);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[288] );
            
            *pFuncIndex = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pFuncIndex);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetFuncIndexOfMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    UINT _M51;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    INVOKEKIND invKind;
    MEMBERID memid;
    UINT __RPC_FAR *pFuncIndex;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pFuncIndex = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[288] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        NdrSimpleTypeUnmarshall(
                           ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                           ( unsigned char __RPC_FAR * )&invKind,
                           14);
        pFuncIndex = &_M51;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetFuncIndexOfMemId(
                       (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                       memid,
                       invKind,
                       pFuncIndex);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pFuncIndex;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetVarIndexOfMemId_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [out] */ UINT __RPC_FAR *pVarIndex)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      25);
        
        
        
        if(!pVarIndex)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
            
            *pVarIndex = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pVarIndex);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetVarIndexOfMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    UINT _M52;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    MEMBERID memid;
    UINT __RPC_FAR *pVarIndex;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarIndex = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[196] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        pVarIndex = &_M52;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetVarIndexOfMemId(
                      (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                      memid,
                      pVarIndex);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pVarIndex;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      26);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[298] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M53;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[298] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M53;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetCustData(
               (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
               guid,
               pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetFuncCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      27);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetFuncCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M54;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    UINT index;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M54;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetFuncCustData(
                   (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                   index,
                   guid,
                   pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetParamCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT indexFunc,
    /* [in] */ UINT indexParam,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      28);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = indexFunc;
            _StubMsg.Buffer += sizeof(UINT);
            
            *( UINT __RPC_FAR * )_StubMsg.Buffer = indexParam;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[320] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetParamCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M55;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    UINT indexFunc;
    UINT indexParam;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[320] );
        
        indexFunc = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        indexParam = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M55;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetParamCustData(
                    (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                    indexFunc,
                    indexParam,
                    guid,
                    pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetVarCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      29);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetVarCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M56;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    UINT index;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M56;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetVarCustData(
                  (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                  index,
                  guid,
                  pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetImplTypeCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      30);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetImplTypeCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M57;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    UINT index;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[308] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M57;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetImplTypeCustData(
                       (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                       index,
                       guid,
                       pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo2_RemoteGetDocumentation2_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pbstrHelpString)
        {
        MIDL_memset(
               pbstrHelpString,
               0,
               sizeof( BSTR  ));
        }
    if(pbstrHelpStringDll)
        {
        MIDL_memset(
               pbstrHelpStringDll,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      31);
        
        
        
        if(!pbstrHelpString)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pdwHelpStringContext)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pbstrHelpStringDll)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( MEMBERID __RPC_FAR * )_StubMsg.Buffer = memid;
            _StubMsg.Buffer += sizeof(MEMBERID);
            
            *( LCID __RPC_FAR * )_StubMsg.Buffer = lcid;
            _StubMsg.Buffer += sizeof(LCID);
            
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = refPtrFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[334] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pbstrHelpString,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pdwHelpStringContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pbstrHelpStringDll,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pbstrHelpString);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pdwHelpStringContext);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pbstrHelpStringDll);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_RemoteGetDocumentation2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M58;
    DWORD _M59;
    BSTR _M60;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    LCID lcid;
    MEMBERID memid;
    BSTR __RPC_FAR *pbstrHelpString;
    BSTR __RPC_FAR *pbstrHelpStringDll;
    DWORD __RPC_FAR *pdwHelpStringContext;
    DWORD refPtrFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pbstrHelpString = 0;
    pdwHelpStringContext = 0;
    pbstrHelpStringDll = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[334] );
        
        memid = *( MEMBERID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(MEMBERID);
        
        lcid = *( LCID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(LCID);
        
        refPtrFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        pbstrHelpString = &_M58;
        MIDL_memset(
               pbstrHelpString,
               0,
               sizeof( BSTR  ));
        pdwHelpStringContext = &_M59;
        pbstrHelpStringDll = &_M60;
        MIDL_memset(
               pbstrHelpStringDll,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeInfo2_GetDocumentation2_Stub(
                                            (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                                            memid,
                                            lcid,
                                            refPtrFlags,
                                            pbstrHelpString,
                                            pdwHelpStringContext,
                                            pbstrHelpStringDll);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 11U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pbstrHelpString,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pbstrHelpString,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( DWORD __RPC_FAR * )_StubMsg.Buffer = *pdwHelpStringContext;
        _StubMsg.Buffer += sizeof(DWORD);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pbstrHelpString,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      32);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[354] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetAllCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllCustData((ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllFuncCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      33);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetAllFuncCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    UINT index;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllFuncCustData(
                      (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                      index,
                      pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllParamCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT indexFunc,
    /* [in] */ UINT indexParam,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      34);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = indexFunc;
            _StubMsg.Buffer += sizeof(UINT);
            
            *( UINT __RPC_FAR * )_StubMsg.Buffer = indexParam;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[368] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetAllParamCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    UINT indexFunc;
    UINT indexParam;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[368] );
        
        indexFunc = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        indexParam = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllParamCustData(
                       (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                       indexFunc,
                       indexParam,
                       pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllVarCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      35);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetAllVarCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    UINT index;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllVarCustData(
                     (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                     index,
                     pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllImplTypeCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      36);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeInfo2_GetAllImplTypeCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    UINT index;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[360] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeInfo2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllImplTypeCustData(
                          (ITypeInfo2 *) ((CStdStubBuffer *)This)->pvServerObject,
                          index,
                          pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(37) _ITypeInfo2ProxyVtbl = 
{
    { &IID_ITypeInfo2 },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeInfo_GetTypeAttr_Proxy ,
        ITypeInfo_GetTypeComp_Proxy ,
        ITypeInfo_GetFuncDesc_Proxy ,
        ITypeInfo_GetVarDesc_Proxy ,
        ITypeInfo_GetNames_Proxy ,
        ITypeInfo_GetRefTypeOfImplType_Proxy ,
        ITypeInfo_GetImplTypeFlags_Proxy ,
        ITypeInfo_GetIDsOfNames_Proxy ,
        ITypeInfo_Invoke_Proxy ,
        ITypeInfo_GetDocumentation_Proxy ,
        ITypeInfo_GetDllEntry_Proxy ,
        ITypeInfo_GetRefTypeInfo_Proxy ,
        ITypeInfo_AddressOfMember_Proxy ,
        ITypeInfo_CreateInstance_Proxy ,
        ITypeInfo_GetMops_Proxy ,
        ITypeInfo_GetContainingTypeLib_Proxy ,
        ITypeInfo_ReleaseTypeAttr_Proxy ,
        ITypeInfo_ReleaseFuncDesc_Proxy ,
        ITypeInfo_ReleaseVarDesc_Proxy ,
        ITypeInfo2_GetTypeKind_Proxy ,
        ITypeInfo2_GetTypeFlags_Proxy ,
        ITypeInfo2_GetFuncIndexOfMemId_Proxy ,
        ITypeInfo2_GetVarIndexOfMemId_Proxy ,
        ITypeInfo2_GetCustData_Proxy ,
        ITypeInfo2_GetFuncCustData_Proxy ,
        ITypeInfo2_GetParamCustData_Proxy ,
        ITypeInfo2_GetVarCustData_Proxy ,
        ITypeInfo2_GetImplTypeCustData_Proxy ,
        ITypeInfo2_GetDocumentation2_Proxy ,
        ITypeInfo2_GetAllCustData_Proxy ,
        ITypeInfo2_GetAllFuncCustData_Proxy ,
        ITypeInfo2_GetAllParamCustData_Proxy ,
        ITypeInfo2_GetAllVarCustData_Proxy ,
        ITypeInfo2_GetAllImplTypeCustData_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeInfo2_table[] =
{
    ITypeInfo_RemoteGetTypeAttr_Stub,
    ITypeInfo_GetTypeComp_Stub,
    ITypeInfo_RemoteGetFuncDesc_Stub,
    ITypeInfo_RemoteGetVarDesc_Stub,
    ITypeInfo_RemoteGetNames_Stub,
    ITypeInfo_GetRefTypeOfImplType_Stub,
    ITypeInfo_GetImplTypeFlags_Stub,
    ITypeInfo_LocalGetIDsOfNames_Stub,
    ITypeInfo_LocalInvoke_Stub,
    ITypeInfo_RemoteGetDocumentation_Stub,
    ITypeInfo_RemoteGetDllEntry_Stub,
    ITypeInfo_GetRefTypeInfo_Stub,
    ITypeInfo_LocalAddressOfMember_Stub,
    ITypeInfo_RemoteCreateInstance_Stub,
    ITypeInfo_GetMops_Stub,
    ITypeInfo_RemoteGetContainingTypeLib_Stub,
    ITypeInfo_LocalReleaseTypeAttr_Stub,
    ITypeInfo_LocalReleaseFuncDesc_Stub,
    ITypeInfo_LocalReleaseVarDesc_Stub,
    ITypeInfo2_GetTypeKind_Stub,
    ITypeInfo2_GetTypeFlags_Stub,
    ITypeInfo2_GetFuncIndexOfMemId_Stub,
    ITypeInfo2_GetVarIndexOfMemId_Stub,
    ITypeInfo2_GetCustData_Stub,
    ITypeInfo2_GetFuncCustData_Stub,
    ITypeInfo2_GetParamCustData_Stub,
    ITypeInfo2_GetVarCustData_Stub,
    ITypeInfo2_GetImplTypeCustData_Stub,
    ITypeInfo2_RemoteGetDocumentation2_Stub,
    ITypeInfo2_GetAllCustData_Stub,
    ITypeInfo2_GetAllFuncCustData_Stub,
    ITypeInfo2_GetAllParamCustData_Stub,
    ITypeInfo2_GetAllVarCustData_Stub,
    ITypeInfo2_GetAllImplTypeCustData_Stub
};

static const CInterfaceStubVtbl _ITypeInfo2StubVtbl =
{
    {
        &IID_ITypeInfo2,
        0,
        37,
        &ITypeInfo2_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeLib, ver. 0.0,
   GUID={0x00020402,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetTypeInfoCount_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pcTInfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!pcTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            *pcTInfo = *( UINT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(UINT);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pcTInfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_RemoteGetTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    UINT _M61;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT __RPC_FAR *pcTInfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pcTInfo = 0;
    RpcTryFinally
        {
        pcTInfo = &_M61;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_GetTypeInfoCount_Stub((ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,pcTInfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( UINT __RPC_FAR * )_StubMsg.Buffer = *pcTInfo;
        _StubMsg.Buffer += sizeof(UINT);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfo_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[246] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTInfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M62;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTInfo = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[246] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        ppTInfo = &_M62;
        _M62 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeInfo(
               (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
               index,
               ppTInfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTInfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTInfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoType_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ TYPEKIND __RPC_FAR *pTKind)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        if(!pTKind)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( UINT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(UINT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[378] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&pTKind,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250],
                                  (unsigned char)0 );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1250],
                         ( void __RPC_FAR * )pTKind);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_GetTypeInfoType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    TYPEKIND _M63;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    UINT index;
    TYPEKIND __RPC_FAR *pTKind;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pTKind = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[378] );
        
        index = *( UINT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(UINT);
        
        pTKind = &_M63;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeInfoType(
                   (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                   index,
                   pTKind);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)pTKind,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1250] );
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoOfGuid_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTinfo)
        {
        MIDL_memset(
               ppTinfo,
               0,
               sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppTinfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[386] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTinfo,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[6],
                         ( void __RPC_FAR * )ppTinfo);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_GetTypeInfoOfGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeInfo __RPC_FAR *_M64;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTinfo = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[386] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        ppTinfo = &_M64;
        _M64 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeInfoOfGuid(
                     (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                     guid,
                     ppTinfo);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTinfo,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTinfo,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[6] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTinfo,
                        &__MIDL_TypeFormatString.Format[6] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ LPTLIBATTR __RPC_FAR *ppTLibAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTLibAttr)
        {
        *ppTLibAttr = 0;
        }
    if(pDummy)
        {
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      7);
        
        
        
        if(!ppTLibAttr)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pDummy)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[396] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTLibAttr,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1804],
                                  (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pDummy,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1838],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1804],
                         ( void __RPC_FAR * )ppTLibAttr);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1832],
                         ( void __RPC_FAR * )pDummy);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_RemoteGetLibAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    LPTLIBATTR _M65;
    CLEANLOCALSTORAGE _M66;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CLEANLOCALSTORAGE __RPC_FAR *pDummy;
    LPTLIBATTR __RPC_FAR *ppTLibAttr;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTLibAttr = 0;
    pDummy = 0;
    RpcTryFinally
        {
        ppTLibAttr = &_M65;
        _M65 = 0;
        pDummy = &_M66;
        MIDL_memset(
               pDummy,
               0,
               sizeof( CLEANLOCALSTORAGE  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_GetLibAttr_Stub(
                                   (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                                   ppTLibAttr,
                                   pDummy);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 7U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTLibAttr,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1804] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTLibAttr,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1804] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pDummy,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1838] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTLibAttr,
                        &__MIDL_TypeFormatString.Format[1804] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pDummy,
                        &__MIDL_TypeFormatString.Format[1832] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeComp_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTComp)
        {
        MIDL_memset(
               ppTComp,
               0,
               sizeof( ITypeComp __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      8);
        
        
        
        if(!ppTComp)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[152] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppTComp,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1540],
                         ( void __RPC_FAR * )ppTComp);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_GetTypeComp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ITypeComp __RPC_FAR *_M67;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ITypeComp __RPC_FAR *__RPC_FAR *ppTComp;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ppTComp = 0;
    RpcTryFinally
        {
        ppTComp = &_M67;
        _M67 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetTypeComp((ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,ppTComp);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppTComp,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppTComp,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1540] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTComp,
                        &__MIDL_TypeFormatString.Format[1540] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetDocumentation_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrName)
        {
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        }
    if(pBstrDocString)
        {
        MIDL_memset(
               pBstrDocString,
               0,
               sizeof( BSTR  ));
        }
    if(pBstrHelpFile)
        {
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      9);
        
        
        
        if(!pBstrName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrDocString)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pdwHelpContext)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrHelpFile)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( INT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(INT);
            
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = refPtrFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[204] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrDocString,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pdwHelpContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrHelpFile,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrName);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrDocString);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pdwHelpContext);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrHelpFile);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_RemoteGetDocumentation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M68;
    BSTR _M69;
    DWORD _M70;
    BSTR _M71;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    INT index;
    BSTR __RPC_FAR *pBstrDocString;
    BSTR __RPC_FAR *pBstrHelpFile;
    BSTR __RPC_FAR *pBstrName;
    DWORD __RPC_FAR *pdwHelpContext;
    DWORD refPtrFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrName = 0;
    pBstrDocString = 0;
    pdwHelpContext = 0;
    pBstrHelpFile = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[204] );
        
        index = *( INT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(INT);
        
        refPtrFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        pBstrName = &_M68;
        MIDL_memset(
               pBstrName,
               0,
               sizeof( BSTR  ));
        pBstrDocString = &_M69;
        MIDL_memset(
               pBstrDocString,
               0,
               sizeof( BSTR  ));
        pdwHelpContext = &_M70;
        pBstrHelpFile = &_M71;
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_GetDocumentation_Stub(
                                         (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                                         index,
                                         refPtrFlags,
                                         pBstrName,
                                         pBstrDocString,
                                         pdwHelpContext,
                                         pBstrHelpFile);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 15U + 11U + 11U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrDocString,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrHelpFile,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrDocString,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( DWORD __RPC_FAR * )_StubMsg.Buffer = *pdwHelpContext;
        _StubMsg.Buffer += sizeof(DWORD);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrHelpFile,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrDocString,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrHelpFile,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteIsName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [out] */ BOOL __RPC_FAR *pfName,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrLibName)
        {
        MIDL_memset(
               pBstrLibName,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      10);
        
        
        
        if(!szNameBuf)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pfName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrLibName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U + 10U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szNameBuf,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szNameBuf,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = lHashVal;
            _StubMsg.Buffer += sizeof(ULONG);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[406] );
            
            *pfName = *( BOOL __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(BOOL);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrLibName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pfName);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrLibName);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_RemoteIsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BOOL _M74;
    BSTR _M75;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG lHashVal;
    BSTR __RPC_FAR *pBstrLibName;
    BOOL __RPC_FAR *pfName;
    LPOLESTR szNameBuf;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szNameBuf = 0;
    pfName = 0;
    pBstrLibName = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[406] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szNameBuf,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        lHashVal = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        pfName = &_M74;
        pBstrLibName = &_M75;
        MIDL_memset(
               pBstrLibName,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_IsName_Stub(
                               (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                               szNameBuf,
                               lHashVal,
                               pfName,
                               pBstrLibName);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrLibName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( BOOL __RPC_FAR * )_StubMsg.Buffer = *pfName;
        _StubMsg.Buffer += sizeof(BOOL);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrLibName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrLibName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteFindName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
    /* [out][in] */ USHORT __RPC_FAR *pcFound,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppTInfo)
        {
        MIDL_memset(
               ppTInfo,
               0,
               *pcFound * sizeof( ITypeInfo __RPC_FAR *__RPC_FAR * ));
        }
    if(pBstrLibName)
        {
        MIDL_memset(
               pBstrLibName,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      11);
        
        
        
        if(!szNameBuf)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppTInfo)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!rgMemId)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pcFound)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pBstrLibName)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U + 10U + 4U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szNameBuf,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szNameBuf,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *( ULONG __RPC_FAR * )_StubMsg.Buffer = lHashVal;
            _StubMsg.Buffer += sizeof(ULONG);
            
            *( USHORT __RPC_FAR * )_StubMsg.Buffer = *pcFound;
            _StubMsg.Buffer += sizeof(USHORT);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[422] );
            
            NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&ppTInfo,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1852],
                                       (unsigned char)0 );
            
            NdrConformantVaryingArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                                 (unsigned char __RPC_FAR * __RPC_FAR *)&rgMemId,
                                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1874],
                                                 (unsigned char)0 );
            
            *pcFound = *( USHORT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(USHORT);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrLibName,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = _StubMsg.MaxCount;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1848],
                         ( void __RPC_FAR * )ppTInfo);
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = _StubMsg.MaxCount;
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1870],
                         ( void __RPC_FAR * )rgMemId);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1888],
                         ( void __RPC_FAR * )pcFound);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrLibName);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_RemoteFindName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M84;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG lHashVal;
    BSTR __RPC_FAR *pBstrLibName;
    USHORT __RPC_FAR *pcFound;
    ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo;
    MEMBERID __RPC_FAR *rgMemId;
    LPOLESTR szNameBuf;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szNameBuf = 0;
    ppTInfo = 0;
    rgMemId = 0;
    pcFound = 0;
    pBstrLibName = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[422] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szNameBuf,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        lHashVal = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(ULONG);
        
        pcFound = ( USHORT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof( USHORT  );
        
        ppTInfo = NdrAllocate(&_StubMsg,*pcFound * 4);
        rgMemId = NdrAllocate(&_StubMsg,*pcFound * 4);
        pBstrLibName = &_M84;
        MIDL_memset(
               pBstrLibName,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_FindName_Stub(
                                 (ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject,
                                 szNameBuf,
                                 lHashVal,
                                 ppTInfo,
                                 rgMemId,
                                 pcFound,
                                 pBstrLibName);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 12U + 15U + 4U + 14U + 11U;
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)ppTInfo,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1852] );
        
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrConformantVaryingArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                             (unsigned char __RPC_FAR *)rgMemId,
                                             (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1874] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrLibName,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)ppTInfo,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1852] );
        
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrConformantVaryingArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                           (unsigned char __RPC_FAR *)rgMemId,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1874] );
        
        *( USHORT __RPC_FAR * )_StubMsg.Buffer = *pcFound;
        _StubMsg.Buffer += sizeof(USHORT);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrLibName,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppTInfo,
                        &__MIDL_TypeFormatString.Format[1848] );
        
        _StubMsg.MaxCount = pcFound ? *pcFound : 0;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = pcFound ? *pcFound : 0;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)rgMemId,
                        &__MIDL_TypeFormatString.Format[1870] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrLibName,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_LocalReleaseTLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      12);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[88] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib_LocalReleaseTLibAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib_ReleaseTLibAttr_Stub((ITypeLib *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(13) _ITypeLibProxyVtbl = 
{
    { &IID_ITypeLib },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeLib_GetTypeInfoCount_Proxy ,
        ITypeLib_GetTypeInfo_Proxy ,
        ITypeLib_GetTypeInfoType_Proxy ,
        ITypeLib_GetTypeInfoOfGuid_Proxy ,
        ITypeLib_GetLibAttr_Proxy ,
        ITypeLib_GetTypeComp_Proxy ,
        ITypeLib_GetDocumentation_Proxy ,
        ITypeLib_IsName_Proxy ,
        ITypeLib_FindName_Proxy ,
        ITypeLib_ReleaseTLibAttr_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeLib_table[] =
{
    ITypeLib_RemoteGetTypeInfoCount_Stub,
    ITypeLib_GetTypeInfo_Stub,
    ITypeLib_GetTypeInfoType_Stub,
    ITypeLib_GetTypeInfoOfGuid_Stub,
    ITypeLib_RemoteGetLibAttr_Stub,
    ITypeLib_GetTypeComp_Stub,
    ITypeLib_RemoteGetDocumentation_Stub,
    ITypeLib_RemoteIsName_Stub,
    ITypeLib_RemoteFindName_Stub,
    ITypeLib_LocalReleaseTLibAttr_Stub
};

static const CInterfaceStubVtbl _ITypeLibStubVtbl =
{
    {
        &IID_ITypeLib,
        0,
        13,
        &ITypeLib_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeLib2, ver. 0.0,
   GUID={0x00020411,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE ITypeLib2_GetCustData_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pVarVal)
        {
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      13);
        
        
        
        if(!guid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pVarVal)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)guid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)guid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[298] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pVarVal,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1102],
                         ( void __RPC_FAR * )pVarVal);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib2_GetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    VARIANT _M85;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID guid = 0;
    VARIANT __RPC_FAR *pVarVal;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pVarVal = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[298] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&guid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        pVarVal = &_M85;
        MIDL_memset(
               pVarVal,
               0,
               sizeof( VARIANT  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetCustData(
               (ITypeLib2 *) ((CStdStubBuffer *)This)->pvServerObject,
               guid,
               pVarVal);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pVarVal,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pVarVal,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1110] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pVarVal,
                        &__MIDL_TypeFormatString.Format[1102] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_RemoteGetLibStatistics_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
    /* [out] */ ULONG __RPC_FAR *pcchUniqueNames)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      14);
        
        
        
        if(!pcUniqueNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pcchUniqueNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[446] );
            
            *pcUniqueNames = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(ULONG);
            
            *pcchUniqueNames = *( ULONG __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(ULONG);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pcUniqueNames);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pcchUniqueNames);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib2_RemoteGetLibStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    ULONG _M86;
    ULONG _M87;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ULONG __RPC_FAR *pcUniqueNames;
    ULONG __RPC_FAR *pcchUniqueNames;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pcUniqueNames = 0;
    pcchUniqueNames = 0;
    RpcTryFinally
        {
        pcUniqueNames = &_M86;
        pcchUniqueNames = &_M87;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib2_GetLibStatistics_Stub(
                                          (ITypeLib2 *) ((CStdStubBuffer *)This)->pvServerObject,
                                          pcUniqueNames,
                                          pcchUniqueNames);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( ULONG __RPC_FAR * )_StubMsg.Buffer = *pcUniqueNames;
        _StubMsg.Buffer += sizeof(ULONG);
        
        *( ULONG __RPC_FAR * )_StubMsg.Buffer = *pcchUniqueNames;
        _StubMsg.Buffer += sizeof(ULONG);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_RemoteGetDocumentation2_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pbstrHelpString)
        {
        MIDL_memset(
               pbstrHelpString,
               0,
               sizeof( BSTR  ));
        }
    if(pbstrHelpStringDll)
        {
        MIDL_memset(
               pbstrHelpStringDll,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      15);
        
        
        
        if(!pbstrHelpString)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pdwHelpStringContext)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!pbstrHelpStringDll)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( INT __RPC_FAR * )_StubMsg.Buffer = index;
            _StubMsg.Buffer += sizeof(INT);
            
            *( LCID __RPC_FAR * )_StubMsg.Buffer = lcid;
            _StubMsg.Buffer += sizeof(LCID);
            
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = refPtrFlags;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[334] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pbstrHelpString,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *pdwHelpStringContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pbstrHelpStringDll,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pbstrHelpString);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pdwHelpStringContext);
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pbstrHelpStringDll);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib2_RemoteGetDocumentation2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M88;
    DWORD _M89;
    BSTR _M90;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    INT index;
    LCID lcid;
    BSTR __RPC_FAR *pbstrHelpString;
    BSTR __RPC_FAR *pbstrHelpStringDll;
    DWORD __RPC_FAR *pdwHelpStringContext;
    DWORD refPtrFlags;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pbstrHelpString = 0;
    pdwHelpStringContext = 0;
    pbstrHelpStringDll = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[334] );
        
        index = *( INT __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(INT);
        
        lcid = *( LCID __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(LCID);
        
        refPtrFlags = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        pbstrHelpString = &_M88;
        MIDL_memset(
               pbstrHelpString,
               0,
               sizeof( BSTR  ));
        pdwHelpStringContext = &_M89;
        pbstrHelpStringDll = &_M90;
        MIDL_memset(
               pbstrHelpStringDll,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        
        
        _RetVal = ITypeLib2_GetDocumentation2_Stub(
                                           (ITypeLib2 *) ((CStdStubBuffer *)This)->pvServerObject,
                                           index,
                                           lcid,
                                           refPtrFlags,
                                           pbstrHelpString,
                                           pdwHelpStringContext,
                                           pbstrHelpStringDll);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U + 11U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pbstrHelpString,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pbstrHelpString,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( DWORD __RPC_FAR * )_StubMsg.Buffer = *pdwHelpStringContext;
        _StubMsg.Buffer += sizeof(DWORD);
        
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pbstrHelpString,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pbstrHelpStringDll,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ITypeLib2_GetAllCustData_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pCustData)
        {
        MIDL_memset(
               pCustData,
               0,
               sizeof( CUSTDATA  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      16);
        
        
        
        if(!pCustData)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[354] );
            
            NdrComplexStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                        (unsigned char __RPC_FAR * __RPC_FAR *)&pCustData,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788],
                                        (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1748],
                         ( void __RPC_FAR * )pCustData);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeLib2_GetAllCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    CUSTDATA _pCustDataM;
    CUSTDATA __RPC_FAR *pCustData;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pCustData = 0;
    RpcTryFinally
        {
        pCustData = &_pCustDataM;
        pCustData -> prgCustData = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeLib2*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetAllCustData((ITypeLib2 *) ((CStdStubBuffer *)This)->pvServerObject,pCustData);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrComplexStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                    (unsigned char __RPC_FAR *)pCustData,
                                    (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrComplexStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                  (unsigned char __RPC_FAR *)pCustData,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1788] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pCustData,
                        &__MIDL_TypeFormatString.Format[1748] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(17) _ITypeLib2ProxyVtbl = 
{
    { &IID_ITypeLib2 },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeLib_GetTypeInfoCount_Proxy ,
        ITypeLib_GetTypeInfo_Proxy ,
        ITypeLib_GetTypeInfoType_Proxy ,
        ITypeLib_GetTypeInfoOfGuid_Proxy ,
        ITypeLib_GetLibAttr_Proxy ,
        ITypeLib_GetTypeComp_Proxy ,
        ITypeLib_GetDocumentation_Proxy ,
        ITypeLib_IsName_Proxy ,
        ITypeLib_FindName_Proxy ,
        ITypeLib_ReleaseTLibAttr_Proxy ,
        ITypeLib2_GetCustData_Proxy ,
        ITypeLib2_GetLibStatistics_Proxy ,
        ITypeLib2_GetDocumentation2_Proxy ,
        ITypeLib2_GetAllCustData_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeLib2_table[] =
{
    ITypeLib_RemoteGetTypeInfoCount_Stub,
    ITypeLib_GetTypeInfo_Stub,
    ITypeLib_GetTypeInfoType_Stub,
    ITypeLib_GetTypeInfoOfGuid_Stub,
    ITypeLib_RemoteGetLibAttr_Stub,
    ITypeLib_GetTypeComp_Stub,
    ITypeLib_RemoteGetDocumentation_Stub,
    ITypeLib_RemoteIsName_Stub,
    ITypeLib_RemoteFindName_Stub,
    ITypeLib_LocalReleaseTLibAttr_Stub,
    ITypeLib2_GetCustData_Stub,
    ITypeLib2_RemoteGetLibStatistics_Stub,
    ITypeLib2_RemoteGetDocumentation2_Stub,
    ITypeLib2_GetAllCustData_Stub
};

const CInterfaceStubVtbl _ITypeLib2StubVtbl =
{
    {
        &IID_ITypeLib2,
        0,
        17,
        &ITypeLib2_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeChangeEvents, ver. 0.0,
   GUID={0x00020410,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IErrorInfo, ver. 0.0,
   GUID={0x1CF2B120,0x547D,0x101B,{0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE IErrorInfo_GetGUID_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pGUID)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pGUID)
        {
        MIDL_memset(
               pGUID,
               0,
               sizeof( IID  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!pGUID)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[456] );
            
            NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&pGUID,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                       (unsigned char)0 );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1892],
                         ( void __RPC_FAR * )pGUID);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IErrorInfo_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    IID _pGUIDM;
    GUID __RPC_FAR *pGUID;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pGUID = 0;
    RpcTryFinally
        {
        pGUID = &_pGUIDM;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetGUID((IErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,pGUID);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)pGUID,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)pGUID,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IErrorInfo_GetSource_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrSource)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrSource)
        {
        MIDL_memset(
               pBstrSource,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!pBstrSource)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[462] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrSource,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrSource);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IErrorInfo_GetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M91;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    BSTR __RPC_FAR *pBstrSource;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrSource = 0;
    RpcTryFinally
        {
        pBstrSource = &_M91;
        MIDL_memset(
               pBstrSource,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetSource((IErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,pBstrSource);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrSource,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrSource,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrSource,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IErrorInfo_GetDescription_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrDescription)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrDescription)
        {
        MIDL_memset(
               pBstrDescription,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        if(!pBstrDescription)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[462] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrDescription,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrDescription);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IErrorInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M92;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    BSTR __RPC_FAR *pBstrDescription;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrDescription = 0;
    RpcTryFinally
        {
        pBstrDescription = &_M92;
        MIDL_memset(
               pBstrDescription,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetDescription((IErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,pBstrDescription);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrDescription,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrDescription,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrDescription,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IErrorInfo_GetHelpFile_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(pBstrHelpFile)
        {
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!pBstrHelpFile)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[462] );
            
            NdrUserMarshalUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&pBstrHelpFile,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128],
                                      (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1708],
                         ( void __RPC_FAR * )pBstrHelpFile);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IErrorInfo_GetHelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    BSTR _M93;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    BSTR __RPC_FAR *pBstrHelpFile;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pBstrHelpFile = 0;
    RpcTryFinally
        {
        pBstrHelpFile = &_M93;
        MIDL_memset(
               pBstrHelpFile,
               0,
               sizeof( BSTR  ));
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetHelpFile((IErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,pBstrHelpFile);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 8U + 11U;
        NdrUserMarshalBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR *)pBstrHelpFile,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        NdrUserMarshalMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                (unsigned char __RPC_FAR *)pBstrHelpFile,
                                (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1128] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)pBstrHelpFile,
                        &__MIDL_TypeFormatString.Format[1708] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE IErrorInfo_GetHelpContext_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      7);
        
        
        
        if(!pdwHelpContext)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            *pdwHelpContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(DWORD);
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[2],
                         ( void __RPC_FAR * )pdwHelpContext);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB IErrorInfo_GetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    DWORD _M94;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    DWORD __RPC_FAR *pdwHelpContext;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pdwHelpContext = 0;
    RpcTryFinally
        {
        pdwHelpContext = &_M94;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((IErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> GetHelpContext((IErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,pdwHelpContext);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( DWORD __RPC_FAR * )_StubMsg.Buffer = *pdwHelpContext;
        _StubMsg.Buffer += sizeof(DWORD);
        
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(8) _IErrorInfoProxyVtbl = 
{
    { &IID_IErrorInfo },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        IErrorInfo_GetGUID_Proxy ,
        IErrorInfo_GetSource_Proxy ,
        IErrorInfo_GetDescription_Proxy ,
        IErrorInfo_GetHelpFile_Proxy ,
        IErrorInfo_GetHelpContext_Proxy
    }
};


static const PRPC_STUB_FUNCTION IErrorInfo_table[] =
{
    IErrorInfo_GetGUID_Stub,
    IErrorInfo_GetSource_Stub,
    IErrorInfo_GetDescription_Stub,
    IErrorInfo_GetHelpFile_Stub,
    IErrorInfo_GetHelpContext_Stub
};

static const CInterfaceStubVtbl _IErrorInfoStubVtbl =
{
    {
        &IID_IErrorInfo,
        0,
        8,
        &IErrorInfo_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ICreateErrorInfo, ver. 0.0,
   GUID={0x22F03340,0x547D,0x101B,{0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetGUID_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!rguid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)rguid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)rguid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[468] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ICreateErrorInfo_SetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    REFGUID rguid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[468] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&rguid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ICreateErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SetGUID((ICreateErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,rguid);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetSource_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szSource)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        if(!szSource)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szSource,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szSource,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ICreateErrorInfo_SetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    LPOLESTR szSource;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szSource = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szSource,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ICreateErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SetSource((ICreateErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,szSource);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetDescription_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szDescription)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        if(!szDescription)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szDescription,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szDescription,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ICreateErrorInfo_SetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    LPOLESTR szDescription;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szDescription = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szDescription,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ICreateErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SetDescription((ICreateErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,szDescription);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetHelpFile_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szHelpFile)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        if(!szHelpFile)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 12U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)szHelpFile,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)szHelpFile,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ICreateErrorInfo_SetHelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    LPOLESTR szHelpFile;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    szHelpFile = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[474] );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&szHelpFile,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1248],
                                       (unsigned char)0 );
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ICreateErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SetHelpFile((ICreateErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,szHelpFile);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetHelpContext_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ DWORD dwHelpContext)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      7);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *( DWORD __RPC_FAR * )_StubMsg.Buffer = dwHelpContext;
            _StubMsg.Buffer += sizeof(DWORD);
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[84] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ICreateErrorInfo_SetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    DWORD dwHelpContext;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[84] );
        
        dwHelpContext = *( DWORD __RPC_FAR * )_StubMsg.Buffer;
        _StubMsg.Buffer += sizeof(DWORD);
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ICreateErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SetHelpContext((ICreateErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,dwHelpContext);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(8) _ICreateErrorInfoProxyVtbl = 
{
    { &IID_ICreateErrorInfo },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ICreateErrorInfo_SetGUID_Proxy ,
        ICreateErrorInfo_SetSource_Proxy ,
        ICreateErrorInfo_SetDescription_Proxy ,
        ICreateErrorInfo_SetHelpFile_Proxy ,
        ICreateErrorInfo_SetHelpContext_Proxy
    }
};


static const PRPC_STUB_FUNCTION ICreateErrorInfo_table[] =
{
    ICreateErrorInfo_SetGUID_Stub,
    ICreateErrorInfo_SetSource_Stub,
    ICreateErrorInfo_SetDescription_Stub,
    ICreateErrorInfo_SetHelpFile_Stub,
    ICreateErrorInfo_SetHelpContext_Stub
};

static const CInterfaceStubVtbl _ICreateErrorInfoStubVtbl =
{
    {
        &IID_ICreateErrorInfo,
        0,
        8,
        &ICreateErrorInfo_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ISupportErrorInfo, ver. 0.0,
   GUID={0xDF0B3D60,0x548F,0x101B,{0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE ISupportErrorInfo_InterfaceSupportsErrorInfo_Proxy( 
    ISupportErrorInfo __RPC_FAR * This,
    /* [in] */ REFIID riid)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!riid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)riid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)riid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[468] );
            
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ISupportErrorInfo_InterfaceSupportsErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    IID* riid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[468] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&riid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ISupportErrorInfo*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> InterfaceSupportsErrorInfo((ISupportErrorInfo *) ((CStdStubBuffer *)This)->pvServerObject,riid);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const CINTERFACE_PROXY_VTABLE(4) _ISupportErrorInfoProxyVtbl = 
{
    { &IID_ISupportErrorInfo },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ISupportErrorInfo_InterfaceSupportsErrorInfo_Proxy
    }
};


static const PRPC_STUB_FUNCTION ISupportErrorInfo_table[] =
{
    ISupportErrorInfo_InterfaceSupportsErrorInfo_Stub
};

static const CInterfaceStubVtbl _ISupportErrorInfoStubVtbl =
{
    {
        &IID_ISupportErrorInfo,
        0,
        4,
        &ISupportErrorInfo_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeFactory, ver. 0.0,
   GUID={0x0000002E,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


extern const MIDL_STUB_DESC Object_StubDesc;


HRESULT STDMETHODCALLTYPE ITypeFactory_CreateFromTypeInfo_Proxy( 
    ITypeFactory __RPC_FAR * This,
    /* [in] */ ITypeInfo __RPC_FAR *pTypeInfo,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppv)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    if(ppv)
        {
        MIDL_memset(
               ppv,
               0,
               sizeof( IUnknown __RPC_FAR *__RPC_FAR * ));
        }
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!riid)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!ppv)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U + 0U;
            NdrInterfacePointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)pTypeInfo,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[10] );
            
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)riid,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrInterfacePointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)pTypeInfo,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[10] );
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)riid,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38] );
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[480] );
            
            NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                  (unsigned char __RPC_FAR * __RPC_FAR *)&ppv,
                                  (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1896],
                                  (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            _RetVal = *( HRESULT __RPC_FAR * )_StubMsg.Buffer;
            _StubMsg.Buffer += sizeof(HRESULT);
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[1896],
                         ( void __RPC_FAR * )ppv);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB ITypeFactory_CreateFromTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    IUnknown __RPC_FAR *__RPC_FAR *_M101;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    ITypeInfo __RPC_FAR *pTypeInfo;
    IUnknown __RPC_FAR *__RPC_FAR *ppv;
    IID* riid = 0;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    pTypeInfo = 0;
    ppv = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[480] );
        
        NdrInterfacePointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&pTypeInfo,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[10],
                                       (unsigned char)0 );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&riid,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[38],
                                   (unsigned char)0 );
        
        ppv = (void *)&_M101;
        _M101 = 0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((ITypeFactory*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> CreateFromTypeInfo(
                      (ITypeFactory *) ((CStdStubBuffer *)This)->pvServerObject,
                      pTypeInfo,
                      riid,
                      ppv);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 0U + 4U;
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ppv,
                              (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1896] );
        
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ppv,
                            (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[1896] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *( HRESULT __RPC_FAR * )_StubMsg.Buffer = _RetVal;
        _StubMsg.Buffer += sizeof(HRESULT);
        
        }
    RpcFinally
        {
        NdrInterfacePointerFree( &_StubMsg,
                                 (unsigned char __RPC_FAR *)pTypeInfo,
                                 &__MIDL_TypeFormatString.Format[10] );
        
        _StubMsg.MaxCount = (unsigned long) ( riid );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ppv,
                        &__MIDL_TypeFormatString.Format[1896] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[3] = 
        {
            {
                (USER_MARSHAL_SIZING_ROUTINE)VARIANT_UserSize,
                (USER_MARSHAL_MARSHALLING_ROUTINE)VARIANT_UserMarshal,
                (USER_MARSHAL_UNMARSHALLING_ROUTINE)VARIANT_UserUnmarshal,
                (USER_MARSHAL_FREEING_ROUTINE)VARIANT_UserFree
            },
            {
                (USER_MARSHAL_SIZING_ROUTINE)BSTR_UserSize,
                (USER_MARSHAL_MARSHALLING_ROUTINE)BSTR_UserMarshal,
                (USER_MARSHAL_UNMARSHALLING_ROUTINE)BSTR_UserUnmarshal,
                (USER_MARSHAL_FREEING_ROUTINE)BSTR_UserFree
            },
            {
                (USER_MARSHAL_SIZING_ROUTINE)CLEANLOCALSTORAGE_UserSize,
                (USER_MARSHAL_MARSHALLING_ROUTINE)CLEANLOCALSTORAGE_UserMarshal,
                (USER_MARSHAL_UNMARSHALLING_ROUTINE)CLEANLOCALSTORAGE_UserUnmarshal,
                (USER_MARSHAL_FREEING_ROUTINE)CLEANLOCALSTORAGE_UserFree
            }

        };

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    { 0 },
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static const CINTERFACE_PROXY_VTABLE(4) _ITypeFactoryProxyVtbl = 
{
    { &IID_ITypeFactory },
    {
        IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        ITypeFactory_CreateFromTypeInfo_Proxy
    }
};


static const PRPC_STUB_FUNCTION ITypeFactory_table[] =
{
    ITypeFactory_CreateFromTypeInfo_Stub
};

static const CInterfaceStubVtbl _ITypeFactoryStubVtbl =
{
    {
        &IID_ITypeFactory,
        0,
        4,
        &ITypeFactory_table[-3]
    },
    { CStdStubBuffer_METHODS }
};


/* Object interface: ITypeMarshal, ver. 0.0,
   GUID={0x0000002D,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IRecordInfo, ver. 0.0,
   GUID={0x0000002F,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICreateTypeInfo, ver. 0.0,
   GUID={0x00020405,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICreateTypeInfo2, ver. 0.0,
   GUID={0x0002040E,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICreateTypeLib, ver. 0.0,
   GUID={0x00020406,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICreateTypeLib2, ver. 0.0,
   GUID={0x0002040F,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   [wire_marshal] or [user_marshal] attribute.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {
			
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/*  2 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/*  4 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/*  6 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/*  8 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 10 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 12 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */
/* 14 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 16 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 18 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 20 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 22 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */
/* 24 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 26 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 28 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 30 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */
/* 32 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 34 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 36 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 38 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 40 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 42 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 44 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 46 */	NdrFcShort( 0x62 ),	/* Type Offset=98 */
/* 48 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 50 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */
/* 52 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 54 */	NdrFcShort( 0x460 ),	/* Type Offset=1120 */
/* 56 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 58 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 60 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 62 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 64 */	NdrFcShort( 0x48e ),	/* Type Offset=1166 */
/* 66 */	
			0x50,		/* FC_IN_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 68 */	NdrFcShort( 0x49c ),	/* Type Offset=1180 */
/* 70 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 72 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 74 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 76 */	NdrFcShort( 0x4b2 ),	/* Type Offset=1202 */
/* 78 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 80 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 82 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 84 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 86 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 88 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 90 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 92 */	NdrFcShort( 0x4c8 ),	/* Type Offset=1224 */
/* 94 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 96 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 98 */	NdrFcShort( 0x4de ),	/* Type Offset=1246 */
/* 100 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 102 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x6,		/* FC_SHORT */
/* 104 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 106 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */
/* 108 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 110 */	NdrFcShort( 0x4e2 ),	/* Type Offset=1250 */
/* 112 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 114 */	NdrFcShort( 0x4e6 ),	/* Type Offset=1254 */
/* 116 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 118 */	NdrFcShort( 0x5b8 ),	/* Type Offset=1464 */
/* 120 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 122 */	NdrFcShort( 0x604 ),	/* Type Offset=1540 */
/* 124 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 126 */	NdrFcShort( 0x61a ),	/* Type Offset=1562 */
/* 128 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 130 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 132 */	NdrFcShort( 0x4de ),	/* Type Offset=1246 */
/* 134 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 136 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 138 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */
/* 140 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 142 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 144 */	NdrFcShort( 0x62a ),	/* Type Offset=1578 */
/* 146 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 148 */	NdrFcShort( 0x666 ),	/* Type Offset=1638 */
/* 150 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 152 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 154 */	NdrFcShort( 0x604 ),	/* Type Offset=1540 */
/* 156 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 158 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 160 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 162 */	NdrFcShort( 0x4e6 ),	/* Type Offset=1254 */
/* 164 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 166 */	NdrFcShort( 0x676 ),	/* Type Offset=1654 */
/* 168 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 170 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 172 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 174 */	NdrFcShort( 0x5b8 ),	/* Type Offset=1464 */
/* 176 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 178 */	NdrFcShort( 0x686 ),	/* Type Offset=1670 */
/* 180 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 182 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 184 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 186 */	NdrFcShort( 0x696 ),	/* Type Offset=1686 */
/* 188 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 190 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 192 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 194 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 196 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 198 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 200 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 202 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 204 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 206 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 208 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 210 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 212 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 214 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 216 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 218 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 220 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 222 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 224 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 226 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 228 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xe,		/* FC_ENUM32 */
/* 230 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 232 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 234 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 236 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 238 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 240 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 242 */	NdrFcShort( 0x6b0 ),	/* Type Offset=1712 */
/* 244 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 246 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 248 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 250 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */
/* 252 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 254 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 256 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 258 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 260 */	NdrFcShort( 0x6b4 ),	/* Type Offset=1716 */
/* 262 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 264 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 266 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 268 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 270 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 272 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 274 */	NdrFcShort( 0x6be ),	/* Type Offset=1726 */
/* 276 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 278 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 280 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 282 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 284 */	NdrFcShort( 0x4e2 ),	/* Type Offset=1250 */
/* 286 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 288 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 290 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xe,		/* FC_ENUM32 */
/* 292 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 294 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 296 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 298 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 300 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 302 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 304 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */
/* 306 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 308 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 310 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 312 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 314 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 316 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */
/* 318 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 320 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 322 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 324 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 326 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 328 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 330 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */
/* 332 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 334 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 336 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 338 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 340 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 342 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 344 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 346 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 348 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 350 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 352 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 354 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 356 */	NdrFcShort( 0x6d4 ),	/* Type Offset=1748 */
/* 358 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 360 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 362 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 364 */	NdrFcShort( 0x6d4 ),	/* Type Offset=1748 */
/* 366 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 368 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 370 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 372 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 374 */	NdrFcShort( 0x6d4 ),	/* Type Offset=1748 */
/* 376 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 378 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 380 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 382 */	NdrFcShort( 0x4e2 ),	/* Type Offset=1250 */
/* 384 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 386 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 388 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 390 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 392 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */
/* 394 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 396 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 398 */	NdrFcShort( 0x70c ),	/* Type Offset=1804 */
/* 400 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 402 */	NdrFcShort( 0x728 ),	/* Type Offset=1832 */
/* 404 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 406 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 408 */	NdrFcShort( 0x4de ),	/* Type Offset=1246 */
/* 410 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 412 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 414 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 416 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 418 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 420 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 422 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 424 */	NdrFcShort( 0x4de ),	/* Type Offset=1246 */
/* 426 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 428 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 430 */	NdrFcShort( 0x738 ),	/* Type Offset=1848 */
/* 432 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 434 */	NdrFcShort( 0x74e ),	/* Type Offset=1870 */
/* 436 */	
			0x50,		/* FC_IN_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 438 */	NdrFcShort( 0x760 ),	/* Type Offset=1888 */
/* 440 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 442 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 444 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 446 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 448 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 450 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 452 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 454 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 456 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 458 */	NdrFcShort( 0x764 ),	/* Type Offset=1892 */
/* 460 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 462 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 464 */	NdrFcShort( 0x6ac ),	/* Type Offset=1708 */
/* 466 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 468 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 470 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 472 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 474 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 476 */	NdrFcShort( 0x4de ),	/* Type Offset=1246 */
/* 478 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 480 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 482 */	NdrFcShort( 0xa ),	/* Type Offset=10 */
/* 484 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 486 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */
/* 488 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 490 */	NdrFcShort( 0x768 ),	/* Type Offset=1896 */
/* 492 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 12 */	NdrFcLong( 0x20401 ),	/* 132097 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 22 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 24 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 26 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 28 */	
			0x11, 0x0,	/* FC_RP */
/* 30 */	NdrFcShort( 0x8 ),	/* Offset= 8 (38) */
/* 32 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 34 */	NdrFcShort( 0x8 ),	/* 8 */
/* 36 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 38 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 40 */	NdrFcShort( 0x10 ),	/* 16 */
/* 42 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 44 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 46 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (32) */
			0x5b,		/* FC_END */
/* 50 */	
			0x11, 0x0,	/* FC_RP */
/* 52 */	NdrFcShort( 0x2 ),	/* Offset= 2 (54) */
/* 54 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 56 */	NdrFcShort( 0x4 ),	/* 4 */
/* 58 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 60 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 62 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 64 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 66 */	NdrFcShort( 0x4 ),	/* 4 */
/* 68 */	NdrFcShort( 0x0 ),	/* 0 */
/* 70 */	NdrFcShort( 0x1 ),	/* 1 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */
/* 76 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 78 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 80 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 82 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 84 */	
			0x11, 0x0,	/* FC_RP */
/* 86 */	NdrFcShort( 0x2 ),	/* Offset= 2 (88) */
/* 88 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 90 */	NdrFcShort( 0x4 ),	/* 4 */
/* 92 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 94 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 96 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 98 */	
			0x11, 0x0,	/* FC_RP */
/* 100 */	NdrFcShort( 0x3d4 ),	/* Offset= 980 (1080) */
/* 102 */	
			0x12, 0x0,	/* FC_UP */
/* 104 */	NdrFcShort( 0x396 ),	/* Offset= 918 (1022) */
/* 106 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x7,		/* FC_USHORT */
/* 108 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 110 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 112 */	NdrFcShort( 0x2 ),	/* Offset= 2 (114) */
/* 114 */	NdrFcShort( 0x10 ),	/* 16 */
/* 116 */	NdrFcShort( 0x2b ),	/* 43 */
/* 118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 122 */	NdrFcShort( 0x0 ),	/* Offset= 0 (122) */
/* 124 */	NdrFcLong( 0x1 ),	/* 1 */
/* 128 */	NdrFcShort( 0x0 ),	/* Offset= 0 (128) */
/* 130 */	NdrFcLong( 0x10 ),	/* 16 */
/* 134 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 136 */	NdrFcLong( 0x12 ),	/* 18 */
/* 140 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 142 */	NdrFcLong( 0x13 ),	/* 19 */
/* 146 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 148 */	NdrFcLong( 0x16 ),	/* 22 */
/* 152 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 154 */	NdrFcLong( 0x17 ),	/* 23 */
/* 158 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 160 */	NdrFcLong( 0x11 ),	/* 17 */
/* 164 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 166 */	NdrFcLong( 0x2 ),	/* 2 */
/* 170 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 172 */	NdrFcLong( 0x3 ),	/* 3 */
/* 176 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 178 */	NdrFcLong( 0x4 ),	/* 4 */
/* 182 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 184 */	NdrFcLong( 0x5 ),	/* 5 */
/* 188 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 190 */	NdrFcLong( 0xb ),	/* 11 */
/* 194 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 196 */	NdrFcLong( 0xa ),	/* 10 */
/* 200 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 202 */	NdrFcLong( 0x7 ),	/* 7 */
/* 206 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 208 */	NdrFcLong( 0x8 ),	/* 8 */
/* 212 */	NdrFcShort( 0xa6 ),	/* Offset= 166 (378) */
/* 214 */	NdrFcLong( 0x6 ),	/* 6 */
/* 218 */	NdrFcShort( 0xb8 ),	/* Offset= 184 (402) */
/* 220 */	NdrFcLong( 0xe ),	/* 14 */
/* 224 */	NdrFcShort( 0xb8 ),	/* Offset= 184 (408) */
/* 226 */	NdrFcLong( 0xd ),	/* 13 */
/* 230 */	NdrFcShort( 0xbe ),	/* Offset= 190 (420) */
/* 232 */	NdrFcLong( 0x9 ),	/* 9 */
/* 236 */	NdrFcShort( 0xca ),	/* Offset= 202 (438) */
/* 238 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 242 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (456) */
/* 244 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 248 */	NdrFcShort( 0x2ce ),	/* Offset= 718 (966) */
/* 250 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 254 */	NdrFcShort( 0x2cc ),	/* Offset= 716 (970) */
/* 256 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 260 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (974) */
/* 262 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 266 */	NdrFcShort( 0x2c4 ),	/* Offset= 708 (974) */
/* 268 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 272 */	NdrFcShort( 0x2be ),	/* Offset= 702 (974) */
/* 274 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 278 */	NdrFcShort( 0x2b0 ),	/* Offset= 688 (966) */
/* 280 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 284 */	NdrFcShort( 0x2ae ),	/* Offset= 686 (970) */
/* 286 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 290 */	NdrFcShort( 0x2ac ),	/* Offset= 684 (974) */
/* 292 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 296 */	NdrFcShort( 0x2aa ),	/* Offset= 682 (978) */
/* 298 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 302 */	NdrFcShort( 0x2a8 ),	/* Offset= 680 (982) */
/* 304 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 308 */	NdrFcShort( 0x296 ),	/* Offset= 662 (970) */
/* 310 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 314 */	NdrFcShort( 0x294 ),	/* Offset= 660 (974) */
/* 316 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 320 */	NdrFcShort( 0x296 ),	/* Offset= 662 (982) */
/* 322 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 326 */	NdrFcShort( 0x294 ),	/* Offset= 660 (986) */
/* 328 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 332 */	NdrFcShort( 0x292 ),	/* Offset= 658 (990) */
/* 334 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 338 */	NdrFcShort( 0x294 ),	/* Offset= 660 (998) */
/* 340 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 344 */	NdrFcShort( 0x292 ),	/* Offset= 658 (1002) */
/* 346 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 350 */	NdrFcShort( 0x290 ),	/* Offset= 656 (1006) */
/* 352 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 356 */	NdrFcShort( 0x28e ),	/* Offset= 654 (1010) */
/* 358 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 362 */	NdrFcShort( 0x28c ),	/* Offset= 652 (1014) */
/* 364 */	NdrFcLong( 0x24 ),	/* 36 */
/* 368 */	NdrFcShort( 0x28a ),	/* Offset= 650 (1018) */
/* 370 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 374 */	NdrFcShort( 0x284 ),	/* Offset= 644 (1018) */
/* 376 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (375) */
/* 378 */	
			0x12, 0x0,	/* FC_UP */
/* 380 */	NdrFcShort( 0xc ),	/* Offset= 12 (392) */
/* 382 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 384 */	NdrFcShort( 0x2 ),	/* 2 */
/* 386 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 388 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 390 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 392 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 394 */	NdrFcShort( 0x8 ),	/* 8 */
/* 396 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (382) */
/* 398 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 400 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 402 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 404 */	NdrFcShort( 0x8 ),	/* 8 */
/* 406 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 408 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 410 */	NdrFcShort( 0x10 ),	/* 16 */
/* 412 */	0x6,		/* FC_SHORT */
			0x2,		/* FC_CHAR */
/* 414 */	0x2,		/* FC_CHAR */
			0x38,		/* FC_ALIGNM4 */
/* 416 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 418 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 420 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 422 */	NdrFcLong( 0x0 ),	/* 0 */
/* 426 */	NdrFcShort( 0x0 ),	/* 0 */
/* 428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 430 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 432 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 434 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 436 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 438 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 440 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 448 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 450 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 452 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 454 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 456 */	
			0x12, 0x0,	/* FC_UP */
/* 458 */	NdrFcShort( 0x1ea ),	/* Offset= 490 (948) */
/* 460 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 462 */	NdrFcShort( 0x18 ),	/* 24 */
/* 464 */	NdrFcShort( 0xa ),	/* 10 */
/* 466 */	NdrFcLong( 0x8 ),	/* 8 */
/* 470 */	NdrFcShort( 0x58 ),	/* Offset= 88 (558) */
/* 472 */	NdrFcLong( 0xd ),	/* 13 */
/* 476 */	NdrFcShort( 0x78 ),	/* Offset= 120 (596) */
/* 478 */	NdrFcLong( 0x9 ),	/* 9 */
/* 482 */	NdrFcShort( 0x94 ),	/* Offset= 148 (630) */
/* 484 */	NdrFcLong( 0xc ),	/* 12 */
/* 488 */	NdrFcShort( 0xbc ),	/* Offset= 188 (676) */
/* 490 */	NdrFcLong( 0x24 ),	/* 36 */
/* 494 */	NdrFcShort( 0x114 ),	/* Offset= 276 (770) */
/* 496 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 500 */	NdrFcShort( 0x11e ),	/* Offset= 286 (786) */
/* 502 */	NdrFcLong( 0x10 ),	/* 16 */
/* 506 */	NdrFcShort( 0x136 ),	/* Offset= 310 (816) */
/* 508 */	NdrFcLong( 0x2 ),	/* 2 */
/* 512 */	NdrFcShort( 0x14e ),	/* Offset= 334 (846) */
/* 514 */	NdrFcLong( 0x3 ),	/* 3 */
/* 518 */	NdrFcShort( 0x166 ),	/* Offset= 358 (876) */
/* 520 */	NdrFcLong( 0x14 ),	/* 20 */
/* 524 */	NdrFcShort( 0x17e ),	/* Offset= 382 (906) */
/* 526 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (525) */
/* 528 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 530 */	NdrFcShort( 0x4 ),	/* 4 */
/* 532 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 536 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 538 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 540 */	NdrFcShort( 0x4 ),	/* 4 */
/* 542 */	NdrFcShort( 0x0 ),	/* 0 */
/* 544 */	NdrFcShort( 0x1 ),	/* 1 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	NdrFcShort( 0x0 ),	/* 0 */
/* 550 */	0x12, 0x0,	/* FC_UP */
/* 552 */	NdrFcShort( 0xffffff60 ),	/* Offset= -160 (392) */
/* 554 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 556 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 558 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 560 */	NdrFcShort( 0x8 ),	/* 8 */
/* 562 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 564 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 566 */	NdrFcShort( 0x4 ),	/* 4 */
/* 568 */	NdrFcShort( 0x4 ),	/* 4 */
/* 570 */	0x11, 0x0,	/* FC_RP */
/* 572 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (528) */
/* 574 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 576 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 578 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 582 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 584 */	NdrFcShort( 0x0 ),	/* 0 */
/* 586 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 590 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 592 */	NdrFcShort( 0xffffff54 ),	/* Offset= -172 (420) */
/* 594 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 596 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 598 */	NdrFcShort( 0x8 ),	/* 8 */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 602 */	NdrFcShort( 0x6 ),	/* Offset= 6 (608) */
/* 604 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 606 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 608 */	
			0x11, 0x0,	/* FC_RP */
/* 610 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (578) */
/* 612 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 614 */	NdrFcShort( 0x0 ),	/* 0 */
/* 616 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 620 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 624 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 626 */	NdrFcShort( 0xffffff44 ),	/* Offset= -188 (438) */
/* 628 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 630 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0x6 ),	/* Offset= 6 (642) */
/* 638 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 640 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 642 */	
			0x11, 0x0,	/* FC_RP */
/* 644 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (612) */
/* 646 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 648 */	NdrFcShort( 0x4 ),	/* 4 */
/* 650 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 654 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 656 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 658 */	NdrFcShort( 0x4 ),	/* 4 */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 662 */	NdrFcShort( 0x1 ),	/* 1 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 668 */	0x12, 0x0,	/* FC_UP */
/* 670 */	NdrFcShort( 0x160 ),	/* Offset= 352 (1022) */
/* 672 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 674 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 676 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 678 */	NdrFcShort( 0x8 ),	/* 8 */
/* 680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 682 */	NdrFcShort( 0x6 ),	/* Offset= 6 (688) */
/* 684 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 686 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 688 */	
			0x11, 0x0,	/* FC_RP */
/* 690 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (646) */
/* 692 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 694 */	NdrFcLong( 0x2f ),	/* 47 */
/* 698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 702 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 704 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 706 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 708 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 710 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 712 */	NdrFcShort( 0x1 ),	/* 1 */
/* 714 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 716 */	NdrFcShort( 0x4 ),	/* 4 */
/* 718 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 720 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 722 */	NdrFcShort( 0x10 ),	/* 16 */
/* 724 */	NdrFcShort( 0x0 ),	/* 0 */
/* 726 */	NdrFcShort( 0xa ),	/* Offset= 10 (736) */
/* 728 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 730 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 732 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (692) */
/* 734 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 736 */	
			0x12, 0x0,	/* FC_UP */
/* 738 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (710) */
/* 740 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 742 */	NdrFcShort( 0x4 ),	/* 4 */
/* 744 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 748 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 750 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 752 */	NdrFcShort( 0x4 ),	/* 4 */
/* 754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 756 */	NdrFcShort( 0x1 ),	/* 1 */
/* 758 */	NdrFcShort( 0x0 ),	/* 0 */
/* 760 */	NdrFcShort( 0x0 ),	/* 0 */
/* 762 */	0x12, 0x0,	/* FC_UP */
/* 764 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (720) */
/* 766 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 768 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 770 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 772 */	NdrFcShort( 0x8 ),	/* 8 */
/* 774 */	NdrFcShort( 0x0 ),	/* 0 */
/* 776 */	NdrFcShort( 0x6 ),	/* Offset= 6 (782) */
/* 778 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 780 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 782 */	
			0x11, 0x0,	/* FC_RP */
/* 784 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (740) */
/* 786 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 788 */	NdrFcShort( 0x18 ),	/* 24 */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */
/* 792 */	NdrFcShort( 0xa ),	/* Offset= 10 (802) */
/* 794 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 796 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 798 */	NdrFcShort( 0xfffffd08 ),	/* Offset= -760 (38) */
/* 800 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 802 */	
			0x11, 0x0,	/* FC_RP */
/* 804 */	NdrFcShort( 0xffffff1e ),	/* Offset= -226 (578) */
/* 806 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 808 */	NdrFcShort( 0x1 ),	/* 1 */
/* 810 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 812 */	NdrFcShort( 0x0 ),	/* 0 */
/* 814 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 816 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 818 */	NdrFcShort( 0x8 ),	/* 8 */
/* 820 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 822 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 824 */	NdrFcShort( 0x4 ),	/* 4 */
/* 826 */	NdrFcShort( 0x4 ),	/* 4 */
/* 828 */	0x12, 0x0,	/* FC_UP */
/* 830 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (806) */
/* 832 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 834 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 836 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 838 */	NdrFcShort( 0x2 ),	/* 2 */
/* 840 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 842 */	NdrFcShort( 0x0 ),	/* 0 */
/* 844 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 846 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 848 */	NdrFcShort( 0x8 ),	/* 8 */
/* 850 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 852 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 854 */	NdrFcShort( 0x4 ),	/* 4 */
/* 856 */	NdrFcShort( 0x4 ),	/* 4 */
/* 858 */	0x12, 0x0,	/* FC_UP */
/* 860 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (836) */
/* 862 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 864 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 866 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 868 */	NdrFcShort( 0x4 ),	/* 4 */
/* 870 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 872 */	NdrFcShort( 0x0 ),	/* 0 */
/* 874 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 876 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 878 */	NdrFcShort( 0x8 ),	/* 8 */
/* 880 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 882 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 884 */	NdrFcShort( 0x4 ),	/* 4 */
/* 886 */	NdrFcShort( 0x4 ),	/* 4 */
/* 888 */	0x12, 0x0,	/* FC_UP */
/* 890 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (866) */
/* 892 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 894 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 896 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 898 */	NdrFcShort( 0x8 ),	/* 8 */
/* 900 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 902 */	NdrFcShort( 0x0 ),	/* 0 */
/* 904 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 906 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 908 */	NdrFcShort( 0x8 ),	/* 8 */
/* 910 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 912 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 914 */	NdrFcShort( 0x4 ),	/* 4 */
/* 916 */	NdrFcShort( 0x4 ),	/* 4 */
/* 918 */	0x12, 0x0,	/* FC_UP */
/* 920 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (896) */
/* 922 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 924 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 926 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 928 */	NdrFcShort( 0x8 ),	/* 8 */
/* 930 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 932 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 934 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 936 */	NdrFcShort( 0x8 ),	/* 8 */
/* 938 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 940 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 942 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 944 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (926) */
/* 946 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 948 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 950 */	NdrFcShort( 0x28 ),	/* 40 */
/* 952 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (934) */
/* 954 */	NdrFcShort( 0x0 ),	/* Offset= 0 (954) */
/* 956 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 958 */	0x38,		/* FC_ALIGNM4 */
			0x8,		/* FC_LONG */
/* 960 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 962 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe09 ),	/* Offset= -503 (460) */
			0x5b,		/* FC_END */
/* 966 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 968 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 970 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 972 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 974 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 976 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 978 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 980 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 982 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 984 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 986 */	
			0x12, 0x10,	/* FC_UP */
/* 988 */	NdrFcShort( 0xfffffd9e ),	/* Offset= -610 (378) */
/* 990 */	
			0x12, 0x10,	/* FC_UP */
/* 992 */	NdrFcShort( 0x2 ),	/* Offset= 2 (994) */
/* 994 */	
			0x12, 0x0,	/* FC_UP */
/* 996 */	NdrFcShort( 0xfffffc1c ),	/* Offset= -996 (0) */
/* 998 */	
			0x12, 0x0,	/* FC_UP */
/* 1000 */	NdrFcShort( 0xfffffdaa ),	/* Offset= -598 (402) */
/* 1002 */	
			0x12, 0x0,	/* FC_UP */
/* 1004 */	NdrFcShort( 0xfffffdac ),	/* Offset= -596 (408) */
/* 1006 */	
			0x12, 0x10,	/* FC_UP */
/* 1008 */	NdrFcShort( 0xfffffdb4 ),	/* Offset= -588 (420) */
/* 1010 */	
			0x12, 0x10,	/* FC_UP */
/* 1012 */	NdrFcShort( 0xfffffdc2 ),	/* Offset= -574 (438) */
/* 1014 */	
			0x12, 0x10,	/* FC_UP */
/* 1016 */	NdrFcShort( 0xfffffdd0 ),	/* Offset= -560 (456) */
/* 1018 */	
			0x12, 0x0,	/* FC_UP */
/* 1020 */	NdrFcShort( 0xfffffed4 ),	/* Offset= -300 (720) */
/* 1022 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 1024 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1026 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1028 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1028) */
/* 1030 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1032 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1034 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1036 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1038 */	NdrFcShort( 0xfffffc5c ),	/* Offset= -932 (106) */
/* 1040 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1042 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1044 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1046 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1050 */	NdrFcShort( 0xfffffc4c ),	/* Offset= -948 (102) */
/* 1052 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1056 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1058 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1060 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1064 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1066 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1042) */
/* 1068 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1070 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1072 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1074 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1076 */	NdrFcShort( 0xc ),	/* 12 */
/* 1078 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1080 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1082 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1084 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1086 */	NdrFcShort( 0x8 ),	/* Offset= 8 (1094) */
/* 1088 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 1090 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1092 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1094 */	
			0x12, 0x0,	/* FC_UP */
/* 1096 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (1052) */
/* 1098 */	
			0x12, 0x0,	/* FC_UP */
/* 1100 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1070) */
/* 1102 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1104 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1110) */
/* 1106 */	
			0x13, 0x0,	/* FC_OP */
/* 1108 */	NdrFcShort( 0xffffffaa ),	/* Offset= -86 (1022) */
/* 1110 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1114 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1118 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1106) */
/* 1120 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1122 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1138) */
/* 1124 */	
			0x13, 0x0,	/* FC_OP */
/* 1126 */	NdrFcShort( 0xfffffd22 ),	/* Offset= -734 (392) */
/* 1128 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1130 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1132 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1136 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1124) */
/* 1138 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1140 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1144 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1144) */
/* 1146 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1148 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1150 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (1128) */
/* 1152 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1154 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (1128) */
/* 1156 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1158 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1128) */
/* 1160 */	0x38,		/* FC_ALIGNM4 */
			0x8,		/* FC_LONG */
/* 1162 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1164 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1166 */	
			0x11, 0x0,	/* FC_RP */
/* 1168 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1170) */
/* 1170 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1172 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1174 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1176 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 1178 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1180 */	
			0x11, 0x0,	/* FC_RP */
/* 1182 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1184) */
/* 1184 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1188 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1190 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 1192 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1196 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1198 */	NdrFcShort( 0xffffffa8 ),	/* Offset= -88 (1110) */
/* 1200 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1202 */	
			0x11, 0x0,	/* FC_RP */
/* 1204 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1206) */
/* 1206 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1210 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1212 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1214 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1216 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1218 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1220 */	NdrFcShort( 0xffffff92 ),	/* Offset= -110 (1110) */
/* 1222 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1224 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1226 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1228) */
/* 1228 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1230 */	NdrFcLong( 0x20404 ),	/* 132100 */
/* 1234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1238 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1240 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1242 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1244 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1246 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1248 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1250 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1252 */	0xe,		/* FC_ENUM32 */
			0x5c,		/* FC_PAD */
/* 1254 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1256 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1258) */
/* 1258 */	
			0x13, 0x0,	/* FC_OP */
/* 1260 */	NdrFcShort( 0xaa ),	/* Offset= 170 (1430) */
/* 1262 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x7,		/* FC_USHORT */
/* 1264 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1266 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1268 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1270) */
/* 1270 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1272 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1274 */	NdrFcLong( 0x1a ),	/* 26 */
/* 1278 */	NdrFcShort( 0x16 ),	/* Offset= 22 (1300) */
/* 1280 */	NdrFcLong( 0x1b ),	/* 27 */
/* 1284 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1300) */
/* 1286 */	NdrFcLong( 0x1c ),	/* 28 */
/* 1290 */	NdrFcShort( 0xe ),	/* Offset= 14 (1304) */
/* 1292 */	NdrFcLong( 0x1d ),	/* 29 */
/* 1296 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1298 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1298) */
/* 1300 */	
			0x13, 0x0,	/* FC_OP */
/* 1302 */	NdrFcShort( 0x24 ),	/* Offset= 36 (1338) */
/* 1304 */	
			0x13, 0x0,	/* FC_OP */
/* 1306 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1322) */
/* 1308 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1310 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1312 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1314 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 1316 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1318 */	NdrFcShort( 0xfffffe78 ),	/* Offset= -392 (926) */
/* 1320 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1322 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1324 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1326 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1308) */
/* 1328 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1328) */
/* 1330 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1332 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1338) */
/* 1334 */	0x6,		/* FC_SHORT */
			0x3e,		/* FC_STRUCTPAD2 */
/* 1336 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1338 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1340 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1342 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1344 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1344) */
/* 1346 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1348 */	NdrFcShort( 0xffffffaa ),	/* Offset= -86 (1262) */
/* 1350 */	0x6,		/* FC_SHORT */
			0x3e,		/* FC_STRUCTPAD2 */
/* 1352 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1354 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1356 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1360 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1360) */
/* 1362 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1364 */	0x4,		/* 4 */
			NdrFcShort( 0xffffff01 ),	/* Offset= -255 (1110) */
			0x5b,		/* FC_END */
/* 1368 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1370 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1374 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1380) */
/* 1376 */	0x36,		/* FC_POINTER */
			0x6,		/* FC_SHORT */
/* 1378 */	0x3e,		/* FC_STRUCTPAD2 */
			0x5b,		/* FC_END */
/* 1380 */	
			0x13, 0x0,	/* FC_OP */
/* 1382 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (1354) */
/* 1384 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1386 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1388 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1390 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1390) */
/* 1392 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1394 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (1338) */
/* 1396 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1398 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (1368) */
/* 1400 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1402 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1404 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1406 */	0x16,		/* Corr desc:  field pointer, FC_SHORT */
			0x0,		/*  */
/* 1408 */	NdrFcShort( 0x1e ),	/* 30 */
/* 1410 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1412 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1414 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1416 */	0x16,		/* Corr desc:  field pointer, FC_SHORT */
			0x0,		/*  */
/* 1418 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1420 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1424 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1426 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (1384) */
/* 1428 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1430 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1432 */	NdrFcShort( 0x34 ),	/* 52 */
/* 1434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1436 */	NdrFcShort( 0x14 ),	/* Offset= 20 (1456) */
/* 1438 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1440 */	0x36,		/* FC_POINTER */
			0xe,		/* FC_ENUM32 */
/* 1442 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 1444 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1446 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1448 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1450 */	NdrFcShort( 0xffffffbe ),	/* Offset= -66 (1384) */
/* 1452 */	0x6,		/* FC_SHORT */
			0x3e,		/* FC_STRUCTPAD2 */
/* 1454 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1456 */	
			0x13, 0x0,	/* FC_OP */
/* 1458 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (1402) */
/* 1460 */	
			0x13, 0x0,	/* FC_OP */
/* 1462 */	NdrFcShort( 0xffffffce ),	/* Offset= -50 (1412) */
/* 1464 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1466 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1468) */
/* 1468 */	
			0x13, 0x0,	/* FC_OP */
/* 1470 */	NdrFcShort( 0x2c ),	/* Offset= 44 (1514) */
/* 1472 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 1474 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 1476 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1478 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1480) */
/* 1480 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1482 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1484 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1488 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1490 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1494 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1496 */	NdrFcLong( 0x1 ),	/* 1 */
/* 1500 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1502 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1506 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1510) */
/* 1508 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (1507) */
/* 1510 */	
			0x13, 0x0,	/* FC_OP */
/* 1512 */	NdrFcShort( 0xfffffe6e ),	/* Offset= -402 (1110) */
/* 1514 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1516 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1520 */	NdrFcShort( 0x10 ),	/* Offset= 16 (1536) */
/* 1522 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1524 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1526 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (1472) */
/* 1528 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1530 */	NdrFcShort( 0xffffff6e ),	/* Offset= -146 (1384) */
/* 1532 */	0x6,		/* FC_SHORT */
			0x38,		/* FC_ALIGNM4 */
/* 1534 */	0xe,		/* FC_ENUM32 */
			0x5b,		/* FC_END */
/* 1536 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1538 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1540 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1542 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1544) */
/* 1544 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1546 */	NdrFcLong( 0x20403 ),	/* 132099 */
/* 1550 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1554 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1556 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1558 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1560 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1562 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1564 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1568) */
/* 1566 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1568 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1570 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1572 */	NdrFcShort( 0xc ),	/* 12 */
/* 1574 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1576 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1566) */
/* 1578 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1580 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1582) */
/* 1582 */	
			0x13, 0x0,	/* FC_OP */
/* 1584 */	NdrFcShort( 0xe ),	/* Offset= 14 (1598) */
/* 1586 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1592 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1592) */
/* 1594 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 1596 */	0x3e,		/* FC_STRUCTPAD2 */
			0x5b,		/* FC_END */
/* 1598 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1600 */	NdrFcShort( 0x4c ),	/* 76 */
/* 1602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1604 */	NdrFcShort( 0x1e ),	/* Offset= 30 (1634) */
/* 1606 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1608 */	NdrFcShort( 0xfffff9de ),	/* Offset= -1570 (38) */
/* 1610 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1612 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1614 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 1616 */	0xe,		/* FC_ENUM32 */
			0x6,		/* FC_SHORT */
/* 1618 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1620 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1622 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1624 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1626 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffedf ),	/* Offset= -289 (1338) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1630 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffd3 ),	/* Offset= -45 (1586) */
			0x5b,		/* FC_END */
/* 1634 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1636 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1638 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1640 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1644) */
/* 1642 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1644 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1646 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1648 */	NdrFcShort( 0xc ),	/* 12 */
/* 1650 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1652 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1642) */
/* 1654 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1656 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1660) */
/* 1658 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1660 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1662 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1664 */	NdrFcShort( 0xc ),	/* 12 */
/* 1666 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1668 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1658) */
/* 1670 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1672 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1676) */
/* 1674 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1676 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1678 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1680 */	NdrFcShort( 0xc ),	/* 12 */
/* 1682 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1684 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1674) */
/* 1686 */	
			0x11, 0x0,	/* FC_RP */
/* 1688 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1690) */
/* 1690 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1692 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1694 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1696 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1698 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1700 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1702 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1704 */	NdrFcShort( 0xfffffdc0 ),	/* Offset= -576 (1128) */
/* 1706 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1708 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1710 */	NdrFcShort( 0xfffffdba ),	/* Offset= -582 (1128) */
/* 1712 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1714 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 1716 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1718 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1720) */
/* 1720 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 1722 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1724 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1726 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1728 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1730) */
/* 1730 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1732 */	NdrFcLong( 0x20402 ),	/* 132098 */
/* 1736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1740 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1742 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1744 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1746 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1748 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1750 */	NdrFcShort( 0x26 ),	/* Offset= 38 (1788) */
/* 1752 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1754 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1758 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1758) */
/* 1760 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1762 */	NdrFcShort( 0xfffff944 ),	/* Offset= -1724 (38) */
/* 1764 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1766 */	NdrFcShort( 0xfffffd70 ),	/* Offset= -656 (1110) */
/* 1768 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1770 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1774 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1776 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1778 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1782 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1784 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1752) */
/* 1786 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1788 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1790 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1794 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1800) */
/* 1796 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1798 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1800 */	
			0x13, 0x0,	/* FC_OP */
/* 1802 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1770) */
/* 1804 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1806 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1808) */
/* 1808 */	
			0x13, 0x0,	/* FC_OP */
/* 1810 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1812) */
/* 1812 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1814 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1816 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1818 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1818) */
/* 1820 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1822 */	NdrFcShort( 0xfffff908 ),	/* Offset= -1784 (38) */
/* 1824 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 1826 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1828 */	0x6,		/* FC_SHORT */
			0x3e,		/* FC_STRUCTPAD2 */
/* 1830 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1832 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1834 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1838) */
/* 1836 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1838 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1840 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1842 */	NdrFcShort( 0xc ),	/* 12 */
/* 1844 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1846 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1836) */
/* 1848 */	
			0x11, 0x0,	/* FC_RP */
/* 1850 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1852) */
/* 1852 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1854 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1856 */	0x27,		/* Corr desc:  parameter, FC_USHORT */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1858 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1860 */	0x27,		/* Corr desc:  parameter, FC_USHORT */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1862 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1864 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1866 */	NdrFcShort( 0xfffff8c0 ),	/* Offset= -1856 (10) */
/* 1868 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1870 */	
			0x11, 0x0,	/* FC_RP */
/* 1872 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1874) */
/* 1874 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 1876 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1878 */	0x27,		/* Corr desc:  parameter, FC_USHORT */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1880 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1882 */	0x27,		/* Corr desc:  parameter, FC_USHORT */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 1884 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1886 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1888 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1890 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 1892 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1894 */	NdrFcShort( 0xfffff8c0 ),	/* Offset= -1856 (38) */
/* 1896 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] */
/* 1898 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1900) */
/* 1900 */	
			0x2f,		/* FC_IP */
			0x5c,		/* FC_PAD */
/* 1902 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 1904 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif

			0x0
        }
    };

static const CInterfaceProxyVtbl * const _oaidl_ProxyVtblList[] = 
{
    ( const CInterfaceProxyVtbl *) &_IDispatchProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeInfoProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeLibProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeCompProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_IEnumVARIANTProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeLib2ProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeInfo2ProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_IErrorInfoProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ITypeFactoryProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ICreateErrorInfoProxyVtbl,
    ( const CInterfaceProxyVtbl *) &_ISupportErrorInfoProxyVtbl,
    0
};

static const CInterfaceStubVtbl * const _oaidl_StubVtblList[] = 
{
    ( const CInterfaceStubVtbl *) &_IDispatchStubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeInfoStubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeLibStubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeCompStubVtbl,
    ( const CInterfaceStubVtbl *) &_IEnumVARIANTStubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeLib2StubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeInfo2StubVtbl,
    ( const CInterfaceStubVtbl *) &_IErrorInfoStubVtbl,
    ( const CInterfaceStubVtbl *) &_ITypeFactoryStubVtbl,
    ( const CInterfaceStubVtbl *) &_ICreateErrorInfoStubVtbl,
    ( const CInterfaceStubVtbl *) &_ISupportErrorInfoStubVtbl,
    0
};

static const PCInterfaceName _oaidl_InterfaceNamesList[] = 
{
    "IDispatch",
    "ITypeInfo",
    "ITypeLib",
    "ITypeComp",
    "IEnumVARIANT",
    "ITypeLib2",
    "ITypeInfo2",
    "IErrorInfo",
    "ITypeFactory",
    "ICreateErrorInfo",
    "ISupportErrorInfo",
    0
};


#define _oaidl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _oaidl, pIID, n)

int __stdcall _oaidl_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _oaidl, 11, 8 )
    IID_BS_LOOKUP_NEXT_TEST( _oaidl, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _oaidl, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _oaidl, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _oaidl, 11, *pIndex )
    
}

const ExtendedProxyFileInfo oaidl_ProxyFileInfo = 
{
    (const PCInterfaceProxyVtblList *) & _oaidl_ProxyVtblList,
    (const PCInterfaceStubVtblList *) & _oaidl_StubVtblList,
    (const PCInterfaceName * ) & _oaidl_InterfaceNamesList,
    0, /* no delegation */
    & _oaidl_IID_Lookup, 
    11,
    1,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
