/*
 * Defines the COM interfaces and APIs related to OLE automation support.
 */

#ifndef __WINE_WINE_OBJ_OLEAUT_H
#define __WINE_WINE_OBJ_OLEAUT_H


DEFINE_OLEGUID(IID_StdOle, 0x00020430,0,0);

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IDispatch,       0x00020400,0,0);
typedef struct IDispatch IDispatch,*LPDISPATCH;

DEFINE_OLEGUID(IID_ITypeInfo,       0x00020401,0,0);
typedef struct ITypeInfo ITypeInfo,*LPTYPEINFO;

DEFINE_OLEGUID(IID_ITypeLib,        0x00020402,0,0);
typedef struct ITypeLib ITypeLib,*LPTYPELIB;

DEFINE_OLEGUID(IID_ITypeComp,       0x00020403,0,0);
typedef struct ITypeComp ITypeComp,*LPTYPECOMP;

DEFINE_OLEGUID(IID_IEnumVARIANT,    0x00020404,0,0);
typedef struct IEnumVARIANT IEnumVARIANT,*LPENUMVARIANT;

DEFINE_OLEGUID(IID_ICreateTypeInfo, 0x00020405,0,0);
typedef struct ICreateTypeInfo ICreateTypeInfo,*LPCREATETYPEINFO;

DEFINE_OLEGUID(IID_ICreateTypeLib,  0x00020406,0,0);
typedef struct ICreateTypeLib ICreateTypeLib,*LPCREATETYPELIB;

DEFINE_OLEGUID(IID_ICreateTypeInfo2,0x0002040E,0,0);
typedef struct ICreateTypeInfo2 ICreateTypeInfo2,*LPCREATETYPEINFO2;

DEFINE_OLEGUID(IID_ICreateTypeLib2, 0x0002040F,0,0);
typedef struct ICreateTypeLib2 ICreateTypeLib2,*LPCREATETYPELIB2;

DEFINE_OLEGUID(IID_ITypeChangeEvents,0x00020410,0,0);
typedef struct ITypeChangeEvents ITypeChangeEvents,*LPTYPECHANGEEVENTS;

DEFINE_OLEGUID(IID_ITypeLib2,       0x00020411,0,0);
typedef struct ITypeLib2 ITypeLib2,*LPTYPELIB2;

DEFINE_OLEGUID(IID_ITypeInfo2,      0x00020412,0,0);
typedef struct ITypeInfo2 ITypeInfo2,*LPTYPEINFO2;

/*****************************************************************************
 * Automation data types
 */

/*****************************************************************
 *  SafeArray defines and structs 
 */

#define FADF_AUTO        ( 0x1 )
#define FADF_STATIC      ( 0x2 )
#define FADF_EMBEDDED    ( 0x4 )
#define FADF_FIXEDSIZE   ( 0x10 )
#define FADF_RECORD      ( 0x20 )
#define FADF_HAVEIID     ( 0x40 )
#define FADF_HAVEVARTYPE ( 0x80 )
#define FADF_BSTR        ( 0x100 )
#define FADF_UNKNOWN     ( 0x200 )
#define FADF_DISPATCH    ( 0x400 )
#define FADF_VARIANT     ( 0x800 )
#define FADF_RESERVED    ( 0xf008 )

/* Undocumented flags */                                                                                  
#define FADF_CREATEVECTOR ( 0x2000 ) /* set when the safe array is created using SafeArrayCreateVector */ 


typedef struct  tagSAFEARRAYBOUND 
{
  ULONG cElements;                  /* Number of elements in dimension */
  LONG  lLbound;                    /* Lower bound of dimension */
} SAFEARRAYBOUND;

typedef struct  tagSAFEARRAY
{ 
  USHORT          cDims;            /* Count of array dimension */
  USHORT          fFeatures;        /* Flags describing the array */
  ULONG           cbElements;       /* Size of each element */
  ULONG           cLocks;           /* Number of lock on array */
  PVOID           pvData;           /* Pointer to data valid when cLocks > 0 */
  SAFEARRAYBOUND  rgsabound[ 1 ];   /* One bound for each dimension */
} SAFEARRAY, *LPSAFEARRAY;

typedef enum tagCALLCONV {
	CC_CDECL    = 1,
	CC_MSCPASCAL  = CC_CDECL + 1,
	CC_PASCAL   = CC_MSCPASCAL,
	CC_MACPASCAL  = CC_PASCAL + 1,
	CC_STDCALL    = CC_MACPASCAL + 1,
	CC_RESERVED   = CC_STDCALL + 1,
	CC_SYSCALL    = CC_RESERVED + 1,
	CC_MPWCDECL   = CC_SYSCALL + 1,
	CC_MPWPASCAL  = CC_MPWCDECL + 1,
	CC_MAX    = CC_MPWPASCAL + 1
} CALLCONV;

typedef CY CURRENCY;

/*
 * Declarations of the VARIANT structure and the VARIANT APIs.
 */
/* S_OK 			   : Success.
 * DISP_E_BADVARTYPE   : The variant type vt in not a valid type of variant.
 * DISP_E_OVERFLOW	   : The data pointed to by pvarSrc does not fit in the destination type.
 * DISP_E_TYPEMISMATCH : The variant type vt is not a valid type of variant.
 * E_INVALIDARG 	   : One argument is invalid.
 * * E_OUTOFMEMORY     : Memory could not be allocated for the conversion.
 * DISP_E_ARRAYISLOCKED : The variant contains an array that is locked.
 */

