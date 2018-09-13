/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    pstore.h

Abstract:

    This module defines the Protected Storage COM interface and associated
    data structures.

Author:

    Scott Field (sfield)    13-Mar-97

--*/

#ifndef __PSTORE_H__
#define __PSTORE_H__


/*
    Typedefs, values 
*/

// provider flags

// provider capabilities
typedef DWORD PST_PROVIDERCAPABILITIES;

#define PST_PC_PFX              0x00000001
#define PST_PC_HARDWARE         0x00000002
#define PST_PC_SMARTCARD        0x00000004
#define PST_PC_PCMCIA           0x00000008
#define PST_PC_MULTIPLE_REPOSITORIES    0x00000010
#define PST_PC_ROAMABLE         0x00000020


// NYI (not yet implemented)
typedef DWORD PST_REPOSITORYCAPABILITIES;

#define PST_RC_REMOVABLE        0x80000000


// provider storage area 
typedef DWORD PST_KEY;

#define PST_KEY_CURRENT_USER    0x00000000
#define PST_KEY_LOCAL_MACHINE   0x00000001



/* 
    dwDefaultConfirmationStyle flags
*/

//
// allows user to choose confirmation style
//
#define     PST_CF_DEFAULT              0x00000000

//
// forces silent item creation 
//
#define     PST_CF_NONE                 0x00000001



/*
    dwPromptFlags
*/

//
// app forces confirmation to be shown
//
#define     PST_PF_ALWAYS_SHOW          0x00000001


/* 
    dwFlags values
*/

//
// Allows caller to specify creation not overwrite 
// of item during WriteItem call
//
#define     PST_NO_OVERWRITE            0x00000002

//
// specifies insecure data stream to be written/read
// there is no protection or guarantees for this data
// flag only valid during item read/write calls
// default: item calls are secure
//
#define     PST_UNRESTRICTED_ITEMDATA   0x00000004



/* 
    Security masks, rule modifiers 
*/

//
// models access after NT access mask
//

// read, write
typedef DWORD PST_ACCESSMODE;

#define     PST_READ                0x0001
#define     PST_WRITE               0x0002


// PST_ACCESSCLAUSETYPE 

// memhash, diskhash, authenticode, etc
typedef DWORD PST_ACCESSCLAUSETYPE;

//
// pbClauseData points to PST_AUTHENTICODEDATA structure.
//
#define     PST_AUTHENTICODE            1

//
// pbClauseData points to PST_BINARYCHECKDATA structure.
//
#define     PST_BINARY_CHECK            2

//
// pbClauseData points to valid Windows NT security descriptor.
// note that performance is improved on Set operations if the security
// descriptor is in self-relative format, with valid owner and group Sids
// (non-NULL).
//
#define     PST_SECURITY_DESCRIPTOR     4

//
// pbClauseData is in self-relative format
// (for internal use only)
//
#define     PST_SELF_RELATIVE_CLAUSE    0x80000000L

//
// currently access clause modifiers - NOT to be or'd together
//


//
// specified image is the immediate caller, and is an application (.exe)
//

#define     PST_AC_SINGLE_CALLER        0

//
// specified image is not necessary the immediate caller, and is an
// application (.exe)
//

#define     PST_AC_TOP_LEVEL_CALLER     1

//
// specified image is the immediate caller.  May be
// an application (.exe) or a .dll
//

#define     PST_AC_IMMEDIATE_CALLER     2


/*
    Provider Parameters
*/
//
// flush the internal cache of passwords -- temporary?
//
#define     PST_PP_FLUSH_PW_CACHE       0x1




/*
    Provider Defns
*/

//
// Microsoft Base Provider (MS_BASE_PSTPROVIDER...)
//
#define MS_BASE_PSTPROVIDER_NAME            L"System Protected Storage"

// {8A078C30-3755-11d0-A0BD-00AA0061426A}
#define MS_BASE_PSTPROVIDER_ID              { 0x8a078c30, 0x3755, 0x11d0, { 0xa0, 0xbd, 0x0, 0xaa, 0x0, 0x61, 0x42, 0x6a } }
#define MS_BASE_PSTPROVIDER_SZID            L"8A078C30-3755-11d0-A0BD-00AA0061426A"

