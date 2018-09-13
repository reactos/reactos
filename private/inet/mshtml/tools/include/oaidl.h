/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.39 */
/* at Sat Jul 13 21:56:52 1996
 */
/* Compiler settings for c:\oa\src\idl\oaidl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __oaidl_h__
#define __oaidl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICreateTypeInfo_FWD_DEFINED__
#define __ICreateTypeInfo_FWD_DEFINED__
typedef interface ICreateTypeInfo ICreateTypeInfo;
#endif 	/* __ICreateTypeInfo_FWD_DEFINED__ */


#ifndef __ICreateTypeInfo2_FWD_DEFINED__
#define __ICreateTypeInfo2_FWD_DEFINED__
typedef interface ICreateTypeInfo2 ICreateTypeInfo2;
#endif 	/* __ICreateTypeInfo2_FWD_DEFINED__ */


#ifndef __ICreateTypeLib_FWD_DEFINED__
#define __ICreateTypeLib_FWD_DEFINED__
typedef interface ICreateTypeLib ICreateTypeLib;
#endif 	/* __ICreateTypeLib_FWD_DEFINED__ */


#ifndef __ICreateTypeLib2_FWD_DEFINED__
#define __ICreateTypeLib2_FWD_DEFINED__
typedef interface ICreateTypeLib2 ICreateTypeLib2;
#endif 	/* __ICreateTypeLib2_FWD_DEFINED__ */


#ifndef __IDispatch_FWD_DEFINED__
#define __IDispatch_FWD_DEFINED__
typedef interface IDispatch IDispatch;
#endif 	/* __IDispatch_FWD_DEFINED__ */


#ifndef __IEnumVARIANT_FWD_DEFINED__
#define __IEnumVARIANT_FWD_DEFINED__
typedef interface IEnumVARIANT IEnumVARIANT;
#endif 	/* __IEnumVARIANT_FWD_DEFINED__ */


#ifndef __ITypeComp_FWD_DEFINED__
#define __ITypeComp_FWD_DEFINED__
typedef interface ITypeComp ITypeComp;
#endif 	/* __ITypeComp_FWD_DEFINED__ */


#ifndef __ITypeInfo_FWD_DEFINED__
#define __ITypeInfo_FWD_DEFINED__
typedef interface ITypeInfo ITypeInfo;
#endif 	/* __ITypeInfo_FWD_DEFINED__ */


#ifndef __ITypeInfo2_FWD_DEFINED__
#define __ITypeInfo2_FWD_DEFINED__
typedef interface ITypeInfo2 ITypeInfo2;
#endif 	/* __ITypeInfo2_FWD_DEFINED__ */


#ifndef __ITypeLib_FWD_DEFINED__
#define __ITypeLib_FWD_DEFINED__
typedef interface ITypeLib ITypeLib;
#endif 	/* __ITypeLib_FWD_DEFINED__ */


#ifndef __ITypeLib2_FWD_DEFINED__
#define __ITypeLib2_FWD_DEFINED__
typedef interface ITypeLib2 ITypeLib2;
#endif 	/* __ITypeLib2_FWD_DEFINED__ */


#ifndef __ITypeChangeEvents_FWD_DEFINED__
#define __ITypeChangeEvents_FWD_DEFINED__
typedef interface ITypeChangeEvents ITypeChangeEvents;
#endif 	/* __ITypeChangeEvents_FWD_DEFINED__ */


#ifndef __IErrorInfo_FWD_DEFINED__
#define __IErrorInfo_FWD_DEFINED__
typedef interface IErrorInfo IErrorInfo;
#endif 	/* __IErrorInfo_FWD_DEFINED__ */


#ifndef __ICreateErrorInfo_FWD_DEFINED__
#define __ICreateErrorInfo_FWD_DEFINED__
typedef interface ICreateErrorInfo ICreateErrorInfo;
#endif 	/* __ICreateErrorInfo_FWD_DEFINED__ */


#ifndef __ISupportErrorInfo_FWD_DEFINED__
#define __ISupportErrorInfo_FWD_DEFINED__
typedef interface ISupportErrorInfo ISupportErrorInfo;
#endif 	/* __ISupportErrorInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local] */ 


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//--------------------------------------------------------------------------

















extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IOleAutomationTypes_INTERFACE_DEFINED__
#define __IOleAutomationTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOleAutomationTypes
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [auto_handle][unique][version] */ 


typedef CY CURRENCY;

typedef struct  tagSAFEARRAYBOUND
    {
    ULONG cElements;
    LONG lLbound;
    }	SAFEARRAYBOUND;

typedef struct tagSAFEARRAYBOUND __RPC_FAR *LPSAFEARRAYBOUND;

#if defined(_OLEAUT32_)
/* the following is what MIDL knows how to remote */
typedef /* [unique] */ struct _wireVARIANT __RPC_FAR *wireVARIANT;

typedef struct  _wireSAFEARR_BSTR
    {
    ULONG Size;
    /* [ref][size_is] */ wireBSTR __RPC_FAR *aBstr;
    }	SAFEARR_BSTR;

typedef struct  _wireSAFEARR_UNKNOWN
    {
    ULONG Size;
    /* [ref][size_is] */ IUnknown __RPC_FAR *__RPC_FAR *apUnknown;
    }	SAFEARR_UNKNOWN;

typedef struct  _wireSAFEARR_DISPATCH
    {
    ULONG Size;
    /* [ref][size_is] */ IDispatch __RPC_FAR *__RPC_FAR *apDispatch;
    }	SAFEARR_DISPATCH;

typedef struct  _wireSAFEARR_VARIANT
    {
    ULONG Size;
    /* [ref][size_is] */ wireVARIANT __RPC_FAR *aVariant;
    }	SAFEARR_VARIANT;

typedef /* [v1_enum] */ 
enum tagSF_TYPE
    {	SF_ERROR	= VT_ERROR,
	SF_I1	= VT_I1,
	SF_I2	= VT_I2,
	SF_I4	= VT_I4,
	SF_I8	= VT_I8,
	SF_BSTR	= VT_BSTR,
	SF_UNKNOWN	= VT_UNKNOWN,
	SF_DISPATCH	= VT_DISPATCH,
	SF_VARIANT	= VT_VARIANT
    }	SF_TYPE;

typedef struct  _wireSAFEARRAY_UNION
    {
    ULONG sfType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IOleAutomationTypes_0001
        {
        /* [case()] */ SAFEARR_BSTR BstrStr;
        /* [case()] */ SAFEARR_UNKNOWN UnknownStr;
        /* [case()] */ SAFEARR_DISPATCH DispatchStr;
        /* [case()] */ SAFEARR_VARIANT VariantStr;
        /* [case()] */ BYTE_SIZEDARR ByteStr;
        /* [case()] */ WORD_SIZEDARR WordStr;
        /* [case()] */ DWORD_SIZEDARR LongStr;
        /* [case()] */ HYPER_SIZEDARR HyperStr;
        /* [default] */  /* Empty union arm */ 
        }	u;
    }	SAFEARRAYUNION;

typedef /* [unique] */ struct  _wireSAFEARRAY
    {
    USHORT cDims;
    USHORT fFeatures;
    ULONG cbElements;
    ULONG cLocks;
    SAFEARRAYUNION uArrayStructs;
    /* [size_is] */ SAFEARRAYBOUND rgsabound[ 1 ];
    }	__RPC_FAR *wireSAFEARRAY;

typedef /* [unique] */ wireSAFEARRAY __RPC_FAR *wirePSAFEARRAY;

#endif /* _OLEAUT32_ */
typedef struct  tagSAFEARRAY
    {
    USHORT cDims;
    USHORT fFeatures;
    ULONG cbElements;
    ULONG cLocks;
    PVOID pvData;
    SAFEARRAYBOUND rgsabound[ 1 ];
    }	SAFEARRAY;

typedef /* [wire_marshal] */ SAFEARRAY __RPC_FAR *LPSAFEARRAY;

#define	FADF_AUTO	( 0x1 )

#define	FADF_STATIC	( 0x2 )

#define	FADF_EMBEDDED	( 0x4 )

#define	FADF_FIXEDSIZE	( 0x10 )

#define	FADF_BSTR	( 0x100 )

#define	FADF_UNKNOWN	( 0x200 )

#define	FADF_DISPATCH	( 0x400 )

#define	FADF_VARIANT	( 0x800 )

#define	FADF_RESERVED	( 0xf0e8 )

/* VARIANT STRUCTURE
 *
 *  VARTYPE vt;
 *  WORD wReserved1;
 *  WORD wReserved2;
 *  WORD wReserved3;
 *  union {
 *    LONG           VT_I4
 *    BYTE           VT_UI1
 *    SHORT          VT_I2
 *    FLOAT          VT_R4
 *    DOUBLE         VT_R8
 *    VARIANT_BOOL   VT_BOOL
 *    SCODE          VT_ERROR
 *    CY             VT_CY
 *    DATE           VT_DATE
 *    BSTR           VT_BSTR
 *    IUnknown *     VT_UNKNOWN
 *    IDispatch *    VT_DISPATCH
 *    SAFEARRAY *    VT_ARRAY
 *    BYTE *         VT_BYREF|VT_UI1
 *    SHORT *        VT_BYREF|VT_I2
 *    LONG *         VT_BYREF|VT_I4
 *    FLOAT *        VT_BYREF|VT_R4
 *    DOUBLE *       VT_BYREF|VT_R8
 *    VARIANT_BOOL * VT_BYREF|VT_BOOL
 *    SCODE *        VT_BYREF|VT_ERROR
 *    CY *           VT_BYREF|VT_CY
 *    DATE *         VT_BYREF|VT_DATE
 *    BSTR *         VT_BYREF|VT_BSTR
 *    IUnknown **    VT_BYREF|VT_UNKNOWN
 *    IDispatch **   VT_BYREF|VT_DISPATCH
 *    SAFEARRAY **   VT_BYREF|VT_ARRAY
 *    VARIANT *      VT_BYREF|VT_VARIANT
 *    PVOID          VT_BYREF (Generic ByRef)
 *    CHAR           VT_I1
 *    USHORT         VT_UI2
 *    ULONG          VT_UI4
 *    INT            VT_INT
 *    UINT           VT_UINT
 *    DECIMAL *      VT_BYREF|VT_DECIMAL
 *    CHAR *         VT_BYREF|VT_I1
 *    USHORT *       VT_BYREF|VT_UI2
 *    ULONG *        VT_BYREF|VT_UI4
 *    INT *          VT_BYREF|VT_INT
 *    UINT *         VT_BYREF|VT_UINT
 *  }
 */
#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
#define __VARIANT_NAME_1 n1
#define __VARIANT_NAME_2 n2
#define __VARIANT_NAME_3 n3
#else
#define __tagVARIANT
#define __VARIANT_NAME_1
#define __VARIANT_NAME_2
#define __VARIANT_NAME_3
#endif
typedef /* [wire_marshal] */ struct tagVARIANT VARIANT;

struct  tagVARIANT
    {
    union 
        {
        struct  __tagVARIANT
            {
            VARTYPE vt;
            WORD wReserved1;
            WORD wReserved2;
            WORD wReserved3;
            union 
                {
                LONG lVal;
                BYTE bVal;
                SHORT iVal;
                FLOAT fltVal;
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                _VARIANT_BOOL bool;
                SCODE scode;
                CY cyVal;
                DATE date;
                BSTR bstrVal;
                IUnknown __RPC_FAR *punkVal;
                IDispatch __RPC_FAR *pdispVal;
                SAFEARRAY __RPC_FAR *parray;
                BYTE __RPC_FAR *pbVal;
                SHORT __RPC_FAR *piVal;
                LONG __RPC_FAR *plVal;
                FLOAT __RPC_FAR *pfltVal;
                DOUBLE __RPC_FAR *pdblVal;
                VARIANT_BOOL __RPC_FAR *pboolVal;
                _VARIANT_BOOL __RPC_FAR *pbool;
                SCODE __RPC_FAR *pscode;
                CY __RPC_FAR *pcyVal;
                DATE __RPC_FAR *pdate;
                BSTR __RPC_FAR *pbstrVal;
                IUnknown __RPC_FAR *__RPC_FAR *ppunkVal;
                IDispatch __RPC_FAR *__RPC_FAR *ppdispVal;
                SAFEARRAY __RPC_FAR *__RPC_FAR *pparray;
                VARIANT __RPC_FAR *pvarVal;
                PVOID byref;
                CHAR cVal;
                USHORT uiVal;
                ULONG ulVal;
                INT intVal;
                UINT uintVal;
                DECIMAL __RPC_FAR *pdecVal;
                CHAR __RPC_FAR *pcVal;
                USHORT __RPC_FAR *puiVal;
                ULONG __RPC_FAR *pulVal;
                INT __RPC_FAR *pintVal;
                UINT __RPC_FAR *puintVal;
                }	__VARIANT_NAME_3;
            }	__VARIANT_NAME_2;
        DECIMAL decVal;
        }	__VARIANT_NAME_1;
    };
typedef VARIANT __RPC_FAR *LPVARIANT;

typedef VARIANT VARIANTARG;

typedef VARIANT __RPC_FAR *LPVARIANTARG;