typedef struct tagVARIANT VARIANT, *LPVARIANT;
typedef struct tagVARIANT VARIANTARG, *LPVARIANTARG;

struct tagVARIANT {
	VARTYPE vt;
	WORD wReserved1;
	WORD wReserved2;
	WORD wReserved3;
	union /* DUMMYUNIONNAME */
	{
	/* By value. */
		CHAR cVal;
		USHORT uiVal;
		ULONG ulVal;
		INT intVal;
		UINT uintVal;
		BYTE bVal;
		short iVal;
		long lVal;
		float fltVal;
		double dblVal;
		VARIANT_BOOL boolVal;
		SCODE scode;
		DATE date;
		BSTR bstrVal;
		CY cyVal;
		/* FIXME: This is not supposed to be at this level
		 * See bug #181 in bugzilla
		 * DECIMAL decVal;
		 */
		IUnknown* punkVal;
		IDispatch* pdispVal;
	        SAFEARRAY* parray;

	 	/* By reference */
		CHAR* pcVal;
		USHORT* puiVal;
		ULONG* pulVal;
		INT* pintVal;
		UINT* puintVal;
		BYTE* pbVal;
		short* piVal;
		long* plVal;
		float* pfltVal;
		double* pdblVal;
		VARIANT_BOOL* pboolVal;
		SCODE* pscode;
		DATE* pdate;
		BSTR* pbstrVal;
		VARIANT* pvarVal;
		PVOID byref;
		CY* pcyVal;
		DECIMAL* pdecVal;
		IUnknown** ppunkVal;
		IDispatch** ppdispVal;
	        SAFEARRAY** pparray;
	} DUMMYUNIONNAME;
};

             
typedef LONG DISPID;
typedef DWORD HREFTYPE;
typedef DISPID MEMBERID;

#define DISPATCH_METHOD         0x1
#define DISPATCH_PROPERTYGET    0x2
#define DISPATCH_PROPERTYPUT    0x4
#define DISPATCH_PROPERTYPUTREF 0x8

#define DISPID_UNKNOWN  ( -1 )
#define DISPID_VALUE  ( 0 )
#define DISPID_PROPERTYPUT  ( -3 )
#define DISPID_NEWENUM  ( -4 )
#define DISPID_EVALUATE ( -5 )
#define DISPID_CONSTRUCTOR  ( -6 )
#define DISPID_DESTRUCTOR ( -7 )
#define DISPID_COLLECT  ( -8 )

#define MEMBERID_NIL DISPID_UNKNOWN

#define IMPLTYPEFLAG_FDEFAULT         (0x1)
#define IMPLTYPEFLAG_FSOURCE          (0x2)
#define IMPLTYPEFLAG_FRESTRICTED      (0x4)
#define IMPLTYPEFLAG_FDEFAULTVTABLE   (0x8)

typedef struct  tagDISPPARAMS
{
  VARIANTARG* rgvarg;
  DISPID*     rgdispidNamedArgs;
  UINT      cArgs;
  UINT      cNamedArgs;
} DISPPARAMS;

typedef struct tagEXCEPINFO 
{
    WORD  wCode;
    WORD  wReserved;
    BSTR  bstrSource;
    BSTR  bstrDescription;
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    PVOID pvReserved;
    HRESULT __stdcall (*pfnDeferredFillIn)(struct tagEXCEPINFO *);
    SCODE scode;
} EXCEPINFO, * LPEXCEPINFO;

typedef struct tagIDLDESC
{
	ULONG dwReserved;
	USHORT wIDLFlags;
} IDLDESC;

typedef struct tagPARAMDESCEX
{
	ULONG cBytes;
	VARIANTARG varDefaultValue;
} PARAMDESCEX, *LPPARAMDESCEX;

typedef struct tagPARAMDESC
{
	LPPARAMDESCEX pparamdescex;
	USHORT wParamFlags;
} PARAMDESC;

#define PARAMFLAG_NONE      	(0x00)
#define PARAMFLAG_FIN           (0x01)
#define PARAMFLAG_FOUT          (0x02)
#define PARAMFLAG_FLCID     	(0x04)
#define PARAMFLAG_FRETVAL       (0x08)
#define PARAMFLAG_FOPT          (0x10)
#define PARAMFLAG_FHASDEFAULT   (0x20)


typedef struct tagTYPEDESC
{
	union {
		struct tagTYPEDESC *lptdesc;
		struct tagARRAYDESC *lpadesc;
		HREFTYPE hreftype;
	} DUMMYUNIONNAME;
	VARTYPE vt;
} TYPEDESC;
 
typedef struct tagELEMDESC
{
	TYPEDESC tdesc;
	union {
		IDLDESC idldesc;
		PARAMDESC paramdesc;
	} DUMMYUNIONNAME;
} ELEMDESC, *LPELEMDESC;

typedef enum tagTYPEKIND
{
	TKIND_ENUM = 0,
	TKIND_RECORD = 1,
	TKIND_MODULE = 2,
	TKIND_INTERFACE = 3,
	TKIND_DISPATCH = 4,
	TKIND_COCLASS = 5,
	TKIND_ALIAS = 6,
	TKIND_UNION = 7,
	TKIND_MAX = 8
} TYPEKIND;

typedef struct tagTYPEATTR
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
} TYPEATTR, *LPTYPEATTR;