//
// Microsoft PFX Provider (MS_PFX_PSTPROVIDER...)
//
#define MS_PFX_PSTPROVIDER_NAME             L"PFX Storage Provider"

// {3ca94f30-7ac1-11d0-8c42-00c04fc299eb}
#define MS_PFX_PSTPROVIDER_ID               { 0x3ca94f30, 0x7ac1, 0x11d0, {0x8c, 0x42, 0x00, 0xc0, 0x4f, 0xc2, 0x99, 0xeb} }
#define MS_PFX_PSTPROVIDER_SZID             L"3ca94f30-7ac1-11d0-8c42-00c04fc299eb"



/*
    Globally registered Type/Subtype guid/name pairs
*/

#define PST_CONFIGDATA_TYPE_STRING              L"Configuration Data"
// 8ec99652-8909-11d0-8c4d-00c04fc297eb
#define PST_CONFIGDATA_TYPE_GUID                        \
{   0x8ec99652,                                         \
    0x8909,                                             \
    0x11d0,                                             \
    {0x8c, 0x4d, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0xeb}    \
}

#define PST_PROTECTEDSTORAGE_SUBTYPE_STRING     L"Protected Storage"
// d3121b8e-8a7d-11d0-8c4f-00c04fc297eb 
#define PST_PROTECTEDSTORAGE_SUBTYPE_GUID               \
{   0xd3121b8e,                                         \
    0x8a7d,                                             \
    0x11d0,                                             \
    {0x8c, 0x4f, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0xeb}    \
}


#define PST_PSTORE_PROVIDERS_SUBTYPE_STRING     L"Protected Storage Provider List"
// 8ed17a64-91d0-11d0-8c43-00c04fc2c621
#define PST_PSTORE_PROVIDERS_SUBTYPE_GUID               \
{                                                       \
    0x8ed17a64,                                         \
    0x91d0,                                             \
    0x11d0,                                             \
    {0x8c, 0x43, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0x21}    \
}


//
// error codes
//


#ifndef PST_E_OK
#define PST_E_OK                        _HRESULT_TYPEDEF_(0x00000000L)


#define PST_E_FAIL                      _HRESULT_TYPEDEF_(0x800C0001L)
#define PST_E_PROV_DLL_NOT_FOUND        _HRESULT_TYPEDEF_(0x800C0002L)
#define PST_E_INVALID_HANDLE            _HRESULT_TYPEDEF_(0x800C0003L)
#define PST_E_TYPE_EXISTS               _HRESULT_TYPEDEF_(0x800C0004L)
#define PST_E_TYPE_NO_EXISTS            _HRESULT_TYPEDEF_(0x800C0005L)
#define PST_E_INVALID_RULESET           _HRESULT_TYPEDEF_(0x800C0006L)
#define PST_E_NO_PERMISSIONS            _HRESULT_TYPEDEF_(0x800C0007L)
#define PST_E_STORAGE_ERROR             _HRESULT_TYPEDEF_(0x800C0008L)
#define PST_E_CALLER_NOT_VERIFIED       _HRESULT_TYPEDEF_(0x800C0009L)
#define PST_E_WRONG_PASSWORD            _HRESULT_TYPEDEF_(0x800C000AL)
#define PST_E_DISK_IMAGE_MISMATCH       _HRESULT_TYPEDEF_(0x800C000BL)
// 000C pending
#define PST_E_UNKNOWN_EXCEPTION         _HRESULT_TYPEDEF_(0x800C000DL)
#define PST_E_BAD_FLAGS                 _HRESULT_TYPEDEF_(0x800C000EL)
#define PST_E_ITEM_EXISTS               _HRESULT_TYPEDEF_(0x800C000FL)
#define PST_E_ITEM_NO_EXISTS            _HRESULT_TYPEDEF_(0x800C0010L)
#define PST_E_SERVICE_UNAVAILABLE       _HRESULT_TYPEDEF_(0x800C0011L)
#define PST_E_NOTEMPTY                  _HRESULT_TYPEDEF_(0x800C0012L)
#define PST_E_INVALID_STRING            _HRESULT_TYPEDEF_(0x800C0013L)
#define PST_E_STATE_INVALID             _HRESULT_TYPEDEF_(0x800C0014L)
#define PST_E_NOT_OPEN                  _HRESULT_TYPEDEF_(0x800C0015L)
#define PST_E_ALREADY_OPEN              _HRESULT_TYPEDEF_(0x800C0016L)
#define PST_E_NYI                       _HRESULT_TYPEDEF_(0x800C0F00L)


