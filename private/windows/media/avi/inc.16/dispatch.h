/*** 
*dispatch.h - Application Programmability definitions.
*
*  Copyright (C) 1992-1993, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*  This file defines the Ole Automation interfaces and APIs.
*
*Implementation Notes:
*  This file requires ole2.h
*
*****************************************************************************/

#ifndef _DISPATCH_H_
#define _DISPATCH_H_


#include "variant.h"


#ifdef _MAC

DEFINE_OLEGUID(IID_IDispatch,		0x00, 0x04, 0x02, 0, 0, 0, 0, 0); 
DEFINE_OLEGUID(IID_IEnumVARIANT,	0x04, 0x04, 0x02, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_ITypeInfo,		0x01, 0x04, 0x02, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_ITypeLib,		0x02, 0x04, 0x02, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_ITypeComp,		0x03, 0x04, 0x02, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_ICreateTypeInfo,	0x05, 0x04, 0x02, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_ICreateTypeLib,	0x06, 0x04, 0x02, 0, 0, 0, 0, 0);

#else

DEFINE_OLEGUID(IID_IDispatch,		0x00020400L, 0, 0);
DEFINE_OLEGUID(IID_IEnumVARIANT,	0x00020404L, 0, 0);
DEFINE_OLEGUID(IID_ITypeInfo,		0x00020401L, 0, 0);
DEFINE_OLEGUID(IID_ITypeLib,		0x00020402L, 0, 0);
DEFINE_OLEGUID(IID_ITypeComp,		0x00020403L, 0, 0);
DEFINE_OLEGUID(IID_ICreateTypeInfo,	0x00020405L, 0, 0);
DEFINE_OLEGUID(IID_ICreateTypeLib,	0x00020406L, 0, 0);

#endif


/* forward declarations */
#ifdef __cplusplus
interface IDispatch;
interface IEnumVARIANT;
interface ITypeInfo;
interface ITypeLib;
interface ITypeComp;
interface ICreateTypeInfo;
interface ICreateTypeLib;
#else
typedef interface IDispatch IDispatch;
typedef interface IEnumVARIANT IEnumVARIANT;
typedef interface ITypeInfo ITypeInfo;
typedef interface ITypeLib ITypeLib;
typedef interface ITypeComp ITypeComp;
typedef interface ICreateTypeInfo ICreateTypeInfo;
typedef interface ICreateTypeLib ICreateTypeLib;
#endif


/* IDispatch related error codes */

#define DISP_ERROR(X) MAKE_SCODE(SEVERITY_ERROR, FACILITY_DISPATCH, X)

#define DISP_E_UNKNOWNINTERFACE		DISP_ERROR(1)
#define DISP_E_MEMBERNOTFOUND		DISP_ERROR(3)
#define DISP_E_PARAMNOTFOUND		DISP_ERROR(4)
#define DISP_E_TYPEMISMATCH		DISP_ERROR(5)
#define DISP_E_UNKNOWNNAME		DISP_ERROR(6)
#define DISP_E_NONAMEDARGS		DISP_ERROR(7)
#define DISP_E_BADVARTYPE		DISP_ERROR(8)
#define DISP_E_EXCEPTION		DISP_ERROR(9)
#define DISP_E_OVERFLOW			DISP_ERROR(10)
#define DISP_E_BADINDEX			DISP_ERROR(11)
#define DISP_E_UNKNOWNLCID		DISP_ERROR(12)
#define DISP_E_ARRAYISLOCKED		DISP_ERROR(13)
#define DISP_E_BADPARAMCOUNT		DISP_ERROR(14)
#define DISP_E_PARAMNOTOPTIONAL		DISP_ERROR(15)
#define DISP_E_BADCALLEE		DISP_ERROR(16)
#define DISP_E_NOTACOLLECTION		DISP_ERROR(17)


#define TYPE_ERROR(X) MAKE_SCODE(SEVERITY_ERROR, FACILITY_DISPATCH, X)