#define TYPEFLAG_NONE                     (0x00)
#define TYPEFLAG_FAPPOBJECT               (0x01)
#define TYPEFLAG_FCANCREATE               (0x02)
#define TYPEFLAG_FLICENSED                (0x04)
#define TYPEFLAG_FPREDECLID               (0x08) 
#define TYPEFLAG_FHIDDEN                  (0x0f) 
#define TYPEFLAG_FCONTROL                 (0x20)  
#define TYPEFLAG_FDUAL                    (0x40)  
#define TYPEFLAG_FNONEXTENSIBLE           (0x80)           
#define TYPEFLAG_FOLEAUTOMATION           (0x100)  
#define TYPEFLAG_FRESTRICTED              (0x200)  
#define TYPEFLAG_FAGGREGATABLE            (0x400)  
#define TYPEFLAG_FREPLACEABLE             (0x800)  
#define TYPEFLAG_FDISPATCHABLE            (0x1000)  
#define TYPEFLAG_FREVERSEBIND             (0x2000) 
#define TYPEFLAG_FPROXY                   (0x4000)
#define TYPEFLAG_DEFAULTFILTER            (0x8000) 
#define TYPEFLAG_COCLASSATTRIBUTES        (0x63f)
#define TYPEFLAG_INTERFACEATTRIBUTES      (0x7bd0)
#define TYPEFLAG_DISPATCHATTRIBUTES       (0x5a90)
#define TYPEFLAG_ALIASATTRIBUTES          (0x210)
#define TYPEFLAG_MODULEATTRIBUTES         (0x210) 
#define TYPEFLAG_ENUMATTRIBUTES           (0x210) 
#define TYPEFLAG_RECORDATTRIBUTES         (0x210) 
#define TYPEFLAG_UNIONATTRIBUTES          (0x210) 

typedef struct tagARRAYDESC
{
	TYPEDESC tdescElem;
	USHORT cDims;
	SAFEARRAYBOUND rgbounds[1];
} ARRAYDESC;

typedef enum tagFUNCKIND
{
	FUNC_VIRTUAL = 0,
	FUNC_PUREVIRTUAL = 1,
	FUNC_NONVIRTUAL = 2,
	FUNC_STATIC = 3,
	FUNC_DISPATCH = 4
} FUNCKIND;

typedef enum tagINVOKEKIND
{
	INVOKE_FUNC = 1,
	INVOKE_PROPERTYGET = 2,
	INVOKE_PROPERTYPUT = 3,
	INVOKE_PROPERTYPUTREF = 4
} INVOKEKIND;

typedef struct tagFUNCDESC
{
	MEMBERID memid;
	SCODE *lprgscode;
	ELEMDESC *lprgelemdescParam;
	FUNCKIND funckind;
	INVOKEKIND invkind;
	CALLCONV callconv;
	SHORT cParams;
	SHORT cParamsOpt;
	SHORT oVft;
	SHORT cScodes;
	ELEMDESC elemdescFunc;
	WORD wFuncFlags;
} FUNCDESC, *LPFUNCDESC;

typedef enum tagVARKIND
{
	VAR_PERINSTANCE = 0,
	VAR_STATIC = 1,
	VAR_CONST = 2,
	VAR_DISPATCH = 3
} VARKIND;

typedef struct tagVARDESC
{
	MEMBERID memid;
	LPOLESTR lpstrSchema;
	union {
		ULONG oInst;
		VARIANT *lpvarValue;
	} DUMMYUNIONNAME;
	ELEMDESC elemdescVar;
	WORD wVarFlags;
	VARKIND varkind;
} VARDESC, *LPVARDESC;

typedef enum tagSYSKIND
{
	SYS_WIN16 = 0,
	SYS_WIN32 = 1,
	SYS_MAC = 2
} SYSKIND;

typedef enum tagLIBFLAGS
{
	LIBFLAG_FRESTRICTED = 0x1,
	LIBFLAG_FCONTROL = 0x2,
	LIBFLAG_FHIDDEN = 0x4,
	LIBFLAG_FHASDISKIMAGE = 0x8
} LIBFLAGS;

typedef struct tagTLIBATTR
{
	GUID guid;
	LCID lcid;
	SYSKIND syskind;
	WORD wMajorVerNum;
	WORD wMinorVerNum;
	WORD wLibFlags;
} TLIBATTR, *LPTLIBATTR;

typedef enum tagDESCKIND
{
	DESCKIND_NONE = 0,
	DESCKIND_FUNCDESC = 1,
	DESCKIND_VARDESC = 2,
	DESCKIND_TYPECOMP = 3,
	DESCKIND_IMPLICITAPPOBJ = 4,
	DESCKIND_MAX = 6
} DESCKIND;

typedef union tagBINDPTR
{
	FUNCDESC *lpfuncdesc;
	VARDESC  *lpvardesc;
	ITypeComp *lptcomp;
} BINDPTR, *LPBINDPTR;

typedef enum tagVARFLAGS
{
	VARFLAG_FREADONLY = 0x1,
	VARFLAG_FSOURCE = 0x2,
	VARFLAG_FBINDABLE = 0x4,
	VARFLAG_FREQUESTEDIT  = 0x8,
	VARFLAG_FDISPLAYBIND  = 0x10,
	VARFLAG_FDEFAULTBIND  = 0x20,
	VARFLAG_FHIDDEN = 0x40,
	VARFLAG_FRESTRICTED = 0x80,
	VARFLAG_FDEFAULTCOLLELEM  = 0x100,
	VARFLAG_FUIDEFAULT  = 0x200,
	VARFLAG_FNONBROWSABLE = 0x400,
	VARFLAG_FREPLACEABLE  = 0x800,
	VARFLAG_FIMMEDIATEBIND  = 0x1000
} VARFLAGS;


