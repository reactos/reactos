/*
 * Low level variant functions
 *
 * Copyright 2003 Jon Griffiths
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(variant);

extern HMODULE hProxyDll DECLSPEC_HIDDEN;

#define CY_MULTIPLIER   10000             /* 4 dp of precision */
#define CY_MULTIPLIER_F 10000.0
#define CY_HALF         (CY_MULTIPLIER/2) /* 0.5 */
#define CY_HALF_F       (CY_MULTIPLIER_F/2.0)

static const WCHAR szFloatFormatW[] = { '%','.','7','G','\0' };
static const WCHAR szDoubleFormatW[] = { '%','.','1','5','G','\0' };

/* Copy data from one variant to another. */
static inline void VARIANT_CopyData(const VARIANT *srcVar, VARTYPE vt, void *pOut)
{
  switch (vt)
  {
  case VT_I1:
  case VT_UI1: memcpy(pOut, &V_UI1(srcVar), sizeof(BYTE)); break;
  case VT_BOOL:
  case VT_I2:
  case VT_UI2: memcpy(pOut, &V_UI2(srcVar), sizeof(SHORT)); break;
  case VT_R4:
  case VT_INT:
  case VT_I4:
  case VT_UINT:
  case VT_UI4: memcpy(pOut, &V_UI4(srcVar), sizeof (LONG)); break;
  case VT_R8:
  case VT_DATE:
  case VT_CY:
  case VT_I8:
  case VT_UI8: memcpy(pOut, &V_UI8(srcVar), sizeof (LONG64)); break;
  case VT_INT_PTR: memcpy(pOut, &V_INT_PTR(srcVar), sizeof (INT_PTR)); break;
  case VT_DECIMAL: memcpy(pOut, &V_DECIMAL(srcVar), sizeof (DECIMAL)); break;
  case VT_BSTR: memcpy(pOut, &V_BSTR(srcVar), sizeof(BSTR)); break;
  default:
    FIXME("VT_ type %d unhandled, please report!\n", vt);
  }
}

/* Macro to inline conversion from a float or double to any integer type,
 * rounding according to the 'dutch' convention.
 */
#define VARIANT_DutchRound(typ, value, res) do { \
  double whole = value < 0 ? ceil(value) : floor(value); \
  double fract = value - whole; \
  if (fract > 0.5) res = (typ)whole + (typ)1; \
  else if (fract == 0.5) { typ is_odd = (typ)whole & 1; res = whole + is_odd; } \
  else if (fract >= 0.0) res = (typ)whole; \
  else if (fract == -0.5) { typ is_odd = (typ)whole & 1; res = whole - is_odd; } \
  else if (fract > -0.5) res = (typ)whole; \
  else res = (typ)whole - (typ)1; \
} while(0)


/* Coerce VT_BSTR to a numeric type */
static HRESULT VARIANT_NumberFromBstr(OLECHAR* pStrIn, LCID lcid, ULONG ulFlags,
                                      void* pOut, VARTYPE vt)
{
  VARIANTARG dstVar;
  HRESULT hRet;
  NUMPARSE np;
  BYTE rgb[1024];

  /* Use VarParseNumFromStr/VarNumFromParseNum as MSDN indicates */
  np.cDig = sizeof(rgb) / sizeof(BYTE);
  np.dwInFlags = NUMPRS_STD;

  hRet = VarParseNumFromStr(pStrIn, lcid, ulFlags, &np, rgb);

  if (SUCCEEDED(hRet))
  {
    /* 1 << vt gives us the VTBIT constant for the destination number type */
    hRet = VarNumFromParseNum(&np, rgb, 1 << vt, &dstVar);
    if (SUCCEEDED(hRet))
      VARIANT_CopyData(&dstVar, vt, pOut);
  }
  return hRet;
}

/* Coerce VT_DISPATCH to another type */
static HRESULT VARIANT_FromDisp(IDispatch* pdispIn, LCID lcid, void* pOut,
                                VARTYPE vt, DWORD dwFlags)
{
  static DISPPARAMS emptyParams = { NULL, NULL, 0, 0 };
  VARIANTARG srcVar, dstVar;
  HRESULT hRet;

  if (!pdispIn)
    return DISP_E_BADVARTYPE;

  /* Get the default 'value' property from the IDispatch */
  VariantInit(&srcVar);
  hRet = IDispatch_Invoke(pdispIn, DISPID_VALUE, &IID_NULL, lcid, DISPATCH_PROPERTYGET,
                          &emptyParams, &srcVar, NULL, NULL);

  if (SUCCEEDED(hRet))
  {
    /* Convert the property to the requested type */
    V_VT(&dstVar) = VT_EMPTY;
    hRet = VariantChangeTypeEx(&dstVar, &srcVar, lcid, dwFlags, vt);
    VariantClear(&srcVar);

    if (SUCCEEDED(hRet))
    {
      VARIANT_CopyData(&dstVar, vt, pOut);
      VariantClear(&srcVar);
    }
  }
  else
    hRet = DISP_E_TYPEMISMATCH;
  return hRet;
}

/* Inline return type */
#define RETTYP static inline HRESULT


/* Simple compiler cast from one type to another */
#define SIMPLE(dest, src, func) RETTYP _##func(src in, dest* out) { \
  *out = in; return S_OK; }

/* Compiler cast where input cannot be negative */
#define NEGTST(dest, src, func) RETTYP _##func(src in, dest* out) { \
  if (in < 0) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be > some number */
#define POSTST(dest, src, func, tst) RETTYP _##func(src in, dest* out) { \
  if (in > (dest)tst) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be < some number or >= some other number */
#define BOTHTST(dest, src, func, lo, hi) RETTYP _##func(src in, dest* out) { \
  if (in < (dest)lo || in > hi) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* I1 */
POSTST(signed char, BYTE, VarI1FromUI1, I1_MAX)
BOTHTST(signed char, SHORT, VarI1FromI2, I1_MIN, I1_MAX)
BOTHTST(signed char, LONG, VarI1FromI4, I1_MIN, I1_MAX)
SIMPLE(signed char, VARIANT_BOOL, VarI1FromBool)
POSTST(signed char, USHORT, VarI1FromUI2, I1_MAX)
POSTST(signed char, ULONG, VarI1FromUI4, I1_MAX)
BOTHTST(signed char, LONG64, VarI1FromI8, I1_MIN, I1_MAX)
POSTST(signed char, ULONG64, VarI1FromUI8, I1_MAX)

/* UI1 */
BOTHTST(BYTE, SHORT, VarUI1FromI2, UI1_MIN, UI1_MAX)
SIMPLE(BYTE, VARIANT_BOOL, VarUI1FromBool)
NEGTST(BYTE, signed char, VarUI1FromI1)
POSTST(BYTE, USHORT, VarUI1FromUI2, UI1_MAX)
BOTHTST(BYTE, LONG, VarUI1FromI4, UI1_MIN, UI1_MAX)
POSTST(BYTE, ULONG, VarUI1FromUI4, UI1_MAX)
BOTHTST(BYTE, LONG64, VarUI1FromI8, UI1_MIN, UI1_MAX)
POSTST(BYTE, ULONG64, VarUI1FromUI8, UI1_MAX)

/* I2 */
SIMPLE(SHORT, BYTE, VarI2FromUI1)
BOTHTST(SHORT, LONG, VarI2FromI4, I2_MIN, I2_MAX)
SIMPLE(SHORT, VARIANT_BOOL, VarI2FromBool)
SIMPLE(SHORT, signed char, VarI2FromI1)
POSTST(SHORT, USHORT, VarI2FromUI2, I2_MAX)
POSTST(SHORT, ULONG, VarI2FromUI4, I2_MAX)
BOTHTST(SHORT, LONG64, VarI2FromI8, I2_MIN, I2_MAX)
POSTST(SHORT, ULONG64, VarI2FromUI8, I2_MAX)

/* UI2 */
SIMPLE(USHORT, BYTE, VarUI2FromUI1)
NEGTST(USHORT, SHORT, VarUI2FromI2)
BOTHTST(USHORT, LONG, VarUI2FromI4, UI2_MIN, UI2_MAX)
SIMPLE(USHORT, VARIANT_BOOL, VarUI2FromBool)
NEGTST(USHORT, signed char, VarUI2FromI1)
POSTST(USHORT, ULONG, VarUI2FromUI4, UI2_MAX)
BOTHTST(USHORT, LONG64, VarUI2FromI8, UI2_MIN, UI2_MAX)
POSTST(USHORT, ULONG64, VarUI2FromUI8, UI2_MAX)

/* I4 */
SIMPLE(LONG, BYTE, VarI4FromUI1)
SIMPLE(LONG, SHORT, VarI4FromI2)
SIMPLE(LONG, VARIANT_BOOL, VarI4FromBool)
SIMPLE(LONG, signed char, VarI4FromI1)
SIMPLE(LONG, USHORT, VarI4FromUI2)
POSTST(LONG, ULONG, VarI4FromUI4, I4_MAX)
BOTHTST(LONG, LONG64, VarI4FromI8, I4_MIN, I4_MAX)
POSTST(LONG, ULONG64, VarI4FromUI8, I4_MAX)

/* UI4 */
SIMPLE(ULONG, BYTE, VarUI4FromUI1)
NEGTST(ULONG, SHORT, VarUI4FromI2)
NEGTST(ULONG, LONG, VarUI4FromI4)
SIMPLE(ULONG, VARIANT_BOOL, VarUI4FromBool)
NEGTST(ULONG, signed char, VarUI4FromI1)
SIMPLE(ULONG, USHORT, VarUI4FromUI2)
BOTHTST(ULONG, LONG64, VarUI4FromI8, UI4_MIN, UI4_MAX)
POSTST(ULONG, ULONG64, VarUI4FromUI8, UI4_MAX)

/* I8 */
SIMPLE(LONG64, BYTE, VarI8FromUI1)
SIMPLE(LONG64, SHORT, VarI8FromI2)
SIMPLE(LONG64, signed char, VarI8FromI1)
SIMPLE(LONG64, USHORT, VarI8FromUI2)
SIMPLE(LONG64, ULONG, VarI8FromUI4)
POSTST(LONG64, ULONG64, VarI8FromUI8, I8_MAX)

/* UI8 */
SIMPLE(ULONG64, BYTE, VarUI8FromUI1)
NEGTST(ULONG64, SHORT, VarUI8FromI2)
NEGTST(ULONG64, signed char, VarUI8FromI1)
SIMPLE(ULONG64, USHORT, VarUI8FromUI2)
SIMPLE(ULONG64, ULONG, VarUI8FromUI4)
NEGTST(ULONG64, LONG64, VarUI8FromI8)

/* R4 (float) */
SIMPLE(float, BYTE, VarR4FromUI1)
SIMPLE(float, SHORT, VarR4FromI2)
SIMPLE(float, signed char, VarR4FromI1)
SIMPLE(float, USHORT, VarR4FromUI2)
SIMPLE(float, LONG, VarR4FromI4)
SIMPLE(float, ULONG, VarR4FromUI4)
SIMPLE(float, LONG64, VarR4FromI8)
SIMPLE(float, ULONG64, VarR4FromUI8)

/* R8 (double) */
SIMPLE(double, BYTE, VarR8FromUI1)
SIMPLE(double, SHORT, VarR8FromI2)
SIMPLE(double, float, VarR8FromR4)
RETTYP _VarR8FromCy(CY i, double* o) { *o = (double)i.int64 / CY_MULTIPLIER_F; return S_OK; }
SIMPLE(double, DATE, VarR8FromDate)
SIMPLE(double, signed char, VarR8FromI1)
SIMPLE(double, USHORT, VarR8FromUI2)
SIMPLE(double, LONG, VarR8FromI4)
SIMPLE(double, ULONG, VarR8FromUI4)
SIMPLE(double, LONG64, VarR8FromI8)
SIMPLE(double, ULONG64, VarR8FromUI8)


/* I1
 */

/************************************************************************
 * VarI1FromUI1 (OLEAUT32.244)
 *
 * Convert a VT_UI1 to a VT_I1.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromUI1(BYTE bIn, signed char* pcOut)
{
  return _VarI1FromUI1(bIn, pcOut);
}

/************************************************************************
 * VarI1FromI2 (OLEAUT32.245)
 *
 * Convert a VT_I2 to a VT_I1.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromI2(SHORT sIn, signed char* pcOut)
{
  return _VarI1FromI2(sIn, pcOut);
}

/************************************************************************
 * VarI1FromI4 (OLEAUT32.246)
 *
 * Convert a VT_I4 to a VT_I1.
 *
 * PARAMS
 *  iIn     [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromI4(LONG iIn, signed char* pcOut)
{
  return _VarI1FromI4(iIn, pcOut);
}

/************************************************************************
 * VarI1FromR4 (OLEAUT32.247)
 *
 * Convert a VT_R4 to a VT_I1.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromR4(FLOAT fltIn, signed char* pcOut)
{
  return VarI1FromR8(fltIn, pcOut);
}

/************************************************************************
 * VarI1FromR8 (OLEAUT32.248)
 *
 * Convert a VT_R8 to a VT_I1.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarI1FromR8(double dblIn, signed char* pcOut)
{
  if (dblIn < I1_MIN - 0.5 || dblIn >= I1_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(CHAR, dblIn, *pcOut);
  return S_OK;
}

/************************************************************************
 * VarI1FromDate (OLEAUT32.249)
 *
 * Convert a VT_DATE to a VT_I1.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromDate(DATE dateIn, signed char* pcOut)
{
  return VarI1FromR8(dateIn, pcOut);
}

/************************************************************************
 * VarI1FromCy (OLEAUT32.250)
 *
 * Convert a VT_CY to a VT_I1.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromCy(CY cyIn, signed char* pcOut)
{
  LONG i = I1_MAX + 1;

  VarI4FromCy(cyIn, &i);
  return _VarI1FromI4(i, pcOut);
}

/************************************************************************
 * VarI1FromStr (OLEAUT32.251)
 *
 * Convert a VT_BSTR to a VT_I1.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, signed char* pcOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pcOut, VT_I1);
}

/************************************************************************
 * VarI1FromDisp (OLEAUT32.252)
 *
 * Convert a VT_DISPATCH to a VT_I1.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pcOut    [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI1FromDisp(IDispatch* pdispIn, LCID lcid, signed char* pcOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pcOut, VT_I1, 0);
}

/************************************************************************
 * VarI1FromBool (OLEAUT32.253)
 *
 * Convert a VT_BOOL to a VT_I1.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI1FromBool(VARIANT_BOOL boolIn, signed char* pcOut)
{
  return _VarI1FromBool(boolIn, pcOut);
}

/************************************************************************
 * VarI1FromUI2 (OLEAUT32.254)
 *
 * Convert a VT_UI2 to a VT_I1.
 *
 * PARAMS
 *  usIn    [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromUI2(USHORT usIn, signed char* pcOut)
{
  return _VarI1FromUI2(usIn, pcOut);
}

/************************************************************************
 * VarI1FromUI4 (OLEAUT32.255)
 *
 * Convert a VT_UI4 to a VT_I1.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI1FromUI4(ULONG ulIn, signed char* pcOut)
{
  return _VarI1FromUI4(ulIn, pcOut);
}

/************************************************************************
 * VarI1FromDec (OLEAUT32.256)
 *
 * Convert a VT_DECIMAL to a VT_I1.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromDec(DECIMAL *pdecIn, signed char* pcOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI1FromI8(i64, pcOut);
  return hRet;
}

/************************************************************************
 * VarI1FromI8 (OLEAUT32.376)
 *
 * Convert a VT_I8 to a VT_I1.
 *
 * PARAMS
 *  llIn  [I] Source
 *  pcOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromI8(LONG64 llIn, signed char* pcOut)
{
  return _VarI1FromI8(llIn, pcOut);
}

/************************************************************************
 * VarI1FromUI8 (OLEAUT32.377)
 *
 * Convert a VT_UI8 to a VT_I1.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pcOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI1FromUI8(ULONG64 ullIn, signed char* pcOut)
{
  return _VarI1FromUI8(ullIn, pcOut);
}

/* UI1
 */

/************************************************************************
 * VarUI1FromI2 (OLEAUT32.130)
 *
 * Convert a VT_I2 to a VT_UI1.
 *
 * PARAMS
 *  sIn   [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromI2(SHORT sIn, BYTE* pbOut)
{
  return _VarUI1FromI2(sIn, pbOut);
}

/************************************************************************
 * VarUI1FromI4 (OLEAUT32.131)
 *
 * Convert a VT_I4 to a VT_UI1.
 *
 * PARAMS
 *  iIn   [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromI4(LONG iIn, BYTE* pbOut)
{
  return _VarUI1FromI4(iIn, pbOut);
}

/************************************************************************
 * VarUI1FromR4 (OLEAUT32.132)
 *
 * Convert a VT_R4 to a VT_UI1.
 *
 * PARAMS
 *  fltIn [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI1FromR4(FLOAT fltIn, BYTE* pbOut)
{
  return VarUI1FromR8(fltIn, pbOut);
}

/************************************************************************
 * VarUI1FromR8 (OLEAUT32.133)
 *
 * Convert a VT_R8 to a VT_UI1.
 *
 * PARAMS
 *  dblIn [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarUI1FromR8(double dblIn, BYTE* pbOut)
{
  if (dblIn < -0.5 || dblIn >= UI1_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(BYTE, dblIn, *pbOut);
  return S_OK;
}

/************************************************************************
 * VarUI1FromCy (OLEAUT32.134)
 *
 * Convert a VT_CY to a VT_UI1.
 *
 * PARAMS
 *  cyIn     [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  Negative values >= -5000 will be converted to 0.
 */
HRESULT WINAPI VarUI1FromCy(CY cyIn, BYTE* pbOut)
{
  ULONG i = UI1_MAX + 1;

  VarUI4FromCy(cyIn, &i);
  return _VarUI1FromUI4(i, pbOut);
}

/************************************************************************
 * VarUI1FromDate (OLEAUT32.135)
 *
 * Convert a VT_DATE to a VT_UI1.
 *
 * PARAMS
 *  dateIn [I] Source
 *  pbOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromDate(DATE dateIn, BYTE* pbOut)
{
  return VarUI1FromR8(dateIn, pbOut);
}

/************************************************************************
 * VarUI1FromStr (OLEAUT32.136)
 *
 * Convert a VT_BSTR to a VT_UI1.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, BYTE* pbOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pbOut, VT_UI1);
}

/************************************************************************
 * VarUI1FromDisp (OLEAUT32.137)
 *
 * Convert a VT_DISPATCH to a VT_UI1.
 *
 * PARAMS
 *  pdispIn [I] Source
 *  lcid    [I] LCID for conversion
 *  pbOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI1FromDisp(IDispatch* pdispIn, LCID lcid, BYTE* pbOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pbOut, VT_UI1, 0);
}

/************************************************************************
 * VarUI1FromBool (OLEAUT32.138)
 *
 * Convert a VT_BOOL to a VT_UI1.
 *
 * PARAMS
 *  boolIn [I] Source
 *  pbOut  [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL boolIn, BYTE* pbOut)
{
  return _VarUI1FromBool(boolIn, pbOut);
}

/************************************************************************
 * VarUI1FromI1 (OLEAUT32.237)
 *
 * Convert a VT_I1 to a VT_UI1.
 *
 * PARAMS
 *  cIn   [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromI1(signed char cIn, BYTE* pbOut)
{
  return _VarUI1FromI1(cIn, pbOut);
}

/************************************************************************
 * VarUI1FromUI2 (OLEAUT32.238)
 *
 * Convert a VT_UI2 to a VT_UI1.
 *
 * PARAMS
 *  usIn  [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromUI2(USHORT usIn, BYTE* pbOut)
{
  return _VarUI1FromUI2(usIn, pbOut);
}

/************************************************************************
 * VarUI1FromUI4 (OLEAUT32.239)
 *
 * Convert a VT_UI4 to a VT_UI1.
 *
 * PARAMS
 *  ulIn  [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromUI4(ULONG ulIn, BYTE* pbOut)
{
  return _VarUI1FromUI4(ulIn, pbOut);
}

/************************************************************************
 * VarUI1FromDec (OLEAUT32.240)
 *
 * Convert a VT_DECIMAL to a VT_UI1.
 *
 * PARAMS
 *  pDecIn [I] Source
 *  pbOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromDec(DECIMAL *pdecIn, BYTE* pbOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI1FromI8(i64, pbOut);
  return hRet;
}

/************************************************************************
 * VarUI1FromI8 (OLEAUT32.372)
 *
 * Convert a VT_I8 to a VT_UI1.
 *
 * PARAMS
 *  llIn  [I] Source
 *  pbOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromI8(LONG64 llIn, BYTE* pbOut)
{
  return _VarUI1FromI8(llIn, pbOut);
}

/************************************************************************
 * VarUI1FromUI8 (OLEAUT32.373)
 *
 * Convert a VT_UI8 to a VT_UI1.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pbOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI1FromUI8(ULONG64 ullIn, BYTE* pbOut)
{
  return _VarUI1FromUI8(ullIn, pbOut);
}


/* I2
 */

/************************************************************************
 * VarI2FromUI1 (OLEAUT32.48)
 *
 * Convert a VT_UI2 to a VT_I2.
 *
 * PARAMS
 *  bIn     [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI2FromUI1(BYTE bIn, SHORT* psOut)
{
  return _VarI2FromUI1(bIn, psOut);
}

/************************************************************************
 * VarI2FromI4 (OLEAUT32.49)
 *
 * Convert a VT_I4 to a VT_I2.
 *
 * PARAMS
 *  iIn     [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromI4(LONG iIn, SHORT* psOut)
{
  return _VarI2FromI4(iIn, psOut);
}

/************************************************************************
 * VarI2FromR4 (OLEAUT32.50)
 *
 * Convert a VT_R4 to a VT_I2.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromR4(FLOAT fltIn, SHORT* psOut)
{
  return VarI2FromR8(fltIn, psOut);
}

/************************************************************************
 * VarI2FromR8 (OLEAUT32.51)
 *
 * Convert a VT_R8 to a VT_I2.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarI2FromR8(double dblIn, SHORT* psOut)
{
  if (dblIn < I2_MIN - 0.5 || dblIn >= I2_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(SHORT, dblIn, *psOut);
  return S_OK;
}

/************************************************************************
 * VarI2FromCy (OLEAUT32.52)
 *
 * Convert a VT_CY to a VT_I2.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromCy(CY cyIn, SHORT* psOut)
{
  LONG i = I2_MAX + 1;

  VarI4FromCy(cyIn, &i);
  return _VarI2FromI4(i, psOut);
}

/************************************************************************
 * VarI2FromDate (OLEAUT32.53)
 *
 * Convert a VT_DATE to a VT_I2.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromDate(DATE dateIn, SHORT* psOut)
{
  return VarI2FromR8(dateIn, psOut);
}

/************************************************************************
 * VarI2FromStr (OLEAUT32.54)
 *
 * Convert a VT_BSTR to a VT_I2.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if any parameter is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, SHORT* psOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, psOut, VT_I2);
}

/************************************************************************
 * VarI2FromDisp (OLEAUT32.55)
 *
 * Convert a VT_DISPATCH to a VT_I2.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  psOut    [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pdispIn is invalid,
 *           DISP_E_OVERFLOW, if the value will not fit in the destination,
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI2FromDisp(IDispatch* pdispIn, LCID lcid, SHORT* psOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, psOut, VT_I2, 0);
}

/************************************************************************
 * VarI2FromBool (OLEAUT32.56)
 *
 * Convert a VT_BOOL to a VT_I2.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL boolIn, SHORT* psOut)
{
  return _VarI2FromBool(boolIn, psOut);
}

/************************************************************************
 * VarI2FromI1 (OLEAUT32.205)
 *
 * Convert a VT_I1 to a VT_I2.
 *
 * PARAMS
 *  cIn     [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI2FromI1(signed char cIn, SHORT* psOut)
{
  return _VarI2FromI1(cIn, psOut);
}

/************************************************************************
 * VarI2FromUI2 (OLEAUT32.206)
 *
 * Convert a VT_UI2 to a VT_I2.
 *
 * PARAMS
 *  usIn    [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromUI2(USHORT usIn, SHORT* psOut)
{
  return _VarI2FromUI2(usIn, psOut);
}

/************************************************************************
 * VarI2FromUI4 (OLEAUT32.207)
 *
 * Convert a VT_UI4 to a VT_I2.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromUI4(ULONG ulIn, SHORT* psOut)
{
  return _VarI2FromUI4(ulIn, psOut);
}

/************************************************************************
 * VarI2FromDec (OLEAUT32.208)
 *
 * Convert a VT_DECIMAL to a VT_I2.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  psOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromDec(DECIMAL *pdecIn, SHORT* psOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI2FromI8(i64, psOut);
  return hRet;
}

/************************************************************************
 * VarI2FromI8 (OLEAUT32.346)
 *
 * Convert a VT_I8 to a VT_I2.
 *
 * PARAMS
 *  llIn  [I] Source
 *  psOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromI8(LONG64 llIn, SHORT* psOut)
{
  return _VarI2FromI8(llIn, psOut);
}

/************************************************************************
 * VarI2FromUI8 (OLEAUT32.347)
 *
 * Convert a VT_UI8 to a VT_I2.
 *
 * PARAMS
 *  ullIn [I] Source
 *  psOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI2FromUI8(ULONG64 ullIn, SHORT* psOut)
{
  return _VarI2FromUI8(ullIn, psOut);
}

/* UI2
 */