#define TYPE_E_IOERROR			TYPE_ERROR(   57)
#define TYPE_E_COMPILEERROR		TYPE_ERROR(   90)
#define TYPE_E_CANTCREATETMPFILE	TYPE_ERROR(  322)
#define TYPE_E_ILLEGALINDEX		TYPE_ERROR(  341)
#define TYPE_E_IDNOTFOUND		TYPE_ERROR( 1000)
#define TYPE_E_BUFFERTOOSMALL		TYPE_ERROR(32790)
#define TYPE_E_READONLY 		TYPE_ERROR(32791)
#define TYPE_E_INVDATAREAD		TYPE_ERROR(32792)
#define TYPE_E_UNSUPFORMAT		TYPE_ERROR(32793)
#define TYPE_E_ALREADYCONTAINSNAME	TYPE_ERROR(32794)
#define TYPE_E_NOMATCHINGARITY		TYPE_ERROR(32795)
#define TYPE_E_REGISTRYACCESS		TYPE_ERROR(32796)
#define TYPE_E_LIBNOTREGISTERED 	TYPE_ERROR(32797)
#define TYPE_E_DUPLICATEDEFINITION	TYPE_ERROR(32798)
#define TYPE_E_USAGE			TYPE_ERROR(32799)
#define TYPE_E_DESTNOTKNOWN		TYPE_ERROR(32800)
#define TYPE_E_UNDEFINEDTYPE		TYPE_ERROR(32807)
#define TYPE_E_QUALIFIEDNAMEDISALLOWED	TYPE_ERROR(32808)
#define TYPE_E_INVALIDSTATE		TYPE_ERROR(32809)
#define TYPE_E_WRONGTYPEKIND		TYPE_ERROR(32810)
#define TYPE_E_ELEMENTNOTFOUND		TYPE_ERROR(32811)
#define TYPE_E_AMBIGUOUSNAME		TYPE_ERROR(32812)
#define TYPE_E_INVOKEFUNCTIONMISMATCH	TYPE_ERROR(32813)
#define TYPE_E_DLLFUNCTIONNOTFOUND	TYPE_ERROR(32814)
#define TYPE_E_BADMODULEKIND		TYPE_ERROR(35005)
#define TYPE_E_WRONGPLATFORM		TYPE_ERROR(35006)
#define TYPE_E_ALREADYBEINGLAIDOUT	TYPE_ERROR(35007)
#define TYPE_E_CANTLOADLIBRARY		TYPE_ERROR(40010)


/* if not already picked up from olenls.h */
#ifndef _LCID_DEFINED
typedef DWORD LCID;
# define _LCID_DEFINED
#endif


/*---------------------------------------------------------------------*/
/*                            BSTR API                                 */
/*---------------------------------------------------------------------*/


STDAPI_(BSTR) SysAllocString(char FAR*);
STDAPI_(BOOL) SysReAllocString(BSTR FAR*, char FAR*);
STDAPI_(BSTR) SysAllocStringLen(char FAR*, UINT);
STDAPI_(BOOL) SysReAllocStringLen(BSTR FAR*, char FAR*, UINT);
STDAPI_(void) SysFreeString(BSTR);
STDAPI_(UINT) SysStringLen(BSTR);


/*---------------------------------------------------------------------*/
/*                            Time API                                 */
/*---------------------------------------------------------------------*/

STDAPI_(BOOL)
DosDateTimeToVariantTime(
    WORD wDosDate,
    WORD wDosTime,
    double FAR* pvtime);

STDAPI_(BOOL)
VariantTimeToDosDateTime(
    double vtime,
    WORD FAR* pwDosDate,
    WORD FAR* pwDosTime);


/*---------------------------------------------------------------------*/
/*                          SafeArray API                              */
/*---------------------------------------------------------------------*/

STDAPI_(SAFEARRAY FAR*)
SafeArrayCreate(
    VARTYPE vt,
    UINT cDims,
    SAFEARRAYBOUND FAR* rgsabound);

STDAPI SafeArrayDestroy(SAFEARRAY FAR* psa);

STDAPI_(UINT) SafeArrayGetDim(SAFEARRAY FAR* psa);

STDAPI_(UINT) SafeArrayGetElemsize(SAFEARRAY FAR* psa);

STDAPI
SafeArrayGetUBound(
    SAFEARRAY FAR* psa,
    UINT nDim,
    LONG FAR* plUbound);

STDAPI
SafeArrayGetLBound(
    SAFEARRAY FAR* psa,
    UINT nDim,
    LONG FAR* plLbound);

STDAPI SafeArrayLock(SAFEARRAY FAR* psa);

