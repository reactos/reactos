/*
 * Variant Inlines
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
#include "windef.h"
#include "winerror.h"
#include "oleauto.h"
#include <math.h>


/* Get just the type from a variant pointer */
#define V_TYPE(v)  (V_VT((v)) & VT_TYPEMASK)

/* Flags set in V_VT, other than the actual type value */
#define VT_EXTRA_TYPE (VT_VECTOR|VT_ARRAY|VT_BYREF|VT_RESERVED)

/* Get the extra flags from a variant pointer */
#define V_EXTRA_TYPE(v) (V_VT((v)) & VT_EXTRA_TYPE)

extern const char* wine_vtypes[];
#define debugstr_vt(v) (((v)&VT_TYPEMASK) <= VT_CLSID ? wine_vtypes[((v)&VT_TYPEMASK)] : \
  ((v)&VT_TYPEMASK) == VT_BSTR_BLOB ? "VT_BSTR_BLOB": "Invalid")
#define debugstr_VT(v) (!(v) ? "(null)" : debugstr_vt(V_TYPE((v))))

extern const char* wine_vflags[];
#define debugstr_vf(v) (wine_vflags[((v)&VT_EXTRA_TYPE)>>12])
#define debugstr_VF(v) (!(v) ? "(null)" : debugstr_vf(V_EXTRA_TYPE(v)))

/* Size constraints */
#define I1_MAX   0x7f
#define I1_MIN   ((-I1_MAX)-1)
#define UI1_MAX  0xff
#define UI1_MIN  0
#define I2_MAX   0x7fff
#define I2_MIN   ((-I2_MAX)-1)
#define UI2_MAX  0xffff
#define UI2_MIN  0
#define I4_MAX   0x7fffffff
#define I4_MIN   ((-I4_MAX)-1)
#define UI4_MAX  0xffffffff
#define UI4_MIN  0
#define I8_MAX   (((LONGLONG)I4_MAX << 32) | UI4_MAX)
#define I8_MIN   ((-I8_MAX)-1)
#define UI8_MAX  (((ULONGLONG)UI4_MAX << 32) | UI4_MAX)
#define UI8_MIN  0
#define DATE_MAX 2958465
#define DATE_MIN -657434
#define R4_MAX 3.402823567797336e38
#define R4_MIN 1.40129846432481707e-45
#define R8_MAX 1.79769313486231470e+308
#define R8_MIN 4.94065645841246544e-324

/* Value of sign for a positive decimal number */
#define DECIMAL_POS 0

/* Native headers don't change the union ordering for DECIMAL sign/scale (duh).
 * This means that the signscale member is only useful for setting both members to 0.
 * SIGNSCALE creates endian-correct values so that we can properly set both at once
 * to values other than 0.
 */
#ifdef WORDS_BIGENDIAN
#define SIGNSCALE(sign,scale) (((scale) << 8) | sign)
#else
#define SIGNSCALE(sign,scale) (((sign) << 8) | scale)
#endif

/* Macros for getting at a DECIMAL's parts */
#ifndef DEC_LO64
#define DEC_SIGN(d)      ((d)->u.s.sign)
#define DEC_SCALE(d)     ((d)->u.s.scale)
#define DEC_SIGNSCALE(d) ((d)->u.signscale)
#define DEC_HI32(d)      ((d)->Hi32)
#define DEC_MID32(d)     ((d)->DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.Mid32)
#define DEC_LO32(d)      ((d)->DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.Lo32)
#define DEC_LO64(d)      ((d)->DUMMYUNIONNAME2.Lo64)
#endif

#define DEC_MAX_SCALE    28 /* Maximum scale for a decimal */

/* Inline return type */
#define RETTYP inline static HRESULT

/* Simple compiler cast from one type to another */
#define SIMPLE(dest, src, func) RETTYP _##func(src in, dest* out) { \
  *out = in; return S_OK; }

/* Compiler cast where input cannot be negative */
#define NEGTST(dest, src, func) RETTYP _##func(src in, dest* out) { \
  if (in < (src)0) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be > some number */
#define POSTST(dest, src, func, tst) RETTYP _##func(src in, dest* out) { \
  if (in > (dest)tst) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be < some number or >= some other number */
#define BOTHTST(dest, src, func, lo, hi) RETTYP _##func(src in, dest* out) { \
  if (in < (dest)lo || in > hi) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Conversions from IDispatch use the same code */
HRESULT VARIANT_FromDisp(IDispatch*,LCID,void*,VARTYPE);
/* As do conversions from BSTR to numeric types */
HRESULT VARIANT_NumberFromBstr(OLECHAR*,LCID,ULONG,void*,VARTYPE);

