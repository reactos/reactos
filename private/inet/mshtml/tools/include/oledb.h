/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.15 */
/* at Tue Aug 20 16:27:55 1996
 */
/* Compiler settings for oledb.idl:
    Os, W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __oledb_h__
#define __oledb_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISequentialStream_FWD_DEFINED__
#define __ISequentialStream_FWD_DEFINED__
typedef interface ISequentialStream ISequentialStream;
#endif 	/* __ISequentialStream_FWD_DEFINED__ */


#ifndef __IAccessor_FWD_DEFINED__
#define __IAccessor_FWD_DEFINED__
typedef interface IAccessor IAccessor;
#endif 	/* __IAccessor_FWD_DEFINED__ */


#ifndef __IRowset_FWD_DEFINED__
#define __IRowset_FWD_DEFINED__
typedef interface IRowset IRowset;
#endif 	/* __IRowset_FWD_DEFINED__ */


#ifndef __IRowsetInfo_FWD_DEFINED__
#define __IRowsetInfo_FWD_DEFINED__
typedef interface IRowsetInfo IRowsetInfo;
#endif 	/* __IRowsetInfo_FWD_DEFINED__ */


#ifndef __IRowsetLocate_FWD_DEFINED__
#define __IRowsetLocate_FWD_DEFINED__
typedef interface IRowsetLocate IRowsetLocate;
#endif 	/* __IRowsetLocate_FWD_DEFINED__ */


#ifndef __IRowsetResynch_FWD_DEFINED__
#define __IRowsetResynch_FWD_DEFINED__
typedef interface IRowsetResynch IRowsetResynch;
#endif 	/* __IRowsetResynch_FWD_DEFINED__ */


#ifndef __IRowsetScroll_FWD_DEFINED__
#define __IRowsetScroll_FWD_DEFINED__
typedef interface IRowsetScroll IRowsetScroll;
#endif 	/* __IRowsetScroll_FWD_DEFINED__ */


#ifndef __IRowsetExactScroll_FWD_DEFINED__
#define __IRowsetExactScroll_FWD_DEFINED__
typedef interface IRowsetExactScroll IRowsetExactScroll;
#endif 	/* __IRowsetExactScroll_FWD_DEFINED__ */


#ifndef __IRowsetChange_FWD_DEFINED__
#define __IRowsetChange_FWD_DEFINED__
typedef interface IRowsetChange IRowsetChange;
#endif 	/* __IRowsetChange_FWD_DEFINED__ */


#ifndef __IRowsetUpdate_FWD_DEFINED__
#define __IRowsetUpdate_FWD_DEFINED__
typedef interface IRowsetUpdate IRowsetUpdate;
#endif 	/* __IRowsetUpdate_FWD_DEFINED__ */


#ifndef __IRowsetNextRowset_FWD_DEFINED__
#define __IRowsetNextRowset_FWD_DEFINED__
typedef interface IRowsetNextRowset IRowsetNextRowset;
#endif 	/* __IRowsetNextRowset_FWD_DEFINED__ */


#ifndef __IRowsetIdentity_FWD_DEFINED__
#define __IRowsetIdentity_FWD_DEFINED__
typedef interface IRowsetIdentity IRowsetIdentity;
#endif 	/* __IRowsetIdentity_FWD_DEFINED__ */


#ifndef __IRowsetNewRowAfter_FWD_DEFINED__
#define __IRowsetNewRowAfter_FWD_DEFINED__
typedef interface IRowsetNewRowAfter IRowsetNewRowAfter;
#endif 	/* __IRowsetNewRowAfter_FWD_DEFINED__ */


#ifndef __IRowsetWithParameters_FWD_DEFINED__
#define __IRowsetWithParameters_FWD_DEFINED__
typedef interface IRowsetWithParameters IRowsetWithParameters;
#endif 	/* __IRowsetWithParameters_FWD_DEFINED__ */


#ifndef __IRowsetFind_FWD_DEFINED__
#define __IRowsetFind_FWD_DEFINED__
typedef interface IRowsetFind IRowsetFind;
#endif 	/* __IRowsetFind_FWD_DEFINED__ */


#ifndef __IRowsetAsynch_FWD_DEFINED__
#define __IRowsetAsynch_FWD_DEFINED__
typedef interface IRowsetAsynch IRowsetAsynch;
#endif 	/* __IRowsetAsynch_FWD_DEFINED__ */


#ifndef __IRowsetKeys_FWD_DEFINED__
#define __IRowsetKeys_FWD_DEFINED__
typedef interface IRowsetKeys IRowsetKeys;
#endif 	/* __IRowsetKeys_FWD_DEFINED__ */


#ifndef __IRowsetNotify_FWD_DEFINED__
#define __IRowsetNotify_FWD_DEFINED__
typedef interface IRowsetNotify IRowsetNotify;
#endif 	/* __IRowsetNotify_FWD_DEFINED__ */


#ifndef __IRowsetIndex_FWD_DEFINED__
#define __IRowsetIndex_FWD_DEFINED__
typedef interface IRowsetIndex IRowsetIndex;
#endif 	/* __IRowsetIndex_FWD_DEFINED__ */


#ifndef __IRowsetWatchAll_FWD_DEFINED__
#define __IRowsetWatchAll_FWD_DEFINED__
typedef interface IRowsetWatchAll IRowsetWatchAll;
#endif 	/* __IRowsetWatchAll_FWD_DEFINED__ */


#ifndef __IRowsetWatchNotify_FWD_DEFINED__
#define __IRowsetWatchNotify_FWD_DEFINED__
typedef interface IRowsetWatchNotify IRowsetWatchNotify;
#endif 	/* __IRowsetWatchNotify_FWD_DEFINED__ */


#ifndef __IRowsetWatchRegion_FWD_DEFINED__
#define __IRowsetWatchRegion_FWD_DEFINED__
typedef interface IRowsetWatchRegion IRowsetWatchRegion;
#endif 	/* __IRowsetWatchRegion_FWD_DEFINED__ */


#ifndef __IRowsetCopyRows_FWD_DEFINED__
#define __IRowsetCopyRows_FWD_DEFINED__
typedef interface IRowsetCopyRows IRowsetCopyRows;
#endif 	/* __IRowsetCopyRows_FWD_DEFINED__ */


#ifndef __IReadData_FWD_DEFINED__
#define __IReadData_FWD_DEFINED__
typedef interface IReadData IReadData;
#endif 	/* __IReadData_FWD_DEFINED__ */


#ifndef __ICommand_FWD_DEFINED__
#define __ICommand_FWD_DEFINED__
typedef interface ICommand ICommand;
#endif 	/* __ICommand_FWD_DEFINED__ */


#ifndef __IConvertType_FWD_DEFINED__
#define __IConvertType_FWD_DEFINED__
typedef interface IConvertType IConvertType;
#endif 	/* __IConvertType_FWD_DEFINED__ */


#ifndef __ICommandCost_FWD_DEFINED__
#define __ICommandCost_FWD_DEFINED__
typedef interface ICommandCost ICommandCost;
#endif 	/* __ICommandCost_FWD_DEFINED__ */


#ifndef __ICommandPrepare_FWD_DEFINED__
#define __ICommandPrepare_FWD_DEFINED__
typedef interface ICommandPrepare ICommandPrepare;
#endif 	/* __ICommandPrepare_FWD_DEFINED__ */


#ifndef __ICommandProperties_FWD_DEFINED__
#define __ICommandProperties_FWD_DEFINED__
typedef interface ICommandProperties ICommandProperties;
#endif 	/* __ICommandProperties_FWD_DEFINED__ */


#ifndef __ICommandText_FWD_DEFINED__
#define __ICommandText_FWD_DEFINED__
typedef interface ICommandText ICommandText;
#endif 	/* __ICommandText_FWD_DEFINED__ */


#ifndef __ICommandTree_FWD_DEFINED__
#define __ICommandTree_FWD_DEFINED__
typedef interface ICommandTree ICommandTree;
#endif 	/* __ICommandTree_FWD_DEFINED__ */


#ifndef __ICommandValidate_FWD_DEFINED__
#define __ICommandValidate_FWD_DEFINED__
typedef interface ICommandValidate ICommandValidate;
#endif 	/* __ICommandValidate_FWD_DEFINED__ */


#ifndef __ICommandWithParameters_FWD_DEFINED__
#define __ICommandWithParameters_FWD_DEFINED__
typedef interface ICommandWithParameters ICommandWithParameters;
#endif 	/* __ICommandWithParameters_FWD_DEFINED__ */


#ifndef __IQuery_FWD_DEFINED__
#define __IQuery_FWD_DEFINED__
typedef interface IQuery IQuery;
#endif 	/* __IQuery_FWD_DEFINED__ */


#ifndef __IColumnsRowset_FWD_DEFINED__
#define __IColumnsRowset_FWD_DEFINED__
typedef interface IColumnsRowset IColumnsRowset;
#endif 	/* __IColumnsRowset_FWD_DEFINED__ */


#ifndef __IColumnsInfo_FWD_DEFINED__
#define __IColumnsInfo_FWD_DEFINED__
typedef interface IColumnsInfo IColumnsInfo;
#endif 	/* __IColumnsInfo_FWD_DEFINED__ */


#ifndef __IDBCreateCommand_FWD_DEFINED__
#define __IDBCreateCommand_FWD_DEFINED__
typedef interface IDBCreateCommand IDBCreateCommand;
#endif 	/* __IDBCreateCommand_FWD_DEFINED__ */


#ifndef __IDBCreateSession_FWD_DEFINED__
#define __IDBCreateSession_FWD_DEFINED__
typedef interface IDBCreateSession IDBCreateSession;
#endif 	/* __IDBCreateSession_FWD_DEFINED__ */


#ifndef __ISourcesRowset_FWD_DEFINED__
#define __ISourcesRowset_FWD_DEFINED__
typedef interface ISourcesRowset ISourcesRowset;
#endif 	/* __ISourcesRowset_FWD_DEFINED__ */


#ifndef __IDBProperties_FWD_DEFINED__
#define __IDBProperties_FWD_DEFINED__
typedef interface IDBProperties IDBProperties;
#endif 	/* __IDBProperties_FWD_DEFINED__ */


#ifndef __IDBInitialize_FWD_DEFINED__
#define __IDBInitialize_FWD_DEFINED__
typedef interface IDBInitialize IDBInitialize;
#endif 	/* __IDBInitialize_FWD_DEFINED__ */


#ifndef __IDBInfo_FWD_DEFINED__
#define __IDBInfo_FWD_DEFINED__
typedef interface IDBInfo IDBInfo;
#endif 	/* __IDBInfo_FWD_DEFINED__ */


#ifndef __IDBDataSourceAdmin_FWD_DEFINED__
#define __IDBDataSourceAdmin_FWD_DEFINED__
typedef interface IDBDataSourceAdmin IDBDataSourceAdmin;
#endif 	/* __IDBDataSourceAdmin_FWD_DEFINED__ */


#ifndef __ISessionProperties_FWD_DEFINED__
#define __ISessionProperties_FWD_DEFINED__
typedef interface ISessionProperties ISessionProperties;
#endif 	/* __ISessionProperties_FWD_DEFINED__ */


#ifndef __IIndexDefinition_FWD_DEFINED__
#define __IIndexDefinition_FWD_DEFINED__
typedef interface IIndexDefinition IIndexDefinition;
#endif 	/* __IIndexDefinition_FWD_DEFINED__ */


#ifndef __ITableDefinition_FWD_DEFINED__
#define __ITableDefinition_FWD_DEFINED__
typedef interface ITableDefinition ITableDefinition;
#endif 	/* __ITableDefinition_FWD_DEFINED__ */


#ifndef __IOpenRowset_FWD_DEFINED__
#define __IOpenRowset_FWD_DEFINED__
typedef interface IOpenRowset IOpenRowset;
#endif 	/* __IOpenRowset_FWD_DEFINED__ */


#ifndef __ITableRename_FWD_DEFINED__
#define __ITableRename_FWD_DEFINED__
typedef interface ITableRename ITableRename;
#endif 	/* __ITableRename_FWD_DEFINED__ */


#ifndef __IDBSchemaCommand_FWD_DEFINED__
#define __IDBSchemaCommand_FWD_DEFINED__
typedef interface IDBSchemaCommand IDBSchemaCommand;
#endif 	/* __IDBSchemaCommand_FWD_DEFINED__ */


#ifndef __IDBSecurityInfo_FWD_DEFINED__
#define __IDBSecurityInfo_FWD_DEFINED__
typedef interface IDBSecurityInfo IDBSecurityInfo;
#endif 	/* __IDBSecurityInfo_FWD_DEFINED__ */


#ifndef __IDBSchemaRowset_FWD_DEFINED__
#define __IDBSchemaRowset_FWD_DEFINED__
typedef interface IDBSchemaRowset IDBSchemaRowset;
#endif 	/* __IDBSchemaRowset_FWD_DEFINED__ */


#ifndef __IProvideMoniker_FWD_DEFINED__
#define __IProvideMoniker_FWD_DEFINED__
typedef interface IProvideMoniker IProvideMoniker;
#endif 	/* __IProvideMoniker_FWD_DEFINED__ */


#ifndef __IErrorRecords_FWD_DEFINED__
#define __IErrorRecords_FWD_DEFINED__
typedef interface IErrorRecords IErrorRecords;
#endif 	/* __IErrorRecords_FWD_DEFINED__ */


#ifndef __IErrorLookup_FWD_DEFINED__
#define __IErrorLookup_FWD_DEFINED__
typedef interface IErrorLookup IErrorLookup;
#endif 	/* __IErrorLookup_FWD_DEFINED__ */


#ifndef __ISQLErrorInfo_FWD_DEFINED__
#define __ISQLErrorInfo_FWD_DEFINED__
typedef interface ISQLErrorInfo ISQLErrorInfo;
#endif 	/* __ISQLErrorInfo_FWD_DEFINED__ */


#ifndef __IGetDataSource_FWD_DEFINED__
#define __IGetDataSource_FWD_DEFINED__
typedef interface IGetDataSource IGetDataSource;
#endif 	/* __IGetDataSource_FWD_DEFINED__ */


#ifndef __ITransactionLocal_FWD_DEFINED__
#define __ITransactionLocal_FWD_DEFINED__
typedef interface ITransactionLocal ITransactionLocal;
#endif 	/* __ITransactionLocal_FWD_DEFINED__ */


#ifndef __ITransactionJoin_FWD_DEFINED__
#define __ITransactionJoin_FWD_DEFINED__
typedef interface ITransactionJoin ITransactionJoin;
#endif 	/* __ITransactionJoin_FWD_DEFINED__ */


#ifndef __ITransactionObject_FWD_DEFINED__
#define __ITransactionObject_FWD_DEFINED__
typedef interface ITransactionObject ITransactionObject;
#endif 	/* __ITransactionObject_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "oaidl.h"
#include "transact.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


//+---------------------------------------------------------------------------
//
//  Microsoft OLE DB
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//----------------------------------------------------------------------------

#include <pshpack2.h>	// 2-byte structure packing

//
// OLEDBVER
//	OLE DB version number (0x0100); to use version 2.0 features,
//	#define OLEDBVER 0x0200 before including this file.
//

// If OLEDBVER is not defined, assume version 1.0
// For Trident, we need 2.0 (for IRowsetExactScroll and other things) - SWB
#ifndef OLEDBVER
#define OLEDBVER 0x0200
#endif


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __ISequentialStream_INTERFACE_DEFINED__
#define __ISequentialStream_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISequentialStream
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ISequentialStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISequentialStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISequentialStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISequentialStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISequentialStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISequentialStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            ISequentialStream __RPC_FAR * This,
            /* [out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            ISequentialStream __RPC_FAR * This,
            /* [in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
        
        END_INTERFACE
    } ISequentialStreamVtbl;

    interface ISequentialStream
    {
        CONST_VTBL struct ISequentialStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISequentialStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISequentialStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISequentialStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISequentialStream_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define ISequentialStream_Write(This,pv,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pv,cb,pcbWritten)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISequentialStream_Read_Proxy( 
    ISequentialStream __RPC_FAR * This,
    /* [out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead);


void __RPC_STUB ISequentialStream_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISequentialStream_Write_Proxy( 
    ISequentialStream __RPC_FAR * This,
    /* [in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten);


void __RPC_STUB ISequentialStream_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISequentialStream_INTERFACE_DEFINED__ */


#ifndef __DBStructureDefinitions_INTERFACE_DEFINED__
#define __DBStructureDefinitions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: DBStructureDefinitions
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][unique][uuid] */ 


typedef DWORD DBKIND;


enum DBKINDENUM
    {	DBKIND_GUID_NAME	= 0,
	DBKIND_GUID_PROPID	= DBKIND_GUID_NAME + 1,
	DBKIND_NAME	= DBKIND_GUID_PROPID + 1,
	DBKIND_PGUID_NAME	= DBKIND_NAME + 1,
	DBKIND_PGUID_PROPID	= DBKIND_PGUID_NAME + 1,
	DBKIND_PROPID	= DBKIND_PGUID_PROPID + 1,
	DBKIND_GUID	= DBKIND_PROPID + 1
    };
typedef struct  tagDBID
    {
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ GUID guid;
        /* [case()] */ GUID __RPC_FAR *pguid;
        }	uGuid;
    DBKIND eKind;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ LPOLESTR pwszName;
        /* [case()] */ ULONG ulPropid;
        }	uName;
    }	DBID;

typedef struct  tagDB_NUMERIC
    {
    BYTE precision;
    BYTE scale;
    BYTE sign;
    BYTE val[ 16 ];
    }	DB_NUMERIC;

typedef struct  tagDBVECTOR
    {
    ULONG size;
    /* [size_is] */ void __RPC_FAR *ptr;
    }	DBVECTOR;

typedef struct  tagDBDATE
    {
    SHORT year;
    USHORT month;
    USHORT day;
    }	DBDATE;

typedef struct  tagDBTIME
    {
    USHORT hour;
    USHORT minute;
    USHORT second;
    }	DBTIME;

typedef struct  tagDBTIMESTAMP
    {
    SHORT year;
    USHORT month;
    USHORT day;
    USHORT hour;
    USHORT minute;
    USHORT second;
    ULONG fraction;
    }	DBTIMESTAMP;

typedef struct  tagDB_DECIMAL
    {
    short vt;
    short sign	: 8;
    short scale	: 8;
    unsigned long HiPart;
    MIDL_uhyper LoPart;
    }	DB_DECIMAL;

typedef WORD DBTYPE;


enum DBTYPEENUM
    {	DBTYPE_EMPTY	= 0,
	DBTYPE_NULL	= 1,
	DBTYPE_I2	= 2,
	DBTYPE_I4	= 3,
	DBTYPE_R4	= 4,
	DBTYPE_R8	= 5,
	DBTYPE_CY	= 6,
	DBTYPE_DATE	= 7,
	DBTYPE_BSTR	= 8,
	DBTYPE_IDISPATCH	= 9,
	DBTYPE_ERROR	= 10,
	DBTYPE_BOOL	= 11,
	DBTYPE_VARIANT	= 12,
	DBTYPE_IUNKNOWN	= 13,
	DBTYPE_DECIMAL	= 14,
	DBTYPE_UI1	= 17,
	DBTYPE_ARRAY	= 0x2000,
	DBTYPE_BYREF	= 0x4000,
	DBTYPE_I1	= 16,
	DBTYPE_UI2	= 18,
	DBTYPE_UI4	= 19,
	DBTYPE_I8	= 20,
	DBTYPE_UI8	= 21,
	DBTYPE_GUID	= 72,
	DBTYPE_VECTOR	= 0x1000,
	DBTYPE_RESERVED	= 0x8000,
	DBTYPE_BYTES	= 128,
	DBTYPE_STR	= 129,
	DBTYPE_WSTR	= 130,
	DBTYPE_NUMERIC	= 131,
	DBTYPE_UDT	= 132,
	DBTYPE_DBDATE	= 133,
	DBTYPE_DBTIME	= 134,
	DBTYPE_DBTIMESTAMP	= 135
    };
typedef DWORD DBPART;


enum DBPARTENUM
    {	DBPART_INVALID	= 0,
	DBPART_VALUE	= 0x1,
	DBPART_LENGTH	= 0x2,
	DBPART_STATUS	= 0x4
    };
typedef DWORD DBPARAMIO;


enum DBPARAMIOENUM
    {	DBPARAMIO_NOTPARAM	= 0,
	DBPARAMIO_INPUT	= 0x1,
	DBPARAMIO_OUTPUT	= 0x2
    };
typedef DWORD DBMEMOWNER;


enum DBMEMOWNERENUM
    {	DBMEMOWNER_CLIENTOWNED	= 0,
	DBMEMOWNER_PROVIDEROWNED	= 0x1
    };
typedef struct  tagDBOBJECT
    {
    DWORD dwFlags;
    IID iid;
    }	DBOBJECT;

typedef DWORD DBSTATUS;


enum DBSTATUSENUM
    {	DBSTATUS_S_OK	= 0,
	DBSTATUS_E_BADACCESSOR	= 1,
	DBSTATUS_E_CANTCONVERTVALUE	= 2,
	DBSTATUS_S_ISNULL	= 3,
	DBSTATUS_S_TRUNCATED	= 4,
	DBSTATUS_E_SIGNMISMATCH	= 5,
	DBSTATUS_E_DATAOVERFLOW	= 6,
	DBSTATUS_E_CANTCREATE	= 7,
	DBSTATUS_E_UNAVAILABLE	= 8,
	DBSTATUS_E_PERMISSIONDENIED	= 9,
	DBSTATUS_E_INTEGRITYVIOLATION	= 10,
	DBSTATUS_E_SCHEMAVIOLATION	= 11,
	DBSTATUS_E_BADSTATUS	= 12
    };
typedef struct  tagDBBINDEXT
    {
    /* [size_is] */ BYTE __RPC_FAR *pExtension;
    ULONG ulExtension;
    }	DBBINDEXT;

