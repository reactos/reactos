/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0101 */
/* at Thu Mar 28 23:04:03 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __marqinfo_h__
#define __marqinfo_h__

const IID IID_IMarqueeInfo = {0x0bdc6ae0,0x6d11,0x11cf,{0xbe,0x62,0x00,0x80,0xc7,0x2e,0xdd,0x2d}};

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMarqueeInfo_FWD_DEFINED__
#define __IMarqueeInfo_FWD_DEFINED__
typedef interface IMarqueeInfo IMarqueeInfo;
#endif  /* __IMarqueeInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Mar 28 23:04:03 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


            /* size is 0 */



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IMarqueeInfo_INTERFACE_DEFINED__
#define __IMarqueeInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMarqueeInfo
 * at Thu Mar 28 23:04:03 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IMarqueeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMarqueeInfo : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetDocCoords( 
            /* [out] */ LPRECT prcView,
            /* [in] */ BOOL bGetOnlyIfFullyLoaded,
            /* [out] */ BOOL __RPC_FAR *pfFullyLoaded,
            /* [in] */ int WidthToFormatPageTo) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IMarqueeInfoVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMarqueeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMarqueeInfo __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMarqueeInfo __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetDocCoords )( 
            IMarqueeInfo __RPC_FAR * This,
            /* [out] */ LPRECT prcView,
            /* [in] */ BOOL bGetOnlyIfFullyLoaded,
            /* [out] */ BOOL __RPC_FAR *pfFullyLoaded,
            /* [in] */ int WidthToFormatPageTo);
        
    } IMarqueeInfoVtbl;

    interface IMarqueeInfo
    {
        CONST_VTBL struct IMarqueeInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMarqueeInfo_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMarqueeInfo_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IMarqueeInfo_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IMarqueeInfo_GetDocCoords(This,prcView,bGetOnlyIfFullyLoaded,pfFullyLoaded,WidthToFormatPageTo) \
    (This)->lpVtbl -> GetDocCoords(This,prcView,bGetOnlyIfFullyLoaded,pfFullyLoaded,WidthToFormatPageTo)

#endif /* COBJMACROS */


#endif  /* C style interface */

#endif  /* __IMarqueeInfo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
