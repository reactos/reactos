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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/debug.h"
#include "wine/unicode.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "variant.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);


#ifdef __REACTOS__ /*FIXME*/
/* problems with decVal member of VARIANT union in MinGW headers */
#undef V_DECIMAL
#define V_DECIMAL(X) (X->__VARIANT_NAME_1.decVal)
#endif


extern HMODULE OLEAUT32_hModule;

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
#ifndef __REACTOS__ /*FIXME*/
  case VT_UI2: memcpy(pOut, &V_UI2(srcVar), sizeof(SHORT));
#endif
	break;
  case VT_R4:
  case VT_INT:
  case VT_I4:
  case VT_UINT:
#ifndef __REACTOS__ /*FIXME*/
  case VT_UI4: memcpy(pOut, &V_UI4(srcVar), sizeof (LONG));
#endif
	break;
  case VT_R8:
  case VT_DATE:
  case VT_CY:
  case VT_I8:
#ifndef __REACTOS__ /*FIXME*/
  case VT_UI8: memcpy(pOut, &V_UI8(srcVar), sizeof (LONG64));
#endif
	break;
  case VT_INT_PTR: memcpy(pOut, &V_INT_PTR(srcVar), sizeof (INT_PTR)); break;
  case VT_DECIMAL: memcpy(pOut, &V_DECIMAL(srcVar), sizeof (DECIMAL)); break;
  default:
    FIXME("VT_ type %d unhandled, please report!\n", vt);
  }
}