#if defined(_OLEAUT32_)
/* the following is what MIDL knows how to remote */
struct  _wireVARIANT
    {
    DWORD clSize;
    DWORD rpcReserved;
    USHORT vt;
    USHORT wReserved1;
    USHORT wReserved2;
    USHORT wReserved3;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ LONG lVal;
        /* [case()] */ BYTE bVal;
        /* [case()] */ SHORT iVal;
        /* [case()] */ FLOAT fltVal;
        /* [case()] */ DOUBLE dblVal;
        /* [case()] */ VARIANT_BOOL boolVal;
        /* [case()] */ SCODE scode;
        /* [case()] */ CY cyVal;
        /* [case()] */ DATE date;
        /* [case()] */ wireBSTR bstrVal;
        /* [case()] */ IUnknown __RPC_FAR *punkVal;
        /* [case()] */ IDispatch __RPC_FAR *pdispVal;
        /* [case()] */ wireSAFEARRAY parray;
        /* [case()] */ BYTE __RPC_FAR *pbVal;
        /* [case()] */ SHORT __RPC_FAR *piVal;
        /* [case()] */ LONG __RPC_FAR *plVal;
        /* [case()] */ FLOAT __RPC_FAR *pfltVal;
        /* [case()] */ DOUBLE __RPC_FAR *pdblVal;
        /* [case()] */ VARIANT_BOOL __RPC_FAR *pboolVal;
        /* [case()] */ SCODE __RPC_FAR *pscode;
        /* [case()] */ CY __RPC_FAR *pcyVal;
        /* [case()] */ DATE __RPC_FAR *pdate;
        /* [case()] */ wireBSTR __RPC_FAR *pbstrVal;
        /* [case()] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkVal;
        /* [case()] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispVal;
        /* [case()] */ wireSAFEARRAY __RPC_FAR *pparray;
        /* [case()] */ wireVARIANT __RPC_FAR *pvarVal;
        /* [case()] */ CHAR cVal;
        /* [case()] */ USHORT uiVal;
        /* [case()] */ ULONG ulVal;
        /* [case()] */ INT intVal;
        /* [case()] */ UINT uintVal;
        /* [case()] */ DECIMAL decVal;
        /* [case()] */ DECIMAL __RPC_FAR *pdecVal;
        /* [case()] */ CHAR __RPC_FAR *pcVal;
        /* [case()] */ USHORT __RPC_FAR *puiVal;
        /* [case()] */ ULONG __RPC_FAR *pulVal;
        /* [case()] */ INT __RPC_FAR *pintVal;
        /* [case()] */ UINT __RPC_FAR *puintVal;
        /* [case()] */  /* Empty union arm */ 
        /* [case()] */  /* Empty union arm */ 
        }	;
    };
#endif /* _OLEAUT32_ */
typedef LONG DISPID;

typedef DISPID MEMBERID;

typedef DWORD HREFTYPE;

typedef /* [v1_enum] */ 
enum tagTYPEKIND
    {	TKIND_ENUM	= 0,
	TKIND_RECORD	= TKIND_ENUM + 1,
	TKIND_MODULE	= TKIND_RECORD + 1,
	TKIND_INTERFACE	= TKIND_MODULE + 1,
	TKIND_DISPATCH	= TKIND_INTERFACE + 1,
	TKIND_COCLASS	= TKIND_DISPATCH + 1,
	TKIND_ALIAS	= TKIND_COCLASS + 1,
	TKIND_UNION	= TKIND_ALIAS + 1,
	TKIND_MAX	= TKIND_UNION + 1
    }	TYPEKIND;

typedef struct  tagTYPEDESC
    {
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ struct tagTYPEDESC __RPC_FAR *lptdesc;
        /* [case()] */ struct tagARRAYDESC __RPC_FAR *lpadesc;
        /* [case()] */ HREFTYPE hreftype;
        /* [default] */  /* Empty union arm */ 
        }	;
    VARTYPE vt;
    }	TYPEDESC;

typedef struct  tagARRAYDESC
    {
    TYPEDESC tdescElem;
    USHORT cDims;
    /* [size_is] */ SAFEARRAYBOUND rgbounds[ 1 ];
    }	ARRAYDESC;

typedef struct  tagPARAMDESCEX
    {
    ULONG cBytes;
    VARIANTARG varDefaultValue;
    }	PARAMDESCEX;

typedef struct tagPARAMDESCEX __RPC_FAR *LPPARAMDESCEX;

typedef struct  tagPARAMDESC
    {
    LPPARAMDESCEX pparamdescex;
    USHORT wParamFlags;
    }	PARAMDESC;

typedef struct tagPARAMDESC __RPC_FAR *LPPARAMDESC;

#define	PARAMFLAG_NONE	( 0 )

#define	PARAMFLAG_FIN	( 0x1 )

#define	PARAMFLAG_FOUT	( 0x2 )

#define	PARAMFLAG_FLCID	( 0x4 )

#define	PARAMFLAG_FRETVAL	( 0x8 )

#define	PARAMFLAG_FOPT	( 0x10 )

#define	PARAMFLAG_FHASDEFAULT	( 0x20 )

typedef struct  tagIDLDESC
    {
    ULONG dwReserved;
    USHORT wIDLFlags;
    }	IDLDESC;

typedef struct tagIDLDESC __RPC_FAR *LPIDLDESC;

#define	IDLFLAG_NONE	( PARAMFLAG_NONE )

#define	IDLFLAG_FIN	( PARAMFLAG_FIN )

#define	IDLFLAG_FOUT	( PARAMFLAG_FOUT )

#define	IDLFLAG_FLCID	( PARAMFLAG_FLCID )

#define	IDLFLAG_FRETVAL	( PARAMFLAG_FRETVAL )

#if 0
/* the following is what MIDL knows how to remote */
typedef struct  tagELEMDESC
    {
    TYPEDESC tdesc;
    PARAMDESC paramdesc;
    }	ELEMDESC;

#else /* 0 */
typedef struct tagELEMDESC {
    TYPEDESC tdesc;             /* the type of the element */
    union {
        IDLDESC idldesc;        /* info for remoting the element */
        PARAMDESC paramdesc;    /* info about the parameter */
    };
} ELEMDESC, * LPELEMDESC;
#endif /* 0 */
typedef struct  tagTYPEATTR
    {
    GUID guid;
    LCID lcid;
    DWORD dwReserved;
    MEMBERID memidConstructor;
    MEMBERID memidDestructor;
    LPOLESTR lpstrSchema;
    ULONG cbSizeInstance;
    TYPEKIND typekind;
    WORD cFuncs;
    WORD cVars;
    WORD cImplTypes;
    WORD cbSizeVft;
    WORD cbAlignment;
    WORD wTypeFlags;
    WORD wMajorVerNum;
    WORD wMinorVerNum;
    TYPEDESC tdescAlias;
    IDLDESC idldescType;
    }	TYPEATTR;

typedef struct tagTYPEATTR __RPC_FAR *LPTYPEATTR;

typedef struct  tagDISPPARAMS
    {
    /* [size_is] */ VARIANTARG __RPC_FAR *rgvarg;
    /* [size_is] */ DISPID __RPC_FAR *rgdispidNamedArgs;
    UINT cArgs;
    UINT cNamedArgs;
    }	DISPPARAMS;

#if 0
/* the following is what MIDL knows how to remote */
typedef struct  tagEXCEPINFO
    {
    WORD wCode;
    WORD wReserved;
    BSTR bstrSource;
    BSTR bstrDescription;
    BSTR bstrHelpFile;
    DWORD dwHelpContext;
    ULONG pvReserved;
    ULONG pfnDeferredFillIn;
    SCODE scode;
    }	EXCEPINFO;

#else /* 0 */
typedef struct tagEXCEPINFO {
    WORD  wCode;
    WORD  wReserved;
    BSTR  bstrSource;
    BSTR  bstrDescription;
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    PVOID pvReserved;
    HRESULT (__stdcall *pfnDeferredFillIn)(struct tagEXCEPINFO *);
    SCODE scode;
} EXCEPINFO, * LPEXCEPINFO;
#endif /* 0 */
typedef /* [v1_enum] */ 
enum tagCALLCONV
    {	CC_FASTCALL	= 0,
	CC_CDECL	= 1,
	CC_MSCPASCAL	= CC_CDECL + 1,
	CC_PASCAL	= CC_MSCPASCAL,
	CC_MACPASCAL	= CC_PASCAL + 1,
	CC_STDCALL	= CC_MACPASCAL + 1,
	CC_FPFASTCALL	= CC_STDCALL + 1,
	CC_SYSCALL	= CC_FPFASTCALL + 1,
	CC_MPWCDECL	= CC_SYSCALL + 1,
	CC_MPWPASCAL	= CC_MPWCDECL + 1,
	CC_MAX	= CC_MPWPASCAL + 1
    }	CALLCONV;

typedef /* [v1_enum] */ 
enum tagFUNCKIND
    {	FUNC_VIRTUAL	= 0,
	FUNC_PUREVIRTUAL	= FUNC_VIRTUAL + 1,
	FUNC_NONVIRTUAL	= FUNC_PUREVIRTUAL + 1,
	FUNC_STATIC	= FUNC_NONVIRTUAL + 1,
	FUNC_DISPATCH	= FUNC_STATIC + 1
    }	FUNCKIND;

typedef /* [v1_enum] */ 
enum tagINVOKEKIND
    {	INVOKE_FUNC	= 1,
	INVOKE_PROPERTYGET	= 2,
	INVOKE_PROPERTYPUT	= 4,
	INVOKE_PROPERTYPUTREF	= 8
    }	INVOKEKIND;

typedef struct  tagFUNCDESC
    {
    MEMBERID memid;
    /* [size_is] */ SCODE __RPC_FAR *lprgscode;
    /* [size_is] */ ELEMDESC __RPC_FAR *lprgelemdescParam;
    FUNCKIND funckind;
    INVOKEKIND invkind;
    CALLCONV callconv;
    SHORT cParams;
    SHORT cParamsOpt;
    SHORT oVft;
    SHORT cScodes;
    ELEMDESC elemdescFunc;
    WORD wFuncFlags;
    }	FUNCDESC;

typedef struct tagFUNCDESC __RPC_FAR *LPFUNCDESC;

typedef /* [v1_enum] */ 
enum tagVARKIND
    {	VAR_PERINSTANCE	= 0,
	VAR_STATIC	= VAR_PERINSTANCE + 1,
	VAR_CONST	= VAR_STATIC + 1,
	VAR_DISPATCH	= VAR_CONST + 1
    }	VARKIND;

#define	IMPLTYPEFLAG_FDEFAULT	( 0x1 )

#define	IMPLTYPEFLAG_FSOURCE	( 0x2 )

#define	IMPLTYPEFLAG_FRESTRICTED	( 0x4 )

#define	IMPLTYPEFLAG_FDEFAULTVTABLE	( 0x8 )

typedef struct  tagVARDESC
    {
    MEMBERID memid;
    LPOLESTR lpstrSchema;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ ULONG oInst;
        /* [case()] */ VARIANT __RPC_FAR *lpvarValue;
        }	;
    ELEMDESC elemdescVar;
    WORD wVarFlags;
    VARKIND varkind;
    }	VARDESC;

typedef struct tagVARDESC __RPC_FAR *LPVARDESC;

typedef 
enum tagTYPEFLAGS
    {	TYPEFLAG_FAPPOBJECT	= 0x1,
	TYPEFLAG_FCANCREATE	= 0x2,
	TYPEFLAG_FLICENSED	= 0x4,
	TYPEFLAG_FPREDECLID	= 0x8,
	TYPEFLAG_FHIDDEN	= 0x10,
	TYPEFLAG_FCONTROL	= 0x20,
	TYPEFLAG_FDUAL	= 0x40,
	TYPEFLAG_FNONEXTENSIBLE	= 0x80,
	TYPEFLAG_FOLEAUTOMATION	= 0x100,
	TYPEFLAG_FRESTRICTED	= 0x200,
	TYPEFLAG_FAGGREGATABLE	= 0x400,
	TYPEFLAG_FREPLACEABLE	= 0x800,
	TYPEFLAG_FDISPATCHABLE	= 0x1000,
	TYPEFLAG_FREVERSEBIND	= 0x2000
    }	TYPEFLAGS;

typedef 
enum tagFUNCFLAGS
    {	FUNCFLAG_FRESTRICTED	= 0x1,
	FUNCFLAG_FSOURCE	= 0x2,
	FUNCFLAG_FBINDABLE	= 0x4,
	FUNCFLAG_FREQUESTEDIT	= 0x8,
	FUNCFLAG_FDISPLAYBIND	= 0x10,
	FUNCFLAG_FDEFAULTBIND	= 0x20,
	FUNCFLAG_FHIDDEN	= 0x40,
	FUNCFLAG_FUSESGETLASTERROR	= 0x80,
	FUNCFLAG_FDEFAULTCOLLELEM	= 0x100,
	FUNCFLAG_FUIDEFAULT	= 0x200,
	FUNCFLAG_FNONBROWSABLE	= 0x400,
	FUNCFLAG_FREPLACEABLE	= 0x800,
	FUNCFLAG_FIMMEDIATEBIND	= 0x1000
    }	FUNCFLAGS;

typedef 
enum tagVARFLAGS
    {	VARFLAG_FREADONLY	= 0x1,
	VARFLAG_FSOURCE	= 0x2,
	VARFLAG_FBINDABLE	= 0x4,
	VARFLAG_FREQUESTEDIT	= 0x8,
	VARFLAG_FDISPLAYBIND	= 0x10,
	VARFLAG_FDEFAULTBIND	= 0x20,
	VARFLAG_FHIDDEN	= 0x40,
	VARFLAG_FRESTRICTED	= 0x80,
	VARFLAG_FDEFAULTCOLLELEM	= 0x100,
	VARFLAG_FUIDEFAULT	= 0x200,
	VARFLAG_FNONBROWSABLE	= 0x400,
	VARFLAG_FREPLACEABLE	= 0x800,
	VARFLAG_FIMMEDIATEBIND	= 0x1000
    }	VARFLAGS;

typedef /* [wire_marshal] */ struct  tagCLEANLOCALSTORAGE
    {
    IUnknown __RPC_FAR *pInterface;
    PVOID pStorage;
    DWORD flags;
    }	CLEANLOCALSTORAGE;

typedef struct  tagCUSTDATAITEM
    {
    GUID guid;
    VARIANTARG varValue;
    }	CUSTDATAITEM;

typedef struct tagCUSTDATAITEM __RPC_FAR *LPCUSTDATAITEM;

typedef struct  tagCUSTDATA
    {
    DWORD cCustData;
    /* [size_is] */ LPCUSTDATAITEM prgCustData;
    }	CUSTDATA;