typedef struct  tagDBBINDING
    {
    ULONG iOrdinal;
    ULONG obValue;
    ULONG obLength;
    ULONG obStatus;
    ITypeInfo __RPC_FAR *pTypeInfo;
    DBOBJECT __RPC_FAR *pObject;
    DBBINDEXT __RPC_FAR *pBindExt;
    DBPART dwPart;
    DBMEMOWNER dwMemOwner;
    DBPARAMIO eParamIO;
    ULONG cbMaxLen;
    DWORD dwFlags;
    DBTYPE wType;
    BYTE bPrecision;
    BYTE bScale;
    }	DBBINDING;

typedef DWORD DBROWSTATUS;


enum DBROWSTATUSENUM
    {	DBROWSTATUS_S_OK	= 0,
	DBROWSTATUS_S_MULTIPLECHANGES	= 2,
	DBROWSTATUS_S_PENDINGCHANGES	= 3,
	DBROWSTATUS_E_CANCELED	= 4,
	DBROWSTATUS_E_CANTRELEASE	= 6,
	DBROWSTATUS_E_CONCURRENCYVIOLATION	= 7,
	DBROWSTATUS_E_DELETED	= 8,
	DBROWSTATUS_E_PENDINGINSERT	= 9,
	DBROWSTATUS_E_NEWLYINSERTED	= 10,
	DBROWSTATUS_E_INTEGRITYVIOLATION	= 11,
	DBROWSTATUS_E_INVALID	= 12,
	DBROWSTATUS_E_MAXPENDCHANGESEXCEEDED	= 13,
	DBROWSTATUS_E_OBJECTOPEN	= 14,
	DBROWSTATUS_E_OUTOFMEMORY	= 15,
	DBROWSTATUS_E_PERMISSIONDENIED	= 16,
	DBROWSTATUS_E_LIMITREACHED	= 17,
	DBROWSTATUS_E_SCHEMAVIOLATION	= 18
    };
typedef ULONG HACCESSOR;

#define DB_NULL_HACCESSOR 0x00
typedef ULONG HROW;

#define DB_NULL_HROW 0x00
typedef ULONG HWATCHREGION;

#define DBWATCHREGION_NULL NULL
typedef ULONG HCHAPTER;

#define DB_INVALID_HCHAPTER 0x00
typedef struct  tagDBFAILUREINFO
    {
    HROW hRow;
    ULONG iColumn;
    HRESULT failure;
    }	DBFAILUREINFO;

typedef DWORD DBCOLUMNFLAGS;


enum DBCOLUMNFLAGSENUM
    {	DBCOLUMNFLAGS_ISBOOKMARK	= 0x1,
	DBCOLUMNFLAGS_MAYDEFER	= 0x2,
	DBCOLUMNFLAGS_WRITE	= 0x4,
	DBCOLUMNFLAGS_WRITEUNKNOWN	= 0x8,
	DBCOLUMNFLAGS_ISFIXEDLENGTH	= 0x10,
	DBCOLUMNFLAGS_ISNULLABLE	= 0x20,
	DBCOLUMNFLAGS_MAYBENULL	= 0x40,
	DBCOLUMNFLAGS_ISLONG	= 0x80,
	DBCOLUMNFLAGS_ISROWID	= 0x100,
	DBCOLUMNFLAGS_ISROWVER	= 0x200,
	DBCOLUMNFLAGS_CACHEDEFERRED	= 0x1000
    };
typedef struct  tagDBCOLUMNINFO
    {
    LPOLESTR pwszName;
    ITypeInfo __RPC_FAR *pTypeInfo;
    ULONG iOrdinal;
    DBCOLUMNFLAGS dwFlags;
    ULONG ulColumnSize;
    DBTYPE wType;
    BYTE bPrecision;
    BYTE bScale;
    DBID columnid;
    }	DBCOLUMNINFO;

typedef 
enum tagDBBOOKMARK
    {	DBBMK_INVALID	= 0,
	DBBMK_FIRST	= DBBMK_INVALID + 1,
	DBBMK_LAST	= DBBMK_FIRST + 1
    }	DBBOOKMARK;

#ifdef __cplusplus
inline BOOL IsEqualGUIDBase(const GUID &rguid1, const GUID &rguid2)
{ return !memcmp(&(rguid1.Data2), &(rguid1.Data2), sizeof(GUID) - sizeof(rguid1.Data1)); }
#else // !__cplusplus
#define IsEqualGuidBase(rguid1, rguid2) \
	(!memcmp(&((rguid1).Data2), &((rguid2).Data2), sizeof(GUID) - sizeof((rguid1).Data1)))
