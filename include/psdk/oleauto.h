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
BSTR WINAPI SysAllocString(_In_opt_z_ const OLECHAR*);
BSTR WINAPI SysAllocStringByteLen(_In_opt_z_ LPCSTR, _In_ UINT);

_Ret_writes_maybenull_z_(ui + 1)
BSTR
WINAPI
SysAllocStringLen(
  _In_reads_opt_(ui) const OLECHAR*,
  UINT ui);

void WINAPI SysFreeString(_In_opt_ BSTR);

INT
WINAPI
SysReAllocString(
  _Inout_ _At_(*pbstr, _Pre_z_ _Post_z_ _Post_readable_size_(_String_length_(psz) + 1)) LPBSTR pbstr,
  _In_opt_z_ const OLECHAR *psz);

_Check_return_
int
WINAPI
SysReAllocStringLen(
  _Inout_ _At_(*pbstr, _Pre_z_ _Post_z_ _Post_readable_size_(len + 1)) BSTR *pbstr,
  _In_opt_z_ const OLECHAR*,
  _In_ UINT len);

_Post_equal_to_(_String_length_(bstr) * sizeof(OLECHAR))
UINT
WINAPI
SysStringByteLen(
  _In_opt_ BSTR bstr);

_Post_equal_to_(pbstr == NULL ? 0 : _String_length_(pbstr))
UINT
WINAPI
SysStringLen(
  _In_opt_ BSTR pbstr);

/* IErrorInfo helpers */
HRESULT WINAPI SetErrorInfo(_In_ ULONG, _In_opt_ IErrorInfo*);
_Check_return_ HRESULT WINAPI GetErrorInfo(_In_ ULONG, _Outptr_ IErrorInfo**);
_Check_return_ HRESULT WINAPI CreateErrorInfo(_Outptr_ ICreateErrorInfo**);

/* SafeArray functions */

SAFEARRAY*
WINAPI
SafeArrayCreate(
  _In_ VARTYPE,
  _In_ UINT,
  _In_ SAFEARRAYBOUND*);

SAFEARRAY*
WINAPI
SafeArrayCreateEx(
  _In_ VARTYPE,
  _In_ UINT,
  _In_ SAFEARRAYBOUND*,
  _In_ LPVOID);

SAFEARRAY*
WINAPI
SafeArrayCreateVector(
  _In_ VARTYPE,
  _In_ LONG,
  _In_ ULONG);

SAFEARRAY*
WINAPI
SafeArrayCreateVectorEx(
  _In_ VARTYPE,
  _In_ LONG,
  _In_ ULONG,
  _In_ LPVOID);

HRESULT WINAPI SafeArrayAllocDescriptor(_In_ UINT, _Outptr_ SAFEARRAY**);

HRESULT
WINAPI
SafeArrayAllocDescriptorEx(
  _In_ VARTYPE,
  _In_ UINT,
  _Outptr_ SAFEARRAY**);

HRESULT WINAPI SafeArrayAllocData(_In_ SAFEARRAY*);
HRESULT WINAPI SafeArrayDestroyDescriptor(_In_ SAFEARRAY*);

_Check_return_
HRESULT
WINAPI
SafeArrayPutElement(
  _In_ SAFEARRAY*,
  LONG*,
  _In_ void*);

HRESULT WINAPI SafeArrayGetElement(_In_ SAFEARRAY*, LONG*, _Out_ void*);
HRESULT WINAPI SafeArrayLock(_In_ SAFEARRAY*);
HRESULT WINAPI SafeArrayUnlock(_In_ SAFEARRAY*);
HRESULT WINAPI SafeArrayGetUBound(_In_ SAFEARRAY*, _In_ UINT, _Out_ LONG*);
HRESULT WINAPI SafeArrayGetLBound(_In_ SAFEARRAY*, _In_ UINT, _Out_ LONG*);
UINT    WINAPI SafeArrayGetDim(_In_ SAFEARRAY*);
UINT    WINAPI SafeArrayGetElemsize(_In_ SAFEARRAY*);
HRESULT WINAPI SafeArrayGetVartype(_In_ SAFEARRAY*, _Out_ VARTYPE*);
HRESULT WINAPI SafeArrayAccessData(_In_ SAFEARRAY*, void**);
HRESULT WINAPI SafeArrayUnaccessData(_In_ SAFEARRAY*);

