/*
 * VARIANT
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2003 Jon Griffiths
 * The alorithm for conversion from Julian days to day/month/year is based on
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "oleauto.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "variant.h"


#ifdef __REACTOS__ /*FIXME*/
#include "ros-mingw-fixes.h"
#endif


WINE_DEFAULT_DEBUG_CHANNEL(ole);

const char* wine_vtypes[VT_CLSID] =
{
  "VT_EMPTY","VT_NULL","VT_I2","VT_I4","VT_R4","VT_R8","VT_CY","VT_DATE",
  "VT_BSTR","VT_DISPATCH","VT_ERROR","VT_BOOL","VT_VARIANT","VT_UNKNOWN",
  "VT_DECIMAL","15","VT_I1","VT_UI1","VT_UI2","VT_UI4","VT_I8","VT_UI8",
  "VT_INT","VT_UINT","VT_VOID","VT_HRESULT","VT_PTR","VT_SAFEARRAY",
  "VT_CARRAY","VT_USERDEFINED","VT_LPSTR","VT_LPWSTR""32","33","34","35",
  "VT_RECORD","VT_INT_PTR","VT_UINT_PTR","39","40","41","42","43","44","45",
  "46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
  "61","62","63","VT_FILETIME","VT_BLOB","VT_STREAM","VT_STORAGE",
  "VT_STREAMED_OBJECT","VT_STORED_OBJECT","VT_BLOB_OBJECT","VT_CF","VT_CLSID"
};

const char* wine_vflags[16] =
{
 "",
 "|VT_VECTOR",
 "|VT_ARRAY",
 "|VT_VECTOR|VT_ARRAY",
 "|VT_BYREF",
 "|VT_VECTOR|VT_ARRAY",
 "|VT_ARRAY|VT_BYREF",
 "|VT_VECTOR|VT_ARRAY|VT_BYREF",
 "|VT_HARDTYPE",
 "|VT_VECTOR|VT_HARDTYPE",
 "|VT_ARRAY|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_HARDTYPE",
 "|VT_BYREF|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_HARDTYPE",
 "|VT_ARRAY|VT_BYREF|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_BYREF|VT_HARDTYPE",
};

