/*
 * Low level variant tests
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define CONST_VTABLE
#define COBJMACROS

#include <wine/test.h>
#include <winnls.h>
#include <objbase.h>
#include <oleauto.h>
#include <math.h>
#include <test_tlb.h>

#include <initguid.h>

DEFINE_GUID(UUID_test_struct, 0x4029f190, 0xca4a, 0x4611, 0xae,0xb9,0x67,0x39,0x83,0xcb,0x96,0xdd);

/* Some Visual C++ versions choke on __uint64 to float conversions.
 * To fix this you need either VC++ 6.0 plus the processor pack
 * or Visual C++ >=7.0.
 */
#ifndef _MSC_VER
#  define HAS_UINT64_TO_FLOAT
#else
#  if _MSC_VER >= 1300
#    define HAS_UINT64_TO_FLOAT
#  else
#    include <malloc.h>
#    if defined(_mm_free)
/*     _mm_free is defined if the Processor Pack has been installed */
#      define HAS_UINT64_TO_FLOAT
#    endif

#  endif
#endif

static HMODULE hOleaut32;

/* Get a conversion function ptr, return if function not available */
#define CHECKPTR(func) p##func = (void*)GetProcAddress(hOleaut32, #func); \
  if (!p##func) { \
    win_skip("function " # func " not available, not testing it\n"); return; }

/* Has I8/UI8 data type? */
static BOOL has_i8;
/* Has proper locale conversions? */
static BOOL has_locales;

/* Is vt a type unavailable to ancient versions? */
#define IS_MODERN_VTYPE(vt) (vt==VT_VARIANT||vt==VT_DECIMAL|| \
    vt==VT_I1||vt==VT_UI2||vt==VT_UI4||vt == VT_INT||vt == VT_UINT)

/* Macros for converting and testing results */
#define CONVVARS(typ) HRESULT hres; CONV_TYPE out; typ in

#define _EXPECT_NO_OUT(res)     ok(hres == res, "expected " #res ", got hres=0x%08x\n", hres)
#define EXPECT_OVERFLOW _EXPECT_NO_OUT(DISP_E_OVERFLOW)
#define EXPECT_MISMATCH _EXPECT_NO_OUT(DISP_E_TYPEMISMATCH)
#define EXPECT_BADVAR   _EXPECT_NO_OUT(DISP_E_BADVARTYPE)
#define EXPECT_INVALID  _EXPECT_NO_OUT(E_INVALIDARG)
#define EXPECT_LT       _EXPECT_NO_OUT(VARCMP_LT)
#define EXPECT_GT       _EXPECT_NO_OUT(VARCMP_GT)
#define EXPECT_EQ       _EXPECT_NO_OUT(VARCMP_EQ)

#define _EXPECTRES(res, x, fs) \
  ok(hres == S_OK && out == (CONV_TYPE)(x), "expected " #x ", got " fs "; hres=0x%08x\n", out, hres)
#define EXPECT(x)       EXPECTRES(S_OK, (x))
#define EXPECT_DBL(x)   \
  ok(hres == S_OK && fabs(out-(x))<=1e-14*(x), "expected %16.16g, got %16.16g; hres=0x%08x\n", (x), out, hres)

#define CONVERT(func, val) in = val; hres = p##func(in, &out)
#define CONVERTRANGE(func,start,end) for (i = start; i < end; i+=1) { CONVERT(func, i); EXPECT(i); };
#define OVERFLOWRANGE(func,start,end) for (i = start; i < end; i+=1) { CONVERT(func, i); EXPECT_OVERFLOW; };

#define CY_MULTIPLIER   10000

#define DATE_MIN -657434
#define DATE_MAX 2958465

#define CONVERT_I8(func,hi,lo) in = hi; in = (in << 32) | lo; hres = p##func(in, &out)

#define CONVERT_CY(func,val) in.int64 = (LONGLONG)(val * CY_MULTIPLIER); hres = p##func(in, &out)

#define CONVERT_CY64(func,hi,lo) S(in).Hi = hi; S(in).Lo = lo; in.int64 *= CY_MULTIPLIER; hres = p##func(in, &out)

#define SETDEC(dec, scl, sgn, hi, lo) S(U(dec)).scale = (BYTE)scl; S(U(dec)).sign = (BYTE)sgn; \
  dec.Hi32 = (ULONG)hi; U1(dec).Lo64 = (ULONG64)lo

#define SETDEC64(dec, scl, sgn, hi, mid, lo) S(U(dec)).scale = (BYTE)scl; S(U(dec)).sign = (BYTE)sgn; \
  dec.Hi32 = (ULONG)hi; S1(U1(dec)).Mid32 = mid; S1(U1(dec)).Lo32 = lo;

#define CONVERT_DEC(func,scl,sgn,hi,lo) SETDEC(in,scl,sgn,hi,lo); hres = p##func(&in, &out)

#define CONVERT_DEC64(func,scl,sgn,hi,mid,lo) SETDEC64(in,scl,sgn,hi,mid,lo); hres = p##func(&in, &out)

#define CONVERT_BADDEC(func) \
  CONVERT_DEC(func,29,0,0,0);   EXPECT_INVALID; \
  CONVERT_DEC(func,0,0x1,0,0);  EXPECT_INVALID; \
  CONVERT_DEC(func,0,0x40,0,0); EXPECT_INVALID; \
  CONVERT_DEC(func,0,0x7f,0,0); EXPECT_INVALID;

#define CONVERT_STR(func,str,flags) \
  SetLastError(0); \
  if (str) MultiByteToWideChar(CP_ACP,0,str,-1,buff,sizeof(buff)/sizeof(WCHAR)); \
  hres = p##func(str ? buff : NULL,in,flags,&out)

#define COPYTEST(val, vt, srcval, dstval, srcref, dstref, fs) do { \
  HRESULT hres; VARIANTARG vSrc, vDst; CONV_TYPE in = val; \
  VariantInit(&vSrc); VariantInit(&vDst); \
  V_VT(&vSrc) = vt; srcval = in; \
  hres = VariantCopy(&vDst, &vSrc); \
  ok(hres == S_OK && V_VT(&vDst) == vt && dstval == in, \
     "copy hres 0x%X, type %d, value (" fs ") " fs "\n", hres, V_VT(&vDst), val, dstval); \
  V_VT(&vSrc) = vt|VT_BYREF; srcref = &in; \
  hres = VariantCopy(&vDst, &vSrc); \
  ok(hres == S_OK && V_VT(&vDst) == (vt|VT_BYREF) && dstref == &in, \
     "ref hres 0x%X, type %d, ref (%p) %p\n", hres, V_VT(&vDst), &in, dstref); \
  hres = VariantCopyInd(&vDst, &vSrc); \
  ok(hres == S_OK && V_VT(&vDst) == vt && dstval == in, \
     "ind hres 0x%X, type %d, value (" fs ") " fs "\n", hres, V_VT(&vDst), val, dstval); \
  } while(0)

#define CHANGETYPEEX(typ) hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, typ)

#define TYPETEST(typ,res,fs) CHANGETYPEEX(typ); \
  ok(hres == S_OK && V_VT(&vDst) == typ && (CONV_TYPE)res == in, \
     "hres=0x%X, type=%d (should be %d(" #typ ")), value=" fs " (should be " fs ")\n", \
      hres, V_VT(&vDst), typ, (CONV_TYPE)res, in);
#define TYPETESTI8(typ,res) CHANGETYPEEX(typ); \
  ok(hres == S_OK && V_VT(&vDst) == typ && (CONV_TYPE)res == in, \
     "hres=0x%X, type=%d (should be %d(" #typ ")), value=%d (should be 1)\n", \
      hres, V_VT(&vDst), typ, (int)res);
#define BADVAR(typ)   CHANGETYPEEX(typ); EXPECT_BADVAR
#define MISMATCH(typ) CHANGETYPEEX(typ); EXPECT_MISMATCH

#define INITIAL_TYPETEST(vt, val, fs) \
  VariantInit(&vSrc); \
  VariantInit(&vDst); \
  V_VT(&vSrc) = vt; \
  (val(&vSrc)) = in; \
  TYPETEST(VT_I1, V_I1(&vDst), fs); \
  TYPETEST(VT_UI2, V_UI2(&vDst), fs); \
  TYPETEST(VT_UI4, V_UI4(&vDst), fs); \
  TYPETEST(VT_INT, V_INT(&vDst), fs); \
  TYPETEST(VT_UINT, V_UINT(&vDst), fs); \
  TYPETEST(VT_UI1, V_UI1(&vDst), fs); \
  TYPETEST(VT_I2, V_I2(&vDst), fs); \
  TYPETEST(VT_I4, V_I4(&vDst), fs); \
  TYPETEST(VT_R4, V_R4(&vDst), fs); \
  TYPETEST(VT_R8, V_R8(&vDst), fs); \
  TYPETEST(VT_DATE, V_DATE(&vDst), fs); \
  if (has_i8) \
  { \
    TYPETEST(VT_I8, V_I8(&vDst), fs); \
    TYPETEST(VT_UI8, V_UI8(&vDst), fs); \
  }
#define NEGATIVE_TYPETEST(vt, val, fs, vtneg, valneg) \
  in = -in; \
  VariantInit(&vSrc); \
  VariantInit(&vDst); \
  V_VT(&vSrc) = vt; \
  (val(&vSrc)) = in; \
  TYPETEST(vtneg, valneg(&vDst), fs);

#define INITIAL_TYPETESTI8(vt, val) \
  VariantInit(&vSrc); \
  VariantInit(&vDst); \
  V_VT(&vSrc) = vt; \
  (val(&vSrc)) = in; \
  TYPETESTI8(VT_I1, V_I1(&vDst)); \
  TYPETESTI8(VT_UI1, V_UI1(&vDst)); \
  TYPETESTI8(VT_I2, V_I2(&vDst)); \
  TYPETESTI8(VT_UI2, V_UI2(&vDst)); \
  TYPETESTI8(VT_I4, V_I4(&vDst)); \
  TYPETESTI8(VT_UI4, V_UI4(&vDst)); \
  TYPETESTI8(VT_INT, V_INT(&vDst)); \
  TYPETESTI8(VT_UINT, V_UINT(&vDst)); \
  TYPETESTI8(VT_R4, V_R4(&vDst)); \
  TYPETESTI8(VT_R8, V_R8(&vDst)); \
  TYPETESTI8(VT_DATE, V_DATE(&vDst)); \
  TYPETESTI8(VT_I8, V_I8(&vDst)); \
  TYPETESTI8(VT_UI8, V_UI8(&vDst))

#define COMMON_TYPETEST \
  hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_BOOL); \
  ok(hres == S_OK && V_VT(&vDst) == VT_BOOL && \
     (V_BOOL(&vDst) == VARIANT_TRUE || (V_VT(&vSrc) == VT_BOOL && V_BOOL(&vDst) == 1)), \
     "->VT_BOOL hres=0x%X, type=%d (should be VT_BOOL), value %d (should be VARIANT_TRUE)\n", \
     hres, V_VT(&vDst), V_BOOL(&vDst)); \
  hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_CY); \
  ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == CY_MULTIPLIER, \
     "->VT_CY hres=0x%X, type=%d (should be VT_CY), value (%08x,%08x) (should be CY_MULTIPLIER)\n", \
     hres, V_VT(&vDst), S(V_CY(&vDst)).Hi, S(V_CY(&vDst)).Lo); \
  if (V_VT(&vSrc) != VT_DATE) \
  { \
    hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_BSTR); \
    ok(hres == S_OK && V_VT(&vDst) == VT_BSTR && \
       V_BSTR(&vDst) && V_BSTR(&vDst)[0] == '1' && V_BSTR(&vDst)[1] == '\0', \
       "->VT_BSTR hres=0x%X, type=%d (should be VT_BSTR), *bstr='%c'\n", \
       hres, V_VT(&vDst), V_BSTR(&vDst) ? *V_BSTR(&vDst) : '?'); \
  } \
  hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_DECIMAL); \
  ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL && \
     S(U(V_DECIMAL(&vDst))).sign == 0 && S(U(V_DECIMAL(&vDst))).scale == 0 && \
     V_DECIMAL(&vDst).Hi32 == 0 && U1(V_DECIMAL(&vDst)).Lo64 == (ULONGLONG)in, \
     "->VT_DECIMAL hres=0x%X, type=%d (should be VT_DECIMAL), sign=%d, scale=%d, hi=%u, lo=(%8x %8x),\n", \
     hres, V_VT(&vDst), S(U(V_DECIMAL(&vDst))).sign, S(U(V_DECIMAL(&vDst))).scale, \
     V_DECIMAL(&vDst).Hi32, S1(U1(V_DECIMAL(&vDst))).Mid32, S1(U1(V_DECIMAL(&vDst))).Lo32); \
  hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_EMPTY); \
  ok(hres == S_OK && V_VT(&vDst) == VT_EMPTY, "->VT_EMPTY hres=0x%X, type=%d (should be VT_EMPTY)\n", hres, V_VT(&vDst)); \
  hres = VariantChangeTypeEx(&vDst, &vSrc, 0, 0, VT_NULL); \
  ok(hres == S_OK && V_VT(&vDst) == VT_NULL, "->VT_NULL hres=0x%X, type=%d (should be VT_NULL)\n", hres, V_VT(&vDst)); \
  MISMATCH(VT_DISPATCH); \
  MISMATCH(VT_ERROR); \
  MISMATCH(VT_UNKNOWN); \
  MISMATCH(VT_VARIANT); \
  MISMATCH(VT_RECORD); \
  BADVAR(VT_VOID); \
  BADVAR(VT_HRESULT); \
  BADVAR(VT_SAFEARRAY); \
  BADVAR(VT_CARRAY); \
  BADVAR(VT_USERDEFINED); \
  BADVAR(VT_LPSTR); \
  BADVAR(VT_LPWSTR); \
  BADVAR(VT_PTR); \
  BADVAR(VT_INT_PTR); \
  BADVAR(VT_UINT_PTR); \
  BADVAR(VT_FILETIME); \
  BADVAR(VT_BLOB); \
  BADVAR(VT_STREAM); \
  BADVAR(VT_STORAGE); \
  BADVAR(VT_STREAMED_OBJECT); \
  BADVAR(VT_STORED_OBJECT); \
  BADVAR(VT_BLOB_OBJECT); \
  BADVAR(VT_CF); \
  BADVAR(VT_CLSID); \
  BADVAR(VT_BSTR_BLOB)

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(dispatch_invoke);

/* Early versions of oleaut32 are missing many functions */
static HRESULT (WINAPI *pVarI1FromUI1)(BYTE,signed char*);
static HRESULT (WINAPI *pVarI1FromI2)(SHORT,signed char*);
static HRESULT (WINAPI *pVarI1FromI4)(LONG,signed char*);
static HRESULT (WINAPI *pVarI1FromR4)(FLOAT,signed char*);
static HRESULT (WINAPI *pVarI1FromR8)(double,signed char*);
static HRESULT (WINAPI *pVarI1FromDate)(DATE,signed char*);
static HRESULT (WINAPI *pVarI1FromCy)(CY,signed char*);
static HRESULT (WINAPI *pVarI1FromStr)(OLECHAR*,LCID,ULONG,signed char*);
static HRESULT (WINAPI *pVarI1FromBool)(VARIANT_BOOL,signed char*);
static HRESULT (WINAPI *pVarI1FromUI2)(USHORT,signed char*);
static HRESULT (WINAPI *pVarI1FromUI4)(ULONG,signed char*);
static HRESULT (WINAPI *pVarI1FromDec)(DECIMAL*,signed char*);
static HRESULT (WINAPI *pVarI1FromI8)(LONG64,signed char*);
static HRESULT (WINAPI *pVarI1FromUI8)(ULONG64,signed char*);
static HRESULT (WINAPI *pVarUI1FromI2)(SHORT,BYTE*);
static HRESULT (WINAPI *pVarUI1FromI4)(LONG,BYTE*);
static HRESULT (WINAPI *pVarUI1FromR4)(FLOAT,BYTE*);
static HRESULT (WINAPI *pVarUI1FromR8)(double,BYTE*);
static HRESULT (WINAPI *pVarUI1FromCy)(CY,BYTE*);
static HRESULT (WINAPI *pVarUI1FromDate)(DATE,BYTE*);
static HRESULT (WINAPI *pVarUI1FromStr)(OLECHAR*,LCID,ULONG,BYTE*);
static HRESULT (WINAPI *pVarUI1FromBool)(VARIANT_BOOL,BYTE*);
static HRESULT (WINAPI *pVarUI1FromI1)(signed char,BYTE*);
static HRESULT (WINAPI *pVarUI1FromUI2)(USHORT,BYTE*);
static HRESULT (WINAPI *pVarUI1FromUI4)(ULONG,BYTE*);
static HRESULT (WINAPI *pVarUI1FromDec)(DECIMAL*,BYTE*);
static HRESULT (WINAPI *pVarUI1FromI8)(LONG64,BYTE*);
static HRESULT (WINAPI *pVarUI1FromUI8)(ULONG64,BYTE*);
static HRESULT (WINAPI *pVarUI1FromDisp)(IDispatch*,LCID,BYTE*);

static HRESULT (WINAPI *pVarI2FromUI1)(BYTE,SHORT*);
static HRESULT (WINAPI *pVarI2FromI4)(LONG,SHORT*);
static HRESULT (WINAPI *pVarI2FromR4)(FLOAT,SHORT*);
static HRESULT (WINAPI *pVarI2FromR8)(double,SHORT*);
static HRESULT (WINAPI *pVarI2FromCy)(CY,SHORT*);
static HRESULT (WINAPI *pVarI2FromDate)(DATE,SHORT*);
static HRESULT (WINAPI *pVarI2FromStr)(OLECHAR*,LCID,ULONG,SHORT*);
static HRESULT (WINAPI *pVarI2FromBool)(VARIANT_BOOL,SHORT*);
static HRESULT (WINAPI *pVarI2FromI1)(signed char,SHORT*);
static HRESULT (WINAPI *pVarI2FromUI2)(USHORT,SHORT*);
static HRESULT (WINAPI *pVarI2FromUI4)(ULONG,SHORT*);
static HRESULT (WINAPI *pVarI2FromDec)(DECIMAL*,SHORT*);
static HRESULT (WINAPI *pVarI2FromI8)(LONG64,SHORT*);
static HRESULT (WINAPI *pVarI2FromUI8)(ULONG64,SHORT*);
static HRESULT (WINAPI *pVarUI2FromUI1)(BYTE,USHORT*);
static HRESULT (WINAPI *pVarUI2FromI2)(SHORT,USHORT*);
static HRESULT (WINAPI *pVarUI2FromI4)(LONG,USHORT*);
static HRESULT (WINAPI *pVarUI2FromR4)(FLOAT,USHORT*);
static HRESULT (WINAPI *pVarUI2FromR8)(double,USHORT*);
static HRESULT (WINAPI *pVarUI2FromDate)(DATE,USHORT*);
static HRESULT (WINAPI *pVarUI2FromCy)(CY,USHORT*);
static HRESULT (WINAPI *pVarUI2FromStr)(OLECHAR*,LCID,ULONG,USHORT*);
static HRESULT (WINAPI *pVarUI2FromBool)(VARIANT_BOOL,USHORT*);
static HRESULT (WINAPI *pVarUI2FromI1)(signed char,USHORT*);
static HRESULT (WINAPI *pVarUI2FromUI4)(ULONG,USHORT*);
static HRESULT (WINAPI *pVarUI2FromDec)(DECIMAL*,USHORT*);
static HRESULT (WINAPI *pVarUI2FromI8)(LONG64,USHORT*);
static HRESULT (WINAPI *pVarUI2FromUI8)(ULONG64,USHORT*);

static HRESULT (WINAPI *pVarI4FromUI1)(BYTE,LONG*);
static HRESULT (WINAPI *pVarI4FromI2)(SHORT,LONG*);
static HRESULT (WINAPI *pVarI4FromR4)(FLOAT,LONG*);
static HRESULT (WINAPI *pVarI4FromR8)(DOUBLE,LONG*);
static HRESULT (WINAPI *pVarI4FromCy)(CY,LONG*);
static HRESULT (WINAPI *pVarI4FromDate)(DATE,LONG*);
static HRESULT (WINAPI *pVarI4FromStr)(OLECHAR*,LCID,ULONG,LONG*);
static HRESULT (WINAPI *pVarI4FromBool)(VARIANT_BOOL,LONG*);
static HRESULT (WINAPI *pVarI4FromI1)(signed char,LONG*);
static HRESULT (WINAPI *pVarI4FromUI2)(USHORT,LONG*);
static HRESULT (WINAPI *pVarI4FromUI4)(ULONG,LONG*);
static HRESULT (WINAPI *pVarI4FromDec)(DECIMAL*,LONG*);
static HRESULT (WINAPI *pVarI4FromI8)(LONG64,LONG*);
static HRESULT (WINAPI *pVarI4FromUI8)(ULONG64,LONG*);
static HRESULT (WINAPI *pVarUI4FromUI1)(BYTE,ULONG*);
static HRESULT (WINAPI *pVarUI4FromI2)(SHORT,ULONG*);
static HRESULT (WINAPI *pVarUI4FromI4)(LONG,ULONG*);
static HRESULT (WINAPI *pVarUI4FromR4)(FLOAT,ULONG*);
static HRESULT (WINAPI *pVarUI4FromR8)(DOUBLE,ULONG*);
static HRESULT (WINAPI *pVarUI4FromDate)(DATE,ULONG*);
static HRESULT (WINAPI *pVarUI4FromCy)(CY,ULONG*);
static HRESULT (WINAPI *pVarUI4FromStr)(OLECHAR*,LCID,ULONG,ULONG*);
static HRESULT (WINAPI *pVarUI4FromBool)(VARIANT_BOOL,ULONG*);
static HRESULT (WINAPI *pVarUI4FromI1)(signed char,ULONG*);
static HRESULT (WINAPI *pVarUI4FromUI2)(USHORT,ULONG*);
static HRESULT (WINAPI *pVarUI4FromDec)(DECIMAL*,ULONG*);
static HRESULT (WINAPI *pVarUI4FromI8)(LONG64,ULONG*);
static HRESULT (WINAPI *pVarUI4FromUI8)(ULONG64,ULONG*);

static HRESULT (WINAPI *pVarI8FromUI1)(BYTE,LONG64*);
static HRESULT (WINAPI *pVarI8FromI2)(SHORT,LONG64*);
static HRESULT (WINAPI *pVarI8FromR4)(FLOAT,LONG64*);
static HRESULT (WINAPI *pVarI8FromR8)(double,LONG64*);
static HRESULT (WINAPI *pVarI8FromCy)(CY,LONG64*);
static HRESULT (WINAPI *pVarI8FromDate)(DATE,LONG64*);
static HRESULT (WINAPI *pVarI8FromStr)(OLECHAR*,LCID,ULONG,LONG64*);
static HRESULT (WINAPI *pVarI8FromBool)(VARIANT_BOOL,LONG64*);
static HRESULT (WINAPI *pVarI8FromI1)(signed char,LONG64*);
static HRESULT (WINAPI *pVarI8FromUI2)(USHORT,LONG64*);
static HRESULT (WINAPI *pVarI8FromUI4)(ULONG,LONG64*);
static HRESULT (WINAPI *pVarI8FromDec)(DECIMAL*,LONG64*);
static HRESULT (WINAPI *pVarI8FromUI8)(ULONG64,LONG64*);
static HRESULT (WINAPI *pVarUI8FromI8)(LONG64,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromUI1)(BYTE,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromI2)(SHORT,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromR4)(FLOAT,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromR8)(double,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromCy)(CY,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromDate)(DATE,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromStr)(OLECHAR*,LCID,ULONG,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromBool)(VARIANT_BOOL,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromI1)(signed char,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromUI2)(USHORT,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromUI4)(ULONG,ULONG64*);
static HRESULT (WINAPI *pVarUI8FromDec)(DECIMAL*,ULONG64*);

static HRESULT (WINAPI *pVarR4FromUI1)(BYTE,float*);
static HRESULT (WINAPI *pVarR4FromI2)(SHORT,float*);
static HRESULT (WINAPI *pVarR4FromI4)(LONG,float*);
static HRESULT (WINAPI *pVarR4FromR8)(double,float*);
static HRESULT (WINAPI *pVarR4FromCy)(CY,float*);
static HRESULT (WINAPI *pVarR4FromDate)(DATE,float*);
static HRESULT (WINAPI *pVarR4FromStr)(OLECHAR*,LCID,ULONG,float*);
static HRESULT (WINAPI *pVarR4FromBool)(VARIANT_BOOL,float*);
static HRESULT (WINAPI *pVarR4FromI1)(signed char,float*);
static HRESULT (WINAPI *pVarR4FromUI2)(USHORT,float*);
static HRESULT (WINAPI *pVarR4FromUI4)(ULONG,float*);
static HRESULT (WINAPI *pVarR4FromDec)(DECIMAL*,float*);
static HRESULT (WINAPI *pVarR4FromI8)(LONG64,float*);
static HRESULT (WINAPI *pVarR4FromUI8)(ULONG64,float*);

static HRESULT (WINAPI *pVarR8FromUI1)(BYTE,double*);
static HRESULT (WINAPI *pVarR8FromI2)(SHORT,double*);
static HRESULT (WINAPI *pVarR8FromI4)(LONG,double*);
static HRESULT (WINAPI *pVarR8FromR4)(FLOAT,double*);
static HRESULT (WINAPI *pVarR8FromCy)(CY,double*);
static HRESULT (WINAPI *pVarR8FromDate)(DATE,double*);
static HRESULT (WINAPI *pVarR8FromStr)(OLECHAR*,LCID,ULONG,double*);
static HRESULT (WINAPI *pVarR8FromBool)(VARIANT_BOOL,double*);
static HRESULT (WINAPI *pVarR8FromI1)(signed char,double*);
static HRESULT (WINAPI *pVarR8FromUI2)(USHORT,double*);
static HRESULT (WINAPI *pVarR8FromUI4)(ULONG,double*);
static HRESULT (WINAPI *pVarR8FromDec)(DECIMAL*,double*);
static HRESULT (WINAPI *pVarR8FromI8)(LONG64,double*);
static HRESULT (WINAPI *pVarR8FromUI8)(ULONG64,double*);
static HRESULT (WINAPI *pVarR8Round)(double,int,double*);

static HRESULT (WINAPI *pVarDateFromUI1)(BYTE,DATE*);
static HRESULT (WINAPI *pVarDateFromI2)(SHORT,DATE*);
static HRESULT (WINAPI *pVarDateFromI4)(LONG,DATE*);
static HRESULT (WINAPI *pVarDateFromR4)(FLOAT,DATE*);
static HRESULT (WINAPI *pVarDateFromCy)(CY,DATE*);
static HRESULT (WINAPI *pVarDateFromR8)(double,DATE*);
static HRESULT (WINAPI *pVarDateFromStr)(OLECHAR*,LCID,ULONG,DATE*);
static HRESULT (WINAPI *pVarDateFromBool)(VARIANT_BOOL,DATE*);
static HRESULT (WINAPI *pVarDateFromI1)(signed char,DATE*);
static HRESULT (WINAPI *pVarDateFromUI2)(USHORT,DATE*);
static HRESULT (WINAPI *pVarDateFromUI4)(ULONG,DATE*);
static HRESULT (WINAPI *pVarDateFromDec)(DECIMAL*,DATE*);
static HRESULT (WINAPI *pVarDateFromI8)(LONG64,DATE*);
static HRESULT (WINAPI *pVarDateFromUI8)(ULONG64,DATE*);

static HRESULT (WINAPI *pVarCyFromUI1)(BYTE,CY*);
static HRESULT (WINAPI *pVarCyFromI2)(SHORT,CY*);
static HRESULT (WINAPI *pVarCyFromI4)(LONG,CY*);
static HRESULT (WINAPI *pVarCyFromR4)(FLOAT,CY*);
static HRESULT (WINAPI *pVarCyFromR8)(double,CY*);
static HRESULT (WINAPI *pVarCyFromDate)(DATE,CY*);
static HRESULT (WINAPI *pVarCyFromBool)(VARIANT_BOOL,CY*);
static HRESULT (WINAPI *pVarCyFromI1)(signed char,CY*);
static HRESULT (WINAPI *pVarCyFromUI2)(USHORT,CY*);
static HRESULT (WINAPI *pVarCyFromUI4)(ULONG,CY*);
static HRESULT (WINAPI *pVarCyFromDec)(DECIMAL*,CY*);
static HRESULT (WINAPI *pVarCyFromI8)(LONG64,CY*);
static HRESULT (WINAPI *pVarCyFromUI8)(ULONG64,CY*);
static HRESULT (WINAPI *pVarCyAdd)(const CY,const CY,CY*);
static HRESULT (WINAPI *pVarCyMul)(const CY,const CY,CY*);
static HRESULT (WINAPI *pVarCyMulI4)(const CY,LONG,CY*);
static HRESULT (WINAPI *pVarCySub)(const CY,const CY,CY*);
static HRESULT (WINAPI *pVarCyAbs)(const CY,CY*);
static HRESULT (WINAPI *pVarCyFix)(const CY,CY*);
static HRESULT (WINAPI *pVarCyInt)(const CY,CY*);
static HRESULT (WINAPI *pVarCyNeg)(const CY,CY*);
static HRESULT (WINAPI *pVarCyRound)(const CY,int,CY*);
static HRESULT (WINAPI *pVarCyCmp)(const CY,const CY);
static HRESULT (WINAPI *pVarCyCmpR8)(const CY,double);
static HRESULT (WINAPI *pVarCyMulI8)(const CY,LONG64,CY*);

static HRESULT (WINAPI *pVarDecFromUI1)(BYTE,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromI2)(SHORT,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromI4)(LONG,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromI8)(LONG64,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromR4)(FLOAT,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromR8)(DOUBLE,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromDate)(DATE,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromStr)(OLECHAR*,LCID,ULONG,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromBool)(VARIANT_BOOL,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromI1)(signed char,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromUI2)(USHORT,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromUI4)(ULONG,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromUI8)(ULONG64,DECIMAL*);
static HRESULT (WINAPI *pVarDecFromCy)(CY,DECIMAL*);
static HRESULT (WINAPI *pVarDecAbs)(const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecAdd)(const DECIMAL*,const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecSub)(const DECIMAL*,const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecMul)(const DECIMAL*,const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecDiv)(const DECIMAL*,const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecCmp)(const DECIMAL*,const DECIMAL*);
static HRESULT (WINAPI *pVarDecCmpR8)(const DECIMAL*,double);
static HRESULT (WINAPI *pVarDecNeg)(const DECIMAL*,DECIMAL*);
static HRESULT (WINAPI *pVarDecRound)(const DECIMAL*,int,DECIMAL*);

static HRESULT (WINAPI *pVarBoolFromUI1)(BYTE,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromI2)(SHORT,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromI4)(LONG,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromR4)(FLOAT,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromR8)(DOUBLE,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromDate)(DATE,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromCy)(CY,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromStr)(OLECHAR*,LCID,ULONG,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromI1)(signed char,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromUI2)(USHORT,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromUI4)(ULONG,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromDec)(DECIMAL*,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromI8)(LONG64,VARIANT_BOOL*);
static HRESULT (WINAPI *pVarBoolFromUI8)(ULONG64,VARIANT_BOOL*);

static HRESULT (WINAPI *pVarBstrFromR4)(FLOAT,LCID,ULONG,BSTR*);
static HRESULT (WINAPI *pVarBstrFromDate)(DATE,LCID,ULONG,BSTR*);
static HRESULT (WINAPI *pVarBstrFromCy)(CY,LCID,ULONG,BSTR*);
static HRESULT (WINAPI *pVarBstrFromDec)(DECIMAL*,LCID,ULONG,BSTR*);
static HRESULT (WINAPI *pVarBstrCmp)(BSTR,BSTR,LCID,ULONG);
static HRESULT (WINAPI *pVarBstrCat)(BSTR,BSTR,BSTR*);

static INT (WINAPI *pSystemTimeToVariantTime)(LPSYSTEMTIME,double*);
static void (WINAPI *pClearCustData)(LPCUSTDATA);

/* Internal representation of a BSTR */
typedef struct tagINTERNAL_BSTR
{
  DWORD   dwLen;
  OLECHAR szString[1];
} INTERNAL_BSTR, *LPINTERNAL_BSTR;

typedef struct
{
  IDispatch IDispatch_iface;
  LONG ref;
  VARTYPE vt;
  BOOL bFailInvoke;
} DummyDispatch;

static inline DummyDispatch *impl_from_IDispatch(IDispatch *iface)
{
  return CONTAINING_RECORD(iface, DummyDispatch, IDispatch_iface);
}