HRESULT
WINAPI
SafeArrayPtrOfIndex(
  _In_ SAFEARRAY *psa,
  _In_reads_(psa->cDims) LONG*,
  _Outptr_result_bytebuffer_(psa->cbElements) void **);

_Check_return_
HRESULT
WINAPI
SafeArrayCopyData(
  _In_ SAFEARRAY*,
  _In_ SAFEARRAY*);

HRESULT WINAPI SafeArrayDestroyData(_In_ SAFEARRAY*);
HRESULT WINAPI SafeArrayDestroy(_In_ SAFEARRAY*);

_Check_return_
HRESULT
WINAPI
SafeArrayCopy(
  _In_ SAFEARRAY*,
  _Outptr_ SAFEARRAY**);

HRESULT WINAPI SafeArrayRedim(_Inout_ SAFEARRAY*, _In_ SAFEARRAYBOUND*);
HRESULT WINAPI SafeArraySetRecordInfo(_In_ SAFEARRAY*, _In_ IRecordInfo*);
HRESULT WINAPI SafeArrayGetRecordInfo(_In_ SAFEARRAY*, _Outptr_ IRecordInfo**);
HRESULT WINAPI SafeArraySetIID(_In_ SAFEARRAY*, _In_ REFGUID);
HRESULT WINAPI SafeArrayGetIID(_In_ SAFEARRAY*, _Out_ GUID*);

_Check_return_ HRESULT WINAPI VectorFromBstr(_In_ BSTR, _Outptr_ SAFEARRAY**);
_Check_return_ HRESULT WINAPI BstrFromVector(_In_ SAFEARRAY*, _Out_ BSTR*);

/* Object registration helpers */
#define ACTIVEOBJECT_STRONG 0
#define ACTIVEOBJECT_WEAK   1

_Check_return_
HRESULT
WINAPI
RegisterActiveObject(
  LPUNKNOWN,
  REFCLSID,
  DWORD,
  LPDWORD);

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
#define V_VT(A)         ((A)->n1.n2.vt)
#define V_UNION(A,B)    ((A)->n1.n2.n3.B)
#define V_RECORD(A)     (V_UNION(A,brecVal).pvRecord)
#define V_RECORDINFO(A) (V_UNION(A,brecVal).pRecInfo)
#else
#define V_VT(A)         ((A)->vt)
#define V_UNION(A,B)    ((A)->B)
#define V_RECORD(A)     ((A)->pvRecord)
#define V_RECORDINFO(A) ((A)->pRecInfo)
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

void    WINAPI VariantInit(_Out_ VARIANT*);
HRESULT WINAPI VariantClear(_Inout_ VARIANT*);
_Check_return_ HRESULT WINAPI VariantCopy(_Inout_ VARIANT*, _In_ VARIANT*);
_Check_return_ HRESULT WINAPI VariantCopyInd(_Inout_ VARIANT*, _In_ VARIANT*);

_Check_return_
HRESULT
WINAPI
VariantChangeType(
  _Inout_ VARIANT*,
  _In_ VARIANT*,
  _In_ USHORT,
  _In_ VARTYPE);

_Check_return_
HRESULT
WINAPI
VariantChangeTypeEx(
  _Inout_ VARIANT*,
  _In_ VARIANT*,
  _In_ LCID,
  _In_ USHORT,
  _In_ VARTYPE);

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