STDAPI SafeArrayUnlock(SAFEARRAY FAR* psa);

STDAPI SafeArrayAccessData(SAFEARRAY FAR* psa, void FAR* HUGEP* ppvData);

STDAPI SafeArrayUnaccessData(SAFEARRAY FAR* psa);

STDAPI
SafeArrayGetElement(
    SAFEARRAY FAR* psa,
    LONG FAR* rgIndices,
    void FAR* pv);

STDAPI
SafeArrayPutElement(
    SAFEARRAY FAR* psa,
    LONG FAR* rgIndices,
    void FAR* pv);

/* return a copy of the given SafeArray
 */
STDAPI
SafeArrayCopy(
    SAFEARRAY FAR* psa,
    SAFEARRAY FAR* FAR* ppsaOut);


/*---------------------------------------------------------------------*/
/*                           VARIANT API                               */
/*---------------------------------------------------------------------*/

STDAPI_(void)
VariantInit(
    VARIANTARG FAR* pvarg);

STDAPI
    VariantClear(VARIANTARG FAR* pvarg);

STDAPI
VariantCopy(
    VARIANTARG FAR* pvargDest,
    VARIANTARG FAR* pvargSrc);

STDAPI
VariantCopyInd(
    VARIANT FAR* pvarDest,
    VARIANTARG FAR* pvargSrc);

STDAPI
VariantChangeType(
    VARIANTARG FAR* pvargDest,
    VARIANTARG FAR* pvarSrc,
    WORD wFlags,
    VARTYPE vt);

#define VARIANT_NOVALUEPROP 1


/*---------------------------------------------------------------------*/
/*                             ITypeLib                                */
/*---------------------------------------------------------------------*/

typedef struct FARSTRUCT tagTLIBATTR {
    LCID lcid;			/* locale of the TypeLibrary */
    WORD wMajorVerNum;		/* major version number	*/
    WORD wMinorVerNum;		/* minor version number	*/
    GUID guid;			/* globally unique library id */
} TLIBATTR, FAR* LPTLIBATTR;


#undef  INTERFACE
#define INTERFACE ITypeLib

DECLARE_INTERFACE_(ITypeLib, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* ITypeLib methods */
    STDMETHOD_(UINT,GetTypeInfoCount)(THIS) PURE;

    STDMETHOD(GetTypeInfo)(THIS_
      UINT index, ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetTypeInfoOfGuid)(THIS_
      REFGUID guid, ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetLibAttr)(THIS_
      TLIBATTR FAR* FAR* pptlibattr) PURE;

    STDMETHOD(GetTypeComp)(THIS_
      ITypeComp FAR* FAR* pptcomp) PURE;

    STDMETHOD(GetDocumentation)(THIS_
      int index,
      BSTR FAR* pbstrName,
      BSTR FAR* pbstrDocString,
      DWORD FAR* pdwHelpContext,
      BSTR FAR* pbstrHelpFile) PURE;

    STDMETHOD(IsName)(THIS_
      LPSTR szNameBuf, WORD wHashVal, BOOL FAR* pfName) PURE;

    STDMETHOD_(void, ReleaseTLibAttr)(THIS_ TLIBATTR FAR* ptlibattr) PURE;

    STDMETHOD(Load)(THIS_
      IStorage FAR* pstg, LPSTR szFileName) PURE;
};

typedef ITypeLib FAR* LPTYPELIB;


/*---------------------------------------------------------------------*/
/*                            ITypeInfo                                */
/*---------------------------------------------------------------------*/


typedef LONG DISPID;
typedef DISPID MEMBERID;

#define MEMBERID_NIL DISPID_UNKNOWN
#define ID_DEFAULTINST	-2

typedef DWORD HREFTYPE;

typedef enum tagTYPEKIND {
    TKIND_ENUM = 0,
    TKIND_RECORD,
    TKIND_MODULE,
    TKIND_INTERFACE,
    TKIND_DISPATCH,
    TKIND_COCLASS,
    TKIND_ALIAS,
    TKIND_UNION,
    TKIND_ENCUNION,
    TKIND_Class,
    TKIND_MAX			/* end of enum marker */
} TYPEKIND;