/* Convert a variant from one type to another */
static inline HRESULT VARIANT_Coerce(VARIANTARG* pd, LCID lcid, USHORT wFlags,
                                     VARIANTARG* ps, VARTYPE vt)
{
  HRESULT res = DISP_E_TYPEMISMATCH;
  VARTYPE vtFrom =  V_TYPE(ps);
  DWORD dwFlags = 0;

  TRACE("(%p->(%s%s),0x%08lx,0x%04x,%p->(%s%s),%s%s)\n", pd, debugstr_VT(pd),
        debugstr_VF(pd), lcid, wFlags, ps, debugstr_VT(ps), debugstr_VF(ps),
        debugstr_vt(vt), debugstr_vf(vt));

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
    case VT_UI1:      return VarI1FromUI1(V_UI1(ps), &V_I1(pd));
    case VT_UI2:      return VarI1FromUI2(V_UI2(ps), &V_I1(pd));
    case VT_UI4:      return VarI1FromUI4(V_UI4(ps), &V_I1(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarI1FromI8(V_I8(ps), &V_I1(pd));
    case VT_UI8:      return VarI1FromUI8(V_UI8(ps), &V_I1(pd));
#endif
    case VT_R4:       return VarI1FromR4(V_R4(ps), &V_I1(pd));
    case VT_R8:       return VarI1FromR8(V_R8(ps), &V_I1(pd));
    case VT_DATE:     return VarI1FromDate(V_DATE(ps), &V_I1(pd));
    case VT_BOOL:     return VarI1FromBool(V_BOOL(ps), &V_I1(pd));
    case VT_CY:       return VarI1FromCy(V_CY(ps), &V_I1(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarI1FromDec(&V_DECIMAL(ps), &V_I1(pd) );
#endif
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
    case VT_UI2:      return VarI2FromUI2(V_UI2(ps), &V_I2(pd));
    case VT_UI4:      return VarI2FromUI4(V_UI4(ps), &V_I2(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarI2FromI8(V_I8(ps), &V_I2(pd));
    case VT_UI8:      return VarI2FromUI8(V_UI8(ps), &V_I2(pd));
#endif
    case VT_R4:       return VarI2FromR4(V_R4(ps), &V_I2(pd));
    case VT_R8:       return VarI2FromR8(V_R8(ps), &V_I2(pd));
    case VT_DATE:     return VarI2FromDate(V_DATE(ps), &V_I2(pd));
    case VT_BOOL:     return VarI2FromBool(V_BOOL(ps), &V_I2(pd));
    case VT_CY:       return VarI2FromCy(V_CY(ps), &V_I2(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarI2FromDec(&V_DECIMAL(ps), &V_I2(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarI2FromDisp() in MinGW header */
    case VT_DISPATCH: return VarI2FromDisp(V_DISPATCH(ps), lcid, &V_I2(pd));
#endif
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
    case VT_UI4:      return VarI4FromUI4(V_UI4(ps), &V_I4(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarI4FromI8(V_I8(ps), &V_I4(pd));
    case VT_UI8:      return VarI4FromUI8(V_UI8(ps), &V_I4(pd));
#endif
    case VT_R4:       return VarI4FromR4(V_R4(ps), &V_I4(pd));
    case VT_R8:       return VarI4FromR8(V_R8(ps), &V_I4(pd));
    case VT_DATE:     return VarI4FromDate(V_DATE(ps), &V_I4(pd));
    case VT_BOOL:     return VarI4FromBool(V_BOOL(ps), &V_I4(pd));
    case VT_CY:       return VarI4FromCy(V_CY(ps), &V_I4(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarI4FromDec(&V_DECIMAL(ps), &V_I4(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarI4FromDisp() in MinGW header */
    case VT_DISPATCH: return VarI4FromDisp(V_DISPATCH(ps), lcid, &V_I4(pd));
#endif
    case VT_BSTR:     return VarI4FromStr(V_BSTR(ps), lcid, dwFlags, &V_I4(pd));
    }
    break;

  case VT_UI1:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI1(pd) = 0; return S_OK;
    case VT_I1:       return VarUI1FromI1(V_I1(ps), &V_UI1(pd));
    case VT_I2:       return VarUI1FromI2(V_I2(ps), &V_UI1(pd));
    case VT_I4:       return VarUI1FromI4(V_I4(ps), &V_UI1(pd));
    case VT_UI2:      return VarUI1FromUI2(V_UI2(ps), &V_UI1(pd));
    case VT_UI4:      return VarUI1FromUI4(V_UI4(ps), &V_UI1(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarUI1FromI8(V_I8(ps), &V_UI1(pd));
    case VT_UI8:      return VarUI1FromUI8(V_UI8(ps), &V_UI1(pd));
#endif
    case VT_R4:       return VarUI1FromR4(V_R4(ps), &V_UI1(pd));
    case VT_R8:       return VarUI1FromR8(V_R8(ps), &V_UI1(pd));
    case VT_DATE:     return VarUI1FromDate(V_DATE(ps), &V_UI1(pd));
    case VT_BOOL:     return VarUI1FromBool(V_BOOL(ps), &V_UI1(pd));
    case VT_CY:       return VarUI1FromCy(V_CY(ps), &V_UI1(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarUI1FromDec(&V_DECIMAL(ps), &V_UI1(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarUI1FromDisp() in MinGW header */
    case VT_DISPATCH: return VarUI1FromDisp(V_DISPATCH(ps), lcid, &V_UI1(pd));
#endif
    case VT_BSTR:     return VarUI1FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI1(pd));
    }
    break;

  case VT_UI2:
    switch (vtFrom)
    {
    case VT_EMPTY:    V_UI2(pd) = 0; return S_OK;
    case VT_I1:       return VarUI2FromI1(V_I1(ps), &V_UI2(pd));
    case VT_I2:       return VarUI2FromI2(V_I2(ps), &V_UI2(pd));
    case VT_I4:       return VarUI2FromI4(V_I4(ps), &V_UI2(pd));
    case VT_UI1:      return VarUI2FromUI1(V_UI1(ps), &V_UI2(pd));
    case VT_UI4:      return VarUI2FromUI4(V_UI4(ps), &V_UI2(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarUI4FromI8(V_I8(ps), &V_UI4(pd));
    case VT_UI8:      return VarUI4FromUI8(V_UI8(ps), &V_UI4(pd));
#endif
    case VT_R4:       return VarUI2FromR4(V_R4(ps), &V_UI2(pd));
    case VT_R8:       return VarUI2FromR8(V_R8(ps), &V_UI2(pd));
    case VT_DATE:     return VarUI2FromDate(V_DATE(ps), &V_UI2(pd));
    case VT_BOOL:     return VarUI2FromBool(V_BOOL(ps), &V_UI2(pd));
    case VT_CY:       return VarUI2FromCy(V_CY(ps), &V_UI2(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarUI2FromDec(&V_DECIMAL(ps), &V_UI2(pd));
#endif
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
    case VT_I4:       return VarUI4FromI4(V_I4(ps), &V_UI4(pd));
    case VT_UI1:      return VarUI4FromUI1(V_UI1(ps), &V_UI4(pd));
    case VT_UI2:      return VarUI4FromUI2(V_UI2(ps), &V_UI4(pd));
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarUI4FromI8(V_I8(ps), &V_UI4(pd));
    case VT_UI8:      return VarUI4FromUI8(V_UI8(ps), &V_UI4(pd));
#endif
    case VT_R4:       return VarUI4FromR4(V_R4(ps), &V_UI4(pd));
    case VT_R8:       return VarUI4FromR8(V_R8(ps), &V_UI4(pd));
    case VT_DATE:     return VarUI4FromDate(V_DATE(ps), &V_UI4(pd));
    case VT_BOOL:     return VarUI4FromBool(V_BOOL(ps), &V_UI4(pd));
    case VT_CY:       return VarUI4FromCy(V_CY(ps), &V_UI4(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarUI4FromDec(&V_DECIMAL(ps), &V_UI4(pd));
#endif
    case VT_DISPATCH: return VarUI4FromDisp(V_DISPATCH(ps), lcid, &V_UI4(pd));
    case VT_BSTR:     return VarUI4FromStr(V_BSTR(ps), lcid, dwFlags, &V_UI4(pd));
    }
    break;

#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
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
    case VT_I8:       return VarUI8FromI8(V_I8(ps), &V_UI8(pd));
    case VT_R4:       return VarUI8FromR4(V_R4(ps), &V_UI8(pd));
    case VT_R8:       return VarUI8FromR8(V_R8(ps), &V_UI8(pd));
    case VT_DATE:     return VarUI8FromDate(V_DATE(ps), &V_UI8(pd));
    case VT_BOOL:     return VarUI8FromBool(V_BOOL(ps), &V_UI8(pd));
    case VT_CY:       return VarUI8FromCy(V_CY(ps), &V_UI8(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarUI8FromDec(&V_DECIMAL(ps), &V_UI8(pd));
#endif
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
    case VT_UI8:      return VarI8FromUI8(V_I8(ps), &V_I8(pd));
    case VT_R4:       return VarI8FromR4(V_R4(ps), &V_I8(pd));
    case VT_R8:       return VarI8FromR8(V_R8(ps), &V_I8(pd));
    case VT_DATE:     return VarI8FromDate(V_DATE(ps), &V_I8(pd));
    case VT_BOOL:     return VarI8FromBool(V_BOOL(ps), &V_I8(pd));
    case VT_CY:       return VarI8FromCy(V_CY(ps), &V_I8(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarI8FromDec(&V_DECIMAL(ps), &V_I8(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarI8FromDisp() in MinGW header */
    case VT_DISPATCH: return VarI8FromDisp(V_DISPATCH(ps), lcid, &V_I8(pd));
#endif
    case VT_BSTR:     return VarI8FromStr(V_BSTR(ps), lcid, dwFlags, &V_I8(pd));
    }
    break;
#endif

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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarR4FromI8(V_I8(ps), &V_R4(pd));
    case VT_UI8:      return VarR4FromUI8(V_UI8(ps), &V_R4(pd));
#endif
    case VT_R8:       return VarR4FromR8(V_R8(ps), &V_R4(pd));
    case VT_DATE:     return VarR4FromDate(V_DATE(ps), &V_R4(pd));
    case VT_BOOL:     return VarR4FromBool(V_BOOL(ps), &V_R4(pd));
    case VT_CY:       return VarR4FromCy(V_CY(ps), &V_R4(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarR4FromDec(&V_DECIMAL(ps), &V_R4(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarR4FromDisp() in MinGW header */
    case VT_DISPATCH: return VarR4FromDisp(V_DISPATCH(ps), lcid, &V_R4(pd));
#endif
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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarR8FromI8(V_I8(ps), &V_R8(pd));
    case VT_UI8:      return VarR8FromUI8(V_UI8(ps), &V_R8(pd));
#endif
    case VT_R4:       return VarR8FromR4(V_R4(ps), &V_R8(pd));
    case VT_DATE:     return VarR8FromDate(V_DATE(ps), &V_R8(pd));
    case VT_BOOL:     return VarR8FromBool(V_BOOL(ps), &V_R8(pd));
    case VT_CY:       return VarR8FromCy(V_CY(ps), &V_R8(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarR8FromDec(&V_DECIMAL(ps), &V_R8(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarR8FromDisp() in MinGW header */
    case VT_DISPATCH: return VarR8FromDisp(V_DISPATCH(ps), lcid, &V_R8(pd));
#endif
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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarDateFromI8(V_I8(ps), &V_DATE(pd));
    case VT_UI8:      return VarDateFromUI8(V_UI8(ps), &V_DATE(pd));
#endif
    case VT_R4:       return VarDateFromR4(V_R4(ps), &V_DATE(pd));
    case VT_R8:       return VarDateFromR8(V_R8(ps), &V_DATE(pd));
    case VT_BOOL:     return VarDateFromBool(V_BOOL(ps), &V_DATE(pd));
    case VT_CY:       return VarDateFromCy(V_CY(ps), &V_DATE(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarDateFromDec(&V_DECIMAL(ps), &V_DATE(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarDateFromDisp() in MinGW header */
    case VT_DISPATCH: return VarDateFromDisp(V_DISPATCH(ps), lcid, &V_DATE(pd));
#endif
#ifndef __REACTOS__	/*FIXME: no date funtions yet */
    case VT_BSTR:     return VarDateFromStr(V_BSTR(ps), lcid, dwFlags, &V_DATE(pd));
#endif
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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarBoolFromI8(V_I8(ps), &V_BOOL(pd));
    case VT_UI8:      return VarBoolFromUI8(V_UI8(ps), &V_BOOL(pd));
#endif
    case VT_R4:       return VarBoolFromR4(V_R4(ps), &V_BOOL(pd));
    case VT_R8:       return VarBoolFromR8(V_R8(ps), &V_BOOL(pd));
    case VT_DATE:     return VarBoolFromDate(V_DATE(ps), &V_BOOL(pd));
    case VT_CY:       return VarBoolFromCy(V_CY(ps), &V_BOOL(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarBoolFromDec(&V_DECIMAL(ps), &V_BOOL(pd));
#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarBoolFromDisp() in MinGW header */
    case VT_DISPATCH: return VarBoolFromDisp(V_DISPATCH(ps), lcid, &V_BOOL(pd));
#endif
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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarBstrFromI8(V_I8(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_UI8:      return VarBstrFromUI8(V_UI8(ps), lcid, dwFlags, &V_BSTR(pd));
#endif
    case VT_R4:       return VarBstrFromR4(V_R4(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_R8:       return VarBstrFromR8(V_R8(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_DATE:     return VarBstrFromDate(V_DATE(ps), lcid, dwFlags, &V_BSTR(pd));
    case VT_CY:       return VarBstrFromCy(V_CY(ps), lcid, dwFlags, &V_BSTR(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarBstrFromDec(&V_DECIMAL(ps), lcid, dwFlags, &V_BSTR(pd));
#endif
/*  case VT_DISPATCH: return VarBstrFromDisp(V_DISPATCH(ps), lcid, dwFlags, &V_BSTR(pd)); */
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
#ifndef __REACTOS__	/*FIXME: hVal and ullVal missing in VARIANT union of MinGW header of MinGW header */
    case VT_I8:       return VarCyFromI8(V_I8(ps), &V_CY(pd));
    case VT_UI8:      return VarCyFromUI8(V_UI8(ps), &V_CY(pd));
#endif
    case VT_R4:       return VarCyFromR4(V_R4(ps), &V_CY(pd));
    case VT_R8:       return VarCyFromR8(V_R8(ps), &V_CY(pd));
    case VT_DATE:     return VarCyFromDate(V_DATE(ps), &V_CY(pd));
    case VT_BOOL:     return VarCyFromBool(V_BOOL(ps), &V_CY(pd));
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:  return VarCyFromDec(&V_DECIMAL(ps), &V_CY(pd));

#endif
#ifndef __REACTOS__	/*FIXME: wrong declaration of VarCyFromDisp() in MinGW header */
    case VT_DISPATCH: return VarCyFromDisp(V_DISPATCH(ps), lcid, &V_CY(pd));
#endif
    case VT_BSTR:     return VarCyFromStr(V_BSTR(ps), lcid, dwFlags, &V_CY(pd));
    }
    break;

  case VT_DECIMAL:
    switch (vtFrom)
    {
    case VT_EMPTY:
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_MID32 */
    case VT_BOOL:
       DEC_SIGNSCALE(&V_DECIMAL(pd)) = SIGNSCALE(DECIMAL_POS,0);
       DEC_HI32(&V_DECIMAL(pd)) = 0;
       DEC_MID32(&V_DECIMAL(pd)) = 0;
        /* VarDecFromBool() coerces to -1/0, ChangeTypeEx() coerces to 1/0.
         * VT_NULL and VT_EMPTY always give a 0 value.
         */
       DEC_LO32(&V_DECIMAL(pd)) = vtFrom == VT_BOOL && V_BOOL(ps) ? 1 : 0;
       return S_OK;
    case VT_I1:       return VarDecFromI1(V_I1(ps), &V_DECIMAL(pd));
    case VT_I2:       return VarDecFromI2(V_I2(ps), &V_DECIMAL(pd));
    case VT_I4:       return VarDecFromI4(V_I4(ps), &V_DECIMAL(pd));
    case VT_UI1:      return VarDecFromUI1(V_UI1(ps), &V_DECIMAL(pd));
    case VT_UI2:      return VarDecFromUI2(V_UI2(ps), &V_DECIMAL(pd));
    case VT_UI4:      return VarDecFromUI4(V_UI4(ps), &V_DECIMAL(pd));
#ifndef __REACTOS__	/*FIXME: hVal missing in VARIANT union of MinGW header */
    case VT_I8:       return VarDecFromI8(V_I8(ps), &V_DECIMAL(pd));
    case VT_UI8:      return VarDecFromUI8(V_UI8(ps), &V_DECIMAL(pd));
#endif
    case VT_R4:       return VarDecFromR4(V_R4(ps), &V_DECIMAL(pd));
    case VT_R8:       return VarDecFromR8(V_R8(ps), &V_DECIMAL(pd));
    case VT_DATE:     return VarDecFromDate(V_DATE(ps), &V_DECIMAL(pd));
    case VT_CY:       return VarDecFromCy(V_CY(pd), &V_DECIMAL(ps));
    case VT_DISPATCH: return VarDecFromDisp(V_DISPATCH(ps), lcid, &V_DECIMAL(ps));
    case VT_BSTR:     return VarDecFromStr(V_BSTR(ps), lcid, dwFlags, &V_DECIMAL(pd));
#endif
    }
    break;

  case VT_UNKNOWN:
    switch (vtFrom)
    {
    case VT_DISPATCH:
      if (V_DISPATCH(ps) == NULL)
        V_UNKNOWN(pd) = NULL;
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
        V_DISPATCH(pd) = NULL;
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

#ifndef __REACTOS__	/*FIXME: no safearray yet */
/* Coerce to/from an array */
static inline HRESULT VARIANT_CoerceArray(VARIANTARG* pd, VARIANTARG* ps, VARTYPE vt)
{
  if (vt == VT_BSTR && V_VT(ps) == (VT_ARRAY|VT_UI1))
    return BstrFromVector(V_ARRAY(ps), &V_BSTR(pd));

  if (V_VT(ps) == VT_BSTR && vt == (VT_ARRAY|VT_UI1))
    return VectorFromBstr(V_BSTR(ps), &V_ARRAY(ps));

  if (V_VT(ps) == vt)
    return SafeArrayCopy(V_ARRAY(ps), &V_ARRAY(pd));

  return DISP_E_TYPEMISMATCH;
}
#endif

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
 * Initialise a variant.
 *
 * PARAMS
 *  pVarg [O] Variant to initialise
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  This function simply sets the type of the variant to VT_EMPTY. It does not
 *  free any existing value, use VariantClear() for that.
 */
void WINAPI VariantInit(VARIANTARG* pVarg)
{
  TRACE("(%p)\n", pVarg);

  V_VT(pVarg) = VT_EMPTY; /* Native doesn't set any other fields */
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
 *  Failure: DISP_E_BADVARTYPE, if the variant is a not a valid variant type.
 */
HRESULT WINAPI VariantClear(VARIANTARG* pVarg)
{
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s))\n", pVarg, debugstr_VT(pVarg), debugstr_VF(pVarg));

  hres = VARIANT_ValidateType(V_VT(pVarg));

  if (SUCCEEDED(hres))
  {
    if (!V_ISBYREF(pVarg))
    {
#ifndef __REACTOS__	/*FIXME: no safearray yet */
      if (V_ISARRAY(pVarg) || V_VT(pVarg) == VT_SAFEARRAY)
      {
        if (V_ARRAY(pVarg))
          hres = SafeArrayDestroy(V_ARRAY(pVarg));
      }
	  else
#endif
      if (V_VT(pVarg) == VT_BSTR)
      {
        if (V_BSTR(pVarg))
          SysFreeString(V_BSTR(pVarg));
      }
#ifndef __REACTOS__	/*FIXME: problems with MinGW and brecVal */
      else if (V_VT(pVarg) == VT_RECORD)
      {
        struct __tagBRECORD* pBr = &V_UNION(pVarg,brecVal);
        if (pBr->pRecInfo)
        {
          IRecordInfo_RecordClear(pBr->pRecInfo, pBr->pvRecord);
          IRecordInfo_Release(pBr->pRecInfo);
        }
      }
#endif
      else if (V_VT(pVarg) == VT_DISPATCH ||
               V_VT(pVarg) == VT_UNKNOWN)
      {
        if (V_UNKNOWN(pVarg))
          IUnknown_Release(V_UNKNOWN(pVarg));
      }
      else if (V_VT(pVarg) == VT_VARIANT)
      {
        if (V_VARIANTREF(pVarg))
          VariantClear(V_VARIANTREF(pVarg));
      }
    }
    V_VT(pVarg) = VT_EMPTY;
  }
  return hres;
}

#ifndef __REACTOS__	/*FIXME: missing __tagBRECORD in MinGW */
/******************************************************************************
 * Copy an IRecordInfo object contained in a variant.
 */
static HRESULT VARIANT_CopyIRecordInfo(struct __tagBRECORD* pBr)
{
  HRESULT hres = S_OK;

  if (pBr->pRecInfo)
  {
    ULONG ulSize;

    hres = IRecordInfo_GetSize(pBr->pRecInfo, &ulSize);
    if (SUCCEEDED(hres))
    {
      PVOID pvRecord = HeapAlloc(GetProcessHeap(), 0, ulSize);
      if (!pvRecord)
        hres = E_OUTOFMEMORY;
      else
      {
        memcpy(pvRecord, pBr->pvRecord, ulSize);
        pBr->pvRecord = pvRecord;

        hres = IRecordInfo_RecordCopy(pBr->pRecInfo, pvRecord, pvRecord);
        if (SUCCEEDED(hres))
          IRecordInfo_AddRef(pBr->pRecInfo);
      }
    }
  }
  else if (pBr->pvRecord)
    hres = E_INVALIDARG;
  return hres;
}
#endif

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
HRESULT WINAPI VariantCopy(VARIANTARG* pvargDest, VARIANTARG* pvargSrc)
{
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s),%p->(%s%s))\n", pvargDest, debugstr_VT(pvargDest),
        debugstr_VF(pvargDest), pvargSrc, debugstr_VT(pvargSrc),
        debugstr_VF(pvargSrc));

  if (V_TYPE(pvargSrc) == VT_CLSID || /* VT_CLSID is a special case */
      FAILED(VARIANT_ValidateType(V_VT(pvargSrc))))
    return DISP_E_BADVARTYPE;

  if (pvargSrc != pvargDest &&
      SUCCEEDED(hres = VariantClear(pvargDest)))
  {
    *pvargDest = *pvargSrc; /* Shallow copy the value */

    if (!V_ISBYREF(pvargSrc))
    {
#ifndef __REACTOS__	/*FIXME: no safearray yet */
      if (V_ISARRAY(pvargSrc))
      {
        if (V_ARRAY(pvargSrc))
          hres = SafeArrayCopy(V_ARRAY(pvargSrc), &V_ARRAY(pvargDest));
      }
	  else
#endif
      if (V_VT(pvargSrc) == VT_BSTR)
      {
        if (V_BSTR(pvargSrc))
        {
          V_BSTR(pvargDest) = SysAllocStringByteLen((char*)V_BSTR(pvargSrc), SysStringByteLen(V_BSTR(pvargSrc)));
          if (!V_BSTR(pvargDest))
	  {
	    TRACE("!V_BSTR(pvargDest), SysAllocStringByteLen() failed to allocate %d bytes\n", SysStringByteLen(V_BSTR(pvargSrc)));
            hres = E_OUTOFMEMORY;
	  }
        }
      }
#ifndef __REACTOS__	/*FIXME: missing __tagBRECORD in MinGW */
      else if (V_VT(pvargSrc) == VT_RECORD)
      {
        hres = VARIANT_CopyIRecordInfo(&V_UNION(pvargDest,brecVal));
      }
#endif
      else if (V_VT(pvargSrc) == VT_DISPATCH ||
               V_VT(pvargSrc) == VT_UNKNOWN)
      {
        if (V_UNKNOWN(pvargSrc))
          IUnknown_AddRef(V_UNKNOWN(pvargSrc));
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
  case VT_UI1:   return sizeof(BYTE); break;
  case VT_I2:
  case VT_UI2:   return sizeof(SHORT); break;
  case VT_INT:
  case VT_UINT:
  case VT_I4:
  case VT_UI4:   return sizeof(LONG); break;
  case VT_I8:
  case VT_UI8:   return sizeof(LONGLONG); break;
  case VT_R4:    return sizeof(float); break;
  case VT_R8:    return sizeof(double); break;
  case VT_DATE:  return sizeof(DATE); break;
  case VT_BOOL:  return sizeof(VARIANT_BOOL); break;
  case VT_DISPATCH:
  case VT_UNKNOWN:
  case VT_BSTR:  return sizeof(void*); break;
  case VT_CY:    return sizeof(CY); break;
  case VT_ERROR: return sizeof(SCODE); break;
  }
  TRACE("Shouldn't be called for vt %s%s!\n", debugstr_VT(pv), debugstr_VF(pv));
  return 0;
}

/******************************************************************************
 *    VariantCopyInd  [OLEAUT32.11]
 *
 * Copy a variant, dereferencing it it is by-reference.
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
HRESULT WINAPI VariantCopyInd(VARIANT* pvargDest, VARIANTARG* pvargSrc)
{
  VARIANTARG vTmp, *pSrc = pvargSrc;
  VARTYPE vt;
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s),%p->(%s%s))\n", pvargDest, debugstr_VT(pvargDest),
        debugstr_VF(pvargDest), pvargSrc, debugstr_VT(pvargSrc),
        debugstr_VF(pvargSrc));

  if (!V_ISBYREF(pvargSrc))
    return VariantCopy(pvargDest, pvargSrc);

  /* Argument checking is more lax than VariantCopy()... */
  vt = V_TYPE(pvargSrc);
  if (V_ISARRAY(pvargSrc) ||
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

#ifndef __REACTOS__	/*FIXME: no safearray yet */
  if (V_ISARRAY(pSrc))
  {
    /* Native doesn't check that *V_ARRAYREF(pSrc) is valid */
    hres = SafeArrayCopy(*V_ARRAYREF(pSrc), &V_ARRAY(pvargDest));
  }
  else
#endif
  if (V_VT(pSrc) == (VT_BSTR|VT_BYREF))
  {
    /* Native doesn't check that *V_BSTRREF(pSrc) is valid */
    V_BSTR(pvargDest) = SysAllocStringByteLen((char*)*V_BSTRREF(pSrc), SysStringByteLen(*V_BSTRREF(pSrc)));
  }
#ifndef __REACTOS__	/*FIXME: missing __tagBRECORD in MinGW */
  else if (V_VT(pSrc) == (VT_RECORD|VT_BYREF))
  {
    V_UNION(pvargDest,brecVal) = V_UNION(pvargSrc,brecVal);
    hres = VARIANT_CopyIRecordInfo(&V_UNION(pvargDest,brecVal));
  }
#endif
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
    memcpy(&DEC_SCALE(&V_DECIMAL(pvargDest)), &DEC_SCALE(V_DECIMALREF(pSrc)),
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
    VariantClear(pSrc);

  TRACE("returning 0x%08lx, %p->(%s%s)\n", hres, pvargDest,
        debugstr_VT(pvargDest), debugstr_VF(pvargDest));
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
HRESULT WINAPI VariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
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
HRESULT WINAPI VariantChangeTypeEx(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
                                   LCID lcid, USHORT wFlags, VARTYPE vt)
{
  HRESULT res = S_OK;

  TRACE("(%p->(%s%s),%p->(%s%s),0x%08lx,0x%04x,%s%s)\n", pvargDest,
        debugstr_VT(pvargDest), debugstr_VF(pvargDest), pvargSrc,
        debugstr_VT(pvargSrc), debugstr_VF(pvargSrc), lcid, wFlags,
        debugstr_vt(vt), debugstr_vf(vt));

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
        VARIANTARG vTmp;

        V_VT(&vTmp) = VT_EMPTY;
        res = VariantCopyInd(&vTmp, pvargSrc);

        if (SUCCEEDED(res))
        {
          res = VariantClear(pvargDest);

          if (SUCCEEDED(res))
          {
#ifndef __REACTOS__	/*FIXME: no safearray yet */
            if (V_ISARRAY(&vTmp) || (vt & VT_ARRAY))
              res = VARIANT_CoerceArray(pvargDest, &vTmp, vt);
            else
#endif
              res = VARIANT_Coerce(pvargDest, lcid, wFlags, &vTmp, vt);

            if (SUCCEEDED(res))
              V_VT(pvargDest) = vt;
          }
          VariantClear(&vTmp);
        }
      }
    }
  }

  TRACE("returning 0x%08lx, %p->(%s%s)\n", res, pvargDest,
        debugstr_VT(pvargDest), debugstr_VF(pvargDest));
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

#ifndef __REACTOS__	/*FIXME: disabled for now */

/* Roll a date forwards or backwards to correct it */
static HRESULT VARIANT_RollUdate(UDATE *lpUd)
{
  static const BYTE days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  TRACE("Raw date: %d/%d/%d %d:%d:%d\n", lpUd->st.wDay, lpUd->st.wMonth,
        lpUd->st.wYear, lpUd->st.wHour, lpUd->st.wMinute, lpUd->st.wSecond);

  /* Years < 100 are treated as 1900 + year */
  if (lpUd->st.wYear < 100)
    lpUd->st.wYear += 1900;

  if (!lpUd->st.wMonth)
  {
    /* Roll back to December of the previous year */
    lpUd->st.wMonth = 12;
    lpUd->st.wYear--;
  }
  else while (lpUd->st.wMonth > 12)
  {
    /* Roll forward the correct number of months */
    lpUd->st.wYear++;
    lpUd->st.wMonth -= 12;
  }

  if (lpUd->st.wYear > 9999 || lpUd->st.wHour > 23 ||
      lpUd->st.wMinute > 59 || lpUd->st.wSecond > 59)
    return E_INVALIDARG; /* Invalid values */

  if (!lpUd->st.wDay)
  {
    /* Roll back the date one day */
    if (lpUd->st.wMonth == 1)
    {
      /* Roll back to December 31 of the previous year */
      lpUd->st.wDay   = 31;
      lpUd->st.wMonth = 12;
      lpUd->st.wYear--;
    }
    else
    {
      lpUd->st.wMonth--; /* Previous month */
      if (lpUd->st.wMonth == 2 && IsLeapYear(lpUd->st.wYear))
        lpUd->st.wDay = 29; /* Februaury has 29 days on leap years */
      else
        lpUd->st.wDay = days[lpUd->st.wMonth]; /* Last day of the month */
    }
  }
  else if (lpUd->st.wDay > 28)
  {
    int rollForward = 0;

    /* Possibly need to roll the date forward */
    if (lpUd->st.wMonth == 2 && IsLeapYear(lpUd->st.wYear))
      rollForward = lpUd->st.wDay - 29; /* Februaury has 29 days on leap years */
    else
      rollForward = lpUd->st.wDay - days[lpUd->st.wMonth];

    if (rollForward > 0)
    {
      lpUd->st.wDay = rollForward;
      lpUd->st.wMonth++;
      if (lpUd->st.wMonth > 12)
      {
        lpUd->st.wMonth = 1; /* Roll forward into January of the next year */
        lpUd->st.wYear++;
      }
    }
  }
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

  return !VarDateFromUdate(&ud, 0, pDateOut);
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

  memcpy(&ud.st, lpSt, sizeof(ud.st));
  return !VarDateFromUdate(&ud, 0, pDateOut);
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

  memcpy(lpSt, &ud.st, sizeof(ud.st));
  return TRUE;
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
 */
HRESULT WINAPI VarDateFromUdate(UDATE *pUdateIn, ULONG dwFlags, DATE *pDateOut)
{
  UDATE ud;
  double dateVal;

  TRACE("(%p->%d/%d/%d %d:%d:%d:%d %d %d,0x%08lx,%p)\n", pUdateIn,
        pUdateIn->st.wMonth, pUdateIn->st.wDay, pUdateIn->st.wYear,
        pUdateIn->st.wHour, pUdateIn->st.wMinute, pUdateIn->st.wSecond,
        pUdateIn->st.wMilliseconds, pUdateIn->st.wDayOfWeek,
        pUdateIn->wDayOfYear, dwFlags, pDateOut);

  memcpy(&ud, pUdateIn, sizeof(ud));

  if (dwFlags & VAR_VALIDDATE)
    WARN("Ignoring VAR_VALIDDATE\n");

  if (FAILED(VARIANT_RollUdate(&ud)))
    return E_INVALIDARG;

  /* Date */
  dateVal = VARIANT_DateFromJulian(VARIANT_JulianFromDMY(ud.st.wYear, ud.st.wMonth, ud.st.wDay));

  /* Time */
  dateVal += ud.st.wHour / 24.0;
  dateVal += ud.st.wMinute / 1440.0;
  dateVal += ud.st.wSecond / 86400.0;
  dateVal += ud.st.wMilliseconds / 86400000.0;

  TRACE("Returning %g\n", dateVal);
  *pDateOut = dateVal;
  return S_OK;
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

  TRACE("(%g,0x%08lx,%p)\n", dateIn, dwFlags, lpUdate);

  if (dateIn <= (DATE_MIN - 1.0) || dateIn >= (DATE_MAX + 1.0))
    return E_INVALIDARG;

  datePart = dateIn < 0.0 ? ceil(dateIn) : floor(dateIn);
  /* Compensate for int truncation (always downwards) */
  timePart = dateIn - datePart + 0.00000000001;
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

#define GET_NUMBER_TEXT(fld,name) \
  buff[0] = 0; \
  if (!GetLocaleInfoW(lcid, lctype|fld, buff, sizeof(WCHAR) * 2)) \
    WARN("buffer too small for " #fld "\n"); \
  else \
    if (buff[0]) lpChars->name = buff[0]; \
  TRACE("lcid 0x%lx, " #name "=%d '%c'\n", lcid, lpChars->name, lpChars->name)

/* Get the valid number characters for an lcid */
void VARIANT_GetLocalisedNumberChars(VARIANT_NUMBER_CHARS *lpChars, LCID lcid, DWORD dwFlags)
{
  static const VARIANT_NUMBER_CHARS defaultChars = { '-','+','.',',','$',0,'.',',' };
  LCTYPE lctype = dwFlags & LOCALE_NOUSEROVERRIDE;
  WCHAR buff[4];

  memcpy(lpChars, &defaultChars, sizeof(defaultChars));
  GET_NUMBER_TEXT(LOCALE_SNEGATIVESIGN, cNegativeSymbol);
  GET_NUMBER_TEXT(LOCALE_SPOSITIVESIGN, cPositiveSymbol);
  GET_NUMBER_TEXT(LOCALE_SDECIMAL, cDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_STHOUSAND, cDigitSeperator);
  GET_NUMBER_TEXT(LOCALE_SMONDECIMALSEP, cCurrencyDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_SMONTHOUSANDSEP, cCurrencyDigitSeperator);

  /* Local currency symbols are often 2 characters */
  lpChars->cCurrencyLocal2 = '\0';
  switch(GetLocaleInfoW(lcid, lctype|LOCALE_SCURRENCY, buff, sizeof(WCHAR) * 4))
  {
    case 3: lpChars->cCurrencyLocal2 = buff[1]; /* Fall through */
    case 2: lpChars->cCurrencyLocal  = buff[0];
            break;
    default: WARN("buffer too small for LOCALE_SCURRENCY\n");
  }
  TRACE("lcid 0x%lx, cCurrencyLocal =%d,%d '%c','%c'\n", lcid, lpChars->cCurrencyLocal,
        lpChars->cCurrencyLocal2, lpChars->cCurrencyLocal, lpChars->cCurrencyLocal2);
}

/* Number Parsing States */
#define B_PROCESSING_EXPONENT 0x1
#define B_NEGATIVE_EXPONENT   0x2
#define B_EXPONENT_START      0x4
#define B_INEXACT_ZEROS       0x8
#define B_LEADING_ZERO        0x10

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
 *  - I am unsure if this function should parse non-arabic (e.g. Thai)
 *   numerals, so this has not been implemented.
 */
HRESULT WINAPI VarParseNumFromStr(OLECHAR *lpszStr, LCID lcid, ULONG dwFlags,
                                  NUMPARSE *pNumprs, BYTE *rgbDig)
{
  VARIANT_NUMBER_CHARS chars;
  BYTE rgbTmp[1024];
  DWORD dwState = B_EXPONENT_START|B_INEXACT_ZEROS;
  int iMaxDigits = sizeof(rgbTmp) / sizeof(BYTE);
  int cchUsed = 0;

  TRACE("(%s,%ld,0x%08lx,%p,%p)\n", debugstr_w(lpszStr), lcid, dwFlags, pNumprs, rgbDig);

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

  /* First consume all the leading symbols and space from the string */
  while (1)
  {
    if (pNumprs->dwInFlags & NUMPRS_LEADING_WHITE && isspaceW(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_LEADING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (isspaceW(*lpszStr));
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
             *lpszStr == chars.cCurrencyLocal &&
             (!chars.cCurrencyLocal2 || lpszStr[1] == chars.cCurrencyLocal2))
    {
      pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed++;
      lpszStr++;
      /* Only accept currency characters */
      chars.cDecimalPoint = chars.cCurrencyDecimalPoint;
      chars.cDigitSeperator = chars.cCurrencyDigitSeperator;
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

  if (!(pNumprs->dwOutFlags & NUMPRS_CURRENCY))
  {
    /* Only accept non-currency characters */
    chars.cCurrencyDecimalPoint = chars.cDecimalPoint;
    chars.cCurrencyDigitSeperator = chars.cDigitSeperator;
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
    if (isdigitW(*lpszStr))
    {
      if (dwState & B_PROCESSING_EXPONENT)
      {
        int exponentSize = 0;
        if (dwState & B_EXPONENT_START)
        {
          while (*lpszStr == '0')
          {
            /* Skip leading zero's in the exponent */
            cchUsed++;
            lpszStr++;
          }
          if (!isdigitW(*lpszStr))
            break; /* No exponent digits - invalid */
        }

        while (isdigitW(*lpszStr))
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
        if (pNumprs->cDig >= iMaxDigits)
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
          if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
            pNumprs->nPwr10--; /* Count decimal points in nPwr10 */
          rgbTmp[pNumprs->cDig] = *lpszStr - '0';
        }
        pNumprs->cDig++;
        cchUsed++;
      }
    }
    else if (*lpszStr == chars.cDigitSeperator && pNumprs->dwInFlags & NUMPRS_THOUSANDS)
    {
      pNumprs->dwOutFlags |= NUMPRS_THOUSANDS;
      cchUsed++;
    }
    else if (*lpszStr == chars.cDecimalPoint &&
             pNumprs->dwInFlags & NUMPRS_DECIMAL &&
             !(pNumprs->dwOutFlags & (NUMPRS_DECIMAL|NUMPRS_EXPONENT)))
    {
      pNumprs->dwOutFlags |= NUMPRS_DECIMAL;
      cchUsed++;

      /* Remove trailing zeros from the whole number part */
      while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
      {
        pNumprs->nPwr10++;
        pNumprs->cDig--;
      }

      /* If we have no digits so far, skip leading zeros */
      if (!pNumprs->cDig)
      {
        while (lpszStr[1] == '0')
        {
          dwState |= B_LEADING_ZERO;
          cchUsed++;
          lpszStr++;
        }
      }
    }
    else if ((*lpszStr == 'e' || *lpszStr == 'E') &&
             pNumprs->dwInFlags & NUMPRS_EXPONENT &&
             !(pNumprs->dwOutFlags & NUMPRS_EXPONENT))
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

    while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
    {
      if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
        pNumprs->nPwr10--;
      else
        pNumprs->nPwr10++;

      pNumprs->cDig--;
    }
  } else
  {
    /* Remove trailing zeros from the last (whole number or decimal) part */
    while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
    {
      if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
        pNumprs->nPwr10--;
      else
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
    if ((pNumprs->dwInFlags & NUMPRS_TRAILING_WHITE) && isspaceW(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_TRAILING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (isspaceW(*lpszStr));
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
 *  - Signed types are preferrred over unsigned types of the same size.
 *  - Preferred types in order are: integer, float, double, currency then decimal.
 *  - Rounding (dropping of decimal points) occurs without error. See VarI8FromR8()
 *    for details of the rounding method.
 *  - pVarDst is not cleared before the result is stored in it.
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

  TRACE("(%p,%p,0x%lx,%p)\n", pNumprs, rgbDig, dwVtBits, pVarDst);

  if (pNumprs->nBaseShift)
  {
    /* nBaseShift indicates a hex or octal number */
    FIXME("nBaseShift=%d not yet implemented, returning overflow\n", pNumprs->nBaseShift);
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

  TRACE("cDig %d; nPwr10 %d, whole %d, frac %d ", pNumprs->cDig,
        pNumprs->nPwr10, wholeNumberDigits, fractionalDigits);
  TRACE("mult %d; div %d\n", multiplier10, divisor10);

  if (dwVtBits & (INTEGER_VTBITS|VTBIT_DECIMAL) &&
      (!fractionalDigits || !(dwVtBits & (REAL_VTBITS|VTBIT_CY|VTBIT_DECIMAL))))
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
      if (ul64 > (UI8_MAX / 10 - rgbDig[i]))
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
    bNegative = pNumprs->dwOutFlags & NUMPRS_NEG && ul64 ? TRUE : FALSE;

    TRACE("Integer value is %lld, bNeg %d\n", ul64, bNegative);

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
#ifndef __REACTOS__	/*FIXME: hVal missing in VARIANT union of MinGW header */
        else if (dwVtBits & VTBIT_I8 && ul64 <= (ULONGLONG)I8_MAX + 1)
        {
          V_VT(pVarDst) = VT_I8;
          V_I8(pVarDst) = -ul64;
          return S_OK;
        }
#endif
        else if ((dwVtBits & REAL_VTBITS) == VTBIT_DECIMAL)
        {
          /* Decimal is only output choice left - fast path */
          V_VT(pVarDst) = VT_DECIMAL;
          DEC_SIGNSCALE(&V_DECIMAL(pVarDst)) = SIGNSCALE(DECIMAL_NEG,0);
          DEC_HI32(&V_DECIMAL(pVarDst)) = 0;
          DEC_LO64(&V_DECIMAL(pVarDst)) = -ul64;
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
#ifndef __REACTOS__	/*FIXME: hVal missing in VARIANT union of MinGW header */
      else if (dwVtBits & VTBIT_I8 && ul64 <= I8_MAX)
      {
        V_VT(pVarDst) = VT_I8;
        V_I8(pVarDst) = ul64;
        return S_OK;
      }
#endif
      else if (dwVtBits & VTBIT_UI8)
      {
        V_VT(pVarDst) = VT_UI8;
        V_UI8(pVarDst) = ul64;
        return S_OK;
      }
      else if ((dwVtBits & REAL_VTBITS) == VTBIT_DECIMAL)
      {
        /* Decimal is only output choice left - fast path */
        V_VT(pVarDst) = VT_DECIMAL;
        DEC_SIGNSCALE(&V_DECIMAL(pVarDst)) = SIGNSCALE(DECIMAL_POS,0);
        DEC_HI32(&V_DECIMAL(pVarDst)) = 0;
        DEC_LO64(&V_DECIMAL(pVarDst)) = ul64;
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
    if (multiplier10)
    {
      if (whole > dblMaximums[multiplier10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY);
        bOverflow = TRUE;
      }
      else
        whole = whole * dblMultipliers[multiplier10];
    }

    TRACE("Scaled double value is %16.16g\n", whole);

    while (divisor10 > 10)
    {
      if (whole < dblMinimums[10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY); /* Underflow */
        bOverflow = TRUE;
        break;
      }
      whole = whole / dblMultipliers[10];
      divisor10 -= 10;
    }
    if (divisor10)
    {
      if (whole < dblMinimums[divisor10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY); /* Underflow */
        bOverflow = TRUE;
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

    DECIMAL_SETZERO(pDec);
    DEC_LO32(pDec) = 0;

    if (pNumprs->dwOutFlags & NUMPRS_NEG)
      DEC_SIGN(pDec) = DECIMAL_NEG;
    else
      DEC_SIGN(pDec) = DECIMAL_POS;

    /* Factor the significant digits */
    for (i = 0; i < pNumprs->cDig; i++)
    {
      tmp = (ULONG64)DEC_LO32(pDec) * 10 + rgbDig[i];
      carry = (ULONG)(tmp >> 32);
      DEC_LO32(pDec) = (ULONG)(tmp & UI4_MAX);
      tmp = (ULONG64)DEC_MID32(pDec) * 10 + carry;
      carry = (ULONG)(tmp >> 32);
      DEC_MID32(pDec) = (ULONG)(tmp & UI4_MAX);
      tmp = (ULONG64)DEC_HI32(pDec) * 10 + carry;
      DEC_HI32(pDec) = (ULONG)(tmp & UI4_MAX);

      if (tmp >> 32 & UI4_MAX)
      {
VarNumFromParseNum_DecOverflow:
        TRACE("Overflow\n");
        DEC_LO32(pDec) = DEC_MID32(pDec) = DEC_HI32(pDec) = UI4_MAX;
        return DISP_E_OVERFLOW;
      }
    }

    /* Account for the scale of the number */
    while (multiplier10 > 0)
    {
      tmp = (ULONG64)DEC_LO32(pDec) * 10;
      carry = (ULONG)(tmp >> 32);
      DEC_LO32(pDec) = (ULONG)(tmp & UI4_MAX);
      tmp = (ULONG64)DEC_MID32(pDec) * 10 + carry;
      carry = (ULONG)(tmp >> 32);
      DEC_MID32(pDec) = (ULONG)(tmp & UI4_MAX);
      tmp = (ULONG64)DEC_HI32(pDec) * 10 + carry;
      DEC_HI32(pDec) = (ULONG)(tmp & UI4_MAX);

      if (tmp >> 32 & UI4_MAX)
        goto VarNumFromParseNum_DecOverflow;
      multiplier10--;
    }
    DEC_SCALE(pDec) = divisor10;

    V_VT(pVarDst) = VT_DECIMAL;
    return S_OK;
  }
  return DISP_E_OVERFLOW; /* No more output choices */
}

/**********************************************************************
 *              VarCat [OLEAUT32.318]
 */
HRESULT WINAPI VarCat(LPVARIANT left, LPVARIANT right, LPVARIANT out)
{
    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), out);

    /* Should we VariantClear out? */
    /* Can we handle array, vector, by ref etc. */
    if ((V_VT(left)&VT_TYPEMASK) == VT_NULL &&
        (V_VT(right)&VT_TYPEMASK) == VT_NULL)
    {
        V_VT(out) = VT_NULL;
        return S_OK;
    }

    if (V_VT(left) == VT_BSTR && V_VT(right) == VT_BSTR)
    {
        V_VT(out) = VT_BSTR;
        VarBstrCat (V_BSTR(left), V_BSTR(right), &V_BSTR(out));
        return S_OK;
    }
    if (V_VT(left) == VT_BSTR) {
        VARIANT bstrvar;
	HRESULT hres;

        V_VT(out) = VT_BSTR;
        hres = VariantChangeTypeEx(&bstrvar,right,0,0,VT_BSTR);
	if (hres) {
	    FIXME("Failed to convert right side from vt %d to VT_BSTR?\n",V_VT(right));
	    return hres;
        }
        VarBstrCat (V_BSTR(left), V_BSTR(&bstrvar), &V_BSTR(out));
        return S_OK;
    }
    if (V_VT(right) == VT_BSTR) {
        VARIANT bstrvar;
	HRESULT hres;

        V_VT(out) = VT_BSTR;
        hres = VariantChangeTypeEx(&bstrvar,left,0,0,VT_BSTR);
	if (hres) {
	    FIXME("Failed to convert right side from vt %d to VT_BSTR?\n",V_VT(right));
	    return hres;
        }
        VarBstrCat (V_BSTR(&bstrvar), V_BSTR(right), &V_BSTR(out));
        return S_OK;
    }
    FIXME ("types %d / %d not supported\n",V_VT(left)&VT_TYPEMASK, V_VT(right)&VT_TYPEMASK);
    return S_OK;
}

/**********************************************************************
 *              VarCmp [OLEAUT32.176]
 *
 * flags can be:
 *   NORM_IGNORECASE, NORM_IGNORENONSPACE, NORM_IGNORESYMBOLS
 *   NORM_IGNOREWIDTH, NORM_IGNOREKANATYPE, NORM_IGNOREKASHIDA
 *
 */
HRESULT WINAPI VarCmp(LPVARIANT left, LPVARIANT right, LCID lcid, DWORD flags)
{
    BOOL	lOk        = TRUE;
    BOOL	rOk        = TRUE;
    LONGLONG	lVal = -1;
    LONGLONG	rVal = -1;
    VARIANT	rv,lv;
    DWORD	xmask;
    HRESULT	rc;

    TRACE("(%p->(%s%s),%p->(%s%s),0x%08lx,0x%08lx)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), lcid, flags);

    VariantInit(&lv);VariantInit(&rv);
    V_VT(right) &= ~0x8000; /* hack since we sometime get this flag.  */
    V_VT(left) &= ~0x8000; /* hack since we sometime get this flag. */

    /* If either are null, then return VARCMP_NULL */
    if ((V_VT(left)&VT_TYPEMASK) == VT_NULL ||
        (V_VT(right)&VT_TYPEMASK) == VT_NULL)
        return VARCMP_NULL;

    /* Strings - use VarBstrCmp */
    if ((V_VT(left)&VT_TYPEMASK) == VT_BSTR &&
        (V_VT(right)&VT_TYPEMASK) == VT_BSTR) {
        return VarBstrCmp(V_BSTR(left), V_BSTR(right), lcid, flags);
    }

    xmask = (1<<(V_VT(left)&VT_TYPEMASK))|(1<<(V_VT(right)&VT_TYPEMASK));
    if (xmask & (1<<VT_R8)) {
	rc = VariantChangeType(&lv,left,0,VT_R8);
	if (FAILED(rc)) return rc;
	rc = VariantChangeType(&rv,right,0,VT_R8);
	if (FAILED(rc)) return rc;

	if (V_R8(&lv) == V_R8(&rv)) return VARCMP_EQ;
	if (V_R8(&lv) < V_R8(&rv)) return VARCMP_LT;
	if (V_R8(&lv) > V_R8(&rv)) return VARCMP_GT;
	return E_FAIL; /* can't get here */
    }
    if (xmask & (1<<VT_R4)) {
	rc = VariantChangeType(&lv,left,0,VT_R4);
	if (FAILED(rc)) return rc;
	rc = VariantChangeType(&rv,right,0,VT_R4);
	if (FAILED(rc)) return rc;

	if (V_R4(&lv) == V_R4(&rv)) return VARCMP_EQ;
	if (V_R4(&lv) < V_R4(&rv)) return VARCMP_LT;
	if (V_R4(&lv) > V_R4(&rv)) return VARCMP_GT;
	return E_FAIL; /* can't get here */
    }

    /* Integers - Ideally like to use VarDecCmp, but no Dec support yet
           Use LONGLONG to maximize ranges                              */
    lOk = TRUE;
    switch (V_VT(left)&VT_TYPEMASK) {
    case VT_I1   : lVal = V_UNION(left,cVal); break;
    case VT_I2   : lVal = V_UNION(left,iVal); break;
    case VT_I4   : lVal = V_UNION(left,lVal); break;
    case VT_INT  : lVal = V_UNION(left,lVal); break;
    case VT_UI1  : lVal = V_UNION(left,bVal); break;
    case VT_UI2  : lVal = V_UNION(left,uiVal); break;
    case VT_UI4  : lVal = V_UNION(left,ulVal); break;
    case VT_UINT : lVal = V_UNION(left,ulVal); break;
    case VT_BOOL : lVal = V_UNION(left,boolVal); break;
    default: lOk = FALSE;
    }

    rOk = TRUE;
    switch (V_VT(right)&VT_TYPEMASK) {
    case VT_I1   : rVal = V_UNION(right,cVal); break;
    case VT_I2   : rVal = V_UNION(right,iVal); break;
    case VT_I4   : rVal = V_UNION(right,lVal); break;
    case VT_INT  : rVal = V_UNION(right,lVal); break;
    case VT_UI1  : rVal = V_UNION(right,bVal); break;
    case VT_UI2  : rVal = V_UNION(right,uiVal); break;
    case VT_UI4  : rVal = V_UNION(right,ulVal); break;
    case VT_UINT : rVal = V_UNION(right,ulVal); break;
    case VT_BOOL : rVal = V_UNION(right,boolVal); break;
    default: rOk = FALSE;
    }

    if (lOk && rOk) {
        if (lVal < rVal) {
            return VARCMP_LT;
        } else if (lVal > rVal) {
            return VARCMP_GT;
        } else {
            return VARCMP_EQ;
        }
    }

    /* Strings - use VarBstrCmp */
    if ((V_VT(left)&VT_TYPEMASK) == VT_DATE &&
        (V_VT(right)&VT_TYPEMASK) == VT_DATE) {

        if (floor(V_UNION(left,date)) == floor(V_UNION(right,date))) {
            /* Due to floating point rounding errors, calculate varDate in whole numbers) */
            double wholePart = 0.0;
            double leftR;
            double rightR;

            /* Get the fraction * 24*60*60 to make it into whole seconds */
            wholePart = (double) floor( V_UNION(left,date) );
            if (wholePart == 0) wholePart = 1;
            leftR = floor(fmod( V_UNION(left,date), wholePart ) * (24*60*60));

            wholePart = (double) floor( V_UNION(right,date) );
            if (wholePart == 0) wholePart = 1;
            rightR = floor(fmod( V_UNION(right,date), wholePart ) * (24*60*60));

            if (leftR < rightR) {
                return VARCMP_LT;
            } else if (leftR > rightR) {
                return VARCMP_GT;
            } else {
                return VARCMP_EQ;
            }

        } else if (V_UNION(left,date) < V_UNION(right,date)) {
            return VARCMP_LT;
        } else if (V_UNION(left,date) > V_UNION(right,date)) {
            return VARCMP_GT;
        }
    }
    FIXME("VarCmp partial implementation, doesn't support vt 0x%x / 0x%x\n",V_VT(left), V_VT(right));
    return E_FAIL;
}

/**********************************************************************
 *              VarAnd [OLEAUT32.142]
 *
 */
HRESULT WINAPI VarAnd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    if ((V_VT(left)&VT_TYPEMASK) == VT_BOOL &&
        (V_VT(right)&VT_TYPEMASK) == VT_BOOL) {

        V_VT(result) = VT_BOOL;
        if (V_BOOL(left) && V_BOOL(right)) {
            V_BOOL(result) = VARIANT_TRUE;
        } else {
            V_BOOL(result) = VARIANT_FALSE;
        }
        rc = S_OK;

    } else {
        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 & I2 == I2, all else
                                  becomes I4, even unsigned ints (incl. UI2) */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal & rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("VarAnd stub\n");
        }
    }

    TRACE("returning 0x%8lx (%s%s),%ld\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_I4 ? V_I4(result) : V_I2(result));
    return rc;
}

/**********************************************************************
 *              VarAdd [OLEAUT32.141]
 * FIXME: From MSDN: If ... Then
 * Both expressions are of the string type Concatenated.
 * One expression is a string type and the other a character Addition.
 * One expression is numeric and the other is a string Addition.
 * Both expressions are numeric Addition.
 * Either expression is NULL NULL is returned.
 * Both expressions are empty  Integer subtype is returned.
 *
 */
HRESULT WINAPI VarAdd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    if ((V_VT(left)&VT_TYPEMASK) == VT_EMPTY)
    	return VariantCopy(result,right);

    if ((V_VT(right)&VT_TYPEMASK) == VT_EMPTY)
    	return VariantCopy(result,left);

    /* check if we add doubles */
    if (((V_VT(left)&VT_TYPEMASK) == VT_R8) || ((V_VT(right)&VT_TYPEMASK) == VT_R8)) {
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        double       lVal = -1;
        double       rVal = -1;
        double       res  = -1;

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);   break;
        case VT_I2   : lVal = V_UNION(left,iVal);   break;
        case VT_I4   : lVal = V_UNION(left,lVal);   break;
        case VT_INT  : lVal = V_UNION(left,lVal);   break;
        case VT_UI1  : lVal = V_UNION(left,bVal);   break;
        case VT_UI2  : lVal = V_UNION(left,uiVal);  break;
        case VT_UI4  : lVal = V_UNION(left,ulVal);  break;
        case VT_UINT : lVal = V_UNION(left,ulVal);  break;
        case VT_R4   : lVal = V_UNION(left,fltVal);  break;
        case VT_R8   : lVal = V_UNION(left,dblVal);  break;
	case VT_NULL : lVal = 0.0;  break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  break;
        case VT_I2   : rVal = V_UNION(right,iVal);  break;
        case VT_I4   : rVal = V_UNION(right,lVal);  break;
        case VT_INT  : rVal = V_UNION(right,lVal);  break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); break;
        case VT_UINT : rVal = V_UNION(right,ulVal); break;
        case VT_R4   : rVal = V_UNION(right,fltVal);break;
        case VT_R8   : rVal = V_UNION(right,dblVal);break;
	case VT_NULL : rVal = 0.0; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal + rVal);
            V_VT(result) = VT_R8;
            V_UNION(result,dblVal)  = res;
            rc = S_OK;
        } else {
	    FIXME("Unhandled type pair %d / %d in double addition.\n",
        	(V_VT(left)&VT_TYPEMASK),
        	(V_VT(right)&VT_TYPEMASK)
	    );
	}
	return rc;
    }

    /* now check if we add floats. VT_R8 can no longer happen here! */
    if (((V_VT(left)&VT_TYPEMASK) == VT_R4) || ((V_VT(right)&VT_TYPEMASK) == VT_R4)) {
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        float        lVal = -1;
        float        rVal = -1;
        float        res  = -1;

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);   break;
        case VT_I2   : lVal = V_UNION(left,iVal);   break;
        case VT_I4   : lVal = V_UNION(left,lVal);   break;
        case VT_INT  : lVal = V_UNION(left,lVal);   break;
        case VT_UI1  : lVal = V_UNION(left,bVal);   break;
        case VT_UI2  : lVal = V_UNION(left,uiVal);  break;
        case VT_UI4  : lVal = V_UNION(left,ulVal);  break;
        case VT_UINT : lVal = V_UNION(left,ulVal);  break;
        case VT_R4   : lVal = V_UNION(left,fltVal);  break;
	case VT_NULL : lVal = 0.0;  break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  break;
        case VT_I2   : rVal = V_UNION(right,iVal);  break;
        case VT_I4   : rVal = V_UNION(right,lVal);  break;
        case VT_INT  : rVal = V_UNION(right,lVal);  break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); break;
        case VT_UINT : rVal = V_UNION(right,ulVal); break;
        case VT_R4   : rVal = V_UNION(right,fltVal);break;
	case VT_NULL : rVal = 0.0; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal + rVal);
            V_VT(result) = VT_R4;
            V_UNION(result,fltVal)  = res;
            rc = S_OK;
        } else {
	    FIXME("Unhandled type pair %d / %d in float addition.\n",
        	(V_VT(left)&VT_TYPEMASK),
        	(V_VT(right)&VT_TYPEMASK)
	    );
	}
	return rc;
    }

    /* Handle strings as concat */
    if ((V_VT(left)&VT_TYPEMASK) == VT_BSTR &&
        (V_VT(right)&VT_TYPEMASK) == VT_BSTR) {
        V_VT(result) = VT_BSTR;
        return VarBstrCat(V_BSTR(left), V_BSTR(right), &V_BSTR(result));
    } else {

        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 + I2 == I2, all else
                                  becomes I4                                */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
	case VT_NULL : lVal = 0; resT = VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
	case VT_NULL : rVal = 0; resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal + rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("unimplemented part (0x%x + 0x%x)\n",V_VT(left), V_VT(right));
        }
    }

    TRACE("returning 0x%8lx (%s%s),%ld\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_I4 ? V_I4(result) : V_I2(result));
    return rc;
}

/**********************************************************************
 *              VarMul [OLEAUT32.156]
 *
 */
HRESULT WINAPI VarMul(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt=VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) * V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) * V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("returning 0x%8lx (%s%s),%g\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_R8 ? V_R8(result) : (double)V_I4(result));
    return rc;
}

/**********************************************************************
 *              VarDiv [OLEAUT32.143]
 *
 */
HRESULT WINAPI VarDiv(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt = VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) / V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) / V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("returning 0x%8lx (%s%s),%g\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_R8 ? V_R8(result) : (double)V_I4(result));
    return rc;
}

/**********************************************************************
 *              VarSub [OLEAUT32.159]
 *
 */
HRESULT WINAPI VarSub(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt = VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_DATE)|(1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) - V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) - V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("returning 0x%8lx (%s%s),%g\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_R8 ? V_R8(result) : (double)V_I4(result));
    return rc;
}

/**********************************************************************
 *              VarOr [OLEAUT32.157]
 *
 */
HRESULT WINAPI VarOr(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
          debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    if ((V_VT(left)&VT_TYPEMASK) == VT_BOOL &&
        (V_VT(right)&VT_TYPEMASK) == VT_BOOL) {

        V_VT(result) = VT_BOOL;
        if (V_BOOL(left) || V_BOOL(right)) {
            V_BOOL(result) = VARIANT_TRUE;
        } else {
            V_BOOL(result) = VARIANT_FALSE;
        }
        rc = S_OK;

    } else {
        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 & I2 == I2, all else
                                  becomes I4, even unsigned ints (incl. UI2) */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal | rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("unimplemented part, V_VT(left) == 0x%X, V_VT(right) == 0x%X\n", 
		V_VT(left) & VT_TYPEMASK, V_VT(right) & VT_TYPEMASK);
        }
    }

    TRACE("returning 0x%8lx (%s%s),%ld\n", rc, debugstr_VT(result),
          debugstr_VF(result), V_VT(result) == VT_I4 ? V_I4(result) : V_I2(result));
    return rc;
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
    VARIANT varIn;
    HRESULT hRet = S_OK;

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

    if (V_ISARRAY(pVarIn) || V_VT(pVarIn) == VT_UNKNOWN ||
        V_VT(pVarIn) == VT_DISPATCH || V_VT(pVarIn) == VT_RECORD ||
        V_VT(pVarIn) == VT_ERROR)
        return DISP_E_TYPEMISMATCH;

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
    ABS_CASE(R4,R4_MIN);
    case VT_BSTR:
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn));
        if (FAILED(hRet))
            break;
        V_VT(pVarOut) = VT_R8;
        pVarIn = &varIn;
        /* Fall through ... */
    case VT_DATE:
    ABS_CASE(R8,R8_MIN);
    case VT_CY:
        hRet = VarCyAbs(V_CY(pVarIn), & V_CY(pVarOut));
        break;
    case VT_DECIMAL:
        DEC_SIGN(&V_DECIMAL(pVarOut)) &= ~DECIMAL_NEG;
        break;
    case VT_UI1:
    case VT_UI2:
    case VT_UINT:
    case VT_UI4:
    case VT_UI8:
    case VT_EMPTY:
    case VT_NULL:
        /* No-Op */
        break;
    default:
        hRet = DISP_E_BADVARTYPE;
    }

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

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

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
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;

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

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

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
        return VarFix(pVarIn, pVarOut);
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

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

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
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;

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
 *  - Strictly speaking, this function performs a bitwise ones compliment
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

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_I1:  V_I1(pVarOut) = ~V_I1(pVarIn); break;
    case VT_UI1: V_UI1(pVarOut) = ~V_UI1(pVarIn); break;
    case VT_BOOL:
    case VT_I2:  V_I2(pVarOut) = ~V_I2(pVarIn); break;
    case VT_UI2: V_UI2(pVarOut) = ~V_UI2(pVarIn); break;
#ifndef __REACTOS__	/*FIXME: problems with MinGW and DEC_LO32 */
    case VT_DECIMAL:
        hRet = VarI4FromDec(&V_DECIMAL(pVarIn), &V_I4(&varIn));
        if (FAILED(hRet))
            break;
        pVarIn = &varIn;
        V_VT(pVarOut) = VT_I4;
        /* Fall through ... */
#endif
    case VT_INT:
    case VT_I4:  V_I4(pVarOut) = ~V_I4(pVarIn); break;
    case VT_UINT:
    case VT_UI4: V_UI4(pVarOut) = ~V_UI4(pVarIn); break;
    case VT_I8:  V_I8(pVarOut) = ~V_I8(pVarIn); break;
    case VT_UI8: V_UI8(pVarOut) = ~V_UI8(pVarIn); break;
    case VT_R4:
        hRet = VarI4FromR4(V_R4(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_BSTR:
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn));
        if (FAILED(hRet))
            break;
        pVarIn = &varIn;
        /* Fall through ... */
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
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;

    return hRet;
}

/**********************************************************************
 *              VarMod [OLEAUT32.154]
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
    BOOL         lOk        = TRUE;
    BOOL         rOk        = TRUE;
    HRESULT      rc         = E_FAIL;
    int          resT = 0;
    VARIANT      lv,rv;

    VariantInit(&lv);
    VariantInit(&rv);

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left),
		  debugstr_VF(left), right, debugstr_VT(right), debugstr_VF(right), result);

    /* check for invalid inputs */
    lOk = TRUE;
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
      break;
    case VT_VARIANT:
    case VT_UNKNOWN:
      V_VT(result) = VT_EMPTY;
      return DISP_E_TYPEMISMATCH;
    case VT_DECIMAL:
      V_VT(result) = VT_EMPTY;
      return E_INVALIDARG;
    case VT_ERROR:
      return DISP_E_TYPEMISMATCH;
    case VT_RECORD:
      V_VT(result) = VT_EMPTY;
      return DISP_E_TYPEMISMATCH;
    case VT_NULL:
      break;
    default:
      V_VT(result) = VT_EMPTY;
      return DISP_E_BADVARTYPE;
    }


    rOk = TRUE;
    switch (V_VT(right) & VT_TYPEMASK) {
    case VT_BOOL :
    case VT_I1   :
    case VT_I2   :
    case VT_I4   :
    case VT_I8   :
      if((V_VT(left) == VT_INT) && (V_VT(right) == VT_I8))
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_TYPEMISMATCH;
      }
    case VT_INT  :
      if((V_VT(right) == VT_INT) && (V_VT(left) == VT_I8))
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_TYPEMISMATCH;
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
	return S_OK;
      }
    case VT_EMPTY:
    case VT_DATE :
    case VT_BSTR:
      if(V_VT(left) == VT_NULL)
      {
	V_VT(result) = VT_NULL;
	return S_OK;
      }
      break;

    case VT_VOID:
      V_VT(result) = VT_EMPTY;
      return DISP_E_BADVARTYPE;
    case VT_NULL:
      if(V_VT(left) == VT_VOID)
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_BADVARTYPE;
      } else if((V_VT(left) == VT_NULL) || (V_VT(left) == VT_EMPTY) || (V_VT(left) == VT_ERROR) ||
		lOk)
      {
        V_VT(result) = VT_NULL;
	return S_OK;
      } else
      {
	V_VT(result) = VT_NULL;
	return DISP_E_BADVARTYPE;
      }
    case VT_VARIANT:
    case VT_UNKNOWN:
      V_VT(result) = VT_EMPTY;
      return DISP_E_TYPEMISMATCH;
    case VT_DECIMAL:
      if(V_VT(left) == VT_ERROR)
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_TYPEMISMATCH;
      } else
      {
	V_VT(result) = VT_EMPTY;
        return E_INVALIDARG;
      }
    case VT_ERROR:
      return DISP_E_TYPEMISMATCH;
    case VT_RECORD:
      if((V_VT(left) == 15) || ((V_VT(left) >= 24) && (V_VT(left) <= 35)) || !lOk)
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_BADVARTYPE;
      } else
      {
	V_VT(result) = VT_EMPTY;
	return DISP_E_TYPEMISMATCH;
      }
    default:
      V_VT(result) = VT_EMPTY;
      return DISP_E_BADVARTYPE;
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
      FIXME("Could not convert left type %d to %d? rc == 0x%lX\n", V_VT(left), VT_I8, rc);
      return rc;
    }

    rc = VariantChangeType(&rv, right, 0, VT_I8);
    if(FAILED(rc))
    {
      FIXME("Could not convert right type %d to %d? rc == 0x%lX\n", V_VT(right), VT_I8, rc);
      return rc;
    }

    /* if right is zero set VT_EMPTY and return divide by zero */
    if(V_I8(&rv) == 0)
    {
      V_VT(result) = VT_EMPTY;
      return DISP_E_DIVBYZERO;
    }

    /* perform the modulo operation */
    V_VT(result) = VT_I8;
    V_I8(result) = V_I8(&lv) % V_I8(&rv);

    TRACE("V_I8(left) == %ld, V_I8(right) == %ld, V_I8(result) == %ld\n", (long)V_I8(&lv), (long)V_I8(&rv), (long)V_I8(result));

    /* convert left and right to the destination type */
    rc = VariantChangeType(result, result, 0, resT);
    if(FAILED(rc))
    {
      FIXME("Could not convert 0x%x to %d?\n", V_VT(result), resT);
      return rc;
    }

    return S_OK;
}

/**********************************************************************
 *              VarPow [OLEAUT32.158]
 *
 */
HRESULT WINAPI VarPow(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT hr;
    VARIANT dl,dr;

    TRACE("(%p->(%s%s),%p->(%s%s),%p)\n", left, debugstr_VT(left), debugstr_VF(left),
          right, debugstr_VT(right), debugstr_VF(right), result);

    hr = VariantChangeType(&dl,left,0,VT_R8);
    if (!SUCCEEDED(hr)) {
        ERR("Could not change passed left argument to VT_R8, handle it differently.\n");
        return E_FAIL;
    }
    hr = VariantChangeType(&dr,right,0,VT_R8);
    if (!SUCCEEDED(hr)) {
        ERR("Could not change passed right argument to VT_R8, handle it differently.\n");
        return E_FAIL;
    }
    V_VT(result) = VT_R8;
    V_R8(result) = pow(V_R8(&dl),V_R8(&dr));
    return S_OK;
}

#endif	/*FIXME*/