/************************************************************************
 * VarUI2FromUI1 (OLEAUT32.257)
 *
 * Convert a VT_UI1 to a VT_UI2.
 *
 * PARAMS
 *  bIn    [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI2FromUI1(BYTE bIn, USHORT* pusOut)
{
  return _VarUI2FromUI1(bIn, pusOut);
}

/************************************************************************
 * VarUI2FromI2 (OLEAUT32.258)
 *
 * Convert a VT_I2 to a VT_UI2.
 *
 * PARAMS
 *  sIn    [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromI2(SHORT sIn, USHORT* pusOut)
{
  return _VarUI2FromI2(sIn, pusOut);
}

/************************************************************************
 * VarUI2FromI4 (OLEAUT32.259)
 *
 * Convert a VT_I4 to a VT_UI2.
 *
 * PARAMS
 *  iIn    [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromI4(LONG iIn, USHORT* pusOut)
{
  return _VarUI2FromI4(iIn, pusOut);
}

/************************************************************************
 * VarUI2FromR4 (OLEAUT32.260)
 *
 * Convert a VT_R4 to a VT_UI2.
 *
 * PARAMS
 *  fltIn  [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromR4(FLOAT fltIn, USHORT* pusOut)
{
  return VarUI2FromR8(fltIn, pusOut);
}

/************************************************************************
 * VarUI2FromR8 (OLEAUT32.261)
 *
 * Convert a VT_R8 to a VT_UI2.
 *
 * PARAMS
 *  dblIn  [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarUI2FromR8(double dblIn, USHORT* pusOut)
{
  if (dblIn < -0.5 || dblIn >= UI2_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(USHORT, dblIn, *pusOut);
  return S_OK;
}

/************************************************************************
 * VarUI2FromDate (OLEAUT32.262)
 *
 * Convert a VT_DATE to a VT_UI2.
 *
 * PARAMS
 *  dateIn [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromDate(DATE dateIn, USHORT* pusOut)
{
  return VarUI2FromR8(dateIn, pusOut);
}

/************************************************************************
 * VarUI2FromCy (OLEAUT32.263)
 *
 * Convert a VT_CY to a VT_UI2.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  Negative values >= -5000 will be converted to 0.
 */
HRESULT WINAPI VarUI2FromCy(CY cyIn, USHORT* pusOut)
{
  ULONG i = UI2_MAX + 1;

  VarUI4FromCy(cyIn, &i);
  return _VarUI2FromUI4(i, pusOut);
}

/************************************************************************
 * VarUI2FromStr (OLEAUT32.264)
 *
 * Convert a VT_BSTR to a VT_UI2.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pusOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, USHORT* pusOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pusOut, VT_UI2);
}

/************************************************************************
 * VarUI2FromDisp (OLEAUT32.265)
 *
 * Convert a VT_DISPATCH to a VT_UI2.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pusOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI2FromDisp(IDispatch* pdispIn, LCID lcid, USHORT* pusOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pusOut, VT_UI2, 0);
}

/************************************************************************
 * VarUI2FromBool (OLEAUT32.266)
 *
 * Convert a VT_BOOL to a VT_UI2.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pusOut  [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL boolIn, USHORT* pusOut)
{
  return _VarUI2FromBool(boolIn, pusOut);
}

/************************************************************************
 * VarUI2FromI1 (OLEAUT32.267)
 *
 * Convert a VT_I1 to a VT_UI2.
 *
 * PARAMS
 *  cIn    [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromI1(signed char cIn, USHORT* pusOut)
{
  return _VarUI2FromI1(cIn, pusOut);
}

/************************************************************************
 * VarUI2FromUI4 (OLEAUT32.268)
 *
 * Convert a VT_UI4 to a VT_UI2.
 *
 * PARAMS
 *  ulIn   [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromUI4(ULONG ulIn, USHORT* pusOut)
{
  return _VarUI2FromUI4(ulIn, pusOut);
}

/************************************************************************
 * VarUI2FromDec (OLEAUT32.269)
 *
 * Convert a VT_DECIMAL to a VT_UI2.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pusOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromDec(DECIMAL *pdecIn, USHORT* pusOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI2FromI8(i64, pusOut);
  return hRet;
}

/************************************************************************
 * VarUI2FromI8 (OLEAUT32.378)
 *
 * Convert a VT_I8 to a VT_UI2.
 *
 * PARAMS
 *  llIn   [I] Source
 *  pusOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromI8(LONG64 llIn, USHORT* pusOut)
{
  return _VarUI2FromI8(llIn, pusOut);
}

/************************************************************************
 * VarUI2FromUI8 (OLEAUT32.379)
 *
 * Convert a VT_UI8 to a VT_UI2.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  pusOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI2FromUI8(ULONG64 ullIn, USHORT* pusOut)
{
  return _VarUI2FromUI8(ullIn, pusOut);
}

/* I4
 */

/************************************************************************
 * VarI4FromUI1 (OLEAUT32.58)
 *
 * Convert a VT_UI1 to a VT_I4.
 *
 * PARAMS
 *  bIn     [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI4FromUI1(BYTE bIn, LONG *piOut)
{
  return _VarI4FromUI1(bIn, piOut);
}

/************************************************************************
 * VarI4FromI2 (OLEAUT32.59)
 *
 * Convert a VT_I2 to a VT_I4.
 *
 * PARAMS
 *  sIn     [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromI2(SHORT sIn, LONG *piOut)
{
  return _VarI4FromI2(sIn, piOut);
}

/************************************************************************
 * VarI4FromR4 (OLEAUT32.60)
 *
 * Convert a VT_R4 to a VT_I4.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromR4(FLOAT fltIn, LONG *piOut)
{
  return VarI4FromR8(fltIn, piOut);
}

/************************************************************************
 * VarI4FromR8 (OLEAUT32.61)
 *
 * Convert a VT_R8 to a VT_I4.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarI4FromR8(double dblIn, LONG *piOut)
{
  if (dblIn < I4_MIN - 0.5 || dblIn >= I4_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(LONG, dblIn, *piOut);
  return S_OK;
}

/************************************************************************
 * VarI4FromCy (OLEAUT32.62)
 *
 * Convert a VT_CY to a VT_I4.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromCy(CY cyIn, LONG *piOut)
{
  double d = cyIn.int64 / CY_MULTIPLIER_F;
  return VarI4FromR8(d, piOut);
}

/************************************************************************
 * VarI4FromDate (OLEAUT32.63)
 *
 * Convert a VT_DATE to a VT_I4.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromDate(DATE dateIn, LONG *piOut)
{
  return VarI4FromR8(dateIn, piOut);
}

/************************************************************************
 * VarI4FromStr (OLEAUT32.64)
 *
 * Convert a VT_BSTR to a VT_I4.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if any parameter is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if strIn cannot be converted
 */
HRESULT WINAPI VarI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, LONG *piOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, piOut, VT_I4);
}

/************************************************************************
 * VarI4FromDisp (OLEAUT32.65)
 *
 * Convert a VT_DISPATCH to a VT_I4.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  piOut    [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI4FromDisp(IDispatch* pdispIn, LCID lcid, LONG *piOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, piOut, VT_I4, 0);
}

/************************************************************************
 * VarI4FromBool (OLEAUT32.66)
 *
 * Convert a VT_BOOL to a VT_I4.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL boolIn, LONG *piOut)
{
  return _VarI4FromBool(boolIn, piOut);
}

/************************************************************************
 * VarI4FromI1 (OLEAUT32.209)
 *
 * Convert a VT_I1 to a VT_I4.
 *
 * PARAMS
 *  cIn     [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI4FromI1(signed char cIn, LONG *piOut)
{
  return _VarI4FromI1(cIn, piOut);
}

/************************************************************************
 * VarI4FromUI2 (OLEAUT32.210)
 *
 * Convert a VT_UI2 to a VT_I4.
 *
 * PARAMS
 *  usIn    [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI4FromUI2(USHORT usIn, LONG *piOut)
{
  return _VarI4FromUI2(usIn, piOut);
}

/************************************************************************
 * VarI4FromUI4 (OLEAUT32.211)
 *
 * Convert a VT_UI4 to a VT_I4.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromUI4(ULONG ulIn, LONG *piOut)
{
  return _VarI4FromUI4(ulIn, piOut);
}

/************************************************************************
 * VarI4FromDec (OLEAUT32.212)
 *
 * Convert a VT_DECIMAL to a VT_I4.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  piOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pdecIn is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromDec(DECIMAL *pdecIn, LONG *piOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI4FromI8(i64, piOut);
  return hRet;
}

/************************************************************************
 * VarI4FromI8 (OLEAUT32.348)
 *
 * Convert a VT_I8 to a VT_I4.
 *
 * PARAMS
 *  llIn  [I] Source
 *  piOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromI8(LONG64 llIn, LONG *piOut)
{
  return _VarI4FromI8(llIn, piOut);
}

/************************************************************************
 * VarI4FromUI8 (OLEAUT32.349)
 *
 * Convert a VT_UI8 to a VT_I4.
 *
 * PARAMS
 *  ullIn [I] Source
 *  piOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI4FromUI8(ULONG64 ullIn, LONG *piOut)
{
  return _VarI4FromUI8(ullIn, piOut);
}

/* UI4
 */

/************************************************************************
 * VarUI4FromUI1 (OLEAUT32.270)
 *
 * Convert a VT_UI1 to a VT_UI4.
 *
 * PARAMS
 *  bIn    [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI4FromUI1(BYTE bIn, ULONG *pulOut)
{
  return _VarUI4FromUI1(bIn, pulOut);
}

/************************************************************************
 * VarUI4FromI2 (OLEAUT32.271)
 *
 * Convert a VT_I2 to a VT_UI4.
 *
 * PARAMS
 *  sIn    [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromI2(SHORT sIn, ULONG *pulOut)
{
  return _VarUI4FromI2(sIn, pulOut);
}

/************************************************************************
 * VarUI4FromI4 (OLEAUT32.272)
 *
 * Convert a VT_I4 to a VT_UI4.
 *
 * PARAMS
 *  iIn    [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromI4(LONG iIn, ULONG *pulOut)
{
  return _VarUI4FromI4(iIn, pulOut);
}

/************************************************************************
 * VarUI4FromR4 (OLEAUT32.273)
 *
 * Convert a VT_R4 to a VT_UI4.
 *
 * PARAMS
 *  fltIn  [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromR4(FLOAT fltIn, ULONG *pulOut)
{
  return VarUI4FromR8(fltIn, pulOut);
}

/************************************************************************
 * VarUI4FromR8 (OLEAUT32.274)
 *
 * Convert a VT_R8 to a VT_UI4.
 *
 * PARAMS
 *  dblIn  [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarUI4FromR8(double dblIn, ULONG *pulOut)
{
  if (dblIn < -0.5 || dblIn >= UI4_MAX + 0.5)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(ULONG, dblIn, *pulOut);
  return S_OK;
}

/************************************************************************
 * VarUI4FromDate (OLEAUT32.275)
 *
 * Convert a VT_DATE to a VT_UI4.
 *
 * PARAMS
 *  dateIn [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromDate(DATE dateIn, ULONG *pulOut)
{
  return VarUI4FromR8(dateIn, pulOut);
}

/************************************************************************
 * VarUI4FromCy (OLEAUT32.276)
 *
 * Convert a VT_CY to a VT_UI4.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromCy(CY cyIn, ULONG *pulOut)
{
  double d = cyIn.int64 / CY_MULTIPLIER_F;
  return VarUI4FromR8(d, pulOut);
}

/************************************************************************
 * VarUI4FromStr (OLEAUT32.277)
 *
 * Convert a VT_BSTR to a VT_UI4.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pulOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if any parameter is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if strIn cannot be converted
 */
HRESULT WINAPI VarUI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, ULONG *pulOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pulOut, VT_UI4);
}

/************************************************************************
 * VarUI4FromDisp (OLEAUT32.278)
 *
 * Convert a VT_DISPATCH to a VT_UI4.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pulOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI4FromDisp(IDispatch* pdispIn, LCID lcid, ULONG *pulOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pulOut, VT_UI4, 0);
}

/************************************************************************
 * VarUI4FromBool (OLEAUT32.279)
 *
 * Convert a VT_BOOL to a VT_UI4.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pulOut  [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL boolIn, ULONG *pulOut)
{
  return _VarUI4FromBool(boolIn, pulOut);
}

/************************************************************************
 * VarUI4FromI1 (OLEAUT32.280)
 *
 * Convert a VT_I1 to a VT_UI4.
 *
 * PARAMS
 *  cIn    [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromI1(signed char cIn, ULONG *pulOut)
{
  return _VarUI4FromI1(cIn, pulOut);
}

/************************************************************************
 * VarUI4FromUI2 (OLEAUT32.281)
 *
 * Convert a VT_UI2 to a VT_UI4.
 *
 * PARAMS
 *  usIn   [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI4FromUI2(USHORT usIn, ULONG *pulOut)
{
  return _VarUI4FromUI2(usIn, pulOut);
}

/************************************************************************
 * VarUI4FromDec (OLEAUT32.282)
 *
 * Convert a VT_DECIMAL to a VT_UI4.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pulOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pdecIn is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromDec(DECIMAL *pdecIn, ULONG *pulOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI4FromI8(i64, pulOut);
  return hRet;
}

/************************************************************************
 * VarUI4FromI8 (OLEAUT32.425)
 *
 * Convert a VT_I8 to a VT_UI4.
 *
 * PARAMS
 *  llIn   [I] Source
 *  pulOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromI8(LONG64 llIn, ULONG *pulOut)
{
  return _VarUI4FromI8(llIn, pulOut);
}

/************************************************************************
 * VarUI4FromUI8 (OLEAUT32.426)
 *
 * Convert a VT_UI8 to a VT_UI4.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  pulOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI4FromUI8(ULONG64 ullIn, ULONG *pulOut)
{
  return _VarUI4FromUI8(ullIn, pulOut);
}

/* I8
 */

/************************************************************************
 * VarI8FromUI1 (OLEAUT32.333)
 *
 * Convert a VT_UI1 to a VT_I8.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromUI1(BYTE bIn, LONG64* pi64Out)
{
  return _VarI8FromUI1(bIn, pi64Out);
}


/************************************************************************
 * VarI8FromI2 (OLEAUT32.334)
 *
 * Convert a VT_I2 to a VT_I8.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromI2(SHORT sIn, LONG64* pi64Out)
{
  return _VarI8FromI2(sIn, pi64Out);
}

/************************************************************************
 * VarI8FromR4 (OLEAUT32.335)
 *
 * Convert a VT_R4 to a VT_I8.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI8FromR4(FLOAT fltIn, LONG64* pi64Out)
{
  return VarI8FromR8(fltIn, pi64Out);
}

/************************************************************************
 * VarI8FromR8 (OLEAUT32.336)
 *
 * Convert a VT_R8 to a VT_I8.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  Only values that fit into 63 bits are accepted. Due to rounding issues,
 *  very high or low values will not be accurately converted.
 *
 *  Numbers are rounded using Dutch rounding, as follows:
 *
 *|  Fractional Part   Sign  Direction  Example
 *|  ---------------   ----  ---------  -------
 *|  < 0.5              +    Down        0.4 ->  0.0
 *|  < 0.5              -    Up         -0.4 ->  0.0
 *|  > 0.5              +    Up          0.6 ->  1.0
 *|  < 0.5              -    Up         -0.6 -> -1.0
 *|  = 0.5              +    Up/Down    Down if even, Up if odd
 *|  = 0.5              -    Up/Down    Up if even, Down if odd
 *
 *  This system is often used in supermarkets.
 */
HRESULT WINAPI VarI8FromR8(double dblIn, LONG64* pi64Out)
{
  if ( dblIn < -4611686018427387904.0 || dblIn >= 4611686018427387904.0)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(LONG64, dblIn, *pi64Out);
  return S_OK;
}

/************************************************************************
 * VarI8FromCy (OLEAUT32.337)
 *
 * Convert a VT_CY to a VT_I8.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 *
 * NOTES
 *  All negative numbers are rounded down by 1, including those that are
 *  evenly divisible by 10000 (this is a Win32 bug that Wine mimics).
 *  Positive numbers are rounded using Dutch rounding: See VarI8FromR8()
 *  for details.
 */
HRESULT WINAPI VarI8FromCy(CY cyIn, LONG64* pi64Out)
{
  *pi64Out = cyIn.int64 / CY_MULTIPLIER;

  if (cyIn.int64 < 0)
    (*pi64Out)--; /* Mimic Win32 bug */
  else
  {
    cyIn.int64 -= *pi64Out * CY_MULTIPLIER; /* cyIn.s.Lo now holds fractional remainder */

    if (cyIn.s.Lo > CY_HALF || (cyIn.s.Lo == CY_HALF && (*pi64Out & 0x1)))
      (*pi64Out)++;
  }
  return S_OK;
}

/************************************************************************
 * VarI8FromDate (OLEAUT32.338)
 *
 * Convert a VT_DATE to a VT_I8.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI8FromDate(DATE dateIn, LONG64* pi64Out)
{
  return VarI8FromR8(dateIn, pi64Out);
}

/************************************************************************
 * VarI8FromStr (OLEAUT32.339)
 *
 * Convert a VT_BSTR to a VT_I8.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI8FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, LONG64* pi64Out)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pi64Out, VT_I8);
}

/************************************************************************
 * VarI8FromDisp (OLEAUT32.340)
 *
 * Convert a VT_DISPATCH to a VT_I8.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pi64Out  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarI8FromDisp(IDispatch* pdispIn, LCID lcid, LONG64* pi64Out)
{
  return VARIANT_FromDisp(pdispIn, lcid, pi64Out, VT_I8, 0);
}

/************************************************************************
 * VarI8FromBool (OLEAUT32.341)
 *
 * Convert a VT_BOOL to a VT_I8.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromBool(VARIANT_BOOL boolIn, LONG64* pi64Out)
{
  return VarI8FromI2(boolIn, pi64Out);
}

/************************************************************************
 * VarI8FromI1 (OLEAUT32.342)
 *
 * Convert a VT_I1 to a VT_I8.
 *
 * PARAMS
 *  cIn     [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromI1(signed char cIn, LONG64* pi64Out)
{
  return _VarI8FromI1(cIn, pi64Out);
}

/************************************************************************
 * VarI8FromUI2 (OLEAUT32.343)
 *
 * Convert a VT_UI2 to a VT_I8.
 *
 * PARAMS
 *  usIn    [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromUI2(USHORT usIn, LONG64* pi64Out)
{
  return _VarI8FromUI2(usIn, pi64Out);
}

/************************************************************************
 * VarI8FromUI4 (OLEAUT32.344)
 *
 * Convert a VT_UI4 to a VT_I8.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarI8FromUI4(ULONG ulIn, LONG64* pi64Out)
{
  return _VarI8FromUI4(ulIn, pi64Out);
}

/************************************************************************
 * VarI8FromDec (OLEAUT32.345)
 *
 * Convert a VT_DECIMAL to a VT_I8.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI8FromDec(DECIMAL *pdecIn, LONG64* pi64Out)
{
  if (!DEC_SCALE(pdecIn))
  {
    /* This decimal is just a 96 bit integer */
    if (DEC_SIGN(pdecIn) & ~DECIMAL_NEG)
      return E_INVALIDARG;

    if (DEC_HI32(pdecIn) || DEC_MID32(pdecIn) & 0x80000000)
      return DISP_E_OVERFLOW;

    if (DEC_SIGN(pdecIn))
      *pi64Out = -DEC_LO64(pdecIn);
    else
      *pi64Out = DEC_LO64(pdecIn);
    return S_OK;
  }
  else
  {
    /* Decimal contains a floating point number */
    HRESULT hRet;
    double dbl;

    hRet = VarR8FromDec(pdecIn, &dbl);
    if (SUCCEEDED(hRet))
      hRet = VarI8FromR8(dbl, pi64Out);
    return hRet;
  }
}

/************************************************************************
 * VarI8FromUI8 (OLEAUT32.427)
 *
 * Convert a VT_UI8 to a VT_I8.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pi64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarI8FromUI8(ULONG64 ullIn, LONG64* pi64Out)
{
  return _VarI8FromUI8(ullIn, pi64Out);
}

/* UI8
 */

/************************************************************************
 * VarUI8FromI8 (OLEAUT32.428)
 *
 * Convert a VT_I8 to a VT_UI8.
 *
 * PARAMS
 *  ulIn     [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI8FromI8(LONG64 llIn, ULONG64* pui64Out)
{
  return _VarUI8FromI8(llIn, pui64Out);
}

/************************************************************************
 * VarUI8FromUI1 (OLEAUT32.429)
 *
 * Convert a VT_UI1 to a VT_UI8.
 *
 * PARAMS
 *  bIn      [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI8FromUI1(BYTE bIn, ULONG64* pui64Out)
{
  return _VarUI8FromUI1(bIn, pui64Out);
}

/************************************************************************
 * VarUI8FromI2 (OLEAUT32.430)
 *
 * Convert a VT_I2 to a VT_UI8.
 *
 * PARAMS
 *  sIn      [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI8FromI2(SHORT sIn, ULONG64* pui64Out)
{
  return _VarUI8FromI2(sIn, pui64Out);
}

/************************************************************************
 * VarUI8FromR4 (OLEAUT32.431)
 *
 * Convert a VT_R4 to a VT_UI8.
 *
 * PARAMS
 *  fltIn    [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI8FromR4(FLOAT fltIn, ULONG64* pui64Out)
{
  return VarUI8FromR8(fltIn, pui64Out);
}

/************************************************************************
 * VarUI8FromR8 (OLEAUT32.432)
 *
 * Convert a VT_R8 to a VT_UI8.
 *
 * PARAMS
 *  dblIn    [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  See VarI8FromR8() for details concerning rounding.
 */
HRESULT WINAPI VarUI8FromR8(double dblIn, ULONG64* pui64Out)
{
  if (dblIn < -0.5 || dblIn > 1.844674407370955e19)
    return DISP_E_OVERFLOW;
  VARIANT_DutchRound(ULONG64, dblIn, *pui64Out);
  return S_OK;
}

/************************************************************************
 * VarUI8FromCy (OLEAUT32.433)
 *
 * Convert a VT_CY to a VT_UI8.
 *
 * PARAMS
 *  cyIn     [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  Negative values >= -5000 will be converted to 0.
 */
HRESULT WINAPI VarUI8FromCy(CY cyIn, ULONG64* pui64Out)
{
  if (cyIn.int64 < 0)
  {
    if (cyIn.int64 < -CY_HALF)
      return DISP_E_OVERFLOW;
    *pui64Out = 0;
  }
  else
  {
    *pui64Out = cyIn.int64 / CY_MULTIPLIER;

    cyIn.int64 -= *pui64Out * CY_MULTIPLIER; /* cyIn.s.Lo now holds fractional remainder */

    if (cyIn.s.Lo > CY_HALF || (cyIn.s.Lo == CY_HALF && (*pui64Out & 0x1)))
      (*pui64Out)++;
  }
  return S_OK;
}