#define CY_MULTIPLIER   10000             /* 4 dp of precision */
#define CY_MULTIPLIER_F 10000.0
#define CY_HALF         (CY_MULTIPLIER/2) /* 0.5 */
#define CY_HALF_F       (CY_MULTIPLIER_F/2.0)

/* I1 */
POSTST(signed char, BYTE, VarI1FromUI1, I1_MAX);
BOTHTST(signed char, SHORT, VarI1FromI2, I1_MIN, I1_MAX);
BOTHTST(signed char, LONG, VarI1FromI4, I1_MIN, I1_MAX);
#define _VarI1FromR4(flt,out) VarI1FromR8((double)flt,out)
#define _VarI1FromR8 VarI1FromR8
#define _VarI1FromCy VarI1FromCy
#define _VarI1FromDate(dt,out) VarI1FromR8((double)dt,out)
#define _VarI1FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_I1)
#define _VarI1FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, (BYTE*)out, VT_I1)
SIMPLE(signed char, VARIANT_BOOL, VarI1FromBool);
POSTST(signed char, USHORT, VarI1FromUI2, I1_MAX);
POSTST(signed char, ULONG, VarI1FromUI4, I1_MAX);
#define _VarI1FromDec VarI1FromDec
BOTHTST(signed char, LONG64, VarI1FromI8, I1_MIN, I1_MAX);
POSTST(signed char, ULONG64, VarI1FromUI8, I1_MAX);

/* UI1 */
BOTHTST(BYTE, SHORT, VarUI1FromI2, UI1_MIN, UI1_MAX);
#define _VarUI1FromR4(flt,out) VarUI1FromR8((double)flt,out)
#define _VarUI1FromR8 VarUI1FromR8
#define _VarUI1FromCy VarUI1FromCy
#define _VarUI1FromDate(dt,out) VarUI1FromR8((double)dt,out)
#define _VarUI1FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_UI1)
#define _VarUI1FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_UI1)
SIMPLE(BYTE, VARIANT_BOOL, VarUI1FromBool);
NEGTST(BYTE, signed char, VarUI1FromI1);
POSTST(BYTE, USHORT, VarUI1FromUI2, UI1_MAX);
BOTHTST(BYTE, LONG, VarUI1FromI4, UI1_MIN, UI1_MAX);
POSTST(BYTE, ULONG, VarUI1FromUI4, UI1_MAX);
#define _VarUI1FromDec VarUI1FromDec
BOTHTST(BYTE, LONG64, VarUI1FromI8, UI1_MIN, UI1_MAX);
POSTST(BYTE, ULONG64, VarUI1FromUI8, UI1_MAX);

/* I2 */
SIMPLE(SHORT, BYTE, VarI2FromUI1);
BOTHTST(SHORT, LONG, VarI2FromI4, I2_MIN, I2_MAX);
#define _VarI2FromR4(flt,out) VarI2FromR8((double)flt,out)
#define _VarI2FromR8 VarI2FromR8
#define _VarI2FromCy VarI2FromCy
#define _VarI2FromDate(dt,out) VarI2FromR8((double)dt,out)
#define _VarI2FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_I2)
#define _VarI2FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, (BYTE*)out, VT_I2)
SIMPLE(SHORT, VARIANT_BOOL, VarI2FromBool);
SIMPLE(SHORT, signed char, VarI2FromI1);
POSTST(SHORT, USHORT, VarI2FromUI2, I2_MAX);
POSTST(SHORT, ULONG, VarI2FromUI4, I2_MAX);
#define _VarI2FromDec VarI2FromDec
BOTHTST(SHORT, LONG64, VarI2FromI8, I2_MIN, I2_MAX);
POSTST(SHORT, ULONG64, VarI2FromUI8, I2_MAX);

/* UI2 */
SIMPLE(USHORT, BYTE, VarUI2FromUI1);
NEGTST(USHORT, SHORT, VarUI2FromI2);
BOTHTST(USHORT, LONG, VarUI2FromI4, UI2_MIN, UI2_MAX);
#define _VarUI2FromR4(flt,out) VarUI2FromR8((double)flt,out)
#define _VarUI2FromR8 VarUI2FromR8
#define _VarUI2FromCy VarUI2FromCy
#define _VarUI2FromDate(dt,out) VarUI2FromR8((double)dt,out)
#define _VarUI2FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_UI2)
#define _VarUI2FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_UI2)
SIMPLE(USHORT, VARIANT_BOOL, VarUI2FromBool);
NEGTST(USHORT, signed char, VarUI2FromI1);
POSTST(USHORT, ULONG, VarUI2FromUI4, UI2_MAX);
#define _VarUI2FromDec VarUI2FromDec
BOTHTST(USHORT, LONG64, VarUI2FromI8, UI2_MIN, UI2_MAX);
POSTST(USHORT, ULONG64, VarUI2FromUI8, UI2_MAX);

