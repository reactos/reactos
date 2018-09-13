/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Mon Oct 28 10:14:19 1996
 */
/* Compiler settings for omscript.idl:
    Oic (OptLev=i1), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __omscript_h__
#define __omscript_h__

#ifdef __cplusplus
extern "C"{
#endif 

#ifndef __IBrowseControl_FWD_DEFINED__
#define __IBrowseControl_FWD_DEFINED__
typedef interface IBrowseControl IBrowseControl;
#endif 	/* __IBrowseControl_FWD_DEFINED__ */


// Flags for IBrowseCtl flags.
// BUGBUG - They need to go into omscript.idl when omscript.odl
// is converted into an IDL file

#define DLCTL_FLAGS_SET              0x00000002
#define DLCTL_DLIMAGES               0x00000010
#define DLCTL_VIDEOS	             0x00000020
#define DLCTL_BGSOUNDS	             0x00000040
#define DLCTL_RUNSCRIPTS             0x00000080
#define DLCTL_RUNJAVA	             0x00000100
#define DLCTL_RUNACTIVEXCTLS         0x00000200
#define DLCTL_RUNSILENTACTIVEXCTLS   0x00000400

#define DLCTL_OFFLINE                0x80000000
#define DLCTL_SILENT                 0x40000000

#define DLCTL_FLAGS_MASK             (DLCTL_FLAGS_SET | \
                                      DLCTL_DLIMAGES | \
                                      DLCTL_VIDEOS | \
                                      DLCTL_BGSOUNDS | \
                                      DLCTL_RUNSCRIPTS | \
                                      DLCTL_RUNJAVA | \
                                      DLCTL_RUNACTIVEXCTLS | \
                                      DLCTL_RUNSILENTACTIVEXCTLS )

// name of the IBrowseCtl Object when attached to teh BindContext 
// sent to IHlinkFrame:;Navigate

#define BROWSER_OPTIONS_OBJECT_NAME L"Browser Control" 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Mon Oct 28 10:14:19 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// Omscript.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

#ifndef __IBrowseControl_INTERFACE_DEFINED__
#define __IBrowseControl_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBrowseControl
 * at Mon Oct 28 10:14:19 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][oleautomation][uuid][helpstring] */ 



EXTERN_C const IID IID_IBrowseControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IBrowseControl : public IUnknown
    {
    public:
        virtual /* [helpcontext][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ long __RPC_FAR *pdwRetVal) = 0;
        
        virtual /* [helpcontext][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Flags( 
            /* [in] */ long dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBrowseControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBrowseControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBrowseControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBrowseControl __RPC_FAR * This);
        
        /* [helpcontext][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Flags )( 
            IBrowseControl __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pdwRetVal);
        
        /* [helpcontext][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Flags )( 
            IBrowseControl __RPC_FAR * This,
            /* [in] */ long dwFlags);
        
        END_INTERFACE
    } IBrowseControlVtbl;

    interface IBrowseControl
    {
        CONST_VTBL struct IBrowseControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBrowseControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBrowseControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBrowseControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBrowseControl_get_Flags(This,pdwRetVal)	\
    (This)->lpVtbl -> get_Flags(This,pdwRetVal)

#define IBrowseControl_put_Flags(This,dwFlags)	\
    (This)->lpVtbl -> put_Flags(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IBrowseControl_get_Flags_Proxy( 
    IBrowseControl __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pdwRetVal);


void __RPC_STUB IBrowseControl_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IBrowseControl_put_Flags_Proxy( 
    IBrowseControl __RPC_FAR * This,
    /* [in] */ long dwFlags);


void __RPC_STUB IBrowseControl_put_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBrowseControl_INTERFACE_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL__intf_0096
 * at Mon Oct 28 10:14:19 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


extern RPC_IF_HANDLE __MIDL__intf_0096_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0096_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif //OMSCRIPT_H