typedef struct tagCUSTDATA __RPC_FAR *LPCUSTDATA;



extern RPC_IF_HANDLE IOleAutomationTypes_v1_0_c_ifspec;
extern RPC_IF_HANDLE IOleAutomationTypes_v1_0_s_ifspec;
#endif /* __IOleAutomationTypes_INTERFACE_DEFINED__ */

#ifndef __ICreateTypeInfo_INTERFACE_DEFINED__
#define __ICreateTypeInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateTypeInfo
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ICreateTypeInfo __RPC_FAR *LPCREATETYPEINFO;


EXTERN_C const IID IID_ICreateTypeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateTypeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGuid( 
            /* [in] */ REFGUID guid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTypeFlags( 
            /* [in] */ UINT uTypeFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDocString( 
            /* [in] */ LPOLESTR pStrDoc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpContext( 
            /* [in] */ DWORD dwHelpContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVersion( 
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRefTypeInfo( 
            /* [in] */ ITypeInfo __RPC_FAR *pTInfo,
            /* [in] */ HREFTYPE __RPC_FAR *phRefType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFuncDesc( 
            /* [in] */ UINT index,
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddImplType( 
            /* [in] */ UINT index,
            /* [in] */ HREFTYPE hRefType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImplTypeFlags( 
            /* [in] */ UINT index,
            /* [in] */ INT implTypeFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAlignment( 
            /* [in] */ WORD cbAlignment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSchema( 
            /* [in] */ LPOLESTR pStrSchema) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddVarDesc( 
            /* [in] */ UINT index,
            /* [in] */ VARDESC __RPC_FAR *pVarDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFuncAndParamNames( 
            /* [in] */ UINT index,
            /* [in][size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVarName( 
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTypeDescAlias( 
            /* [in] */ TYPEDESC __RPC_FAR *pTDescAlias) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefineFuncAsDllEntry( 
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDllName,
            /* [in] */ LPOLESTR szProcName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFuncDocString( 
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVarDocString( 
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFuncHelpContext( 
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVarHelpContext( 
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMops( 
            /* [in] */ UINT index,
            /* [in] */ BSTR bstrMops) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTypeIdldesc( 
            /* [in] */ IDLDESC __RPC_FAR *pIdlDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LayOut( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateTypeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateTypeInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateTypeInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGuid )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeFlags )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT uTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDocString )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ LPOLESTR pStrDoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpContext )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVersion )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefTypeInfo )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfo,
            /* [in] */ HREFTYPE __RPC_FAR *phRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddFuncDesc )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddImplType )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ HREFTYPE hRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImplTypeFlags )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ INT implTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlignment )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ WORD cbAlignment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSchema )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ LPOLESTR pStrSchema);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddVarDesc )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ VARDESC __RPC_FAR *pVarDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncAndParamNames )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in][size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarName )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeDescAlias )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ TYPEDESC __RPC_FAR *pTDescAlias);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DefineFuncAsDllEntry )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDllName,
            /* [in] */ LPOLESTR szProcName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncDocString )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarDocString )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncHelpContext )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarHelpContext )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMops )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ BSTR bstrMops);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeIdldesc )( 
            ICreateTypeInfo __RPC_FAR * This,
            /* [in] */ IDLDESC __RPC_FAR *pIdlDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LayOut )( 
            ICreateTypeInfo __RPC_FAR * This);
        
        END_INTERFACE
    } ICreateTypeInfoVtbl;

    interface ICreateTypeInfo
    {
        CONST_VTBL struct ICreateTypeInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateTypeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateTypeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateTypeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateTypeInfo_SetGuid(This,guid)	\
    (This)->lpVtbl -> SetGuid(This,guid)

#define ICreateTypeInfo_SetTypeFlags(This,uTypeFlags)	\
    (This)->lpVtbl -> SetTypeFlags(This,uTypeFlags)

#define ICreateTypeInfo_SetDocString(This,pStrDoc)	\
    (This)->lpVtbl -> SetDocString(This,pStrDoc)

#define ICreateTypeInfo_SetHelpContext(This,dwHelpContext)	\
    (This)->lpVtbl -> SetHelpContext(This,dwHelpContext)

#define ICreateTypeInfo_SetVersion(This,wMajorVerNum,wMinorVerNum)	\
    (This)->lpVtbl -> SetVersion(This,wMajorVerNum,wMinorVerNum)

#define ICreateTypeInfo_AddRefTypeInfo(This,pTInfo,phRefType)	\
    (This)->lpVtbl -> AddRefTypeInfo(This,pTInfo,phRefType)

#define ICreateTypeInfo_AddFuncDesc(This,index,pFuncDesc)	\
    (This)->lpVtbl -> AddFuncDesc(This,index,pFuncDesc)

#define ICreateTypeInfo_AddImplType(This,index,hRefType)	\
    (This)->lpVtbl -> AddImplType(This,index,hRefType)

#define ICreateTypeInfo_SetImplTypeFlags(This,index,implTypeFlags)	\
    (This)->lpVtbl -> SetImplTypeFlags(This,index,implTypeFlags)

#define ICreateTypeInfo_SetAlignment(This,cbAlignment)	\
    (This)->lpVtbl -> SetAlignment(This,cbAlignment)

#define ICreateTypeInfo_SetSchema(This,pStrSchema)	\
    (This)->lpVtbl -> SetSchema(This,pStrSchema)

#define ICreateTypeInfo_AddVarDesc(This,index,pVarDesc)	\
    (This)->lpVtbl -> AddVarDesc(This,index,pVarDesc)

#define ICreateTypeInfo_SetFuncAndParamNames(This,index,rgszNames,cNames)	\
    (This)->lpVtbl -> SetFuncAndParamNames(This,index,rgszNames,cNames)

#define ICreateTypeInfo_SetVarName(This,index,szName)	\
    (This)->lpVtbl -> SetVarName(This,index,szName)

#define ICreateTypeInfo_SetTypeDescAlias(This,pTDescAlias)	\
    (This)->lpVtbl -> SetTypeDescAlias(This,pTDescAlias)

#define ICreateTypeInfo_DefineFuncAsDllEntry(This,index,szDllName,szProcName)	\
    (This)->lpVtbl -> DefineFuncAsDllEntry(This,index,szDllName,szProcName)

#define ICreateTypeInfo_SetFuncDocString(This,index,szDocString)	\
    (This)->lpVtbl -> SetFuncDocString(This,index,szDocString)

#define ICreateTypeInfo_SetVarDocString(This,index,szDocString)	\
    (This)->lpVtbl -> SetVarDocString(This,index,szDocString)

#define ICreateTypeInfo_SetFuncHelpContext(This,index,dwHelpContext)	\
    (This)->lpVtbl -> SetFuncHelpContext(This,index,dwHelpContext)

#define ICreateTypeInfo_SetVarHelpContext(This,index,dwHelpContext)	\
    (This)->lpVtbl -> SetVarHelpContext(This,index,dwHelpContext)

#define ICreateTypeInfo_SetMops(This,index,bstrMops)	\
    (This)->lpVtbl -> SetMops(This,index,bstrMops)

#define ICreateTypeInfo_SetTypeIdldesc(This,pIdlDesc)	\
    (This)->lpVtbl -> SetTypeIdldesc(This,pIdlDesc)

#define ICreateTypeInfo_LayOut(This)	\
    (This)->lpVtbl -> LayOut(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetGuid_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ REFGUID guid);


void __RPC_STUB ICreateTypeInfo_SetGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetTypeFlags_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT uTypeFlags);


void __RPC_STUB ICreateTypeInfo_SetTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetDocString_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR pStrDoc);


void __RPC_STUB ICreateTypeInfo_SetDocString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetHelpContext_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ DWORD dwHelpContext);


void __RPC_STUB ICreateTypeInfo_SetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetVersion_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ WORD wMajorVerNum,
    /* [in] */ WORD wMinorVerNum);


void __RPC_STUB ICreateTypeInfo_SetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_AddRefTypeInfo_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ ITypeInfo __RPC_FAR *pTInfo,
    /* [in] */ HREFTYPE __RPC_FAR *phRefType);


void __RPC_STUB ICreateTypeInfo_AddRefTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_AddFuncDesc_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);


void __RPC_STUB ICreateTypeInfo_AddFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_AddImplType_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ HREFTYPE hRefType);


void __RPC_STUB ICreateTypeInfo_AddImplType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetImplTypeFlags_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ INT implTypeFlags);


void __RPC_STUB ICreateTypeInfo_SetImplTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetAlignment_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ WORD cbAlignment);


void __RPC_STUB ICreateTypeInfo_SetAlignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetSchema_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR pStrSchema);


void __RPC_STUB ICreateTypeInfo_SetSchema_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_AddVarDesc_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ VARDESC __RPC_FAR *pVarDesc);


void __RPC_STUB ICreateTypeInfo_AddVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetFuncAndParamNames_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in][size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames);


void __RPC_STUB ICreateTypeInfo_SetFuncAndParamNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetVarName_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ LPOLESTR szName);


void __RPC_STUB ICreateTypeInfo_SetVarName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetTypeDescAlias_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ TYPEDESC __RPC_FAR *pTDescAlias);


void __RPC_STUB ICreateTypeInfo_SetTypeDescAlias_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_DefineFuncAsDllEntry_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ LPOLESTR szDllName,
    /* [in] */ LPOLESTR szProcName);


void __RPC_STUB ICreateTypeInfo_DefineFuncAsDllEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetFuncDocString_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ LPOLESTR szDocString);


void __RPC_STUB ICreateTypeInfo_SetFuncDocString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetVarDocString_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ LPOLESTR szDocString);


void __RPC_STUB ICreateTypeInfo_SetVarDocString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetFuncHelpContext_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ DWORD dwHelpContext);


void __RPC_STUB ICreateTypeInfo_SetFuncHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetVarHelpContext_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ DWORD dwHelpContext);


void __RPC_STUB ICreateTypeInfo_SetVarHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetMops_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ BSTR bstrMops);


void __RPC_STUB ICreateTypeInfo_SetMops_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_SetTypeIdldesc_Proxy( 
    ICreateTypeInfo __RPC_FAR * This,
    /* [in] */ IDLDESC __RPC_FAR *pIdlDesc);


void __RPC_STUB ICreateTypeInfo_SetTypeIdldesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo_LayOut_Proxy( 
    ICreateTypeInfo __RPC_FAR * This);


void __RPC_STUB ICreateTypeInfo_LayOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateTypeInfo_INTERFACE_DEFINED__ */


#ifndef __ICreateTypeInfo2_INTERFACE_DEFINED__
#define __ICreateTypeInfo2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateTypeInfo2
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ICreateTypeInfo2 __RPC_FAR *LPCREATETYPEINFO2;


