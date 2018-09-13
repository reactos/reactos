//
// Major ugliness warning:
//
// These are definitions stolen from the 32-bit OLE headers and
// munged to have the word FAR in all of the right places.
//



interface IRpcProxyBuffer;
interface IRpcChannelBuffer;
interface IRpcStubBuffer;
interface IPSFactoryBuffer;

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//
//  Contents: Interface header file for IRpcProxyBuffer
//
//  History: Created by Microsoft (R) MIDL Compiler Version 1.10.85
//
//--------------------------------------------------------------------------

#ifndef __IRpcProxyBuffer__
#define __IRpcProxyBuffer__


#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


EXTERN_C const IID IID_IRpcProxyBuffer;
#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */

interface IRpcProxyBuffer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Connect
    (
	IRpcChannelBuffer FAR *pRpcChannelBuffer
    ) = 0;

    virtual void STDMETHODCALLTYPE Disconnect
    (
        void
    ) = 0;

};

#else

/* C Language Binding */

typedef struct IRpcProxyBufferVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        IRpcProxyBuffer FAR * This,
	REFIID riid,
	void FAR * FAR *ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        IRpcProxyBuffer FAR * This
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        IRpcProxyBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *Connect)
    (
        IRpcProxyBuffer FAR * This,
	IRpcChannelBuffer FAR *pRpcChannelBuffer
    );

    void (STDMETHODCALLTYPE FAR *Disconnect)
    (
        IRpcProxyBuffer FAR * This
    );

} IRpcProxyBufferVtbl;

interface IRpcProxyBuffer
{
    IRpcProxyBufferVtbl FAR *lpVtbl;
} ;


#endif

#endif /*__IRpcProxyBuffer__*/


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//
//  Contents: Interface header file for IRpcChannelBuffer
//
//  History: Created by Microsoft (R) MIDL Compiler Version 1.10.85
//
//--------------------------------------------------------------------------

#ifndef __IRpcChannelBuffer__
#define __IRpcChannelBuffer__

/* Forward declaration */
// typedef interface IRpcChannelBuffer IRpcChannelBuffer;


#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


typedef unsigned long RPCOLEDATAREP;

typedef enum tagRPCFLG
#if 0
  {
	RPCFLG_ASYNCHRONOUS = 1073741824,
	RPCFLG_INPUT_SYNCHRONOUS = 536870912
  }
#endif
RPCFLG;


typedef struct tagRPCOLEMESSAGE
  {
  void FAR *reserved1;
  RPCOLEDATAREP dataRepresentation;
  void FAR *Buffer;
  ULONG cbBuffer;
  ULONG iMethod;
  void FAR *reserved2[5];
  ULONG rpcFlags;
  }
RPCOLEMESSAGE;


typedef RPCOLEMESSAGE FAR *PRPCOLEMESSAGE;


EXTERN_C const IID IID_IRpcChannelBuffer;
#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */

