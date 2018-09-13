/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu May 22 17:20:43 1997
 */
/* Compiler settings for msdadc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __rowpos_h__
#define __rowpos_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRowPosition_FWD_DEFINED__
#define __IRowPosition_FWD_DEFINED__
typedef interface IRowPosition IRowPosition;
#endif 	/* __IRowPosition_FWD_DEFINED__ */


#ifndef __IRowPositionChange_FWD_DEFINED__
#define __IRowPositionChange_FWD_DEFINED__
typedef interface IRowPositionChange IRowPositionChange;
#endif 	/* __IRowPositionChange_FWD_DEFINED__ */


#ifndef __RowPosition_FWD_DEFINED__
#define __RowPosition_FWD_DEFINED__

#ifdef __cplusplus
typedef class RowPosition RowPosition;
#else
typedef struct RowPosition RowPosition;
#endif /* __cplusplus */

#endif 	/* __RowPosition_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __IRowPosition_INTERFACE_DEFINED__
#define __IRowPosition_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowPosition
 * at Thu May 22 17:20:43 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 


typedef DWORD DBPOSITIONFLAGS;


enum DBPOSITIONFLAGSENUM
    {	DBPOSITION_OK	= 0,
	DBPOSITION_NOROW	= DBPOSITION_OK + 1,
	DBPOSITION_BOF	= DBPOSITION_NOROW + 1,
	DBPOSITION_EOF	= DBPOSITION_BOF + 1
    };