/************************************************************************
 * VarUI8FromDate (OLEAUT32.434)
 *
 * Convert a VT_DATE to a VT_UI8.
 *
 * PARAMS
 *  dateIn   [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI8FromDate(DATE dateIn, ULONG64* pui64Out)
{
  return VarUI8FromR8(dateIn, pui64Out);
}

/************************************************************************
 * VarUI8FromStr (OLEAUT32.435)
 *
 * Convert a VT_BSTR to a VT_UI8.
 *
 * PARAMS
 *  strIn    [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI8FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, ULONG64* pui64Out)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pui64Out, VT_UI8);
}

/************************************************************************
 * VarUI8FromDisp (OLEAUT32.436)
 *
 * Convert a VT_DISPATCH to a VT_UI8.
 *
 * PARAMS
 *  pdispIn   [I] Source
 *  lcid      [I] LCID for conversion
 *  pui64Out  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarUI8FromDisp(IDispatch* pdispIn, LCID lcid, ULONG64* pui64Out)
{
  return VARIANT_FromDisp(pdispIn, lcid, pui64Out, VT_UI8, 0);
}

/************************************************************************
 * VarUI8FromBool (OLEAUT32.437)
 *
 * Convert a VT_BOOL to a VT_UI8.
 *
 * PARAMS
 *  boolIn   [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI8FromBool(VARIANT_BOOL boolIn, ULONG64* pui64Out)
{
  return VarI8FromI2(boolIn, (LONG64 *)pui64Out);
}
/************************************************************************
 * VarUI8FromI1 (OLEAUT32.438)
 *
 * Convert a VT_I1 to a VT_UI8.
 *
 * PARAMS
 *  cIn      [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarUI8FromI1(signed char cIn, ULONG64* pui64Out)
{
  return _VarUI8FromI1(cIn, pui64Out);
}

/************************************************************************
 * VarUI8FromUI2 (OLEAUT32.439)
 *
 * Convert a VT_UI2 to a VT_UI8.
 *
 * PARAMS
 *  usIn     [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI8FromUI2(USHORT usIn, ULONG64* pui64Out)
{
  return _VarUI8FromUI2(usIn, pui64Out);
}

/************************************************************************
 * VarUI8FromUI4 (OLEAUT32.440)
 *
 * Convert a VT_UI4 to a VT_UI8.
 *
 * PARAMS
 *  ulIn     [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarUI8FromUI4(ULONG ulIn, ULONG64* pui64Out)
{
  return _VarUI8FromUI4(ulIn, pui64Out);
}

/************************************************************************
 * VarUI8FromDec (OLEAUT32.441)
 *
 * Convert a VT_DECIMAL to a VT_UI8.
 *
 * PARAMS
 *  pDecIn   [I] Source
 *  pui64Out [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  Under native Win32, if the source value has a scale of 0, its sign is
 *  ignored, i.e. this function takes the absolute value rather than fail
 *  with DISP_E_OVERFLOW. This bug has been fixed in Wine's implementation
 *  (use VarAbs() on pDecIn first if you really want this behaviour).
 */
HRESULT WINAPI VarUI8FromDec(DECIMAL *pdecIn, ULONG64* pui64Out)
{
  if (!DEC_SCALE(pdecIn))
  {
    /* This decimal is just a 96 bit integer */
    if (DEC_SIGN(pdecIn) & ~DECIMAL_NEG)
      return E_INVALIDARG;

    if (DEC_HI32(pdecIn))
      return DISP_E_OVERFLOW;

    if (DEC_SIGN(pdecIn))
    {
      WARN("Sign would be ignored under Win32!\n");
      return DISP_E_OVERFLOW;
    }

    *pui64Out = DEC_LO64(pdecIn);
    return S_OK;
  }
  else
  {
    /* Decimal contains a floating point number */
    HRESULT hRet;
    double dbl;

    hRet = VarR8FromDec(pdecIn, &dbl);
    if (SUCCEEDED(hRet))
      hRet = VarUI8FromR8(dbl, pui64Out);
    return hRet;
  }
}

/* R4
 */

/************************************************************************
 * VarR4FromUI1 (OLEAUT32.68)
 *
 * Convert a VT_UI1 to a VT_R4.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromUI1(BYTE bIn, float *pFltOut)
{
  return _VarR4FromUI1(bIn, pFltOut);
}

/************************************************************************
 * VarR4FromI2 (OLEAUT32.69)
 *
 * Convert a VT_I2 to a VT_R4.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromI2(SHORT sIn, float *pFltOut)
{
  return _VarR4FromI2(sIn, pFltOut);
}

/************************************************************************
 * VarR4FromI4 (OLEAUT32.70)
 *
 * Convert a VT_I4 to a VT_R4.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromI4(LONG lIn, float *pFltOut)
{
  return _VarR4FromI4(lIn, pFltOut);
}

/************************************************************************
 * VarR4FromR8 (OLEAUT32.71)
 *
 * Convert a VT_R8 to a VT_R4.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination.
 */
HRESULT WINAPI VarR4FromR8(double dblIn, float *pFltOut)
{
  double d = dblIn < 0.0 ? -dblIn : dblIn;
  if (d > R4_MAX) return DISP_E_OVERFLOW;
  *pFltOut = dblIn;
  return S_OK;
}

/************************************************************************
 * VarR4FromCy (OLEAUT32.72)
 *
 * Convert a VT_CY to a VT_R4.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromCy(CY cyIn, float *pFltOut)
{
  *pFltOut = (double)cyIn.int64 / CY_MULTIPLIER_F;
  return S_OK;
}

/************************************************************************
 * VarR4FromDate (OLEAUT32.73)
 *
 * Convert a VT_DATE to a VT_R4.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination.
 */
HRESULT WINAPI VarR4FromDate(DATE dateIn, float *pFltOut)
{
  return VarR4FromR8(dateIn, pFltOut);
}

/************************************************************************
 * VarR4FromStr (OLEAUT32.74)
 *
 * Convert a VT_BSTR to a VT_R4.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if strIn or pFltOut is invalid.
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, float *pFltOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pFltOut, VT_R4);
}

/************************************************************************
 * VarR4FromDisp (OLEAUT32.75)
 *
 * Convert a VT_DISPATCH to a VT_R4.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pFltOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR4FromDisp(IDispatch* pdispIn, LCID lcid, float *pFltOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pFltOut, VT_R4, 0);
}

/************************************************************************
 * VarR4FromBool (OLEAUT32.76)
 *
 * Convert a VT_BOOL to a VT_R4.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL boolIn, float *pFltOut)
{
  return VarR4FromI2(boolIn, pFltOut);
}

/************************************************************************
 * VarR4FromI1 (OLEAUT32.213)
 *
 * Convert a VT_I1 to a VT_R4.
 *
 * PARAMS
 *  cIn     [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR4FromI1(signed char cIn, float *pFltOut)
{
  return _VarR4FromI1(cIn, pFltOut);
}

/************************************************************************
 * VarR4FromUI2 (OLEAUT32.214)
 *
 * Convert a VT_UI2 to a VT_R4.
 *
 * PARAMS
 *  usIn    [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR4FromUI2(USHORT usIn, float *pFltOut)
{
  return _VarR4FromUI2(usIn, pFltOut);
}

/************************************************************************
 * VarR4FromUI4 (OLEAUT32.215)
 *
 * Convert a VT_UI4 to a VT_R4.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR4FromUI4(ULONG ulIn, float *pFltOut)
{
  return _VarR4FromUI4(ulIn, pFltOut);
}

/************************************************************************
 * VarR4FromDec (OLEAUT32.216)
 *
 * Convert a VT_DECIMAL to a VT_R4.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid.
 */
HRESULT WINAPI VarR4FromDec(DECIMAL* pDecIn, float *pFltOut)
{
  BYTE scale = DEC_SCALE(pDecIn);
  int divisor = 1;
  double highPart;

  if (scale > DEC_MAX_SCALE || DEC_SIGN(pDecIn) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  while (scale--)
    divisor *= 10;

  if (DEC_SIGN(pDecIn))
    divisor = -divisor;

  if (DEC_HI32(pDecIn))
  {
    highPart = (double)DEC_HI32(pDecIn) / (double)divisor;
    highPart *= 4294967296.0F;
    highPart *= 4294967296.0F;
  }
  else
    highPart = 0.0;

  *pFltOut = (double)DEC_LO64(pDecIn) / (double)divisor + highPart;
  return S_OK;
}

/************************************************************************
 * VarR4FromI8 (OLEAUT32.360)
 *
 * Convert a VT_I8 to a VT_R4.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromI8(LONG64 llIn, float *pFltOut)
{
  return _VarR4FromI8(llIn, pFltOut);
}

/************************************************************************
 * VarR4FromUI8 (OLEAUT32.361)
 *
 * Convert a VT_UI8 to a VT_R4.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pFltOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR4FromUI8(ULONG64 ullIn, float *pFltOut)
{
  return _VarR4FromUI8(ullIn, pFltOut);
}

/************************************************************************
 * VarR4CmpR8 (OLEAUT32.316)
 *
 * Compare a VT_R4 to a VT_R8.
 *
 * PARAMS
 *  fltLeft  [I] Source
 *  dblRight [I] Value to compare
 *
 * RETURNS
 *  VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that fltLeft is less than,
 *  equal to or greater than dblRight respectively.
 */
HRESULT WINAPI VarR4CmpR8(float fltLeft, double dblRight)
{
  if (fltLeft < dblRight)
    return VARCMP_LT;
  else if (fltLeft > dblRight)
    return VARCMP_GT;
  return VARCMP_EQ;
}

/* R8
 */

/************************************************************************
 * VarR8FromUI1 (OLEAUT32.78)
 *
 * Convert a VT_UI1 to a VT_R8.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromUI1(BYTE bIn, double *pDblOut)
{
  return _VarR8FromUI1(bIn, pDblOut);
}

/************************************************************************
 * VarR8FromI2 (OLEAUT32.79)
 *
 * Convert a VT_I2 to a VT_R8.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromI2(SHORT sIn, double *pDblOut)
{
  return _VarR8FromI2(sIn, pDblOut);
}

/************************************************************************
 * VarR8FromI4 (OLEAUT32.80)
 *
 * Convert a VT_I4 to a VT_R8.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromI4(LONG lIn, double *pDblOut)
{
  return _VarR8FromI4(lIn, pDblOut);
}

/************************************************************************
 * VarR8FromR4 (OLEAUT32.81)
 *
 * Convert a VT_R4 to a VT_R8.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromR4(FLOAT fltIn, double *pDblOut)
{
  return _VarR8FromR4(fltIn, pDblOut);
}

/************************************************************************
 * VarR8FromCy (OLEAUT32.82)
 *
 * Convert a VT_CY to a VT_R8.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromCy(CY cyIn, double *pDblOut)
{
  return _VarR8FromCy(cyIn, pDblOut);
}

/************************************************************************
 * VarR8FromDate (OLEAUT32.83)
 *
 * Convert a VT_DATE to a VT_R8.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromDate(DATE dateIn, double *pDblOut)
{
  return _VarR8FromDate(dateIn, pDblOut);
}

/************************************************************************
 * VarR8FromStr (OLEAUT32.84)
 *
 * Convert a VT_BSTR to a VT_R8.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if strIn or pDblOut is invalid.
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR8FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, double *pDblOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pDblOut, VT_R8);
}

/************************************************************************
 * VarR8FromDisp (OLEAUT32.85)
 *
 * Convert a VT_DISPATCH to a VT_R8.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pDblOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR8FromDisp(IDispatch* pdispIn, LCID lcid, double *pDblOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pDblOut, VT_R8, 0);
}

/************************************************************************
 * VarR8FromBool (OLEAUT32.86)
 *
 * Convert a VT_BOOL to a VT_R8.
 *
 * PARAMS
 *  boolIn  [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL boolIn, double *pDblOut)
{
  return VarR8FromI2(boolIn, pDblOut);
}

/************************************************************************
 * VarR8FromI1 (OLEAUT32.217)
 *
 * Convert a VT_I1 to a VT_R8.
 *
 * PARAMS
 *  cIn     [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR8FromI1(signed char cIn, double *pDblOut)
{
  return _VarR8FromI1(cIn, pDblOut);
}

/************************************************************************
 * VarR8FromUI2 (OLEAUT32.218)
 *
 * Convert a VT_UI2 to a VT_R8.
 *
 * PARAMS
 *  usIn    [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR8FromUI2(USHORT usIn, double *pDblOut)
{
  return _VarR8FromUI2(usIn, pDblOut);
}

/************************************************************************
 * VarR8FromUI4 (OLEAUT32.219)
 *
 * Convert a VT_UI4 to a VT_R8.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarR8FromUI4(ULONG ulIn, double *pDblOut)
{
  return _VarR8FromUI4(ulIn, pDblOut);
}

/************************************************************************
 * VarR8FromDec (OLEAUT32.220)
 *
 * Convert a VT_DECIMAL to a VT_R8.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid.
 */
HRESULT WINAPI VarR8FromDec(const DECIMAL* pDecIn, double *pDblOut)
{
  BYTE scale = DEC_SCALE(pDecIn);
  double divisor = 1.0, highPart;

  if (scale > DEC_MAX_SCALE || DEC_SIGN(pDecIn) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  while (scale--)
    divisor *= 10;

  if (DEC_SIGN(pDecIn))
    divisor = -divisor;

  if (DEC_HI32(pDecIn))
  {
    highPart = (double)DEC_HI32(pDecIn) / divisor;
    highPart *= 4294967296.0F;
    highPart *= 4294967296.0F;
  }
  else
    highPart = 0.0;

  *pDblOut = (double)DEC_LO64(pDecIn) / divisor + highPart;
  return S_OK;
}

/************************************************************************
 * VarR8FromI8 (OLEAUT32.362)
 *
 * Convert a VT_I8 to a VT_R8.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromI8(LONG64 llIn, double *pDblOut)
{
  return _VarR8FromI8(llIn, pDblOut);
}

/************************************************************************
 * VarR8FromUI8 (OLEAUT32.363)
 *
 * Convert a VT_UI8 to a VT_R8.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarR8FromUI8(ULONG64 ullIn, double *pDblOut)
{
  return _VarR8FromUI8(ullIn, pDblOut);
}

/************************************************************************
 * VarR8Pow (OLEAUT32.315)
 *
 * Raise a VT_R8 to a power.
 *
 * PARAMS
 *  dblLeft [I] Source
 *  dblPow  [I] Power to raise dblLeft by
 *  pDblOut [O] Destination
 *
 * RETURNS
 *  S_OK. pDblOut contains dblLeft to the power of dblRight.
 */
HRESULT WINAPI VarR8Pow(double dblLeft, double dblPow, double *pDblOut)
{
  *pDblOut = pow(dblLeft, dblPow);
  return S_OK;
}

/************************************************************************
 * VarR8Round (OLEAUT32.317)
 *
 * Round a VT_R8 to a given number of decimal points.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  nDig    [I] Number of decimal points to round to
 *  pDblOut [O] Destination for rounded number
 *
 * RETURNS
 *  Success: S_OK. pDblOut is rounded to nDig digits.
 *  Failure: E_INVALIDARG, if cDecimals is less than 0.
 *
 * NOTES
 *  The native version of this function rounds using the internal
 *  binary representation of the number. Wine uses the dutch rounding
 *  convention, so therefore small differences can occur in the value returned.
 *  MSDN says that you should use your own rounding function if you want
 *  rounding to be predictable in your application.
 */
HRESULT WINAPI VarR8Round(double dblIn, int nDig, double *pDblOut)
{
  double scale, whole, fract;

  if (nDig < 0)
    return E_INVALIDARG;

  scale = pow(10.0, nDig);

  dblIn *= scale;
  whole = dblIn < 0 ? ceil(dblIn) : floor(dblIn);
  fract = dblIn - whole;

  if (fract > 0.5)
    dblIn = whole + 1.0;
  else if (fract == 0.5)
    dblIn = whole + fmod(whole, 2.0);
  else if (fract >= 0.0)
    dblIn = whole;
  else if (fract == -0.5)
    dblIn = whole - fmod(whole, 2.0);
  else if (fract > -0.5)
    dblIn = whole;
  else
    dblIn = whole - 1.0;

  *pDblOut = dblIn / scale;
  return S_OK;
}

/* CY
 */

/* Powers of 10 from 0..4 D.P. */
static const int CY_Divisors[5] = { CY_MULTIPLIER/10000, CY_MULTIPLIER/1000,
  CY_MULTIPLIER/100, CY_MULTIPLIER/10, CY_MULTIPLIER };

/************************************************************************
 * VarCyFromUI1 (OLEAUT32.98)
 *
 * Convert a VT_UI1 to a VT_CY.
 *
 * PARAMS
 *  bIn    [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromUI1(BYTE bIn, CY* pCyOut)
{
    pCyOut->int64 = (ULONG64)bIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromI2 (OLEAUT32.99)
 *
 * Convert a VT_I2 to a VT_CY.
 *
 * PARAMS
 *  sIn    [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromI2(SHORT sIn, CY* pCyOut)
{
    pCyOut->int64 = (LONG64)sIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromI4 (OLEAUT32.100)
 *
 * Convert a VT_I4 to a VT_CY.
 *
 * PARAMS
 *  sIn    [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromI4(LONG lIn, CY* pCyOut)
{
    pCyOut->int64 = (LONG64)lIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromR4 (OLEAUT32.101)
 *
 * Convert a VT_R4 to a VT_CY.
 *
 * PARAMS
 *  fltIn  [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromR4(FLOAT fltIn, CY* pCyOut)
{
  return VarCyFromR8(fltIn, pCyOut);
}

/************************************************************************
 * VarCyFromR8 (OLEAUT32.102)
 *
 * Convert a VT_R8 to a VT_CY.
 *
 * PARAMS
 *  dblIn  [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromR8(double dblIn, CY* pCyOut)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  /* This code gives identical results to Win32 on Intel.
   * Here we use fp exceptions to catch overflows when storing the value.
   */
  static const unsigned short r8_fpcontrol = 0x137f;
  static const double r8_multiplier = CY_MULTIPLIER_F;
  unsigned short old_fpcontrol, result_fpstatus;

  /* Clear exceptions, save the old fp state and load the new state */
  __asm__ __volatile__( "fnclex" );
  __asm__ __volatile__( "fstcw %0"   :   "=m" (old_fpcontrol) : );
  __asm__ __volatile__( "fldcw %0"   : : "m"  (r8_fpcontrol) );
  /* Perform the conversion. */
  __asm__ __volatile__( "fldl  %0"   : : "m"  (dblIn) );
  __asm__ __volatile__( "fmull %0"   : : "m"  (r8_multiplier) );
  __asm__ __volatile__( "fistpll %0" : : "m"  (*pCyOut) );
  /* Save the resulting fp state, load the old state and clear exceptions */
  __asm__ __volatile__( "fstsw %0"   :   "=m" (result_fpstatus) : );
  __asm__ __volatile__( "fnclex" );
  __asm__ __volatile__( "fldcw %0"   : : "m"  (old_fpcontrol) );

  if (result_fpstatus & 0x9) /* Overflow | Invalid */
    return DISP_E_OVERFLOW;
#else
  /* This version produces slightly different results for boundary cases */
  if (dblIn < -922337203685477.5807 || dblIn >= 922337203685477.5807)
    return DISP_E_OVERFLOW;
  dblIn *= CY_MULTIPLIER_F;
  VARIANT_DutchRound(LONG64, dblIn, pCyOut->int64);
#endif
  return S_OK;
}

/************************************************************************
 * VarCyFromDate (OLEAUT32.103)
 *
 * Convert a VT_DATE to a VT_CY.
 *
 * PARAMS
 *  dateIn [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromDate(DATE dateIn, CY* pCyOut)
{
  return VarCyFromR8(dateIn, pCyOut);
}

/************************************************************************
 * VarCyFromStr (OLEAUT32.104)
 *
 * Convert a VT_BSTR to a VT_CY.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, CY* pCyOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pCyOut, VT_CY);
}

/************************************************************************
 * VarCyFromDisp (OLEAUT32.105)
 *
 * Convert a VT_DISPATCH to a VT_CY.
 *
 * PARAMS
 *  pdispIn [I] Source
 *  lcid    [I] LCID for conversion
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromDisp(IDispatch* pdispIn, LCID lcid, CY* pCyOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pCyOut, VT_CY, 0);
}

/************************************************************************
 * VarCyFromBool (OLEAUT32.106)
 *
 * Convert a VT_BOOL to a VT_CY.
 *
 * PARAMS
 *  boolIn [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 *
 * NOTES
 *  While the sign of the boolean is stored in the currency, the value is
 *  converted to either 0 or 1.
 */
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL boolIn, CY* pCyOut)
{
    pCyOut->int64 = (LONG64)boolIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromI1 (OLEAUT32.225)
 *
 * Convert a VT_I1 to a VT_CY.
 *
 * PARAMS
 *  cIn    [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromI1(signed char cIn, CY* pCyOut)
{
    pCyOut->int64 = (LONG64)cIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromUI2 (OLEAUT32.226)
 *
 * Convert a VT_UI2 to a VT_CY.
 *
 * PARAMS
 *  usIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromUI2(USHORT usIn, CY* pCyOut)
{
    pCyOut->int64 = (ULONG64)usIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromUI4 (OLEAUT32.227)
 *
 * Convert a VT_UI4 to a VT_CY.
 *
 * PARAMS
 *  ulIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromUI4(ULONG ulIn, CY* pCyOut)
{
    pCyOut->int64 = (ULONG64)ulIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyFromDec (OLEAUT32.228)
 *
 * Convert a VT_DECIMAL to a VT_CY.
 *
 * PARAMS
 *  pdecIn  [I] Source
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromDec(DECIMAL* pdecIn, CY* pCyOut)
{
  DECIMAL rounded;
  HRESULT hRet;

  hRet = VarDecRound(pdecIn, 4, &rounded);

  if (SUCCEEDED(hRet))
  {
    double d;

    if (DEC_HI32(&rounded))
      return DISP_E_OVERFLOW;

    /* Note: Without the casts this promotes to int64 which loses precision */
    d = (double)DEC_LO64(&rounded) / (double)CY_Divisors[DEC_SCALE(&rounded)];
    if (DEC_SIGN(&rounded))
      d = -d;
    return VarCyFromR8(d, pCyOut);
  }
  return hRet;
}

/************************************************************************
 * VarCyFromI8 (OLEAUT32.366)
 *
 * Convert a VT_I8 to a VT_CY.
 *
 * PARAMS
 *  ullIn  [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromI8(LONG64 llIn, CY* pCyOut)
{
  if (llIn <= (I8_MIN/CY_MULTIPLIER) || llIn >= (I8_MAX/CY_MULTIPLIER)) return DISP_E_OVERFLOW;
  pCyOut->int64 = llIn * CY_MULTIPLIER;
  return S_OK;
}

/************************************************************************
 * VarCyFromUI8 (OLEAUT32.375)
 *
 * Convert a VT_UI8 to a VT_CY.
 *
 * PARAMS
 *  ullIn  [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarCyFromUI8(ULONG64 ullIn, CY* pCyOut)
{
    if (ullIn > (I8_MAX/CY_MULTIPLIER)) return DISP_E_OVERFLOW;
    pCyOut->int64 = ullIn * CY_MULTIPLIER;
    return S_OK;
}

/************************************************************************
 * VarCyAdd (OLEAUT32.299)
 *
 * Add one CY to another.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  cyRight [I] Value to add
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyAdd(const CY cyLeft, const CY cyRight, CY* pCyOut)
{
  double l,r;
  _VarR8FromCy(cyLeft, &l);
  _VarR8FromCy(cyRight, &r);
  l = l + r;
  return VarCyFromR8(l, pCyOut);
}

/************************************************************************
 * VarCyMul (OLEAUT32.303)
 *
 * Multiply one CY by another.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  cyRight [I] Value to multiply by
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyMul(const CY cyLeft, const CY cyRight, CY* pCyOut)
{
  double l,r;
  _VarR8FromCy(cyLeft, &l);
  _VarR8FromCy(cyRight, &r);
  l = l * r;
  return VarCyFromR8(l, pCyOut);
}

/************************************************************************
 * VarCyMulI4 (OLEAUT32.304)
 *
 * Multiply one CY by a VT_I4.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  lRight  [I] Value to multiply by
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyMulI4(const CY cyLeft, LONG lRight, CY* pCyOut)
{
  double d;

  _VarR8FromCy(cyLeft, &d);
  d = d * lRight;
  return VarCyFromR8(d, pCyOut);
}

/************************************************************************
 * VarCySub (OLEAUT32.305)
 *
 * Subtract one CY from another.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  cyRight [I] Value to subtract
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCySub(const CY cyLeft, const CY cyRight, CY* pCyOut)
{
  double l,r;
  _VarR8FromCy(cyLeft, &l);
  _VarR8FromCy(cyRight, &r);
  l = l - r;
  return VarCyFromR8(l, pCyOut);
}

/************************************************************************
 * VarCyAbs (OLEAUT32.306)
 *
 * Convert a VT_CY into its absolute value.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK. pCyOut contains the absolute value.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyAbs(const CY cyIn, CY* pCyOut)
{
  if (cyIn.s.Hi == (int)0x80000000 && !cyIn.s.Lo)
    return DISP_E_OVERFLOW;

  pCyOut->int64 = cyIn.int64 < 0 ? -cyIn.int64 : cyIn.int64;
  return S_OK;
}

/************************************************************************
 * VarCyFix (OLEAUT32.307)
 *
 * Return the integer part of a VT_CY.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  - The difference between this function and VarCyInt() is that VarCyInt() rounds
 *    negative numbers away from 0, while this function rounds them towards zero.
 */
HRESULT WINAPI VarCyFix(const CY cyIn, CY* pCyOut)
{
  pCyOut->int64 = cyIn.int64 / CY_MULTIPLIER;
  pCyOut->int64 *= CY_MULTIPLIER;
  return S_OK;
}

/************************************************************************
 * VarCyInt (OLEAUT32.308)
 *
 * Return the integer part of a VT_CY.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  - The difference between this function and VarCyFix() is that VarCyFix() rounds
 *    negative numbers towards 0, while this function rounds them away from zero.
 */
HRESULT WINAPI VarCyInt(const CY cyIn, CY* pCyOut)
{
  pCyOut->int64 = cyIn.int64 / CY_MULTIPLIER;
  pCyOut->int64 *= CY_MULTIPLIER;

  if (cyIn.int64 < 0 && cyIn.int64 % CY_MULTIPLIER != 0)
  {
    pCyOut->int64 -= CY_MULTIPLIER;
  }
  return S_OK;
}

/************************************************************************
 * VarCyNeg (OLEAUT32.309)
 *
 * Change the sign of a VT_CY.
 *
 * PARAMS
 *  cyIn   [I] Source
 *  pCyOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyNeg(const CY cyIn, CY* pCyOut)
{
  if (cyIn.s.Hi == (int)0x80000000 && !cyIn.s.Lo)
    return DISP_E_OVERFLOW;

  pCyOut->int64 = -cyIn.int64;
  return S_OK;
}

/************************************************************************
 * VarCyRound (OLEAUT32.310)
 *
 * Change the precision of a VT_CY.
 *
 * PARAMS
 *  cyIn      [I] Source
 *  cDecimals [I] New number of decimals to keep
 *  pCyOut    [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if cDecimals is less than 0.
 */
HRESULT WINAPI VarCyRound(const CY cyIn, int cDecimals, CY* pCyOut)
{
  if (cDecimals < 0)
    return E_INVALIDARG;

  if (cDecimals > 3)
  {
    /* Rounding to more precision than we have */
    *pCyOut = cyIn;
    return S_OK;
  }
  else
  {
    double d, div = CY_Divisors[cDecimals];

    _VarR8FromCy(cyIn, &d);
    d = d * div;
    VARIANT_DutchRound(LONGLONG, d, pCyOut->int64);
    d = (double)pCyOut->int64 / div * CY_MULTIPLIER_F;
    VARIANT_DutchRound(LONGLONG, d, pCyOut->int64);
    return S_OK;
  }
}

/************************************************************************
 * VarCyCmp (OLEAUT32.311)
 *
 * Compare two VT_CY values.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  cyRight [I] Value to compare
 *
 * RETURNS
 *  Success: VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that the value to
 *           compare is less, equal or greater than source respectively.
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparison
 */
HRESULT WINAPI VarCyCmp(const CY cyLeft, const CY cyRight)
{
  HRESULT hRet;
  CY result;

  /* Subtract right from left, and compare the result to 0 */
  hRet = VarCySub(cyLeft, cyRight, &result);

  if (SUCCEEDED(hRet))
  {
    if (result.int64 < 0)
      hRet = (HRESULT)VARCMP_LT;
    else if (result.int64 > 0)
      hRet = (HRESULT)VARCMP_GT;
    else
      hRet = (HRESULT)VARCMP_EQ;
  }
  return hRet;
}

/************************************************************************
 * VarCyCmpR8 (OLEAUT32.312)
 *
 * Compare a VT_CY to a double
 *
 * PARAMS
 *  cyLeft   [I] Currency Source
 *  dblRight [I] double to compare to cyLeft
 *
 * RETURNS
 *  Success: VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that dblRight is
 *           less than, equal to or greater than cyLeft respectively.
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparison
 */
HRESULT WINAPI VarCyCmpR8(const CY cyLeft, double dblRight)
{
  HRESULT hRet;
  CY cyRight;

  hRet = VarCyFromR8(dblRight, &cyRight);

  if (SUCCEEDED(hRet))
    hRet = VarCyCmp(cyLeft, cyRight);

  return hRet;
}

/************************************************************************
 * VarCyMulI8 (OLEAUT32.329)
 *
 * Multiply a VT_CY by a VT_I8.
 *
 * PARAMS
 *  cyLeft  [I] Source
 *  llRight [I] Value to multiply by
 *  pCyOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarCyMulI8(const CY cyLeft, LONG64 llRight, CY* pCyOut)
{
  double d;

  _VarR8FromCy(cyLeft, &d);
  d = d  * (double)llRight;
  return VarCyFromR8(d, pCyOut);
}

/* DECIMAL
 */

/************************************************************************
 * VarDecFromUI1 (OLEAUT32.190)
 *
 * Convert a VT_UI1 to a DECIMAL.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromUI1(BYTE bIn, DECIMAL* pDecOut)
{
  return VarDecFromUI4(bIn, pDecOut);
}

/************************************************************************
 * VarDecFromI2 (OLEAUT32.191)
 *
 * Convert a VT_I2 to a DECIMAL.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromI2(SHORT sIn, DECIMAL* pDecOut)
{
  return VarDecFromI4(sIn, pDecOut);
}

/************************************************************************
 * VarDecFromI4 (OLEAUT32.192)
 *
 * Convert a VT_I4 to a DECIMAL.
 *
 * PARAMS
 *  sIn     [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromI4(LONG lIn, DECIMAL* pDecOut)
{
  DEC_HI32(pDecOut) = 0;
  DEC_MID32(pDecOut) = 0;

  if (lIn < 0)
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_NEG,0);
    DEC_LO32(pDecOut) = -lIn;
  }
  else
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
    DEC_LO32(pDecOut) = lIn;
  }
  return S_OK;
}

/* internal representation of the value stored in a DECIMAL. The bytes are
   stored from LSB at index 0 to MSB at index 11
 */
typedef struct DECIMAL_internal
{
    DWORD bitsnum[3];  /* 96 significant bits, unsigned */
    unsigned char scale;      /* number scaled * 10 ^ -(scale) */
    unsigned int  sign : 1;   /* 0 - positive, 1 - negative */
} VARIANT_DI;

static HRESULT VARIANT_DI_FromR4(float source, VARIANT_DI * dest);
static HRESULT VARIANT_DI_FromR8(double source, VARIANT_DI * dest);
static void VARIANT_DIFromDec(const DECIMAL * from, VARIANT_DI * to);
static void VARIANT_DecFromDI(const VARIANT_DI * from, DECIMAL * to);
static unsigned char VARIANT_int_divbychar(DWORD * p, unsigned int n, unsigned char divisor);
static BOOL VARIANT_int_iszero(const DWORD * p, unsigned int n);

/************************************************************************
 * VarDecFromR4 (OLEAUT32.193)
 *
 * Convert a VT_R4 to a DECIMAL.
 *
 * PARAMS
 *  fltIn   [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromR4(FLOAT fltIn, DECIMAL* pDecOut)
{
  VARIANT_DI di;
  HRESULT hres;

  hres = VARIANT_DI_FromR4(fltIn, &di);
  if (hres == S_OK) VARIANT_DecFromDI(&di, pDecOut);
  return hres;
}

/************************************************************************
 * VarDecFromR8 (OLEAUT32.194)
 *
 * Convert a VT_R8 to a DECIMAL.
 *
 * PARAMS
 *  dblIn   [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromR8(double dblIn, DECIMAL* pDecOut)
{
  VARIANT_DI di;
  HRESULT hres;

  hres = VARIANT_DI_FromR8(dblIn, &di);
  if (hres == S_OK) VARIANT_DecFromDI(&di, pDecOut);
  return hres;
}

/************************************************************************
 * VarDecFromDate (OLEAUT32.195)
 *
 * Convert a VT_DATE to a DECIMAL.
 *
 * PARAMS
 *  dateIn  [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromDate(DATE dateIn, DECIMAL* pDecOut)
{
  return VarDecFromR8(dateIn, pDecOut);
}

/************************************************************************
 * VarDecFromCy (OLEAUT32.196)
 *
 * Convert a VT_CY to a DECIMAL.
 *
 * PARAMS
 *  cyIn    [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromCy(CY cyIn, DECIMAL* pDecOut)
{
  DEC_HI32(pDecOut) = 0;

  /* Note: This assumes 2s complement integer representation */
  if (cyIn.s.Hi & 0x80000000)
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_NEG,4);
    DEC_LO64(pDecOut) = -cyIn.int64;
  }
  else
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,4);
    DEC_MID32(pDecOut) = cyIn.s.Hi;
    DEC_LO32(pDecOut) = cyIn.s.Lo;
  }
  return S_OK;
}