static ULONG WINAPI DummyDispatch_AddRef(IDispatch *iface)
{
  DummyDispatch *This = impl_from_IDispatch(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DummyDispatch_Release(IDispatch *iface)
{
  DummyDispatch *This = impl_from_IDispatch(iface);
  return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI DummyDispatch_QueryInterface(IDispatch *iface,
                                                   REFIID riid,
                                                   void** ppvObject)
{
  *ppvObject = NULL;

  if (IsEqualIID(riid, &IID_IDispatch) ||
      IsEqualIID(riid, &IID_IUnknown))
  {
      *ppvObject = iface;
      IDispatch_AddRef(iface);
  }

  return *ppvObject ? S_OK : E_NOINTERFACE;
}

static HRESULT WINAPI DummyDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
  ok(0, "Unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_GetTypeInfo(IDispatch *iface, UINT tinfo, LCID lcid, ITypeInfo **ti)
{
  ok(0, "Unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names,
    UINT cnames, LCID lcid, DISPID *dispid)
{
  ok(0, "Unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_Invoke(IDispatch *iface,
                                           DISPID dispid, REFIID riid,
                                           LCID lcid, WORD wFlags,
                                           DISPPARAMS *params,
                                           VARIANT *res,
                                           EXCEPINFO *ei,
                                           UINT *arg_err)
{
  DummyDispatch *This = impl_from_IDispatch(iface);

  CHECK_EXPECT(dispatch_invoke);

  ok(dispid == DISPID_VALUE, "got dispid %d\n", dispid);
  ok(IsEqualIID(riid, &IID_NULL), "go riid %s\n", wine_dbgstr_guid(riid));
  ok(wFlags == DISPATCH_PROPERTYGET, "Flags wrong\n");

  ok(params->rgvarg == NULL, "got %p\n", params->rgvarg);
  ok(params->rgdispidNamedArgs == NULL, "got %p\n", params->rgdispidNamedArgs);
  ok(params->cArgs == 0, "got %d\n", params->cArgs);
  ok(params->cNamedArgs == 0, "got %d\n", params->cNamedArgs);

  ok(res != NULL, "got %p\n", res);
  ok(V_VT(res) == VT_EMPTY, "got %d\n", V_VT(res));
  ok(ei == NULL, "got %p\n", ei);
  ok(arg_err == NULL, "got %p\n", arg_err);

  if (This->bFailInvoke)
    return E_OUTOFMEMORY;

  V_VT(res) = This->vt;
  if (This->vt == VT_UI1)
      V_UI1(res) = 1;
  else
      memset(res, 0, sizeof(*res));

  return S_OK;
}

static const IDispatchVtbl DummyDispatch_VTable =
{
  DummyDispatch_QueryInterface,
  DummyDispatch_AddRef,
  DummyDispatch_Release,
  DummyDispatch_GetTypeInfoCount,
  DummyDispatch_GetTypeInfo,
  DummyDispatch_GetIDsOfNames,
  DummyDispatch_Invoke
};

static void init_test_dispatch(LONG ref, VARTYPE vt, DummyDispatch *dispatch)
{
    dispatch->IDispatch_iface.lpVtbl = &DummyDispatch_VTable;
    dispatch->ref = ref;
    dispatch->vt = vt;
    dispatch->bFailInvoke = FALSE;
}

/*
 * VT_I1/VT_UI1
 */

#undef CONV_TYPE
#define CONV_TYPE signed char
#undef EXPECTRES
#define EXPECTRES(res, x) _EXPECTRES(res, x, "%d")

static void test_VarI1FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarI1FromI2);
  OVERFLOWRANGE(VarI1FromI2, -32768, -128);
  CONVERTRANGE(VarI1FromI2, -128, 128);
  OVERFLOWRANGE(VarI1FromI2, 129, 32768);
}

static void test_VarI1FromI4(void)
{
  CONVVARS(LONG);
  int i;

  CHECKPTR(VarI1FromI4);
  CONVERT(VarI1FromI4, -129); EXPECT_OVERFLOW;
  CONVERTRANGE(VarI1FromI4, -128, 128);
  CONVERT(VarI1FromI4, 128);  EXPECT_OVERFLOW;
}

static void test_VarI1FromI8(void)
{
  CONVVARS(LONG64);
  int i;

  CHECKPTR(VarI1FromI8);
  CONVERT(VarI1FromI8, -129);   EXPECT_OVERFLOW;
  CONVERTRANGE(VarI1FromI8, -127, 128);
  CONVERT(VarI1FromI8, 128);    EXPECT_OVERFLOW;
}

static void test_VarI1FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarI1FromUI1);
  CONVERTRANGE(VarI1FromUI1, 0, 127);
  OVERFLOWRANGE(VarI1FromUI1, 128, 255);
}

static void test_VarI1FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarI1FromUI2);
  CONVERTRANGE(VarI1FromUI2, 0, 127);
  OVERFLOWRANGE(VarI1FromUI2, 128, 32768);
}

static void test_VarI1FromUI4(void)
{
  CONVVARS(ULONG);
  int i;

  CHECKPTR(VarI1FromUI4);
  CONVERTRANGE(VarI1FromUI4, 0, 127);
  CONVERT(VarI1FromUI4, 128); EXPECT_OVERFLOW;
}

static void test_VarI1FromUI8(void)
{
  CONVVARS(ULONG64);
  int i;

  CHECKPTR(VarI1FromUI8);
  CONVERTRANGE(VarI1FromUI8, 0, 127);
  CONVERT(VarI1FromUI8, 128); EXPECT_OVERFLOW;
}

static void test_VarI1FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarI1FromBool);
  /* Note that conversions from bool wrap around! */
  CONVERT(VarI1FromBool, -129);  EXPECT(127);
  CONVERTRANGE(VarI1FromBool, -128, 128);
  CONVERT(VarI1FromBool, 128); EXPECT(-128);
}

static void test_VarI1FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarI1FromR4);
  CONVERT(VarI1FromR4, -129.0f); EXPECT_OVERFLOW;
  CONVERT(VarI1FromR4, -128.51f); EXPECT_OVERFLOW;
  CONVERT(VarI1FromR4, -128.5f); EXPECT(-128);
  CONVERT(VarI1FromR4, -128.0f); EXPECT(-128);
  CONVERT(VarI1FromR4, -1.0f);   EXPECT(-1);
  CONVERT(VarI1FromR4, 0.0f);    EXPECT(0);
  CONVERT(VarI1FromR4, 1.0f);    EXPECT(1);
  CONVERT(VarI1FromR4, 127.0f);  EXPECT(127);
  CONVERT(VarI1FromR4, 127.49f);  EXPECT(127);
  CONVERT(VarI1FromR4, 127.5f);  EXPECT_OVERFLOW;
  CONVERT(VarI1FromR4, 128.0f);  EXPECT_OVERFLOW;

  CONVERT(VarI1FromR4, -1.5f); EXPECT(-2);
  CONVERT(VarI1FromR4, -0.6f); EXPECT(-1);
  CONVERT(VarI1FromR4, -0.5f); EXPECT(0);
  CONVERT(VarI1FromR4, -0.4f); EXPECT(0);
  CONVERT(VarI1FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarI1FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarI1FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarI1FromR4, 1.5f);  EXPECT(2);
}

static void test_VarI1FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarI1FromR8);
  CONVERT(VarI1FromR8, -129.0); EXPECT_OVERFLOW;
  CONVERT(VarI1FromR8, -128.51); EXPECT_OVERFLOW;
  CONVERT(VarI1FromR8, -128.5); EXPECT(-128);
  CONVERT(VarI1FromR8, -128.0); EXPECT(-128);
  CONVERT(VarI1FromR8, -1.0);   EXPECT(-1);
  CONVERT(VarI1FromR8, 0.0);    EXPECT(0);
  CONVERT(VarI1FromR8, 1.0);    EXPECT(1);
  CONVERT(VarI1FromR8, 127.0);  EXPECT(127);
  CONVERT(VarI1FromR8, 127.49);  EXPECT(127);
  CONVERT(VarI1FromR8, 127.5);  EXPECT_OVERFLOW;
  CONVERT(VarI1FromR8, 128.0);  EXPECT_OVERFLOW;

  CONVERT(VarI1FromR8, -1.5); EXPECT(-2);
  CONVERT(VarI1FromR8, -0.6); EXPECT(-1);
  CONVERT(VarI1FromR8, -0.5); EXPECT(0);
  CONVERT(VarI1FromR8, -0.4); EXPECT(0);
  CONVERT(VarI1FromR8, 0.4);  EXPECT(0);
  CONVERT(VarI1FromR8, 0.5);  EXPECT(0);
  CONVERT(VarI1FromR8, 0.6);  EXPECT(1);
  CONVERT(VarI1FromR8, 1.5);  EXPECT(2);
}

static void test_VarI1FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarI1FromDate);
  CONVERT(VarI1FromDate, -129.0); EXPECT_OVERFLOW;
  CONVERT(VarI1FromDate, -128.0); EXPECT(-128);
  CONVERT(VarI1FromDate, -1.0);   EXPECT(-1);
  CONVERT(VarI1FromDate, 0.0);    EXPECT(0);
  CONVERT(VarI1FromDate, 1.0);    EXPECT(1);
  CONVERT(VarI1FromDate, 127.0);  EXPECT(127);
  CONVERT(VarI1FromDate, 128.0);  EXPECT_OVERFLOW;

  CONVERT(VarI1FromDate, -1.5); EXPECT(-2);
  CONVERT(VarI1FromDate, -0.6); EXPECT(-1);
  CONVERT(VarI1FromDate, -0.5); EXPECT(0);
  CONVERT(VarI1FromDate, -0.4); EXPECT(0);
  CONVERT(VarI1FromDate, 0.4);  EXPECT(0);
  CONVERT(VarI1FromDate, 0.5);  EXPECT(0);
  CONVERT(VarI1FromDate, 0.6);  EXPECT(1);
  CONVERT(VarI1FromDate, 1.5);  EXPECT(2);
}

static void test_VarI1FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarI1FromCy);
  CONVERT_CY(VarI1FromCy,-129); EXPECT_OVERFLOW;
  CONVERT_CY(VarI1FromCy,-128); EXPECT(128);
  CONVERT_CY(VarI1FromCy,-1);   EXPECT(-1);
  CONVERT_CY(VarI1FromCy,0);    EXPECT(0);
  CONVERT_CY(VarI1FromCy,1);    EXPECT(1);
  CONVERT_CY(VarI1FromCy,127);  EXPECT(127);
  CONVERT_CY(VarI1FromCy,128);  EXPECT_OVERFLOW;

  CONVERT_CY(VarI1FromCy,-1.5); EXPECT(-2);
  CONVERT_CY(VarI1FromCy,-0.6); EXPECT(-1);
  CONVERT_CY(VarI1FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarI1FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarI1FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarI1FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarI1FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarI1FromCy,1.5);  EXPECT(2);
}

static void test_VarI1FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarI1FromDec);

  CONVERT_BADDEC(VarI1FromDec);

  CONVERT_DEC(VarI1FromDec,0,0x80,0,129); EXPECT_OVERFLOW;
  CONVERT_DEC(VarI1FromDec,0,0x80,0,128); EXPECT(-128);
  CONVERT_DEC(VarI1FromDec,0,0x80,0,1);   EXPECT(-1);
  CONVERT_DEC(VarI1FromDec,0,0,0,0);      EXPECT(0);
  CONVERT_DEC(VarI1FromDec,0,0,0,1);      EXPECT(1);
  CONVERT_DEC(VarI1FromDec,0,0,0,127);    EXPECT(127);
  CONVERT_DEC(VarI1FromDec,0,0,0,128);    EXPECT_OVERFLOW;

  CONVERT_DEC(VarI1FromDec,2,0x80,0,12800); EXPECT(-128);
  CONVERT_DEC(VarI1FromDec,2,0,0,12700);    EXPECT(127);
}

static void test_VarI1FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarI1FromStr);

  CONVERT_STR(VarI1FromStr,NULL, 0);   EXPECT_MISMATCH;
  CONVERT_STR(VarI1FromStr,"0", 0);    EXPECT(0);
  CONVERT_STR(VarI1FromStr,"-129", 0); EXPECT_OVERFLOW;
  CONVERT_STR(VarI1FromStr,"-128", 0); EXPECT(-128);
  CONVERT_STR(VarI1FromStr,"127", 0);  EXPECT(127);
  CONVERT_STR(VarI1FromStr,"128", 0);  EXPECT_OVERFLOW;

  CONVERT_STR(VarI1FromStr,"-1.5", LOCALE_NOUSEROVERRIDE); EXPECT(-2);
  CONVERT_STR(VarI1FromStr,"-0.6", LOCALE_NOUSEROVERRIDE); EXPECT(-1);
  CONVERT_STR(VarI1FromStr,"-0.5", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI1FromStr,"-0.4", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI1FromStr,"0.4", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI1FromStr,"0.5", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI1FromStr,"0.6", LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarI1FromStr,"1.5", LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarI1Copy(void)
{
  COPYTEST(1, VT_I1, V_I1(&vSrc), V_I1(&vDst), V_I1REF(&vSrc), V_I1REF(&vDst), "%d");
}

static void test_VarI1ChangeTypeEx(void)
{
  HRESULT hres;
  signed char in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_I1, V_I1, "%d");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_I1, V_I1, "%d", VT_UI1, V_UI1);
}

#undef CONV_TYPE
#define CONV_TYPE BYTE

static void test_VarUI1FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarUI1FromI1);
  OVERFLOWRANGE(VarUI1FromI1, -128, 0);
  CONVERTRANGE(VarUI1FromI1, 0, 128);
}

static void test_VarUI1FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarUI1FromI2);
  OVERFLOWRANGE(VarUI1FromI2, -32768, 0);
  CONVERTRANGE(VarUI1FromI2, 0, 256);
  OVERFLOWRANGE(VarUI1FromI2, 256, 32768);
}

static void test_VarUI1FromI4(void)
{
  CONVVARS(LONG);
  int i;

  CHECKPTR(VarUI1FromI4);
  CONVERT(VarUI1FromI4, -1);  EXPECT_OVERFLOW;
  CONVERTRANGE(VarUI1FromI4, 0, 256);
  CONVERT(VarUI1FromI4, 256); EXPECT_OVERFLOW;
}

static void test_VarUI1FromI8(void)
{
  CONVVARS(LONG64);
  int i;

  CHECKPTR(VarUI1FromI8);
  CONVERT(VarUI1FromI8, -1);  EXPECT_OVERFLOW;
  CONVERTRANGE(VarUI1FromI8, 0, 256);
  CONVERT(VarUI1FromI8, 256); EXPECT_OVERFLOW;
}

static void test_VarUI1FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarUI1FromUI2);
  CONVERTRANGE(VarUI1FromUI2, 0, 256);
  OVERFLOWRANGE(VarUI1FromUI2, 256, 65536);
}

static void test_VarUI1FromUI4(void)
{
  CONVVARS(ULONG);
  int i;

  CHECKPTR(VarUI1FromUI4);
  CONVERTRANGE(VarUI1FromUI4, 0, 256);
  CONVERT(VarUI1FromUI4, 256); EXPECT_OVERFLOW;
}

static void test_VarUI1FromUI8(void)
{
  CONVVARS(ULONG64);
  int i;

  CHECKPTR(VarUI1FromUI8);
  CONVERTRANGE(VarUI1FromUI8, 0, 256);
  CONVERT(VarUI1FromUI8, 256); EXPECT_OVERFLOW;
}

static void test_VarUI1FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarUI1FromBool);
  /* Note that conversions from bool overflow! */
  CONVERT(VarUI1FromBool, -1); EXPECT(255);
  CONVERTRANGE(VarUI1FromBool, 0, 256);
  CONVERT(VarUI1FromBool, 256); EXPECT(0);
}

static void test_VarUI1FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarUI1FromR4);
  CONVERT(VarUI1FromR4, -1.0f);  EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR4, -0.51f);  EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR4, -0.5f);   EXPECT(0);
  CONVERT(VarUI1FromR4, 0.0f);   EXPECT(0);
  CONVERT(VarUI1FromR4, 1.0f);   EXPECT(1);
  CONVERT(VarUI1FromR4, 255.0f); EXPECT(255);
  CONVERT(VarUI1FromR4, 255.49f); EXPECT(255);
  CONVERT(VarUI1FromR4, 255.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR4, 256.0f); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI1FromR4, -1.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR4, -0.6f); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR4, -0.5f); EXPECT(0);
  CONVERT(VarUI1FromR4, -0.4f); EXPECT(0);
  CONVERT(VarUI1FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarUI1FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarUI1FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarUI1FromR4, 1.5f);  EXPECT(2);
}

static void test_VarUI1FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarUI1FromR8);
  CONVERT(VarUI1FromR8, -1.0);  EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR8, -0.51);  EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR8, -0.5);   EXPECT(0);
  CONVERT(VarUI1FromR8, 0.0);   EXPECT(0);
  CONVERT(VarUI1FromR8, 1.0);   EXPECT(1);
  CONVERT(VarUI1FromR8, 255.0); EXPECT(255);
  CONVERT(VarUI1FromR8, 255.49); EXPECT(255);
  CONVERT(VarUI1FromR8, 255.5); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR8, 256.0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI1FromR8, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR8, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromR8, -0.5); EXPECT(0);
  CONVERT(VarUI1FromR8, -0.4); EXPECT(0);
  CONVERT(VarUI1FromR8, 0.4);  EXPECT(0);
  CONVERT(VarUI1FromR8, 0.5);  EXPECT(0);
  CONVERT(VarUI1FromR8, 0.6);  EXPECT(1);
  CONVERT(VarUI1FromR8, 1.5);  EXPECT(2);
}

static void test_VarUI1FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarUI1FromDate);
  CONVERT(VarUI1FromDate, -1.0);  EXPECT_OVERFLOW;
  CONVERT(VarUI1FromDate, 0.0);   EXPECT(0);
  CONVERT(VarUI1FromDate, 1.0);   EXPECT(1);
  CONVERT(VarUI1FromDate, 255.0); EXPECT(255);
  CONVERT(VarUI1FromDate, 256.0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI1FromDate, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromDate, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI1FromDate, -0.5); EXPECT(0);
  CONVERT(VarUI1FromDate, -0.4); EXPECT(0);
  CONVERT(VarUI1FromDate, 0.4);  EXPECT(0);
  CONVERT(VarUI1FromDate, 0.5);  EXPECT(0);
  CONVERT(VarUI1FromDate, 0.6);  EXPECT(1);
  CONVERT(VarUI1FromDate, 1.5);  EXPECT(2);
}

static void test_VarUI1FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarUI1FromCy);
  CONVERT_CY(VarUI1FromCy,-1);  EXPECT_OVERFLOW;
  CONVERT_CY(VarUI1FromCy,0);   EXPECT(0);
  CONVERT_CY(VarUI1FromCy,1);   EXPECT(1);
  CONVERT_CY(VarUI1FromCy,255); EXPECT(255);
  CONVERT_CY(VarUI1FromCy,256); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_CY(VarUI1FromCy,-1.5); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI1FromCy,-0.6); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI1FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarUI1FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarUI1FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarUI1FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarUI1FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarUI1FromCy,1.5);  EXPECT(2);
}

static void test_VarUI1FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarUI1FromDec);

  CONVERT_BADDEC(VarUI1FromDec);

  CONVERT_DEC(VarUI1FromDec,0,0x80,0,1); EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI1FromDec,0,0,0,0);    EXPECT(0);
  CONVERT_DEC(VarUI1FromDec,0,0,0,1);    EXPECT(1);
  CONVERT_DEC(VarUI1FromDec,0,0,0,255);  EXPECT(255);
  CONVERT_DEC(VarUI1FromDec,0,0,0,256);  EXPECT_OVERFLOW;

  CONVERT_DEC(VarUI1FromDec,2,0x80,0,100); EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI1FromDec,2,0,0,25500);  EXPECT(255);
}

static void test_VarUI1FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarUI1FromStr);

  CONVERT_STR(VarUI1FromStr,NULL, 0);   EXPECT_MISMATCH;
  CONVERT_STR(VarUI1FromStr,"0", 0);    EXPECT(0);
  CONVERT_STR(VarUI1FromStr,"-1", 0);   EXPECT_OVERFLOW;
  CONVERT_STR(VarUI1FromStr,"255", 0);  EXPECT(255);
  CONVERT_STR(VarUI1FromStr,"256", 0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_STR(VarUI1FromStr,"-1.5", LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI1FromStr,"-0.6", LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI1FromStr,"-0.5", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI1FromStr,"-0.4", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI1FromStr,"0.4", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI1FromStr,"0.5", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI1FromStr,"0.6", LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarUI1FromStr,"1.5", LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarUI1FromDisp(void)
{
  DummyDispatch dispatch;
  CONVVARS(LCID);
  VARIANTARG vSrc, vDst;

  CHECKPTR(VarUI1FromDisp);

  /* FIXME
   * Conversions from IDispatch should get the default 'value' property
   * from the IDispatch pointer and return it. The following tests this.
   * However, I can't get these tests to return a valid value under native
   * oleaut32, regardless of the value returned in response to the Invoke()
   * call (early versions of oleaut32 call AddRef/Release, but not Invoke.
   * I'm obviously missing something, as these conversions work fine
   * when called through VBA on an object to get its default value property.
   *
   * Should this test be corrected so that it works under native it should be
   * generalised and the remaining types checked as well.
   */
  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  VariantInit(&vSrc);
  VariantInit(&vDst);

  init_test_dispatch(1, VT_UI1, &dispatch);
  V_VT(&vSrc) = VT_DISPATCH;
  V_DISPATCH(&vSrc) = &dispatch.IDispatch_iface;

  SET_EXPECT(dispatch_invoke);
  out = 10;
  hres = pVarUI1FromDisp(&dispatch.IDispatch_iface, in, &out);
  ok(broken(hres == DISP_E_BADVARTYPE) || hres == S_OK, "got 0x%08x\n", hres);
  ok(broken(out == 10) || out == 1, "got %d\n", out);
  CHECK_CALLED(dispatch_invoke);

  SET_EXPECT(dispatch_invoke);
  V_VT(&vDst) = VT_EMPTY;
  V_UI1(&vDst) = 0;
  hres = VariantChangeTypeEx(&vDst, &vSrc, in, 0, VT_UI1);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(V_VT(&vDst) == VT_UI1, "got %d\n", V_VT(&vDst));
  ok(V_UI1(&vDst) == 1, "got %d\n", V_UI1(&vDst));
  CHECK_CALLED(dispatch_invoke);

  dispatch.bFailInvoke = TRUE;

  SET_EXPECT(dispatch_invoke);
  out = 10;
  hres = pVarUI1FromDisp(&dispatch.IDispatch_iface, in, &out);
  ok(hres == DISP_E_TYPEMISMATCH, "got 0x%08x\n", hres);
  ok(out == 10, "got %d\n", out);
  CHECK_CALLED(dispatch_invoke);

  SET_EXPECT(dispatch_invoke);
  V_VT(&vDst) = VT_EMPTY;
  hres = VariantChangeTypeEx(&vDst, &vSrc, in, 0, VT_UI1);
  ok(hres == DISP_E_TYPEMISMATCH, "got 0x%08x\n", hres);
  ok(V_VT(&vDst) == VT_EMPTY, "got %d\n", V_VT(&vDst));
  CHECK_CALLED(dispatch_invoke);
}

static void test_VarUI1Copy(void)
{
  COPYTEST(1, VT_UI1, V_UI1(&vSrc), V_UI1(&vDst), V_UI1REF(&vSrc), V_UI1REF(&vDst), "%d");
}

static void test_VarUI1ChangeTypeEx(void)
{
  HRESULT hres;
  BYTE in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_UI1, V_UI1, "%d");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_UI1, V_UI1, "%d", VT_I1, V_I1);
}

/*
 * VT_I2/VT_UI2
 */

#undef CONV_TYPE
#define CONV_TYPE SHORT

static void test_VarI2FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarI2FromI1);
  CONVERTRANGE(VarI2FromI1, -128, 128);
}

static void test_VarI2FromI4(void)
{
  CONVVARS(LONG);
  int i;

  CHECKPTR(VarI2FromI4);
  CONVERT(VarI2FromI4, -32769); EXPECT_OVERFLOW;
  CONVERTRANGE(VarI2FromI4, -32768, 32768);
  CONVERT(VarI2FromI4, 32768);  EXPECT_OVERFLOW;
}

static void test_VarI2FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarI2FromI8);
  CONVERT(VarI2FromI8, -32769); EXPECT_OVERFLOW;
  CONVERT(VarI2FromI8, -32768); EXPECT(-32768);
  CONVERT(VarI2FromI8, 32767);  EXPECT(32767);
  CONVERT(VarI2FromI8, 32768);  EXPECT_OVERFLOW;
}

static void test_VarI2FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarI2FromUI1);
  CONVERTRANGE(VarI2FromUI1, 0, 256);
}

static void test_VarI2FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarI2FromUI2);
  CONVERTRANGE(VarI2FromUI2, 0, 32768);
  CONVERT(VarI2FromUI2, 32768); EXPECT_OVERFLOW;
}

static void test_VarI2FromUI4(void)
{
  CONVVARS(ULONG);
  int i;

  CHECKPTR(VarI2FromUI4);
  CONVERTRANGE(VarI2FromUI4, 0, 32768);
  CONVERT(VarI2FromUI4, 32768); EXPECT_OVERFLOW;
}

static void test_VarI2FromUI8(void)
{
  CONVVARS(ULONG64);
  int i;

  CHECKPTR(VarI2FromUI8);
  CONVERTRANGE(VarI2FromUI8, 0, 32768);
  CONVERT(VarI2FromUI8, 32768); EXPECT_OVERFLOW;
}

static void test_VarI2FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarI2FromBool);
  CONVERTRANGE(VarI2FromBool, -32768, 32768);
}

static void test_VarI2FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarI2FromR4);
  CONVERT(VarI2FromR4, -32769.0f); EXPECT_OVERFLOW;
  CONVERT(VarI2FromR4, -32768.51f); EXPECT_OVERFLOW;
  CONVERT(VarI2FromR4, -32768.5f); EXPECT(-32768);
  CONVERT(VarI2FromR4, -32768.0f); EXPECT(-32768);
  CONVERT(VarI2FromR4, -1.0f);     EXPECT(-1);
  CONVERT(VarI2FromR4, 0.0f);      EXPECT(0);
  CONVERT(VarI2FromR4, 1.0f);      EXPECT(1);
  CONVERT(VarI2FromR4, 32767.0f);  EXPECT(32767);
  CONVERT(VarI2FromR4, 32767.49f);  EXPECT(32767);
  CONVERT(VarI2FromR4, 32767.5f);  EXPECT_OVERFLOW;
  CONVERT(VarI2FromR4, 32768.0f);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarI2FromR4, -1.5f); EXPECT(-2);
  CONVERT(VarI2FromR4, -0.6f); EXPECT(-1);
  CONVERT(VarI2FromR4, -0.5f); EXPECT(0);
  CONVERT(VarI2FromR4, -0.4f); EXPECT(0);
  CONVERT(VarI2FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarI2FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarI2FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarI2FromR4, 1.5f);  EXPECT(2);
}

static void test_VarI2FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarI2FromR8);
  CONVERT(VarI2FromR8, -32769.0); EXPECT_OVERFLOW;
  CONVERT(VarI2FromR8, -32768.51); EXPECT_OVERFLOW;
  CONVERT(VarI2FromR8, -32768.5); EXPECT(-32768);
  CONVERT(VarI2FromR8, -32768.0); EXPECT(-32768);
  CONVERT(VarI2FromR8, -1.0);     EXPECT(-1);
  CONVERT(VarI2FromR8, 0.0);      EXPECT(0);
  CONVERT(VarI2FromR8, 1.0);      EXPECT(1);
  CONVERT(VarI2FromR8, 32767.0);  EXPECT(32767);
  CONVERT(VarI2FromR8, 32767.49);  EXPECT(32767);
  CONVERT(VarI2FromR8, 32767.5);  EXPECT_OVERFLOW;
  CONVERT(VarI2FromR8, 32768.0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarI2FromR8, -1.5); EXPECT(-2);
  CONVERT(VarI2FromR8, -0.6); EXPECT(-1);
  CONVERT(VarI2FromR8, -0.5); EXPECT(0);
  CONVERT(VarI2FromR8, -0.4); EXPECT(0);
  CONVERT(VarI2FromR8, 0.4);  EXPECT(0);
  CONVERT(VarI2FromR8, 0.5);  EXPECT(0);
  CONVERT(VarI2FromR8, 0.6);  EXPECT(1);
  CONVERT(VarI2FromR8, 1.5);  EXPECT(2);
}

static void test_VarI2FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarI2FromDate);
  CONVERT(VarI2FromDate, -32769.0); EXPECT_OVERFLOW;
  CONVERT(VarI2FromDate, -32768.0); EXPECT(-32768);
  CONVERT(VarI2FromDate, -1.0);   EXPECT(-1);
  CONVERT(VarI2FromDate, 0.0);    EXPECT(0);
  CONVERT(VarI2FromDate, 1.0);    EXPECT(1);
  CONVERT(VarI2FromDate, 32767.0);  EXPECT(32767);
  CONVERT(VarI2FromDate, 32768.0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarI2FromDate, -1.5); EXPECT(-2);
  CONVERT(VarI2FromDate, -0.6); EXPECT(-1);
  CONVERT(VarI2FromDate, -0.5); EXPECT(0);
  CONVERT(VarI2FromDate, -0.4); EXPECT(0);
  CONVERT(VarI2FromDate, 0.4);  EXPECT(0);
  CONVERT(VarI2FromDate, 0.5);  EXPECT(0);
  CONVERT(VarI2FromDate, 0.6);  EXPECT(1);
  CONVERT(VarI2FromDate, 1.5);  EXPECT(2);
}

static void test_VarI2FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarI2FromCy);
  CONVERT_CY(VarI2FromCy,-32769); EXPECT_OVERFLOW;
  CONVERT_CY(VarI2FromCy,-32768); EXPECT(32768);
  CONVERT_CY(VarI2FromCy,-1);     EXPECT(-1);
  CONVERT_CY(VarI2FromCy,0);      EXPECT(0);
  CONVERT_CY(VarI2FromCy,1);      EXPECT(1);
  CONVERT_CY(VarI2FromCy,32767);  EXPECT(32767);
  CONVERT_CY(VarI2FromCy,32768);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_CY(VarI2FromCy,-1.5); EXPECT(-2);
  CONVERT_CY(VarI2FromCy,-0.6); EXPECT(-1);
  CONVERT_CY(VarI2FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarI2FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarI2FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarI2FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarI2FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarI2FromCy,1.5);  EXPECT(2);
}

static void test_VarI2FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarI2FromDec);

  CONVERT_BADDEC(VarI2FromDec);

  CONVERT_DEC(VarI2FromDec,0,0x80,0,32769); EXPECT_OVERFLOW;
  CONVERT_DEC(VarI2FromDec,0,0x80,0,32768); EXPECT(-32768);
  CONVERT_DEC(VarI2FromDec,0,0x80,0,1);     EXPECT(-1);
  CONVERT_DEC(VarI2FromDec,0,0,0,0);        EXPECT(0);
  CONVERT_DEC(VarI2FromDec,0,0,0,1);        EXPECT(1);
  CONVERT_DEC(VarI2FromDec,0,0,0,32767);    EXPECT(32767);
  CONVERT_DEC(VarI2FromDec,0,0,0,32768);    EXPECT_OVERFLOW;

  CONVERT_DEC(VarI2FromDec,2,0x80,0,3276800); EXPECT(-32768);
  CONVERT_DEC(VarI2FromDec,2,0,0,3276700);    EXPECT(32767);
  CONVERT_DEC(VarI2FromDec,2,0,0,3276800);    EXPECT_OVERFLOW;
}

static void test_VarI2FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarI2FromStr);

  CONVERT_STR(VarI2FromStr,NULL, 0);     EXPECT_MISMATCH;
  CONVERT_STR(VarI2FromStr,"0", 0);      EXPECT(0);
  CONVERT_STR(VarI2FromStr,"-32769", 0); EXPECT_OVERFLOW;
  CONVERT_STR(VarI2FromStr,"-32768", 0); EXPECT(-32768);
  CONVERT_STR(VarI2FromStr,"32767", 0);  EXPECT(32767);
  CONVERT_STR(VarI2FromStr,"32768", 0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_STR(VarI2FromStr,"-1.5", LOCALE_NOUSEROVERRIDE); EXPECT(-2);
  CONVERT_STR(VarI2FromStr,"-0.6", LOCALE_NOUSEROVERRIDE); EXPECT(-1);
  CONVERT_STR(VarI2FromStr,"-0.5", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI2FromStr,"-0.4", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI2FromStr,"0.4", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI2FromStr,"0.5", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI2FromStr,"0.6", LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarI2FromStr,"1.5", LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarI2Copy(void)
{
  COPYTEST(1, VT_I2, V_I2(&vSrc), V_I2(&vDst), V_I2REF(&vSrc), V_I2REF(&vDst), "%d");
}

static void test_VarI2ChangeTypeEx(void)
{
  HRESULT hres;
  SHORT in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_I2, V_I2, "%d");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_I2, V_I2, "%d", VT_UI2, V_UI2);
}

#undef CONV_TYPE
#define CONV_TYPE USHORT

static void test_VarUI2FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarUI2FromI1);
  OVERFLOWRANGE(VarUI2FromI1, -128, 0);
  CONVERTRANGE(VarUI2FromI1, 0, 128);
}

static void test_VarUI2FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarUI2FromI2);
  OVERFLOWRANGE(VarUI2FromI2, -32768, 0);
  CONVERTRANGE(VarUI2FromI2, 0, 32768);
}

static void test_VarUI2FromI4(void)
{
  CONVVARS(LONG);
  int i;

  CHECKPTR(VarUI2FromI4);
  OVERFLOWRANGE(VarUI2FromI4, -32768, 0);
  CONVERT(VarUI2FromI4, 0);     EXPECT(0);
  CONVERT(VarUI2FromI4, 65535); EXPECT(65535);
  CONVERT(VarUI2FromI4, 65536); EXPECT_OVERFLOW;
}

static void test_VarUI2FromI8(void)
{
  CONVVARS(LONG64);
  int i;

  CHECKPTR(VarUI2FromI8);
  OVERFLOWRANGE(VarUI2FromI8, -32768, 0);
  CONVERT(VarUI2FromI8, 0);     EXPECT(0);
  CONVERT(VarUI2FromI8, 65535); EXPECT(65535);
  CONVERT(VarUI2FromI8, 65536); EXPECT_OVERFLOW;
}

static void test_VarUI2FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarUI2FromUI1);
  CONVERTRANGE(VarUI2FromUI1, 0, 256);
}

static void test_VarUI2FromUI4(void)
{
  CONVVARS(ULONG);

  CHECKPTR(VarUI2FromUI4);
  CONVERT(VarUI2FromUI4, 0);     EXPECT(0);
  CONVERT(VarUI2FromUI4, 65535); EXPECT(65535);
  CONVERT(VarUI2FromUI4, 65536); EXPECT_OVERFLOW;
}

static void test_VarUI2FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarUI2FromUI8);
  CONVERT(VarUI2FromUI8, 0);     EXPECT(0);
  CONVERT(VarUI2FromUI8, 65535); EXPECT(65535);
  CONVERT(VarUI2FromUI8, 65536); EXPECT_OVERFLOW;
}

static void test_VarUI2FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarUI2FromBool);
  CONVERT(VarUI2FromBool, -1); EXPECT(65535); /* Wraps! */
  CONVERTRANGE(VarUI2FromBool, 0, 32768);
}

static void test_VarUI2FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarUI2FromR4);
  CONVERT(VarUI2FromR4, -1.0f);    EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR4, -0.51f);    EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR4, -0.5f);     EXPECT(0);
  CONVERT(VarUI2FromR4, 0.0f);     EXPECT(0);
  CONVERT(VarUI2FromR4, 1.0f);     EXPECT(1);
  CONVERT(VarUI2FromR4, 65535.0f); EXPECT(65535);
  CONVERT(VarUI2FromR4, 65535.49f); EXPECT(65535);
  CONVERT(VarUI2FromR4, 65535.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR4, 65536.0f); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI2FromR4, -1.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR4, -0.6f); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR4, -0.5f); EXPECT(0);
  CONVERT(VarUI2FromR4, -0.4f); EXPECT(0);
  CONVERT(VarUI2FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarUI2FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarUI2FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarUI2FromR4, 1.5f);  EXPECT(2);
}

