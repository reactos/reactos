//.-------------------------------------------------------------------------
//.
//.  Microsoft Windows
//.  Copyright (C) Microsoft Corporation, 1995.
//.
//.  File: transact.idl
//.
//.  Contents: The basic transaction interfaces and types.
//.
//.--------------------------------------------------------------------------


/* File created by MIDL compiler version 2.00.0102 */
/* at Tue Nov 21 16:54:32 1995
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __transact_h__
#define __transact_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITransaction_FWD_DEFINED__
#define __ITransaction_FWD_DEFINED__
typedef interface ITransaction ITransaction;
#endif 	/* __ITransaction_FWD_DEFINED__ */


#ifndef __ITransactionDispenser_FWD_DEFINED__
#define __ITransactionDispenser_FWD_DEFINED__
typedef interface ITransactionDispenser ITransactionDispenser;
#endif 	/* __ITransactionDispenser_FWD_DEFINED__ */


#ifndef __ITransactionOptions_FWD_DEFINED__
#define __ITransactionOptions_FWD_DEFINED__
typedef interface ITransactionOptions ITransactionOptions;
#endif 	/* __ITransactionOptions_FWD_DEFINED__ */


#ifndef __ITransactionOutcomeEvents_FWD_DEFINED__
#define __ITransactionOutcomeEvents_FWD_DEFINED__
typedef interface ITransactionOutcomeEvents ITransactionOutcomeEvents;
#endif 	/* __ITransactionOutcomeEvents_FWD_DEFINED__ */


#ifndef __ITransactionCompletionEvents_FWD_DEFINED__
#define __ITransactionCompletionEvents_FWD_DEFINED__
typedef interface ITransactionCompletionEvents ITransactionCompletionEvents;
#endif 	/* __ITransactionCompletionEvents_FWD_DEFINED__ */


/* header files for imported files */

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __BasicTransactionTypes_INTERFACE_DEFINED__
#define __BasicTransactionTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: BasicTransactionTypes
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][local] */ 


			/* size is 16 */
typedef struct  BOID
    {
    BYTE rgb[ 16 ];
    }	BOID;

#define BOID_NULL (*((BOID*)(&IID_NULL)))

#define MAX_TRAN_DESC 40		// used by XACTOPT via midl - see transact.idl 
			/* size is 16 */
typedef BOID XACTUOW;

			/* size is 4 */
typedef LONG ISOLEVEL;

#if defined(_WIN32)
			/* size is 2 */
typedef 
enum ISOLATIONLEVEL
    {	ISOLATIONLEVEL_UNSPECIFIED	= 0xffffffff,
	ISOLATIONLEVEL_CHAOS	= 0x10,
	ISOLATIONLEVEL_READUNCOMMITTED	= 0x100,
	ISOLATIONLEVEL_BROWSE	= 0x100,
	ISOLATIONLEVEL_CURSORSTABILITY	= 0x1000,
	ISOLATIONLEVEL_READCOMMITTED	= 0x1000,
	ISOLATIONLEVEL_REPEATABLEREAD	= 0x10000,
	ISOLATIONLEVEL_SERIALIZABLE	= 0x100000,
	ISOLATIONLEVEL_ISOLATED	= 0x100000
    }	ISOLATIONLEVEL;

#else
#define ISOLATIONLEVEL_UNSPECIFIED      0xFFFFFFFF
#define ISOLATIONLEVEL_CHAOS            0x00000010
#define ISOLATIONLEVEL_READUNCOMMITTED  0x00000100
#define ISOLATIONLEVEL_BROWSE           0x00000100
#define ISOLATIONLEVEL_CURSORSTABILITY  0x00001000
#define ISOLATIONLEVEL_READCOMMITTED    0x00001000
#define ISOLATIONLEVEL_REPEATABLEREAD   0x00010000
#define ISOLATIONLEVEL_SERIALIZABLE     0x00100000
#define ISOLATIONLEVEL_ISOLATED         0x00100000
#endif
			/* size is 40 */
typedef struct  XACTTRANSINFO
    {
    XACTUOW uow;
    ISOLEVEL isoLevel;
    ULONG isoFlags;
    DWORD grfTCSupported;
    DWORD grfRMSupported;
    DWORD grfTCSupportedRetaining;
    DWORD grfRMSupportedRetaining;
    }	XACTTRANSINFO;

			/* size is 36 */