#define MIN_PST_ERROR                   0x800C0001
#define MAX_PST_ERROR                   0x800C0F00

#endif  // !PST_OK



/* this ALWAYS GENERATED file contains the definitions for the interfaces */

//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Wed Apr 23 23:56:10 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


typedef DWORD PST_PROVIDERCAPABILITIES;

typedef DWORD PST_REPOSITORYCAPABILITIES;

typedef DWORD PST_KEY;

typedef DWORD PST_ACCESSMODE;

typedef DWORD PST_ACCESSCLAUSETYPE;

typedef GUID UUID;

typedef ULARGE_INTEGER PST_PROVIDER_HANDLE;

typedef GUID PST_PROVIDERID;

typedef PST_PROVIDERID __RPC_FAR *PPST_PROVIDERID;

typedef struct  _PST_PROVIDERINFO
    {
    DWORD cbSize;
    PST_PROVIDERID ID;
    PST_PROVIDERCAPABILITIES Capabilities;
    LPWSTR szProviderName;
    }	PST_PROVIDERINFO;

typedef struct _PST_PROVIDERINFO __RPC_FAR *PPST_PROVIDERINFO;

typedef struct  _PST_TYPEINFO
    {
    DWORD cbSize;
    LPWSTR szDisplayName;
    }	PST_TYPEINFO;

typedef struct _PST_TYPEINFO __RPC_FAR *PPST_TYPEINFO;

typedef struct  _PST_PROMPTINFO
    {
    DWORD cbSize;
    DWORD dwPromptFlags;
    HWND  hwndApp;
    LPCWSTR szPrompt;
    }	PST_PROMPTINFO;

typedef struct _PST_PROMPTINFO __RPC_FAR *PPST_PROMPTINFO;

typedef struct  _PST_ACCESSCLAUSE
    {
    DWORD cbSize;
    PST_ACCESSCLAUSETYPE ClauseType;
    DWORD cbClauseData;
    /* [size_is] */ VOID __RPC_FAR *pbClauseData;
    }	PST_ACCESSCLAUSE;

typedef struct _PST_ACCESSCLAUSE __RPC_FAR *PPST_ACCESSCLAUSE;

typedef struct  _PST_ACCESSRULE
    {
    DWORD cbSize;
    PST_ACCESSMODE AccessModeFlags;
    DWORD cClauses;
    /* [size_is] */ PST_ACCESSCLAUSE __RPC_FAR *rgClauses;
    }	PST_ACCESSRULE;

typedef struct _PST_ACCESSRULE __RPC_FAR *PPST_ACCESSRULE;

typedef struct  _PST_ACCESSRULESET
    {
    DWORD cbSize;
    DWORD cRules;
    /* [size_is] */ PST_ACCESSRULE __RPC_FAR *rgRules;
    }	PST_ACCESSRULESET;

typedef struct _PST_ACCESSRULESET __RPC_FAR *PPST_ACCESSRULESET;

typedef struct  _PST_AUTHENTICODEDATA
    {
    DWORD cbSize;
    DWORD dwModifiers;
    LPCWSTR szRootCA;
    LPCWSTR szIssuer;
    LPCWSTR szPublisher;
    LPCWSTR szProgramName;
    }	PST_AUTHENTICODEDATA;