static void test_VarUI2FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarUI2FromR8);
  CONVERT(VarUI2FromR8, -1.0);    EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR8, -0.51);    EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR8, -0.5);     EXPECT(0);
  CONVERT(VarUI2FromR8, 0.0);     EXPECT(0);
  CONVERT(VarUI2FromR8, 1.0);     EXPECT(1);
  CONVERT(VarUI2FromR8, 65535.0); EXPECT(65535);
  CONVERT(VarUI2FromR8, 65535.49); EXPECT(65535);
  CONVERT(VarUI2FromR8, 65535.5); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR8, 65536.0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI2FromR8, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR8, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromR8, -0.5); EXPECT(0);
  CONVERT(VarUI2FromR8, -0.4); EXPECT(0);
  CONVERT(VarUI2FromR8, 0.4);  EXPECT(0);
  CONVERT(VarUI2FromR8, 0.5);  EXPECT(0);
  CONVERT(VarUI2FromR8, 0.6);  EXPECT(1);
  CONVERT(VarUI2FromR8, 1.5);  EXPECT(2);
}

static void test_VarUI2FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarUI2FromDate);
  CONVERT(VarUI2FromDate, -1.0);    EXPECT_OVERFLOW;
  CONVERT(VarUI2FromDate, 0.0);     EXPECT(0);
  CONVERT(VarUI2FromDate, 1.0);     EXPECT(1);
  CONVERT(VarUI2FromDate, 65535.0); EXPECT(65535);
  CONVERT(VarUI2FromDate, 65536.0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarUI2FromDate, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromDate, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI2FromDate, -0.5); EXPECT(0);
  CONVERT(VarUI2FromDate, -0.4); EXPECT(0);
  CONVERT(VarUI2FromDate, 0.4);  EXPECT(0);
  CONVERT(VarUI2FromDate, 0.5);  EXPECT(0);
  CONVERT(VarUI2FromDate, 0.6);  EXPECT(1);
  CONVERT(VarUI2FromDate, 1.5);  EXPECT(2);
}

static void test_VarUI2FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarUI2FromCy);
  CONVERT_CY(VarUI2FromCy,-1);    EXPECT_OVERFLOW;
  CONVERT_CY(VarUI2FromCy,0);     EXPECT(0);
  CONVERT_CY(VarUI2FromCy,1);     EXPECT(1);
  CONVERT_CY(VarUI2FromCy,65535); EXPECT(65535);
  CONVERT_CY(VarUI2FromCy,65536); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_CY(VarUI2FromCy,-1.5); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI2FromCy,-0.6); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI2FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarUI2FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarUI2FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarUI2FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarUI2FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarUI2FromCy,1.5);  EXPECT(2);
}

static void test_VarUI2FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarUI2FromDec);

  CONVERT_BADDEC(VarUI2FromDec);

  CONVERT_DEC(VarUI2FromDec,0,0x80,0,1);  EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI2FromDec,0,0,0,0);     EXPECT(0);
  CONVERT_DEC(VarUI2FromDec,0,0,0,1);     EXPECT(1);
  CONVERT_DEC(VarUI2FromDec,0,0,0,65535); EXPECT(65535);
  CONVERT_DEC(VarUI2FromDec,0,0,0,65536); EXPECT_OVERFLOW;

  CONVERT_DEC(VarUI2FromDec,2,0x80,0,100);  EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI2FromDec,2,0,0,6553500); EXPECT(65535);
  CONVERT_DEC(VarUI2FromDec,2,0,0,6553600); EXPECT_OVERFLOW;
}

static void test_VarUI2FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarUI2FromStr);

  CONVERT_STR(VarUI2FromStr,NULL, 0);    EXPECT_MISMATCH;
  CONVERT_STR(VarUI2FromStr,"0", 0);     EXPECT(0);
  CONVERT_STR(VarUI2FromStr,"-1", 0);    EXPECT_OVERFLOW;
  CONVERT_STR(VarUI2FromStr,"65535", 0); EXPECT(65535);
  CONVERT_STR(VarUI2FromStr,"65536", 0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_STR(VarUI2FromStr,"-1.5", LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI2FromStr,"-0.6", LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI2FromStr,"-0.5", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI2FromStr,"-0.4", LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI2FromStr,"0.4", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI2FromStr,"0.5", LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI2FromStr,"0.6", LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarUI2FromStr,"1.5", LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarUI2Copy(void)
{
  COPYTEST(1, VT_UI2, V_UI2(&vSrc), V_UI2(&vDst), V_UI2REF(&vSrc), V_UI2REF(&vDst), "%d");
}

static void test_VarUI2ChangeTypeEx(void)
{
  HRESULT hres;
  USHORT in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_UI2, V_UI2, "%d");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_UI2, V_UI2, "%d", VT_I2, V_I2);
}

/*
 * VT_I4/VT_UI4
 */

#undef CONV_TYPE
#define CONV_TYPE LONG

static void test_VarI4FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarI4FromI1);
  CONVERTRANGE(VarI4FromI1, -128, 128);
}

static void test_VarI4FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarI4FromI2);
  CONVERTRANGE(VarI4FromI2, -32768, 32768);
}

static void test_VarI4FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarI4FromI8);
  CHECKPTR(VarI4FromDec);

  CONVERT(VarI4FromI8, -1);                   EXPECT(-1);
  CONVERT(VarI4FromI8, 0);                    EXPECT(0);
  CONVERT(VarI4FromI8, 1);                    EXPECT(1);

  CONVERT_I8(VarI4FromI8, -1, 2147483647ul); EXPECT_OVERFLOW;
  CONVERT_I8(VarI4FromI8, -1, 2147483648ul); EXPECT(-2147483647 - 1);
  CONVERT_I8(VarI4FromI8, 0, 2147483647ul);  EXPECT(2147483647);
  CONVERT_I8(VarI4FromI8, 0, 2147483648ul);  EXPECT_OVERFLOW;
}

static void test_VarI4FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarI4FromUI1);
  CONVERTRANGE(VarI4FromUI1, 0, 256);
}

static void test_VarI4FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarI4FromUI2);
  CONVERTRANGE(VarI4FromUI2, 0, 65536);
}

static void test_VarI4FromUI4(void)
{
  CONVVARS(ULONG);

  CHECKPTR(VarI4FromUI4);
  CONVERT(VarI4FromUI4, 0);            EXPECT(0);
  CONVERT(VarI4FromUI4, 1);            EXPECT(1);
  CONVERT(VarI4FromUI4, 2147483647);   EXPECT(2147483647);
  CONVERT(VarI4FromUI4, 2147483648ul); EXPECT_OVERFLOW;
}

static void test_VarI4FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarI4FromUI8);
  CONVERT(VarI4FromUI8, 0);             EXPECT(0);
  CONVERT(VarI4FromUI8, 1);             EXPECT(1);
  CONVERT(VarI4FromUI8, 2147483647);    EXPECT(2147483647);
  CONVERT(VarI4FromUI8, 2147483648ul);  EXPECT_OVERFLOW;
}

static void test_VarI4FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarI4FromBool);
  CONVERTRANGE(VarI4FromBool, -32768, 32768);
}

static void test_VarI4FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarI4FromR4);

  /* min/max values are not exactly representable in a float */
  CONVERT(VarI4FromR4, -1.0f); EXPECT(-1);
  CONVERT(VarI4FromR4, 0.0f);  EXPECT(0);
  CONVERT(VarI4FromR4, 1.0f);  EXPECT(1);

  CONVERT(VarI4FromR4, -1.5f); EXPECT(-2);
  CONVERT(VarI4FromR4, -0.6f); EXPECT(-1);
  CONVERT(VarI4FromR4, -0.5f); EXPECT(0);
  CONVERT(VarI4FromR4, -0.4f); EXPECT(0);
  CONVERT(VarI4FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarI4FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarI4FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarI4FromR4, 1.5f);  EXPECT(2);
}

static void test_VarI4FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarI4FromR8);
  CONVERT(VarI4FromR8, -2147483649.0); EXPECT_OVERFLOW;
  CONVERT(VarI4FromR8, -2147483648.51); EXPECT_OVERFLOW;
  CONVERT(VarI4FromR8, -2147483648.5); EXPECT(-2147483647 - 1);
  CONVERT(VarI4FromR8, -2147483648.0); EXPECT(-2147483647 - 1);
  CONVERT(VarI4FromR8, -1.0);          EXPECT(-1);
  CONVERT(VarI4FromR8, 0.0);           EXPECT(0);
  CONVERT(VarI4FromR8, 1.0);           EXPECT(1);
  CONVERT(VarI4FromR8, 2147483647.0);  EXPECT(2147483647);
  CONVERT(VarI4FromR8, 2147483647.49);  EXPECT(2147483647);
  CONVERT(VarI4FromR8, 2147483647.5);  EXPECT_OVERFLOW;
  CONVERT(VarI4FromR8, 2147483648.0);  EXPECT_OVERFLOW;

  CONVERT(VarI4FromR8, -1.5); EXPECT(-2);
  CONVERT(VarI4FromR8, -0.6); EXPECT(-1);
  CONVERT(VarI4FromR8, -0.5); EXPECT(0);
  CONVERT(VarI4FromR8, -0.4); EXPECT(0);
  CONVERT(VarI4FromR8, 0.4);  EXPECT(0);
  CONVERT(VarI4FromR8, 0.5);  EXPECT(0);
  CONVERT(VarI4FromR8, 0.6);  EXPECT(1);
  CONVERT(VarI4FromR8, 1.5);  EXPECT(2);
}

static void test_VarI4FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarI4FromDate);
  CONVERT(VarI4FromDate, -2147483649.0); EXPECT_OVERFLOW;
  CONVERT(VarI4FromDate, -2147483648.0); EXPECT(-2147483647 - 1);
  CONVERT(VarI4FromDate, -1.0);          EXPECT(-1);
  CONVERT(VarI4FromDate, 0.0);           EXPECT(0);
  CONVERT(VarI4FromDate, 1.0);           EXPECT(1);
  CONVERT(VarI4FromDate, 2147483647.0);  EXPECT(2147483647);
  CONVERT(VarI4FromDate, 2147483648.0);  EXPECT_OVERFLOW;

  CONVERT(VarI4FromDate, -1.5); EXPECT(-2);
  CONVERT(VarI4FromDate, -0.6); EXPECT(-1);
  CONVERT(VarI4FromDate, -0.5); EXPECT(0);
  CONVERT(VarI4FromDate, -0.4); EXPECT(0);
  CONVERT(VarI4FromDate, 0.4);  EXPECT(0);
  CONVERT(VarI4FromDate, 0.5);  EXPECT(0);
  CONVERT(VarI4FromDate, 0.6);  EXPECT(1);
  CONVERT(VarI4FromDate, 1.5);  EXPECT(2);
}

static void test_VarI4FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarI4FromCy);
  CONVERT_CY(VarI4FromCy,-1); EXPECT(-1);
  CONVERT_CY(VarI4FromCy,0);  EXPECT(0);
  CONVERT_CY(VarI4FromCy,1);  EXPECT(1);

  CONVERT_CY64(VarI4FromCy,-1,2147483647ul); EXPECT_OVERFLOW;
  CONVERT_CY64(VarI4FromCy,-1,2147483648ul); EXPECT(-2147483647 - 1);
  CONVERT_CY64(VarI4FromCy,0,2147483647ul);  EXPECT(2147483647ul);
  CONVERT_CY64(VarI4FromCy,0,2147483648ul);  EXPECT_OVERFLOW;

  CONVERT_CY(VarI4FromCy,-1.5); EXPECT(-2);
  CONVERT_CY(VarI4FromCy,-0.6); EXPECT(-1);
  CONVERT_CY(VarI4FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarI4FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarI4FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarI4FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarI4FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarI4FromCy,1.5);  EXPECT(2);
}

static void test_VarI4FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarI4FromDec);

  CONVERT_BADDEC(VarI4FromDec);

  CONVERT_DEC(VarI4FromDec,0,0x80,0,1); EXPECT(-1);
  CONVERT_DEC(VarI4FromDec,0,0,0,0);    EXPECT(0);
  CONVERT_DEC(VarI4FromDec,0,0,0,1);    EXPECT(1);

  CONVERT_DEC64(VarI4FromDec,0,0x80,0,0,2147483649ul);  EXPECT_OVERFLOW;
  CONVERT_DEC64(VarI4FromDec,0,0x80,0,0,2147483648ul);  EXPECT(-2147483647 - 1);
  CONVERT_DEC64(VarI4FromDec,0,0,0,0,2147483647ul);     EXPECT(2147483647ul);
  CONVERT_DEC64(VarI4FromDec,0,0,0,0,2147483648ul);     EXPECT_OVERFLOW;

  CONVERT_DEC64(VarI4FromDec,2,0x80,0,50,100);       EXPECT_OVERFLOW;
  CONVERT_DEC64(VarI4FromDec,2,0x80,0,50,0);         EXPECT(-2147483647 - 1);
  CONVERT_DEC64(VarI4FromDec,2,0,0,49,4294967196ul); EXPECT(2147483647);
  CONVERT_DEC64(VarI4FromDec,2,0,0,50,0);            EXPECT_OVERFLOW;
}

static void test_VarI4FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarI4FromStr);

  CONVERT_STR(VarI4FromStr,NULL,0);          EXPECT_MISMATCH;
  CONVERT_STR(VarI4FromStr,"0",0);           EXPECT(0);
  CONVERT_STR(VarI4FromStr,"-2147483649",0); EXPECT_OVERFLOW;
  CONVERT_STR(VarI4FromStr,"-2147483648",0); EXPECT(-2147483647 -1);
  CONVERT_STR(VarI4FromStr,"2147483647",0);  EXPECT(2147483647);
  CONVERT_STR(VarI4FromStr,"2147483648",0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_STR(VarI4FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECT(-2);
  CONVERT_STR(VarI4FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECT(-1);
  CONVERT_STR(VarI4FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI4FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarI4FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI4FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarI4FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarI4FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarI4Copy(void)
{
  COPYTEST(1, VT_I4, V_I4(&vSrc), V_I4(&vDst), V_I4REF(&vSrc), V_I4REF(&vDst), "%d");
}

static void test_VarI4ChangeTypeEx(void)
{
  HRESULT hres;
  LONG in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_I4, V_I4, "%d");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_I4, V_I4, "%d", VT_UI4, V_UI4);
}

#undef CONV_TYPE
#define CONV_TYPE ULONG
#undef EXPECTRES
#define EXPECTRES(res, x) _EXPECTRES(res, x, "%u")

static void test_VarUI4FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarUI4FromI1);
  OVERFLOWRANGE(VarUI4FromI1, -127, 0);
  CONVERTRANGE(VarUI4FromI1, 0, 128);
}

static void test_VarUI4FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarUI4FromI2);
  OVERFLOWRANGE(VarUI4FromI2, -32768, 0);
  CONVERTRANGE(VarUI4FromI2, 0, 32768);
}

static void test_VarUI4FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarUI4FromUI2);
  CONVERTRANGE(VarUI4FromUI2, 0, 65536);
}

static void test_VarUI4FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarUI4FromI8);
  CONVERT(VarUI4FromI8, -1);           EXPECT_OVERFLOW;
  CONVERT(VarUI4FromI8, 0);            EXPECT(0);
  CONVERT(VarUI4FromI8, 1);            EXPECT(1);
  CONVERT(VarUI4FromI8, 4294967295ul); EXPECT(4294967295ul);
  CONVERT_I8(VarUI4FromI8, 1, 0);      EXPECT_OVERFLOW;
}

static void test_VarUI4FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarUI4FromUI1);
  CONVERTRANGE(VarUI4FromUI1, 0, 256);
}

static void test_VarUI4FromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarUI4FromI4);
  CONVERT(VarUI4FromI4, -1);         EXPECT_OVERFLOW;
  CONVERT(VarUI4FromI4, 0);          EXPECT(0);
  CONVERT(VarUI4FromI4, 1);          EXPECT(1);
  CONVERT(VarUI4FromI4, 2147483647); EXPECT(2147483647);
}

static void test_VarUI4FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarUI4FromUI8);
  CONVERT(VarUI4FromUI8, 0);           EXPECT(0);
  CONVERT(VarUI4FromUI8, 1);           EXPECT(1);
  CONVERT(VarUI4FromI8, 4294967295ul); EXPECT(4294967295ul);
  CONVERT_I8(VarUI4FromI8, 1, 0);      EXPECT_OVERFLOW;
}

static void test_VarUI4FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarUI4FromBool);
  CONVERTRANGE(VarUI4FromBool, -32768, 32768);
}

static void test_VarUI4FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarUI4FromR4);
  /* We can't test max values as they are not exactly representable in a float */
  CONVERT(VarUI4FromR4, -1.0f); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.51f); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.5f);  EXPECT(0);
  CONVERT(VarUI4FromR4, 0.0f);  EXPECT(0);
  CONVERT(VarUI4FromR4, 1.0f);  EXPECT(1);

  CONVERT(VarUI4FromR4, -1.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.6f); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.5f); EXPECT(0);
  CONVERT(VarUI4FromR4, -0.4f); EXPECT(0);
  CONVERT(VarUI4FromR4, 0.4f);  EXPECT(0);
  CONVERT(VarUI4FromR4, 0.5f);  EXPECT(0);
  CONVERT(VarUI4FromR4, 0.6f);  EXPECT(1);
  CONVERT(VarUI4FromR4, 1.5f);  EXPECT(2);

}

static void test_VarUI4FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarUI4FromR8);
  CONVERT(VarUI4FromR8, -1.0);         EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.51f);       EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR4, -0.5f);        EXPECT(0);
  CONVERT(VarUI4FromR8, 0.0);          EXPECT(0);
  CONVERT(VarUI4FromR8, 1.0);          EXPECT(1);
  CONVERT(VarUI4FromR8, 4294967295.0); EXPECT(4294967295ul);
  CONVERT(VarUI4FromR8, 4294967295.49); EXPECT(4294967295ul);
  CONVERT(VarUI4FromR8, 4294967295.5); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR8, 4294967296.0); EXPECT_OVERFLOW;

  CONVERT(VarUI4FromR8, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR8, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromR8, -0.5); EXPECT(0);
  CONVERT(VarUI4FromR8, -0.4); EXPECT(0);
  CONVERT(VarUI4FromR8, 0.4);  EXPECT(0);
  CONVERT(VarUI4FromR8, 0.5);  EXPECT(0);
  CONVERT(VarUI4FromR8, 0.6);  EXPECT(1);
  CONVERT(VarUI4FromR8, 1.5);  EXPECT(2);
}

static void test_VarUI4FromDate(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarUI4FromDate);
  CONVERT(VarUI4FromDate, -1.0);         EXPECT_OVERFLOW;
  CONVERT(VarUI4FromDate, 0.0);          EXPECT(0);
  CONVERT(VarUI4FromDate, 1.0);          EXPECT(1);
  CONVERT(VarUI4FromDate, 4294967295.0); EXPECT(4294967295ul);
  CONVERT(VarUI4FromDate, 4294967296.0); EXPECT_OVERFLOW;

  CONVERT(VarUI4FromDate, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromDate, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI4FromDate, -0.5); EXPECT(0);
  CONVERT(VarUI4FromDate, -0.4); EXPECT(0);
  CONVERT(VarUI4FromDate, 0.4);  EXPECT(0);
  CONVERT(VarUI4FromDate, 0.5);  EXPECT(0);
  CONVERT(VarUI4FromDate, 0.6);  EXPECT(1);
  CONVERT(VarUI4FromDate, 1.5);  EXPECT(2);
}

static void test_VarUI4FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarUI4FromCy);
  CONVERT_CY(VarUI4FromCy,-1);               EXPECT_OVERFLOW;
  CONVERT_CY(VarUI4FromCy,0);                EXPECT(0);
  CONVERT_CY(VarUI4FromCy,1);                EXPECT(1);
  CONVERT_CY64(VarUI4FromCy,0,4294967295ul); EXPECT(4294967295ul);
  CONVERT_CY64(VarUI4FromCy,1,0);            EXPECT_OVERFLOW;

  CONVERT_CY(VarUI4FromCy,-1.5); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI4FromCy,-0.6); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI4FromCy,-0.5); EXPECT(0);
  CONVERT_CY(VarUI4FromCy,-0.4); EXPECT(0);
  CONVERT_CY(VarUI4FromCy,0.4);  EXPECT(0);
  CONVERT_CY(VarUI4FromCy,0.5);  EXPECT(0);
  CONVERT_CY(VarUI4FromCy,0.6);  EXPECT(1);
  CONVERT_CY(VarUI4FromCy,1.5);  EXPECT(2);
}

static void test_VarUI4FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarUI4FromDec);

  CONVERT_BADDEC(VarUI4FromDec);

  CONVERT_DEC(VarUI4FromDec,0,0x80,0,1);              EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI4FromDec,0,0,0,0);                 EXPECT(0);
  CONVERT_DEC(VarUI4FromDec,0,0,0,1);                 EXPECT(1);
  CONVERT_DEC64(VarUI4FromDec,0,0,0,0,4294967295ul);  EXPECT(4294967295ul);
  CONVERT_DEC64(VarUI4FromDec,0,0,0,1,0);             EXPECT_OVERFLOW;

  CONVERT_DEC64(VarUI4FromDec,2,0,0,99,4294967196ul); EXPECT(4294967295ul);
  CONVERT_DEC64(VarUI4FromDec,2,0,0,100,0);           EXPECT_OVERFLOW;
}

static void test_VarUI4FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarUI4FromStr);

  CONVERT_STR(VarUI4FromStr,NULL,0);         EXPECT_MISMATCH;
  CONVERT_STR(VarUI4FromStr,"-1",0);         EXPECT_OVERFLOW;
  CONVERT_STR(VarUI4FromStr,"0",0);          EXPECT(0);
  CONVERT_STR(VarUI4FromStr,"4294967295",0); EXPECT(4294967295ul);
  CONVERT_STR(VarUI4FromStr,"4294967296",0); EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT_STR(VarUI4FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI4FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI4FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI4FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECT(0);
  CONVERT_STR(VarUI4FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI4FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECT(0);
  CONVERT_STR(VarUI4FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECT(1);
  CONVERT_STR(VarUI4FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECT(2);
}

static void test_VarUI4Copy(void)
{
  COPYTEST(1u, VT_UI4, V_UI4(&vSrc), V_UI4(&vDst), V_UI4REF(&vSrc), V_UI4REF(&vDst), "%u");
}

static void test_VarUI4ChangeTypeEx(void)
{
  HRESULT hres;
  ULONG in;
  VARIANTARG vSrc, vDst;

  in = 1;

  INITIAL_TYPETEST(VT_UI4, V_UI4, "%u");
  COMMON_TYPETEST;
  NEGATIVE_TYPETEST(VT_UI4, V_UI4, "%u", VT_I4, V_I4);
}

/*
 * VT_I8/VT_UI8
 */

#undef CONV_TYPE
#define CONV_TYPE LONG64

#define EXPECTI8(x) \
  ok((hres == S_OK && out == (CONV_TYPE)(x)), \
     "expected " #x "(%u,%u), got (%u,%u); hres=0x%08x\n", \
      (ULONG)((LONG64)(x) >> 32), (ULONG)((x) & 0xffffffff), \
      (ULONG)(out >> 32), (ULONG)(out & 0xffffffff), hres)

#define EXPECTI864(x,y) \
  ok(hres == S_OK && (out >> 32) == (CONV_TYPE)(x) && (out & 0xffffffff) == (CONV_TYPE)(y), \
     "expected " #x "(%u,%u), got (%u,%u); hres=0x%08x\n", \
      (ULONG)(x), (ULONG)(y), \
      (ULONG)(out >> 32), (ULONG)(out & 0xffffffff), hres)

static void test_VarI8FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarI8FromI1);
  for (i = -128; i < 128; i++)
  {
    CONVERT(VarI8FromI1,i); EXPECTI8(i);
  }
}

static void test_VarI8FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarI8FromUI1);
  for (i = 0; i < 256; i++)
  {
    CONVERT(VarI8FromUI1,i); EXPECTI8(i);
  }
}

static void test_VarI8FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarI8FromI2);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarI8FromI2,i); EXPECTI8(i);
  }
}

static void test_VarI8FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarI8FromUI2);
  for (i = -0; i < 65535; i++)
  {
    CONVERT(VarI8FromUI2,i); EXPECTI8(i);
  }
}

static void test_VarI8FromUI4(void)
{
  CONVVARS(ULONG);

  CHECKPTR(VarI8FromUI4);
  CONVERT(VarI8FromUI4, 0);            EXPECTI8(0);
  CONVERT(VarI8FromUI4, 1);            EXPECTI8(1);
  CONVERT(VarI8FromUI4, 4294967295ul); EXPECTI8(4294967295ul);
}

static void test_VarI8FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarI8FromR4);

  CONVERT(VarI8FromR4, -128.0f); EXPECTI8(-128);
  CONVERT(VarI8FromR4, -1.0f);   EXPECTI8(-1);
  CONVERT(VarI8FromR4, 0.0f);    EXPECTI8(0);
  CONVERT(VarI8FromR4, 1.0f);    EXPECTI8(1);
  CONVERT(VarI8FromR4, 127.0f);  EXPECTI8(127);

  CONVERT(VarI8FromR4, -1.5f); EXPECTI8(-2);
  CONVERT(VarI8FromR4, -0.6f); EXPECTI8(-1);
  CONVERT(VarI8FromR4, -0.5f); EXPECTI8(0);
  CONVERT(VarI8FromR4, -0.4f); EXPECTI8(0);
  CONVERT(VarI8FromR4, 0.4f);  EXPECTI8(0);
  CONVERT(VarI8FromR4, 0.5f);  EXPECTI8(0);
  CONVERT(VarI8FromR4, 0.6f);  EXPECTI8(1);
  CONVERT(VarI8FromR4, 1.5f);  EXPECTI8(2);
}

static void test_VarI8FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarI8FromR8);
  CONVERT(VarI8FromR8, -128.0); EXPECTI8(-128);
  CONVERT(VarI8FromR8, -1.0);   EXPECTI8(-1);
  CONVERT(VarI8FromR8, 0.0);    EXPECTI8(0);
  CONVERT(VarI8FromR8, 1.0);    EXPECTI8(1);
  CONVERT(VarI8FromR8, 127.0);  EXPECTI8(127);

  CONVERT(VarI8FromR8, -1.5); EXPECTI8(-2);
  CONVERT(VarI8FromR8, -0.6); EXPECTI8(-1);
  CONVERT(VarI8FromR8, -0.5); EXPECTI8(0);
  CONVERT(VarI8FromR8, -0.4); EXPECTI8(0);
  CONVERT(VarI8FromR8, 0.4);  EXPECTI8(0);
  CONVERT(VarI8FromR8, 0.5);  EXPECTI8(0);
  CONVERT(VarI8FromR8, 0.6);  EXPECTI8(1);
  CONVERT(VarI8FromR8, 1.5);  EXPECTI8(2);
}

static void test_VarI8FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarI8FromDate);
  CONVERT(VarI8FromDate, -128.0); EXPECTI8(-128);
  CONVERT(VarI8FromDate, -1.0);   EXPECTI8(-1);
  CONVERT(VarI8FromDate, 0.0);    EXPECTI8(0);
  CONVERT(VarI8FromDate, 1.0);    EXPECTI8(1);
  CONVERT(VarI8FromDate, 127.0);  EXPECTI8(127);

  CONVERT(VarI8FromDate, -1.5); EXPECTI8(-2);
  CONVERT(VarI8FromDate, -0.6); EXPECTI8(-1);
  CONVERT(VarI8FromDate, -0.5); EXPECTI8(0);
  CONVERT(VarI8FromDate, -0.4); EXPECTI8(0);
  CONVERT(VarI8FromDate, 0.4);  EXPECTI8(0);
  CONVERT(VarI8FromDate, 0.5);  EXPECTI8(0);
  CONVERT(VarI8FromDate, 0.6);  EXPECTI8(1);
  CONVERT(VarI8FromDate, 1.5);  EXPECTI8(2);
}

static void test_VarI8FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarI8FromBool);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarI8FromBool,i); EXPECTI8(i);
  }
}

static void test_VarI8FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarI8FromUI8);
  CONVERT(VarI8FromUI8, 0); EXPECTI8(0);
  CONVERT(VarI8FromUI8, 1); EXPECTI8(1);
  CONVERT_I8(VarI8FromUI8, 0x7fffffff, 0xffffffff); EXPECTI864(0x7fffffff, 0xffffffff);
  CONVERT_I8(VarI8FromUI8, 0x80000000, 0);          EXPECT_OVERFLOW;
}

static void test_VarI8FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarI8FromCy);
  CONVERT_CY(VarI8FromCy,-128); EXPECTI8(-129);
  CONVERT_CY(VarI8FromCy,-1);   EXPECTI8(-2);
  CONVERT_CY(VarI8FromCy,0);    EXPECTI8(0);
  CONVERT_CY(VarI8FromCy,1);    EXPECTI8(1);
  CONVERT_CY(VarI8FromCy,127);  EXPECTI8(127);

  CONVERT_CY(VarI8FromCy,-1.5); EXPECTI8(-2);
  CONVERT_CY(VarI8FromCy,-0.6); EXPECTI8(-1);
  CONVERT_CY(VarI8FromCy,-0.5); EXPECTI8(-1);
  CONVERT_CY(VarI8FromCy,-0.4); EXPECTI8(-1);
  CONVERT_CY(VarI8FromCy,0.4);  EXPECTI8(0);
  CONVERT_CY(VarI8FromCy,0.5);  EXPECTI8(0);
  CONVERT_CY(VarI8FromCy,0.6);  EXPECTI8(1);
  CONVERT_CY(VarI8FromCy,1.5);  EXPECTI8(2);
}

static void test_VarI8FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarI8FromDec);

  CONVERT_BADDEC(VarI8FromDec);

  CONVERT_DEC(VarI8FromDec,0,0x80,0,128); EXPECTI8(-128);
  CONVERT_DEC(VarI8FromDec,0,0x80,0,1);   EXPECTI8(-1);
  CONVERT_DEC(VarI8FromDec,0,0,0,0);      EXPECTI8(0);
  CONVERT_DEC(VarI8FromDec,0,0,0,1);      EXPECTI8(1);
  CONVERT_DEC(VarI8FromDec,0,0,0,127);    EXPECTI8(127);

  CONVERT_DEC(VarI8FromDec,2,0x80,0,12700); EXPECTI8(-127);
  CONVERT_DEC(VarI8FromDec,2,0,0,12700);    EXPECTI8(127);
}

static void test_VarI8FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarI8FromStr);

  CONVERT_STR(VarI8FromStr,NULL,0);         EXPECT_MISMATCH;
  CONVERT_STR(VarI8FromStr,"0",0);          EXPECTI8(0);
  CONVERT_STR(VarI8FromStr,"-1",0);         EXPECTI8(-1);
  CONVERT_STR(VarI8FromStr,"2147483647",0); EXPECTI8(2147483647);

  CONVERT_STR(VarI8FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECTI8(-2);
  CONVERT_STR(VarI8FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECTI8(-1);
  CONVERT_STR(VarI8FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECTI8(0);
  CONVERT_STR(VarI8FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECTI8(0);
  CONVERT_STR(VarI8FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECTI8(0);
  CONVERT_STR(VarI8FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECTI8(0);
  CONVERT_STR(VarI8FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECTI8(1);
  CONVERT_STR(VarI8FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECTI8(2);
}

static void test_VarI8Copy(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  LONGLONG in = 1;

  if (!has_i8)
  {
    win_skip("I8 and UI8 data types are not available\n");
    return;
  }

  VariantInit(&vSrc);
  VariantInit(&vDst);
  V_VT(&vSrc) = VT_I8;
  V_I8(&vSrc) = in;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == VT_I8 && V_I8(&vDst) == in,
     "copy hres 0x%X, type %d, value (%x%08x) %x%08x\n",
     hres, V_VT(&vDst), (UINT)(in >> 32), (UINT)in, (UINT)(V_I8(&vDst) >> 32), (UINT)V_I8(&vDst) );
  V_VT(&vSrc) = VT_I8|VT_BYREF;
  V_I8REF(&vSrc) = &in;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == (VT_I8|VT_BYREF) && V_I8REF(&vDst) == &in,
     "ref hres 0x%X, type %d, ref (%p) %p\n", hres, V_VT(&vDst), &in, V_I8REF(&vDst));
  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == VT_I8 && V_I8(&vDst) == in,
     "copy hres 0x%X, type %d, value (%x%08x) %x%08x\n",
     hres, V_VT(&vDst), (UINT)(in >> 32), (UINT)in, (UINT)(V_I8(&vDst) >> 32), (UINT)V_I8(&vDst) );
}

static void test_VarI8ChangeTypeEx(void)
{
  HRESULT hres;
  LONG64 in;
  VARIANTARG vSrc, vDst;

  if (!has_i8)
  {
    win_skip("I8 and UI8 data types are not available\n");
    return;
  }

  in = 1;

  INITIAL_TYPETESTI8(VT_I8, V_I8);
  COMMON_TYPETEST;
}

/* Adapt the test macros to UI8 */
#undef CONV_TYPE
#define CONV_TYPE ULONG64

static void test_VarUI8FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarUI8FromI1);
  for (i = -128; i < 128; i++)
  {
    CONVERT(VarUI8FromI1,i);
    if (i < 0)
      EXPECT_OVERFLOW;
    else
      EXPECTI8(i);
  }
}

static void test_VarUI8FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarUI8FromUI1);
  for (i = 0; i < 256; i++)
  {
    CONVERT(VarUI8FromUI1,i); EXPECTI8(i);
  }
}

static void test_VarUI8FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarUI8FromI2);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarUI8FromI2,i);
    if (i < 0)
      EXPECT_OVERFLOW;
    else
      EXPECTI8(i);
  }
}

static void test_VarUI8FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarUI8FromUI2);
  for (i = 0; i < 65535; i++)
  {
    CONVERT(VarUI8FromUI2,i); EXPECTI8(i);
  }
}

static void test_VarUI8FromUI4(void)
{
  CONVVARS(ULONG);

  CHECKPTR(VarUI8FromUI4);
  CONVERT(VarUI8FromUI4, 0); EXPECTI8(0);
  CONVERT(VarUI8FromUI4, 0xffffffff); EXPECTI8(0xffffffff);
}

static void test_VarUI8FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarUI8FromR4);
  CONVERT(VarUI8FromR4, -1.0f);  EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR4, 0.0f);   EXPECTI8(0);
  CONVERT(VarUI8FromR4, 1.0f);   EXPECTI8(1);
  CONVERT(VarUI8FromR4, 255.0f); EXPECTI8(255);

  CONVERT(VarUI8FromR4, -1.5f); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR4, -0.6f); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR4, -0.5f); EXPECTI8(0);
  CONVERT(VarUI8FromR4, -0.4f); EXPECTI8(0);
  CONVERT(VarUI8FromR4, 0.4f);  EXPECTI8(0);
  CONVERT(VarUI8FromR4, 0.5f);  EXPECTI8(0);
  CONVERT(VarUI8FromR4, 0.6f);  EXPECTI8(1);
  CONVERT(VarUI8FromR4, 1.5f);  EXPECTI8(2);
}