/************************************************************************
 * VarDecFromStr (OLEAUT32.197)
 *
 * Convert a VT_BSTR to a DECIMAL.
 *
 * PARAMS
 *  strIn   [I] Source
 *  lcid    [I] LCID for the conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDecFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, DECIMAL* pDecOut)
{
  return VARIANT_NumberFromBstr(strIn, lcid, dwFlags, pDecOut, VT_DECIMAL);
}

/************************************************************************
 * VarDecFromDisp (OLEAUT32.198)
 *
 * Convert a VT_DISPATCH to a DECIMAL.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pDecOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarDecFromDisp(IDispatch* pdispIn, LCID lcid, DECIMAL* pDecOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pDecOut, VT_DECIMAL, 0);
}

/************************************************************************
 * VarDecFromBool (OLEAUT32.199)
 *
 * Convert a VT_BOOL to a DECIMAL.
 *
 * PARAMS
 *  bIn     [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 *
 * NOTES
 *  The value is converted to either 0 (if bIn is FALSE) or -1 (TRUE).
 */
HRESULT WINAPI VarDecFromBool(VARIANT_BOOL bIn, DECIMAL* pDecOut)
{
  DEC_HI32(pDecOut) = 0;
  DEC_MID32(pDecOut) = 0;
  if (bIn)
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_NEG,0);
    DEC_LO32(pDecOut) = 1;
  }
  else
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
    DEC_LO32(pDecOut) = 0;
  }
  return S_OK;
}

/************************************************************************
 * VarDecFromI1 (OLEAUT32.241)
 *
 * Convert a VT_I1 to a DECIMAL.
 *
 * PARAMS
 *  cIn     [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromI1(signed char cIn, DECIMAL* pDecOut)
{
  return VarDecFromI4(cIn, pDecOut);
}

/************************************************************************
 * VarDecFromUI2 (OLEAUT32.242)
 *
 * Convert a VT_UI2 to a DECIMAL.
 *
 * PARAMS
 *  usIn    [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromUI2(USHORT usIn, DECIMAL* pDecOut)
{
  return VarDecFromUI4(usIn, pDecOut);
}

/************************************************************************
 * VarDecFromUI4 (OLEAUT32.243)
 *
 * Convert a VT_UI4 to a DECIMAL.
 *
 * PARAMS
 *  ulIn    [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromUI4(ULONG ulIn, DECIMAL* pDecOut)
{
  DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
  DEC_HI32(pDecOut) = 0;
  DEC_MID32(pDecOut) = 0;
  DEC_LO32(pDecOut) = ulIn;
  return S_OK;
}

/************************************************************************
 * VarDecFromI8 (OLEAUT32.374)
 *
 * Convert a VT_I8 to a DECIMAL.
 *
 * PARAMS
 *  llIn    [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromI8(LONG64 llIn, DECIMAL* pDecOut)
{
  PULARGE_INTEGER pLi = (PULARGE_INTEGER)&llIn;

  DEC_HI32(pDecOut) = 0;

  /* Note: This assumes 2s complement integer representation */
  if (pLi->u.HighPart & 0x80000000)
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_NEG,0);
    DEC_LO64(pDecOut) = -pLi->QuadPart;
  }
  else
  {
    DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
    DEC_MID32(pDecOut) = pLi->u.HighPart;
    DEC_LO32(pDecOut) = pLi->u.LowPart;
  }
  return S_OK;
}

/************************************************************************
 * VarDecFromUI8 (OLEAUT32.375)
 *
 * Convert a VT_UI8 to a DECIMAL.
 *
 * PARAMS
 *  ullIn   [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDecFromUI8(ULONG64 ullIn, DECIMAL* pDecOut)
{
  DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
  DEC_HI32(pDecOut) = 0;
  DEC_LO64(pDecOut) = ullIn;
  return S_OK;
}

/* Make two DECIMALS the same scale; used by math functions below */
static HRESULT VARIANT_DecScale(const DECIMAL** ppDecLeft,
                                const DECIMAL** ppDecRight,
                                DECIMAL pDecOut[2])
{
  static DECIMAL scaleFactor;
  unsigned char remainder;
  DECIMAL decTemp;
  VARIANT_DI di;
  int scaleAmount, i;

  if (DEC_SIGN(*ppDecLeft) & ~DECIMAL_NEG || DEC_SIGN(*ppDecRight) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  DEC_LO32(&scaleFactor) = 10;

  i = scaleAmount = DEC_SCALE(*ppDecLeft) - DEC_SCALE(*ppDecRight);

  if (!scaleAmount)
    return S_OK; /* Same scale */

  if (scaleAmount > 0)
  {
    decTemp = *(*ppDecRight); /* Left is bigger - scale the right hand side */
    *ppDecRight = &pDecOut[0];
  }
  else
  {
    decTemp = *(*ppDecLeft); /* Right is bigger - scale the left hand side */
    *ppDecLeft  = &pDecOut[0];
    i = -scaleAmount;
  }

  /* Multiply up the value to be scaled by the correct amount (if possible) */
  while (i > 0 && SUCCEEDED(VarDecMul(&decTemp, &scaleFactor, &pDecOut[0])))
  {
    decTemp = pDecOut[0];
    i--;
  }

  if (!i)
  {
    DEC_SCALE(&pDecOut[0]) += (scaleAmount > 0) ? scaleAmount : (-scaleAmount);
    return S_OK; /* Same scale */
  }

  /* Scaling further not possible, reduce accuracy of other argument */
  pDecOut[0] = decTemp;
  if (scaleAmount > 0)
  {
    DEC_SCALE(&pDecOut[0]) += scaleAmount - i;
    VARIANT_DIFromDec(*ppDecLeft, &di);
    *ppDecLeft = &pDecOut[1];
  }
  else
  {
    DEC_SCALE(&pDecOut[0]) += (-scaleAmount) - i;
    VARIANT_DIFromDec(*ppDecRight, &di);
    *ppDecRight = &pDecOut[1];
  }

  di.scale -= i;
  remainder = 0;
  while (i-- > 0 && !VARIANT_int_iszero(di.bitsnum, sizeof(di.bitsnum)/sizeof(DWORD)))
  {
    remainder = VARIANT_int_divbychar(di.bitsnum, sizeof(di.bitsnum)/sizeof(DWORD), 10);
    if (remainder > 0) WARN("losing significant digits (remainder %u)...\n", remainder);
  }

  /* round up the result - native oleaut32 does this */
  if (remainder >= 5) {
      for (remainder = 1, i = 0; i < sizeof(di.bitsnum)/sizeof(DWORD) && remainder; i++) {
          ULONGLONG digit = di.bitsnum[i] + 1;
          remainder = (digit > 0xFFFFFFFF) ? 1 : 0;
          di.bitsnum[i] = digit & 0xFFFFFFFF;
      }
  }

  VARIANT_DecFromDI(&di, &pDecOut[1]);
  return S_OK;
}

/* Add two unsigned 32 bit values with overflow */
static ULONG VARIANT_Add(ULONG ulLeft, ULONG ulRight, ULONG* pulHigh)
{
  ULARGE_INTEGER ul64;

  ul64.QuadPart = (ULONG64)ulLeft + (ULONG64)ulRight + (ULONG64)*pulHigh;
  *pulHigh = ul64.u.HighPart;
  return ul64.u.LowPart;
}

/* Subtract two unsigned 32 bit values with underflow */
static ULONG VARIANT_Sub(ULONG ulLeft, ULONG ulRight, ULONG* pulHigh)
{
  BOOL invert = FALSE;
  ULARGE_INTEGER ul64;

  ul64.QuadPart = (LONG64)ulLeft - (ULONG64)ulRight;
  if (ulLeft < ulRight)
    invert = TRUE;

  if (ul64.QuadPart > (ULONG64)*pulHigh)
    ul64.QuadPart -= (ULONG64)*pulHigh;
  else
  {
    ul64.QuadPart -= (ULONG64)*pulHigh;
    invert = TRUE;
  }
  if (invert)
    ul64.u.HighPart = -ul64.u.HighPart ;

  *pulHigh = ul64.u.HighPart;
  return ul64.u.LowPart;
}

/* Multiply two unsigned 32 bit values with overflow */
static ULONG VARIANT_Mul(ULONG ulLeft, ULONG ulRight, ULONG* pulHigh)
{
  ULARGE_INTEGER ul64;

  ul64.QuadPart = (ULONG64)ulLeft * (ULONG64)ulRight + (ULONG64)*pulHigh;
  *pulHigh = ul64.u.HighPart;
  return ul64.u.LowPart;
}

/* Compare two decimals that have the same scale */
static inline int VARIANT_DecCmp(const DECIMAL *pDecLeft, const DECIMAL *pDecRight)
{
  if ( DEC_HI32(pDecLeft) < DEC_HI32(pDecRight) ||
      (DEC_HI32(pDecLeft) <= DEC_HI32(pDecRight) && DEC_LO64(pDecLeft) < DEC_LO64(pDecRight)))
    return -1;
  else if (DEC_HI32(pDecLeft) == DEC_HI32(pDecRight) && DEC_LO64(pDecLeft) == DEC_LO64(pDecRight))
    return 0;
  return 1;
}

/************************************************************************
 * VarDecAdd (OLEAUT32.177)
 *
 * Add one DECIMAL to another.
 *
 * PARAMS
 *  pDecLeft  [I] Source
 *  pDecRight [I] Value to add
 *  pDecOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDecAdd(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  HRESULT hRet;
  DECIMAL scaled[2];

  hRet = VARIANT_DecScale(&pDecLeft, &pDecRight, scaled);

  if (SUCCEEDED(hRet))
  {
    /* Our decimals now have the same scale, we can add them as 96 bit integers */
    ULONG overflow = 0;
    BYTE sign = DECIMAL_POS;
    int cmp;

    /* Correct for the sign of the result */
    if (DEC_SIGN(pDecLeft) && DEC_SIGN(pDecRight))
    {
      /* -x + -y : Negative */
      sign = DECIMAL_NEG;
      goto VarDecAdd_AsPositive;
    }
    else if (DEC_SIGN(pDecLeft) && !DEC_SIGN(pDecRight))
    {
      cmp = VARIANT_DecCmp(pDecLeft, pDecRight);

      /* -x + y : Negative if x > y */
      if (cmp > 0)
      {
        sign = DECIMAL_NEG;
VarDecAdd_AsNegative:
        DEC_LO32(pDecOut)  = VARIANT_Sub(DEC_LO32(pDecLeft),  DEC_LO32(pDecRight),  &overflow);
        DEC_MID32(pDecOut) = VARIANT_Sub(DEC_MID32(pDecLeft), DEC_MID32(pDecRight), &overflow);
        DEC_HI32(pDecOut)  = VARIANT_Sub(DEC_HI32(pDecLeft),  DEC_HI32(pDecRight),  &overflow);
      }
      else
      {
VarDecAdd_AsInvertedNegative:
        DEC_LO32(pDecOut)  = VARIANT_Sub(DEC_LO32(pDecRight),  DEC_LO32(pDecLeft),  &overflow);
        DEC_MID32(pDecOut) = VARIANT_Sub(DEC_MID32(pDecRight), DEC_MID32(pDecLeft), &overflow);
        DEC_HI32(pDecOut)  = VARIANT_Sub(DEC_HI32(pDecRight),  DEC_HI32(pDecLeft),  &overflow);
      }
    }
    else if (!DEC_SIGN(pDecLeft) && DEC_SIGN(pDecRight))
    {
      cmp = VARIANT_DecCmp(pDecLeft, pDecRight);

      /* x + -y : Negative if x <= y */
      if (cmp <= 0)
      {
        sign = DECIMAL_NEG;
        goto VarDecAdd_AsInvertedNegative;
      }
      goto VarDecAdd_AsNegative;
    }
    else
    {
      /* x + y : Positive */
VarDecAdd_AsPositive:
      DEC_LO32(pDecOut)  = VARIANT_Add(DEC_LO32(pDecLeft),  DEC_LO32(pDecRight),  &overflow);
      DEC_MID32(pDecOut) = VARIANT_Add(DEC_MID32(pDecLeft), DEC_MID32(pDecRight), &overflow);
      DEC_HI32(pDecOut)  = VARIANT_Add(DEC_HI32(pDecLeft),  DEC_HI32(pDecRight),  &overflow);
    }

    if (overflow)
      return DISP_E_OVERFLOW; /* overflowed */

    DEC_SCALE(pDecOut) = DEC_SCALE(pDecLeft);
    DEC_SIGN(pDecOut) = sign;
  }
  return hRet;
}

/* translate from external DECIMAL format into an internal representation */
static void VARIANT_DIFromDec(const DECIMAL * from, VARIANT_DI * to)
{
    to->scale = DEC_SCALE(from);
    to->sign = DEC_SIGN(from) ? 1 : 0;

    to->bitsnum[0] = DEC_LO32(from);
    to->bitsnum[1] = DEC_MID32(from);
    to->bitsnum[2] = DEC_HI32(from);
}

static void VARIANT_DecFromDI(const VARIANT_DI * from, DECIMAL * to)
{
    if (from->sign) {
        DEC_SIGNSCALE(to) = SIGNSCALE(DECIMAL_NEG, from->scale);
    } else {
        DEC_SIGNSCALE(to) = SIGNSCALE(DECIMAL_POS, from->scale);
    }

    DEC_LO32(to) = from->bitsnum[0];
    DEC_MID32(to) = from->bitsnum[1];
    DEC_HI32(to) = from->bitsnum[2];
}

/* clear an internal representation of a DECIMAL */
static void VARIANT_DI_clear(VARIANT_DI * i)
{
    memset(i, 0, sizeof(VARIANT_DI));
}

/* divide the (unsigned) number stored in p (LSB) by a byte value (<= 0xff). Any nonzero
   size is supported. The value in p is replaced by the quotient of the division, and
   the remainder is returned as a result. This routine is most often used with a divisor
   of 10 in order to scale up numbers, and in the DECIMAL->string conversion.
 */
static unsigned char VARIANT_int_divbychar(DWORD * p, unsigned int n, unsigned char divisor)
{
    if (divisor == 0) {
        /* division by 0 */
        return 0xFF;
    } else if (divisor == 1) {
        /* dividend remains unchanged */
        return 0;
    } else {
        unsigned char remainder = 0;
        ULONGLONG iTempDividend;
        signed int i;
        
        for (i = n - 1; i >= 0 && !p[i]; i--);  /* skip leading zeros */
        for (; i >= 0; i--) {
            iTempDividend = ((ULONGLONG)remainder << 32) + p[i];
            remainder = iTempDividend % divisor;
            p[i] = iTempDividend / divisor;
        }
        
        return remainder;
    }
}

/* check to test if encoded number is a zero. Returns 1 if zero, 0 for nonzero */
static BOOL VARIANT_int_iszero(const DWORD * p, unsigned int n)
{
    for (; n > 0; n--) if (*p++ != 0) return FALSE;
    return TRUE;
}

/* multiply two DECIMALS, without changing either one, and place result in third
   parameter. Result is normalized when scale is > 0. Attempts to remove significant
   digits when scale > 0 in order to fit an overflowing result. Final overflow
   flag is returned.
 */