typedef struct  XACTSTATS
    {
    ULONG cOpen;
    ULONG cCommitting;
    ULONG cCommitted;
    ULONG cAborting;
    ULONG cAborted;
    ULONG cInDoubt;
    ULONG cHeuristicDecision;
    FILETIME timeTransactionsUp;
    }	XACTSTATS;

			/* size is 2 */
typedef 
enum ISOFLAG
    {	ISOFLAG_RETAIN_COMMIT_DC	= 1,
	ISOFLAG_RETAIN_COMMIT	= 2,
	ISOFLAG_RETAIN_COMMIT_NO	= 3,
	ISOFLAG_RETAIN_ABORT_DC	= 4,
	ISOFLAG_RETAIN_ABORT	= 8,
	ISOFLAG_RETAIN_ABORT_NO	= 12,
	ISOFLAG_RETAIN_DONTCARE	= ISOFLAG_RETAIN_COMMIT_DC | ISOFLAG_RETAIN_ABORT_DC,
	ISOFLAG_RETAIN_BOTH	= ISOFLAG_RETAIN_COMMIT | ISOFLAG_RETAIN_ABORT,
	ISOFLAG_RETAIN_NONE	= ISOFLAG_RETAIN_COMMIT_NO | ISOFLAG_RETAIN_ABORT_NO,
	ISOFLAG_OPTIMISTIC	= 16
    }	ISOFLAG;

			/* size is 2 */
typedef 
enum XACTTC
    {	XACTTC_SYNC_PHASEONE	= 1,
	XACTTC_SYNC_PHASETWO	= 2,
	XACTTC_SYNC	= 2,
	XACTTC_ASYNC_PHASEONE	= 4
    }	XACTTC;

			/* size is 2 */
typedef 
enum XACTRM
    {	XACTRM_OPTIMISTICLASTWINS	= 1,
	XACTRM_NOREADONLYPREPARES	= 2
    }	XACTRM;

			/* size is 2 */
typedef 
enum XACTCONST
    {	XACTCONST_TIMEOUTINFINITE	= 0
    }	XACTCONST;

			/* size is 2 */
typedef 
enum XACTHEURISTIC
    {	XACTHEURISTIC_ABORT	= 1,
	XACTHEURISTIC_COMMIT	= 2,
	XACTHEURISTIC_DAMAGE	= 3,
	XACTHEURISTIC_DANGER	= 4
    }	XACTHEURISTIC;

#if defined(_WIN32)
			/* size is 2 */
typedef 
enum XACTSTAT
    {	XACTSTAT_NONE	= 0,
	XACTSTAT_OPENNORMAL	= 0x1,
	XACTSTAT_OPENREFUSED	= 0x2,
	XACTSTAT_PREPARING	= 0x4,
	XACTSTAT_PREPARED	= 0x8,
	XACTSTAT_PREPARERETAINING	= 0x10,
	XACTSTAT_PREPARERETAINED	= 0x20,
	XACTSTAT_COMMITTING	= 0x40,
	XACTSTAT_COMMITRETAINING	= 0x80,
	XACTSTAT_ABORTING	= 0x100,
	XACTSTAT_ABORTED	= 0x200,
	XACTSTAT_COMMITTED	= 0x400,
	XACTSTAT_HEURISTIC_ABORT	= 0x800,
	XACTSTAT_HEURISTIC_COMMIT	= 0x1000,
	XACTSTAT_HEURISTIC_DAMAGE	= 0x2000,
	XACTSTAT_HEURISTIC_DANGER	= 0x4000,
	XACTSTAT_FORCED_ABORT	= 0x8000,
	XACTSTAT_FORCED_COMMIT	= 0x10000,
	XACTSTAT_INDOUBT	= 0x20000,
	XACTSTAT_CLOSED	= 0x40000,
	XACTSTAT_OPEN	= 0x3,
	XACTSTAT_NOTPREPARED	= 0x7ffc3,
	XACTSTAT_ALL	= 0x7ffff
    }	XACTSTAT;