static void test_VarUI8FromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarUI8FromR8);
  CONVERT(VarUI8FromR8, -1.0);  EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR8, 0.0);   EXPECTI8(0);
  CONVERT(VarUI8FromR8, 1.0);   EXPECTI8(1);
  CONVERT(VarUI8FromR8, 255.0); EXPECTI8(255);

  CONVERT(VarUI8FromR8, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR8, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromR8, -0.5); EXPECTI8(0);
  CONVERT(VarUI8FromR8, -0.4); EXPECTI8(0);
  CONVERT(VarUI8FromR8, 0.4);  EXPECTI8(0);
  CONVERT(VarUI8FromR8, 0.5);  EXPECTI8(0);
  CONVERT(VarUI8FromR8, 0.6);  EXPECTI8(1);
  CONVERT(VarUI8FromR8, 1.5);  EXPECTI8(2);
}

static void test_VarUI8FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarUI8FromDate);
  CONVERT(VarUI8FromDate, -1.0);  EXPECT_OVERFLOW;
  CONVERT(VarUI8FromDate, 0.0);   EXPECTI8(0);
  CONVERT(VarUI8FromDate, 1.0);   EXPECTI8(1);
  CONVERT(VarUI8FromDate, 255.0); EXPECTI8(255);

  CONVERT(VarUI8FromDate, -1.5); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromDate, -0.6); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromDate, -0.5); EXPECTI8(0);
  CONVERT(VarUI8FromDate, -0.4); EXPECTI8(0);
  CONVERT(VarUI8FromDate, 0.4);  EXPECTI8(0);
  CONVERT(VarUI8FromDate, 0.5);  EXPECTI8(0);
  CONVERT(VarUI8FromDate, 0.6);  EXPECTI8(1);
  CONVERT(VarUI8FromDate, 1.5);  EXPECTI8(2);
}

static void test_VarUI8FromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarUI8FromBool);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarUI8FromBool, i); EXPECTI8(i);
  }
}

static void test_VarUI8FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarUI8FromI8);
  CONVERT(VarUI8FromI8, -1); EXPECT_OVERFLOW;
  CONVERT(VarUI8FromI8, 0);  EXPECTI8(0);
  CONVERT(VarUI8FromI8, 1);  EXPECTI8(1);
}

static void test_VarUI8FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarUI8FromCy);
  CONVERT_CY(VarUI8FromCy,-1);  EXPECT_OVERFLOW;
  CONVERT_CY(VarUI8FromCy,0);   EXPECTI8(0);
  CONVERT_CY(VarUI8FromCy,1);   EXPECTI8(1);
  CONVERT_CY(VarUI8FromCy,255); EXPECTI8(255);

  CONVERT_CY(VarUI8FromCy,-1.5); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI8FromCy,-0.6); EXPECT_OVERFLOW;
  CONVERT_CY(VarUI8FromCy,-0.5); EXPECTI8(0);
  CONVERT_CY(VarUI8FromCy,-0.4); EXPECTI8(0);
  CONVERT_CY(VarUI8FromCy,0.4);  EXPECTI8(0);
  CONVERT_CY(VarUI8FromCy,0.5);  EXPECTI8(0);
  CONVERT_CY(VarUI8FromCy,0.6);  EXPECTI8(1);
  CONVERT_CY(VarUI8FromCy,1.5);  EXPECTI8(2);
}

static void test_VarUI8FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarUI8FromDec);

  CONVERT_BADDEC(VarUI8FromDec);

  /* This returns 1 under native; Wine fixes this bug and returns overflow */
  if (0)
  {
      CONVERT_DEC(VarUI8FromDec,0,0x80,0,1);
  }

  CONVERT_DEC(VarUI8FromDec,0,0,0,0);   EXPECTI8(0);
  CONVERT_DEC(VarUI8FromDec,0,0,0,1);   EXPECTI8(1);
  CONVERT_DEC(VarUI8FromDec,0,0,0,255); EXPECTI8(255);

  CONVERT_DEC(VarUI8FromDec,2,0x80,0,100); EXPECT_OVERFLOW;
  CONVERT_DEC(VarUI8FromDec,2,0,0,25500);  EXPECTI8(255);
}

static void test_VarUI8FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarUI8FromStr);

  CONVERT_STR(VarUI8FromStr,NULL,0);                    EXPECT_MISMATCH;
  CONVERT_STR(VarUI8FromStr,"0",0);                     EXPECTI8(0);
  CONVERT_STR(VarUI8FromStr,"-1",0);                    EXPECT_OVERFLOW;
  CONVERT_STR(VarUI8FromStr,"2147483647",0);            EXPECTI8(2147483647);
  CONVERT_STR(VarUI8FromStr,"18446744073709551614",0);  EXPECTI864(0xFFFFFFFF,0xFFFFFFFE);
  CONVERT_STR(VarUI8FromStr,"18446744073709551615",0);  EXPECTI864(0xFFFFFFFF,0xFFFFFFFF);
  CONVERT_STR(VarUI8FromStr,"18446744073709551616",0);  EXPECT_OVERFLOW;

  CONVERT_STR(VarUI8FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI8FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECT_OVERFLOW;
  CONVERT_STR(VarUI8FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECTI8(0);
  CONVERT_STR(VarUI8FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECTI8(0);
  CONVERT_STR(VarUI8FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECTI8(0);
  CONVERT_STR(VarUI8FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECTI8(0);
  CONVERT_STR(VarUI8FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECTI8(1);
  CONVERT_STR(VarUI8FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECTI8(2);
}

static void test_VarUI8Copy(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  ULONGLONG in = 1;

  if (!has_i8)
  {
    win_skip("I8 and UI8 data types are not available\n");
    return;
  }

  VariantInit(&vSrc);
  VariantInit(&vDst);
  V_VT(&vSrc) = VT_UI8;
  V_UI8(&vSrc) = in;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == VT_UI8 && V_UI8(&vDst) == in,
     "copy hres 0x%X, type %d, value (%x%08x) %x%08x\n",
     hres, V_VT(&vDst), (UINT)(in >> 32), (UINT)in, (UINT)(V_UI8(&vDst) >> 32), (UINT)V_UI8(&vDst) );
  V_VT(&vSrc) = VT_UI8|VT_BYREF;
  V_UI8REF(&vSrc) = &in;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == (VT_UI8|VT_BYREF) && V_UI8REF(&vDst) == &in,
     "ref hres 0x%X, type %d, ref (%p) %p\n", hres, V_VT(&vDst), &in, V_UI8REF(&vDst));
  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == VT_UI8 && V_UI8(&vDst) == in,
     "copy hres 0x%X, type %d, value (%x%08x) %x%08x\n",
     hres, V_VT(&vDst), (UINT)(in >> 32), (UINT)in, (UINT)(V_UI8(&vDst) >> 32), (UINT)V_UI8(&vDst) );
}

static void test_VarUI8ChangeTypeEx(void)
{
  HRESULT hres;
  ULONG64 in;
  VARIANTARG vSrc, vDst;

  if (!has_i8)
  {
    win_skip("I8 and UI8 data types are not available\n");
    return;
  }

  in = 1;

  INITIAL_TYPETESTI8(VT_UI8, V_UI8);
  COMMON_TYPETEST;
}

/*
 * VT_R4
 */

#undef CONV_TYPE
#define CONV_TYPE float
#undef EXPECTRES
#define EXPECTRES(res, x) _EXPECTRES(res, x, "%15.15f")

static void test_VarR4FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarR4FromI1);
  CONVERTRANGE(VarR4FromI1, -128, 128);
}

static void test_VarR4FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarR4FromUI1);
  CONVERTRANGE(VarR4FromUI1, 0, 256);
}

static void test_VarR4FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarR4FromI2);
  CONVERTRANGE(VarR4FromI2, -32768, 32768);
}

static void test_VarR4FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarR4FromUI2);
  CONVERTRANGE(VarR4FromUI2, 0, 65536);
}

static void test_VarR4FromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarR4FromI4);
  CONVERT(VarR4FromI4, -2147483647-1); EXPECT(-2147483648.0f);
  CONVERT(VarR4FromI4, -1);            EXPECT(-1.0f);
  CONVERT(VarR4FromI4, 0);             EXPECT(0.0f);
  CONVERT(VarR4FromI4, 1);             EXPECT(1.0f);
  CONVERT(VarR4FromI4, 2147483647);    EXPECT(2147483647.0f);
}

static void test_VarR4FromUI4(void)
{
  CONVVARS(unsigned int);

  CHECKPTR(VarR4FromUI4);
  CONVERT(VarR4FromUI4, 0);          EXPECT(0.0f);
  CONVERT(VarR4FromUI4, 1);          EXPECT(1.0f);
#if defined(__i386__) && (defined(_MSC_VER) || defined(__GNUC__))
  CONVERT(VarR4FromUI4, 0xffffffff); EXPECT(4294967296.0f);
#endif
}

static void test_VarR4FromR8(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarR4FromR8);
  CONVERT(VarR4FromR8, -1.0); EXPECT(-1.0f);
  CONVERT(VarR4FromR8, 0.0); EXPECT(0.0f);
  CONVERT(VarR4FromR8, 1.0); EXPECT(1.0f);
  CONVERT(VarR4FromR8, 1.5); EXPECT(1.5f);

  /* Skip rounding tests - no rounding is done */
}

static void test_VarR4FromBool(void)
{
  CONVVARS(VARIANT_BOOL);

  CHECKPTR(VarR4FromBool);
  CONVERT(VarR4FromBool, VARIANT_TRUE);  EXPECT(VARIANT_TRUE * 1.0f);
  CONVERT(VarR4FromBool, VARIANT_FALSE); EXPECT(VARIANT_FALSE * 1.0f);
}

static void test_VarR4FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarR4FromCy);
  CONVERT_CY(VarR4FromCy,-32768); EXPECT(-32768.0f);
  CONVERT_CY(VarR4FromCy,-1);     EXPECT(-1.0f);
  CONVERT_CY(VarR4FromCy,0);      EXPECT(0.0f);
  CONVERT_CY(VarR4FromCy,1);      EXPECT(1.0f);
  CONVERT_CY(VarR4FromCy,32768);  EXPECT(32768.0f);

  CONVERT_CY(VarR4FromCy,-1.5); EXPECT(-1.5f);
  CONVERT_CY(VarR4FromCy,-0.6); EXPECT(-0.6f);
  CONVERT_CY(VarR4FromCy,-0.5); EXPECT(-0.5f);
  CONVERT_CY(VarR4FromCy,-0.4); EXPECT(-0.4f);
  CONVERT_CY(VarR4FromCy,0.4);  EXPECT(0.4f);
  CONVERT_CY(VarR4FromCy,0.5);  EXPECT(0.5f);
  CONVERT_CY(VarR4FromCy,0.6);  EXPECT(0.6f);
  CONVERT_CY(VarR4FromCy,1.5);  EXPECT(1.5f);
}

static void test_VarR4FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarR4FromI8);
  CONVERT(VarR4FromI8, -1); EXPECT(-1.0f);
  CONVERT(VarR4FromI8, 0);  EXPECT(0.0f);
  CONVERT(VarR4FromI8, 1);  EXPECT(1.0f);
}

static void test_VarR4FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarR4FromUI8);
  CONVERT(VarR4FromUI8, 0); EXPECT(0.0f);
  CONVERT(VarR4FromUI8, 1); EXPECT(1.0f);
}

static void test_VarR4FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarR4FromDec);

  CONVERT_BADDEC(VarR4FromDec);

  CONVERT_DEC(VarR4FromDec,0,0x80,0,32768); EXPECT(-32768.0f);
  CONVERT_DEC(VarR4FromDec,0,0x80,0,1);     EXPECT(-1.0f);
  CONVERT_DEC(VarR4FromDec,0,0,0,0);        EXPECT(0.0f);
  CONVERT_DEC(VarR4FromDec,0,0,0,1);        EXPECT(1.0f);
  CONVERT_DEC(VarR4FromDec,0,0,0,32767);    EXPECT(32767.0f);

  CONVERT_DEC(VarR4FromDec,2,0x80,0,3276800); EXPECT(-32768.0f);
  CONVERT_DEC(VarR4FromDec,2,0,0,3276700);    EXPECT(32767.0f);
  CONVERT_DEC(VarR4FromDec,10,0,0,3276700);   EXPECT(0.00032767f);

  CONVERT_DEC(VarR4FromDec,0,0,1,0);        EXPECT(18446744073709551616.0f);
}

static void test_VarR4FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarR4FromDate);
  CONVERT(VarR4FromDate, -1.0); EXPECT(-1.0f);
  CONVERT(VarR4FromDate, 0.0);  EXPECT(0.0f);
  CONVERT(VarR4FromDate, 1.0);  EXPECT(1.0f);
}

static void test_VarR4FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarR4FromStr);

  CONVERT_STR(VarR4FromStr,NULL,0);    EXPECT_MISMATCH;
  CONVERT_STR(VarR4FromStr,"-1", 0);   EXPECT(-1.0f);
  CONVERT_STR(VarR4FromStr,"0", 0);    EXPECT(0.0f);
  CONVERT_STR(VarR4FromStr,"1", 0);    EXPECT(1.0f);

  CONVERT_STR(VarR4FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECT(-1.5f);
  CONVERT_STR(VarR4FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECT(-0.6f);
  CONVERT_STR(VarR4FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECT(-0.5f);
  CONVERT_STR(VarR4FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECT(-0.4f);
  CONVERT_STR(VarR4FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECT(0.4f);
  CONVERT_STR(VarR4FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECT(0.5f);
  CONVERT_STR(VarR4FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECT(0.6f);
  CONVERT_STR(VarR4FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECT(1.5f);
}

static void test_VarR4Copy(void)
{
  COPYTEST(77665544.0f, VT_R4, V_R4(&vSrc), V_R4(&vDst), V_R4REF(&vSrc),V_R4REF(&vDst), "%15.15f");
}

static void test_VarR4ChangeTypeEx(void)
{
#ifdef HAS_UINT64_TO_FLOAT
  HRESULT hres;
  float in;
  VARIANTARG vSrc, vDst;

  in = 1.0f;

  INITIAL_TYPETEST(VT_R4, V_R4, "%f");
  COMMON_TYPETEST;
#endif
}

/*
 * VT_R8
 */

#undef CONV_TYPE
#define CONV_TYPE double

static void test_VarR8FromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarR8FromI1);
  CONVERTRANGE(VarR8FromI1, -128, 128);
}

static void test_VarR8FromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarR8FromUI1);
  CONVERTRANGE(VarR8FromUI1, 0, 256);
}

static void test_VarR8FromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarR8FromI2);
  CONVERTRANGE(VarR8FromI2, -32768, 32768);
}

static void test_VarR8FromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarR8FromUI2);
  CONVERTRANGE(VarR8FromUI2, 0, 65536);
}

static void test_VarR8FromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarR8FromI4);
  CONVERT(VarR8FromI4, -2147483647-1); EXPECT(-2147483648.0);
  CONVERT(VarR8FromI4, -1);            EXPECT(-1.0);
  CONVERT(VarR8FromI4, 0);             EXPECT(0.0);
  CONVERT(VarR8FromI4, 1);             EXPECT(1.0);
  CONVERT(VarR8FromI4, 0x7fffffff);    EXPECT(2147483647.0);
}

static void test_VarR8FromUI4(void)
{
  CONVVARS(unsigned int);

  CHECKPTR(VarR8FromUI4);
  CONVERT(VarR8FromUI4, 0);          EXPECT(0.0);
  CONVERT(VarR8FromUI4, 1);          EXPECT(1.0);
  CONVERT(VarR8FromUI4, 0xffffffff); EXPECT(4294967295.0);
}

static void test_VarR8FromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarR8FromR4);
  CONVERT(VarR8FromR4, -1.0f); EXPECT(-1.0);
  CONVERT(VarR8FromR4, 0.0f);  EXPECT(0.0);
  CONVERT(VarR8FromR4, 1.0f);  EXPECT(1.0);
  CONVERT(VarR8FromR4, 1.5f);  EXPECT(1.5);

  /* Skip rounding tests - no rounding is done */
}

static void test_VarR8FromBool(void)
{
  CONVVARS(VARIANT_BOOL);

  CHECKPTR(VarR8FromBool);
  CONVERT(VarR8FromBool, VARIANT_TRUE);  EXPECT(VARIANT_TRUE * 1.0);
  CONVERT(VarR8FromBool, VARIANT_FALSE); EXPECT(VARIANT_FALSE * 1.0);
}

static void test_VarR8FromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarR8FromCy);
  CONVERT_CY(VarR8FromCy,-32769); EXPECT(-32769.0);
  CONVERT_CY(VarR8FromCy,-32768); EXPECT(-32768.0);
  CONVERT_CY(VarR8FromCy,-1);     EXPECT(-1.0);
  CONVERT_CY(VarR8FromCy,0);      EXPECT(0.0);
  CONVERT_CY(VarR8FromCy,1);      EXPECT(1.0);
  CONVERT_CY(VarR8FromCy,32767);  EXPECT(32767.0);
  CONVERT_CY(VarR8FromCy,32768);  EXPECT(32768.0);

  CONVERT_CY(VarR8FromCy,-1.5); EXPECT(-1.5);
  CONVERT_CY(VarR8FromCy,-0.6); EXPECT(-0.6);
  CONVERT_CY(VarR8FromCy,-0.5); EXPECT(-0.5);
  CONVERT_CY(VarR8FromCy,-0.4); EXPECT(-0.4);
  CONVERT_CY(VarR8FromCy,0.4);  EXPECT(0.4);
  CONVERT_CY(VarR8FromCy,0.5);  EXPECT(0.5);
  CONVERT_CY(VarR8FromCy,0.6);  EXPECT(0.6);
  CONVERT_CY(VarR8FromCy,1.5);  EXPECT(1.5);
}

static void test_VarR8FromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarR8FromI8);
  CONVERT(VarR8FromI8, -1); EXPECT(-1.0);
  CONVERT(VarR8FromI8, 0);  EXPECT(0.0);
  CONVERT(VarR8FromI8, 1);  EXPECT(1.0);
#if defined(__i386__) && (defined(_MSC_VER) || defined(__GNUC__))
  CONVERT_I8(VarR8FromI8, 0x7fffffff,0xffffffff); EXPECT(9223372036854775808.0);
#endif
}

static void test_VarR8FromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarR8FromUI8);
  CONVERT(VarR8FromUI8, 0); EXPECT(0.0);
  CONVERT(VarR8FromUI8, 1); EXPECT(1.0);
#if defined(__i386__) && (defined(_MSC_VER) || defined(__GNUC__))
  CONVERT_I8(VarR8FromUI8, 0x80000000,0); EXPECT(9223372036854775808.0);
#endif
}

static void test_VarR8FromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarR8FromDec);

  CONVERT_BADDEC(VarR8FromDec);

  CONVERT_DEC(VarR8FromDec,0,0x80,0,32768); EXPECT(-32768.0);
  CONVERT_DEC(VarR8FromDec,0,0x80,0,1);     EXPECT(-1.0);
  CONVERT_DEC(VarR8FromDec,0,0,0,0);        EXPECT(0.0);
  CONVERT_DEC(VarR8FromDec,0,0,0,1);        EXPECT(1.0);
  CONVERT_DEC(VarR8FromDec,0,0,0,32767);    EXPECT(32767.0);

  CONVERT_DEC(VarR8FromDec,2,0x80,0,3276800); EXPECT(-32768.0);
  CONVERT_DEC(VarR8FromDec,2,0,0,3276700);    EXPECT(32767.0);

  CONVERT_DEC(VarR8FromDec,0,0,1,0);        EXPECT(18446744073709551616.0);
}

static void test_VarR8FromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarR8FromDate);
  CONVERT(VarR8FromDate, -1.0); EXPECT(-1.0);
  CONVERT(VarR8FromDate, -0.0); EXPECT(0.0);
  CONVERT(VarR8FromDate, 1.0);  EXPECT(1.0);
}

static void test_VarR8FromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  in = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarR8FromStr);

  CONVERT_STR(VarR8FromStr,NULL,0);   EXPECT_MISMATCH;
  CONVERT_STR(VarR8FromStr,"",0);     EXPECT_MISMATCH;
  CONVERT_STR(VarR8FromStr," ",0);    EXPECT_MISMATCH;

  CONVERT_STR(VarR8FromStr,"0",LOCALE_NOUSEROVERRIDE);    EXPECT(0.0);
  CONVERT_STR(VarR8FromStr,"-1.5",LOCALE_NOUSEROVERRIDE); EXPECT(-1.5);
  CONVERT_STR(VarR8FromStr,"-0.6",LOCALE_NOUSEROVERRIDE); EXPECT(-0.6);
  CONVERT_STR(VarR8FromStr,"-0.5",LOCALE_NOUSEROVERRIDE); EXPECT(-0.5);
  CONVERT_STR(VarR8FromStr,"-0.4",LOCALE_NOUSEROVERRIDE); EXPECT(-0.4);
  CONVERT_STR(VarR8FromStr,"0.4",LOCALE_NOUSEROVERRIDE);  EXPECT(0.4);
  CONVERT_STR(VarR8FromStr,"0.5",LOCALE_NOUSEROVERRIDE);  EXPECT(0.5);
  CONVERT_STR(VarR8FromStr,"0.6",LOCALE_NOUSEROVERRIDE);  EXPECT(0.6);
  CONVERT_STR(VarR8FromStr,"1.5",LOCALE_NOUSEROVERRIDE);  EXPECT(1.5);

  /* We already have exhaustive tests for number parsing, so skip those tests here */
}

static void test_VarR8Copy(void)
{
  COPYTEST(77665544.0, VT_R8, V_R8(&vSrc), V_R8(&vDst), V_R8REF(&vSrc),V_R8REF(&vDst), "%16.16g");
}

static void test_VarR8ChangeTypeEx(void)
{
#ifdef HAS_UINT64_TO_FLOAT
  HRESULT hres;
  double in;
  VARIANTARG vSrc, vDst;

  in = 1.0;

  INITIAL_TYPETEST(VT_R8, V_R8, "%g");
  COMMON_TYPETEST;
#endif
}

#define MATHRND(l, r) left = l; right = r; hres = pVarR8Round(left, right, &out)

static void test_VarR8Round(void)
{
  HRESULT hres;
  double left = 0.0, out;
  int right;

  CHECKPTR(VarR8Round);
  MATHRND(0.5432, 5);  EXPECT(0.5432);
  MATHRND(0.5432, 4);  EXPECT(0.5432);
  MATHRND(0.5432, 3);  EXPECT(0.543);
  MATHRND(0.5432, 2);  EXPECT(0.54);
  MATHRND(0.5432, 1);  EXPECT(0.5);
  MATHRND(0.5532, 0);  EXPECT(1);
  MATHRND(0.5532, -1); EXPECT_INVALID;

  MATHRND(0.5568, 5);  EXPECT(0.5568);
  MATHRND(0.5568, 4);  EXPECT(0.5568);
  MATHRND(0.5568, 3);  EXPECT(0.557);
  MATHRND(0.5568, 2);  EXPECT(0.56);
  MATHRND(0.5568, 1);  EXPECT(0.6);
  MATHRND(0.5568, 0);  EXPECT(1);
  MATHRND(0.5568, -1); EXPECT_INVALID;

  MATHRND(0.4999, 0); EXPECT(0);
  MATHRND(0.5000, 0); EXPECT(0);
  MATHRND(0.5001, 0); EXPECT(1);
  MATHRND(1.4999, 0); EXPECT(1);
  MATHRND(1.5000, 0); EXPECT(2);
  MATHRND(1.5001, 0); EXPECT(2);
}

/*
 * VT_DATE
 */

#undef CONV_TYPE
#define CONV_TYPE DATE

static void test_VarDateFromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarDateFromI1);
  CONVERTRANGE(VarDateFromI1, -128, 128);
}

static void test_VarDateFromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarDateFromUI1);
  CONVERTRANGE(VarDateFromUI1, 0, 256);
}

static void test_VarDateFromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarDateFromI2);
  CONVERTRANGE(VarDateFromI2, -32768, 32768);
}

static void test_VarDateFromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarDateFromUI2);
  CONVERTRANGE(VarDateFromUI2, 0, 65536);
}

static void test_VarDateFromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarDateFromI4);
  CONVERT(VarDateFromI4, DATE_MIN-1);
  if (hres != DISP_E_TYPEMISMATCH) /* Early versions return this, incorrectly */
    EXPECT_OVERFLOW;
  CONVERT(VarDateFromI4, DATE_MIN);   EXPECT(DATE_MIN);
  CONVERT(VarDateFromI4, -1);         EXPECT(-1.0);
  CONVERT(VarDateFromI4, 0);          EXPECT(0.0);
  CONVERT(VarDateFromI4, 1);          EXPECT(1.0);
  CONVERT(VarDateFromI4, DATE_MAX);   EXPECT(DATE_MAX);
  CONVERT(VarDateFromI4, DATE_MAX+1);
  if (hres != DISP_E_TYPEMISMATCH) /* Early versions return this, incorrectly */
    EXPECT_OVERFLOW;
}

static void test_VarDateFromUI4(void)
{
  CONVVARS(unsigned int);

  CHECKPTR(VarDateFromUI4);
  CONVERT(VarDateFromUI4, 0);          EXPECT(0.0);
  CONVERT(VarDateFromUI4, 1);          EXPECT(1.0);
  CONVERT(VarDateFromUI4, DATE_MAX);   EXPECT(DATE_MAX);
  CONVERT(VarDateFromUI4, DATE_MAX+1);
  if (hres != DISP_E_TYPEMISMATCH) /* Early versions return this, incorrectly */
    EXPECT_OVERFLOW;
}

static void test_VarDateFromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarDateFromR4);
  CONVERT(VarDateFromR4, -1.0f); EXPECT(-1.0);
  CONVERT(VarDateFromR4, 0.0f);  EXPECT(0.0);
  CONVERT(VarDateFromR4, 1.0f);  EXPECT(1.0);
  CONVERT(VarDateFromR4, 1.5f);  EXPECT(1.5);
}

static void test_VarDateFromR8(void)
{
  CONVVARS(double);

  CHECKPTR(VarDateFromR8);
  CONVERT(VarDateFromR8, -1.0f); EXPECT(-1.0);
  CONVERT(VarDateFromR8, 0.0f);  EXPECT(0.0);
  CONVERT(VarDateFromR8, 1.0f);  EXPECT(1.0);
  CONVERT(VarDateFromR8, 1.5f);  EXPECT(1.5);
}

static void test_VarDateFromBool(void)
{
  CONVVARS(VARIANT_BOOL);

  CHECKPTR(VarDateFromBool);
  CONVERT(VarDateFromBool, VARIANT_TRUE);  EXPECT(VARIANT_TRUE * 1.0);
  CONVERT(VarDateFromBool, VARIANT_FALSE); EXPECT(VARIANT_FALSE * 1.0);
}

static void test_VarDateFromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarDateFromCy);
  CONVERT_CY(VarDateFromCy,-32769); EXPECT(-32769.0);
  CONVERT_CY(VarDateFromCy,-32768); EXPECT(-32768.0);
  CONVERT_CY(VarDateFromCy,-1);     EXPECT(-1.0);
  CONVERT_CY(VarDateFromCy,0);      EXPECT(0.0);
  CONVERT_CY(VarDateFromCy,1);      EXPECT(1.0);
  CONVERT_CY(VarDateFromCy,32767);  EXPECT(32767.0);
  CONVERT_CY(VarDateFromCy,32768);  EXPECT(32768.0);

  CONVERT_CY(VarDateFromCy,-1.5); EXPECT(-1.5);
  CONVERT_CY(VarDateFromCy,-0.6); EXPECT(-0.6);
  CONVERT_CY(VarDateFromCy,-0.5); EXPECT(-0.5);
  CONVERT_CY(VarDateFromCy,-0.4); EXPECT(-0.4);
  CONVERT_CY(VarDateFromCy,0.4);  EXPECT(0.4);
  CONVERT_CY(VarDateFromCy,0.5);  EXPECT(0.5);
  CONVERT_CY(VarDateFromCy,0.6);  EXPECT(0.6);
  CONVERT_CY(VarDateFromCy,1.5);  EXPECT(1.5);
}

static void test_VarDateFromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarDateFromI8);
  CONVERT(VarDateFromI8, DATE_MIN-1); EXPECT_OVERFLOW;
  CONVERT(VarDateFromI8, DATE_MIN);   EXPECT(DATE_MIN);
  CONVERT(VarDateFromI8, -1);         EXPECT(-1.0);
  CONVERT(VarDateFromI8, 0);          EXPECT(0.0);
  CONVERT(VarDateFromI8, 1);          EXPECT(1.0);
  CONVERT(VarDateFromI8, DATE_MAX);   EXPECT(DATE_MAX);
  CONVERT(VarDateFromI8, DATE_MAX+1); EXPECT_OVERFLOW;
}

static void test_VarDateFromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarDateFromUI8);
  CONVERT(VarDateFromUI8, 0);          EXPECT(0.0);
  CONVERT(VarDateFromUI8, 1);          EXPECT(1.0);
  CONVERT(VarDateFromUI8, DATE_MAX);   EXPECT(DATE_MAX);
  CONVERT(VarDateFromUI8, DATE_MAX+1); EXPECT_OVERFLOW;
}

static void test_VarDateFromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarDateFromDec);

  CONVERT_BADDEC(VarDateFromDec);

  CONVERT_DEC(VarDateFromDec,0,0x80,0,32768); EXPECT(-32768.0);
  CONVERT_DEC(VarDateFromDec,0,0x80,0,1);     EXPECT(-1.0);
  CONVERT_DEC(VarDateFromDec,0,0,0,0);        EXPECT(0.0);
  CONVERT_DEC(VarDateFromDec,0,0,0,1);        EXPECT(1.0);
  CONVERT_DEC(VarDateFromDec,0,0,0,32767);    EXPECT(32767.0);

  CONVERT_DEC(VarDateFromDec,2,0x80,0,3276800); EXPECT(-32768.0);
  CONVERT_DEC(VarDateFromDec,2,0,0,3276700);    EXPECT(32767.0);
}

#define DFS(str) \
  buff[0] = '\0'; out = 0.0; \
  if (str) MultiByteToWideChar(CP_ACP,0,str,-1,buff,sizeof(buff)/sizeof(WCHAR)); \
  hres = pVarDateFromStr(str ? buff : NULL,lcid,LOCALE_NOUSEROVERRIDE,&out)

#define MKRELDATE(day,mth) st.wMonth = mth; st.wDay = day; \
  pSystemTimeToVariantTime(&st,&relative)

static const char * const BadDateStrings[] =
{
  "True", "False", /* Plain text */
  "0.", ".0", "-1.1", "1.1-", /* Partial specifications */
  "1;2;3", "1*2*3", "1@2@3", "1#2#3", "(1:2)","<1:2>","1|2|3", /* Bad chars */
  "0", "1", /* 1 element */
  "0.60", "24.00", "0:60", "24:00", "1 2 am", "1 am 2", /* 2 elements */
  "1.5 2", "1 5.2", "2 32 3", "1 2 am 3", /* 3 elements */
  "1 2.3 4", "1.2.3 4", "1 2.3.4", "1.2 3.4", "1.2.3.4", "1 2 3 4",
  "1 am 2 3.4", "1 2 am 3.4", "1.2 3 am 4", "1.2 3 4 am", /* 4 elements */
  "1.2.3.4.5", "1.2.3.4 5", "1.2.3 4.5", "1.2 3.4.5", "1.2 3.4 5", "1.2 3 4.5",
  "1 2.3.4.5", "1 2.3.4 5", "1 2.3 4.5", "1 2.3 4 5", "1 2 3.4 5", "1 2 3 4 5",
  "1.2.3 4 am 5", "1.2.3 4 5 am", "1.2 3 am 4 5",
  "1.2 3 4 am 5", "1.2 3 4 5 am", "1 am 2 3.4.5", "1 2 am 3.4.5",
  "1 am 2 3 4.5", "1 2 am 3 4.5", "1 2 3 am 4.5", /* 5 elements */
  /* 6 elements */
  "1.2.3.4.5.6", "1.2.3.4.5 6", "1.2.3.4 5.6", "1.2.3.4 5 6", "1.2.3 4.5.6",
  "1.2.3 4.5 6", "1.2.3 4 5.6", "1.2 3.4.5.6", "1.2 3.4.5 6", "1.2 3.4 5.6",
  "1.2 3.4 5 6", "1.2 3 4.5.6", "1.2 3 4.5 6", "1.2 3 4 5.6", "1.2 3 4 5 6",
  "1 2.3.4.5.6", "1 2.3.4.5 6", "1 2.3.4 5.6", "1 2.3.4 5 6", "1 2.3 4.5.6",
#if 0
  /* following throws an exception on winME */
  "1 2.3 4.5 6", "1 2.3 4 5.6", "1 2.3 4 5 6", "1 2 3.4.5.6", "1 2 3.4.5 6",
#endif
  "1 2 3.4 5.6", "1 2 3.4 5 6", "1 2 3 4.5 6", "1 2 3 4 5.6", "1 2 3 4 5 6",
#if 0
  /* following throws an exception on winME */
  "1.2.3 4 am 5 6", "1.2.3 4 5 am 6", "1.2.3 4 5 6 am", "1 am 2 3 4.5.6",
#endif
  "1 2 am 3 4.5.6", "1 2 3 am 4.5.6"
};