typedef struct FARSTRUCT tagTYPEDESC {
    VARTYPE vt;
    union {
      /* VT_PTR - the pointed-at type */
      struct FARSTRUCT tagTYPEDESC FAR* lptdesc;

      /* VT_CARRAY */
      struct FARSTRUCT tagARRAYDESC FAR* lpadesc;

      /* VT_USERDEFINED - this is used to get a TypeInfo for the UDT */
      HREFTYPE hreftype;
    }
#ifdef NONAMELESSUNION
    u
#endif
    ;
} TYPEDESC;


typedef struct FARSTRUCT tagARRAYDESC {
    TYPEDESC tdescElem;		/* element type */
    USHORT cDims;		/* dimension count */
    SAFEARRAYBOUND rgbounds[1];	/* variable length array of bounds */
} ARRAYDESC;


typedef struct FARSTRUCT tagIDLDESC {
    WORD wIDLFlags;		/* IN, OUT, etc */
    BSTR bstrIDLInfo;
} IDLDESC, FAR* LPIDLDESC;

#define IDLFLAG_NONE	0
#define IDLFLAG_FIN	0x1
#define IDLFLAG_FOUT	0x2

typedef struct FARSTRUCT tagELEMDESC {
    TYPEDESC tdesc;		/* the type of the element */
    IDLDESC idldesc;		/* info for remoting the element */ 
} ELEMDESC, FAR* LPELEMDESC;


typedef struct FARSTRUCT tagTYPEATTR {
    TYPEKIND typekind;		/* the kind of type this typeinfo describes */
    WORD wMajorVerNum;		/* major version number */
    WORD wMinorVerNum;		/* minor version number */
    LCID lcid;			/* locale of member names and doc strings */
    WORD cFuncs;		/* number of functions */
    WORD cVars;			/* number of variables / data members */
    WORD cImplTypes;		/* number of implemented interfaces */
    TYPEDESC tdescAlias;	/* if typekind == TKIND_ALIAS this field
				   specifies the type for which this type
				   is an alias */
    GUID guid;			/* the GUID of the TypeInfo */
    WORD wTypeFlags;
    DWORD dwReserved;
    WORD cbAlignment;		/* specifies the alignment requirements for
				   an instance of this type,
				     0 = align on 64k boundary
				     1 = byte align
				     2 = word align
				     4 = dword align... */
    WORD cbSizeInstance;	/* the size of an instance of this type */
    WORD cbSizeVft;		/* the size of this types virtual func table */
    IDLDESC idldescType;        /* IDL attributes of the described type */
    MEMBERID memidConstructor;	/* ID of constructor, MEMBERID_NIL if none */
    MEMBERID memidDestructor;	/* ID of destructor, MEMBERID_NIL if none */
} TYPEATTR, FAR* LPTYPEATTR;


typedef struct FARSTRUCT tagDISPPARAMS{
    VARIANTARG FAR* rgvarg;
    DISPID FAR* rgdispidNamedArgs;
    UINT cArgs;
    UINT cNamedArgs;
} DISPPARAMS;


typedef struct FARSTRUCT tagEXCEPINFO {
    WORD wCode;             /* An error code describing the error. */
    WORD wReserved;

    BSTR bstrSource;	    /* A textual, human readable name of the
			       source of the exception. It is up to the
			       IDispatch implementor to fill this in.
			       Typically this will be an application name. */

    BSTR bstrDescription;   /* A textual, human readable description of the
			       error. If no description is available, NULL
			       should be used. */

    BSTR bstrHelpFile;      /* Fully qualified drive, path, and file name
			       of a help file with more information about
			       the error.  If no help is available, NULL
			       should be used. */

    DWORD dwHelpContext;    /* help context of topic within the help file. */

    void FAR* pvReserved;

    HRESULT (STDAPICALLTYPE FAR* pfnDeferredFillIn)(struct tagEXCEPINFO FAR*);
			    /* Use of this field allows an application
			       to defer filling in the bstrDescription,
			       bstrHelpFile, and dwHelpContext fields
			       until they are needed.  This field might
			       be used, for example, if loading the
			       string for the error is a time-consuming
			       operation. If deferred fill-in is not
			       desired, this field should be set to NULL. */
    DWORD dwReserved;
} EXCEPINFO, FAR* LPEXCEPINFO;