/*
 * Data types for Variants.
 */

enum VARENUM {
	VT_EMPTY = 0,
	VT_NULL = 1,
	VT_I2 = 2,
	VT_I4 = 3,
	VT_R4 = 4,
	VT_R8 = 5,
	VT_CY = 6,
	VT_DATE = 7,
	VT_BSTR = 8,
	VT_DISPATCH = 9,
	VT_ERROR  = 10,
	VT_BOOL = 11,
	VT_VARIANT	= 12,
	VT_UNKNOWN	= 13,
	VT_DECIMAL	= 14,
	VT_I1 = 16,
	VT_UI1	= 17,
	VT_UI2	= 18,
	VT_UI4	= 19,
	VT_I8 = 20,
	VT_UI8	= 21,
	VT_INT	= 22,
	VT_UINT = 23,
	VT_VOID = 24,
	VT_HRESULT	= 25,
	VT_PTR	= 26,
	VT_SAFEARRAY  = 27,
	VT_CARRAY = 28,
	VT_USERDEFINED	= 29,
	VT_LPSTR  = 30,
	VT_LPWSTR = 31,
	VT_RECORD = 36,
	VT_FILETIME = 64,
	VT_BLOB = 65,
	VT_STREAM = 66,
	VT_STORAGE	= 67,
	VT_STREAMED_OBJECT	= 68,
	VT_STORED_OBJECT  = 69,
	VT_BLOB_OBJECT	= 70,
	VT_CF = 71,
	VT_CLSID  = 72,
	VT_VECTOR = 0x1000,
	VT_ARRAY  = 0x2000,
	VT_BYREF  = 0x4000,
	VT_RESERVED = 0x8000,
	VT_ILLEGAL	= 0xffff,
	VT_ILLEGALMASKED  = 0xfff,
	VT_TYPEMASK = 0xfff
};

/* the largest valide type
 */
#define VT_MAXVALIDTYPE VT_CLSID


/*
 * Declarations of the VARIANT structure and the VARIANT APIs.
 */

/* S_OK 			   : Success.
 * DISP_E_BADVARTYPE   : The variant type vt in not a valid type of variant.
 * DISP_E_OVERFLOW	   : The data pointed to by pvarSrc does not fit in the destination type.
 * DISP_E_TYPEMISMATCH : The variant type vt is not a valid type of variant.
 * E_INVALIDARG 	   : One argument is invalid.
 * E_OUTOFMEMORY	   : Memory could not be allocated for the conversion.
 * DISP_E_ARRAYISLOCKED : The variant contains an array that is locked.
 */


typedef struct  tagCUSTDATAITEM {
    GUID guid;
    VARIANTARG varValue;
} CUSTDATAITEM, *LPCUSTDATAITEM;

typedef struct  tagCUSTDATA {
    INT cCustData;
    LPCUSTDATAITEM prgCustData; /* count cCustdata */
} CUSTDATA, *LPCUSTDATA;



/*****************************************************************************
 * IDispatch interface
 */
#define ICOM_INTERFACE IDispatch
#define IDispatch_METHODS \
  ICOM_METHOD1(HRESULT, GetTypeInfoCount, UINT*, pctinfo) \
  ICOM_METHOD3(HRESULT, GetTypeInfo, UINT, iTInfo, LCID, lcid, ITypeInfo**, ppTInfo) \
  ICOM_METHOD5(HRESULT, GetIDsOfNames, REFIID, riid, LPOLESTR*, rgszNames, UINT, cNames, LCID, lcid, DISPID*, rgDispId) \
  ICOM_METHOD8(HRESULT, Invoke, DISPID, dispIdMember, REFIID, riid, LCID, lcid, WORD, wFlags, DISPPARAMS*, pDispParams, VARIANT*, pVarResult, EXCEPINFO*, pExepInfo, UINT*, puArgErr)
#define IDispatch_IMETHODS \
	IUnknown_IMETHODS \
	IDispatch_METHODS
ICOM_DEFINE(IDispatch,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDispatch_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDispatch_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDispatch_Release(p)            ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IDispatch_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IDispatch_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define IDispatch_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IDispatch_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)


/*****************************************************************************
 * ITypeInfo interface
 */