static int VARIANT_DI_mul(const VARIANT_DI * a, const VARIANT_DI * b, VARIANT_DI * result)
{
    BOOL r_overflow = FALSE;
    DWORD running[6];
    signed int mulstart;

    VARIANT_DI_clear(result);
    result->sign = (a->sign ^ b->sign) ? 1 : 0;

    /* Multiply 128-bit operands into a (max) 256-bit result. The scale
       of the result is formed by adding the scales of the operands.
     */
    result->scale = a->scale + b->scale;
    memset(running, 0, sizeof(running));

    /* count number of leading zero-bytes in operand A */
    for (mulstart = sizeof(a->bitsnum)/sizeof(DWORD) - 1; mulstart >= 0 && !a->bitsnum[mulstart]; mulstart--);
    if (mulstart < 0) {
        /* result is 0, because operand A is 0 */
        result->scale = 0;
        result->sign = 0;
    } else {
        unsigned char remainder = 0;
        int iA;        

        /* perform actual multiplication */
        for (iA = 0; iA <= mulstart; iA++) {
            ULONG iOverflowMul;
            int iB;
            
            for (iOverflowMul = 0, iB = 0; iB < sizeof(b->bitsnum)/sizeof(DWORD); iB++) {
                ULONG iRV;
                int iR;
                
                iRV = VARIANT_Mul(b->bitsnum[iB], a->bitsnum[iA], &iOverflowMul);
                iR = iA + iB;
                do {
                    running[iR] = VARIANT_Add(running[iR], 0, &iRV);
                    iR++;
                } while (iRV);
            }
        }

/* Too bad - native oleaut does not do this, so we should not either */
#if 0
        /* While the result is divisible by 10, and the scale > 0, divide by 10.
           This operation should not lose significant digits, and gives an
           opportunity to reduce the possibility of overflows in future
           operations issued by the application.
         */
        while (result->scale > 0) {
            memcpy(quotient, running, sizeof(quotient));
            remainder = VARIANT_int_divbychar(quotient, sizeof(quotient) / sizeof(DWORD), 10);
            if (remainder > 0) break;
            memcpy(running, quotient, sizeof(quotient));
            result->scale--;
        }
#endif
        /* While the 256-bit result overflows, and the scale > 0, divide by 10.
           This operation *will* lose significant digits of the result because
           all the factors of 10 were consumed by the previous operation.
        */
        while (result->scale > 0 && !VARIANT_int_iszero(
            running + sizeof(result->bitsnum) / sizeof(DWORD),
            (sizeof(running) - sizeof(result->bitsnum)) / sizeof(DWORD))) {
            
            remainder = VARIANT_int_divbychar(running, sizeof(running) / sizeof(DWORD), 10);
            if (remainder > 0) WARN("losing significant digits (remainder %u)...\n", remainder);
            result->scale--;
        }
        
        /* round up the result - native oleaut32 does this */
        if (remainder >= 5) {
            unsigned int i;
            for (remainder = 1, i = 0; i < sizeof(running)/sizeof(DWORD) && remainder; i++) {
                ULONGLONG digit = running[i] + 1;
                remainder = (digit > 0xFFFFFFFF) ? 1 : 0;
                running[i] = digit & 0xFFFFFFFF;
            }
        }

        /* Signal overflow if scale == 0 and 256-bit result still overflows,
           and copy result bits into result structure
        */
        r_overflow = !VARIANT_int_iszero(
            running + sizeof(result->bitsnum)/sizeof(DWORD), 
            (sizeof(running) - sizeof(result->bitsnum))/sizeof(DWORD));
        memcpy(result->bitsnum, running, sizeof(result->bitsnum));
    }
    return r_overflow;
}

/* cast DECIMAL into string. Any scale should be handled properly. en_US locale is
   hardcoded (period for decimal separator, dash as negative sign). Returns TRUE for
   success, FALSE if insufficient space in output buffer.
 */
static BOOL VARIANT_DI_tostringW(const VARIANT_DI * a, WCHAR * s, unsigned int n)
{
    BOOL overflow = FALSE;
    DWORD quotient[3];
    unsigned char remainder;
    unsigned int i;

    /* place negative sign */
    if (!VARIANT_int_iszero(a->bitsnum, sizeof(a->bitsnum) / sizeof(DWORD)) && a->sign) {
        if (n > 0) {
            *s++ = '-';
            n--;
        }
        else overflow = TRUE;
    }

    /* prepare initial 0 */
    if (!overflow) {
        if (n >= 2) {
            s[0] = '0';
            s[1] = '\0';
        } else overflow = TRUE;
    }

    i = 0;
    memcpy(quotient, a->bitsnum, sizeof(a->bitsnum));
    while (!overflow && !VARIANT_int_iszero(quotient, sizeof(quotient) / sizeof(DWORD))) {
        remainder = VARIANT_int_divbychar(quotient, sizeof(quotient) / sizeof(DWORD), 10);
        if (i + 2 > n) {
            overflow = TRUE;
        } else {
            s[i++] = '0' + remainder;
            s[i] = '\0';
        }
    }

    if (!overflow && !VARIANT_int_iszero(a->bitsnum, sizeof(a->bitsnum) / sizeof(DWORD))) {

        /* reverse order of digits */
        WCHAR * x = s; WCHAR * y = s + i - 1;
        while (x < y) {
            *x ^= *y;
            *y ^= *x;
            *x++ ^= *y--;
        }

        /* check for decimal point. "i" now has string length */
        if (i <= a->scale) {
            unsigned int numzeroes = a->scale + 1 - i;
            if (i + 1 + numzeroes >= n) {
                overflow = TRUE;
            } else {
                memmove(s + numzeroes, s, (i + 1) * sizeof(WCHAR));
                i += numzeroes;
                while (numzeroes > 0) {
                    s[--numzeroes] = '0';
                }
            }
        }

        /* place decimal point */
        if (a->scale > 0) {
            unsigned int periodpos = i - a->scale;
            if (i + 2 >= n) {
                overflow = TRUE;
            } else {
                memmove(s + periodpos + 1, s + periodpos, (i + 1 - periodpos) * sizeof(WCHAR));
                s[periodpos] = '.'; i++;
                
                /* remove extra zeros at the end, if any */
                while (s[i - 1] == '0') s[--i] = '\0';
                if (s[i - 1] == '.') s[--i] = '\0';
            }
        }
    }

    return !overflow;
}

/* shift the bits of a DWORD array to the left. p[0] is assumed LSB */
static void VARIANT_int_shiftleft(DWORD * p, unsigned int n, unsigned int shift)
{
    DWORD shifted;
    unsigned int i;
    
    /* shift whole DWORDs to the left */
    while (shift >= 32)
    {
        memmove(p + 1, p, (n - 1) * sizeof(DWORD));
        *p = 0; shift -= 32;
    }
    
    /* shift remainder (1..31 bits) */
    shifted = 0;
    if (shift > 0) for (i = 0; i < n; i++)
    {
        DWORD b;
        b = p[i] >> (32 - shift);
        p[i] = (p[i] << shift) | shifted;
        shifted = b;
    }
}

/* add the (unsigned) numbers stored in two DWORD arrays with LSB at index 0.
   Value at v is incremented by the value at p. Any size is supported, provided
   that v is not shorter than p. Any unapplied carry is returned as a result.
 */
static unsigned char VARIANT_int_add(DWORD * v, unsigned int nv, const DWORD * p,
    unsigned int np)
{
    unsigned char carry = 0;

    if (nv >= np) {
        ULONGLONG sum;
        unsigned int i;

        for (i = 0; i < np; i++) {
            sum = (ULONGLONG)v[i]
                + (ULONGLONG)p[i]
                + (ULONGLONG)carry;
            v[i] = sum & 0xffffffff;
            carry = sum >> 32;
        }
        for (; i < nv && carry; i++) {
            sum = (ULONGLONG)v[i]
                + (ULONGLONG)carry;
            v[i] = sum & 0xffffffff;
            carry = sum >> 32;
        }
    }
    return carry;
}

/* perform integral division with operand p as dividend. Parameter n indicates 
   number of available DWORDs in divisor p, but available space in p must be 
   actually at least 2 * n DWORDs, because the remainder of the integral 
   division is built in the next n DWORDs past the start of the quotient. This 
   routine replaces the dividend in p with the quotient, and appends n 
   additional DWORDs for the remainder.

   Thanks to Lee & Mark Atkinson for their book _Using_C_ (my very first book on
   C/C++ :-) where the "longhand binary division" algorithm was exposed for the
   source code to the VLI (Very Large Integer) division operator. This algorithm
   was then heavily modified by me (Alex Villacis Lasso) in order to handle
   variably-scaled integers such as the MS DECIMAL representation.
 */
static void VARIANT_int_div(DWORD * p, unsigned int n, const DWORD * divisor,
    unsigned int dn)
{
    unsigned int i;
    DWORD tempsub[8];
    DWORD * negdivisor = tempsub + n;

    /* build 2s-complement of divisor */
    for (i = 0; i < n; i++) negdivisor[i] = (i < dn) ? ~divisor[i] : 0xFFFFFFFF;
    p[n] = 1;
    VARIANT_int_add(negdivisor, n, p + n, 1);
    memset(p + n, 0, n * sizeof(DWORD));

    /* skip all leading zero DWORDs in quotient */
    for (i = 0; i < n && !p[n - 1]; i++) VARIANT_int_shiftleft(p, n, 32);
    /* i is now number of DWORDs left to process */
    for (i <<= 5; i < (n << 5); i++) {
        VARIANT_int_shiftleft(p, n << 1, 1);    /* shl quotient+remainder */

        /* trial subtraction */
        memcpy(tempsub, p + n, n * sizeof(DWORD));
        VARIANT_int_add(tempsub, n, negdivisor, n);

        /* check whether result of subtraction was negative */
        if ((tempsub[n - 1] & 0x80000000) == 0) {
            memcpy(p + n, tempsub, n * sizeof(DWORD));
            p[0] |= 1;
        }
    }
}

/* perform integral multiplication by a byte operand. Used for scaling by 10 */
static unsigned char VARIANT_int_mulbychar(DWORD * p, unsigned int n, unsigned char m)
{
    unsigned int i;
    ULONG iOverflowMul;
    
    for (iOverflowMul = 0, i = 0; i < n; i++)
        p[i] = VARIANT_Mul(p[i], m, &iOverflowMul);
    return (unsigned char)iOverflowMul;
}

/* increment value in A by the value indicated in B, with scale adjusting. 
   Modifies parameters by adjusting scales. Returns 0 if addition was 
   successful, nonzero if a parameter underflowed before it could be 
   successfully used in the addition.
 */
static int VARIANT_int_addlossy(
    DWORD * a, int * ascale, unsigned int an,
    DWORD * b, int * bscale, unsigned int bn)
{
    int underflow = 0;

    if (VARIANT_int_iszero(a, an)) {
        /* if A is zero, copy B into A, after removing digits */
        while (bn > an && !VARIANT_int_iszero(b + an, bn - an)) {
            VARIANT_int_divbychar(b, bn, 10);
            (*bscale)--;
        }
        memcpy(a, b, an * sizeof(DWORD));
        *ascale = *bscale;
    } else if (!VARIANT_int_iszero(b, bn)) {
        unsigned int tn = an + 1;
        DWORD t[5];

        if (bn + 1 > tn) tn = bn + 1;
        if (*ascale != *bscale) {
            /* first (optimistic) try - try to scale down the one with the bigger
               scale, while this number is divisible by 10 */
            DWORD * digitchosen;
            unsigned int nchosen;
            int * scalechosen;
            int targetscale;

            if (*ascale < *bscale) {
                targetscale = *ascale;
                scalechosen = bscale;
                digitchosen = b;
                nchosen = bn;
            } else {
                targetscale = *bscale;
                scalechosen = ascale;
                digitchosen = a;
                nchosen = an;
            }
            memset(t, 0, tn * sizeof(DWORD));
            memcpy(t, digitchosen, nchosen * sizeof(DWORD));

            /* divide by 10 until target scale is reached */
            while (*scalechosen > targetscale) {
                unsigned char remainder = VARIANT_int_divbychar(t, tn, 10);
                if (!remainder) {
                    (*scalechosen)--;
                    memcpy(digitchosen, t, nchosen * sizeof(DWORD));
                } else break;
            }
        }

        if (*ascale != *bscale) {
            DWORD * digitchosen;
            unsigned int nchosen;
            int * scalechosen;
            int targetscale;

            /* try to scale up the one with the smaller scale */
            if (*ascale > *bscale) {
                targetscale = *ascale;
                scalechosen = bscale;
                digitchosen = b;
                nchosen = bn;
            } else {
                targetscale = *bscale;
                scalechosen = ascale;
                digitchosen = a;
                nchosen = an;
            }
            memset(t, 0, tn * sizeof(DWORD));
            memcpy(t, digitchosen, nchosen * sizeof(DWORD));

            /* multiply by 10 until target scale is reached, or
               significant bytes overflow the number
             */
            while (*scalechosen < targetscale && t[nchosen] == 0) {
                VARIANT_int_mulbychar(t, tn, 10);
                if (t[nchosen] == 0) {
                    /* still does not overflow */
                    (*scalechosen)++;
                    memcpy(digitchosen, t, nchosen * sizeof(DWORD));
                }
            }
        }

        if (*ascale != *bscale) {
            /* still different? try to scale down the one with the bigger scale
               (this *will* lose significant digits) */
            DWORD * digitchosen;
            unsigned int nchosen;
            int * scalechosen;
            int targetscale;

            if (*ascale < *bscale) {
                targetscale = *ascale;
                scalechosen = bscale;
                digitchosen = b;
                nchosen = bn;
            } else {
                targetscale = *bscale;
                scalechosen = ascale;
                digitchosen = a;
                nchosen = an;
            }
            memset(t, 0, tn * sizeof(DWORD));
            memcpy(t, digitchosen, nchosen * sizeof(DWORD));

            /* divide by 10 until target scale is reached */
            while (*scalechosen > targetscale) {
                VARIANT_int_divbychar(t, tn, 10);
                (*scalechosen)--;
                memcpy(digitchosen, t, nchosen * sizeof(DWORD));
            }
        }

        /* check whether any of the operands still has significant digits
           (underflow case 1)
         */
        if (VARIANT_int_iszero(a, an) || VARIANT_int_iszero(b, bn)) {
            underflow = 1;
        } else {
            /* at this step, both numbers have the same scale and can be added
               as integers. However, the result might not fit in A, so further
               scaling down might be necessary.
             */
            while (!underflow) {
                memset(t, 0, tn * sizeof(DWORD));
                memcpy(t, a, an * sizeof(DWORD));

                VARIANT_int_add(t, tn, b, bn);
                if (VARIANT_int_iszero(t + an, tn - an)) {
                    /* addition was successful */
                    memcpy(a, t, an * sizeof(DWORD));
                    break;
                } else {
                    /* addition overflowed - remove significant digits
                       from both operands and try again */
                    VARIANT_int_divbychar(a, an, 10); (*ascale)--;
                    VARIANT_int_divbychar(b, bn, 10); (*bscale)--;
                    /* check whether any operand keeps significant digits after
                       scaledown (underflow case 2)
                     */
                    underflow = (VARIANT_int_iszero(a, an) || VARIANT_int_iszero(b, bn));
                }
            }
        }
    }
    return underflow;
}

/* perform complete DECIMAL division in the internal representation. Returns
   0 if the division was completed (even if quotient is set to 0), or nonzero
   in case of quotient overflow.
 */
static HRESULT VARIANT_DI_div(const VARIANT_DI * dividend, const VARIANT_DI * divisor,
                              VARIANT_DI * quotient, BOOL round_remainder)
{
    HRESULT r_overflow = S_OK;

    if (VARIANT_int_iszero(divisor->bitsnum, sizeof(divisor->bitsnum)/sizeof(DWORD))) {
        /* division by 0 */
        r_overflow = DISP_E_DIVBYZERO;
    } else if (VARIANT_int_iszero(dividend->bitsnum, sizeof(dividend->bitsnum)/sizeof(DWORD))) {
        VARIANT_DI_clear(quotient);
    } else {
        int quotientscale, remainderscale, tempquotientscale;
        DWORD remainderplusquotient[8];
        int underflow;

        quotientscale = remainderscale = (int)dividend->scale - (int)divisor->scale;
        tempquotientscale = quotientscale;
        VARIANT_DI_clear(quotient);
        quotient->sign = (dividend->sign ^ divisor->sign) ? 1 : 0;

        /*  The following strategy is used for division
            1) if there was a nonzero remainder from previous iteration, use it as
               dividend for this iteration, else (for first iteration) use intended
               dividend
            2) perform integer division in temporary buffer, develop quotient in
               low-order part, remainder in high-order part
            3) add quotient from step 2 to final result, with possible loss of
               significant digits
            4) multiply integer part of remainder by 10, while incrementing the
               scale of the remainder. This operation preserves the intended value
               of the remainder.
            5) loop to step 1 until one of the following is true:
                a) remainder is zero (exact division achieved)
                b) addition in step 3 fails to modify bits in quotient (remainder underflow)
         */
        memset(remainderplusquotient, 0, sizeof(remainderplusquotient));
        memcpy(remainderplusquotient, dividend->bitsnum, sizeof(dividend->bitsnum));
        do {
            VARIANT_int_div(
                remainderplusquotient, 4,
                divisor->bitsnum, sizeof(divisor->bitsnum)/sizeof(DWORD));
            underflow = VARIANT_int_addlossy(
                quotient->bitsnum, &quotientscale, sizeof(quotient->bitsnum) / sizeof(DWORD),
                remainderplusquotient, &tempquotientscale, 4);
            if (round_remainder) {
                if(remainderplusquotient[4] >= 5){
                    unsigned int i;
                    unsigned char remainder = 1;
                    for (i = 0; i < sizeof(quotient->bitsnum) / sizeof(DWORD) && remainder; i++) {
                        ULONGLONG digit = quotient->bitsnum[i] + 1;
                        remainder = (digit > 0xFFFFFFFF) ? 1 : 0;
                        quotient->bitsnum[i] = digit & 0xFFFFFFFF;
                    }
                }
                memset(remainderplusquotient, 0, sizeof(remainderplusquotient));
            } else {
                VARIANT_int_mulbychar(remainderplusquotient + 4, 4, 10);
                memcpy(remainderplusquotient, remainderplusquotient + 4, 4 * sizeof(DWORD));
            }
            tempquotientscale = ++remainderscale;
        } while (!underflow && !VARIANT_int_iszero(remainderplusquotient + 4, 4));

        /* quotient scale might now be negative (extremely big number). If, so, try
           to multiply quotient by 10 (without overflowing), while adjusting the scale,
           until scale is 0. If this cannot be done, it is a real overflow.
         */
        while (r_overflow == S_OK && quotientscale < 0) {
            memset(remainderplusquotient, 0, sizeof(remainderplusquotient));
            memcpy(remainderplusquotient, quotient->bitsnum, sizeof(quotient->bitsnum));
            VARIANT_int_mulbychar(remainderplusquotient, sizeof(remainderplusquotient)/sizeof(DWORD), 10);
            if (VARIANT_int_iszero(remainderplusquotient + sizeof(quotient->bitsnum)/sizeof(DWORD),
                (sizeof(remainderplusquotient) - sizeof(quotient->bitsnum))/sizeof(DWORD))) {
                quotientscale++;
                memcpy(quotient->bitsnum, remainderplusquotient, sizeof(quotient->bitsnum));
            } else r_overflow = DISP_E_OVERFLOW;
        }
        if (r_overflow == S_OK) {
            if (quotientscale <= 255) quotient->scale = quotientscale;
            else VARIANT_DI_clear(quotient);
        }
    }
    return r_overflow;
}

/* This procedure receives a VARIANT_DI with a defined mantissa and sign, but
   with an undefined scale, which will be assigned to (if possible). It also
   receives an exponent of 2. This procedure will then manipulate the mantissa
   and calculate a corresponding scale, so that the exponent2 value is assimilated
   into the VARIANT_DI and is therefore no longer necessary. Returns S_OK if
   successful, or DISP_E_OVERFLOW if the represented value is too big to fit into
   a DECIMAL. */
static HRESULT VARIANT_DI_normalize(VARIANT_DI * val, int exponent2, BOOL isDouble)
{
    HRESULT hres = S_OK;
    int exponent5, exponent10;

    /* A factor of 2^exponent2 is equivalent to (10^exponent2)/(5^exponent2), and
       thus equal to (5^-exponent2)*(10^exponent2). After all manipulations,
       exponent10 might be used to set the VARIANT_DI scale directly. However,
       the value of 5^-exponent5 must be assimilated into the VARIANT_DI. */
    exponent5 = -exponent2;
    exponent10 = exponent2;

    /* Handle exponent5 > 0 */
    while (exponent5 > 0) {
        char bPrevCarryBit;
        char bCurrCarryBit;

        /* In order to multiply the value represented by the VARIANT_DI by 5, it
           is best to multiply by 10/2. Therefore, exponent10 is incremented, and
           somehow the mantissa should be divided by 2.  */
        if ((val->bitsnum[0] & 1) == 0) {
            /* The mantissa is divisible by 2. Therefore the division can be done
               without losing significant digits. */
            exponent10++; exponent5--;

            /* Shift right */
            bPrevCarryBit = val->bitsnum[2] & 1;
            val->bitsnum[2] >>= 1;
            bCurrCarryBit = val->bitsnum[1] & 1;
            val->bitsnum[1] = (val->bitsnum[1] >> 1) | (bPrevCarryBit ? 0x80000000 : 0);
            val->bitsnum[0] = (val->bitsnum[0] >> 1) | (bCurrCarryBit ? 0x80000000 : 0);
        } else {
            /* The mantissa is NOT divisible by 2. Therefore the mantissa should
               be multiplied by 5, unless the multiplication overflows. */
            DWORD temp_bitsnum[3];

            exponent5--;

            memcpy(temp_bitsnum, val->bitsnum, 3 * sizeof(DWORD));
            if (0 == VARIANT_int_mulbychar(temp_bitsnum, 3, 5)) {
                /* Multiplication succeeded without overflow, so copy result back
                   into VARIANT_DI */
                memcpy(val->bitsnum, temp_bitsnum, 3 * sizeof(DWORD));

                /* Mask out 3 extraneous bits introduced by the multiply */
            } else {
                /* Multiplication by 5 overflows. The mantissa should be divided
                   by 2, and therefore will lose significant digits. */
                exponent10++;

                /* Shift right */
                bPrevCarryBit = val->bitsnum[2] & 1;
                val->bitsnum[2] >>= 1;
                bCurrCarryBit = val->bitsnum[1] & 1;
                val->bitsnum[1] = (val->bitsnum[1] >> 1) | (bPrevCarryBit ? 0x80000000 : 0);
                val->bitsnum[0] = (val->bitsnum[0] >> 1) | (bCurrCarryBit ? 0x80000000 : 0);
            }
        }
    }

    /* Handle exponent5 < 0 */
    while (exponent5 < 0) {
        /* In order to divide the value represented by the VARIANT_DI by 5, it
           is best to multiply by 2/10. Therefore, exponent10 is decremented,
           and the mantissa should be multiplied by 2 */
        if ((val->bitsnum[2] & 0x80000000) == 0) {
            /* The mantissa can withstand a shift-left without overflowing */
            exponent10--; exponent5++;
            VARIANT_int_shiftleft(val->bitsnum, 3, 1);
        } else {
            /* The mantissa would overflow if shifted. Therefore it should be
               directly divided by 5. This will lose significant digits, unless
               by chance the mantissa happens to be divisible by 5 */
            exponent5++;
            VARIANT_int_divbychar(val->bitsnum, 3, 5);
        }
    }

    /* At this point, the mantissa has assimilated the exponent5, but the
       exponent10 might not be suitable for assignment. The exponent10 must be
       in the range [-DEC_MAX_SCALE..0], so the mantissa must be scaled up or
       down appropriately. */
    while (hres == S_OK && exponent10 > 0) {
        /* In order to bring exponent10 down to 0, the mantissa should be
           multiplied by 10 to compensate. If the exponent10 is too big, this
           will cause the mantissa to overflow. */
        if (0 == VARIANT_int_mulbychar(val->bitsnum, 3, 10)) {
            exponent10--;
        } else {
            hres = DISP_E_OVERFLOW;
        }
    }
    while (exponent10 < -DEC_MAX_SCALE) {
        int rem10;
        /* In order to bring exponent up to -DEC_MAX_SCALE, the mantissa should
           be divided by 10 to compensate. If the exponent10 is too small, this
           will cause the mantissa to underflow and become 0 */
        rem10 = VARIANT_int_divbychar(val->bitsnum, 3, 10);
        exponent10++;
        if (VARIANT_int_iszero(val->bitsnum, 3)) {
            /* Underflow, unable to keep dividing */
            exponent10 = 0;
        } else if (rem10 >= 5) {
            DWORD x = 1;
            VARIANT_int_add(val->bitsnum, 3, &x, 1);
        }
    }
    /* This step is required in order to remove excess bits of precision from the
       end of the bit representation, down to the precision guaranteed by the
       floating point number. */
    if (isDouble) {
        while (exponent10 < 0 && (val->bitsnum[2] != 0 || (val->bitsnum[2] == 0 && (val->bitsnum[1] & 0xFFE00000) != 0))) {
            int rem10;

            rem10 = VARIANT_int_divbychar(val->bitsnum, 3, 10);
            exponent10++;
            if (rem10 >= 5) {
                DWORD x = 1;
                VARIANT_int_add(val->bitsnum, 3, &x, 1);
            }
        }
    } else {
        while (exponent10 < 0 && (val->bitsnum[2] != 0 || val->bitsnum[1] != 0 ||
            (val->bitsnum[2] == 0 && val->bitsnum[1] == 0 && (val->bitsnum[0] & 0xFF000000) != 0))) {
            int rem10;

            rem10 = VARIANT_int_divbychar(val->bitsnum, 3, 10);
            exponent10++;
            if (rem10 >= 5) {
                DWORD x = 1;
                VARIANT_int_add(val->bitsnum, 3, &x, 1);
            }
        }
    }
    /* Remove multiples of 10 from the representation */
    while (exponent10 < 0) {
        DWORD temp_bitsnum[3];

        memcpy(temp_bitsnum, val->bitsnum, 3 * sizeof(DWORD));
        if (0 == VARIANT_int_divbychar(temp_bitsnum, 3, 10)) {
            exponent10++;
            memcpy(val->bitsnum, temp_bitsnum, 3 * sizeof(DWORD));
        } else break;
    }

    /* Scale assignment */
    if (hres == S_OK) val->scale = -exponent10;

    return hres;
}

