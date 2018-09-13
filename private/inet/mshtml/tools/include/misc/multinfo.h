/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0102 */
/* at Fri Mar 29 16:59:57 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __multinfo_h__
#define __multinfo_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IProvideClassInfo_FWD_DEFINED__
#define __IProvideClassInfo_FWD_DEFINED__
typedef interface IProvideClassInfo IProvideClassInfo;
#endif 	/* __IProvideClassInfo_FWD_DEFINED__ */


#ifndef __IProvideClassInfo2_FWD_DEFINED__
#define __IProvideClassInfo2_FWD_DEFINED__
typedef interface IProvideClassInfo2 IProvideClassInfo2;
#endif 	/* __IProvideClassInfo2_FWD_DEFINED__ */


#ifndef __IProvideMultipleClassInfo_FWD_DEFINED__
#define __IProvideMultipleClassInfo_FWD_DEFINED__
typedef interface IProvideMultipleClassInfo IProvideMultipleClassInfo;
#endif 	/* __IProvideMultipleClassInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Mar 29 16:59:57 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#ifndef _OLECTL_H_
#include <olectl.h>
#endif
#if 0


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IProvideClassInfo_INTERFACE_DEFINED__
#define __IProvideClassInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideClassInfo
 * at Fri Mar 29 16:59:57 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IProvideClassInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IProvideClassInfo : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetClassInfo( 
            /* [out] */ LPTYPEINFO __RPC_FAR *ppTI) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideClassInfoVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IProvideClassInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IProvideClassInfo __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IProvideClassInfo __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetClassInfo )( 
            IProvideClassInfo __RPC_FAR * This,
            /* [out] */ LPTYPEINFO __RPC_FAR *ppTI);
        
    } IProvideClassInfoVtbl;

    interface IProvideClassInfo
    {
        CONST_VTBL struct IProvideClassInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideClassInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideClassInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideClassInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideClassInfo_GetClassInfo(This,ppTI)	\
    (This)->lpVtbl -> GetClassInfo(This,ppTI)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IProvideClassInfo_GetClassInfo_Proxy( 
    IProvideClassInfo __RPC_FAR * This,
    /* [out] */ LPTYPEINFO __RPC_FAR *ppTI);


void __RPC_STUB IProvideClassInfo_GetClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideClassInfo_INTERFACE_DEFINED__ */


#ifndef __IProvideClassInfo2_INTERFACE_DEFINED__
#define __IProvideClassInfo2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideClassInfo2
 * at Fri Mar 29 16:59:57 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IProvideClassInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IProvideClassInfo2 : public IProvideClassInfo
    {
    public:
        virtual HRESULT __stdcall GetGUID( 
            /* [in] */ DWORD dwGuidKind,
            /* [out] */ GUID __RPC_FAR *pGUID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideClassInfo2Vtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IProvideClassInfo2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IProvideClassInfo2 __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IProvideClassInfo2 __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetClassInfo )( 
            IProvideClassInfo2 __RPC_FAR * This,
            /* [out] */ LPTYPEINFO __RPC_FAR *ppTI);
        
        HRESULT ( __stdcall __RPC_FAR *GetGUID )( 
            IProvideClassInfo2 __RPC_FAR * This,
            /* [in] */ DWORD dwGuidKind,
            /* [out] */ GUID __RPC_FAR *pGUID);
        
    } IProvideClassInfo2Vtbl;

    interface IProvideClassInfo2
    {
        CONST_VTBL struct IProvideClassInfo2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideClassInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideClassInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideClassInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideClassInfo2_GetClassInfo(This,ppTI)	\
    (This)->lpVtbl -> GetClassInfo(This,ppTI)


#define IProvideClassInfo2_GetGUID(This,dwGuidKind,pGUID)	\
    (This)->lpVtbl -> GetGUID(This,dwGuidKind,pGUID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IProvideClassInfo2_GetGUID_Proxy( 
    IProvideClassInfo2 __RPC_FAR * This,
    /* [in] */ DWORD dwGuidKind,
    /* [out] */ GUID __RPC_FAR *pGUID);


void __RPC_STUB IProvideClassInfo2_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideClassInfo2_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0053
 * at Fri Mar 29 16:59:57 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#endif // 0
// {A7ABA9C1-8983-11cf-8F20-00805F2CD064}
DEFINE_GUID(IID_IProvideMultipleClassInfo,
0xa7aba9c1, 0x8983, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);


extern RPC_IF_HANDLE __MIDL__intf_0053_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0053_v0_0_s_ifspec;

#ifndef __IProvideMultipleClassInfo_INTERFACE_DEFINED__
#define __IProvideMultipleClassInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideMultipleClassInfo
 * at Fri Mar 29 16:59:57 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 


#define MULTICLASSINFO_GETTYPEINFO           0x00000001
#define MULTICLASSINFO_GETNUMRESERVEDDISPIDS 0x00000002
#define MULTICLASSINFO_GETIIDPRIMARY         0x00000004
#define MULTICLASSINFO_GETIIDSOURCE          0x00000008
#define TIFLAGS_EXTENDDISPATCHONLY           0x00000001

EXTERN_C const IID IID_IProvideMultipleClassInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IProvideMultipleClassInfo : public IProvideClassInfo2
    {
    public:
        virtual HRESULT __stdcall GetMultiTypeInfoCount( 
            /* [out] */ ULONG __RPC_FAR *pcti) = 0;
        
        virtual HRESULT __stdcall GetInfoOfIndex( 
            /* [in] */ ULONG iti,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptiCoClass,
            /* [out] */ DWORD __RPC_FAR *pdwTIFlags,
            /* [out] */ ULONG __RPC_FAR *pcdispidReserved,
            /* [out] */ IID __RPC_FAR *piidPrimary,
            /* [out] */ IID __RPC_FAR *piidSource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideMultipleClassInfoVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IProvideMultipleClassInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IProvideMultipleClassInfo __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IProvideMultipleClassInfo __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetClassInfo )( 
            IProvideMultipleClassInfo __RPC_FAR * This,
            /* [out] */ LPTYPEINFO __RPC_FAR *ppTI);
        
        HRESULT ( __stdcall __RPC_FAR *GetGUID )( 
            IProvideMultipleClassInfo __RPC_FAR * This,
            /* [in] */ DWORD dwGuidKind,
            /* [out] */ GUID __RPC_FAR *pGUID);
        
        HRESULT ( __stdcall __RPC_FAR *GetMultiTypeInfoCount )( 
            IProvideMultipleClassInfo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcti);
        
        HRESULT ( __stdcall __RPC_FAR *GetInfoOfIndex )( 
            IProvideMultipleClassInfo __RPC_FAR * This,
            /* [in] */ ULONG iti,
            /* [in] */ DWORD dwFlags,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptiCoClass,
            /* [out] */ DWORD __RPC_FAR *pdwTIFlags,
            /* [out] */ ULONG __RPC_FAR *pcdispidReserved,
            /* [out] */ IID __RPC_FAR *piidPrimary,
            /* [out] */ IID __RPC_FAR *piidSource);
        
    } IProvideMultipleClassInfoVtbl;

    interface IProvideMultipleClassInfo
    {
        CONST_VTBL struct IProvideMultipleClassInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideMultipleClassInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideMultipleClassInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideMultipleClassInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideMultipleClassInfo_GetClassInfo(This,ppTI)	\
    (This)->lpVtbl -> GetClassInfo(This,ppTI)


#define IProvideMultipleClassInfo_GetGUID(This,dwGuidKind,pGUID)	\
    (This)->lpVtbl -> GetGUID(This,dwGuidKind,pGUID)


#define IProvideMultipleClassInfo_GetMultiTypeInfoCount(This,pcti)	\
    (This)->lpVtbl -> GetMultiTypeInfoCount(This,pcti)

#define IProvideMultipleClassInfo_GetInfoOfIndex(This,iti,dwFlags,pptiCoClass,pdwTIFlags,pcdispidReserved,piidPrimary,piidSource)	\
    (This)->lpVtbl -> GetInfoOfIndex(This,iti,dwFlags,pptiCoClass,pdwTIFlags,pcdispidReserved,piidPrimary,piidSource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IProvideMultipleClassInfo_GetMultiTypeInfoCount_Proxy( 
    IProvideMultipleClassInfo __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcti);


void __RPC_STUB IProvideMultipleClassInfo_GetMultiTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IProvideMultipleClassInfo_GetInfoOfIndex_Proxy( 
    IProvideMultipleClassInfo __RPC_FAR * This,
    /* [in] */ ULONG iti,
    /* [in] */ DWORD dwFlags,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptiCoClass,
    /* [out] */ DWORD __RPC_FAR *pdwTIFlags,
    /* [out] */ ULONG __RPC_FAR *pcdispidReserved,
    /* [out] */ IID __RPC_FAR *piidPrimary,
    /* [out] */ IID __RPC_FAR *piidSource);


void __RPC_STUB IProvideMultipleClassInfo_GetInfoOfIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideMultipleClassInfo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