EXTERN_C const IID IID_ICreateTypeInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateTypeInfo2 : public ICreateTypeInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteFuncDesc( 
            /* [in] */ UINT index) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteFuncDescByMemId( 
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteVarDesc( 
            /* [in] */ UINT index) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteVarDescByMemId( 
            /* [in] */ MEMBERID memid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteImplType( 
            /* [in] */ UINT index) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustData( 
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFuncCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetParamCustData( 
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVarCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImplTypeCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpStringContext( 
            /* [in] */ ULONG dwHelpStringContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFuncHelpStringContext( 
            /* [in] */ UINT index,
            /* [in] */ ULONG dwHelpStringContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVarHelpStringContext( 
            /* [in] */ UINT index,
            /* [in] */ ULONG dwHelpStringContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Invalidate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPOLESTR szName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateTypeInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateTypeInfo2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateTypeInfo2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGuid )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeFlags )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT uTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDocString )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ LPOLESTR pStrDoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVersion )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefTypeInfo )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfo,
            /* [in] */ HREFTYPE __RPC_FAR *phRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddFuncDesc )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddImplType )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ HREFTYPE hRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImplTypeFlags )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ INT implTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlignment )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ WORD cbAlignment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSchema )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ LPOLESTR pStrSchema);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddVarDesc )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ VARDESC __RPC_FAR *pVarDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncAndParamNames )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in][size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarName )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeDescAlias )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ TYPEDESC __RPC_FAR *pTDescAlias);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DefineFuncAsDllEntry )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDllName,
            /* [in] */ LPOLESTR szProcName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncDocString )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarDocString )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ LPOLESTR szDocString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncHelpContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarHelpContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMops )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ BSTR bstrMops);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTypeIdldesc )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ IDLDESC __RPC_FAR *pIdlDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LayOut )( 
            ICreateTypeInfo2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteFuncDesc )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteFuncDescByMemId )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteVarDesc )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteVarDescByMemId )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteImplType )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustData )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncCustData )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetParamCustData )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarCustData )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImplTypeCustData )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpStringContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ ULONG dwHelpStringContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFuncHelpStringContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ ULONG dwHelpStringContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVarHelpStringContext )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ ULONG dwHelpStringContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invalidate )( 
            ICreateTypeInfo2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            ICreateTypeInfo2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szName);
        
        END_INTERFACE
    } ICreateTypeInfo2Vtbl;

    interface ICreateTypeInfo2
    {
        CONST_VTBL struct ICreateTypeInfo2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateTypeInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateTypeInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateTypeInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateTypeInfo2_SetGuid(This,guid)	\
    (This)->lpVtbl -> SetGuid(This,guid)

#define ICreateTypeInfo2_SetTypeFlags(This,uTypeFlags)	\
    (This)->lpVtbl -> SetTypeFlags(This,uTypeFlags)

#define ICreateTypeInfo2_SetDocString(This,pStrDoc)	\
    (This)->lpVtbl -> SetDocString(This,pStrDoc)

#define ICreateTypeInfo2_SetHelpContext(This,dwHelpContext)	\
    (This)->lpVtbl -> SetHelpContext(This,dwHelpContext)

#define ICreateTypeInfo2_SetVersion(This,wMajorVerNum,wMinorVerNum)	\
    (This)->lpVtbl -> SetVersion(This,wMajorVerNum,wMinorVerNum)

#define ICreateTypeInfo2_AddRefTypeInfo(This,pTInfo,phRefType)	\
    (This)->lpVtbl -> AddRefTypeInfo(This,pTInfo,phRefType)

#define ICreateTypeInfo2_AddFuncDesc(This,index,pFuncDesc)	\
    (This)->lpVtbl -> AddFuncDesc(This,index,pFuncDesc)

#define ICreateTypeInfo2_AddImplType(This,index,hRefType)	\
    (This)->lpVtbl -> AddImplType(This,index,hRefType)

#define ICreateTypeInfo2_SetImplTypeFlags(This,index,implTypeFlags)	\
    (This)->lpVtbl -> SetImplTypeFlags(This,index,implTypeFlags)

#define ICreateTypeInfo2_SetAlignment(This,cbAlignment)	\
    (This)->lpVtbl -> SetAlignment(This,cbAlignment)

#define ICreateTypeInfo2_SetSchema(This,pStrSchema)	\
    (This)->lpVtbl -> SetSchema(This,pStrSchema)

#define ICreateTypeInfo2_AddVarDesc(This,index,pVarDesc)	\
    (This)->lpVtbl -> AddVarDesc(This,index,pVarDesc)

#define ICreateTypeInfo2_SetFuncAndParamNames(This,index,rgszNames,cNames)	\
    (This)->lpVtbl -> SetFuncAndParamNames(This,index,rgszNames,cNames)

#define ICreateTypeInfo2_SetVarName(This,index,szName)	\
    (This)->lpVtbl -> SetVarName(This,index,szName)

#define ICreateTypeInfo2_SetTypeDescAlias(This,pTDescAlias)	\
    (This)->lpVtbl -> SetTypeDescAlias(This,pTDescAlias)

#define ICreateTypeInfo2_DefineFuncAsDllEntry(This,index,szDllName,szProcName)	\
    (This)->lpVtbl -> DefineFuncAsDllEntry(This,index,szDllName,szProcName)

#define ICreateTypeInfo2_SetFuncDocString(This,index,szDocString)	\
    (This)->lpVtbl -> SetFuncDocString(This,index,szDocString)

#define ICreateTypeInfo2_SetVarDocString(This,index,szDocString)	\
    (This)->lpVtbl -> SetVarDocString(This,index,szDocString)

#define ICreateTypeInfo2_SetFuncHelpContext(This,index,dwHelpContext)	\
    (This)->lpVtbl -> SetFuncHelpContext(This,index,dwHelpContext)

#define ICreateTypeInfo2_SetVarHelpContext(This,index,dwHelpContext)	\
    (This)->lpVtbl -> SetVarHelpContext(This,index,dwHelpContext)

#define ICreateTypeInfo2_SetMops(This,index,bstrMops)	\
    (This)->lpVtbl -> SetMops(This,index,bstrMops)

#define ICreateTypeInfo2_SetTypeIdldesc(This,pIdlDesc)	\
    (This)->lpVtbl -> SetTypeIdldesc(This,pIdlDesc)

#define ICreateTypeInfo2_LayOut(This)	\
    (This)->lpVtbl -> LayOut(This)


#define ICreateTypeInfo2_DeleteFuncDesc(This,index)	\
    (This)->lpVtbl -> DeleteFuncDesc(This,index)

#define ICreateTypeInfo2_DeleteFuncDescByMemId(This,memid,invKind)	\
    (This)->lpVtbl -> DeleteFuncDescByMemId(This,memid,invKind)

#define ICreateTypeInfo2_DeleteVarDesc(This,index)	\
    (This)->lpVtbl -> DeleteVarDesc(This,index)

#define ICreateTypeInfo2_DeleteVarDescByMemId(This,memid)	\
    (This)->lpVtbl -> DeleteVarDescByMemId(This,memid)

#define ICreateTypeInfo2_DeleteImplType(This,index)	\
    (This)->lpVtbl -> DeleteImplType(This,index)

#define ICreateTypeInfo2_SetCustData(This,guid,pVarVal)	\
    (This)->lpVtbl -> SetCustData(This,guid,pVarVal)

#define ICreateTypeInfo2_SetFuncCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> SetFuncCustData(This,index,guid,pVarVal)

#define ICreateTypeInfo2_SetParamCustData(This,indexFunc,indexParam,guid,pVarVal)	\
    (This)->lpVtbl -> SetParamCustData(This,indexFunc,indexParam,guid,pVarVal)

#define ICreateTypeInfo2_SetVarCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> SetVarCustData(This,index,guid,pVarVal)

#define ICreateTypeInfo2_SetImplTypeCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> SetImplTypeCustData(This,index,guid,pVarVal)

#define ICreateTypeInfo2_SetHelpStringContext(This,dwHelpStringContext)	\
    (This)->lpVtbl -> SetHelpStringContext(This,dwHelpStringContext)

#define ICreateTypeInfo2_SetFuncHelpStringContext(This,index,dwHelpStringContext)	\
    (This)->lpVtbl -> SetFuncHelpStringContext(This,index,dwHelpStringContext)

#define ICreateTypeInfo2_SetVarHelpStringContext(This,index,dwHelpStringContext)	\
    (This)->lpVtbl -> SetVarHelpStringContext(This,index,dwHelpStringContext)

#define ICreateTypeInfo2_Invalidate(This)	\
    (This)->lpVtbl -> Invalidate(This)

#define ICreateTypeInfo2_SetName(This,szName)	\
    (This)->lpVtbl -> SetName(This,szName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_DeleteFuncDesc_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index);


void __RPC_STUB ICreateTypeInfo2_DeleteFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_DeleteFuncDescByMemId_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind);


void __RPC_STUB ICreateTypeInfo2_DeleteFuncDescByMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_DeleteVarDesc_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index);


void __RPC_STUB ICreateTypeInfo2_DeleteVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_DeleteVarDescByMemId_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid);


void __RPC_STUB ICreateTypeInfo2_DeleteVarDescByMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_DeleteImplType_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index);


void __RPC_STUB ICreateTypeInfo2_DeleteImplType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetCustData_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeInfo2_SetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetFuncCustData_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeInfo2_SetFuncCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetParamCustData_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT indexFunc,
    /* [in] */ UINT indexParam,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeInfo2_SetParamCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetVarCustData_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeInfo2_SetVarCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetImplTypeCustData_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeInfo2_SetImplTypeCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetHelpStringContext_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ ULONG dwHelpStringContext);


void __RPC_STUB ICreateTypeInfo2_SetHelpStringContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetFuncHelpStringContext_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ ULONG dwHelpStringContext);


void __RPC_STUB ICreateTypeInfo2_SetFuncHelpStringContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetVarHelpStringContext_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ ULONG dwHelpStringContext);


void __RPC_STUB ICreateTypeInfo2_SetVarHelpStringContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_Invalidate_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This);


void __RPC_STUB ICreateTypeInfo2_Invalidate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeInfo2_SetName_Proxy( 
    ICreateTypeInfo2 __RPC_FAR * This,
    /* [in] */ LPOLESTR szName);


void __RPC_STUB ICreateTypeInfo2_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateTypeInfo2_INTERFACE_DEFINED__ */


#ifndef __ICreateTypeLib_INTERFACE_DEFINED__
#define __ICreateTypeLib_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateTypeLib
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ICreateTypeLib __RPC_FAR *LPCREATETYPELIB;


EXTERN_C const IID IID_ICreateTypeLib;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateTypeLib : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTypeInfo( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ TYPEKIND tkind,
            /* [out] */ ICreateTypeInfo __RPC_FAR *__RPC_FAR *ppCTInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPOLESTR szName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVersion( 
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGuid( 
            /* [in] */ REFGUID guid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDocString( 
            /* [in] */ LPOLESTR szDoc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpFileName( 
            /* [in] */ LPOLESTR szHelpFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpContext( 
            /* [in] */ DWORD dwHelpContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLcid( 
            /* [in] */ LCID lcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLibFlags( 
            /* [in] */ UINT uLibFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveAllChanges( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateTypeLibVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateTypeLib __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateTypeLib __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTypeInfo )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ TYPEKIND tkind,
            /* [out] */ ICreateTypeInfo __RPC_FAR *__RPC_FAR *ppCTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ LPOLESTR szName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVersion )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGuid )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDocString )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ LPOLESTR szDoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpFileName )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ LPOLESTR szHelpFileName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpContext )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLcid )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLibFlags )( 
            ICreateTypeLib __RPC_FAR * This,
            /* [in] */ UINT uLibFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveAllChanges )( 
            ICreateTypeLib __RPC_FAR * This);
        
        END_INTERFACE
    } ICreateTypeLibVtbl;

    interface ICreateTypeLib
    {
        CONST_VTBL struct ICreateTypeLibVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateTypeLib_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateTypeLib_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateTypeLib_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateTypeLib_CreateTypeInfo(This,szName,tkind,ppCTInfo)	\
    (This)->lpVtbl -> CreateTypeInfo(This,szName,tkind,ppCTInfo)

#define ICreateTypeLib_SetName(This,szName)	\
    (This)->lpVtbl -> SetName(This,szName)

#define ICreateTypeLib_SetVersion(This,wMajorVerNum,wMinorVerNum)	\
    (This)->lpVtbl -> SetVersion(This,wMajorVerNum,wMinorVerNum)

#define ICreateTypeLib_SetGuid(This,guid)	\
    (This)->lpVtbl -> SetGuid(This,guid)

#define ICreateTypeLib_SetDocString(This,szDoc)	\
    (This)->lpVtbl -> SetDocString(This,szDoc)

#define ICreateTypeLib_SetHelpFileName(This,szHelpFileName)	\
    (This)->lpVtbl -> SetHelpFileName(This,szHelpFileName)

#define ICreateTypeLib_SetHelpContext(This,dwHelpContext)	\
    (This)->lpVtbl -> SetHelpContext(This,dwHelpContext)

#define ICreateTypeLib_SetLcid(This,lcid)	\
    (This)->lpVtbl -> SetLcid(This,lcid)

#define ICreateTypeLib_SetLibFlags(This,uLibFlags)	\
    (This)->lpVtbl -> SetLibFlags(This,uLibFlags)

#define ICreateTypeLib_SaveAllChanges(This)	\
    (This)->lpVtbl -> SaveAllChanges(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateTypeLib_CreateTypeInfo_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ TYPEKIND tkind,
    /* [out] */ ICreateTypeInfo __RPC_FAR *__RPC_FAR *ppCTInfo);


void __RPC_STUB ICreateTypeLib_CreateTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetName_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szName);


void __RPC_STUB ICreateTypeLib_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetVersion_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ WORD wMajorVerNum,
    /* [in] */ WORD wMinorVerNum);


void __RPC_STUB ICreateTypeLib_SetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetGuid_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ REFGUID guid);


void __RPC_STUB ICreateTypeLib_SetGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetDocString_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szDoc);


void __RPC_STUB ICreateTypeLib_SetDocString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetHelpFileName_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szHelpFileName);


void __RPC_STUB ICreateTypeLib_SetHelpFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetHelpContext_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ DWORD dwHelpContext);


void __RPC_STUB ICreateTypeLib_SetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetLcid_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ LCID lcid);


void __RPC_STUB ICreateTypeLib_SetLcid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SetLibFlags_Proxy( 
    ICreateTypeLib __RPC_FAR * This,
    /* [in] */ UINT uLibFlags);


void __RPC_STUB ICreateTypeLib_SetLibFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib_SaveAllChanges_Proxy( 
    ICreateTypeLib __RPC_FAR * This);


void __RPC_STUB ICreateTypeLib_SaveAllChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateTypeLib_INTERFACE_DEFINED__ */


#ifndef __ICreateTypeLib2_INTERFACE_DEFINED__
#define __ICreateTypeLib2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateTypeLib2
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ICreateTypeLib2 __RPC_FAR *LPCREATETYPELIB2;