static void test_VarDateFromStr(void)
{
  LCID lcid;
  DATE out, relative;
  HRESULT hres;
  SYSTEMTIME st;
  OLECHAR buff[128];
  size_t i;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  CHECKPTR(VarDateFromStr);
  CHECKPTR(SystemTimeToVariantTime);

  /* Some date formats are relative, so we need to find the current year */
  GetSystemTime(&st);
  st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
  DFS(NULL); EXPECT_MISMATCH;

  /* Floating point number are not recognised */
  DFS("0.0");
  if (hres == S_OK)
    EXPECT_DBL(0.0); /* Very old versions accept this string */
  else
    EXPECT_MISMATCH;

  /* 1 element - can only be a time, and only if it has am/pm */
  DFS("1 am"); EXPECT_DBL(0.04166666666666666);
  /* 2 elements */
  /* A decimal point is treated as a time separator.
   * The following are converted as hours/minutes.
   */
  DFS("0.1");  EXPECT_DBL(0.0006944444444444445);
  DFS("0.40"); EXPECT_DBL(0.02777777777777778);
  DFS("2.5");  EXPECT_DBL(0.08680555555555555);
  /* A colon acts as a decimal point */
  DFS("0:1");  EXPECT_DBL(0.0006944444444444445);
  DFS("0:20"); EXPECT_DBL(0.01388888888888889);
  DFS("0:40"); EXPECT_DBL(0.02777777777777778);
  DFS("3:5");  EXPECT_DBL(0.1284722222222222);
  /* Check the am/pm limits */
  DFS("00:00 AM"); EXPECT_DBL(0.0);
  DFS("00:00 a");  EXPECT_DBL(0.0);
  DFS("12:59 AM"); EXPECT_DBL(0.04097222222222222);
  DFS("12:59 A");  EXPECT_DBL(0.04097222222222222);
  DFS("00:00 pm"); EXPECT_DBL(0.5);
  DFS("00:00 p");  EXPECT_DBL(0.5);
  DFS("12:59 pm"); EXPECT_DBL(0.5409722222222222);
  DFS("12:59 p");  EXPECT_DBL(0.5409722222222222);
  /* AM/PM is ignored if hours > 12 */
  DFS("13:00 AM"); EXPECT_DBL(0.5416666666666666);
  DFS("13:00 PM"); EXPECT_DBL(0.5416666666666666);

  /* Space, dash and slash all indicate a date format. */
  /* If both numbers are valid month values => month/day of current year */
  DFS("1 2"); MKRELDATE(2,1); EXPECT_DBL(relative);
  DFS("2 1"); MKRELDATE(1,2); EXPECT_DBL(relative);
  /* one number not valid month, is a valid day, other number valid month:
   * that number becomes the day.
   */
  DFS("14 1");   MKRELDATE(14,1); EXPECT_DBL(relative);
  DFS("1 14");   EXPECT_DBL(relative);
  /* If the numbers can't be day/month, they are assumed to be year/month */
  DFS("30 2");   EXPECT_DBL(10990.0);
  DFS("2 30");   EXPECT_DBL(10990.0);
  DFS("32 49");  EXPECT_MISMATCH; /* Can't be any format */
  DFS("0 49");   EXPECT_MISMATCH; /* Can't be any format */
  /* If a month name is given the other number is the day */
  DFS("Jan 2");  MKRELDATE(2,1); EXPECT_DBL(relative);
  DFS("2 Jan");  EXPECT_DBL(relative);
  /* Unless it can't be, in which case it becomes the year */
  DFS("Jan 35"); EXPECT_DBL(12785.0);
  DFS("35 Jan"); EXPECT_DBL(12785.0);
  DFS("Jan-35"); EXPECT_DBL(12785.0);
  DFS("35-Jan"); EXPECT_DBL(12785.0);
  DFS("Jan/35"); EXPECT_DBL(12785.0);
  DFS("35/Jan"); EXPECT_DBL(12785.0);
  /* 3 elements */
  /* 3 numbers and time separator => h:m:s */
  DFS("0.1.0");  EXPECT_DBL(0.0006944444444444445);
  DFS("1.5.2");  EXPECT_DBL(0.04516203703703704);
  /* 3 numbers => picks date giving preference to lcid format */
  DFS("1 2 3");  EXPECT_DBL(37623.0);
  DFS("14 2 3"); EXPECT_DBL(41673.0);
  DFS("2 14 3"); EXPECT_DBL(37666.0);
  DFS("2 3 14"); EXPECT_DBL(41673.0);
  DFS("32 2 3"); EXPECT_DBL(11722.0);
  DFS("2 3 32"); EXPECT_DBL(11722.0);
  DFS("1 2 29"); EXPECT_DBL(47120.0);
  /* After 30, two digit dates are expected to be in the 1900's */
  DFS("1 2 30"); EXPECT_DBL(10960.0);
  DFS("1 2 31"); EXPECT_DBL(11325.0);
  DFS("3 am 1 2"); MKRELDATE(2,1); relative += 0.125; EXPECT_DBL(relative);
  DFS("1 2 3 am"); EXPECT_DBL(relative);

  /* 4 elements -interpreted as 2 digit date & time */
  DFS("1.2 3 4");   MKRELDATE(4,3); relative += 0.04305555556; EXPECT_DBL(relative);
  DFS("3 4 1.2");   EXPECT_DBL(relative);
  /* 5 elements - interpreted as 2 & 3 digit date/times */
  DFS("1.2.3 4 5"); MKRELDATE(5,4); relative += 0.04309027778; EXPECT_DBL(relative);
  DFS("1.2 3 4 5"); EXPECT_DBL(38415.04305555556);
#if 0
  /* following throws an exception on winME */
  DFS("1 2 3.4.5"); MKRELDATE(2,1); relative += 0.12783564815; EXPECT_DBL(relative);
#endif
  DFS("1 2 3 4.5"); EXPECT_DBL(37623.17013888889);
  /* 6 elements - interpreted as 3 digit date/times */
  DFS("1.2.3 4 5 6"); EXPECT_DBL(38812.04309027778);
  DFS("1 2 3 4.5.6"); EXPECT_DBL(37623.17020833334);

  for (i = 0; i < sizeof(BadDateStrings)/sizeof(char*); i++)
  {
    DFS(BadDateStrings[i]); EXPECT_MISMATCH;
  }

  /* Some normal-ish strings */
  DFS("2 January, 1970"); EXPECT_DBL(25570.0);
  DFS("2 January 1970");  EXPECT_DBL(25570.0);
  DFS("2 Jan 1970");      EXPECT_DBL(25570.0);
  DFS("2/Jan/1970");      EXPECT_DBL(25570.0);
  DFS("2-Jan-1970");      EXPECT_DBL(25570.0);
  DFS("1 2 1970");        EXPECT_DBL(25570.0);
  DFS("1/2/1970");        EXPECT_DBL(25570.0);
  DFS("1-2-1970");        EXPECT_DBL(25570.0);
  DFS("13-1-1970");       EXPECT_DBL(25581.0);
  DFS("1970-1-13");       EXPECT_DBL(25581.0);
  /* Native fails "1999 January 3, 9AM". I consider that a bug in native */

  /* test a non-english data string */
  DFS("02.01.1970"); EXPECT_MISMATCH;
  DFS("02.01.1970 00:00:00"); EXPECT_MISMATCH;
  lcid = MAKELCID(MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN),SORT_DEFAULT);
  DFS("02.01.1970"); EXPECT_DBL(25570.0);
  DFS("02.13.1970"); EXPECT_DBL(25612.0);
  DFS("02-13-1970"); EXPECT_DBL(25612.0);
  DFS("2020-01-11"); EXPECT_DBL(43841.0);
  DFS("2173-10-14"); EXPECT_DBL(100000.0);

  DFS("02.01.1970 00:00:00"); EXPECT_DBL(25570.0);
  lcid = MAKELCID(MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH),SORT_DEFAULT);
  DFS("02.01.1970"); EXPECT_MISMATCH;
  DFS("02.01.1970 00:00:00"); EXPECT_MISMATCH;
}

static void test_VarDateCopy(void)
{
  COPYTEST(77665544.0, VT_DATE, V_DATE(&vSrc), V_DATE(&vDst), V_DATEREF(&vSrc),
           V_DATEREF(&vDst), "%16.16g");
}

static const char* wtoascii(LPWSTR lpszIn)
{
    static char buff[256];
    WideCharToMultiByte(CP_ACP, 0, lpszIn, -1, buff, sizeof(buff), NULL, NULL);
    return buff;
}

static void test_VarDateChangeTypeEx(void)
{
  static const WCHAR sz25570[] = {
    '1','/','2','/','1','9','7','0','\0' };
  static const WCHAR sz25570_2[] = {
          '1','/','2','/','7','0','\0' };
  static const WCHAR sz25570Nls[] = {
    '1','/','2','/','1','9','7','0',' ','1','2',':','0','0',':','0','0',' ','A','M','\0' };
  HRESULT hres;
  DATE in;
  VARIANTARG vSrc, vDst;
  LCID lcid;

  in = 1.0;

#ifdef HAS_UINT64_TO_FLOAT
  INITIAL_TYPETEST(VT_DATE, V_DATE, "%g");
  COMMON_TYPETEST;
#endif

  V_VT(&vDst) = VT_EMPTY;
  V_VT(&vSrc) = VT_DATE;
  V_DATE(&vSrc) = 25570.0;
  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

  hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, VARIANT_NOUSEROVERRIDE, VT_BSTR); 
  ok(hres == S_OK && V_VT(&vDst) == VT_BSTR && V_BSTR(&vDst) &&
          (!lstrcmpW(V_BSTR(&vDst), sz25570) || !lstrcmpW(V_BSTR(&vDst), sz25570_2)),
          "hres=0x%X, type=%d (should be VT_BSTR), *bstr=%s\n", 
          hres, V_VT(&vDst), V_BSTR(&vDst) ? wtoascii(V_BSTR(&vDst)) : "?");
  VariantClear(&vDst);

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  if (has_locales)
  {
    hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, VARIANT_NOUSEROVERRIDE|VARIANT_USE_NLS, VT_BSTR);
    ok(hres == S_OK && V_VT(&vDst) == VT_BSTR && V_BSTR(&vDst) && !lstrcmpW(V_BSTR(&vDst), sz25570Nls), 
            "hres=0x%X, type=%d (should be VT_BSTR), *bstr=%s\n", 
            hres, V_VT(&vDst), V_BSTR(&vDst) ? wtoascii(V_BSTR(&vDst)) : "?");
    VariantClear(&vDst);
  }
}

/*
 * VT_CY
 */

#undef CONV_TYPE
#define CONV_TYPE CY

#define EXPECTCY(x) \
  ok((hres == S_OK && out.int64 == (LONGLONG)(x*CY_MULTIPLIER)), \
     "expected " #x "*CY_MULTIPLIER, got (%8x %8x); hres=0x%08x\n", S(out).Hi, S(out).Lo, hres)

#define EXPECTCY64(x,y) \
  ok(hres == S_OK && S(out).Hi == (LONG)x && S(out).Lo == y, \
     "expected " #x " " #y " (%u,%u), got (%u,%u); hres=0x%08x\n", \
      (ULONG)(x), (ULONG)(y), S(out).Hi, S(out).Lo, hres)

static void test_VarCyFromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarCyFromI1);
  for (i = -128; i < 128; i++)
  {
    CONVERT(VarCyFromI1,i); EXPECTCY(i);
  }
}

static void test_VarCyFromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarCyFromUI1);
  for (i = 0; i < 256; i++)
  {
    CONVERT(VarCyFromUI1,i); EXPECTCY(i);
  }
}

static void test_VarCyFromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarCyFromI2);
  for (i = -16384; i < 16384; i++)
  {
    CONVERT(VarCyFromI2,i); EXPECTCY(i);
  }
}

static void test_VarCyFromUI2(void)
{
  CONVVARS(int);
  int i;

  CHECKPTR(VarCyFromUI2);
  for (i = 0; i < 32768; i++)
  {
    CONVERT(VarCyFromUI2,i); EXPECTCY(i);
  }
}

static void test_VarCyFromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarCyFromI4);
  CONVERT(VarCyFromI4, -1);         EXPECTCY(-1);
  CONVERT(VarCyFromI4, 0);          EXPECTCY(0);
  CONVERT(VarCyFromI4, 1);          EXPECTCY(1);
  CONVERT(VarCyFromI4, 0x7fffffff); EXPECTCY64(0x1387, 0xffffd8f0);
  CONVERT(VarCyFromI4, 0x80000000); EXPECTCY64(0xffffec78, 0);
}

static void test_VarCyFromUI4(void)
{
  CONVVARS(unsigned int);

  CHECKPTR(VarCyFromUI4);
  CONVERT(VarCyFromUI4, 0); EXPECTCY(0);
  CONVERT(VarCyFromUI4, 1); EXPECTCY(1);
  CONVERT(VarCyFromUI4, 0x80000000); EXPECTCY64(5000, 0);
}

static void test_VarCyFromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarCyFromR4);
  CONVERT(VarCyFromR4, -1.0f); EXPECTCY(-1);
  CONVERT(VarCyFromR4, 0.0f);  EXPECTCY(0);
  CONVERT(VarCyFromR4, 1.0f);  EXPECTCY(1);
  CONVERT(VarCyFromR4, 1.5f);  EXPECTCY(1.5);

  CONVERT(VarCyFromR4, -1.5f);     EXPECTCY(-1.5);
  CONVERT(VarCyFromR4, -0.6f);     EXPECTCY(-0.6);
  CONVERT(VarCyFromR4, -0.5f);     EXPECTCY(-0.5);
  CONVERT(VarCyFromR4, -0.4f);     EXPECTCY(-0.4);
  CONVERT(VarCyFromR4, 0.4f);      EXPECTCY(0.4);
  CONVERT(VarCyFromR4, 0.5f);      EXPECTCY(0.5);
  CONVERT(VarCyFromR4, 0.6f);      EXPECTCY(0.6);
  CONVERT(VarCyFromR4, 1.5f);      EXPECTCY(1.5);
  CONVERT(VarCyFromR4, 1.00009f);  EXPECTCY(1.0001);
  CONVERT(VarCyFromR4, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromR4, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromR4, -0.00009f); EXPECTCY(-0.0001);
  CONVERT(VarCyFromR4, -0.00005f); EXPECTCY(0);
  CONVERT(VarCyFromR4, -0.00001f); EXPECTCY(0);
  CONVERT(VarCyFromR4, 0.00001f);  EXPECTCY(0);
  CONVERT(VarCyFromR4, 0.00005f);  EXPECTCY(0);
  CONVERT(VarCyFromR4, 0.00009f);  EXPECTCY(0.0001);
  CONVERT(VarCyFromR4, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromR4, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromR4, -1.00009f); EXPECTCY(-1.0001);
}

static void test_VarCyFromR8(void)
{
  CONVVARS(DOUBLE);

  CHECKPTR(VarCyFromR8);

#if defined(__i386__) && (defined(_MSC_VER) || defined(__GNUC__))
  /* Test our rounding is exactly the same. This fails if the special x86
   * code is taken out of VarCyFromR8.
   */
  CONVERT(VarCyFromR8, -461168601842738.7904); EXPECTCY64(0xbfffffff, 0xffffff23);
#endif

  CONVERT(VarCyFromR8, -4611686018427388416.1); EXPECT_OVERFLOW;
  CONVERT(VarCyFromR8, -1.0);                   EXPECTCY(-1);
  CONVERT(VarCyFromR8, -0.0);                   EXPECTCY(0);
  CONVERT(VarCyFromR8, 1.0);                    EXPECTCY(1);
  CONVERT(VarCyFromR8, 4611686018427387648.0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarCyFromR8, -1.5f);     EXPECTCY(-1.5);
  CONVERT(VarCyFromR8, -0.6f);     EXPECTCY(-0.6);
  CONVERT(VarCyFromR8, -0.5f);     EXPECTCY(-0.5);
  CONVERT(VarCyFromR8, -0.4f);     EXPECTCY(-0.4);
  CONVERT(VarCyFromR8, 0.4f);      EXPECTCY(0.4);
  CONVERT(VarCyFromR8, 0.5f);      EXPECTCY(0.5);
  CONVERT(VarCyFromR8, 0.6f);      EXPECTCY(0.6);
  CONVERT(VarCyFromR8, 1.5f);      EXPECTCY(1.5);
  CONVERT(VarCyFromR8, 1.00009f);  EXPECTCY(1.0001);
  CONVERT(VarCyFromR8, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromR8, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromR8, -0.00009f); EXPECTCY(-0.0001);
  CONVERT(VarCyFromR8, -0.00005f); EXPECTCY(0);
  CONVERT(VarCyFromR8, -0.00001f); EXPECTCY(0);
  CONVERT(VarCyFromR8, 0.00001f);  EXPECTCY(0);
  CONVERT(VarCyFromR8, 0.00005f);  EXPECTCY(0);
  CONVERT(VarCyFromR8, 0.00009f);  EXPECTCY(0.0001);
  CONVERT(VarCyFromR8, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromR8, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromR8, -1.00009f); EXPECTCY(-1.0001);
}

static void test_VarCyFromBool(void)
{
  CONVVARS(VARIANT_BOOL);
  int i;

  CHECKPTR(VarCyFromBool);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarCyFromBool, i);  EXPECTCY(i);
  }
}

static void test_VarCyFromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarCyFromI8);
  CONVERT_I8(VarCyFromI8, -214749, 2728163227ul);   EXPECT_OVERFLOW;
  CONVERT_I8(VarCyFromI8, -214749, 2728163228ul);   EXPECTCY64(2147483648ul,15808);
  CONVERT(VarCyFromI8, -1); EXPECTCY(-1);
  CONVERT(VarCyFromI8, 0);  EXPECTCY(0);
  CONVERT(VarCyFromI8, 1);  EXPECTCY(1);
  CONVERT_I8(VarCyFromI8, 214748, 1566804068); EXPECTCY64(2147483647ul, 4294951488ul);
  CONVERT_I8(VarCyFromI8, 214748, 1566804069); EXPECT_OVERFLOW;
}

static void test_VarCyFromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarCyFromUI8);
  CONVERT(VarCyFromUI8, 0); EXPECTCY(0);
  CONVERT(VarCyFromUI8, 1); EXPECTCY(1);
  CONVERT_I8(VarCyFromUI8, 214748, 1566804068); EXPECTCY64(2147483647ul, 4294951488ul);
  CONVERT_I8(VarCyFromUI8, 214748, 1566804069); EXPECTCY64(2147483647ul, 4294961488ul);
  CONVERT_I8(VarCyFromUI8, 214748, 1566804070); EXPECT_OVERFLOW;
  CONVERT_I8(VarCyFromUI8, 214749, 1566804068); EXPECT_OVERFLOW;
}

static void test_VarCyFromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarCyFromDec);

  CONVERT_BADDEC(VarCyFromDec);

  CONVERT_DEC(VarCyFromDec,0,0x80,0,1); EXPECTCY(-1);
  CONVERT_DEC(VarCyFromDec,0,0,0,0);    EXPECTCY(0);
  CONVERT_DEC(VarCyFromDec,0,0,0,1);    EXPECTCY(1);

  CONVERT_DEC64(VarCyFromDec,0,0,0,214748, 1566804068); EXPECTCY64(2147483647ul, 4294951488ul);
  CONVERT_DEC64(VarCyFromDec,0,0,0,214748, 1566804069); EXPECTCY64(2147483647ul, 4294961488ul);
  CONVERT_DEC64(VarCyFromDec,0,0,0,214748, 1566804070); EXPECT_OVERFLOW;
  CONVERT_DEC64(VarCyFromDec,0,0,0,214749, 1566804068); EXPECT_OVERFLOW;

  CONVERT_DEC(VarCyFromDec,2,0,0,100);     EXPECTCY(1);
  CONVERT_DEC(VarCyFromDec,2,0x80,0,100);  EXPECTCY(-1);
  CONVERT_DEC(VarCyFromDec,2,0x80,0,1);    EXPECTCY(-0.01);
  CONVERT_DEC(VarCyFromDec,2,0,0,1);       EXPECTCY(0.01);
  CONVERT_DEC(VarCyFromDec,2,0x80,0,1);    EXPECTCY(-0.01);
  CONVERT_DEC(VarCyFromDec,2,0,0,999);     EXPECTCY(9.99);
  CONVERT_DEC(VarCyFromDec,2,0x80,0,999);  EXPECTCY(-9.99);
  CONVERT_DEC(VarCyFromDec,2,0,0,1500);    EXPECTCY(15);
  CONVERT_DEC(VarCyFromDec,2,0x80,0,1500); EXPECTCY(-15);
}

static void test_VarCyFromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarCyFromDate);

#if defined(__i386__) && (defined(_MSC_VER) || defined(__GNUC__))
  CONVERT(VarCyFromR8, -461168601842738.7904); EXPECTCY64(0xbfffffff, 0xffffff23);
#endif

  CONVERT(VarCyFromDate, -1.0); EXPECTCY(-1);
  CONVERT(VarCyFromDate, -0.0); EXPECTCY(0);
  CONVERT(VarCyFromDate, 1.0);  EXPECTCY(1);
  CONVERT(VarCyFromDate, -4611686018427388416.1); EXPECT_OVERFLOW;
  CONVERT(VarCyFromDate, 4611686018427387648.0);  EXPECT_OVERFLOW;

  /* Rounding */
  CONVERT(VarCyFromDate, -1.5f);     EXPECTCY(-1.5);
  CONVERT(VarCyFromDate, -0.6f);     EXPECTCY(-0.6);
  CONVERT(VarCyFromDate, -0.5f);     EXPECTCY(-0.5);
  CONVERT(VarCyFromDate, -0.4f);     EXPECTCY(-0.4);
  CONVERT(VarCyFromDate, 0.4f);      EXPECTCY(0.4);
  CONVERT(VarCyFromDate, 0.5f);      EXPECTCY(0.5);
  CONVERT(VarCyFromDate, 0.6f);      EXPECTCY(0.6);
  CONVERT(VarCyFromDate, 1.5f);      EXPECTCY(1.5);
  CONVERT(VarCyFromDate, 1.00009f);  EXPECTCY(1.0001);
  CONVERT(VarCyFromDate, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromDate, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromDate, -0.00009f); EXPECTCY(-0.0001);
  CONVERT(VarCyFromDate, -0.00005f); EXPECTCY(0);
  CONVERT(VarCyFromDate, -0.00001f); EXPECTCY(0);
  CONVERT(VarCyFromDate, 0.00001f);  EXPECTCY(0);
  CONVERT(VarCyFromDate, 0.00005f);  EXPECTCY(0);
  CONVERT(VarCyFromDate, 0.00009f);  EXPECTCY(0.0001);
  CONVERT(VarCyFromDate, -1.00001f); EXPECTCY(-1);
  CONVERT(VarCyFromDate, -1.00005f); EXPECTCY(-1);
  CONVERT(VarCyFromDate, -1.00009f); EXPECTCY(-1.0001);
}

#define MATHVARS1 HRESULT hres; double left = 0.0; CY cyLeft, out
#define MATHVARS2 MATHVARS1; double right = 0.0; CY cyRight
#define MATH1(func, l) left = (double)l; pVarCyFromR8(left, &cyLeft); hres = p##func(cyLeft, &out)
#define MATH2(func, l, r) left = (double)l; right = (double)r; \
  pVarCyFromR8(left, &cyLeft); pVarCyFromR8(right, &cyRight); \
  hres = p##func(cyLeft, cyRight, &out)

static void test_VarCyAdd(void)
{
  MATHVARS2;

  CHECKPTR(VarCyAdd);
  MATH2(VarCyAdd, 0.5, 0.5);   EXPECTCY(1);
  MATH2(VarCyAdd, 0.5, -0.4);  EXPECTCY(0.1);
  MATH2(VarCyAdd, 0.5, -0.6);  EXPECTCY(-0.1);
  MATH2(VarCyAdd, -0.5, -0.5); EXPECTCY(-1);
  MATH2(VarCyAdd, -922337203685476.0, -922337203685476.0); EXPECT_OVERFLOW;
  MATH2(VarCyAdd, -922337203685476.0, 922337203685476.0);  EXPECTCY(0);
  MATH2(VarCyAdd, 922337203685476.0, -922337203685476.0);  EXPECTCY(0);
  MATH2(VarCyAdd, 922337203685476.0, 922337203685476.0);   EXPECT_OVERFLOW;
}

static void test_VarCyMul(void)
{
  MATHVARS2;

  CHECKPTR(VarCyMul);
  MATH2(VarCyMul, 534443.0, 0.0); EXPECTCY(0);
  MATH2(VarCyMul, 0.5, 0.5);      EXPECTCY(0.25);
  MATH2(VarCyMul, 0.5, -0.4);     EXPECTCY(-0.2);
  MATH2(VarCyMul, 0.5, -0.6);     EXPECTCY(-0.3);
  MATH2(VarCyMul, -0.5, -0.5);    EXPECTCY(0.25);
  MATH2(VarCyMul, 922337203685476.0, 20000); EXPECT_OVERFLOW;
}

static void test_VarCySub(void)
{
  MATHVARS2;

  CHECKPTR(VarCySub);
  MATH2(VarCySub, 0.5, 0.5);   EXPECTCY(0);
  MATH2(VarCySub, 0.5, -0.4);  EXPECTCY(0.9);
  MATH2(VarCySub, 0.5, -0.6);  EXPECTCY(1.1);
  MATH2(VarCySub, -0.5, -0.5); EXPECTCY(0);
  MATH2(VarCySub, -922337203685476.0, -922337203685476.0); EXPECTCY(0);
  MATH2(VarCySub, -922337203685476.0, 922337203685476.0);  EXPECT_OVERFLOW;
  MATH2(VarCySub, 922337203685476.0, -922337203685476.0);  EXPECT_OVERFLOW;
  MATH2(VarCySub, 922337203685476.0, 922337203685476.0);   EXPECTCY(0);
}

static void test_VarCyAbs(void)
{
  MATHVARS1;

  CHECKPTR(VarCyAbs);
  MATH1(VarCyAbs, 0.5);  EXPECTCY(0.5);
  MATH1(VarCyAbs, -0.5); EXPECTCY(0.5);
  MATH1(VarCyAbs, 922337203685476.0);  EXPECTCY64(2147483647ul,4294951488ul);
  MATH1(VarCyAbs, -922337203685476.0); EXPECTCY64(2147483647ul,4294951488ul);
}

static void test_VarCyNeg(void)
{
  MATHVARS1;

  CHECKPTR(VarCyNeg);
  MATH1(VarCyNeg, 0.5); EXPECTCY(-0.5);
  MATH1(VarCyNeg, -0.5); EXPECTCY(0.5);
  MATH1(VarCyNeg, 922337203685476.0);  EXPECTCY64(2147483648ul,15808);
  MATH1(VarCyNeg, -922337203685476.0); EXPECTCY64(2147483647ul,4294951488ul);
}

#define MATHMULI4(l, r) left = l; right = r; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyMulI4(cyLeft, right, &out)

static void test_VarCyMulI4(void)
{
  MATHVARS1;
  LONG right;

  CHECKPTR(VarCyMulI4);
  MATHMULI4(534443.0, 0); EXPECTCY(0);
  MATHMULI4(0.5, 1);      EXPECTCY(0.5);
  MATHMULI4(0.5, 2);      EXPECTCY(1);
  MATHMULI4(922337203685476.0, 1); EXPECTCY64(2147483647ul,4294951488ul);
  MATHMULI4(922337203685476.0, 2); EXPECT_OVERFLOW;
}

#define MATHMULI8(l, r) left = l; right = r; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyMulI8(cyLeft, right, &out)

static void test_VarCyMulI8(void)
{
  MATHVARS1;
  LONG64 right;

  CHECKPTR(VarCyMulI8);
  MATHMULI8(534443.0, 0); EXPECTCY(0);
  MATHMULI8(0.5, 1);      EXPECTCY(0.5);
  MATHMULI8(0.5, 2);      EXPECTCY(1);
  MATHMULI8(922337203685476.0, 1); EXPECTCY64(2147483647ul,4294951488ul);
  MATHMULI8(922337203685476.0, 2); EXPECT_OVERFLOW;
}

#define MATHCMP(l, r) left = l; right = r; pVarCyFromR8(left, &cyLeft); pVarCyFromR8(right, &cyRight); \
  hres = pVarCyCmp(cyLeft, cyRight)

static void test_VarCyCmp(void)
{
  HRESULT hres;
  double left = 0.0, right = 0.0;
  CY cyLeft, cyRight;

  CHECKPTR(VarCyCmp);
  MATHCMP(-1.0, -1.0); EXPECT_EQ;
  MATHCMP(-1.0, 0.0);  EXPECT_LT;
  MATHCMP(-1.0, 1.0);  EXPECT_LT;
  MATHCMP(-1.0, 2.0);  EXPECT_LT;
  MATHCMP(0.0, 1.0);   EXPECT_LT;
  MATHCMP(0.0, 0.0);   EXPECT_EQ;
  MATHCMP(0.0, -1.0);  EXPECT_GT;
  MATHCMP(1.0, -1.0);  EXPECT_GT;
  MATHCMP(1.0, 0.0);   EXPECT_GT;
  MATHCMP(1.0, 1.0);   EXPECT_EQ;
  MATHCMP(1.0, 2.0);   EXPECT_LT;
}

#define MATHCMPR8(l, r) left = l; right = r; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyCmpR8(cyLeft, right);

static void test_VarCyCmpR8(void)
{
  HRESULT hres;
  double left = 0.0;
  CY cyLeft;
  double right;

  CHECKPTR(VarCyCmpR8);
  MATHCMPR8(-1.0, -1.0); EXPECT_EQ;
  MATHCMPR8(-1.0, 0.0);  EXPECT_LT;
  MATHCMPR8(-1.0, 1.0);  EXPECT_LT;
  MATHCMPR8(-1.0, 2.0);  EXPECT_LT;
  MATHCMPR8(0.0, 1.0);   EXPECT_LT;
  MATHCMPR8(0.0, 0.0);   EXPECT_EQ;
  MATHCMPR8(0.0, -1.0);  EXPECT_GT;
  MATHCMPR8(1.0, -1.0);  EXPECT_GT;
  MATHCMPR8(1.0, 0.0);   EXPECT_GT;
  MATHCMPR8(1.0, 1.0);   EXPECT_EQ;
  MATHCMPR8(1.0, 2.0);   EXPECT_LT;
}

#undef MATHRND
#define MATHRND(l, r) left = l; right = r; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyRound(cyLeft, right, &out)

static void test_VarCyRound(void)
{
  MATHVARS1;
  int right;

  CHECKPTR(VarCyRound);
  MATHRND(0.5432, 5);  EXPECTCY(0.5432);
  MATHRND(0.5432, 4);  EXPECTCY(0.5432);
  MATHRND(0.5432, 3);  EXPECTCY(0.543);
  MATHRND(0.5432, 2);  EXPECTCY(0.54);
  MATHRND(0.5432, 1);  EXPECTCY(0.5);
  MATHRND(0.5532, 0);  EXPECTCY(1);
  MATHRND(0.5532, -1); EXPECT_INVALID;

  MATHRND(0.5568, 5);  EXPECTCY(0.5568);
  MATHRND(0.5568, 4);  EXPECTCY(0.5568);
  MATHRND(0.5568, 3);  EXPECTCY(0.557);
  MATHRND(0.5568, 2);  EXPECTCY(0.56);
  MATHRND(0.5568, 1);  EXPECTCY(0.6);
  MATHRND(0.5568, 0);  EXPECTCY(1);
  MATHRND(0.5568, -1); EXPECT_INVALID;

  MATHRND(0.4999, 0); EXPECTCY(0);
  MATHRND(0.5000, 0); EXPECTCY(0);
  MATHRND(0.5001, 0); EXPECTCY(1);
  MATHRND(1.4999, 0); EXPECTCY(1);
  MATHRND(1.5000, 0); EXPECTCY(2);
  MATHRND(1.5001, 0); EXPECTCY(2);
}

#define MATHFIX(l) left = l; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyFix(cyLeft, &out)

static void test_VarCyFix(void)
{
  MATHVARS1;

  CHECKPTR(VarCyFix);
  MATHFIX(-1.0001); EXPECTCY(-1);
  MATHFIX(-1.4999); EXPECTCY(-1);
  MATHFIX(-1.5001); EXPECTCY(-1);
  MATHFIX(-1.9999); EXPECTCY(-1);
  MATHFIX(-0.0001); EXPECTCY(0);
  MATHFIX(-0.4999); EXPECTCY(0);
  MATHFIX(-0.5001); EXPECTCY(0);
  MATHFIX(-0.9999); EXPECTCY(0);
  MATHFIX(0.0001);  EXPECTCY(0);
  MATHFIX(0.4999);  EXPECTCY(0);
  MATHFIX(0.5001);  EXPECTCY(0);
  MATHFIX(0.9999);  EXPECTCY(0);
  MATHFIX(1.0001);  EXPECTCY(1);
  MATHFIX(1.4999);  EXPECTCY(1);
  MATHFIX(1.5001);  EXPECTCY(1);
  MATHFIX(1.9999);  EXPECTCY(1);
}

#define MATHINT(l) left = l; pVarCyFromR8(left, &cyLeft); \
  hres = pVarCyInt(cyLeft, &out)

static void test_VarCyInt(void)
{
  MATHVARS1;

  CHECKPTR(VarCyInt);
  MATHINT(-1.0001); EXPECTCY(-2);
  MATHINT(-1.4999); EXPECTCY(-2);
  MATHINT(-1.5001); EXPECTCY(-2);
  MATHINT(-1.9999); EXPECTCY(-2);
  MATHINT(-0.0001); EXPECTCY(-1);
  MATHINT(-0.4999); EXPECTCY(-1);
  MATHINT(-0.5001); EXPECTCY(-1);
  MATHINT(-0.9999); EXPECTCY(-1);
  MATHINT(0.0001);  EXPECTCY(0);
  MATHINT(0.4999);  EXPECTCY(0);
  MATHINT(0.5001);  EXPECTCY(0);
  MATHINT(0.9999);  EXPECTCY(0);
  MATHINT(1.0001);  EXPECTCY(1);
  MATHINT(1.4999);  EXPECTCY(1);
  MATHINT(1.5001);  EXPECTCY(1);
  MATHINT(1.9999);  EXPECTCY(1);
}

/*
 * VT_DECIMAL
 */

#undef CONV_TYPE
#define CONV_TYPE DECIMAL

#define EXPECTDEC(scl, sgn, hi, lo) ok(hres == S_OK && \
  S(U(out)).scale == (BYTE)(scl) && S(U(out)).sign == (BYTE)(sgn) && \
  out.Hi32 == (ULONG)(hi) && U1(out).Lo64 == (ULONG64)(lo), \
  "expected (%d,%d,%d,(%x %x)), got (%d,%d,%d,(%x %x)) hres 0x%08x\n", \
  scl, sgn, hi, (LONG)((LONG64)(lo) >> 32), (LONG)((lo) & 0xffffffff), S(U(out)).scale, \
  S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres)

#define EXPECTDEC64(scl, sgn, hi, mid, lo) ok(hres == S_OK && \
  S(U(out)).scale == (BYTE)(scl) && S(U(out)).sign == (BYTE)(sgn) && \
  out.Hi32 == (ULONG)(hi) && S1(U1(out)).Mid32 == (ULONG)(mid) && \
  S1(U1(out)).Lo32 == (ULONG)(lo), \
  "expected (%d,%d,%d,(%x %x)), got (%d,%d,%d,(%x %x)) hres 0x%08x\n", \
  scl, sgn, hi, (LONG)(mid), (LONG)(lo), S(U(out)).scale, \
  S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres)