HRESULT WINAPI VarUI1FromI2(SHORT, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromI4(LONG, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromI8(LONG64, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromR4(FLOAT, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromR8(DOUBLE, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromDate(DATE, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromI1(signed char, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromUI2(USHORT, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromUI4(ULONG, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromUI8(ULONG64, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromCy(CY, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromDec(_In_ DECIMAL*, _Out_ BYTE*);
HRESULT WINAPI VarUI1FromDisp(IDispatch*, LCID, _Out_ BYTE*);

HRESULT WINAPI VarI2FromUI1(BYTE, _Out_ SHORT*);
HRESULT WINAPI VarI2FromI4(LONG, _Out_ SHORT*);
HRESULT WINAPI VarI2FromI8(LONG64, _Out_ SHORT*);
HRESULT WINAPI VarI2FromR4(FLOAT, _Out_ SHORT*);
HRESULT WINAPI VarI2FromR8(DOUBLE, _Out_ SHORT*);
HRESULT WINAPI VarI2FromDate(DATE, _Out_ SHORT*);
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL, _Out_ SHORT*);
HRESULT WINAPI VarI2FromI1(signed char, _Out_ SHORT*);
HRESULT WINAPI VarI2FromUI2(USHORT, _Out_ SHORT*);
HRESULT WINAPI VarI2FromUI4(ULONG, _Out_ SHORT*);
HRESULT WINAPI VarI2FromUI8(ULONG64, _Out_ SHORT*);
HRESULT WINAPI VarI2FromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ SHORT*);
HRESULT WINAPI VarI2FromCy(CY,SHORT*);
HRESULT WINAPI VarI2FromDec(_In_ DECIMAL*, _Out_ SHORT*);
HRESULT WINAPI VarI2FromDisp(IDispatch*, LCID, _Out_ SHORT*);

HRESULT WINAPI VarI4FromUI1(BYTE, _Out_ LONG*);
HRESULT WINAPI VarI4FromI2(SHORT, _Out_ LONG*);
HRESULT WINAPI VarI4FromI8(LONG64, _Out_ LONG*);
HRESULT WINAPI VarI4FromR4(FLOAT, _Out_ LONG*);
HRESULT WINAPI VarI4FromR8(DOUBLE, _Out_ LONG*);
HRESULT WINAPI VarI4FromDate(DATE, _Out_ LONG*);
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL, _Out_ LONG*);
HRESULT WINAPI VarI4FromI1(signed char, _Out_ LONG*);
HRESULT WINAPI VarI4FromUI2(USHORT, _Out_ LONG*);
HRESULT WINAPI VarI4FromUI4(ULONG, _Out_ LONG*);
HRESULT WINAPI VarI4FromUI8(ULONG64, _Out_ LONG*);
HRESULT WINAPI VarI4FromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ LONG*);
HRESULT WINAPI VarI4FromCy(CY, _Out_ LONG*);
HRESULT WINAPI VarI4FromDec(_In_ DECIMAL*, _Out_ LONG*);
HRESULT WINAPI VarI4FromDisp(IDispatch*, _In_ LCID, _Out_ LONG*);

HRESULT WINAPI VarI8FromUI1(BYTE, _Out_ LONG64*);
HRESULT WINAPI VarI8FromI2(SHORT, _Out_ LONG64*);
HRESULT WINAPI VarI8FromI4(LONG,LONG64*);
HRESULT WINAPI VarI8FromR4(FLOAT, _Out_ LONG64*);
HRESULT WINAPI VarI8FromR8(DOUBLE, _Out_ LONG64*);
HRESULT WINAPI VarI8FromDate(DATE, _Out_ LONG64*);
HRESULT WINAPI VarI8FromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ LONG64*);
HRESULT WINAPI VarI8FromBool(VARIANT_BOOL, _Out_ LONG64*);
HRESULT WINAPI VarI8FromI1(signed char, _Out_ LONG64*);
HRESULT WINAPI VarI8FromUI2(USHORT, _Out_ LONG64*);
HRESULT WINAPI VarI8FromUI4(ULONG, _Out_ LONG64*);
HRESULT WINAPI VarI8FromUI8(ULONG64, _Out_ LONG64*);
HRESULT WINAPI VarI8FromDec(_In_ DECIMAL *pdecIn, _Out_ LONG64*);
HRESULT WINAPI VarI8FromInt(INT intIn,LONG64*);
HRESULT WINAPI VarI8FromCy(_In_ CY, _Out_ LONG64*);
HRESULT WINAPI VarI8FromDisp(IDispatch*, _In_ LCID, _Out_ LONG64*);

HRESULT WINAPI VarR4FromUI1(BYTE, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromI2(SHORT, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromI4(LONG, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromI8(LONG64, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromR8(DOUBLE, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromDate(DATE, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromI1(signed char, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromUI2(USHORT, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromUI4(ULONG, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromUI8(ULONG64, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromCy(CY,FLOAT*);
HRESULT WINAPI VarR4FromDec(_In_ DECIMAL*, _Out_ FLOAT*);
HRESULT WINAPI VarR4FromDisp(IDispatch*, LCID, _Out_ FLOAT*);

HRESULT WINAPI VarR8FromUI1(BYTE, _Out_ double*);
HRESULT WINAPI VarR8FromI2(SHORT, _Out_ double*);
HRESULT WINAPI VarR8FromI4(LONG, _Out_ double*);
HRESULT WINAPI VarR8FromI8(LONG64, _Out_ double*);
HRESULT WINAPI VarR8FromR4(FLOAT, _Out_ double*);
HRESULT WINAPI VarR8FromDate(DATE, _Out_ double*);
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL, _Out_ double*);
HRESULT WINAPI VarR8FromI1(signed char,double*);
HRESULT WINAPI VarR8FromUI2(USHORT, _Out_ double*);
HRESULT WINAPI VarR8FromUI4(ULONG, _Out_ double*);
HRESULT WINAPI VarR8FromUI8(ULONG64, _Out_ double*);
HRESULT WINAPI VarR8FromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ double*);
HRESULT WINAPI VarR8FromCy(CY,double*);
HRESULT WINAPI VarR8FromDec(_In_ const DECIMAL*, _Out_ double*);
HRESULT WINAPI VarR8FromDisp(IDispatch*, LCID, _Out_ double*);

HRESULT WINAPI VarDateFromUI1(BYTE, _Out_ DATE*);
HRESULT WINAPI VarDateFromI2(SHORT, _Out_ DATE*);
HRESULT WINAPI VarDateFromI4(LONG, _Out_ DATE*);
HRESULT WINAPI VarDateFromI8(LONG64, _Out_ DATE*);
HRESULT WINAPI VarDateFromR4(FLOAT, _Out_ DATE*);
HRESULT WINAPI VarDateFromR8(DOUBLE, _Out_ DATE*);
HRESULT WINAPI VarDateFromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ DATE*);
HRESULT WINAPI VarDateFromI1(signed char, _Out_ DATE*);
HRESULT WINAPI VarDateFromUI2(USHORT, _Out_ DATE*);
HRESULT WINAPI VarDateFromUI4(ULONG, _Out_ DATE*);
HRESULT WINAPI VarDateFromUI8(ULONG64, _Out_ DATE*);
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL, _Out_ DATE*);
HRESULT WINAPI VarDateFromCy(CY, _Out_ DATE*);
HRESULT WINAPI VarDateFromDec(_In_ DECIMAL*, _Out_ DATE*);
HRESULT WINAPI VarDateFromDisp(IDispatch*, LCID, _Out_ DATE*);

HRESULT WINAPI VarCyFromUI1(BYTE, _Out_ CY*);
HRESULT WINAPI VarCyFromI2(SHORT sIn, _Out_ CY*);
HRESULT WINAPI VarCyFromI4(LONG, _Out_ CY*);
HRESULT WINAPI VarCyFromI8(LONG64, _Out_ CY*);
HRESULT WINAPI VarCyFromR4(FLOAT, _Out_ CY*);
HRESULT WINAPI VarCyFromR8(DOUBLE, _Out_ CY*);
HRESULT WINAPI VarCyFromDate(DATE, _Out_ CY*);
HRESULT WINAPI VarCyFromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ CY*);
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL, _Out_ CY*);
HRESULT WINAPI VarCyFromI1(signed char, _Out_ CY*);
HRESULT WINAPI VarCyFromUI2(USHORT, _Out_ CY*);
HRESULT WINAPI VarCyFromUI4(ULONG, _Out_ CY*);
HRESULT WINAPI VarCyFromUI8(ULONG64, _Out_ CY*);
HRESULT WINAPI VarCyFromDec(_In_ DECIMAL*, _Out_ CY*);
HRESULT WINAPI VarCyFromDisp(_In_ IDispatch*, LCID, _Out_ CY*);

HRESULT WINAPI VarBstrFromUI1(BYTE, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromI2(SHORT,LCID,ULONG,BSTR*);
HRESULT WINAPI VarBstrFromI4(LONG, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromI8(LONG64, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromR4(FLOAT, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromR8(DOUBLE, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromDate(_In_ DATE, _In_ LCID, _In_ ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromI1(signed char, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromUI2(USHORT, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromUI8(ULONG64, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromUI4(ULONG, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromCy(CY, LCID, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromDec(_In_ DECIMAL*, _In_ LCID, _In_ ULONG, _Out_ BSTR*);
HRESULT WINAPI VarBstrFromDisp(IDispatch*, LCID, ULONG, _Out_ BSTR*);

HRESULT WINAPI VarBoolFromUI1(BYTE, _Out_ VARIANT_BOOL*);
_Check_return_ HRESULT WINAPI VarBoolFromI2(_In_ SHORT, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI4(LONG, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI8(LONG64, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromR4(FLOAT, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromR8(DOUBLE, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDate(DATE, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromStr(_In_ OLECHAR*, LCID, ULONG, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromI1(signed char, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI2(USHORT, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI4(ULONG, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromUI8(ULONG64, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromCy(CY, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDec(_In_ DECIMAL*, _Out_ VARIANT_BOOL*);
HRESULT WINAPI VarBoolFromDisp(IDispatch*, LCID, _Out_ VARIANT_BOOL*);

HRESULT WINAPI VarI1FromUI1(_In_ BYTE, _Out_ signed char*);
HRESULT WINAPI VarI1FromI2(_In_ SHORT, _Out_ signed char*);
HRESULT WINAPI VarI1FromI4(_In_ LONG, _Out_ signed char*);
HRESULT WINAPI VarI1FromI8(_In_ LONG64, _Out_ signed char*);
HRESULT WINAPI VarI1FromR4(_In_ FLOAT, _Out_ signed char*);
HRESULT WINAPI VarI1FromR8(_In_ DOUBLE, _Out_ signed char*);
HRESULT WINAPI VarI1FromDate(_In_ DATE, _Out_ signed char*);
HRESULT WINAPI VarI1FromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ signed char*);
HRESULT WINAPI VarI1FromBool(_In_ VARIANT_BOOL, _Out_ signed char*);
HRESULT WINAPI VarI1FromUI2(_In_ USHORT, _Out_ signed char*);
HRESULT WINAPI VarI1FromUI4(_In_ ULONG, _Out_ signed char*);
HRESULT WINAPI VarI1FromUI8(_In_ ULONG64, _Out_ signed char*);
HRESULT WINAPI VarI1FromCy(_In_ CY, _Out_ signed char*);
HRESULT WINAPI VarI1FromDec(_In_ DECIMAL*, _Out_ signed char*);
HRESULT WINAPI VarI1FromDisp(_In_ IDispatch*, _In_ LCID, _Out_ signed char*);

HRESULT WINAPI VarUI2FromUI1(BYTE, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromI2(SHORT, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromI4(LONG, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromI8(LONG64, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromR4(FLOAT, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromR8(DOUBLE,USHORT*);
HRESULT WINAPI VarUI2FromDate(DATE, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromI1(signed char, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromUI4(ULONG, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromUI8(ULONG64, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromCy(CY, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromDec(_In_ DECIMAL*, _Out_ USHORT*);
HRESULT WINAPI VarUI2FromDisp(_In_ IDispatch*, LCID, _Out_ USHORT*);

HRESULT WINAPI VarUI4FromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromUI1(BYTE, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromI2(_In_ SHORT, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromI4(LONG, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromI8(LONG64, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromR4(FLOAT, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromR8(DOUBLE, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromDate(DATE, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromI1(signed char, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromUI2(USHORT, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromUI8(ULONG64, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromCy(CY, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromDec(_In_ DECIMAL*, _Out_ ULONG*);
HRESULT WINAPI VarUI4FromDisp(_In_ IDispatch*, LCID, _Out_ ULONG*);

HRESULT WINAPI VarUI8FromUI1(BYTE, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromI2(SHORT, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromI4(LONG, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromI8(LONG64, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromR4(FLOAT, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromR8(DOUBLE, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromDate(DATE, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromBool(VARIANT_BOOL, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromI1(signed char, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromUI2(USHORT, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromUI4(ULONG, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromDec(_In_ DECIMAL*, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromInt(INT,ULONG64*);
HRESULT WINAPI VarUI8FromCy(CY, _Out_ ULONG64*);
HRESULT WINAPI VarUI8FromDisp(_In_ IDispatch*, LCID, _Out_ ULONG64*);

HRESULT WINAPI VarDecFromUI1(_In_ BYTE, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromI2(_In_ SHORT, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromI4(_In_ LONG, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromI8(LONG64, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromR4(_In_ FLOAT, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromR8(_In_ DOUBLE, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromDate(_In_ DATE, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromStr(_In_ OLECHAR*, _In_ LCID, _In_ ULONG, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromBool(_In_ VARIANT_BOOL, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromI1(_In_ signed char, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromUI2(_In_ USHORT, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromUI4(_In_ ULONG, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromUI8(ULONG64, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromCy(_In_ CY, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFromDisp(_In_ IDispatch*, _In_ LCID, _Out_ DECIMAL*);

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

HRESULT WINAPI VarR4CmpR8(_In_ float, _In_ double);

HRESULT WINAPI VarR8Pow(_In_ double, _In_ double, _Out_ double*);
HRESULT WINAPI VarR8Round(_In_ double, _In_ int, _Out_ double*);

HRESULT WINAPI VarDecAbs(_In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecAdd(_In_ const DECIMAL*, _In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecCmp(_In_ const DECIMAL*, _In_ const DECIMAL*);
HRESULT WINAPI VarDecCmpR8(_In_ const DECIMAL*, _In_ DOUBLE);
HRESULT WINAPI VarDecDiv(_In_ const DECIMAL*, _In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecFix(_In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecInt(_In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecMul(_In_ const DECIMAL*, _In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecNeg(_In_ const DECIMAL*, _Out_ DECIMAL*);
HRESULT WINAPI VarDecRound(_In_ const DECIMAL*, int, _Out_ DECIMAL*);
HRESULT WINAPI VarDecSub(_In_ const DECIMAL*, _In_ const DECIMAL*, _Out_ DECIMAL*);

HRESULT WINAPI VarCyAbs(_In_ const CY, _Out_ CY*);
HRESULT WINAPI VarCyAdd(_In_ const CY, _In_ const CY, _Out_ CY*);
HRESULT WINAPI VarCyCmp(_In_ const CY, _In_ const CY);
HRESULT WINAPI VarCyCmpR8(_In_ const CY, _In_ DOUBLE);
HRESULT WINAPI VarCyFix(_In_ const CY, _Out_ CY*);
HRESULT WINAPI VarCyInt(_In_ const CY, _Out_ CY*);
HRESULT WINAPI VarCyMul(_In_ const CY, _In_ CY, _Out_ CY*);
HRESULT WINAPI VarCyMulI4(_In_ const CY, _In_ LONG, _Out_ CY*);
HRESULT WINAPI VarCyMulI8(_In_ const CY, _In_ LONG64, _Out_ CY*);
HRESULT WINAPI VarCyNeg(_In_ const CY, _Out_ CY*);
HRESULT WINAPI VarCyRound(_In_ const CY, _In_ INT, _Out_ CY*);
HRESULT WINAPI VarCySub(_In_ const CY, _In_ const CY, _Out_ CY*);

HRESULT WINAPI VarAdd(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarAnd(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarCat(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarDiv(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarEqv(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarIdiv(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarImp(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarMod(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarMul(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarOr(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarPow(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarSub(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarXor(_In_ LPVARIANT, _In_ LPVARIANT, _Out_ LPVARIANT);

HRESULT WINAPI VarAbs(_In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarFix(_In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarInt(_In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarNeg(_In_ LPVARIANT, _Out_ LPVARIANT);
HRESULT WINAPI VarNot(_In_ LPVARIANT, _Out_ LPVARIANT);

HRESULT WINAPI VarRound(_In_ LPVARIANT, _In_ int, _Out_ LPVARIANT);

HRESULT WINAPI VarCmp(_In_ LPVARIANT, _In_ LPVARIANT, _In_ LCID, _In_ ULONG);

HRESULT WINAPI VarBstrCmp(_In_ BSTR, _In_ BSTR, _In_ LCID, _In_ ULONG);
HRESULT WINAPI VarBstrCat(_In_ BSTR, _In_ BSTR, _Out_ BSTR*);


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

_Check_return_
HRESULT
WINAPI
VarParseNumFromStr(
  _In_ OLECHAR*,
  _In_ LCID,
  _In_ ULONG,
  _Out_ NUMPARSE*,
  _Out_ BYTE*);

_Check_return_
HRESULT
WINAPI
VarNumFromParseNum(
  _In_ NUMPARSE*,
  _In_ BYTE*,
  _In_ ULONG,
  _Out_ VARIANT*);

INT WINAPI DosDateTimeToVariantTime(_In_ USHORT, _In_ USHORT, _Out_ double*);
INT WINAPI VariantTimeToDosDateTime(_In_ double, _Out_ USHORT*, _Out_ USHORT*);

INT WINAPI VariantTimeToSystemTime(_In_ DOUBLE, _Out_ LPSYSTEMTIME);
INT WINAPI SystemTimeToVariantTime(_In_ LPSYSTEMTIME, _Out_ double*);

_Check_return_
HRESULT
WINAPI
VarDateFromUdate(
  _In_ UDATE*,
  _In_ ULONG,
  _Out_ DATE*);

HRESULT
WINAPI
VarDateFromUdateEx(
  _In_ UDATE*,
  _In_ LCID,
  _In_ ULONG,
  _Out_ DATE*);

_Check_return_
HRESULT
WINAPI
VarUdateFromDate(
  _In_ DATE,
  _In_ ULONG,
  _Out_ UDATE*);

/* Variant formatting */
HRESULT WINAPI VarWeekdayName(int, int, int, ULONG, _Out_ BSTR*);
HRESULT WINAPI VarMonthName(int, int, ULONG, _Out_ BSTR*);

_Check_return_
HRESULT
WINAPI
GetAltMonthNames(
  LCID,
  _Outptr_result_buffer_maybenull_(13) LPOLESTR**);

HRESULT
WINAPI
VarFormat(
  _In_ LPVARIANT,
  _In_opt_ LPOLESTR,
  int,
  int,
  ULONG,
  _Out_ BSTR*);

HRESULT
WINAPI
VarFormatCurrency(
  _In_ LPVARIANT,
  int,
  int,
  int,
  int,
  ULONG,
  _Out_ BSTR*);

HRESULT WINAPI VarFormatDateTime(_In_ LPVARIANT, int, ULONG, _Out_ BSTR*);

HRESULT
WINAPI
VarFormatNumber(
  _In_ LPVARIANT,
  int,
  int,
  int,
  int,
  ULONG,
  _Out_ BSTR*);

HRESULT
WINAPI
VarFormatPercent(
  _In_ LPVARIANT,
  int,
  int,
  int,
  int,
  ULONG,
  _Out_ BSTR*);

HRESULT
WINAPI
VarFormatFromTokens(
  _In_ LPVARIANT,
  _In_opt_ LPOLESTR,
  LPBYTE,
  ULONG,
  _Out_ BSTR*,
  LCID);

HRESULT
WINAPI
VarTokenizeFormatString(
  _In_opt_ LPOLESTR,
  _Inout_ LPBYTE,
  int,
  int,
  int,
  LCID,
  _In_opt_ int*);


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

_Check_return_
HRESULT
WINAPI
DispGetParam(
  _In_ DISPPARAMS*,
  UINT,
  VARTYPE,
  _Out_ VARIANT*,
  _Out_opt_ UINT*);

_Check_return_
HRESULT
WINAPI
DispGetIDsOfNames(
  ITypeInfo*,
  _In_reads_(cNames) OLECHAR**,
  UINT cNames,
  _Out_writes_(cNames) DISPID*);

_Check_return_
HRESULT
WINAPI
DispInvoke(
  void*,
  ITypeInfo*,
  DISPID,
  WORD,
  DISPPARAMS*,
  VARIANT*,
  EXCEPINFO*,
  UINT*);

_Check_return_
HRESULT
WINAPI
CreateDispTypeInfo(
  INTERFACEDATA*,
  LCID,
  ITypeInfo**);

_Check_return_
HRESULT
WINAPI
CreateStdDispatch(
  IUnknown*,
  void*,
  ITypeInfo*,
  IUnknown**);

HRESULT
WINAPI
DispCallFunc(
  void*,
  ULONG_PTR,
  CALLCONV,
  VARTYPE,
  UINT,
  VARTYPE*,
  VARIANTARG**,
  VARIANT*);


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

_Check_return_
HRESULT
WINAPI
CreateTypeLib(
  SYSKIND,
  const OLECHAR*,
  ICreateTypeLib**);

_Check_return_
HRESULT
WINAPI
CreateTypeLib2(
  SYSKIND,
  LPCOLESTR,
  ICreateTypeLib2**);

_Check_return_
HRESULT
WINAPI
LoadRegTypeLib(
  REFGUID,
  WORD,
  WORD,
  LCID,
  ITypeLib**);

HRESULT WINAPI LoadTypeLib(_In_z_ const OLECHAR*, ITypeLib**);
_Check_return_ HRESULT WINAPI LoadTypeLibEx(LPCOLESTR, REGKIND, ITypeLib**);
HRESULT WINAPI QueryPathOfRegTypeLib(REFGUID,WORD,WORD,LCID,LPBSTR);

_Check_return_
HRESULT
WINAPI
RegisterTypeLib(
  ITypeLib*,
  _In_ OLECHAR*,
  _In_opt_ OLECHAR*);

_Check_return_
HRESULT
WINAPI
UnRegisterTypeLib(
  REFGUID,
  WORD,
  WORD,
  LCID,
  SYSKIND);

HRESULT
WINAPI
RegisterTypeLibForUser(
  ITypeLib*,
  _In_ OLECHAR*,
  _In_opt_ OLECHAR*);

HRESULT WINAPI UnRegisterTypeLibForUser(REFGUID,WORD,WORD,LCID,SYSKIND);

VOID WINAPI ClearCustData(LPCUSTDATA);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /*__WINE_OLEAUTO_H*/