typedef struct _PST_AUTHENTICODEDATA __RPC_FAR *PPST_AUTHENTICODEDATA;

typedef struct _PST_AUTHENTICODEDATA __RPC_FAR *LPPST_AUTHENTICODEDATA;

typedef struct  _PST_BINARYCHECKDATA
    {
    DWORD cbSize;
    DWORD dwModifiers;
    LPCWSTR szFilePath;
    }	PST_BINARYCHECKDATA;

typedef struct _PST_BINARYCHECKDATA __RPC_FAR *PPST_BINARYCHECKDATA;

typedef struct _PST_BINARYCHECKDATA __RPC_FAR *LPPST_BINARYCHECKDATA;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif



/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Wed Apr 23 23:56:14 1997
 */
/* Compiler settings for pstorec.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEnumPStoreItems_FWD_DEFINED__
#define __IEnumPStoreItems_FWD_DEFINED__
typedef interface IEnumPStoreItems IEnumPStoreItems;
#endif 	/* __IEnumPStoreItems_FWD_DEFINED__ */


#ifndef __IEnumPStoreTypes_FWD_DEFINED__
#define __IEnumPStoreTypes_FWD_DEFINED__
typedef interface IEnumPStoreTypes IEnumPStoreTypes;
#endif 	/* __IEnumPStoreTypes_FWD_DEFINED__ */


#ifndef __IPStore_FWD_DEFINED__
#define __IPStore_FWD_DEFINED__
typedef interface IPStore IPStore;
#endif 	/* __IPStore_FWD_DEFINED__ */


