/*
 * Copyright 2008 James Hawkins for CodeWeavers
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

#ifndef __WINE_PROPVARUTIL_H
#define __WINE_PROPVARUTIL_H

#include <shtypes.h>
#include <shlwapi.h>

#ifndef PSSTDAPI
#ifdef _PROPSYS_
#define PSSTDAPI          STDAPI
#define PSSTDAPI_(type)   STDAPI_(type)
#else
#define PSSTDAPI          DECLSPEC_IMPORT STDAPI
#define PSSTDAPI_(type)   DECLSPEC_IMPORT STDAPI_(type)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum tagPROPVAR_CHANGE_FLAGS
{
    PVCHF_DEFAULT           = 0x00000000,
    PVCHF_NOVALUEPROP       = 0x00000001,
    PVCHF_ALPHABOOL         = 0x00000002,
    PVCHF_NOUSEROVERRIDE    = 0x00000004,
    PVCHF_LOCALBOOL         = 0x00000008,
    PVCHF_NOHEXSTRING       = 0x00000010,
};

typedef int PROPVAR_CHANGE_FLAGS;

enum tagPROPVAR_COMPARE_UNIT
{
    PVCU_DEFAULT           = 0x00000000,
    PVCU_SECOND            = 0x00000001,
    PVCU_MINUTE            = 0x00000002,
    PVCU_HOUR              = 0x00000003,
    PVCU_DAY               = 0x00000004,
    PVCU_MONTH             = 0x00000005,
    PVCU_YEAR              = 0x00000006,
};

typedef int PROPVAR_COMPARE_UNIT;

enum tagPROPVAR_COMPARE_FLAGS
{
    PVCF_DEFAULT           = 0x00000000,
    PVCF_TREATEMPTYASGREATERTHAN = 0x00000001,
    PVCF_USESTRCMP         = 0x00000002,
    PVCF_USESTRCMPC        = 0x00000004,
    PVCF_USESTRCMPI        = 0x00000008,
    PVCF_USESTRCMPIC       = 0x00000010,
};

typedef int PROPVAR_COMPARE_FLAGS;

PSSTDAPI PropVariantChangeType(PROPVARIANT *ppropvarDest, REFPROPVARIANT propvarSrc,
                                     PROPVAR_CHANGE_FLAGS flags, VARTYPE vt);
PSSTDAPI InitPropVariantFromGUIDAsString(REFGUID guid, PROPVARIANT *ppropvar);
PSSTDAPI InitVariantFromFileTime(const FILETIME *ft, VARIANT *var);
PSSTDAPI InitVariantFromGUIDAsString(REFGUID guid, VARIANT *pvar);
PSSTDAPI InitPropVariantFromBuffer(const VOID *pv, UINT cb, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromCLSID(REFCLSID clsid, PROPVARIANT *ppropvar);
PSSTDAPI InitVariantFromBuffer(const VOID *pv, UINT cb, VARIANT *pvar);
PSSTDAPI PropVariantToGUID(const PROPVARIANT *ppropvar, GUID *guid);
PSSTDAPI VariantToGUID(const VARIANT *pvar, GUID *guid);
PSSTDAPI_(INT) PropVariantCompareEx(REFPROPVARIANT propvar1, REFPROPVARIANT propvar2,
                                PROPVAR_COMPARE_UNIT uint, PROPVAR_COMPARE_FLAGS flags);
PSSTDAPI InitPropVariantFromFileTime(const FILETIME *pftIn, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromStringVector(PCWSTR *strs, ULONG count, PROPVARIANT *ppropvar);

PSSTDAPI PropVariantToDouble(REFPROPVARIANT propvarIn, double *ret);
PSSTDAPI PropVariantToInt16(REFPROPVARIANT propvarIn, SHORT *ret);
PSSTDAPI PropVariantToInt32(REFPROPVARIANT propvarIn, LONG *ret);
PSSTDAPI PropVariantToInt64(REFPROPVARIANT propvarIn, LONGLONG *ret);
PSSTDAPI PropVariantToUInt16(REFPROPVARIANT propvarIn, USHORT *ret);
PSSTDAPI PropVariantToUInt32(REFPROPVARIANT propvarIn, ULONG *ret);
PSSTDAPI_(ULONG) PropVariantToUInt32WithDefault(REFPROPVARIANT propvarIn, ULONG uLDefault);
PSSTDAPI PropVariantToUInt64(REFPROPVARIANT propvarIn, ULONGLONG *ret);
PSSTDAPI PropVariantToBoolean(REFPROPVARIANT propvarIn, BOOL *ret);
PSSTDAPI PropVariantToBSTR(REFPROPVARIANT propvar, BSTR *bstr);
PSSTDAPI PropVariantToBuffer(REFPROPVARIANT propvarIn, void *ret, UINT cb);
PSSTDAPI PropVariantToString(REFPROPVARIANT propvarIn, PWSTR ret, UINT cch);
PSSTDAPI_(PCWSTR) PropVariantToStringWithDefault(REFPROPVARIANT propvarIn, LPCWSTR pszDefault);
PSSTDAPI_(PCWSTR) VariantToStringWithDefault(const VARIANT *pvar, LPCWSTR pszDefault);
PSSTDAPI VariantToString(REFVARIANT var, PWSTR ret, UINT cch);

PSSTDAPI PropVariantToStringAlloc(REFPROPVARIANT propvarIn, WCHAR **ret);

PSSTDAPI PropVariantToVariant(const PROPVARIANT *propvar, VARIANT *var);
PSSTDAPI VariantToPropVariant(const VARIANT* var, PROPVARIANT* propvar);

typedef enum tagPSTIME_FLAGS {
    PSTF_UTC   = 0,
    PSTF_LOCAL = 1
} PSTIME_FLAGS;

PSSTDAPI PropVariantToFileTime(REFPROPVARIANT propvarIn, PSTIME_FLAGS pstfOut, FILETIME *pftOut);
PSSTDAPI_(ULONG) PropVariantGetElementCount(REFPROPVARIANT propvar);

PSSTDAPI InitPropVariantFromBooleanVector(const BOOL *prgf, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromInt16Vector(const SHORT *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromUInt16Vector(const USHORT *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromInt32Vector(const LONG *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromUInt32Vector(const ULONG *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromInt64Vector(const LONGLONG *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromUInt64Vector(const ULONGLONG *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromDoubleVector(const DOUBLE *prgn, ULONG cElems, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromPropVariantVectorElem(REFPROPVARIANT propvarIn, ULONG iElem, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantVectorFromPropVariant(REFPROPVARIANT propvarSingle, PROPVARIANT *ppropvar);
PSSTDAPI InitPropVariantFromResource(HMODULE hinst, UINT id, PROPVARIANT *ppropvar);

PSSTDAPI_(BOOL)      PropVariantToBooleanWithDefault(REFPROPVARIANT propvarIn, BOOL fDefault);
PSSTDAPI_(LONG)      PropVariantToInt32WithDefault(REFPROPVARIANT propvarIn, LONG lDefault);
PSSTDAPI_(LONGLONG)  PropVariantToInt64WithDefault(REFPROPVARIANT propvarIn, LONGLONG llDefault);
PSSTDAPI_(USHORT)    PropVariantToUInt16WithDefault(REFPROPVARIANT propvarIn, USHORT uiDefault);
PSSTDAPI_(ULONGLONG) PropVariantToUInt64WithDefault(REFPROPVARIANT propvarIn, ULONGLONG ullDefault);
PSSTDAPI_(DOUBLE)    PropVariantToDoubleWithDefault(REFPROPVARIANT propvarIn, DOUBLE dblDefault);

PSSTDAPI PropVariantGetStringElem(REFPROPVARIANT propvar, ULONG iElem, PWSTR *ppszVal);
PSSTDAPI PropVariantGetBooleanElem(REFPROPVARIANT propvar, ULONG iElem, BOOL *pfVal);
PSSTDAPI PropVariantGetInt16Elem(REFPROPVARIANT propvar, ULONG iElem, SHORT *pnVal);
PSSTDAPI PropVariantGetUInt16Elem(REFPROPVARIANT propvar, ULONG iElem, USHORT *pnVal);
PSSTDAPI PropVariantGetInt32Elem(REFPROPVARIANT propvar, ULONG iElem, LONG *pnVal);
PSSTDAPI PropVariantGetUInt32Elem(REFPROPVARIANT propvar, ULONG iElem, ULONG *pnVal);
PSSTDAPI PropVariantGetInt64Elem(REFPROPVARIANT propvar, ULONG iElem, LONGLONG *pnVal);
PSSTDAPI PropVariantGetUInt64Elem(REFPROPVARIANT propvar, ULONG iElem, ULONGLONG *pnVal);
PSSTDAPI PropVariantGetDoubleElem(REFPROPVARIANT propvar, ULONG iElem, DOUBLE *pnVal);
PSSTDAPI PropVariantGetFileTimeElem(REFPROPVARIANT propvar, ULONG iElem, FILETIME *pftVal);

PSSTDAPI PropVariantToStringVectorAlloc(REFPROPVARIANT propvar, PWSTR **pprgsz, ULONG *pcElem);
PSSTDAPI PropVariantToStringVector(REFPROPVARIANT propvar, PWSTR *prgsz, ULONG crgsz);

void WINAPI ClearPropVariantArray(PROPVARIANT *rgPropVar, UINT cVars);
void WINAPI ClearVariantArray(VARIANT *rgVar, UINT cVars);
PSSTDAPI_(ULONG) VariantGetElementCount(REFVARIANT var);

PSSTDAPI PSFormatForDisplay(REFPROPERTYKEY key, REFPROPVARIANT propvar,
                            PROPDESC_FORMAT_FLAGS flags, WCHAR *pszDisplay, DWORD cchDisplay);
PSSTDAPI PSFormatForDisplayAlloc(REFPROPERTYKEY key, REFPROPVARIANT propvar,
                                 PROPDESC_FORMAT_FLAGS flags, WCHAR **ppszDisplay);

PSSTDAPI InitVariantFromStrRet(STRRET *pstrret, PCUITEMID_CHILD pidl, VARIANT *pvar);
PSSTDAPI VariantToStrRet(REFVARIANT pvar, STRRET *pstrret);
PSSTDAPI VariantToStringAlloc(REFVARIANT pvar, WCHAR **ppszBuf);
PSSTDAPI VariantToFileTime(REFVARIANT pvar, PSTIME_FLAGS stfOut, FILETIME *pftOut);

PSSTDAPI VariantToBoolean(REFVARIANT pvar, BOOL *pfRet);
PSSTDAPI VariantToDouble(REFVARIANT pvar, double *pdblRet);
PSSTDAPI VariantToInt16(REFVARIANT pvar, SHORT *piRet);
PSSTDAPI VariantToInt32(REFVARIANT pvar, LONG *plRet);
PSSTDAPI VariantToInt64(REFVARIANT pvar, LONGLONG *pllRet);
PSSTDAPI VariantToUInt16(REFVARIANT pvar, USHORT *puiRet);
PSSTDAPI VariantToUInt32(REFVARIANT pvar, ULONG *pulRet);
PSSTDAPI VariantToUInt64(REFVARIANT pvar, ULONGLONG *pullRet);

PSSTDAPI_(BOOL)      VariantToBooleanWithDefault(REFVARIANT pvar, BOOL fDefault);
PSSTDAPI_(DOUBLE)    VariantToDoubleWithDefault(REFVARIANT pvar, DOUBLE dblDefault);
PSSTDAPI_(SHORT)     VariantToInt16WithDefault(REFVARIANT pvar, SHORT iDefault);
PSSTDAPI_(LONG)      VariantToInt32WithDefault(REFVARIANT pvar, LONG lDefault);
PSSTDAPI_(LONGLONG)  VariantToInt64WithDefault(REFVARIANT pvar, LONGLONG llDefault);
PSSTDAPI_(USHORT)    VariantToUInt16WithDefault(REFVARIANT pvar, USHORT uiDefault);
PSSTDAPI_(ULONG)     VariantToUInt32WithDefault(REFVARIANT pvar, ULONG ulDefault);
PSSTDAPI_(ULONGLONG) VariantToUInt64WithDefault(REFVARIANT pvar, ULONGLONG ullDefault);

#ifdef __cplusplus

HRESULT InitPropVariantFromBoolean(BOOL fVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromInt16(SHORT nVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromUInt16(USHORT uiVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromInt32(LONG lVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromUInt32(ULONG ulVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromInt64(LONGLONG llVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromUInt64(ULONGLONG ullVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromDouble(DOUBLE dblVal, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromString(PCWSTR psz, PROPVARIANT *ppropvar);
HRESULT InitPropVariantFromGUIDAsBuffer(REFGUID guid, PROPVARIANT *ppropvar);
BOOL IsPropVariantVector(REFPROPVARIANT propvar);
BOOL IsPropVariantString(REFPROPVARIANT propvar);

#ifndef NO_PROPVAR_INLINES

inline HRESULT InitPropVariantFromBoolean(BOOL fVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_BOOL;
    ppropvar->boolVal = fVal ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

inline HRESULT InitPropVariantFromInt16(SHORT nVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_I2;
    ppropvar->iVal = nVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromUInt16(USHORT uiVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_UI2;
    ppropvar->uiVal = uiVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromInt32(LONG lVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_I4;
    ppropvar->lVal = lVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromUInt32(ULONG ulVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_UI4;
    ppropvar->ulVal = ulVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromInt64(LONGLONG llVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_I8;
    ppropvar->hVal.QuadPart = llVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromUInt64(ULONGLONG ullVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_UI8;
    ppropvar->uhVal.QuadPart = ullVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromDouble(DOUBLE dblVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_R8;
    ppropvar->dblVal = dblVal;
    return S_OK;
}

inline HRESULT InitPropVariantFromString(PCWSTR psz, PROPVARIANT *ppropvar)
{
    HRESULT hres;

    hres = SHStrDupW(psz, &ppropvar->pwszVal);
    if(SUCCEEDED(hres))
        ppropvar->vt = VT_LPWSTR;
    else
        PropVariantInit(ppropvar);

    return hres;
}

inline HRESULT InitPropVariantFromGUIDAsBuffer(REFGUID guid, PROPVARIANT *ppropvar)
{
    return InitPropVariantFromBuffer(&guid, sizeof(GUID), ppropvar);
}

inline BOOL IsPropVariantVector(REFPROPVARIANT propvar)
{
    return (propvar.vt & (VT_ARRAY | VT_VECTOR));
}

inline BOOL IsPropVariantString(REFPROPVARIANT propvar)
{
    return (PropVariantToStringWithDefault(propvar, NULL) != NULL);
}

#endif /* NO_PROPVAR_INLINES */
#endif /* __cplusplus */

PSSTDAPI InitPropVariantFromStrRet(STRRET *pstrret, PCUITEMID_CHILD pidl, PROPVARIANT *ppropvar);
PSSTDAPI PropVariantToStrRet(REFPROPVARIANT propvarIn, STRRET *pstrret);

PSSTDAPI StgSerializePropVariant(const PROPVARIANT *ppropvar, SERIALIZEDPROPERTYVALUE **ppprop, ULONG *pcb);
PSSTDAPI StgDeserializePropVariant(const SERIALIZEDPROPERTYVALUE *pprop, ULONG cbmax, PROPVARIANT *ppropvar);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_PROPVARUTIL_H */
