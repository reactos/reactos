/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_OLEAUTO_H
#define __WINE_OLEAUTO_H

#include <oaidl.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_OLEGUID(IID_StdOle, 0x00020430,0,0);

#define STDOLE_MAJORVERNUM  1
#define STDOLE_MINORVERNUM  0
#define STDOLE_LCID         0

#define STDOLE2_MAJORVERNUM 2
#define STDOLE2_MINORVERNUM 0
#define STDOLE2_LCID        0

ULONG WINAPI OaBuildVersion(void);

/* BSTR functions */
BSTR WINAPI SysAllocString(const OLECHAR*);
BSTR WINAPI SysAllocStringByteLen(LPCSTR,UINT);
BSTR WINAPI SysAllocStringLen(const OLECHAR*,UINT);
void WINAPI SysFreeString(BSTR);
INT  WINAPI SysReAllocString(LPBSTR,const OLECHAR*);
int  WINAPI SysReAllocStringLen(BSTR*,const OLECHAR*,UINT);
UINT WINAPI SysStringByteLen(BSTR);
UINT WINAPI SysStringLen(BSTR);

/* IErrorInfo helpers */
HRESULT WINAPI SetErrorInfo(ULONG,IErrorInfo*);
HRESULT WINAPI GetErrorInfo(ULONG,IErrorInfo**);
HRESULT WINAPI CreateErrorInfo(ICreateErrorInfo**);

/* SafeArray functions */
SAFEARRAY* WINAPI SafeArrayCreate(VARTYPE,UINT,SAFEARRAYBOUND*);
SAFEARRAY* WINAPI SafeArrayCreateEx(VARTYPE,UINT,SAFEARRAYBOUND*,LPVOID);
SAFEARRAY* WINAPI SafeArrayCreateVector(VARTYPE,LONG,ULONG);
SAFEARRAY* WINAPI SafeArrayCreateVectorEx(VARTYPE,LONG,ULONG,LPVOID);

HRESULT WINAPI SafeArrayAllocDescriptor(UINT,SAFEARRAY**);
HRESULT WINAPI SafeArrayAllocDescriptorEx(VARTYPE,UINT,SAFEARRAY**);
HRESULT WINAPI SafeArrayAllocData(SAFEARRAY*);
HRESULT WINAPI SafeArrayDestroyDescriptor(SAFEARRAY*);
HRESULT WINAPI SafeArrayPutElement(SAFEARRAY*,LONG*,void*);
HRESULT WINAPI SafeArrayGetElement(SAFEARRAY*,LONG*,void*);
HRESULT WINAPI SafeArrayLock(SAFEARRAY*);
HRESULT WINAPI SafeArrayUnlock(SAFEARRAY*);
HRESULT WINAPI SafeArrayGetUBound(SAFEARRAY*,UINT,LONG*);
HRESULT WINAPI SafeArrayGetLBound(SAFEARRAY*,UINT,LONG*);
UINT    WINAPI SafeArrayGetDim(SAFEARRAY*);
UINT    WINAPI SafeArrayGetElemsize(SAFEARRAY*);
HRESULT WINAPI SafeArrayGetVartype(SAFEARRAY*,VARTYPE*);
HRESULT WINAPI SafeArrayAccessData(SAFEARRAY*,void**);
HRESULT WINAPI SafeArrayUnaccessData(SAFEARRAY*);
HRESULT WINAPI SafeArrayPtrOfIndex(SAFEARRAY*,LONG*,void **);
HRESULT WINAPI SafeArrayCopyData(SAFEARRAY*,SAFEARRAY*);
HRESULT WINAPI SafeArrayDestroyData(SAFEARRAY*);
HRESULT WINAPI SafeArrayDestroy(SAFEARRAY*);
HRESULT WINAPI SafeArrayCopy(SAFEARRAY*,SAFEARRAY**);
HRESULT WINAPI SafeArrayRedim(SAFEARRAY*,SAFEARRAYBOUND*);
HRESULT WINAPI SafeArraySetRecordInfo(SAFEARRAY*,IRecordInfo*);
HRESULT WINAPI SafeArrayGetRecordInfo(SAFEARRAY*,IRecordInfo**);
HRESULT WINAPI SafeArraySetIID(SAFEARRAY*,REFGUID);
HRESULT WINAPI SafeArrayGetIID(SAFEARRAY*,GUID*);

HRESULT WINAPI VectorFromBstr(BSTR,SAFEARRAY**);
HRESULT WINAPI BstrFromVector(SAFEARRAY*,BSTR*);

/* Object registration helpers */
#define ACTIVEOBJECT_STRONG 0
#define ACTIVEOBJECT_WEAK   1

HRESULT WINAPI RegisterActiveObject(LPUNKNOWN,REFCLSID,DWORD,LPDWORD);
HRESULT WINAPI RevokeActiveObject(DWORD,LPVOID);
HRESULT WINAPI GetActiveObject(REFCLSID,LPVOID,LPUNKNOWN*);

/* IRecordInfo helpers */
HRESULT WINAPI GetRecordInfoFromTypeInfo(ITypeInfo*,IRecordInfo**);
HRESULT WINAPI GetRecordInfoFromGuids(REFGUID,ULONG,ULONG,LCID,REFGUID,IRecordInfo**);

/*
 * Variants
 */

/* Macros for accessing the fields of the VARIANT type */
#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
#define V_UNION(A,B) ((A)->n1.n2.n3.B)
#define V_VT(A)      ((A)->n1.n2.vt)
#else
#define V_UNION(A,B) ((A)->B)
#define V_VT(A)      ((A)->vt)
#endif