typedef enum tagCALLCONV {
    CC_CDECL = 1,
    CC_MSCPASCAL,
    CC_PASCAL = CC_MSCPASCAL,
    CC_MACPASCAL,
    CC_STDCALL,
    CC_THISCALL,
    CC_MAX			/* end of enum marker */
} CALLCONV;


typedef enum tagFUNCKIND {
    FUNC_VIRTUAL,
    FUNC_PUREVIRTUAL,
    FUNC_NONVIRTUAL,
    FUNC_STATIC,
    FUNC_DISPATCH
} FUNCKIND;


/* Flags for IDispatch::Invoke */
#define DISPATCH_METHOD		0x1
#define DISPATCH_PROPERTYGET	0x2
#define DISPATCH_PROPERTYPUT	0x4
#define DISPATCH_PROPERTYPUTREF	0x8


typedef enum tagINVOKEKIND {
    INVOKE_FUNC = DISPATCH_METHOD,
    INVOKE_PROPERTYGET = DISPATCH_PROPERTYGET,
    INVOKE_PROPERTYPUT = DISPATCH_PROPERTYPUT,
    INVOKE_PROPERTYPUTREF = DISPATCH_PROPERTYPUTREF
} INVOKEKIND;


typedef struct FARSTRUCT tagFUNCDESC {
    MEMBERID memid;
    FUNCKIND funckind;
    INVOKEKIND invkind;
    CALLCONV callconv;
    SHORT cParams;
    SHORT cParamsOpt;
    SHORT oVft;
    WORD wFuncFlags;
    ELEMDESC elemdescFunc;
    ELEMDESC FAR* lprgelemdescParam;  /* array of parameter types */
    SHORT cScodes;
    SCODE FAR* lprgscode;
} FUNCDESC, FAR* LPFUNCDESC;


typedef enum tagVARKIND {
    VAR_PERINSTANCE,
    VAR_STATIC,
    VAR_CONST,
    VAR_DISPATCH
} VARKIND;


typedef struct FARSTRUCT tagVARDESC {
    MEMBERID memid;
    WORD wVarFlags;
    VARKIND varkind;
    ELEMDESC elemdescVar;
    union {
      ULONG oInst;		/* VAR_PERINSTANCE - the offset of this
				   variable within the instance */
      VARIANT FAR* lpvarValue;  /* VAR_CONST - the value of the constant */
    }
#ifdef NONAMELESSUNION
    u
#endif
    ;
} VARDESC, FAR* LPVARDESC;


typedef enum tagTYPEFLAGS {
    TYPEFLAG_FAPPOBJECT = 1,
    TYPEFLAG_FCANCREATE = 2
} TYPEFLAGS;


typedef enum tagFUNCFLAGS {
    FUNCFLAG_FRESTRICTED = 1
} FUNCFLAGS;


typedef enum tagVARFLAGS {
    VARFLAG_FREADONLY = 1
} VARFLAGS;


#undef  INTERFACE
#define INTERFACE ITypeInfo