typedef union
{
    struct
    {
        unsigned int m : 23;
        unsigned int exp_bias : 8;
        unsigned int sign : 1;
    } i;
    float f;
} R4_FIELDS;

/* Convert a 32-bit floating point number into a DECIMAL, without using an
   intermediate string step. */
static HRESULT VARIANT_DI_FromR4(float source, VARIANT_DI * dest)
{
    HRESULT hres = S_OK;
    R4_FIELDS fx;

    fx.f = source;

    /* Detect special cases */
    if (fx.i.m == 0 && fx.i.exp_bias == 0) {
        /* Floating-point zero */
        VARIANT_DI_clear(dest);
    } else if (fx.i.m == 0  && fx.i.exp_bias == 0xFF) {
        /* Floating-point infinity */
        hres = DISP_E_OVERFLOW;
    } else if (fx.i.exp_bias == 0xFF) {
        /* Floating-point NaN */
        hres = DISP_E_BADVARTYPE;
    } else {
        int exponent2;
        VARIANT_DI_clear(dest);

        exponent2 = fx.i.exp_bias - 127;   /* Get unbiased exponent */
        dest->sign = fx.i.sign;             /* Sign is simply copied */

        /* Copy significant bits to VARIANT_DI mantissa */
        dest->bitsnum[0] = fx.i.m;
        dest->bitsnum[0] &= 0x007FFFFF;
        if (fx.i.exp_bias == 0) {
            /* Denormalized number - correct exponent */
            exponent2++;
        } else {
            /* Add hidden bit to mantissa */
            dest->bitsnum[0] |= 0x00800000;
        }

        /* The act of copying a FP mantissa as integer bits is equivalent to
           shifting left the mantissa 23 bits. The exponent2 is reduced to
           compensate. */
        exponent2 -= 23;

        hres = VARIANT_DI_normalize(dest, exponent2, FALSE);
    }

    return hres;
}

typedef union
{
    struct
    {
        unsigned int m_lo : 32;     /* 52 bits of precision */
        unsigned int m_hi : 20;
        unsigned int exp_bias : 11; /* bias == 1023 */
        unsigned int sign : 1;
    } i;
    double d;
} R8_FIELDS;

/* Convert a 64-bit floating point number into a DECIMAL, without using an
   intermediate string step. */
static HRESULT VARIANT_DI_FromR8(double source, VARIANT_DI * dest)
{
    HRESULT hres = S_OK;
    R8_FIELDS fx;

    fx.d = source;

    /* Detect special cases */
    if (fx.i.m_lo == 0 && fx.i.m_hi == 0 && fx.i.exp_bias == 0) {
        /* Floating-point zero */
        VARIANT_DI_clear(dest);
    } else if (fx.i.m_lo == 0 && fx.i.m_hi == 0 && fx.i.exp_bias == 0x7FF) {
        /* Floating-point infinity */
        hres = DISP_E_OVERFLOW;
    } else if (fx.i.exp_bias == 0x7FF) {
        /* Floating-point NaN */
        hres = DISP_E_BADVARTYPE;
    } else {
        int exponent2;
        VARIANT_DI_clear(dest);

        exponent2 = fx.i.exp_bias - 1023;   /* Get unbiased exponent */
        dest->sign = fx.i.sign;             /* Sign is simply copied */

        /* Copy significant bits to VARIANT_DI mantissa */
        dest->bitsnum[0] = fx.i.m_lo;
        dest->bitsnum[1] = fx.i.m_hi;
        dest->bitsnum[1] &= 0x000FFFFF;
        if (fx.i.exp_bias == 0) {
            /* Denormalized number - correct exponent */
            exponent2++;
        } else {
            /* Add hidden bit to mantissa */
            dest->bitsnum[1] |= 0x00100000;
        }

        /* The act of copying a FP mantissa as integer bits is equivalent to
           shifting left the mantissa 52 bits. The exponent2 is reduced to
           compensate. */
        exponent2 -= 52;

        hres = VARIANT_DI_normalize(dest, exponent2, TRUE);
    }

    return hres;
}

static HRESULT VARIANT_do_division(const DECIMAL *pDecLeft, const DECIMAL *pDecRight, DECIMAL *pDecOut,
        BOOL round)
{
  HRESULT hRet = S_OK;
  VARIANT_DI di_left, di_right, di_result;
  HRESULT divresult;

  VARIANT_DIFromDec(pDecLeft, &di_left);
  VARIANT_DIFromDec(pDecRight, &di_right);
  divresult = VARIANT_DI_div(&di_left, &di_right, &di_result, round);
  if (divresult != S_OK)
  {
      /* division actually overflowed */
      hRet = divresult;
  }
  else
  {
      hRet = S_OK;

      if (di_result.scale > DEC_MAX_SCALE)
      {
        unsigned char remainder = 0;
      
        /* division underflowed. In order to comply with the MSDN
           specifications for DECIMAL ranges, some significant digits
           must be removed
         */
        WARN("result scale is %u, scaling (with loss of significant digits)...\n",
            di_result.scale);
        while (di_result.scale > DEC_MAX_SCALE && 
               !VARIANT_int_iszero(di_result.bitsnum, sizeof(di_result.bitsnum) / sizeof(DWORD)))
        {
            remainder = VARIANT_int_divbychar(di_result.bitsnum, sizeof(di_result.bitsnum) / sizeof(DWORD), 10);
            di_result.scale--;
        }
        if (di_result.scale > DEC_MAX_SCALE)
        {
            WARN("result underflowed, setting to 0\n");
            di_result.scale = 0;
            di_result.sign = 0;
        }
        else if (remainder >= 5)    /* round up result - native oleaut32 does this */
        {
            unsigned int i;
            for (remainder = 1, i = 0; i < sizeof(di_result.bitsnum) / sizeof(DWORD) && remainder; i++) {
                ULONGLONG digit = di_result.bitsnum[i] + 1;
                remainder = (digit > 0xFFFFFFFF) ? 1 : 0;
                di_result.bitsnum[i] = digit & 0xFFFFFFFF;
            }
        }
      }
      VARIANT_DecFromDI(&di_result, pDecOut);
  }
  return hRet;
}

/************************************************************************
 * VarDecDiv (OLEAUT32.178)
 *
 * Divide one DECIMAL by another.
 *
 * PARAMS
 *  pDecLeft  [I] Source
 *  pDecRight [I] Value to divide by
 *  pDecOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDecDiv(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  if (!pDecLeft || !pDecRight || !pDecOut) return E_INVALIDARG;

  return VARIANT_do_division(pDecLeft, pDecRight, pDecOut, FALSE);
}

/************************************************************************
 * VarDecMul (OLEAUT32.179)
 *
 * Multiply one DECIMAL by another.
 *
 * PARAMS
 *  pDecLeft  [I] Source
 *  pDecRight [I] Value to multiply by
 *  pDecOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDecMul(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  HRESULT hRet = S_OK;
  VARIANT_DI di_left, di_right, di_result;
  int mulresult;

  VARIANT_DIFromDec(pDecLeft, &di_left);
  VARIANT_DIFromDec(pDecRight, &di_right);
  mulresult = VARIANT_DI_mul(&di_left, &di_right, &di_result);
  if (mulresult)
  {
    /* multiplication actually overflowed */
    hRet = DISP_E_OVERFLOW;
  }
  else
  {
    if (di_result.scale > DEC_MAX_SCALE)
    {
      /* multiplication underflowed. In order to comply with the MSDN
         specifications for DECIMAL ranges, some significant digits
         must be removed
       */
      WARN("result scale is %u, scaling (with loss of significant digits)...\n",
          di_result.scale);
      while (di_result.scale > DEC_MAX_SCALE && 
            !VARIANT_int_iszero(di_result.bitsnum, sizeof(di_result.bitsnum)/sizeof(DWORD)))
      {
        VARIANT_int_divbychar(di_result.bitsnum, sizeof(di_result.bitsnum)/sizeof(DWORD), 10);
        di_result.scale--;
      }
      if (di_result.scale > DEC_MAX_SCALE)
      {
        WARN("result underflowed, setting to 0\n");
        di_result.scale = 0;
        di_result.sign = 0;
      }
    }
    VARIANT_DecFromDI(&di_result, pDecOut);
  }
  return hRet;
}

/************************************************************************
 * VarDecSub (OLEAUT32.181)
 *
 * Subtract one DECIMAL from another.
 *
 * PARAMS
 *  pDecLeft  [I] Source
 *  pDecRight [I] DECIMAL to subtract from pDecLeft
 *  pDecOut   [O] Destination
 *
 * RETURNS
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDecSub(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  DECIMAL decRight;

  /* Implement as addition of the negative */
  VarDecNeg(pDecRight, &decRight);
  return VarDecAdd(pDecLeft, &decRight, pDecOut);
}

/************************************************************************
 * VarDecAbs (OLEAUT32.182)
 *
 * Convert a DECIMAL into its absolute value.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK. This function does not fail.
 */
HRESULT WINAPI VarDecAbs(const DECIMAL* pDecIn, DECIMAL* pDecOut)
{
  *pDecOut = *pDecIn;
  DEC_SIGN(pDecOut) &= ~DECIMAL_NEG;
  return S_OK;
}

/************************************************************************
 * VarDecFix (OLEAUT32.187)
 *
 * Return the integer portion of a DECIMAL.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  - The difference between this function and VarDecInt() is that VarDecInt() rounds
 *    negative numbers away from 0, while this function rounds them towards zero.
 */