/* Coerce VT_BSTR to a numeric type */
HRESULT VARIANT_NumberFromBstr(OLECHAR* pStrIn, LCID lcid, ULONG ulFlags,
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
HRESULT VARIANT_FromDisp(IDispatch* pdispIn, LCID lcid, void* pOut, VARTYPE vt)
{
  static const DISPPARAMS emptyParams = { NULL, NULL, 0, 0 };
  VARIANTARG srcVar, dstVar;
  HRESULT hRet;

  if (!pdispIn)
    return DISP_E_BADVARTYPE;

  /* Get the default 'value' property from the IDispatch */
  hRet = IDispatch_Invoke(pdispIn, DISPID_VALUE, &IID_NULL, lcid, DISPATCH_PROPERTYGET,
                          (DISPPARAMS*)&emptyParams, &srcVar, NULL, NULL);

  if (SUCCEEDED(hRet))
  {
    /* Convert the property to the requested type */
    V_VT(&dstVar) = VT_EMPTY;
    hRet = VariantChangeTypeEx(&dstVar, &srcVar, lcid, 0, vt);
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
  return _VarI1FromR4(fltIn, pcOut);
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
  if (dblIn < (double)I1_MIN || dblIn > (double)I1_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(CHAR, dblIn, *pcOut);
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
  return _VarI1FromDate(dateIn, pcOut);
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

  _VarI4FromCy(cyIn, &i);
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
  return _VarI1FromStr(strIn, lcid, dwFlags, pcOut);
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
  return _VarI1FromDisp(pdispIn, lcid, pcOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarI1FromDec(DECIMAL *pdecIn, signed char* pcOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI1FromI8(i64, pcOut);
  return hRet;
}
#endif

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
  return _VarUI1FromR4(fltIn, pbOut);
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
  if (dblIn < -0.5 || dblIn > (double)UI1_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(BYTE, dblIn, *pbOut);
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

  _VarUI4FromCy(cyIn, &i);
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
  return _VarUI1FromDate(dateIn, pbOut);
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
  return _VarUI1FromStr(strIn, lcid, dwFlags, pbOut);
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
  return _VarUI1FromDisp(pdispIn, lcid, pbOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarUI1FromDec(DECIMAL *pdecIn, BYTE* pbOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI1FromI8(i64, pbOut);
  return hRet;
}
#endif

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
  return _VarI2FromR4(fltIn, psOut);
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
  if (dblIn < (double)I2_MIN || dblIn > (double)I2_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(SHORT, dblIn, *psOut);
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

  _VarI4FromCy(cyIn, &i);
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
  return _VarI2FromDate(dateIn, psOut);
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
  return _VarI2FromStr(strIn, lcid, dwFlags, psOut);
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
  return _VarI2FromDisp(pdispIn, lcid, psOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
HRESULT WINAPI VarI2FromDec(DECIMAL *pdecIn, SHORT* psOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI2FromI8(i64, psOut);
  return hRet;
}
#endif

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
  return _VarUI2FromR4(fltIn, pusOut);
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
  if (dblIn < -0.5 || dblIn > (double)UI2_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(USHORT, dblIn, *pusOut);
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
  return _VarUI2FromDate(dateIn, pusOut);
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

  _VarUI4FromCy(cyIn, &i);
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
  return _VarUI2FromStr(strIn, lcid, dwFlags, pusOut);
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
  return _VarUI2FromDisp(pdispIn, lcid, pusOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarUI2FromDec(DECIMAL *pdecIn, USHORT* pusOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI2FromI8(i64, pusOut);
  return hRet;
}
#endif

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
 *  iIn     [I] Source
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
  return _VarI4FromR4(fltIn, piOut);
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
  if (dblIn < (double)I4_MIN || dblIn > (double)I4_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(LONG, dblIn, *piOut);
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
  return _VarI4FromR8(d, piOut);
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
  return _VarI4FromDate(dateIn, piOut);
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
  return _VarI4FromStr(strIn, lcid, dwFlags, piOut);
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
  return _VarI4FromDisp(pdispIn, lcid, piOut);
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
 * Convert a VT_I4 to a VT_I4.
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarI4FromDec(DECIMAL *pdecIn, LONG *piOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarI4FromI8(i64, piOut);
  return hRet;
}
#endif

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
  return _VarUI4FromR4(fltIn, pulOut);
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
  if (dblIn < -0.5 || dblIn > (double)UI4_MAX)
    return DISP_E_OVERFLOW;
  OLEAUT32_DutchRound(ULONG, dblIn, *pulOut);
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
  return _VarUI4FromDate(dateIn, pulOut);
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
  return _VarUI4FromR8(d, pulOut);
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
  return _VarUI4FromStr(strIn, lcid, dwFlags, pulOut);
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
  return _VarUI4FromDisp(pdispIn, lcid, pulOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarUI4FromDec(DECIMAL *pdecIn, ULONG *pulOut)
{
  LONG64 i64;
  HRESULT hRet;

  hRet = _VarI8FromDec(pdecIn, &i64);

  if (SUCCEEDED(hRet))
    hRet = _VarUI4FromI8(i64, pulOut);
  return hRet;
}
#endif

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
  return _VarI8FromR4(fltIn, pi64Out);
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
  OLEAUT32_DutchRound(LONG64, dblIn, *pi64Out);
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
  return _VarI8FromDate(dateIn, pi64Out);
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
  return _VarI8FromStr(strIn, lcid, dwFlags, pi64Out);
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
  return _VarI8FromDisp(pdispIn, lcid, pi64Out);
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
  return _VarI8FromBool(boolIn, pi64Out);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
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

    hRet = _VarR8FromDec(pdecIn, &dbl);
    if (SUCCEEDED(hRet))
      hRet = VarI8FromR8(dbl, pi64Out);
    return hRet;
  }
}
#endif

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
  return _VarUI8FromR4(fltIn, pui64Out);
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
  OLEAUT32_DutchRound(ULONG64, dblIn, *pui64Out);
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
  return _VarUI8FromDate(dateIn, pui64Out);
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
  return _VarUI8FromStr(strIn, lcid, dwFlags, pui64Out);
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
  return _VarUI8FromDisp(pdispIn, lcid, pui64Out);
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
  return _VarUI8FromBool(boolIn, pui64Out);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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

    hRet = _VarR8FromDec(pdecIn, &dbl);
    if (SUCCEEDED(hRet))
      hRet = VarUI8FromR8(dbl, pui64Out);
    return hRet;
  }
}
#endif

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
  return _VarR4FromR8(dblIn, pFltOut);
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
  return _VarR4FromCy(cyIn, pFltOut);
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
  return _VarR4FromDate(dateIn, pFltOut);
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
  return _VarR4FromStr(strIn, lcid, dwFlags, pFltOut);
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
  return _VarR4FromDisp(pdispIn, lcid, pFltOut);
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
  return _VarR4FromBool(boolIn, pFltOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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
    highPart *= 1.0e64;
  }
  else
    highPart = 0.0;

  *pFltOut = (double)DEC_LO64(pDecIn) / (double)divisor + highPart;
  return S_OK;
}
#endif


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
  return _VarR8FromStr(strIn, lcid, dwFlags, pDblOut);
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
  return _VarR8FromDisp(pdispIn, lcid, pDblOut);
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
  return _VarR8FromBool(boolIn, pDblOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
HRESULT WINAPI VarR8FromDec(DECIMAL* pDecIn, double *pDblOut)
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
    highPart *= 1.0e64;
  }
  else
    highPart = 0.0;

  *pDblOut = (double)DEC_LO64(pDecIn) / divisor + highPart;
  return S_OK;
}
#endif

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
  return _VarCyFromUI1(bIn, pCyOut);
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
  return _VarCyFromI2(sIn, pCyOut);
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
  return _VarCyFromI4(lIn, pCyOut);
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
  return _VarCyFromR4(fltIn, pCyOut);
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
#if defined(__GNUC__) && defined(__i386__)
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
  return S_OK;
#else
  /* This version produces slightly different results for boundary cases */
  if (dblIn < -922337203685477.5807 || dblIn >= 922337203685477.5807)
    return DISP_E_OVERFLOW;
  dblIn *= CY_MULTIPLIER_F;
  OLEAUT32_DutchRound(LONG64, dblIn, pCyOut->int64);
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
  return _VarCyFromDate(dateIn, pCyOut);
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
  return _VarCyFromStr(strIn, lcid, dwFlags, pCyOut);
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
  return _VarCyFromDisp(pdispIn, lcid, pCyOut);
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
  return _VarCyFromBool(boolIn, pCyOut);
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
  return _VarCyFromI1(cIn, pCyOut);
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
  return _VarCyFromUI2(usIn, pCyOut);
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
  return _VarCyFromUI4(ulIn, pCyOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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
    return _VarCyFromR8(d, pCyOut);
  }
  return hRet;
}
#endif

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
  return _VarCyFromI8(llIn, pCyOut);
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
  return _VarCyFromUI8(ullIn, pCyOut);
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
  return _VarCyFromR8(l, pCyOut);
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
  return _VarCyFromR8(l, pCyOut);
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
  return _VarCyFromR8(d, pCyOut);
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
  return _VarCyFromR8(l, pCyOut);
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
    OLEAUT32_DutchRound(LONGLONG, d, pCyOut->int64)
    d = (double)pCyOut->int64 / div * CY_MULTIPLIER_F;
    OLEAUT32_DutchRound(LONGLONG, d, pCyOut->int64)
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
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparason
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
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparason
 */
HRESULT WINAPI VarCyCmpR8(const CY cyLeft, double dblRight)
{
  HRESULT hRet;
  CY cyRight;

  hRet = _VarCyFromR8(dblRight, &cyRight);

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
  return _VarCyFromR8(d, pCyOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
HRESULT WINAPI VarDecFromUI1(BYTE bIn, DECIMAL* pDecOut)
{
  return _VarDecFromUI1(bIn, pDecOut);
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
  return _VarDecFromI2(sIn, pDecOut);
}
#endif

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
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
#endif

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
  WCHAR buff[256];

  sprintfW( buff, szFloatFormatW, fltIn );
  return _VarDecFromStr(buff, LOCALE_SYSTEM_DEFAULT, 0, pDecOut);
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
  WCHAR buff[256];

  sprintfW( buff, szDoubleFormatW, dblIn );
  return _VarDecFromStr(buff, LOCALE_USER_DEFAULT, 0, pDecOut);
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
  return _VarDecFromDate(dateIn, pDecOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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
#endif


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
  return _VarDecFromStr(strIn, lcid, dwFlags, pDecOut);
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
  return _VarDecFromDisp(pdispIn, lcid, pDecOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
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
  return _VarDecFromI1(cIn, pDecOut);
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
  return _VarDecFromUI2(usIn, pDecOut);
}
#endif

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarDecFromUI4(ULONG ulIn, DECIMAL* pDecOut)
{
  DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
  DEC_HI32(pDecOut) = 0;
  DEC_MID32(pDecOut) = 0;
  DEC_LO32(pDecOut) = ulIn;
  return S_OK;
}
#endif

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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
#endif

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
HRESULT WINAPI VarDecFromUI8(ULONG64 ullIn, DECIMAL* pDecOut)
{
  DEC_SIGNSCALE(pDecOut) = SIGNSCALE(DECIMAL_POS,0);
  DEC_HI32(pDecOut) = 0;
  DEC_LO64(pDecOut) = ullIn;
  return S_OK;
}
#endif

#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
/* Make two DECIMALS the same scale; used by math functions below */
static HRESULT VARIANT_DecScale(const DECIMAL** ppDecLeft,
                                const DECIMAL** ppDecRight,
                                DECIMAL* pDecOut)
{
  static DECIMAL scaleFactor;
  DECIMAL decTemp;
  int scaleAmount, i;
  HRESULT hRet = S_OK;

  if (DEC_SIGN(*ppDecLeft) & ~DECIMAL_NEG || DEC_SIGN(*ppDecRight) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  DEC_LO32(&scaleFactor) = 10;

  i = scaleAmount = DEC_SCALE(*ppDecLeft) - DEC_SCALE(*ppDecRight);

  if (!scaleAmount)
    return S_OK; /* Same scale */

  if (scaleAmount > 0)
  {
    decTemp = *(*ppDecRight); /* Left is bigger - scale the right hand side */
    *ppDecRight = pDecOut;
  }
  else
  {
    decTemp = *(*ppDecLeft); /* Right is bigger - scale the left hand side */
    *ppDecLeft = pDecOut;
    i = scaleAmount = -scaleAmount;
  }

  if (DEC_SCALE(&decTemp) + scaleAmount > DEC_MAX_SCALE)
    return DISP_E_OVERFLOW; /* Can't scale up */

  /* Multiply up the value to be scaled by the correct amount */
  while (SUCCEEDED(hRet) && i--)
  {
    /* Note we are multiplying by a value with a scale of 0, so we don't recurse */
    hRet = VarDecMul(&decTemp, &scaleFactor, pDecOut);
    decTemp = *pDecOut;
  }
  DEC_SCALE(pDecOut) += scaleAmount; /* Set the new scale */
  return hRet;
}
#endif

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
  int invert = 0;
  ULARGE_INTEGER ul64;

  ul64.QuadPart = (LONG64)ulLeft - (ULONG64)ulRight;
  if (ulLeft < ulRight)
    invert = 1;

  if (ul64.QuadPart > (ULONG64)*pulHigh)
    ul64.QuadPart -= (ULONG64)*pulHigh;
  else
  {
    ul64.QuadPart -= (ULONG64)*pulHigh;
    invert = 1;
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

#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
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
#endif

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
HRESULT WINAPI VarDecAdd(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  HRESULT hRet;
  DECIMAL scaled;

  hRet = VARIANT_DecScale(&pDecLeft, &pDecRight, &scaled);

  if (SUCCEEDED(hRet))
  {
    /* Our decimals now have the same scale, we can add them as 96 bit integers */
    ULONG overflow = 0;
    BYTE sign = DECIMAL_POS;

    /* Correct for the sign of the result */
    if (DEC_SIGN(pDecLeft) && DEC_SIGN(pDecRight))
    {
      /* -x + -y : Negative */
      sign = DECIMAL_NEG;
      goto VarDecAdd_AsPositive;
    }
    else if (DEC_SIGN(pDecLeft) && !DEC_SIGN(pDecRight))
    {
      int cmp = VARIANT_DecCmp(pDecLeft, pDecRight);

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
      int cmp = VARIANT_DecCmp(pDecLeft, pDecRight);

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
#endif

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
  FIXME("(%p,%p,%p)-stub!\n",pDecLeft,pDecRight,pDecOut);
  return DISP_E_OVERFLOW;
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
HRESULT WINAPI VarDecMul(const DECIMAL* pDecLeft, const DECIMAL* pDecRight, DECIMAL* pDecOut)
{
  /* FIXME: This only allows multiplying by a fixed integer <= 0xffffffff */

  if (!DEC_SCALE(pDecLeft) || !DEC_SCALE(pDecRight))
  {
    /* At least one term is an integer */
    const DECIMAL* pDecInteger = DEC_SCALE(pDecLeft) ? pDecRight : pDecLeft;
    const DECIMAL* pDecOperand = DEC_SCALE(pDecLeft) ? pDecLeft  : pDecRight;
    HRESULT hRet = S_OK;
    unsigned int multiplier = DEC_LO32(pDecInteger);
    ULONG overflow = 0;

    if (DEC_HI32(pDecInteger) || DEC_MID32(pDecInteger))
    {
      FIXME("(%p,%p,%p) semi-stub!\n",pDecLeft,pDecRight,pDecOut);
      return DISP_E_OVERFLOW;
    }

    DEC_LO32(pDecOut)  = VARIANT_Mul(DEC_LO32(pDecOperand),  multiplier, &overflow);
    DEC_MID32(pDecOut) = VARIANT_Mul(DEC_MID32(pDecOperand), multiplier, &overflow);
    DEC_HI32(pDecOut)  = VARIANT_Mul(DEC_HI32(pDecOperand),  multiplier, &overflow);

    if (overflow)
       hRet = DISP_E_OVERFLOW;
    else
    {
      BYTE sign = DECIMAL_POS;

      if (DEC_SIGN(pDecLeft) != DEC_SIGN(pDecRight))
        sign = DECIMAL_NEG; /* pos * neg => negative */
      DEC_SIGN(pDecOut) = sign;
      DEC_SCALE(pDecOut) = DEC_SCALE(pDecOperand);
    }
    return hRet;
  }
  FIXME("(%p,%p,%p) semi-stub!\n",pDecLeft,pDecRight,pDecOut);
  return DISP_E_OVERFLOW;
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
#endif

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
  if (DEC_SIGN(pDecOut) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  if (!DEC_SCALE(pDecIn))
  {
    *pDecOut = *pDecIn; /* Already an integer */
    return S_OK;
  }

  FIXME("semi-stub!\n");
  return DISP_E_OVERFLOW;
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
  if (DEC_SIGN(pDecOut) & ~DECIMAL_NEG)
    return E_INVALIDARG;

  if (!(DEC_SIGN(pDecOut) & DECIMAL_NEG) || !DEC_SCALE(pDecIn))
    return VarDecFix(pDecIn, pDecOut); /* The same, if +ve or no fractionals */

  FIXME("semi-stub!\n");
  return DISP_E_OVERFLOW;
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
  if (cDecimals < 0 || (DEC_SIGN(pDecIn) & ~DECIMAL_NEG) || DEC_SCALE(pDecIn) > DEC_MAX_SCALE)
    return E_INVALIDARG;

  if (cDecimals >= DEC_SCALE(pDecIn))
  {
    *pDecOut = *pDecIn; /* More precision than we have */
    return S_OK;
  }

  FIXME("semi-stub!\n");

  return DISP_E_OVERFLOW;
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
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparason
 */
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
HRESULT WINAPI VarDecCmp(const DECIMAL* pDecLeft, const DECIMAL* pDecRight)
{
  HRESULT hRet;
  DECIMAL result;

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
 *  Failure: DISP_E_OVERFLOW, if overflow occurs during the comparason
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
#endif

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
  return _VarBoolFromUI1(bIn, pBoolOut);
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
  return _VarBoolFromI2(sIn, pBoolOut);
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
  return _VarBoolFromI4(lIn, pBoolOut);
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
  return _VarBoolFromR4(fltIn, pBoolOut);
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
  return _VarBoolFromR8(dblIn, pBoolOut);
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
  return _VarBoolFromDate(dateIn, pBoolOut);
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
  return _VarBoolFromCy(cyIn, pBoolOut);
}

static BOOL VARIANT_GetLocalisedText(LANGID langId, DWORD dwId, WCHAR *lpszDest)
{
  HRSRC hrsrc;

  hrsrc = FindResourceExW( OLEAUT32_hModule, (LPWSTR)RT_STRING,
                           (LPCWSTR)((dwId >> 4) + 1), langId );
  if (hrsrc)
  {
    HGLOBAL hmem = LoadResource( OLEAUT32_hModule, hrsrc );

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
    hRes = _VarR8FromStr(strIn, lcid, dwFlags, &d);
    if (SUCCEEDED(hRes))
      hRes = _VarBoolFromR8(d, pBoolOut);
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
  return _VarBoolFromDisp(pdispIn, lcid, pBoolOut);
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
  return _VarBoolFromI1(cIn, pBoolOut);
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
  return _VarBoolFromUI2(usIn, pBoolOut);
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
  return _VarBoolFromUI4(ulIn, pBoolOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
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
#endif

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
  return _VarBoolFromI8(llIn, pBoolOut);
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
  return _VarBoolFromUI8(ullIn, pBoolOut);
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
    ul64 = -lIn;
    dwFlags |= VAR_NEGATIVE;
  }
  return VARIANT_BstrFromUInt(ul64, lcid, dwFlags, pbstrOut);
}

static HRESULT VARIANT_BstrFromReal(DOUBLE dblIn, LCID lcid, ULONG dwFlags,
                                    BSTR* pbstrOut, LPCWSTR lpszFormat)
{
  WCHAR buff[256];

  if (!pbstrOut)
    return E_INVALIDARG;

  sprintfW( buff, lpszFormat, dblIn );
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
    *pbstrOut = SysAllocString(buff);
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
  double dblVal;

  if (!pbstrOut)
    return E_INVALIDARG;

  VarR8FromCy(cyIn, &dblVal);
  sprintfW(buff, szDoubleFormatW, dblVal);

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
    *pbstrOut = SysAllocString(buff);

  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
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
  WCHAR date[128], *time;

  TRACE("(%g,0x%08lx,0x%08lx,%p)\n", dateIn, lcid, dwFlags, pbstrOut);

#ifndef __REACTOS__ /*FIXME: no VariantTimeToSystemTime() yet */
  if (!pbstrOut || !VariantTimeToSystemTime(dateIn, &st))
#else
  if (!pbstrOut)
#endif
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
    else if (partial < 1e-12)
      dwFlags |= VAR_DATEVALUEONLY;
  }

  if (dwFlags & VAR_TIMEVALUEONLY)
    date[0] = '\0';
  else
    if (!GetDateFormatW(lcid, dwFormatFlags|DATE_SHORTDATE, &st, NULL, date,
                        sizeof(date)/sizeof(WCHAR)))
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

  TRACE("%d,0x%08lx,0x%08lx,%p\n", boolIn, lcid, dwFlags, pbstrOut);

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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO64 */
HRESULT WINAPI VarBstrFromDec(DECIMAL* pDecIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  if (!pbstrOut)
    return E_INVALIDARG;

  if (!DEC_SCALE(pDecIn) && !DEC_HI32(pDecIn))
  {
    WCHAR szBuff[256], *szOut = szBuff + sizeof(szBuff)/sizeof(WCHAR) - 1;

    /* Create the basic number string */
    *szOut-- = '\0';
    szOut = VARIANT_WriteNumber(DEC_LO64(pDecIn), szOut);
    if (DEC_SIGN(pDecIn))
      dwFlags |= VAR_NEGATIVE;

    *pbstrOut = VARIANT_MakeBstr(lcid, dwFlags, szOut);
    TRACE("returning %s\n", debugstr_w(*pbstrOut));
    return *pbstrOut ? S_OK : E_OUTOFMEMORY;
  }
  FIXME("semi-stub\n");
  return E_INVALIDARG;
}
#endif

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
  unsigned int len;

  if (!pbstrOut)
    return E_INVALIDARG;

  len = pbstrLeft ? strlenW(pbstrLeft) : 0;
  if (pbstrRight)
    len += strlenW(pbstrRight);

  *pbstrOut = SysAllocStringLen(NULL, len);
  if (!*pbstrOut)
    return E_OUTOFMEMORY;

  (*pbstrOut)[0] = '\0';

  if (pbstrLeft)
    strcpyW(*pbstrOut, pbstrLeft);

  if (pbstrRight)
    strcatW(*pbstrOut, pbstrRight);

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
 *  lcid       [I] LCID for the comparason
 *  dwFlags    [I] Flags to pass directly to CompareStringW().
 *
 * RETURNS
 *  VARCMP_LT, VARCMP_EQ or VARCMP_GT indicating that pbstrLeft is less
 *  than, equal to or greater than pbstrRight respectively.
 *  VARCMP_NULL is returned if either string is NULL, unless both are NULL
 *  in which case VARCMP_EQ is returned.
 */
HRESULT WINAPI VarBstrCmp(BSTR pbstrLeft, BSTR pbstrRight, LCID lcid, DWORD dwFlags)
{
    if (!pbstrLeft)
    {
      if (!pbstrRight || !*pbstrRight)
        return VARCMP_EQ;
      return VARCMP_NULL;
    }
    else if (!pbstrRight)
    {
      if (!*pbstrLeft)
        return VARCMP_EQ;
      return VARCMP_NULL;
    }

    return CompareStringW(lcid, dwFlags, pbstrLeft, -1, pbstrRight, -1) - 1;
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
  return _VarDateFromUI1(bIn, pdateOut);
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
  return _VarDateFromI2(sIn, pdateOut);
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
  return _VarDateFromI4(lIn, pdateOut);
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
  return _VarDateFromR4(fltIn, pdateOut);
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
  return _VarDateFromR8(dblIn, pdateOut);
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
  return _VarDateFromDisp(pdispIn, lcid, pdateOut);
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
  return _VarDateFromBool(boolIn, pdateOut);
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
  return _VarDateFromCy(cyIn, pdateOut);
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

  TRACE("(%ld,%ld,%ld,%ld,%ld)\n", v1, v2, v3, iDate, offset);

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
  TRACE("dwAllOrders is 0x%08lx\n", dwAllOrders);

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
      case 1:  dwTry = dwAllOrders & ~(ORDER_MDY|ORDER_YMD|ORDER_MYD); break;
      default: dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
      }
    }
    else
    {
      /* Finally: Try any remaining orders */
      dwTry = dwAllOrders;
    }

    TRACE("Attempt %ld, dwTry is 0x%08lx\n", dwCount, dwTry);

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
  TRACE("Returning date %ld/%ld/%d\n", v1, v2, st->wYear);
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
 *  FAILURE: An HRESULT error code indicating the prolem.
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
    LOCALE_S1159, LOCALE_S2359
  };
  static const BYTE ParseDateMonths[] =
  {
    1,2,3,4,5,6,7,8,9,10,11,12,13,
    1,2,3,4,5,6,7,8,9,10,11,12,13
  };
  size_t i;
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

  TRACE("(%s,0x%08lx,0x%08lx,%p)\n", debugstr_w(strIn), lcid, dwFlags, pdateOut);

  memset(&dp, 0, sizeof(dp));

  GetLocaleInfoW(lcid, LOCALE_IDATE|LOCALE_RETURN_NUMBER|(dwFlags & LOCALE_NOUSEROVERRIDE),
                 (LPWSTR)&iDate, sizeof(iDate)/sizeof(WCHAR));
  TRACE("iDate is %ld\n", iDate);

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
    if (dp.dwCount > 6)
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
          else if (i > 39)
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
    TRACE("0x%08lx\n", TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4));

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
  return _VarDateFromI1(cIn, pdateOut);
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
  return _VarDateFromUI2(uiIn, pdateOut);
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
  return _VarDateFromUI4(ulIn, pdateOut);
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
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
HRESULT WINAPI VarDateFromDec(DECIMAL *pdecIn, DATE* pdateOut)
{
  return _VarDateFromDec(pdecIn, pdateOut);
}
#endif

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
  return _VarDateFromI8(llIn, pdateOut);
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
  return _VarDateFromUI8(ullIn, pdateOut);
}
