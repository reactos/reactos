/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Wed Jun 18 20:10:23 1997
 */
/* Compiler settings for jps.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __jps_h__
#define __jps_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IJavaClsStream_FWD_DEFINED__
#define __IJavaClsStream_FWD_DEFINED__
typedef interface IJavaClsStream IJavaClsStream;
#endif 	/* __IJavaClsStream_FWD_DEFINED__ */


#ifndef __IJavaCls_FWD_DEFINED__
#define __IJavaCls_FWD_DEFINED__
typedef interface IJavaCls IJavaCls;
#endif 	/* __IJavaCls_FWD_DEFINED__ */


#ifndef __IJavaPkg_FWD_DEFINED__
#define __IJavaPkg_FWD_DEFINED__
typedef interface IJavaPkg IJavaPkg;
#endif 	/* __IJavaPkg_FWD_DEFINED__ */


#ifndef __IEnumJavaPkg_FWD_DEFINED__
#define __IEnumJavaPkg_FWD_DEFINED__
typedef interface IEnumJavaPkg IEnumJavaPkg;
#endif 	/* __IEnumJavaPkg_FWD_DEFINED__ */


#ifndef __IEnumJavaCls_FWD_DEFINED__
#define __IEnumJavaCls_FWD_DEFINED__
typedef interface IEnumJavaCls IEnumJavaCls;
#endif 	/* __IEnumJavaCls_FWD_DEFINED__ */


#ifndef __IJavaPkgService_FWD_DEFINED__
#define __IJavaPkgService_FWD_DEFINED__
typedef interface IJavaPkgService IJavaPkgService;
#endif 	/* __IJavaPkgService_FWD_DEFINED__ */


#ifndef __JavaPkgService_FWD_DEFINED__
#define __JavaPkgService_FWD_DEFINED__

#ifdef __cplusplus
typedef class JavaPkgService JavaPkgService;
#else
typedef struct JavaPkgService JavaPkgService;
#endif /* __cplusplus */

#endif 	/* __JavaPkgService_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __JavaPackageService_LIBRARY_DEFINED__
#define __JavaPackageService_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: JavaPackageService
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [version][uuid] */ 








typedef 
enum _accflags
    {	ACC_PUBLIC	= 0x1,
	ACC_PRIVATE	= 0x2,
	ACC_PROTECTED	= 0x4,
	ACC_STATIC	= 0x8,
	ACC_FINAL	= 0x10,
	ACC_SYNCHRONIZED	= 0x20,
	ACC_SUPER	= 0x20,
	ACC_VOLATILE	= 0x40,
	ACC_TRANSIENT	= 0x80,
	ACC_NATIVE	= 0x100,
	ACC_INTERFACE	= 0x200,
	ACC_ABSTRACT	= 0x400
    }	ACCFLAGS;

typedef 
enum _cptags
    {	CONSTANT_Class	= 7,
	CONSTANT_Fieldref	= 9,
	CONSTANT_Methodref	= 10,
	CONSTANT_InterfaceMethodref	= 11,
	CONSTANT_String	= 8,
	CONSTANT_Integer	= 3,
	CONSTANT_Float	= 4,
	CONSTANT_Long	= 5,
	CONSTANT_Double	= 6,
	CONSTANT_NameAndType	= 12,
	CONSTANT_Utf8	= 1,
	CONSTANT_Unicode	= 2
    }	CPTAGS;

typedef unsigned char U1;

typedef unsigned short U2;

typedef unsigned long U4;

typedef MIDL_uhyper U8;

typedef struct  _longdbl
    {
    union 
        {
        U8 iValue;
        double fValue;
        struct  
            {
            U4 iLow;
            U4 iHigh;
            }	;
        }	;
    }	LONGDBL;