HRESULT WINAPI VarDecFix(const DECIMAL* pDecIn, DECIMAL* pDecOut)
{
  double dbl;
  HRESULT hr;

  if (DEC_SIGN(pDecIn) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  if (!DEC_SCALE(pDecIn))
  {
    *pDecOut = *pDecIn; /* Already an integer */
    return S_OK;
  }

  hr = VarR8FromDec(pDecIn, &dbl);
  if (SUCCEEDED(hr)) {
    LONGLONG rounded = dbl;

    hr = VarDecFromI8(rounded, pDecOut);
  }
  return hr;
}

/************************************************************************
 * VarDecInt (OLEAUT32.188)
 *
 * Return the integer portion of a DECIMAL.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 *
 * NOTES
 *  - The difference between this function and VarDecFix() is that VarDecFix() rounds
 *    negative numbers towards 0, while this function rounds them away from zero.
 */
HRESULT WINAPI VarDecInt(const DECIMAL* pDecIn, DECIMAL* pDecOut)
{
  double dbl;
  HRESULT hr;

  if (DEC_SIGN(pDecIn) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  if (!(DEC_SIGN(pDecIn) & DECIMAL_NEG) || !DEC_SCALE(pDecIn))
    return VarDecFix(pDecIn, pDecOut); /* The same, if +ve or no fractionals */

  hr = VarR8FromDec(pDecIn, &dbl);
  if (SUCCEEDED(hr)) {
    LONGLONG rounded = dbl >= 0.0 ? dbl + 0.5 : dbl - 0.5;

    hr = VarDecFromI8(rounded, pDecOut);
  }
  return hr;
}

/************************************************************************
 * VarDecNeg (OLEAUT32.189)
 *
 * Change the sign of a DECIMAL.
 *
 * PARAMS
 *  pDecIn  [I] Source
 *  pDecOut [O] Destination
 *
 * RETURNS
 *  S_OK. This function does not fail.
 */
HRESULT WINAPI VarDecNeg(const DECIMAL* pDecIn, DECIMAL* pDecOut)
{
  *pDecOut = *pDecIn;
  DEC_SIGN(pDecOut) ^= DECIMAL_NEG;
  return S_OK;
}

/************************************************************************
 * VarDecRound (OLEAUT32.203)
 *
 * Change the precision of a DECIMAL.
 *
 * PARAMS
 *  pDecIn    [I] Source
 *  cDecimals [I] New number of decimals to keep
 *  pDecOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK. pDecOut contains the rounded value.
 *  Failure: E_INVALIDARG if any argument is invalid.
 */
HRESULT WINAPI VarDecRound(const DECIMAL* pDecIn, int cDecimals, DECIMAL* pDecOut)
{
  DECIMAL divisor, tmp;
  HRESULT hr;
  unsigned int i;

  if (cDecimals < 0 || (DEC_SIGN(pDecIn) & ~DECIMAL_NEG) || DEC_SCALE(pDecIn) > DEC_MAX_SCALE)
    return E_INVALIDARG;

  if (cDecimals >= DEC_SCALE(pDecIn))
  {
    *pDecOut = *pDecIn; /* More precision than we have */
    return S_OK;
  }

  /* truncate significant digits and rescale */
  memset(&divisor, 0, sizeof(divisor));
  DEC_LO64(&divisor) = 1;

  memset(&tmp, 0, sizeof(tmp));
  DEC_LO64(&tmp) = 10;
  for (i = 0; i < DEC_SCALE(pDecIn) - cDecimals; ++i)
  {
    hr = VarDecMul(&divisor, &tmp, &divisor);
    if (FAILED(hr))
      return hr;
  }

  hr = VARIANT_do_division(pDecIn, &divisor, pDecOut, TRUE);
  if (FAILED(hr))
    return hr;

  DEC_SCALE(pDecOut) = cDecimals;

  return S_OK;
}

/************************************************************************
 * VarDecCmp (OLEAUT32.204)
 *
 * Compare two DECIMAL values.
 *
 * PARAMS
 *  pDecLeft  [I] Source
 *  pDecRight [I] Value to compare
 *
 * RETURNS
 *  Success: VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that pDecLeft
 *           is less than, equal to or greater than pDecRight respectively.
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparison
 */
HRESULT WINAPI VarDecCmp(const DECIMAL* pDecLeft, const DECIMAL* pDecRight)
{
  HRESULT hRet;
  DECIMAL result;

  if (!pDecLeft || !pDecRight)
    return VARCMP_NULL;

  if ((!(DEC_SIGN(pDecLeft) & DECIMAL_NEG)) && (DEC_SIGN(pDecRight) & DECIMAL_NEG) &&
      (DEC_HI32(pDecLeft) | DEC_MID32(pDecLeft) | DEC_LO32(pDecLeft)))
    return VARCMP_GT;
  else if ((DEC_SIGN(pDecLeft) & DECIMAL_NEG) && (!(DEC_SIGN(pDecRight) & DECIMAL_NEG)) &&
      (DEC_HI32(pDecLeft) | DEC_MID32(pDecLeft) | DEC_LO32(pDecLeft)))
    return VARCMP_LT;

  /* Subtract right from left, and compare the result to 0 */
  hRet = VarDecSub(pDecLeft, pDecRight, &result);

  if (SUCCEEDED(hRet))
  {
    int non_zero = DEC_HI32(&result) | DEC_MID32(&result) | DEC_LO32(&result);

    if ((DEC_SIGN(&result) & DECIMAL_NEG) && non_zero)
      hRet = (HRESULT)VARCMP_LT;
    else if (non_zero)
      hRet = (HRESULT)VARCMP_GT;
    else
      hRet = (HRESULT)VARCMP_EQ;
  }
  return hRet;
}

/************************************************************************
 * VarDecCmpR8 (OLEAUT32.298)
 *
 * Compare a DECIMAL to a double
 *
 * PARAMS
 *  pDecLeft [I] DECIMAL Source
 *  dblRight [I] double to compare to pDecLeft
 *
 * RETURNS
 *  Success: VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that dblRight
 *           is less than, equal to or greater than pDecLeft respectively.
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparison
 */
HRESULT WINAPI VarDecCmpR8(const DECIMAL* pDecLeft, double dblRight)
{
  HRESULT hRet;
  DECIMAL decRight;

  hRet = VarDecFromR8(dblRight, &decRight);

  if (SUCCEEDED(hRet))
    hRet = VarDecCmp(pDecLeft, &decRight);

  return hRet;
}

/* BOOL
 */

/************************************************************************
 * VarBoolFromUI1 (OLEAUT32.118)
 *
 * Convert a VT_UI1 to a VT_BOOL.
 *
 * PARAMS
 *  bIn      [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromUI1(BYTE bIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = bIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromI2 (OLEAUT32.119)
 *
 * Convert a VT_I2 to a VT_BOOL.
 *
 * PARAMS
 *  sIn      [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromI2(SHORT sIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = sIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromI4 (OLEAUT32.120)
 *
 * Convert a VT_I4 to a VT_BOOL.
 *
 * PARAMS
 *  sIn      [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromI4(LONG lIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = lIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromR4 (OLEAUT32.121)
 *
 * Convert a VT_R4 to a VT_BOOL.
 *
 * PARAMS
 *  fltIn    [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromR4(FLOAT fltIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = fltIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromR8 (OLEAUT32.122)
 *
 * Convert a VT_R8 to a VT_BOOL.
 *
 * PARAMS
 *  dblIn    [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromR8(double dblIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = dblIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromDate (OLEAUT32.123)
 *
 * Convert a VT_DATE to a VT_BOOL.
 *
 * PARAMS
 *  dateIn   [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromDate(DATE dateIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = dateIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromCy (OLEAUT32.124)
 *
 * Convert a VT_CY to a VT_BOOL.
 *
 * PARAMS
 *  cyIn     [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromCy(CY cyIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = cyIn.int64 ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VARIANT_GetLocalisedText [internal]
 *
 * Get a localized string from the resources
 *
 */
BOOL VARIANT_GetLocalisedText(LANGID langId, DWORD dwId, WCHAR *lpszDest)
{
  HRSRC hrsrc;

  hrsrc = FindResourceExW( hProxyDll, (LPWSTR)RT_STRING,
                           MAKEINTRESOURCEW((dwId >> 4) + 1), langId );
  if (hrsrc)
  {
    HGLOBAL hmem = LoadResource( hProxyDll, hrsrc );

    if (hmem)
    {
      const WCHAR *p;
      unsigned int i;

      p = LockResource( hmem );
      for (i = 0; i < (dwId & 0x0f); i++) p += *p + 1;

      memcpy( lpszDest, p + 1, *p * sizeof(WCHAR) );
      lpszDest[*p] = '\0';
      TRACE("got %s for LANGID %08x\n", debugstr_w(lpszDest), langId);
      return TRUE;
    }
  }
  return FALSE;
}

/************************************************************************
 * VarBoolFromStr (OLEAUT32.125)
 *
 * Convert a VT_BSTR to a VT_BOOL.
 *
 * PARAMS
 *  strIn    [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pBoolOut is invalid.
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 *
 * NOTES
 *  - strIn will be recognised if it contains "#TRUE#" or "#FALSE#". Additionally,
 *  it may contain (in any case mapping) the text "true" or "false".
 *  - If dwFlags includes VAR_LOCALBOOL, then the text may also match the
 *  localised text of "True" or "False" in the language specified by lcid.
 *  - If none of these matches occur, the string is treated as a numeric string
 *  and the boolean pBoolOut will be set according to whether the number is zero
 *  or not. The dwFlags parameter is passed to VarR8FromStr() for this conversion.
 *  - If the text is not numeric and does not match any of the above, then
 *  DISP_E_TYPEMISMATCH is returned.
 */
HRESULT WINAPI VarBoolFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL *pBoolOut)
{
  /* Any VB/VBA programmers out there should recognise these strings... */
  static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
  static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
  WCHAR szBuff[64];
  LANGID langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
  HRESULT hRes = S_OK;

  if (!strIn || !pBoolOut)
    return DISP_E_TYPEMISMATCH;

  /* Check if we should be comparing against localised text */
  if (dwFlags & VAR_LOCALBOOL)
  {
    /* Convert our LCID into a usable value */
    lcid = ConvertDefaultLocale(lcid);

    langId = LANGIDFROMLCID(lcid);

    if (PRIMARYLANGID(langId) == LANG_NEUTRAL)
      langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

    /* Note: Native oleaut32 always copies strIn and maps halfwidth characters.
     * I don't think this is needed unless any of the localised text strings
     * contain characters that can be so mapped. In the event that this is
     * true for a given language (possibly some Asian languages), then strIn
     * should be mapped here _only_ if langId is an Id for which this can occur.
     */
  }

  /* Note that if we are not comparing against localised strings, langId
   * will have its default value of LANG_ENGLISH. This allows us to mimic
   * the native behaviour of always checking against English strings even
   * after we've checked for localised ones.
   */
VarBoolFromStr_CheckLocalised:
  if (VARIANT_GetLocalisedText(langId, IDS_TRUE, szBuff))
  {
    /* Compare against localised strings, ignoring case */
    if (!strcmpiW(strIn, szBuff))
    {
      *pBoolOut = VARIANT_TRUE; /* Matched localised 'true' text */
      return hRes;
    }
    VARIANT_GetLocalisedText(langId, IDS_FALSE, szBuff);
    if (!strcmpiW(strIn, szBuff))
    {
      *pBoolOut = VARIANT_FALSE; /* Matched localised 'false' text */
      return hRes;
    }
  }

  if (langId != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
  {
    /* We have checked the localised text, now check English */
    langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    goto VarBoolFromStr_CheckLocalised;
  }

  /* All checks against localised text have failed, try #TRUE#/#FALSE# */
  if (!strcmpW(strIn, szFalse))
    *pBoolOut = VARIANT_FALSE;
  else if (!strcmpW(strIn, szTrue))
    *pBoolOut = VARIANT_TRUE;
  else
  {
    double d;

    /* If this string is a number, convert it as one */
    hRes = VarR8FromStr(strIn, lcid, dwFlags, &d);
    if (SUCCEEDED(hRes)) *pBoolOut = d ? VARIANT_TRUE : VARIANT_FALSE;
  }
  return hRes;
}

/************************************************************************
 * VarBoolFromDisp (OLEAUT32.126)
 *
 * Convert a VT_DISPATCH to a VT_BOOL.
 *
 * PARAMS
 *  pdispIn   [I] Source
 *  lcid      [I] LCID for conversion
 *  pBoolOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarBoolFromDisp(IDispatch* pdispIn, LCID lcid, VARIANT_BOOL *pBoolOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pBoolOut, VT_BOOL, 0);
}

/************************************************************************
 * VarBoolFromI1 (OLEAUT32.233)
 *
 * Convert a VT_I1 to a VT_BOOL.
 *
 * PARAMS
 *  cIn      [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromI1(signed char cIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = cIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromUI2 (OLEAUT32.234)
 *
 * Convert a VT_UI2 to a VT_BOOL.
 *
 * PARAMS
 *  usIn     [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromUI2(USHORT usIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = usIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromUI4 (OLEAUT32.235)
 *
 * Convert a VT_UI4 to a VT_BOOL.
 *
 * PARAMS
 *  ulIn     [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromUI4(ULONG ulIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = ulIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromDec (OLEAUT32.236)
 *
 * Convert a VT_DECIMAL to a VT_BOOL.
 *
 * PARAMS
 *  pDecIn   [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pDecIn is invalid.
 */
HRESULT WINAPI VarBoolFromDec(DECIMAL* pDecIn, VARIANT_BOOL *pBoolOut)
{
  if (DEC_SCALE(pDecIn) > DEC_MAX_SCALE || (DEC_SIGN(pDecIn) & ~DECIMAL_NEG))
    return E_INVALIDARG;

  if (DEC_HI32(pDecIn) || DEC_MID32(pDecIn) || DEC_LO32(pDecIn))
    *pBoolOut = VARIANT_TRUE;
  else
    *pBoolOut = VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromI8 (OLEAUT32.370)
 *
 * Convert a VT_I8 to a VT_BOOL.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromI8(LONG64 llIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = llIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/************************************************************************
 * VarBoolFromUI8 (OLEAUT32.371)
 *
 * Convert a VT_UI8 to a VT_BOOL.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  pBoolOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarBoolFromUI8(ULONG64 ullIn, VARIANT_BOOL *pBoolOut)
{
  *pBoolOut = ullIn ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

/* BSTR
 */

/* Write a number from a UI8 and sign */
static WCHAR *VARIANT_WriteNumber(ULONG64 ulVal, WCHAR* szOut)
{
  do
  {
    WCHAR ulNextDigit = ulVal % 10;

    *szOut-- = '0' + ulNextDigit;
    ulVal = (ulVal - ulNextDigit) / 10;
  } while (ulVal);

  szOut++;
  return szOut;
}

/* Create a (possibly localised) BSTR from a UI8 and sign */
static BSTR VARIANT_MakeBstr(LCID lcid, DWORD dwFlags, WCHAR *szOut)
{
  WCHAR szConverted[256];

  if (dwFlags & VAR_NEGATIVE)
    *--szOut = '-';

  if (dwFlags & LOCALE_USE_NLS)
  {
    /* Format the number for the locale */
    szConverted[0] = '\0';
    GetNumberFormatW(lcid,
                     dwFlags & LOCALE_NOUSEROVERRIDE,
                     szOut, NULL, szConverted, sizeof(szConverted)/sizeof(WCHAR));
    szOut = szConverted;
  }
  return SysAllocStringByteLen((LPCSTR)szOut, strlenW(szOut) * sizeof(WCHAR));
}

/* Create a (possibly localised) BSTR from a UI8 and sign */
static HRESULT VARIANT_BstrFromUInt(ULONG64 ulVal, LCID lcid, DWORD dwFlags, BSTR *pbstrOut)
{
  WCHAR szBuff[64], *szOut = szBuff + sizeof(szBuff)/sizeof(WCHAR) - 1;

  if (!pbstrOut)
    return E_INVALIDARG;

  /* Create the basic number string */
  *szOut-- = '\0';
  szOut = VARIANT_WriteNumber(ulVal, szOut);

  *pbstrOut = VARIANT_MakeBstr(lcid, dwFlags, szOut);
  TRACE("returning %s\n", debugstr_w(*pbstrOut));
  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

/******************************************************************************
 * VarBstrFromUI1 (OLEAUT32.108)
 *
 * Convert a VT_UI1 to a VT_BSTR.
 *
 * PARAMS
 *  bIn      [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromUI1(BYTE bIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromUInt(bIn, lcid, dwFlags, pbstrOut);
}

/******************************************************************************
 * VarBstrFromI2 (OLEAUT32.109)
 *
 * Convert a VT_I2 to a VT_BSTR.
 *
 * PARAMS
 *  sIn      [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromI2(short sIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  ULONG64 ul64 = sIn;

  if (sIn < 0)
  {
    ul64 = -sIn;
    dwFlags |= VAR_NEGATIVE;
  }
  return VARIANT_BstrFromUInt(ul64, lcid, dwFlags, pbstrOut);
}

/******************************************************************************
 * VarBstrFromI4 (OLEAUT32.110)
 *
 * Convert a VT_I4 to a VT_BSTR.
 *
 * PARAMS
 *  lIn      [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromI4(LONG lIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  ULONG64 ul64 = lIn;

  if (lIn < 0)
  {
    ul64 = (ULONG)-lIn;
    dwFlags |= VAR_NEGATIVE;
  }
  return VARIANT_BstrFromUInt(ul64, lcid, dwFlags, pbstrOut);
}

static BSTR VARIANT_BstrReplaceDecimal(const WCHAR * buff, LCID lcid, ULONG dwFlags)
{
  BSTR bstrOut;
  WCHAR lpDecimalSep[16];

  /* Native oleaut32 uses the locale-specific decimal separator even in the
     absence of the LOCALE_USE_NLS flag. For example, the Spanish/Latin 
     American locales will see "one thousand and one tenth" as "1000,1" 
     instead of "1000.1" (notice the comma). The following code checks for
     the need to replace the decimal separator, and if so, will prepare an
     appropriate NUMBERFMTW structure to do the job via GetNumberFormatW().
   */
  GetLocaleInfoW(lcid, LOCALE_SDECIMAL | (dwFlags & LOCALE_NOUSEROVERRIDE),
                 lpDecimalSep, sizeof(lpDecimalSep) / sizeof(WCHAR));
  if (lpDecimalSep[0] == '.' && lpDecimalSep[1] == '\0')
  {
    /* locale is compatible with English - return original string */
    bstrOut = SysAllocString(buff);
  }
  else
  {
    WCHAR *p;
    WCHAR numbuff[256];
    WCHAR empty[] = {'\0'};
    NUMBERFMTW minFormat;

    minFormat.NumDigits = 0;
    minFormat.LeadingZero = 0;
    minFormat.Grouping = 0;
    minFormat.lpDecimalSep = lpDecimalSep;
    minFormat.lpThousandSep = empty;
    minFormat.NegativeOrder = 1; /* NLS_NEG_LEFT */

    /* count number of decimal digits in string */
    p = strchrW( buff, '.' );
    if (p) minFormat.NumDigits = strlenW(p + 1);

    numbuff[0] = '\0';
    if (!GetNumberFormatW(lcid, 0, buff, &minFormat, numbuff, sizeof(numbuff) / sizeof(WCHAR)))
    {
      WARN("GetNumberFormatW() failed, returning raw number string instead\n");
      bstrOut = SysAllocString(buff);
    }
    else
    {
      TRACE("created minimal NLS string %s\n", debugstr_w(numbuff));
      bstrOut = SysAllocString(numbuff);
    }
  }
  return bstrOut;
}

static HRESULT VARIANT_BstrFromReal(DOUBLE dblIn, LCID lcid, ULONG dwFlags,
                                    BSTR* pbstrOut, LPCWSTR lpszFormat)
{
  WCHAR buff[256];

  if (!pbstrOut)
    return E_INVALIDARG;

  sprintfW( buff, lpszFormat, dblIn );

  /* Negative zeroes are disallowed (some applications depend on this).
     If buff starts with a minus, and then nothing follows but zeroes
     and/or a period, it is a negative zero and is replaced with a
     canonical zero. This duplicates native oleaut32 behavior.
   */
  if (buff[0] == '-')
  {
    const WCHAR szAccept[] = {'0', '.', '\0'};
    if (strlenW(buff + 1) == strspnW(buff + 1, szAccept))
    { buff[0] = '0'; buff[1] = '\0'; }
  }

  TRACE("created string %s\n", debugstr_w(buff));
  if (dwFlags & LOCALE_USE_NLS)
  {
    WCHAR numbuff[256];

    /* Format the number for the locale */
    numbuff[0] = '\0';
    GetNumberFormatW(lcid, dwFlags & LOCALE_NOUSEROVERRIDE,
                     buff, NULL, numbuff, sizeof(numbuff) / sizeof(WCHAR));
    TRACE("created NLS string %s\n", debugstr_w(numbuff));
    *pbstrOut = SysAllocString(numbuff);
  }
  else
  {
    *pbstrOut = VARIANT_BstrReplaceDecimal(buff, lcid, dwFlags);
  }
  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

/******************************************************************************
 * VarBstrFromR4 (OLEAUT32.111)
 *
 * Convert a VT_R4 to a VT_BSTR.
 *
 * PARAMS
 *  fltIn    [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromR4(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromReal(fltIn, lcid, dwFlags, pbstrOut, szFloatFormatW);
}

/******************************************************************************
 * VarBstrFromR8 (OLEAUT32.112)
 *
 * Convert a VT_R8 to a VT_BSTR.
 *
 * PARAMS
 *  dblIn    [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromR8(double dblIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromReal(dblIn, lcid, dwFlags, pbstrOut, szDoubleFormatW);
}

/******************************************************************************
 *    VarBstrFromCy   [OLEAUT32.113]
 *
 * Convert a VT_CY to a VT_BSTR.
 *
 * PARAMS
 *  cyIn     [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromCy(CY cyIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut)
{
  WCHAR buff[256];
  VARIANT_DI decVal;

  if (!pbstrOut)
    return E_INVALIDARG;

  decVal.scale = 4;
  decVal.sign = 0;
  decVal.bitsnum[0] = cyIn.s.Lo;
  decVal.bitsnum[1] = cyIn.s.Hi;
  if (cyIn.s.Hi & 0x80000000UL) {
    DWORD one = 1;

    /* Negative number! */
    decVal.sign = 1;
    decVal.bitsnum[0] = ~decVal.bitsnum[0];
    decVal.bitsnum[1] = ~decVal.bitsnum[1];
    VARIANT_int_add(decVal.bitsnum, 3, &one, 1);
  }
  decVal.bitsnum[2] = 0;
  VARIANT_DI_tostringW(&decVal, buff, sizeof(buff)/sizeof(buff[0]));

  if (dwFlags & LOCALE_USE_NLS)
  {
    WCHAR cybuff[256];

    /* Format the currency for the locale */
    cybuff[0] = '\0';
    GetCurrencyFormatW(lcid, dwFlags & LOCALE_NOUSEROVERRIDE,
                       buff, NULL, cybuff, sizeof(cybuff) / sizeof(WCHAR));
    *pbstrOut = SysAllocString(cybuff);
  }
  else
    *pbstrOut = VARIANT_BstrReplaceDecimal(buff,lcid,dwFlags);

  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

static inline int output_int_len(int o, int min_len, WCHAR *date, int date_len)
{
    int len, tmp;

    if(min_len >= date_len)
        return -1;

    for(len=0, tmp=o; tmp; tmp/=10) len++;
    if(!len) len++;
    if(len >= date_len)
        return -1;

    for(tmp=min_len-len; tmp>0; tmp--)
        *date++ = '0';
    for(tmp=len; tmp>0; tmp--, o/=10)
        date[tmp-1] = '0' + o%10;
    return min_len>len ? min_len : len;
}

/* format date string, similar to GetDateFormatW function but works on bigger range of dates */
BOOL get_date_format(LCID lcid, DWORD flags, const SYSTEMTIME *st,
        const WCHAR *fmt, WCHAR *date, int date_len)
{
    static const LCTYPE dayname[] = {
        LOCALE_SDAYNAME7, LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
        LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6
    };
    static const LCTYPE sdayname[] = {
        LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
        LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
        LOCALE_SABBREVDAYNAME6
    };
    static const LCTYPE monthname[] = {
        LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12
    };
    static const LCTYPE smonthname[] = {
        LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12
    };

    if(flags & ~(LOCALE_NOUSEROVERRIDE|VAR_DATEVALUEONLY))
        FIXME("ignoring flags %x\n", flags);
    flags &= LOCALE_NOUSEROVERRIDE;

    while(*fmt && date_len) {
        int count = 1;

        switch(*fmt) {
            case 'd':
            case 'M':
            case 'y':
            case 'g':
                while(*fmt == *(fmt+count))
                    count++;
                fmt += count-1;
        }

        switch(*fmt) {
        case 'd':
            if(count >= 4)
                count = GetLocaleInfoW(lcid, dayname[st->wDayOfWeek] | flags, date, date_len)-1;
            else if(count == 3)
                count = GetLocaleInfoW(lcid, sdayname[st->wDayOfWeek] | flags, date, date_len)-1;
            else
                count = output_int_len(st->wDay, count, date, date_len);
            break;
        case 'M':
            if(count >= 4)
                count = GetLocaleInfoW(lcid, monthname[st->wMonth-1] | flags, date, date_len)-1;
            else if(count == 3)
                count = GetLocaleInfoW(lcid, smonthname[st->wMonth-1] | flags, date, date_len)-1;
            else
                count = output_int_len(st->wMonth, count, date, date_len);
            break;
        case 'y':
            if(count >= 3)
                count = output_int_len(st->wYear, 0, date, date_len);
            else
                count = output_int_len(st->wYear%100, count, date, date_len);
            break;
        case 'g':
            if(count == 2) {
                FIXME("Should be using GetCalendarInfo(CAL_SERASTRING), defaulting to 'AD'\n");

                *date++ = 'A';
                date_len--;
                if(date_len)
                    *date = 'D';
                else
                    count = -1;
                break;
            }
        /* fall through */
        default:
            *date = *fmt;
        }

        if(count < 0)
            break;
        fmt++;
        date += count;
        date_len -= count;
    }

    if(!date_len)
        return FALSE;
    *date++ = 0;
    return TRUE;
}

/******************************************************************************
 *    VarBstrFromDate    [OLEAUT32.114]
 *
 * Convert a VT_DATE to a VT_BSTR.
 *
 * PARAMS
 *  dateIn   [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut or dateIn is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromDate(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  SYSTEMTIME st;
  DWORD dwFormatFlags = dwFlags & LOCALE_NOUSEROVERRIDE;
  WCHAR date[128], fmt_buff[80], *time;

  TRACE("(%g,0x%08x,0x%08x,%p)\n", dateIn, lcid, dwFlags, pbstrOut);

  if (!pbstrOut || !VariantTimeToSystemTime(dateIn, &st))
    return E_INVALIDARG;

  *pbstrOut = NULL;

  if (dwFlags & VAR_CALENDAR_THAI)
      st.wYear += 553; /* Use the Thai buddhist calendar year */
  else if (dwFlags & (VAR_CALENDAR_HIJRI|VAR_CALENDAR_GREGORIAN))
      FIXME("VAR_CALENDAR_HIJRI/VAR_CALENDAR_GREGORIAN not handled\n");

  if (dwFlags & LOCALE_USE_NLS)
    dwFlags &= ~(VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY);
  else
  {
    double whole = dateIn < 0 ? ceil(dateIn) : floor(dateIn);
    double partial = dateIn - whole;

    if (whole == 0.0)
      dwFlags |= VAR_TIMEVALUEONLY;
    else if (partial > -1e-12 && partial < 1e-12)
      dwFlags |= VAR_DATEVALUEONLY;
  }

  if (dwFlags & VAR_TIMEVALUEONLY)
    date[0] = '\0';
  else
    if (!GetLocaleInfoW(lcid, LOCALE_SSHORTDATE, fmt_buff, sizeof(fmt_buff)/sizeof(WCHAR)) ||
        !get_date_format(lcid, dwFlags, &st, fmt_buff, date, sizeof(date)/sizeof(WCHAR)))
      return E_INVALIDARG;

  if (!(dwFlags & VAR_DATEVALUEONLY))
  {
    time = date + strlenW(date);
    if (time != date)
      *time++ = ' ';
    if (!GetTimeFormatW(lcid, dwFormatFlags, &st, NULL, time,
                        sizeof(date)/sizeof(WCHAR)-(time-date)))
      return E_INVALIDARG;
  }

  *pbstrOut = SysAllocString(date);
  if (*pbstrOut)
    TRACE("returning %s\n", debugstr_w(*pbstrOut));
  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

/******************************************************************************
 * VarBstrFromBool (OLEAUT32.116)
 *
 * Convert a VT_BOOL to a VT_BSTR.
 *
 * PARAMS
 *  boolIn   [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 *
 * NOTES
 *  If dwFlags includes VARIANT_LOCALBOOL, this function converts to the
 *  localised text of "True" or "False". To convert a bool into a
 *  numeric string of "0" or "-1", use VariantChangeTypeTypeEx().
 */
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  WCHAR szBuff[64];
  DWORD dwResId = IDS_TRUE;
  LANGID langId;

  TRACE("%d,0x%08x,0x%08x,%p\n", boolIn, lcid, dwFlags, pbstrOut);

  if (!pbstrOut)
    return E_INVALIDARG;

  /* VAR_BOOLONOFF and VAR_BOOLYESNO are internal flags used
   * for variant formatting */
  switch (dwFlags & (VAR_LOCALBOOL|VAR_BOOLONOFF|VAR_BOOLYESNO))
  {
  case VAR_BOOLONOFF:
      dwResId = IDS_ON;
      break;
  case VAR_BOOLYESNO:
      dwResId = IDS_YES;
      break;
  case VAR_LOCALBOOL:
      break;
  default:
    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),SORT_DEFAULT);
  }

  lcid = ConvertDefaultLocale(lcid);
  langId = LANGIDFROMLCID(lcid);
  if (PRIMARYLANGID(langId) == LANG_NEUTRAL)
    langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);

  if (boolIn == VARIANT_FALSE)
    dwResId++; /* Use negative form */

VarBstrFromBool_GetLocalised:
  if (VARIANT_GetLocalisedText(langId, dwResId, szBuff))
  {
    *pbstrOut = SysAllocString(szBuff);
    return *pbstrOut ? S_OK : E_OUTOFMEMORY;
  }

  if (langId != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
  {
    langId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    goto VarBstrFromBool_GetLocalised;
  }

  /* Should never get here */
  WARN("Failed to load bool text!\n");
  return E_OUTOFMEMORY;
}

/******************************************************************************
 * VarBstrFromI1 (OLEAUT32.229)
 *
 * Convert a VT_I1 to a VT_BSTR.
 *
 * PARAMS
 *  cIn      [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromI1(signed char cIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  ULONG64 ul64 = cIn;

  if (cIn < 0)
  {
    ul64 = -cIn;
    dwFlags |= VAR_NEGATIVE;
  }
  return VARIANT_BstrFromUInt(ul64, lcid, dwFlags, pbstrOut);
}

/******************************************************************************
 * VarBstrFromUI2 (OLEAUT32.230)
 *
 * Convert a VT_UI2 to a VT_BSTR.
 *
 * PARAMS
 *  usIn     [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromUI2(USHORT usIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromUInt(usIn, lcid, dwFlags, pbstrOut);
}

/******************************************************************************
 * VarBstrFromUI4 (OLEAUT32.231)
 *
 * Convert a VT_UI4 to a VT_BSTR.
 *
 * PARAMS
 *  ulIn     [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromUI4(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromUInt(ulIn, lcid, dwFlags, pbstrOut);
}

/******************************************************************************
 * VarBstrFromDec (OLEAUT32.232)
 *
 * Convert a VT_DECIMAL to a VT_BSTR.
 *
 * PARAMS
 *  pDecIn   [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromDec(DECIMAL* pDecIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  WCHAR buff[256];
  VARIANT_DI temp;

  if (!pbstrOut)
    return E_INVALIDARG;

  VARIANT_DIFromDec(pDecIn, &temp);
  VARIANT_DI_tostringW(&temp, buff, 256);

  if (dwFlags & LOCALE_USE_NLS)
  {
    WCHAR numbuff[256];

    /* Format the number for the locale */
    numbuff[0] = '\0';
    GetNumberFormatW(lcid, dwFlags & LOCALE_NOUSEROVERRIDE,
                     buff, NULL, numbuff, sizeof(numbuff) / sizeof(WCHAR));
    TRACE("created NLS string %s\n", debugstr_w(numbuff));
    *pbstrOut = SysAllocString(numbuff);
  }
  else
  {
    *pbstrOut = VARIANT_BstrReplaceDecimal(buff, lcid, dwFlags);
  }
  
  TRACE("returning %s\n", debugstr_w(*pbstrOut));
  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

/************************************************************************
 * VarBstrFromI8 (OLEAUT32.370)
 *
 * Convert a VT_I8 to a VT_BSTR.
 *
 * PARAMS
 *  llIn     [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromI8(LONG64 llIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  ULONG64 ul64 = llIn;

  if (llIn < 0)
  {
    ul64 = -llIn;
    dwFlags |= VAR_NEGATIVE;
  }
  return VARIANT_BstrFromUInt(ul64, lcid, dwFlags, pbstrOut);
}

/************************************************************************
 * VarBstrFromUI8 (OLEAUT32.371)
 *
 * Convert a VT_UI8 to a VT_BSTR.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromUI8(ULONG64 ullIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_BstrFromUInt(ullIn, lcid, dwFlags, pbstrOut);
}

/************************************************************************
 * VarBstrFromDisp (OLEAUT32.115)
 *
 * Convert a VT_DISPATCH to a BSTR.
 *
 * PARAMS
 *  pdispIn [I] Source
 *  lcid    [I] LCID for conversion
 *  dwFlags [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut  [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarBstrFromDisp(IDispatch* pdispIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pbstrOut, VT_BSTR, dwFlags);
}

/**********************************************************************
 * VarBstrCat (OLEAUT32.313)
 *
 * Concatenate two BSTR values.
 *
 * PARAMS
 *  pbstrLeft  [I] Source
 *  pbstrRight [I] Value to concatenate
 *  pbstrOut   [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrCat(BSTR pbstrLeft, BSTR pbstrRight, BSTR *pbstrOut)
{
  unsigned int lenLeft, lenRight;

  TRACE("%s,%s,%p\n",
   debugstr_wn(pbstrLeft, SysStringLen(pbstrLeft)),
   debugstr_wn(pbstrRight, SysStringLen(pbstrRight)), pbstrOut);

  if (!pbstrOut)
    return E_INVALIDARG;

  /* use byte length here to properly handle ansi-allocated BSTRs */
  lenLeft = pbstrLeft ? SysStringByteLen(pbstrLeft) : 0;
  lenRight = pbstrRight ? SysStringByteLen(pbstrRight) : 0;

  *pbstrOut = SysAllocStringByteLen(NULL, lenLeft + lenRight);
  if (!*pbstrOut)
    return E_OUTOFMEMORY;

  (*pbstrOut)[0] = '\0';

  if (pbstrLeft)
    memcpy(*pbstrOut, pbstrLeft, lenLeft);

  if (pbstrRight)
    memcpy((CHAR*)*pbstrOut + lenLeft, pbstrRight, lenRight);

  TRACE("%s\n", debugstr_wn(*pbstrOut, SysStringLen(*pbstrOut)));
  return S_OK;
}

/**********************************************************************
 * VarBstrCmp (OLEAUT32.314)
 *
 * Compare two BSTR values.
 *
 * PARAMS
 *  pbstrLeft  [I] Source
 *  pbstrRight [I] Value to compare
 *  lcid       [I] LCID for the comparison
 *  dwFlags    [I] Flags to pass directly to CompareStringW().
 *
 * RETURNS
 *  VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that pbstrLeft is less
 *  than, equal to or greater than pbstrRight respectively.
 *
 * NOTES
 *  VARCMP_NULL is NOT returned if either string is NULL unlike MSDN
 *  states. A NULL BSTR pointer is equivalent to an empty string.
 *  If LCID is equal to 0, a byte by byte comparison is performed.
 */
HRESULT WINAPI VarBstrCmp(BSTR pbstrLeft, BSTR pbstrRight, LCID lcid, DWORD dwFlags)
{
    HRESULT hres;
    int ret;

    TRACE("%s,%s,%d,%08x\n",
     debugstr_wn(pbstrLeft, SysStringLen(pbstrLeft)),
     debugstr_wn(pbstrRight, SysStringLen(pbstrRight)), lcid, dwFlags);

    if (!pbstrLeft || !*pbstrLeft)
    {
      if (pbstrRight && *pbstrRight)
        return VARCMP_LT;
    }
    else if (!pbstrRight || !*pbstrRight)
        return VARCMP_GT;

    if (lcid == 0)
    {
      unsigned int lenLeft = SysStringByteLen(pbstrLeft);
      unsigned int lenRight = SysStringByteLen(pbstrRight);
      ret = memcmp(pbstrLeft, pbstrRight, min(lenLeft, lenRight));
      if (ret < 0)
        return VARCMP_LT;
      if (ret > 0)
        return VARCMP_GT;
      if (lenLeft < lenRight)
        return VARCMP_LT;
      if (lenLeft > lenRight)
        return VARCMP_GT;
      return VARCMP_EQ;
    }
    else
    {
      unsigned int lenLeft = SysStringLen(pbstrLeft);
      unsigned int lenRight = SysStringLen(pbstrRight);

      if (lenLeft == 0 || lenRight == 0)
      {
          if (lenLeft == 0 && lenRight == 0) return VARCMP_EQ;
          return lenLeft < lenRight ? VARCMP_LT : VARCMP_GT;
      }

      hres = CompareStringW(lcid, dwFlags, pbstrLeft, lenLeft,
              pbstrRight, lenRight) - CSTR_LESS_THAN;
      TRACE("%d\n", hres);
      return hres;
    }
}