#else
#define XACTSTAT_NONE               0x00000000
#define XACTSTAT_OPENNORMAL         0x00000001
#define XACTSTAT_OPENREFUSED        0x00000002
#define XACTSTAT_PREPARING          0x00000004
#define XACTSTAT_PREPARED           0x00000008
#define XACTSTAT_PREPARERETAINING   0x00000010
#define XACTSTAT_PREPARERETAINED    0x00000020
#define XACTSTAT_COMMITTING         0x00000040
#define XACTSTAT_COMMITRETAINING    0x00000080
#define XACTSTAT_ABORTING           0x00000100
#define XACTSTAT_ABORTED            0x00000200
#define XACTSTAT_COMMITTED          0x00000400
#define XACTSTAT_HEURISTIC_ABORT    0x00000800
#define XACTSTAT_HEURISTIC_COMMIT   0x00001000
#define XACTSTAT_HEURISTIC_DAMAGE   0x00002000
#define XACTSTAT_HEURISTIC_DANGER   0x00004000
#define XACTSTAT_FORCED_ABORT       0x00008000
#define XACTSTAT_FORCED_COMMIT      0x00010000
#define XACTSTAT_INDOUBT            0x00020000
#define XACTSTAT_CLOSED             0x00040000
#define XACTSTAT_OPEN               0x00000003
#define XACTSTAT_NOTPREPARED        0x0007FFC3
#define XACTSTAT_ALL                0x0007FFFF
#endif
			/* size is 44 */
typedef struct  XACTOPT
    {
    ULONG ulTimeout;
    unsigned char szDescription[ 40 ];
    }	XACTOPT;



extern RPC_IF_HANDLE BasicTransactionTypes_v0_0_c_ifspec;
extern RPC_IF_HANDLE BasicTransactionTypes_v0_0_s_ifspec;
#endif /* __BasicTransactionTypes_INTERFACE_DEFINED__ */

