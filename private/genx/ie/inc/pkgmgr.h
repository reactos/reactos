/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Jul 11 15:30:21 1997
 */
/* Compiler settings for pkgmgr.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __pkgmgr_h__
#define __pkgmgr_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEnumCodeStoreDBEntry_FWD_DEFINED__
#define __IEnumCodeStoreDBEntry_FWD_DEFINED__
typedef interface IEnumCodeStoreDBEntry IEnumCodeStoreDBEntry;
#endif 	/* __IEnumCodeStoreDBEntry_FWD_DEFINED__ */


#ifndef __ICodeStoreDBEntry_FWD_DEFINED__
#define __ICodeStoreDBEntry_FWD_DEFINED__
typedef interface ICodeStoreDBEntry ICodeStoreDBEntry;
#endif 	/* __ICodeStoreDBEntry_FWD_DEFINED__ */


#ifndef __ICodeStoreDB_FWD_DEFINED__
#define __ICodeStoreDB_FWD_DEFINED__
typedef interface ICodeStoreDB ICodeStoreDB;
#endif 	/* __ICodeStoreDB_FWD_DEFINED__ */


#ifndef __IJavaPackageManager_FWD_DEFINED__
#define __IJavaPackageManager_FWD_DEFINED__
typedef interface IJavaPackageManager IJavaPackageManager;
#endif 	/* __IJavaPackageManager_FWD_DEFINED__ */


#ifndef __IJavaPackage_FWD_DEFINED__
#define __IJavaPackage_FWD_DEFINED__
typedef interface IJavaPackage IJavaPackage;
#endif 	/* __IJavaPackage_FWD_DEFINED__ */


#ifndef __ICreateJavaPackageMgr_FWD_DEFINED__
#define __ICreateJavaPackageMgr_FWD_DEFINED__
typedef interface ICreateJavaPackageMgr ICreateJavaPackageMgr;
#endif 	/* __ICreateJavaPackageMgr_FWD_DEFINED__ */


#ifndef __IJavaFile_FWD_DEFINED__
#define __IJavaFile_FWD_DEFINED__
typedef interface IJavaFile IJavaFile;
#endif 	/* __IJavaFile_FWD_DEFINED__ */


#ifndef __IEnumJavaPackage_FWD_DEFINED__
#define __IEnumJavaPackage_FWD_DEFINED__
typedef interface IEnumJavaPackage IEnumJavaPackage;
#endif 	/* __IEnumJavaPackage_FWD_DEFINED__ */


#ifndef __IEnumJavaFile_FWD_DEFINED__
#define __IEnumJavaFile_FWD_DEFINED__
typedef interface IEnumJavaFile IEnumJavaFile;
#endif 	/* __IEnumJavaFile_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 







typedef const BYTE __RPC_FAR *LPCBYTE;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0000_0001
    {	JPMPII_SYSTEMCLASS	= 0,
	JPMPII_NONSYSTEMCLASS	= 1,
	JPMPII_NEEDSTRUSTEDSOURCE	= 2,
	ALL_JPMPII_FLAGS	= JPMPII_SYSTEMCLASS | JPMPII_NONSYSTEMCLASS | JPMPII_NEEDSTRUSTEDSOURCE
    }	JPMPII_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0000_0002
    {	JPMINST_NOVERSIONCHECK	= 1,
	JPMINST_NOSIGNERCHECK	= 2,
	JPMINST_AUTODETECTPACKAGES	= 4,
	JPMINST_DELETEINPUTFILE	= 8,
	ALL_JPMINST_FLAGS	= JPMINST_NOVERSIONCHECK | JPMINST_NOSIGNERCHECK | JPMINST_AUTODETECTPACKAGES | JPMINST_DELETEINPUTFILE
    }	JPMINST_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0000_0003
    {	JPMINST_CAB	= 0,
	JPMINST_ZIP	= 1,
	JPMINST_JAR	= 2
    }	JPMINST_FILETYPES;