/* I4 */
SIMPLE(LONG, BYTE, VarI4FromUI1);
SIMPLE(LONG, SHORT, VarI4FromI2);
#define _VarI4FromR4(flt,out) VarI4FromR8((double)flt,out)
#define _VarI4FromR8 VarI4FromR8
#define _VarI4FromCy VarI4FromCy
#define _VarI4FromDate(dt,out) VarI4FromR8((double)dt,out)
#define _VarI4FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_I4)
#define _VarI4FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, (BYTE*)out, VT_I4)
SIMPLE(LONG, VARIANT_BOOL, VarI4FromBool);
SIMPLE(LONG, signed char, VarI4FromI1);
SIMPLE(LONG, USHORT, VarI4FromUI2);
POSTST(LONG, ULONG, VarI4FromUI4, I4_MAX);
#define _VarI4FromDec VarI4FromDec
BOTHTST(LONG, LONG64, VarI4FromI8, I4_MIN, I4_MAX);
POSTST(LONG, ULONG64, VarI4FromUI8, I4_MAX);

/* UI4 */
SIMPLE(ULONG, BYTE, VarUI4FromUI1);
NEGTST(ULONG, SHORT, VarUI4FromI2);
NEGTST(ULONG, LONG, VarUI4FromI4);
#define _VarUI4FromR4(flt,out) VarUI4FromR8((double)flt,out)
#define _VarUI4FromR8 VarUI4FromR8
#define _VarUI4FromCy VarUI4FromCy
#define _VarUI4FromDate(dt,out) VarUI4FromR8((double)dt,out)
#define _VarUI4FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_UI4)
#define _VarUI4FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_UI4)
SIMPLE(ULONG, VARIANT_BOOL, VarUI4FromBool);
NEGTST(ULONG, signed char, VarUI4FromI1);
SIMPLE(ULONG, USHORT, VarUI4FromUI2);
#define _VarUI4FromDec VarUI4FromDec
BOTHTST(ULONG, LONG64, VarUI4FromI8, UI4_MIN, UI4_MAX);
POSTST(ULONG, ULONG64, VarUI4FromUI8, UI4_MAX);

/* I8 */
SIMPLE(LONG64, BYTE, VarI8FromUI1);
SIMPLE(LONG64, SHORT, VarI8FromI2);
#define _VarI8FromR4 VarI8FromR8
#define _VarI8FromR8 VarI8FromR8
#define _VarI8FromCy VarI8FromCy
#define _VarI8FromDate(dt,out) VarI8FromR8((double)dt,out)
#define _VarI8FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_I8)
#define _VarI8FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_I8)
#define _VarI8FromBool   _VarI8FromI2
SIMPLE(LONG64, signed char, VarI8FromI1);
SIMPLE(LONG64, USHORT, VarI8FromUI2);
SIMPLE(LONG64, LONG, VarI8FromI4);
SIMPLE(LONG64, ULONG, VarI8FromUI4);
#define _VarI8FromDec VarI8FromDec
POSTST(LONG64, ULONG64, VarI8FromUI8, I8_MAX);

/* UI8 */
SIMPLE(ULONG64, BYTE, VarUI8FromUI1);
NEGTST(ULONG64, SHORT, VarUI8FromI2);
#define _VarUI8FromR4 VarUI8FromR8
#define _VarUI8FromR8 VarUI8FromR8
#define _VarUI8FromCy VarUI8FromCy
#define _VarUI8FromDate(dt,out) VarUI8FromR8((double)dt,out)
#define _VarUI8FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_UI8)
#define _VarUI8FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_UI8)
#define _VarUI8FromBool _VarI8FromI2
NEGTST(ULONG64, signed char, VarUI8FromI1);
SIMPLE(ULONG64, USHORT, VarUI8FromUI2);
NEGTST(ULONG64, LONG, VarUI8FromI4);
SIMPLE(ULONG64, ULONG, VarUI8FromUI4);
#define _VarUI8FromDec VarUI8FromDec
NEGTST(ULONG64, LONG64, VarUI8FromI8);