#define V_ISBYREF(A)  (V_VT(A) & VT_BYREF)
#define V_ISARRAY(A)  (V_VT(A) & VT_ARRAY)
#define V_ISVECTOR(A) (V_VT(A) & VT_VECTOR)
#define V_NONE(A)     V_I2(A)

#define V_ARRAY(A)       V_UNION(A,parray)
#define V_ARRAYREF(A)    V_UNION(A,pparray)
#define V_BOOL(A)        V_UNION(A,boolVal)
#define V_BOOLREF(A)     V_UNION(A,pboolVal)
#define V_BSTR(A)        V_UNION(A,bstrVal)
#define V_BSTRREF(A)     V_UNION(A,pbstrVal)
#define V_BYREF(A)       V_UNION(A,byref)
#define V_CY(A)          V_UNION(A,cyVal)
#define V_CYREF(A)       V_UNION(A,pcyVal)
#define V_DATE(A)        V_UNION(A,date)
#define V_DATEREF(A)     V_UNION(A,pdate)
#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
#define V_DECIMAL(A)     ((A)->n1.decVal)
#else
#define V_DECIMAL(A)     ((A)->decVal)
#endif
#define V_DECIMALREF(A)  V_UNION(A,pdecVal)
#define V_DISPATCH(A)    V_UNION(A,pdispVal)
#define V_DISPATCHREF(A) V_UNION(A,ppdispVal)
#define V_ERROR(A)       V_UNION(A,scode)
#define V_ERRORREF(A)    V_UNION(A,pscode)
#define V_I1(A)          V_UNION(A,cVal)
#define V_I1REF(A)       V_UNION(A,pcVal)
#define V_I2(A)          V_UNION(A,iVal)
#define V_I2REF(A)       V_UNION(A,piVal)
#define V_I4(A)          V_UNION(A,lVal)
#define V_I4REF(A)       V_UNION(A,plVal)
#define V_I8(A)          V_UNION(A,llVal)
#define V_I8REF(A)       V_UNION(A,pllVal)
#define V_INT(A)         V_UNION(A,intVal)
#define V_INTREF(A)      V_UNION(A,pintVal)
#ifdef _WIN64
#define V_INT_PTR(A)     V_I8(A)
#define V_INT_PTRREF(A)  V_I8REF(A)
#else
#define V_INT_PTR(A)     V_I4(A)
#define V_INT_PTRREF(A)  V_I4REF(A)
#endif
#define V_R4(A)          V_UNION(A,fltVal)
#define V_R4REF(A)       V_UNION(A,pfltVal)
#define V_R8(A)          V_UNION(A,dblVal)
#define V_R8REF(A)       V_UNION(A,pdblVal)
#define V_UINT(A)        V_UNION(A,uintVal)
#define V_UINTREF(A)     V_UNION(A,puintVal)
#define V_UI1(A)         V_UNION(A,bVal)
#define V_UI1REF(A)      V_UNION(A,pbVal)
#define V_UI2(A)         V_UNION(A,uiVal)
#define V_UI2REF(A)      V_UNION(A,puiVal)
#define V_UI4(A)         V_UNION(A,ulVal)
#define V_UI4REF(A)      V_UNION(A,pulVal)
#define V_UI8(A)         V_UNION(A,ullVal)
#define V_UI8REF(A)      V_UNION(A,pullVal)
#ifdef _WIN64
#define V_UINT_PTR(A)    V_UI8(A)
#define V_UINT_PTRREF(A) V_UI8REF(A)
#else
#define V_UINT_PTR(A)    V_UI4(A)
#define V_UINT_PTRREF(A) V_UI4REF(A)
#endif
#define V_UNKNOWN(A)     V_UNION(A,punkVal)
#define V_UNKNOWNREF(A)  V_UNION(A,ppunkVal)
#define V_VARIANTREF(A)  V_UNION(A,pvarVal)

void    WINAPI VariantInit(VARIANT*);
HRESULT WINAPI VariantClear(VARIANT*);
HRESULT WINAPI VariantCopy(VARIANT*,VARIANT*);
HRESULT WINAPI VariantCopyInd(VARIANT*,VARIANT*);
HRESULT WINAPI VariantChangeType(VARIANT*,VARIANT*,USHORT,VARTYPE);
HRESULT WINAPI VariantChangeTypeEx(VARIANT*,VARIANT*,LCID,USHORT,VARTYPE);

/* VariantChangeType/VariantChangeTypeEx flags */
#define VARIANT_NOVALUEPROP        0x01 /* Don't get the default value property from IDispatch */
#define VARIANT_ALPHABOOL          0x02 /* Coerce to "True"|"False" instead of "-1"|"0" */
#define VARIANT_NOUSEROVERRIDE     0x04 /* Pass LOCALE_NOUSEROVERRIDE to low level conversions */
#define VARIANT_CALENDAR_HIJRI     0x08 /* Use the Hijri calendar */
#define VARIANT_LOCALBOOL          0x10 /* Like VARIANT_ALPHABOOL, but use localised text */
#define VARIANT_CALENDAR_THAI      0x20 /* Use the Thai buddhist calendar */
#define VARIANT_CALENDAR_GREGORIAN 0x40 /* Use the Gregorian calendar */
#define VARIANT_USE_NLS            0x80 /* Format result using NLS calls */

/*
 * Low level Variant coercion functions
 */

#define VT_HARDTYPE VT_RESERVED /* Don't coerce this variant when comparing it to others */