/*
 * DATE
 */

/******************************************************************************
 * VarDateFromUI1 (OLEAUT32.88)
 *
 * Convert a VT_UI1 to a VT_DATE.
 *
 * PARAMS
 *  bIn      [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromUI1(BYTE bIn, DATE* pdateOut)
{
  return VarR8FromUI1(bIn, pdateOut);
}

/******************************************************************************
 * VarDateFromI2 (OLEAUT32.89)
 *
 * Convert a VT_I2 to a VT_DATE.
 *
 * PARAMS
 *  sIn      [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromI2(short sIn, DATE* pdateOut)
{
  return VarR8FromI2(sIn, pdateOut);
}

/******************************************************************************
 * VarDateFromI4 (OLEAUT32.90)
 *
 * Convert a VT_I4 to a VT_DATE.
 *
 * PARAMS
 *  lIn      [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromI4(LONG lIn, DATE* pdateOut)
{
  return VarDateFromR8(lIn, pdateOut);
}

/******************************************************************************
 * VarDateFromR4 (OLEAUT32.91)
 *
 * Convert a VT_R4 to a VT_DATE.
 *
 * PARAMS
 *  fltIn    [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromR4(FLOAT fltIn, DATE* pdateOut)
{
  return VarR8FromR4(fltIn, pdateOut);
}

/******************************************************************************
 * VarDateFromR8 (OLEAUT32.92)
 *
 * Convert a VT_R8 to a VT_DATE.
 *
 * PARAMS
 *  dblIn    [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromR8(double dblIn, DATE* pdateOut)
{
  if (dblIn <= (DATE_MIN - 1.0) || dblIn >= (DATE_MAX + 1.0)) return DISP_E_OVERFLOW;
  *pdateOut = (DATE)dblIn;
  return S_OK;
}

/**********************************************************************
 * VarDateFromDisp (OLEAUT32.95)
 *
 * Convert a VT_DISPATCH to a VT_DATE.
 *
 * PARAMS
 *  pdispIn  [I] Source
 *  lcid     [I] LCID for conversion
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if the source value is invalid
 *           DISP_E_OVERFLOW, if the value will not fit in the destination
 *           DISP_E_TYPEMISMATCH, if the type cannot be converted
 */
HRESULT WINAPI VarDateFromDisp(IDispatch* pdispIn, LCID lcid, DATE* pdateOut)
{
  return VARIANT_FromDisp(pdispIn, lcid, pdateOut, VT_DATE, 0);
}

/******************************************************************************
 * VarDateFromBool (OLEAUT32.96)
 *
 * Convert a VT_BOOL to a VT_DATE.
 *
 * PARAMS
 *  boolIn   [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL boolIn, DATE* pdateOut)
{
  return VarR8FromBool(boolIn, pdateOut);
}

/**********************************************************************
 * VarDateFromCy (OLEAUT32.93)
 *
 * Convert a VT_CY to a VT_DATE.
 *
 * PARAMS
 *  lIn      [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromCy(CY cyIn, DATE* pdateOut)
{
  return VarR8FromCy(cyIn, pdateOut);
}

/* Date string parsing */
#define DP_TIMESEP 0x01 /* Time separator ( _must_ remain 0x1, used as a bitmask) */
#define DP_DATESEP 0x02 /* Date separator */
#define DP_MONTH   0x04 /* Month name */
#define DP_AM      0x08 /* AM */
#define DP_PM      0x10 /* PM */

typedef struct tagDATEPARSE
{
    DWORD dwCount;      /* Number of fields found so far (maximum 6) */
    DWORD dwParseFlags; /* Global parse flags (DP_ Flags above) */
    DWORD dwFlags[6];   /* Flags for each field */
    DWORD dwValues[6];  /* Value of each field */
} DATEPARSE;

#define TIMEFLAG(i) ((dp.dwFlags[i] & DP_TIMESEP) << i)

#define IsLeapYear(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/* Determine if a day is valid in a given month of a given year */
static BOOL VARIANT_IsValidMonthDay(DWORD day, DWORD month, DWORD year)
{
  static const BYTE days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (day && month && month < 13)
  {
    if (day <= days[month] || (month == 2 && day == 29 && IsLeapYear(year)))
      return TRUE;
  }
  return FALSE;
}

/* Possible orders for 3 numbers making up a date */
#define ORDER_MDY 0x01
#define ORDER_YMD 0x02
#define ORDER_YDM 0x04
#define ORDER_DMY 0x08
#define ORDER_MYD 0x10 /* Synthetic order, used only for funky 2 digit dates */

/* Determine a date for a particular locale, from 3 numbers */
static inline HRESULT VARIANT_MakeDate(DATEPARSE *dp, DWORD iDate,
                                       DWORD offset, SYSTEMTIME *st)
{
  DWORD dwAllOrders, dwTry, dwCount = 0, v1, v2, v3;

  if (!dp->dwCount)
  {
    v1 = 30; /* Default to (Variant) 0 date part */
    v2 = 12;
    v3 = 1899;
    goto VARIANT_MakeDate_OK;
  }

  v1 = dp->dwValues[offset + 0];
  v2 = dp->dwValues[offset + 1];
  if (dp->dwCount == 2)
  {
    SYSTEMTIME current;
    GetSystemTime(&current);
    v3 = current.wYear;
  }
  else
    v3 = dp->dwValues[offset + 2];

  TRACE("(%d,%d,%d,%d,%d)\n", v1, v2, v3, iDate, offset);

  /* If one number must be a month (Because a month name was given), then only
   * consider orders with the month in that position.
   * If we took the current year as 'v3', then only allow a year in that position.
   */
  if (dp->dwFlags[offset + 0] & DP_MONTH)
  {
    dwAllOrders = ORDER_MDY;
  }
  else if (dp->dwFlags[offset + 1] & DP_MONTH)
  {
    dwAllOrders = ORDER_DMY;
    if (dp->dwCount > 2)
      dwAllOrders |= ORDER_YMD;
  }
  else if (dp->dwCount > 2 && dp->dwFlags[offset + 2] & DP_MONTH)
  {
    dwAllOrders = ORDER_YDM;
  }
  else
  {
    dwAllOrders = ORDER_MDY|ORDER_DMY;
    if (dp->dwCount > 2)
      dwAllOrders |= (ORDER_YMD|ORDER_YDM);
  }

VARIANT_MakeDate_Start:
  TRACE("dwAllOrders is 0x%08x\n", dwAllOrders);

  while (dwAllOrders)
  {
    DWORD dwTemp;

    if (dwCount == 0)
    {
      /* First: Try the order given by iDate */
      switch (iDate)
      {
      case 0:  dwTry = dwAllOrders & ORDER_MDY; break;
      case 1:  dwTry = dwAllOrders & ORDER_DMY; break;
      default: dwTry = dwAllOrders & ORDER_YMD; break;
      }
    }
    else if (dwCount == 1)
    {
      /* Second: Try all the orders compatible with iDate */
      switch (iDate)
      {
      case 0:  dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
      case 1:  dwTry = dwAllOrders & ~(ORDER_MDY|ORDER_YDM|ORDER_MYD); break;
      default: dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
      }
    }
    else
    {
      /* Finally: Try any remaining orders */
      dwTry = dwAllOrders;
    }

    TRACE("Attempt %d, dwTry is 0x%08x\n", dwCount, dwTry);

    dwCount++;
    if (!dwTry)
      continue;

#define DATE_SWAP(x,y) do { dwTemp = x; x = y; y = dwTemp; } while (0)

    if (dwTry & ORDER_MDY)
    {
      if (VARIANT_IsValidMonthDay(v2,v1,v3))
      {
        DATE_SWAP(v1,v2);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_MDY;
    }
    if (dwTry & ORDER_YMD)
    {
      if (VARIANT_IsValidMonthDay(v3,v2,v1))
      {
        DATE_SWAP(v1,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_YMD;
    }
    if (dwTry & ORDER_YDM)
    {
      if (VARIANT_IsValidMonthDay(v2,v3,v1))
      {
        DATE_SWAP(v1,v2);
        DATE_SWAP(v2,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_YDM;
    }
    if (dwTry & ORDER_DMY)
    {
      if (VARIANT_IsValidMonthDay(v1,v2,v3))
        goto VARIANT_MakeDate_OK;
      dwAllOrders &= ~ORDER_DMY;
    }
    if (dwTry & ORDER_MYD)
    {
      /* Only occurs if we are trying a 2 year date as M/Y not D/M */
      if (VARIANT_IsValidMonthDay(v3,v1,v2))
      {
        DATE_SWAP(v1,v3);
        DATE_SWAP(v2,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_MYD;
    }
  }

  if (dp->dwCount == 2)
  {
    /* We couldn't make a date as D/M or M/D, so try M/Y or Y/M */
    v3 = 1; /* 1st of the month */
    dwAllOrders = ORDER_YMD|ORDER_MYD;
    dp->dwCount = 0; /* Don't return to this code path again */
    dwCount = 0;
    goto VARIANT_MakeDate_Start;
  }

  /* No valid dates were able to be constructed */
  return DISP_E_TYPEMISMATCH;

VARIANT_MakeDate_OK:

  /* Check that the time part is ok */
  if (st->wHour > 23 || st->wMinute > 59 || st->wSecond > 59)
    return DISP_E_TYPEMISMATCH;

  TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);
  if (st->wHour < 12 && (dp->dwParseFlags & DP_PM))
    st->wHour += 12;
  else if (st->wHour == 12 && (dp->dwParseFlags & DP_AM))
    st->wHour = 0;
  TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);

  st->wDay = v1;
  st->wMonth = v2;
  /* FIXME: For 2 digit dates, I'm not sure if 30 is hard coded or not. It may
   * be retrieved from:
   * HKCU\Control Panel\International\Calendars\TwoDigitYearMax
   * But Wine doesn't have/use that key as at the time of writing.
   */
  st->wYear = v3 < 30 ? 2000 + v3 : v3 < 100 ? 1900 + v3 : v3;
  TRACE("Returning date %d/%d/%d\n", v1, v2, st->wYear);
  return S_OK;
}

/******************************************************************************
 * VarDateFromStr [OLEAUT32.94]
 *
 * Convert a VT_BSTR to at VT_DATE.
 *
 * PARAMS
 *  strIn    [I] String to convert
 *  lcid     [I] Locale identifier for the conversion
 *  dwFlags  [I] Flags affecting the conversion (VAR_ flags from "oleauto.h")
 *  pdateOut [O] Destination for the converted value
 *
 * RETURNS
 *  Success: S_OK. pdateOut contains the converted value.
 *  FAILURE: An HRESULT error code indicating the problem.
 *
 * NOTES
 *  Any date format that can be created using the date formats from lcid
 *  (Either from kernel Nls functions, variant conversion or formatting) is a
 *  valid input to this function. In addition, a few more esoteric formats are
 *  also supported for compatibility with the native version. The date is
 *  interpreted according to the date settings in the control panel, unless
 *  the date is invalid in that format, in which the most compatible format
 *  that produces a valid date will be used.
 */
HRESULT WINAPI VarDateFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, DATE* pdateOut)
{
  static const USHORT ParseDateTokens[] =
  {
    LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME4,
    LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8,
    LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
    LOCALE_SMONTHNAME13,
    LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
    LOCALE_SABBREVMONTHNAME13,
    LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3, LOCALE_SDAYNAME4,
    LOCALE_SDAYNAME5, LOCALE_SDAYNAME6, LOCALE_SDAYNAME7,
    LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3,
    LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6,
    LOCALE_SABBREVDAYNAME7,
    LOCALE_S1159, LOCALE_S2359,
    LOCALE_SDATE
  };
  static const BYTE ParseDateMonths[] =
  {
    1,2,3,4,5,6,7,8,9,10,11,12,13,
    1,2,3,4,5,6,7,8,9,10,11,12,13
  };
  unsigned int i;
  BSTR tokens[sizeof(ParseDateTokens)/sizeof(ParseDateTokens[0])];
  DATEPARSE dp;
  DWORD dwDateSeps = 0, iDate = 0;
  HRESULT hRet = S_OK;

  if ((dwFlags & (VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY)) ==
      (VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY))
    return E_INVALIDARG;

  if (!strIn)
    return DISP_E_TYPEMISMATCH;

  *pdateOut = 0.0;

  TRACE("(%s,0x%08x,0x%08x,%p)\n", debugstr_w(strIn), lcid, dwFlags, pdateOut);

  memset(&dp, 0, sizeof(dp));

  GetLocaleInfoW(lcid, LOCALE_IDATE|LOCALE_RETURN_NUMBER|(dwFlags & LOCALE_NOUSEROVERRIDE),
                 (LPWSTR)&iDate, sizeof(iDate)/sizeof(WCHAR));
  TRACE("iDate is %d\n", iDate);

  /* Get the month/day/am/pm tokens for this locale */
  for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
  {
    WCHAR buff[128];
    LCTYPE lctype =  ParseDateTokens[i] | (dwFlags & LOCALE_NOUSEROVERRIDE);

    /* FIXME: Alternate calendars - should use GetCalendarInfo() and/or
     *        GetAltMonthNames(). We should really cache these strings too.
     */
    buff[0] = '\0';
    GetLocaleInfoW(lcid, lctype, buff, sizeof(buff)/sizeof(WCHAR));
    tokens[i] = SysAllocString(buff);
    TRACE("token %d is %s\n", i, debugstr_w(tokens[i]));
  }

  /* Parse the string into our structure */
  while (*strIn)
  {
    if (dp.dwCount >= 6)
      break;

    if (isdigitW(*strIn))
    {
      dp.dwValues[dp.dwCount] = strtoulW(strIn, &strIn, 10);
      dp.dwCount++;
      strIn--;
    }
    else if (isalpha(*strIn))
    {
      BOOL bFound = FALSE;

      for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
      {
        DWORD dwLen = strlenW(tokens[i]);
        if (dwLen && !strncmpiW(strIn, tokens[i], dwLen))
        {
          if (i <= 25)
          {
            dp.dwValues[dp.dwCount] = ParseDateMonths[i];
            dp.dwFlags[dp.dwCount] |= (DP_MONTH|DP_DATESEP);
            dp.dwCount++;
          }
          else if (i > 39 && i < 42)
          {
            if (!dp.dwCount || dp.dwParseFlags & (DP_AM|DP_PM))
              hRet = DISP_E_TYPEMISMATCH;
            else
            {
              dp.dwFlags[dp.dwCount - 1] |= (i == 40 ? DP_AM : DP_PM);
              dp.dwParseFlags |= (i == 40 ? DP_AM : DP_PM);
            }
          }
          strIn += (dwLen - 1);
          bFound = TRUE;
          break;
        }
      }

      if (!bFound)
      {
        if ((*strIn == 'a' || *strIn == 'A' || *strIn == 'p' || *strIn == 'P') &&
            (dp.dwCount && !(dp.dwParseFlags & (DP_AM|DP_PM))))
        {
          /* Special case - 'a' and 'p' are recognised as short for am/pm */
          if (*strIn == 'a' || *strIn == 'A')
          {
            dp.dwFlags[dp.dwCount - 1] |= DP_AM;
            dp.dwParseFlags |=  DP_AM;
          }
          else
          {
            dp.dwFlags[dp.dwCount - 1] |= DP_PM;
            dp.dwParseFlags |=  DP_PM;
          }
          strIn++;
        }
        else
        {
          TRACE("No matching token for %s\n", debugstr_w(strIn));
          hRet = DISP_E_TYPEMISMATCH;
          break;
        }
      }
    }
    else if (*strIn == ':' ||  *strIn == '.')
    {
      if (!dp.dwCount || !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
      else
        if (tokens[42][0] == *strIn)
        {
          dwDateSeps++;
          if (dwDateSeps > 2)
            hRet = DISP_E_TYPEMISMATCH;
          else
            dp.dwFlags[dp.dwCount - 1] |= DP_DATESEP;
        }
        else
          dp.dwFlags[dp.dwCount - 1] |= DP_TIMESEP;
    }
    else if (*strIn == '-' || *strIn == '/')
    {
      dwDateSeps++;
      if (dwDateSeps > 2 || !dp.dwCount || !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
      else
        dp.dwFlags[dp.dwCount - 1] |= DP_DATESEP;
    }
    else if (*strIn == ',' || isspaceW(*strIn))
    {
      if (*strIn == ',' && !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
    }
    else
    {
      hRet = DISP_E_TYPEMISMATCH;
    }
    strIn++;
  }

  if (!dp.dwCount || dp.dwCount > 6 ||
      (dp.dwCount == 1 && !(dp.dwParseFlags & (DP_AM|DP_PM))))
    hRet = DISP_E_TYPEMISMATCH;

  if (SUCCEEDED(hRet))
  {
    SYSTEMTIME st;
    DWORD dwOffset = 0; /* Start of date fields in dp.dwValues */

    st.wDayOfWeek = st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;

    /* Figure out which numbers correspond to which fields.
     *
     * This switch statement works based on the fact that native interprets any
     * fields that are not joined with a time separator ('.' or ':') as date
     * fields. Thus we construct a value from 0-32 where each set bit indicates
     * a time field. This encapsulates the hundreds of permutations of 2-6 fields.
     * For valid permutations, we set dwOffset to point to the first date field
     * and shorten dp.dwCount by the number of time fields found. The real
     * magic here occurs in VARIANT_MakeDate() above, where we determine what
     * each date number must represent in the context of iDate.
     */
    TRACE("0x%08x\n", TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4));

    switch (TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4))
    {
    case 0x1: /* TT TTDD TTDDD */
      if (dp.dwCount > 3 &&
          ((dp.dwFlags[2] & (DP_AM|DP_PM)) || (dp.dwFlags[3] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[4] & (DP_AM|DP_PM))))
        hRet = DISP_E_TYPEMISMATCH;
      else if (dp.dwCount != 2 && dp.dwCount != 4 && dp.dwCount != 5)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour = dp.dwValues[0];
      st.wMinute  = dp.dwValues[1];
      dp.dwCount -= 2;
      dwOffset = 2;
      break;

    case 0x3: /* TTT TTTDD TTTDDD */
      if (dp.dwCount > 4 &&
          ((dp.dwFlags[3] & (DP_AM|DP_PM)) || (dp.dwFlags[4] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[5] & (DP_AM|DP_PM))))
        hRet = DISP_E_TYPEMISMATCH;
      else if (dp.dwCount != 3 && dp.dwCount != 5 && dp.dwCount != 6)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[0];
      st.wMinute = dp.dwValues[1];
      st.wSecond = dp.dwValues[2];
      dwOffset = 3;
      dp.dwCount -= 3;
      break;

    case 0x4: /* DDTT */
      if (dp.dwCount != 4 ||
          (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;

      st.wHour = dp.dwValues[2];
      st.wMinute  = dp.dwValues[3];
      dp.dwCount -= 2;
      break;

   case 0x0: /* T DD DDD TDDD TDDD */
      if (dp.dwCount == 1 && (dp.dwParseFlags & (DP_AM|DP_PM)))
      {
        st.wHour = dp.dwValues[0]; /* T */
        dp.dwCount = 0;
        break;
      }
      else if (dp.dwCount > 4 || (dp.dwCount < 3 && dp.dwParseFlags & (DP_AM|DP_PM)))
      {
        hRet = DISP_E_TYPEMISMATCH;
      }
      else if (dp.dwCount == 3)
      {
        if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDD */
        {
          dp.dwCount = 2;
          st.wHour = dp.dwValues[0];
          dwOffset = 1;
          break;
        }
        if (dp.dwFlags[2] & (DP_AM|DP_PM)) /* DDT */
        {
          dp.dwCount = 2;
          st.wHour = dp.dwValues[2];
          break;
        }
        else if (dp.dwParseFlags & (DP_AM|DP_PM))
          hRet = DISP_E_TYPEMISMATCH;
      }
      else if (dp.dwCount == 4)
      {
        dp.dwCount = 3;
        if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDDD */
        {
          st.wHour = dp.dwValues[0];
          dwOffset = 1;
        }
        else if (dp.dwFlags[3] & (DP_AM|DP_PM)) /* DDDT */
        {
          st.wHour = dp.dwValues[3];
        }
        else
          hRet = DISP_E_TYPEMISMATCH;
        break;
      }
      /* .. fall through .. */

    case 0x8: /* DDDTT */
      if ((dp.dwCount == 2 && (dp.dwParseFlags & (DP_AM|DP_PM))) ||
          (dp.dwCount == 5 && ((dp.dwFlags[0] & (DP_AM|DP_PM)) ||
           (dp.dwFlags[1] & (DP_AM|DP_PM)) || (dp.dwFlags[2] & (DP_AM|DP_PM)))) ||
           dp.dwCount == 4 || dp.dwCount == 6)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[3];
      st.wMinute = dp.dwValues[4];
      if (dp.dwCount == 5)
        dp.dwCount -= 2;
      break;

    case 0xC: /* DDTTT */
      if (dp.dwCount != 5 ||
          (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[2];
      st.wMinute = dp.dwValues[3];
      st.wSecond = dp.dwValues[4];
      dp.dwCount -= 3;
      break;

    case 0x18: /* DDDTTT */
      if ((dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[2] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[3];
      st.wMinute = dp.dwValues[4];
      st.wSecond = dp.dwValues[5];
      dp.dwCount -= 3;
      break;

    default:
      hRet = DISP_E_TYPEMISMATCH;
      break;
    }

    if (SUCCEEDED(hRet))
    {
      hRet = VARIANT_MakeDate(&dp, iDate, dwOffset, &st);

      if (dwFlags & VAR_TIMEVALUEONLY)
      {
        st.wYear = 1899;
        st.wMonth = 12;
        st.wDay = 30;
      }
      else if (dwFlags & VAR_DATEVALUEONLY)
       st.wHour = st.wMinute = st.wSecond = 0;

      /* Finally, convert the value to a VT_DATE */
      if (SUCCEEDED(hRet))
        hRet = SystemTimeToVariantTime(&st, pdateOut) ? S_OK : DISP_E_TYPEMISMATCH;
    }
  }

  for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
    SysFreeString(tokens[i]);
  return hRet;
}

/******************************************************************************
 * VarDateFromI1 (OLEAUT32.221)
 *
 * Convert a VT_I1 to a VT_DATE.
 *
 * PARAMS
 *  cIn      [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromI1(signed char cIn, DATE* pdateOut)
{
  return VarR8FromI1(cIn, pdateOut);
}

/******************************************************************************
 * VarDateFromUI2 (OLEAUT32.222)
 *
 * Convert a VT_UI2 to a VT_DATE.
 *
 * PARAMS
 *  uiIn     [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromUI2(USHORT uiIn, DATE* pdateOut)
{
  return VarR8FromUI2(uiIn, pdateOut);
}

/******************************************************************************
 * VarDateFromUI4 (OLEAUT32.223)
 *
 * Convert a VT_UI4 to a VT_DATE.
 *
 * PARAMS
 *  ulIn     [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromUI4(ULONG ulIn, DATE* pdateOut)
{
  return VarDateFromR8(ulIn, pdateOut);
}

/**********************************************************************
 * VarDateFromDec (OLEAUT32.224)
 *
 * Convert a VT_DECIMAL to a VT_DATE.
 *
 * PARAMS
 *  pdecIn   [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI VarDateFromDec(DECIMAL *pdecIn, DATE* pdateOut)
{
  return VarR8FromDec(pdecIn, pdateOut);
}

/******************************************************************************
 * VarDateFromI8 (OLEAUT32.364)
 *
 * Convert a VT_I8 to a VT_DATE.
 *
 * PARAMS
 *  llIn     [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDateFromI8(LONG64 llIn, DATE* pdateOut)
{
  if (llIn < DATE_MIN || llIn > DATE_MAX) return DISP_E_OVERFLOW;
  *pdateOut = (DATE)llIn;
  return S_OK;
}

/******************************************************************************
 * VarDateFromUI8 (OLEAUT32.365)
 *
 * Convert a VT_UI8 to a VT_DATE.
 *
 * PARAMS
 *  ullIn    [I] Source
 *  pdateOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_OVERFLOW, if the value will not fit in the destination
 */
HRESULT WINAPI VarDateFromUI8(ULONG64 ullIn, DATE* pdateOut)
{
  if (ullIn > DATE_MAX) return DISP_E_OVERFLOW;
  *pdateOut = (DATE)ullIn;
  return S_OK;
}