DECLARE_INTERFACE_(ITypeInfo, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* ITypeInfo methods */
    STDMETHOD(GetTypeAttr)(THIS_ TYPEATTR FAR* FAR* pptypeattr) PURE;

    STDMETHOD(GetTypeComp)(THIS_ ITypeComp FAR* FAR* pptcomp) PURE;

    STDMETHOD(GetFuncDesc)(THIS_
      UINT index, FUNCDESC FAR* FAR* ppfuncdesc) PURE;

    STDMETHOD(GetVarDesc)(THIS_
      UINT index, VARDESC FAR* FAR* ppvardesc) PURE;

    STDMETHOD(GetNames)(THIS_
      MEMBERID memid,
      BSTR FAR* rgbstrNames,
      UINT cMaxNames,
      UINT FAR* pcNames) PURE;

    STDMETHOD(GetRefTypeOfImplType)(THIS_
      UINT index, HREFTYPE FAR* phreftype) PURE;

    STDMETHOD(GetIDsOfNames)(THIS_
      char FAR* FAR* rgszNames, UINT cNames, MEMBERID FAR* rgmemid) PURE;

    STDMETHOD(Invoke)(THIS_
      void FAR* pvInstance,
      MEMBERID memid,
      WORD wFlags,
      DISPPARAMS FAR *pdispparams,
      VARIANT FAR *pvarResult,
      EXCEPINFO FAR *pexcepinfo,
      UINT FAR *puArgErr) PURE;

    STDMETHOD(GetDocumentation)(THIS_
      MEMBERID memid,
      BSTR FAR* pbstrName,
      BSTR FAR* pbstrDocString,
      DWORD FAR* pdwHelpContext,
      BSTR FAR* pbstrHelpFile) PURE;

    STDMETHOD(GetDllEntry)(THIS_
      MEMBERID memid,
      BSTR FAR* pbstrDllName,
      BSTR FAR* pbstrName,
      WORD FAR* pwOrdinal) PURE;

    STDMETHOD(GetRefTypeInfo)(THIS_
      HREFTYPE hreftype, ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(AddressOfMember)(THIS_
      MEMBERID memid, INVOKEKIND invkind, void FAR* FAR* ppv) PURE;

    STDMETHOD(CreateInstance)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;

    STDMETHOD(GetMops)(THIS_ MEMBERID memid, BSTR FAR* pbstrMops) PURE;

    STDMETHOD(GetContainingTypeLib)(THIS_
      ITypeLib FAR* FAR* pptlib, UINT FAR* pindex) PURE;

    STDMETHOD_(void, ReleaseTypeAttr)(THIS_ TYPEATTR FAR* ptypeattr) PURE;
    STDMETHOD_(void, ReleaseFuncDesc)(THIS_ FUNCDESC FAR* pfuncdesc) PURE;
    STDMETHOD_(void, ReleaseVarDesc)(THIS_ VARDESC FAR* pvardesc) PURE;
};

typedef ITypeInfo FAR* LPTYPEINFO;


/*---------------------------------------------------------------------*/
/*                            ITypeComp                                */
/*---------------------------------------------------------------------*/


typedef enum tagDESCKIND {
    DESCKIND_NONE = 0,
    DESCKIND_FUNCDESC,
    DESCKIND_VARDESC,
    DESCKIND_TYPECOMP,
    DESCKIND_MAX		/* end of enum marker */
} DESCKIND;


typedef union tagBINDPTR {
    FUNCDESC FAR* lpfuncdesc;
    VARDESC FAR* lpvardesc;
    ITypeComp FAR* lptcomp;
} BINDPTR, FAR* LPBINDPTR;


#undef  INTERFACE
#define INTERFACE ITypeComp

DECLARE_INTERFACE_(ITypeComp, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* ITypeComp methods */
    STDMETHOD(Bind)(THIS_
      LPSTR szName,
      WORD wHashVal,
      WORD wflags,
      ITypeInfo FAR* FAR* pptinfo,
      DESCKIND FAR* pdesckind,
      BINDPTR FAR* pbindptr) PURE;

    STDMETHOD(BindType)(THIS_
      LPSTR szName,
      WORD wHashVal,
      ITypeInfo FAR* FAR* pptinfo,
      ITypeComp FAR* FAR* pptcomp) PURE;
};

typedef ITypeComp FAR* LPTYPECOMP;


/*---------------------------------------------------------------------*/
/*                         ICreateTypeLib                              */
/*---------------------------------------------------------------------*/


#undef  INTERFACE
#define INTERFACE ICreateTypeLib

DECLARE_INTERFACE_(ICreateTypeLib, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* ICreateTypeLib methods */
    STDMETHOD(CreateTypeInfo)(THIS_
      LPSTR szName,
      TYPEKIND tkind,
      ICreateTypeInfo FAR* FAR* lplpictinfo) PURE;

    STDMETHOD(SetName)(THIS_ LPSTR szName) PURE;

    STDMETHOD(SetVersion)(THIS_
      WORD wMajorVerNum, WORD wMinorVerNum) PURE;

    STDMETHOD(SetGuid) (THIS_ REFGUID guid) PURE;

    STDMETHOD(SetDocString)(THIS_ LPSTR szDoc) PURE;

    STDMETHOD(SetHelpFileName)(THIS_ LPSTR szHelpFileName) PURE;

    STDMETHOD(SetHelpContext)(THIS_ DWORD dwHelpContext) PURE;

    STDMETHOD(SetLcid)(THIS_ LCID lcid) PURE;

    STDMETHOD(SaveAllChanges)(THIS_ IStorage FAR* pstg) PURE;
};

typedef ICreateTypeLib FAR* LPCREATETYPELIB;


/*---------------------------------------------------------------------*/
/*                         ICreateTypeInfo                             */
/*---------------------------------------------------------------------*/

#undef  INTERFACE
#define INTERFACE ICreateTypeInfo

DECLARE_INTERFACE_(ICreateTypeInfo, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* ICreateTypeInfo methods */
    STDMETHOD(SetGuid)(THIS_ REFGUID guid) PURE;

    STDMETHOD(SetTypeFlags)(THIS_ UINT uTypeFlags) PURE;

    STDMETHOD(SetDocString)(THIS_ LPSTR pstrDoc) PURE;

    STDMETHOD(SetHelpContext)(THIS_ DWORD dwHelpContext) PURE;

    STDMETHOD(SetVersion)(THIS_
      WORD wMajorVerNum, WORD wMinorVerNum) PURE;

    STDMETHOD(AddRefTypeInfo)(THIS_
      ITypeInfo FAR* ptinfo, HREFTYPE FAR* phreftype) PURE;

    STDMETHOD(AddFuncDesc)(THIS_
      UINT index, FUNCDESC FAR* pfuncdesc) PURE;

    STDMETHOD(AddImplType)(THIS_
      UINT index, HREFTYPE hreftype) PURE;

    STDMETHOD(AddVarDesc)(THIS_
      UINT index, VARDESC FAR* pvardesc) PURE;

    STDMETHOD(SetFuncAndParamNames)(THIS_
      UINT index, LPSTR FAR* rgszNames, UINT cNames) PURE;

    STDMETHOD(SetVarName)(THIS_
      UINT index, LPSTR szName) PURE;

    STDMETHOD(SetTypeDescAlias)(THIS_
      TYPEDESC FAR* ptdescAlias) PURE;

    STDMETHOD(DefineFuncAsDllEntry)(THIS_
      UINT index, LPSTR szDllName, LPSTR szProcName) PURE;

    STDMETHOD(SetFuncDocString)(THIS_
      UINT index, LPSTR szDocString) PURE;

    STDMETHOD(SetVarDocString)(THIS_
      UINT index, LPSTR szDocString) PURE;

    STDMETHOD(SetFuncHelpContext)(THIS_
      UINT index, DWORD dwHelpContext) PURE;

    STDMETHOD(SetVarHelpContext)(THIS_
      UINT index, DWORD dwHelpContext) PURE;

    STDMETHOD(SetMops)(THIS_
      UINT index, BSTR bstrMops) PURE;

    STDMETHOD(SetTypeIdldesc)(THIS_
      IDLDESC FAR* pidldesc) PURE;

    STDMETHOD(LayOut)(THIS) PURE;
};

typedef ICreateTypeInfo FAR* LPCREATETYPEINFO;

/*---------------------------------------------------------------------*/
/*                         TypeInfo APIs                               */
/*---------------------------------------------------------------------*/


/* compute a 16bit hash value for the given name
 */
STDAPI_(WORD)
WHashValOfName(LPSTR szName);

/* load the typelib from the file with the given filename
 */
STDAPI
LoadTypeLib(LPSTR szFile, ITypeLib FAR* FAR* pptlib);

/* load registered typelib
 */
STDAPI
LoadRegTypeLib(
    REFGUID rguid,
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    ITypeLib FAR* FAR* pptlib);

/* add typelib to registry
 */
STDAPI
RegisterTypeLib(ITypeLib FAR* ptlib, LPSTR szFullPath, LPSTR szHelpDir);

/* remove typelib from registry
 */
STDAPI
DeregisterTypeLib(REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid);

typedef enum tagSYSKIND {
    SYS_WIN16,
    SYS_WIN32,
    SYS_MAC
} SYSKIND;

STDAPI
CreateTypeLib(SYSKIND syskind, ICreateTypeLib FAR* FAR* ppctlib);


/*---------------------------------------------------------------------*/
/*                          IEnumVARIANT                               */
/*---------------------------------------------------------------------*/

#undef  INTERFACE
#define INTERFACE IEnumVARIANT

DECLARE_INTERFACE_(IEnumVARIANT, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IEnumVARIANT methods */
    STDMETHOD(Next)(
      THIS_ ULONG celt, VARIANT FAR* rgvar, ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumVARIANT FAR* FAR* ppenum) PURE;
};

typedef IEnumVARIANT FAR* LPENUMVARIANT;


/*---------------------------------------------------------------------*/
/*                             IDispatch                               */
/*---------------------------------------------------------------------*/


#undef  INTERFACE
#define INTERFACE IDispatch

DECLARE_INTERFACE_(IDispatch, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      char FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;
};

typedef IDispatch FAR* LPDISPATCH;


/* DISPID reserved for the standard "value" property */
#define DISPID_VALUE	0

/* DISPID reserved to indicate an "unknown" name */
#define DISPID_UNKNOWN	-1

/* The following DISPID is reserved to indicate the param
 * that is the right-hand-side (or "put" value) of a PropertyPut
 */
#define DISPID_PROPERTYPUT -3

/* DISPID reserved for the standard "NewEnum" method */
#define DISPID_NEWENUM	-4

/* DISPID reserved for the standard "Evaluate" method */
#define DISPID_EVALUATE	-5


/*---------------------------------------------------------------------*/
/*                   IDispatch implementation support                  */
/*---------------------------------------------------------------------*/

typedef struct FARSTRUCT tagPARAMDATA {
    char FAR* szName;		/* parameter name */
    VARTYPE vt;			/* parameter type */
} PARAMDATA, FAR* LPPARAMDATA;

typedef struct FARSTRUCT tagMETHODDATA {
    char FAR* szName;		/* method name */
    PARAMDATA FAR* ppdata;	/* pointer to an array of PARAMDATAs */
    DISPID dispid;		/* method ID */
    UINT iMeth;			/* method index */
    CALLCONV cc;		/* calling convention */
    UINT cArgs;			/* count of arguments */
    WORD wFlags;		/* same wFlags as on IDispatch::Invoke() */
    VARTYPE vtReturn;
} METHODDATA, FAR* LPMETHODDATA;

typedef struct FARSTRUCT tagINTERFACEDATA {
    METHODDATA FAR* pmethdata;	/* pointer to an array of METHODDATAs */
    UINT cMembers;		/* count of members */
} INTERFACEDATA, FAR* LPINTERFACEDATA;



/* Locate the parameter indicated by the given position, and
 * return it coerced to the given target VARTYPE (vtTarg).
 */
STDAPI
DispGetParam(
    DISPPARAMS FAR* pdispparams,
    UINT position,
    VARTYPE vtTarg,
    VARIANT FAR* pvarResult,
    UINT FAR* puArgErr);

/* Automatic TypeInfo driven implementation of IDispatch::GetIDsOfNames()
 */ 
STDAPI
DispGetIDsOfNames(
    ITypeInfo FAR* ptinfo,
    char FAR* FAR* rgszNames,
    UINT cNames,
    DISPID FAR* rgdispid);

/* Automatic TypeInfo driven implementation of IDispatch::Invoke()
 */
STDAPI
DispInvoke(
    void FAR* _this,
    ITypeInfo FAR* ptinfo,
    DISPID dispidMember,
    WORD wFlags,
    DISPPARAMS FAR* pparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    UINT FAR* puArgErr);

/* Construct a TypeInfo from an interface data description
 */
STDAPI
CreateDispTypeInfo(
    INTERFACEDATA FAR* pidata,
    LCID lcid,
    ITypeInfo FAR* FAR* pptinfo);

/* Create an instance of the standard TypeInfo driven IDispatch
 * implementation.
 */
STDAPI
CreateStdDispatch(
    IUnknown FAR* punkOuter,
    void FAR* pvThis,
    ITypeInfo FAR* ptinfo,
    IUnknown FAR* FAR* ppunkStdDisp);


/*---------------------------------------------------------------------*/
/*                    Active Object Registration API                   */
/*---------------------------------------------------------------------*/


STDAPI
RegisterActiveObject(
   IUnknown FAR* punk,
   REFCLSID rclsid,
   void FAR* pvReserved,
   DWORD FAR* pdwRegister);

STDAPI
RevokeActiveObject(
    DWORD dwRegister,
    void FAR* pvReserved);

STDAPI
GetActiveObject(
    REFCLSID rclsid,
    void FAR* pvReserved,
    IUnknown FAR* FAR* ppunk);


#endif /* _DISPATCH_H_ */