typedef struct  tagPACKAGEINSTALLINFO
    {
    DWORD cbStruct;
    LPCOLESTR pszPackageName;
    DWORD dwVersionMS;
    DWORD dwVersionLS;
    DWORD dwFlags;
    LPCOLESTR pszCodebaseURL;
    LPCOLESTR pszDistributionUnit;
    LPUNKNOWN pUnknown;
    HRESULT result;
    }	PACKAGEINSTALLINFO;

typedef /* [unique] */ PACKAGEINSTALLINFO __RPC_FAR *LPPACKAGEINSTALLINFO;

typedef /* [unique] */ const PACKAGEINSTALLINFO __RPC_FAR *LPCPACKAGEINSTALLINFO;

typedef struct  tagPACKAGESECURITYINFO
    {
    DWORD cbStruct;
    LPCBYTE pCapabilities;
    DWORD cbCapabilities;
    LPCBYTE pSigner;
    DWORD cbSigner;
    BOOL fAllPermissions;
    }	PACKAGESECURITYINFO;

typedef /* [unique] */ const PACKAGESECURITYINFO __RPC_FAR *LPCPACKAGESECURITYINFO;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;


#ifndef __JPKGMGR_LIBRARY_DEFINED__
#define __JPKGMGR_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: JPKGMGR
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [helpstring][version][uuid] */ 


typedef /* [public] */ 
enum __MIDL___MIDL__intf_0071_0001
    {	CRF_REMOVECHILDREN	= 1,
	CRF_REMOVEVALUESONLY	= 2,
	CRF_REMOVEEMPTYPARENTSALSO	= 4
    }	CSDB_REMOVE_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0071_0002
    {	CGF_FULLYQUALIFIED	= 1
    }	CSDB_GETNAME_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0073_0001
    {	JPMFI_NEEDS_TRUSTED_SOURCE	= 0x1,
	JPMFI_IS_STREAM	= 0x2,
	JPMFI_IS_PE_NATIVE	= 0x8,
	JPMFI_FROM_PKG_DATABASE	= 0x10,
	JPMFI_FROM_CLASSPATH	= 0x20
    }	JPM_FILEINFO_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL__intf_0073_0002
    {	JPMGFN_SIMPLE	= 1,
	JPMGFN_FULLYQUALIFIED	= 2,
	JPMGFN_LOCALFILEPATH	= 3
    }	JPM_GETFILENAME_TYPE;


EXTERN_C const IID LIBID_JPKGMGR;

#ifndef __IEnumCodeStoreDBEntry_INTERFACE_DEFINED__
#define __IEnumCodeStoreDBEntry_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumCodeStoreDBEntry
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEnumCodeStoreDBEntry __RPC_FAR *LPENUMCODESTOREDBENTRY;


EXTERN_C const IID IID_IEnumCodeStoreDBEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumCodeStoreDBEntry : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCodeStoreDBEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumCodeStoreDBEntry __RPC_FAR * This,
            /* [out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumCodeStoreDBEntryVtbl;

    interface IEnumCodeStoreDBEntry
    {
        CONST_VTBL struct IEnumCodeStoreDBEntryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCodeStoreDBEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCodeStoreDBEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCodeStoreDBEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCodeStoreDBEntry_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumCodeStoreDBEntry_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumCodeStoreDBEntry_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCodeStoreDBEntry_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCodeStoreDBEntry_Next_Proxy( 
    IEnumCodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumCodeStoreDBEntry_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCodeStoreDBEntry_Skip_Proxy( 
    IEnumCodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumCodeStoreDBEntry_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCodeStoreDBEntry_Reset_Proxy( 
    IEnumCodeStoreDBEntry __RPC_FAR * This);


void __RPC_STUB IEnumCodeStoreDBEntry_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCodeStoreDBEntry_Clone_Proxy( 
    IEnumCodeStoreDBEntry __RPC_FAR * This,
    /* [out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumCodeStoreDBEntry_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCodeStoreDBEntry_INTERFACE_DEFINED__ */