#endif // __cplusplus
#define DB_INVALIDCOLUMN ULONG_MAX
#define DBCIDGUID   {0x0C733A81L,0x2A1C,0x11CE,{0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D}}
#define DB_NULLGUID {0x00000000L,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
#ifdef DBINITCONSTANTS
extern const DBID DB_NULLID                      = {DB_NULLGUID, 0, (LPOLESTR)0};
extern const DBID DBCOLUMN_IDNAME                = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)2};
extern const DBID DBCOLUMN_NAME                  = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)3};
extern const DBID DBCOLUMN_NUMBER                = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)4};
extern const DBID DBCOLUMN_TYPE                  = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)5};
extern const DBID DBCOLUMN_PRECISION             = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)7};
extern const DBID DBCOLUMN_SCALE                 = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)8};
extern const DBID DBCOLUMN_FLAGS                 = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)9};
extern const DBID DBCOLUMN_BASECOLUMNNAME        = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)10};
extern const DBID DBCOLUMN_BASETABLENAME         = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)11};
extern const DBID DBCOLUMN_COLLATINGSEQUENCE     = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)12};
extern const DBID DBCOLUMN_COMPUTEMODE           = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)13};
extern const DBID DBCOLUMN_DEFAULTVALUE          = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)14};
extern const DBID DBCOLUMN_DOMAINNAME            = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)15};
extern const DBID DBCOLUMN_HASDEFAULT            = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)16};
extern const DBID DBCOLUMN_ISAUTOINCREMENT       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)17};
extern const DBID DBCOLUMN_ISCASESENSITIVE       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)18};
extern const DBID DBCOLUMN_ISSEARCHABLE          = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)20};
extern const DBID DBCOLUMN_ISUNIQUE              = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)21};
extern const DBID DBCOLUMN_BASECATALOGNAME       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)23};
extern const DBID DBCOLUMN_BASESCHEMANAME        = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)24};
extern const DBID DBCOLUMN_GUID                  = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)29};
extern const DBID DBCOLUMN_PROPID                = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)30};
extern const DBID DBCOLUMN_TYPEINFO              = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)31};
extern const DBID DBCOLUMN_DOMAINCATALOG         = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)32};
extern const DBID DBCOLUMN_DOMAINSCHEMA          = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)33};
extern const DBID DBCOLUMN_DATETIMEPRECISION     = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)34};
extern const DBID DBCOLUMN_NUMERICPRECISIONRADIX = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)35};
extern const DBID DBCOLUMN_OCTETLENGTH           = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)36};
extern const DBID DBCOLUMN_COLUMNSIZE            = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)37};
extern const DBID DBCOLUMN_CLSID                 = {DBCIDGUID, DBKIND_GUID_PROPID, (LPOLESTR)38};
#else // !DBINITCONSTANTS
extern const DBID DB_NULLID;
extern const DBID DBCOLUMN_IDNAME;
extern const DBID DBCOLUMN_NAME;
extern const DBID DBCOLUMN_NUMBER;
extern const DBID DBCOLUMN_TYPE;
extern const DBID DBCOLUMN_PRECISION;
extern const DBID DBCOLUMN_SCALE;
extern const DBID DBCOLUMN_FLAGS;
extern const DBID DBCOLUMN_BASECOLUMNNAME;
extern const DBID DBCOLUMN_BASETABLENAME;
extern const DBID DBCOLUMN_COLLATINGSEQUENCE;
extern const DBID DBCOLUMN_COMPUTEMODE;
extern const DBID DBCOLUMN_DEFAULTVALUE;
extern const DBID DBCOLUMN_DOMAINNAME;
extern const DBID DBCOLUMN_HASDEFAULT;
extern const DBID DBCOLUMN_ISAUTOINCREMENT;
extern const DBID DBCOLUMN_ISCASESENSITIVE;
extern const DBID DBCOLUMN_ISSEARCHABLE;
extern const DBID DBCOLUMN_ISUNIQUE;
extern const DBID DBCOLUMN_BASECATALOGNAME;
extern const DBID DBCOLUMN_BASESCHEMANAME;
extern const DBID DBCOLUMN_GUID;
extern const DBID DBCOLUMN_PROPID;
extern const DBID DBCOLUMN_TYPEINFO;
extern const DBID DBCOLUMN_DOMAINCATALOG;
extern const DBID DBCOLUMN_DOMAINSCHEMA;
extern const DBID DBCOLUMN_DATETIMEPRECISION;
extern const DBID DBCOLUMN_NUMERICPRECISIONRADIX;
extern const DBID DBCOLUMN_OCTETLENGTH;
extern const DBID DBCOLUMN_COLUMNSIZE;
extern const DBID DBCOLUMN_CLSID;
#endif // DBINITCONSTANTS
#ifdef DBINITCONSTANTS
extern const GUID DB_PROPERTY_CHECK_OPTION               = {0xc8b5220b,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_CONSTRAINT_CHECK_DEFERRED  = {0xc8b521f0,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_DROP_CASCADE               = {0xc8b521f3,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_UNIQUE                     = {0xc8b521f5,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_ON_COMMIT_PRESERVE_ROWS    = {0xc8b52230,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_PRIMARY                    = {0xc8b521fc,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_CLUSTERED                  = {0xc8b521ff,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_NONCLUSTERED               = {0xc8b52200,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_BTREE                      = {0xc8b52201,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_HASH                       = {0xc8b52202,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_FILLFACTOR                 = {0xc8b52203,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_INITIALSIZE                = {0xc8b52204,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_DISALLOWNULL               = {0xc8b52205,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_IGNORENULL                 = {0xc8b52206,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_IGNOREANYNULL              = {0xc8b52207,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_SORTBOOKMARKS              = {0xc8b52208,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_AUTOMATICUPDATE            = {0xc8b52209,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DB_PROPERTY_EXPLICITUPDATE             = {0xc8b5220a,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_ASSERTIONS                    = {0xc8b52210,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_CATALOGS                      = {0xc8b52211,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_CHARACTER_SETS                = {0xc8b52212,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_COLLATIONS                    = {0xc8b52213,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_COLUMNS                       = {0xc8b52214,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_CHECK_CONSTRAINTS             = {0xc8b52215,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_CONSTRAINT_COLUMN_USAGE       = {0xc8b52216,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_CONSTRAINT_TABLE_USAGE        = {0xc8b52217,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_KEY_COLUMN_USAGE              = {0xc8b52218,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_REFERENTIAL_CONSTRAINTS       = {0xc8b52219,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_TABLE_CONSTRAINTS             = {0xc8b5221a,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_COLUMN_DOMAIN_USAGE           = {0xc8b5221b,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_INDEXES                       = {0xc8b5221e,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_COLUMN_PRIVILEGES             = {0xc8b52221,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_TABLE_PRIVILEGES              = {0xc8b52222,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_USAGE_PRIVILEGES              = {0xc8b52223,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_PROCEDURES                    = {0xc8b52224,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_SCHEMATA                      = {0xc8b52225,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_SQL_LANGUAGES                 = {0xc8b52226,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_STATISTICS                    = {0xc8b52227,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_TABLES                        = {0xc8b52229,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_TRANSLATIONS                  = {0xc8b5222a,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_PROVIDER_TYPES                = {0xc8b5222c,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_VIEWS                         = {0xc8b5222d,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_VIEW_COLUMN_USAGE             = {0xc8b5222e,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_VIEW_TABLE_USAGE              = {0xc8b5222f,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_PROCEDURE_PARAMETERS          = {0xc8b522b8,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_FOREIGN_KEYS                  = {0xc8b522c4,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_PRIMARY_KEYS                  = {0xc8b522c5,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBSCHEMA_PROCEDURE_COLUMNS             = {0xc8b522c9,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBCOL_SELFCOLUMNS                      = {0xc8b52231,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBCOL_SPECIALCOL                       = {0xc8b52232,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID PSGUID_QUERY                           = {0x49691c90,0x7e17,0x101a,{0xa9,0x1c,0x08,0x00,0x2b,0x2e,0xcd,0xa9}};
extern const GUID DBPROPSET_COLUMN                       = {0xc8b522b9,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DATASOURCE                   = {0xc8b522ba,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DATASOURCEINFO               = {0xc8b522bb,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DBINIT                       = {0xc8b522bc,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_INDEX                        = {0xc8b522bd,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_ROWSET                       = {0xc8b522be,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_TABLE                        = {0xc8b522bf,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DATASOURCEALL                = {0xc8b522c0,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DATASOURCEINFOALL            = {0xc8b522c1,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_ROWSETALL                    = {0xc8b522c2,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_SESSION                      = {0xc8b522c6,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_SESSIONALL                   = {0xc8b522c7,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_DBINITALL                    = {0xc8b522ca,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBPROPSET_PROPERTIESINERROR            = {0xc8b522d4,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
extern const GUID DBGUID_DBSQL                           = {0xc8b521fb,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
#else // !DBINITCONSTANTS
extern const GUID DB_PROPERTY_CHECK_OPTION;
extern const GUID DB_PROPERTY_CONSTRAINT_CHECK_DEFERRED;
extern const GUID DB_PROPERTY_DROP_CASCADE;
extern const GUID DB_PROPERTY_ON_COMMIT_PRESERVE_ROWS;
extern const GUID DB_PROPERTY_UNIQUE;
extern const GUID DB_PROPERTY_PRIMARY;
extern const GUID DB_PROPERTY_CLUSTERED;
extern const GUID DB_PROPERTY_NONCLUSTERED;
extern const GUID DB_PROPERTY_BTREE;
extern const GUID DB_PROPERTY_HASH;
extern const GUID DB_PROPERTY_FILLFACTOR;
extern const GUID DB_PROPERTY_INITIALSIZE;
extern const GUID DB_PROPERTY_DISALLOWNULL;
extern const GUID DB_PROPERTY_IGNORENULL;
extern const GUID DB_PROPERTY_IGNOREANYNULL;
extern const GUID DB_PROPERTY_SORTBOOKMARKS;
extern const GUID DB_PROPERTY_AUTOMATICUPDATE;
extern const GUID DB_PROPERTY_EXPLICITUPDATE;
extern const GUID DBSCHEMA_ASSERTIONS;
extern const GUID DBSCHEMA_CATALOGS;
extern const GUID DBSCHEMA_CHARACTER_SETS;
extern const GUID DBSCHEMA_COLLATIONS;
extern const GUID DBSCHEMA_COLUMNS;
extern const GUID DBSCHEMA_CHECK_CONSTRAINTS;
extern const GUID DBSCHEMA_CONSTRAINT_COLUMN_USAGE;
extern const GUID DBSCHEMA_CONSTRAINT_TABLE_USAGE;
extern const GUID DBSCHEMA_KEY_COLUMN_USAGE;
extern const GUID DBSCHEMA_REFERENTIAL_CONSTRAINTS;
extern const GUID DBSCHEMA_TABLE_CONSTRAINTS;
extern const GUID DBSCHEMA_COLUMN_DOMAIN_USAGE;
extern const GUID DBSCHEMA_INDEXES;
extern const GUID DBSCHEMA_COLUMN_PRIVILEGES;
extern const GUID DBSCHEMA_TABLE_PRIVILEGES;
extern const GUID DBSCHEMA_USAGE_PRIVILEGES;
extern const GUID DBSCHEMA_PROCEDURES;
extern const GUID DBSCHEMA_SCHEMATA;
extern const GUID DBSCHEMA_SQL_LANGUAGES;
extern const GUID DBSCHEMA_STATISTICS;
extern const GUID DBSCHEMA_TABLES;
extern const GUID DBSCHEMA_TRANSLATIONS;
extern const GUID DBSCHEMA_PROVIDER_TYPES;
extern const GUID DBSCHEMA_VIEWS;
extern const GUID DBSCHEMA_VIEW_COLUMN_USAGE;
extern const GUID DBSCHEMA_VIEW_TABLE_USAGE;
extern const GUID DBSCHEMA_PROCEDURE_PARAMETERS;
extern const GUID DBSCHEMA_FOREIGN_KEYS;
extern const GUID DBSCHEMA_PRIMARY_KEYS;
extern const GUID DBSCHEMA_PROCEDURE_COLUMNS;
extern const GUID DBCOL_SELFCOLUMNS;
extern const GUID DBCOL_SPECIALCOL;
extern const GUID PSGUID_QUERY;
extern const GUID DBPROPSET_COLUMN;
extern const GUID DBPROPSET_DATASOURCE;
extern const GUID DBPROPSET_DATASOURCEINFO;
extern const GUID DBPROPSET_DBINIT;
extern const GUID DBPROPSET_INDEX;
extern const GUID DBPROPSET_ROWSET;
extern const GUID DBPROPSET_TABLE;
extern const GUID DBPROPSET_DATASOURCEALL;
extern const GUID DBPROPSET_DATASOURCEINFOALL;
extern const GUID DBPROPSET_ROWSETALL;
extern const GUID DBPROPSET_SESSION;
extern const GUID DBPROPSET_SESSIONALL;
extern const GUID DBPROPSET_DBINITALL;
extern const GUID DBPROPSET_PROPERTIESINERROR;
extern const GUID DBGUID_DBSQL;
#endif // DBINITCONSTANTS

enum DBPROPENUM
    {	DBPROP_ABORTPRESERVE	= 0x2L,
	DBPROP_ACTIVESESSIONS	= 0x3L,
	DBPROP_APPENDONLY	= 0xbbL,
	DBPROP_ASYNCTXNABORT	= 0xa8L,
	DBPROP_ASYNCTXNCOMMIT	= 0x4L,
	DBPROP_AUTH_CACHE_AUTHINFO	= 0x5L,
	DBPROP_AUTH_ENCRYPT_PASSWORD	= 0x6L,
	DBPROP_AUTH_INTEGRATED	= 0x7L,
	DBPROP_AUTH_MASK_PASSWORD	= 0x8L,
	DBPROP_AUTH_PASSWORD	= 0x9L,
	DBPROP_AUTH_PERSIST_ENCRYPTED	= 0xaL,
	DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO	= 0xbL,
	DBPROP_AUTH_USERID	= 0xcL,
	DBPROP_BLOCKINGSTORAGEOBJECTS	= 0xdL,
	DBPROP_BOOKMARKS	= 0xeL,
	DBPROP_BOOKMARKSKIPPED	= 0xfL,
	DBPROP_BOOKMARKTYPE	= 0x10L,
	DBPROP_BYREFACCESSORS	= 0x78L,
	DBPROP_CACHEDEFERRED	= 0x11L,
	DBPROP_CANFETCHBACKWARDS	= 0x12L,
	DBPROP_CANHOLDROWS	= 0x13L,
	DBPROP_CANSCROLLBACKWARDS	= 0x15L,
	DBPROP_CATALOGLOCATION	= 0x16L,
	DBPROP_CATALOGTERM	= 0x17L,
	DBPROP_CATALOGUSAGE	= 0x18L,
	DBPROP_CHANGEINSERTEDROWS	= 0xbcL,
	DBPROP_CLSID	= 0x19L,
	DBPROP_COL_AUTOINCREMENT	= 0x1aL,
	DBPROP_COL_DEFAULT	= 0x1bL,
	DBPROP_COL_DESCRIPTION	= 0x1cL,
	DBPROP_COL_FIXEDLENGTH	= 0xa7L,
	DBPROP_COL_NULLABLE	= 0x1dL,
	DBPROP_COL_PRIMARYKEY	= 0x1eL,
	DBPROP_COL_UNIQUE	= 0x1fL,
	DBPROP_COLUMNDEFINITION	= 0x20L,
	DBPROP_COLUMNRESTRICT	= 0x21L,
	DBPROP_COMMANDTIMEOUT	= 0x22L,
	DBPROP_COMMITPRESERVE	= 0x23L,
	DBPROP_CONCATNULLBEHAVIOR	= 0x24L,
	DBPROP_CURRENTCATALOG	= 0x25L,
	DBPROP_DATASOURCENAME	= 0x26L,
	DBPROP_DATASOURCEREADONLY	= 0x27L,
	DBPROP_DBMSNAME	= 0x28L,
	DBPROP_DBMSVER	= 0x29L,
	DBPROP_DEFERRED	= 0x2aL,
	DBPROP_DELAYSTORAGEOBJECTS	= 0x2bL,
	DBPROP_DSOTHREADMODEL	= 0xa9L,
	DBPROP_GROUPBY	= 0x2cL,
	DBPROP_HETEROGENEOUSTABLES	= 0x2dL,
	DBPROP_IAccessor	= 0x79L,
	DBPROP_IColumnsInfo	= 0x7aL,
	DBPROP_IColumnsRowset	= 0x7bL,
	DBPROP_IConnectionPointContainer	= 0x7cL,
	DBPROP_IConvertType	= 0xc2L,
	DBPROP_IRowset	= 0x7eL,
	DBPROP_IRowsetChange	= 0x7fL,
	DBPROP_IRowsetIdentity	= 0x80L,
	DBPROP_IRowsetIndex	= 0x9fL,
	DBPROP_IRowsetInfo	= 0x81L,
	DBPROP_IRowsetLocate	= 0x82L,
	DBPROP_IRowsetResynch	= 0x84L,
	DBPROP_IRowsetScroll	= 0x85L,
	DBPROP_IRowsetUpdate	= 0x86L,
	DBPROP_ISupportErrorInfo	= 0x87L,
	DBPROP_ILockBytes	= 0x88L,
	DBPROP_ISequentialStream	= 0x89L,
	DBPROP_IStorage	= 0x8aL,
	DBPROP_IStream	= 0x8bL,
	DBPROP_IDENTIFIERCASE	= 0x2eL,
	DBPROP_IMMOBILEROWS	= 0x2fL,
	DBPROP_INDEX_AUTOUPDATE	= 0x30L,
	DBPROP_INDEX_CLUSTERED	= 0x31L,
	DBPROP_INDEX_FILLFACTOR	= 0x32L,
	DBPROP_INDEX_INITIALSIZE	= 0x33L,
	DBPROP_INDEX_NULLCOLLATION	= 0x34L,
	DBPROP_INDEX_NULLS	= 0x35L,
	DBPROP_INDEX_PRIMARYKEY	= 0x36L,
	DBPROP_INDEX_SORTBOOKMARKS	= 0x37L,
	DBPROP_INDEX_TEMPINDEX	= 0xa3L,
	DBPROP_INDEX_TYPE	= 0x38L,
	DBPROP_INDEX_UNIQUE	= 0x39L,
	DBPROP_INIT_DATASOURCE	= 0x3bL,
	DBPROP_INIT_HWND	= 0x3cL,
	DBPROP_INIT_IMPERSONATION_LEVEL	= 0x3dL,
	DBPROP_INIT_LCID	= 0xbaL,
	DBPROP_INIT_LOCATION	= 0x3eL,
	DBPROP_INIT_MODE	= 0x3fL,
	DBPROP_INIT_PROMPT	= 0x40L,
	DBPROP_INIT_PROTECTION_LEVEL	= 0x41L,
	DBPROP_INIT_PROVIDERSTRING	= 0xa0L,
	DBPROP_INIT_TIMEOUT	= 0x42L,
	DBPROP_LITERALBOOKMARKS	= 0x43L,
	DBPROP_LITERALIDENTITY	= 0x44L,
	DBPROP_MAXINDEXSIZE	= 0x46L,
	DBPROP_MAXOPENROWS	= 0x47L,
	DBPROP_MAXPENDINGROWS	= 0x48L,
	DBPROP_MAXROWS	= 0x49L,
	DBPROP_MAXROWSIZE	= 0x4aL,
	DBPROP_MAXROWSIZEINCLUDESBLOB	= 0x4bL,
	DBPROP_MAXTABLESINSELECT	= 0x4cL,
	DBPROP_MAYWRITECOLUMN	= 0x4dL,
	DBPROP_MEMORYUSAGE	= 0x4eL,
	DBPROP_MONIKER	= 0x4fL,
	DBPROP_MULTIPLEPARAMSETS	= 0xbfL,
	DBPROP_MULTIPLESTORAGEOBJECTS	= 0x50L,
	DBPROP_MULTITABLEUPDATE	= 0x51L,
	DBPROP_NOTIFICATIONPHASES	= 0x52L,
	DBPROP_NOTIFYCOLUMNRECALCULATED	= 0xaaL,
	DBPROP_NOTIFYCOLUMNSET	= 0xabL,
	DBPROP_NOTIFYROWACTIVATE	= 0xacL,
	DBPROP_NOTIFYROWDELETE	= 0xadL,
	DBPROP_NOTIFYROWFIRSTCHANGE	= 0xaeL,
	DBPROP_NOTIFYROWINSERT	= 0xafL,
	DBPROP_NOTIFYROWRELEASE	= 0xb0L,
	DBPROP_NOTIFYROWRESYNCH	= 0xb1L,
	DBPROP_NOTIFYROWSETRELEASE	= 0xb2L,
	DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE	= 0xb3L,
	DBPROP_NOTIFYROWUNDOCHANGE	= 0xb4L,
	DBPROP_NOTIFYROWUNDODELETE	= 0xb5L,
	DBPROP_NOTIFYROWUNDOINSERT	= 0xb6L,
	DBPROP_NOTIFYROWUPDATE	= 0xb7L,
	DBPROP_NULLCOLLATION	= 0x53L,
	DBPROP_OLEOBJECTS	= 0x54L,
	DBPROP_ORDERBYCOLUMNSINSELECT	= 0x55L,
	DBPROP_ORDEREDBOOKMARKS	= 0x56L,
	DBPROP_OTHERINSERT	= 0x57L,
	DBPROP_OTHERUPDATEDELETE	= 0x58L,
	DBPROP_OUTPUTPARAMETERAVAILABILITY	= 0xb8L,
	DBPROP_OWNINSERT	= 0x59L,
	DBPROP_OWNUPDATEDELETE	= 0x5aL,
	DBPROP_PERSISTENTIDTYPE	= 0xb9L,
	DBPROP_PREPAREABORTBEHAVIOR	= 0x5bL,
	DBPROP_PREPARECOMMITBEHAVIOR	= 0x5cL,
	DBPROP_PROCEDURETERM	= 0x5dL,
	DBPROP_PROVIDERNAME	= 0x60L,
	DBPROP_PROVIDEROLEDBVER	= 0x61L,
	DBPROP_PROVIDERVER	= 0x62L,
	DBPROP_QUICKRESTART	= 0x63L,
	DBPROP_QUOTEDIDENTIFIERCASE	= 0x64L,
	DBPROP_REENTRANTEVENTS	= 0x65L,
	DBPROP_REMOVEDELETED	= 0x66L,
	DBPROP_REPORTMULTIPLECHANGES	= 0x67L,
	DBPROP_RETURNPENDINGINSERTS	= 0xbdL,
	DBPROP_ROWRESTRICT	= 0x68L,
	DBPROP_ROWSETCONVERSIONSONCOMMAND	= 0xc0L,
	DBPROP_ROWTHREADMODEL	= 0x69L,
	DBPROP_SCHEMATERM	= 0x6aL,
	DBPROP_SCHEMAUSAGE	= 0x6bL,
	DBPROP_SERVERCURSOR	= 0x6cL,
	DBPROP_SESS_AUTOCOMMITISOLEVELS	= 0xbeL,
	DBPROP_SQLSUPPORT	= 0x6dL,
	DBPROP_STRING	= 0x6eL,
	DBPROP_STRONGIDENTITY	= 0x77L,
	DBPROP_STRUCTUREDSTORAGE	= 0x6fL,
	DBPROP_SUBQUERIES	= 0x70L,
	DBPROP_SUPPORTEDTXNDDL	= 0xa1L,
	DBPROP_SUPPORTEDTXNISOLEVELS	= 0x71L,
	DBPROP_SUPPORTEDTXNISORETAIN	= 0x72L,
	DBPROP_TABLETERM	= 0x73L,
	DBPROP_TBL_TEMPTABLE	= 0x8cL,
	DBPROP_TRANSACTEDOBJECT	= 0x74L,
	DBPROP_UPDATABILITY	= 0x75L,
	DBPROP_USERNAME	= 0x76L
    };
#define DBPROPVAL_BMK_NUMERIC							 0x00000001L
#define DBPROPVAL_BMK_KEY								 0x00000002L
#define DBPROPVAL_CL_START								 0x00000001L
#define DBPROPVAL_CL_END									 0x00000002L
#define DBPROPVAL_CU_DML_STATEMENTS						 0x00000001L
#define DBPROPVAL_CU_TABLE_DEFINITION					 0x00000002L
#define DBPROPVAL_CU_INDEX_DEFINITION					 0x00000004L
#define DBPROPVAL_CU_PRIVILEGE_DEFINITION				 0x00000008L
#define DBPROPVAL_CD_NOTNULL								 0x00000001L
#define DBPROPVAL_CB_NULL								 0x00000001L
#define DBPROPVAL_CB_NON_NULL							 0x00000002L
#define DBPROPVAL_FU_NOT_SUPPORTED						 0x00000001L
#define DBPROPVAL_FU_COLUMN								 0x00000002L
#define DBPROPVAL_FU_TABLE								 0x00000004L
#define DBPROPVAL_FU_CATALOG								 0x00000008L
#define DBPROPVAL_GB_NOT_SUPPORTED						 0x00000001L
#define DBPROPVAL_GB_EQUALS_SELECT						 0x00000002L
#define DBPROPVAL_GB_CONTAINS_SELECT						 0x00000004L
#define DBPROPVAL_GB_NO_RELATION							 0x00000008L
#define DBPROPVAL_HT_DIFFERENT_CATALOGS					 0x00000001L
#define DBPROPVAL_HT_DIFFERENT_PROVIDERS					 0x00000002L
#define DBPROPVAL_IC_UPPER								 0x00000001L
#define DBPROPVAL_IC_LOWER								 0x00000002L
#define DBPROPVAL_IC_SENSITIVE							 0x00000004L
#define DBPROPVAL_IC_MIXED								 0x00000008L
#define DBPROPVAL_LM_NONE								 0x00000001L
#define DBPROPVAL_LM_READ								 0x00000002L
#define DBPROPVAL_LM_INTENT								 0x00000004L
#define DBPROPVAL_LM_WRITE								 0x00000008L
#define DBPROPVAL_NP_OKTODO								 0x00000001L
#define DBPROPVAL_NP_ABOUTTODO							 0x00000002L
#define DBPROPVAL_NP_SYNCHAFTER							 0x00000004L
#define DBPROPVAL_NP_FAILEDTODO							 0x00000008L
#define DBPROPVAL_NP_DIDEVENT							 0x00000010L
#define DBPROPVAL_NC_END									 0x00000001L
#define DBPROPVAL_NC_HIGH								 0x00000002L
#define DBPROPVAL_NC_LOW									 0x00000004L
#define DBPROPVAL_NC_START								 0x00000008L
#define DBPROPVAL_OO_BLOB								 0x00000001L
#define DBPROPVAL_OO_IPERSIST							 0x00000002L
#define DBPROPVAL_CB_DELETE								 0x00000001L
#define DBPROPVAL_CB_PRESERVE							 0x00000002L
#define DBPROPVAL_SU_DML_STATEMENTS						 0x00000001L
#define DBPROPVAL_SU_TABLE_DEFINITION					 0x00000002L
#define DBPROPVAL_SU_INDEX_DEFINITION					 0x00000004L
#define DBPROPVAL_SU_PRIVILEGE_DEFINITION				 0x00000008L
#define DBPROPVAL_SO_CORRELATEDSUBQUERIES				 0x00000001L
#define DBPROPVAL_SO_COMPARISON							 0x00000002L
#define DBPROPVAL_SO_EXISTS								 0x00000004L
#define DBPROPVAL_SO_IN									 0x00000008L
#define DBPROPVAL_SO_QUANTIFIED							 0x00000010L
#define DBPROPVAL_SS_ISEQUENTIALSTREAM					 0x00000001L
#define DBPROPVAL_SS_ISTREAM								 0x00000002L
#define DBPROPVAL_SS_ISTORAGE							 0x00000004L
#define DBPROPVAL_SS_ILOCKBYTES							 0x00000008L
#define DBPROPVAL_TI_CHAOS								 0x00000010L
#define DBPROPVAL_TI_READUNCOMMITTED						 0x00000100L
#define DBPROPVAL_TI_BROWSE								 0x00000100L
#define DBPROPVAL_TI_CURSORSTABILITY						 0x00001000L
#define DBPROPVAL_TI_READCOMMITTED						 0x00001000L
#define DBPROPVAL_TI_REPEATABLEREAD						 0x00010000L
#define DBPROPVAL_TI_SERIALIZABLE						 0x00100000L
#define DBPROPVAL_TI_ISOLATED							 0x00100000L
#define DBPROPVAL_TR_COMMIT_DC							 0x00000001L
#define DBPROPVAL_TR_COMMIT								 0x00000002L
#define DBPROPVAL_TR_COMMIT_NO							 0x00000004L
#define DBPROPVAL_TR_ABORT_DC							 0x00000008L
#define DBPROPVAL_TR_ABORT								 0x00000010L
#define DBPROPVAL_TR_ABORT_NO							 0x00000020L
#define DBPROPVAL_TR_DONTCARE							 0x00000040L
#define DBPROPVAL_TR_BOTH								 0x00000080L
#define DBPROPVAL_TR_NONE								 0x00000100L
#define DBPROPVAL_TR_OPTIMISTIC							 0x00000200L
#define DBPROPVAL_RT_FREETHREAD							 0x00000001L
#define DBPROPVAL_RT_APTMTTHREAD							 0x00000002L
#define DBPROPVAL_RT_SINGLETHREAD						 0x00000004L
#define DBPROPVAL_UP_CHANGE								 0x00000001L
#define DBPROPVAL_UP_DELETE								 0x00000002L
#define DBPROPVAL_UP_INSERT								 0x00000004L
#define DBPROPVAL_SQL_NONE								 0x00000000L
#define DBPROPVAL_SQL_ODBC_MINIMUM						 0x00000001L
#define DBPROPVAL_SQL_ODBC_CORE							 0x00000002L
#define DBPROPVAL_SQL_ODBC_EXTENDED						 0x00000004L
#define DBPROPVAL_SQL_ANSI89_IEF							 0x00000008L
#define DBPROPVAL_SQL_ANSI92_ENTRY						 0x00000010L
#define DBPROPVAL_SQL_FIPS_TRANSITIONAL					 0x00000020L
#define DBPROPVAL_SQL_ANSI92_INTERMEDIATE				 0x00000040L
#define DBPROPVAL_SQL_ANSI92_FULL						 0x00000080L
#define DBPROPVAL_SQL_ESCAPECLAUSES						 0x00000100L
#define DBPROPVAL_IT_BTREE                                0x00000001L
#define DBPROPVAL_IT_HASH                                 0x00000002L
#define DBPROPVAL_IT_CONTENT                              0x00000003L
#define DBPROPVAL_IT_OTHER                                0x00000004L
#define DBPROPVAL_IN_DISALLOWNULL                         0x00000001L
#define DBPROPVAL_IN_IGNORENULL                           0x00000002L
#define DBPROPVAL_IN_IGNOREANYNULL                        0x00000004L
#define DBPROPVAL_TC_NONE                                 0x00000000L
#define DBPROPVAL_TC_DML                                  0x00000001L
#define DBPROPVAL_TC_DDL_COMMIT                           0x00000002L
#define DBPROPVAL_TC_DDL_IGNORE                           0x00000004L
#define DBPROPVAL_TC_ALL                                  0x00000008L
#define DBPROPVAL_NP_OKTODO                               0x00000001L
#define DBPROPVAL_NP_ABOUTTODO                            0x00000002L
#define DBPROPVAL_NP_SYNCHAFTER                           0x00000004L
#define DBPROPVAL_OA_NOTSUPPORTED                         0x00000001L
#define DBPROPVAL_OA_ATEXECUTE                            0x00000002L
#define DBPROPVAL_OA_ATROWRELEASE                         0x00000004L
#define DBPROPVAL_PT_GUID_NAME                            0x00000001L
#define DBPROPVAL_PT_GUID_PROPID                          0x00000002L
#define DBPROPVAL_PT_NAME                                 0x00000004L
#define DBPROPVAL_PT_GUID                                 0x00000008L
#define DB_IMP_LEVEL_ANONYMOUS		0x00
#define DB_IMP_LEVEL_IDENTIFY		0x01
#define DB_IMP_LEVEL_IMPERSONATE		0x02
#define DB_IMP_LEVEL_DELEGATE		0x03
#define DBPROMPT_PROMPT				0x01
#define DBPROMPT_COMPLETE			0x02
#define DBPROMPT_COMPLETEREQUIRED	0x03
#define DBPROMPT_NOPROMPT			0x04
#define DB_PROT_LEVEL_NONE			0x00
#define DB_PROT_LEVEL_CONNECT		0x01
#define DB_PROT_LEVEL_CALL			0x02
#define DB_PROT_LEVEL_PKT			0x03
#define DB_PROT_LEVEL_PKT_INTEGRITY	0x04
#define DB_PROT_LEVEL_PKT_PRIVACY	0x05
#define DB_MODE_READ					0x01
#define DB_MODE_WRITE				0x02
#define DB_MODE_READWRITE			0x03
#define DB_MODE_SHARE_DENY_READ		0x04
#define DB_MODE_SHARE_DENY_WRITE		0x08
#define DB_MODE_SHARE_EXCLUSIVE		0x0c
#define DB_MODE_SHARE_DENY_NONE		0x10
typedef struct  tagDBPARAMS
    {
    void __RPC_FAR *pData;
    ULONG cParamSets;
    HACCESSOR hAccessor;
    }	DBPARAMS;

typedef DWORD DBPARAMFLAGS;


enum DBPARAMFLAGSENUM
    {	DBPARAMFLAGS_ISINPUT	= 0x1,
	DBPARAMFLAGS_ISOUTPUT	= 0x2,
	DBPARAMFLAGS_ISSIGNED	= 0x10,
	DBPARAMFLAGS_ISNULLABLE	= 0x40,
	DBPARAMFLAGS_ISLONG	= 0x80
    };
typedef struct  tagDBPARAMINFO
    {
    DBPARAMFLAGS dwFlags;
    ULONG iOrdinal;
    LPOLESTR pwszName;
    ITypeInfo __RPC_FAR *pTypeInfo;
    ULONG ulParamSize;
    DBTYPE wType;
    BYTE bPrecision;
    BYTE bScale;
    }	DBPARAMINFO;

typedef DWORD DBPROPID;

typedef struct  tagDBPROPIDSET
    {
    /* [size_is] */ DBPROPID __RPC_FAR *rgPropertyIDs;
    ULONG cPropertyIDs;
    GUID guidPropertySet;
    }	DBPROPIDSET;

typedef DWORD DBPROPFLAGS;


enum DBPROPFLAGSENUM
    {	DBPROPFLAGS_NOTSUPPORTED	= 0,
	DBPROPFLAGS_COLUMN	= 0x1,
	DBPROPFLAGS_DATASOURCE	= 0x2,
	DBPROPFLAGS_DATASOURCECREATE	= 0x4,
	DBPROPFLAGS_DATASOURCEINFO	= 0x8,
	DBPROPFLAGS_DBINIT	= 0x10,
	DBPROPFLAGS_INDEX	= 0x20,
	DBPROPFLAGS_ROWSET	= 0x40,
	DBPROPFLAGS_TABLE	= 0x80,
	DBPROPFLAGS_COLUMNOK	= 0x100,
	DBPROPFLAGS_READ	= 0x200,
	DBPROPFLAGS_WRITE	= 0x400,
	DBPROPFLAGS_REQUIRED	= 0x800,
	DBPROPFLAGS_SESSION	= 0x1000
    };
typedef struct  tagDBPROPINFO
    {
    LPOLESTR pwszDescription;
    DBPROPID dwPropertyID;
    DBPROPFLAGS dwFlags;
    VARTYPE vtType;
    VARIANT vValues;
    }	DBPROPINFO;

typedef struct  tagDBPROPINFOSET
    {
    /* [size_is] */ DBPROPINFO __RPC_FAR *rgPropertyInfos;
    ULONG cPropertyInfos;
    GUID guidPropertySet;
    }	DBPROPINFOSET;

typedef DWORD DBPROPOPTIONS;


enum DBPROPOPTIONSENUM
    {	DBPROPOPTIONS_REQUIRED	= 0,
	DBPROPOPTIONS_SETIFCHEAP	= 0x1
    };
typedef DWORD DBPROPSTATUS;


enum DBPROPSTATUSENUM
    {	DBPROPSTATUS_OK	= 0,
	DBPROPSTATUS_NOTSUPPORTED	= 1,
	DBPROPSTATUS_BADVALUE	= 2,
	DBPROPSTATUS_BADOPTION	= 3,
	DBPROPSTATUS_BADCOLUMN	= 4,
	DBPROPSTATUS_NOTALLSETTABLE	= 5,
	DBPROPSTATUS_NOTSETTABLE	= 6,
	DBPROPSTATUS_NOTSET	= 7,
	DBPROPSTATUS_CONFLICTING	= 8
    };
typedef struct  tagDBPROP
    {
    DBPROPID dwPropertyID;
    DBPROPOPTIONS dwOptions;
    DBPROPSTATUS dwStatus;
    DBID colid;
    VARIANT vValue;
    }	DBPROP;

typedef struct  tagDBPROPSET
    {
    /* [size_is] */ DBPROP __RPC_FAR *rgProperties;
    ULONG cProperties;
    GUID guidPropertySet;
    }	DBPROPSET;

#define DBPARAMTYPE_INPUT			0x01
#define DBPARAMTYPE_INPUTOUTPUT		0x02
#define DBPARAMTYPE_OUTPUT			0x03
#define DBPARAMTYPE_RETURNVALUE		0x04
#define DB_PT_UNKNOWN				0x01
#define DB_PT_PROCEDURE				0x02
#define DB_PT_FUNCTION				0x03
#define DB_REMOTE					0x01
#define DB_LOCAL_SHARED				0x02
#define DB_LOCAL_EXCLUSIVE			0x03
#define DB_COLLATION_ASC				0x01
#define DB_COLLATION_DESC			0x02
#define DB_UNSEARCHABLE				0x01
#define DB_LIKE_ONLY					0x02
#define DB_ALL_EXCEPT_LIKE			0x03
#define DB_SEARCHABLE				0x04
typedef DWORD DBINDEX_COL_ORDER;


enum DBINDEX_COL_ORDERENUM
    {	DBINDEX_COL_ORDER_ASC	= 0,
	DBINDEX_COL_ORDER_DESC	= DBINDEX_COL_ORDER_ASC + 1
    };
typedef struct  tagDBINDEXCOLUMNDESC
    {
    DBID __RPC_FAR *pColumnID;
    DBINDEX_COL_ORDER eIndexColOrder;
    }	DBINDEXCOLUMNDESC;

typedef struct  tagDBCOLUMNDESC
    {
    LPOLESTR pwszTypeName;
    ITypeInfo __RPC_FAR *pTypeInfo;
    /* [size_is] */ DBPROPSET __RPC_FAR *rgPropertySets;
    CLSID __RPC_FAR *pclsid;
    ULONG cPropertySets;
    ULONG ulColumnSize;
    DBID dbcid;
    DBTYPE wType;
    BYTE bPrecision;
    BYTE bScale;
    }	DBCOLUMNDESC;



extern RPC_IF_HANDLE DBStructureDefinitions_v0_0_c_ifspec;
extern RPC_IF_HANDLE DBStructureDefinitions_v0_0_s_ifspec;
#endif /* __DBStructureDefinitions_INTERFACE_DEFINED__ */

#ifndef __IAccessor_INTERFACE_DEFINED__
#define __IAccessor_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAccessor
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef DWORD DBACCESSORFLAGS;


enum DBACCESSORFLAGSENUM
    {	DBACCESSOR_INVALID	= 0,
	DBACCESSOR_PASSBYREF	= 0x1,
	DBACCESSOR_ROWDATA	= 0x2,
	DBACCESSOR_PARAMETERDATA	= 0x4,
	DBACCESSOR_OPTIMIZED	= 0x8
    };
typedef DWORD DBBINDSTATUS;


enum DBBINDSTATUSENUM
    {	DBBINDSTATUS_OK	= 0,
	DBBINDSTATUS_BADORDINAL	= 1,
	DBBINDSTATUS_UNSUPPORTEDCONVERSION	= 2,
	DBBINDSTATUS_BADBINDINFO	= 3,
	DBBINDSTATUS_BADSTORAGEFLAGS	= 4,
	DBBINDSTATUS_NOINTERFACE	= 5
    };

EXTERN_C const IID IID_IAccessor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAccessor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddRefAccessor( 
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ ULONG __RPC_FAR *pcRefCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAccessor( 
            /* [in] */ DBACCESSORFLAGS dwAccessorFlags,
            /* [in] */ ULONG cBindings,
            /* [size_is][in] */ const DBBINDING __RPC_FAR rgBindings[  ],
            /* [in] */ ULONG cbRowSize,
            /* [out] */ HACCESSOR __RPC_FAR *phAccessor,
            /* [size_is][out] */ DBBINDSTATUS __RPC_FAR rgStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBindings( 
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ DBACCESSORFLAGS __RPC_FAR *pdwAccessorFlags,
            /* [out][in] */ ULONG __RPC_FAR *pcBindings,
            /* [size_is][size_is][out] */ DBBINDING __RPC_FAR *__RPC_FAR *prgBindings) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseAccessor( 
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ ULONG __RPC_FAR *pcRefCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAccessor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAccessor __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAccessor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefAccessor )( 
            IAccessor __RPC_FAR * This,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ ULONG __RPC_FAR *pcRefCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAccessor )( 
            IAccessor __RPC_FAR * This,
            /* [in] */ DBACCESSORFLAGS dwAccessorFlags,
            /* [in] */ ULONG cBindings,
            /* [size_is][in] */ const DBBINDING __RPC_FAR rgBindings[  ],
            /* [in] */ ULONG cbRowSize,
            /* [out] */ HACCESSOR __RPC_FAR *phAccessor,
            /* [size_is][out] */ DBBINDSTATUS __RPC_FAR rgStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindings )( 
            IAccessor __RPC_FAR * This,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ DBACCESSORFLAGS __RPC_FAR *pdwAccessorFlags,
            /* [out][in] */ ULONG __RPC_FAR *pcBindings,
            /* [size_is][size_is][out] */ DBBINDING __RPC_FAR *__RPC_FAR *prgBindings);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseAccessor )( 
            IAccessor __RPC_FAR * This,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ ULONG __RPC_FAR *pcRefCount);
        
        END_INTERFACE
    } IAccessorVtbl;

    interface IAccessor
    {
        CONST_VTBL struct IAccessorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccessor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccessor_AddRefAccessor(This,hAccessor,pcRefCount)	\
    (This)->lpVtbl -> AddRefAccessor(This,hAccessor,pcRefCount)

#define IAccessor_CreateAccessor(This,dwAccessorFlags,cBindings,rgBindings,cbRowSize,phAccessor,rgStatus)	\
    (This)->lpVtbl -> CreateAccessor(This,dwAccessorFlags,cBindings,rgBindings,cbRowSize,phAccessor,rgStatus)

#define IAccessor_GetBindings(This,hAccessor,pdwAccessorFlags,pcBindings,prgBindings)	\
    (This)->lpVtbl -> GetBindings(This,hAccessor,pdwAccessorFlags,pcBindings,prgBindings)

#define IAccessor_ReleaseAccessor(This,hAccessor,pcRefCount)	\
    (This)->lpVtbl -> ReleaseAccessor(This,hAccessor,pcRefCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAccessor_AddRefAccessor_Proxy( 
    IAccessor __RPC_FAR * This,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ ULONG __RPC_FAR *pcRefCount);


void __RPC_STUB IAccessor_AddRefAccessor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessor_CreateAccessor_Proxy( 
    IAccessor __RPC_FAR * This,
    /* [in] */ DBACCESSORFLAGS dwAccessorFlags,
    /* [in] */ ULONG cBindings,
    /* [size_is][in] */ const DBBINDING __RPC_FAR rgBindings[  ],
    /* [in] */ ULONG cbRowSize,
    /* [out] */ HACCESSOR __RPC_FAR *phAccessor,
    /* [size_is][out] */ DBBINDSTATUS __RPC_FAR rgStatus[  ]);


void __RPC_STUB IAccessor_CreateAccessor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessor_GetBindings_Proxy( 
    IAccessor __RPC_FAR * This,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ DBACCESSORFLAGS __RPC_FAR *pdwAccessorFlags,
    /* [out][in] */ ULONG __RPC_FAR *pcBindings,
    /* [size_is][size_is][out] */ DBBINDING __RPC_FAR *__RPC_FAR *prgBindings);


void __RPC_STUB IAccessor_GetBindings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessor_ReleaseAccessor_Proxy( 
    IAccessor __RPC_FAR * This,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ ULONG __RPC_FAR *pcRefCount);


void __RPC_STUB IAccessor_ReleaseAccessor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccessor_INTERFACE_DEFINED__ */


#ifndef __IRowset_INTERFACE_DEFINED__
#define __IRowset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowset
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef DWORD DBROWOPTIONS;


EXTERN_C const IID IID_IRowset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddRefRows( 
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextRows( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseRows( 
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][in] */ DBROWOPTIONS __RPC_FAR rgRowOptions[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestartPosition( 
            /* [in] */ HCHAPTER hReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefRows )( 
            IRowset __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IRowset __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRows )( 
            IRowset __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseRows )( 
            IRowset __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][in] */ DBROWOPTIONS __RPC_FAR rgRowOptions[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestartPosition )( 
            IRowset __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved);
        
        END_INTERFACE
    } IRowsetVtbl;

    interface IRowset
    {
        CONST_VTBL struct IRowsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowset_AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)

#define IRowset_GetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetData(This,hRow,hAccessor,pData)

#define IRowset_GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowset_ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)

#define IRowset_RestartPosition(This,hReserved)	\
    (This)->lpVtbl -> RestartPosition(This,hReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowset_AddRefRows_Proxy( 
    IRowset __RPC_FAR * This,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
    /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);


void __RPC_STUB IRowset_AddRefRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowset_GetData_Proxy( 
    IRowset __RPC_FAR * This,
    /* [in] */ HROW hRow,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ void __RPC_FAR *pData);


void __RPC_STUB IRowset_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowset_GetNextRows_Proxy( 
    IRowset __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ LONG lRowsOffset,
    /* [in] */ LONG cRows,
    /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);


void __RPC_STUB IRowset_GetNextRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowset_ReleaseRows_Proxy( 
    IRowset __RPC_FAR * This,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [size_is][in] */ DBROWOPTIONS __RPC_FAR rgRowOptions[  ],
    /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
    /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);


void __RPC_STUB IRowset_ReleaseRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowset_RestartPosition_Proxy( 
    IRowset __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved);


void __RPC_STUB IRowset_RestartPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowset_INTERFACE_DEFINED__ */


#ifndef __IRowsetInfo_INTERFACE_DEFINED__
#define __IRowsetInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetInfo
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IRowsetInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperties( 
            /* [in] */ const ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferencedRowset( 
            /* [in] */ ULONG iOrdinal,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppReferencedRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpecification( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSpecification) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperties )( 
            IRowsetInfo __RPC_FAR * This,
            /* [in] */ const ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReferencedRowset )( 
            IRowsetInfo __RPC_FAR * This,
            /* [in] */ ULONG iOrdinal,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppReferencedRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSpecification )( 
            IRowsetInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSpecification);
        
        END_INTERFACE
    } IRowsetInfoVtbl;

    interface IRowsetInfo
    {
        CONST_VTBL struct IRowsetInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetInfo_GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)	\
    (This)->lpVtbl -> GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)

#define IRowsetInfo_GetReferencedRowset(This,iOrdinal,riid,ppReferencedRowset)	\
    (This)->lpVtbl -> GetReferencedRowset(This,iOrdinal,riid,ppReferencedRowset)

#define IRowsetInfo_GetSpecification(This,riid,ppSpecification)	\
    (This)->lpVtbl -> GetSpecification(This,riid,ppSpecification)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetInfo_GetProperties_Proxy( 
    IRowsetInfo __RPC_FAR * This,
    /* [in] */ const ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
    /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);


void __RPC_STUB IRowsetInfo_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetInfo_GetReferencedRowset_Proxy( 
    IRowsetInfo __RPC_FAR * This,
    /* [in] */ ULONG iOrdinal,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppReferencedRowset);


void __RPC_STUB IRowsetInfo_GetReferencedRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetInfo_GetSpecification_Proxy( 
    IRowsetInfo __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSpecification);


void __RPC_STUB IRowsetInfo_GetSpecification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetInfo_INTERFACE_DEFINED__ */


#ifndef __IRowsetLocate_INTERFACE_DEFINED__
#define __IRowsetLocate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetLocate
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef DWORD DBCOMPARE;


enum DBCOMPAREENUM
    {	DBCOMPARE_LT	= 0,
	DBCOMPARE_EQ	= DBCOMPARE_LT + 1,
	DBCOMPARE_GT	= DBCOMPARE_EQ + 1,
	DBCOMPARE_NE	= DBCOMPARE_GT + 1,
	DBCOMPARE_NOTCOMPARABLE	= DBCOMPARE_NE + 1
    };

EXTERN_C const IID IID_IRowsetLocate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetLocate : public IRowset
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Compare( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark1,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark1,
            /* [in] */ ULONG cbBookmark2,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark2,
            /* [out] */ DBCOMPARE __RPC_FAR *pComparison) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowsAt( 
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowsByBookmark( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Hash( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cBookmarks,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ DWORD __RPC_FAR rgHashedValues[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgBookmarkStatus[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetLocateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetLocate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetLocate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefRows )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRows )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseRows )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][in] */ DBROWOPTIONS __RPC_FAR rgRowOptions[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestartPosition )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compare )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark1,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark1,
            /* [in] */ ULONG cbBookmark2,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark2,
            /* [out] */ DBCOMPARE __RPC_FAR *pComparison);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsAt )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsByBookmark )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hash )( 
            IRowsetLocate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cBookmarks,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ DWORD __RPC_FAR rgHashedValues[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgBookmarkStatus[  ]);
        
        END_INTERFACE
    } IRowsetLocateVtbl;

    interface IRowsetLocate
    {
        CONST_VTBL struct IRowsetLocateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetLocate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetLocate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetLocate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetLocate_AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)

#define IRowsetLocate_GetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetData(This,hRow,hAccessor,pData)

#define IRowsetLocate_GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetLocate_ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)

#define IRowsetLocate_RestartPosition(This,hReserved)	\
    (This)->lpVtbl -> RestartPosition(This,hReserved)


#define IRowsetLocate_Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)	\
    (This)->lpVtbl -> Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)

#define IRowsetLocate_GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetLocate_GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,rghRows,rgRowStatus)	\
    (This)->lpVtbl -> GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,rghRows,rgRowStatus)

#define IRowsetLocate_Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)	\
    (This)->lpVtbl -> Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetLocate_Compare_Proxy( 
    IRowsetLocate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cbBookmark1,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark1,
    /* [in] */ ULONG cbBookmark2,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark2,
    /* [out] */ DBCOMPARE __RPC_FAR *pComparison);


void __RPC_STUB IRowsetLocate_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetLocate_GetRowsAt_Proxy( 
    IRowsetLocate __RPC_FAR * This,
    /* [in] */ HWATCHREGION hReserved1,
    /* [in] */ HCHAPTER hReserved2,
    /* [in] */ ULONG cbBookmark,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
    /* [in] */ LONG lRowsOffset,
    /* [in] */ LONG cRows,
    /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);


void __RPC_STUB IRowsetLocate_GetRowsAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetLocate_GetRowsByBookmark_Proxy( 
    IRowsetLocate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
    /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
    /* [size_is][out] */ HROW __RPC_FAR rghRows[  ],
    /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);


void __RPC_STUB IRowsetLocate_GetRowsByBookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetLocate_Hash_Proxy( 
    IRowsetLocate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cBookmarks,
    /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
    /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
    /* [size_is][out] */ DWORD __RPC_FAR rgHashedValues[  ],
    /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgBookmarkStatus[  ]);


void __RPC_STUB IRowsetLocate_Hash_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetLocate_INTERFACE_DEFINED__ */


#ifndef __IRowsetResynch_INTERFACE_DEFINED__
#define __IRowsetResynch_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetResynch
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetResynch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetResynch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVisibleData( 
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResynchRows( 
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out] */ ULONG __RPC_FAR *pcRowsResynched,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRowsResynched,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetResynchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetResynch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetResynch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetResynch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVisibleData )( 
            IRowsetResynch __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResynchRows )( 
            IRowsetResynch __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out] */ ULONG __RPC_FAR *pcRowsResynched,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRowsResynched,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);
        
        END_INTERFACE
    } IRowsetResynchVtbl;

    interface IRowsetResynch
    {
        CONST_VTBL struct IRowsetResynchVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetResynch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetResynch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetResynch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetResynch_GetVisibleData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetVisibleData(This,hRow,hAccessor,pData)

#define IRowsetResynch_ResynchRows(This,cRows,rghRows,pcRowsResynched,prghRowsResynched,prgRowStatus)	\
    (This)->lpVtbl -> ResynchRows(This,cRows,rghRows,pcRowsResynched,prghRowsResynched,prgRowStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetResynch_GetVisibleData_Proxy( 
    IRowsetResynch __RPC_FAR * This,
    /* [in] */ HROW hRow,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ void __RPC_FAR *pData);


void __RPC_STUB IRowsetResynch_GetVisibleData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetResynch_ResynchRows_Proxy( 
    IRowsetResynch __RPC_FAR * This,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [out] */ ULONG __RPC_FAR *pcRowsResynched,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRowsResynched,
    /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);


void __RPC_STUB IRowsetResynch_ResynchRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetResynch_INTERFACE_DEFINED__ */


#ifndef __IRowsetScroll_INTERFACE_DEFINED__
#define __IRowsetScroll_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetScroll
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetScroll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetScroll : public IRowsetLocate
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetApproximatePosition( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ ULONG __RPC_FAR *pulPosition,
            /* [out] */ ULONG __RPC_FAR *pcRows) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowsAtRatio( 
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG ulNumerator,
            /* [in] */ ULONG ulDenominator,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetScrollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetScroll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetScroll __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefRows )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRows )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseRows )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][in] */ DBROWOPTIONS __RPC_FAR rgRowOptions[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestartPosition )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compare )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark1,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark1,
            /* [in] */ ULONG cbBookmark2,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark2,
            /* [out] */ DBCOMPARE __RPC_FAR *pComparison);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsAt )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsByBookmark )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hash )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cBookmarks,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out] */ DWORD __RPC_FAR rgHashedValues[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgBookmarkStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetApproximatePosition )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ ULONG __RPC_FAR *pulPosition,
            /* [out] */ ULONG __RPC_FAR *pcRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsAtRatio )( 
            IRowsetScroll __RPC_FAR * This,
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG ulNumerator,
            /* [in] */ ULONG ulDenominator,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        END_INTERFACE
    } IRowsetScrollVtbl;

    interface IRowsetScroll
    {
        CONST_VTBL struct IRowsetScrollVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetScroll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetScroll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetScroll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetScroll_AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)

#define IRowsetScroll_GetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetData(This,hRow,hAccessor,pData)

#define IRowsetScroll_GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetScroll_ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> ReleaseRows(This,cRows,rghRows,rgRowOptions,rgRefCounts,rgRowStatus)

#define IRowsetScroll_RestartPosition(This,hReserved)	\
    (This)->lpVtbl -> RestartPosition(This,hReserved)


#define IRowsetScroll_Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)	\
    (This)->lpVtbl -> Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)

#define IRowsetScroll_GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetScroll_GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,rghRows,rgRowStatus)	\
    (This)->lpVtbl -> GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,rghRows,rgRowStatus)

#define IRowsetScroll_Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)	\
    (This)->lpVtbl -> Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)


#define IRowsetScroll_GetApproximatePosition(This,hReserved,cbBookmark,pBookmark,pulPosition,pcRows)	\
    (This)->lpVtbl -> GetApproximatePosition(This,hReserved,cbBookmark,pBookmark,pulPosition,pcRows)

#define IRowsetScroll_GetRowsAtRatio(This,hReserved1,hReserved2,ulNumerator,ulDenominator,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsAtRatio(This,hReserved1,hReserved2,ulNumerator,ulDenominator,cRows,pcRowsObtained,prghRows)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetScroll_GetApproximatePosition_Proxy( 
    IRowsetScroll __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cbBookmark,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
    /* [out] */ ULONG __RPC_FAR *pulPosition,
    /* [out] */ ULONG __RPC_FAR *pcRows);


void __RPC_STUB IRowsetScroll_GetApproximatePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetScroll_GetRowsAtRatio_Proxy( 
    IRowsetScroll __RPC_FAR * This,
    /* [in] */ HWATCHREGION hReserved1,
    /* [in] */ HCHAPTER hReserved2,
    /* [in] */ ULONG ulNumerator,
    /* [in] */ ULONG ulDenominator,
    /* [in] */ LONG cRows,
    /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);


void __RPC_STUB IRowsetScroll_GetRowsAtRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetScroll_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0076
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


//@@@+ V2.0
#if( OLEDBVER >= 0x0200 )


extern RPC_IF_HANDLE __MIDL__intf_0076_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0076_v0_0_s_ifspec;

#ifndef __IRowsetExactScroll_INTERFACE_DEFINED__
#define __IRowsetExactScroll_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetExactScroll
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetExactScroll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetExactScroll : public IRowsetScroll
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetExactPosition( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ ULONG __RPC_FAR *pulPosition,
            /* [out] */ ULONG __RPC_FAR *pcRows) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetExactScrollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetExactScroll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetExactScroll __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefRows )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRows )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseRows )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgRefCounts[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestartPosition )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compare )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark1,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark1,
            /* [in] */ ULONG cbBookmark2,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark2,
            /* [out] */ DBCOMPARE __RPC_FAR *pComparison);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsAt )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsByBookmark )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows,
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hash )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cBookmarks,
            /* [size_is][in] */ const ULONG __RPC_FAR rgcbBookmarks[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgpBookmarks[  ],
            /* [size_is][out][in] */ DWORD __RPC_FAR rgHashedValues[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgBookmarkStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetApproximatePosition )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ ULONG __RPC_FAR *pulPosition,
            /* [out] */ ULONG __RPC_FAR *pcRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsAtRatio )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HWATCHREGION hReserved1,
            /* [in] */ HCHAPTER hReserved2,
            /* [in] */ ULONG ulNumerator,
            /* [in] */ ULONG ulDenominator,
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExactPosition )( 
            IRowsetExactScroll __RPC_FAR * This,
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [out] */ ULONG __RPC_FAR *pulPosition,
            /* [out] */ ULONG __RPC_FAR *pcRows);
        
        END_INTERFACE
    } IRowsetExactScrollVtbl;

    interface IRowsetExactScroll
    {
        CONST_VTBL struct IRowsetExactScrollVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetExactScroll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetExactScroll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetExactScroll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetExactScroll_AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> AddRefRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)

#define IRowsetExactScroll_GetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetData(This,hRow,hAccessor,pData)

#define IRowsetExactScroll_GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetNextRows(This,hReserved,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetExactScroll_ReleaseRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)	\
    (This)->lpVtbl -> ReleaseRows(This,cRows,rghRows,rgRefCounts,rgRowStatus)

#define IRowsetExactScroll_RestartPosition(This,hReserved)	\
    (This)->lpVtbl -> RestartPosition(This,hReserved)


#define IRowsetExactScroll_Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)	\
    (This)->lpVtbl -> Compare(This,hReserved,cbBookmark1,pBookmark1,cbBookmark2,pBookmark2,pComparison)

#define IRowsetExactScroll_GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsAt(This,hReserved1,hReserved2,cbBookmark,pBookmark,lRowsOffset,cRows,pcRowsObtained,prghRows)

#define IRowsetExactScroll_GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,pcRowsObtained,prghRows,rgRowStatus)	\
    (This)->lpVtbl -> GetRowsByBookmark(This,hReserved,cRows,rgcbBookmarks,rgpBookmarks,pcRowsObtained,prghRows,rgRowStatus)

#define IRowsetExactScroll_Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)	\
    (This)->lpVtbl -> Hash(This,hReserved,cBookmarks,rgcbBookmarks,rgpBookmarks,rgHashedValues,rgBookmarkStatus)


#define IRowsetExactScroll_GetApproximatePosition(This,hReserved,cbBookmark,pBookmark,pulPosition,pcRows)	\
    (This)->lpVtbl -> GetApproximatePosition(This,hReserved,cbBookmark,pBookmark,pulPosition,pcRows)

#define IRowsetExactScroll_GetRowsAtRatio(This,hReserved1,hReserved2,ulNumerator,ulDenominator,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsAtRatio(This,hReserved1,hReserved2,ulNumerator,ulDenominator,cRows,pcRowsObtained,prghRows)


#define IRowsetExactScroll_GetExactPosition(This,hChapter,cbBookmark,pBookmark,pulPosition,pcRows)	\
    (This)->lpVtbl -> GetExactPosition(This,hChapter,cbBookmark,pBookmark,pulPosition,pcRows)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetExactScroll_GetExactPosition_Proxy( 
    IRowsetExactScroll __RPC_FAR * This,
    /* [in] */ HCHAPTER hChapter,
    /* [in] */ ULONG cbBookmark,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
    /* [out] */ ULONG __RPC_FAR *pulPosition,
    /* [out] */ ULONG __RPC_FAR *pcRows);


void __RPC_STUB IRowsetExactScroll_GetExactPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetExactScroll_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0077
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


#endif // OLEDBVER >= 0x0200
//@@@- V2.0


extern RPC_IF_HANDLE __MIDL__intf_0077_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0077_v0_0_s_ifspec;


/****************************************
 * Generated header for interface: __MIDL__intf_0076
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0077_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0077_v0_0_s_ifspec;

#ifndef __IRowsetChange_INTERFACE_DEFINED__
#define __IRowsetChange_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetChange
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetChange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetChange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteRows( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetData( 
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertRow( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetChangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetChange __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetChange __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetChange __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteRows )( 
            IRowsetChange __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )( 
            IRowsetChange __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertRow )( 
            IRowsetChange __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow);
        
        END_INTERFACE
    } IRowsetChangeVtbl;

    interface IRowsetChange
    {
        CONST_VTBL struct IRowsetChangeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetChange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetChange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetChange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetChange_DeleteRows(This,hReserved,cRows,rghRows,rgRowStatus)	\
    (This)->lpVtbl -> DeleteRows(This,hReserved,cRows,rghRows,rgRowStatus)

#define IRowsetChange_SetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> SetData(This,hRow,hAccessor,pData)

#define IRowsetChange_InsertRow(This,hReserved,hAccessor,pData,phRow)	\
    (This)->lpVtbl -> InsertRow(This,hReserved,hAccessor,pData,phRow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetChange_DeleteRows_Proxy( 
    IRowsetChange __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);


void __RPC_STUB IRowsetChange_DeleteRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetChange_SetData_Proxy( 
    IRowsetChange __RPC_FAR * This,
    /* [in] */ HROW hRow,
    /* [in] */ HACCESSOR hAccessor,
    /* [in] */ void __RPC_FAR *pData);


void __RPC_STUB IRowsetChange_SetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetChange_InsertRow_Proxy( 
    IRowsetChange __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ HACCESSOR hAccessor,
    /* [in] */ void __RPC_FAR *pData,
    /* [out] */ HROW __RPC_FAR *phRow);


void __RPC_STUB IRowsetChange_InsertRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetChange_INTERFACE_DEFINED__ */


#ifndef __IRowsetUpdate_INTERFACE_DEFINED__
#define __IRowsetUpdate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetUpdate
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef DWORD DBPENDINGSTATUS;


enum DBPENDINGSTATUSENUM
    {	DBPENDINGSTATUS_NEW	= 0x1,
	DBPENDINGSTATUS_CHANGED	= 0x2,
	DBPENDINGSTATUS_DELETED	= 0x4,
	DBPENDINGSTATUS_UNCHANGED	= 0x8,
	DBPENDINGSTATUS_INVALIDROW	= 0x10
    };

EXTERN_C const IID IID_IRowsetUpdate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetUpdate : public IRowsetChange
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOriginalData( 
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPendingRows( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ DBPENDINGSTATUS dwRowStatus,
            /* [out][in] */ ULONG __RPC_FAR *pcPendingRows,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgPendingRows,
            /* [size_is][size_is][out] */ DBPENDINGSTATUS __RPC_FAR *__RPC_FAR *prgPendingStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowStatus( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgPendingStatus[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Undo( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcRowsUndone,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRowsUndone,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcRows,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRows,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetUpdateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetUpdate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetUpdate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteRows )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBROWSTATUS __RPC_FAR rgRowStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertRow )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ void __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOriginalData )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HROW hRow,
            /* [in] */ HACCESSOR hAccessor,
            /* [out] */ void __RPC_FAR *pData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPendingRows )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ DBPENDINGSTATUS dwRowStatus,
            /* [out][in] */ ULONG __RPC_FAR *pcPendingRows,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgPendingRows,
            /* [size_is][size_is][out] */ DBPENDINGSTATUS __RPC_FAR *__RPC_FAR *prgPendingStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowStatus )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgPendingStatus[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Undo )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcRowsUndone,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRowsUndone,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IRowsetUpdate __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcRows,
            /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRows,
            /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);
        
        END_INTERFACE
    } IRowsetUpdateVtbl;

    interface IRowsetUpdate
    {
        CONST_VTBL struct IRowsetUpdateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetUpdate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetUpdate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetUpdate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetUpdate_DeleteRows(This,hReserved,cRows,rghRows,rgRowStatus)	\
    (This)->lpVtbl -> DeleteRows(This,hReserved,cRows,rghRows,rgRowStatus)

#define IRowsetUpdate_SetData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> SetData(This,hRow,hAccessor,pData)

#define IRowsetUpdate_InsertRow(This,hReserved,hAccessor,pData,phRow)	\
    (This)->lpVtbl -> InsertRow(This,hReserved,hAccessor,pData,phRow)


#define IRowsetUpdate_GetOriginalData(This,hRow,hAccessor,pData)	\
    (This)->lpVtbl -> GetOriginalData(This,hRow,hAccessor,pData)

#define IRowsetUpdate_GetPendingRows(This,hReserved,dwRowStatus,pcPendingRows,prgPendingRows,prgPendingStatus)	\
    (This)->lpVtbl -> GetPendingRows(This,hReserved,dwRowStatus,pcPendingRows,prgPendingRows,prgPendingStatus)

#define IRowsetUpdate_GetRowStatus(This,hReserved,cRows,rghRows,rgPendingStatus)	\
    (This)->lpVtbl -> GetRowStatus(This,hReserved,cRows,rghRows,rgPendingStatus)

#define IRowsetUpdate_Undo(This,hReserved,cRows,rghRows,pcRowsUndone,prgRowsUndone,prgRowStatus)	\
    (This)->lpVtbl -> Undo(This,hReserved,cRows,rghRows,pcRowsUndone,prgRowsUndone,prgRowStatus)

#define IRowsetUpdate_Update(This,hReserved,cRows,rghRows,pcRows,prgRows,prgRowStatus)	\
    (This)->lpVtbl -> Update(This,hReserved,cRows,rghRows,pcRows,prgRows,prgRowStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetUpdate_GetOriginalData_Proxy( 
    IRowsetUpdate __RPC_FAR * This,
    /* [in] */ HROW hRow,
    /* [in] */ HACCESSOR hAccessor,
    /* [out] */ void __RPC_FAR *pData);


void __RPC_STUB IRowsetUpdate_GetOriginalData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetUpdate_GetPendingRows_Proxy( 
    IRowsetUpdate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ DBPENDINGSTATUS dwRowStatus,
    /* [out][in] */ ULONG __RPC_FAR *pcPendingRows,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgPendingRows,
    /* [size_is][size_is][out] */ DBPENDINGSTATUS __RPC_FAR *__RPC_FAR *prgPendingStatus);


void __RPC_STUB IRowsetUpdate_GetPendingRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetUpdate_GetRowStatus_Proxy( 
    IRowsetUpdate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgPendingStatus[  ]);


void __RPC_STUB IRowsetUpdate_GetRowStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetUpdate_Undo_Proxy( 
    IRowsetUpdate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcRowsUndone,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRowsUndone,
    /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);


void __RPC_STUB IRowsetUpdate_Undo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetUpdate_Update_Proxy( 
    IRowsetUpdate __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcRows,
    /* [size_is][size_is][out] */ HROW __RPC_FAR *__RPC_FAR *prgRows,
    /* [size_is][size_is][out] */ DBROWSTATUS __RPC_FAR *__RPC_FAR *prgRowStatus);


void __RPC_STUB IRowsetUpdate_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetUpdate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0079
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0080_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0080_v0_0_s_ifspec;

#ifndef __IRowsetIdentity_INTERFACE_DEFINED__
#define __IRowsetIdentity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetIdentity
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IRowsetIdentity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetIdentity : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsSameRow( 
            /* [in] */ HROW hThisRow,
            /* [in] */ HROW hThatRow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetIdentityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetIdentity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetIdentity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetIdentity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameRow )( 
            IRowsetIdentity __RPC_FAR * This,
            /* [in] */ HROW hThisRow,
            /* [in] */ HROW hThatRow);
        
        END_INTERFACE
    } IRowsetIdentityVtbl;

    interface IRowsetIdentity
    {
        CONST_VTBL struct IRowsetIdentityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetIdentity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetIdentity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetIdentity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetIdentity_IsSameRow(This,hThisRow,hThatRow)	\
    (This)->lpVtbl -> IsSameRow(This,hThisRow,hThatRow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetIdentity_IsSameRow_Proxy( 
    IRowsetIdentity __RPC_FAR * This,
    /* [in] */ HROW hThisRow,
    /* [in] */ HROW hThatRow);


void __RPC_STUB IRowsetIdentity_IsSameRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetIdentity_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0082
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


//@@@+ V2.0
#if( OLEDBVER >= 0x0200 )


extern RPC_IF_HANDLE __MIDL__intf_0082_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0082_v0_0_s_ifspec;

#ifndef __IRowsetNewRowAfter_INTERFACE_DEFINED__
#define __IRowsetNewRowAfter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetNewRowAfter
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetNewRowAfter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetNewRowAfter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetNewDataAfter( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbbmPrevious,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbmPrevious,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ BYTE __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetNewRowAfterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetNewRowAfter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetNewRowAfter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetNewRowAfter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNewDataAfter )( 
            IRowsetNewRowAfter __RPC_FAR * This,
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbbmPrevious,
            /* [size_is][in] */ const BYTE __RPC_FAR *pbmPrevious,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ const BYTE __RPC_FAR *pData,
            /* [out] */ HROW __RPC_FAR *phRow);
        
        END_INTERFACE
    } IRowsetNewRowAfterVtbl;

    interface IRowsetNewRowAfter
    {
        CONST_VTBL struct IRowsetNewRowAfterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetNewRowAfter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetNewRowAfter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetNewRowAfter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetNewRowAfter_SetNewDataAfter(This,hChapter,cbbmPrevious,pbmPrevious,hAccessor,pData,phRow)	\
    (This)->lpVtbl -> SetNewDataAfter(This,hChapter,cbbmPrevious,pbmPrevious,hAccessor,pData,phRow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetNewRowAfter_SetNewDataAfter_Proxy( 
    IRowsetNewRowAfter __RPC_FAR * This,
    /* [in] */ HCHAPTER hChapter,
    /* [in] */ ULONG cbbmPrevious,
    /* [size_is][in] */ const BYTE __RPC_FAR *pbmPrevious,
    /* [in] */ HACCESSOR hAccessor,
    /* [in] */ const BYTE __RPC_FAR *pData,
    /* [out] */ HROW __RPC_FAR *phRow);


void __RPC_STUB IRowsetNewRowAfter_SetNewDataAfter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetNewRowAfter_INTERFACE_DEFINED__ */


#ifndef __IRowsetFind_INTERFACE_DEFINED__
#define __IRowsetFind_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetFind
 * at Thu Jun 27 21:09:23 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef DWORD DBCOMPAREOPS;


enum DBCOMPAREOPSENUM
    {	DBCOMPAREOPS_LT	= 0,
	DBCOMPAREOPS_LE	= DBCOMPAREOPS_LT + 1,
	DBCOMPAREOPS_EQ	= DBCOMPAREOPS_LE + 1,
	DBCOMPAREOPS_GE	= DBCOMPAREOPS_EQ + 1,
	DBCOMPAREOPS_GT	= DBCOMPAREOPS_GE + 1,
	DBCOMPAREOPS_PARTIALEQ	= DBCOMPAREOPS_GT + 1,
	DBCOMPAREOPS_NE	= DBCOMPAREOPS_PARTIALEQ + 1,
	DBCOMPAREOPS_INCLUDENULLS	= 0x1000
    };

EXTERN_C const IID IID_IRowsetFind;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetFind : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRowsByValues( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ ULONG cValues,
            /* [size_is][in] */ const ULONG __RPC_FAR rgColumns[  ],
            /* [size_is][in] */ const DBTYPE __RPC_FAR rgValueTypes[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgValues[  ],
            /* [size_is][in] */ const DBCOMPAREOPS __RPC_FAR rgCompareOps[  ],
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetFindVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetFind __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetFind __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetFind __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowsByValues )( 
            IRowsetFind __RPC_FAR * This,
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG cbBookmark,
            /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
            /* [in] */ LONG lRowsOffset,
            /* [in] */ ULONG cValues,
            /* [size_is][in] */ const ULONG __RPC_FAR rgColumns[  ],
            /* [size_is][in] */ const DBTYPE __RPC_FAR rgValueTypes[  ],
            /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgValues[  ],
            /* [size_is][in] */ const DBCOMPAREOPS __RPC_FAR rgCompareOps[  ],
            /* [in] */ LONG cRows,
            /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
            /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows);
        
        END_INTERFACE
    } IRowsetFindVtbl;

    interface IRowsetFind
    {
        CONST_VTBL struct IRowsetFindVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetFind_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetFind_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetFind_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetFind_GetRowsByValues(This,hChapter,cbBookmark,pBookmark,lRowsOffset,cValues,rgColumns,rgValueTypes,rgValues,rgCompareOps,cRows,pcRowsObtained,prghRows)	\
    (This)->lpVtbl -> GetRowsByValues(This,hChapter,cbBookmark,pBookmark,lRowsOffset,cValues,rgColumns,rgValueTypes,rgValues,rgCompareOps,cRows,pcRowsObtained,prghRows)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetFind_GetRowsByValues_Proxy( 
    IRowsetFind __RPC_FAR * This,
    /* [in] */ HCHAPTER hChapter,
    /* [in] */ ULONG cbBookmark,
    /* [size_is][in] */ const BYTE __RPC_FAR *pBookmark,
    /* [in] */ LONG lRowsOffset,
    /* [in] */ ULONG cValues,
    /* [size_is][in] */ const ULONG __RPC_FAR rgColumns[  ],
    /* [size_is][in] */ const DBTYPE __RPC_FAR rgValueTypes[  ],
    /* [size_is][in] */ const BYTE __RPC_FAR *__RPC_FAR rgValues[  ],
    /* [size_is][in] */ const DBCOMPAREOPS __RPC_FAR rgCompareOps[  ],
    /* [in] */ LONG cRows,
    /* [out] */ ULONG __RPC_FAR *pcRowsObtained,
    /* [size_is][out][in] */ HROW __RPC_FAR *__RPC_FAR *prghRows);


void __RPC_STUB IRowsetFind_GetRowsByValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetFind_INTERFACE_DEFINED__ */

#endif /* OLEDBVER >= 0x0200 */


/****************************************
 * Generated header for interface: __MIDL__intf_0081
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0086_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0086_v0_0_s_ifspec;

#ifndef __IRowsetNotify_INTERFACE_DEFINED__
#define __IRowsetNotify_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetNotify
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef DWORD DBEVENTPHASE;


enum DBEVENTPHASEENUM
    {	DBEVENTPHASE_OKTODO	= 0,
	DBEVENTPHASE_ABOUTTODO	= DBEVENTPHASE_OKTODO + 1,
	DBEVENTPHASE_SYNCHAFTER	= DBEVENTPHASE_ABOUTTODO + 1,
	DBEVENTPHASE_FAILEDTODO	= DBEVENTPHASE_SYNCHAFTER + 1,
	DBEVENTPHASE_DIDEVENT	= DBEVENTPHASE_FAILEDTODO + 1
    };
typedef DWORD DBREASON;


enum DBREASONENUM
    {	DBREASON_ROWSET_FETCHPOSITIONCHANGE	= 0,
	DBREASON_ROWSET_RELEASE	= DBREASON_ROWSET_FETCHPOSITIONCHANGE + 1,
	DBREASON_COLUMN_SET	= DBREASON_ROWSET_RELEASE + 1,
	DBREASON_COLUMN_RECALCULATED	= DBREASON_COLUMN_SET + 1,
	DBREASON_ROW_ACTIVATE	= DBREASON_COLUMN_RECALCULATED + 1,
	DBREASON_ROW_RELEASE	= DBREASON_ROW_ACTIVATE + 1,
	DBREASON_ROW_DELETE	= DBREASON_ROW_RELEASE + 1,
	DBREASON_ROW_FIRSTCHANGE	= DBREASON_ROW_DELETE + 1,
	DBREASON_ROW_INSERT	= DBREASON_ROW_FIRSTCHANGE + 1,
	DBREASON_ROW_RESYNCH	= DBREASON_ROW_INSERT + 1,
	DBREASON_ROW_UNDOCHANGE	= DBREASON_ROW_RESYNCH + 1,
	DBREASON_ROW_UNDOINSERT	= DBREASON_ROW_UNDOCHANGE + 1,
	DBREASON_ROW_UNDODELETE	= DBREASON_ROW_UNDOINSERT + 1,
	DBREASON_ROW_UPDATE	= DBREASON_ROW_UNDODELETE + 1
    };

EXTERN_C const IID IID_IRowsetNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnFieldChange( 
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ HROW hRow,
            /* [in] */ ULONG cColumns,
            /* [size_is][in] */ ULONG __RPC_FAR rgColumns[  ],
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnRowChange( 
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnRowsetChange( 
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnFieldChange )( 
            IRowsetNotify __RPC_FAR * This,
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ HROW hRow,
            /* [in] */ ULONG cColumns,
            /* [size_is][in] */ ULONG __RPC_FAR rgColumns[  ],
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnRowChange )( 
            IRowsetNotify __RPC_FAR * This,
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ ULONG cRows,
            /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnRowsetChange )( 
            IRowsetNotify __RPC_FAR * This,
            /* [in] */ IRowset __RPC_FAR *pRowset,
            /* [in] */ DBREASON eReason,
            /* [in] */ DBEVENTPHASE ePhase,
            /* [in] */ BOOL fCantDeny);
        
        END_INTERFACE
    } IRowsetNotifyVtbl;

    interface IRowsetNotify
    {
        CONST_VTBL struct IRowsetNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetNotify_OnFieldChange(This,pRowset,hRow,cColumns,rgColumns,eReason,ePhase,fCantDeny)	\
    (This)->lpVtbl -> OnFieldChange(This,pRowset,hRow,cColumns,rgColumns,eReason,ePhase,fCantDeny)

#define IRowsetNotify_OnRowChange(This,pRowset,cRows,rghRows,eReason,ePhase,fCantDeny)	\
    (This)->lpVtbl -> OnRowChange(This,pRowset,cRows,rghRows,eReason,ePhase,fCantDeny)

#define IRowsetNotify_OnRowsetChange(This,pRowset,eReason,ePhase,fCantDeny)	\
    (This)->lpVtbl -> OnRowsetChange(This,pRowset,eReason,ePhase,fCantDeny)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetNotify_OnFieldChange_Proxy( 
    IRowsetNotify __RPC_FAR * This,
    /* [in] */ IRowset __RPC_FAR *pRowset,
    /* [in] */ HROW hRow,
    /* [in] */ ULONG cColumns,
    /* [size_is][in] */ ULONG __RPC_FAR rgColumns[  ],
    /* [in] */ DBREASON eReason,
    /* [in] */ DBEVENTPHASE ePhase,
    /* [in] */ BOOL fCantDeny);


void __RPC_STUB IRowsetNotify_OnFieldChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetNotify_OnRowChange_Proxy( 
    IRowsetNotify __RPC_FAR * This,
    /* [in] */ IRowset __RPC_FAR *pRowset,
    /* [in] */ ULONG cRows,
    /* [size_is][in] */ const HROW __RPC_FAR rghRows[  ],
    /* [in] */ DBREASON eReason,
    /* [in] */ DBEVENTPHASE ePhase,
    /* [in] */ BOOL fCantDeny);


void __RPC_STUB IRowsetNotify_OnRowChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetNotify_OnRowsetChange_Proxy( 
    IRowsetNotify __RPC_FAR * This,
    /* [in] */ IRowset __RPC_FAR *pRowset,
    /* [in] */ DBREASON eReason,
    /* [in] */ DBEVENTPHASE ePhase,
    /* [in] */ BOOL fCantDeny);


void __RPC_STUB IRowsetNotify_OnRowsetChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetNotify_INTERFACE_DEFINED__ */


#ifndef __IRowsetIndex_INTERFACE_DEFINED__
#define __IRowsetIndex_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetIndex
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef DWORD DBSEEK;


enum DBSEEKENUM
    {	DBSEEK_INVALID	= 0,
	DBSEEK_FIRSTEQ	= 0x1,
	DBSEEK_LASTEQ	= 0x2,
	DBSEEK_GE	= 0x4,
	DBSEEK_GT	= 0x8,
	DBSEEK_LE	= 0x10,
	DBSEEK_LT	= 0x20
    };
typedef DWORD DBRANGE;


enum DBRANGEENUM
    {	DBRANGE_INCLUSIVESTART	= 0,
	DBRANGE_INCLUSIVEEND	= 0,
	DBRANGE_EXCLUSIVESTART	= 0x1,
	DBRANGE_EXCLUSIVEEND	= 0x2,
	DBRANGE_EXCLUDENULLS	= 0x4,
	DBRANGE_PREFIX	= 0x8,
	DBRANGE_MATCH	= 0x10
    };

EXTERN_C const IID IID_IRowsetIndex;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetIndex : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIndexInfo( 
            /* [out][in] */ ULONG __RPC_FAR *pcKeyColumns,
            /* [size_is][size_is][out] */ DBINDEXCOLUMNDESC __RPC_FAR *__RPC_FAR *prgIndexColumnDesc,
            /* [out][in] */ ULONG __RPC_FAR *pcIndexProperties,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgIndexProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ ULONG cKeyValues,
            /* [in] */ void __RPC_FAR *pData,
            /* [in] */ DBSEEK dwSeekOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRange( 
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ ULONG cStartKeyColumns,
            /* [in] */ void __RPC_FAR *pStartData,
            /* [in] */ ULONG cEndKeyColumns,
            /* [in] */ void __RPC_FAR *pEndData,
            /* [in] */ DBRANGE dwRangeOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetIndexVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetIndex __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetIndex __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetIndex __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIndexInfo )( 
            IRowsetIndex __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcKeyColumns,
            /* [size_is][size_is][out] */ DBINDEXCOLUMNDESC __RPC_FAR *__RPC_FAR *prgIndexColumnDesc,
            /* [out][in] */ ULONG __RPC_FAR *pcIndexProperties,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgIndexProperties);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IRowsetIndex __RPC_FAR * This,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ ULONG cKeyValues,
            /* [in] */ void __RPC_FAR *pData,
            /* [in] */ DBSEEK dwSeekOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRange )( 
            IRowsetIndex __RPC_FAR * This,
            /* [in] */ HACCESSOR hAccessor,
            /* [in] */ ULONG cStartKeyColumns,
            /* [in] */ void __RPC_FAR *pStartData,
            /* [in] */ ULONG cEndKeyColumns,
            /* [in] */ void __RPC_FAR *pEndData,
            /* [in] */ DBRANGE dwRangeOptions);
        
        END_INTERFACE
    } IRowsetIndexVtbl;

    interface IRowsetIndex
    {
        CONST_VTBL struct IRowsetIndexVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetIndex_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetIndex_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetIndex_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetIndex_GetIndexInfo(This,pcKeyColumns,prgIndexColumnDesc,pcIndexProperties,prgIndexProperties)	\
    (This)->lpVtbl -> GetIndexInfo(This,pcKeyColumns,prgIndexColumnDesc,pcIndexProperties,prgIndexProperties)

#define IRowsetIndex_Seek(This,hAccessor,cKeyValues,pData,dwSeekOptions)	\
    (This)->lpVtbl -> Seek(This,hAccessor,cKeyValues,pData,dwSeekOptions)

#define IRowsetIndex_SetRange(This,hAccessor,cStartKeyColumns,pStartData,cEndKeyColumns,pEndData,dwRangeOptions)	\
    (This)->lpVtbl -> SetRange(This,hAccessor,cStartKeyColumns,pStartData,cEndKeyColumns,pEndData,dwRangeOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetIndex_GetIndexInfo_Proxy( 
    IRowsetIndex __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcKeyColumns,
    /* [size_is][size_is][out] */ DBINDEXCOLUMNDESC __RPC_FAR *__RPC_FAR *prgIndexColumnDesc,
    /* [out][in] */ ULONG __RPC_FAR *pcIndexProperties,
    /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgIndexProperties);


void __RPC_STUB IRowsetIndex_GetIndexInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetIndex_Seek_Proxy( 
    IRowsetIndex __RPC_FAR * This,
    /* [in] */ HACCESSOR hAccessor,
    /* [in] */ ULONG cKeyValues,
    /* [in] */ void __RPC_FAR *pData,
    /* [in] */ DBSEEK dwSeekOptions);


void __RPC_STUB IRowsetIndex_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetIndex_SetRange_Proxy( 
    IRowsetIndex __RPC_FAR * This,
    /* [in] */ HACCESSOR hAccessor,
    /* [in] */ ULONG cStartKeyColumns,
    /* [in] */ void __RPC_FAR *pStartData,
    /* [in] */ ULONG cEndKeyColumns,
    /* [in] */ void __RPC_FAR *pEndData,
    /* [in] */ DBRANGE dwRangeOptions);


void __RPC_STUB IRowsetIndex_SetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetIndex_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0088
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0093_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0093_v0_0_s_ifspec;

#ifndef __ICommand_INTERFACE_DEFINED__
#define __ICommand_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICommand
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ICommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICommand : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out][in] */ DBPARAMS __RPC_FAR *pParams,
            /* [out] */ LONG __RPC_FAR *pcRowsAffected,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDBSession( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommand __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            ICommand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Execute )( 
            ICommand __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out][in] */ DBPARAMS __RPC_FAR *pParams,
            /* [out] */ LONG __RPC_FAR *pcRowsAffected,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDBSession )( 
            ICommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSession);
        
        END_INTERFACE
    } ICommandVtbl;

    interface ICommand
    {
        CONST_VTBL struct ICommandVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommand_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define ICommand_Execute(This,pUnkOuter,riid,pParams,pcRowsAffected,ppRowset)	\
    (This)->lpVtbl -> Execute(This,pUnkOuter,riid,pParams,pcRowsAffected,ppRowset)

#define ICommand_GetDBSession(This,riid,ppSession)	\
    (This)->lpVtbl -> GetDBSession(This,riid,ppSession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICommand_Cancel_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommand_Execute_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [out][in] */ DBPARAMS __RPC_FAR *pParams,
    /* [out] */ LONG __RPC_FAR *pcRowsAffected,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);


void __RPC_STUB ICommand_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommand_GetDBSession_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSession);


void __RPC_STUB ICommand_GetDBSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommand_INTERFACE_DEFINED__ */


#ifndef __IConvertType_INTERFACE_DEFINED__
#define __IConvertType_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IConvertType
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef DWORD DBCONVERTFLAGS;


enum DBCONVERTFLAGSENUM
    {	DBCONVERTFLAGS_COLUMN	= 0,
	DBCONVERTFLAGS_PARAMETER	= 0x1
    };

EXTERN_C const IID IID_IConvertType;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IConvertType : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CanConvert( 
            /* [in] */ DBTYPE wFromType,
            /* [in] */ DBTYPE wToType,
            /* [in] */ DBCONVERTFLAGS dwConvertFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IConvertTypeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IConvertType __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IConvertType __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IConvertType __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanConvert )( 
            IConvertType __RPC_FAR * This,
            /* [in] */ DBTYPE wFromType,
            /* [in] */ DBTYPE wToType,
            /* [in] */ DBCONVERTFLAGS dwConvertFlags);
        
        END_INTERFACE
    } IConvertTypeVtbl;

    interface IConvertType
    {
        CONST_VTBL struct IConvertTypeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IConvertType_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IConvertType_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IConvertType_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IConvertType_CanConvert(This,wFromType,wToType,dwConvertFlags)	\
    (This)->lpVtbl -> CanConvert(This,wFromType,wToType,dwConvertFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IConvertType_CanConvert_Proxy( 
    IConvertType __RPC_FAR * This,
    /* [in] */ DBTYPE wFromType,
    /* [in] */ DBTYPE wToType,
    /* [in] */ DBCONVERTFLAGS dwConvertFlags);


void __RPC_STUB IConvertType_CanConvert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IConvertType_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0095
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0096_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0096_v0_0_s_ifspec;

#ifndef __ICommandPrepare_INTERFACE_DEFINED__
#define __ICommandPrepare_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICommandPrepare
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ICommandPrepare;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICommandPrepare : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Prepare( 
            /* [in] */ ULONG cExpectedRuns) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unprepare( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandPrepareVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommandPrepare __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommandPrepare __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommandPrepare __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Prepare )( 
            ICommandPrepare __RPC_FAR * This,
            /* [in] */ ULONG cExpectedRuns);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unprepare )( 
            ICommandPrepare __RPC_FAR * This);
        
        END_INTERFACE
    } ICommandPrepareVtbl;

    interface ICommandPrepare
    {
        CONST_VTBL struct ICommandPrepareVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommandPrepare_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommandPrepare_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommandPrepare_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommandPrepare_Prepare(This,cExpectedRuns)	\
    (This)->lpVtbl -> Prepare(This,cExpectedRuns)

#define ICommandPrepare_Unprepare(This)	\
    (This)->lpVtbl -> Unprepare(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICommandPrepare_Prepare_Proxy( 
    ICommandPrepare __RPC_FAR * This,
    /* [in] */ ULONG cExpectedRuns);


void __RPC_STUB ICommandPrepare_Prepare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommandPrepare_Unprepare_Proxy( 
    ICommandPrepare __RPC_FAR * This);


void __RPC_STUB ICommandPrepare_Unprepare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommandPrepare_INTERFACE_DEFINED__ */


#ifndef __ICommandProperties_INTERFACE_DEFINED__
#define __ICommandProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICommandProperties
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ICommandProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICommandProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperties( 
            /* [in] */ const ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperties( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommandProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommandProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommandProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperties )( 
            ICommandProperties __RPC_FAR * This,
            /* [in] */ const ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperties )( 
            ICommandProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);
        
        END_INTERFACE
    } ICommandPropertiesVtbl;

    interface ICommandProperties
    {
        CONST_VTBL struct ICommandPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommandProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommandProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommandProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommandProperties_GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)	\
    (This)->lpVtbl -> GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)

#define ICommandProperties_SetProperties(This,cPropertySets,rgPropertySets)	\
    (This)->lpVtbl -> SetProperties(This,cPropertySets,rgPropertySets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICommandProperties_GetProperties_Proxy( 
    ICommandProperties __RPC_FAR * This,
    /* [in] */ const ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out] */ ULONG __RPC_FAR *pcPropertySets,
    /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);


void __RPC_STUB ICommandProperties_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommandProperties_SetProperties_Proxy( 
    ICommandProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);


void __RPC_STUB ICommandProperties_SetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommandProperties_INTERFACE_DEFINED__ */


#ifndef __ICommandText_INTERFACE_DEFINED__
#define __ICommandText_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICommandText
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ICommandText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICommandText : public ICommand
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCommandText( 
            /* [out][in] */ GUID __RPC_FAR *pguidDialect,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszCommand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCommandText( 
            /* [in] */ REFGUID rguidDialect,
            /* [in] */ LPCOLESTR pwszCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommandText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommandText __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommandText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            ICommandText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Execute )( 
            ICommandText __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out][in] */ DBPARAMS __RPC_FAR *pParams,
            /* [out] */ LONG __RPC_FAR *pcRowsAffected,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDBSession )( 
            ICommandText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSession);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCommandText )( 
            ICommandText __RPC_FAR * This,
            /* [out][in] */ GUID __RPC_FAR *pguidDialect,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszCommand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCommandText )( 
            ICommandText __RPC_FAR * This,
            /* [in] */ REFGUID rguidDialect,
            /* [in] */ LPCOLESTR pwszCommand);
        
        END_INTERFACE
    } ICommandTextVtbl;

    interface ICommandText
    {
        CONST_VTBL struct ICommandTextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommandText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommandText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommandText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommandText_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define ICommandText_Execute(This,pUnkOuter,riid,pParams,pcRowsAffected,ppRowset)	\
    (This)->lpVtbl -> Execute(This,pUnkOuter,riid,pParams,pcRowsAffected,ppRowset)

#define ICommandText_GetDBSession(This,riid,ppSession)	\
    (This)->lpVtbl -> GetDBSession(This,riid,ppSession)


#define ICommandText_GetCommandText(This,pguidDialect,ppwszCommand)	\
    (This)->lpVtbl -> GetCommandText(This,pguidDialect,ppwszCommand)

#define ICommandText_SetCommandText(This,rguidDialect,pwszCommand)	\
    (This)->lpVtbl -> SetCommandText(This,rguidDialect,pwszCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICommandText_GetCommandText_Proxy( 
    ICommandText __RPC_FAR * This,
    /* [out][in] */ GUID __RPC_FAR *pguidDialect,
    /* [out] */ LPOLESTR __RPC_FAR *ppwszCommand);


void __RPC_STUB ICommandText_GetCommandText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommandText_SetCommandText_Proxy( 
    ICommandText __RPC_FAR * This,
    /* [in] */ REFGUID rguidDialect,
    /* [in] */ LPCOLESTR pwszCommand);


void __RPC_STUB ICommandText_SetCommandText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommandText_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0099
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0101_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0101_v0_0_s_ifspec;

#ifndef __ICommandWithParameters_INTERFACE_DEFINED__
#define __ICommandWithParameters_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICommandWithParameters
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef struct  tagDBPARAMBINDINFO
    {
    LPOLESTR pwszDataSourceType;
    LPOLESTR pwszName;
    ULONG ulParamSize;
    DBPARAMFLAGS dwFlags;
    BYTE bPrecision;
    BYTE bScale;
    }	DBPARAMBINDINFO;


EXTERN_C const IID IID_ICommandWithParameters;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICommandWithParameters : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetParameterInfo( 
            /* [out][in] */ ULONG __RPC_FAR *pcParams,
            /* [size_is][size_is][out] */ DBPARAMINFO __RPC_FAR *__RPC_FAR *prgParamInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppNamesBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapParameterNames( 
            /* [in] */ ULONG cParamNames,
            /* [size_is][in] */ const OLECHAR __RPC_FAR *__RPC_FAR rgParamNames[  ],
            /* [size_is][out] */ LONG __RPC_FAR rgParamOrdinals[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetParameterInfo( 
            /* [in] */ ULONG cParams,
            /* [size_is][in] */ const ULONG __RPC_FAR rgParamOrdinals[  ],
            /* [size_is][in] */ const DBPARAMBINDINFO __RPC_FAR rgParamBindInfo[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandWithParametersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommandWithParameters __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommandWithParameters __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommandWithParameters __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParameterInfo )( 
            ICommandWithParameters __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcParams,
            /* [size_is][size_is][out] */ DBPARAMINFO __RPC_FAR *__RPC_FAR *prgParamInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppNamesBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapParameterNames )( 
            ICommandWithParameters __RPC_FAR * This,
            /* [in] */ ULONG cParamNames,
            /* [size_is][in] */ const OLECHAR __RPC_FAR *__RPC_FAR rgParamNames[  ],
            /* [size_is][out] */ LONG __RPC_FAR rgParamOrdinals[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetParameterInfo )( 
            ICommandWithParameters __RPC_FAR * This,
            /* [in] */ ULONG cParams,
            /* [size_is][in] */ const ULONG __RPC_FAR rgParamOrdinals[  ],
            /* [size_is][in] */ const DBPARAMBINDINFO __RPC_FAR rgParamBindInfo[  ]);
        
        END_INTERFACE
    } ICommandWithParametersVtbl;

    interface ICommandWithParameters
    {
        CONST_VTBL struct ICommandWithParametersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommandWithParameters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommandWithParameters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommandWithParameters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommandWithParameters_GetParameterInfo(This,pcParams,prgParamInfo,ppNamesBuffer)	\
    (This)->lpVtbl -> GetParameterInfo(This,pcParams,prgParamInfo,ppNamesBuffer)

#define ICommandWithParameters_MapParameterNames(This,cParamNames,rgParamNames,rgParamOrdinals)	\
    (This)->lpVtbl -> MapParameterNames(This,cParamNames,rgParamNames,rgParamOrdinals)

#define ICommandWithParameters_SetParameterInfo(This,cParams,rgParamOrdinals,rgParamBindInfo)	\
    (This)->lpVtbl -> SetParameterInfo(This,cParams,rgParamOrdinals,rgParamBindInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICommandWithParameters_GetParameterInfo_Proxy( 
    ICommandWithParameters __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcParams,
    /* [size_is][size_is][out] */ DBPARAMINFO __RPC_FAR *__RPC_FAR *prgParamInfo,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppNamesBuffer);


void __RPC_STUB ICommandWithParameters_GetParameterInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommandWithParameters_MapParameterNames_Proxy( 
    ICommandWithParameters __RPC_FAR * This,
    /* [in] */ ULONG cParamNames,
    /* [size_is][in] */ const OLECHAR __RPC_FAR *__RPC_FAR rgParamNames[  ],
    /* [size_is][out] */ LONG __RPC_FAR rgParamOrdinals[  ]);


void __RPC_STUB ICommandWithParameters_MapParameterNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICommandWithParameters_SetParameterInfo_Proxy( 
    ICommandWithParameters __RPC_FAR * This,
    /* [in] */ ULONG cParams,
    /* [size_is][in] */ const ULONG __RPC_FAR rgParamOrdinals[  ],
    /* [size_is][in] */ const DBPARAMBINDINFO __RPC_FAR rgParamBindInfo[  ]);


void __RPC_STUB ICommandWithParameters_SetParameterInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommandWithParameters_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0102
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0103_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0103_v0_0_s_ifspec;

#ifndef __IColumnsRowset_INTERFACE_DEFINED__
#define __IColumnsRowset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IColumnsRowset
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IColumnsRowset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IColumnsRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAvailableColumns( 
            /* [out][in] */ ULONG __RPC_FAR *pcOptColumns,
            /* [size_is][size_is][out] */ DBID __RPC_FAR *__RPC_FAR *prgOptColumns) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumnsRowset( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ ULONG cOptColumns,
            /* [size_is][in] */ const DBID __RPC_FAR rgOptColumns[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppColRowset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnsRowsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColumnsRowset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColumnsRowset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColumnsRowset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAvailableColumns )( 
            IColumnsRowset __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcOptColumns,
            /* [size_is][size_is][out] */ DBID __RPC_FAR *__RPC_FAR *prgOptColumns);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnsRowset )( 
            IColumnsRowset __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ ULONG cOptColumns,
            /* [size_is][in] */ const DBID __RPC_FAR rgOptColumns[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppColRowset);
        
        END_INTERFACE
    } IColumnsRowsetVtbl;

    interface IColumnsRowset
    {
        CONST_VTBL struct IColumnsRowsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColumnsRowset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColumnsRowset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColumnsRowset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColumnsRowset_GetAvailableColumns(This,pcOptColumns,prgOptColumns)	\
    (This)->lpVtbl -> GetAvailableColumns(This,pcOptColumns,prgOptColumns)

#define IColumnsRowset_GetColumnsRowset(This,pUnkOuter,cOptColumns,rgOptColumns,riid,cPropertySets,rgPropertySets,ppColRowset)	\
    (This)->lpVtbl -> GetColumnsRowset(This,pUnkOuter,cOptColumns,rgOptColumns,riid,cPropertySets,rgPropertySets,ppColRowset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColumnsRowset_GetAvailableColumns_Proxy( 
    IColumnsRowset __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcOptColumns,
    /* [size_is][size_is][out] */ DBID __RPC_FAR *__RPC_FAR *prgOptColumns);


void __RPC_STUB IColumnsRowset_GetAvailableColumns_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColumnsRowset_GetColumnsRowset_Proxy( 
    IColumnsRowset __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ ULONG cOptColumns,
    /* [size_is][in] */ const DBID __RPC_FAR rgOptColumns[  ],
    /* [in] */ REFIID riid,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppColRowset);


void __RPC_STUB IColumnsRowset_GetColumnsRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColumnsRowset_INTERFACE_DEFINED__ */


#ifndef __IColumnsInfo_INTERFACE_DEFINED__
#define __IColumnsInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IColumnsInfo
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IColumnsInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IColumnsInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetColumnInfo( 
            /* [out][in] */ ULONG __RPC_FAR *pcColumns,
            /* [size_is][size_is][out] */ DBCOLUMNINFO __RPC_FAR *__RPC_FAR *prgInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapColumnIDs( 
            /* [in] */ ULONG cColumnIDs,
            /* [size_is][in] */ const DBID __RPC_FAR rgColumnIDs[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgColumns[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnsInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColumnsInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColumnsInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColumnsInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnInfo )( 
            IColumnsInfo __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcColumns,
            /* [size_is][size_is][out] */ DBCOLUMNINFO __RPC_FAR *__RPC_FAR *prgInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapColumnIDs )( 
            IColumnsInfo __RPC_FAR * This,
            /* [in] */ ULONG cColumnIDs,
            /* [size_is][in] */ const DBID __RPC_FAR rgColumnIDs[  ],
            /* [size_is][out] */ ULONG __RPC_FAR rgColumns[  ]);
        
        END_INTERFACE
    } IColumnsInfoVtbl;

    interface IColumnsInfo
    {
        CONST_VTBL struct IColumnsInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColumnsInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColumnsInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColumnsInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColumnsInfo_GetColumnInfo(This,pcColumns,prgInfo,ppStringsBuffer)	\
    (This)->lpVtbl -> GetColumnInfo(This,pcColumns,prgInfo,ppStringsBuffer)

#define IColumnsInfo_MapColumnIDs(This,cColumnIDs,rgColumnIDs,rgColumns)	\
    (This)->lpVtbl -> MapColumnIDs(This,cColumnIDs,rgColumnIDs,rgColumns)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColumnsInfo_GetColumnInfo_Proxy( 
    IColumnsInfo __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcColumns,
    /* [size_is][size_is][out] */ DBCOLUMNINFO __RPC_FAR *__RPC_FAR *prgInfo,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer);


void __RPC_STUB IColumnsInfo_GetColumnInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColumnsInfo_MapColumnIDs_Proxy( 
    IColumnsInfo __RPC_FAR * This,
    /* [in] */ ULONG cColumnIDs,
    /* [size_is][in] */ const DBID __RPC_FAR rgColumnIDs[  ],
    /* [size_is][out] */ ULONG __RPC_FAR rgColumns[  ]);


void __RPC_STUB IColumnsInfo_MapColumnIDs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColumnsInfo_INTERFACE_DEFINED__ */


#ifndef __IDBCreateCommand_INTERFACE_DEFINED__
#define __IDBCreateCommand_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBCreateCommand
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBCreateCommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBCreateCommand : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateCommand( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBCreateCommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBCreateCommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBCreateCommand __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBCreateCommand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateCommand )( 
            IDBCreateCommand __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvCommand);
        
        END_INTERFACE
    } IDBCreateCommandVtbl;

    interface IDBCreateCommand
    {
        CONST_VTBL struct IDBCreateCommandVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBCreateCommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBCreateCommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBCreateCommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBCreateCommand_CreateCommand(This,pUnkOuter,riid,ppvCommand)	\
    (This)->lpVtbl -> CreateCommand(This,pUnkOuter,riid,ppvCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBCreateCommand_CreateCommand_Proxy( 
    IDBCreateCommand __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvCommand);


void __RPC_STUB IDBCreateCommand_CreateCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBCreateCommand_INTERFACE_DEFINED__ */


#ifndef __IDBCreateSession_INTERFACE_DEFINED__
#define __IDBCreateSession_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBCreateSession
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBCreateSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBCreateSession : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateSession( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBCreateSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBCreateSession __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBCreateSession __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBCreateSession __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSession )( 
            IDBCreateSession __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession);
        
        END_INTERFACE
    } IDBCreateSessionVtbl;

    interface IDBCreateSession
    {
        CONST_VTBL struct IDBCreateSessionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBCreateSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBCreateSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBCreateSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBCreateSession_CreateSession(This,pUnkOuter,riid,ppDBSession)	\
    (This)->lpVtbl -> CreateSession(This,pUnkOuter,riid,ppDBSession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBCreateSession_CreateSession_Proxy( 
    IDBCreateSession __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession);


void __RPC_STUB IDBCreateSession_CreateSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBCreateSession_INTERFACE_DEFINED__ */


#ifndef __ISourcesRowset_INTERFACE_DEFINED__
#define __ISourcesRowset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISourcesRowset
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef DWORD DBSOURCETYPE;


enum DBSOURCETYPEENUM
    {	DBSOURCETYPE_DATASOURCE	= 1,
	DBSOURCETYPE_ENUMERATOR	= 2
    };

EXTERN_C const IID IID_ISourcesRowset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISourcesRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSourcesRowset( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cProperties,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgProperties[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSourcesRowset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISourcesRowsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISourcesRowset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISourcesRowset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISourcesRowset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourcesRowset )( 
            ISourcesRowset __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cProperties,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgProperties[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSourcesRowset);
        
        END_INTERFACE
    } ISourcesRowsetVtbl;

    interface ISourcesRowset
    {
        CONST_VTBL struct ISourcesRowsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISourcesRowset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISourcesRowset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISourcesRowset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISourcesRowset_GetSourcesRowset(This,pUnkOuter,riid,cProperties,rgProperties,ppSourcesRowset)	\
    (This)->lpVtbl -> GetSourcesRowset(This,pUnkOuter,riid,cProperties,rgProperties,ppSourcesRowset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISourcesRowset_GetSourcesRowset_Proxy( 
    ISourcesRowset __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [in] */ ULONG cProperties,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgProperties[  ],
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppSourcesRowset);


void __RPC_STUB ISourcesRowset_GetSourcesRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISourcesRowset_INTERFACE_DEFINED__ */


#ifndef __IDBProperties_INTERFACE_DEFINED__
#define __IDBProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBProperties
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperties( 
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyInfo( 
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
            /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *prgPropertyInfoSets,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperties( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperties )( 
            IDBProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyInfo )( 
            IDBProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
            /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *prgPropertyInfoSets,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperties )( 
            IDBProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);
        
        END_INTERFACE
    } IDBPropertiesVtbl;

    interface IDBProperties
    {
        CONST_VTBL struct IDBPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBProperties_GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)	\
    (This)->lpVtbl -> GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)

#define IDBProperties_GetPropertyInfo(This,cPropertyIDSets,rgPropertyIDSets,pcPropertyInfoSets,prgPropertyInfoSets,ppDescBuffer)	\
    (This)->lpVtbl -> GetPropertyInfo(This,cPropertyIDSets,rgPropertyIDSets,pcPropertyInfoSets,prgPropertyInfoSets,ppDescBuffer)

#define IDBProperties_SetProperties(This,cPropertySets,rgPropertySets)	\
    (This)->lpVtbl -> SetProperties(This,cPropertySets,rgPropertySets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBProperties_GetProperties_Proxy( 
    IDBProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
    /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);


void __RPC_STUB IDBProperties_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBProperties_GetPropertyInfo_Proxy( 
    IDBProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
    /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *prgPropertyInfoSets,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer);


void __RPC_STUB IDBProperties_GetPropertyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBProperties_SetProperties_Proxy( 
    IDBProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);


void __RPC_STUB IDBProperties_SetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBProperties_INTERFACE_DEFINED__ */


#ifndef __IDBInitialize_INTERFACE_DEFINED__
#define __IDBInitialize_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBInitialize
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBInitialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBInitialize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Uninitialize( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBInitializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBInitialize __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBInitialize __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBInitialize __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IDBInitialize __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Uninitialize )( 
            IDBInitialize __RPC_FAR * This);
        
        END_INTERFACE
    } IDBInitializeVtbl;

    interface IDBInitialize
    {
        CONST_VTBL struct IDBInitializeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBInitialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBInitialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBInitialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBInitialize_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define IDBInitialize_Uninitialize(This)	\
    (This)->lpVtbl -> Uninitialize(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBInitialize_Initialize_Proxy( 
    IDBInitialize __RPC_FAR * This);


void __RPC_STUB IDBInitialize_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBInitialize_Uninitialize_Proxy( 
    IDBInitialize __RPC_FAR * This);


void __RPC_STUB IDBInitialize_Uninitialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBInitialize_INTERFACE_DEFINED__ */


#ifndef __IDBInfo_INTERFACE_DEFINED__
#define __IDBInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBInfo
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


typedef DWORD DBLITERAL;


enum DBLITERALENUM
    {	DBLITERAL_INVALID	= 0,
	DBLITERAL_BINARY_LITERAL	= DBLITERAL_INVALID + 1,
	DBLITERAL_CATALOG_NAME	= DBLITERAL_BINARY_LITERAL + 1,
	DBLITERAL_CATALOG_SEPARATOR	= DBLITERAL_CATALOG_NAME + 1,
	DBLITERAL_CHAR_LITERAL	= DBLITERAL_CATALOG_SEPARATOR + 1,
	DBLITERAL_COLUMN_ALIAS	= DBLITERAL_CHAR_LITERAL + 1,
	DBLITERAL_COLUMN_NAME	= DBLITERAL_COLUMN_ALIAS + 1,
	DBLITERAL_CORRELATION_NAME	= DBLITERAL_COLUMN_NAME + 1,
	DBLITERAL_CURSOR_NAME	= DBLITERAL_CORRELATION_NAME + 1,
	DBLITERAL_ESCAPE_PERCENT	= DBLITERAL_CURSOR_NAME + 1,
	DBLITERAL_ESCAPE_UNDERSCORE	= DBLITERAL_ESCAPE_PERCENT + 1,
	DBLITERAL_INDEX_NAME	= DBLITERAL_ESCAPE_UNDERSCORE + 1,
	DBLITERAL_LIKE_PERCENT	= DBLITERAL_INDEX_NAME + 1,
	DBLITERAL_LIKE_UNDERSCORE	= DBLITERAL_LIKE_PERCENT + 1,
	DBLITERAL_PROCEDURE_NAME	= DBLITERAL_LIKE_UNDERSCORE + 1,
	DBLITERAL_QUOTE	= DBLITERAL_PROCEDURE_NAME + 1,
	DBLITERAL_SCHEMA_NAME	= DBLITERAL_QUOTE + 1,
	DBLITERAL_TABLE_NAME	= DBLITERAL_SCHEMA_NAME + 1,
	DBLITERAL_TEXT_COMMAND	= DBLITERAL_TABLE_NAME + 1,
	DBLITERAL_USER_NAME	= DBLITERAL_TEXT_COMMAND + 1,
	DBLITERAL_VIEW_NAME	= DBLITERAL_USER_NAME + 1
    };
typedef struct  tagDBLITERALINFO
    {
    LPOLESTR pwszLiteralValue;
    LPOLESTR pwszInvalidChars;
    LPOLESTR pwszInvalidStartingChars;
    DBLITERAL lt;
    BOOL fSupported;
    ULONG cchMaxLen;
    }	DBLITERALINFO;


EXTERN_C const IID IID_IDBInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetKeywords( 
            /* [out] */ LPOLESTR __RPC_FAR *ppwszKeywords) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLiteralInfo( 
            /* [in] */ ULONG cLiterals,
            /* [size_is][in] */ const DBLITERAL __RPC_FAR rgLiterals[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcLiteralInfo,
            /* [size_is][size_is][out] */ DBLITERALINFO __RPC_FAR *__RPC_FAR *prgLiteralInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppCharBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKeywords )( 
            IDBInfo __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppwszKeywords);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLiteralInfo )( 
            IDBInfo __RPC_FAR * This,
            /* [in] */ ULONG cLiterals,
            /* [size_is][in] */ const DBLITERAL __RPC_FAR rgLiterals[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcLiteralInfo,
            /* [size_is][size_is][out] */ DBLITERALINFO __RPC_FAR *__RPC_FAR *prgLiteralInfo,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppCharBuffer);
        
        END_INTERFACE
    } IDBInfoVtbl;

    interface IDBInfo
    {
        CONST_VTBL struct IDBInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBInfo_GetKeywords(This,ppwszKeywords)	\
    (This)->lpVtbl -> GetKeywords(This,ppwszKeywords)

#define IDBInfo_GetLiteralInfo(This,cLiterals,rgLiterals,pcLiteralInfo,prgLiteralInfo,ppCharBuffer)	\
    (This)->lpVtbl -> GetLiteralInfo(This,cLiterals,rgLiterals,pcLiteralInfo,prgLiteralInfo,ppCharBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBInfo_GetKeywords_Proxy( 
    IDBInfo __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppwszKeywords);


void __RPC_STUB IDBInfo_GetKeywords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBInfo_GetLiteralInfo_Proxy( 
    IDBInfo __RPC_FAR * This,
    /* [in] */ ULONG cLiterals,
    /* [size_is][in] */ const DBLITERAL __RPC_FAR rgLiterals[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcLiteralInfo,
    /* [size_is][size_is][out] */ DBLITERALINFO __RPC_FAR *__RPC_FAR *prgLiteralInfo,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppCharBuffer);


void __RPC_STUB IDBInfo_GetLiteralInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBInfo_INTERFACE_DEFINED__ */


#ifndef __IDBDataSourceAdmin_INTERFACE_DEFINED__
#define __IDBDataSourceAdmin_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBDataSourceAdmin
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBDataSourceAdmin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBDataSourceAdmin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateDataSource( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyDataSource( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCreationProperties( 
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
            /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *rgPropertyInfoSets,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModifyDataSource( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBDataSourceAdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBDataSourceAdmin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBDataSourceAdmin __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBDataSourceAdmin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateDataSource )( 
            IDBDataSourceAdmin __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyDataSource )( 
            IDBDataSourceAdmin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCreationProperties )( 
            IDBDataSourceAdmin __RPC_FAR * This,
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
            /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *rgPropertyInfoSets,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyDataSource )( 
            IDBDataSourceAdmin __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);
        
        END_INTERFACE
    } IDBDataSourceAdminVtbl;

    interface IDBDataSourceAdmin
    {
        CONST_VTBL struct IDBDataSourceAdminVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBDataSourceAdmin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBDataSourceAdmin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBDataSourceAdmin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBDataSourceAdmin_CreateDataSource(This,cPropertySets,rgPropertySets,pUnkOuter,riid,ppDBSession)	\
    (This)->lpVtbl -> CreateDataSource(This,cPropertySets,rgPropertySets,pUnkOuter,riid,ppDBSession)

#define IDBDataSourceAdmin_DestroyDataSource(This)	\
    (This)->lpVtbl -> DestroyDataSource(This)

#define IDBDataSourceAdmin_GetCreationProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertyInfoSets,rgPropertyInfoSets,ppDescBuffer)	\
    (This)->lpVtbl -> GetCreationProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertyInfoSets,rgPropertyInfoSets,ppDescBuffer)

#define IDBDataSourceAdmin_ModifyDataSource(This,cPropertySets,rgPropertySets)	\
    (This)->lpVtbl -> ModifyDataSource(This,cPropertySets,rgPropertySets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBDataSourceAdmin_CreateDataSource_Proxy( 
    IDBDataSourceAdmin __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDBSession);


void __RPC_STUB IDBDataSourceAdmin_CreateDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBDataSourceAdmin_DestroyDataSource_Proxy( 
    IDBDataSourceAdmin __RPC_FAR * This);


void __RPC_STUB IDBDataSourceAdmin_DestroyDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBDataSourceAdmin_GetCreationProperties_Proxy( 
    IDBDataSourceAdmin __RPC_FAR * This,
    /* [in] */ ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
    /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *rgPropertyInfoSets,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer);


void __RPC_STUB IDBDataSourceAdmin_GetCreationProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBDataSourceAdmin_ModifyDataSource_Proxy( 
    IDBDataSourceAdmin __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);


void __RPC_STUB IDBDataSourceAdmin_ModifyDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBDataSourceAdmin_INTERFACE_DEFINED__ */


#ifndef __ISessionProperties_INTERFACE_DEFINED__
#define __ISessionProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISessionProperties
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ISessionProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISessionProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperties( 
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperties( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISessionPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISessionProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISessionProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISessionProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperties )( 
            ISessionProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertyIDSets,
            /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
            /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
            /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperties )( 
            ISessionProperties __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);
        
        END_INTERFACE
    } ISessionPropertiesVtbl;

    interface ISessionProperties
    {
        CONST_VTBL struct ISessionPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISessionProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISessionProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISessionProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISessionProperties_GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)	\
    (This)->lpVtbl -> GetProperties(This,cPropertyIDSets,rgPropertyIDSets,pcPropertySets,prgPropertySets)

#define ISessionProperties_SetProperties(This,cPropertySets,rgPropertySets)	\
    (This)->lpVtbl -> SetProperties(This,cPropertySets,rgPropertySets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISessionProperties_GetProperties_Proxy( 
    ISessionProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertyIDSets,
    /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
    /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
    /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets);


void __RPC_STUB ISessionProperties_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISessionProperties_SetProperties_Proxy( 
    ISessionProperties __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);


void __RPC_STUB ISessionProperties_SetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISessionProperties_INTERFACE_DEFINED__ */


#ifndef __IIndexDefinition_INTERFACE_DEFINED__
#define __IIndexDefinition_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IIndexDefinition
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IIndexDefinition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IIndexDefinition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateIndex( 
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID,
            /* [in] */ ULONG cIndexColumnDescs,
            /* [size_is][in] */ const DBINDEXCOLUMNDESC __RPC_FAR rgIndexColumnDescs[  ],
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppIndexID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DropIndex( 
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIndexDefinitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIndexDefinition __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIndexDefinition __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIndexDefinition __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateIndex )( 
            IIndexDefinition __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID,
            /* [in] */ ULONG cIndexColumnDescs,
            /* [size_is][in] */ const DBINDEXCOLUMNDESC __RPC_FAR rgIndexColumnDescs[  ],
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppIndexID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DropIndex )( 
            IIndexDefinition __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID);
        
        END_INTERFACE
    } IIndexDefinitionVtbl;

    interface IIndexDefinition
    {
        CONST_VTBL struct IIndexDefinitionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIndexDefinition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIndexDefinition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIndexDefinition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIndexDefinition_CreateIndex(This,pTableID,pIndexID,cIndexColumnDescs,rgIndexColumnDescs,cPropertySets,rgPropertySets,ppIndexID)	\
    (This)->lpVtbl -> CreateIndex(This,pTableID,pIndexID,cIndexColumnDescs,rgIndexColumnDescs,cPropertySets,rgPropertySets,ppIndexID)

#define IIndexDefinition_DropIndex(This,pTableID,pIndexID)	\
    (This)->lpVtbl -> DropIndex(This,pTableID,pIndexID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIndexDefinition_CreateIndex_Proxy( 
    IIndexDefinition __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ DBID __RPC_FAR *pIndexID,
    /* [in] */ ULONG cIndexColumnDescs,
    /* [size_is][in] */ const DBINDEXCOLUMNDESC __RPC_FAR rgIndexColumnDescs[  ],
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppIndexID);


void __RPC_STUB IIndexDefinition_CreateIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIndexDefinition_DropIndex_Proxy( 
    IIndexDefinition __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ DBID __RPC_FAR *pIndexID);


void __RPC_STUB IIndexDefinition_DropIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIndexDefinition_INTERFACE_DEFINED__ */


#ifndef __ITableDefinition_INTERFACE_DEFINED__
#define __ITableDefinition_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITableDefinition
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITableDefinition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITableDefinition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTable( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ ULONG cColumnDescs,
            /* [size_is][in] */ const DBCOLUMNDESC __RPC_FAR rgColumnDescs[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppTableID,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DropTable( 
            /* [in] */ DBID __RPC_FAR *pTableID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddColumn( 
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBCOLUMNDESC __RPC_FAR *pColumnDesc,
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppColumnID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DropColumn( 
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pColumnID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITableDefinitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITableDefinition __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITableDefinition __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITableDefinition __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTable )( 
            ITableDefinition __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ ULONG cColumnDescs,
            /* [size_is][in] */ const DBCOLUMNDESC __RPC_FAR rgColumnDescs[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppTableID,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DropTable )( 
            ITableDefinition __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pTableID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddColumn )( 
            ITableDefinition __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBCOLUMNDESC __RPC_FAR *pColumnDesc,
            /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppColumnID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DropColumn )( 
            ITableDefinition __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pColumnID);
        
        END_INTERFACE
    } ITableDefinitionVtbl;

    interface ITableDefinition
    {
        CONST_VTBL struct ITableDefinitionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITableDefinition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITableDefinition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITableDefinition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITableDefinition_CreateTable(This,pUnkOuter,pTableID,cColumnDescs,rgColumnDescs,riid,cPropertySets,rgPropertySets,ppTableID,ppRowset)	\
    (This)->lpVtbl -> CreateTable(This,pUnkOuter,pTableID,cColumnDescs,rgColumnDescs,riid,cPropertySets,rgPropertySets,ppTableID,ppRowset)

#define ITableDefinition_DropTable(This,pTableID)	\
    (This)->lpVtbl -> DropTable(This,pTableID)

#define ITableDefinition_AddColumn(This,pTableID,pColumnDesc,ppColumnID)	\
    (This)->lpVtbl -> AddColumn(This,pTableID,pColumnDesc,ppColumnID)

#define ITableDefinition_DropColumn(This,pTableID,pColumnID)	\
    (This)->lpVtbl -> DropColumn(This,pTableID,pColumnID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITableDefinition_CreateTable_Proxy( 
    ITableDefinition __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ ULONG cColumnDescs,
    /* [size_is][in] */ const DBCOLUMNDESC __RPC_FAR rgColumnDescs[  ],
    /* [in] */ REFIID riid,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppTableID,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);


void __RPC_STUB ITableDefinition_CreateTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITableDefinition_DropTable_Proxy( 
    ITableDefinition __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pTableID);


void __RPC_STUB ITableDefinition_DropTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITableDefinition_AddColumn_Proxy( 
    ITableDefinition __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ DBCOLUMNDESC __RPC_FAR *pColumnDesc,
    /* [out] */ DBID __RPC_FAR *__RPC_FAR *ppColumnID);


void __RPC_STUB ITableDefinition_AddColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITableDefinition_DropColumn_Proxy( 
    ITableDefinition __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ DBID __RPC_FAR *pColumnID);


void __RPC_STUB ITableDefinition_DropColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITableDefinition_INTERFACE_DEFINED__ */


#ifndef __IOpenRowset_INTERFACE_DEFINED__
#define __IOpenRowset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOpenRowset
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IOpenRowset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOpenRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenRowset( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID,
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOpenRowsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOpenRowset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOpenRowset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOpenRowset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenRowset )( 
            IOpenRowset __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ DBID __RPC_FAR *pTableID,
            /* [in] */ DBID __RPC_FAR *pIndexID,
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        END_INTERFACE
    } IOpenRowsetVtbl;

    interface IOpenRowset
    {
        CONST_VTBL struct IOpenRowsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOpenRowset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOpenRowset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOpenRowset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOpenRowset_OpenRowset(This,pUnkOuter,pTableID,pIndexID,riid,cPropertySets,rgPropertySets,ppRowset)	\
    (This)->lpVtbl -> OpenRowset(This,pUnkOuter,pTableID,pIndexID,riid,cPropertySets,rgPropertySets,ppRowset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOpenRowset_OpenRowset_Proxy( 
    IOpenRowset __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ DBID __RPC_FAR *pTableID,
    /* [in] */ DBID __RPC_FAR *pIndexID,
    /* [in] */ REFIID riid,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);


void __RPC_STUB IOpenRowset_OpenRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOpenRowset_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0116
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 




extern RPC_IF_HANDLE __MIDL__intf_0119_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0119_v0_0_s_ifspec;

#ifndef __IDBSchemaRowset_INTERFACE_DEFINED__
#define __IDBSchemaRowset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBSchemaRowset
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


#define CRESTRICTIONS_DBSCHEMA_ASSERTIONS                      3
#define CRESTRICTIONS_DBSCHEMA_CATALOGS                        1
#define CRESTRICTIONS_DBSCHEMA_CHARACTER_SETS                  3
#define CRESTRICTIONS_DBSCHEMA_COLLATIONS                      3
#define CRESTRICTIONS_DBSCHEMA_COLUMNS                         4
#define CRESTRICTIONS_DBSCHEMA_CHECK_CONSTRAINTS               3
#define CRESTRICTIONS_DBSCHEMA_CONSTRAINT_COLUMN_USAGE         4
#define CRESTRICTIONS_DBSCHEMA_CONSTRAINT_TABLE_USAGE          3
#define CRESTRICTIONS_DBSCHEMA_KEY_COLUMN_USAGE                7
#define CRESTRICTIONS_DBSCHEMA_REFERENTIAL_CONSTRAINTS         3
#define CRESTRICTIONS_DBSCHEMA_TABLE_CONSTRAINTS               7
#define CRESTRICTIONS_DBSCHEMA_COLUMN_DOMAIN_USAGE             4
#define CRESTRICTIONS_DBSCHEMA_INDEXES                         5
#define CRESTRICTIONS_DBSCHEMA_OBJECT_ACTIONS                  1
#define CRESTRICTIONS_DBSCHEMA_OBJECTS                         1
#define CRESTRICTIONS_DBSCHEMA_COLUMN_PRIVILEGES               6
#define CRESTRICTIONS_DBSCHEMA_TABLE_PRIVILEGES                5
#define CRESTRICTIONS_DBSCHEMA_USAGE_PRIVILEGES                6
#define CRESTRICTIONS_DBSCHEMA_PROCEDURES                      4
#define CRESTRICTIONS_DBSCHEMA_SCHEMATA                        3
#define CRESTRICTIONS_DBSCHEMA_SQL_LANGUAGES                   0
#define CRESTRICTIONS_DBSCHEMA_STATISTICS                      3
#define CRESTRICTIONS_DBSCHEMA_TABLES                          4
#define CRESTRICTIONS_DBSCHEMA_TRANSLATIONS                    3
#define CRESTRICTIONS_DBSCHEMA_PROVIDER_TYPES                  2
#define CRESTRICTIONS_DBSCHEMA_VIEWS                           3
#define CRESTRICTIONS_DBSCHEMA_VIEW_COLUMN_USAGE               3
#define CRESTRICTIONS_DBSCHEMA_VIEW_TABLE_USAGE                3
#define CRESTRICTIONS_DBSCHEMA_PROCEDURE_PARAMETERS            4
#define CRESTRICTIONS_DBSCHEMA_FOREIGN_KEYS                    6
#define CRESTRICTIONS_DBSCHEMA_PRIMARY_KEYS                    3
#define CRESTRICTIONS_DBSCHEMA_PROCEDURE_COLUMNS               4

EXTERN_C const IID IID_IDBSchemaRowset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDBSchemaRowset : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRowset( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFGUID rguidSchema,
            /* [in] */ ULONG cRestrictions,
            /* [size_is][in] */ const VARIANT __RPC_FAR rgRestrictions[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSchemas( 
            /* [out][in] */ ULONG __RPC_FAR *pcSchemas,
            /* [size_is][size_is][out] */ GUID __RPC_FAR *__RPC_FAR *prgSchemas,
            /* [size_is][size_is][out] */ ULONG __RPC_FAR *__RPC_FAR *prgRestrictionSupport) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBSchemaRowsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBSchemaRowset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBSchemaRowset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBSchemaRowset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowset )( 
            IDBSchemaRowset __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFGUID rguidSchema,
            /* [in] */ ULONG cRestrictions,
            /* [size_is][in] */ const VARIANT __RPC_FAR rgRestrictions[  ],
            /* [in] */ REFIID riid,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSchemas )( 
            IDBSchemaRowset __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *pcSchemas,
            /* [size_is][size_is][out] */ GUID __RPC_FAR *__RPC_FAR *prgSchemas,
            /* [size_is][size_is][out] */ ULONG __RPC_FAR *__RPC_FAR *prgRestrictionSupport);
        
        END_INTERFACE
    } IDBSchemaRowsetVtbl;

    interface IDBSchemaRowset
    {
        CONST_VTBL struct IDBSchemaRowsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBSchemaRowset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBSchemaRowset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBSchemaRowset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBSchemaRowset_GetRowset(This,pUnkOuter,rguidSchema,cRestrictions,rgRestrictions,riid,cPropertySets,rgPropertySets,ppRowset)	\
    (This)->lpVtbl -> GetRowset(This,pUnkOuter,rguidSchema,cRestrictions,rgRestrictions,riid,cPropertySets,rgPropertySets,ppRowset)

#define IDBSchemaRowset_GetSchemas(This,pcSchemas,prgSchemas,prgRestrictionSupport)	\
    (This)->lpVtbl -> GetSchemas(This,pcSchemas,prgSchemas,prgRestrictionSupport)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBSchemaRowset_GetRowset_Proxy( 
    IDBSchemaRowset __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFGUID rguidSchema,
    /* [in] */ ULONG cRestrictions,
    /* [size_is][in] */ const VARIANT __RPC_FAR rgRestrictions[  ],
    /* [in] */ REFIID riid,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ],
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);


void __RPC_STUB IDBSchemaRowset_GetRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBSchemaRowset_GetSchemas_Proxy( 
    IDBSchemaRowset __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *pcSchemas,
    /* [size_is][size_is][out] */ GUID __RPC_FAR *__RPC_FAR *prgSchemas,
    /* [size_is][size_is][out] */ ULONG __RPC_FAR *__RPC_FAR *prgRestrictionSupport);


void __RPC_STUB IDBSchemaRowset_GetSchemas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBSchemaRowset_INTERFACE_DEFINED__ */


#ifndef __IProvideMoniker_INTERFACE_DEFINED__
#define __IProvideMoniker_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideMoniker
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IProvideMoniker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IProvideMoniker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMoniker( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppIMoniker) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideMonikerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProvideMoniker __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProvideMoniker __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProvideMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMoniker )( 
            IProvideMoniker __RPC_FAR * This,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppIMoniker);
        
        END_INTERFACE
    } IProvideMonikerVtbl;

    interface IProvideMoniker
    {
        CONST_VTBL struct IProvideMonikerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideMoniker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideMoniker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideMoniker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideMoniker_GetMoniker(This,ppIMoniker)	\
    (This)->lpVtbl -> GetMoniker(This,ppIMoniker)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IProvideMoniker_GetMoniker_Proxy( 
    IProvideMoniker __RPC_FAR * This,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppIMoniker);


void __RPC_STUB IProvideMoniker_GetMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideMoniker_INTERFACE_DEFINED__ */


#ifndef __IErrorRecords_INTERFACE_DEFINED__
#define __IErrorRecords_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IErrorRecords
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 


#define IDENTIFIER_SDK_MASK	0xF0000000
#define IDENTIFIER_SDK_ERROR	0x10000000
typedef struct  tagERRORINFO
    {
    HRESULT hrError;
    DWORD dwMinor;
    CLSID clsid;
    IID iid;
    DISPID dispid;
    }	ERRORINFO;


EXTERN_C const IID IID_IErrorRecords;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IErrorRecords : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddErrorRecord( 
            /* [in] */ ERRORINFO __RPC_FAR *pErrorInfo,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [in] */ IUnknown __RPC_FAR *punkCustomError,
            /* [in] */ DWORD dwDynamicErrorID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBasicErrorInfo( 
            /* [in] */ ULONG ulRecordNum,
            /* [out] */ ERRORINFO __RPC_FAR *pErrorInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCustomErrorObject( 
            /* [in] */ ULONG ulRecordNum,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
            /* [in] */ ULONG ulRecordNum,
            /* [in] */ LCID lcid,
            /* [out] */ IErrorInfo __RPC_FAR *__RPC_FAR *ppErrorInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorParameters( 
            /* [in] */ ULONG ulRecordNum,
            /* [out] */ DISPPARAMS __RPC_FAR *pdispparams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecordCount( 
            /* [out] */ ULONG __RPC_FAR *pcRecords) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IErrorRecordsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IErrorRecords __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IErrorRecords __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddErrorRecord )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ ERRORINFO __RPC_FAR *pErrorInfo,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [in] */ IUnknown __RPC_FAR *punkCustomError,
            /* [in] */ DWORD dwDynamicErrorID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBasicErrorInfo )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ ULONG ulRecordNum,
            /* [out] */ ERRORINFO __RPC_FAR *pErrorInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCustomErrorObject )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ ULONG ulRecordNum,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorInfo )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ ULONG ulRecordNum,
            /* [in] */ LCID lcid,
            /* [out] */ IErrorInfo __RPC_FAR *__RPC_FAR *ppErrorInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorParameters )( 
            IErrorRecords __RPC_FAR * This,
            /* [in] */ ULONG ulRecordNum,
            /* [out] */ DISPPARAMS __RPC_FAR *pdispparams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRecordCount )( 
            IErrorRecords __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcRecords);
        
        END_INTERFACE
    } IErrorRecordsVtbl;

    interface IErrorRecords
    {
        CONST_VTBL struct IErrorRecordsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IErrorRecords_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IErrorRecords_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IErrorRecords_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IErrorRecords_AddErrorRecord(This,pErrorInfo,dwLookupID,pdispparams,punkCustomError,dwDynamicErrorID)	\
    (This)->lpVtbl -> AddErrorRecord(This,pErrorInfo,dwLookupID,pdispparams,punkCustomError,dwDynamicErrorID)

#define IErrorRecords_GetBasicErrorInfo(This,ulRecordNum,pErrorInfo)	\
    (This)->lpVtbl -> GetBasicErrorInfo(This,ulRecordNum,pErrorInfo)

#define IErrorRecords_GetCustomErrorObject(This,ulRecordNum,riid,ppObject)	\
    (This)->lpVtbl -> GetCustomErrorObject(This,ulRecordNum,riid,ppObject)

#define IErrorRecords_GetErrorInfo(This,ulRecordNum,lcid,ppErrorInfo)	\
    (This)->lpVtbl -> GetErrorInfo(This,ulRecordNum,lcid,ppErrorInfo)

#define IErrorRecords_GetErrorParameters(This,ulRecordNum,pdispparams)	\
    (This)->lpVtbl -> GetErrorParameters(This,ulRecordNum,pdispparams)

#define IErrorRecords_GetRecordCount(This,pcRecords)	\
    (This)->lpVtbl -> GetRecordCount(This,pcRecords)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IErrorRecords_AddErrorRecord_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [in] */ ERRORINFO __RPC_FAR *pErrorInfo,
    /* [in] */ DWORD dwLookupID,
    /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
    /* [in] */ IUnknown __RPC_FAR *punkCustomError,
    /* [in] */ DWORD dwDynamicErrorID);


void __RPC_STUB IErrorRecords_AddErrorRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorRecords_GetBasicErrorInfo_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [in] */ ULONG ulRecordNum,
    /* [out] */ ERRORINFO __RPC_FAR *pErrorInfo);


void __RPC_STUB IErrorRecords_GetBasicErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorRecords_GetCustomErrorObject_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [in] */ ULONG ulRecordNum,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppObject);


void __RPC_STUB IErrorRecords_GetCustomErrorObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorRecords_GetErrorInfo_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [in] */ ULONG ulRecordNum,
    /* [in] */ LCID lcid,
    /* [out] */ IErrorInfo __RPC_FAR *__RPC_FAR *ppErrorInfo);


void __RPC_STUB IErrorRecords_GetErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorRecords_GetErrorParameters_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [in] */ ULONG ulRecordNum,
    /* [out] */ DISPPARAMS __RPC_FAR *pdispparams);


void __RPC_STUB IErrorRecords_GetErrorParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorRecords_GetRecordCount_Proxy( 
    IErrorRecords __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcRecords);


void __RPC_STUB IErrorRecords_GetRecordCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IErrorRecords_INTERFACE_DEFINED__ */


#ifndef __IErrorLookup_INTERFACE_DEFINED__
#define __IErrorLookup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IErrorLookup
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IErrorLookup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IErrorLookup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrSource,
            /* [out] */ BSTR __RPC_FAR *pbstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelpInfo( 
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpFile,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseErrors( 
            /* [in] */ const DWORD dwDynamicErrorID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IErrorLookupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IErrorLookup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IErrorLookup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IErrorLookup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorDescription )( 
            IErrorLookup __RPC_FAR * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrSource,
            /* [out] */ BSTR __RPC_FAR *pbstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHelpInfo )( 
            IErrorLookup __RPC_FAR * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD dwLookupID,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpFile,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseErrors )( 
            IErrorLookup __RPC_FAR * This,
            /* [in] */ const DWORD dwDynamicErrorID);
        
        END_INTERFACE
    } IErrorLookupVtbl;

    interface IErrorLookup
    {
        CONST_VTBL struct IErrorLookupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IErrorLookup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IErrorLookup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IErrorLookup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IErrorLookup_GetErrorDescription(This,hrError,dwLookupID,pdispparams,lcid,pbstrSource,pbstrDescription)	\
    (This)->lpVtbl -> GetErrorDescription(This,hrError,dwLookupID,pdispparams,lcid,pbstrSource,pbstrDescription)

#define IErrorLookup_GetHelpInfo(This,hrError,dwLookupID,lcid,pbstrHelpFile,pdwHelpContext)	\
    (This)->lpVtbl -> GetHelpInfo(This,hrError,dwLookupID,lcid,pbstrHelpFile,pdwHelpContext)

#define IErrorLookup_ReleaseErrors(This,dwDynamicErrorID)	\
    (This)->lpVtbl -> ReleaseErrors(This,dwDynamicErrorID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IErrorLookup_GetErrorDescription_Proxy( 
    IErrorLookup __RPC_FAR * This,
    /* [in] */ HRESULT hrError,
    /* [in] */ DWORD dwLookupID,
    /* [in] */ DISPPARAMS __RPC_FAR *pdispparams,
    /* [in] */ LCID lcid,
    /* [out] */ BSTR __RPC_FAR *pbstrSource,
    /* [out] */ BSTR __RPC_FAR *pbstrDescription);


void __RPC_STUB IErrorLookup_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorLookup_GetHelpInfo_Proxy( 
    IErrorLookup __RPC_FAR * This,
    /* [in] */ HRESULT hrError,
    /* [in] */ DWORD dwLookupID,
    /* [in] */ LCID lcid,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpFile,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext);


void __RPC_STUB IErrorLookup_GetHelpInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorLookup_ReleaseErrors_Proxy( 
    IErrorLookup __RPC_FAR * This,
    /* [in] */ const DWORD dwDynamicErrorID);


void __RPC_STUB IErrorLookup_ReleaseErrors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IErrorLookup_INTERFACE_DEFINED__ */


#ifndef __ISQLErrorInfo_INTERFACE_DEFINED__
#define __ISQLErrorInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISQLErrorInfo
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ISQLErrorInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISQLErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSQLInfo( 
            /* [out] */ BSTR __RPC_FAR *pbstrSQLState,
            /* [out] */ LONG __RPC_FAR *plNativeError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISQLErrorInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISQLErrorInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISQLErrorInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISQLErrorInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSQLInfo )( 
            ISQLErrorInfo __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrSQLState,
            /* [out] */ LONG __RPC_FAR *plNativeError);
        
        END_INTERFACE
    } ISQLErrorInfoVtbl;

    interface ISQLErrorInfo
    {
        CONST_VTBL struct ISQLErrorInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISQLErrorInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISQLErrorInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISQLErrorInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISQLErrorInfo_GetSQLInfo(This,pbstrSQLState,plNativeError)	\
    (This)->lpVtbl -> GetSQLInfo(This,pbstrSQLState,plNativeError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISQLErrorInfo_GetSQLInfo_Proxy( 
    ISQLErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrSQLState,
    /* [out] */ LONG __RPC_FAR *plNativeError);


void __RPC_STUB ISQLErrorInfo_GetSQLInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISQLErrorInfo_INTERFACE_DEFINED__ */


#ifndef __IGetDataSource_INTERFACE_DEFINED__
#define __IGetDataSource_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetDataSource
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IGetDataSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGetDataSource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDataSource( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetDataSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetDataSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetDataSource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetDataSource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataSource )( 
            IGetDataSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);
        
        END_INTERFACE
    } IGetDataSourceVtbl;

    interface IGetDataSource
    {
        CONST_VTBL struct IGetDataSourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetDataSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetDataSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetDataSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetDataSource_GetDataSource(This,riid,ppDataSource)	\
    (This)->lpVtbl -> GetDataSource(This,riid,ppDataSource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGetDataSource_GetDataSource_Proxy( 
    IGetDataSource __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppDataSource);


void __RPC_STUB IGetDataSource_GetDataSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetDataSource_INTERFACE_DEFINED__ */


#ifndef __ITransactionLocal_INTERFACE_DEFINED__
#define __ITransactionLocal_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionLocal
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ITransactionLocal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionLocal : public ITransaction
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOptionsObject( 
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartTransaction( 
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions,
            /* [out] */ ULONG __RPC_FAR *pulTransactionLevel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionLocalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITransactionLocal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITransactionLocal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITransactionLocal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            ITransactionLocal __RPC_FAR * This,
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            ITransactionLocal __RPC_FAR * This,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransactionInfo )( 
            ITransactionLocal __RPC_FAR * This,
            /* [out] */ XACTTRANSINFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOptionsObject )( 
            ITransactionLocal __RPC_FAR * This,
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartTransaction )( 
            ITransactionLocal __RPC_FAR * This,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions,
            /* [out] */ ULONG __RPC_FAR *pulTransactionLevel);
        
        END_INTERFACE
    } ITransactionLocalVtbl;

    interface ITransactionLocal
    {
        CONST_VTBL struct ITransactionLocalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionLocal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionLocal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionLocal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionLocal_Commit(This,fRetaining,grfTC,grfRM)	\
    (This)->lpVtbl -> Commit(This,fRetaining,grfTC,grfRM)

#define ITransactionLocal_Abort(This,pboidReason,fRetaining,fAsync)	\
    (This)->lpVtbl -> Abort(This,pboidReason,fRetaining,fAsync)

#define ITransactionLocal_GetTransactionInfo(This,pinfo)	\
    (This)->lpVtbl -> GetTransactionInfo(This,pinfo)


#define ITransactionLocal_GetOptionsObject(This,ppOptions)	\
    (This)->lpVtbl -> GetOptionsObject(This,ppOptions)

#define ITransactionLocal_StartTransaction(This,isoLevel,isoFlags,pOtherOptions,pulTransactionLevel)	\
    (This)->lpVtbl -> StartTransaction(This,isoLevel,isoFlags,pOtherOptions,pulTransactionLevel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionLocal_GetOptionsObject_Proxy( 
    ITransactionLocal __RPC_FAR * This,
    /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);


void __RPC_STUB ITransactionLocal_GetOptionsObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionLocal_StartTransaction_Proxy( 
    ITransactionLocal __RPC_FAR * This,
    /* [in] */ ISOLEVEL isoLevel,
    /* [in] */ ULONG isoFlags,
    /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions,
    /* [out] */ ULONG __RPC_FAR *pulTransactionLevel);


void __RPC_STUB ITransactionLocal_StartTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionLocal_INTERFACE_DEFINED__ */


#ifndef __ITransactionJoin_INTERFACE_DEFINED__
#define __ITransactionJoin_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionJoin
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionJoin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionJoin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOptionsObject( 
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JoinTransaction( 
            /* [in] */ IUnknown __RPC_FAR *punkTransactionCoord,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionJoinVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITransactionJoin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITransactionJoin __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITransactionJoin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOptionsObject )( 
            ITransactionJoin __RPC_FAR * This,
            /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *JoinTransaction )( 
            ITransactionJoin __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *punkTransactionCoord,
            /* [in] */ ISOLEVEL isoLevel,
            /* [in] */ ULONG isoFlags,
            /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions);
        
        END_INTERFACE
    } ITransactionJoinVtbl;

    interface ITransactionJoin
    {
        CONST_VTBL struct ITransactionJoinVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionJoin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionJoin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionJoin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionJoin_GetOptionsObject(This,ppOptions)	\
    (This)->lpVtbl -> GetOptionsObject(This,ppOptions)

#define ITransactionJoin_JoinTransaction(This,punkTransactionCoord,isoLevel,isoFlags,pOtherOptions)	\
    (This)->lpVtbl -> JoinTransaction(This,punkTransactionCoord,isoLevel,isoFlags,pOtherOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionJoin_GetOptionsObject_Proxy( 
    ITransactionJoin __RPC_FAR * This,
    /* [out] */ ITransactionOptions __RPC_FAR *__RPC_FAR *ppOptions);


void __RPC_STUB ITransactionJoin_GetOptionsObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransactionJoin_JoinTransaction_Proxy( 
    ITransactionJoin __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *punkTransactionCoord,
    /* [in] */ ISOLEVEL isoLevel,
    /* [in] */ ULONG isoFlags,
    /* [in] */ ITransactionOptions __RPC_FAR *pOtherOptions);


void __RPC_STUB ITransactionJoin_JoinTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionJoin_INTERFACE_DEFINED__ */


#ifndef __ITransactionObject_INTERFACE_DEFINED__
#define __ITransactionObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionObject
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ITransactionObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITransactionObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTransactionObject( 
            /* [in] */ ULONG ulTransactionLevel,
            /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransactionObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITransactionObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITransactionObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITransactionObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransactionObject )( 
            ITransactionObject __RPC_FAR * This,
            /* [in] */ ULONG ulTransactionLevel,
            /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransactionObject);
        
        END_INTERFACE
    } ITransactionObjectVtbl;

    interface ITransactionObject
    {
        CONST_VTBL struct ITransactionObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionObject_GetTransactionObject(This,ulTransactionLevel,ppTransactionObject)	\
    (This)->lpVtbl -> GetTransactionObject(This,ulTransactionLevel,ppTransactionObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionObject_GetTransactionObject_Proxy( 
    ITransactionObject __RPC_FAR * This,
    /* [in] */ ULONG ulTransactionLevel,
    /* [out] */ ITransaction __RPC_FAR *__RPC_FAR *ppTransactionObject);


void __RPC_STUB ITransactionObject_GetTransactionObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionObject_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0136
 * at Tue Aug 20 16:27:55 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


#include <poppack.h>	// restore original structure packing


extern RPC_IF_HANDLE __MIDL__intf_0136_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0136_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  DISPPARAMS_UserSize(     unsigned long __RPC_FAR *, unsigned long            , DISPPARAMS __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  DISPPARAMS_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, DISPPARAMS __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  DISPPARAMS_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, DISPPARAMS __RPC_FAR * ); 
void                      __RPC_USER  DISPPARAMS_UserFree(     unsigned long __RPC_FAR *, DISPPARAMS __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