/* expect either a positive or negative zero */
#define EXPECTDECZERO() ok(hres == S_OK && S(U(out)).scale == 0 && \
  (S(U(out)).sign == 0 || S(U(out)).sign == 0x80) && out.Hi32 == 0 && U1(out).Lo64 == 0, \
  "expected zero, got (%d,%d,%d,(%x %x)) hres 0x%08x\n", \
  S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres)

#define EXPECTDECI if (i < 0) EXPECTDEC(0, 0x80, 0, -i); else EXPECTDEC(0, 0, 0, i)

static void test_VarDecFromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarDecFromI1);
  for (i = -128; i < 128; i++)
  {
    CONVERT(VarDecFromI1,i); EXPECTDECI;
  }
}

static void test_VarDecFromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarDecFromI2);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarDecFromI2,i); EXPECTDECI;
  }
}

static void test_VarDecFromI4(void)
{
  CONVVARS(LONG);
  int i;

  CHECKPTR(VarDecFromI4);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarDecFromI4,i); EXPECTDECI;
  }
}

static void test_VarDecFromI8(void)
{
  CONVVARS(LONG64);
  int i;

  CHECKPTR(VarDecFromI8);
  for (i = -32768; i < 32768; i++)
  {
    CONVERT(VarDecFromI8,i); EXPECTDECI;
  }
}

static void test_VarDecFromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarDecFromUI1);
  for (i = 0; i < 256; i++)
  {
    CONVERT(VarDecFromUI1,i); EXPECTDECI;
  }
}

static void test_VarDecFromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarDecFromUI2);
  for (i = 0; i < 65536; i++)
  {
    CONVERT(VarDecFromUI2,i); EXPECTDECI;
  }
}

static void test_VarDecFromUI4(void)
{
  CONVVARS(ULONG);
  int i;

  CHECKPTR(VarDecFromUI4);
  for (i = 0; i < 65536; i++)
  {
    CONVERT(VarDecFromUI4,i); EXPECTDECI;
  }
}

static void test_VarDecFromUI8(void)
{
  CONVVARS(ULONG64);
  int i;

  CHECKPTR(VarDecFromUI8);
  for (i = 0; i < 65536; i++)
  {
    CONVERT(VarDecFromUI8,i); EXPECTDECI;
  }
}

static void test_VarDecFromBool(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarDecFromBool);
  /* Test all possible type values. Note that the result is reduced to 0 or -1 */
  for (i = -32768; i < 0; i++)
  {
    CONVERT(VarDecFromBool,i);
    if (i)
      EXPECTDEC(0,0x80,0,1);
    else
      EXPECTDEC(0,0,0,0);
  }
}

static void test_VarDecFromR4(void)
{
  CONVVARS(float);

  CHECKPTR(VarDecFromR4);

  CONVERT(VarDecFromR4,-0.6f); EXPECTDEC(1,0x80,0,6);
  CONVERT(VarDecFromR4,-0.5f); EXPECTDEC(1,0x80,0,5);
  CONVERT(VarDecFromR4,-0.4f); EXPECTDEC(1,0x80,0,4);
  CONVERT(VarDecFromR4,0.0f);  EXPECTDEC(0,0,0,0);
  CONVERT(VarDecFromR4,0.4f);  EXPECTDEC(1,0,0,4);
  CONVERT(VarDecFromR4,0.5f);  EXPECTDEC(1,0,0,5);
  CONVERT(VarDecFromR4,0.6f);  EXPECTDEC(1,0,0,6);
}

static void test_VarDecFromR8(void)
{
  CONVVARS(double);

  CHECKPTR(VarDecFromR8);

  CONVERT(VarDecFromR8,-0.6); EXPECTDEC(1,0x80,0,6);
  CONVERT(VarDecFromR8,-0.5); EXPECTDEC(1,0x80,0,5);
  CONVERT(VarDecFromR8,-0.4); EXPECTDEC(1,0x80,0,4);
  CONVERT(VarDecFromR8,0.0);  EXPECTDEC(0,0,0,0);
  CONVERT(VarDecFromR8,0.4);  EXPECTDEC(1,0,0,4);
  CONVERT(VarDecFromR8,0.5);  EXPECTDEC(1,0,0,5);
  CONVERT(VarDecFromR8,0.6);  EXPECTDEC(1,0,0,6);
}

static void test_VarDecFromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarDecFromDate);

  CONVERT(VarDecFromDate,-0.6); EXPECTDEC(1,0x80,0,6);
  CONVERT(VarDecFromDate,-0.5); EXPECTDEC(1,0x80,0,5);
  CONVERT(VarDecFromDate,-0.4); EXPECTDEC(1,0x80,0,4);
  CONVERT(VarDecFromDate,0.0);  EXPECTDEC(0,0,0,0);
  CONVERT(VarDecFromDate,0.4);  EXPECTDEC(1,0,0,4);
  CONVERT(VarDecFromDate,0.5);  EXPECTDEC(1,0,0,5);
  CONVERT(VarDecFromDate,0.6);  EXPECTDEC(1,0,0,6);
}

static void test_VarDecFromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  CHECKPTR(VarDecFromStr);

  in = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  CONVERT_STR(VarDecFromStr,NULL,0);                       EXPECT_MISMATCH;
  CONVERT_STR(VarDecFromStr,"-1",  LOCALE_NOUSEROVERRIDE); EXPECTDEC(0,0x80,0,1);
  CONVERT_STR(VarDecFromStr,"0",   LOCALE_NOUSEROVERRIDE); EXPECTDEC(0,0,0,0);
  CONVERT_STR(VarDecFromStr,"1",   LOCALE_NOUSEROVERRIDE); EXPECTDEC(0,0,0,1);
  CONVERT_STR(VarDecFromStr,"0.5", LOCALE_NOUSEROVERRIDE); EXPECTDEC(1,0,0,5);
  CONVERT_STR(VarDecFromStr,"4294967296", LOCALE_NOUSEROVERRIDE); EXPECTDEC64(0,0,0,1,0);
  CONVERT_STR(VarDecFromStr,"18446744073709551616", LOCALE_NOUSEROVERRIDE); EXPECTDEC(0,0,1,0);
  CONVERT_STR(VarDecFromStr,"4294967296.0", LOCALE_NOUSEROVERRIDE); EXPECTDEC64(0,0,0,1,0);
  CONVERT_STR(VarDecFromStr,"18446744073709551616.0", LOCALE_NOUSEROVERRIDE); EXPECTDEC(0,0,1,0);
}

static void test_VarDecFromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarDecFromCy);

  CONVERT_CY(VarDecFromCy, -1);  EXPECTDEC(4,0x80,0,10000);
  CONVERT_CY(VarDecFromCy, 0);   EXPECTDEC(4,0,0,0);
  CONVERT_CY(VarDecFromCy, 1);   EXPECTDEC(4,0,0,10000);
  CONVERT_CY(VarDecFromCy, 0.5); EXPECTDEC(4,0,0,5000);
}

#undef MATHVARS1
#define MATHVARS1 HRESULT hres; DECIMAL l, out
#undef MATHVARS2
#define MATHVARS2 MATHVARS1; DECIMAL r
#undef MATH1
#define MATH1(func) hres = p##func(&l, &out)
#undef MATH2
#define MATH2(func) hres = p##func(&l, &r, &out)
#undef MATH3
#define MATH3(func) hres = p##func(&l, r)

static void test_VarDecAbs(void)
{
  MATHVARS1;

  CHECKPTR(VarDecAbs);
  SETDEC(l,0,0x80,0,1);  MATH1(VarDecAbs); EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0,0,0);     MATH1(VarDecAbs); EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0x80,0,0);  MATH1(VarDecAbs); EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,1);     MATH1(VarDecAbs); EXPECTDEC(0,0,0,1);

  /* Doesn't check for invalid input */
  SETDEC(l,0,0x7f,0,1);  MATH1(VarDecAbs); EXPECTDEC(0,0x7f,0,1);
  SETDEC(l,0,0x80,29,1); MATH1(VarDecAbs); EXPECTDEC(0,0,29,1);
}

static void test_VarDecNeg(void)
{
  MATHVARS1;

  CHECKPTR(VarDecNeg);
  SETDEC(l,0,0x80,0,1); MATH1(VarDecNeg); EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0,0,0);    MATH1(VarDecNeg); EXPECTDEC(0,0x80,0,0); /* '-0'! */
  SETDEC(l,0,0x80,0,0); MATH1(VarDecNeg); EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,1);    MATH1(VarDecNeg); EXPECTDEC(0,0x80,0,1);

  /* Doesn't check for invalid input */
  SETDEC(l,0,0x7f,0,1);  MATH1(VarDecNeg); EXPECTDEC(0,0xff,0,1);
  SETDEC(l,0,0x80,29,1); MATH1(VarDecNeg); EXPECTDEC(0,0,29,1);
  SETDEC(l,0,0,29,1);    MATH1(VarDecNeg); EXPECTDEC(0,0x80,29,1);
}

static void test_VarDecAdd(void)
{
  MATHVARS2;

  CHECKPTR(VarDecAdd);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,0);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd); EXPECTDEC(0,0x80,0,1);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,1);

  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,0);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,2);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd); EXPECTDECZERO();
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0x80,0,2); MATH2(VarDecAdd); EXPECTDEC(0,0x80,0,1);

  SETDEC(l,0,0x80,0,0); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0x80,0,1); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDECZERO();
  SETDEC(l,0,0x80,0,1); SETDEC(r,0,0,0,2);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0x80,0,1); SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd); EXPECTDEC(0,0x80,0,2);
  SETDEC(l,0,0x80,0,2); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0x80,0,1);

  SETDEC(l,0,0,0,0xffffffff); SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd); EXPECTDEC(0,0,0,0xfffffffe);
  SETDEC(l,0,0,0,0xffffffff); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,(ULONG64)1 << 32);
  SETDEC(l,0,0,0,0xffffffff); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0,(ULONG64)1 << 32);

  SETDEC64(l,0,0,0,0xffffffff,0); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC64(0,0,0,0xffffffff,1);
  SETDEC64(l,0,0,0,0xffffffff,0); SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd);
  EXPECTDEC64(0,0,0,0xfffffffe,0xffffffff);

  SETDEC64(l,0,0,0,0xffffffff,0xffffffff); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,1,0);
  SETDEC64(l,0,0,0,0xffffffff,0xffffffff); SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd);
  EXPECTDEC64(0,0,0,0xffffffff,0xfffffffe);

  SETDEC(l,0,0,0xffffffff,0); SETDEC(r,0,0,0,1);    MATH2(VarDecAdd); EXPECTDEC(0,0,0xffffffff,1);
  SETDEC(l,0,0,0xffffffff,0); SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd);
  EXPECTDEC64(0,0,0xfffffffe,0xffffffff,0xffffffff);

  SETDEC64(l,0,0,0xffffffff,0xffffffff,0xffffffff);SETDEC(r,0,0x80,0,1); MATH2(VarDecAdd);
  EXPECTDEC64(0,0,0xffffffff,0xffffffff,0xfffffffe);
  SETDEC64(l,0,0,0xffffffff,0xffffffff,0xffffffff);SETDEC(r,0,0,0,1); MATH2(VarDecAdd);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);

  SETDEC64(l,1,0,0xffffffff,0xffffffff,0xffffffff);SETDEC(r,1,0,0,1); MATH2(VarDecAdd);
  todo_wine EXPECTDEC64(0,0,0x19999999,0x99999999,0x9999999A);

  SETDEC64(l,0,0,0xe22ea493,0xb30310a7,0x70000000);SETDEC64(r,0,0,0xe22ea493,0xb30310a7,0x70000000); MATH2(VarDecAdd);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);

  SETDEC64(l,1,0,0xe22ea493,0xb30310a7,0x70000000);SETDEC64(r,1,0,0xe22ea493,0xb30310a7,0x70000000); MATH2(VarDecAdd);
  todo_wine EXPECTDEC64(0,0,0x2d3c8750,0xbd670354,0xb0000000);

  SETDEC(l,3,128,0,123456); SETDEC64(r,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
  MATH2(VarDecAdd); EXPECTDEC64(0,0,-1,0xFFFFFFFF,0xFFFFFF84);

  SETDEC(l,3,0,0,123456); SETDEC64(r,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF); MATH2(VarDecAdd);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);

  SETDEC(l,4,0,0,123456); SETDEC64(r,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF); MATH2(VarDecAdd);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);

  SETDEC(l,5,0,0,123456); SETDEC64(r,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF); MATH2(VarDecAdd);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);

  SETDEC(l,6,0,0,123456); SETDEC64(r,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
  MATH2(VarDecAdd); EXPECTDEC64(0,0,-1,0xFFFFFFFF,0xFFFFFFFF);

  SETDEC(l,3,128,0,123456); SETDEC64(r,0,0,0x19999999,0x99999999,0x99999999);
  MATH2(VarDecAdd); EXPECTDEC64(1,0,-1,0xFFFFFFFF,0xFFFFFB27);

  SETDEC(l,3,128,0,123567); SETDEC64(r,0,0,0x19999999,0x99999999,0x99999999);
  MATH2(VarDecAdd); EXPECTDEC64(1,0,-1,0xFFFFFFFF,0xFFFFFB26);

  /* Promotes to the highest scale, so here the results are in the scale of 2 */
  SETDEC(l,2,0,0,0);   SETDEC(r,0,0,0,0); MATH2(VarDecAdd); EXPECTDEC(2,0,0,0);
  SETDEC(l,2,0,0,100); SETDEC(r,0,0,0,1); MATH2(VarDecAdd); EXPECTDEC(2,0,0,200);
}

static void test_VarDecSub(void)
{
  MATHVARS2;

  CHECKPTR(VarDecSub);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,0);    MATH2(VarDecSub); EXPECTDECZERO();
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,1);    MATH2(VarDecSub); EXPECTDEC(0,0x80,0,1);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,1);    MATH2(VarDecSub); EXPECTDECZERO();
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0x80,0,1); MATH2(VarDecSub); EXPECTDEC(0,0,0,2);
}

static void test_VarDecMul(void)
{
  MATHVARS2;
  
  CHECKPTR(VarDecMul);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,0);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,0);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,1);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,1);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,1);
  SETDEC(l,0,0,0,45000);SETDEC(r,0,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,90000);
  SETDEC(l,0,0,0,2);    SETDEC(r,0,0,0,45000);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,90000);

  SETDEC(l,0,0x80,0,2); SETDEC(r,0,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(0,0x80,0,4);
  SETDEC(l,0,0,0,2);    SETDEC(r,0,0x80,0,2);  MATH2(VarDecMul);   EXPECTDEC(0,0x80,0,4);
  SETDEC(l,0,0x80,0,2); SETDEC(r,0,0x80,0,2);  MATH2(VarDecMul);   EXPECTDEC(0,0,0,4);

  SETDEC(l,4,0,0,2);    SETDEC(r,0,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(4,0,0,4);
  SETDEC(l,0,0,0,2);    SETDEC(r,3,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(3,0,0,4);
  SETDEC(l,4,0,0,2);    SETDEC(r,3,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(7,0,0,4);
  /* this last one shows that native oleaut32 does *not* gratuitously seize opportunities
     to reduce the scale if possible - the canonical result for the expected value is (6,0,0,1)
   */
  SETDEC(l,4,0,0,5);    SETDEC(r,3,0,0,2);  MATH2(VarDecMul);   EXPECTDEC(7,0,0,10);
  
  SETDEC64(l,0,0,0,0xFFFFFFFF,0xFFFFFFFF);    SETDEC(r,0,0,0,2);  MATH2(VarDecMul);   EXPECTDEC64(0,0,1,0xFFFFFFFF,0xFFFFFFFE);
  SETDEC(l,0,0,0,2);    SETDEC64(r,0,0,0,0xFFFFFFFF,0xFFFFFFFF);  MATH2(VarDecMul);   EXPECTDEC64(0,0,1,0xFFFFFFFF,0xFFFFFFFE);
  SETDEC(l,0,0,1,1);    SETDEC(r,0,0,0,0x80000000);  MATH2(VarDecMul);   EXPECTDEC(0,0,0x80000000,0x80000000);
  SETDEC(l,0,0,0,0x80000000);    SETDEC(r,0,0,1,1);  MATH2(VarDecMul);   EXPECTDEC(0,0,0x80000000,0x80000000);
  
  /* near-overflow, used as a reference */
  SETDEC64(l,0,0,0,0xFFFFFFFF,0xFFFFFFFF);    SETDEC(r,0,0,0,2000000000);  MATH2(VarDecMul);EXPECTDEC64(0,0,1999999999,0xFFFFFFFF,0x88CA6C00);
  /* actual overflow - right operand is 10 times the previous value */
  SETDEC64(l,0,0,0,0xFFFFFFFF,0xFFFFFFFF);    SETDEC64(r,0,0,0,4,0xA817C800);  MATH2(VarDecMul);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);
  /* here, native oleaut32 has an opportunity to avert the overflow, by reducing the scale of the result  */
  SETDEC64(l,1,0,0,0xFFFFFFFF,0xFFFFFFFF);    SETDEC64(r,0,0,0,4,0xA817C800);  MATH2(VarDecMul);EXPECTDEC64(0,0,1999999999,0xFFFFFFFF,0x88CA6C00);

  /* near-overflow, used as a reference */
  SETDEC64(l,0,0,1,0xFFFFFFFF,0xFFFFFFFE);    SETDEC(r,0,0,0,1000000000);  MATH2(VarDecMul);EXPECTDEC64(0,0,1999999999,0xFFFFFFFF,0x88CA6C00);
  /* actual overflow - right operand is 10 times the previous value */
  SETDEC64(l,0,0,1,0xFFFFFFFF,0xFFFFFFFE);    SETDEC64(r,0,0,0,2,0x540BE400);  MATH2(VarDecMul);
  ok(hres == DISP_E_OVERFLOW,"Expected overflow, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);
  /* here, native oleaut32 has an opportunity to avert the overflow, by reducing the scale of the result  */
  SETDEC64(l,1,0,1,0xFFFFFFFF,0xFFFFFFFE);    SETDEC64(r,0,0,0,2,0x540BE400);  MATH2(VarDecMul);EXPECTDEC64(0,0,1999999999,0xFFFFFFFF,0x88CA6C00);
  
  /* this one shows that native oleaut32 is willing to lose significant digits in order to avert an overflow */
  SETDEC64(l,2,0,0,0xFFFFFFFF,0xFFFFFFFF);    SETDEC64(r,0,0,0,9,0x502F9001);  MATH2(VarDecMul);EXPECTDEC64(1,0,0xee6b2800,0x19999998,0xab2e719a);
}

static void test_VarDecDiv(void)
{
  MATHVARS2;
  
  CHECKPTR(VarDecDiv);
  /* identity divisions */
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,1);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,0);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,1);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,1);
  SETDEC(l,1,0,0,1);    SETDEC(r,0,0,0,1);  MATH2(VarDecDiv);   EXPECTDEC(1,0,0,1);

  /* exact divisions */  
  SETDEC(l,0,0,0,45);    SETDEC(r,0,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,5);
  SETDEC(l,1,0,0,45);    SETDEC(r,0,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(1,0,0,5);
  SETDEC(l,0,0,0,45);    SETDEC(r,1,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,50);
  SETDEC(l,1,0,0,45);    SETDEC(r,2,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,50);
  /* these last three results suggest that native oleaut32 scales both operands down to zero
     before the division, but does not always try to scale the result, even if it is possible -
     analogous to multiplication behavior.
   */
  SETDEC(l,1,0,0,45);    SETDEC(r,1,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(0,0,0,5);
  SETDEC(l,2,0,0,450);    SETDEC(r,1,0,0,9);  MATH2(VarDecDiv);
  if (S(U(out)).scale == 1) EXPECTDEC(1,0,0,50);
  else EXPECTDEC(0,0,0,5);

  /* inexact divisions */
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,180700362,0x14b700cb,0x05555555);
  SETDEC(l,1,0,0,1);    SETDEC(r,0,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,18070036,0x35458014,0x4d555555);
  SETDEC(l,0,0,0,1);    SETDEC(r,1,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,1807003620,0xcf2607ee,0x35555555);
  SETDEC(l,1,0,0,1);    SETDEC(r,2,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,1807003620,0xcf2607ee,0x35555555);
  SETDEC(l,1,0,0,1);    SETDEC(r,1,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,180700362,0x14b700cb,0x05555555);
  SETDEC(l,2,0,0,10);    SETDEC(r,1,0,0,3);  MATH2(VarDecDiv);  EXPECTDEC64(28,0,180700362,0x14b700cb,0x05555555);

  /* this one shows that native oleaut32 rounds up the result */
  SETDEC(l,0,0,0,2);    SETDEC(r,0,0,0,3);  MATH2(VarDecDiv);   EXPECTDEC64(28,0,361400724,0x296e0196,0x0aaaaaab);
  
  /* sign tests */
  SETDEC(l,0,0x80,0,45);    SETDEC(r,0,0,0,9);  MATH2(VarDecDiv);   EXPECTDEC(0,0x80,0,5);
  SETDEC(l,0,0,0,45);       SETDEC(r,0,0x80,0,9);  MATH2(VarDecDiv);EXPECTDEC(0,0x80,0,5);
  SETDEC(l,0,0x80,0,45);    SETDEC(r,0,0x80,0,9);  MATH2(VarDecDiv);EXPECTDEC(0,0,0,5);
  
  /* oddballs */
  SETDEC(l,0,0,0,0);    SETDEC(r,0,0,0,0);  MATH2(VarDecDiv);/* indeterminate */
  ok(hres == DISP_E_DIVBYZERO,"Expected division-by-zero, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);
  SETDEC(l,0,0,0,1);    SETDEC(r,0,0,0,0);  MATH2(VarDecDiv);/* division by zero */
  ok(hres == DISP_E_DIVBYZERO,"Expected division-by-zero, got (%d,%d,%d,(%8x,%8x)x) hres 0x%08x\n",
     S(U(out)).scale, S(U(out)).sign, out.Hi32, S1(U1(out)).Mid32, S1(U1(out)).Lo32, hres);
  
}

static void test_VarDecCmp(void)
{
  MATHVARS1;

  CHECKPTR(VarDecCmp);

  SETDEC(l,0,0,0,1); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,0,0,1); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,0,1); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(l,0,0,0,1); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,0,1); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,0,1); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,1); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(l,0,0,0,0); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,0,0,0); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,0,0,0); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(l,0,0,0,0); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,0,0); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,0,0,0); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(l,0,DECIMAL_NEG,0,0); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(l,0,0,-1,-1); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,-1,-1); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,-1,-1); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_EQ;

  SETDEC(l,0,0,-1,-1); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,-1,-1); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(l,0,0,-1,-1); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,0,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,0,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,-1,-1); SETDEC(out,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_EQ;


  SETDEC(out,0,0,0,1); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,0,0,1); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,0,1); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(out,0,0,0,1); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,0,1); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,0,1); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,0,1); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(out,0,0,0,0); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,0,0,0); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,0,0,0); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(out,0,0,0,0); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,0,0); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,0,0,0); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_EQ;
  SETDEC(out,0,DECIMAL_NEG,0,0); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(out,0,0,-1,-1); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,-1,-1); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,-1,-1); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_EQ;

  SETDEC(out,0,0,-1,-1); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,-1,-1); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_LT;
  SETDEC(out,0,0,-1,-1); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_LT;

  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,0,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,0,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,0,-1,-1); MATH1(VarDecCmp); EXPECT_GT;

  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,DECIMAL_NEG,0,1); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,DECIMAL_NEG,0,0); MATH1(VarDecCmp); EXPECT_GT;
  SETDEC(out,0,DECIMAL_NEG,-1,-1); SETDEC(l,0,DECIMAL_NEG,-1,-1); MATH1(VarDecCmp); EXPECT_EQ;

  SETDEC(l,3,0,0,123456); SETDEC64(out,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
  MATH1(VarDecCmp); EXPECT_LT;
}

static void test_VarDecCmpR8(void)
{
  HRESULT hres;
  DECIMAL l;
  double r;

  CHECKPTR(VarDecCmpR8);

  SETDEC(l,0,0,0,1); r = 0.0;  MATH3(VarDecCmpR8); EXPECT_GT;
  SETDEC(l,0,0,0,1); r = 0.1;  MATH3(VarDecCmpR8); EXPECT_GT;
  SETDEC(l,0,0,0,1); r = -0.1; MATH3(VarDecCmpR8); EXPECT_GT;

  SETDEC(l,0,DECIMAL_NEG,0,1); r = 0.0;  MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,1); r = 0.1;  MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,1); r = -0.1; MATH3(VarDecCmpR8); EXPECT_LT;

  SETDEC(l,0,0,0,0); r = 0.0;  MATH3(VarDecCmpR8); EXPECT_EQ;
  SETDEC(l,0,0,0,0); r = 0.1;  MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,0,0,0); r = -0.1; MATH3(VarDecCmpR8); EXPECT_GT;

  SETDEC(l,0,DECIMAL_NEG,0,0); r = 0.0;  MATH3(VarDecCmpR8); EXPECT_EQ;
  SETDEC(l,0,DECIMAL_NEG,0,0); r = 0.1;  MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,0); r = -0.1; MATH3(VarDecCmpR8); EXPECT_GT;

  SETDEC(l,0,0,0,1);             r = DECIMAL_NEG; MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,DECIMAL_NEG,0,0);   r = DECIMAL_NEG; MATH3(VarDecCmpR8); EXPECT_LT;
  SETDEC(l,0,0,-1,-1);           r = DECIMAL_NEG; MATH3(VarDecCmpR8); EXPECT_GT;
  SETDEC(l,0,DECIMAL_NEG,-1,-1); r = DECIMAL_NEG; MATH3(VarDecCmpR8); EXPECT_LT;
}

#define CLEAR(x) memset(&(x), 0xBB, sizeof(x))

static void test_VarDecRound(void)
{
    HRESULT hres;
    DECIMAL l, out;

    CHECKPTR(VarDecRound);

    CLEAR(out); SETDEC(l, 0, 0, 0, 1); hres = pVarDecRound(&l, 3, &out); EXPECTDEC(0, 0, 0, 1);

    CLEAR(out); SETDEC(l, 0, 0, 0, 1); hres = pVarDecRound(&l, 0, &out); EXPECTDEC(0, 0, 0, 1);
    CLEAR(out); SETDEC(l, 1, 0, 0, 1); hres = pVarDecRound(&l, 0, &out); EXPECTDEC(0, 0, 0, 0);
    CLEAR(out); SETDEC(l, 1, 0, 0, 1); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, 0, 0, 1);
    CLEAR(out); SETDEC(l, 2, 0, 0, 11); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, 0, 0, 1);
    CLEAR(out); SETDEC(l, 2, 0, 0, 15); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, 0, 0, 2);
    CLEAR(out); SETDEC(l, 6, 0, 0, 550001); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, 0, 0, 6);

    CLEAR(out); SETDEC(l, 0, DECIMAL_NEG, 0, 1); hres = pVarDecRound(&l, 0, &out); EXPECTDEC(0, DECIMAL_NEG, 0, 1);
    CLEAR(out); SETDEC(l, 1, DECIMAL_NEG, 0, 1); hres = pVarDecRound(&l, 0, &out); EXPECTDEC(0, DECIMAL_NEG, 0, 0);
    CLEAR(out); SETDEC(l, 1, DECIMAL_NEG, 0, 1); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, DECIMAL_NEG, 0, 1);
    CLEAR(out); SETDEC(l, 2, DECIMAL_NEG, 0, 11); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, DECIMAL_NEG, 0, 1);
    CLEAR(out); SETDEC(l, 2, DECIMAL_NEG, 0, 15); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, DECIMAL_NEG, 0, 2);
    CLEAR(out); SETDEC(l, 6, DECIMAL_NEG, 0, 550001); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, DECIMAL_NEG, 0, 6);

    CLEAR(out); SETDEC64(l, 0, 0, 0xffffffff, 0xffffffff, 0xffffffff); hres = pVarDecRound(&l, 0, &out); EXPECTDEC64(0, 0, 0xffffffff, 0xffffffff, 0xffffffff);
    CLEAR(out); SETDEC64(l, 28, 0, 0xffffffff, 0xffffffff, 0xffffffff); hres = pVarDecRound(&l, 0, &out); EXPECTDEC64(0, 0, 0, 0, 8);
    CLEAR(out); SETDEC64(l, 0, DECIMAL_NEG, 0xffffffff, 0xffffffff, 0xffffffff); hres = pVarDecRound(&l, 0, &out); EXPECTDEC64(0, DECIMAL_NEG, 0xffffffff, 0xffffffff, 0xffffffff);
    CLEAR(out); SETDEC64(l, 28, DECIMAL_NEG, 0xffffffff, 0xffffffff, 0xffffffff); hres = pVarDecRound(&l, 0, &out); EXPECTDEC64(0, DECIMAL_NEG, 0, 0, 8);

    CLEAR(out); SETDEC(l, 2, 0, 0, 0); hres = pVarDecRound(&l, 1, &out); EXPECTDEC(1, 0, 0, 0);
}

/*
 * VT_BOOL
 */

#undef CONV_TYPE
#define CONV_TYPE VARIANT_BOOL
#undef EXPECTRES
#define EXPECTRES(res, x) _EXPECTRES(res, x, "%d")
#undef CONVERTRANGE
#define CONVERTRANGE(func,start,end) for (i = start; i < end; i++) { \
  CONVERT(func, i); if (i) { EXPECT(VARIANT_TRUE); } else { EXPECT(VARIANT_FALSE); } }

static void test_VarBoolFromI1(void)
{
  CONVVARS(signed char);
  int i;

  CHECKPTR(VarBoolFromI1);
  CONVERTRANGE(VarBoolFromI1, -128, 128);
}

static void test_VarBoolFromUI1(void)
{
  CONVVARS(BYTE);
  int i;

  CHECKPTR(VarBoolFromUI1);
  CONVERTRANGE(VarBoolFromUI1, 0, 256);
}

static void test_VarBoolFromI2(void)
{
  CONVVARS(SHORT);
  int i;

  CHECKPTR(VarBoolFromI2);
  CONVERTRANGE(VarBoolFromI2, -32768, 32768);
}

static void test_VarBoolFromUI2(void)
{
  CONVVARS(USHORT);
  int i;

  CHECKPTR(VarBoolFromUI2);
  CONVERTRANGE(VarBoolFromUI2, 0, 65536);
}