typedef struct  _cpoolentry
    {
    U1 iTag;
    union 
        {
        struct  
            {
            U2 iName;
            }	Class;
        struct  
            {
            U2 iClass;
            U2 iNameAndType;
            }	Fieldref;
        struct  
            {
            U2 iClass;
            U2 iNameAndType;
            }	Methodref;
        struct  
            {
            U2 iClass;
            U2 iNameAndType;
            }	InterfaceMethodref;
        struct  
            {
            U2 iIndex;
            }	String;
        struct  
            {
            U4 iValue;
            }	Integer;
        struct  
            {
            float fValue;
            }	Float;
        struct  
            {
            LONGDBL __RPC_FAR *pVal;
            }	Long;
        struct  _DoubleStruct
            {
            LONGDBL __RPC_FAR *pVal;
            }	Double;
        struct  
            {
            U2 iName;
            U2 iSignature;
            }	NameAndType;
        struct  
            {
            U2 iLength;
            U1 __RPC_FAR *pBytes;
            }	Utf8;
        struct  
            {
            U2 iLength;
            U2 __RPC_FAR *pBytes;
            }	Unicode;
        }	;
    }	CPOOLENTRY;

typedef struct  _attrinfo
    {
    struct _attrinfo __RPC_FAR *pNext;
    U2 iName;
    U4 iLength;
    U1 rgBytes[ 1 ];
    }	ATTRINFO;

typedef struct  _memberinfo
    {
    U2 iAccessFlags;
    U2 iName;
    U2 iSignature;
    ATTRINFO __RPC_FAR *pAttrList;
    }	MEMBERINFO;

STDAPI_(IJavaPkgService *) CreateJPS ();

EXTERN_C const IID LIBID_JavaPackageService;