#ifndef __ICodeStoreDBEntry_INTERFACE_DEFINED__
#define __ICodeStoreDBEntry_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICodeStoreDBEntry
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_ICodeStoreDBEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICodeStoreDBEntry : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateChild( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveChild( 
            /* [in] */ LPCOLESTR pszName,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChild( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumChildren( 
            /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParent( 
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteProperty( 
            /* [in] */ LPCOLESTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDWORDPropertyA( 
            /* [in] */ LPCSTR pszName,
            /* [out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDWORDPropertyA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStringPropertyA( 
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPSTR pszVal,
            /* [in] */ DWORD cbVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStringPropertyA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodeStoreDBEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICodeStoreDBEntry __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICodeStoreDBEntry __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateChild )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveChild )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChild )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumChildren )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParent )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppParent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperty )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteProperty )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDWORDPropertyA )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ DWORD __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDWORDPropertyA )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStringPropertyA )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPSTR pszVal,
            /* [in] */ DWORD cbVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStringPropertyA )( 
            ICodeStoreDBEntry __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszVal);
        
        END_INTERFACE
    } ICodeStoreDBEntryVtbl;

    interface ICodeStoreDBEntry
    {
        CONST_VTBL struct ICodeStoreDBEntryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodeStoreDBEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodeStoreDBEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodeStoreDBEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodeStoreDBEntry_GetName(This,dwFlags,pbstrName)	\
    (This)->lpVtbl -> GetName(This,dwFlags,pbstrName)

#define ICodeStoreDBEntry_CreateChild(This,pszName,ppEntry)	\
    (This)->lpVtbl -> CreateChild(This,pszName,ppEntry)

#define ICodeStoreDBEntry_RemoveChild(This,pszName,dwFlags)	\
    (This)->lpVtbl -> RemoveChild(This,pszName,dwFlags)

#define ICodeStoreDBEntry_GetChild(This,pszName,ppEntry)	\
    (This)->lpVtbl -> GetChild(This,pszName,ppEntry)

#define ICodeStoreDBEntry_EnumChildren(This,ppEntry)	\
    (This)->lpVtbl -> EnumChildren(This,ppEntry)

#define ICodeStoreDBEntry_GetParent(This,ppParent)	\
    (This)->lpVtbl -> GetParent(This,ppParent)

#define ICodeStoreDBEntry_GetProperty(This,pszName,pvarProperty)	\
    (This)->lpVtbl -> GetProperty(This,pszName,pvarProperty)

#define ICodeStoreDBEntry_SetProperty(This,pszName,pvarProperty)	\
    (This)->lpVtbl -> SetProperty(This,pszName,pvarProperty)

#define ICodeStoreDBEntry_DeleteProperty(This,pszName)	\
    (This)->lpVtbl -> DeleteProperty(This,pszName)

#define ICodeStoreDBEntry_GetDWORDPropertyA(This,pszName,pVal)	\
    (This)->lpVtbl -> GetDWORDPropertyA(This,pszName,pVal)

#define ICodeStoreDBEntry_SetDWORDPropertyA(This,pszName,dwVal)	\
    (This)->lpVtbl -> SetDWORDPropertyA(This,pszName,dwVal)

#define ICodeStoreDBEntry_GetStringPropertyA(This,pszName,pszVal,cbVal)	\
    (This)->lpVtbl -> GetStringPropertyA(This,pszName,pszVal,cbVal)

#define ICodeStoreDBEntry_SetStringPropertyA(This,pszName,pszVal)	\
    (This)->lpVtbl -> SetStringPropertyA(This,pszName,pszVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetName_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB ICodeStoreDBEntry_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_CreateChild_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDBEntry_CreateChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_RemoveChild_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    DWORD dwFlags);


void __RPC_STUB ICodeStoreDBEntry_RemoveChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetChild_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDBEntry_GetChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_EnumChildren_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDBEntry_EnumChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetParent_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppParent);


void __RPC_STUB ICodeStoreDBEntry_GetParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetProperty_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty);


void __RPC_STUB ICodeStoreDBEntry_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_SetProperty_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty);


void __RPC_STUB ICodeStoreDBEntry_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_DeleteProperty_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName);


void __RPC_STUB ICodeStoreDBEntry_DeleteProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetDWORDPropertyA_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB ICodeStoreDBEntry_GetDWORDPropertyA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_SetDWORDPropertyA_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwVal);