static void test_VarBoolFromI4(void)
{
  CONVVARS(int);

  CHECKPTR(VarBoolFromI4);
  CONVERT(VarBoolFromI4, 0x80000000); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromI4, -1);         EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromI4, 0);          EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromI4, 1);          EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromI4, 0x7fffffff); EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromUI4(void)
{
  CONVVARS(ULONG);

  CHECKPTR(VarBoolFromUI4);
  CONVERT(VarBoolFromI4, 0);          EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromI4, 1);          EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromI4, 0x80000000); EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromR4(void)
{
  CONVVARS(FLOAT);

  CHECKPTR(VarBoolFromR4);
  CONVERT(VarBoolFromR4, -1.0f); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 0.0f);  EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromR4, 1.0f);  EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 1.5f);  EXPECT(VARIANT_TRUE);

  /* Rounding */
  CONVERT(VarBoolFromR4, -1.5f); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, -0.6f); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, -0.5f); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, -0.4f); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 0.4f);  EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 0.5f);  EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 0.6f);  EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR4, 1.5f);  EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromR8(void)
{
  CONVVARS(DOUBLE);

  /* Hopefully we made the point with R4 above that rounding is
   * irrelevant, so we'll skip that for R8 and Date
   */
  CHECKPTR(VarBoolFromR8);
  CONVERT(VarBoolFromR8, -1.0); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromR8, -0.0); EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromR8, 1.0);  EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromCy(void)
{
  CONVVARS(CY);

  CHECKPTR(VarBoolFromCy);
  CONVERT_CY(VarBoolFromCy, -32769); EXPECT(VARIANT_TRUE);
  CONVERT_CY(VarBoolFromCy, -32768); EXPECT(VARIANT_TRUE);
  CONVERT_CY(VarBoolFromCy, -1);     EXPECT(VARIANT_TRUE);
  CONVERT_CY(VarBoolFromCy, 0);      EXPECT(VARIANT_FALSE);
  CONVERT_CY(VarBoolFromCy, 1);      EXPECT(VARIANT_TRUE);
  CONVERT_CY(VarBoolFromCy, 32767);  EXPECT(VARIANT_TRUE);
  CONVERT_CY(VarBoolFromCy, 32768);  EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromI8(void)
{
  CONVVARS(LONG64);

  CHECKPTR(VarBoolFromI8);
  CONVERT(VarBoolFromI8, -1); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromI8, 0);  EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromI8, 1);  EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromUI8(void)
{
  CONVVARS(ULONG64);

  CHECKPTR(VarBoolFromUI8);
  CONVERT(VarBoolFromUI8, 0); EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromUI8, 1); EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromDec(void)
{
  CONVVARS(DECIMAL);

  CHECKPTR(VarBoolFromDec);
  CONVERT_BADDEC(VarBoolFromDec);

  CONVERT_DEC(VarBoolFromDec,29,0,0,0);   EXPECT_INVALID;
  CONVERT_DEC(VarBoolFromDec,0,0x1,0,0);  EXPECT_INVALID;
  CONVERT_DEC(VarBoolFromDec,0,0x40,0,0); EXPECT_INVALID;
  CONVERT_DEC(VarBoolFromDec,0,0x7f,0,0); EXPECT_INVALID;

  CONVERT_DEC(VarBoolFromDec,0,0x80,0,1); EXPECT(VARIANT_TRUE);
  CONVERT_DEC(VarBoolFromDec,0,0,0,0);    EXPECT(VARIANT_FALSE);
  CONVERT_DEC(VarBoolFromDec,0,0,0,1);    EXPECT(VARIANT_TRUE);
  CONVERT_DEC(VarBoolFromDec,0,0,1,0);    EXPECT(VARIANT_TRUE);

  CONVERT_DEC(VarBoolFromDec,2,0,0,CY_MULTIPLIER);    EXPECT(VARIANT_TRUE);
  CONVERT_DEC(VarBoolFromDec,2,0x80,0,CY_MULTIPLIER); EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromDate(void)
{
  CONVVARS(DATE);

  CHECKPTR(VarBoolFromDate);
  CONVERT(VarBoolFromDate, -1.0); EXPECT(VARIANT_TRUE);
  CONVERT(VarBoolFromDate, -0.0); EXPECT(VARIANT_FALSE);
  CONVERT(VarBoolFromDate, 1.0);  EXPECT(VARIANT_TRUE);
}

static void test_VarBoolFromStr(void)
{
  CONVVARS(LCID);
  OLECHAR buff[128];

  CHECKPTR(VarBoolFromStr);

  in = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  CONVERT_STR(VarBoolFromStr,NULL,0);
  if (hres != E_INVALIDARG)
    EXPECT_MISMATCH;

  /* #FALSE# and #TRUE# Are always accepted */
  CONVERT_STR(VarBoolFromStr,"#FALSE#",0); EXPECT(VARIANT_FALSE);
  CONVERT_STR(VarBoolFromStr,"#TRUE#",0);  EXPECT(VARIANT_TRUE);

  /* Match of #FALSE# and #TRUE# is case sensitive */
  CONVERT_STR(VarBoolFromStr,"#False#",0); EXPECT_MISMATCH;
  /* But match against English is not */
  CONVERT_STR(VarBoolFromStr,"false",0);   EXPECT(VARIANT_FALSE);
  CONVERT_STR(VarBoolFromStr,"False",0);   EXPECT(VARIANT_FALSE);
  /* On/Off and yes/no are not acceptable inputs, with any flags set */
  CONVERT_STR(VarBoolFromStr,"On",0xffffffff);  EXPECT_MISMATCH;
  CONVERT_STR(VarBoolFromStr,"Yes",0xffffffff); EXPECT_MISMATCH;

  /* Change the LCID. This doesn't make any difference for text,unless we ask
   * to check local boolean text with the VARIANT_LOCALBOOL flag. */
  in = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  /* #FALSE# and #TRUE# are accepted in all locales */
  CONVERT_STR(VarBoolFromStr,"#FALSE#",0); EXPECT(VARIANT_FALSE);
  CONVERT_STR(VarBoolFromStr,"#TRUE#",0);  EXPECT(VARIANT_TRUE);
  CONVERT_STR(VarBoolFromStr,"#FALSE#",VARIANT_LOCALBOOL); EXPECT(VARIANT_FALSE);
  CONVERT_STR(VarBoolFromStr,"#TRUE#",VARIANT_LOCALBOOL);  EXPECT(VARIANT_TRUE);

  /* English is accepted regardless of the locale */
  CONVERT_STR(VarBoolFromStr,"false",0); EXPECT(VARIANT_FALSE);
  /* And is still not case sensitive */
  CONVERT_STR(VarBoolFromStr,"False",0); EXPECT(VARIANT_FALSE);

  if (has_locales)
  {
    /* French is rejected without VARIANT_LOCALBOOL */
    CONVERT_STR(VarBoolFromStr,"faux",0); EXPECT_MISMATCH;
    /* But accepted if this flag is given */
    CONVERT_STR(VarBoolFromStr,"faux",VARIANT_LOCALBOOL); EXPECT(VARIANT_FALSE);
    /* Regardless of case - from this we assume locale text comparisons ignore case */
    CONVERT_STR(VarBoolFromStr,"Faux",VARIANT_LOCALBOOL); EXPECT(VARIANT_FALSE);

    /* Changing the locale prevents the localised text from being compared -
     * this demonstrates that only the indicated LCID and English are searched */
    in = MAKELCID(MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT);
    CONVERT_STR(VarBoolFromStr,"faux",VARIANT_LOCALBOOL); EXPECT_MISMATCH;
  }

  /* Numeric strings are read as 0 or non-0 */
  CONVERT_STR(VarBoolFromStr,"0",0);  EXPECT(VARIANT_FALSE);
  CONVERT_STR(VarBoolFromStr,"-1",0); EXPECT(VARIANT_TRUE);
  CONVERT_STR(VarBoolFromStr,"+1",0); EXPECT(VARIANT_TRUE);

  if (has_locales)
  {
    /* Numeric strings are read as floating point numbers. The line below fails
     * because '.' is not a valid decimal separator for Polish numbers */
    CONVERT_STR(VarBoolFromStr,"0.1",LOCALE_NOUSEROVERRIDE); EXPECT_MISMATCH;
  }

  /* Changing the lcid back to US English reads the r8 correctly */
  in = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  CONVERT_STR(VarBoolFromStr,"0.1",LOCALE_NOUSEROVERRIDE); EXPECT(VARIANT_TRUE);
}

static void test_VarBoolCopy(void)
{
  COPYTEST(1, VT_BOOL, V_BOOL(&vSrc), V_BOOL(&vDst), V_BOOLREF(&vSrc), V_BOOLREF(&vDst), "%d");
}

#define BOOL_STR(flags, str) hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, flags, VT_BSTR); \
  ok(hres == S_OK && V_VT(&vDst) == VT_BSTR && \
     V_BSTR(&vDst) && !memcmp(V_BSTR(&vDst), str, sizeof(str)), \
     "hres=0x%X, type=%d (should be VT_BSTR), *bstr='%c'\n", \
     hres, V_VT(&vDst), V_BSTR(&vDst) ? *V_BSTR(&vDst) : '?'); \
  VariantClear(&vDst)

static void test_VarBoolChangeTypeEx(void)
{
  static const WCHAR szTrue[] = { 'T','r','u','e','\0' };
  static const WCHAR szFalse[] = { 'F','a','l','s','e','\0' };
  static const WCHAR szFaux[] = { 'F','a','u','x','\0' };
  HRESULT hres;
  VARIANT_BOOL in;
  VARIANTARG vSrc, vDst;
  LCID lcid;

  in = 1;

  INITIAL_TYPETEST(VT_BOOL, V_BOOL, "%d");
  COMMON_TYPETEST;

  /* The common tests convert to a number. Try the different flags */
  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  V_VT(&vSrc) = VT_BOOL;
  V_BOOL(&vSrc) = 1;

  BOOL_STR(VARIANT_ALPHABOOL, szTrue);
  V_BOOL(&vSrc) = 0;
  BOOL_STR(VARIANT_ALPHABOOL, szFalse);

  if (has_locales)
  {
    lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

    /* VARIANT_ALPHABOOL is always English */
    BOOL_STR(VARIANT_ALPHABOOL, szFalse);
    /* VARIANT_LOCALBOOL uses the localised text */
    BOOL_STR(VARIANT_LOCALBOOL, szFaux);
    /* Both flags together acts as VARIANT_LOCALBOOL */
    BOOL_STR(VARIANT_ALPHABOOL|VARIANT_LOCALBOOL, szFaux);
  }
}

/*
 * BSTR
 */

static void test_VarBstrFromR4(void)
{
  static const WCHAR szNative[] = { '6','5','4','3','2','2','.','3','\0' };
  static const WCHAR szZero[] = {'0', '\0'};
  static const WCHAR szOneHalf_English[] = { '0','.','5','\0' };    /* uses period */
  static const WCHAR szOneHalf_Spanish[] = { '0',',','5','\0' };    /* uses comma */
  LCID lcid;
  LCID lcid_spanish;
  HRESULT hres;
  BSTR bstr = NULL;

  float f;

  CHECKPTR(VarBstrFromR4);

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  lcid_spanish = MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT);
  f = 654322.23456f;
  hres = pVarBstrFromR4(f, lcid, 0, &bstr);
  ok(hres == S_OK, "got hres 0x%08x\n", hres);
  if (bstr)
  {
    todo_wine {
    /* MSDN states that rounding of R4/R8 is dependent on the underlying
     * bit pattern of the number and so is architecture dependent. In this
     * case Wine returns .2 (which is more correct) and Native returns .3
     */
    ok(memcmp(bstr, szNative, sizeof(szNative)) == 0, "string different\n");
    }
    SysFreeString(bstr);
  }

  f = -0.0;
  hres = pVarBstrFromR4(f, lcid, 0, &bstr);
  ok(hres == S_OK, "got hres 0x%08x\n", hres);
  if (bstr)
  {
      if (bstr[0] == '-')
          ok(memcmp(bstr + 1, szZero, sizeof(szZero)) == 0, "negative zero (got %s)\n", wtoascii(bstr));
      else
          ok(memcmp(bstr, szZero, sizeof(szZero)) == 0, "negative zero (got %s)\n", wtoascii(bstr));
      SysFreeString(bstr);
  }
  
  /* The following tests that lcid is used for decimal separator even without LOCALE_USE_NLS */
  f = 0.5;
  hres = pVarBstrFromR4(f, lcid, LOCALE_NOUSEROVERRIDE, &bstr);
  ok(hres == S_OK, "got hres 0x%08x\n", hres);
  if (bstr)
  {
    ok(memcmp(bstr, szOneHalf_English, sizeof(szOneHalf_English)) == 0, "English locale failed (got %s)\n", wtoascii(bstr));
    SysFreeString(bstr);
  }
  f = 0.5;
  hres = pVarBstrFromR4(f, lcid_spanish, LOCALE_NOUSEROVERRIDE, &bstr);
  ok(hres == S_OK, "got hres 0x%08x\n", hres);
  if (bstr)
  {
    ok(memcmp(bstr, szOneHalf_Spanish, sizeof(szOneHalf_Spanish)) == 0, "Spanish locale failed (got %s)\n", wtoascii(bstr));
    SysFreeString(bstr);
  }
}

static void _BSTR_DATE(DATE dt, const char *str, int line)
{
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  char buff[256];
  BSTR bstr = NULL;
  HRESULT hres;

  hres = pVarBstrFromDate(dt, lcid, LOCALE_NOUSEROVERRIDE, &bstr);
  if (bstr)
  {
    WideCharToMultiByte(CP_ACP, 0, bstr, -1, buff, sizeof(buff), 0, 0);
    SysFreeString(bstr);
  }
  else
    buff[0] = 0;
  ok_(__FILE__, line)(hres == S_OK && !strcmp(str, buff),
      "Expected '%s', got '%s', hres = 0x%08x\n", str, buff, hres);
}

static void test_VarBstrFromDate(void)
{
#define BSTR_DATE(dt,str) _BSTR_DATE(dt,str,__LINE__)

  CHECKPTR(VarBstrFromDate);

  BSTR_DATE(0.0, "12:00:00 AM");
  BSTR_DATE(3.34, "1/2/1900 8:09:36 AM");
  BSTR_DATE(3339.34, "2/20/1909 8:09:36 AM");
  BSTR_DATE(365.00, "12/30/1900");
  BSTR_DATE(365.25, "12/30/1900 6:00:00 AM");
  BSTR_DATE(1461.0, "12/31/1903");
  BSTR_DATE(1461.5, "12/31/1903 12:00:00 PM");
  BSTR_DATE(-49192.24, "4/24/1765 5:45:36 AM");
  BSTR_DATE(-657434.0, "1/1/100");
  BSTR_DATE(2958465.0, "12/31/9999");

#undef BSTR_DATE
}

static void _BSTR_CY(LONG a, LONG b, const char *str, LCID lcid, int line)
{
  HRESULT hr;
  BSTR bstr = NULL;
  char buff[256];
  CY l;

  S(l).Lo = b;
  S(l).Hi = a;
  hr = pVarBstrFromCy(l, lcid, LOCALE_NOUSEROVERRIDE, &bstr);
  ok(hr == S_OK, "got hr 0x%08x\n", hr);

  if(bstr)
  {
    WideCharToMultiByte(CP_ACP, 0, bstr, -1, buff, sizeof(buff), 0, 0);
    SysFreeString(bstr);
  }
  else
    buff[0] = 0;

  if(hr == S_OK)
  {
    ok_(__FILE__, line)(!strcmp(str, buff), "Expected '%s', got '%s'\n", str, buff);
  }
}

static void test_VarBstrFromCy(void)
{
#define BSTR_CY(a, b, str, lcid) _BSTR_CY(a, b, str, lcid, __LINE__)

  LCID en_us, sp;

  CHECKPTR(VarBstrFromCy);

  en_us = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  sp = MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT);

  BSTR_CY(0, 0, "0", en_us);
  BSTR_CY(0, 10000, "1", en_us);
  BSTR_CY(0, 15000, "1.5", en_us);
  BSTR_CY(0xffffffff, ((15000)^0xffffffff)+1, "-1.5", en_us);
  /* (1 << 32) - 1 / 1000 */
  BSTR_CY(0, 0xffffffff, "429496.7295", en_us);
  /* (1 << 32) / 1000 */
  BSTR_CY(1, 0, "429496.7296", en_us);
  /* ((1 << 63) - 1)/10000 */
  BSTR_CY(0x7fffffff, 0xffffffff, "922337203685477.5807", en_us);
  BSTR_CY(0, 9, "0.0009", en_us);
  BSTR_CY(0, 9, "0,0009", sp);

#undef BSTR_CY
}

static void _BSTR_DEC(BYTE scale, BYTE sign, ULONG hi, ULONG mid, ULONGLONG lo, const char *str,
    LCID lcid, int line)
{
  char buff[256];
  HRESULT hr;
  BSTR bstr = NULL;
  DECIMAL dec;

  SETDEC64(dec, scale, sign, hi, mid, lo);
  hr = pVarBstrFromDec(&dec, lcid, LOCALE_NOUSEROVERRIDE, &bstr);
  ok_(__FILE__, line)(hr == S_OK, "got hr 0x%08x\n", hr);

  if(bstr)
  {
    WideCharToMultiByte(CP_ACP, 0, bstr, -1, buff, sizeof(buff), 0, 0);
    SysFreeString(bstr);
  }
  else
    buff[0] = 0;

  if(hr == S_OK)
  {
    ok_(__FILE__, line)(!strcmp(str, buff), "Expected '%s', got '%s'\n", str, buff);
  }
}

static void test_VarBstrFromDec(void)
{
#define BSTR_DEC(scale, sign, hi, lo, str, lcid) _BSTR_DEC(scale, sign, hi, 0, lo, str, lcid, __LINE__)
#define BSTR_DEC64(scale, sign, hi, mid, lo, str, lcid) _BSTR_DEC(scale, sign, hi, mid, lo, str, lcid, __LINE__)

  LCID en_us, sp;

  CHECKPTR(VarBstrFromDec);

  en_us = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  sp = MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT);

  BSTR_DEC(0,0,0,0, "0", en_us);

  BSTR_DEC(0,0,0,1,   "1", en_us);
  BSTR_DEC(1,0,0,10,  "1", en_us);
  BSTR_DEC(2,0,0,100, "1", en_us);
  BSTR_DEC(3,0,0,1000,"1", en_us);

  BSTR_DEC(1,0,0,15,  "1.5", en_us);
  BSTR_DEC(2,0,0,150, "1.5", en_us);
  BSTR_DEC(3,0,0,1500,"1.5", en_us);

  BSTR_DEC(1,0x80,0,15, "-1.5", en_us);

  /* (1 << 32) - 1 */
  BSTR_DEC(0,0,0,0xffffffff, "4294967295", en_us);
  /* (1 << 32) */
  BSTR_DEC64(0,0,0,1,0, "4294967296", en_us);
  /* (1 << 64) - 1 */
  BSTR_DEC64(0,0,0,0xffffffff,0xffffffff, "18446744073709551615", en_us);
  /* (1 << 64) */
  BSTR_DEC(0,0,1,0, "18446744073709551616", en_us);
  /* (1 << 96) - 1 */
  BSTR_DEC64(0,0,0xffffffff,0xffffffff,0xffffffff, "79228162514264337593543950335", en_us);
  /* 1 * 10^-10 */
  BSTR_DEC(10,0,0,1, "0.0000000001", en_us);
  /* ((1 << 96) - 1) * 10^-10 */
  BSTR_DEC64(10,0,0xffffffffUL,0xffffffff,0xffffffff, "7922816251426433759.3543950335", en_us);
  /* ((1 << 96) - 1) * 10^-28 */
  BSTR_DEC64(28,0,0xffffffffUL,0xffffffff,0xffffffff, "7.9228162514264337593543950335", en_us);

  /* check leading zeros and decimal sep. for English locale */
  BSTR_DEC(4,0,0,9, "0.0009", en_us);
  BSTR_DEC(5,0,0,90, "0.0009", en_us);
  BSTR_DEC(6,0,0,900, "0.0009", en_us);
  BSTR_DEC(7,0,0,9000, "0.0009", en_us);
  
  /* check leading zeros and decimal sep. for Spanish locale */
  BSTR_DEC(4,0,0,9, "0,0009", sp);
  BSTR_DEC(5,0,0,90, "0,0009", sp);
  BSTR_DEC(6,0,0,900, "0,0009", sp);
  BSTR_DEC(7,0,0,9000, "0,0009", sp);

#undef BSTR_DEC
#undef BSTR_DEC64
}

#define _VARBSTRCMP(left,right,lcid,flags,result) \
        hres = pVarBstrCmp(left,right,lcid,flags); \
        ok(hres == result, "VarBstrCmp: expected " #result ", got hres=0x%x\n", hres)
#define VARBSTRCMP(left,right,flags,result) \
        _VARBSTRCMP(left,right,lcid,flags,result)

static void test_VarBstrCmp(void)
{
    LCID lcid;
    HRESULT hres;
    static const WCHAR sz[] = {'W','u','r','s','c','h','t','\0'};
    static const WCHAR szempty[] = {'\0'};
    static const WCHAR sz1[] = { 'a',0 };
    static const WCHAR sz2[] = { 'A',0 };
    static const WCHAR s1[] = { 'a',0 };
    static const WCHAR s2[] = { 'a',0,'b' };
    static const char sb1[] = {1,0,1};
    static const char sb2[] = {1,0,2};
    static const char sbchr0[] = {0,0};
    static const char sbchr00[] = {0,0,0};
    BSTR bstr, bstrempty, bstr2;

    CHECKPTR(VarBstrCmp);
    
    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
    bstr = SysAllocString(sz);
    bstrempty = SysAllocString(szempty);
    
    /* NULL handling. Yepp, MSDN is totally wrong here */
    VARBSTRCMP(NULL,NULL,0,VARCMP_EQ);
    VARBSTRCMP(bstr,NULL,0,VARCMP_GT);
    VARBSTRCMP(NULL,bstr,0,VARCMP_LT);

    /* NULL and empty string comparisons */
    VARBSTRCMP(bstrempty,NULL,0,VARCMP_EQ);
    VARBSTRCMP(NULL,bstrempty,0,VARCMP_EQ);

    SysFreeString(bstr);
    bstr = SysAllocString(sz1);

    bstr2 = SysAllocString(sz2);
    VARBSTRCMP(bstr,bstr2,0,VARCMP_LT);
    VARBSTRCMP(bstr,bstr2,NORM_IGNORECASE,VARCMP_EQ);
    SysFreeString(bstr2);
    /* These two strings are considered equal even though one is
     * NULL-terminated and the other not.
     */
    bstr2 = SysAllocStringLen(s1, sizeof(s1) / sizeof(WCHAR));
    VARBSTRCMP(bstr,bstr2,0,VARCMP_EQ);
    SysFreeString(bstr2);

    /* These two strings are not equal */
    bstr2 = SysAllocStringLen(s2, sizeof(s2) / sizeof(WCHAR));
    VARBSTRCMP(bstr,bstr2,0,VARCMP_LT);
    SysFreeString(bstr2);

    SysFreeString(bstr);

    bstr = SysAllocStringByteLen(sbchr0, sizeof(sbchr0));
    bstr2 = SysAllocStringByteLen(sbchr00, sizeof(sbchr00));
    VARBSTRCMP(bstr,bstrempty,0,VARCMP_GT);
    VARBSTRCMP(bstrempty,bstr,0,VARCMP_LT);
    VARBSTRCMP(bstr2,bstrempty,0,VARCMP_GT);
    VARBSTRCMP(bstr2,bstr,0,VARCMP_EQ);
    SysFreeString(bstr2);
    SysFreeString(bstr);

    /* When (LCID == 0) it should be a binary comparison
     * so these two strings could not match.
     */
    bstr = SysAllocStringByteLen(sb1, sizeof(sb1));
    bstr2 = SysAllocStringByteLen(sb2, sizeof(sb2));
    lcid = 0;
    VARBSTRCMP(bstr,bstr2,0,VARCMP_LT);
    SysFreeString(bstr2);
    SysFreeString(bstr);

    bstr = SysAllocStringByteLen(sbchr0, sizeof(sbchr0));
    bstr2 = SysAllocStringByteLen(sbchr00, sizeof(sbchr00));
    VARBSTRCMP(bstr,bstrempty,0,VARCMP_GT);
    VARBSTRCMP(bstrempty,bstr,0,VARCMP_LT);
    VARBSTRCMP(bstr2,bstrempty,0,VARCMP_GT);
    VARBSTRCMP(bstr2,bstr,0,VARCMP_GT);
    SysFreeString(bstr2);
    SysFreeString(bstr);
    SysFreeString(bstrempty);
}

/* Get the internal representation of a BSTR */
static inline LPINTERNAL_BSTR Get(const BSTR lpszString)
{
  return lpszString ? (LPINTERNAL_BSTR)((char*)lpszString - sizeof(DWORD)) : NULL;
}

static inline BSTR GetBSTR(const LPINTERNAL_BSTR bstr)
{
  return (BSTR)bstr->szString;
}

static void test_SysStringLen(void)
{
  INTERNAL_BSTR bstr;
  BSTR str = GetBSTR(&bstr);

  bstr.dwLen = 0;
  ok (SysStringLen(str) == 0, "Expected dwLen 0, got %d\n", SysStringLen(str));
  bstr.dwLen = 2;
  ok (SysStringLen(str) == 1, "Expected dwLen 1, got %d\n", SysStringLen(str));
}

static void test_SysStringByteLen(void)
{
  INTERNAL_BSTR bstr;
  BSTR str = GetBSTR(&bstr);

  bstr.dwLen = 0;
  ok (SysStringByteLen(str) == 0, "Expected dwLen 0, got %d\n", SysStringByteLen(str));
  bstr.dwLen = 2;
  ok (SysStringByteLen(str) == 2, "Expected dwLen 2, got %d\n", SysStringByteLen(str));
}

static void test_SysAllocString(void)
{
  const OLECHAR szTest[5] = { 'T','e','s','t','\0' };
  BSTR str;

  str = SysAllocString(NULL);
  ok (str == NULL, "Expected NULL, got %p\n", str);

  str = SysAllocString(szTest);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 8, "Expected 8, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szTest), "String different\n");
    SysFreeString(str);
  }
}

static void test_SysAllocStringLen(void)
{
  const OLECHAR szTest[5] = { 'T','e','s','t','\0' };
  BSTR str;

  /* Very early native dlls do not limit the size of strings, so skip this test */
  if (0)
  {
  str = SysAllocStringLen(szTest, 0x80000000);
  ok (str == NULL, "Expected NULL, got %p\n", str);
  }
  
  str = SysAllocStringLen(NULL, 0);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 0, "Expected 0, got %d\n", bstr->dwLen);
    ok (!bstr->szString[0], "String not empty\n");
    SysFreeString(str);
  }

  str = SysAllocStringLen(szTest, 4);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 8, "Expected 8, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szTest), "String different\n");
    SysFreeString(str);
  }
}

static void test_SysAllocStringByteLen(void)
{
  const OLECHAR szTest[10] = { 'T','e','s','t','\0' };
  const CHAR szTestA[6] = { 'T','e','s','t','\0','?' };
  BSTR str;

  if (sizeof(void *) == 4)  /* not limited to 0x80000000 on Win64 */
  {
      str = SysAllocStringByteLen(szTestA, 0x80000000);
      ok (str == NULL, "Expected NULL, got %p\n", str);
  }

  str = SysAllocStringByteLen(szTestA, 0xffffffff);
  ok (str == NULL, "Expected NULL, got %p\n", str);

  str = SysAllocStringByteLen(NULL, 0);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 0, "Expected 0, got %d\n", bstr->dwLen);
    ok (!bstr->szString[0], "String not empty\n");
    SysFreeString(str);
  }

  str = SysAllocStringByteLen(szTestA, 4);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 4, "Expected 4, got %d\n", bstr->dwLen);
    ok (!lstrcmpA((LPCSTR)bstr->szString, szTestA), "String different\n");
    SysFreeString(str);
  }

  /* Odd lengths are allocated rounded up, but truncated at the right position */
  str = SysAllocStringByteLen(szTestA, 3);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    const CHAR szTestTruncA[4] = { 'T','e','s','\0' };
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 3, "Expected 3, got %d\n", bstr->dwLen);
    ok (!lstrcmpA((LPCSTR)bstr->szString, szTestTruncA), "String different\n");
    SysFreeString(str);
  }

  str = SysAllocStringByteLen((LPCSTR)szTest, 8);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr = Get(str);

    ok (bstr->dwLen == 8, "Expected 8, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szTest), "String different\n");
    SysFreeString(str);
  }
}