/* R4 (float) */
SIMPLE(float, BYTE, VarR4FromUI1);
SIMPLE(float, SHORT, VarR4FromI2);
RETTYP _VarR4FromR8(double i, float* o) {
  double d = i < 0.0 ? -i : i;
  if (d > R4_MAX) return DISP_E_OVERFLOW;
  *o = i;
  return S_OK;
}
RETTYP _VarR4FromCy(CY i, float* o) { *o = (double)i.int64 / CY_MULTIPLIER_F; return S_OK; }
#define _VarR4FromDate(dt,out) _VarR4FromR8((double)dt,out)
#define _VarR4FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_R4)
#define _VarR4FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_R4)
#define _VarR4FromBool _VarR4FromI2
SIMPLE(float, signed char, VarR4FromI1);
SIMPLE(float, USHORT, VarR4FromUI2);
SIMPLE(float, LONG, VarR4FromI4);
SIMPLE(float, ULONG, VarR4FromUI4);
#define _VarR4FromDec VarR4FromDec
SIMPLE(float, LONG64, VarR4FromI8);
SIMPLE(float, ULONG64, VarR4FromUI8);

/* R8 (double) */
SIMPLE(double, BYTE, VarR8FromUI1);
SIMPLE(double, SHORT, VarR8FromI2);
SIMPLE(double, float, VarR8FromR4);
RETTYP _VarR8FromCy(CY i, double* o) { *o = (double)i.int64 / CY_MULTIPLIER_F; return S_OK; }
SIMPLE(double, DATE, VarR8FromDate);
#define _VarR8FromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_R8)
#define _VarR8FromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_R8)
#define _VarR8FromBool _VarR8FromI2
SIMPLE(double, signed char, VarR8FromI1);
SIMPLE(double, USHORT, VarR8FromUI2);
SIMPLE(double, LONG, VarR8FromI4);
SIMPLE(double, ULONG, VarR8FromUI4);
#define _VarR8FromDec VarR8FromDec
SIMPLE(double, LONG64, VarR8FromI8);
SIMPLE(double, ULONG64, VarR8FromUI8);

/* BOOL */
#define BOOLFUNC(src, func) RETTYP _##func(src in, VARIANT_BOOL* out) { \
  *out = in ? VARIANT_TRUE : VARIANT_FALSE; return S_OK; }

BOOLFUNC(signed char,VarBoolFromI1);
BOOLFUNC(BYTE,VarBoolFromUI1);
BOOLFUNC(SHORT,VarBoolFromI2);
BOOLFUNC(USHORT,VarBoolFromUI2);
BOOLFUNC(LONG,VarBoolFromI4);
BOOLFUNC(ULONG,VarBoolFromUI4);
BOOLFUNC(LONG64,VarBoolFromI8);
BOOLFUNC(ULONG64,VarBoolFromUI8);
#define _VarBoolFromR4(flt,out) _VarBoolFromR8((double)flt,out)
BOOLFUNC(double,VarBoolFromR8);
#define _VarBoolFromCy(i,o) _VarBoolFromI8(i.int64,o)
#define _VarBoolFromDate(dt,out) _VarBoolFromR8((double)dt,out)
#define _VarBoolFromStr VarBoolFromStr
#define _VarBoolFromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, (BYTE*)out, VT_BOOL)
#define _VarBoolFromDec VarBoolFromDec

/* Internal flags for low level conversion functions */
#define  VAR_BOOLONOFF 0x0400 /* Convert bool to "On"/"Off" */
#define  VAR_BOOLYESNO 0x0800 /* Convert bool to "Yes"/"No" */
#define  VAR_NEGATIVE  0x1000 /* Number is negative */

/* DECIMAL */
#define _VarDecFromUI1 VarDecFromUI4
#define _VarDecFromI2 VarDecFromI4
#define _VarDecFromR4 VarDecFromR4
#define _VarDecFromR8 VarDecFromR8
#define _VarDecFromCy VarDecFromCy
#define _VarDecFromDate(dt,out) VarDecFromR8((double)dt,out)
#define _VarDecFromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_DECIMAL)
#define _VarDecFromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_DECIMAL)
#define _VarDecFromBool VarDecFromBool
#define _VarDecFromI1 VarDecFromI4
#define _VarDecFromUI2 VarDecFromUI4
#define _VarDecFromI4 VarDecFromI4
#define _VarDecFromUI4 VarDecFromUI4
#define _VarDecFromI8 VarDecFromI8
#define _VarDecFromUI8 VarDecFromUI8