interface IRpcChannelBuffer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetBuffer
    (
	RPCOLEMESSAGE FAR *pMessage,
	REFIID riid
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SendReceive
    (
	RPCOLEMESSAGE FAR *pMessage,
	ULONG FAR *pStatus
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE FreeBuffer
    (
	RPCOLEMESSAGE FAR *pMessage
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDestCtx
    (
	DWORD FAR *pdwDestContext,
	void FAR * FAR *ppvDestContext
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE IsConnected
    (
        void
    ) = 0;

};

#else

/* C Language Binding */

typedef struct IRpcChannelBufferVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        IRpcChannelBuffer FAR * This,
	REFIID riid,
	void FAR * FAR *ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        IRpcChannelBuffer FAR * This
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        IRpcChannelBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBuffer)
    (
        IRpcChannelBuffer FAR * This,
	RPCOLEMESSAGE FAR *pMessage,
	REFIID riid
    );

    HRESULT (STDMETHODCALLTYPE FAR *SendReceive)
    (
        IRpcChannelBuffer FAR * This,
	RPCOLEMESSAGE FAR *pMessage,
	ULONG FAR *pStatus
    );

    HRESULT (STDMETHODCALLTYPE FAR *FreeBuffer)
    (
        IRpcChannelBuffer FAR * This,
	RPCOLEMESSAGE FAR *pMessage
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetDestCtx)
    (
        IRpcChannelBuffer FAR * This,
	DWORD FAR *pdwDestContext,
	void FAR * FAR *ppvDestContext
    );

    HRESULT (STDMETHODCALLTYPE FAR *IsConnected)
    (
        IRpcChannelBuffer FAR * This
    );

} IRpcChannelBufferVtbl;

interface IRpcChannelBuffer
{
    IRpcChannelBufferVtbl FAR *lpVtbl;
} ;


#endif

#endif /*__IRpcChannelBuffer__*/


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//
//  Contents: Interface header file for IRpcStubBuffer
//
//  History: Created by Microsoft (R) MIDL Compiler Version 1.10.85
//
//--------------------------------------------------------------------------

#ifndef __IRpcStubBuffer__
#define __IRpcStubBuffer__

/* Forward declaration */
// typedef interface IRpcStubBuffer IRpcStubBuffer;


#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


EXTERN_C const IID IID_IRpcStubBuffer;
#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */

interface IRpcStubBuffer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Connect
    (
	IUnknown FAR *pUnkServer
    ) = 0;

    virtual void STDMETHODCALLTYPE Disconnect
    (
        void
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Invoke
    (
	RPCOLEMESSAGE FAR *_prpcmsg,
	IRpcChannelBuffer FAR *_pRpcChannelBuffer
    ) = 0;

    virtual IRpcStubBuffer  FAR *STDMETHODCALLTYPE IsIIDSupported
    (
	REFIID riid
    ) = 0;

    virtual ULONG STDMETHODCALLTYPE CountRefs
    (
        void
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DebugServerQueryInterface
    (
	void FAR * FAR *ppv
    ) = 0;

    virtual void STDMETHODCALLTYPE DebugServerRelease
    (
	void FAR *pv
    ) = 0;

};

#else

/* C Language Binding */

typedef struct IRpcStubBufferVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        IRpcStubBuffer FAR * This,
	REFIID riid,
	void FAR * FAR *ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        IRpcStubBuffer FAR * This
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        IRpcStubBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *Connect)
    (
        IRpcStubBuffer FAR * This,
	IUnknown FAR *pUnkServer
    );

    void (STDMETHODCALLTYPE FAR *Disconnect)
    (
        IRpcStubBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *Invoke)
    (
        IRpcStubBuffer FAR * This,
	RPCOLEMESSAGE FAR *_prpcmsg,
	IRpcChannelBuffer FAR *_pRpcChannelBuffer
    );

    IRpcStubBuffer  FAR *(STDMETHODCALLTYPE FAR *IsIIDSupported)
    (
        IRpcStubBuffer FAR * This,
	REFIID riid
    );

    ULONG (STDMETHODCALLTYPE FAR *CountRefs)
    (
        IRpcStubBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *DebugServerQueryInterface)
    (
        IRpcStubBuffer FAR * This,
	void FAR * FAR *ppv
    );

    void (STDMETHODCALLTYPE FAR *DebugServerRelease)
    (
        IRpcStubBuffer FAR * This,
	void FAR *pv
    );

} IRpcStubBufferVtbl;

interface IRpcStubBuffer
{
    IRpcStubBufferVtbl FAR *lpVtbl;
} ;


#endif

#endif /*__IRpcStubBuffer__*/


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//
//  Contents: Interface header file for IPSFactoryBuffer
//
//  History: Created by Microsoft (R) MIDL Compiler Version 1.10.85
//
//--------------------------------------------------------------------------

#ifndef __IPSFactoryBuffer__
#define __IPSFactoryBuffer__

/* Forward declaration */
// typedef interface IPSFactoryBuffer IPSFactoryBuffer;


#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


EXTERN_C const IID IID_IPSFactoryBuffer;
#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */

interface IPSFactoryBuffer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE CreateProxy
    (
	IUnknown FAR *pUnkOuter,
	REFIID riid,
	IRpcProxyBuffer FAR * FAR *ppProxy,
	void FAR * FAR *ppv
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateStub
    (
	REFIID riid,
	IUnknown FAR *pUnkServer,
	IRpcStubBuffer FAR * FAR *ppStub
    ) = 0;

};

#else

/* C Language Binding */

typedef struct IPSFactoryBufferVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        IPSFactoryBuffer FAR * This,
	REFIID riid,
	void FAR * FAR *ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        IPSFactoryBuffer FAR * This
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        IPSFactoryBuffer FAR * This
    );

    HRESULT (STDMETHODCALLTYPE FAR *CreateProxy)
    (
        IPSFactoryBuffer FAR * This,
	IUnknown FAR *pUnkOuter,
	REFIID riid,
	IRpcProxyBuffer FAR * FAR *ppProxy,
	void FAR * FAR *ppv
    );

    HRESULT (STDMETHODCALLTYPE FAR *CreateStub)
    (
        IPSFactoryBuffer FAR * This,
	REFIID riid,
	IUnknown FAR *pUnkServer,
	IRpcStubBuffer FAR * FAR *ppStub
    );

} IPSFactoryBufferVtbl;

interface IPSFactoryBuffer
{
    IPSFactoryBufferVtbl FAR *lpVtbl;
} ;


#endif

#endif /*__IPSFactoryBuffer__*/