void __RPC_STUB ICodeStoreDBEntry_SetDWORDPropertyA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_GetStringPropertyA_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out] */ LPSTR pszVal,
    /* [in] */ DWORD cbVal);


void __RPC_STUB ICodeStoreDBEntry_GetStringPropertyA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDBEntry_SetStringPropertyA_Proxy( 
    ICodeStoreDBEntry __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ LPCSTR pszVal);


void __RPC_STUB ICodeStoreDBEntry_SetStringPropertyA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodeStoreDBEntry_INTERFACE_DEFINED__ */


#ifndef __ICodeStoreDB_INTERFACE_DEFINED__
#define __ICodeStoreDB_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICodeStoreDB
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_ICodeStoreDB;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICodeStoreDB : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateEntry( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveEntry( 
            /* [in] */ LPCOLESTR pszName,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEntry( 
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRootEntries( 
            /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodeStoreDBVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICodeStoreDB __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICodeStoreDB __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICodeStoreDB __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateEntry )( 
            ICodeStoreDB __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveEntry )( 
            ICodeStoreDB __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEntry )( 
            ICodeStoreDB __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszName,
            /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRootEntries )( 
            ICodeStoreDB __RPC_FAR * This,
            /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        END_INTERFACE
    } ICodeStoreDBVtbl;

    interface ICodeStoreDB
    {
        CONST_VTBL struct ICodeStoreDBVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodeStoreDB_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodeStoreDB_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodeStoreDB_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodeStoreDB_CreateEntry(This,pszName,ppEntry)	\
    (This)->lpVtbl -> CreateEntry(This,pszName,ppEntry)

#define ICodeStoreDB_RemoveEntry(This,pszName,dwFlags)	\
    (This)->lpVtbl -> RemoveEntry(This,pszName,dwFlags)

#define ICodeStoreDB_GetEntry(This,pszName,ppEntry)	\
    (This)->lpVtbl -> GetEntry(This,pszName,ppEntry)

#define ICodeStoreDB_EnumRootEntries(This,ppEntry)	\
    (This)->lpVtbl -> EnumRootEntries(This,ppEntry)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodeStoreDB_CreateEntry_Proxy( 
    ICodeStoreDB __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDB_CreateEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDB_RemoveEntry_Proxy( 
    ICodeStoreDB __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    DWORD dwFlags);


void __RPC_STUB ICodeStoreDB_RemoveEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDB_GetEntry_Proxy( 
    ICodeStoreDB __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszName,
    /* [retval][out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDB_GetEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICodeStoreDB_EnumRootEntries_Proxy( 
    ICodeStoreDB __RPC_FAR * This,
    /* [retval][out] */ IEnumCodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB ICodeStoreDB_EnumRootEntries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodeStoreDB_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CLSID_CodeStoreDB;

class CLSID_CodeStoreDB;
#endif

#ifndef __IJavaPackageManager_INTERFACE_DEFINED__
#define __IJavaPackageManager_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaPackageManager
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IJavaPackageManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaPackageManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InstallPackage( 
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFileType,
            /* [out][in] */ LPPACKAGEINSTALLINFO pPackageInfo,
            /* [in] */ UINT cPackages,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCPACKAGESECURITYINFO pSecurityInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UninstallPackage( 
            /* [in] */ LPCOLESTR pszPackageName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPackages( 
            /* [in] */ LPCOLESTR pszNamespace,
            /* [retval][out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPackage( 
            /* [in] */ LPCOLESTR pszPackageName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [retval][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *ppPackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFile( 
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IJavaFile __RPC_FAR *__RPC_FAR *ppFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExplicitClassPath( 
            /* [out] */ BSTR __RPC_FAR *pbstrPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExplicitClassPath( 
            /* [in] */ LPCOLESTR pszPath,
            /* [in] */ BOOL fAppend) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCurrentDirectory( 
            /* [in] */ LPCOLESTR pszDir) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaPackageManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaPackageManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaPackageManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InstallPackage )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFileType,
            /* [out][in] */ LPPACKAGEINSTALLINFO pPackageInfo,
            /* [in] */ UINT cPackages,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCPACKAGESECURITYINFO pSecurityInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UninstallPackage )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszPackageName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumPackages )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [retval][out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPackage )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszPackageName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [retval][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *ppPackage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFile )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFileName,
            /* [in] */ LPCOLESTR pszNamespace,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IJavaFile __RPC_FAR *__RPC_FAR *ppFile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExplicitClassPath )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetExplicitClassPath )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszPath,
            /* [in] */ BOOL fAppend);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCurrentDirectory )( 
            IJavaPackageManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszDir);
        
        END_INTERFACE
    } IJavaPackageManagerVtbl;

    interface IJavaPackageManager
    {
        CONST_VTBL struct IJavaPackageManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaPackageManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaPackageManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaPackageManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaPackageManager_InstallPackage(This,pszFileName,pszNamespace,dwFileType,pPackageInfo,cPackages,dwFlags,pSecurityInfo)	\
    (This)->lpVtbl -> InstallPackage(This,pszFileName,pszNamespace,dwFileType,pPackageInfo,cPackages,dwFlags,pSecurityInfo)

#define IJavaPackageManager_UninstallPackage(This,pszPackageName,pszNamespace,dwFlags)	\
    (This)->lpVtbl -> UninstallPackage(This,pszPackageName,pszNamespace,dwFlags)

#define IJavaPackageManager_EnumPackages(This,pszNamespace,ppEnum)	\
    (This)->lpVtbl -> EnumPackages(This,pszNamespace,ppEnum)

#define IJavaPackageManager_GetPackage(This,pszPackageName,pszNamespace,ppPackage)	\
    (This)->lpVtbl -> GetPackage(This,pszPackageName,pszNamespace,ppPackage)

#define IJavaPackageManager_GetFile(This,pszFileName,pszNamespace,dwFlags,ppFile)	\
    (This)->lpVtbl -> GetFile(This,pszFileName,pszNamespace,dwFlags,ppFile)

#define IJavaPackageManager_GetExplicitClassPath(This,pbstrPath)	\
    (This)->lpVtbl -> GetExplicitClassPath(This,pbstrPath)

#define IJavaPackageManager_SetExplicitClassPath(This,pszPath,fAppend)	\
    (This)->lpVtbl -> SetExplicitClassPath(This,pszPath,fAppend)

#define IJavaPackageManager_SetCurrentDirectory(This,pszDir)	\
    (This)->lpVtbl -> SetCurrentDirectory(This,pszDir)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaPackageManager_InstallPackage_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFileName,
    /* [in] */ LPCOLESTR pszNamespace,
    /* [in] */ DWORD dwFileType,
    /* [out][in] */ LPPACKAGEINSTALLINFO pPackageInfo,
    /* [in] */ UINT cPackages,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCPACKAGESECURITYINFO pSecurityInfo);


void __RPC_STUB IJavaPackageManager_InstallPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_UninstallPackage_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszPackageName,
    /* [in] */ LPCOLESTR pszNamespace,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IJavaPackageManager_UninstallPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_EnumPackages_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszNamespace,
    /* [retval][out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IJavaPackageManager_EnumPackages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_GetPackage_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszPackageName,
    /* [in] */ LPCOLESTR pszNamespace,
    /* [retval][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *ppPackage);


void __RPC_STUB IJavaPackageManager_GetPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_GetFile_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFileName,
    /* [in] */ LPCOLESTR pszNamespace,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IJavaFile __RPC_FAR *__RPC_FAR *ppFile);


void __RPC_STUB IJavaPackageManager_GetFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_GetExplicitClassPath_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrPath);


void __RPC_STUB IJavaPackageManager_GetExplicitClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_SetExplicitClassPath_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszPath,
    /* [in] */ BOOL fAppend);


void __RPC_STUB IJavaPackageManager_SetExplicitClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackageManager_SetCurrentDirectory_Proxy( 
    IJavaPackageManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszDir);


void __RPC_STUB IJavaPackageManager_SetCurrentDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaPackageManager_INTERFACE_DEFINED__ */


#ifndef __IJavaPackage_INTERFACE_DEFINED__
#define __IJavaPackage_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaPackage
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IJavaPackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaPackage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPackageName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
            /* [out] */ DWORD __RPC_FAR *pdwVersionLS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilePath( 
            /* [out] */ BSTR __RPC_FAR *pbstrPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsSystemClass( 
            /* [out] */ BOOL __RPC_FAR *pfIsSystemClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NeedsTrustedSource( 
            /* [out] */ BOOL __RPC_FAR *pfNeedsTrustedSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ LPCBYTE __RPC_FAR *ppCapabilities,
            /* [out] */ DWORD __RPC_FAR *pcbCapabilities) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSigner( 
            /* [out] */ LPCBYTE __RPC_FAR *ppSigner,
            /* [out] */ DWORD __RPC_FAR *pcbSigner) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDatabaseEntry( 
            /* [out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumFiles( 
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilePathA( 
            /* [out] */ LPSTR pszPath,
            /* [in] */ DWORD cbPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaPackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaPackage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaPackage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaPackage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPackageName )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
            /* [out] */ DWORD __RPC_FAR *pdwVersionLS);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFilePath )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSystemClass )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfIsSystemClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NeedsTrustedSource )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfNeedsTrustedSource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCapabilities )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ LPCBYTE __RPC_FAR *ppCapabilities,
            /* [out] */ DWORD __RPC_FAR *pcbCapabilities);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSigner )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ LPCBYTE __RPC_FAR *ppSigner,
            /* [out] */ DWORD __RPC_FAR *pcbSigner);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDatabaseEntry )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumFiles )( 
            IJavaPackage __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFilePathA )( 
            IJavaPackage __RPC_FAR * This,
            /* [out] */ LPSTR pszPath,
            /* [in] */ DWORD cbPath);
        
        END_INTERFACE
    } IJavaPackageVtbl;

    interface IJavaPackage
    {
        CONST_VTBL struct IJavaPackageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaPackage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaPackage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaPackage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaPackage_GetPackageName(This,pbstrName)	\
    (This)->lpVtbl -> GetPackageName(This,pbstrName)

#define IJavaPackage_GetVersion(This,pdwVersionMS,pdwVersionLS)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersionMS,pdwVersionLS)

#define IJavaPackage_GetFilePath(This,pbstrPath)	\
    (This)->lpVtbl -> GetFilePath(This,pbstrPath)

#define IJavaPackage_IsSystemClass(This,pfIsSystemClass)	\
    (This)->lpVtbl -> IsSystemClass(This,pfIsSystemClass)

#define IJavaPackage_NeedsTrustedSource(This,pfNeedsTrustedSource)	\
    (This)->lpVtbl -> NeedsTrustedSource(This,pfNeedsTrustedSource)

#define IJavaPackage_GetCapabilities(This,ppCapabilities,pcbCapabilities)	\
    (This)->lpVtbl -> GetCapabilities(This,ppCapabilities,pcbCapabilities)

#define IJavaPackage_GetSigner(This,ppSigner,pcbSigner)	\
    (This)->lpVtbl -> GetSigner(This,ppSigner,pcbSigner)

#define IJavaPackage_GetDatabaseEntry(This,ppEntry)	\
    (This)->lpVtbl -> GetDatabaseEntry(This,ppEntry)

#define IJavaPackage_EnumFiles(This,dwFlags,ppEnum)	\
    (This)->lpVtbl -> EnumFiles(This,dwFlags,ppEnum)

#define IJavaPackage_GetFilePathA(This,pszPath,cbPath)	\
    (This)->lpVtbl -> GetFilePathA(This,pszPath,cbPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaPackage_GetPackageName_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB IJavaPackage_GetPackageName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetVersion_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
    /* [out] */ DWORD __RPC_FAR *pdwVersionLS);


void __RPC_STUB IJavaPackage_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetFilePath_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrPath);