#ifndef __ITransaction_INTERFACE_DEFINED__
#define __ITransaction_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransaction
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransaction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransaction : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Commit( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM) = 0;
        
        virtual HRESULT __stdcall Abort( 
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync) = 0;
        
        virtual HRESULT __stdcall GetTransactionInfo( 
            /* [out] */ XACTTRANSINFO __RPC_FAR *pinfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ITransaction __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ITransaction __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ITransaction __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Commit )( 
            ITransaction __RPC_FAR * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
        HRESULT ( __stdcall __RPC_FAR *Abort )( 
            ITransaction __RPC_FAR * This,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
        HRESULT ( __stdcall __RPC_FAR *GetTransactionInfo )( 
            ITransaction __RPC_FAR * This,
            /* [out] */ XACTTRANSINFO __RPC_FAR *pinfo);
        
    } ITransactionVtbl;

    interface ITransaction
    {
        CONST_VTBL struct ITransactionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransaction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransaction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransaction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransaction_Commit(This,fRetaining,grfTC,grfRM)	\
    (This)->lpVtbl -> Commit(This,fRetaining,grfTC,grfRM)

#define ITransaction_Abort(This,pboidReason,fRetaining,fAsync)	\
    (This)->lpVtbl -> Abort(This,pboidReason,fRetaining,fAsync)

#define ITransaction_GetTransactionInfo(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo(This,pinfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITransaction_Commit_Proxy( 
    ITransaction __RPC_FAR * This,
    /* [in] */ BOOL fRetaining,
    /* [in] */ DWORD grfTC,
    /* [in] */ DWORD grfRM);


void __RPC_STUB ITransaction_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransaction_Abort_Proxy( 
    ITransaction __RPC_FAR * This,
    /* [in] */ BOID __RPC_FAR *pboidReason,
    /* [in] */ BOOL fRetaining,
    /* [in] */ BOOL fAsync);


void __RPC_STUB ITransaction_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransaction_GetTransactionInfo_Proxy( 
    ITransaction __RPC_FAR * This,
    /* [out] */ XACTTRANSINFO __RPC_FAR *pinfo);


void __RPC_STUB ITransaction_GetTransactionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransaction_INTERFACE_DEFINED__ */


#ifndef __ITransactionDispenser_INTERFACE_DEFINED__
#define __ITransactionDispenser_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionDispenser
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionDispenser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionDispenser : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetOptionsObject( 
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions) = 0;
        
        virtual HRESULT __stdcall BeginTransaction( 
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOptions,
            /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransaction) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionDispenserVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ITransactionDispenser __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ITransactionDispenser __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ITransactionDispenser __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetOptionsObject )( 
            ITransactionDispenser __RPC_FAR * This,
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);
        
        HRESULT ( __stdcall __RPC_FAR *BeginTransaction )( 
            ITransactionDispenser __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOptions,
            /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransaction);
        
    } ITransactionDispenserVtbl;

    interface ITransactionDispenser
    {
        CONST_VTBL struct ITransactionDispenserVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionDispenser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionDispenser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionDispenser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionDispenser_GetOptionsObject(This,ppOptions)	\
    (This)->lpVtbl -> GetOptionsObject(This,ppOptions)

#define ITransactionDispenser_BeginTransaction(This,punkOuter,isoLevel,isoFlags,pOptions,ppTransaction)	\
    (This)->lpVtbl -> BeginTransaction(This,punkOuter,isoLevel,isoFlags,pOptions,ppTransaction)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITransactionDispenser_GetOptionsObject_Proxy( 
    ITransactionDispenser __RPC_FAR * This,
    /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);


void __RPC_STUB ITransactionDispenser_GetOptionsObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionDispenser_BeginTransaction_Proxy( 
    ITransactionDispenser __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *punkOuter,
    /* [in] */ ISOLEVEL isoLevel,
    /* [in] */ ULONG isoFlags,
    /* [in] */ ITransactionOptions __RPC_FAR *pOptions,
    /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransaction);


void __RPC_STUB ITransactionDispenser_BeginTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionDispenser_INTERFACE_DEFINED__ */


#ifndef __ITransactionOptions_INTERFACE_DEFINED__
#define __ITransactionOptions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionOptions
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionOptions : public IUnknown
    {
    public:
        virtual HRESULT __stdcall SetOptions( 
            /* [in] */ XACTOPT __RPC_FAR *pOptions) = 0;
        
        virtual HRESULT __stdcall GetOptions( 
            /* [out][in] */ XACTOPT __RPC_FAR *pOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionOptionsVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ITransactionOptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ITransactionOptions __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ITransactionOptions __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *SetOptions )( 
            ITransactionOptions __RPC_FAR * This,
            /* [in] */ XACTOPT __RPC_FAR *pOptions);
        
        HRESULT ( __stdcall __RPC_FAR *GetOptions )( 
            ITransactionOptions __RPC_FAR * This,
            /* [out][in] */ XACTOPT __RPC_FAR *pOptions);
        
    } ITransactionOptionsVtbl;

    interface ITransactionOptions
    {
        CONST_VTBL struct ITransactionOptionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionOptions_SetOptions(This,pOptions)	\
    (This)->lpVtbl -> SetOptions(This,pOptions)

#define ITransactionOptions_GetOptions(This,pOptions)	\
    (This)->lpVtbl -> GetOptions(This,pOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITransactionOptions_SetOptions_Proxy( 
    ITransactionOptions __RPC_FAR * This,
    /* [in] */ XACTOPT __RPC_FAR *pOptions);


void __RPC_STUB ITransactionOptions_SetOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionOptions_GetOptions_Proxy( 
    ITransactionOptions __RPC_FAR * This,
    /* [out][in] */ XACTOPT __RPC_FAR *pOptions);


void __RPC_STUB ITransactionOptions_GetOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionOptions_INTERFACE_DEFINED__ */


#ifndef __ITransactionOutcomeEvents_INTERFACE_DEFINED__
#define __ITransactionOutcomeEvents_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionOutcomeEvents
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionOutcomeEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionOutcomeEvents : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Committed( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall Aborted( 
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall HeuristicDecision( 
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall Indoubt( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionOutcomeEventsVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ITransactionOutcomeEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ITransactionOutcomeEvents __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ITransactionOutcomeEvents __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Committed )( 
            ITransactionOutcomeEvents __RPC_FAR * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *Aborted )( 
            ITransactionOutcomeEvents __RPC_FAR * This,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *HeuristicDecision )( 
            ITransactionOutcomeEvents __RPC_FAR * This,
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *Indoubt )( 
            ITransactionOutcomeEvents __RPC_FAR * This);
        
    } ITransactionOutcomeEventsVtbl;

    interface ITransactionOutcomeEvents
    {
        CONST_VTBL struct ITransactionOutcomeEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionOutcomeEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionOutcomeEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionOutcomeEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionOutcomeEvents_Committed(This,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Committed(This,fRetaining,pNewUOW,hr)

#define ITransactionOutcomeEvents_Aborted(This,pboidReason,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Aborted(This,pboidReason,fRetaining,pNewUOW,hr)

#define ITransactionOutcomeEvents_HeuristicDecision(This,dwDecision,pboidReason,hr)	\
    (This)->lpVtbl -> HeuristicDecision(This,dwDecision,pboidReason,hr)

#define ITransactionOutcomeEvents_Indoubt(This)	\
    (This)->lpVtbl -> Indoubt(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITransactionOutcomeEvents_Committed_Proxy( 
    ITransactionOutcomeEvents __RPC_FAR * This,
    /* [in] */ BOOL fRetaining,
    /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_Committed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionOutcomeEvents_Aborted_Proxy( 
    ITransactionOutcomeEvents __RPC_FAR * This,
    /* [in] */ BOID __RPC_FAR *pboidReason,
    /* [in] */ BOOL fRetaining,
    /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_Aborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionOutcomeEvents_HeuristicDecision_Proxy( 
    ITransactionOutcomeEvents __RPC_FAR * This,
    /* [in] */ DWORD dwDecision,
    /* [in] */ BOID __RPC_FAR *pboidReason,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionOutcomeEvents_HeuristicDecision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionOutcomeEvents_Indoubt_Proxy( 
    ITransactionOutcomeEvents __RPC_FAR * This);


void __RPC_STUB ITransactionOutcomeEvents_Indoubt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionOutcomeEvents_INTERFACE_DEFINED__ */


#ifndef __ITransactionCompletionEvents_INTERFACE_DEFINED__
#define __ITransactionCompletionEvents_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionCompletionEvents
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionCompletionEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionCompletionEvents : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Committed( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall Aborted( 
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall HeuristicDecision( 
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT __stdcall Indoubt( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionCompletionEventsVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ITransactionCompletionEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ITransactionCompletionEvents __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ITransactionCompletionEvents __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Committed )( 
            ITransactionCompletionEvents __RPC_FAR * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *Aborted )( 
            ITransactionCompletionEvents __RPC_FAR * This,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *HeuristicDecision )( 
            ITransactionCompletionEvents __RPC_FAR * This,
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr);
        
        HRESULT ( __stdcall __RPC_FAR *Indoubt )( 
            ITransactionCompletionEvents __RPC_FAR * This);
        
    } ITransactionCompletionEventsVtbl;

    interface ITransactionCompletionEvents
    {
        CONST_VTBL struct ITransactionCompletionEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionCompletionEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionCompletionEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionCompletionEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionCompletionEvents_Committed(This,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Committed(This,fRetaining,pNewUOW,hr)

#define ITransactionCompletionEvents_Aborted(This,pboidReason,fRetaining,pNewUOW,hr)	\
    (This)->lpVtbl -> Aborted(This,pboidReason,fRetaining,pNewUOW,hr)

#define ITransactionCompletionEvents_HeuristicDecision(This,dwDecision,pboidReason,hr)	\
    (This)->lpVtbl -> HeuristicDecision(This,dwDecision,pboidReason,hr)

#define ITransactionCompletionEvents_Indoubt(This)	\
    (This)->lpVtbl -> Indoubt(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ITransactionCompletionEvents_Committed_Proxy( 
    ITransactionCompletionEvents __RPC_FAR * This,
    /* [in] */ BOOL fRetaining,
    /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionCompletionEvents_Committed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionCompletionEvents_Aborted_Proxy( 
    ITransactionCompletionEvents __RPC_FAR * This,
    /* [in] */ BOID __RPC_FAR *pboidReason,
    /* [in] */ BOOL fRetaining,
    /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionCompletionEvents_Aborted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionCompletionEvents_HeuristicDecision_Proxy( 
    ITransactionCompletionEvents __RPC_FAR * This,
    /* [in] */ DWORD dwDecision,
    /* [in] */ BOID __RPC_FAR *pboidReason,
    /* [in] */ HRESULT hr);


void __RPC_STUB ITransactionCompletionEvents_HeuristicDecision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ITransactionCompletionEvents_Indoubt_Proxy( 
    ITransactionCompletionEvents __RPC_FAR * This);


void __RPC_STUB ITransactionCompletionEvents_Indoubt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionCompletionEvents_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0011
 * at Tue Nov 21 16:54:32 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#define XACT_E_FIRST                    0x8004D000
#define XACT_E_LAST                     0x8004D01E
#define XACT_S_FIRST                    0x0004D000
#define XACT_S_LAST                     0x0004D009

#define XACT_E_ABORTED                  0x8004D019
#define XACT_E_ALREADYOTHERSINGLEPHASE  0x8004D000
#define XACT_E_ALREADYINPROGRESS        0x8004D018
#define XACT_E_CANTRETAIN               0x8004D001
#define XACT_E_COMMITFAILED             0x8004D002
#define XACT_E_COMMITPREVENTED          0x8004D003
#define XACT_E_CONNECTION_DENIED        0x8004D01D
#define XACT_E_CONNECTION_DOWN          0x8004D01C
#define XACT_E_HEURISTICABORT           0x8004D004
#define XACT_E_HEURISTICCOMMIT          0x8004D005
#define XACT_E_HEURISTICDAMAGE          0x8004D006
#define XACT_E_HEURISTICDANGER          0x8004D007
#define XACT_E_INDOUBT                  0x8004D016
#define XACT_E_INVALIDCOOKIE            0x8004D015
#define XACT_E_ISOLATIONLEVEL           0x8004D008
#define XACT_E_LOGFULL                  0x8004D01A
#define XACT_E_NOASYNC                  0x8004D009
#define XACT_E_NOENLIST                 0x8004D00A
#define XACT_E_NOIMPORTOBJECT           0x8004D014
#define XACT_E_NOISORETAIN              0x8004D00B
#define XACT_E_NORESOURCE               0x8004D00C
#define XACT_E_NOTCURRENT               0x8004D00D
#define XACT_E_NOTIMEOUT                0x8004D017
#define XACT_E_NOTRANSACTION            0x8004D00E
#define XACT_E_NOTSUPPORTED             0x8004D00F
#define XACT_E_REENLISTTIMEOUT          0x8004D01E
#define XACT_E_TMNOTAVAILABLE           0x8004D01B
#define XACT_E_UNKNOWNRMGRID            0x8004D010
#define XACT_E_WRONGSTATE               0x8004D011
#define XACT_E_WRONGUOW                 0x8004D012
#define XACT_E_XTIONEXISTS              0x8004D013

#define XACT_S_ABORTING                 0x0004D008
#define XACT_S_ALLNORETAIN              0x0004D007
#define XACT_S_ASYNC                    0x0004D000
#define XACT_S_DEFECT                   0x0004D001
#define XACT_S_OKINFORM                 0x0004D004
#define XACT_S_MADECHANGESCONTENT       0x0004D005
#define XACT_S_MADECHANGESINFORM        0x0004D006
#define XACT_S_READONLY                 0x0004D002
#define XACT_S_SINGLEPHASE              0x0004D009
#define XACT_S_SOMENORETAIN             0x0004D003


extern RPC_IF_HANDLE __MIDL__intf_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0011_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

///////////////////////////////////////////////////////////////////////
//
// IID definitions for interfaces defined in this header file
//

#if !defined(_transact_iid_) && defined(INITGUID)
#define      _transact_iid_
const IID IID_ITransaction = {0x0fb15084,0xaf41,0x11ce,{0xbd,0x2b,0x20,0x4c,0x4f,0x4f,0x50,0x20}};
const IID IID_ITransactionDispenser = {0x3A6AD9E1,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}};
const IID IID_ITransactionOptions = {0x3A6AD9E0,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}};
const IID IID_ITransactionOutcomeEvents = {0x3A6AD9E2,0x23B9,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}};
const IID IID_ITransactionCompletionEvents = {0xB38D5220,0x23CE,0x11cf,{0xAD,0x60,0x00,0xAA,0x00,0xA7,0x4C,0xCD}};
#endif

///////////////////////////////////////////////////////////////////////