/* Flags for low level coercions. LOCALE_ flags can also be passed */
#define VAR_TIMEVALUEONLY       0x001 /* Ignore date portion of VT_DATE */
#define VAR_DATEVALUEONLY       0x002 /* Ignore time portion of VT_DATE */
#define VAR_VALIDDATE           0x004
#define VAR_CALENDAR_HIJRI      0x008 /* Use the Hijri calendar */
#define VAR_LOCALBOOL           0x010 /* VT_BOOL<->VT_BSTR: Use localised boolean text */
#define VAR_FORMAT_NOSUBSTITUTE 0x020 /* Don't change format strings for un-coercable types */
#define VAR_FOURDIGITYEARS      0x040 /* Always print years with 4 digits */
#define VAR_CALENDAR_THAI       0x080 /* Use the Thai buddhist calendar */
#define VAR_CALENDAR_GREGORIAN  0x100 /* Use the Gregorian calendar */

#ifndef LOCALE_USE_NLS
/* This is missing from native winnls.h, but may be added at some point */
#define LOCALE_USE_NLS          0x10000000
#endif

#define VTDATEGRE_MIN -657434 /* Minimum possible Gregorian date: 1/1/100 */
#define VTDATEGRE_MAX 2958465 /* Maximum possible Gregorian date: 31/12/9999 */

HRESULT WINAPI VarUI1FromI2(SHORT,BYTE*);
HRESULT WINAPI VarUI1FromI4(LONG,BYTE*);
HRESULT WINAPI VarUI1FromI8(LONG64,BYTE*);
HRESULT WINAPI VarUI1FromR4(FLOAT,BYTE*);
HRESULT WINAPI VarUI1FromR8(DOUBLE,BYTE*);
HRESULT WINAPI VarUI1FromDate(DATE,BYTE*);
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL,BYTE*);
HRESULT WINAPI VarUI1FromI1(signed char,BYTE*);
HRESULT WINAPI VarUI1FromUI2(USHORT,BYTE*);
HRESULT WINAPI VarUI1FromUI4(ULONG,BYTE*);
HRESULT WINAPI VarUI1FromUI8(ULONG64,BYTE*);
HRESULT WINAPI VarUI1FromStr(OLECHAR*,LCID,ULONG,BYTE*);
HRESULT WINAPI VarUI1FromCy(CY,BYTE*);
HRESULT WINAPI VarUI1FromDec(DECIMAL*,BYTE*);
HRESULT WINAPI VarUI1FromDisp(IDispatch*,LCID,BYTE*);

HRESULT WINAPI VarI2FromUI1(BYTE,SHORT*);
HRESULT WINAPI VarI2FromI4(LONG,SHORT*);
HRESULT WINAPI VarI2FromI8(LONG64,SHORT*);
HRESULT WINAPI VarI2FromR4(FLOAT,SHORT*);
HRESULT WINAPI VarI2FromR8(DOUBLE,SHORT*);
HRESULT WINAPI VarI2FromDate(DATE,SHORT*);
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL,SHORT*);
HRESULT WINAPI VarI2FromI1(signed char,SHORT*);
HRESULT WINAPI VarI2FromUI2(USHORT,SHORT*);
HRESULT WINAPI VarI2FromUI4(ULONG,SHORT*);
HRESULT WINAPI VarI2FromUI8(ULONG64,SHORT*);
HRESULT WINAPI VarI2FromStr(OLECHAR*,LCID,ULONG,SHORT*);
HRESULT WINAPI VarI2FromCy(CY,SHORT*);
HRESULT WINAPI VarI2FromDec(DECIMAL*,SHORT*);
HRESULT WINAPI VarI2FromDisp(IDispatch*,LCID,SHORT*);

HRESULT WINAPI VarI4FromUI1(BYTE,LONG*);
HRESULT WINAPI VarI4FromI2(SHORT,LONG*);
HRESULT WINAPI VarI4FromI8(LONG64,LONG*);
HRESULT WINAPI VarI4FromR4(FLOAT,LONG*);
HRESULT WINAPI VarI4FromR8(DOUBLE,LONG*);
HRESULT WINAPI VarI4FromDate(DATE,LONG*);
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL,LONG*);
HRESULT WINAPI VarI4FromI1(signed char,LONG*);
HRESULT WINAPI VarI4FromUI2(USHORT,LONG*);
HRESULT WINAPI VarI4FromUI4(ULONG,LONG*);
HRESULT WINAPI VarI4FromUI8(ULONG64,LONG*);
HRESULT WINAPI VarI4FromStr(OLECHAR*,LCID,ULONG,LONG*);
HRESULT WINAPI VarI4FromCy(CY,LONG*);
HRESULT WINAPI VarI4FromDec(DECIMAL*,LONG*);
HRESULT WINAPI VarI4FromDisp(IDispatch*,LCID,LONG*);

HRESULT WINAPI VarI8FromUI1(BYTE,LONG64*);
HRESULT WINAPI VarI8FromI2(SHORT,LONG64*);
HRESULT WINAPI VarI8FromI4(LONG,LONG64*);
HRESULT WINAPI VarI8FromR4(FLOAT,LONG64*);
HRESULT WINAPI VarI8FromR8(DOUBLE,LONG64*);
HRESULT WINAPI VarI8FromDate(DATE,LONG64*);
HRESULT WINAPI VarI8FromStr(OLECHAR*,LCID,ULONG,LONG64*);
HRESULT WINAPI VarI8FromBool(VARIANT_BOOL,LONG64*);
HRESULT WINAPI VarI8FromI1(signed char,LONG64*);
HRESULT WINAPI VarI8FromUI2(USHORT,LONG64*);
HRESULT WINAPI VarI8FromUI4(ULONG,LONG64*);
HRESULT WINAPI VarI8FromUI8(ULONG64,LONG64*);
HRESULT WINAPI VarI8FromDec(DECIMAL *pdecIn,LONG64*);
HRESULT WINAPI VarI8FromInt(INT intIn,LONG64*);
HRESULT WINAPI VarI8FromCy(CY,LONG64*);
HRESULT WINAPI VarI8FromDisp(IDispatch*,LCID,LONG64*);