EXTERN_C const IID IID_ICreateTypeLib2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateTypeLib2 : public ICreateTypeLib
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteTypeInfo( 
            /* [in] */ LPOLESTR szName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustData( 
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpStringContext( 
            /* [in] */ ULONG dwHelpStringContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpStringDll( 
            /* [in] */ LPOLESTR szFileName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateTypeLib2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateTypeLib2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateTypeLib2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTypeInfo )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ TYPEKIND tkind,
            /* [out] */ ICreateTypeInfo __RPC_FAR *__RPC_FAR *ppCTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVersion )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ WORD wMajorVerNum,
            /* [in] */ WORD wMinorVerNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGuid )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDocString )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szDoc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpFileName )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szHelpFileName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpContext )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ DWORD dwHelpContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLcid )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLibFlags )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ UINT uLibFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveAllChanges )( 
            ICreateTypeLib2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteTypeInfo )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustData )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [in] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpStringContext )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ ULONG dwHelpStringContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpStringDll )( 
            ICreateTypeLib2 __RPC_FAR * This,
            /* [in] */ LPOLESTR szFileName);
        
        END_INTERFACE
    } ICreateTypeLib2Vtbl;

    interface ICreateTypeLib2
    {
        CONST_VTBL struct ICreateTypeLib2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateTypeLib2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateTypeLib2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateTypeLib2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateTypeLib2_CreateTypeInfo(This,szName,tkind,ppCTInfo)	\
    (This)->lpVtbl -> CreateTypeInfo(This,szName,tkind,ppCTInfo)

#define ICreateTypeLib2_SetName(This,szName)	\
    (This)->lpVtbl -> SetName(This,szName)

#define ICreateTypeLib2_SetVersion(This,wMajorVerNum,wMinorVerNum)	\
    (This)->lpVtbl -> SetVersion(This,wMajorVerNum,wMinorVerNum)

#define ICreateTypeLib2_SetGuid(This,guid)	\
    (This)->lpVtbl -> SetGuid(This,guid)

#define ICreateTypeLib2_SetDocString(This,szDoc)	\
    (This)->lpVtbl -> SetDocString(This,szDoc)

#define ICreateTypeLib2_SetHelpFileName(This,szHelpFileName)	\
    (This)->lpVtbl -> SetHelpFileName(This,szHelpFileName)

#define ICreateTypeLib2_SetHelpContext(This,dwHelpContext)	\
    (This)->lpVtbl -> SetHelpContext(This,dwHelpContext)

#define ICreateTypeLib2_SetLcid(This,lcid)	\
    (This)->lpVtbl -> SetLcid(This,lcid)

#define ICreateTypeLib2_SetLibFlags(This,uLibFlags)	\
    (This)->lpVtbl -> SetLibFlags(This,uLibFlags)

#define ICreateTypeLib2_SaveAllChanges(This)	\
    (This)->lpVtbl -> SaveAllChanges(This)


#define ICreateTypeLib2_DeleteTypeInfo(This,szName)	\
    (This)->lpVtbl -> DeleteTypeInfo(This,szName)

#define ICreateTypeLib2_SetCustData(This,guid,pVarVal)	\
    (This)->lpVtbl -> SetCustData(This,guid,pVarVal)

#define ICreateTypeLib2_SetHelpStringContext(This,dwHelpStringContext)	\
    (This)->lpVtbl -> SetHelpStringContext(This,dwHelpStringContext)

#define ICreateTypeLib2_SetHelpStringDll(This,szFileName)	\
    (This)->lpVtbl -> SetHelpStringDll(This,szFileName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateTypeLib2_DeleteTypeInfo_Proxy( 
    ICreateTypeLib2 __RPC_FAR * This,
    /* [in] */ LPOLESTR szName);


void __RPC_STUB ICreateTypeLib2_DeleteTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib2_SetCustData_Proxy( 
    ICreateTypeLib2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [in] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ICreateTypeLib2_SetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib2_SetHelpStringContext_Proxy( 
    ICreateTypeLib2 __RPC_FAR * This,
    /* [in] */ ULONG dwHelpStringContext);


void __RPC_STUB ICreateTypeLib2_SetHelpStringContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateTypeLib2_SetHelpStringDll_Proxy( 
    ICreateTypeLib2 __RPC_FAR * This,
    /* [in] */ LPOLESTR szFileName);


void __RPC_STUB ICreateTypeLib2_SetHelpStringDll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateTypeLib2_INTERFACE_DEFINED__ */


#ifndef __IDispatch_INTERFACE_DEFINED__
#define __IDispatch_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDispatch
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IDispatch __RPC_FAR *LPDISPATCH;

/* DISPID reserved to indicate an "unknown" name */
/* only reserved for data members (properties); reused as a method dispid below */
#define	DISPID_UNKNOWN	( -1 )

/* DISPID reserved for the "value" property */
#define	DISPID_VALUE	( 0 )

/* The following DISPID is reserved to indicate the param
 * that is the right-hand-side (or "put" value) of a PropertyPut
 */
#define	DISPID_PROPERTYPUT	( -3 )

/* DISPID reserved for the standard "NewEnum" method */
#define	DISPID_NEWENUM	( -4 )

/* DISPID reserved for the standard "Evaluate" method */
#define	DISPID_EVALUATE	( -5 )

#define	DISPID_CONSTRUCTOR	( -6 )

#define	DISPID_DESTRUCTOR	( -7 )

#define	DISPID_COLLECT	( -8 )

/* The range -500 through -999 is reserved for Controls */
/* The range 0x80010000 through 0x8001FFFF is reserved for Controls */
/* The range -5000 through -5499 is reserved for ActiveX Accessability */
/* The remainder of the negative DISPIDs are reserved for future use */

EXTERN_C const IID IID_IDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDispatch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
            /* [out] */ UINT __RPC_FAR *pctinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDispatchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDispatch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDispatch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDispatch __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDispatch __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDispatch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDispatch __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IDispatchVtbl;

    interface IDispatch
    {
        CONST_VTBL struct IDispatchVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDispatch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDispatch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDispatch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDispatch_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDispatch_GetTypeInfoCount_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pctinfo);


void __RPC_STUB IDispatch_GetTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDispatch_GetTypeInfo_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);


void __RPC_STUB IDispatch_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDispatch_GetIDsOfNames_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);


void __RPC_STUB IDispatch_GetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IDispatch_RemoteInvoke_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *pArgErr,
    /* [in] */ UINT cVarRef,
    /* [size_is][in] */ UINT __RPC_FAR *rgVarRefIdx,
    /* [size_is][out][in] */ VARIANTARG __RPC_FAR *rgVarRef);


void __RPC_STUB IDispatch_RemoteInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDispatch_INTERFACE_DEFINED__ */


#ifndef __IEnumVARIANT_INTERFACE_DEFINED__
#define __IEnumVARIANT_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumVARIANT
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEnumVARIANT __RPC_FAR *LPENUMVARIANT;


EXTERN_C const IID IID_IEnumVARIANT;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumVARIANT : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
            /* [out] */ ULONG __RPC_FAR *pCeltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumVARIANTVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumVARIANT __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumVARIANT __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumVARIANT __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumVARIANT __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
            /* [out] */ ULONG __RPC_FAR *pCeltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumVARIANT __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumVARIANT __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumVARIANT __RPC_FAR * This,
            /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumVARIANTVtbl;

    interface IEnumVARIANT
    {
        CONST_VTBL struct IEnumVARIANTVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumVARIANT_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumVARIANT_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumVARIANT_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumVARIANT_Next(This,celt,rgVar,pCeltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgVar,pCeltFetched)

#define IEnumVARIANT_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumVARIANT_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumVARIANT_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumVARIANT_RemoteNext_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
    /* [out] */ ULONG __RPC_FAR *pCeltFetched);


void __RPC_STUB IEnumVARIANT_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Skip_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumVARIANT_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Reset_Proxy( 
    IEnumVARIANT __RPC_FAR * This);


void __RPC_STUB IEnumVARIANT_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumVARIANT_Clone_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumVARIANT_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumVARIANT_INTERFACE_DEFINED__ */


#ifndef __ITypeComp_INTERFACE_DEFINED__
#define __ITypeComp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeComp
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITypeComp __RPC_FAR *LPTYPECOMP;

typedef /* [v1_enum] */ 
enum tagDESCKIND
    {	DESCKIND_NONE	= 0,
	DESCKIND_FUNCDESC	= DESCKIND_NONE + 1,
	DESCKIND_VARDESC	= DESCKIND_FUNCDESC + 1,
	DESCKIND_TYPECOMP	= DESCKIND_VARDESC + 1,
	DESCKIND_IMPLICITAPPOBJ	= DESCKIND_TYPECOMP + 1,
	DESCKIND_MAX	= DESCKIND_IMPLICITAPPOBJ + 1
    }	DESCKIND;

typedef union tagBINDPTR
    {
    FUNCDESC __RPC_FAR *lpfuncdesc;
    VARDESC __RPC_FAR *lpvardesc;
    ITypeComp __RPC_FAR *lptcomp;
    }	BINDPTR;

typedef union tagBINDPTR __RPC_FAR *LPBINDPTR;


EXTERN_C const IID IID_ITypeComp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeComp : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Bind( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ ULONG lHashVal,
            /* [in] */ WORD wFlags,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [out] */ DESCKIND __RPC_FAR *pDescKind,
            /* [out] */ BINDPTR __RPC_FAR *pBindPtr) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE BindType( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ ULONG lHashVal,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeCompVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeComp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeComp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeComp __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Bind )( 
            ITypeComp __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ ULONG lHashVal,
            /* [in] */ WORD wFlags,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [out] */ DESCKIND __RPC_FAR *pDescKind,
            /* [out] */ BINDPTR __RPC_FAR *pBindPtr);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindType )( 
            ITypeComp __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ ULONG lHashVal,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
        
        END_INTERFACE
    } ITypeCompVtbl;

    interface ITypeComp
    {
        CONST_VTBL struct ITypeCompVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeComp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeComp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeComp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeComp_Bind(This,szName,lHashVal,wFlags,ppTInfo,pDescKind,pBindPtr)	\
    (This)->lpVtbl -> Bind(This,szName,lHashVal,wFlags,ppTInfo,pDescKind,pBindPtr)

#define ITypeComp_BindType(This,szName,lHashVal,ppTInfo,ppTComp)	\
    (This)->lpVtbl -> BindType(This,szName,lHashVal,ppTInfo,ppTComp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_RemoteBind_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [in] */ WORD wFlags,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [out] */ DESCKIND __RPC_FAR *pDescKind,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTypeComp,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);


void __RPC_STUB ITypeComp_RemoteBind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_RemoteBindType_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);


void __RPC_STUB ITypeComp_RemoteBindType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeComp_INTERFACE_DEFINED__ */


#ifndef __ITypeInfo_INTERFACE_DEFINED__
#define __ITypeInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeInfo
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITypeInfo __RPC_FAR *LPTYPEINFO;


EXTERN_C const IID IID_ITypeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetTypeAttr( 
            /* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeComp( 
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetFuncDesc( 
            /* [in] */ UINT index,
            /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetVarDesc( 
            /* [in] */ UINT index,
            /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ MEMBERID memid,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
            /* [in] */ UINT cMaxNames,
            /* [out] */ UINT __RPC_FAR *pcNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRefTypeOfImplType( 
            /* [in] */ UINT index,
            /* [out] */ HREFTYPE __RPC_FAR *pRefType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImplTypeFlags( 
            /* [in] */ UINT index,
            /* [out] */ INT __RPC_FAR *pImplTypeFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ PVOID pvInstance,
            /* [in] */ MEMBERID memid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDocumentation( 
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDllEntry( 
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ BSTR __RPC_FAR *pBstrDllName,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ WORD __RPC_FAR *pwOrdinal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRefTypeInfo( 
            /* [in] */ HREFTYPE hRefType,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE AddressOfMember( 
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ PVOID __RPC_FAR *ppv) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMops( 
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrMops) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetContainingTypeLib( 
            /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
            /* [out] */ UINT __RPC_FAR *pIndex) = 0;
        
        virtual /* [local] */ void STDMETHODCALLTYPE ReleaseTypeAttr( 
            /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr) = 0;
        
        virtual /* [local] */ void STDMETHODCALLTYPE ReleaseFuncDesc( 
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc) = 0;
        
        virtual /* [local] */ void STDMETHODCALLTYPE ReleaseVarDesc( 
            /* [in] */ VARDESC __RPC_FAR *pVarDesc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeAttr )( 
            ITypeInfo __RPC_FAR * This,
            /* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeComp )( 
            ITypeInfo __RPC_FAR * This,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFuncDesc )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVarDesc )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
            /* [in] */ UINT cMaxNames,
            /* [out] */ UINT __RPC_FAR *pcNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefTypeOfImplType )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ HREFTYPE __RPC_FAR *pRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImplTypeFlags )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ INT __RPC_FAR *pImplTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITypeInfo __RPC_FAR * This,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ PVOID pvInstance,
            /* [in] */ MEMBERID memid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDllEntry )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ BSTR __RPC_FAR *pBstrDllName,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ WORD __RPC_FAR *pwOrdinal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefTypeInfo )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ HREFTYPE hRefType,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddressOfMember )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ PVOID __RPC_FAR *ppv);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstance )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMops )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrMops);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainingTypeLib )( 
            ITypeInfo __RPC_FAR * This,
            /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
            /* [out] */ UINT __RPC_FAR *pIndex);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseTypeAttr )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseFuncDesc )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseVarDesc )( 
            ITypeInfo __RPC_FAR * This,
            /* [in] */ VARDESC __RPC_FAR *pVarDesc);
        
        END_INTERFACE
    } ITypeInfoVtbl;

    interface ITypeInfo
    {
        CONST_VTBL struct ITypeInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeInfo_GetTypeAttr(This,ppTypeAttr)	\
    (This)->lpVtbl -> GetTypeAttr(This,ppTypeAttr)

#define ITypeInfo_GetTypeComp(This,ppTComp)	\
    (This)->lpVtbl -> GetTypeComp(This,ppTComp)

#define ITypeInfo_GetFuncDesc(This,index,ppFuncDesc)	\
    (This)->lpVtbl -> GetFuncDesc(This,index,ppFuncDesc)

#define ITypeInfo_GetVarDesc(This,index,ppVarDesc)	\
    (This)->lpVtbl -> GetVarDesc(This,index,ppVarDesc)

#define ITypeInfo_GetNames(This,memid,rgBstrNames,cMaxNames,pcNames)	\
    (This)->lpVtbl -> GetNames(This,memid,rgBstrNames,cMaxNames,pcNames)

#define ITypeInfo_GetRefTypeOfImplType(This,index,pRefType)	\
    (This)->lpVtbl -> GetRefTypeOfImplType(This,index,pRefType)

#define ITypeInfo_GetImplTypeFlags(This,index,pImplTypeFlags)	\
    (This)->lpVtbl -> GetImplTypeFlags(This,index,pImplTypeFlags)

#define ITypeInfo_GetIDsOfNames(This,rgszNames,cNames,pMemId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,rgszNames,cNames,pMemId)

#define ITypeInfo_Invoke(This,pvInstance,memid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,pvInstance,memid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#define ITypeInfo_GetDocumentation(This,memid,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)	\
    (This)->lpVtbl -> GetDocumentation(This,memid,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)

#define ITypeInfo_GetDllEntry(This,memid,invKind,pBstrDllName,pBstrName,pwOrdinal)	\
    (This)->lpVtbl -> GetDllEntry(This,memid,invKind,pBstrDllName,pBstrName,pwOrdinal)

#define ITypeInfo_GetRefTypeInfo(This,hRefType,ppTInfo)	\
    (This)->lpVtbl -> GetRefTypeInfo(This,hRefType,ppTInfo)

#define ITypeInfo_AddressOfMember(This,memid,invKind,ppv)	\
    (This)->lpVtbl -> AddressOfMember(This,memid,invKind,ppv)

#define ITypeInfo_CreateInstance(This,pUnkOuter,riid,ppvObj)	\
    (This)->lpVtbl -> CreateInstance(This,pUnkOuter,riid,ppvObj)

#define ITypeInfo_GetMops(This,memid,pBstrMops)	\
    (This)->lpVtbl -> GetMops(This,memid,pBstrMops)

#define ITypeInfo_GetContainingTypeLib(This,ppTLib,pIndex)	\
    (This)->lpVtbl -> GetContainingTypeLib(This,ppTLib,pIndex)

#define ITypeInfo_ReleaseTypeAttr(This,pTypeAttr)	\
    (This)->lpVtbl -> ReleaseTypeAttr(This,pTypeAttr)

#define ITypeInfo_ReleaseFuncDesc(This,pFuncDesc)	\
    (This)->lpVtbl -> ReleaseFuncDesc(This,pFuncDesc)

#define ITypeInfo_ReleaseVarDesc(This,pVarDesc)	\
    (This)->lpVtbl -> ReleaseVarDesc(This,pVarDesc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ LPTYPEATTR __RPC_FAR *ppTypeAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);


void __RPC_STUB ITypeInfo_RemoteGetTypeAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeComp_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);


