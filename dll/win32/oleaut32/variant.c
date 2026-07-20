/*
 * VARIANT
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Daniel Remenak
 * Copyright 2006 Google (Benjamin Arai)
 *
 * The algorithm for conversion from Julian days to day/month/year is based on
 * that devised by Henry Fliegel, as implemented in PostgreSQL, which is
 * Copyright 1994-7 Regents of the University of California
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "variant.h"
#include "resource.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(variant);


/* Convert a variant from one type to another */
static inline HRESULT VARIANT_Coerce(VARIANTARG* pd, LCID lcid, USHORT wFlags,
                                     VARIANTARG* ps, VARTYPE vt)
{
  HRESULT res = DISP_E_TYPEMISMATCH;
  VARTYPE vtFrom =  V_TYPE(ps);
  DWORD dwFlags = 0;

  TRACE("%s, %#lx, 0x%04x, %s, %s.\n", debugstr_variant(pd), lcid, wFlags,
        debugstr_variant(ps), debugstr_vt(vt));

  if (vt == VT_BSTR || vtFrom == VT_BSTR)
  {
    /* All flags passed to low level function are only used for
     * changing to or from strings. Map these here.
     */
    if (wFlags & VARIANT_LOCALBOOL)
      dwFlags |= VAR_LOCALBOOL;
    if (wFlags & VARIANT_CALENDAR_HIJRI)
      dwFlags |= VAR_CALENDAR_HIJRI;
    if (wFlags & VARIANT_CALENDAR_THAI)
      dwFlags |= VAR_CALENDAR_THAI;
    if (wFlags & VARIANT_CALENDAR_GREGORIAN)
      dwFlags |= VAR_CALENDAR_GREGORIAN;
    if (wFlags & VARIANT_NOUSEROVERRIDE)
      dwFlags |= LOCALE_NOUSEROVERRIDE;
    if (wFlags & VARIANT_USE_NLS)
      dwFlags |= LOCALE_USE_NLS;
  }

  /* Map int/uint to i4/ui4 */
  if (vt == VT_INT)
    vt = VT_I4;
  else if (vt == VT_UINT)
    vt = VT_UI4;

  if (vtFrom == VT_INT)
    vtFrom = VT_I4;
  else if (vtFrom == VT_UINT)
    vtFrom = VT_UI4;

  if (vt == vtFrom)
     return VariantCopy(pd, ps);

  if (wFlags & VARIANT_NOVALUEPROP && vtFrom == VT_DISPATCH && vt != VT_UNKNOWN)
  {
    /* VARIANT_NOVALUEPROP prevents IDispatch objects from being coerced by
     * accessing the default object property.
     */
    return DISP_E_TYPEMISMATCH;
  }

  switch (vt)
  {
  case VT_EMPTY:
    if (vtFrom == VT_NULL)
      return DISP_E_TYPEMISMATCH;
    /* ... Fall through */
  case VT_NULL:
    if (vtFrom <= VT_UINT && vtFrom != (VARTYPE)15 && vtFrom != VT_ERROR)
    {
      res = VariantClear( pd );
      if (vt == VT_NULL && SUCCEEDED(res))
        V_VT(pd) = VT_NULL;
    }
    return res;

  case VT_I1:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_I1(pd) = 0; return S_OK;
    case VT_I2:       return VarI1FromI2(V_I2(ps), &V_I1(pd));
    case VT_I4:       return VarI1FromI4(V_I4(ps), &V_I1(pd));
    case VT_UI1:      V_I1(pd) = V_UI1(ps); return S_OK;
    case VT_UI2:      return VarI1FromUI2(V_UI2(ps), &V_I1(pd));
    case VT_UI4:      return VarI1FromUI4(V_UI4(ps), &V_I1(pd));
    case VT_I8:       return VarI1FromI8(V_I8(ps), &V_I1(pd));
    case VT_UI8:      return VarI1FromUI8(V_UI8(ps), &V_I1(pd));
    case VT_R4:       return VarI1FromR4(V_R4(ps), &V_I1(pd));
    case VT_R8:       return VarI1FromR8(V_R8(ps), &V_I1(pd));
    case VT_DATE:     return VarI1FromDate(V_DATE(ps), &V_I1(pd));
    case VT_BOOL:     return VarI1FromBool(V_BOOL(ps), &V_I1(pd));
    case VT_CY:       return VarI1FromCy(V_CY(ps), &V_I1(pd));
    case VT_DECIMAL:  return VarI1FromDec(&V_DECIMAL(ps), &V_I1(pd) );
    case VT_DISPATCH: return VarI1FromDisp(V_DISPATCH(ps), lcid, &V_I1(pd) );
    case VT_BSTR:     return VarI1FromStr(V_BSTR(ps), lcid, dwFlags, &V_I1(pd) );
    }
    break;

  case VT_I2:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_I2(pd) = 0; return S_OK;
    case VT_I1:       return VarI2FromI1(V_I1(ps), &V_I2(pd));
    case VT_I4:       return VarI2FromI4(V_I4(ps), &V_I2(pd));
    case VT_UI1:      return VarI2FromUI1(V_UI1(ps), &V_I2(pd));
    case VT_UI2:      V_I2(pd) = V_UI2(ps); return S_OK;
    case VT_UI4:      return VarI2FromUI4(V_UI4(ps), &V_I2(pd));
    case VT_I8:       return VarI2FromI8(V_I8(ps), &V_I2(pd));
    case VT_UI8:      return VarI2FromUI8(V_UI8(ps), &V_I2(pd));
    case VT_R4:       return VarI2FromR4(V_R4(ps), &V_I2(pd));
    case VT_R8:       return VarI2FromR8(V_R8(ps), &V_I2(pd));
    case VT_DATE:     return VarI2FromDate(V_DATE(ps), &V_I2(pd));
    case VT_BOOL:     return VarI2FromBool(V_BOOL(ps), &V_I2(pd));
    case VT_CY:       return VarI2FromCy(V_CY(ps), &V_I2(pd));
    case VT_DECIMAL:  return VarI2FromDec(&V_DECIMAL(ps), &V_I2(pd));
    case VT_DISPATCH: return VarI2FromDisp(V_DISPATCH(ps), lcid, &V_I2(pd));
    case VT_BSTR:     return VarI2FromStr(V_BSTR(ps), lcid, dwFlags, &V_I2(pd));
    }
    break;

  case VT_I4:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_I4(pd) = 0; return S_OK;
    case VT_I1:       return VarI4FromI1(V_I1(ps), &V_I4(pd));
    case VT_I2:       return VarI4FromI2(V_I2(ps), &V_I4(pd));
    case VT_UI1:      return VarI4FromUI1(V_UI1(ps), &V_I4(pd));
    case VT_UI2:      return VarI4FromUI2(V_UI2(ps), &V_I4(pd));
    case VT_UI4:      V_I4(pd) = V_UI4(ps); return S_OK;
    case VT_I8:       return VarI4FromI8(V_I8(ps), &V_I4(pd));
    case VT_UI8:      return VarI4FromUI8(V_UI8(ps), &V_I4(pd));
    case VT_R4:       return VarI4FromR4(V_R4(ps), &V_I4(pd));
    case VT_R8:       return VarI4FromR8(V_R8(ps), &V_I4(pd));
    case VT_DATE:     return VarI4FromDate(V_DATE(ps), &V_I4(pd));
    case VT_BOOL:     return VarI4FromBool(V_BOOL(ps), &V_I4(pd));
    case VT_CY:       return VarI4FromCy(V_CY(ps), &V_I4(pd));
    case VT_DECIMAL:  return VarI4FromDec(&V_DECIMAL(ps), &V_I4(pd));
    case VT_DISPATCH: return VarI4FromDisp(V_DISPATCH(ps), lcid, &V_I4(pd));
    case VT_BSTR:     return VarI4FromStr(V_BSTR(ps), lcid, dwFlags, &V_I4(pd));
    }
    break;

  case VT_UI1:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI1(pd) = 0; return S_OK;
    case VT_I1:       V_UI1(pd) = V_I1(ps); return S_OK;
    case VT_I2:       return VarUI1FromI2(V_I2(ps), &V_UI1(pd));
    case VT_I4:       return VarUI1FromI4(V_I4(ps), &V_UI1(pd));
    case VT_UI2:      return VarUI1FromUI2(V_UI2(ps), &V_UI1(pd));
    case VT_UI4:      return VarUI1FromUI4(V_UI4(ps), &V_UI1(pd));
    case VT_I8:       return VarUI1FromI8(V_I8(ps), &V_UI1(pd));
    case VT_UI8:      return VarUI1FromUI8(V_UI8(ps), &V_UI1(pd));
    case VT_R4:       return VarUI1FromR4(V_R4(ps), &V_UI1(pd));
    case VT_R8:       return VarUI1FromR8(V_R8(ps), &V_UI1(pd));
    case VT_DATE:     return VarUI1FromDate(V_DATE(ps), &V_UI1(pd));
    case VT_BOOL:     return VarUI1FromBool(V_BOOL(ps), &V_UI1(pd));
    case VT_CY:       return VarUI1FromCy(V_CY(ps), &V_UI1(pd));
    case VT_DECIMAL:  return VarUI1FromDec(&V_DECIMAL(ps), &V_UI1(pd));
    case VT_DISPATCH: return VarUI1FromDisp(V_DISPATCH(ps), lcid, &V_UI1(pd));
    case VT_BSTR:     return VarUI1FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI1(pd));
    }
    break;

  case VT_UI2:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI2(pd) = 0; return S_OK;
    case VT_I1:       return VarUI2FromI1(V_I1(ps), &V_UI2(pd));
    case VT_I2:       V_UI2(pd) = V_I2(ps); return S_OK;
    case VT_I4:       return VarUI2FromI4(V_I4(ps), &V_UI2(pd));
    case VT_UI1:      return VarUI2FromUI1(V_UI1(ps), &V_UI2(pd));
    case VT_UI4:      return VarUI2FromUI4(V_UI4(ps), &V_UI2(pd));
    case VT_I8:       return VarUI4FromI8(V_I8(ps), &V_UI4(pd));
    case VT_UI8:      return VarUI4FromUI8(V_UI8(ps), &V_UI4(pd));
    case VT_R4:       return VarUI2FromR4(V_R4(ps), &V_UI2(pd));
    case VT_R8:       return VarUI2FromR8(V_R8(ps), &V_UI2(pd));
    case VT_DATE:     return VarUI2FromDate(V_DATE(ps), &V_UI2(pd));
    case VT_BOOL:     return VarUI2FromBool(V_BOOL(ps), &V_UI2(pd));
    case VT_CY:       return VarUI2FromCy(V_CY(ps), &V_UI2(pd));
    case VT_DECIMAL:  return VarUI2FromDec(&V_DECIMAL(ps), &V_UI2(pd));
    case VT_DISPATCH: return VarUI2FromDisp(V_DISPATCH(ps), lcid, &V_UI2(pd));
    case VT_BSTR:     return VarUI2FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI2(pd));
    }
    break;

  case VT_UI4:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI4(pd) = 0; return S_OK;
    case VT_I1:       return VarUI4FromI1(V_I1(ps), &V_UI4(pd));
    case VT_I2:       return VarUI4FromI2(V_I2(ps), &V_UI4(pd));
    case VT_I4:       V_UI4(pd) = V_I4(ps); return S_OK;
    case VT_UI1:      return VarUI4FromUI1(V_UI1(ps), &V_UI4(pd));
    case VT_UI2:      return VarUI4FromUI2(V_UI2(ps), &V_UI4(pd));
    case VT_I8:       return VarUI4FromI8(V_I8(ps), &V_UI4(pd));
    case VT_UI8:      return VarUI4FromUI8(V_UI8(ps), &V_UI4(pd));
    case VT_R4:       return VarUI4FromR4(V_R4(ps), &V_UI4(pd));
    case VT_R8:       return VarUI4FromR8(V_R8(ps), &V_UI4(pd));
    case VT_DATE:     return VarUI4FromDate(V_DATE(ps), &V_UI4(pd));
    case VT_BOOL:     return VarUI4FromBool(V_BOOL(ps), &V_UI4(pd));
    case VT_CY:       return VarUI4FromCy(V_CY(ps), &V_UI4(pd));
    case VT_DECIMAL:  return VarUI4FromDec(&V_DECIMAL(ps), &V_UI4(pd));
    case VT_DISPATCH: return VarUI4FromDisp(V_DISPATCH(ps), lcid, &V_UI4(pd));
    case VT_BSTR:     return VarUI4FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI4(pd));
    }
    break;

  case VT_UI8:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI8(pd) = 0; return S_OK;
    case VT_I4:       if (V_I4(ps) < 0) return DISP_E_OVERFLOW; V_UI8(pd) = V_I4(ps); return S_OK;
    case VT_I1:       return VarUI8FromI1(V_I1(ps), &V_UI8(pd));
    case VT_I2:       return VarUI8FromI2(V_I2(ps), &V_UI8(pd));
    case VT_UI1:      return VarUI8FromUI1(V_UI1(ps), &V_UI8(pd));
    case VT_UI2:      return VarUI8FromUI2(V_UI2(ps), &V_UI8(pd));
    case VT_UI4:      return VarUI8FromUI4(V_UI4(ps), &V_UI8(pd));
    case VT_I8:       V_UI8(pd) = V_I8(ps); return S_OK;
    case VT_R4:       return VarUI8FromR4(V_R4(ps), &V_UI8(pd));
    case VT_R8:       return VarUI8FromR8(V_R8(ps), &V_UI8(pd));
    case VT_DATE:     return VarUI8FromDate(V_DATE(ps), &V_UI8(pd));
    case VT_BOOL:     return VarUI8FromBool(V_BOOL(ps), &V_UI8(pd));
    case VT_CY:       return VarUI8FromCy(V_CY(ps), &V_UI8(pd));
    case VT_DECIMAL:  return VarUI8FromDec(&V_DECIMAL(ps), &V_UI8(pd));
    case VT_DISPATCH: return VarUI8FromDisp(V_DISPATCH(ps), lcid, &V_UI8(pd));
    case VT_BSTR:     return VarUI8FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI8(pd));
    }
    break;

  case VT_I8:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_I8(pd) = 0; return S_OK;
    case VT_I4:       V_I8(pd) = V_I4(ps); return S_OK;
    case VT_I1:       return VarI8FromI1(V_I1(ps), &V_I8(pd));
    case VT_I2:       return VarI8FromI2(V_I2(ps), &V_I8(pd));
    case VT_UI1:      return VarI8FromUI1(V_UI1(ps), &V_I8(pd));
    case VT_UI2:      return VarI8FromUI2(V_UI2(ps), &V_I8(pd));
    case VT_UI4:      return VarI8FromUI4(V_UI4(ps), &V_I8(pd));
    case VT_UI8:      V_I8(pd) = V_UI8(ps); return S_OK;
    case VT_R4:       return VarI8FromR4(V_R4(ps), &V_I8(pd));
    case VT_R8:       return VarI8FromR8(V_R8(ps), &V_I8(pd));
    case VT_DATE:     return VarI8FromDate(V_DATE(ps), &V_I8(pd));
    case VT_BOOL:     return VarI8FromBool(V_BOOL(ps), &V_I8(pd));
    case VT_CY:       return VarI8FromCy(V_CY(ps), &V_I8(pd));
    case VT_DECIMAL:  return VarI8FromDec(&V_DECIMAL(ps), &V_I8(pd));
    case VT_DISPATCH: return VarI8FromDisp(V_DISPATCH(ps), lcid, &V_I8(pd));
    case VT_BSTR:     return VarI8FromStr(V_BSTR(ps), lcid, dwFlags, &V_I8(pd));
    }
    break;

  case VT_R4:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_R4(pd) = 0.0f; return S_OK;
    case VT_I1:       return VarR4FromI1(V_I1(ps), &V_R4(pd));
    case VT_I2:       return VarR4FromI2(V_I2(ps), &V_R4(pd));
    case VT_I4:       return VarR4FromI4(V_I4(ps), &V_R4(pd));
    case VT_UI1:      return VarR4FromUI1(V_UI1(ps), &V_R4(pd));
    case VT_UI2:      return VarR4FromUI2(V_UI2(ps), &V_R4(pd));
    case VT_UI4:      return VarR4FromUI4(V_UI4(ps), &V_R4(pd));
    case VT_I8:       return VarR4FromI8(V_I8(ps), &V_R4(pd));
    case VT_UI8:      return VarR4FromUI8(V_UI8(ps), &V_R4(pd));
    case VT_R8:       return VarR4FromR8(V_R8(ps), &V_R4(pd));
    case VT_DATE:     return VarR4FromDate(V_DATE(ps), &V_R4(pd));
    case VT_BOOL:     return VarR4FromBool(V_BOOL(ps), &V_R4(pd));
    case VT_CY:       return VarR4FromCy(V_CY(ps), &V_R4(pd));
    case VT_DECIMAL:  return VarR4FromDec(&V_DECIMAL(ps), &V_R4(pd));
    case VT_DISPATCH: return VarR4FromDisp(V_DISPATCH(ps), lcid, &V_R4(pd));
    case VT_BSTR:     return VarR4FromStr(V_BSTR(ps), lcid, dwFlags, &V_R4(pd));
    }
    break;

  case VT_R8:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_R8(pd) = 0.0; return S_OK;
    case VT_I1:       return VarR8FromI1(V_I1(ps), &V_R8(pd));
    case VT_I2:       return VarR8FromI2(V_I2(ps), &V_R8(pd));
    case VT_I4:       return VarR8FromI4(V_I4(ps), &V_R8(pd));
    case VT_UI1:      return VarR8FromUI1(V_UI1(ps), &V_R8(pd));
    case VT_UI2:      return VarR8FromUI2(V_UI2(ps), &V_R8(pd));
    case VT_UI4:      return VarR8FromUI4(V_UI4(ps), &V_R8(pd));
    case VT_I8:       return VarR8FromI8(V_I8(ps), &V_R8(pd));
    case VT_UI8:      return VarR8FromUI8(V_UI8(ps), &V_R8(pd));
    case VT_R4:       return VarR8FromR4(V_R4(ps), &V_R8(pd));
    case VT_DATE:     return VarR8FromDate(V_DATE(ps), &V_R8(pd));
    case VT_BOOL:     return VarR8FromBool(V_BOOL(ps), &V_R8(pd));
    case VT_CY:       return VarR8FromCy(V_CY(ps), &V_R8(pd));
    case VT_DECIMAL:  return VarR8FromDec(&V_DECIMAL(ps), &V_R8(pd));
    case VT_DISPATCH: return VarR8FromDisp(V_DISPATCH(ps), lcid, &V_R8(pd));
    case VT_BSTR:     return VarR8FromStr(V_BSTR(ps), lcid, dwFlags, &V_R8(pd));
    }
    break;

  case VT_DATE:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_DATE(pd) = 0.0; return S_OK;
    case VT_I1:       return VarDateFromI1(V_I1(ps), &V_DATE(pd));
    case VT_I2:       return VarDateFromI2(V_I2(ps), &V_DATE(pd));
    case VT_I4:       return VarDateFromI4(V_I4(ps), &V_DATE(pd));
    case VT_UI1:      return VarDateFromUI1(V_UI1(ps), &V_DATE(pd));
    case VT_UI2:      return VarDateFromUI2(V_UI2(ps), &V_DATE(pd));
    case VT_UI4:      return VarDateFromUI4(V_UI4(ps), &V_DATE(pd));
    case VT_I8:       return VarDateFromI8(V_I8(ps), &V_DATE(pd));
    case VT_UI8:      return VarDateFromUI8(V_UI8(ps), &V_DATE(pd));
    case VT_R4:       return VarDateFromR4(V_R4(ps), &V_DATE(pd));
    case VT_R8:       return VarDateFromR8(V_R8(ps), &V_DATE(pd));
    case VT_BOOL:     return VarDateFromBool(V_BOOL(ps), &V_DATE(pd));
    case VT_CY:       return VarDateFromCy(V_CY(ps), &V_DATE(pd));
    case VT_DECIMAL:  return VarDateFromDec(&V_DECIMAL(ps), &V_DATE(pd));
    case VT_DISPATCH: return VarDateFromDisp(V_DISPATCH(ps), lcid, &V_DATE(pd));
    case VT_BSTR:     return VarDateFromStr(V_BSTR(ps), lcid, dwFlags, &V_DATE(pd));
    }
    break;

  case VT_BOOL:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_BOOL(pd) = 0; return S_OK;
    case VT_I1:       return VarBoolFromI1(V_I1(ps), &V_BOOL(pd));
    case VT_I2:       return VarBoolFromI2(V_I2(ps), &V_BOOL(pd));
    case VT_I4:       return VarBoolFromI4(V_I4(ps), &V_BOOL(pd));
    case VT_UI1:      return VarBoolFromUI1(V_UI1(ps), &V_BOOL(pd));
    case VT_UI2:      return VarBoolFromUI2(V_UI2(ps), &V_BOOL(pd));
    case VT_UI4:      return VarBoolFromUI4(V_UI4(ps), &V_BOOL(pd));
    case VT_I8:       return VarBoolFromI8(V_I8(ps), &V_BOOL(pd));
    case VT_UI8:      return VarBoolFromUI8(V_UI8(ps), &V_BOOL(pd));
    case VT_R4:       return VarBoolFromR4(V_R4(ps), &V_BOOL(pd));
    case VT_R8:       return VarBoolFromR8(V_R8(ps), &V_BOOL(pd));
    case VT_DATE:     return VarBoolFromDate(V_DATE(ps), &V_BOOL(pd));
    case VT_CY:       return VarBoolFromCy(V_CY(ps), &V_BOOL(pd));
    case VT_DECIMAL:  return VarBoolFromDec(&V_DECIMAL(ps), &V_BOOL(pd));
    case VT_DISPATCH: return VarBoolFromDisp(V_DISPATCH(ps), lcid, &V_BOOL(pd));
    case VT_BSTR:     return VarBoolFromStr(V_BSTR(ps), lcid, dwFlags, &V_BOOL(pd));
    }
    break;

  case VT_BSTR:
    switch (vtFrom)
    {
    case VT_EMPTY:
      V_BSTR(pd) = SysAllocStringLen(NULL, 0);
      return V_BSTR(pd) ? S_OK : E_OUTOFMEMORY;
    case VT_BOOL:
      if (wFlags & (VARIANT_ALPHABOOL|VARIANT_LOCALBOOL))
         return VarBstrFromBool(V_BOOL(ps), lcid, dwFlags, &V_BSTR(pd));
      return VarBstrFromI2(V_BOOL(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_I1:       return VarBstrFromI1(V_I1(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_I2:       return VarBstrFromI2(V_I2(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_I4:       return VarBstrFromI4(V_I4(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_UI1:      return VarBstrFromUI1(V_UI1(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_UI2:      return VarBstrFromUI2(V_UI2(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_UI4:      return VarBstrFromUI4(V_UI4(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_I8:       return VarBstrFromI8(V_I8(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_UI8:      return VarBstrFromUI8(V_UI8(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_R4:       return VarBstrFromR4(V_R4(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_R8:       return VarBstrFromR8(V_R8(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_DATE:     return VarBstrFromDate(V_DATE(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_CY:       return VarBstrFromCy(V_CY(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_DECIMAL:  return VarBstrFromDec(&V_DECIMAL(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_DISPATCH: return VarBstrFromDisp(V_DISPATCH(ps), lcid, dwFlags, &V_BSTR(pd));
    }
    break;

  case VT_CY:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_CY(pd).int64 = 0; return S_OK;
    case VT_I1:       return VarCyFromI1(V_I1(ps), &V_CY(pd));
    case VT_I2:       return VarCyFromI2(V_I2(ps), &V_CY(pd));
    case VT_I4:       return VarCyFromI4(V_I4(ps), &V_CY(pd));
    case VT_UI1:      return VarCyFromUI1(V_UI1(ps), &V_CY(pd));
    case VT_UI2:      return VarCyFromUI2(V_UI2(ps), &V_CY(pd));
    case VT_UI4:      return VarCyFromUI4(V_UI4(ps), &V_CY(pd));
    case VT_I8:       return VarCyFromI8(V_I8(ps), &V_CY(pd));
    case VT_UI8:      return VarCyFromUI8(V_UI8(ps), &V_CY(pd));
    case VT_R4:       return VarCyFromR4(V_R4(ps), &V_CY(pd));
    case VT_R8:       return VarCyFromR8(V_R8(ps), &V_CY(pd));
    case VT_DATE:     return VarCyFromDate(V_DATE(ps), &V_CY(pd));
    case VT_BOOL:     return VarCyFromBool(V_BOOL(ps), &V_CY(pd));
    case VT_DECIMAL:  return VarCyFromDec(&V_DECIMAL(ps), &V_CY(pd));
    case VT_DISPATCH: return VarCyFromDisp(V_DISPATCH(ps), lcid, &V_CY(pd));
    case VT_BSTR:     return VarCyFromStr(V_BSTR(ps), lcid, dwFlags, &V_CY(pd));
    }
    break;

  case VT_DECIMAL:
    switch (vtFrom)
    {
    case VT_EMPTY:
    case VT_BOOL:
       V_DECIMAL(pd).sign = DECIMAL_POS;
       V_DECIMAL(pd).scale = 0;
       V_DECIMAL(pd).Hi32 = 0;
        /* VarDecFromBool() coerces to -1/0, ChangeTypeEx() coerces to 1/0.
         * VT_NULL and VT_EMPTY always give a 0 value.
         */
       V_DECIMAL(pd).Lo64 = vtFrom == VT_BOOL && V_BOOL(ps) ? 1 : 0;
       return S_OK;
    case VT_I1:       return VarDecFromI1(V_I1(ps), &V_DECIMAL(pd));
    case VT_I2:       return VarDecFromI2(V_I2(ps), &V_DECIMAL(pd));
    case VT_I4:       return VarDecFromI4(V_I4(ps), &V_DECIMAL(pd));
    case VT_UI1:      return VarDecFromUI1(V_UI1(ps), &V_DECIMAL(pd));
    case VT_UI2:      return VarDecFromUI2(V_UI2(ps), &V_DECIMAL(pd));
    case VT_UI4:      return VarDecFromUI4(V_UI4(ps), &V_DECIMAL(pd));
    case VT_I8:       return VarDecFromI8(V_I8(ps), &V_DECIMAL(pd));
    case VT_UI8:      return VarDecFromUI8(V_UI8(ps), &V_DECIMAL(pd));
    case VT_R4:       return VarDecFromR4(V_R4(ps), &V_DECIMAL(pd));
    case VT_R8:       return VarDecFromR8(V_R8(ps), &V_DECIMAL(pd));
    case VT_DATE:     return VarDecFromDate(V_DATE(ps), &V_DECIMAL(pd));
    case VT_CY:       return VarDecFromCy(V_CY(ps), &V_DECIMAL(pd));
    case VT_DISPATCH: return VarDecFromDisp(V_DISPATCH(ps), lcid, &V_DECIMAL(pd));
    case VT_BSTR:     return VarDecFromStr(V_BSTR(ps), lcid, dwFlags, &V_DECIMAL(pd));
    }
    break;

  case VT_UNKNOWN:
    switch (vtFrom)
    {
    case VT_DISPATCH:
      if (V_DISPATCH(ps) == NULL)
      {
        V_UNKNOWN(pd) = NULL;
        res = S_OK;
      }
      else
        res = IDispatch_QueryInterface(V_DISPATCH(ps), &IID_IUnknown, (LPVOID*)&V_UNKNOWN(pd));
      break;
    }
    break;

  case VT_DISPATCH:
    switch (vtFrom)
    {
    case VT_UNKNOWN:
      if (V_UNKNOWN(ps) == NULL)
      {
        V_DISPATCH(pd) = NULL;
        res = S_OK;
      }
      else
        res = IUnknown_QueryInterface(V_UNKNOWN(ps), &IID_IDispatch, (LPVOID*)&V_DISPATCH(pd));
      break;
    }
    break;

  case VT_RECORD:
    break;
  }
  return res;
}

/* Coerce to/from an array */
static inline HRESULT VARIANT_CoerceArray(VARIANTARG* pd, VARIANTARG* ps, VARTYPE vt)
{
  if (vt == VT_BSTR && V_VT(ps) == (VT_ARRAY|VT_UI1))
    return BstrFromVector(V_ARRAY(ps), &V_BSTR(pd));

  if (V_VT(ps) == VT_BSTR && vt == (VT_ARRAY|VT_UI1))
    return VectorFromBstr(V_BSTR(ps), &V_ARRAY(pd));

  if (V_VT(ps) == vt)
    return SafeArrayCopy(V_ARRAY(ps), &V_ARRAY(pd));

  return DISP_E_TYPEMISMATCH;
}

static HRESULT VARIANT_FetchDispatchValue(LPVARIANT pvDispatch, LPVARIANT pValue)
{
    DISPPARAMS params = { 0 };
    HRESULT hres;

    if ((V_VT(pvDispatch) & VT_TYPEMASK) == VT_DISPATCH) {
        if (NULL == V_DISPATCH(pvDispatch)) return DISP_E_TYPEMISMATCH;
        hres = IDispatch_Invoke(V_DISPATCH(pvDispatch), DISPID_VALUE, &IID_NULL,
            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, pValue,
            NULL, NULL);
    } else {
        hres = DISP_E_TYPEMISMATCH;
    }
    return hres;
}

/******************************************************************************
 * Check if a variants type is valid.
 */
static inline HRESULT VARIANT_ValidateType(VARTYPE vt)
{
  VARTYPE vtExtra = vt & VT_EXTRA_TYPE;

  vt &= VT_TYPEMASK;

  if (!(vtExtra & (VT_VECTOR|VT_RESERVED)))
  {
    if (vt < VT_VOID || vt == VT_RECORD || vt == VT_CLSID)
    {
      if ((vtExtra & (VT_BYREF|VT_ARRAY)) && vt <= VT_NULL)
        return DISP_E_BADVARTYPE;
      if (vt != (VARTYPE)15)
        return S_OK;
    }
  }
  return DISP_E_BADVARTYPE;
}

/******************************************************************************
 *		VariantInit	[OLEAUT32.8]
 *
 * Since Windows 8.1 whole structure is initialized, before that only type field was reset to VT_EMPTY.
 */
void WINAPI VariantInit(VARIANTARG* pVarg)
{
    TRACE("(%p)\n", pVarg);

    memset(pVarg, 0, sizeof(*pVarg));
}

HRESULT VARIANT_ClearInd(VARIANTARG *pVarg)
{
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(pVarg));

    hres = VARIANT_ValidateType(V_VT(pVarg));
    if (FAILED(hres))
        return hres;

    switch (V_VT(pVarg))
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
        if (V_UNKNOWN(pVarg))
            IUnknown_Release(V_UNKNOWN(pVarg));
        break;
    case VT_UNKNOWN | VT_BYREF:
    case VT_DISPATCH | VT_BYREF:
        if(*V_UNKNOWNREF(pVarg))
            IUnknown_Release(*V_UNKNOWNREF(pVarg));
        break;
    case VT_BSTR:
        SysFreeString(V_BSTR(pVarg));
        break;
    case VT_BSTR | VT_BYREF:
        SysFreeString(*V_BSTRREF(pVarg));
        break;
    case VT_VARIANT | VT_BYREF:
        VariantClear(V_VARIANTREF(pVarg));
        break;
    case VT_RECORD:
    case VT_RECORD | VT_BYREF:
    {
        IRecordInfo *rec_info = V_RECORDINFO(pVarg);
        if (rec_info)
        {
            IRecordInfo_RecordClear(rec_info, V_RECORD(pVarg));
            IRecordInfo_Release(rec_info);
        }
        break;
    }
    default:
        if (V_ISARRAY(pVarg) || (V_VT(pVarg) & ~VT_BYREF) == VT_SAFEARRAY)
        {
            if (V_ISBYREF(pVarg))
            {
                if (*V_ARRAYREF(pVarg))
                    hres = SafeArrayDestroy(*V_ARRAYREF(pVarg));
            }
            else if (V_ARRAY(pVarg))
                hres = SafeArrayDestroy(V_ARRAY(pVarg));
        }
        break;
    }

    V_VT(pVarg) = VT_EMPTY;
    return hres;
}

/******************************************************************************
 *		VariantClear	[OLEAUT32.9]
 *
 * Clear a variant.
 *
 * PARAMS
 *  pVarg [I/O] Variant to clear
 *
 * RETURNS
 *  Success: S_OK. Any previous value in pVarg is freed and its type is set to VT_EMPTY.
 *  Failure: DISP_E_BADVARTYPE, if the variant is not a valid variant type.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH VariantClear(VARIANTARG* pVarg)
{
  HRESULT hres;

  TRACE("(%s)\n", debugstr_variant(pVarg));

  hres = VARIANT_ValidateType(V_VT(pVarg));

  if (SUCCEEDED(hres))
  {
    if (!V_ISBYREF(pVarg))
    {
      if (V_ISARRAY(pVarg) || V_VT(pVarg) == VT_SAFEARRAY)
      {
        hres = SafeArrayDestroy(V_ARRAY(pVarg));
      }
      else if (V_VT(pVarg) == VT_BSTR)
      {
        SysFreeString(V_BSTR(pVarg));
      }
      else if (V_VT(pVarg) == VT_RECORD)
      {
        IRecordInfo *rec_info = V_RECORDINFO(pVarg);
        if (rec_info)
        {
          IRecordInfo_RecordClear(rec_info, V_RECORD(pVarg));
          IRecordInfo_Release(rec_info);
        }
      }
      else if (V_VT(pVarg) == VT_DISPATCH ||
               V_VT(pVarg) == VT_UNKNOWN)
      {
        if (V_UNKNOWN(pVarg))
          IUnknown_Release(V_UNKNOWN(pVarg));
      }
    }
    V_VT(pVarg) = VT_EMPTY;
  }
  return hres;
}

/******************************************************************************
 * Copy an IRecordInfo object contained in a variant.
 */
static HRESULT VARIANT_CopyIRecordInfo(VARIANT *dest, const VARIANT *src)
{
  IRecordInfo *src_info = V_RECORDINFO(src);
  HRESULT hr = S_OK;
  ULONG size;

  if (!src_info)
  {
    if (V_RECORD(src)) return E_INVALIDARG;
    return S_OK;
  }

  hr = IRecordInfo_GetSize(src_info, &size);
  if (FAILED(hr)) return hr;

  /* Windows does not use RecordCreate() here, memory should be allocated in compatible way so RecordDestroy()
     could free it later. */
  V_RECORD(dest) = CoTaskMemAlloc(size);
  if (!V_RECORD(dest)) return E_OUTOFMEMORY;
  if (size)
      memset(V_RECORD(dest), 0, size);

  IRecordInfo_AddRef(V_RECORDINFO(dest) = src_info);
  return IRecordInfo_RecordCopy(src_info, V_RECORD(src), V_RECORD(dest));
}

/******************************************************************************
 *    VariantCopy  [OLEAUT32.10]
 *
 * Copy a variant.
 *
 * PARAMS
 *  pvargDest [O] Destination for copy
 *  pvargSrc  [I] Source variant to copy
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains a copy of pvargSrc.
 *  Failure: DISP_E_BADVARTYPE, if either variant has an invalid type.
 *           E_OUTOFMEMORY, if memory cannot be allocated. Otherwise an
 *           HRESULT error code from SafeArrayCopy(), IRecordInfo_GetSize(),
 *           or IRecordInfo_RecordCopy(), depending on the type of pvargSrc.
 *
 * NOTES
 *  - If pvargSrc == pvargDest, this function does nothing, and succeeds if
 *    pvargSrc is valid. Otherwise, pvargDest is always cleared using
 *    VariantClear() before pvargSrc is copied to it. If clearing pvargDest
 *    fails, so does this function.
 *  - VT_CLSID is a valid type type for pvargSrc, but not for pvargDest.
 *  - For by-value non-intrinsic types, a deep copy is made, i.e. The whole value
 *    is copied rather than just any pointers to it.
 *  - For by-value object types the object pointer is copied and the objects
 *    reference count increased using IUnknown_AddRef().
 *  - For all by-reference types, only the referencing pointer is copied.
 */
HRESULT WINAPI VariantCopy(VARIANTARG* pvargDest, const VARIANTARG* pvargSrc)
{
  HRESULT hres = S_OK;

  TRACE("(%s,%s)\n", debugstr_variant(pvargDest), debugstr_variant(pvargSrc));

  if (V_TYPE(pvargSrc) == VT_CLSID || /* VT_CLSID is a special case */
      FAILED(VARIANT_ValidateType(V_VT(pvargSrc))))
    return DISP_E_BADVARTYPE;

  if (pvargSrc != pvargDest &&
      SUCCEEDED(hres = VariantClear(pvargDest)))
  {
    *pvargDest = *pvargSrc; /* Shallow copy the value */

    if (!V_ISBYREF(pvargSrc))
    {
      switch (V_VT(pvargSrc))
      {
      case VT_BSTR:
        V_BSTR(pvargDest) = SysAllocStringByteLen((char*)V_BSTR(pvargSrc), SysStringByteLen(V_BSTR(pvargSrc)));
        if (!V_BSTR(pvargDest))
          hres = E_OUTOFMEMORY;
        break;
      case VT_RECORD:
        hres = VARIANT_CopyIRecordInfo(pvargDest, pvargSrc);
        break;
      case VT_DISPATCH:
      case VT_UNKNOWN:
        V_UNKNOWN(pvargDest) = V_UNKNOWN(pvargSrc);
        if (V_UNKNOWN(pvargSrc))
          IUnknown_AddRef(V_UNKNOWN(pvargSrc));
        break;
      default:
        if (V_ISARRAY(pvargSrc))
          hres = SafeArrayCopy(V_ARRAY(pvargSrc), &V_ARRAY(pvargDest));
      }
    }
  }
  return hres;
}

/* Return the byte size of a variants data */
static inline size_t VARIANT_DataSize(const VARIANT* pv)
{
  switch (V_TYPE(pv))
  {
  case VT_I1:
  case VT_UI1:   return sizeof(BYTE);
  case VT_I2:
  case VT_UI2:   return sizeof(SHORT);
  case VT_INT:
  case VT_UINT:
  case VT_I4:
  case VT_UI4:   return sizeof(LONG);
  case VT_I8:
  case VT_UI8:   return sizeof(LONGLONG);
  case VT_R4:    return sizeof(float);
  case VT_R8:    return sizeof(double);
  case VT_DATE:  return sizeof(DATE);
  case VT_BOOL:  return sizeof(VARIANT_BOOL);
  case VT_DISPATCH:
  case VT_UNKNOWN:
  case VT_BSTR:  return sizeof(void*);
  case VT_CY:    return sizeof(CY);
  case VT_ERROR: return sizeof(SCODE);
  }
  TRACE("Shouldn't be called for variant %s!\n", debugstr_variant(pv));
  return 0;
}

/******************************************************************************
 *    VariantCopyInd  [OLEAUT32.11]
 *
 * Copy a variant, dereferencing it if it is by-reference.
 *
 * PARAMS
 *  pvargDest [O] Destination for copy
 *  pvargSrc  [I] Source variant to copy
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains a copy of pvargSrc.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Failure: DISP_E_BADVARTYPE, if either variant has an invalid by-value type.
 *           E_INVALIDARG, if pvargSrc  is an invalid by-reference type.
 *           E_OUTOFMEMORY, if memory cannot be allocated. Otherwise an
 *           HRESULT error code from SafeArrayCopy(), IRecordInfo_GetSize(),
 *           or IRecordInfo_RecordCopy(), depending on the type of pvargSrc.
 *
 * NOTES
 *  - If pvargSrc is by-value, this function behaves exactly as VariantCopy().
 *  - If pvargSrc is by-reference, the value copied to pvargDest is the pointed-to
 *    value.
 *  - if pvargSrc == pvargDest, this function dereferences in place. Otherwise,
 *    pvargDest is always cleared using VariantClear() before pvargSrc is copied
 *    to it. If clearing pvargDest fails, so does this function.
 */
HRESULT WINAPI VariantCopyInd(VARIANT* pvargDest, const VARIANTARG* pvargSrc)
{
  const VARIANTARG *pSrc = pvargSrc;
  VARIANTARG vTmp;
  VARTYPE vt;
  HRESULT hres = S_OK;

  TRACE("(%s,%s)\n", debugstr_variant(pvargDest), debugstr_variant(pvargSrc));

  if (!V_ISBYREF(pvargSrc))
    return VariantCopy(pvargDest, pvargSrc);

  /* Argument checking is more lax than VariantCopy()... */
  vt = V_TYPE(pvargSrc);
  if (V_ISARRAY(pvargSrc) || (V_VT(pvargSrc) == (VT_RECORD|VT_BYREF)) ||
     (vt > VT_NULL && vt != (VARTYPE)15 && vt < VT_VOID &&
     !(V_VT(pvargSrc) & (VT_VECTOR|VT_RESERVED))))
  {
    /* OK */
  }
  else
    return E_INVALIDARG; /* ...And the return value for invalid types differs too */

  if (pvargSrc == pvargDest)
  {
    /* In place copy. Use a shallow copy of pvargSrc & init pvargDest.
     * This avoids an expensive VariantCopy() call - e.g. SafeArrayCopy().
     */
    vTmp = *pvargSrc;
    pSrc = &vTmp;
    V_VT(pvargDest) = VT_EMPTY;
  }
  else
  {
    /* Copy into another variant. Free the variant in pvargDest */
    if (FAILED(hres = VariantClear(pvargDest)))
    {
      TRACE("VariantClear() of destination failed\n");
      return hres;
    }
  }

  if (V_ISARRAY(pSrc))
  {
    /* Native doesn't check that *V_ARRAYREF(pSrc) is valid */
    hres = SafeArrayCopy(*V_ARRAYREF(pSrc), &V_ARRAY(pvargDest));
  }
  else if (V_VT(pSrc) == (VT_BSTR|VT_BYREF))
  {
    /* Native doesn't check that *V_BSTRREF(pSrc) is valid */
    V_BSTR(pvargDest) = SysAllocStringByteLen((char*)*V_BSTRREF(pSrc), SysStringByteLen(*V_BSTRREF(pSrc)));
  }
  else if (V_VT(pSrc) == (VT_RECORD|VT_BYREF))
  {
    hres = VARIANT_CopyIRecordInfo(pvargDest, pvargSrc);
  }
  else if (V_VT(pSrc) == (VT_DISPATCH|VT_BYREF) ||
           V_VT(pSrc) == (VT_UNKNOWN|VT_BYREF))
  {
    /* Native doesn't check that *V_UNKNOWNREF(pSrc) is valid */
    V_UNKNOWN(pvargDest) = *V_UNKNOWNREF(pSrc);
    if (*V_UNKNOWNREF(pSrc))
      IUnknown_AddRef(*V_UNKNOWNREF(pSrc));
  }
  else if (V_VT(pSrc) == (VT_VARIANT|VT_BYREF))
  {
    /* Native doesn't check that *V_VARIANTREF(pSrc) is valid */
    if (V_VT(V_VARIANTREF(pSrc)) == (VT_VARIANT|VT_BYREF))
      hres = E_INVALIDARG; /* Don't dereference more than one level */
    else
      hres = VariantCopyInd(pvargDest, V_VARIANTREF(pSrc));

    /* Use the dereferenced variants type value, not VT_VARIANT */
    goto VariantCopyInd_Return;
  }
  else if (V_VT(pSrc) == (VT_DECIMAL|VT_BYREF))
  {
    memcpy(&V_DECIMAL(pvargDest).scale, &V_DECIMALREF(pSrc)->scale,
           sizeof(DECIMAL) - sizeof(USHORT));
  }
  else
  {
    /* Copy the pointed to data into this variant */
    memcpy(&V_BYREF(pvargDest), V_BYREF(pSrc), VARIANT_DataSize(pSrc));
  }

  V_VT(pvargDest) = V_VT(pSrc) & ~VT_BYREF;

VariantCopyInd_Return:

  if (pSrc != pvargSrc)
    VariantClear(&vTmp);

  TRACE("returning %#lx, %s\n", hres, debugstr_variant(pvargDest));
  return hres;
}

/******************************************************************************
 *    VariantChangeType  [OLEAUT32.12]
 *
 * Change the type of a variant.
 *
 * PARAMS
 *  pvargDest [O] Destination for the converted variant
 *  pvargSrc  [O] Source variant to change the type of
 *  wFlags    [I] VARIANT_ flags from "oleauto.h"
 *  vt        [I] Variant type to change pvargSrc into
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains the converted value.
 *  Failure: An HRESULT error code describing the failure.
 *
 * NOTES
 *  The LCID used for the conversion is LOCALE_USER_DEFAULT.
 *  See VariantChangeTypeEx.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH VariantChangeType(VARIANTARG* pvargDest, const VARIANTARG* pvargSrc,
                                                   USHORT wFlags, VARTYPE vt)
{
  return VariantChangeTypeEx( pvargDest, pvargSrc, LOCALE_USER_DEFAULT, wFlags, vt );
}

/******************************************************************************
 *    VariantChangeTypeEx  [OLEAUT32.147]
 *
 * Change the type of a variant.
 *
 * PARAMS
 *  pvargDest [O] Destination for the converted variant
 *  pvargSrc  [O] Source variant to change the type of
 *  lcid      [I] LCID for the conversion
 *  wFlags    [I] VARIANT_ flags from "oleauto.h"
 *  vt        [I] Variant type to change pvargSrc into
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains the converted value.
 *  Failure: An HRESULT error code describing the failure.
 *
 * NOTES
 *  pvargDest and pvargSrc can point to the same variant to perform an in-place
 *  conversion. If the conversion is successful, pvargSrc will be freed.
 */
HRESULT WINAPI VariantChangeTypeEx(VARIANTARG* pvargDest, const VARIANTARG* pvargSrc,
                                   LCID lcid, USHORT wFlags, VARTYPE vt)
{
  HRESULT res = S_OK;

  TRACE("%s, %s, %#lx, 0x%04x, %s.\n", debugstr_variant(pvargDest),
        debugstr_variant(pvargSrc), lcid, wFlags, debugstr_vt(vt));

  if (vt == VT_CLSID)
    res = DISP_E_BADVARTYPE;
  else
  {
    res = VARIANT_ValidateType(V_VT(pvargSrc));

    if (SUCCEEDED(res))
    {
      res = VARIANT_ValidateType(vt);

      if (SUCCEEDED(res))
      {
        VARIANTARG vTmp, vSrcDeref;

        if(V_ISBYREF(pvargSrc) && !V_BYREF(pvargSrc))
          res = DISP_E_TYPEMISMATCH;
        else
        {
          V_VT(&vTmp) = VT_EMPTY;
          V_VT(&vSrcDeref) = VT_EMPTY;
          VariantClear(&vTmp);
          VariantClear(&vSrcDeref);
        }

        if (SUCCEEDED(res))
        {
          res = VariantCopyInd(&vSrcDeref, pvargSrc);
          if (SUCCEEDED(res))
          {
            if (V_ISARRAY(&vSrcDeref) || (vt & VT_ARRAY))
              res = VARIANT_CoerceArray(&vTmp, &vSrcDeref, vt);
            else
              res = VARIANT_Coerce(&vTmp, lcid, wFlags, &vSrcDeref, vt);

            if (SUCCEEDED(res)) {
                V_VT(&vTmp) = vt;
                res = VariantCopy(pvargDest, &vTmp);
            }
            VariantClear(&vTmp);
            VariantClear(&vSrcDeref);
          }
        }
      }
    }
  }

  TRACE("returning %#lx, %s\n", res, debugstr_variant(pvargDest));
  return res;
}

/* Date Conversions */

#define IsLeapYear(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/* Convert a VT_DATE value to a Julian Date */
static inline int VARIANT_JulianFromDate(int dateIn)
{
  int julianDays = dateIn;

  julianDays -= DATE_MIN; /* Convert to + days from 1 Jan 100 AD */
  julianDays += 1757585;  /* Convert to + days from 23 Nov 4713 BC (Julian) */
  return julianDays;
}

/* Convert a Julian Date to a VT_DATE value */
static inline int VARIANT_DateFromJulian(int dateIn)
{
  int julianDays = dateIn;

  julianDays -= 1757585;  /* Convert to + days from 1 Jan 100 AD */
  julianDays += DATE_MIN; /* Convert to +/- days from 1 Jan 1899 AD */
  return julianDays;
}

/* Convert a Julian date to Day/Month/Year - from PostgreSQL */
static inline void VARIANT_DMYFromJulian(int jd, USHORT *year, USHORT *month, USHORT *day)
{
  int j, i, l, n;

  l = jd + 68569;
  n = l * 4 / 146097;
  l -= (n * 146097 + 3) / 4;
  i = (4000 * (l + 1)) / 1461001;
  l += 31 - (i * 1461) / 4;
  j = (l * 80) / 2447;
  *day = l - (j * 2447) / 80;
  l = j / 11;
  *month = (j + 2) - (12 * l);
  *year = 100 * (n - 49) + i + l;
}

/* Convert Day/Month/Year to a Julian date - from PostgreSQL */
static inline double VARIANT_JulianFromDMY(USHORT year, USHORT month, USHORT day)
{
  int m12 = (month - 14) / 12;

  return ((1461 * (year + 4800 + m12)) / 4 + (367 * (month - 2 - 12 * m12)) / 12 -
           (3 * ((year + 4900 + m12) / 100)) / 4 + day - 32075);
}

/* Macros for accessing DOS format date/time fields */
#define DOS_YEAR(x)   (1980 + (x >> 9))
#define DOS_MONTH(x)  ((x >> 5) & 0xf)
#define DOS_DAY(x)    (x & 0x1f)
#define DOS_HOUR(x)   (x >> 11)
#define DOS_MINUTE(x) ((x >> 5) & 0x3f)
#define DOS_SECOND(x) ((x & 0x1f) << 1)
/* Create a DOS format date/time */
#define DOS_DATE(d,m,y) (d | (m << 5) | ((y-1980) << 9))
#define DOS_TIME(h,m,s) ((s >> 1) | (m << 5) | (h << 11))

/* Roll a date forwards or backwards to correct it */
static HRESULT VARIANT_RollUdate(UDATE *lpUd)
{
  static const BYTE days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  short iYear, iMonth, iDay, iHour, iMinute, iSecond;

  /* interpret values signed */
  iYear   = lpUd->st.wYear;
  iMonth  = lpUd->st.wMonth;
  iDay    = lpUd->st.wDay;
  iHour   = lpUd->st.wHour;
  iMinute = lpUd->st.wMinute;
  iSecond = lpUd->st.wSecond;

  TRACE("Raw date: %d/%d/%d %d:%d:%d\n", iDay, iMonth,
        iYear, iHour, iMinute, iSecond);

  if (iYear > 9999 || iYear < -9999)
    return E_INVALIDARG; /* Invalid value */
  /* Years 0 to 49 are treated as 2000 + year, see also VARIANT_MakeDate() */
  if (0 <= iYear && iYear <= 49)
    iYear += 2000;
  /* Remaining years 50 to 99 are treated as 1900 + year */
  else if (50 <= iYear && iYear <= 99)
    iYear += 1900;

  iMinute += iSecond / 60;
  iSecond  = iSecond % 60;
  iHour   += iMinute / 60;
  iMinute  = iMinute % 60;
  iDay    += iHour / 24;
  iHour    = iHour % 24;
  iYear   += iMonth / 12;
  iMonth   = iMonth % 12;
  if (iMonth<=0) {iMonth+=12; iYear--;}
  while (iDay > days[iMonth])
  {
    if (iMonth == 2 && IsLeapYear(iYear))
      iDay -= 29;
    else
      iDay -= days[iMonth];
    iMonth++;
    iYear += iMonth / 12;
    iMonth = iMonth % 12;
  }
  while (iDay <= 0)
  {
    iMonth--;
    if (iMonth<=0) {iMonth+=12; iYear--;}
    if (iMonth == 2 && IsLeapYear(iYear))
      iDay += 29;
    else
      iDay += days[iMonth];
  }

  if (iSecond<0){iSecond+=60; iMinute--;}
  if (iMinute<0){iMinute+=60; iHour--;}
  if (iHour<0)  {iHour+=24; iDay--;}
  if (iYear<=0)  iYear+=2000;

  lpUd->st.wYear   = iYear;
  lpUd->st.wMonth  = iMonth;
  lpUd->st.wDay    = iDay;
  lpUd->st.wHour   = iHour;
  lpUd->st.wMinute = iMinute;
  lpUd->st.wSecond = iSecond;

  TRACE("Rolled date: %d/%d/%d %d:%d:%d\n", lpUd->st.wDay, lpUd->st.wMonth,
        lpUd->st.wYear, lpUd->st.wHour, lpUd->st.wMinute, lpUd->st.wSecond);
  return S_OK;
}

/**********************************************************************
 *              DosDateTimeToVariantTime [OLEAUT32.14]
 *
 * Convert a Dos format date and time into variant VT_DATE format.
 *
 * PARAMS
 *  wDosDate [I] Dos format date
 *  wDosTime [I] Dos format time
 *  pDateOut [O] Destination for VT_DATE format
 *
 * RETURNS
 *  Success: TRUE. pDateOut contains the converted time.
 *  Failure: FALSE, if wDosDate or wDosTime are invalid (see notes).
 *
 * NOTES
 * - Dos format dates can only hold dates from 1-Jan-1980 to 31-Dec-2099.
 * - Dos format times are accurate to only 2 second precision.
 * - The format of a Dos Date is:
 *| Bits   Values  Meaning
 *| ----   ------  -------
 *| 0-4    1-31    Day of the week. 0 rolls back one day. A value greater than
 *|                the days in the month rolls forward the extra days.
 *| 5-8    1-12    Month of the year. 0 rolls back to December of the previous
 *|                year. 13-15 are invalid.
 *| 9-15   0-119   Year based from 1980 (Max 2099). 120-127 are invalid.
 * - The format of a Dos Time is:
 *| Bits   Values  Meaning
 *| ----   ------  -------
 *| 0-4    0-29    Seconds/2. 30 and 31 are invalid.
 *| 5-10   0-59    Minutes. 60-63 are invalid.
 *| 11-15  0-23    Hours (24 hour clock). 24-32 are invalid.
 */
INT WINAPI DosDateTimeToVariantTime(USHORT wDosDate, USHORT wDosTime,
                                    double *pDateOut)
{
  UDATE ud;

  TRACE("(0x%x(%d/%d/%d),0x%x(%d:%d:%d),%p)\n",
        wDosDate, DOS_YEAR(wDosDate), DOS_MONTH(wDosDate), DOS_DAY(wDosDate),
        wDosTime, DOS_HOUR(wDosTime), DOS_MINUTE(wDosTime), DOS_SECOND(wDosTime),
        pDateOut);

  ud.st.wYear = DOS_YEAR(wDosDate);
  ud.st.wMonth = DOS_MONTH(wDosDate);
  if (ud.st.wYear > 2099 || ud.st.wMonth > 12)
    return FALSE;
  ud.st.wDay = DOS_DAY(wDosDate);
  ud.st.wHour = DOS_HOUR(wDosTime);
  ud.st.wMinute = DOS_MINUTE(wDosTime);
  ud.st.wSecond = DOS_SECOND(wDosTime);
  ud.st.wDayOfWeek = ud.st.wMilliseconds = 0;
  if (ud.st.wHour > 23 || ud.st.wMinute > 59 || ud.st.wSecond > 59)
    return FALSE; /* Invalid values in Dos*/

  return VarDateFromUdate(&ud, 0, pDateOut) == S_OK;
}

/**********************************************************************
 *              VariantTimeToDosDateTime [OLEAUT32.13]
 *
 * Convert a variant format date into a Dos format date and time.
 *
 *  dateIn    [I] VT_DATE time format
 *  pwDosDate [O] Destination for Dos format date
 *  pwDosTime [O] Destination for Dos format time
 *
 * RETURNS
 *  Success: TRUE. pwDosDate and pwDosTime contains the converted values.
 *  Failure: FALSE, if dateIn cannot be represented in Dos format.
 *
 * NOTES
 *   See DosDateTimeToVariantTime() for Dos format details and bugs.
 */
INT WINAPI VariantTimeToDosDateTime(double dateIn, USHORT *pwDosDate, USHORT *pwDosTime)
{
  UDATE ud;

  TRACE("(%g,%p,%p)\n", dateIn, pwDosDate, pwDosTime);

  if (FAILED(VarUdateFromDate(dateIn, 0, &ud)))
    return FALSE;

  if (ud.st.wYear < 1980 || ud.st.wYear > 2099)
    return FALSE;

  *pwDosDate = DOS_DATE(ud.st.wDay, ud.st.wMonth, ud.st.wYear);
  *pwDosTime = DOS_TIME(ud.st.wHour, ud.st.wMinute, ud.st.wSecond);

  TRACE("Returning 0x%x(%d/%d/%d), 0x%x(%d:%d:%d)\n",
        *pwDosDate, DOS_YEAR(*pwDosDate), DOS_MONTH(*pwDosDate), DOS_DAY(*pwDosDate),
        *pwDosTime, DOS_HOUR(*pwDosTime), DOS_MINUTE(*pwDosTime), DOS_SECOND(*pwDosTime));
  return TRUE;
}

/***********************************************************************
 *              SystemTimeToVariantTime [OLEAUT32.184]
 *
 * Convert a System format date and time into variant VT_DATE format.
 *
 * PARAMS
 *  lpSt     [I] System format date and time
 *  pDateOut [O] Destination for VT_DATE format date
 *
 * RETURNS
 *  Success: TRUE. *pDateOut contains the converted value.
 *  Failure: FALSE, if lpSt cannot be represented in VT_DATE format.
 */
INT WINAPI SystemTimeToVariantTime(LPSYSTEMTIME lpSt, double *pDateOut)
{
  UDATE ud;

  TRACE("(%p->%d/%d/%d %d:%d:%d,%p)\n", lpSt, lpSt->wDay, lpSt->wMonth,
        lpSt->wYear, lpSt->wHour, lpSt->wMinute, lpSt->wSecond, pDateOut);

  if (lpSt->wMonth > 12)
    return FALSE;
  if (lpSt->wDay > 31)
    return FALSE;
  if ((short)lpSt->wYear < 0)
    return FALSE;

  ud.st = *lpSt;
  return VarDateFromUdate(&ud, 0, pDateOut) == S_OK;
}

/***********************************************************************
 *              VariantTimeToSystemTime [OLEAUT32.185]
 *
 * Convert a variant VT_DATE into a System format date and time.
 *
 * PARAMS
 *  datein [I] Variant VT_DATE format date
 *  lpSt   [O] Destination for System format date and time
 *
 * RETURNS
 *  Success: TRUE. *lpSt contains the converted value.
 *  Failure: FALSE, if dateIn is too large or small.
 */
INT WINAPI VariantTimeToSystemTime(double dateIn, LPSYSTEMTIME lpSt)
{
  UDATE ud;

  TRACE("(%g,%p)\n", dateIn, lpSt);

  if (FAILED(VarUdateFromDate(dateIn, 0, &ud)))
    return FALSE;

  *lpSt = ud.st;
  return TRUE;
}

/***********************************************************************
 *              VarDateFromUdateEx [OLEAUT32.319]
 *
 * Convert an unpacked format date and time to a variant VT_DATE.
 *
 * PARAMS
 *  pUdateIn [I] Unpacked format date and time to convert
 *  lcid     [I] Locale identifier for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pDateOut [O] Destination for variant VT_DATE.
 *
 * RETURNS
 *  Success: S_OK. *pDateOut contains the converted value.
 *  Failure: E_INVALIDARG, if pUdateIn cannot be represented in VT_DATE format.
 */
HRESULT WINAPI VarDateFromUdateEx(UDATE *pUdateIn, LCID lcid, ULONG dwFlags, DATE *pDateOut)
{
  UDATE ud;
  double dateVal = 0;

  TRACE("%p, %d/%d/%d, %d:%d:%d:%d, %#x, %d, %#lx, %#lx, %p.\n", pUdateIn,
        pUdateIn->st.wMonth, pUdateIn->st.wDay, pUdateIn->st.wYear,
        pUdateIn->st.wHour, pUdateIn->st.wMinute, pUdateIn->st.wSecond,
        pUdateIn->st.wMilliseconds, pUdateIn->st.wDayOfWeek,
        pUdateIn->wDayOfYear, lcid, dwFlags, pDateOut);

  if (lcid != MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT))
    FIXME("lcid possibly not handled, treating as en-us\n");
  if (dwFlags & ~(VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY))
    FIXME("unsupported flags: %lx\n", dwFlags);

  ud = *pUdateIn;

  if (dwFlags & VAR_VALIDDATE)
    WARN("Ignoring VAR_VALIDDATE\n");

  if (FAILED(VARIANT_RollUdate(&ud)))
    return E_INVALIDARG;

  /* Date */
  if (!(dwFlags & VAR_TIMEVALUEONLY))
    dateVal = VARIANT_DateFromJulian(VARIANT_JulianFromDMY(ud.st.wYear, ud.st.wMonth, ud.st.wDay));

  if ((dwFlags & VAR_TIMEVALUEONLY) || !(dwFlags & VAR_DATEVALUEONLY))
  {
    double dateSign = (dateVal < 0.0) ? -1.0 : 1.0;

    /* Time */
    dateVal += ud.st.wHour / 24.0 * dateSign;
    dateVal += ud.st.wMinute / 1440.0 * dateSign;
    dateVal += ud.st.wSecond / 86400.0 * dateSign;
  }

  TRACE("Returning %g\n", dateVal);
  *pDateOut = dateVal;
  return S_OK;
}

/***********************************************************************
 *              VarDateFromUdate [OLEAUT32.330]
 *
 * Convert an unpacked format date and time to a variant VT_DATE.
 *
 * PARAMS
 *  pUdateIn [I] Unpacked format date and time to convert
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pDateOut [O] Destination for variant VT_DATE.
 *
 * RETURNS
 *  Success: S_OK. *pDateOut contains the converted value.
 *  Failure: E_INVALIDARG, if pUdateIn cannot be represented in VT_DATE format.
 *
 * NOTES
 *  This function uses the United States English locale for the conversion. Use
 *  VarDateFromUdateEx() for alternate locales.
 */
HRESULT WINAPI VarDateFromUdate(UDATE *pUdateIn, ULONG dwFlags, DATE *pDateOut)
{
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  
  return VarDateFromUdateEx(pUdateIn, lcid, dwFlags, pDateOut);
}

/***********************************************************************
 *              VarUdateFromDate [OLEAUT32.331]
 *
 * Convert a variant VT_DATE into an unpacked format date and time.
 *
 * PARAMS
 *  datein    [I] Variant VT_DATE format date
 *  dwFlags   [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  lpUdate   [O] Destination for unpacked format date and time
 *
 * RETURNS
 *  Success: S_OK. *lpUdate contains the converted value.
 *  Failure: E_INVALIDARG, if dateIn is too large or small.
 */
HRESULT WINAPI VarUdateFromDate(DATE dateIn, ULONG dwFlags, UDATE *lpUdate)
{
  /* Cumulative totals of days per month */
  static const USHORT cumulativeDays[] =
  {
    0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
  };
  double datePart, timePart;
  int julianDays;

  TRACE("%g, %#lx, %p.\n", dateIn, dwFlags, lpUdate);

  if (dateIn <= (DATE_MIN - 1.0) || dateIn >= (DATE_MAX + 1.0))
    return E_INVALIDARG;

  datePart = dateIn < 0.0 ? ceil(dateIn) : floor(dateIn);
  /* Compensate for int truncation (always downwards) */
  timePart = fabs(dateIn - datePart) + 0.00000000001;
  if (timePart >= 1.0)
    timePart -= 0.00000000001;

  /* Date */
  julianDays = VARIANT_JulianFromDate(dateIn);
  VARIANT_DMYFromJulian(julianDays, &lpUdate->st.wYear, &lpUdate->st.wMonth,
                        &lpUdate->st.wDay);

  datePart = (datePart + 1.5) / 7.0;
  lpUdate->st.wDayOfWeek = (datePart - floor(datePart)) * 7;
  if (lpUdate->st.wDayOfWeek == 0)
    lpUdate->st.wDayOfWeek = 5;
  else if (lpUdate->st.wDayOfWeek == 1)
    lpUdate->st.wDayOfWeek = 6;
  else
    lpUdate->st.wDayOfWeek -= 2;

  if (lpUdate->st.wMonth > 2 && IsLeapYear(lpUdate->st.wYear))
    lpUdate->wDayOfYear = 1; /* After February, in a leap year */
  else
    lpUdate->wDayOfYear = 0;

  lpUdate->wDayOfYear += cumulativeDays[lpUdate->st.wMonth];
  lpUdate->wDayOfYear += lpUdate->st.wDay;

  /* Time */
  timePart *= 24.0;
  lpUdate->st.wHour = timePart;
  timePart -= lpUdate->st.wHour;
  timePart *= 60.0;
  lpUdate->st.wMinute = timePart;
  timePart -= lpUdate->st.wMinute;
  timePart *= 60.0;
  lpUdate->st.wSecond = timePart;
  timePart -= lpUdate->st.wSecond;
  lpUdate->st.wMilliseconds = 0;
  if (timePart > 0.5)
  {
    /* Round the milliseconds, adjusting the time/date forward if needed */
    if (lpUdate->st.wSecond < 59)
      lpUdate->st.wSecond++;
    else
    {
      lpUdate->st.wSecond = 0;
      if (lpUdate->st.wMinute < 59)
        lpUdate->st.wMinute++;
      else
      {
        lpUdate->st.wMinute = 0;
        if (lpUdate->st.wHour < 23)
          lpUdate->st.wHour++;
        else
        {
          lpUdate->st.wHour = 0;
          /* Roll over a whole day */
          if (++lpUdate->st.wDay > 28)
            VARIANT_RollUdate(lpUdate);
        }
      }
    }
  }
  return S_OK;
}

/* The localised characters that make up a valid number */
typedef struct tagVARIANT_NUMBER_CHARS
{
  WCHAR cNegativeSymbol;
  WCHAR cPositiveSymbol;
  WCHAR cDecimalPoint;
  WCHAR cDigitSeparator;
  DWORD sCurrencyLen;
  WCHAR sCurrency[8];
  WCHAR cCurrencyDecimalPoint;
  WCHAR cCurrencyDigitSeparator;
} VARIANT_NUMBER_CHARS;

#define GET_NUMBER_TEXT(fld,name) \
  buff[0] = 0; \
  if (!GetLocaleInfoW(lcid, lctype|fld, buff, ARRAY_SIZE(buff))) \
    WARN("buffer too small for " #fld "\n"); \
  else \
    if (buff[0]) lpChars->name = buff[0]; \
  TRACE("lcid 0x%lx, " #name "=%s\n", lcid, wine_dbgstr_wn(&lpChars->name, 1))

/* Get the valid number characters for an lcid */
static void VARIANT_GetLocalisedNumberChars(VARIANT_NUMBER_CHARS *lpChars, LCID lcid, DWORD dwFlags)
{
  static const VARIANT_NUMBER_CHARS defaultChars = { '-','+','.',0,1,{'$',0},0,',' };
  LCTYPE lctype = dwFlags & LOCALE_NOUSEROVERRIDE;
  WCHAR buff[4];

  memcpy(lpChars, &defaultChars, sizeof(defaultChars));
  GET_NUMBER_TEXT(LOCALE_SNEGATIVESIGN, cNegativeSymbol);
  GET_NUMBER_TEXT(LOCALE_SPOSITIVESIGN, cPositiveSymbol);
  GET_NUMBER_TEXT(LOCALE_SDECIMAL, cDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_STHOUSAND, cDigitSeparator);
  GET_NUMBER_TEXT(LOCALE_SMONDECIMALSEP, cCurrencyDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_SMONTHOUSANDSEP, cCurrencyDigitSeparator);

  if (!GetLocaleInfoW(lcid, lctype|LOCALE_SCURRENCY, lpChars->sCurrency, ARRAY_SIZE(lpChars->sCurrency)))
  {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      WARN("buffer too small for LOCALE_SCURRENCY\n");
    *lpChars->sCurrency = 0;
  }
  if (!*lpChars->sCurrency)
    wcscpy(lpChars->sCurrency, L"$");
  lpChars->sCurrencyLen = wcslen(lpChars->sCurrency);
  TRACE("lcid %#lx, sCurrency %lu %s\n", lcid, lpChars->sCurrencyLen, wine_dbgstr_w(lpChars->sCurrency));
}

/* Number Parsing States */
#define B_PROCESSING_EXPONENT 0x1
#define B_NEGATIVE_EXPONENT   0x2
#define B_EXPONENT_START      0x4
#define B_INEXACT_ZEROS       0x8
#define B_LEADING_ZERO        0x10
#define B_PROCESSING_HEX      0x20
#define B_PROCESSING_OCT      0x40

static inline BOOL is_digit(WCHAR c)
{
    return '0' <= c && c <= '9';
}

/**********************************************************************
 *              VarParseNumFromStr [OLEAUT32.46]
 *
 * Parse a string containing a number into a NUMPARSE structure.
 *
 * PARAMS
 *  lpszStr [I]   String to parse number from
 *  lcid    [I]   Locale Id for the conversion
 *  dwFlags [I]   0, or LOCALE_NOUSEROVERRIDE to use system default number chars
 *  pNumprs [I/O] Destination for parsed number
 *  rgbDig  [O]   Destination for digits read in
 *
 * RETURNS
 *  Success: S_OK. pNumprs and rgbDig contain the parsed representation of
 *           the number.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           DISP_E_TYPEMISMATCH, if the string is not a number or is formatted
 *           incorrectly.
 *           DISP_E_OVERFLOW, if rgbDig is too small to hold the number.
 *
 * NOTES
 *  pNumprs must have the following fields set:
 *   cDig: Set to the size of rgbDig.
 *   dwInFlags: Set to the allowable syntax of the number using NUMPRS_ flags
 *            from "oleauto.h".
 *
 * FIXME
 *  - I am unsure if this function should parse non-Arabic (e.g. Thai)
 *   numerals, so this has not been implemented.
 */
HRESULT WINAPI VarParseNumFromStr(const OLECHAR *lpszStr, LCID lcid, ULONG dwFlags,
                                  NUMPARSE *pNumprs, BYTE *rgbDig)
{
  VARIANT_NUMBER_CHARS chars;
  BYTE rgbTmp[1024];
  DWORD dwState = B_EXPONENT_START|B_INEXACT_ZEROS;
  int iMaxDigits = ARRAY_SIZE(rgbTmp);
  int cchUsed = 0;
  OLECHAR cDigitSeparator2;

  TRACE("%s, %#lx, %#lx, %p, %p.\n", debugstr_w(lpszStr), lcid, dwFlags, pNumprs, rgbDig);

  if (!pNumprs || !rgbDig)
    return E_INVALIDARG;

  if (pNumprs->cDig < iMaxDigits)
    iMaxDigits = pNumprs->cDig;

  pNumprs->cDig = 0;
  pNumprs->dwOutFlags = 0;
  pNumprs->cchUsed = 0;
  pNumprs->nBaseShift = 0;
  pNumprs->nPwr10 = 0;

  if (!lpszStr)
    return DISP_E_TYPEMISMATCH;

  VARIANT_GetLocalisedNumberChars(&chars, lcid, dwFlags);
  if (chars.cDigitSeparator == chars.cDecimalPoint)
    /* The decimal point completely masks the digit separator */
    chars.cDigitSeparator = 0;
  /* Setting the thousands separator to a non-breaking space implies regular
   * spaces are allowed too. But the converse is not true.
   */
  cDigitSeparator2 = chars.cDigitSeparator == 0xa0 ? ' ' : 0;

  /* First consume all the leading symbols and space from the string */
  while (1)
  {
    if (pNumprs->dwInFlags & NUMPRS_DECIMAL &&
        (*lpszStr == chars.cDecimalPoint ||
         *lpszStr == chars.cCurrencyDecimalPoint))
    {
      pNumprs->dwOutFlags |= NUMPRS_DECIMAL;
      if (*lpszStr == chars.cCurrencyDecimalPoint &&
        chars.cDecimalPoint != chars.cCurrencyDecimalPoint)
        pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed++;
      lpszStr++;

      /* If we have no digits so far, skip leading zeros */
      if (!pNumprs->cDig)
      {
        while (*lpszStr == '0')
        {
          dwState |= B_LEADING_ZERO;
          cchUsed++;
          lpszStr++;
          pNumprs->nPwr10--;
        }
      }
      break;
    }
    else if (pNumprs->dwInFlags & NUMPRS_LEADING_WHITE && iswspace(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_LEADING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (iswspace(*lpszStr));
    }
    else if (pNumprs->dwInFlags & NUMPRS_THOUSANDS &&
             ((chars.cDigitSeparator && *lpszStr == chars.cDigitSeparator) ||
              (cDigitSeparator2 && *lpszStr == cDigitSeparator2)))
    {
      return DISP_E_TYPEMISMATCH; /* Not allowed before the first digit */
    }
    else if ((pNumprs->dwInFlags & (NUMPRS_THOUSANDS|NUMPRS_CURRENCY)) == (NUMPRS_THOUSANDS|NUMPRS_CURRENCY) &&
             chars.cCurrencyDigitSeparator && *lpszStr == chars.cCurrencyDigitSeparator)
    {
      return DISP_E_TYPEMISMATCH; /* Not allowed before the first digit */
    }
    else if (pNumprs->dwInFlags & NUMPRS_LEADING_PLUS &&
             *lpszStr == chars.cPositiveSymbol &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_PLUS))
    {
      pNumprs->dwOutFlags |= NUMPRS_LEADING_PLUS;
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_LEADING_MINUS &&
             *lpszStr == chars.cNegativeSymbol &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_MINUS))
    {
      pNumprs->dwOutFlags |= (NUMPRS_LEADING_MINUS|NUMPRS_NEG);
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_CURRENCY &&
             !(pNumprs->dwOutFlags & NUMPRS_CURRENCY) &&
             wcsncmp(lpszStr, chars.sCurrency, chars.sCurrencyLen) == 0)
    {
      pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed += chars.sCurrencyLen;
      lpszStr += chars.sCurrencyLen;
    }
    else if (pNumprs->dwInFlags & NUMPRS_PARENS && *lpszStr == '(' &&
             !(pNumprs->dwOutFlags & NUMPRS_PARENS))
    {
      pNumprs->dwOutFlags |= NUMPRS_PARENS;
      cchUsed++;
      lpszStr++;
    }
    else
      break;
  }

  if (!(pNumprs->dwOutFlags & (NUMPRS_CURRENCY|NUMPRS_DECIMAL)))
  {
    if ((*lpszStr == '&' && (*(lpszStr+1) == 'H' || *(lpszStr+1) == 'h')) &&
        pNumprs->dwInFlags & NUMPRS_HEX_OCT)
    {
      dwState |= B_PROCESSING_HEX;
      pNumprs->dwOutFlags |= NUMPRS_HEX_OCT;
      cchUsed=cchUsed+2;
      lpszStr=lpszStr+2;
    }
    else if ((*lpszStr == '&' && (*(lpszStr+1) == 'O' || *(lpszStr+1) == 'o')) &&
             pNumprs->dwInFlags & NUMPRS_HEX_OCT)
    {
      dwState |= B_PROCESSING_OCT;
      pNumprs->dwOutFlags |= NUMPRS_HEX_OCT;
      cchUsed=cchUsed+2;
      lpszStr=lpszStr+2;
    }
  }

  /* Strip Leading zeros */
  while (*lpszStr == '0')
  {
    dwState |= B_LEADING_ZERO;
    cchUsed++;
    lpszStr++;
  }

  while (*lpszStr)
  {
    if (is_digit(*lpszStr))
    {
      if (dwState & B_PROCESSING_EXPONENT)
      {
        int exponentSize = 0;
        if (dwState & B_EXPONENT_START)
        {
          if (!is_digit(*lpszStr))
            break; /* No exponent digits - invalid */
          while (*lpszStr == '0')
          {
            /* Skip leading zero's in the exponent */
            cchUsed++;
            lpszStr++;
          }
        }

        while (is_digit(*lpszStr))
        {
          exponentSize *= 10;
          exponentSize += *lpszStr - '0';
          cchUsed++;
          lpszStr++;
        }
        if (dwState & B_NEGATIVE_EXPONENT)
          exponentSize = -exponentSize;
        /* Add the exponent into the powers of 10 */
        pNumprs->nPwr10 += exponentSize;
        dwState &= ~(B_PROCESSING_EXPONENT|B_EXPONENT_START);
        lpszStr--; /* back up to allow processing of next char */
      }
      else
      {
        if ((pNumprs->cDig >= iMaxDigits) && !(dwState & B_PROCESSING_HEX)
          && !(dwState & B_PROCESSING_OCT))
        {
          pNumprs->dwOutFlags |= NUMPRS_INEXACT;

          if (*lpszStr != '0')
            dwState &= ~B_INEXACT_ZEROS; /* Inexact number with non-trailing zeros */

          /* This digit can't be represented, but count it in nPwr10 */
          if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
            pNumprs->nPwr10--;
          else
            pNumprs->nPwr10++;
        }
        else
        {
          if ((dwState & B_PROCESSING_OCT) && ((*lpszStr == '8') || (*lpszStr == '9')))
            break;

          if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
            pNumprs->nPwr10--; /* Count decimal points in nPwr10 */

          rgbTmp[pNumprs->cDig] = *lpszStr - '0';
        }
        pNumprs->cDig++;
        cchUsed++;
      }
    }
    else if (pNumprs->dwInFlags & NUMPRS_THOUSANDS &&
             !(pNumprs->dwOutFlags & NUMPRS_HEX_OCT) &&
             ((chars.cDigitSeparator && *lpszStr == chars.cDigitSeparator) ||
              (cDigitSeparator2 && *lpszStr == cDigitSeparator2)))
    {
      pNumprs->dwOutFlags |= NUMPRS_THOUSANDS;
      cchUsed++;
    }
    else if ((pNumprs->dwInFlags & (NUMPRS_THOUSANDS|NUMPRS_CURRENCY)) == (NUMPRS_THOUSANDS|NUMPRS_CURRENCY) &&
             !(pNumprs->dwOutFlags & NUMPRS_HEX_OCT) &&
             chars.cCurrencyDigitSeparator && *lpszStr == chars.cCurrencyDigitSeparator)
    {
      pNumprs->dwOutFlags |= NUMPRS_THOUSANDS|NUMPRS_CURRENCY;
      cchUsed++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_DECIMAL &&
             (*lpszStr == chars.cDecimalPoint ||
              *lpszStr == chars.cCurrencyDecimalPoint) &&
             !(pNumprs->dwOutFlags & (NUMPRS_HEX_OCT|NUMPRS_DECIMAL|NUMPRS_EXPONENT)))
    {
      pNumprs->dwOutFlags |= NUMPRS_DECIMAL;
      if (*lpszStr == chars.cCurrencyDecimalPoint &&
        chars.cDecimalPoint != chars.cCurrencyDecimalPoint)
        pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed++;

      /* If we have no digits so far, skip leading zeros */
      if (!pNumprs->cDig)
      {
        while (lpszStr[1] == '0')
        {
          dwState |= B_LEADING_ZERO;
          cchUsed++;
          lpszStr++;
          pNumprs->nPwr10--;
        }
      }
    }
    else if (((*lpszStr >= 'a' && *lpszStr <= 'f') ||
             (*lpszStr >= 'A' && *lpszStr <= 'F')) &&
             dwState & B_PROCESSING_HEX)
    {
      if (pNumprs->cDig >= iMaxDigits)
      {
        return DISP_E_OVERFLOW;
      }
      else
      {
        if (*lpszStr >= 'a')
          rgbTmp[pNumprs->cDig] = *lpszStr - 'a' + 10;
        else
          rgbTmp[pNumprs->cDig] = *lpszStr - 'A' + 10;
      }
      pNumprs->cDig++;
      cchUsed++;
    }
    else if ((*lpszStr == 'e' || *lpszStr == 'E') &&
             pNumprs->dwInFlags & NUMPRS_EXPONENT &&
             !(pNumprs->dwOutFlags & (NUMPRS_HEX_OCT|NUMPRS_CURRENCY|NUMPRS_EXPONENT)))
    {
      dwState |= B_PROCESSING_EXPONENT;
      pNumprs->dwOutFlags |= NUMPRS_EXPONENT;
      cchUsed++;
    }
    else if (dwState & B_PROCESSING_EXPONENT && *lpszStr == chars.cPositiveSymbol)
    {
      cchUsed++; /* Ignore positive exponent */
    }
    else if (dwState & B_PROCESSING_EXPONENT && *lpszStr == chars.cNegativeSymbol)
    {
      dwState |= B_NEGATIVE_EXPONENT;
      cchUsed++;
    }
    else
      break; /* Stop at an unrecognised character */

    lpszStr++;
  }

  if (!pNumprs->cDig && dwState & B_LEADING_ZERO)
  {
    /* Ensure a 0 on its own gets stored */
    pNumprs->cDig = 1;
    rgbTmp[0] = 0;
  }

  if (pNumprs->dwOutFlags & NUMPRS_EXPONENT && dwState & B_PROCESSING_EXPONENT)
  {
    pNumprs->cchUsed = cchUsed;
    WARN("didn't completely parse exponent\n");
    return DISP_E_TYPEMISMATCH; /* Failed to completely parse the exponent */
  }

  if (pNumprs->dwOutFlags & NUMPRS_INEXACT)
  {
    if (dwState & B_INEXACT_ZEROS)
      pNumprs->dwOutFlags &= ~NUMPRS_INEXACT; /* All zeros doesn't set NUMPRS_INEXACT */
  } else if(pNumprs->dwInFlags & NUMPRS_HEX_OCT)
  {
    /* copy all of the digits into the output digit buffer */
    /* this is exactly what windows does although it also returns */
    /* cDig of X and writes X+Y where Y>=0 number of digits to rgbDig */
    memcpy(rgbDig, rgbTmp, pNumprs->cDig * sizeof(BYTE));

    if (dwState & B_PROCESSING_HEX) {
      /* hex numbers have always the same format */
      pNumprs->nPwr10=0;
      pNumprs->nBaseShift=4;
    } else {
      if (dwState & B_PROCESSING_OCT) {
        /* oct numbers have always the same format */
        pNumprs->nPwr10=0;
        pNumprs->nBaseShift=3;
      } else {
        while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
        {
          pNumprs->nPwr10++;
          pNumprs->cDig--;
        }
      }
    }
  } else
  {
    /* Remove trailing zeros from the last (whole number or decimal) part */
    while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
    {
      pNumprs->nPwr10++;
      pNumprs->cDig--;
    }
  }

  if (pNumprs->cDig <= iMaxDigits)
    pNumprs->dwOutFlags &= ~NUMPRS_INEXACT; /* Ignore stripped zeros for NUMPRS_INEXACT */
  else
    pNumprs->cDig = iMaxDigits; /* Only return iMaxDigits worth of digits */

  /* Copy the digits we processed into rgbDig */
  memcpy(rgbDig, rgbTmp, pNumprs->cDig * sizeof(BYTE));

  /* Consume any trailing symbols and space */
  while (1)
  {
    if ((chars.cDigitSeparator && *lpszStr == chars.cDigitSeparator) ||
        (cDigitSeparator2 && *lpszStr == cDigitSeparator2))
    {
      if (pNumprs->dwInFlags & NUMPRS_THOUSANDS &&
          !(pNumprs->dwOutFlags & NUMPRS_HEX_OCT))
      {
        pNumprs->dwOutFlags |= NUMPRS_THOUSANDS;
        cchUsed++;
        lpszStr++;
      }
      else
      {
        /* Not allowed, even with NUMPRS_TRAILING_WHITE */
        break;
      }
    }
    else if (*lpszStr == chars.cCurrencyDigitSeparator)
    {
      if ((pNumprs->dwInFlags & (NUMPRS_THOUSANDS|NUMPRS_CURRENCY)) == (NUMPRS_THOUSANDS|NUMPRS_CURRENCY) &&
          !(pNumprs->dwOutFlags & NUMPRS_HEX_OCT))
      {
        pNumprs->dwOutFlags |= NUMPRS_THOUSANDS|NUMPRS_CURRENCY;
        cchUsed++;
        lpszStr++;
      }
      else
      {
        /* Not allowed, even with NUMPRS_TRAILING_WHITE */
        break;
      }
    }
    else if ((pNumprs->dwInFlags & NUMPRS_TRAILING_WHITE) && iswspace(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_TRAILING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (iswspace(*lpszStr));
    }
    else if (pNumprs->dwInFlags & NUMPRS_TRAILING_PLUS &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_PLUS) &&
             *lpszStr == chars.cPositiveSymbol)
    {
      pNumprs->dwOutFlags |= NUMPRS_TRAILING_PLUS;
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_TRAILING_MINUS &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_MINUS) &&
             *lpszStr == chars.cNegativeSymbol)
    {
      pNumprs->dwOutFlags |= (NUMPRS_TRAILING_MINUS|NUMPRS_NEG);
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_PARENS && *lpszStr == ')' &&
             pNumprs->dwOutFlags & NUMPRS_PARENS)
    {
      cchUsed++;
      lpszStr++;
      pNumprs->dwOutFlags |= NUMPRS_NEG;
    }
    else if (pNumprs->dwInFlags & NUMPRS_CURRENCY &&
             !(pNumprs->dwOutFlags & NUMPRS_HEX_OCT) &&
             wcsncmp(lpszStr, chars.sCurrency, chars.sCurrencyLen) == 0)
    {
      pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed += chars.sCurrencyLen;
      lpszStr += chars.sCurrencyLen;
    }
    else
      break;
  }

  if (pNumprs->dwOutFlags & NUMPRS_PARENS && !(pNumprs->dwOutFlags & NUMPRS_NEG))
  {
    pNumprs->cchUsed = cchUsed;
    return DISP_E_TYPEMISMATCH; /* Opening parenthesis not matched */
  }

  if (pNumprs->dwInFlags & NUMPRS_USE_ALL && *lpszStr != '\0')
    return DISP_E_TYPEMISMATCH; /* Not all chars were consumed */

  if (!pNumprs->cDig)
    return DISP_E_TYPEMISMATCH; /* No Number found */

  pNumprs->cchUsed = cchUsed;
  return S_OK;
}

/* VTBIT flags indicating an integer value */
#define INTEGER_VTBITS (VTBIT_I1|VTBIT_UI1|VTBIT_I2|VTBIT_UI2|VTBIT_I4|VTBIT_UI4|VTBIT_I8|VTBIT_UI8)
/* VTBIT flags indicating a real number value */
#define REAL_VTBITS (VTBIT_R4|VTBIT_R8|VTBIT_CY)

/* Helper macros to check whether bit pattern fits in VARIANT (x is a ULONG64 ) */
#define FITS_AS_I1(x) ((x) >> 8 == 0)
#define FITS_AS_I2(x) ((x) >> 16 == 0)
#define FITS_AS_I4(x) ((x) >> 32 == 0)

/**********************************************************************
 *              VarNumFromParseNum [OLEAUT32.47]
 *
 * Convert a NUMPARSE structure into a numeric Variant type.
 *
 * PARAMS
 *  pNumprs  [I] Source for parsed number. cDig must be set to the size of rgbDig
 *  rgbDig   [I] Source for the numbers digits
 *  dwVtBits [I] VTBIT_ flags from "oleauto.h" indicating the acceptable dest types
 *  pVarDst  [O] Destination for the converted Variant value.
 *
 * RETURNS
 *  Success: S_OK. pVarDst contains the converted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           DISP_E_OVERFLOW, if the number is too big for the types set in dwVtBits.
 *
 * NOTES
 *  - The smallest favoured type present in dwVtBits that can represent the
 *    number in pNumprs without losing precision is used.
 *  - Signed types are preferred over unsigned types of the same size.
 *  - Preferred types in order are: integer, float, double, currency then decimal.
 *  - Rounding (dropping of decimal points) occurs without error. See VarI8FromR8()
 *    for details of the rounding method.
 *  - pVarDst is not cleared before the result is stored in it.
 *  - WinXP and Win2003 support VTBIT_I8, VTBIT_UI8 but that's buggy (by
 *    design?): If some other VTBIT's for integers are specified together
 *    with VTBIT_I8 and the number will fit only in a VT_I8 Windows will "cast"
 *    the number to the smallest requested integer truncating this way the
 *    number.  Wine doesn't implement this "feature" (yet?).
 */
HRESULT WINAPI VarNumFromParseNum(NUMPARSE *pNumprs, BYTE *rgbDig,
                                  ULONG dwVtBits, VARIANT *pVarDst)
{
  /* Scale factors and limits for double arithmetic */
  static const double dblMultipliers[11] = {
    1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0,
    1000000.0, 10000000.0, 100000000.0, 1000000000.0, 10000000000.0
  };
  static const double dblMinimums[11] = {
    R8_MIN, R8_MIN*10.0, R8_MIN*100.0, R8_MIN*1000.0, R8_MIN*10000.0,
    R8_MIN*100000.0, R8_MIN*1000000.0, R8_MIN*10000000.0,
    R8_MIN*100000000.0, R8_MIN*1000000000.0, R8_MIN*10000000000.0
  };
  static const double dblMaximums[11] = {
    R8_MAX, R8_MAX/10.0, R8_MAX/100.0, R8_MAX/1000.0, R8_MAX/10000.0,
    R8_MAX/100000.0, R8_MAX/1000000.0, R8_MAX/10000000.0,
    R8_MAX/100000000.0, R8_MAX/1000000000.0, R8_MAX/10000000000.0
  };

  int wholeNumberDigits, fractionalDigits, divisor10 = 0, multiplier10 = 0;

  TRACE("%p, %p, %lx, %p.\n", pNumprs, rgbDig, dwVtBits, pVarDst);

  if (pNumprs->nBaseShift)
  {
    /* nBaseShift indicates a hex or octal number */
    ULONG64 ul64 = 0;
    LONG64 l64;
    int i;

    /* Convert the hex or octal number string into a UI64 */
    for (i = 0; i < pNumprs->cDig; i++)
    {
      if (ul64 > ((UI8_MAX>>pNumprs->nBaseShift) - rgbDig[i]))
      {
        TRACE("Overflow multiplying digits\n");
        return DISP_E_OVERFLOW;
      }
      ul64 = (ul64<<pNumprs->nBaseShift) + rgbDig[i];
    }

    /* also make a negative representation */
    l64=-ul64;

    /* Try signed and unsigned types in size order */
    if (dwVtBits & VTBIT_I1 && FITS_AS_I1(ul64))
    {
      V_VT(pVarDst) = VT_I1;
      V_I1(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_UI1 && FITS_AS_I1(ul64))
    {
      V_VT(pVarDst) = VT_UI1;
      V_UI1(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_I2 && FITS_AS_I2(ul64))
    {
      V_VT(pVarDst) = VT_I2;
      V_I2(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_UI2 && FITS_AS_I2(ul64))
    {
      V_VT(pVarDst) = VT_UI2;
      V_UI2(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_I4 && FITS_AS_I4(ul64))
    {
      V_VT(pVarDst) = VT_I4;
      V_I4(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_UI4 && FITS_AS_I4(ul64))
    {
      V_VT(pVarDst) = VT_UI4;
      V_UI4(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_I8 && ((ul64 <= I8_MAX)||(l64>=I8_MIN)))
    {
      V_VT(pVarDst) = VT_I8;
      V_I8(pVarDst) = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_UI8)
    {
      V_VT(pVarDst) = VT_UI8;
      V_UI8(pVarDst) = ul64;
      return S_OK;
    }
    else if ((dwVtBits & VTBIT_DECIMAL) == VTBIT_DECIMAL)
    {
      V_VT(pVarDst) = VT_DECIMAL;
      V_DECIMAL(pVarDst).sign = DECIMAL_POS;
      V_DECIMAL(pVarDst).scale = 0;
      V_DECIMAL(pVarDst).Hi32 = 0;
      V_DECIMAL(pVarDst).Lo64 = ul64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_R4 && ((ul64 <= I4_MAX)||(l64 >= I4_MIN)))
    {
      V_VT(pVarDst) = VT_R4;
      if (ul64 <= I4_MAX)
          V_R4(pVarDst) = ul64;
      else
          V_R4(pVarDst) = l64;
      return S_OK;
    }
    else if (dwVtBits & VTBIT_R8 && ((ul64 <= I4_MAX)||(l64 >= I4_MIN)))
    {
      V_VT(pVarDst) = VT_R8;
      if (ul64 <= I4_MAX)
          V_R8(pVarDst) = ul64;
      else
          V_R8(pVarDst) = l64;
      return S_OK;
    }

    TRACE("Overflow: possible return types: %#lx, value: %s\n", dwVtBits, wine_dbgstr_longlong(ul64));
    return DISP_E_OVERFLOW;
  }

  /* Count the number of relevant fractional and whole digits stored,
   * And compute the divisor/multiplier to scale the number by.
   */
  if (pNumprs->nPwr10 < 0)
  {
    if (-pNumprs->nPwr10 >= pNumprs->cDig)
    {
      /* A real number < +/- 1.0 e.g. 0.1024 or 0.01024 */
      wholeNumberDigits = 0;
      fractionalDigits = pNumprs->cDig;
      divisor10 = -pNumprs->nPwr10;
    }
    else
    {
      /* An exactly represented real number e.g. 1.024 */
      wholeNumberDigits = pNumprs->cDig + pNumprs->nPwr10;
      fractionalDigits = pNumprs->cDig - wholeNumberDigits;
      divisor10 = pNumprs->cDig - wholeNumberDigits;
    }
  }
  else if (pNumprs->nPwr10 == 0)
  {
    /* An exactly represented whole number e.g. 1024 */
    wholeNumberDigits = pNumprs->cDig;
    fractionalDigits = 0;
  }
  else /* pNumprs->nPwr10 > 0 */
  {
    /* A whole number followed by nPwr10 0's e.g. 102400 */
    wholeNumberDigits = pNumprs->cDig;
    fractionalDigits = 0;
    multiplier10 = pNumprs->nPwr10;
  }

  TRACE("cDig %d; nPwr10 %d, whole %d, frac %d mult %d; div %d\n",
        pNumprs->cDig, pNumprs->nPwr10, wholeNumberDigits, fractionalDigits,
        multiplier10, divisor10);

  if (dwVtBits & (INTEGER_VTBITS|VTBIT_DECIMAL) &&
      (!fractionalDigits || !(dwVtBits & (REAL_VTBITS|VTBIT_DECIMAL))))
  {
    /* We have one or more integer output choices, and either:
     *  1) An integer input value, or
     *  2) A real number input value but no floating output choices.
     * Alternately, we have a DECIMAL output available and an integer input.
     *
     * So, place the integer value into pVarDst, using the smallest type
     * possible and preferring signed over unsigned types.
     */
    BOOL bOverflow = FALSE, bNegative;
    ULONG64 ul64 = 0;
    int i;

    /* Convert the integer part of the number into a UI8 */
    for (i = 0; i < wholeNumberDigits; i++)
    {
      if (ul64 > UI8_MAX / 10 || (ul64 == UI8_MAX / 10 && rgbDig[i] > UI8_MAX % 10))
      {
        TRACE("Overflow multiplying digits\n");
        bOverflow = TRUE;
        break;
      }
      ul64 = ul64 * 10 + rgbDig[i];
    }

    /* Account for the scale of the number */
    if (!bOverflow && multiplier10)
    {
      for (i = 0; i < multiplier10; i++)
      {
        if (ul64 > (UI8_MAX / 10))
        {
          TRACE("Overflow scaling number\n");
          bOverflow = TRUE;
          break;
        }
        ul64 = ul64 * 10;
      }
    }

    /* If we have any fractional digits, round the value.
     * Note we don't have to do this if divisor10 is < 1,
     * because this means the fractional part must be < 0.5
     */
    if (!bOverflow && fractionalDigits && divisor10 > 0)
    {
      const BYTE* fracDig = rgbDig + wholeNumberDigits;
      BOOL bAdjust = FALSE;

      TRACE("first decimal value is %d\n", *fracDig);

      if (*fracDig > 5)
        bAdjust = TRUE; /* > 0.5 */
      else if (*fracDig == 5)
      {
        for (i = 1; i < fractionalDigits; i++)
        {
          if (fracDig[i])
          {
            bAdjust = TRUE; /* > 0.5 */
            break;
          }
        }
        /* If exactly 0.5, round only odd values */
        if (i == fractionalDigits && (ul64 & 1))
          bAdjust = TRUE;
      }

      if (bAdjust)
      {
        if (ul64 == UI8_MAX)
        {
          TRACE("Overflow after rounding\n");
          bOverflow = TRUE;
        }
        ul64++;
      }
    }

    /* Zero is not a negative number */
    bNegative = pNumprs->dwOutFlags & NUMPRS_NEG && ul64;

    TRACE("Integer value is 0x%s, bNeg %d\n", wine_dbgstr_longlong(ul64), bNegative);

    /* For negative integers, try the signed types in size order */
    if (!bOverflow && bNegative)
    {
      if (dwVtBits & (VTBIT_I1|VTBIT_I2|VTBIT_I4|VTBIT_I8))
      {
        if (dwVtBits & VTBIT_I1 && ul64 <= -I1_MIN)
        {
          V_VT(pVarDst) = VT_I1;
          V_I1(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I2 && ul64 <= -I2_MIN)
        {
          V_VT(pVarDst) = VT_I2;
          V_I2(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I4 && ul64 <= -((LONGLONG)I4_MIN))
        {
          V_VT(pVarDst) = VT_I4;
          V_I4(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I8 && ul64 <= (ULONGLONG)I8_MAX + 1)
        {
          V_VT(pVarDst) = VT_I8;
          V_I8(pVarDst) = -ul64;
          return S_OK;
        }
        else if ((dwVtBits & (REAL_VTBITS|VTBIT_DECIMAL)) == VTBIT_DECIMAL)
        {
          /* Decimal is only output choice left - fast path */
          V_VT(pVarDst) = VT_DECIMAL;
          V_DECIMAL(pVarDst).sign = DECIMAL_NEG;
          V_DECIMAL(pVarDst).scale = 0;
          V_DECIMAL(pVarDst).Hi32 = 0;
          V_DECIMAL(pVarDst).Lo64 = -ul64;
          return S_OK;
        }
      }
    }
    else if (!bOverflow)
    {
      /* For positive integers, try signed then unsigned types in size order */
      if (dwVtBits & VTBIT_I1 && ul64 <= I1_MAX)
      {
        V_VT(pVarDst) = VT_I1;
        V_I1(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_UI1 && ul64 <= UI1_MAX)
      {
        V_VT(pVarDst) = VT_UI1;
        V_UI1(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_I2 && ul64 <= I2_MAX)
      {
        V_VT(pVarDst) = VT_I2;
        V_I2(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_UI2 && ul64 <= UI2_MAX)
      {
        V_VT(pVarDst) = VT_UI2;
        V_UI2(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_I4 && ul64 <= I4_MAX)
      {
        V_VT(pVarDst) = VT_I4;
        V_I4(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_UI4 && ul64 <= UI4_MAX)
      {
        V_VT(pVarDst) = VT_UI4;
        V_UI4(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_I8 && ul64 <= I8_MAX)
      {
        V_VT(pVarDst) = VT_I8;
        V_I8(pVarDst) = ul64;
        return S_OK;
      }
      else if (dwVtBits & VTBIT_UI8)
      {
        V_VT(pVarDst) = VT_UI8;
        V_UI8(pVarDst) = ul64;
        return S_OK;
      }
      else if ((dwVtBits & (REAL_VTBITS|VTBIT_DECIMAL)) == VTBIT_DECIMAL)
      {
        /* Decimal is only output choice left - fast path */
        V_VT(pVarDst) = VT_DECIMAL;
        V_DECIMAL(pVarDst).sign = DECIMAL_POS;
        V_DECIMAL(pVarDst).scale = 0;
        V_DECIMAL(pVarDst).Hi32 = 0;
        V_DECIMAL(pVarDst).Lo64 = ul64;
        return S_OK;
      }
    }
  }

  if (dwVtBits & REAL_VTBITS)
  {
    /* Try to put the number into a float or real */
    BOOL bOverflow = FALSE, bNegative = pNumprs->dwOutFlags & NUMPRS_NEG;
    double whole = 0.0;
    int i;

    /* Convert the number into a double */
    for (i = 0; i < pNumprs->cDig; i++)
      whole = whole * 10.0 + rgbDig[i];

    TRACE("Whole double value is %16.16g\n", whole);

    /* Account for the scale */
    while (multiplier10 > 10)
    {
      if (whole > dblMaximums[10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY);
        bOverflow = TRUE;
        break;
      }
      whole = whole * dblMultipliers[10];
      multiplier10 -= 10;
    }
    if (multiplier10 && !bOverflow)
    {
      if (whole > dblMaximums[multiplier10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY);
        bOverflow = TRUE;
      }
      else
        whole = whole * dblMultipliers[multiplier10];
    }

    if (!bOverflow)
        TRACE("Scaled double value is %16.16g\n", whole);

    while (divisor10 > 10 && !bOverflow)
    {
      if (whole < dblMinimums[10] && whole != 0)
      {
        whole = 0; /* ignore underflow */
        divisor10 = 0;
        break;
      }
      whole = whole / dblMultipliers[10];
      divisor10 -= 10;
    }
    if (divisor10 && !bOverflow)
    {
      if (whole < dblMinimums[divisor10] && whole != 0)
      {
        whole = 0; /* ignore underflow */
        divisor10 = 0;
      }
      else
        whole = whole / dblMultipliers[divisor10];
    }
    if (!bOverflow)
      TRACE("Final double value is %16.16g\n", whole);

    if (dwVtBits & VTBIT_R4 &&
        ((whole <= R4_MAX && whole >= R4_MIN) || whole == 0.0))
    {
      TRACE("Set R4 to final value\n");
      V_VT(pVarDst) = VT_R4; /* Fits into a float */
      V_R4(pVarDst) = pNumprs->dwOutFlags & NUMPRS_NEG ? -whole : whole;
      return S_OK;
    }

    if (dwVtBits & VTBIT_R8)
    {
      TRACE("Set R8 to final value\n");
      V_VT(pVarDst) = VT_R8; /* Fits into a double */
      V_R8(pVarDst) = pNumprs->dwOutFlags & NUMPRS_NEG ? -whole : whole;
      return S_OK;
    }

    if (dwVtBits & VTBIT_CY)
    {
      if (SUCCEEDED(VarCyFromR8(bNegative ? -whole : whole, &V_CY(pVarDst))))
      {
        V_VT(pVarDst) = VT_CY; /* Fits into a currency */
        TRACE("Set CY to final value\n");
        return S_OK;
      }
      TRACE("Value Overflows CY\n");
    }
  }

  if (dwVtBits & VTBIT_DECIMAL)
  {
    int i;
    ULONG carry;
    ULONG64 tmp;
    DECIMAL* pDec = &V_DECIMAL(pVarDst);

    DECIMAL_SETZERO(*pDec);
    pDec->Lo32 = 0;

    if (pNumprs->dwOutFlags & NUMPRS_NEG)
      pDec->sign = DECIMAL_NEG;
    else
      pDec->sign = DECIMAL_POS;

    /* Factor the significant digits */
    for (i = 0; i < pNumprs->cDig; i++)
    {
      tmp = (ULONG64)pDec->Lo32 * 10 + rgbDig[i];
      carry = (ULONG)(tmp >> 32);
      pDec->Lo32 = (ULONG)tmp;
      tmp = (ULONG64)pDec->Mid32 * 10 + carry;
      carry = (ULONG)(tmp >> 32);
      pDec->Mid32 = (ULONG)tmp;
      tmp = (ULONG64)pDec->Hi32 * 10 + carry;
      pDec->Hi32 = (ULONG)tmp;

      if (tmp >> 32)
      {
VarNumFromParseNum_DecOverflow:
        TRACE("Overflow\n");
        pDec->Lo32 = pDec->Mid32 = pDec->Hi32 = UI4_MAX;
        return DISP_E_OVERFLOW;
      }
    }

    /* Account for the scale of the number */
    while (multiplier10 > 0)
    {
      tmp = (ULONG64)pDec->Lo32 * 10;
      carry = (ULONG)(tmp >> 32);
      pDec->Lo32 = (ULONG)tmp;
      tmp = (ULONG64)pDec->Mid32 * 10 + carry;
      carry = (ULONG)(tmp >> 32);
      pDec->Mid32 = (ULONG)tmp;
      tmp = (ULONG64)pDec->Hi32 * 10 + carry;
      pDec->Hi32 = (ULONG)tmp;

      if (tmp >> 32)
        goto VarNumFromParseNum_DecOverflow;
      multiplier10--;
    }
    pDec->scale = divisor10;

    V_VT(pVarDst) = VT_DECIMAL;
    return S_OK;
  }
  return DISP_E_OVERFLOW; /* No more output choices */
}

/**********************************************************************
 *              VarCat [OLEAUT32.318]
 *
 * Concatenates one variant onto another.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarCat(LPVARIANT left, LPVARIANT right, LPVARIANT out)
{
    BSTR left_str = NULL, right_str = NULL;
    VARTYPE leftvt, rightvt;
    HRESULT hres;

    TRACE("%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), out);

    leftvt = V_VT(left);
    rightvt = V_VT(right);

    /* when both left and right are NULL the result is NULL */
    if (leftvt == VT_NULL && rightvt == VT_NULL)
    {
        V_VT(out) = VT_NULL;
        return S_OK;
    }

    /* There are many special case for errors and return types */
    if (leftvt == VT_VARIANT && (rightvt == VT_ERROR ||
        rightvt == VT_DATE || rightvt == VT_DECIMAL))
        hres = DISP_E_TYPEMISMATCH;
    else if ((leftvt == VT_I2 || leftvt == VT_I4 ||
        leftvt == VT_R4 || leftvt == VT_R8 ||
        leftvt == VT_CY || leftvt == VT_BOOL ||
        leftvt == VT_BSTR || leftvt == VT_I1 ||
        leftvt == VT_UI1 || leftvt == VT_UI2 ||
        leftvt == VT_UI4 || leftvt == VT_I8 ||
        leftvt == VT_UI8 || leftvt == VT_INT ||
        leftvt == VT_UINT || leftvt == VT_EMPTY ||
        leftvt == VT_NULL || leftvt == VT_DATE ||
        leftvt == VT_DECIMAL || leftvt == VT_DISPATCH)
        &&
        (rightvt == VT_I2 || rightvt == VT_I4 ||
        rightvt == VT_R4 || rightvt == VT_R8 ||
        rightvt == VT_CY || rightvt == VT_BOOL ||
        rightvt == VT_BSTR || rightvt == VT_I1 ||
        rightvt == VT_UI1 || rightvt == VT_UI2 ||
        rightvt == VT_UI4 || rightvt == VT_I8 ||
        rightvt == VT_UI8 || rightvt == VT_INT ||
        rightvt == VT_UINT || rightvt == VT_EMPTY ||
        rightvt == VT_NULL || rightvt == VT_DATE ||
        rightvt == VT_DECIMAL || rightvt == VT_DISPATCH))
        hres = S_OK;
    else if (rightvt == VT_ERROR && leftvt < VT_VOID)
        hres = DISP_E_TYPEMISMATCH;
    else if (leftvt == VT_ERROR && (rightvt == VT_DATE ||
        rightvt == VT_ERROR || rightvt == VT_DECIMAL))
        hres = DISP_E_TYPEMISMATCH;
    else if (rightvt == VT_DATE || rightvt == VT_ERROR ||
        rightvt == VT_DECIMAL)
        hres = DISP_E_BADVARTYPE;
    else if (leftvt == VT_ERROR || rightvt == VT_ERROR)
        hres = DISP_E_TYPEMISMATCH;
    else if (leftvt == VT_VARIANT)
        hres = DISP_E_TYPEMISMATCH;
    else if (rightvt == VT_VARIANT && (leftvt == VT_EMPTY ||
        leftvt == VT_NULL || leftvt ==  VT_I2 ||
        leftvt == VT_I4 || leftvt == VT_R4 ||
        leftvt == VT_R8 || leftvt == VT_CY ||
        leftvt == VT_DATE || leftvt == VT_BSTR ||
        leftvt == VT_BOOL ||  leftvt == VT_DECIMAL ||
        leftvt == VT_I1 || leftvt == VT_UI1 ||
        leftvt == VT_UI2 || leftvt == VT_UI4 ||
        leftvt == VT_I8 || leftvt == VT_UI8 ||
        leftvt == VT_INT || leftvt == VT_UINT))
        hres = DISP_E_TYPEMISMATCH;
    else
        hres = DISP_E_BADVARTYPE;

    /* if result type is not S_OK, then no need to go further */
    if (hres != S_OK)
    {
        V_VT(out) = VT_EMPTY;
        return hres;
    }

    if (leftvt == VT_BSTR)
        left_str = V_BSTR(left);
    else
    {
        VARIANT converted, *tmp = left;

        VariantInit(&converted);
        if(leftvt == VT_DISPATCH)
        {
            hres = VARIANT_FetchDispatchValue(left, &converted);
            if(FAILED(hres))
                goto failed;

            tmp = &converted;
        }

        hres = VariantChangeTypeEx(&converted, tmp, 0, VARIANT_ALPHABOOL|VARIANT_LOCALBOOL, VT_BSTR);
        if (SUCCEEDED(hres))
            left_str = V_BSTR(&converted);
        else if (hres != DISP_E_TYPEMISMATCH)
        {
            VariantClear(&converted);
            goto failed;
        }
    }

    if (rightvt == VT_BSTR)
        right_str = V_BSTR(right);
    else
    {
        VARIANT converted, *tmp = right;

        VariantInit(&converted);
        if(rightvt == VT_DISPATCH)
        {
            hres = VARIANT_FetchDispatchValue(right, &converted);
            if(FAILED(hres))
                goto failed;

            tmp = &converted;
        }

        hres = VariantChangeTypeEx(&converted, tmp, 0, VARIANT_ALPHABOOL|VARIANT_LOCALBOOL, VT_BSTR);
        if (SUCCEEDED(hres))
            right_str = V_BSTR(&converted);
        else if (hres != DISP_E_TYPEMISMATCH)
        {
            VariantClear(&converted);
            goto failed;
        }
    }


    V_VT(out) = VT_BSTR;
    hres = VarBstrCat(left_str, right_str, &V_BSTR(out));

failed:
    if(V_VT(left) != VT_BSTR)
        SysFreeString(left_str);
    if(V_VT(right) != VT_BSTR)
        SysFreeString(right_str);
    return hres;
}


/* Wrapper around VariantChangeTypeEx() which permits changing a
   variant with VT_RESERVED flag set. Needed by VarCmp. */
static HRESULT _VarChangeTypeExWrap (VARIANTARG* pvargDest,
                    VARIANTARG* pvargSrc, LCID lcid, USHORT wFlags, VARTYPE vt)
{
    VARIANTARG vtmpsrc = *pvargSrc;

    V_VT(&vtmpsrc) &= ~VT_RESERVED;
    return VariantChangeTypeEx(pvargDest,&vtmpsrc,lcid,wFlags,vt);
}

/**********************************************************************
 *              VarCmp [OLEAUT32.176]
 *
 * Compare two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  lcid    [I] LCID (locale identifier) for the comparison
 *  flags   [I] Flags to be used in the comparison:
 *              NORM_IGNORECASE, NORM_IGNORENONSPACE, NORM_IGNORESYMBOLS,
 *              NORM_IGNOREWIDTH, NORM_IGNOREKANATYPE, NORM_IGNOREKASHIDA
 *
 * RETURNS
 *  VARCMP_LT:   left variant is less than right variant.
 *  VARCMP_EQ:   input variants are equal.
 *  VARCMP_GT:   left variant is greater than right variant.
 *  VARCMP_NULL: either one of the input variants is NULL.
 *  Failure:     An HRESULT error code indicating the error.
 *
 * NOTES
 *  Native VarCmp up to and including WinXP doesn't like I1, UI2, VT_UI4,
 *  UI8 and UINT as input variants. INT is accepted only as left variant.
 *
 *  If both input variants are ERROR then VARCMP_EQ will be returned, else
 *  an ERROR variant will trigger an error.
 *
 *  Both input variants can have VT_RESERVED flag set which is ignored
 *  unless one and only one of the variants is a BSTR and the other one
 *  is not an EMPTY variant. All four VT_RESERVED combinations have a
 *  different meaning:
 *   - BSTR and other: BSTR is always greater than the other variant.
 *   - BSTR|VT_RESERVED and other: a string comparison is performed.
 *   - BSTR and other|VT_RESERVED: If the BSTR is a number a numeric
 *     comparison will take place else the BSTR is always greater.
 *   - BSTR|VT_RESERVED and other|VT_RESERVED: It seems that the other
 *     variant is ignored and the return value depends only on the sign
 *     of the BSTR if it is a number else the BSTR is always greater. A
 *     positive BSTR is greater, a negative one is smaller than the other
 *     variant.
 *
 * SEE
 *  VarBstrCmp for the lcid and flags usage.
 */
HRESULT WINAPI VarCmp(LPVARIANT left, LPVARIANT right, LCID lcid, DWORD flags)
{
    VARTYPE     lvt, rvt, vt;
    VARIANT     rv,lv;
    DWORD       xmask;
    HRESULT     rc;

    TRACE("%s, %s, %#lx, %#lx.\n", debugstr_variant(left), debugstr_variant(right), lcid, flags);

    lvt = V_VT(left) & VT_TYPEMASK;
    rvt = V_VT(right) & VT_TYPEMASK;
    xmask = (1 << lvt) | (1 << rvt);

    /* If we have any flag set except VT_RESERVED bail out.
       Same for the left input variant type > VT_INT and for the
       right input variant type > VT_I8. Yes, VT_INT is only supported
       as left variant. Go figure */
    if (((V_VT(left) | V_VT(right)) & ~VT_TYPEMASK & ~VT_RESERVED) ||
            lvt > VT_INT || rvt > VT_I8) {
        return DISP_E_BADVARTYPE;
    }

    /* Don't ask me why but native VarCmp cannot handle: VT_I1, VT_UI2, VT_UI4,
       VT_UINT and VT_UI8. Tested with DCOM98, Win2k, WinXP */
    if (rvt == VT_INT || xmask & (VTBIT_I1 | VTBIT_UI2 | VTBIT_UI4 | VTBIT_UI8 |
                VTBIT_DISPATCH | VTBIT_VARIANT | VTBIT_UNKNOWN | VTBIT_15))
        return DISP_E_TYPEMISMATCH;

    /* If both variants are VT_ERROR return VARCMP_EQ */
    if (xmask == VTBIT_ERROR)
        return VARCMP_EQ;
    else if (xmask & VTBIT_ERROR)
        return DISP_E_TYPEMISMATCH;

    if (xmask & VTBIT_NULL)
        return VARCMP_NULL;

    VariantInit(&lv);
    VariantInit(&rv);

    /* Two BSTRs, ignore VT_RESERVED */
    if (xmask == VTBIT_BSTR)
        return VarBstrCmp(V_BSTR(left), V_BSTR(right), lcid, flags);

    /* A BSTR and another variant; we have to take care of VT_RESERVED */
    if (xmask & VTBIT_BSTR) {
        VARIANT *bstrv, *nonbv;
        VARTYPE nonbvt;
        int swap = 0;

        /* Swap the variants so the BSTR is always on the left */
        if (lvt == VT_BSTR) {
            bstrv = left;
            nonbv = right;
            nonbvt = rvt;
        } else {
            swap = 1;
            bstrv = right;
            nonbv = left;
            nonbvt = lvt;
        }

        /* BSTR and EMPTY: ignore VT_RESERVED */
        if (nonbvt == VT_EMPTY)
            rc = (!V_BSTR(bstrv) || !*V_BSTR(bstrv)) ? VARCMP_EQ : VARCMP_GT;
        else {
            VARTYPE breserv = V_VT(bstrv) & ~VT_TYPEMASK;
            VARTYPE nreserv = V_VT(nonbv) & ~VT_TYPEMASK;

            if (!breserv && !nreserv) 
                /* No VT_RESERVED set ==> BSTR always greater */
                rc = VARCMP_GT;
            else if (breserv && !nreserv) {
                /* BSTR has VT_RESERVED set. Do a string comparison */
                rc = VariantChangeTypeEx(&rv,nonbv,lcid,0,VT_BSTR);
                if (FAILED(rc))
                    return rc;
                rc = VarBstrCmp(V_BSTR(bstrv), V_BSTR(&rv), lcid, flags);
                VariantClear(&rv);
            } else if (V_BSTR(bstrv) && *V_BSTR(bstrv)) {
            /* Non NULL nor empty BSTR */
                /* If the BSTR is not a number the BSTR is greater */
                rc = _VarChangeTypeExWrap(&lv,bstrv,lcid,0,VT_R8);
                if (FAILED(rc))
                    rc = VARCMP_GT;
                else if (breserv && nreserv)
                    /* FIXME: This is strange: with both VT_RESERVED set it
                       looks like the result depends only on the sign of
                       the BSTR number */
                    rc = (V_R8(&lv) >= 0) ? VARCMP_GT : VARCMP_LT;
                else
                    /* Numeric comparison, will be handled below.
                       VARCMP_NULL used only to break out. */
                    rc = VARCMP_NULL;
                VariantClear(&lv);
                VariantClear(&rv);
            } else
                /* Empty or NULL BSTR */
                rc = VARCMP_GT;
        }
        /* Fixup the return code if we swapped left and right */
        if (swap) {
            if (rc == VARCMP_GT)
                rc = VARCMP_LT;
            else if (rc == VARCMP_LT)
                rc = VARCMP_GT;
        }
        if (rc != VARCMP_NULL)
            return rc;
    }

    if (xmask & VTBIT_DECIMAL)
        vt = VT_DECIMAL;
    else if (xmask & VTBIT_BSTR)
        vt = VT_R8;
    else if (xmask & VTBIT_R4)
        vt = VT_R4;
    else if (xmask & (VTBIT_R8 | VTBIT_DATE))
        vt = VT_R8;
    else if (xmask & VTBIT_CY)
        vt = VT_CY;
    else
        /* default to I8 */
        vt = VT_I8;

    /* Coerce the variants */
    rc = _VarChangeTypeExWrap(&lv,left,lcid,0,vt);
    if (rc == DISP_E_OVERFLOW && vt != VT_R8) {
        /* Overflow, change to R8 */
        vt = VT_R8;
        rc = _VarChangeTypeExWrap(&lv,left,lcid,0,vt);
    }
    if (FAILED(rc))
        return rc;
    rc = _VarChangeTypeExWrap(&rv,right,lcid,0,vt);
    if (rc == DISP_E_OVERFLOW && vt != VT_R8) {
        /* Overflow, change to R8 */
        vt = VT_R8;
        rc = _VarChangeTypeExWrap(&lv,left,lcid,0,vt);
        if (FAILED(rc))
            return rc;
        rc = _VarChangeTypeExWrap(&rv,right,lcid,0,vt);
    }
    if (FAILED(rc))
        return rc;

#define _VARCMP(a,b) \
    (((a) == (b)) ? VARCMP_EQ : (((a) < (b)) ? VARCMP_LT : VARCMP_GT))

    switch (vt) {
        case VT_CY:
            return VarCyCmp(V_CY(&lv), V_CY(&rv));
        case VT_DECIMAL:
            return VarDecCmp(&V_DECIMAL(&lv), &V_DECIMAL(&rv));
        case VT_I8:
            return _VARCMP(V_I8(&lv), V_I8(&rv));
        case VT_R4:
            return _VARCMP(V_R4(&lv), V_R4(&rv));
        case VT_R8:
            return _VARCMP(V_R8(&lv), V_R8(&rv));
        default:
            /* We should never get here */
            return E_FAIL;
    }
#undef _VARCMP
}

/**********************************************************************
 *              VarAnd [OLEAUT32.142]
 *
 * Computes the logical AND of two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarAnd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres = S_OK;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT varLeft, varRight;
    VARIANT tempLeft, tempRight;

    VariantInit(&varLeft);
    VariantInit(&varRight);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hres)) goto VarAnd_Exit;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hres)) goto VarAnd_Exit;
        right = &tempRight;
    }

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hres = DISP_E_BADVARTYPE;
        goto VarAnd_Exit;
    }
    ExtraFlags = leftExtraFlags;

    /* Native VarAnd always returns an error when using extra
     * flags or if the variant combination is I8 and INT.
     */
    if ((leftvt == VT_I8 && rightvt == VT_INT) ||
        (leftvt == VT_INT && rightvt == VT_I8) ||
        ExtraFlags != 0)
    {
        hres = DISP_E_BADVARTYPE;
        goto VarAnd_Exit;
    }

    /* Determine return type */
    else if (leftvt == VT_I8 || rightvt == VT_I8)
        resvt = VT_I8;
    else if (leftvt == VT_I4 || rightvt == VT_I4 ||
        leftvt == VT_UINT || rightvt == VT_UINT ||
        leftvt == VT_INT || rightvt == VT_INT ||
        leftvt == VT_R4 || rightvt == VT_R4 ||
        leftvt == VT_R8 || rightvt == VT_R8 ||
        leftvt == VT_CY || rightvt == VT_CY ||
        leftvt == VT_DATE || rightvt == VT_DATE ||
        leftvt == VT_I1 || rightvt == VT_I1 ||
        leftvt == VT_UI2 || rightvt == VT_UI2 ||
        leftvt == VT_UI4 || rightvt == VT_UI4 ||
        leftvt == VT_UI8 || rightvt == VT_UI8 ||
        leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
        resvt = VT_I4;
    else if (leftvt == VT_UI1 || rightvt == VT_UI1 ||
        leftvt == VT_I2 || rightvt == VT_I2 ||
        leftvt == VT_EMPTY || rightvt == VT_EMPTY)
        if ((leftvt == VT_NULL && rightvt == VT_UI1) ||
            (leftvt == VT_UI1 && rightvt == VT_NULL) ||
            (leftvt == VT_UI1 && rightvt == VT_UI1))
            resvt = VT_UI1;
        else
            resvt = VT_I2;
    else if (leftvt == VT_BOOL || rightvt == VT_BOOL ||
        (leftvt == VT_BSTR && rightvt == VT_BSTR))
        resvt = VT_BOOL;
    else if (leftvt == VT_NULL || rightvt == VT_NULL ||
        leftvt == VT_BSTR || rightvt == VT_BSTR)
        resvt = VT_NULL;
    else
    {
        hres = DISP_E_BADVARTYPE;
        goto VarAnd_Exit;
    }

    if (leftvt == VT_NULL || rightvt == VT_NULL)
    {
        /*
         * Special cases for when left variant is VT_NULL
         * (VT_NULL & 0 = VT_NULL, VT_NULL & value = value)
         */
        if (leftvt == VT_NULL)
        {
            VARIANT_BOOL b;
            switch(rightvt)
            {
            case VT_I1:   if (V_I1(right)) resvt = VT_NULL; break;
            case VT_UI1:  if (V_UI1(right)) resvt = VT_NULL; break;
            case VT_I2:   if (V_I2(right)) resvt = VT_NULL; break;
            case VT_UI2:  if (V_UI2(right)) resvt = VT_NULL; break;
            case VT_I4:   if (V_I4(right)) resvt = VT_NULL; break;
            case VT_UI4:  if (V_UI4(right)) resvt = VT_NULL; break;
            case VT_I8:   if (V_I8(right)) resvt = VT_NULL; break;
            case VT_UI8:  if (V_UI8(right)) resvt = VT_NULL; break;
            case VT_INT:  if (V_INT(right)) resvt = VT_NULL; break;
            case VT_UINT: if (V_UINT(right)) resvt = VT_NULL; break;
            case VT_BOOL: if (V_BOOL(right)) resvt = VT_NULL; break;
            case VT_R4:   if (V_R4(right)) resvt = VT_NULL; break;
            case VT_R8:   if (V_R8(right)) resvt = VT_NULL; break;
            case VT_CY:
                if(V_CY(right).int64)
                    resvt = VT_NULL;
                break;
            case VT_DECIMAL:
                if (V_DECIMAL(right).Hi32 || V_DECIMAL(right).Lo64)
                    resvt = VT_NULL;
                break;
            case VT_BSTR:
                hres = VarBoolFromStr(V_BSTR(right),
                LOCALE_USER_DEFAULT, VAR_LOCALBOOL, &b);
                if (FAILED(hres))
                    return hres;
                else if (b)
                    V_VT(result) = VT_NULL;
                else
                {
                    V_VT(result) = VT_BOOL;
                    V_BOOL(result) = b;
                }
                goto VarAnd_Exit;
            }
        }
        V_VT(result) = resvt;
        goto VarAnd_Exit;
    }

    hres = VariantCopy(&varLeft, left);
    if (FAILED(hres)) goto VarAnd_Exit;

    hres = VariantCopy(&varRight, right);
    if (FAILED(hres)) goto VarAnd_Exit;

    if (resvt == VT_I4 && V_VT(&varLeft) == VT_UI4)
        V_VT(&varLeft) = VT_I4; /* Don't overflow */
    else
    {
        double d;

        if (V_VT(&varLeft) == VT_BSTR &&
            FAILED(VarR8FromStr(V_BSTR(&varLeft),
            LOCALE_USER_DEFAULT, 0, &d)))
            hres = VariantChangeType(&varLeft,&varLeft,
            VARIANT_LOCALBOOL, VT_BOOL);
        if (SUCCEEDED(hres) && V_VT(&varLeft) != resvt)
            hres = VariantChangeType(&varLeft,&varLeft,0,resvt);
        if (FAILED(hres)) goto VarAnd_Exit;
    }

    if (resvt == VT_I4 && V_VT(&varRight) == VT_UI4)
        V_VT(&varRight) = VT_I4; /* Don't overflow */
    else
    {
        double d;

        if (V_VT(&varRight) == VT_BSTR &&
            FAILED(VarR8FromStr(V_BSTR(&varRight),
            LOCALE_USER_DEFAULT, 0, &d)))
            hres = VariantChangeType(&varRight, &varRight,
                VARIANT_LOCALBOOL, VT_BOOL);
        if (SUCCEEDED(hres) && V_VT(&varRight) != resvt)
            hres = VariantChangeType(&varRight, &varRight, 0, resvt);
        if (FAILED(hres)) goto VarAnd_Exit;
    }

    V_VT(result) = resvt;
    switch(resvt)
    {
    case VT_I8:
        V_I8(result) = V_I8(&varLeft) & V_I8(&varRight);
        break;
    case VT_I4:
        V_I4(result) = V_I4(&varLeft) & V_I4(&varRight);
        break;
    case VT_I2:
        V_I2(result) = V_I2(&varLeft) & V_I2(&varRight);
        break;
    case VT_UI1:
        V_UI1(result) = V_UI1(&varLeft) & V_UI1(&varRight);
        break;
    case VT_BOOL:
        V_BOOL(result) = V_BOOL(&varLeft) & V_BOOL(&varRight);
        break;
    default:
        FIXME("Couldn't bitwise AND variant types %d,%d\n",
            leftvt,rightvt);
    }

VarAnd_Exit:
    VariantClear(&varLeft);
    VariantClear(&varRight);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);

    return hres;
}

/**********************************************************************
 *              VarAdd [OLEAUT32.141]
 *
 * Add two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Native VarAdd up to and including WinXP doesn't like I1, UI2, UI4,
 *  UI8, INT and UINT as input variants.
 *
 *  Native VarAdd doesn't check for NULL in/out pointers and crashes. We do the
 *  same here.
 *
 * FIXME
 *  Overflow checking for R8 (double) overflow. Return DISP_E_OVERFLOW in that
 *  case.
 */
HRESULT WINAPI VarAdd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres;
    VARTYPE lvt, rvt, resvt, tvt;
    VARIANT lv, rv, tv;
    VARIANT tempLeft, tempRight;
    double r8res;

    /* Variant priority for coercion. Sorted from lowest to highest.
       VT_ERROR shows an invalid input variant type. */
    enum coerceprio { vt_EMPTY, vt_UI1, vt_I2, vt_I4, vt_I8, vt_BSTR,vt_R4,
                      vt_R8, vt_CY, vt_DATE, vt_DECIMAL, vt_DISPATCH, vt_NULL,
                      vt_ERROR };
    /* Mapping from priority to variant type. Keep in sync with coerceprio! */
    static const VARTYPE prio2vt[] = { VT_EMPTY, VT_UI1, VT_I2, VT_I4, VT_I8, VT_BSTR, VT_R4,
                          VT_R8, VT_CY, VT_DATE, VT_DECIMAL, VT_DISPATCH,
                          VT_NULL, VT_ERROR };

    /* Mapping for coercion from input variant to priority of result variant. */
    static const VARTYPE coerce[] = {
        /* VT_EMPTY, VT_NULL, VT_I2, VT_I4, VT_R4 */
        vt_EMPTY, vt_NULL, vt_I2, vt_I4, vt_R4,
        /* VT_R8, VT_CY, VT_DATE, VT_BSTR, VT_DISPATCH */
        vt_R8, vt_CY, vt_DATE, vt_BSTR, vt_DISPATCH,
        /* VT_ERROR, VT_BOOL, VT_VARIANT, VT_UNKNOWN, VT_DECIMAL */
        vt_ERROR, vt_I2, vt_ERROR, vt_ERROR, vt_DECIMAL,
        /* 15, VT_I1, VT_UI1, VT_UI2, VT_UI4 VT_I8 */
        vt_ERROR, vt_ERROR, vt_UI1, vt_ERROR, vt_ERROR, vt_I8
    };

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    VariantInit(&lv);
    VariantInit(&rv);
    VariantInit(&tv);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) != VT_NULL && (V_VT(right) & VT_TYPEMASK) != VT_NULL)
    {
        if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
        {
            hres = VARIANT_FetchDispatchValue(left, &tempLeft);
            if (FAILED(hres)) goto end;
            left = &tempLeft;
        }
        if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
        {
            hres = VARIANT_FetchDispatchValue(right, &tempRight);
            if (FAILED(hres)) goto end;
            right = &tempRight;
        }
    }

    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;

    /* If we have any flag set (VT_ARRAY, VT_VECTOR, etc.) bail out.
       Same for any input variant type > VT_I8 */
    if (V_VT(left) & ~VT_TYPEMASK || V_VT(right) & ~VT_TYPEMASK ||
        lvt > VT_I8 || rvt > VT_I8) {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* Determine the variant type to coerce to. */
    if (coerce[lvt] > coerce[rvt]) {
        resvt = prio2vt[coerce[lvt]];
        tvt = prio2vt[coerce[rvt]];
    } else {
        resvt = prio2vt[coerce[rvt]];
        tvt = prio2vt[coerce[lvt]];
    }

    /* Special cases where the result variant type is defined by both
       input variants and not only that with the highest priority */
    if (resvt == VT_BSTR) {
        if (tvt == VT_EMPTY || tvt == VT_BSTR)
            resvt = VT_BSTR;
        else
            resvt = VT_R8;
    }
    if (resvt == VT_R4 && (tvt == VT_BSTR || tvt == VT_I8 || tvt == VT_I4))
        resvt = VT_R8;

    /* For overflow detection use the biggest compatible type for the
       addition */
    switch (resvt) {
        case VT_ERROR:
            hres = DISP_E_BADVARTYPE;
            goto end;
        case VT_NULL:
            hres = S_OK;
            V_VT(result) = VT_NULL;
            goto end;
        case VT_DISPATCH:
            FIXME("cannot handle variant type VT_DISPATCH\n");
            hres = DISP_E_TYPEMISMATCH;
            goto end;
        case VT_EMPTY:
            resvt = VT_I2;
            /* Fall through */
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
            tvt = VT_I8;
            break;
        case VT_DATE:
        case VT_R4:
            tvt = VT_R8;
            break;
        default:
            tvt = resvt;
    }

    /* Now coerce the variants */
    hres = VariantChangeType(&lv, left, 0, tvt);
    if (FAILED(hres))
        goto end;
    hres = VariantChangeType(&rv, right, 0, tvt);
    if (FAILED(hres))
        goto end;

    /* Do the math */
    hres = S_OK;
    V_VT(result) = resvt;
    switch (tvt) {
        case VT_DECIMAL:
            hres = VarDecAdd(&V_DECIMAL(&lv), &V_DECIMAL(&rv),
                             &V_DECIMAL(result));
            goto end;
        case VT_CY:
            hres = VarCyAdd(V_CY(&lv), V_CY(&rv), &V_CY(result));
            goto end;
        case VT_BSTR:
            /* We do not add those, we concatenate them. */
            hres = VarBstrCat(V_BSTR(&lv), V_BSTR(&rv), &V_BSTR(result));
            goto end;
        case VT_I8:
            /* Overflow detection */
            r8res = (double)V_I8(&lv) + (double)V_I8(&rv);
            if (r8res > (double)I8_MAX || r8res < (double)I8_MIN) {
                V_VT(result) = VT_R8;
                V_R8(result) = r8res;
                goto end;
            } else {
                V_VT(&tv) = tvt;
                V_I8(&tv) = V_I8(&lv) + V_I8(&rv);
            }
            break;
        case VT_R8:
            V_VT(&tv) = tvt;
            /* FIXME: overflow detection */
            V_R8(&tv) = V_R8(&lv) + V_R8(&rv);
            break;
        default:
            ERR("We shouldn't get here! tvt = %d!\n", tvt);
            break;
    }
    if (resvt != tvt) {
        if ((hres = VariantChangeType(result, &tv, 0, resvt)) != S_OK) {
            /* Overflow! Change to the vartype with the next higher priority.
               With one exception: I4 ==> R8 even if it would fit in I8 */
            if (resvt == VT_I4)
                resvt = VT_R8;
            else
                resvt = prio2vt[coerce[resvt] + 1];
            hres = VariantChangeType(result, &tv, 0, resvt);
        }
    } else
        hres = VariantCopy(result, &tv);

end:
    if (hres != S_OK) {
        V_VT(result) = VT_EMPTY;
        V_I4(result) = 0;       /* No V_EMPTY */
    }
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    TRACE("returning %#lx, %s\n", hres, debugstr_variant(result));
    return hres;
}

/**********************************************************************
 *              VarMul [OLEAUT32.156]
 *
 * Multiply two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Native VarMul up to and including WinXP doesn't like I1, UI2, UI4,
 *  UI8, INT and UINT as input variants. But it can multiply apples with oranges.
 *
 *  Native VarMul doesn't check for NULL in/out pointers and crashes. We do the
 *  same here.
 *
 * FIXME
 *  Overflow checking for R8 (double) overflow. Return DISP_E_OVERFLOW in that
 *  case.
 */
HRESULT WINAPI VarMul(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres;
    VARTYPE lvt, rvt, resvt, tvt;
    VARIANT lv, rv, tv;
    VARIANT tempLeft, tempRight;
    double r8res;

    /* Variant priority for coercion. Sorted from lowest to highest.
       VT_ERROR shows an invalid input variant type. */
    enum coerceprio { vt_UI1 = 0, vt_I2, vt_I4, vt_I8, vt_CY, vt_R4, vt_R8,
                      vt_DECIMAL, vt_NULL, vt_ERROR };
    /* Mapping from priority to variant type. Keep in sync with coerceprio! */
    static const VARTYPE prio2vt[] = { VT_UI1, VT_I2, VT_I4, VT_I8, VT_CY, VT_R4, VT_R8,
                          VT_DECIMAL, VT_NULL, VT_ERROR };

    /* Mapping for coercion from input variant to priority of result variant. */
    static const VARTYPE coerce[] = {
        /* VT_EMPTY, VT_NULL, VT_I2, VT_I4, VT_R4 */
        vt_UI1, vt_NULL, vt_I2, vt_I4, vt_R4,
        /* VT_R8, VT_CY, VT_DATE, VT_BSTR, VT_DISPATCH */
        vt_R8, vt_CY, vt_R8, vt_R8, vt_ERROR,
        /* VT_ERROR, VT_BOOL, VT_VARIANT, VT_UNKNOWN, VT_DECIMAL */
        vt_ERROR, vt_I2, vt_ERROR, vt_ERROR, vt_DECIMAL,
        /* 15, VT_I1, VT_UI1, VT_UI2, VT_UI4 VT_I8 */
        vt_ERROR, vt_ERROR, vt_UI1, vt_ERROR, vt_ERROR, vt_I8
    };

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    VariantInit(&lv);
    VariantInit(&rv);
    VariantInit(&tv);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hres)) goto end;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hres)) goto end;
        right = &tempRight;
    }

    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;

    /* If we have any flag set (VT_ARRAY, VT_VECTOR, etc.) bail out.
       Same for any input variant type > VT_I8 */
    if (V_VT(left) & ~VT_TYPEMASK || V_VT(right) & ~VT_TYPEMASK ||
        lvt > VT_I8 || rvt > VT_I8) {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* Determine the variant type to coerce to. */
    if (coerce[lvt] > coerce[rvt]) {
        resvt = prio2vt[coerce[lvt]];
        tvt = prio2vt[coerce[rvt]];
    } else {
        resvt = prio2vt[coerce[rvt]];
        tvt = prio2vt[coerce[lvt]];
    }

    /* Special cases where the result variant type is defined by both
       input variants and not only that with the highest priority */
    if (resvt == VT_R4 && (tvt == VT_CY || tvt == VT_I8 || tvt == VT_I4))
        resvt = VT_R8;
    if (lvt == VT_EMPTY && rvt == VT_EMPTY)
        resvt = VT_I2;

    /* For overflow detection use the biggest compatible type for the
       multiplication */
    switch (resvt) {
        case VT_ERROR:
            hres = DISP_E_BADVARTYPE;
            goto end;
        case VT_NULL:
            hres = S_OK;
            V_VT(result) = VT_NULL;
            goto end;
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
            tvt = VT_I8;
            break;
        case VT_R4:
            tvt = VT_R8;
            break;
        default:
            tvt = resvt;
    }

    /* Now coerce the variants */
    hres = VariantChangeType(&lv, left, 0, tvt);
    if (FAILED(hres))
        goto end;
    hres = VariantChangeType(&rv, right, 0, tvt);
    if (FAILED(hres))
        goto end;

    /* Do the math */
    hres = S_OK;
    V_VT(&tv) = tvt;
    V_VT(result) = resvt;
    switch (tvt) {
        case VT_DECIMAL:
            hres = VarDecMul(&V_DECIMAL(&lv), &V_DECIMAL(&rv),
                             &V_DECIMAL(result));
            goto end;
        case VT_CY:
            hres = VarCyMul(V_CY(&lv), V_CY(&rv), &V_CY(result));
            goto end;
        case VT_I8:
            /* Overflow detection */
            r8res = (double)V_I8(&lv) * (double)V_I8(&rv);
            if (r8res > (double)I8_MAX || r8res < (double)I8_MIN) {
                V_VT(result) = VT_R8;
                V_R8(result) = r8res;
                goto end;
            } else
                V_I8(&tv) = V_I8(&lv) * V_I8(&rv);
            break;
        case VT_R8:
            /* FIXME: overflow detection */
            V_R8(&tv) = V_R8(&lv) * V_R8(&rv);
            break;
        default:
            ERR("We shouldn't get here! tvt = %d!\n", tvt);
            break;
    }
    if (resvt != tvt) {
        while ((hres = VariantChangeType(result, &tv, 0, resvt)) != S_OK) {
            /* Overflow! Change to the vartype with the next higher priority.
               With one exception: I4 ==> R8 even if it would fit in I8 */
            if (resvt == VT_I4)
                resvt = VT_R8;
            else
                resvt = prio2vt[coerce[resvt] + 1];
        }
    } else
        hres = VariantCopy(result, &tv);

end:
    if (hres != S_OK) {
        V_VT(result) = VT_EMPTY;
        V_I4(result) = 0;       /* No V_EMPTY */
    }
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    TRACE("returning %#lx, %s\n", hres, debugstr_variant(result));
    return hres;
}

/**********************************************************************
 *              VarDiv [OLEAUT32.143]
 *
 * Divides one variant with another.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarDiv(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres = S_OK;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT lv,rv;
    VARIANT tempLeft, tempRight;

    VariantInit(&tempLeft);
    VariantInit(&tempRight);
    VariantInit(&lv);
    VariantInit(&rv);

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hres)) goto end;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hres)) goto end;
        right = &tempRight;
    }

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    ExtraFlags = leftExtraFlags;

    /* Native VarDiv always returns an error when using extra flags */
    if (ExtraFlags != 0)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* Determine return type */
    if (rightvt != VT_EMPTY)
    {
        if (leftvt == VT_NULL || rightvt == VT_NULL)
        {
            V_VT(result) = VT_NULL;
            hres = S_OK;
            goto end;
        }
        else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
            resvt = VT_DECIMAL;
        else if (leftvt == VT_I8 || rightvt == VT_I8 ||
            leftvt == VT_CY || rightvt == VT_CY ||
            leftvt == VT_DATE || rightvt == VT_DATE ||
            leftvt == VT_I4 || rightvt == VT_I4 ||
            leftvt == VT_BSTR || rightvt == VT_BSTR ||
            leftvt == VT_I2 || rightvt == VT_I2 ||
            leftvt == VT_BOOL || rightvt == VT_BOOL ||
            leftvt == VT_R8 || rightvt == VT_R8 ||
            leftvt == VT_UI1 || rightvt == VT_UI1)
        {
            if ((leftvt == VT_UI1 && rightvt == VT_R4) ||
                (leftvt == VT_R4 && rightvt == VT_UI1))
                resvt = VT_R4;
            else if ((leftvt == VT_R4 && (rightvt == VT_BOOL ||
                rightvt == VT_I2)) || (rightvt == VT_R4 &&
                (leftvt == VT_BOOL || leftvt == VT_I2)))
                resvt = VT_R4;
            else
                resvt = VT_R8;
        }
        else if (leftvt == VT_R4 || rightvt == VT_R4)
            resvt = VT_R4;
    }
    else if (leftvt == VT_NULL)
    {
        V_VT(result) = VT_NULL;
        hres = S_OK;
        goto end;
    }
    else
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* coerce to the result type */
    hres = VariantChangeType(&lv, left, 0, resvt);
    if (hres != S_OK) goto end;

    hres = VariantChangeType(&rv, right, 0, resvt);
    if (hres != S_OK) goto end;

    /* do the math */
    V_VT(result) = resvt;
    switch (resvt)
    {
    case VT_R4:
    if (V_R4(&lv) == 0.0 && V_R4(&rv) == 0.0)
    {
        hres = DISP_E_OVERFLOW;
        V_VT(result) = VT_EMPTY;
    }
    else if (V_R4(&rv) == 0.0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_R4(result) = V_R4(&lv) / V_R4(&rv);
    break;
    case VT_R8:
    if (V_R8(&lv) == 0.0 && V_R8(&rv) == 0.0)
    {
        hres = DISP_E_OVERFLOW;
        V_VT(result) = VT_EMPTY;
    }
    else if (V_R8(&rv) == 0.0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_R8(result) = V_R8(&lv) / V_R8(&rv);
    break;
    case VT_DECIMAL:
    hres = VarDecDiv(&(V_DECIMAL(&lv)), &(V_DECIMAL(&rv)), &(V_DECIMAL(result)));
    break;
    }

end:
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    TRACE("returning %#lx, %s\n", hres, debugstr_variant(result));
    return hres;
}

/**********************************************************************
 *              VarSub [OLEAUT32.159]
 *
 * Subtract two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarSub(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres = S_OK;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT lv,rv;
    VARIANT tempLeft, tempRight;

    VariantInit(&lv);
    VariantInit(&rv);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH &&
        (V_VT(left)&(~VT_TYPEMASK)) == 0 &&
        (V_VT(right) & VT_TYPEMASK) != VT_NULL)
    {
        if (NULL == V_DISPATCH(left)) {
            if ((V_VT(right) & VT_TYPEMASK) >= VT_INT_PTR)
                hres = DISP_E_BADVARTYPE;
            else if ((V_VT(right) & VT_TYPEMASK) >= VT_UI8 &&
                (V_VT(right) & VT_TYPEMASK) < VT_RECORD)
                hres = DISP_E_BADVARTYPE;
            else switch (V_VT(right) & VT_TYPEMASK)
            {
            case VT_VARIANT:
            case VT_UNKNOWN:
            case 15:
            case VT_I1:
            case VT_UI2:
            case VT_UI4:
                hres = DISP_E_BADVARTYPE;
            }
            if (FAILED(hres)) goto end;
        }
        hres = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hres)) goto end;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH &&
        (V_VT(right)&(~VT_TYPEMASK)) == 0 &&
        (V_VT(left) & VT_TYPEMASK) != VT_NULL)
    {
        if (NULL == V_DISPATCH(right))
        {
            if ((V_VT(left) & VT_TYPEMASK) >= VT_INT_PTR)
                hres = DISP_E_BADVARTYPE;
            else if ((V_VT(left) & VT_TYPEMASK) >= VT_UI8 &&
                (V_VT(left) & VT_TYPEMASK) < VT_RECORD)
                hres = DISP_E_BADVARTYPE;
            else switch (V_VT(left) & VT_TYPEMASK)
            {
            case VT_VARIANT:
            case VT_UNKNOWN:
            case 15:
            case VT_I1:
            case VT_UI2:
            case VT_UI4:
                hres = DISP_E_BADVARTYPE;
            }
            if (FAILED(hres)) goto end;
        }
        hres = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hres)) goto end;
        right = &tempRight;
    }

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    ExtraFlags = leftExtraFlags;

    /* determine return type and return code */
    /* All extra flags produce errors */
    if (ExtraFlags == (VT_VECTOR|VT_BYREF|VT_RESERVED) ||
        ExtraFlags == (VT_VECTOR|VT_RESERVED) ||
        ExtraFlags == (VT_VECTOR|VT_BYREF) ||
        ExtraFlags == (VT_BYREF|VT_RESERVED) ||
        ExtraFlags == VT_VECTOR ||
        ExtraFlags == VT_BYREF ||
        ExtraFlags == VT_RESERVED)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    else if (ExtraFlags >= VT_ARRAY)
    {
        hres = DISP_E_TYPEMISMATCH;
        goto end;
    }
    /* Native VarSub cannot handle: VT_I1, VT_UI2, VT_UI4,
       VT_INT, VT_UINT and VT_UI8. Tested with WinXP */
    else if (leftvt == VT_CLSID || rightvt == VT_CLSID ||
        leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
        leftvt == VT_I1 || rightvt == VT_I1 ||
        leftvt == VT_UI2 || rightvt == VT_UI2 ||
        leftvt == VT_UI4 || rightvt == VT_UI4 ||
        leftvt == VT_UI8 || rightvt == VT_UI8 ||
        leftvt == VT_INT || rightvt == VT_INT ||
        leftvt == VT_UINT || rightvt == VT_UINT ||
        leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN ||
        leftvt == VT_RECORD || rightvt == VT_RECORD)
    {
        if (leftvt == VT_RECORD && rightvt == VT_I8)
            hres = DISP_E_TYPEMISMATCH;
        else if (leftvt < VT_UI1 && rightvt == VT_RECORD)
            hres = DISP_E_TYPEMISMATCH;
        else if (leftvt >= VT_UI1 && rightvt == VT_RECORD)
            hres = DISP_E_TYPEMISMATCH;
        else if (leftvt == VT_RECORD && rightvt <= VT_UI1)
            hres = DISP_E_TYPEMISMATCH;
        else if (leftvt == VT_RECORD && rightvt > VT_UI1)
            hres = DISP_E_BADVARTYPE;
        else
            hres = DISP_E_BADVARTYPE;
        goto end;
    }
    /*  The following flags/types are invalid for left variant */
    else if (!((leftvt <= VT_LPWSTR || leftvt == VT_RECORD ||
        leftvt == VT_CLSID) && leftvt != (VARTYPE)15 /* undefined vt */ &&
        (leftvt < VT_VOID || leftvt > VT_LPWSTR)))
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    /*  The following flags/types are invalid for right variant */
    else if (!((rightvt <= VT_LPWSTR || rightvt == VT_RECORD ||
        rightvt == VT_CLSID) && rightvt != (VARTYPE)15 /* undefined vt */ &&
        (rightvt < VT_VOID || rightvt > VT_LPWSTR)))
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    else if ((leftvt == VT_NULL && rightvt == VT_DISPATCH) ||
        (leftvt == VT_DISPATCH && rightvt == VT_NULL))
        resvt = VT_NULL;
    else if (leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
        leftvt == VT_ERROR || rightvt == VT_ERROR)
    {
        hres = DISP_E_TYPEMISMATCH;
        goto end;
    }
    else if (leftvt == VT_NULL || rightvt == VT_NULL)
        resvt = VT_NULL;
    else if ((leftvt == VT_EMPTY && rightvt == VT_BSTR) ||
        (leftvt == VT_DATE && rightvt == VT_DATE) ||
        (leftvt == VT_BSTR && rightvt == VT_EMPTY) ||
        (leftvt == VT_BSTR && rightvt == VT_BSTR))
        resvt = VT_R8;
    else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
        resvt = VT_DECIMAL;
    else if (leftvt == VT_DATE || rightvt == VT_DATE)
        resvt = VT_DATE;
    else if (leftvt == VT_CY || rightvt == VT_CY)
        resvt = VT_CY;
    else if (leftvt == VT_R8 || rightvt == VT_R8)
        resvt = VT_R8;
    else if (leftvt == VT_BSTR || rightvt == VT_BSTR)
        resvt = VT_R8;
    else if (leftvt == VT_R4 || rightvt == VT_R4)
    {
        if (leftvt == VT_I4 || rightvt == VT_I4 ||
            leftvt == VT_I8 || rightvt == VT_I8)
            resvt = VT_R8;
        else
            resvt = VT_R4;
    }
    else if (leftvt == VT_I8 || rightvt == VT_I8)
        resvt = VT_I8;
    else if (leftvt == VT_I4 || rightvt == VT_I4)
        resvt = VT_I4;
    else if (leftvt == VT_I2 || rightvt == VT_I2 ||
        leftvt == VT_BOOL || rightvt == VT_BOOL ||
        (leftvt == VT_EMPTY && rightvt == VT_EMPTY))
        resvt = VT_I2;
    else if (leftvt == VT_UI1 || rightvt == VT_UI1)
        resvt = VT_UI1;
    else
    {
        hres = DISP_E_TYPEMISMATCH;
        goto end;
    }

    /* coerce to the result type */
    if (leftvt == VT_BSTR && rightvt == VT_DATE)
        hres = VariantChangeType(&lv, left, 0, VT_R8);
    else
        hres = VariantChangeType(&lv, left, 0, resvt);
    if (hres != S_OK) goto end;
    if (leftvt == VT_DATE && rightvt == VT_BSTR)
        hres = VariantChangeType(&rv, right, 0, VT_R8);
    else
        hres = VariantChangeType(&rv, right, 0, resvt);
    if (hres != S_OK) goto end;

    /* do the math */
    V_VT(result) = resvt;
    switch (resvt)
    {
    case VT_NULL:
    break;
    case VT_DATE:
    V_DATE(result) = V_DATE(&lv) - V_DATE(&rv);
    break;
    case VT_CY:
    hres = VarCySub(V_CY(&lv), V_CY(&rv), &(V_CY(result)));
    break;
    case VT_R4:
    V_R4(result) = V_R4(&lv) - V_R4(&rv);
    break;
    case VT_I8:
    V_I8(result) = V_I8(&lv) - V_I8(&rv);
    break;
    case VT_I4:
    V_I4(result) = V_I4(&lv) - V_I4(&rv);
    break;
    case VT_I2:
    V_I2(result) = V_I2(&lv) - V_I2(&rv);
    break;
    case VT_UI1:
    V_UI1(result) = V_UI2(&lv) - V_UI1(&rv);
    break;
    case VT_R8:
    V_R8(result) = V_R8(&lv) - V_R8(&rv);
    break;
    case VT_DECIMAL:
    hres = VarDecSub(&(V_DECIMAL(&lv)), &(V_DECIMAL(&rv)), &(V_DECIMAL(result)));
    break;
    }

end:
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    TRACE("returning %#lx, %s\n", hres, debugstr_variant(result));
    return hres;
}


/**********************************************************************
 *              VarOr [OLEAUT32.157]
 *
 * Perform a logical or (OR) operation on two variants.
 *
 * PARAMS
 *  pVarLeft  [I] First variant
 *  pVarRight [I] Variant to OR with pVarLeft
 *  pVarOut   [O] Destination for OR result
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the result of the operation with its type
 *           taken from the table listed under VarXor().
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  See the Notes section of VarXor() for further information.
 */
HRESULT WINAPI VarOr(LPVARIANT pVarLeft, LPVARIANT pVarRight, LPVARIANT pVarOut)
{
    VARTYPE vt = VT_I4;
    VARIANT varLeft, varRight, varStr;
    HRESULT hRet;
    VARIANT tempLeft, tempRight;

    VariantInit(&tempLeft);
    VariantInit(&tempRight);
    VariantInit(&varLeft);
    VariantInit(&varRight);
    VariantInit(&varStr);

    TRACE("(%s,%s,%p)\n", debugstr_variant(pVarLeft), debugstr_variant(pVarRight), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarLeft) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hRet = VARIANT_FetchDispatchValue(pVarLeft, &tempLeft);
        if (FAILED(hRet)) goto VarOr_Exit;
        pVarLeft = &tempLeft;
    }
    if ((V_VT(pVarRight) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hRet = VARIANT_FetchDispatchValue(pVarRight, &tempRight);
        if (FAILED(hRet)) goto VarOr_Exit;
        pVarRight = &tempRight;
    }

    if (V_EXTRA_TYPE(pVarLeft) || V_EXTRA_TYPE(pVarRight) ||
        V_VT(pVarLeft) == VT_UNKNOWN || V_VT(pVarRight) == VT_UNKNOWN ||
        V_VT(pVarLeft) == VT_DISPATCH || V_VT(pVarRight) == VT_DISPATCH ||
        V_VT(pVarLeft) == VT_RECORD || V_VT(pVarRight) == VT_RECORD)
    {
        hRet = DISP_E_BADVARTYPE;
        goto VarOr_Exit;
    }

    V_VT(&varLeft) = V_VT(&varRight) = V_VT(&varStr) = VT_EMPTY;

    if (V_VT(pVarLeft) == VT_NULL || V_VT(pVarRight) == VT_NULL)
    {
        /* NULL OR Zero is NULL, NULL OR value is value */
        if (V_VT(pVarLeft) == VT_NULL)
            pVarLeft = pVarRight; /* point to the non-NULL var */

        V_VT(pVarOut) = VT_NULL;
        V_I4(pVarOut) = 0;

        switch (V_VT(pVarLeft))
        {
        case VT_DATE: case VT_R8:
            if (V_R8(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_BOOL:
            if (V_BOOL(pVarLeft))
                *pVarOut = *pVarLeft;
            hRet = S_OK;
            goto VarOr_Exit;
         case VT_I2: case VT_UI2:
            if (V_I2(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_I1:
            if (V_I1(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_UI1:
            if (V_UI1(pVarLeft))
                *pVarOut = *pVarLeft;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_R4:
            if (V_R4(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_I4: case VT_UI4: case VT_INT: case VT_UINT:
            if (V_I4(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_CY:
            if (V_CY(pVarLeft).int64)
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_I8: case VT_UI8:
            if (V_I8(pVarLeft))
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_DECIMAL:
            if (V_DECIMAL(pVarLeft).Hi32 || V_DECIMAL(pVarLeft).Lo64)
                goto VarOr_AsEmpty;
            hRet = S_OK;
            goto VarOr_Exit;
        case VT_BSTR:
        {
            VARIANT_BOOL b;

            if (!V_BSTR(pVarLeft))
            {
                hRet = DISP_E_BADVARTYPE;
                goto VarOr_Exit;
            }

            hRet = VarBoolFromStr(V_BSTR(pVarLeft), LOCALE_USER_DEFAULT, VAR_LOCALBOOL, &b);
            if (SUCCEEDED(hRet) && b)
            {
                V_VT(pVarOut) = VT_BOOL;
                V_BOOL(pVarOut) = b;
            }
            goto VarOr_Exit;
        }
        case VT_NULL: case VT_EMPTY:
            V_VT(pVarOut) = VT_NULL;
            hRet = S_OK;
            goto VarOr_Exit;
        default:
            hRet = DISP_E_BADVARTYPE;
            goto VarOr_Exit;
        }
    }

    if (V_VT(pVarLeft) == VT_EMPTY || V_VT(pVarRight) == VT_EMPTY)
    {
        if (V_VT(pVarLeft) == VT_EMPTY)
            pVarLeft = pVarRight; /* point to the non-EMPTY var */

VarOr_AsEmpty:
        /* Since one argument is empty (0), OR'ing it with the other simply
         * gives the others value (as 0|x => x). So just convert the other
         * argument to the required result type.
         */
        switch (V_VT(pVarLeft))
        {
        case VT_BSTR:
            if (!V_BSTR(pVarLeft))
            {
                hRet = DISP_E_BADVARTYPE;
                goto VarOr_Exit;
            }

            hRet = VariantCopy(&varStr, pVarLeft);
            if (FAILED(hRet))
                goto VarOr_Exit;
            pVarLeft = &varStr;
            hRet = VariantChangeType(pVarLeft, pVarLeft, 0, VT_BOOL);
            if (FAILED(hRet))
                goto VarOr_Exit;
            /* Fall Through ... */
        case VT_EMPTY: case VT_UI1: case VT_BOOL: case VT_I2:
            V_VT(pVarOut) = VT_I2;
            break;
        case VT_DATE: case VT_CY: case VT_DECIMAL: case VT_R4: case VT_R8:
        case VT_I1: case VT_UI2: case VT_I4: case VT_UI4:
        case VT_INT: case VT_UINT: case VT_UI8:
            V_VT(pVarOut) = VT_I4;
            break;
        case VT_I8:
            V_VT(pVarOut) = VT_I8;
            break;
        default:
            hRet = DISP_E_BADVARTYPE;
            goto VarOr_Exit;
        }
        hRet = VariantCopy(&varLeft, pVarLeft);
        if (FAILED(hRet))
            goto VarOr_Exit;
        pVarLeft = &varLeft;
        hRet = VariantChangeType(pVarOut, pVarLeft, 0, V_VT(pVarOut));
        goto VarOr_Exit;
    }

    if (V_VT(pVarLeft) == VT_BOOL && V_VT(pVarRight) == VT_BOOL)
    {
        V_VT(pVarOut) = VT_BOOL;
        V_BOOL(pVarOut) = V_BOOL(pVarLeft) | V_BOOL(pVarRight);
        hRet = S_OK;
        goto VarOr_Exit;
    }

    if (V_VT(pVarLeft) == VT_UI1 && V_VT(pVarRight) == VT_UI1)
    {
        V_VT(pVarOut) = VT_UI1;
        V_UI1(pVarOut) = V_UI1(pVarLeft) | V_UI1(pVarRight);
        hRet = S_OK;
        goto VarOr_Exit;
    }

    if (V_VT(pVarLeft) == VT_BSTR)
    {
        hRet = VariantCopy(&varStr, pVarLeft);
        if (FAILED(hRet))
            goto VarOr_Exit;
        pVarLeft = &varStr;
        hRet = VariantChangeType(pVarLeft, pVarLeft, 0, VT_BOOL);
        if (FAILED(hRet))
            goto VarOr_Exit;
    }

    if (V_VT(pVarLeft) == VT_BOOL &&
        (V_VT(pVarRight) == VT_BOOL || V_VT(pVarRight) == VT_BSTR))
    {
        vt = VT_BOOL;
    }
    else if ((V_VT(pVarLeft) == VT_BOOL || V_VT(pVarLeft) == VT_UI1 ||
        V_VT(pVarLeft) == VT_I2 || V_VT(pVarLeft) == VT_BSTR) &&
        (V_VT(pVarRight) == VT_BOOL || V_VT(pVarRight) == VT_UI1 ||
        V_VT(pVarRight) == VT_I2 || V_VT(pVarRight) == VT_BSTR))
    {
        vt = VT_I2;
    }
    else if (V_VT(pVarLeft) == VT_I8 || V_VT(pVarRight) == VT_I8)
    {
        if (V_VT(pVarLeft) == VT_INT || V_VT(pVarRight) == VT_INT)
        {
            hRet = DISP_E_TYPEMISMATCH;
            goto VarOr_Exit;
        }
        vt = VT_I8;
    }

    hRet = VariantCopy(&varLeft, pVarLeft);
    if (FAILED(hRet))
        goto VarOr_Exit;

    hRet = VariantCopy(&varRight, pVarRight);
    if (FAILED(hRet))
        goto VarOr_Exit;

    if (vt == VT_I4 && V_VT(&varLeft) == VT_UI4)
        V_VT(&varLeft) = VT_I4; /* Don't overflow */
    else
    {
        double d;

        if (V_VT(&varLeft) == VT_BSTR &&
            FAILED(VarR8FromStr(V_BSTR(&varLeft), LOCALE_USER_DEFAULT, 0, &d)))
            hRet = VariantChangeType(&varLeft, &varLeft, VARIANT_LOCALBOOL, VT_BOOL);
        if (SUCCEEDED(hRet) && V_VT(&varLeft) != vt)
            hRet = VariantChangeType(&varLeft, &varLeft, 0, vt);
        if (FAILED(hRet))
            goto VarOr_Exit;
    }

    if (vt == VT_I4 && V_VT(&varRight) == VT_UI4)
        V_VT(&varRight) = VT_I4; /* Don't overflow */
    else
    {
        double d;

        if (V_VT(&varRight) == VT_BSTR &&
            FAILED(VarR8FromStr(V_BSTR(&varRight), LOCALE_USER_DEFAULT, 0, &d)))
            hRet = VariantChangeType(&varRight, &varRight, VARIANT_LOCALBOOL, VT_BOOL);
        if (SUCCEEDED(hRet) && V_VT(&varRight) != vt)
            hRet = VariantChangeType(&varRight, &varRight, 0, vt);
        if (FAILED(hRet))
            goto VarOr_Exit;
    }

    V_VT(pVarOut) = vt;
    if (vt == VT_I8)
    {
        V_I8(pVarOut) = V_I8(&varLeft) | V_I8(&varRight);
    }
    else if (vt == VT_I4)
    {
        V_I4(pVarOut) = V_I4(&varLeft) | V_I4(&varRight);
    }
    else
    {
        V_I2(pVarOut) = V_I2(&varLeft) | V_I2(&varRight);
    }

VarOr_Exit:
    VariantClear(&varStr);
    VariantClear(&varLeft);
    VariantClear(&varRight);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    return hRet;
}

/**********************************************************************
 * VarAbs [OLEAUT32.168]
 *
 * Convert a variant to its absolute value.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the absolute value of pVarIn.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - This function does not process by-reference variants.
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *| VT_BOOL          VT_I2
 *| VT_BSTR          VT_R8
 *| (All others)     Unchanged
 */
HRESULT WINAPI VarAbs(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    HRESULT hRet = S_OK;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%p)\n", debugstr_variant(pVarIn), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarAbs_Exit;
        pVarIn = &temp;
    }

    if (V_ISARRAY(pVarIn) || V_VT(pVarIn) == VT_UNKNOWN ||
        V_VT(pVarIn) == VT_DISPATCH || V_VT(pVarIn) == VT_RECORD ||
        V_VT(pVarIn) == VT_ERROR)
    {
        hRet = DISP_E_TYPEMISMATCH;
        goto VarAbs_Exit;
    }
    *pVarOut = *pVarIn; /* Shallow copy the value, and invert it if needed */

#define ABS_CASE(typ,min) \
    case VT_##typ: if (V_##typ(pVarIn) == min) hRet = DISP_E_OVERFLOW; \
                  else if (V_##typ(pVarIn) < 0) V_##typ(pVarOut) = -V_##typ(pVarIn); \
                  break

    switch (V_VT(pVarIn))
    {
    ABS_CASE(I1,I1_MIN);
    case VT_BOOL:
        V_VT(pVarOut) = VT_I2;
        /* BOOL->I2, Fall through ... */
    ABS_CASE(I2,I2_MIN);
    case VT_INT:
    ABS_CASE(I4,I4_MIN);
    ABS_CASE(I8,I8_MIN);
    case VT_R4:
        if (V_R4(pVarOut) < 0.0) V_R4(pVarOut) = -V_R4(pVarOut);
        break;
    case VT_BSTR:
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(pVarOut));
        if (FAILED(hRet))
            break;
        V_VT(pVarOut) = VT_R8;
        /* Fall through ... */
    case VT_DATE:
    case VT_R8:
        if (V_R8(pVarOut) < 0.0) V_R8(pVarOut) = -V_R8(pVarOut);
        break;
    case VT_CY:
        hRet = VarCyAbs(V_CY(pVarIn), & V_CY(pVarOut));
        break;
    case VT_DECIMAL:
        V_DECIMAL(pVarOut).sign &= ~DECIMAL_NEG;
        break;
    case VT_UI1:
    case VT_UI2:
    case VT_UINT:
    case VT_UI4:
    case VT_UI8:
        /* No-Op */
        break;
    case VT_EMPTY:
        V_VT(pVarOut) = VT_I2;
    case VT_NULL:
        V_I2(pVarOut) = 0;
        break;
    default:
        hRet = DISP_E_BADVARTYPE;
    }

VarAbs_Exit:
    VariantClear(&temp);
    return hRet;
}

/**********************************************************************
 *              VarFix [OLEAUT32.169]
 *
 * Truncate a variants value to a whole number.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *|  VT_BOOL          VT_I2
 *|  VT_EMPTY         VT_I2
 *|  VT_BSTR          VT_R8
 *|  All Others       Unchanged
 *  - The difference between this function and VarInt() is that VarInt() rounds
 *    negative numbers away from 0, while this function rounds them towards zero.
 */
HRESULT WINAPI VarFix(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    HRESULT hRet = S_OK;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%p)\n", debugstr_variant(pVarIn), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarFix_Exit;
        pVarIn = &temp;
    }
    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_UI1:
        V_UI1(pVarOut) = V_UI1(pVarIn);
        break;
    case VT_BOOL:
        V_VT(pVarOut) = VT_I2;
        /* Fall through */
     case VT_I2:
        V_I2(pVarOut) = V_I2(pVarIn);
        break;
     case VT_I4:
        V_I4(pVarOut) = V_I4(pVarIn);
        break;
     case VT_I8:
        V_I8(pVarOut) = V_I8(pVarIn);
        break;
    case VT_R4:
        if (V_R4(pVarIn) < 0.0f)
            V_R4(pVarOut) = (float)ceil(V_R4(pVarIn));
        else
            V_R4(pVarOut) = (float)floor(V_R4(pVarIn));
        break;
    case VT_BSTR:
        V_VT(pVarOut) = VT_R8;
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(pVarOut));
        pVarIn = pVarOut;
        /* Fall through */
    case VT_DATE:
    case VT_R8:
        if (V_R8(pVarIn) < 0.0)
            V_R8(pVarOut) = ceil(V_R8(pVarIn));
        else
            V_R8(pVarOut) = floor(V_R8(pVarIn));
        break;
    case VT_CY:
        hRet = VarCyFix(V_CY(pVarIn), &V_CY(pVarOut));
        break;
    case VT_DECIMAL:
        hRet = VarDecFix(&V_DECIMAL(pVarIn), &V_DECIMAL(pVarOut));
        break;
    case VT_EMPTY:
        V_VT(pVarOut) = VT_I2;
        V_I2(pVarOut) = 0;
        break;
    case VT_NULL:
        /* No-Op */
        break;
    default:
        if (V_TYPE(pVarIn) == VT_CLSID || /* VT_CLSID is a special case */
            FAILED(VARIANT_ValidateType(V_VT(pVarIn))))
            hRet = DISP_E_BADVARTYPE;
        else
            hRet = DISP_E_TYPEMISMATCH;
    }
VarFix_Exit:
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;
    VariantClear(&temp);

    return hRet;
}

/**********************************************************************
 *              VarInt [OLEAUT32.172]
 *
 * Truncate a variants value to a whole number.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *|  VT_BOOL          VT_I2
 *|  VT_EMPTY         VT_I2
 *|  VT_BSTR          VT_R8
 *|  All Others       Unchanged
 *  - The difference between this function and VarFix() is that VarFix() rounds
 *    negative numbers towards 0, while this function rounds them away from zero.
 */
HRESULT WINAPI VarInt(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    HRESULT hRet = S_OK;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%p)\n", debugstr_variant(pVarIn), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarInt_Exit;
        pVarIn = &temp;
    }
    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_R4:
        V_R4(pVarOut) = (float)floor(V_R4(pVarIn));
        break;
    case VT_BSTR:
        V_VT(pVarOut) = VT_R8;
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(pVarOut));
        pVarIn = pVarOut;
        /* Fall through */
    case VT_DATE:
    case VT_R8:
        V_R8(pVarOut) = floor(V_R8(pVarIn));
        break;
    case VT_CY:
        hRet = VarCyInt(V_CY(pVarIn), &V_CY(pVarOut));
        break;
    case VT_DECIMAL:
        hRet = VarDecInt(&V_DECIMAL(pVarIn), &V_DECIMAL(pVarOut));
        break;
    default:
        hRet = VarFix(pVarIn, pVarOut);
    }
VarInt_Exit:
    VariantClear(&temp);

    return hRet;
}

/**********************************************************************
 *              VarXor [OLEAUT32.167]
 *
 * Perform a logical exclusive-or (XOR) operation on two variants.
 *
 * PARAMS
 *  pVarLeft  [I] First variant
 *  pVarRight [I] Variant to XOR with pVarLeft
 *  pVarOut   [O] Destination for XOR result
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the result of the operation with its type
 *           taken from the table below).
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - Neither pVarLeft or pVarRight are modified by this function.
 *  - This function does not process by-reference variants.
 *  - Input types of VT_BSTR may be numeric strings or boolean text.
 *  - The type of result stored in pVarOut depends on the types of pVarLeft
 *    and pVarRight, and will be one of VT_UI1, VT_I2, VT_I4, VT_I8, VT_BOOL,
 *    or VT_NULL if the function succeeds.
 *  - Type promotion is inconsistent and as a result certain combinations of
 *    values will return DISP_E_OVERFLOW even when they could be represented.
 *    This matches the behaviour of native oleaut32.
 */
HRESULT WINAPI VarXor(LPVARIANT pVarLeft, LPVARIANT pVarRight, LPVARIANT pVarOut)
{
    VARTYPE vt;
    VARIANT varLeft, varRight;
    VARIANT tempLeft, tempRight;
    double d;
    HRESULT hRet;

    TRACE("(%s,%s,%p)\n", debugstr_variant(pVarLeft), debugstr_variant(pVarRight), pVarOut);

    if (V_EXTRA_TYPE(pVarLeft) || V_EXTRA_TYPE(pVarRight) ||
        V_VT(pVarLeft) > VT_UINT || V_VT(pVarRight) > VT_UINT ||
        V_VT(pVarLeft) == VT_VARIANT || V_VT(pVarRight) == VT_VARIANT ||
        V_VT(pVarLeft) == VT_UNKNOWN || V_VT(pVarRight) == VT_UNKNOWN ||
        V_VT(pVarLeft) == (VARTYPE)15 || V_VT(pVarRight) == (VARTYPE)15 ||
        V_VT(pVarLeft) == VT_ERROR || V_VT(pVarRight) == VT_ERROR)
        return DISP_E_BADVARTYPE;

    if (V_VT(pVarLeft) == VT_NULL || V_VT(pVarRight) == VT_NULL)
    {
        /* NULL XOR anything valid is NULL */
        V_VT(pVarOut) = VT_NULL;
        return S_OK;
    }

    V_VT(&varLeft) = V_VT(&varRight) = VT_EMPTY;
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarLeft) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hRet = VARIANT_FetchDispatchValue(pVarLeft, &tempLeft);
        if (FAILED(hRet)) goto VarXor_Exit;
        pVarLeft = &tempLeft;
    }
    if ((V_VT(pVarRight) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hRet = VARIANT_FetchDispatchValue(pVarRight, &tempRight);
        if (FAILED(hRet)) goto VarXor_Exit;
        pVarRight = &tempRight;
    }

    /* Copy our inputs so we don't disturb anything */
    hRet = VariantCopy(&varLeft, pVarLeft);
    if (FAILED(hRet))
        goto VarXor_Exit;

    hRet = VariantCopy(&varRight, pVarRight);
    if (FAILED(hRet))
        goto VarXor_Exit;

    /* Try any strings first as numbers, then as VT_BOOL */
    if (V_VT(&varLeft) == VT_BSTR)
    {
        hRet = VarR8FromStr(V_BSTR(&varLeft), LOCALE_USER_DEFAULT, 0, &d);
        hRet = VariantChangeType(&varLeft, &varLeft, VARIANT_LOCALBOOL,
                                 FAILED(hRet) ? VT_BOOL : VT_I4);
        if (FAILED(hRet))
            goto VarXor_Exit;
    }

    if (V_VT(&varRight) == VT_BSTR)
    {
        hRet = VarR8FromStr(V_BSTR(&varRight), LOCALE_USER_DEFAULT, 0, &d);
        hRet = VariantChangeType(&varRight, &varRight, VARIANT_LOCALBOOL,
                                 FAILED(hRet) ? VT_BOOL : VT_I4);
        if (FAILED(hRet))
            goto VarXor_Exit;
    }

    /* Determine the result type */
    if (V_VT(&varLeft) == VT_I8 || V_VT(&varRight) == VT_I8)
    {
        if (V_VT(pVarLeft) == VT_INT || V_VT(pVarRight) == VT_INT)
        {
            hRet = DISP_E_TYPEMISMATCH;
            goto VarXor_Exit;
        }
        vt = VT_I8;
    }
    else
    {
        switch ((V_VT(&varLeft) << 16) | V_VT(&varRight))
        {
        case (VT_BOOL  << 16) | VT_BOOL:
            vt = VT_BOOL;
            break;
        case (VT_UI1   << 16) | VT_UI1:
            vt = VT_UI1;
            break;
        case (VT_EMPTY << 16) | VT_EMPTY:
        case (VT_EMPTY << 16) | VT_UI1:
        case (VT_EMPTY << 16) | VT_I2:
        case (VT_EMPTY << 16) | VT_BOOL:
        case (VT_UI1   << 16) | VT_EMPTY:
        case (VT_UI1   << 16) | VT_I2:
        case (VT_UI1   << 16) | VT_BOOL:
        case (VT_I2    << 16) | VT_EMPTY:
        case (VT_I2    << 16) | VT_UI1:
        case (VT_I2    << 16) | VT_I2:
        case (VT_I2    << 16) | VT_BOOL:
        case (VT_BOOL  << 16) | VT_EMPTY:
        case (VT_BOOL  << 16) | VT_UI1:
        case (VT_BOOL  << 16) | VT_I2:
            vt = VT_I2;
            break;
        default:
            vt = VT_I4;
            break;
        }
    }

    /* VT_UI4 does not overflow */
    if (vt != VT_I8)
    {
        if (V_VT(&varLeft) == VT_UI4)
            V_VT(&varLeft) = VT_I4;
        if (V_VT(&varRight) == VT_UI4)
            V_VT(&varRight) = VT_I4;
    }

    /* Convert our input copies to the result type */
    if (V_VT(&varLeft) != vt)
        hRet = VariantChangeType(&varLeft, &varLeft, 0, vt);
    if (FAILED(hRet))
        goto VarXor_Exit;

    if (V_VT(&varRight) != vt)
        hRet = VariantChangeType(&varRight, &varRight, 0, vt);
    if (FAILED(hRet))
        goto VarXor_Exit;

    V_VT(pVarOut) = vt;

    /* Calculate the result */
    switch (vt)
    {
    case VT_I8:
        V_I8(pVarOut) = V_I8(&varLeft) ^ V_I8(&varRight);
        break;
    case VT_I4:
        V_I4(pVarOut) = V_I4(&varLeft) ^ V_I4(&varRight);
        break;
    case VT_BOOL:
    case VT_I2:
        V_I2(pVarOut) = V_I2(&varLeft) ^ V_I2(&varRight);
        break;
    case VT_UI1:
        V_UI1(pVarOut) = V_UI1(&varLeft) ^ V_UI1(&varRight);
        break;
    }

VarXor_Exit:
    VariantClear(&varLeft);
    VariantClear(&varRight);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    return hRet;
}

/**********************************************************************
 *              VarEqv [OLEAUT32.172]
 *
 * Determine if two variants contain the same value.
 *
 * PARAMS
 *  pVarLeft  [I] First variant to compare
 *  pVarRight [I] Variant to compare to pVarLeft
 *  pVarOut   [O] Destination for comparison result
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the result of the comparison (VARIANT_TRUE
 *           if equivalent or non-zero otherwise.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - This function simply calls VarXor() on pVarLeft and pVarRight and inverts
 *    the result.
 */
HRESULT WINAPI VarEqv(LPVARIANT pVarLeft, LPVARIANT pVarRight, LPVARIANT pVarOut)
{
    HRESULT hRet;

    TRACE("(%s,%s,%p)\n", debugstr_variant(pVarLeft), debugstr_variant(pVarRight), pVarOut);

    hRet = VarXor(pVarLeft, pVarRight, pVarOut);
    if (SUCCEEDED(hRet))
    {
        if (V_VT(pVarOut) == VT_I8)
            V_I8(pVarOut) = ~V_I8(pVarOut);
        else
            V_UI4(pVarOut) = ~V_UI4(pVarOut);
    }
    return hRet;
}

/**********************************************************************
 *              VarNeg [OLEAUT32.173]
 *
 * Negate the value of a variant.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *|  VT_EMPTY         VT_I2
 *|  VT_UI1           VT_I2
 *|  VT_BOOL          VT_I2
 *|  VT_BSTR          VT_R8
 *|  All Others       Unchanged (unless promoted)
 *  - Where the negated value of a variant does not fit in its base type, the type
 *    is promoted according to the following table:
 *| Input Type       Promoted To
 *| ----------       -----------
 *|   VT_I2            VT_I4
 *|   VT_I4            VT_R8
 *|   VT_I8            VT_R8
 *  - The native version of this function returns DISP_E_BADVARTYPE for valid
 *    variant types that cannot be negated, and returns DISP_E_TYPEMISMATCH
 *    for types which are not valid. Since this is in contravention of the
 *    meaning of those error codes and unlikely to be relied on by applications,
 *    this implementation returns errors consistent with the other high level
 *    variant math functions.
 */
HRESULT WINAPI VarNeg(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    HRESULT hRet = S_OK;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%p)\n", debugstr_variant(pVarIn), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarNeg_Exit;
        pVarIn = &temp;
    }
    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_UI1:
        V_VT(pVarOut) = VT_I2;
        V_I2(pVarOut) = -V_UI1(pVarIn);
        break;
    case VT_BOOL:
        V_VT(pVarOut) = VT_I2;
        /* Fall through */
    case VT_I2:
        if (V_I2(pVarIn) == I2_MIN)
        {
            V_VT(pVarOut) = VT_I4;
            V_I4(pVarOut) = -(int)V_I2(pVarIn);
        }
        else
            V_I2(pVarOut) = -V_I2(pVarIn);
        break;
    case VT_I4:
        if (V_I4(pVarIn) == I4_MIN)
        {
            V_VT(pVarOut) = VT_R8;
            V_R8(pVarOut) = -(double)V_I4(pVarIn);
        }
        else
            V_I4(pVarOut) = -V_I4(pVarIn);
        break;
    case VT_I8:
        if (V_I8(pVarIn) == I8_MIN)
        {
            V_VT(pVarOut) = VT_R8;
            hRet = VarR8FromI8(V_I8(pVarIn), &V_R8(pVarOut));
            V_R8(pVarOut) *= -1.0;
        }
        else
            V_I8(pVarOut) = -V_I8(pVarIn);
        break;
    case VT_R4:
        V_R4(pVarOut) = -V_R4(pVarIn);
        break;
    case VT_DATE:
    case VT_R8:
        V_R8(pVarOut) = -V_R8(pVarIn);
        break;
    case VT_CY:
        hRet = VarCyNeg(V_CY(pVarIn), &V_CY(pVarOut));
        break;
    case VT_DECIMAL:
        hRet = VarDecNeg(&V_DECIMAL(pVarIn), &V_DECIMAL(pVarOut));
        break;
    case VT_BSTR:
        V_VT(pVarOut) = VT_R8;
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(pVarOut));
        V_R8(pVarOut) = -V_R8(pVarOut);
        break;
    case VT_EMPTY:
        V_VT(pVarOut) = VT_I2;
        V_I2(pVarOut) = 0;
        break;
    case VT_NULL:
        /* No-Op */
        break;
    default:
        if (V_TYPE(pVarIn) == VT_CLSID || /* VT_CLSID is a special case */
            FAILED(VARIANT_ValidateType(V_VT(pVarIn))))
            hRet = DISP_E_BADVARTYPE;
        else
            hRet = DISP_E_TYPEMISMATCH;
    }
VarNeg_Exit:
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;
    VariantClear(&temp);

    return hRet;
}

/**********************************************************************
 *              VarNot [OLEAUT32.174]
 *
 * Perform a not operation on a variant.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - Strictly speaking, this function performs a bitwise ones complement
 *    on the variants value (after possibly converting to VT_I4, see below).
 *    This only behaves like a boolean not operation if the value in
 *    pVarIn is either VARIANT_TRUE or VARIANT_FALSE and the type is signed.
 *  - To perform a genuine not operation, convert the variant to a VT_BOOL
 *    before calling this function.
 *  - This function does not process by-reference variants.
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *| VT_EMPTY         VT_I2
 *| VT_R4            VT_I4
 *| VT_R8            VT_I4
 *| VT_BSTR          VT_I4
 *| VT_DECIMAL       VT_I4
 *| VT_CY            VT_I4
 *| (All others)     Unchanged
 */
HRESULT WINAPI VarNot(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    VARIANT varIn;
    HRESULT hRet = S_OK;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%p)\n", debugstr_variant(pVarIn), pVarOut);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarNot_Exit;
        pVarIn = &temp;
    }

    if (V_VT(pVarIn) == VT_BSTR)
    {
        V_VT(&varIn) = VT_R8;
        hRet = VarR8FromStr( V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn) );
        if (FAILED(hRet))
        {
            V_VT(&varIn) = VT_BOOL;
            hRet = VarBoolFromStr( V_BSTR(pVarIn), LOCALE_USER_DEFAULT, VAR_LOCALBOOL, &V_BOOL(&varIn) );
        }
        if (FAILED(hRet)) goto VarNot_Exit;
        pVarIn = &varIn;
    }

    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_I1:
        V_I4(pVarOut) = ~V_I1(pVarIn);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_UI1: V_UI1(pVarOut) = ~V_UI1(pVarIn); break;
    case VT_BOOL:
    case VT_I2:  V_I2(pVarOut) = ~V_I2(pVarIn); break;
    case VT_UI2:
        V_I4(pVarOut) = ~V_UI2(pVarIn);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_DECIMAL:
        hRet = VarI4FromDec(&V_DECIMAL(pVarIn), &V_I4(&varIn));
        if (FAILED(hRet))
            break;
        pVarIn = &varIn;
        /* Fall through ... */
    case VT_INT:
        V_VT(pVarOut) = VT_I4;
        /* Fall through ... */
    case VT_I4:  V_I4(pVarOut) = ~V_I4(pVarIn); break;
    case VT_UINT:
    case VT_UI4:
        V_I4(pVarOut) = ~V_UI4(pVarIn);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_I8:  V_I8(pVarOut) = ~V_I8(pVarIn); break;
    case VT_UI8:
        V_I4(pVarOut) = ~V_UI8(pVarIn);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_R4:
        hRet = VarI4FromR4(V_R4(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_DATE:
    case VT_R8:
        hRet = VarI4FromR8(V_R8(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_CY:
        hRet = VarI4FromCy(V_CY(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_EMPTY:
        V_I2(pVarOut) = ~0;
        V_VT(pVarOut) = VT_I2;
        break;
    case VT_NULL:
        /* No-Op */
        break;
    default:
        if (V_TYPE(pVarIn) == VT_CLSID || /* VT_CLSID is a special case */
            FAILED(VARIANT_ValidateType(V_VT(pVarIn))))
            hRet = DISP_E_BADVARTYPE;
        else
            hRet = DISP_E_TYPEMISMATCH;
    }
VarNot_Exit:
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;
    VariantClear(&temp);

    return hRet;
}

/**********************************************************************
 *              VarRound [OLEAUT32.175]
 *
 * Perform a round operation on a variant.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  deci    [I] Number of decimals to round to
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - Floating point values are rounded to the desired number of decimals.
 *  - Some integer types are just copied to the return variable.
 *  - Some other integer types are not handled and fail.
 */
HRESULT WINAPI VarRound(LPVARIANT pVarIn, int deci, LPVARIANT pVarOut)
{
    VARIANT varIn;
    HRESULT hRet = S_OK;
    float factor;
    VARIANT temp;

    VariantInit(&temp);

    TRACE("(%s,%d)\n", debugstr_variant(pVarIn), deci);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(pVarIn) & VT_TYPEMASK) == VT_DISPATCH && ((V_VT(pVarIn) & ~VT_TYPEMASK) == 0))
    {
        hRet = VARIANT_FetchDispatchValue(pVarIn, &temp);
        if (FAILED(hRet)) goto VarRound_Exit;
        pVarIn = &temp;
    }

    switch (V_VT(pVarIn))
    {
    /* cases that fail on windows */
    case VT_I1:
    case VT_I8:
    case VT_UI2:
    case VT_UI4:
	hRet = DISP_E_BADVARTYPE;
	break;

    /* cases just copying in to out */
    case VT_UI1:
	V_VT(pVarOut) = V_VT(pVarIn);
	V_UI1(pVarOut) = V_UI1(pVarIn);
	break;
    case VT_I2:
	V_VT(pVarOut) = V_VT(pVarIn);
	V_I2(pVarOut) = V_I2(pVarIn);
	break;
    case VT_I4:
	V_VT(pVarOut) = V_VT(pVarIn);
	V_I4(pVarOut) = V_I4(pVarIn);
	break;
    case VT_NULL:
	V_VT(pVarOut) = V_VT(pVarIn);
	/* value unchanged */
	break;

    /* cases that change type */
    case VT_EMPTY:
	V_VT(pVarOut) = VT_I2;
	V_I2(pVarOut) = 0;
	break;
    case VT_BOOL:
	V_VT(pVarOut) = VT_I2;
	V_I2(pVarOut) = V_BOOL(pVarIn);
	break;
    case VT_BSTR:
	hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn));
	if (FAILED(hRet))
	    break;
	V_VT(&varIn)=VT_R8;
	pVarIn = &varIn;
	/* Fall through ... */

    /* cases we need to do math */
    case VT_R8:
	if (V_R8(pVarIn)>0) {
	    V_R8(pVarOut)=floor(V_R8(pVarIn)*pow(10, deci)+0.5)/pow(10, deci);
	} else {
	    V_R8(pVarOut)=ceil(V_R8(pVarIn)*pow(10, deci)-0.5)/pow(10, deci);
	}
	V_VT(pVarOut) = V_VT(pVarIn);
	break;
    case VT_R4:
	if (V_R4(pVarIn)>0) {
	    V_R4(pVarOut)=floor(V_R4(pVarIn)*pow(10, deci)+0.5)/pow(10, deci);
	} else {
	    V_R4(pVarOut)=ceil(V_R4(pVarIn)*pow(10, deci)-0.5)/pow(10, deci);
	}
	V_VT(pVarOut) = V_VT(pVarIn);
	break;
    case VT_DATE:
	if (V_DATE(pVarIn)>0) {
	    V_DATE(pVarOut)=floor(V_DATE(pVarIn)*pow(10, deci)+0.5)/pow(10, deci);
	} else {
	    V_DATE(pVarOut)=ceil(V_DATE(pVarIn)*pow(10, deci)-0.5)/pow(10, deci);
	}
	V_VT(pVarOut) = V_VT(pVarIn);
	break;
    case VT_CY:
	if (deci>3)
	    factor=1;
	else
	    factor=pow(10, 4-deci);

	if (V_CY(pVarIn).int64>0) {
	    V_CY(pVarOut).int64=floor(V_CY(pVarIn).int64/factor)*factor;
	} else {
	    V_CY(pVarOut).int64=ceil(V_CY(pVarIn).int64/factor)*factor;
	}
	V_VT(pVarOut) = V_VT(pVarIn);
	break;
    case VT_DECIMAL:
    {
        double dbl;

        hRet = VarR8FromDec(&V_DECIMAL(pVarIn), &dbl);
        if (FAILED(hRet))
            break;

        if (dbl>0.0f)
            dbl = floor(dbl*pow(10,deci)+0.5);
        else
            dbl = ceil(dbl*pow(10,deci)-0.5);

        V_VT(pVarOut)=VT_DECIMAL;
        hRet = VarDecFromR8(dbl, &V_DECIMAL(pVarOut));
        break;
    }
    /* cases we don't know yet */
    default:
	FIXME("unimplemented part, V_VT(pVarIn) == 0x%X, deci == %d\n",
		V_VT(pVarIn) & VT_TYPEMASK, deci);
	hRet = DISP_E_BADVARTYPE;
    }
VarRound_Exit:
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;
    VariantClear(&temp);

    TRACE("returning %#lx, %s\n", hRet, debugstr_variant(pVarOut));
    return hRet;
}

/**********************************************************************
 *              VarIdiv [OLEAUT32.153]
 *
 * Converts input variants to integers and divides them. 
 *
 * PARAMS
 *  left     [I] Left hand variant
 *  right    [I] Right hand variant
 *  result   [O] Destination for quotient
 *
 * RETURNS
 *  Success: S_OK.  result contains the quotient.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  If either expression is null, null is returned, as per MSDN
 */
HRESULT WINAPI VarIdiv(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres = S_OK;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT lv,rv;
    VARIANT tempLeft, tempRight;

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    VariantInit(&lv);
    VariantInit(&rv);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }
    ExtraFlags = leftExtraFlags;

    /* Native VarIdiv always returns an error when using extra
     * flags or if the variant combination is I8 and INT.
     */
    if ((leftvt == VT_I8 && rightvt == VT_INT) ||
        (leftvt == VT_INT && rightvt == VT_I8) ||
        (rightvt == VT_EMPTY && leftvt != VT_NULL) ||
        ExtraFlags != 0)
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* Determine variant type */
    else if (leftvt == VT_NULL || rightvt == VT_NULL)
    {
        V_VT(result) = VT_NULL;
        hres = S_OK;
        goto end;
    }
    else if (leftvt == VT_I8 || rightvt == VT_I8)
        resvt = VT_I8;
    else if (leftvt == VT_I4 || rightvt == VT_I4 ||
        leftvt == VT_INT || rightvt == VT_INT ||
        leftvt == VT_UINT || rightvt == VT_UINT ||
        leftvt == VT_UI8 || rightvt == VT_UI8 ||
        leftvt == VT_UI4 || rightvt == VT_UI4 ||
        leftvt == VT_UI2 || rightvt == VT_UI2 ||
        leftvt == VT_I1 || rightvt == VT_I1 ||
        leftvt == VT_BSTR || rightvt == VT_BSTR ||
        leftvt == VT_DATE || rightvt == VT_DATE ||
        leftvt == VT_CY || rightvt == VT_CY ||
        leftvt == VT_DECIMAL || rightvt == VT_DECIMAL ||
        leftvt == VT_R8 || rightvt == VT_R8 ||
        leftvt == VT_R4 || rightvt == VT_R4)
        resvt = VT_I4;
    else if (leftvt == VT_I2 || rightvt == VT_I2 ||
        leftvt == VT_BOOL || rightvt == VT_BOOL ||
        leftvt == VT_EMPTY)
        resvt = VT_I2;
    else if (leftvt == VT_UI1 || rightvt == VT_UI1)
        resvt = VT_UI1;
    else
    {
        hres = DISP_E_BADVARTYPE;
        goto end;
    }

    /* coerce to the result type */
    hres = VariantChangeType(&lv, left, 0, resvt);
    if (hres != S_OK) goto end;
    hres = VariantChangeType(&rv, right, 0, resvt);
    if (hres != S_OK) goto end;

    /* do the math */
    V_VT(result) = resvt;
    switch (resvt)
    {
    case VT_UI1:
    if (V_UI1(&rv) == 0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_UI1(result) = V_UI1(&lv) / V_UI1(&rv);
    break;
    case VT_I2:
    if (V_I2(&rv) == 0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_I2(result) = V_I2(&lv) / V_I2(&rv);
    break;
    case VT_I4:
    if (V_I4(&rv) == 0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_I4(result) = V_I4(&lv) / V_I4(&rv);
    break;
    case VT_I8:
    if (V_I8(&rv) == 0)
    {
        hres = DISP_E_DIVBYZERO;
        V_VT(result) = VT_EMPTY;
    }
    else
        V_I8(result) = V_I8(&lv) / V_I8(&rv);
    break;
    default:
        FIXME("Couldn't integer divide variant types %d,%d\n",
            leftvt,rightvt);
    }

end:
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);

    return hres;
}


/**********************************************************************
 *              VarMod [OLEAUT32.155]
 *
 * Perform the modulus operation of the right hand variant on the left
 *
 * PARAMS
 *  left     [I] Left hand variant
 *  right    [I] Right hand variant
 *  result   [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. result contains the remainder.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTE:
 *   If an error occurs the type of result will be modified but the value will not be.
 *   Doesn't support arrays or any special flags yet.
 */
HRESULT WINAPI VarMod(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT      rc         = E_FAIL;
    int          resT = 0;
    VARIANT      lv,rv;
    VARIANT tempLeft, tempRight;

    VariantInit(&tempLeft);
    VariantInit(&tempRight);
    VariantInit(&lv);
    VariantInit(&rv);
    V_VT(result) = VT_EMPTY;

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        rc = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(rc)) goto end;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        rc = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(rc)) goto end;
        right = &tempRight;
    }

    /* check for invalid inputs */
    switch (V_VT(left) & VT_TYPEMASK) {
    case VT_BOOL :
    case VT_I1   :
    case VT_I2   :
    case VT_I4   :
    case VT_I8   :
    case VT_INT  :
    case VT_UI1  :
    case VT_UI2  :
    case VT_UI4  :
    case VT_UI8  :
    case VT_UINT :
    case VT_R4   :
    case VT_R8   :
    case VT_CY   :
    case VT_EMPTY:
    case VT_DATE :
    case VT_BSTR :
    case VT_DECIMAL:
      break;
    case VT_VARIANT:
    case VT_UNKNOWN:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_TYPEMISMATCH;
      goto end;
    case VT_ERROR:
      rc = DISP_E_TYPEMISMATCH;
      goto end;
    case VT_RECORD:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_TYPEMISMATCH;
      goto end;
    case VT_NULL:
      break;
    default:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_BADVARTYPE;
      goto end;
    }


    switch (V_VT(right) & VT_TYPEMASK) {
    case VT_BOOL :
    case VT_I1   :
    case VT_I2   :
    case VT_I4   :
    case VT_I8   :
      if((V_VT(left) == VT_INT) && (V_VT(right) == VT_I8))
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_TYPEMISMATCH;
        goto end;
      }
    case VT_INT  :
      if((V_VT(right) == VT_INT) && (V_VT(left) == VT_I8))
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_TYPEMISMATCH;
        goto end;
      }
    case VT_UI1  :
    case VT_UI2  :
    case VT_UI4  :
    case VT_UI8  :
    case VT_UINT :
    case VT_R4   :
    case VT_R8   :
    case VT_CY   :
      if(V_VT(left) == VT_EMPTY)
      {
	V_VT(result) = VT_I4;
        V_I4(result) = 0;
        rc = S_OK;
        goto end;
      }
    case VT_EMPTY:
    case VT_DATE :
    case VT_DECIMAL:
      if(V_VT(left) == VT_ERROR)
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_TYPEMISMATCH;
        goto end;
      }
    case VT_BSTR:
      if(V_VT(left) == VT_NULL)
      {
	V_VT(result) = VT_NULL;
        rc = S_OK;
        goto end;
      }
      break;

    case VT_VOID:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_BADVARTYPE;
      goto end;
    case VT_NULL:
      if(V_VT(left) == VT_VOID)
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_BADVARTYPE;
      } else
      {
        V_VT(result) = VT_NULL;
        rc = S_OK;
      }
      goto end;
    case VT_VARIANT:
    case VT_UNKNOWN:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_TYPEMISMATCH;
      goto end;
    case VT_ERROR:
      rc = DISP_E_TYPEMISMATCH;
      goto end;
    case VT_RECORD:
      if((V_VT(left) == 15) || ((V_VT(left) >= 24) && (V_VT(left) <= 35)))
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_BADVARTYPE;
      } else
      {
	V_VT(result) = VT_EMPTY;
        rc = DISP_E_TYPEMISMATCH;
      }
      goto end;
    default:
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_BADVARTYPE;
      goto end;
    }

    /* determine the result type */
    if((V_VT(left) == VT_I8)        || (V_VT(right) == VT_I8))   resT = VT_I8;
    else if((V_VT(left) == VT_UI1)  && (V_VT(right) == VT_BOOL)) resT = VT_I2;
    else if((V_VT(left) == VT_UI1)  && (V_VT(right) == VT_UI1))  resT = VT_UI1;
    else if((V_VT(left) == VT_UI1)  && (V_VT(right) == VT_I2))   resT = VT_I2;
    else if((V_VT(left) == VT_I2)   && (V_VT(right) == VT_BOOL)) resT = VT_I2;
    else if((V_VT(left) == VT_I2)   && (V_VT(right) == VT_UI1))  resT = VT_I2;
    else if((V_VT(left) == VT_I2)   && (V_VT(right) == VT_I2))   resT = VT_I2;
    else if((V_VT(left) == VT_BOOL) && (V_VT(right) == VT_BOOL)) resT = VT_I2;
    else if((V_VT(left) == VT_BOOL) && (V_VT(right) == VT_UI1))  resT = VT_I2;
    else if((V_VT(left) == VT_BOOL) && (V_VT(right) == VT_I2))   resT = VT_I2;
    else resT = VT_I4; /* most outputs are I4 */

    /* convert to I8 for the modulo */
    rc = VariantChangeType(&lv, left, 0, VT_I8);
    if(FAILED(rc))
    {
      FIXME("Could not convert left type %d to %d? rc == %#lx.\n", V_VT(left), VT_I8, rc);
      goto end;
    }

    rc = VariantChangeType(&rv, right, 0, VT_I8);
    if(FAILED(rc))
    {
      FIXME("Could not convert right type %d to %d? rc == %#lx.\n", V_VT(right), VT_I8, rc);
      goto end;
    }

    /* if right is zero set VT_EMPTY and return divide by zero */
    if(V_I8(&rv) == 0)
    {
      V_VT(result) = VT_EMPTY;
      rc = DISP_E_DIVBYZERO;
      goto end;
    }

    /* perform the modulo operation */
    V_VT(result) = VT_I8;
    V_I8(result) = V_I8(&lv) % V_I8(&rv);

    TRACE("V_I8(left) == %s, V_I8(right) == %s, V_I8(result) == %s\n",
          wine_dbgstr_longlong(V_I8(&lv)), wine_dbgstr_longlong(V_I8(&rv)),
          wine_dbgstr_longlong(V_I8(result)));

    /* convert left and right to the destination type */
    rc = VariantChangeType(result, result, 0, resT);
    if(FAILED(rc))
    {
      FIXME("Could not convert 0x%x to %d?\n", V_VT(result), resT);
      /* fall to end of function */
    }

end:
    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);
    return rc;
}

/**********************************************************************
 *              VarPow [OLEAUT32.158]
 *
 * Computes the power of one variant to another variant.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarPow(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hr = S_OK;
    VARIANT dl,dr;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT tempLeft, tempRight;

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    VariantInit(&dl);
    VariantInit(&dr);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hr = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hr)) goto end;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hr = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hr)) goto end;
        right = &tempRight;
    }

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hr = DISP_E_BADVARTYPE;
        goto end;
    }
    ExtraFlags = leftExtraFlags;

    /* Native VarPow always returns an error when using extra flags */
    if (ExtraFlags != 0)
    {
        hr = DISP_E_BADVARTYPE;
        goto end;
    }

    /* Determine return type */
    else if (leftvt == VT_NULL || rightvt == VT_NULL) {
        V_VT(result) = VT_NULL;
        hr = S_OK;
        goto end;
    }
    else if ((leftvt == VT_EMPTY || leftvt == VT_I2 ||
        leftvt == VT_I4 || leftvt == VT_R4 ||
        leftvt == VT_R8 || leftvt == VT_CY ||
        leftvt == VT_DATE || leftvt == VT_BSTR ||
        leftvt == VT_BOOL || leftvt == VT_DECIMAL ||
        (leftvt >= VT_I1 && leftvt <= VT_UINT)) &&
        (rightvt == VT_EMPTY || rightvt == VT_I2 ||
        rightvt == VT_I4 || rightvt == VT_R4 ||
        rightvt == VT_R8 || rightvt == VT_CY ||
        rightvt == VT_DATE || rightvt == VT_BSTR ||
        rightvt == VT_BOOL || rightvt == VT_DECIMAL ||
        (rightvt >= VT_I1 && rightvt <= VT_UINT)))
        resvt = VT_R8;
    else
    {
        hr = DISP_E_BADVARTYPE;
        goto end;
    }

    hr = VariantChangeType(&dl,left,0,resvt);
    if (FAILED(hr)) {
        ERR("Could not change passed left argument to VT_R8, handle it differently.\n");
        hr = E_FAIL;
        goto end;
    }

    hr = VariantChangeType(&dr,right,0,resvt);
    if (FAILED(hr)) {
        ERR("Could not change passed right argument to VT_R8, handle it differently.\n");
        hr = E_FAIL;
        goto end;
    }

    V_VT(result) = VT_R8;
    V_R8(result) = pow(V_R8(&dl),V_R8(&dr));

end:
    VariantClear(&dl);
    VariantClear(&dr);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);

    return hr;
}

/**********************************************************************
 *              VarImp [OLEAUT32.154]
 *
 * Bitwise implication of two variants.
 *
 * PARAMS
 *  left    [I] First variant
 *  right   [I] Second variant
 *  result  [O] Result variant
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI VarImp(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hres = S_OK;
    VARTYPE resvt = VT_EMPTY;
    VARTYPE leftvt,rightvt;
    VARTYPE rightExtraFlags,leftExtraFlags,ExtraFlags;
    VARIANT lv,rv;
    double d;
    VARIANT tempLeft, tempRight;

    VariantInit(&lv);
    VariantInit(&rv);
    VariantInit(&tempLeft);
    VariantInit(&tempRight);

    TRACE("(%s,%s,%p)\n", debugstr_variant(left), debugstr_variant(right), result);

    /* Handle VT_DISPATCH by storing and taking address of returned value */
    if ((V_VT(left) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(left, &tempLeft);
        if (FAILED(hres)) goto VarImp_Exit;
        left = &tempLeft;
    }
    if ((V_VT(right) & VT_TYPEMASK) == VT_DISPATCH)
    {
        hres = VARIANT_FetchDispatchValue(right, &tempRight);
        if (FAILED(hres)) goto VarImp_Exit;
        right = &tempRight;
    }

    leftvt = V_VT(left)&VT_TYPEMASK;
    rightvt = V_VT(right)&VT_TYPEMASK;
    leftExtraFlags = V_VT(left)&(~VT_TYPEMASK);
    rightExtraFlags = V_VT(right)&(~VT_TYPEMASK);

    if (leftExtraFlags != rightExtraFlags)
    {
        hres = DISP_E_BADVARTYPE;
        goto VarImp_Exit;
    }
    ExtraFlags = leftExtraFlags;

    /* Native VarImp always returns an error when using extra
     * flags or if the variants are I8 and INT.
     */
    if ((leftvt == VT_I8 && rightvt == VT_INT) ||
        ExtraFlags != 0)
    {
        hres = DISP_E_BADVARTYPE;
        goto VarImp_Exit;
    }

    /* Determine result type */
    else if ((leftvt == VT_NULL && rightvt == VT_NULL) ||
        (leftvt == VT_NULL && rightvt == VT_EMPTY))
    {
        V_VT(result) = VT_NULL;
        hres = S_OK;
        goto VarImp_Exit;
    }
    else if (leftvt == VT_I8 || rightvt == VT_I8)
        resvt = VT_I8;
    else if (leftvt == VT_I4 || rightvt == VT_I4 ||
        leftvt == VT_INT || rightvt == VT_INT ||
        leftvt == VT_UINT || rightvt == VT_UINT ||
        leftvt == VT_UI4 || rightvt == VT_UI4 ||
        leftvt == VT_UI8 || rightvt == VT_UI8 ||
        leftvt == VT_UI2 || rightvt == VT_UI2 ||
        leftvt == VT_DECIMAL || rightvt == VT_DECIMAL ||
        leftvt == VT_DATE || rightvt == VT_DATE ||
        leftvt == VT_CY || rightvt == VT_CY ||
        leftvt == VT_R8 || rightvt == VT_R8 ||
        leftvt == VT_R4 || rightvt == VT_R4 ||
        leftvt == VT_I1 || rightvt == VT_I1)
        resvt = VT_I4;
    else if ((leftvt == VT_UI1 && rightvt == VT_UI1) ||
        (leftvt == VT_UI1 && rightvt == VT_NULL) ||
        (leftvt == VT_NULL && rightvt == VT_UI1))
        resvt = VT_UI1;
    else if (leftvt == VT_EMPTY || rightvt == VT_EMPTY ||
        leftvt == VT_I2 || rightvt == VT_I2 ||
        leftvt == VT_UI1 || rightvt == VT_UI1)
        resvt = VT_I2;
    else if (leftvt == VT_BOOL || rightvt == VT_BOOL ||
        leftvt == VT_BSTR || rightvt == VT_BSTR)
        resvt = VT_BOOL;

    /* VT_NULL requires special handling for when the opposite
     * variant is equal to something other than -1.
     * (NULL Imp 0 = NULL, NULL Imp n = n)
     */
    if (leftvt == VT_NULL)
    {
        VARIANT_BOOL b;
        switch(rightvt)
        {
        case VT_I1:   if (!V_I1(right)) resvt = VT_NULL; break;
        case VT_UI1:  if (!V_UI1(right)) resvt = VT_NULL; break;
        case VT_I2:   if (!V_I2(right)) resvt = VT_NULL; break;
        case VT_UI2:  if (!V_UI2(right)) resvt = VT_NULL; break;
        case VT_I4:   if (!V_I4(right)) resvt = VT_NULL; break;
        case VT_UI4:  if (!V_UI4(right)) resvt = VT_NULL; break;
        case VT_I8:   if (!V_I8(right)) resvt = VT_NULL; break;
        case VT_UI8:  if (!V_UI8(right)) resvt = VT_NULL; break;
        case VT_INT:  if (!V_INT(right)) resvt = VT_NULL; break;
        case VT_UINT: if (!V_UINT(right)) resvt = VT_NULL; break;
        case VT_BOOL: if (!V_BOOL(right)) resvt = VT_NULL; break;
        case VT_R4:   if (!V_R4(right)) resvt = VT_NULL; break;
        case VT_R8:   if (!V_R8(right)) resvt = VT_NULL; break;
        case VT_DATE: if (!V_DATE(right)) resvt = VT_NULL; break;
        case VT_CY:   if (!V_CY(right).int64) resvt = VT_NULL; break;
        case VT_DECIMAL:
            if (!(V_DECIMAL(right).Hi32 || V_DECIMAL(right).Lo64))
                resvt = VT_NULL;
            break;
        case VT_BSTR:
            hres = VarBoolFromStr(V_BSTR(right),LOCALE_USER_DEFAULT, VAR_LOCALBOOL, &b);
            if (FAILED(hres)) goto VarImp_Exit;
            else if (!b)
                V_VT(result) = VT_NULL;
            else
            {
                V_VT(result) = VT_BOOL;
                V_BOOL(result) = b;
            }
            goto VarImp_Exit;
        }
        if (resvt == VT_NULL)
        {
            V_VT(result) = resvt;
            goto VarImp_Exit;
        }
        else
        {
            hres = VariantChangeType(result,right,0,resvt);
            goto VarImp_Exit;
        }
    }

    /* Special handling is required when NULL is the right variant.
     * (-1 Imp NULL = NULL, n Imp NULL = n Imp 0)
     */
    else if (rightvt == VT_NULL)
    {
        VARIANT_BOOL b;
        switch(leftvt)
        {
        case VT_I1:     if (V_I1(left) == -1) resvt = VT_NULL; break;
        case VT_UI1:    if (V_UI1(left) == 0xff) resvt = VT_NULL; break;
        case VT_I2:     if (V_I2(left) == -1) resvt = VT_NULL; break;
        case VT_UI2:    if (V_UI2(left) == 0xffff) resvt = VT_NULL; break;
        case VT_INT:    if (V_INT(left) == -1) resvt = VT_NULL; break;
        case VT_UINT:   if (V_UINT(left) == ~0u) resvt = VT_NULL; break;
        case VT_I4:     if (V_I4(left) == -1) resvt = VT_NULL; break;
        case VT_UI4:    if (V_UI4(left) == ~0u) resvt = VT_NULL; break;
        case VT_I8:     if (V_I8(left) == -1) resvt = VT_NULL; break;
        case VT_UI8:    if (V_UI8(left) == ~(ULONGLONG)0) resvt = VT_NULL; break;
        case VT_BOOL:   if (V_BOOL(left) == VARIANT_TRUE) resvt = VT_NULL; break;
        case VT_R4:     if (V_R4(left) == -1.0) resvt = VT_NULL; break;
        case VT_R8:     if (V_R8(left) == -1.0) resvt = VT_NULL; break;
        case VT_CY:     if (V_CY(left).int64 == -1) resvt = VT_NULL; break;
        case VT_DECIMAL:
            if (V_DECIMAL(left).Hi32 == 0xffffffff)
                resvt = VT_NULL;
            break;
        case VT_BSTR:
            hres = VarBoolFromStr(V_BSTR(left),LOCALE_USER_DEFAULT, VAR_LOCALBOOL, &b);
            if (FAILED(hres)) goto VarImp_Exit;
            else if (b == VARIANT_TRUE)
                resvt = VT_NULL;
        }
        if (resvt == VT_NULL)
        {
            V_VT(result) = resvt;
            goto VarImp_Exit;
        }
    }

    hres = VariantCopy(&lv, left);
    if (FAILED(hres)) goto VarImp_Exit;

    if (rightvt == VT_NULL)
    {
        memset( &rv, 0, sizeof(rv) );
        V_VT(&rv) = resvt;
    }
    else
    {
        hres = VariantCopy(&rv, right);
        if (FAILED(hres)) goto VarImp_Exit;
    }

    if (V_VT(&lv) == VT_BSTR &&
        FAILED(VarR8FromStr(V_BSTR(&lv),LOCALE_USER_DEFAULT, 0, &d)))
        hres = VariantChangeType(&lv,&lv,VARIANT_LOCALBOOL, VT_BOOL);
    if (SUCCEEDED(hres) && V_VT(&lv) != resvt)
        hres = VariantChangeType(&lv,&lv,0,resvt);
    if (FAILED(hres)) goto VarImp_Exit;

    if (V_VT(&rv) == VT_BSTR &&
        FAILED(VarR8FromStr(V_BSTR(&rv),LOCALE_USER_DEFAULT, 0, &d)))
        hres = VariantChangeType(&rv, &rv,VARIANT_LOCALBOOL, VT_BOOL);
    if (SUCCEEDED(hres) && V_VT(&rv) != resvt)
        hres = VariantChangeType(&rv, &rv, 0, resvt);
    if (FAILED(hres)) goto VarImp_Exit;

    /* do the math */
    V_VT(result) = resvt;
    switch (resvt)
    {
    case VT_I8:
    V_I8(result) = (~V_I8(&lv)) | V_I8(&rv);
    break;
    case VT_I4:
    V_I4(result) = (~V_I4(&lv)) | V_I4(&rv);
    break;
    case VT_I2:
    V_I2(result) = (~V_I2(&lv)) | V_I2(&rv);
    break;
    case VT_UI1:
    V_UI1(result) = (~V_UI1(&lv)) | V_UI1(&rv);
    break;
    case VT_BOOL:
    V_BOOL(result) = (~V_BOOL(&lv)) | V_BOOL(&rv);
    break;
    default:
    FIXME("Couldn't perform bitwise implication on variant types %d,%d\n",
        leftvt,rightvt);
    }

VarImp_Exit:

    VariantClear(&lv);
    VariantClear(&rv);
    VariantClear(&tempLeft);
    VariantClear(&tempRight);

    return hres;
}