static void test_SysReAllocString(void)
{
  const OLECHAR szTest[5] = { 'T','e','s','t','\0' };
  const OLECHAR szSmaller[2] = { 'x','\0' };
  const OLECHAR szLarger[7] = { 'L','a','r','g','e','r','\0' };
  BSTR str;

  str = SysAllocStringLen(szTest, 4);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr;
    int changed;

    bstr = Get(str);
    ok (bstr->dwLen == 8, "Expected 8, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szTest), "String different\n");

    changed = SysReAllocString(&str, szSmaller);
    ok (changed == 1, "Expected 1, got %d\n", changed);
    /* Vista creates a new string, but older versions reuse the existing string. */
    /*ok (str == oldstr, "Created new string\n");*/
    bstr = Get(str);
    ok (bstr->dwLen == 2, "Expected 2, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szSmaller), "String different\n");

    changed = SysReAllocString(&str, szLarger);
    ok (changed == 1, "Expected 1, got %d\n", changed);
    /* Early versions always make new strings rather than resizing */
    /* ok (str == oldstr, "Created new string\n"); */
    bstr = Get(str);
    ok (bstr->dwLen == 12, "Expected 12, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szLarger), "String different\n");

    SysFreeString(str);
  }
}

static void test_SysReAllocStringLen(void)
{
  const OLECHAR szTest[5] = { 'T','e','s','t','\0' };
  const OLECHAR szSmaller[2] = { 'x','\0' };
  const OLECHAR szLarger[7] = { 'L','a','r','g','e','r','\0' };
  BSTR str;

  str = SysAllocStringLen(szTest, 4);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    LPINTERNAL_BSTR bstr;
    int changed;

    bstr = Get(str);
    ok (bstr->dwLen == 8, "Expected 8, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szTest), "String different\n");

    changed = SysReAllocStringLen(&str, szSmaller, 1);
    ok (changed == 1, "Expected 1, got %d\n", changed);
    /* Vista creates a new string, but older versions reuse the existing string. */
    /*ok (str == oldstr, "Created new string\n");*/
    bstr = Get(str);
    ok (bstr->dwLen == 2, "Expected 2, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szSmaller), "String different\n");

    changed = SysReAllocStringLen(&str, szLarger, 6);
    ok (changed == 1, "Expected 1, got %d\n", changed);
    /* Early versions always make new strings rather than resizing */
    /* ok (str == oldstr, "Created new string\n"); */
    bstr = Get(str);
    ok (bstr->dwLen == 12, "Expected 12, got %d\n", bstr->dwLen);
    ok (!lstrcmpW(bstr->szString, szLarger), "String different\n");

    changed = SysReAllocStringLen(&str, str, 6);
    ok (changed == 1, "Expected 1, got %d\n", changed);

    SysFreeString(str);
  }

  /* Windows always returns null terminated strings */
  str = SysAllocStringLen(szTest, 4);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    const int CHUNK_SIZE = 64;
    const int STRING_SIZE = 24;
    int changed;
    changed = SysReAllocStringLen(&str, NULL, CHUNK_SIZE);
    ok (changed == 1, "Expected 1, got %d\n", changed);
    ok (str != NULL, "Expected non-NULL\n");
    if (str)
    {
      BSTR oldstr = str;

      /* Filling string */
      memset (str, 0xAB, CHUNK_SIZE * sizeof (OLECHAR));
      /* Checking null terminator */
      changed = SysReAllocStringLen(&str, NULL, STRING_SIZE);
      ok (changed == 1, "Expected 1, got %d\n", changed);
      ok (str != NULL, "Expected non-NULL\n");
      if (str)
      {
        ok (str == oldstr, "Expected reuse of the old string memory\n");
        ok (str[STRING_SIZE] == 0,
            "Expected null terminator, got 0x%04X\n", str[STRING_SIZE]);
        SysFreeString(str);
      }
    }
  }

  /* Some Windows applications use the same pointer for pbstr and psz */
  str = SysAllocStringLen(szTest, 4);
  ok(str != NULL, "Expected non-NULL\n");
  if(str)
  {
      SysReAllocStringLen(&str, str, 1000000);
      ok(SysStringLen(str)==1000000, "Incorrect string length\n");
      ok(!memcmp(szTest, str, 4*sizeof(WCHAR)), "Incorrect string returned\n");

      SysFreeString(str);
  }
}

static void test_BstrCopy(void)
{
  const CHAR szTestA[6] = { 'T','e','s','t','\0','?' };
  const CHAR szTestTruncA[4] = { 'T','e','s','\0' };
  LPINTERNAL_BSTR bstr;
  BSTR str;
  HRESULT hres;
  VARIANT vt1, vt2;

  str = SysAllocStringByteLen(szTestA, 3);
  ok (str != NULL, "Expected non-NULL\n");
  if (str)
  {
    V_VT(&vt1) = VT_BSTR;
    V_BSTR(&vt1) = str;
    V_VT(&vt2) = VT_EMPTY;
    hres = VariantCopy(&vt2, &vt1);
    ok (hres == S_OK,"Failed to copy binary bstring with hres 0x%08x\n", hres);
    bstr = Get(V_BSTR(&vt2));
    ok (bstr->dwLen == 3, "Expected 3, got %d\n", bstr->dwLen);
    ok (!lstrcmpA((LPCSTR)bstr->szString, szTestTruncA), "String different\n");
    VariantClear(&vt2);
    VariantClear(&vt1);
  }
}

static void test_VarBstrCat(void)
{
    static const WCHAR sz1[] = { 'a',0 };
    static const WCHAR sz2[] = { 'b',0 };
    static const WCHAR sz1sz2[] = { 'a','b',0 };
    static const WCHAR s1[] = { 'a',0 };
    static const WCHAR s2[] = { 'b',0 };
    static const WCHAR s1s2[] = { 'a',0,'b',0 };
    static const char str1A[] = "Have ";
    static const char str2A[] = "A Cigar";
    HRESULT ret;
    BSTR str1, str2, res;
    UINT len;

    CHECKPTR(VarBstrCat);

if (0)
{
    /* Crash */
    pVarBstrCat(NULL, NULL, NULL);
}

    /* Concatenation of two NULL strings works */
    ret = pVarBstrCat(NULL, NULL, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == 0, "Expected a 0-length string\n");
    SysFreeString(res);

    str1 = SysAllocString(sz1);

    /* Concatenation with one NULL arg */
    ret = pVarBstrCat(NULL, str1, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == SysStringLen(str1), "Unexpected length\n");
    ok(!memcmp(res, sz1, SysStringLen(str1)), "Unexpected value\n");
    SysFreeString(res);
    ret = pVarBstrCat(str1, NULL, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == SysStringLen(str1), "Unexpected length\n");
    ok(!memcmp(res, sz1, SysStringLen(str1)), "Unexpected value\n");
    SysFreeString(res);

    /* Concatenation of two zero-terminated strings */
    str2 = SysAllocString(sz2);
    ret = pVarBstrCat(str1, str2, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == sizeof(sz1sz2) / sizeof(WCHAR) - 1,
     "Unexpected length\n");
    ok(!memcmp(res, sz1sz2, sizeof(sz1sz2)), "Unexpected value\n");
    SysFreeString(res);

    SysFreeString(str2);
    SysFreeString(str1);

    /* Concatenation of two strings with embedded NULLs */
    str1 = SysAllocStringLen(s1, sizeof(s1) / sizeof(WCHAR));
    str2 = SysAllocStringLen(s2, sizeof(s2) / sizeof(WCHAR));

    ret = pVarBstrCat(str1, str2, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == sizeof(s1s2) / sizeof(WCHAR),
     "Unexpected length\n");
    ok(!memcmp(res, s1s2, sizeof(s1s2)), "Unexpected value\n");
    SysFreeString(res);

    SysFreeString(str2);
    SysFreeString(str1);

    /* Concatenation of ansi BSTRs, both odd byte count not including termination */
    str1 = SysAllocStringByteLen(str1A, sizeof(str1A)-1);
    str2 = SysAllocStringByteLen(str2A, sizeof(str2A)-1);
    len = SysStringLen(str1);
    ok(len == (sizeof(str1A)-1)/sizeof(WCHAR), "got length %u\n", len);
    len = SysStringLen(str2);
    ok(len == (sizeof(str2A)-1)/sizeof(WCHAR), "got length %u\n", len);

    ret = pVarBstrCat(str1, str2, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    len = (sizeof(str1A) + sizeof(str2A) - 2)/sizeof(WCHAR);
    ok(SysStringLen(res) == len, "got %d, expected %u\n", SysStringLen(res), len);
    ok(!memcmp(res, "Have A Cigar", sizeof(str1A) + sizeof(str2A) - 1), "got (%s)\n", (char*)res);
    SysFreeString(res);

    SysFreeString(str2);
    SysFreeString(str1);

    /* Concatenation of ansi BSTRs, both 1 byte length not including termination */
    str1 = SysAllocStringByteLen(str1A, 1);
    str2 = SysAllocStringByteLen(str2A, 1);
    len = SysStringLen(str1);
    ok(len == 0, "got length %u\n", len);
    len = SysStringLen(str2);
    ok(len == 0, "got length %u\n", len);

    ret = pVarBstrCat(str1, str2, &res);
    ok(ret == S_OK, "VarBstrCat failed: %08x\n", ret);
    ok(res != NULL, "Expected a string\n");
    ok(SysStringLen(res) == 1, "got %d, expected 1\n", SysStringLen(res));
    ok(!memcmp(res, "HA", 2), "got (%s)\n", (char*)res);
    SysFreeString(res);

    SysFreeString(str2);
    SysFreeString(str1);
}

/* IUnknown */

static void test_IUnknownClear(void)
{
  HRESULT hres;
  VARIANTARG v;
  DummyDispatch u;
  IUnknown* pu;

  init_test_dispatch(1, VT_UI1, &u);
  pu = (IUnknown*)&u.IDispatch_iface;

  /* Test that IUnknown_Release is called on by-value */
  V_VT(&v) = VT_UNKNOWN;
  V_UNKNOWN(&v) = (IUnknown*)&u.IDispatch_iface;
  hres = VariantClear(&v);
  ok(hres == S_OK && u.ref == 0 && V_VT(&v) == VT_EMPTY,
     "clear unknown: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 0, VT_EMPTY, hres, u.ref, V_VT(&v));

  /* But not when clearing a by-reference*/
  u.ref = 1;
  V_VT(&v) = VT_UNKNOWN|VT_BYREF;
  V_UNKNOWNREF(&v) = &pu;
  hres = VariantClear(&v);
  ok(hres == S_OK && u.ref == 1 && V_VT(&v) == VT_EMPTY,
     "clear dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 1, VT_EMPTY, hres, u.ref, V_VT(&v));
}

static void test_IUnknownCopy(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  DummyDispatch u;
  IUnknown* pu;

  init_test_dispatch(1, VT_UI1, &u);
  pu = (IUnknown*)&u.IDispatch_iface;

  /* AddRef is called on by-value copy */
  VariantInit(&vDst);
  V_VT(&vSrc) = VT_UNKNOWN;
  V_UNKNOWN(&vSrc) = pu;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && u.ref == 2 && V_VT(&vDst) == VT_UNKNOWN,
     "copy unknown: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_EMPTY, hres, u.ref, V_VT(&vDst));

  /* AddRef is skipped on copy of by-reference IDispatch */
  VariantInit(&vDst);
  u.ref = 1;
  V_VT(&vSrc) = VT_UNKNOWN|VT_BYREF;
  V_UNKNOWNREF(&vSrc) = &pu;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && u.ref == 1 && V_VT(&vDst) == (VT_UNKNOWN|VT_BYREF),
     "copy unknown: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 1, VT_DISPATCH, hres, u.ref, V_VT(&vDst));

  /* AddRef is called copying by-reference IDispatch with indirection */
  VariantInit(&vDst);
  u.ref = 1;
  V_VT(&vSrc) = VT_UNKNOWN|VT_BYREF;
  V_UNKNOWNREF(&vSrc) = &pu;
  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK && u.ref == 2 && V_VT(&vDst) == VT_UNKNOWN,
     "copy unknown: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_DISPATCH, hres, u.ref, V_VT(&vDst));

  /* Indirection in place also calls AddRef */
  u.ref = 1;
  V_VT(&vSrc) = VT_UNKNOWN|VT_BYREF;
  V_UNKNOWNREF(&vSrc) = &pu;
  hres = VariantCopyInd(&vSrc, &vSrc);
  ok(hres == S_OK && u.ref == 2 && V_VT(&vSrc) == VT_UNKNOWN,
     "copy unknown: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_DISPATCH, hres, u.ref, V_VT(&vSrc));
}

static void test_IUnknownChangeTypeEx(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  LCID lcid;
  VARTYPE vt;
  DummyDispatch u;
  IUnknown* pu;

  init_test_dispatch(1, VT_UI1, &u);
  pu = (IUnknown*)&u.IDispatch_iface;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  V_VT(&vSrc) = VT_UNKNOWN;
  V_UNKNOWN(&vSrc) = pu;

  /* =>IDispatch in place */
  hres = VariantChangeTypeEx(&vSrc, &vSrc, lcid, 0, VT_DISPATCH);
  ok(hres == S_OK && u.ref == 1 &&
     V_VT(&vSrc) == VT_DISPATCH && V_DISPATCH(&vSrc) == (IDispatch*)pu,
     "change unk(src=src): expected 0x%08x,%d,%d,%p, got 0x%08x,%d,%d,%p\n",
     S_OK, 1, VT_DISPATCH, pu, hres, u.ref, V_VT(&vSrc), V_DISPATCH(&vSrc));

  /* =>IDispatch */
  u.ref = 1;
  V_VT(&vSrc) = VT_UNKNOWN;
  V_UNKNOWN(&vSrc) = pu;
  VariantInit(&vDst);
  hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, VT_UNKNOWN);
  /* Note vSrc is not cleared, as final refcount is 2 */
  ok(hres == S_OK && u.ref == 2 &&
     V_VT(&vDst) == VT_UNKNOWN && V_UNKNOWN(&vDst) == pu,
     "change unk(src,dst): expected 0x%08x,%d,%d,%p, got 0x%08x,%d,%d,%p\n",
     S_OK, 2, VT_UNKNOWN, pu, hres, u.ref, V_VT(&vDst), V_UNKNOWN(&vDst));

  /* Can't change unknown to anything else */
  for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
  {
    HRESULT hExpected = DISP_E_BADVARTYPE;

    V_VT(&vSrc) = VT_UNKNOWN;
    V_UNKNOWN(&vSrc) = pu;
    VariantInit(&vDst);

    if (vt == VT_UNKNOWN || vt == VT_DISPATCH || vt == VT_EMPTY || vt == VT_NULL)
      hExpected = S_OK;
    else
    {
      if (vt == VT_I8 || vt == VT_UI8)
      {
        if (has_i8)
          hExpected = DISP_E_TYPEMISMATCH;
      }
      else if (vt == VT_RECORD)
      {
        hExpected = DISP_E_TYPEMISMATCH;
      }
      else if (vt  >= VT_I2 && vt <= VT_UINT && vt != (VARTYPE)15)
        hExpected = DISP_E_TYPEMISMATCH;
    }

    hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, vt);
    ok(hres == hExpected,
       "change unk(badvar): vt %d expected 0x%08x, got 0x%08x\n",
       vt, hExpected, hres);
  }
}

/* IDispatch */
static void test_IDispatchClear(void)
{
  HRESULT hres;
  VARIANTARG v;
  DummyDispatch d;
  IDispatch* pd;

  init_test_dispatch(1, VT_UI1, &d);
  pd = &d.IDispatch_iface;

  /* As per IUnknown */

  V_VT(&v) = VT_DISPATCH;
  V_DISPATCH(&v) = pd;
  hres = VariantClear(&v);
  ok(hres == S_OK && d.ref == 0 && V_VT(&v) == VT_EMPTY,
     "clear dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 0, VT_EMPTY, hres, d.ref, V_VT(&v));

  d.ref = 1;
  V_VT(&v) = VT_DISPATCH|VT_BYREF;
  V_DISPATCHREF(&v) = &pd;
  hres = VariantClear(&v);
  ok(hres == S_OK && d.ref == 1 && V_VT(&v) == VT_EMPTY,
     "clear dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 1, VT_EMPTY, hres, d.ref, V_VT(&v));
}

static void test_IDispatchCopy(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  DummyDispatch d;
  IDispatch* pd;

  init_test_dispatch(1, VT_UI1, &d);
  pd = &d.IDispatch_iface;

  /* As per IUnknown */

  VariantInit(&vDst);
  V_VT(&vSrc) = VT_DISPATCH;
  V_DISPATCH(&vSrc) = pd;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && d.ref == 2 && V_VT(&vDst) == VT_DISPATCH,
     "copy dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_EMPTY, hres, d.ref, V_VT(&vDst));

  VariantInit(&vDst);
  d.ref = 1;
  V_VT(&vSrc) = VT_DISPATCH|VT_BYREF;
  V_DISPATCHREF(&vSrc) = &pd;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && d.ref == 1 && V_VT(&vDst) == (VT_DISPATCH|VT_BYREF),
     "copy dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 1, VT_DISPATCH, hres, d.ref, V_VT(&vDst));

  VariantInit(&vDst);
  d.ref = 1;
  V_VT(&vSrc) = VT_DISPATCH|VT_BYREF;
  V_DISPATCHREF(&vSrc) = &pd;
  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK && d.ref == 2 && V_VT(&vDst) == VT_DISPATCH,
     "copy dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_DISPATCH, hres, d.ref, V_VT(&vDst));

  d.ref = 1;
  V_VT(&vSrc) = VT_DISPATCH|VT_BYREF;
  V_DISPATCHREF(&vSrc) = &pd;
  hres = VariantCopyInd(&vSrc, &vSrc);
  ok(hres == S_OK && d.ref == 2 && V_VT(&vSrc) == VT_DISPATCH,
     "copy dispatch: expected 0x%08x, %d, %d, got 0x%08x, %d, %d\n",
     S_OK, 2, VT_DISPATCH, hres, d.ref, V_VT(&vSrc));
}

static void test_IDispatchChangeTypeEx(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  LCID lcid;
  DummyDispatch d;
  IDispatch* pd;

  init_test_dispatch(1, VT_UI1, &d);
  pd = &d.IDispatch_iface;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  V_VT(&vSrc) = VT_DISPATCH;
  V_DISPATCH(&vSrc) = pd;

  /* =>IUnknown in place */
  hres = VariantChangeTypeEx(&vSrc, &vSrc, lcid, 0, VT_UNKNOWN);
  ok(hres == S_OK && d.ref == 1 &&
     V_VT(&vSrc) == VT_UNKNOWN && V_UNKNOWN(&vSrc) == (IUnknown*)pd,
     "change disp(src=src): expected 0x%08x,%d,%d,%p, got 0x%08x,%d,%d,%p\n",
     S_OK, 1, VT_UNKNOWN, pd, hres, d.ref, V_VT(&vSrc), V_UNKNOWN(&vSrc));

  /* =>IUnknown */
  d.ref = 1;
  V_VT(&vSrc) = VT_DISPATCH;
  V_DISPATCH(&vSrc) = pd;
  VariantInit(&vDst);
  hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, VT_UNKNOWN);
  /* Note vSrc is not cleared, as final refcount is 2 */
  ok(hres == S_OK && d.ref == 2 &&
     V_VT(&vDst) == VT_UNKNOWN && V_UNKNOWN(&vDst) == (IUnknown*)pd,
     "change disp(src,dst): expected 0x%08x,%d,%d,%p, got 0x%08x,%d,%d,%p\n",
     S_OK, 2, VT_UNKNOWN, pd, hres, d.ref, V_VT(&vDst), V_UNKNOWN(&vDst));

  /* FIXME: Verify that VARIANT_NOVALUEPROP prevents conversion to integral
   *        types. this requires that the xxxFromDisp tests work first.
   */
}

/* VT_ERROR */
static void test_ErrorChangeTypeEx(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  VARTYPE vt;
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
  {
    HRESULT hExpected = DISP_E_BADVARTYPE;

    V_VT(&vSrc) = VT_ERROR;
    V_ERROR(&vSrc) = 1;
    VariantInit(&vDst);
    hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, vt);

    if (vt == VT_ERROR)
      hExpected = S_OK;
    else
    {
      if (vt == VT_I8 || vt == VT_UI8)
      {
        if (has_i8)
          hExpected = DISP_E_TYPEMISMATCH;
      }
      else if (vt == VT_RECORD)
      {
        hExpected = DISP_E_TYPEMISMATCH;
      }
      else if (vt <= VT_UINT && vt != (VARTYPE)15)
        hExpected = DISP_E_TYPEMISMATCH;
    }

    ok(hres == hExpected,
     "change err: vt %d expected 0x%08x, got 0x%08x\n", vt, hExpected, hres);
  }
}

/* VT_EMPTY */
static void test_EmptyChangeTypeEx(void)
{
  VARTYPE vt;
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  for (vt = VT_EMPTY; vt <= VT_BSTR_BLOB; vt++)
  {
    HRESULT hExpected, hres;
    VARIANTARG vSrc, vDst;

    /* skip for undefined types */
    if ((vt == 15) || (vt > VT_VERSIONED_STREAM && vt < VT_BSTR_BLOB))
        continue;

    switch (vt)
    {
    case VT_I8:
    case VT_UI8:
      if (has_i8)
        hExpected = S_OK;
      else
        hExpected = DISP_E_BADVARTYPE;
      break;
    case VT_RECORD:
    case VT_VARIANT:
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_ERROR:
      hExpected = DISP_E_TYPEMISMATCH;
      break;
    case VT_EMPTY:
    case VT_NULL:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_I1:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
      hExpected = S_OK;
      break;
    default:
      hExpected = DISP_E_BADVARTYPE;
    }

    VariantInit(&vSrc);
    V_VT(&vSrc) = VT_EMPTY;
    memset(&vDst, 0, sizeof(vDst));
    V_VT(&vDst) = VT_NULL;

    hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, vt);
    ok(hres == hExpected, "change empty: vt %d expected 0x%08x, got 0x%08x, vt %d\n",
        vt, hExpected, hres, V_VT(&vDst));
    if (hres == S_OK)
    {
        ok(V_VT(&vDst) == vt, "change empty: vt %d, got %d\n", vt, V_VT(&vDst));
        VariantClear(&vDst);
    }
  }
}

/* VT_NULL */
static void test_NullChangeTypeEx(void)
{
  VARTYPE vt;
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  for (vt = VT_EMPTY; vt <= VT_BSTR_BLOB; vt++)
  {
    VARIANTARG vSrc, vDst;
    HRESULT hExpected, hres;

    /* skip for undefined types */
    if ((vt == 15) || (vt > VT_VERSIONED_STREAM && vt < VT_BSTR_BLOB))
        continue;

    switch (vt)
    {
    case VT_I8:
    case VT_UI8:
        if (has_i8)
            hExpected = DISP_E_TYPEMISMATCH;
        else
            hExpected = DISP_E_BADVARTYPE;
        break;
    case VT_NULL:
        hExpected = S_OK;
        break;
    case VT_EMPTY:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_DISPATCH:
    case VT_ERROR:
    case VT_BOOL:
    case VT_VARIANT:
    case VT_UNKNOWN:
    case VT_DECIMAL:
    case VT_I1:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_RECORD:
        hExpected = DISP_E_TYPEMISMATCH;
        break;
    default:
        hExpected = DISP_E_BADVARTYPE;
    }

    VariantInit(&vSrc);
    V_VT(&vSrc) = VT_NULL;
    memset(&vDst, 0, sizeof(vDst));
    V_VT(&vDst) = VT_EMPTY;

    hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, vt);
    ok(hres == hExpected, "change null: vt %d expected 0x%08x, got 0x%08x, vt %d\n",
       vt, hExpected, hres, V_VT(&vDst));

    /* should work only for VT_NULL -> VT_NULL case */
    if (hres == S_OK)
        ok(V_VT(&vDst) == VT_NULL, "change null: VT_NULL expected 0x%08x, got 0x%08x, vt %d\n",
            hExpected, hres, V_VT(&vDst));
    else
        ok(V_VT(&vDst) == VT_EMPTY, "change null: vt %d expected 0x%08x, got 0x%08x, vt %d\n",
            vt, hExpected, hres, V_VT(&vDst));
  }
}

 
/* VT_UINT */
static void test_UintChangeTypeEx(void)
{
  HRESULT hres;
  VARIANTARG vSrc, vDst;
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  /* Converting a VT_UINT to a VT_INT does not check for overflow */
  V_VT(&vDst) = VT_EMPTY;
  V_VT(&vSrc) = VT_UINT;
  V_UI4(&vSrc) = -1;
  hres = VariantChangeTypeEx(&vDst, &vSrc, lcid, 0, VT_I4);
  ok(hres == S_OK && V_VT(&vDst) == VT_I4 && V_I4(&vDst) == -1,
     "change uint: Expected %d,0x%08x,%d got %d,0x%08x,%d\n",
     VT_I4, S_OK, -1, V_VT(&vDst), hres, V_I4(&vDst));
}

#define NUM_CUST_ITEMS 16

static void test_ClearCustData(void)
{
  CUSTDATA ci;
  unsigned i;

  CHECKPTR(ClearCustData);

  ci.cCustData = NUM_CUST_ITEMS;
  ci.prgCustData = CoTaskMemAlloc( sizeof(CUSTDATAITEM) * NUM_CUST_ITEMS );
  for (i = 0; i < NUM_CUST_ITEMS; i++)
    VariantInit(&ci.prgCustData[i].varValue);
  pClearCustData(&ci);
  ok(!ci.cCustData && !ci.prgCustData, "ClearCustData didn't clear fields!\n");
}

static void test_NullByRef(void)
{
  VARIANT v1, v2;
  HRESULT hRes;

  VariantInit(&v1);
  VariantInit(&v2);
  V_VT(&v1) = VT_BYREF|VT_VARIANT;
  V_BYREF(&v1) = 0;

  hRes = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_I4);
  ok(hRes == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx should return DISP_E_TYPEMISMATCH\n");

  VariantClear(&v1);
  V_VT(&v1) = VT_BYREF|VT_VARIANT;
  V_BYREF(&v1) = 0;
  V_VT(&v2) = VT_I4;
  V_I4(&v2) = 123;

  hRes = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_VARIANT);
  ok(hRes == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx should return DISP_E_TYPEMISMATCH\n");
  ok(V_VT(&v2) == VT_I4 && V_I4(&v2) == 123, "VariantChangeTypeEx shouldn't change pvargDest\n");

  hRes = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_BYREF|VT_I4);
  ok(hRes == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx should return DISP_E_TYPEMISMATCH\n");

  hRes = VariantChangeTypeEx(&v2, &v1, 0, 0, 0x3847);
  ok(hRes == DISP_E_BADVARTYPE, "VariantChangeTypeEx should return DISP_E_BADVARTYPE\n");
}

/* Dst Variant should remain unchanged if VariantChangeType cannot convert */
static void test_ChangeType_keep_dst(void)
{
     VARIANT v1, v2;
     BSTR bstr;
     static const WCHAR testW[] = {'t','e','s','t',0};
     HRESULT hres;

     bstr = SysAllocString(testW);
     VariantInit(&v1);
     VariantInit(&v2);
     V_VT(&v1) = VT_BSTR;
     V_BSTR(&v1) = bstr;
     hres = VariantChangeTypeEx(&v1, &v1, 0, 0, VT_INT);
     ok(hres == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx returns %08x\n", hres);
     ok(V_VT(&v1) == VT_BSTR && V_BSTR(&v1) == bstr, "VariantChangeTypeEx changed dst variant\n");
     V_VT(&v2) = VT_INT;
     V_INT(&v2) = 4;
     hres = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_INT);
     ok(hres == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx returns %08x\n", hres);
     ok(V_VT(&v2) == VT_INT && V_INT(&v2) == 4, "VariantChangeTypeEx changed dst variant\n");
     V_VT(&v2) = 0xff; /* incorrect variant type */
     hres = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_INT);
     ok(hres == DISP_E_TYPEMISMATCH, "VariantChangeTypeEx returns %08x\n", hres);
     ok(V_VT(&v2) == 0xff, "VariantChangeTypeEx changed dst variant\n");
     hres = VariantChangeTypeEx(&v2, &v1, 0, 0, VT_BSTR);
     ok(hres == DISP_E_BADVARTYPE, "VariantChangeTypeEx returns %08x\n", hres);
     ok(V_VT(&v2) == 0xff, "VariantChangeTypeEx changed dst variant\n");
     SysFreeString(bstr);
}

/* This tests assumes an empty cache, so it needs to be ran early in the test. */
static void test_bstr_cache(void)
{
    BSTR str, str2, strs[20];
    unsigned i;

    static const WCHAR testW[] = {'t','e','s','t',0};

    if (GetEnvironmentVariableA("OANOCACHE", NULL, 0)) {
        skip("BSTR cache is disabled, some tests will be skipped.\n");
        return;
    }

    str = SysAllocString(testW);
    /* This should put the string into cache */
    SysFreeString(str);
    /* The string is in cache, this won't touch it */
    SysFreeString(str);

    ok(SysStringLen(str) == 4, "unexpected len\n");
    ok(!lstrcmpW(str, testW), "string changed\n");

    str2 = SysAllocString(testW);
    ok(str == str2, "str != str2\n");
    SysFreeString(str2);

    /* Fill the bucket with cached entries. */
    for(i=0; i < sizeof(strs)/sizeof(*strs); i++)
        strs[i] = SysAllocStringLen(NULL, 24);
    for(i=0; i < sizeof(strs)/sizeof(*strs); i++)
        SysFreeString(strs[i]);

    /* Following allocation will be made from cache */
    str = SysAllocStringLen(NULL, 24);
    ok(str == strs[0], "str != strs[0]\n");

    /* Smaller buffers may also use larget cached buffers */
    str2 = SysAllocStringLen(NULL, 16);
    ok(str2 == strs[1], "str2 != strs[1]\n");

    SysFreeString(str);
    SysFreeString(str2);
    SysFreeString(str);
    SysFreeString(str2);
}

static void write_typelib(int res_no, const char *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed\n" );
    if (file == INVALID_HANDLE_VALUE) return;
    res = FindResourceA( GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(res_no), "TYPELIB" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static const char *create_test_typelib(int res_no)
{
    static char filename[MAX_PATH];

    GetTempFileNameA( ".", "tlb", 0, filename );
    write_typelib(res_no, filename);
    return filename;
}

static void test_recinfo(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    static WCHAR teststructW[] = {'t','e','s','t','_','s','t','r','u','c','t',0};
    static WCHAR teststruct2W[] = {'t','e','s','t','_','s','t','r','u','c','t','2',0};
    static WCHAR teststruct3W[] = {'t','e','s','t','_','s','t','r','u','c','t','3',0};
    WCHAR filenameW[MAX_PATH], filename2W[MAX_PATH];
    ITypeInfo *typeinfo, *typeinfo2, *typeinfo3;
    IRecordInfo *recinfo, *recinfo2, *recinfo3;
    struct test_struct teststruct, testcopy;
    ITypeLib *typelib, *typelib2;
    const char *filename;
    DummyDispatch dispatch;
    TYPEATTR *attr;
    MEMBERID memid;
    UINT16 found;
    HRESULT hr;
    ULONG size;
    BOOL ret;

    filename = create_test_typelib(2);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);
    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &typelib);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    filename = create_test_typelib(3);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename2W, MAX_PATH);
    hr = LoadTypeLibEx(filename2W, REGKIND_NONE, &typelib2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    typeinfo = NULL;
    found = 1;
    hr = ITypeLib_FindName(typelib, teststructW, 0, &typeinfo, &memid, &found);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(typeinfo != NULL, "got %p\n", typeinfo);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&attr->guid, &UUID_test_struct), "got %s\n", wine_dbgstr_guid(&attr->guid));
    ok(attr->typekind == TKIND_RECORD, "got %d\n", attr->typekind);

    typeinfo2 = NULL;
    found = 1;
    hr = ITypeLib_FindName(typelib, teststruct2W, 0, &typeinfo2, &memid, &found);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(typeinfo2 != NULL, "got %p\n", typeinfo2);

    typeinfo3 = NULL;
    found = 1;
    hr = ITypeLib_FindName(typelib2, teststruct3W, 0, &typeinfo3, &memid, &found);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(typeinfo3 != NULL, "got %p\n", typeinfo3);

    hr = GetRecordInfoFromTypeInfo(typeinfo, &recinfo);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = GetRecordInfoFromTypeInfo(typeinfo2, &recinfo2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = GetRecordInfoFromTypeInfo(typeinfo3, &recinfo3);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* IsMatchingType, these two records only differ in GUIDs */
    ret = IRecordInfo_IsMatchingType(recinfo, recinfo2);
    ok(!ret, "got %d\n", ret);

    /* these two have same GUIDs, but different set of fields */
    ret = IRecordInfo_IsMatchingType(recinfo2, recinfo3);
    ok(ret, "got %d\n", ret);

    IRecordInfo_Release(recinfo3);
    ITypeInfo_Release(typeinfo3);
    IRecordInfo_Release(recinfo2);
    ITypeInfo_Release(typeinfo2);

    size = 0;
    hr = IRecordInfo_GetSize(recinfo, &size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size == sizeof(struct test_struct), "got size %d\n", size);
    ok(attr->cbSizeInstance == sizeof(struct test_struct), "got instance size %d\n", attr->cbSizeInstance);
    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);

    /* RecordInit() */
    teststruct.hr = E_FAIL;
    teststruct.b = 0x1;
    teststruct.disp = (void*)0xdeadbeef;
    teststruct.bstr = (void*)0xdeadbeef;

    hr = IRecordInfo_RecordInit(recinfo, &teststruct);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(teststruct.hr == 0, "got 0x%08x\n", teststruct.hr);
    ok(teststruct.b == 0, "got 0x%08x\n", teststruct.b);
    ok(teststruct.disp == NULL, "got %p\n", teststruct.disp);
    ok(teststruct.bstr == NULL, "got %p\n", teststruct.bstr);

    init_test_dispatch(10, VT_UI1, &dispatch);

    /* RecordCopy(), interface field reference increased */
    teststruct.hr = S_FALSE;
    teststruct.b = VARIANT_TRUE;
    teststruct.disp = &dispatch.IDispatch_iface;
    teststruct.bstr = SysAllocString(testW);
    memset(&testcopy, 0, sizeof(testcopy));
    hr = IRecordInfo_RecordCopy(recinfo, &teststruct, &testcopy);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(testcopy.hr == S_FALSE, "got 0x%08x\n", testcopy.hr);
    ok(testcopy.b == VARIANT_TRUE, "got %d\n", testcopy.b);
    ok(testcopy.disp == teststruct.disp, "got %p\n", testcopy.disp);
    ok(dispatch.ref == 11, "got %d\n", dispatch.ref);
    ok(testcopy.bstr != teststruct.bstr, "got %p\n", testcopy.bstr);
    ok(!lstrcmpW(testcopy.bstr, teststruct.bstr), "got %s, %s\n", wine_dbgstr_w(testcopy.bstr), wine_dbgstr_w(teststruct.bstr));

    /* RecordClear() */
    hr = IRecordInfo_RecordClear(recinfo, &teststruct);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(teststruct.bstr == NULL, "got %p\n", teststruct.bstr);
    hr = IRecordInfo_RecordClear(recinfo, &testcopy);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(testcopy.bstr == NULL, "got %p\n", testcopy.bstr);

    /* now the destination contains the interface pointer */
    memset(&testcopy, 0, sizeof(testcopy));
    testcopy.disp = &dispatch.IDispatch_iface;
    dispatch.ref = 10;

    hr = IRecordInfo_RecordCopy(recinfo, &teststruct, &testcopy);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(dispatch.ref == 9, "got %d\n", dispatch.ref);

    IRecordInfo_Release(recinfo);

    ITypeInfo_Release(typeinfo);
    ITypeLib_Release(typelib);
    DeleteFileW(filenameW);
    DeleteFileW(filename2W);
}

START_TEST(vartype)
{
  hOleaut32 = GetModuleHandleA("oleaut32.dll");

  has_i8 = GetProcAddress(hOleaut32, "VarI8FromI1") != NULL;
  has_locales = has_i8 && GetProcAddress(hOleaut32, "GetVarConversionLocaleSetting") != NULL;

  trace("LCIDs: System=0x%08x, User=0x%08x\n", GetSystemDefaultLCID(),
        GetUserDefaultLCID());

  test_bstr_cache();

  test_VarI1FromI2();
  test_VarI1FromI4();
  test_VarI1FromI8();
  test_VarI1FromUI1();
  test_VarI1FromUI2();
  test_VarI1FromUI4();
  test_VarI1FromUI8();
  test_VarI1FromBool();
  test_VarI1FromR4();
  test_VarI1FromR8();
  test_VarI1FromDate();
  test_VarI1FromCy();
  test_VarI1FromDec();
  test_VarI1FromStr();
  test_VarUI1FromDisp();
  test_VarI1Copy();
  test_VarI1ChangeTypeEx();

  test_VarUI1FromI1();
  test_VarUI1FromI2();
  test_VarUI1FromI4();
  test_VarUI1FromI8();
  test_VarUI1FromUI2();
  test_VarUI1FromUI4();
  test_VarUI1FromUI8();
  test_VarUI1FromBool();
  test_VarUI1FromR4();
  test_VarUI1FromR8();
  test_VarUI1FromDate();
  test_VarUI1FromCy();
  test_VarUI1FromDec();
  test_VarUI1FromStr();
  test_VarUI1Copy();
  test_VarUI1ChangeTypeEx();

  test_VarI2FromI1();
  test_VarI2FromI4();
  test_VarI2FromI8();
  test_VarI2FromUI1();
  test_VarI2FromUI2();
  test_VarI2FromUI4();
  test_VarI2FromUI8();
  test_VarI2FromBool();
  test_VarI2FromR4();
  test_VarI2FromR8();
  test_VarI2FromDate();
  test_VarI2FromCy();
  test_VarI2FromDec();
  test_VarI2FromStr();
  test_VarI2Copy();
  test_VarI2ChangeTypeEx();

  test_VarUI2FromI1();
  test_VarUI2FromI2();
  test_VarUI2FromI4();
  test_VarUI2FromI8();
  test_VarUI2FromUI1();
  test_VarUI2FromUI4();
  test_VarUI2FromUI8();
  test_VarUI2FromBool();
  test_VarUI2FromR4();
  test_VarUI2FromR8();
  test_VarUI2FromDate();
  test_VarUI2FromCy();
  test_VarUI2FromDec();
  test_VarUI2FromStr();
  test_VarUI2Copy();
  test_VarUI2ChangeTypeEx();

  test_VarI4FromI1();
  test_VarI4FromI2();
  test_VarI4FromI8();
  test_VarI4FromUI1();
  test_VarI4FromUI2();
  test_VarI4FromUI4();
  test_VarI4FromUI8();
  test_VarI4FromBool();
  test_VarI4FromR4();
  test_VarI4FromR8();
  test_VarI4FromDate();
  test_VarI4FromCy();
  test_VarI4FromDec();
  test_VarI4FromStr();
  test_VarI4Copy();
  test_VarI4ChangeTypeEx();

  test_VarUI4FromI1();
  test_VarUI4FromI2();
  test_VarUI4FromUI2();
  test_VarUI4FromI8();
  test_VarUI4FromUI1();
  test_VarUI4FromI4();
  test_VarUI4FromUI8();
  test_VarUI4FromBool();
  test_VarUI4FromR4();
  test_VarUI4FromR8();
  test_VarUI4FromDate();
  test_VarUI4FromCy();
  test_VarUI4FromDec();
  test_VarUI4FromStr();
  test_VarUI4Copy();
  test_VarUI4ChangeTypeEx();

  test_VarI8FromI1();
  test_VarI8FromUI1();
  test_VarI8FromI2();
  test_VarI8FromUI2();
  test_VarI8FromUI4();
  test_VarI8FromR4();
  test_VarI8FromR8();
  test_VarI8FromBool();
  test_VarI8FromUI8();
  test_VarI8FromCy();
  test_VarI8FromDec();
  test_VarI8FromDate();
  test_VarI8FromStr();
  test_VarI8Copy();
  test_VarI8ChangeTypeEx();

  test_VarUI8FromI1();
  test_VarUI8FromUI1();
  test_VarUI8FromI2();
  test_VarUI8FromUI2();
  test_VarUI8FromUI4();
  test_VarUI8FromR4();
  test_VarUI8FromR8();
  test_VarUI8FromBool();
  test_VarUI8FromI8();
  test_VarUI8FromCy();
  test_VarUI8FromDec();
  test_VarUI8FromDate();
  test_VarUI8FromStr();
  test_VarUI8Copy();
  test_VarUI8ChangeTypeEx();

  test_VarR4FromI1();
  test_VarR4FromUI1();
  test_VarR4FromI2();
  test_VarR4FromUI2();
  test_VarR4FromI4();
  test_VarR4FromUI4();
  test_VarR4FromR8();
  test_VarR4FromBool();
  test_VarR4FromCy();
  test_VarR4FromI8();
  test_VarR4FromUI8();
  test_VarR4FromDec();
  test_VarR4FromDate();
  test_VarR4FromStr();
  test_VarR4Copy();
  test_VarR4ChangeTypeEx();

  test_VarR8FromI1();
  test_VarR8FromUI1();
  test_VarR8FromI2();
  test_VarR8FromUI2();
  test_VarR8FromI4();
  test_VarR8FromUI4();
  test_VarR8FromR4();
  test_VarR8FromBool();
  test_VarR8FromCy();
  test_VarR8FromI8();
  test_VarR8FromUI8();
  test_VarR8FromDec();
  test_VarR8FromDate();
  test_VarR8FromStr();
  test_VarR8Copy();
  test_VarR8ChangeTypeEx();
  test_VarR8Round();

  test_VarDateFromI1();
  test_VarDateFromUI1();
  test_VarDateFromI2();
  test_VarDateFromUI2();
  test_VarDateFromI4();
  test_VarDateFromUI4();
  test_VarDateFromR4();
  test_VarDateFromR8();
  test_VarDateFromBool();
  test_VarDateFromCy();
  test_VarDateFromI8();
  test_VarDateFromUI8();
  test_VarDateFromDec();
  test_VarDateFromStr();
  test_VarDateCopy();
  test_VarDateChangeTypeEx();

  test_VarCyFromI1();
  test_VarCyFromUI1();
  test_VarCyFromI2();
  test_VarCyFromUI2();
  test_VarCyFromI4();
  test_VarCyFromUI4();
  test_VarCyFromR4();
  test_VarCyFromR8();
  test_VarCyFromBool();
  test_VarCyFromI8();
  test_VarCyFromUI8();
  test_VarCyFromDec();
  test_VarCyFromDate();

  test_VarCyAdd();
  test_VarCyMul();
  test_VarCySub();
  test_VarCyAbs();
  test_VarCyNeg();
  test_VarCyMulI4();
  test_VarCyMulI8();
  test_VarCyCmp();
  test_VarCyCmpR8();
  test_VarCyRound();
  test_VarCyFix();
  test_VarCyInt();

  test_VarDecFromI1();
  test_VarDecFromI2();
  test_VarDecFromI4();
  test_VarDecFromI8();
  test_VarDecFromUI1();
  test_VarDecFromUI2();
  test_VarDecFromUI4();
  test_VarDecFromUI8();
  test_VarDecFromR4();
  test_VarDecFromR8();
  test_VarDecFromDate();
  test_VarDecFromStr();
  test_VarDecFromCy();
  test_VarDecFromDate();
  test_VarDecFromBool();

  test_VarDecAbs();
  test_VarDecNeg();
  test_VarDecAdd();
  test_VarDecSub();
  test_VarDecCmp();
  test_VarDecCmpR8();
  test_VarDecMul();
  test_VarDecDiv();
  test_VarDecRound();

  test_VarBoolFromI1();
  test_VarBoolFromUI1();
  test_VarBoolFromI2();
  test_VarBoolFromUI2();
  test_VarBoolFromI4();
  test_VarBoolFromUI4();
  test_VarBoolFromR4();
  test_VarBoolFromR8();
  test_VarBoolFromCy();
  test_VarBoolFromI8();
  test_VarBoolFromUI8();
  test_VarBoolFromDec();
  test_VarBoolFromDate();
  test_VarBoolFromStr();
  test_VarBoolCopy();
  test_VarBoolChangeTypeEx();

  test_VarBstrFromR4();
  test_VarBstrFromDate();
  test_VarBstrFromCy();
  test_VarBstrFromDec();
  test_VarBstrCmp();
  test_SysStringLen();
  test_SysStringByteLen();
  test_SysAllocString();
  test_SysAllocStringLen();
  test_SysAllocStringByteLen();
  test_SysReAllocString();
  test_SysReAllocStringLen();
  test_BstrCopy();
  test_VarBstrCat();

  test_IUnknownClear();
  test_IUnknownCopy();
  test_IUnknownChangeTypeEx();

  test_IDispatchClear();
  test_IDispatchCopy();
  test_IDispatchChangeTypeEx();

  test_ErrorChangeTypeEx();
  test_EmptyChangeTypeEx();
  test_NullChangeTypeEx();
  test_UintChangeTypeEx();

  test_ClearCustData();

  test_NullByRef();
  test_ChangeType_keep_dst();

  test_recinfo();
}