HRESULT WINAPI VarR4FromUI1(BYTE,FLOAT*);
HRESULT WINAPI VarR4FromI2(SHORT,FLOAT*);
HRESULT WINAPI VarR4FromI4(LONG,FLOAT*);
HRESULT WINAPI VarR4FromI8(LONG64,FLOAT*);
HRESULT WINAPI VarR4FromR8(DOUBLE,FLOAT*);
HRESULT WINAPI VarR4FromDate(DATE,FLOAT*);
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL,FLOAT*);
HRESULT WINAPI VarR4FromI1(signed char,FLOAT*);
HRESULT WINAPI VarR4FromUI2(USHORT,FLOAT*);
HRESULT WINAPI VarR4FromUI4(ULONG,FLOAT*);
HRESULT WINAPI VarR4FromUI8(ULONG64,FLOAT*);
HRESULT WINAPI VarR4FromStr(OLECHAR*,LCID,ULONG,FLOAT*);
HRESULT WINAPI VarR4FromCy(CY,FLOAT*);
HRESULT WINAPI VarR4FromDec(DECIMAL*,FLOAT*);
HRESULT WINAPI VarR4FromDisp(IDispatch*,LCID,FLOAT*);

HRESULT WINAPI VarR8FromUI1(BYTE,double*);
HRESULT WINAPI VarR8FromI2(SHORT,double*);
HRESULT WINAPI VarR8FromI4(LONG,double*);
HRESULT WINAPI VarR8FromI8(LONG64,double*);
HRESULT WINAPI VarR8FromR4(FLOAT,double*);
HRESULT WINAPI VarR8FromDate(DATE,double*);
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL,double*);
HRESULT WINAPI VarR8FromI1(signed char,double*);
HRESULT WINAPI VarR8FromUI2(USHORT,double*);
HRESULT WINAPI VarR8FromUI4(ULONG,double*);
HRESULT WINAPI VarR8FromUI8(ULONG64,double*);
HRESULT WINAPI VarR8FromStr(OLECHAR*,LCID,ULONG,double*);
HRESULT WINAPI VarR8FromCy(CY,double*);
HRESULT WINAPI VarR8FromDec(const DECIMAL*,double*);
HRESULT WINAPI VarR8FromDisp(IDispatch*,LCID,double*);

HRESULT WINAPI VarDateFromUI1(BYTE,DATE*);
HRESULT WINAPI VarDateFromI2(SHORT,DATE*);
HRESULT WINAPI VarDateFromI4(LONG,DATE*);
HRESULT WINAPI VarDateFromI8(LONG64,DATE*);
HRESULT WINAPI VarDateFromR4(FLOAT,DATE*);
HRESULT WINAPI VarDateFromR8(DOUBLE,DATE*);
HRESULT WINAPI VarDateFromStr(OLECHAR*,LCID,ULONG,DATE*);
HRESULT WINAPI VarDateFromI1(signed char,DATE*);
HRESULT WINAPI VarDateFromUI2(USHORT,DATE*);
HRESULT WINAPI VarDateFromUI4(ULONG,DATE*);
HRESULT WINAPI VarDateFromUI8(ULONG64,DATE*);
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL,DATE*);
HRESULT WINAPI VarDateFromCy(CY,DATE*);
HRESULT WINAPI VarDateFromDec(DECIMAL*,DATE*);
HRESULT WINAPI VarDateFromDisp(IDispatch*,LCID,DATE*);

HRESULT WINAPI VarCyFromUI1(BYTE,CY*);
HRESULT WINAPI VarCyFromI2(SHORT sIn,CY*);
HRESULT WINAPI VarCyFromI4(LONG,CY*);
HRESULT WINAPI VarCyFromI8(LONG64,CY*);
HRESULT WINAPI VarCyFromR4(FLOAT,CY*);
HRESULT WINAPI VarCyFromR8(DOUBLE,CY*);
HRESULT WINAPI VarCyFromDate(DATE,CY*);
HRESULT WINAPI VarCyFromStr(OLECHAR*,LCID,ULONG,CY*);
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL,CY*);
HRESULT WINAPI VarCyFromI1(signed char,CY*);
HRESULT WINAPI VarCyFromUI2(USHORT,CY*);
HRESULT WINAPI VarCyFromUI4(ULONG,CY*);
HRESULT WINAPI VarCyFromUI8(ULONG64,CY*);
HRESULT WINAPI VarCyFromDec(DECIMAL*,CY*);
HRESULT WINAPI VarCyFromDisp(IDispatch*,LCID,CY*);