EXTERN_C const IID IID_IRowPosition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0c733a94-2a1c-11ce-ade5-00aa0044773d")
    IRowPosition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ClearRowPosition( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowPosition( 
            /* [out] */ HCHAPTER __RPC_FAR *phChapter,
            /* [out] */ HROW __RPC_FAR *phRow,
            /* [out] */ DBPOSITIONFLAGS __RPC_FAR *pdwPositionFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowset( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPUNKNOWN __RPC_FAR *ppRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IUnknown __RPC_FAR *pRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRowPosition( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ HROW hRow,
            /* [in] */ DBPOSITIONFLAGS dwPositionFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowPositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowPosition __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowPosition __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowPosition __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearRowPosition )( 
            IRowPosition __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowPosition )( 
            IRowPosition __RPC_FAR * This,
            /* [out] */ HCHAPTER __RPC_FAR *phChapter,
            /* [out] */ HROW __RPC_FAR *phRow,
            /* [out] */ DBPOSITIONFLAGS __RPC_FAR *pdwPositionFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowset )( 
            IRowPosition __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPUNKNOWN __RPC_FAR *ppRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IRowPosition __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRowPosition )( 
            IRowPosition __RPC_FAR * This,
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ HROW hRow,
            /* [in] */ DBPOSITIONFLAGS dwPositionFlags);
        
        END_INTERFACE
    } IRowPositionVtbl;

    interface IRowPosition
    {
        CONST_VTBL struct IRowPositionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowPosition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowPosition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowPosition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowPosition_ClearRowPosition(This)	\
    (This)->lpVtbl -> ClearRowPosition(This)

#define IRowPosition_GetRowPosition(This,phChapter,phRow,pdwPositionFlags)	\
    (This)->lpVtbl -> GetRowPosition(This,phChapter,phRow,pdwPositionFlags)

#define IRowPosition_GetRowset(This,riid,ppRowset)	\
    (This)->lpVtbl -> GetRowset(This,riid,ppRowset)

#define IRowPosition_Initialize(This,pRowset)	\
    (This)->lpVtbl -> Initialize(This,pRowset)

#define IRowPosition_SetRowPosition(This,hChapter,hRow,dwPositionFlags)	\
    (This)->lpVtbl -> SetRowPosition(This,hChapter,hRow,dwPositionFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowPosition_ClearRowPosition_Proxy( 
    IRowPosition __RPC_FAR * This);


void __RPC_STUB IRowPosition_ClearRowPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowPosition_GetRowPosition_Proxy( 
    IRowPosition __RPC_FAR * This,
    /* [out] */ HCHAPTER __RPC_FAR *phChapter,
    /* [out] */ HROW __RPC_FAR *phRow,
    /* [out] */ DBPOSITIONFLAGS __RPC_FAR *pdwPositionFlags);


void __RPC_STUB IRowPosition_GetRowPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowPosition_GetRowset_Proxy( 
    IRowPosition __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ LPUNKNOWN __RPC_FAR *ppRowset);


void __RPC_STUB IRowPosition_GetRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowPosition_Initialize_Proxy( 
    IRowPosition __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pRowset);


void __RPC_STUB IRowPosition_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowPosition_SetRowPosition_Proxy( 
    IRowPosition __RPC_FAR * This,
    /* [in] */ HCHAPTER hChapter,
    /* [in] */ HROW hRow,
    /* [in] */ DBPOSITIONFLAGS dwPositionFlags);


void __RPC_STUB IRowPosition_SetRowPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowPosition_INTERFACE_DEFINED__ */


#ifndef __IRowPositionChange_INTERFACE_DEFINED__
#define __IRowPositionChange_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowPositionChange
 * at Thu May 22 17:20:43 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



enum DBREASONPOSITIONENUM
    {	DBREASON_ROWPOSITION_CHANGED	= DBREASON_ROWSET_CHANGED + 1,
	DBREASON_ROWPOSITION_CHAPTERCHANGED	= DBREASON_ROWPOSITION_CHANGED + 1,
	DBREASON_ROWPOSITION_CLEARED	= DBREASON_ROWPOSITION_CHAPTERCHANGED + 1
    };

EXTERN_C const IID IID_IRowPositionChange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0997a571-126e-11d0-9f8a-00a0c9a0631e")
    IRowPositionChange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnRowPositionChange( 
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowPositionChangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowPositionChange __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowPositionChange __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowPositionChange __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnRowPositionChange )( 
            IRowPositionChange __RPC_FAR * This,
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny);
        
        END_INTERFACE
    } IRowPositionChangeVtbl;

    interface IRowPositionChange
    {
        CONST_VTBL struct IRowPositionChangeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowPositionChange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowPositionChange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowPositionChange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowPositionChange_OnRowPositionChange(This,eReason,ePhase,fCantDeny)	\
    (This)->lpVtbl -> OnRowPositionChange(This,eReason,ePhase,fCantDeny)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowPositionChange_OnRowPositionChange_Proxy( 
    IRowPositionChange __RPC_FAR * This,
    /* [in] */ DBREASON eReason,
    /* [in] */ DBEVENTPHASE ePhase,
    /* [in] */ BOOL fCantDeny);


void __RPC_STUB IRowPositionChange_OnRowPositionChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowPositionChange_INTERFACE_DEFINED__ */



#ifndef __MSDAUTILLib_LIBRARY_DEFINED__
#define __MSDAUTILLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MSDAUTILLib
 * at Thu May 22 17:20:43 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_RowPosition;

class DECLSPEC_UUID("2048EEE6-7FA2-11D0-9E6A-00A0C9138C29")
RowPosition;

DEFINE_GUID(CLSID_RowPosition, 0x2048eee6, 0x7fa2, 0x11d0, 0x9e, 0x6a, 0x0, 0xa0, 0xc9, 0x13, 0x8c, 0x29);
DEFINE_GUID(IID_IRowPosition,  0x0c733a94, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x0, 0xaa, 0x0, 0x44, 0x77, 0x3d);
DEFINE_GUID(IID_IRowPositionChange,  0x0997a571, 0x126e, 0x11d0, 0x9f, 0x8a, 0x0, 0xa0, 0xc9, 0xa0, 0x63, 0x1e);


#endif
#endif /* __MSDAUTILLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