void __RPC_STUB ITypeInfo_GetTypeComp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);


void __RPC_STUB ITypeInfo_RemoteGetFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);


void __RPC_STUB ITypeInfo_RemoteGetVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetNames_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
    /* [in] */ UINT cMaxNames,
    /* [out] */ UINT __RPC_FAR *pcNames);


void __RPC_STUB ITypeInfo_RemoteGetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetRefTypeOfImplType_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ HREFTYPE __RPC_FAR *pRefType);


void __RPC_STUB ITypeInfo_GetRefTypeOfImplType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetImplTypeFlags_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ INT __RPC_FAR *pImplTypeFlags);


void __RPC_STUB ITypeInfo_GetImplTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetIDsOfNames_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId);


void __RPC_STUB ITypeInfo_GetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteInvoke_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pIUnk,
    /* [in] */ MEMBERID memid,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *pArgErr,
    /* [in] */ UINT cVarRef,
    /* [size_is][in] */ UINT __RPC_FAR *rgVarRefIdx,
    /* [size_is][out][in] */ VARIANTARG __RPC_FAR *rgVarRef);


void __RPC_STUB ITypeInfo_RemoteInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetDocumentation_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);


void __RPC_STUB ITypeInfo_RemoteGetDocumentation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetDllEntry_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrDllName,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ WORD __RPC_FAR *pwOrdinal);


void __RPC_STUB ITypeInfo_RemoteGetDllEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetRefTypeInfo_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ HREFTYPE hRefType,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);


void __RPC_STUB ITypeInfo_GetRefTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalAddressOfMember_Proxy( 
    ITypeInfo __RPC_FAR * This);


void __RPC_STUB ITypeInfo_LocalAddressOfMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteCreateInstance_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);


void __RPC_STUB ITypeInfo_RemoteCreateInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo_GetMops_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [out] */ BSTR __RPC_FAR *pBstrMops);


void __RPC_STUB ITypeInfo_GetMops_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_RemoteGetContainingTypeLib_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
    /* [out] */ UINT __RPC_FAR *pIndex);


void __RPC_STUB ITypeInfo_RemoteGetContainingTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This);


void __RPC_STUB ITypeInfo_LocalReleaseTypeAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This);


void __RPC_STUB ITypeInfo_LocalReleaseFuncDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_LocalReleaseVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This);


void __RPC_STUB ITypeInfo_LocalReleaseVarDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeInfo_INTERFACE_DEFINED__ */


#ifndef __ITypeInfo2_INTERFACE_DEFINED__
#define __ITypeInfo2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeInfo2
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITypeInfo2 __RPC_FAR *LPTYPEINFO2;


EXTERN_C const IID IID_ITypeInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeInfo2 : public ITypeInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTypeKind( 
            /* [out] */ TYPEKIND __RPC_FAR *pTypeKind) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeFlags( 
            /* [out] */ ULONG __RPC_FAR *pTypeFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFuncIndexOfMemId( 
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ UINT __RPC_FAR *pFuncIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarIndexOfMemId( 
            /* [in] */ MEMBERID memid,
            /* [out] */ UINT __RPC_FAR *pVarIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCustData( 
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFuncCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParamCustData( 
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImplTypeCustData( 
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDocumentation2( 
            /* [in] */ MEMBERID memid,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllCustData( 
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllFuncCustData( 
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllParamCustData( 
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllVarCustData( 
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllImplTypeCustData( 
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeInfo2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeInfo2 __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeAttr )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeComp )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFuncDesc )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVarDesc )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
            /* [in] */ UINT cMaxNames,
            /* [out] */ UINT __RPC_FAR *pcNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefTypeOfImplType )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ HREFTYPE __RPC_FAR *pRefType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImplTypeFlags )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ INT __RPC_FAR *pImplTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ PVOID pvInstance,
            /* [in] */ MEMBERID memid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDllEntry )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ BSTR __RPC_FAR *pBstrDllName,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ WORD __RPC_FAR *pwOrdinal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRefTypeInfo )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ HREFTYPE hRefType,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddressOfMember )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ PVOID __RPC_FAR *ppv);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstance )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMops )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [out] */ BSTR __RPC_FAR *pBstrMops);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainingTypeLib )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
            /* [out] */ UINT __RPC_FAR *pIndex);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseTypeAttr )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseFuncDesc )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseVarDesc )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ VARDESC __RPC_FAR *pVarDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeKind )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ TYPEKIND __RPC_FAR *pTypeKind);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeFlags )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pTypeFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFuncIndexOfMemId )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ INVOKEKIND invKind,
            /* [out] */ UINT __RPC_FAR *pFuncIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVarIndexOfMemId )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [out] */ UINT __RPC_FAR *pVarIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFuncCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParamCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVarCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImplTypeCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation2 )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ MEMBERID memid,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllFuncCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllParamCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT indexFunc,
            /* [in] */ UINT indexParam,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllVarCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllImplTypeCustData )( 
            ITypeInfo2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        END_INTERFACE
    } ITypeInfo2Vtbl;

    interface ITypeInfo2
    {
        CONST_VTBL struct ITypeInfo2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeInfo2_GetTypeAttr(This,ppTypeAttr)	\
    (This)->lpVtbl -> GetTypeAttr(This,ppTypeAttr)

#define ITypeInfo2_GetTypeComp(This,ppTComp)	\
    (This)->lpVtbl -> GetTypeComp(This,ppTComp)

#define ITypeInfo2_GetFuncDesc(This,index,ppFuncDesc)	\
    (This)->lpVtbl -> GetFuncDesc(This,index,ppFuncDesc)

#define ITypeInfo2_GetVarDesc(This,index,ppVarDesc)	\
    (This)->lpVtbl -> GetVarDesc(This,index,ppVarDesc)

#define ITypeInfo2_GetNames(This,memid,rgBstrNames,cMaxNames,pcNames)	\
    (This)->lpVtbl -> GetNames(This,memid,rgBstrNames,cMaxNames,pcNames)

#define ITypeInfo2_GetRefTypeOfImplType(This,index,pRefType)	\
    (This)->lpVtbl -> GetRefTypeOfImplType(This,index,pRefType)

#define ITypeInfo2_GetImplTypeFlags(This,index,pImplTypeFlags)	\
    (This)->lpVtbl -> GetImplTypeFlags(This,index,pImplTypeFlags)

#define ITypeInfo2_GetIDsOfNames(This,rgszNames,cNames,pMemId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,rgszNames,cNames,pMemId)

#define ITypeInfo2_Invoke(This,pvInstance,memid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,pvInstance,memid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#define ITypeInfo2_GetDocumentation(This,memid,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)	\
    (This)->lpVtbl -> GetDocumentation(This,memid,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)

#define ITypeInfo2_GetDllEntry(This,memid,invKind,pBstrDllName,pBstrName,pwOrdinal)	\
    (This)->lpVtbl -> GetDllEntry(This,memid,invKind,pBstrDllName,pBstrName,pwOrdinal)

#define ITypeInfo2_GetRefTypeInfo(This,hRefType,ppTInfo)	\
    (This)->lpVtbl -> GetRefTypeInfo(This,hRefType,ppTInfo)

#define ITypeInfo2_AddressOfMember(This,memid,invKind,ppv)	\
    (This)->lpVtbl -> AddressOfMember(This,memid,invKind,ppv)

#define ITypeInfo2_CreateInstance(This,pUnkOuter,riid,ppvObj)	\
    (This)->lpVtbl -> CreateInstance(This,pUnkOuter,riid,ppvObj)

#define ITypeInfo2_GetMops(This,memid,pBstrMops)	\
    (This)->lpVtbl -> GetMops(This,memid,pBstrMops)

#define ITypeInfo2_GetContainingTypeLib(This,ppTLib,pIndex)	\
    (This)->lpVtbl -> GetContainingTypeLib(This,ppTLib,pIndex)

#define ITypeInfo2_ReleaseTypeAttr(This,pTypeAttr)	\
    (This)->lpVtbl -> ReleaseTypeAttr(This,pTypeAttr)

#define ITypeInfo2_ReleaseFuncDesc(This,pFuncDesc)	\
    (This)->lpVtbl -> ReleaseFuncDesc(This,pFuncDesc)

#define ITypeInfo2_ReleaseVarDesc(This,pVarDesc)	\
    (This)->lpVtbl -> ReleaseVarDesc(This,pVarDesc)


#define ITypeInfo2_GetTypeKind(This,pTypeKind)	\
    (This)->lpVtbl -> GetTypeKind(This,pTypeKind)

#define ITypeInfo2_GetTypeFlags(This,pTypeFlags)	\
    (This)->lpVtbl -> GetTypeFlags(This,pTypeFlags)

#define ITypeInfo2_GetFuncIndexOfMemId(This,memid,invKind,pFuncIndex)	\
    (This)->lpVtbl -> GetFuncIndexOfMemId(This,memid,invKind,pFuncIndex)

#define ITypeInfo2_GetVarIndexOfMemId(This,memid,pVarIndex)	\
    (This)->lpVtbl -> GetVarIndexOfMemId(This,memid,pVarIndex)

#define ITypeInfo2_GetCustData(This,guid,pVarVal)	\
    (This)->lpVtbl -> GetCustData(This,guid,pVarVal)

#define ITypeInfo2_GetFuncCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> GetFuncCustData(This,index,guid,pVarVal)

#define ITypeInfo2_GetParamCustData(This,indexFunc,indexParam,guid,pVarVal)	\
    (This)->lpVtbl -> GetParamCustData(This,indexFunc,indexParam,guid,pVarVal)

#define ITypeInfo2_GetVarCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> GetVarCustData(This,index,guid,pVarVal)

#define ITypeInfo2_GetImplTypeCustData(This,index,guid,pVarVal)	\
    (This)->lpVtbl -> GetImplTypeCustData(This,index,guid,pVarVal)

#define ITypeInfo2_GetDocumentation2(This,memid,lcid,pbstrHelpString,pdwHelpStringContext,pbstrHelpStringDll)	\
    (This)->lpVtbl -> GetDocumentation2(This,memid,lcid,pbstrHelpString,pdwHelpStringContext,pbstrHelpStringDll)

#define ITypeInfo2_GetAllCustData(This,pCustData)	\
    (This)->lpVtbl -> GetAllCustData(This,pCustData)

#define ITypeInfo2_GetAllFuncCustData(This,index,pCustData)	\
    (This)->lpVtbl -> GetAllFuncCustData(This,index,pCustData)

#define ITypeInfo2_GetAllParamCustData(This,indexFunc,indexParam,pCustData)	\
    (This)->lpVtbl -> GetAllParamCustData(This,indexFunc,indexParam,pCustData)

#define ITypeInfo2_GetAllVarCustData(This,index,pCustData)	\
    (This)->lpVtbl -> GetAllVarCustData(This,index,pCustData)

#define ITypeInfo2_GetAllImplTypeCustData(This,index,pCustData)	\
    (This)->lpVtbl -> GetAllImplTypeCustData(This,index,pCustData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITypeInfo2_GetTypeKind_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ TYPEKIND __RPC_FAR *pTypeKind);


void __RPC_STUB ITypeInfo2_GetTypeKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetTypeFlags_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pTypeFlags);


void __RPC_STUB ITypeInfo2_GetTypeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetFuncIndexOfMemId_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [out] */ UINT __RPC_FAR *pFuncIndex);


void __RPC_STUB ITypeInfo2_GetFuncIndexOfMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetVarIndexOfMemId_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [out] */ UINT __RPC_FAR *pVarIndex);


void __RPC_STUB ITypeInfo2_GetVarIndexOfMemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeInfo2_GetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetFuncCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeInfo2_GetFuncCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetParamCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT indexFunc,
    /* [in] */ UINT indexParam,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeInfo2_GetParamCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetVarCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeInfo2_GetVarCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetImplTypeCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeInfo2_GetImplTypeCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo2_RemoteGetDocumentation2_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);


void __RPC_STUB ITypeInfo2_RemoteGetDocumentation2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeInfo2_GetAllCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllFuncCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeInfo2_GetAllFuncCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllParamCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT indexFunc,
    /* [in] */ UINT indexParam,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeInfo2_GetAllParamCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllVarCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeInfo2_GetAllVarCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeInfo2_GetAllImplTypeCustData_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeInfo2_GetAllImplTypeCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeInfo2_INTERFACE_DEFINED__ */