HRESULT WINAPI VarBstrFromUI1(BYTE,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromI2(SHORT,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromI4(LONG,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromI8(LONG64,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromR4(FLOAT,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromR8(DOUBLE,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromDate(DATE,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromI1(signed char,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromUI2(USHORT,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromUI8(ULONG64,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromUI4(ULONG,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromCy(CY,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromDec(DECIMAL*,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromDisp(IDispatch*,LCID,ULONG,BSTR*);

HRESULT WINAPI VarBoolFromUI1(BYTE,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI2(SHORT,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI4(LONG,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI8(LONG64,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromR4(FLOAT,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromR8(DOUBLE,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDate(DATE,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromStr(OLECHAR*,LCID,ULONG,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI1(signed char,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI2(USHORT,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI4(ULONG,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI8(ULONG64,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromCy(CY,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDec(DECIMAL*,VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDisp(IDispatch*,LCID,VARIANT_BOOL*);

HRESULT WINAPI VarI1FromUI1(BYTE,signed char*);
HRESULT WINAPI VarI1FromI2(SHORT,signed char*);
HRESULT WINAPI VarI1FromI4(LONG,signed char*);
HRESULT WINAPI VarI1FromI8(LONG64,signed char*);
HRESULT WINAPI VarI1FromR4(FLOAT,signed char*);
HRESULT WINAPI VarI1FromR8(DOUBLE,signed char*);
HRESULT WINAPI VarI1FromDate(DATE,signed char*);
HRESULT WINAPI VarI1FromStr(OLECHAR*,LCID,ULONG,signed char*);
HRESULT WINAPI VarI1FromBool(VARIANT_BOOL,signed char*);
HRESULT WINAPI VarI1FromUI2(USHORT,signed char*);
HRESULT WINAPI VarI1FromUI4(ULONG,signed char*);
HRESULT WINAPI VarI1FromUI8(ULONG64,signed char*);
HRESULT WINAPI VarI1FromCy(CY,signed char*);
HRESULT WINAPI VarI1FromDec(DECIMAL*,signed char*);
HRESULT WINAPI VarI1FromDisp(IDispatch*,LCID,signed char*);

HRESULT WINAPI VarUI2FromUI1(BYTE,USHORT*);
HRESULT WINAPI VarUI2FromI2(SHORT,USHORT*);
HRESULT WINAPI VarUI2FromI4(LONG,USHORT*);
HRESULT WINAPI VarUI2FromI8(LONG64,USHORT*);
HRESULT WINAPI VarUI2FromR4(FLOAT,USHORT*);
HRESULT WINAPI VarUI2FromR8(DOUBLE,USHORT*);
HRESULT WINAPI VarUI2FromDate(DATE,USHORT*);
HRESULT WINAPI VarUI2FromStr(OLECHAR*,LCID,ULONG,USHORT*);
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL,USHORT*);
HRESULT WINAPI VarUI2FromI1(signed char,USHORT*);
HRESULT WINAPI VarUI2FromUI4(ULONG,USHORT*);
HRESULT WINAPI VarUI2FromUI8(ULONG64,USHORT*);
HRESULT WINAPI VarUI2FromCy(CY,USHORT*);
HRESULT WINAPI VarUI2FromDec(DECIMAL*,USHORT*);
HRESULT WINAPI VarUI2FromDisp(IDispatch*,LCID,USHORT*);

HRESULT WINAPI VarUI4FromStr(OLECHAR*,LCID,ULONG,ULONG*);
HRESULT WINAPI VarUI4FromUI1(BYTE,ULONG*);
HRESULT WINAPI VarUI4FromI2(SHORT,ULONG*);
HRESULT WINAPI VarUI4FromI4(LONG,ULONG*);
HRESULT WINAPI VarUI4FromI8(LONG64,ULONG*);
HRESULT WINAPI VarUI4FromR4(FLOAT,ULONG*);
HRESULT WINAPI VarUI4FromR8(DOUBLE,ULONG*);
HRESULT WINAPI VarUI4FromDate(DATE,ULONG*);
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL,ULONG*);
HRESULT WINAPI VarUI4FromI1(signed char,ULONG*);
HRESULT WINAPI VarUI4FromUI2(USHORT,ULONG*);
HRESULT WINAPI VarUI4FromUI8(ULONG64,ULONG*);
HRESULT WINAPI VarUI4FromCy(CY,ULONG*);
HRESULT WINAPI VarUI4FromDec(DECIMAL*,ULONG*);
HRESULT WINAPI VarUI4FromDisp(IDispatch*,LCID,ULONG*);

HRESULT WINAPI VarUI8FromUI1(BYTE,ULONG64*);
HRESULT WINAPI VarUI8FromI2(SHORT,ULONG64*);
HRESULT WINAPI VarUI8FromI4(LONG,ULONG64*);
HRESULT WINAPI VarUI8FromI8(LONG64,ULONG64*);
HRESULT WINAPI VarUI8FromR4(FLOAT,ULONG64*);
HRESULT WINAPI VarUI8FromR8(DOUBLE,ULONG64*);
HRESULT WINAPI VarUI8FromDate(DATE,ULONG64*);
HRESULT WINAPI VarUI8FromStr(OLECHAR*,LCID,ULONG,ULONG64*);
HRESULT WINAPI VarUI8FromBool(VARIANT_BOOL,ULONG64*);
HRESULT WINAPI VarUI8FromI1(signed char,ULONG64*);
HRESULT WINAPI VarUI8FromUI2(USHORT,ULONG64*);
HRESULT WINAPI VarUI8FromUI4(ULONG,ULONG64*);
HRESULT WINAPI VarUI8FromDec(DECIMAL*,ULONG64*);
HRESULT WINAPI VarUI8FromInt(INT,ULONG64*);
HRESULT WINAPI VarUI8FromCy(CY,ULONG64*);
HRESULT WINAPI VarUI8FromDisp(IDispatch*,LCID,ULONG64*);

HRESULT WINAPI VarDecFromUI1(BYTE,DECIMAL*);
HRESULT WINAPI VarDecFromI2(SHORT,DECIMAL*);
HRESULT WINAPI VarDecFromI4(LONG,DECIMAL*);
HRESULT WINAPI VarDecFromI8(LONG64,DECIMAL*);
HRESULT WINAPI VarDecFromR4(FLOAT,DECIMAL*);
HRESULT WINAPI VarDecFromR8(DOUBLE,DECIMAL*);
HRESULT WINAPI VarDecFromDate(DATE,DECIMAL*);
HRESULT WINAPI VarDecFromStr(OLECHAR*,LCID,ULONG,DECIMAL*);
HRESULT WINAPI VarDecFromBool(VARIANT_BOOL,DECIMAL*);
HRESULT WINAPI VarDecFromI1(signed char,DECIMAL*);
HRESULT WINAPI VarDecFromUI2(USHORT,DECIMAL*);
HRESULT WINAPI VarDecFromUI4(ULONG,DECIMAL*);
HRESULT WINAPI VarDecFromUI8(ULONG64,DECIMAL*);
HRESULT WINAPI VarDecFromCy(CY,DECIMAL*);
HRESULT WINAPI VarDecFromDisp(IDispatch*,LCID,DECIMAL*);

#define VarUI4FromUI4( in,pOut ) ( *(pOut) =  (in) )
#define VarI4FromI4( in,pOut )   ( *(pOut) =  (in) )

#define VarUI1FromInt   VarUI1FromI4
#define VarUI1FromUint  VarUI1FromUI4
#define VarI2FromInt    VarI2FromI4
#define VarI2FromUint   VarI2FromUI4
#define VarI4FromInt    VarI4FromI4
#define VarI4FromUint   VarI4FromUI4
#define VarI8FromInt    VarI8FromI4
#define VarI8FromUint   VarI8FromUI4
#define VarR4FromInt    VarR4FromI4
#define VarR4FromUint   VarR4FromUI4
#define VarR8FromInt    VarR8FromI4
#define VarR8FromUint   VarR8FromUI4
#define VarDateFromInt  VarDateFromI4
#define VarDateFromUint VarDateFromUI4
#define VarCyFromInt    VarCyFromI4
#define VarCyFromUint   VarCyFromUI4
#define VarBstrFromInt  VarBstrFromI4
#define VarBstrFromUint VarBstrFromUI4
#define VarBoolFromInt  VarBoolFromI4
#define VarBoolFromUint VarBoolFromUI4
#define VarI1FromInt    VarI1FromI4
#define VarI1FromUint   VarI1FromUI4
#define VarUI2FromInt   VarUI2FromI4
#define VarUI2FromUint  VarUI2FromUI4
#define VarUI4FromInt   VarUI4FromI4
#define VarUI4FromUint  VarUI4FromUI4
#define VarUI8FromInt   VarUI8FromI4
#define VarUI8FromUint  VarUI8FromUI4
#define VarDecFromInt   VarDecFromI4
#define VarDecFromUint  VarDecFromUI4
#define VarIntFromUI1   VarI4FromUI1
#define VarIntFromI2    VarI4FromI2
#define VarIntFromI4    VarI4FromI4
#define VarIntFromI8    VarI4FromI8
#define VarIntFromR4    VarI4FromR4
#define VarIntFromR8    VarI4FromR8
#define VarIntFromDate  VarI4FromDate
#define VarIntFromCy    VarI4FromCy
#define VarIntFromStr   VarI4FromStr
#define VarIntFromDisp  VarI4FromDisp
#define VarIntFromBool  VarI4FromBool
#define VarIntFromI1    VarI4FromI1
#define VarIntFromUI2   VarI4FromUI2
#define VarIntFromUI4   VarI4FromUI4
#define VarIntFromUI8   VarI4FromUI8
#define VarIntFromDec   VarI4FromDec
#define VarIntFromUint  VarI4FromUI4
#define VarUintFromUI1  VarUI4FromUI1
#define VarUintFromI2   VarUI4FromI2
#define VarUintFromI4   VarUI4FromI4
#define VarUintFromI8   VarUI4FromI8
#define VarUintFromR4   VarUI4FromR4
#define VarUintFromR8   VarUI4FromR8
#define VarUintFromDate VarUI4FromDate
#define VarUintFromCy   VarUI4FromCy
#define VarUintFromStr  VarUI4FromStr
#define VarUintFromDisp VarUI4FromDisp
#define VarUintFromBool VarUI4FromBool
#define VarUintFromI1   VarUI4FromI1
#define VarUintFromUI2  VarUI4FromUI2
#define VarUintFromUI4  VarUI4FromUI4
#define VarUintFromUI8  VarUI4FromUI8
#define VarUintFromDec  VarUI4FromDec
#define VarUintFromInt  VarUI4FromI4

/*
 * Variant Math operations
 */
#define VARCMP_LT   0
#define VARCMP_EQ   1
#define VARCMP_GT   2
#define VARCMP_NULL 3

HRESULT WINAPI VarR4CmpR8(float,double);

HRESULT WINAPI VarR8Pow(double,double,double*);
HRESULT WINAPI VarR8Round(double,int,double*);

HRESULT WINAPI VarDecAbs(const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecAdd(const DECIMAL*,const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecCmp(const DECIMAL*,const DECIMAL*);
HRESULT WINAPI VarDecCmpR8(const DECIMAL*,DOUBLE);
HRESULT WINAPI VarDecDiv(const DECIMAL*,const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecFix(const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecInt(const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecMul(const DECIMAL*,const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecNeg(const DECIMAL*,DECIMAL*);
HRESULT WINAPI VarDecRound(const DECIMAL*,int,DECIMAL*);
HRESULT WINAPI VarDecSub(const DECIMAL*,const DECIMAL*,DECIMAL*);

HRESULT WINAPI VarCyAbs(const CY,CY*);
HRESULT WINAPI VarCyAdd(const CY,const CY,CY*);
HRESULT WINAPI VarCyCmp(const CY,const CY);
HRESULT WINAPI VarCyCmpR8(const CY,DOUBLE);
HRESULT WINAPI VarCyFix(const CY,CY*);
HRESULT WINAPI VarCyInt(const CY,CY*);
HRESULT WINAPI VarCyMul(const CY,CY,CY*);
HRESULT WINAPI VarCyMulI4(const CY,LONG,CY*);
HRESULT WINAPI VarCyMulI8(const CY,LONG64,CY*);
HRESULT WINAPI VarCyNeg(const CY,CY*);
HRESULT WINAPI VarCyRound(const CY,INT,CY*);
HRESULT WINAPI VarCySub(const CY,const CY,CY*);

HRESULT WINAPI VarAdd(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarAnd(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarCat(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarDiv(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarEqv(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarIdiv(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarImp(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarMod(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarMul(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarOr(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarPow(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarSub(LPVARIANT,LPVARIANT,LPVARIANT);
HRESULT WINAPI VarXor(LPVARIANT,LPVARIANT,LPVARIANT);

HRESULT WINAPI VarAbs(LPVARIANT,LPVARIANT);
HRESULT WINAPI VarFix(LPVARIANT,LPVARIANT);
HRESULT WINAPI VarInt(LPVARIANT,LPVARIANT);
HRESULT WINAPI VarNeg(LPVARIANT,LPVARIANT);
HRESULT WINAPI VarNot(LPVARIANT,LPVARIANT);

HRESULT WINAPI VarRound(LPVARIANT,int,LPVARIANT);

HRESULT WINAPI VarCmp(LPVARIANT,LPVARIANT,LCID,ULONG);

HRESULT WINAPI VarBstrCmp(BSTR,BSTR,LCID,ULONG);
HRESULT WINAPI VarBstrCat(BSTR,BSTR,BSTR*);


typedef struct {
    SYSTEMTIME st;
    USHORT wDayOfYear;
} UDATE;

typedef struct
{
    INT   cDig;       /* Number of parsed digits */
    ULONG dwInFlags;  /* Acceptable state of the input string (NUMPRS_ flags) */
    ULONG dwOutFlags; /* Parsed state of the output string (NUMPRS_ flags) */
    INT   cchUsed;    /* Number of characters parsed from input string */
    INT   nBaseShift; /* Base of the number (but apparently unused) */
    INT   nPwr10;     /* Scale of the number in powers of 10 */
} NUMPARSE;

#define NUMPRS_LEADING_WHITE  0x00001 /* Leading whitespace */
#define NUMPRS_TRAILING_WHITE 0x00002 /* Trailing whitespace */
#define NUMPRS_LEADING_PLUS   0x00004 /* Leading '+' sign */
#define NUMPRS_TRAILING_PLUS  0x00008 /* Trailing '+' sign */
#define NUMPRS_LEADING_MINUS  0x00010 /* Leading '-' sign */
#define NUMPRS_TRAILING_MINUS 0x00020 /* Trailing '-' sign */
#define NUMPRS_HEX_OCT        0x00040 /* Octal number (with a leading 0) */
#define NUMPRS_PARENS         0x00080 /* Parentheses for negative numbers */
#define NUMPRS_DECIMAL        0x00100 /* Decimal separator */
#define NUMPRS_THOUSANDS      0x00200 /* Thousands separator */
#define NUMPRS_CURRENCY       0x00400 /* Currency symbol */
#define NUMPRS_EXPONENT       0x00800 /* Exponent (e.g. "e-14") */
#define NUMPRS_USE_ALL        0x01000 /* Parse the entire string */
#define NUMPRS_STD            0x01FFF /* Standard flags for internal coercions (All of the above) */
#define NUMPRS_NEG            0x10000 /* Number is negative (dwOutFlags only) */
#define NUMPRS_INEXACT        0x20000 /* Number is represented inexactly (dwOutFlags only) */

#define VTBIT_I1      (1 << VT_I1)
#define VTBIT_UI1     (1 << VT_UI1)
#define VTBIT_I2      (1 << VT_I2)
#define VTBIT_UI2     (1 << VT_UI2)
#define VTBIT_I4      (1 << VT_I4)
#define VTBIT_UI4     (1 << VT_UI4)
#define VTBIT_I8      (1 << VT_I8)
#define VTBIT_UI8     (1 << VT_UI8)
#define VTBIT_R4      (1 << VT_R4)
#define VTBIT_R8      (1 << VT_R8)
#define VTBIT_CY      (1 << VT_CY)
#define VTBIT_DECIMAL (1 << VT_DECIMAL)

HRESULT WINAPI VarParseNumFromStr(OLECHAR*,LCID,ULONG,NUMPARSE*,BYTE*);
HRESULT WINAPI VarNumFromParseNum(NUMPARSE*,BYTE*,ULONG,VARIANT*);

INT WINAPI DosDateTimeToVariantTime(USHORT,USHORT,double*);
INT WINAPI VariantTimeToDosDateTime(double,USHORT*,USHORT*);

INT WINAPI VariantTimeToSystemTime(DOUBLE,LPSYSTEMTIME);
INT WINAPI SystemTimeToVariantTime(LPSYSTEMTIME,double*);

HRESULT WINAPI VarDateFromUdate(UDATE*,ULONG,DATE*);
HRESULT WINAPI VarDateFromUdateEx(UDATE*,LCID,ULONG,DATE*);
HRESULT WINAPI VarUdateFromDate(DATE,ULONG,UDATE*);

/* Variant formatting */
HRESULT WINAPI VarWeekdayName(int,int,int,ULONG,BSTR*);
HRESULT WINAPI VarMonthName(int,int,ULONG,BSTR*);
HRESULT WINAPI GetAltMonthNames(LCID,LPOLESTR**);

HRESULT WINAPI VarFormat(LPVARIANT,LPOLESTR,int,int,ULONG,BSTR*);
HRESULT WINAPI VarFormatCurrency(LPVARIANT,int,int,int,int,ULONG,BSTR*);
HRESULT WINAPI VarFormatDateTime(LPVARIANT,int,ULONG,BSTR*);
HRESULT WINAPI VarFormatNumber(LPVARIANT,int,int,int,int,ULONG,BSTR*);
HRESULT WINAPI VarFormatPercent(LPVARIANT,int,int,int,int,ULONG,BSTR*);

HRESULT WINAPI VarFormatFromTokens(LPVARIANT,LPOLESTR,LPBYTE,ULONG,BSTR*,LCID);
HRESULT WINAPI VarTokenizeFormatString(LPOLESTR,LPBYTE,int,int,int,LCID,int*);


/*
 * IDispatch types and helper functions
 */

/* A structure describing a single parameter to a com object method. */
typedef struct tagPARAMDATA
{
    OLECHAR *szName; /* Name of Parameter */
    VARTYPE  vt;     /* Type of Parameter */
} PARAMDATA, *LPPARAMDATA;

/* A structure describing a single method of a com object. */
typedef struct tagMETHODDATA
{
    OLECHAR   *szName;   /* Name of method */
    PARAMDATA *ppdata;   /* Parameters of the method */
    DISPID     dispid;   /* Id of the method */
    UINT       iMeth;    /* Vtable index of the method */
    CALLCONV   cc;       /* Calling convention of the method */
    UINT       cArgs;    /* Number of parameters in the method */
    WORD       wFlags;   /* Type of the method (DISPATCH_ flags) */
    VARTYPE    vtReturn; /* Type of the return value */
} METHODDATA, *LPMETHODDATA;

/* Structure describing a single com object */
typedef struct tagINTERFACEDATA
{
    METHODDATA *pmethdata;  /* Methods of the object */
    UINT        cMembers;   /* Number of methods in the object */
} INTERFACEDATA, *LPINTERFACEDATA;

typedef enum tagREGKIND
{
    REGKIND_DEFAULT,
    REGKIND_REGISTER,
    REGKIND_NONE
} REGKIND;

HRESULT WINAPI DispGetParam(DISPPARAMS*,UINT,VARTYPE,VARIANT*,UINT*);
HRESULT WINAPI DispGetIDsOfNames(ITypeInfo*,OLECHAR**,UINT,DISPID*);
HRESULT WINAPI DispInvoke(void*,ITypeInfo*,DISPID,WORD,DISPPARAMS*,VARIANT*,
                          EXCEPINFO*,UINT*);
HRESULT WINAPI CreateDispTypeInfo(INTERFACEDATA*,LCID,ITypeInfo**);
HRESULT WINAPI CreateStdDispatch(IUnknown*,void*,ITypeInfo*,IUnknown**);
HRESULT WINAPI DispCallFunc(void*,ULONG_PTR,CALLCONV,VARTYPE,UINT,VARTYPE*,
                            VARIANTARG**,VARIANT*);


/*
 * TypeLib API
 */

ULONG WINAPI LHashValOfNameSysA(SYSKIND,LCID,LPCSTR);
ULONG WINAPI LHashValOfNameSys(SYSKIND,LCID,LPCOLESTR);

#define LHashValOfName(lcid,name) LHashValOfNameSys(SYS_WIN32,lcid,name)
#define WHashValOfLHashVal(hash) ((USHORT)((hash) & 0xffff))
#define IsHashValCompatible(hash1,hash2) ((hash1) & 0xff0000 == (hash2) & 0xff0000)

#define MEMBERID_NIL   DISPID_UNKNOWN
#define ID_DEFAULTINST -2

#define DISPATCH_METHOD         0x1
#define DISPATCH_PROPERTYGET    0x2
#define DISPATCH_PROPERTYPUT    0x4
#define DISPATCH_PROPERTYPUTREF 0x8

#define LOAD_TLB_AS_32BIT       0x20
#define LOAD_TLB_AS_64BIT       0x40
#define MASK_TO_RESET_TLB_BITS  ~(LOAD_TLB_AS_32BIT|LOAD_TLB_AS_64BIT)

HRESULT WINAPI CreateTypeLib(SYSKIND,const OLECHAR*,ICreateTypeLib**);
HRESULT WINAPI CreateTypeLib2(SYSKIND,LPCOLESTR,ICreateTypeLib2**);
HRESULT WINAPI LoadRegTypeLib(REFGUID,WORD,WORD,LCID,ITypeLib**);
HRESULT WINAPI LoadTypeLib(const OLECHAR*,ITypeLib**);
HRESULT WINAPI LoadTypeLibEx(LPCOLESTR,REGKIND,ITypeLib**);
HRESULT WINAPI QueryPathOfRegTypeLib(REFGUID,WORD,WORD,LCID,LPBSTR);
HRESULT WINAPI RegisterTypeLib(ITypeLib*,OLECHAR*,OLECHAR*);
HRESULT WINAPI UnRegisterTypeLib(REFGUID,WORD,WORD,LCID,SYSKIND);

VOID WINAPI ClearCustData(LPCUSTDATA);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /*__WINE_OLEAUTO_H*/