#define ICOM_INTERFACE ITypeInfo
#define ITypeInfo_METHODS \
	ICOM_METHOD1(HRESULT,GetTypeAttr, TYPEATTR**,ppTypeAttr) \
	ICOM_METHOD1(HRESULT,GetTypeComp, ITypeComp**,ppTComp) \
	ICOM_METHOD2(HRESULT,GetFuncDesc, UINT,index, FUNCDESC**,ppFuncDesc) \
	ICOM_METHOD2(HRESULT,GetVarDesc, UINT,index, VARDESC**,ppVarDesc) \
	ICOM_METHOD4(HRESULT,GetNames, MEMBERID,memid, BSTR*,rgBstrNames, UINT,cMaxNames, UINT*,pcNames) \
	ICOM_METHOD2(HRESULT,GetRefTypeOfImplType, UINT,index, HREFTYPE*, pRefType) \
	ICOM_METHOD2(HRESULT,GetImplTypeFlags, UINT,index, INT*,pImplTypeFlags)\
	ICOM_METHOD3(HRESULT,GetIDsOfNames, LPOLESTR*,rgszNames, UINT,cNames, MEMBERID*,pMemId) \
	ICOM_METHOD7(HRESULT,Invoke, PVOID,pvInstance, MEMBERID,memid, WORD,wFlags, DISPPARAMS*,pDispParams, VARIANT*,pVarResult, EXCEPINFO*,pExcepInfo, UINT*,puArgErr) \
	ICOM_METHOD5(HRESULT,GetDocumentation, MEMBERID,memid, BSTR*,pBstrName, BSTR*,pBstrDocString, DWORD*,pdwHelpContext, BSTR*,pBstrHelpFile) \
	ICOM_METHOD5(HRESULT,GetDllEntry, MEMBERID,memid, INVOKEKIND,invKind, BSTR*,pBstrDllName, BSTR*,pBstrName, WORD*,pwOrdinal) \
	ICOM_METHOD2(HRESULT,GetRefTypeInfo, HREFTYPE,hRefType, ITypeInfo**,ppTInfo) \
	ICOM_METHOD3(HRESULT,AddressOfMember, MEMBERID,memid, INVOKEKIND,invKind, PVOID*,ppv) \
	ICOM_METHOD3(HRESULT,CreateInstance, IUnknown*,pUnkOuter, REFIID,riid, PVOID*,ppvObj) \
	ICOM_METHOD2(HRESULT,GetMops, MEMBERID,memid, BSTR*,pBstrMops) \
	ICOM_METHOD2(HRESULT,GetContainingTypeLib, ITypeLib**,ppTLib, UINT*,pIndex) \
	ICOM_METHOD1(HRESULT,ReleaseTypeAttr, TYPEATTR*,pTypeAttr) \
	ICOM_METHOD1(HRESULT,ReleaseFuncDesc, FUNCDESC*,pFuncDesc) \
	ICOM_METHOD1(HRESULT,ReleaseVarDesc, VARDESC*,pVarDesc)

#define ITypeInfo_IMETHODS \
	IUnknown_IMETHODS \
	ITypeInfo_METHODS