/* CY (Currency) */
#define _VarCyFromUI1 VarCyFromR8
#define _VarCyFromI2 VarCyFromR8
#define _VarCyFromR4 VarCyFromR8
#define _VarCyFromR8 VarCyFromR8
#define _VarCyFromDate VarCyFromR8
#define _VarCyFromStr(str,lcid,flags,out) VARIANT_NumberFromBstr(str,lcid,flags,(BYTE*)out,VT_CY)
#define _VarCyFromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_CY)
#define _VarCyFromBool VarCyFromR8
#define _VarCyFromI1 VarCyFromR8
#define _VarCyFromUI2 VarCyFromR8
#define _VarCyFromI4 VarCyFromR8
#define _VarCyFromUI4 VarCyFromR8
#define _VarCyFromDec VarCyFromDec
RETTYP _VarCyFromI8(LONG64 i, CY* o) {
  if (i <= (I8_MIN/CY_MULTIPLIER) || i >= (I8_MAX/CY_MULTIPLIER)) return DISP_E_OVERFLOW;
  o->int64 = i * CY_MULTIPLIER;
  return S_OK;
}
#define _VarCyFromUI8 VarCyFromR8

/* DATE */
#define _VarDateFromUI1 _VarR8FromUI1
#define _VarDateFromI2 _VarR8FromI2
#define _VarDateFromR4 _VarDateFromR8
RETTYP _VarDateFromR8(double i, DATE* o) {
  if (i <= (DATE_MIN - 1.0) || i >= (DATE_MAX + 1.0)) return DISP_E_OVERFLOW;
  *o = (DATE)i;
  return S_OK;
}
#define _VarDateFromCy _VarR8FromCy
#define _VarDateFromStr VarDateFromStr
#define _VarDateFromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_DATE)
#define _VarDateFromBool _VarR8FromBool
#define _VarDateFromI1 _VarR8FromI1
#define _VarDateFromUI2 _VarR8FromUI2
#define _VarDateFromI4 _VarDateFromR8
#define _VarDateFromUI4 _VarDateFromR8
#define _VarDateFromDec _VarR8FromDec
RETTYP _VarDateFromI8(LONG64 i, DATE* o) {
  if (i < DATE_MIN || i > DATE_MAX) return DISP_E_OVERFLOW;
  *o = (DATE)i;
  return S_OK;
}
RETTYP _VarDateFromUI8(ULONG64 i, DATE* o) {
  if (i > DATE_MAX) return DISP_E_OVERFLOW;
  *o = (DATE)i;
  return S_OK;
}

/* BSTR */
#define _VarBstrFromUI1 VarBstrFromUI4
#define _VarBstrFromI2 VarBstrFromI4
#define _VarBstrFromR4 VarBstrFromR8
#define _VarBstrFromR8 VarBstrFromR8
#define _VarBstrFromCy VarBstrFromCy
#define _VarBstrFromDate VarBstrFromDate
#define _VarBstrFromDisp(disp,lcid,out) VARIANT_FromDisp(disp, lcid, out, VT_BSTR)
#define _VarBstrFromBool VarBstrFromBool
#define _VarBstrFromI1 VarBstrFromI4
#define _VarBstrFromUI2 VarBstrFromUI4
#define _VarBstrFromI4 VarBstrFromI4
#define _VarBstrFromUI4 VarBstrFromUI4
#define _VarBstrFromDec VarBstrFromDec
#define _VarBstrFromI8 VarBstrFromI8
#define _VarBstrFromUI8 VarBstrFromUI8

/* Macro to inline conversion from a float or double to any integer type,
 * rounding according to the 'dutch' convention.
 */
#define OLEAUT32_DutchRound(typ, value, res) do { \
  double whole = (double)value < 0 ? ceil((double)value) : floor((double)value); \
  double fract = (double)value - whole; \
  if (fract > 0.5) res = (typ)whole + (typ)1; \
  else if (fract == 0.5) { typ is_odd = (typ)whole & 1; res = whole + is_odd; } \
  else if (fract >= 0.0) res = (typ)whole; \
  else if (fract == -0.5) { typ is_odd = (typ)whole & 1; res = whole - is_odd; } \
  else if (fract > -0.5) res = (typ)whole; \
  else res = (typ)whole - (typ)1; \
} while(0);

/* The localised characters that make up a valid number */
typedef struct tagVARIANT_NUMBER_CHARS
{
  WCHAR cNegativeSymbol;
  WCHAR cPositiveSymbol;
  WCHAR cDecimalPoint;
  WCHAR cDigitSeperator;
  WCHAR cCurrencyLocal;
  WCHAR cCurrencyLocal2;
  WCHAR cCurrencyDecimalPoint;
  WCHAR cCurrencyDigitSeperator;
} VARIANT_NUMBER_CHARS;

void VARIANT_GetLocalisedNumberChars(VARIANT_NUMBER_CHARS*,LCID,DWORD);
