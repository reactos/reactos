#ifndef _OLEAUTO_H
#define _OLEAUTO_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#pragma pack(push,8)
#define WINOLEAUTAPI STDAPI
#define WINOLEAUTAPI_(type) STDAPI_(type)
#define STDOLE_MAJORVERNUM 1
#define STDOLE_MINORVERNUM 0
#define STDOLE_LCID 0
#define VARIANT_NOVALUEPROP 0x01
#define VARIANT_ALPHABOOL 0x02
#define VARIANT_NOUSEOVERRIDE 0x04
#define VARIANT_LOCALBOOL 0x08
#define VAR_TIMEVALUEONLY 0x0001
#define VAR_DATEVALUEONLY 0x0002
#define MEMBERID_NIL DISPID_UNKNOWN
#define ID_DEFAULTINST (-2)
#define DISPATCH_METHOD 1
#define DISPATCH_PROPERTYGET 2
#define DISPATCH_PROPERTYPUT 4
#define DISPATCH_PROPERTYPUTREF 8
#define LHashValOfName(l,n) LHashValOfNameSys(SYS_WIN32,l,n)
#define WHashValOfLHashVal(h) ((unsigned short)(0x0000ffff&(h)))
#define IsHashValCompatible(h1,h2) ((BOOL)((0x00ff0000&(h1))==(0x00ff0000&(h2))))
#define ACTIVEOBJECT_STRONG 0
#define ACTIVEOBJECT_WEAK 1
#ifdef NONAMELESSUNION
#define V_UNION(X,Y) ((X)->n1.n2.n3.Y)
#define V_VT(X) ((X)->n1.n2.vt)
#else
#define V_UNION(X,Y) ((X)->Y)
#define V_VT(X) ((X)->vt)
#endif
#define V_BOOL(X) V_UNION(X,boolVal)
#define V_ISBYREF(X) (V_VT(X)&VT_BYREF)
#define V_ISARRAY(X) (V_VT(X)&VT_ARRAY)
#define V_ISVECTOR(X) (V_VT(X)&VT_VECTOR)
#define V_NONE(X) V_I2(X)
#define V_UI1(X) V_UNION(X,bVal)
#define V_UI1REF(X) V_UNION(X,pbVal)
#define V_I2(X) V_UNION(X,iVal)
#define V_I2REF(X) V_UNION(X,piVal)
#define V_I4(X) V_UNION(X,lVal)
#define V_I4REF(X) V_UNION(X,plVal)
#define V_I8(X) V_UNION(X,hVal)
#define V_I8REF(X) V_UNION(X,phVal)
#define V_R4(X) V_UNION(X,fltVal)
#define V_R4REF(X) V_UNION(X,pfltVal)
#define V_R8(X) V_UNION(X,dblVal)
#define V_R8REF(X) V_UNION(X,pdblVal)
#define V_CY(X) V_UNION(X,cyVal)
#define V_CYREF(X) V_UNION(X,pcyVal)
#define V_DATE(X) V_UNION(X,date)
#define V_DATEREF(X) V_UNION(X,pdate)
#define V_BSTR(X) V_UNION(X,bstrVal)
#define V_BSTRREF(X) V_UNION(X,pbstrVal)
#define V_DISPATCH(X) V_UNION(X,pdispVal)
#define V_DISPATCHREF(X) V_UNION(X,ppdispVal)
#define V_ERROR(X) V_UNION(X,scode)
#define V_ERRORREF(X) V_UNION(X,pscode)
#define V_BOOLREF(X) V_UNION(X,pboolVal)
#define V_UNKNOWN(X) V_UNION(X,punkVal)
#define V_UNKNOWNREF(X) V_UNION(X,ppunkVal)
#define V_VARIANTREF(X) V_UNION(X,pvarVal)
#define V_LPSTR(X) V_UNION(X,pszVal)
#define V_LPSTRREF(X) V_UNION(X,ppszVal)
#define V_LPWSTR(X) V_UNION(X,pwszVal)
#define V_LPWSTRREF(X) V_UNION(X,ppwszVal)
#define V_FILETIME(X) V_UNION(X,filetime)
#define V_FILETIMEREF(X) V_UNION(X,pfiletime)
#define V_BLOB(X) V_UNION(X,blob)
#define V_UUID(X) V_UNION(X,puuid)
#define V_CLSID(X) V_UNION(X,puuid)
#define V_ARRAY(X) V_UNION(X,parray)
#define V_ARRAYREF(X) V_UNION(X,pparray)
#define V_BYREF(X) V_UNION(X,byref)
#define V_DECIMAL(X) V_UNION(X,decVal)
#define V_DECIMALREF(X) V_UNION(X,pdecVal)
#define V_I1(X) V_UNION(X,cVal)