ICOM_DEFINE(ITypeInfo,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ITypeInfo_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define ITypeInfo_AddRef(p)                     ICOM_CALL (AddRef,p)
#define ITypeInfo_Release(p)                    ICOM_CALL (Release,p)
/*** ITypeInfo methods ***/
#define ITypeInfo_GetTypeAttr(p,a)              ICOM_CALL1(GetTypeAttr,p,a)
#define ITypeInfo_GetTypeComp(p,a)              ICOM_CALL1(GetTypeComp,p,a)
#define ITypeInfo_GetFuncDesc(p,a,b)            ICOM_CALL2(GetFuncDesc,p,a,b)
#define ITypeInfo_GetVarDesc(p,a,b)             ICOM_CALL2(GetVarDesc,p,a,b)
#define ITypeInfo_GetNames(p,a,b,c,d)           ICOM_CALL4(GetNames,p,a,b,c,d)
#define ITypeInfo_GetRefTypeOfImplType(p,a,b)   ICOM_CALL2(GetRefTypeOfImplType,p,a,b)
#define ITypeInfo_GetImplTypeFlags(p,a,b)       ICOM_CALL2(GetImplTypeFlags,p,a,b)
#define ITypeInfo_GetIDsOfNames(p,a,b,c)        ICOM_CALL3(GetIDsOfNames,p,a,b,c)
#define ITypeInfo_Invoke(p,a,b,c,d,e,f,g)       ICOM_CALL7(Invoke,p,a,b,c,d,e,f,g)
#define ITypeInfo_GetDocumentation(p,a,b,c,d,e) ICOM_CALL5(GetDocumentation,p,a,b,c,d,e)
#define ITypeInfo_GetDllEntry(p,a,b,c,d,e)      ICOM_CALL5(GetDllEntry,p,a,b,c,d,e)
#define ITypeInfo_GetRefTypeInfo(p,a,b)         ICOM_CALL2(GetRefTypeInfo,p,a,b)
#define ITypeInfo_AddressOfMember(p,a,b,c)      ICOM_CALL3(AddressOfMember,p,a,b,c)
#define ITypeInfo_CreateInstance(p,a,b,c)       ICOM_CALL3(CreateInstance,p,a,b,c)
#define ITypeInfo_GetMops(p,a,b)                ICOM_CALL2(GetMops,p,a,b)
#define ITypeInfo_GetContainingTypeLib(p,a,b)   ICOM_CALL2(GetContainingTypeLib,p,a,b)
#define ITypeInfo_ReleaseTypeAttr(p,a)          ICOM_CALL1(ReleaseTypeAttr,p,a)
#define ITypeInfo_ReleaseFuncDesc(p,a)          ICOM_CALL1(ReleaseFuncDesc,p,a)
#define ITypeInfo_ReleaseVarDesc(p,a)           ICOM_CALL1(ReleaseVarDesc,p,a)
				  

/*****************************************************************************
 * ITypeInfo2 interface
 */
#define ICOM_INTERFACE ITypeInfo2
#define ITypeInfo2_METHODS \
	ICOM_METHOD1(HRESULT, GetTypeKind, TYPEKIND*, pTypeKind) \
	ICOM_METHOD1(HRESULT, GetTypeFlags, UINT*, pTypeFlags) \
	ICOM_METHOD3(HRESULT, GetFuncIndexOfMemId, MEMBERID, memid, INVOKEKIND,\
		invKind, UINT*, pFuncIndex) \
	ICOM_METHOD2(HRESULT, GetVarIndexOfMemId, MEMBERID, memid, UINT*, \
		pVarIndex) \
	ICOM_METHOD2(HRESULT, GetCustData, REFGUID, guid, VARIANT*, pVarVal) \
	ICOM_METHOD3(HRESULT, GetFuncCustData, UINT, index, REFGUID, guid,\
		VARIANT*, pVarVal) \
	ICOM_METHOD4(HRESULT, GetParamCustData, UINT, indexFunc, UINT,\
		indexParam, REFGUID, guid, VARIANT*, pVarVal) \
	ICOM_METHOD3(HRESULT, GetVarCustData, UINT, index, REFGUID, guid,\
		VARIANT*, pVarVal) \
	ICOM_METHOD3(HRESULT, GetImplTypeCustData, UINT, index, REFGUID, guid,\
		VARIANT*, pVarVal) \
	ICOM_METHOD5(HRESULT, GetDocumentation2, MEMBERID, memid, LCID, lcid,\
		BSTR*, pbstrHelpString, DWORD*, pdwHelpStringContext,\
		BSTR*, pbstrHelpStringDll) \
	ICOM_METHOD1(HRESULT, GetAllCustData, CUSTDATA*, pCustData) \
	ICOM_METHOD2(HRESULT, GetAllFuncCustData, UINT, index, CUSTDATA*,\
		pCustData)\
	ICOM_METHOD3(HRESULT, GetAllParamCustData, UINT, indexFunc, UINT,\
		indexParam, CUSTDATA*, pCustData) \
	ICOM_METHOD2(HRESULT, GetAllVarCustData, UINT, index, CUSTDATA*,\
		pCustData) \
	ICOM_METHOD2(HRESULT, GetAllImplTypeCustData, UINT, index, CUSTDATA*,\
		pCustData)

#define ITypeInfo2_IMETHODS \
	IUnknown_IMETHODS \
	ITypeInfo_METHODS \
	ITypeInfo2_METHODS 
ICOM_DEFINE(ITypeInfo2,ITypeInfo)
#undef ICOM_INTERFACE
	
/*** IUnknown methods ***/
#define ITypeInfo2_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define ITypeInfo2_AddRef(p)                     ICOM_CALL (AddRef,p)
#define ITypeInfo2_Release(p)                    ICOM_CALL (Release,p)
/*** ITypeInfo methods ***/
#define ITypeInfo2_GetTypeAttr(p,a)              ICOM_CALL1(GetTypeAttr,p,a)
#define ITypeInfo2_GetTypeComp(p,a)              ICOM_CALL1(GetTypeComp,p,a)
#define ITypeInfo2_GetFuncDesc(p,a,b)            ICOM_CALL2(GetFuncDesc,p,a,b)
#define ITypeInfo2_GetVarDesc(p,a,b)             ICOM_CALL2(GetVarDesc,p,a,b)
#define ITypeInfo2_GetNames(p,a,b,c,d)           ICOM_CALL4(GetNames,p,a,b,c,d)
#define ITypeInfo2_GetRefTypeOfImplType(p,a,b)   ICOM_CALL2(GetRefTypeOfImplType,p,a,b)
#define ITypeInfo2_GetImplTypeFlags(p,a,b)       ICOM_CALL2(GetImplTypeFlags,p,a,b)
#define ITypeInfo2_GetIDsOfNames(p,a,b,c)        ICOM_CALL3(GetIDsOfNames,p,a,b,c)
#define ITypeInfo2_Invoke(p,a,b,c,d,e,f,g)       ICOM_CALL7(Invoke,p,a,b,c,d,e,f,g)
#define ITypeInfo2_GetDocumentation(p,a,b,c,d,e) ICOM_CALL5(GetDocumentation,p,a,b,c,d,e)
#define ITypeInfo2_GetDllEntry(p,a,b,c,d,e)      ICOM_CALL5(GetDllEntry,p,a,b,c,d,e)
#define ITypeInfo2_GetRefTypeInfo(p,a,b)         ICOM_CALL2(GetRefTypeInfo,p,a,b)
#define ITypeInfo2_AddressOfMember(p,a,b,c)      ICOM_CALL3(AddressOfMember,p,a,b,c)
#define ITypeInfo2_CreateInstance(p,a,b,c)       ICOM_CALL3(CreateInstance,p,a,b,c)
#define ITypeInfo2_GetMops(p,a,b)                ICOM_CALL2(GetMops,p,a,b)
#define ITypeInfo2_GetContainingTypeLib(p,a,b)   ICOM_CALL2(GetContainingTypeLib,p,a,b)
#define ITypeInfo2_ReleaseTypeAttr(p,a)          ICOM_CALL1(ReleaseTypeAttr,p,a)
#define ITypeInfo2_ReleaseFuncDesc(p,a)          ICOM_CALL1(ReleaseFuncDesc,p,a)
#define ITypeInfo2_ReleaseVarDesc(p,a)           ICOM_CALL1(ReleaseVarDesc,p,a)
/*** ITypeInfo2 methods ***/
#define ITypeInfo2_GetTypeKind(p,a)              ICOM_CALL1(GetTypeKind,p,a)
#define ITypeInfo2_GetTypeFlags(p,a)             ICOM_CALL1(GetTypeFlags,p,a)
#define ITypeInfo2_GetFuncIndexOfMemId(p,a,b,c)  ICOM_CALL3(GetFuncIndexOfMemId,p,a,b,c)
#define ITypeInfo2_GetVarIndexOfMemId(p,a,b)     ICOM_CALL2(GetVarIndexOfMemId,p,a,b)
#define ITypeInfo2_GetCustData(p,a,b)            ICOM_CALL2(GetCustData,p,a,b)
#define ITypeInfo2_GetFuncCustData(p,a,b,c)      ICOM_CALL3(GetFuncCustData,p,a,b,c)
#define ITypeInfo2_GetParamCustData(p,a,b,c,d)   ICOM_CALL4(GetParamCustData,p,a,b,c,d)
#define ITypeInfo2_GetVarCustData(p,a,b,c)       ICOM_CALL3(GetVarCustData,p,a,b,c)
#define ITypeInfo2_GetImplTypeCustData(p,a,b,c)  ICOM_CALL3(GetImplTypeCustData,p,a,b,c)
#define ITypeInfo2_GetDocumentation2(p,a,b,c,d,e) ICOM_CALL5(GetDocumentation2,p,a,b,c,d,e)
#define ITypeInfo2_GetAllCustData(p,a)           ICOM_CALL1(GetAllCustData,p,a)
#define ITypeInfo2_GetAllFuncCustData(p,a,b)     ICOM_CALL2(GetAllFuncCustData,p,a,b)
#define ITypeInfo2_GetAllParamCustData(p,a,b,c)  ICOM_CALL3(GetAllParamCustData,p,a,b,c)
#define ITypeInfo2_GetAllVarCustData(p,a,b)      ICOM_CALL2(GetAllVarCustData,p,a,b)
#define ITypeInfo2_GetAllImplTypeCustData(p,a,b) ICOM_CALL2(GetAllImplTypeCustData,p,a,b)

/*****************************************************************************
 * ITypeLib interface
 */
#define ICOM_INTERFACE ITypeLib
#define ITypeLib_METHODS \
	ICOM_METHOD (UINT,GetTypeInfoCount) \
	ICOM_METHOD2(HRESULT,GetTypeInfo, UINT,index, ITypeInfo**,ppTInfo) \
	ICOM_METHOD2(HRESULT,GetTypeInfoType, UINT,index, TYPEKIND*,pTKind) \
	ICOM_METHOD2(HRESULT,GetTypeInfoOfGuid, REFGUID,guid, ITypeInfo**,ppTinfo) \
	ICOM_METHOD1(HRESULT,GetLibAttr, TLIBATTR**,ppTLibAttr) \
	ICOM_METHOD1(HRESULT,GetTypeComp, ITypeComp**,ppTComp) \
	ICOM_METHOD5(HRESULT,GetDocumentation, INT,index, BSTR*,pBstrName, BSTR*,pBstrDocString, DWORD*,pdwHelpContext, BSTR*,pBstrHelpFile) \
	ICOM_METHOD3(HRESULT,IsName, LPOLESTR,szNameBuf, ULONG,lHashVal, BOOL*,bfName) \
	ICOM_METHOD5(HRESULT,FindName, LPOLESTR,szNameBuf, ULONG,lHashVal, ITypeInfo**,ppTInfo, MEMBERID*,rgMemId, USHORT*,pcFound) \
	ICOM_METHOD1(VOID,ReleaseTLibAttr, TLIBATTR*,pTLibAttr)
#define ITypeLib_IMETHODS \
	IUnknown_IMETHODS \
	ITypeLib_METHODS
ICOM_DEFINE(ITypeLib,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ITypeLib_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define ITypeLib_AddRef(p)                     ICOM_CALL (AddRef,p)
#define ITypeLib_Release(p)                    ICOM_CALL (Release,p)
/*** ITypeLib methods ***/
#define ITypeLib_GetTypeInfoCount(p)           ICOM_CALL (GetTypeInfoCount,p)
#define ITypeLib_GetTypeInfo(p,a,b)            ICOM_CALL2(GetTypeInfo,p,a,b)
#define ITypeLib_GetTypeInfoType(p,a,b)        ICOM_CALL2(GetTypeInfoType,p,a,b)
#define ITypeLib_GetTypeInfoOfGuid(p,a,b)      ICOM_CALL2(GetTypeInfoOfGuid,p,a,b)
#define ITypeLib_GetLibAttr(p,a)               ICOM_CALL1(GetLibAttr,p,a)
#define ITypeLib_GetTypeComp(p,a)              ICOM_CALL1(GetTypeComp,p,a)
#define ITypeLib_GetDocumentation(p,a,b,c,d,e) ICOM_CALL5(GetDocumentation,p,a,b,c,d,e)
#define ITypeLib_IsName(p,a,b,c)               ICOM_CALL3(IsName,p,a,b,c)
#define ITypeLib_FindName(p,a,b,c,d,e)         ICOM_CALL5(FindName,p,a,b,c,d,e)
#define ITypeLib_ReleaseTLibAttr(p,a)          ICOM_CALL1(ReleaseTLibAttr,p,a)


/*****************************************************************************
 * ITypeLib2 interface
 */
#define ICOM_INTERFACE ITypeLib2
#define ITypeLib2_METHODS \
	ICOM_METHOD2(HRESULT, GetCustData, REFGUID, guid, VARIANT*, pVarVal) \
	ICOM_METHOD2(HRESULT, GetLibStatistics, ULONG *, pcUniqueNames, ULONG*,pcchUniqueNames) \
	ICOM_METHOD5(HRESULT, GetDocumentation2, INT, index, LCID, lcid,BSTR*, pbstrHelpString, DWORD*, pdwHelpStringContext, BSTR*, strHelpStringDll) \
    	ICOM_METHOD1(HRESULT, GetAllCustData, CUSTDATA *, pCustData)
#define ITypeLib2_IMETHODS \
	IUnknown_IMETHODS \
	ITypeLib_IMETHODS \
	ITypeLib2_METHODS
ICOM_DEFINE(ITypeLib2,ITypeLib)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ITypeLib2_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define ITypeLib2_AddRef(p)                     ICOM_CALL (AddRef,p)
#define ITypeLib2_Release(p)                    ICOM_CALL (Release,p)
/*** ITypeLib methods ***/
#define ITypeLib2_GetTypeInfoCount(p)           ICOM_CALL (GetTypeInfoCount,p)
#define ITypeLib2_GetTypeInfo(p,a,b)            ICOM_CALL2(GetTypeInfo,p,a,b)
#define ITypeLib2_GetTypeInfoType(p,a,b)        ICOM_CALL2(GetTypeInfoType,p,a,b)
#define ITypeLib2_GetTypeInfoOfGuid(p,a,b)      ICOM_CALL2(GetTypeInfoOfGuid,p,a,b)
#define ITypeLib2_GetLibAttr(p,a)               ICOM_CALL1(GetLibAttr,p,a)
#define ITypeLib2_GetTypeComp(p,a)              ICOM_CALL1(GetTypeComp,p,a)
#define ITypeLib2_GetDocumentation(p,a,b,c,d,e) ICOM_CALL5(GetDocumentation,p,a,b,c,d,e)
#define ITypeLib2_IsName(p,a,b,c)               ICOM_CALL3(IsName,p,a,b,c)
#define ITypeLib2_FindName(p,a,b,c,d,e)         ICOM_CALL5(FindName,p,a,b,c,d,e)
#define ITypeLib2_ReleaseTLibAttr(p,a)          ICOM_CALL1(ReleaseTLibAttr,p,a)
/*** ITypeLib2 methods ***/
#define ITypeLib2_GetCustData(p,a,b)            ICOM_CALL2(GetCustData,p,a,b)
#define ITypeLib2_GetLibStatistics(p,a,b)       ICOM_CALL2(GetLibStatistics,p,a,b)
#define ITypeLib2_GetDocumentation2(p,a,b,c,d,e,f) ICOM_CALL5(GetDocumentation2,p,a,b,c,d,e)
#define ITypeLib2_GetAllCustData(p,a)           ICOM_CALL1(GetAllCustData,p,a)

/*****************************************************************************
 * ITypeComp interface
 */
#define ICOM_INTERFACE ITypeComp
#define ITypeComp_METHODS \
	ICOM_METHOD6(HRESULT,Bind, LPOLESTR,szName, ULONG,lHashVal, WORD,wFlags, ITypeInfo**,ppTInfo, DESCKIND*,pDescKind, BINDPTR*,pBindPtr) \
	ICOM_METHOD4(HRESULT,BindType, LPOLESTR,szName, ULONG,lHashVal, ITypeInfo**,ppTInfo, ITypeComp**,ppTComp) 
#define ITypeComp_IMETHODS \
	IUnknown_IMETHODS \
	ITypeComp_METHODS
ICOM_DEFINE(ITypeComp,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ITypeComp_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define ITypeComp_AddRef(p)                     ICOM_CALL (AddRef,p)
#define ITypeComp_Release(p)                    ICOM_CALL (Release,p)
/*** ITypeComp methods ***/
#define ITypeComp_Bind(p,a,b,c,d,e,f)           ICOM_CALL6(Bind,p,a,b,c,d,e,f)
#define ITypeComp_BindType(p,a,b,c,d)           ICOM_CALL4(BindType,p,a,b,c,d)
				 
/*****************************************************************************
 * IEnumVARIANT interface
 */
#define ICOM_INTERFACE IEnumVARIANT
#define IEnumVARIANT_METHODS \
	ICOM_METHOD3(HRESULT,Next, ULONG,celt, VARIANT*,rgVar, ULONG*,pCeltFetched) \
	ICOM_METHOD1(HRESULT,Skip, ULONG,celt) \
	ICOM_METHOD (HRESULT,Reset) \
	ICOM_METHOD1(HRESULT,Clone, IEnumVARIANT**,ppEnum) 
#define IEnumVARIANT_IMETHODS \
	IUnknown_IMETHODS \
	IEnumVARIANT_METHODS
ICOM_DEFINE(IEnumVARIANT,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumVARIANT_QueryInterface(p,a,b)   ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumVARIANT_AddRef(p)               ICOM_CALL (AddRef,p)
#define IEnumVARIANT_Release(p)              ICOM_CALL (Release,p)
/*** IEnumVARIANT methods ***/
#define IEnumVARIANT_Next(p,a,b,c)           ICOM_CALL3(Next,p,a,b,c)
#define IEnumVARIANT_Skip(p,a)               ICOM_CALL1(Skip,p,a)
#define IEnumVARIANT_Reset(p)                ICOM_CALL (Reset,p)
#define IEnumVARIANT_Clone(p,a)              ICOM_CALL1(Clone,p,a)
				 
#endif /* __WINE_WINE_OBJ_OLEAUT_H */