void __RPC_STUB IJavaPackage_GetFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_IsSystemClass_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfIsSystemClass);


void __RPC_STUB IJavaPackage_IsSystemClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_NeedsTrustedSource_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfNeedsTrustedSource);


void __RPC_STUB IJavaPackage_NeedsTrustedSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetCapabilities_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ LPCBYTE __RPC_FAR *ppCapabilities,
    /* [out] */ DWORD __RPC_FAR *pcbCapabilities);


void __RPC_STUB IJavaPackage_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetSigner_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ LPCBYTE __RPC_FAR *ppSigner,
    /* [out] */ DWORD __RPC_FAR *pcbSigner);


void __RPC_STUB IJavaPackage_GetSigner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetDatabaseEntry_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ ICodeStoreDBEntry __RPC_FAR *__RPC_FAR *ppEntry);


void __RPC_STUB IJavaPackage_GetDatabaseEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_EnumFiles_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [retval][out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IJavaPackage_EnumFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPackage_GetFilePathA_Proxy( 
    IJavaPackage __RPC_FAR * This,
    /* [out] */ LPSTR pszPath,
    /* [in] */ DWORD cbPath);


void __RPC_STUB IJavaPackage_GetFilePathA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaPackage_INTERFACE_DEFINED__ */


#ifndef __ICreateJavaPackageMgr_INTERFACE_DEFINED__
#define __ICreateJavaPackageMgr_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateJavaPackageMgr
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_ICreateJavaPackageMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateJavaPackageMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPackageManager( 
            /* [out] */ IJavaPackageManager __RPC_FAR *__RPC_FAR *ppPackageMgr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateJavaPackageMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateJavaPackageMgr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateJavaPackageMgr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateJavaPackageMgr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPackageManager )( 
            ICreateJavaPackageMgr __RPC_FAR * This,
            /* [out] */ IJavaPackageManager __RPC_FAR *__RPC_FAR *ppPackageMgr);
        
        END_INTERFACE
    } ICreateJavaPackageMgrVtbl;

    interface ICreateJavaPackageMgr
    {
        CONST_VTBL struct ICreateJavaPackageMgrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateJavaPackageMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateJavaPackageMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateJavaPackageMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateJavaPackageMgr_GetPackageManager(This,ppPackageMgr)	\
    (This)->lpVtbl -> GetPackageManager(This,ppPackageMgr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateJavaPackageMgr_GetPackageManager_Proxy( 
    ICreateJavaPackageMgr __RPC_FAR * This,
    /* [out] */ IJavaPackageManager __RPC_FAR *__RPC_FAR *ppPackageMgr);


void __RPC_STUB ICreateJavaPackageMgr_GetPackageManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateJavaPackageMgr_INTERFACE_DEFINED__ */


#ifndef __IJavaFile_INTERFACE_DEFINED__
#define __IJavaFile_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaFile
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IJavaFile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaFile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [in] */ DWORD dwType,
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileStream( 
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPENativePath( 
            /* [out] */ BSTR __RPC_FAR *pbstrPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPENativePathA( 
            /* [out] */ LPSTR pszPath,
            /* [in] */ DWORD cbPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimestamp( 
            /* [out] */ DWORD __RPC_FAR *pdwTimestamp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaFile __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaFile __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaFile __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IJavaFile __RPC_FAR * This,
            /* [in] */ DWORD dwType,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )( 
            IJavaFile __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileStream )( 
            IJavaFile __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPENativePath )( 
            IJavaFile __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPENativePathA )( 
            IJavaFile __RPC_FAR * This,
            /* [out] */ LPSTR pszPath,
            /* [in] */ DWORD cbPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTimestamp )( 
            IJavaFile __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwTimestamp);
        
        END_INTERFACE
    } IJavaFileVtbl;

    interface IJavaFile
    {
        CONST_VTBL struct IJavaFileVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaFile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaFile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaFile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaFile_GetName(This,dwType,pbstrName)	\
    (This)->lpVtbl -> GetName(This,dwType,pbstrName)

#define IJavaFile_GetFlags(This)	\
    (This)->lpVtbl -> GetFlags(This)

#define IJavaFile_GetFileStream(This,ppStream)	\
    (This)->lpVtbl -> GetFileStream(This,ppStream)

#define IJavaFile_GetPENativePath(This,pbstrPath)	\
    (This)->lpVtbl -> GetPENativePath(This,pbstrPath)

#define IJavaFile_GetPENativePathA(This,pszPath,cbPath)	\
    (This)->lpVtbl -> GetPENativePathA(This,pszPath,cbPath)

#define IJavaFile_GetTimestamp(This,pdwTimestamp)	\
    (This)->lpVtbl -> GetTimestamp(This,pdwTimestamp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaFile_GetName_Proxy( 
    IJavaFile __RPC_FAR * This,
    /* [in] */ DWORD dwType,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB IJavaFile_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IJavaFile_GetFlags_Proxy( 
    IJavaFile __RPC_FAR * This);


void __RPC_STUB IJavaFile_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaFile_GetFileStream_Proxy( 
    IJavaFile __RPC_FAR * This,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IJavaFile_GetFileStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaFile_GetPENativePath_Proxy( 
    IJavaFile __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrPath);


void __RPC_STUB IJavaFile_GetPENativePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaFile_GetPENativePathA_Proxy( 
    IJavaFile __RPC_FAR * This,
    /* [out] */ LPSTR pszPath,
    /* [in] */ DWORD cbPath);


void __RPC_STUB IJavaFile_GetPENativePathA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaFile_GetTimestamp_Proxy( 
    IJavaFile __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwTimestamp);


void __RPC_STUB IJavaFile_GetTimestamp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaFile_INTERFACE_DEFINED__ */


#ifndef __IEnumJavaPackage_INTERFACE_DEFINED__
#define __IEnumJavaPackage_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumJavaPackage
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEnumJavaPackage __RPC_FAR *LPENUMJAVAPACKAGE;


EXTERN_C const IID IID_IEnumJavaPackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumJavaPackage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumJavaPackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumJavaPackage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumJavaPackage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumJavaPackage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumJavaPackage __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumJavaPackage __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumJavaPackage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumJavaPackage __RPC_FAR * This,
            /* [out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumJavaPackageVtbl;

    interface IEnumJavaPackage
    {
        CONST_VTBL struct IEnumJavaPackageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumJavaPackage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumJavaPackage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumJavaPackage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumJavaPackage_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumJavaPackage_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumJavaPackage_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumJavaPackage_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumJavaPackage_Next_Proxy( 
    IEnumJavaPackage __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IJavaPackage __RPC_FAR *__RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumJavaPackage_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPackage_Skip_Proxy( 
    IEnumJavaPackage __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumJavaPackage_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPackage_Reset_Proxy( 
    IEnumJavaPackage __RPC_FAR * This);


void __RPC_STUB IEnumJavaPackage_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPackage_Clone_Proxy( 
    IEnumJavaPackage __RPC_FAR * This,
    /* [out] */ IEnumJavaPackage __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumJavaPackage_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumJavaPackage_INTERFACE_DEFINED__ */


#ifndef __IEnumJavaFile_INTERFACE_DEFINED__
#define __IEnumJavaFile_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumJavaFile
 * at Fri Jul 11 15:30:21 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEnumJavaFile __RPC_FAR *LPENUMJAVAFILE;


EXTERN_C const IID IID_IEnumJavaFile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumJavaFile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IJavaFile __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumJavaFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumJavaFile __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumJavaFile __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumJavaFile __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumJavaFile __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IJavaFile __RPC_FAR *__RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumJavaFile __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumJavaFile __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumJavaFile __RPC_FAR * This,
            /* [out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumJavaFileVtbl;

    interface IEnumJavaFile
    {
        CONST_VTBL struct IEnumJavaFileVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumJavaFile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumJavaFile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumJavaFile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumJavaFile_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumJavaFile_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumJavaFile_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumJavaFile_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumJavaFile_Next_Proxy( 
    IEnumJavaFile __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IJavaFile __RPC_FAR *__RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumJavaFile_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaFile_Skip_Proxy( 
    IEnumJavaFile __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumJavaFile_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaFile_Reset_Proxy( 
    IEnumJavaFile __RPC_FAR * This);


void __RPC_STUB IEnumJavaFile_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaFile_Clone_Proxy( 
    IEnumJavaFile __RPC_FAR * This,
    /* [out] */ IEnumJavaFile __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumJavaFile_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumJavaFile_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CLSID_JavaPackageManager;

class CLSID_JavaPackageManager;
#endif
#endif /* __JPKGMGR_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