#include <oaidl.h>

typedef enum tagREGKIND {
	REGKIND_DEFAULT,
	REGKIND_REGISTER,
	REGKIND_NONE
} REGKIND;
typedef struct tagPARAMDATA {
	OLECHAR *szName;
	VARTYPE vt;
} PARAMDATA,*LPPARAMDATA;
typedef struct tagMETHODDATA {
	OLECHAR *szName;
	PARAMDATA *ppdata;
	DISPID dispid;
	UINT iMeth;
	CALLCONV cc;
	UINT cArgs;
	WORD wFlags;
	VARTYPE vtReturn;
} METHODDATA,*LPMETHODDATA;
typedef struct tagINTERFACEDATA {
	METHODDATA *pmethdata;
	UINT cMembers;
} INTERFACEDATA,*LPINTERFACEDATA;

WINOLEAUTAPI_(BSTR) SysAllocString(const OLECHAR*);
WINOLEAUTAPI_(int) SysReAllocString(BSTR*,const OLECHAR*);
WINOLEAUTAPI_(BSTR) SysAllocStringLen(const OLECHAR*,unsigned int);
WINOLEAUTAPI_(int) SysReAllocStringLen(BSTR*,const OLECHAR*,unsigned int);
WINOLEAUTAPI_(void) SysFreeString(BSTR);
WINOLEAUTAPI_(unsigned int) SysStringLen(BSTR);
WINOLEAUTAPI_(unsigned int) SysStringByteLen(BSTR);
WINOLEAUTAPI_(BSTR) SysAllocStringByteLen(const char*,unsigned int);
WINOLEAUTAPI_(int) DosDateTimeToVariantTime(unsigned short,unsigned short,double*);
WINOLEAUTAPI_(int) VariantTimeToDosDateTime(double,unsigned short*,unsigned short*);
WINOLEAUTAPI_(int) VariantTimeToSystemTime(double,LPSYSTEMTIME);
WINOLEAUTAPI_(int) SystemTimeToVariantTime(LPSYSTEMTIME, double*);
WINOLEAUTAPI SafeArrayAllocDescriptor(unsigned int,SAFEARRAY**);
WINOLEAUTAPI SafeArrayAllocData(SAFEARRAY*);
WINOLEAUTAPI_(SAFEARRAY*) SafeArrayCreate(VARTYPE,unsigned int,SAFEARRAYBOUND*);
WINOLEAUTAPI SafeArrayDestroyDescriptor(SAFEARRAY*);
WINOLEAUTAPI SafeArrayDestroyData(SAFEARRAY*);
WINOLEAUTAPI SafeArrayDestroy(SAFEARRAY*);
WINOLEAUTAPI SafeArrayRedim(SAFEARRAY*,SAFEARRAYBOUND*);
WINOLEAUTAPI_(unsigned int) SafeArrayGetDim(SAFEARRAY*);
WINOLEAUTAPI_(unsigned int) SafeArrayGetElemsize(SAFEARRAY*);
WINOLEAUTAPI SafeArrayGetUBound(SAFEARRAY*,unsigned int,long*);
WINOLEAUTAPI SafeArrayGetLBound(SAFEARRAY*,unsigned int,long*);
WINOLEAUTAPI SafeArrayLock(SAFEARRAY*);
WINOLEAUTAPI SafeArrayUnlock(SAFEARRAY*);
WINOLEAUTAPI SafeArrayAccessData(SAFEARRAY*,void**);
WINOLEAUTAPI SafeArrayUnaccessData(SAFEARRAY*);
WINOLEAUTAPI SafeArrayGetElement(SAFEARRAY*,long*,void*);
WINOLEAUTAPI SafeArrayPutElement(SAFEARRAY*,long*,void*);
WINOLEAUTAPI SafeArrayCopy(SAFEARRAY*,SAFEARRAY**);
WINOLEAUTAPI SafeArrayPtrOfIndex(SAFEARRAY*,long*,void**);
WINOLEAUTAPI_(SAFEARRAY*) SafeArrayCreateVector(VARTYPE,LONG,UINT);
WINOLEAUTAPI_(void) VariantInit(VARIANTARG*);
WINOLEAUTAPI VariantClear(VARIANTARG*);
WINOLEAUTAPI VariantCopy(VARIANTARG*,VARIANTARG*);
WINOLEAUTAPI VariantCopyInd(VARIANT*,VARIANTARG*);
WINOLEAUTAPI VariantChangeType(VARIANTARG*,VARIANTARG*,unsigned short,VARTYPE);
WINOLEAUTAPI VariantChangeTypeEx(VARIANTARG*,VARIANTARG*,LCID,unsigned short,VARTYPE);
WINOLEAUTAPI VarUI1FromI2(short,unsigned char*);
WINOLEAUTAPI VarUI1FromI4(long,unsigned char*);
WINOLEAUTAPI VarUI1FromR4(float,unsigned char*);
WINOLEAUTAPI VarUI1FromR8(double,unsigned char*);
WINOLEAUTAPI VarUI1FromCy(CY,unsigned char*);
WINOLEAUTAPI VarUI1FromDate(DATE,unsigned char*);
WINOLEAUTAPI VarUI1FromStr(OLECHAR*,LCID,unsigned long,unsigned char*);
WINOLEAUTAPI VarUI1FromDisp(LPDISPATCH*,LCID,unsigned char*);
WINOLEAUTAPI VarUI1FromBool(VARIANT_BOOL,unsigned char*);
WINOLEAUTAPI VarI2FromUI1(unsigned char,short*);
WINOLEAUTAPI VarI2FromI4(long,short*);
WINOLEAUTAPI VarI2FromR4(float,short*);
WINOLEAUTAPI VarI2FromR8(double,short*);
WINOLEAUTAPI VarI2FromCy(CY cyIn,short*);
WINOLEAUTAPI VarI2FromDate(DATE,short*);
WINOLEAUTAPI VarI2FromStr(OLECHAR*,LCID,unsigned long,short*);
WINOLEAUTAPI VarI2FromDisp(LPDISPATCH*,LCID,short*);
WINOLEAUTAPI VarI2FromBool(VARIANT_BOOL,short*);
WINOLEAUTAPI VarI4FromUI1(unsigned char,long*);
WINOLEAUTAPI VarI4FromI2(short,long*);
WINOLEAUTAPI VarI4FromR4(float,long*);
WINOLEAUTAPI VarI4FromR8(double,long*);
WINOLEAUTAPI VarI4FromCy(CY,long*);
WINOLEAUTAPI VarI4FromDate(DATE,long*);
WINOLEAUTAPI VarI4FromStr(OLECHAR*,LCID,unsigned long,long*);
WINOLEAUTAPI VarI4FromDisp(LPDISPATCH*,LCID,long*);
WINOLEAUTAPI VarI4FromBool(VARIANT_BOOL,long*);
WINOLEAUTAPI VarR4FromUI1(unsigned char,float*);
WINOLEAUTAPI VarR4FromI2(short,float*);
WINOLEAUTAPI VarR4FromI4(long,float*);
WINOLEAUTAPI VarR4FromR8(double,float*);
WINOLEAUTAPI VarR4FromCy(CY,float*);
WINOLEAUTAPI VarR4FromDate(DATE,float*);
WINOLEAUTAPI VarR4FromStr(OLECHAR*,LCID,unsigned long,float*);
WINOLEAUTAPI VarR4FromDisp(LPDISPATCH*,LCID,float*);
WINOLEAUTAPI VarR4FromBool(VARIANT_BOOL,float*);
WINOLEAUTAPI VarR8FromUI1(unsigned char,double*);
WINOLEAUTAPI VarR8FromI2(short,double*);
WINOLEAUTAPI VarR8FromI4(long,double*);
WINOLEAUTAPI VarR8FromR4(float,double*);
WINOLEAUTAPI VarR8FromCy(CY,double*);
WINOLEAUTAPI VarR8FromDate(DATE,double*);
WINOLEAUTAPI VarR8FromStr(OLECHAR*,LCID,unsigned long,double*);
WINOLEAUTAPI VarR8FromDisp(LPDISPATCH*,LCID,double*);
WINOLEAUTAPI VarR8FromBool(VARIANT_BOOL,double*);
WINOLEAUTAPI VarR8FromDec(DECIMAL*,double*);
WINOLEAUTAPI VarDateFromUI1(unsigned char,DATE*);
WINOLEAUTAPI VarDateFromI2(short,DATE*);
WINOLEAUTAPI VarDateFromI4(long,DATE*);
WINOLEAUTAPI VarDateFromR4(float,DATE*);
WINOLEAUTAPI VarDateFromR8(double,DATE*);
WINOLEAUTAPI VarDateFromCy(CY,DATE*);
WINOLEAUTAPI VarDateFromStr(OLECHAR*,LCID,unsigned long,DATE*);
WINOLEAUTAPI VarDateFromDisp(LPDISPATCH*,LCID,DATE*);
WINOLEAUTAPI VarDateFromBool(VARIANT_BOOL,DATE*);
WINOLEAUTAPI VarCyFromUI1(unsigned char,CY*);
WINOLEAUTAPI VarCyFromI2(short,CY*);
WINOLEAUTAPI VarCyFromI4(long,CY*);
WINOLEAUTAPI VarCyFromR4(float,CY*);
WINOLEAUTAPI VarCyFromR8(double,CY*);
WINOLEAUTAPI VarCyFromDate(DATE,CY*);
WINOLEAUTAPI VarCyFromStr(OLECHAR*,LCID,unsigned long,CY*);
WINOLEAUTAPI VarCyFromDisp(LPDISPATCH*,LCID,CY*);
WINOLEAUTAPI VarCyFromBool(VARIANT_BOOL,CY*);
WINOLEAUTAPI VarBstrFromUI1(unsigned char,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromI2(short,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromI4(long,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromR4(float,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromR8(double,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromCy(CY,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromDate(DATE,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromDisp(LPDISPATCH*,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBstrFromBool(VARIANT_BOOL,LCID,unsigned long,BSTR*);
WINOLEAUTAPI VarBoolFromUI1(unsigned char,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromI2(short,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromI4(long,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromR4(float,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromR8(double,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromDate(DATE,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromCy(CY,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromStr(OLECHAR*,LCID,unsigned long,VARIANT_BOOL*);
WINOLEAUTAPI VarBoolFromDisp(LPDISPATCH*,LCID,VARIANT_BOOL*);
WINOLEAUTAPI VarDecFromR8(double,DECIMAL*);
WINOLEAUTAPI_(ULONG) LHashValOfNameSysA(SYSKIND,LCID,const char*);
WINOLEAUTAPI_(ULONG) LHashValOfNameSys(SYSKIND,LCID,const OLECHAR*);
WINOLEAUTAPI LoadTypeLib(const OLECHAR*,LPTYPELIB*);
WINOLEAUTAPI LoadTypeLibEx(LPCOLESTR,REGKIND,LPTYPELIB*);
WINOLEAUTAPI LoadRegTypeLib(REFGUID,WORD,WORD,LCID,LPTYPELIB*);
WINOLEAUTAPI QueryPathOfRegTypeLib(REFGUID,unsigned short,unsigned short,LCID,LPBSTR);
WINOLEAUTAPI RegisterTypeLib(LPTYPELIB,OLECHAR*,OLECHAR*);
WINOLEAUTAPI UnRegisterTypeLib(REFGUID,WORD,WORD,LCID,SYSKIND);
WINOLEAUTAPI CreateTypeLib(SYSKIND,const OLECHAR*,LPCREATETYPELIB*);
WINOLEAUTAPI DispGetParam(DISPPARAMS*,UINT,VARTYPE,VARIANT*,UINT*);
WINOLEAUTAPI DispGetIDsOfNames(LPTYPEINFO,OLECHAR**,UINT,DISPID*);
WINOLEAUTAPI DispInvoke(void*,LPTYPEINFO,DISPID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
WINOLEAUTAPI CreateDispTypeInfo(INTERFACEDATA*,LCID,LPTYPEINFO*);
WINOLEAUTAPI CreateStdDispatch(IUnknown*,void*,LPTYPEINFO,IUnknown**);
WINOLEAUTAPI RegisterActiveObject(IUnknown*,REFCLSID,DWORD,DWORD*);
WINOLEAUTAPI RevokeActiveObject(DWORD,void*);
WINOLEAUTAPI GetActiveObject(REFCLSID,void*,IUnknown**);
WINOLEAUTAPI SetErrorInfo(unsigned long,LPERRORINFO);
WINOLEAUTAPI GetErrorInfo(unsigned long,LPERRORINFO*);
WINOLEAUTAPI CreateErrorInfo(LPCREATEERRORINFO*);
WINOLEAUTAPI_(unsigned long) OaBuildVersion(void);
WINOLEAUTAPI VectorFromBstr (BSTR, SAFEARRAY **);
WINOLEAUTAPI BstrFromVector (SAFEARRAY *, BSTR *);

WINOLEAUTAPI VarAdd(LPVARIANT, LPVARIANT, LPVARIANT);
WINOLEAUTAPI VarSub(LPVARIANT, LPVARIANT, LPVARIANT);
WINOLEAUTAPI VarMul(LPVARIANT, LPVARIANT, LPVARIANT);
WINOLEAUTAPI VarDiv(LPVARIANT, LPVARIANT, LPVARIANT);
#pragma pack(pop)

#endif