#ifndef __ITypeLib_INTERFACE_DEFINED__
#define __ITypeLib_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeLib
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [v1_enum] */ 
enum tagSYSKIND
    {	SYS_WIN16	= 0,
	SYS_WIN32	= SYS_WIN16 + 1,
	SYS_MAC	= SYS_WIN32 + 1
    }	SYSKIND;

typedef /* [v1_enum] */ 
enum tagLIBFLAGS
    {	LIBFLAG_FRESTRICTED	= 0x1,
	LIBFLAG_FCONTROL	= 0x2,
	LIBFLAG_FHIDDEN	= 0x4,
	LIBFLAG_FHASDISKIMAGE	= 0x8
    }	LIBFLAGS;

typedef /* [unique] */ ITypeLib __RPC_FAR *LPTYPELIB;

typedef struct  tagTLIBATTR
    {
    GUID guid;
    LCID lcid;
    SYSKIND syskind;
    WORD wMajorVerNum;
    WORD wMinorVerNum;
    WORD wLibFlags;
    }	TLIBATTR;

typedef struct tagTLIBATTR __RPC_FAR *LPTLIBATTR;


EXTERN_C const IID IID_ITypeLib;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeLib : public IUnknown
    {
    public:
        virtual /* [local] */ UINT STDMETHODCALLTYPE GetTypeInfoCount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ UINT index,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoType( 
            /* [in] */ UINT index,
            /* [out] */ TYPEKIND __RPC_FAR *pTKind) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoOfGuid( 
            /* [in] */ REFGUID guid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetLibAttr( 
            /* [out] */ TLIBATTR __RPC_FAR *__RPC_FAR *ppTLibAttr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeComp( 
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDocumentation( 
            /* [in] */ INT index,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE IsName( 
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [out] */ BOOL __RPC_FAR *pfName) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE FindName( 
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
            /* [out][in] */ USHORT __RPC_FAR *pcFound) = 0;
        
        virtual /* [local] */ void STDMETHODCALLTYPE ReleaseTLibAttr( 
            /* [in] */ TLIBATTR __RPC_FAR *pTLibAttr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeLibVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeLib __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeLib __RPC_FAR * This);
        
        /* [local] */ UINT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITypeLib __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoType )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ TYPEKIND __RPC_FAR *pTKind);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoOfGuid )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLibAttr )( 
            ITypeLib __RPC_FAR * This,
            /* [out] */ TLIBATTR __RPC_FAR *__RPC_FAR *ppTLibAttr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeComp )( 
            ITypeLib __RPC_FAR * This,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ INT index,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsName )( 
            ITypeLib __RPC_FAR * This,
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [out] */ BOOL __RPC_FAR *pfName);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindName )( 
            ITypeLib __RPC_FAR * This,
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
            /* [out][in] */ USHORT __RPC_FAR *pcFound);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseTLibAttr )( 
            ITypeLib __RPC_FAR * This,
            /* [in] */ TLIBATTR __RPC_FAR *pTLibAttr);
        
        END_INTERFACE
    } ITypeLibVtbl;

    interface ITypeLib
    {
        CONST_VTBL struct ITypeLibVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeLib_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeLib_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeLib_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeLib_GetTypeInfoCount(This)	\
    (This)->lpVtbl -> GetTypeInfoCount(This)

#define ITypeLib_GetTypeInfo(This,index,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,index,ppTInfo)

#define ITypeLib_GetTypeInfoType(This,index,pTKind)	\
    (This)->lpVtbl -> GetTypeInfoType(This,index,pTKind)

#define ITypeLib_GetTypeInfoOfGuid(This,guid,ppTinfo)	\
    (This)->lpVtbl -> GetTypeInfoOfGuid(This,guid,ppTinfo)

#define ITypeLib_GetLibAttr(This,ppTLibAttr)	\
    (This)->lpVtbl -> GetLibAttr(This,ppTLibAttr)

#define ITypeLib_GetTypeComp(This,ppTComp)	\
    (This)->lpVtbl -> GetTypeComp(This,ppTComp)

#define ITypeLib_GetDocumentation(This,index,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)	\
    (This)->lpVtbl -> GetDocumentation(This,index,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)

#define ITypeLib_IsName(This,szNameBuf,lHashVal,pfName)	\
    (This)->lpVtbl -> IsName(This,szNameBuf,lHashVal,pfName)

#define ITypeLib_FindName(This,szNameBuf,lHashVal,ppTInfo,rgMemId,pcFound)	\
    (This)->lpVtbl -> FindName(This,szNameBuf,lHashVal,ppTInfo,rgMemId,pcFound)

#define ITypeLib_ReleaseTLibAttr(This,pTLibAttr)	\
    (This)->lpVtbl -> ReleaseTLibAttr(This,pTLibAttr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetTypeInfoCount_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pcTInfo);


void __RPC_STUB ITypeLib_RemoteGetTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfo_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);


void __RPC_STUB ITypeLib_GetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoType_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ TYPEKIND __RPC_FAR *pTKind);


void __RPC_STUB ITypeLib_GetTypeInfoType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoOfGuid_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo);


void __RPC_STUB ITypeLib_GetTypeInfoOfGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ LPTLIBATTR __RPC_FAR *ppTLibAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);


void __RPC_STUB ITypeLib_RemoteGetLibAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeComp_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);


void __RPC_STUB ITypeLib_GetTypeComp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteGetDocumentation_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);


void __RPC_STUB ITypeLib_RemoteGetDocumentation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteIsName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [out] */ BOOL __RPC_FAR *pfName,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName);


void __RPC_STUB ITypeLib_RemoteIsName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_RemoteFindName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
    /* [out][in] */ USHORT __RPC_FAR *pcFound,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName);


void __RPC_STUB ITypeLib_RemoteFindName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_LocalReleaseTLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This);


void __RPC_STUB ITypeLib_LocalReleaseTLibAttr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeLib_INTERFACE_DEFINED__ */


#ifndef __ITypeLib2_INTERFACE_DEFINED__
#define __ITypeLib2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeLib2
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITypeLib2 __RPC_FAR *LPTYPELIB2;


EXTERN_C const IID IID_ITypeLib2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeLib2 : public ITypeLib
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCustData( 
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetLibStatistics( 
            /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
            /* [out] */ ULONG __RPC_FAR *pcchUniqueNames) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDocumentation2( 
            /* [in] */ INT index,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllCustData( 
            /* [out] */ CUSTDATA __RPC_FAR *pCustData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeLib2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeLib2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeLib2 __RPC_FAR * This);
        
        /* [local] */ UINT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITypeLib2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoType )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ UINT index,
            /* [out] */ TYPEKIND __RPC_FAR *pTKind);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoOfGuid )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLibAttr )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out] */ TLIBATTR __RPC_FAR *__RPC_FAR *ppTLibAttr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeComp )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ INT index,
            /* [out] */ BSTR __RPC_FAR *pBstrName,
            /* [out] */ BSTR __RPC_FAR *pBstrDocString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsName )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [out] */ BOOL __RPC_FAR *pfName);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindName )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out][in] */ LPOLESTR szNameBuf,
            /* [in] */ ULONG lHashVal,
            /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
            /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
            /* [out][in] */ USHORT __RPC_FAR *pcFound);
        
        /* [local] */ void ( STDMETHODCALLTYPE __RPC_FAR *ReleaseTLibAttr )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ TLIBATTR __RPC_FAR *pTLibAttr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCustData )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ REFGUID guid,
            /* [out] */ VARIANT __RPC_FAR *pVarVal);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLibStatistics )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
            /* [out] */ ULONG __RPC_FAR *pcchUniqueNames);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDocumentation2 )( 
            ITypeLib2 __RPC_FAR * This,
            /* [in] */ INT index,
            /* [in] */ LCID lcid,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
            /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
            /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllCustData )( 
            ITypeLib2 __RPC_FAR * This,
            /* [out] */ CUSTDATA __RPC_FAR *pCustData);
        
        END_INTERFACE
    } ITypeLib2Vtbl;

    interface ITypeLib2
    {
        CONST_VTBL struct ITypeLib2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeLib2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeLib2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeLib2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeLib2_GetTypeInfoCount(This)	\
    (This)->lpVtbl -> GetTypeInfoCount(This)

#define ITypeLib2_GetTypeInfo(This,index,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,index,ppTInfo)

#define ITypeLib2_GetTypeInfoType(This,index,pTKind)	\
    (This)->lpVtbl -> GetTypeInfoType(This,index,pTKind)

#define ITypeLib2_GetTypeInfoOfGuid(This,guid,ppTinfo)	\
    (This)->lpVtbl -> GetTypeInfoOfGuid(This,guid,ppTinfo)

#define ITypeLib2_GetLibAttr(This,ppTLibAttr)	\
    (This)->lpVtbl -> GetLibAttr(This,ppTLibAttr)

#define ITypeLib2_GetTypeComp(This,ppTComp)	\
    (This)->lpVtbl -> GetTypeComp(This,ppTComp)

#define ITypeLib2_GetDocumentation(This,index,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)	\
    (This)->lpVtbl -> GetDocumentation(This,index,pBstrName,pBstrDocString,pdwHelpContext,pBstrHelpFile)

#define ITypeLib2_IsName(This,szNameBuf,lHashVal,pfName)	\
    (This)->lpVtbl -> IsName(This,szNameBuf,lHashVal,pfName)

#define ITypeLib2_FindName(This,szNameBuf,lHashVal,ppTInfo,rgMemId,pcFound)	\
    (This)->lpVtbl -> FindName(This,szNameBuf,lHashVal,ppTInfo,rgMemId,pcFound)

#define ITypeLib2_ReleaseTLibAttr(This,pTLibAttr)	\
    (This)->lpVtbl -> ReleaseTLibAttr(This,pTLibAttr)


#define ITypeLib2_GetCustData(This,guid,pVarVal)	\
    (This)->lpVtbl -> GetCustData(This,guid,pVarVal)

#define ITypeLib2_GetLibStatistics(This,pcUniqueNames,pcchUniqueNames)	\
    (This)->lpVtbl -> GetLibStatistics(This,pcUniqueNames,pcchUniqueNames)

#define ITypeLib2_GetDocumentation2(This,index,lcid,pbstrHelpString,pdwHelpStringContext,pbstrHelpStringDll)	\
    (This)->lpVtbl -> GetDocumentation2(This,index,lcid,pbstrHelpString,pdwHelpStringContext,pbstrHelpStringDll)

#define ITypeLib2_GetAllCustData(This,pCustData)	\
    (This)->lpVtbl -> GetAllCustData(This,pCustData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITypeLib2_GetCustData_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ REFGUID guid,
    /* [out] */ VARIANT __RPC_FAR *pVarVal);


void __RPC_STUB ITypeLib2_GetCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_RemoteGetLibStatistics_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
    /* [out] */ ULONG __RPC_FAR *pcchUniqueNames);


void __RPC_STUB ITypeLib2_RemoteGetLibStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_RemoteGetDocumentation2_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);


void __RPC_STUB ITypeLib2_RemoteGetDocumentation2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeLib2_GetAllCustData_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ CUSTDATA __RPC_FAR *pCustData);


void __RPC_STUB ITypeLib2_GetAllCustData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeLib2_INTERFACE_DEFINED__ */


#ifndef __ITypeChangeEvents_INTERFACE_DEFINED__
#define __ITypeChangeEvents_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITypeChangeEvents
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ITypeChangeEvents __RPC_FAR *LPTYPECHANGEEVENTS;

typedef 
enum tagCHANGEKIND
    {	CHANGEKIND_ADDMEMBER	= 0,
	CHANGEKIND_DELETEMEMBER	= CHANGEKIND_ADDMEMBER + 1,
	CHANGEKIND_SETNAMES	= CHANGEKIND_DELETEMEMBER + 1,
	CHANGEKIND_SETDOCUMENTATION	= CHANGEKIND_SETNAMES + 1,
	CHANGEKIND_GENERAL	= CHANGEKIND_SETDOCUMENTATION + 1,
	CHANGEKIND_INVALIDATE	= CHANGEKIND_GENERAL + 1,
	CHANGEKIND_CHANGEFAILED	= CHANGEKIND_INVALIDATE + 1,
	CHANGEKIND_MAX	= CHANGEKIND_CHANGEFAILED + 1
    }	CHANGEKIND;