#ifndef __IEnumPStoreProviders_FWD_DEFINED__
#define __IEnumPStoreProviders_FWD_DEFINED__
typedef interface IEnumPStoreProviders IEnumPStoreProviders;
#endif 	/* __IEnumPStoreProviders_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IEnumPStoreItems_INTERFACE_DEFINED__
#define __IEnumPStoreItems_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumPStoreItems
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IEnumPStoreItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumPStoreItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [out] */ LPWSTR __RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPStoreItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumPStoreItems __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumPStoreItems __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumPStoreItems __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumPStoreItems __RPC_FAR * This,
            /* [in] */ DWORD celt,
            /* [out] */ LPWSTR __RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumPStoreItems __RPC_FAR * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumPStoreItems __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumPStoreItems __RPC_FAR * This,
            /* [out] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumPStoreItemsVtbl;

    interface IEnumPStoreItems
    {
        CONST_VTBL struct IEnumPStoreItemsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPStoreItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPStoreItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPStoreItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPStoreItems_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumPStoreItems_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumPStoreItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumPStoreItems_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumPStoreItems_Next_Proxy( 
    IEnumPStoreItems __RPC_FAR * This,
    /* [in] */ DWORD celt,
    /* [out] */ LPWSTR __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumPStoreItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreItems_Skip_Proxy( 
    IEnumPStoreItems __RPC_FAR * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IEnumPStoreItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreItems_Reset_Proxy( 
    IEnumPStoreItems __RPC_FAR * This);


void __RPC_STUB IEnumPStoreItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreItems_Clone_Proxy( 
    IEnumPStoreItems __RPC_FAR * This,
    /* [out] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumPStoreItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPStoreItems_INTERFACE_DEFINED__ */


#ifndef __IEnumPStoreTypes_INTERFACE_DEFINED__
#define __IEnumPStoreTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumPStoreTypes
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IEnumPStoreTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumPStoreTypes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [out][size_is] */ GUID __RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPStoreTypesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumPStoreTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumPStoreTypes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumPStoreTypes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumPStoreTypes __RPC_FAR * This,
            /* [in] */ DWORD celt,
            /* [out][size_is] */ GUID __RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumPStoreTypes __RPC_FAR * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumPStoreTypes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumPStoreTypes __RPC_FAR * This,
            /* [out] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumPStoreTypesVtbl;

    interface IEnumPStoreTypes
    {
        CONST_VTBL struct IEnumPStoreTypesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPStoreTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPStoreTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPStoreTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPStoreTypes_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumPStoreTypes_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumPStoreTypes_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumPStoreTypes_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumPStoreTypes_Next_Proxy( 
    IEnumPStoreTypes __RPC_FAR * This,
    /* [in] */ DWORD celt,
    /* [out][size_is] */ GUID __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumPStoreTypes_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreTypes_Skip_Proxy( 
    IEnumPStoreTypes __RPC_FAR * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IEnumPStoreTypes_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreTypes_Reset_Proxy( 
    IEnumPStoreTypes __RPC_FAR * This);


void __RPC_STUB IEnumPStoreTypes_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreTypes_Clone_Proxy( 
    IEnumPStoreTypes __RPC_FAR * This,
    /* [out] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumPStoreTypes_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPStoreTypes_INTERFACE_DEFINED__ */


#ifndef __IPStore_INTERFACE_DEFINED__
#define __IPStore_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPStore
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IPStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPStore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppProperties) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProvParam( 
            /* [in] */ DWORD dwParam,
            /* [out] */ DWORD __RPC_FAR *pcbData,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProvParam( 
            /* [in] */ DWORD dwParam,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE __RPC_FAR *pbData,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateType( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ PPST_TYPEINFO pInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteType( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSubtype( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ PPST_TYPEINFO pInfo,
            /* [in] */ PPST_ACCESSRULESET pRules,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubtypeInfo( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteSubtype( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadAccessRuleset( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [out] */ PPST_ACCESSRULESET __RPC_FAR *ppRules,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAccessRuleset( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ PPST_ACCESSRULESET pRules,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumTypes( 
            /* [in] */ PST_KEY Key,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSubtypes( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadItem( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [out] */ DWORD __RPC_FAR *pcbData,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteItem( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE __RPC_FAR *pbData,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwDefaultConfirmationStyle,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenItem( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ PST_ACCESSMODE ModeFlags,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseItem( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPStore __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPStore __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPStore __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            IPStore __RPC_FAR * This,
            /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppProperties);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProvParam )( 
            IPStore __RPC_FAR * This,
            /* [in] */ DWORD dwParam,
            /* [out] */ DWORD __RPC_FAR *pcbData,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProvParam )( 
            IPStore __RPC_FAR * This,
            /* [in] */ DWORD dwParam,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE __RPC_FAR *pbData,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateType )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ PPST_TYPEINFO pInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteType )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSubtype )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ PPST_TYPEINFO pInfo,
            /* [in] */ PPST_ACCESSRULESET pRules,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSubtypeInfo )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteSubtype )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadAccessRuleset )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [out] */ PPST_ACCESSRULESET __RPC_FAR *ppRules,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteAccessRuleset )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ const GUID __RPC_FAR *pSubtype,
            /* [in] */ PPST_ACCESSRULESET pRules,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumTypes )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumSubtypes )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pType,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadItem )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [out] */ DWORD __RPC_FAR *pcbData,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteItem )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE __RPC_FAR *pbData,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwDefaultConfirmationStyle,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenItem )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ PST_ACCESSMODE ModeFlags,
            /* [in] */ PPST_PROMPTINFO pPromptInfo,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseItem )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ LPCWSTR szItemName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumItems )( 
            IPStore __RPC_FAR * This,
            /* [in] */ PST_KEY Key,
            /* [in] */ const GUID __RPC_FAR *pItemType,
            /* [in] */ const GUID __RPC_FAR *pItemSubtype,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IPStoreVtbl;

    interface IPStore
    {
        CONST_VTBL struct IPStoreVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPStore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPStore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPStore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPStore_GetInfo(This,ppProperties)	\
    (This)->lpVtbl -> GetInfo(This,ppProperties)

#define IPStore_GetProvParam(This,dwParam,pcbData,ppbData,dwFlags)	\
    (This)->lpVtbl -> GetProvParam(This,dwParam,pcbData,ppbData,dwFlags)

#define IPStore_SetProvParam(This,dwParam,cbData,pbData,dwFlags)	\
    (This)->lpVtbl -> SetProvParam(This,dwParam,cbData,pbData,dwFlags)

#define IPStore_CreateType(This,Key,pType,pInfo,dwFlags)	\
    (This)->lpVtbl -> CreateType(This,Key,pType,pInfo,dwFlags)

#define IPStore_GetTypeInfo(This,Key,pType,ppInfo,dwFlags)	\
    (This)->lpVtbl -> GetTypeInfo(This,Key,pType,ppInfo,dwFlags)

#define IPStore_DeleteType(This,Key,pType,dwFlags)	\
    (This)->lpVtbl -> DeleteType(This,Key,pType,dwFlags)

#define IPStore_CreateSubtype(This,Key,pType,pSubtype,pInfo,pRules,dwFlags)	\
    (This)->lpVtbl -> CreateSubtype(This,Key,pType,pSubtype,pInfo,pRules,dwFlags)

#define IPStore_GetSubtypeInfo(This,Key,pType,pSubtype,ppInfo,dwFlags)	\
    (This)->lpVtbl -> GetSubtypeInfo(This,Key,pType,pSubtype,ppInfo,dwFlags)

#define IPStore_DeleteSubtype(This,Key,pType,pSubtype,dwFlags)	\
    (This)->lpVtbl -> DeleteSubtype(This,Key,pType,pSubtype,dwFlags)

#define IPStore_ReadAccessRuleset(This,Key,pType,pSubtype,ppRules,dwFlags)	\
    (This)->lpVtbl -> ReadAccessRuleset(This,Key,pType,pSubtype,ppRules,dwFlags)

#define IPStore_WriteAccessRuleset(This,Key,pType,pSubtype,pRules,dwFlags)	\
    (This)->lpVtbl -> WriteAccessRuleset(This,Key,pType,pSubtype,pRules,dwFlags)

#define IPStore_EnumTypes(This,Key,dwFlags,ppenum)	\
    (This)->lpVtbl -> EnumTypes(This,Key,dwFlags,ppenum)

#define IPStore_EnumSubtypes(This,Key,pType,dwFlags,ppenum)	\
    (This)->lpVtbl -> EnumSubtypes(This,Key,pType,dwFlags,ppenum)

#define IPStore_DeleteItem(This,Key,pItemType,pItemSubtype,szItemName,pPromptInfo,dwFlags)	\
    (This)->lpVtbl -> DeleteItem(This,Key,pItemType,pItemSubtype,szItemName,pPromptInfo,dwFlags)

#define IPStore_ReadItem(This,Key,pItemType,pItemSubtype,szItemName,pcbData,ppbData,pPromptInfo,dwFlags)	\
    (This)->lpVtbl -> ReadItem(This,Key,pItemType,pItemSubtype,szItemName,pcbData,ppbData,pPromptInfo,dwFlags)

#define IPStore_WriteItem(This,Key,pItemType,pItemSubtype,szItemName,cbData,pbData,pPromptInfo,dwDefaultConfirmationStyle,dwFlags)	\
    (This)->lpVtbl -> WriteItem(This,Key,pItemType,pItemSubtype,szItemName,cbData,pbData,pPromptInfo,dwDefaultConfirmationStyle,dwFlags)

#define IPStore_OpenItem(This,Key,pItemType,pItemSubtype,szItemName,ModeFlags,pPromptInfo,dwFlags)	\
    (This)->lpVtbl -> OpenItem(This,Key,pItemType,pItemSubtype,szItemName,ModeFlags,pPromptInfo,dwFlags)

#define IPStore_CloseItem(This,Key,pItemType,pItemSubtype,szItemName,dwFlags)	\
    (This)->lpVtbl -> CloseItem(This,Key,pItemType,pItemSubtype,szItemName,dwFlags)

#define IPStore_EnumItems(This,Key,pItemType,pItemSubtype,dwFlags,ppenum)	\
    (This)->lpVtbl -> EnumItems(This,Key,pItemType,pItemSubtype,dwFlags,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPStore_GetInfo_Proxy( 
    IPStore __RPC_FAR * This,
    /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppProperties);


void __RPC_STUB IPStore_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_GetProvParam_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ DWORD dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_GetProvParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_SetProvParam_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ DWORD dwParam,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE __RPC_FAR *pbData,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_SetProvParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_CreateType_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_CreateType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_GetTypeInfo_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_DeleteType_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_DeleteType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_CreateSubtype_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_CreateSubtype_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_GetSubtypeInfo_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_GetSubtypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_DeleteSubtype_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_DeleteSubtype_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_ReadAccessRuleset_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_ACCESSRULESET __RPC_FAR *ppRules,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_ReadAccessRuleset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_WriteAccessRuleset_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_WriteAccessRuleset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_EnumTypes_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IPStore_EnumTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_EnumSubtypes_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IPStore_EnumSubtypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_DeleteItem_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_ReadItem_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_ReadItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_WriteItem_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE __RPC_FAR *pbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwDefaultConfirmationStyle,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_WriteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_OpenItem_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PST_ACCESSMODE ModeFlags,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_OpenItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_CloseItem_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IPStore_CloseItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPStore_EnumItems_Proxy( 
    IPStore __RPC_FAR * This,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IPStore_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPStore_INTERFACE_DEFINED__ */


#ifndef __IEnumPStoreProviders_INTERFACE_DEFINED__
#define __IEnumPStoreProviders_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumPStoreProviders
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IEnumPStoreProviders;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumPStoreProviders : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [out][size_is] */ PST_PROVIDERINFO __RPC_FAR *__RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumPStoreProvidersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumPStoreProviders __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumPStoreProviders __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumPStoreProviders __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumPStoreProviders __RPC_FAR * This,
            /* [in] */ DWORD celt,
            /* [out][size_is] */ PST_PROVIDERINFO __RPC_FAR *__RPC_FAR *rgelt,
            /* [out][in] */ DWORD __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumPStoreProviders __RPC_FAR * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumPStoreProviders __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumPStoreProviders __RPC_FAR * This,
            /* [out] */ IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumPStoreProvidersVtbl;

    interface IEnumPStoreProviders
    {
        CONST_VTBL struct IEnumPStoreProvidersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumPStoreProviders_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumPStoreProviders_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumPStoreProviders_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumPStoreProviders_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumPStoreProviders_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumPStoreProviders_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumPStoreProviders_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumPStoreProviders_Next_Proxy( 
    IEnumPStoreProviders __RPC_FAR * This,
    /* [in] */ DWORD celt,
    /* [out][size_is] */ PST_PROVIDERINFO __RPC_FAR *__RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumPStoreProviders_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreProviders_Skip_Proxy( 
    IEnumPStoreProviders __RPC_FAR * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IEnumPStoreProviders_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreProviders_Reset_Proxy( 
    IEnumPStoreProviders __RPC_FAR * This);


void __RPC_STUB IEnumPStoreProviders_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumPStoreProviders_Clone_Proxy( 
    IEnumPStoreProviders __RPC_FAR * This,
    /* [out] */ IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumPStoreProviders_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumPStoreProviders_INTERFACE_DEFINED__ */



#ifndef __PSTORECLib_LIBRARY_DEFINED__
#define __PSTORECLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: PSTORECLib
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_PSTORECLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CPStore;

class CPStore;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CEnumTypes;

class CEnumTypes;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CEnumItems;

class CEnumItems;
#endif
#endif /* __PSTORECLib_LIBRARY_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL__intf_0080
 * at Wed Apr 23 23:56:14 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


HRESULT __stdcall PStoreCreateInstance( 
    IPStore __RPC_FAR *__RPC_FAR *ppProvider,
    PST_PROVIDERID __RPC_FAR *pProviderID,
    void __RPC_FAR *pReserved,
    DWORD dwFlags);

HRESULT __stdcall PStoreEnumProviders( 
    DWORD dwFlags,
    IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum);



extern RPC_IF_HANDLE __MIDL__intf_0080_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0080_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif // PSTORE_H