#ifndef __IJavaClsStream_INTERFACE_DEFINED__
#define __IJavaClsStream_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaClsStream
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IJavaClsStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF7-A46E-11d0-A8BE-00A0C921A4D2")
    IJavaClsStream : public IStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetCurrentPosition( 
            long iPos,
            DWORD dwMoveType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            void __RPC_FAR *pvDest,
            long __RPC_FAR *piRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadU2( 
            U2 __RPC_FAR *piVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadU4( 
            U4 __RPC_FAR *piVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaClsStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaClsStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaClsStream __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IJavaClsStream __RPC_FAR * This,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IJavaClsStream __RPC_FAR * This,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSize )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libNewSize);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )( 
            IJavaClsStream __RPC_FAR * This,
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Revert )( 
            IJavaClsStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockRegion )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockRegion )( 
            IJavaClsStream __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stat )( 
            IJavaClsStream __RPC_FAR * This,
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaClsStream __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCurrentPosition )( 
            IJavaClsStream __RPC_FAR * This,
            long iPos,
            DWORD dwMoveType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IJavaClsStream __RPC_FAR * This,
            void __RPC_FAR *pvDest,
            long __RPC_FAR *piRead);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadU2 )( 
            IJavaClsStream __RPC_FAR * This,
            U2 __RPC_FAR *piVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadU4 )( 
            IJavaClsStream __RPC_FAR * This,
            U4 __RPC_FAR *piVal);
        
        END_INTERFACE
    } IJavaClsStreamVtbl;

    interface IJavaClsStream
    {
        CONST_VTBL struct IJavaClsStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaClsStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaClsStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaClsStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaClsStream_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define IJavaClsStream_Write(This,pv,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pv,cb,pcbWritten)


#define IJavaClsStream_Seek(This,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,dlibMove,dwOrigin,plibNewPosition)

#define IJavaClsStream_SetSize(This,libNewSize)	\
    (This)->lpVtbl -> SetSize(This,libNewSize)

#define IJavaClsStream_CopyTo(This,pstm,cb,pcbRead,pcbWritten)	\
    (This)->lpVtbl -> CopyTo(This,pstm,cb,pcbRead,pcbWritten)

#define IJavaClsStream_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IJavaClsStream_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IJavaClsStream_LockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> LockRegion(This,libOffset,cb,dwLockType)

#define IJavaClsStream_UnlockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> UnlockRegion(This,libOffset,cb,dwLockType)

#define IJavaClsStream_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)

#define IJavaClsStream_Clone(This,ppstm)	\
    (This)->lpVtbl -> Clone(This,ppstm)


#define IJavaClsStream_SetCurrentPosition(This,iPos,dwMoveType)	\
    (This)->lpVtbl -> SetCurrentPosition(This,iPos,dwMoveType)

#define IJavaClsStream_Read(This,pvDest,piRead)	\
    (This)->lpVtbl -> Read(This,pvDest,piRead)

#define IJavaClsStream_ReadU2(This,piVal)	\
    (This)->lpVtbl -> ReadU2(This,piVal)

#define IJavaClsStream_ReadU4(This,piVal)	\
    (This)->lpVtbl -> ReadU4(This,piVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaClsStream_SetCurrentPosition_Proxy( 
    IJavaClsStream __RPC_FAR * This,
    long iPos,
    DWORD dwMoveType);


void __RPC_STUB IJavaClsStream_SetCurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaClsStream_Read_Proxy( 
    IJavaClsStream __RPC_FAR * This,
    void __RPC_FAR *pvDest,
    long __RPC_FAR *piRead);


void __RPC_STUB IJavaClsStream_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaClsStream_ReadU2_Proxy( 
    IJavaClsStream __RPC_FAR * This,
    U2 __RPC_FAR *piVal);


void __RPC_STUB IJavaClsStream_ReadU2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaClsStream_ReadU4_Proxy( 
    IJavaClsStream __RPC_FAR * This,
    U4 __RPC_FAR *piVal);


void __RPC_STUB IJavaClsStream_ReadU4_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaClsStream_INTERFACE_DEFINED__ */


#ifndef __IJavaCls_INTERFACE_DEFINED__
#define __IJavaCls_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaCls
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IJavaCls;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF3-A46E-11d0-A8BE-00A0C921A4D2")
    IJavaCls : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFullName( 
            BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual const WCHAR __RPC_FAR *STDMETHODCALLTYPE GetBaseName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassMoniker( 
            BSTR __RPC_FAR *pbstrClassMoniker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassStream( 
            IJavaClsStream __RPC_FAR *__RPC_FAR *ppClassStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceMoniker( 
            BSTR __RPC_FAR *pbstrSourceMoniker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceStream( 
            IStream __RPC_FAR *__RPC_FAR *ppSourceStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsOutOfDate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            BOOL fFullOpen) = 0;
        
        virtual void STDMETHODCALLTYPE Close( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetMajorVersion( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetMinorVersion( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetCPCount( void) = 0;
        
        virtual CPOOLENTRY __RPC_FAR *STDMETHODCALLTYPE GetCPArray( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetAccessFlags( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetThisClass( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetSuperClass( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetInterfaceCount( void) = 0;
        
        virtual U2 __RPC_FAR *STDMETHODCALLTYPE GetInterfaceArray( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetFieldCount( void) = 0;
        
        virtual MEMBERINFO __RPC_FAR *STDMETHODCALLTYPE GetFieldArray( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetMethodCount( void) = 0;
        
        virtual MEMBERINFO __RPC_FAR *STDMETHODCALLTYPE GetMethodArray( void) = 0;
        
        virtual U2 STDMETHODCALLTYPE GetAttributeCount( void) = 0;
        
        virtual ATTRINFO __RPC_FAR *STDMETHODCALLTYPE GetAttributeList( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConstantString( 
            U2 iCPEntry,
            BSTR __RPC_FAR *pbstrString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemberName( 
            MEMBERINFO __RPC_FAR *pMbrInfo,
            BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemberSignature( 
            MEMBERINFO __RPC_FAR *pMbrInfo,
            BSTR __RPC_FAR *pbstrSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoesSourceStreamExist( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoesClassStreamExist( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaClsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaCls __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaCls __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFullName )( 
            IJavaCls __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrName);
        
        const WCHAR __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetBaseName )( 
            IJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassMoniker )( 
            IJavaCls __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrClassMoniker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassStream )( 
            IJavaCls __RPC_FAR * This,
            IJavaClsStream __RPC_FAR *__RPC_FAR *ppClassStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceMoniker )( 
            IJavaCls __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrSourceMoniker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceStream )( 
            IJavaCls __RPC_FAR * This,
            IStream __RPC_FAR *__RPC_FAR *ppSourceStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsOutOfDate )( 
            IJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IJavaCls __RPC_FAR * This,
            BOOL fFullOpen);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetMajorVersion )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetMinorVersion )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetCPCount )( 
            IJavaCls __RPC_FAR * This);
        
        CPOOLENTRY __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetCPArray )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetAccessFlags )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetThisClass )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetSuperClass )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetInterfaceCount )( 
            IJavaCls __RPC_FAR * This);
        
        U2 __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetInterfaceArray )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetFieldCount )( 
            IJavaCls __RPC_FAR * This);
        
        MEMBERINFO __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFieldArray )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetMethodCount )( 
            IJavaCls __RPC_FAR * This);
        
        MEMBERINFO __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetMethodArray )( 
            IJavaCls __RPC_FAR * This);
        
        U2 ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeCount )( 
            IJavaCls __RPC_FAR * This);
        
        ATTRINFO __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetAttributeList )( 
            IJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConstantString )( 
            IJavaCls __RPC_FAR * This,
            U2 iCPEntry,
            BSTR __RPC_FAR *pbstrString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberName )( 
            IJavaCls __RPC_FAR * This,
            MEMBERINFO __RPC_FAR *pMbrInfo,
            BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberSignature )( 
            IJavaCls __RPC_FAR * This,
            MEMBERINFO __RPC_FAR *pMbrInfo,
            BSTR __RPC_FAR *pbstrSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoesSourceStreamExist )( 
            IJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoesClassStreamExist )( 
            IJavaCls __RPC_FAR * This);
        
        END_INTERFACE
    } IJavaClsVtbl;

    interface IJavaCls
    {
        CONST_VTBL struct IJavaClsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaCls_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaCls_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaCls_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaCls_GetFullName(This,pbstrName)	\
    (This)->lpVtbl -> GetFullName(This,pbstrName)

#define IJavaCls_GetBaseName(This)	\
    (This)->lpVtbl -> GetBaseName(This)

#define IJavaCls_GetClassMoniker(This,pbstrClassMoniker)	\
    (This)->lpVtbl -> GetClassMoniker(This,pbstrClassMoniker)

#define IJavaCls_GetClassStream(This,ppClassStream)	\
    (This)->lpVtbl -> GetClassStream(This,ppClassStream)

#define IJavaCls_GetSourceMoniker(This,pbstrSourceMoniker)	\
    (This)->lpVtbl -> GetSourceMoniker(This,pbstrSourceMoniker)

#define IJavaCls_GetSourceStream(This,ppSourceStream)	\
    (This)->lpVtbl -> GetSourceStream(This,ppSourceStream)

#define IJavaCls_IsOutOfDate(This)	\
    (This)->lpVtbl -> IsOutOfDate(This)

#define IJavaCls_Open(This,fFullOpen)	\
    (This)->lpVtbl -> Open(This,fFullOpen)

#define IJavaCls_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IJavaCls_GetMajorVersion(This)	\
    (This)->lpVtbl -> GetMajorVersion(This)

#define IJavaCls_GetMinorVersion(This)	\
    (This)->lpVtbl -> GetMinorVersion(This)

#define IJavaCls_GetCPCount(This)	\
    (This)->lpVtbl -> GetCPCount(This)

#define IJavaCls_GetCPArray(This)	\
    (This)->lpVtbl -> GetCPArray(This)

#define IJavaCls_GetAccessFlags(This)	\
    (This)->lpVtbl -> GetAccessFlags(This)

#define IJavaCls_GetThisClass(This)	\
    (This)->lpVtbl -> GetThisClass(This)

#define IJavaCls_GetSuperClass(This)	\
    (This)->lpVtbl -> GetSuperClass(This)

#define IJavaCls_GetInterfaceCount(This)	\
    (This)->lpVtbl -> GetInterfaceCount(This)

#define IJavaCls_GetInterfaceArray(This)	\
    (This)->lpVtbl -> GetInterfaceArray(This)

#define IJavaCls_GetFieldCount(This)	\
    (This)->lpVtbl -> GetFieldCount(This)

#define IJavaCls_GetFieldArray(This)	\
    (This)->lpVtbl -> GetFieldArray(This)

#define IJavaCls_GetMethodCount(This)	\
    (This)->lpVtbl -> GetMethodCount(This)

#define IJavaCls_GetMethodArray(This)	\
    (This)->lpVtbl -> GetMethodArray(This)

#define IJavaCls_GetAttributeCount(This)	\
    (This)->lpVtbl -> GetAttributeCount(This)

#define IJavaCls_GetAttributeList(This)	\
    (This)->lpVtbl -> GetAttributeList(This)

#define IJavaCls_GetConstantString(This,iCPEntry,pbstrString)	\
    (This)->lpVtbl -> GetConstantString(This,iCPEntry,pbstrString)

#define IJavaCls_GetMemberName(This,pMbrInfo,pbstrName)	\
    (This)->lpVtbl -> GetMemberName(This,pMbrInfo,pbstrName)

#define IJavaCls_GetMemberSignature(This,pMbrInfo,pbstrSignature)	\
    (This)->lpVtbl -> GetMemberSignature(This,pMbrInfo,pbstrSignature)

#define IJavaCls_DoesSourceStreamExist(This)	\
    (This)->lpVtbl -> DoesSourceStreamExist(This)

#define IJavaCls_DoesClassStreamExist(This)	\
    (This)->lpVtbl -> DoesClassStreamExist(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaCls_GetFullName_Proxy( 
    IJavaCls __RPC_FAR * This,
    BSTR __RPC_FAR *pbstrName);


void __RPC_STUB IJavaCls_GetFullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


const WCHAR __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetBaseName_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetBaseName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetClassMoniker_Proxy( 
    IJavaCls __RPC_FAR * This,
    BSTR __RPC_FAR *pbstrClassMoniker);


void __RPC_STUB IJavaCls_GetClassMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetClassStream_Proxy( 
    IJavaCls __RPC_FAR * This,
    IJavaClsStream __RPC_FAR *__RPC_FAR *ppClassStream);


void __RPC_STUB IJavaCls_GetClassStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetSourceMoniker_Proxy( 
    IJavaCls __RPC_FAR * This,
    BSTR __RPC_FAR *pbstrSourceMoniker);


void __RPC_STUB IJavaCls_GetSourceMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetSourceStream_Proxy( 
    IJavaCls __RPC_FAR * This,
    IStream __RPC_FAR *__RPC_FAR *ppSourceStream);


void __RPC_STUB IJavaCls_GetSourceStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_IsOutOfDate_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_IsOutOfDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_Open_Proxy( 
    IJavaCls __RPC_FAR * This,
    BOOL fFullOpen);


void __RPC_STUB IJavaCls_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IJavaCls_Close_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetMajorVersion_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetMajorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetMinorVersion_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetMinorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetCPCount_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetCPCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


CPOOLENTRY __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetCPArray_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetCPArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetAccessFlags_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetAccessFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetThisClass_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetThisClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetSuperClass_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetSuperClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetInterfaceCount_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetInterfaceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetInterfaceArray_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetInterfaceArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetFieldCount_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetFieldCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


MEMBERINFO __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetFieldArray_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetFieldArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetMethodCount_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetMethodCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


MEMBERINFO __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetMethodArray_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetMethodArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


U2 STDMETHODCALLTYPE IJavaCls_GetAttributeCount_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetAttributeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ATTRINFO __RPC_FAR *STDMETHODCALLTYPE IJavaCls_GetAttributeList_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_GetAttributeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetConstantString_Proxy( 
    IJavaCls __RPC_FAR * This,
    U2 iCPEntry,
    BSTR __RPC_FAR *pbstrString);


void __RPC_STUB IJavaCls_GetConstantString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetMemberName_Proxy( 
    IJavaCls __RPC_FAR * This,
    MEMBERINFO __RPC_FAR *pMbrInfo,
    BSTR __RPC_FAR *pbstrName);


void __RPC_STUB IJavaCls_GetMemberName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_GetMemberSignature_Proxy( 
    IJavaCls __RPC_FAR * This,
    MEMBERINFO __RPC_FAR *pMbrInfo,
    BSTR __RPC_FAR *pbstrSignature);


void __RPC_STUB IJavaCls_GetMemberSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_DoesSourceStreamExist_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_DoesSourceStreamExist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaCls_DoesClassStreamExist_Proxy( 
    IJavaCls __RPC_FAR * This);


void __RPC_STUB IJavaCls_DoesClassStreamExist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaCls_INTERFACE_DEFINED__ */


#ifndef __IJavaPkg_INTERFACE_DEFINED__
#define __IJavaPkg_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaPkg
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IJavaPkg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF2-A46E-11d0-A8BE-00A0C921A4D2")
    IJavaPkg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFullName( 
            BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual const WCHAR __RPC_FAR *STDMETHODCALLTYPE GetBaseName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindPackage( 
            const WCHAR __RPC_FAR *pszPackage,
            IJavaPkg __RPC_FAR *__RPC_FAR *ppFinder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindClass( 
            const WCHAR __RPC_FAR *pszClass,
            IJavaCls __RPC_FAR *__RPC_FAR *ppClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPackages( 
            IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumClasses( 
            IEnumJavaCls __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParentPackage( 
            IJavaPkg __RPC_FAR *__RPC_FAR *ppPkg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaPkgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaPkg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaPkg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaPkg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFullName )( 
            IJavaPkg __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrName);
        
        const WCHAR __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetBaseName )( 
            IJavaPkg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPackage )( 
            IJavaPkg __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszPackage,
            IJavaPkg __RPC_FAR *__RPC_FAR *ppFinder);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindClass )( 
            IJavaPkg __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszClass,
            IJavaCls __RPC_FAR *__RPC_FAR *ppClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumPackages )( 
            IJavaPkg __RPC_FAR * This,
            IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumClasses )( 
            IJavaPkg __RPC_FAR * This,
            IEnumJavaCls __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentPackage )( 
            IJavaPkg __RPC_FAR * This,
            IJavaPkg __RPC_FAR *__RPC_FAR *ppPkg);
        
        END_INTERFACE
    } IJavaPkgVtbl;

    interface IJavaPkg
    {
        CONST_VTBL struct IJavaPkgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaPkg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaPkg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaPkg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaPkg_GetFullName(This,pbstrName)	\
    (This)->lpVtbl -> GetFullName(This,pbstrName)

#define IJavaPkg_GetBaseName(This)	\
    (This)->lpVtbl -> GetBaseName(This)

#define IJavaPkg_FindPackage(This,pszPackage,ppFinder)	\
    (This)->lpVtbl -> FindPackage(This,pszPackage,ppFinder)

#define IJavaPkg_FindClass(This,pszClass,ppClass)	\
    (This)->lpVtbl -> FindClass(This,pszClass,ppClass)

#define IJavaPkg_EnumPackages(This,ppEnum)	\
    (This)->lpVtbl -> EnumPackages(This,ppEnum)

#define IJavaPkg_EnumClasses(This,ppEnum)	\
    (This)->lpVtbl -> EnumClasses(This,ppEnum)

#define IJavaPkg_GetParentPackage(This,ppPkg)	\
    (This)->lpVtbl -> GetParentPackage(This,ppPkg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaPkg_GetFullName_Proxy( 
    IJavaPkg __RPC_FAR * This,
    BSTR __RPC_FAR *pbstrName);


void __RPC_STUB IJavaPkg_GetFullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


const WCHAR __RPC_FAR *STDMETHODCALLTYPE IJavaPkg_GetBaseName_Proxy( 
    IJavaPkg __RPC_FAR * This);


void __RPC_STUB IJavaPkg_GetBaseName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkg_FindPackage_Proxy( 
    IJavaPkg __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszPackage,
    IJavaPkg __RPC_FAR *__RPC_FAR *ppFinder);


void __RPC_STUB IJavaPkg_FindPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkg_FindClass_Proxy( 
    IJavaPkg __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszClass,
    IJavaCls __RPC_FAR *__RPC_FAR *ppClass);


void __RPC_STUB IJavaPkg_FindClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkg_EnumPackages_Proxy( 
    IJavaPkg __RPC_FAR * This,
    IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IJavaPkg_EnumPackages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkg_EnumClasses_Proxy( 
    IJavaPkg __RPC_FAR * This,
    IEnumJavaCls __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IJavaPkg_EnumClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkg_GetParentPackage_Proxy( 
    IJavaPkg __RPC_FAR * This,
    IJavaPkg __RPC_FAR *__RPC_FAR *ppPkg);


void __RPC_STUB IJavaPkg_GetParentPackage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaPkg_INTERFACE_DEFINED__ */


#ifndef __IEnumJavaPkg_INTERFACE_DEFINED__
#define __IEnumJavaPkg_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumJavaPkg
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IEnumJavaPkg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF4-A46E-11d0-A8BE-00A0C921A4D2")
    IEnumJavaPkg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            ULONG celt,
            IJavaPkg __RPC_FAR *__RPC_FAR *rgelt,
            ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumJavaPkgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumJavaPkg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumJavaPkg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumJavaPkg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumJavaPkg __RPC_FAR * This,
            ULONG celt,
            IJavaPkg __RPC_FAR *__RPC_FAR *rgelt,
            ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumJavaPkg __RPC_FAR * This,
            ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumJavaPkg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumJavaPkg __RPC_FAR * This,
            IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumJavaPkgVtbl;

    interface IEnumJavaPkg
    {
        CONST_VTBL struct IEnumJavaPkgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumJavaPkg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumJavaPkg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumJavaPkg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumJavaPkg_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumJavaPkg_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumJavaPkg_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumJavaPkg_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumJavaPkg_Next_Proxy( 
    IEnumJavaPkg __RPC_FAR * This,
    ULONG celt,
    IJavaPkg __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumJavaPkg_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPkg_Skip_Proxy( 
    IEnumJavaPkg __RPC_FAR * This,
    ULONG celt);


void __RPC_STUB IEnumJavaPkg_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPkg_Reset_Proxy( 
    IEnumJavaPkg __RPC_FAR * This);


void __RPC_STUB IEnumJavaPkg_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaPkg_Clone_Proxy( 
    IEnumJavaPkg __RPC_FAR * This,
    IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumJavaPkg_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumJavaPkg_INTERFACE_DEFINED__ */


#ifndef __IEnumJavaCls_INTERFACE_DEFINED__
#define __IEnumJavaCls_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumJavaCls
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IEnumJavaCls;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF5-A46E-11d0-A8BE-00A0C921A4D2")
    IEnumJavaCls : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            ULONG celt,
            IJavaCls __RPC_FAR *__RPC_FAR *rgelt,
            ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            IEnumJavaCls __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumJavaClsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumJavaCls __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumJavaCls __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumJavaCls __RPC_FAR * This,
            ULONG celt,
            IJavaCls __RPC_FAR *__RPC_FAR *rgelt,
            ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumJavaCls __RPC_FAR * This,
            ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumJavaCls __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumJavaCls __RPC_FAR * This,
            IEnumJavaCls __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumJavaClsVtbl;

    interface IEnumJavaCls
    {
        CONST_VTBL struct IEnumJavaClsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumJavaCls_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumJavaCls_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumJavaCls_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumJavaCls_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumJavaCls_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumJavaCls_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumJavaCls_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumJavaCls_Next_Proxy( 
    IEnumJavaCls __RPC_FAR * This,
    ULONG celt,
    IJavaCls __RPC_FAR *__RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumJavaCls_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaCls_Skip_Proxy( 
    IEnumJavaCls __RPC_FAR * This,
    ULONG celt);


void __RPC_STUB IEnumJavaCls_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaCls_Reset_Proxy( 
    IEnumJavaCls __RPC_FAR * This);


void __RPC_STUB IEnumJavaCls_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumJavaCls_Clone_Proxy( 
    IEnumJavaCls __RPC_FAR * This,
    IEnumJavaCls __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumJavaCls_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumJavaCls_INTERFACE_DEFINED__ */


#ifndef __IJavaPkgService_INTERFACE_DEFINED__
#define __IJavaPkgService_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaPkgService
 * at Wed Jun 18 20:10:23 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IJavaPkgService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("57771EF1-A46E-11d0-A8BE-00A0C921A4D2")
    IJavaPkgService : public IJavaPkg
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetClassPath( 
            const WCHAR __RPC_FAR *pszClassPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppendClassPath( 
            const WCHAR __RPC_FAR *pszAppendPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrependClassPath( 
            const WCHAR __RPC_FAR *pszPrependPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveClassPath( 
            const WCHAR __RPC_FAR *pszRemovePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassPath( 
            BSTR __RPC_FAR *pbstrClassPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCurrentDirectory( 
            const WCHAR __RPC_FAR *pszCurDir) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockArchives( 
            BOOL fLock) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaPkgServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaPkgService __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaPkgService __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaPkgService __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFullName )( 
            IJavaPkgService __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrName);
        
        const WCHAR __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetBaseName )( 
            IJavaPkgService __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindPackage )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszPackage,
            IJavaPkg __RPC_FAR *__RPC_FAR *ppFinder);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindClass )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszClass,
            IJavaCls __RPC_FAR *__RPC_FAR *ppClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumPackages )( 
            IJavaPkgService __RPC_FAR * This,
            IEnumJavaPkg __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumClasses )( 
            IJavaPkgService __RPC_FAR * This,
            IEnumJavaCls __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentPackage )( 
            IJavaPkgService __RPC_FAR * This,
            IJavaPkg __RPC_FAR *__RPC_FAR *ppPkg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClassPath )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszClassPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendClassPath )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszAppendPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PrependClassPath )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszPrependPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveClassPath )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszRemovePath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassPath )( 
            IJavaPkgService __RPC_FAR * This,
            BSTR __RPC_FAR *pbstrClassPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCurrentDirectory )( 
            IJavaPkgService __RPC_FAR * This,
            const WCHAR __RPC_FAR *pszCurDir);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockArchives )( 
            IJavaPkgService __RPC_FAR * This,
            BOOL fLock);
        
        END_INTERFACE
    } IJavaPkgServiceVtbl;

    interface IJavaPkgService
    {
        CONST_VTBL struct IJavaPkgServiceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaPkgService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaPkgService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaPkgService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaPkgService_GetFullName(This,pbstrName)	\
    (This)->lpVtbl -> GetFullName(This,pbstrName)

#define IJavaPkgService_GetBaseName(This)	\
    (This)->lpVtbl -> GetBaseName(This)

#define IJavaPkgService_FindPackage(This,pszPackage,ppFinder)	\
    (This)->lpVtbl -> FindPackage(This,pszPackage,ppFinder)

#define IJavaPkgService_FindClass(This,pszClass,ppClass)	\
    (This)->lpVtbl -> FindClass(This,pszClass,ppClass)

#define IJavaPkgService_EnumPackages(This,ppEnum)	\
    (This)->lpVtbl -> EnumPackages(This,ppEnum)

#define IJavaPkgService_EnumClasses(This,ppEnum)	\
    (This)->lpVtbl -> EnumClasses(This,ppEnum)

#define IJavaPkgService_GetParentPackage(This,ppPkg)	\
    (This)->lpVtbl -> GetParentPackage(This,ppPkg)


#define IJavaPkgService_SetClassPath(This,pszClassPath)	\
    (This)->lpVtbl -> SetClassPath(This,pszClassPath)

#define IJavaPkgService_AppendClassPath(This,pszAppendPath)	\
    (This)->lpVtbl -> AppendClassPath(This,pszAppendPath)

#define IJavaPkgService_PrependClassPath(This,pszPrependPath)	\
    (This)->lpVtbl -> PrependClassPath(This,pszPrependPath)

#define IJavaPkgService_RemoveClassPath(This,pszRemovePath)	\
    (This)->lpVtbl -> RemoveClassPath(This,pszRemovePath)

#define IJavaPkgService_GetClassPath(This,pbstrClassPath)	\
    (This)->lpVtbl -> GetClassPath(This,pbstrClassPath)

#define IJavaPkgService_SetCurrentDirectory(This,pszCurDir)	\
    (This)->lpVtbl -> SetCurrentDirectory(This,pszCurDir)

#define IJavaPkgService_LockArchives(This,fLock)	\
    (This)->lpVtbl -> LockArchives(This,fLock)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaPkgService_SetClassPath_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszClassPath);


void __RPC_STUB IJavaPkgService_SetClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_AppendClassPath_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszAppendPath);


void __RPC_STUB IJavaPkgService_AppendClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_PrependClassPath_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszPrependPath);


void __RPC_STUB IJavaPkgService_PrependClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_RemoveClassPath_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszRemovePath);


void __RPC_STUB IJavaPkgService_RemoveClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_GetClassPath_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    BSTR __RPC_FAR *pbstrClassPath);


void __RPC_STUB IJavaPkgService_GetClassPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_SetCurrentDirectory_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    const WCHAR __RPC_FAR *pszCurDir);


void __RPC_STUB IJavaPkgService_SetCurrentDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaPkgService_LockArchives_Proxy( 
    IJavaPkgService __RPC_FAR * This,
    BOOL fLock);


void __RPC_STUB IJavaPkgService_LockArchives_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaPkgService_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_JavaPkgService;

class DECLSPEC_UUID("57771EF6-A46E-11d0-A8BE-00A0C921A4D2")
JavaPkgService;
#endif
#endif /* __JavaPackageService_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