EXTERN_C const IID IID_ITypeChangeEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ITypeChangeEvents : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestTypeChange( 
            /* [in] */ CHANGEKIND changeKind,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfoBefore,
            /* [in] */ LPOLESTR pStrName,
            /* [out] */ INT __RPC_FAR *pfCancel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AfterTypeChange( 
            /* [in] */ CHANGEKIND changeKind,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfoAfter,
            /* [in] */ LPOLESTR pStrName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITypeChangeEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITypeChangeEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITypeChangeEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITypeChangeEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestTypeChange )( 
            ITypeChangeEvents __RPC_FAR * This,
            /* [in] */ CHANGEKIND changeKind,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfoBefore,
            /* [in] */ LPOLESTR pStrName,
            /* [out] */ INT __RPC_FAR *pfCancel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AfterTypeChange )( 
            ITypeChangeEvents __RPC_FAR * This,
            /* [in] */ CHANGEKIND changeKind,
            /* [in] */ ITypeInfo __RPC_FAR *pTInfoAfter,
            /* [in] */ LPOLESTR pStrName);
        
        END_INTERFACE
    } ITypeChangeEventsVtbl;

    interface ITypeChangeEvents
    {
        CONST_VTBL struct ITypeChangeEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeChangeEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITypeChangeEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITypeChangeEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITypeChangeEvents_RequestTypeChange(This,changeKind,pTInfoBefore,pStrName,pfCancel)	\
    (This)->lpVtbl -> RequestTypeChange(This,changeKind,pTInfoBefore,pStrName,pfCancel)

#define ITypeChangeEvents_AfterTypeChange(This,changeKind,pTInfoAfter,pStrName)	\
    (This)->lpVtbl -> AfterTypeChange(This,changeKind,pTInfoAfter,pStrName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITypeChangeEvents_RequestTypeChange_Proxy( 
    ITypeChangeEvents __RPC_FAR * This,
    /* [in] */ CHANGEKIND changeKind,
    /* [in] */ ITypeInfo __RPC_FAR *pTInfoBefore,
    /* [in] */ LPOLESTR pStrName,
    /* [out] */ INT __RPC_FAR *pfCancel);


void __RPC_STUB ITypeChangeEvents_RequestTypeChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITypeChangeEvents_AfterTypeChange_Proxy( 
    ITypeChangeEvents __RPC_FAR * This,
    /* [in] */ CHANGEKIND changeKind,
    /* [in] */ ITypeInfo __RPC_FAR *pTInfoAfter,
    /* [in] */ LPOLESTR pStrName);


void __RPC_STUB ITypeChangeEvents_AfterTypeChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITypeChangeEvents_INTERFACE_DEFINED__ */


#ifndef __IErrorInfo_INTERFACE_DEFINED__
#define __IErrorInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IErrorInfo
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ IErrorInfo __RPC_FAR *LPERRORINFO;


EXTERN_C const IID IID_IErrorInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGUID( 
            /* [out] */ GUID __RPC_FAR *pGUID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSource( 
            /* [out] */ BSTR __RPC_FAR *pBstrSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ BSTR __RPC_FAR *pBstrDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelpFile( 
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHelpContext( 
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IErrorInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IErrorInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IErrorInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IErrorInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGUID )( 
            IErrorInfo __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pGUID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSource )( 
            IErrorInfo __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pBstrSource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IErrorInfo __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pBstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHelpFile )( 
            IErrorInfo __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHelpContext )( 
            IErrorInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwHelpContext);
        
        END_INTERFACE
    } IErrorInfoVtbl;

    interface IErrorInfo
    {
        CONST_VTBL struct IErrorInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IErrorInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IErrorInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IErrorInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IErrorInfo_GetGUID(This,pGUID)	\
    (This)->lpVtbl -> GetGUID(This,pGUID)

#define IErrorInfo_GetSource(This,pBstrSource)	\
    (This)->lpVtbl -> GetSource(This,pBstrSource)

#define IErrorInfo_GetDescription(This,pBstrDescription)	\
    (This)->lpVtbl -> GetDescription(This,pBstrDescription)

#define IErrorInfo_GetHelpFile(This,pBstrHelpFile)	\
    (This)->lpVtbl -> GetHelpFile(This,pBstrHelpFile)

#define IErrorInfo_GetHelpContext(This,pdwHelpContext)	\
    (This)->lpVtbl -> GetHelpContext(This,pdwHelpContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IErrorInfo_GetGUID_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pGUID);


void __RPC_STUB IErrorInfo_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorInfo_GetSource_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrSource);


void __RPC_STUB IErrorInfo_GetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorInfo_GetDescription_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrDescription);


void __RPC_STUB IErrorInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorInfo_GetHelpFile_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);


void __RPC_STUB IErrorInfo_GetHelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IErrorInfo_GetHelpContext_Proxy( 
    IErrorInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext);


void __RPC_STUB IErrorInfo_GetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IErrorInfo_INTERFACE_DEFINED__ */


#ifndef __ICreateErrorInfo_INTERFACE_DEFINED__
#define __ICreateErrorInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICreateErrorInfo
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [local][unique][uuid][object] */ 


typedef /* [unique] */ ICreateErrorInfo __RPC_FAR *LPCREATEERRORINFO;


EXTERN_C const IID IID_ICreateErrorInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICreateErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGUID( 
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSource( 
            /* [in] */ LPOLESTR szSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [in] */ LPOLESTR szDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpFile( 
            /* [in] */ LPOLESTR szHelpFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpContext( 
            /* [in] */ DWORD dwHelpContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateErrorInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateErrorInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateErrorInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGUID )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSource )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ LPOLESTR szSource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDescription )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ LPOLESTR szDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpFile )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ LPOLESTR szHelpFile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHelpContext )( 
            ICreateErrorInfo __RPC_FAR * This,
            /* [in] */ DWORD dwHelpContext);
        
        END_INTERFACE
    } ICreateErrorInfoVtbl;

    interface ICreateErrorInfo
    {
        CONST_VTBL struct ICreateErrorInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateErrorInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateErrorInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateErrorInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateErrorInfo_SetGUID(This,rguid)	\
    (This)->lpVtbl -> SetGUID(This,rguid)

#define ICreateErrorInfo_SetSource(This,szSource)	\
    (This)->lpVtbl -> SetSource(This,szSource)

#define ICreateErrorInfo_SetDescription(This,szDescription)	\
    (This)->lpVtbl -> SetDescription(This,szDescription)

#define ICreateErrorInfo_SetHelpFile(This,szHelpFile)	\
    (This)->lpVtbl -> SetHelpFile(This,szHelpFile)

#define ICreateErrorInfo_SetHelpContext(This,dwHelpContext)	\
    (This)->lpVtbl -> SetHelpContext(This,dwHelpContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetGUID_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid);


void __RPC_STUB ICreateErrorInfo_SetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetSource_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szSource);


void __RPC_STUB ICreateErrorInfo_SetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetDescription_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szDescription);


void __RPC_STUB ICreateErrorInfo_SetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetHelpFile_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ LPOLESTR szHelpFile);


void __RPC_STUB ICreateErrorInfo_SetHelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICreateErrorInfo_SetHelpContext_Proxy( 
    ICreateErrorInfo __RPC_FAR * This,
    /* [in] */ DWORD dwHelpContext);


void __RPC_STUB ICreateErrorInfo_SetHelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateErrorInfo_INTERFACE_DEFINED__ */


#ifndef __ISupportErrorInfo_INTERFACE_DEFINED__
#define __ISupportErrorInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISupportErrorInfo
 * at Sat Jul 13 21:56:52 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ISupportErrorInfo __RPC_FAR *LPSUPPORTERRORINFO;


EXTERN_C const IID IID_ISupportErrorInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISupportErrorInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo( 
            /* [in] */ REFIID riid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISupportErrorInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISupportErrorInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISupportErrorInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISupportErrorInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InterfaceSupportsErrorInfo )( 
            ISupportErrorInfo __RPC_FAR * This,
            /* [in] */ REFIID riid);
        
        END_INTERFACE
    } ISupportErrorInfoVtbl;

    interface ISupportErrorInfo
    {
        CONST_VTBL struct ISupportErrorInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISupportErrorInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISupportErrorInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISupportErrorInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISupportErrorInfo_InterfaceSupportsErrorInfo(This,riid)	\
    (This)->lpVtbl -> InterfaceSupportsErrorInfo(This,riid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISupportErrorInfo_InterfaceSupportsErrorInfo_Proxy( 
    ISupportErrorInfo __RPC_FAR * This,
    /* [in] */ REFIID riid);


void __RPC_STUB ISupportErrorInfo_InterfaceSupportsErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISupportErrorInfo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  CLEANLOCALSTORAGE_UserSize(     unsigned long __RPC_FAR *, unsigned long            , CLEANLOCALSTORAGE __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  CLEANLOCALSTORAGE_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, CLEANLOCALSTORAGE __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  CLEANLOCALSTORAGE_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, CLEANLOCALSTORAGE __RPC_FAR * ); 
void                      __RPC_USER  CLEANLOCALSTORAGE_UserFree(     unsigned long __RPC_FAR *, CLEANLOCALSTORAGE __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* [local] */ HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Proxy( 
    IDispatch __RPC_FAR * This,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Stub( 
    IDispatch __RPC_FAR * This,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *pArgErr,
    /* [in] */ UINT cVarRef,
    /* [size_is][in] */ UINT __RPC_FAR *rgVarRefIdx,
    /* [size_is][out][in] */ VARIANTARG __RPC_FAR *rgVarRef);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumVARIANT_Next_Proxy( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
    /* [out] */ ULONG __RPC_FAR *pCeltFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumVARIANT_Next_Stub( 
    IEnumVARIANT __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
    /* [out] */ ULONG __RPC_FAR *pCeltFetched);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeComp_Bind_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [in] */ WORD wFlags,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [out] */ DESCKIND __RPC_FAR *pDescKind,
    /* [out] */ BINDPTR __RPC_FAR *pBindPtr);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_Bind_Stub( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [in] */ WORD wFlags,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [out] */ DESCKIND __RPC_FAR *pDescKind,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTypeComp,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeComp_BindType_Proxy( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeComp_BindType_Stub( 
    ITypeComp __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ ULONG lHashVal,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetTypeAttr_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ LPTYPEATTR __RPC_FAR *ppTypeAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetFuncDesc_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPFUNCDESC __RPC_FAR *ppFuncDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetVarDesc_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ UINT index,
    /* [out] */ LPVARDESC __RPC_FAR *ppVarDesc,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetNames_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
    /* [in] */ UINT cMaxNames,
    /* [out] */ UINT __RPC_FAR *pcNames);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetNames_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
    /* [in] */ UINT cMaxNames,
    /* [out] */ UINT __RPC_FAR *pcNames);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_Invoke_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ PVOID pvInstance,
    /* [in] */ MEMBERID memid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_Invoke_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pIUnk,
    /* [in] */ MEMBERID memid,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *pArgErr,
    /* [in] */ UINT cVarRef,
    /* [size_is][in] */ UINT __RPC_FAR *rgVarRefIdx,
    /* [size_is][out][in] */ VARIANTARG __RPC_FAR *rgVarRef);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetDocumentation_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetDocumentation_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetDllEntry_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [out] */ BSTR __RPC_FAR *pBstrDllName,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ WORD __RPC_FAR *pwOrdinal);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetDllEntry_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrDllName,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ WORD __RPC_FAR *pwOrdinal);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_AddressOfMember_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ INVOKEKIND invKind,
    /* [out] */ PVOID __RPC_FAR *ppv);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_AddressOfMember_Stub( 
    ITypeInfo __RPC_FAR * This);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_CreateInstance_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_CreateInstance_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetContainingTypeLib_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
    /* [out] */ UINT __RPC_FAR *pIndex);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_GetContainingTypeLib_Stub( 
    ITypeInfo __RPC_FAR * This,
    /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
    /* [out] */ UINT __RPC_FAR *pIndex);

/* [local] */ void STDMETHODCALLTYPE ITypeInfo_ReleaseTypeAttr_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_ReleaseTypeAttr_Stub( 
    ITypeInfo __RPC_FAR * This);

/* [local] */ void STDMETHODCALLTYPE ITypeInfo_ReleaseFuncDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_ReleaseFuncDesc_Stub( 
    ITypeInfo __RPC_FAR * This);

/* [local] */ void STDMETHODCALLTYPE ITypeInfo_ReleaseVarDesc_Proxy( 
    ITypeInfo __RPC_FAR * This,
    /* [in] */ VARDESC __RPC_FAR *pVarDesc);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo_ReleaseVarDesc_Stub( 
    ITypeInfo __RPC_FAR * This);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeInfo2_GetDocumentation2_Proxy( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ LCID lcid,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeInfo2_GetDocumentation2_Stub( 
    ITypeInfo2 __RPC_FAR * This,
    /* [in] */ MEMBERID memid,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);

/* [local] */ UINT STDMETHODCALLTYPE ITypeLib_GetTypeInfoCount_Proxy( 
    ITypeLib __RPC_FAR * This);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_GetTypeInfoCount_Stub( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pcTInfo);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib_GetLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ TLIBATTR __RPC_FAR *__RPC_FAR *ppTLibAttr);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_GetLibAttr_Stub( 
    ITypeLib __RPC_FAR * This,
    /* [out] */ LPTLIBATTR __RPC_FAR *ppTLibAttr,
    /* [out] */ CLEANLOCALSTORAGE __RPC_FAR *pDummy);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib_GetDocumentation_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ INT index,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_GetDocumentation_Stub( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pBstrName,
    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib_IsName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out][in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [out] */ BOOL __RPC_FAR *pfName);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_IsName_Stub( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [out] */ BOOL __RPC_FAR *pfName,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib_FindName_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [out][in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
    /* [out][in] */ USHORT __RPC_FAR *pcFound);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_FindName_Stub( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ LPOLESTR szNameBuf,
    /* [in] */ ULONG lHashVal,
    /* [length_is][size_is][out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo,
    /* [length_is][size_is][out] */ MEMBERID __RPC_FAR *rgMemId,
    /* [out][in] */ USHORT __RPC_FAR *pcFound,
    /* [out] */ BSTR __RPC_FAR *pBstrLibName);

/* [local] */ void STDMETHODCALLTYPE ITypeLib_ReleaseTLibAttr_Proxy( 
    ITypeLib __RPC_FAR * This,
    /* [in] */ TLIBATTR __RPC_FAR *pTLibAttr);


/* [nocode][call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib_ReleaseTLibAttr_Stub( 
    ITypeLib __RPC_FAR * This);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib2_GetLibStatistics_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
    /* [out] */ ULONG __RPC_FAR *pcchUniqueNames);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_GetLibStatistics_Stub( 
    ITypeLib2 __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcUniqueNames,
    /* [out] */ ULONG __RPC_FAR *pcchUniqueNames);

/* [local] */ HRESULT STDMETHODCALLTYPE ITypeLib2_GetDocumentation2_Proxy( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ LCID lcid,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);


/* [call_as] */ HRESULT STDMETHODCALLTYPE ITypeLib2_GetDocumentation2_Stub( 
    ITypeLib2 __RPC_FAR * This,
    /* [in] */ INT index,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD refPtrFlags,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpString,
    /* [out] */ DWORD __RPC_FAR *pdwHelpStringContext,
    /* [out] */ BSTR __RPC_FAR *pbstrHelpStringDll);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
