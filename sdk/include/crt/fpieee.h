/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_FPIEEE
#define _INC_FPIEEE

#include <corecrt.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum {
    _FpCompareEqual,_FpCompareGreater,_FpCompareLess,_FpCompareUnordered
  } _FPIEEE_COMPARE_RESULT;

  typedef enum {
    _FpFormatFp32,_FpFormatFp64,_FpFormatFp80,_FpFormatFp128,_FpFormatI16,_FpFormatI32,_FpFormatI64,_FpFormatU16,_FpFormatU32,_FpFormatU64,_FpFormatBcd80,_FpFormatCompare,_FpFormatString,
#if defined(__ia64__)
    _FpFormatFp82
#endif
  } _FPIEEE_FORMAT;

  typedef enum {
    _FpCodeUnspecified,
    _FpCodeAdd,
    _FpCodeSubtract,
    _FpCodeMultiply,
    _FpCodeDivide,
    _FpCodeSquareRoot,
    _FpCodeRemainder,
    _FpCodeCompare,
    _FpCodeConvert,
    _FpCodeRound,
    _FpCodeTruncate,
    _FpCodeFloor,
    _FpCodeCeil,
    _FpCodeAcos,
    _FpCodeAsin,
    _FpCodeAtan,
    _FpCodeAtan2,
    _FpCodeCabs,
    _FpCodeCos,
    _FpCodeCosh,
    _FpCodeExp,
    _FpCodeFabs,
    _FpCodeFmod,
    _FpCodeFrexp,
    _FpCodeHypot,
    _FpCodeLdexp,
    _FpCodeLog,
    _FpCodeLog10,
    _FpCodeModf,
    _FpCodePow,
    _FpCodeSin,
    _FpCodeSinh,
    _FpCodeTan,
    _FpCodeTanh,
    _FpCodeY0,
    _FpCodeY1,
    _FpCodeYn,
    _FpCodeLogb,
    _FpCodeNextafter,
    _FpCodeNegate,
    _FpCodeFmin,
    _FpCodeFmax,
    _FpCodeConvertTrunc,
    _XMMIAddps,
    _XMMIAddss,
    _XMMISubps,
    _XMMISubss,
    _XMMIMulps,
    _XMMIMulss,
    _XMMIDivps,
    _XMMIDivss,
    _XMMISqrtps,
    _XMMISqrtss,
    _XMMIMaxps,
    _XMMIMaxss,
    _XMMIMinps,
    _XMMIMinss,
    _XMMICmpps,
    _XMMICmpss,
    _XMMIComiss,
    _XMMIUComiss,
    _XMMICvtpi2ps,
    _XMMICvtsi2ss,
    _XMMICvtps2pi,
    _XMMICvtss2si,
    _XMMICvttps2pi,
    _XMMICvttss2si,
    _XMMIAddsubps,
    _XMMIHaddps,
    _XMMIHsubps,
    _XMMI2Addpd,
    _XMMI2Addsd,
    _XMMI2Subpd,
    _XMMI2Subsd,
    _XMMI2Mulpd,
    _XMMI2Mulsd,
    _XMMI2Divpd,
    _XMMI2Divsd,
    _XMMI2Sqrtpd,
    _XMMI2Sqrtsd,
    _XMMI2Maxpd,
    _XMMI2Maxsd,
    _XMMI2Minpd,
    _XMMI2Minsd,
    _XMMI2Cmppd,
    _XMMI2Cmpsd,
    _XMMI2Comisd,
    _XMMI2UComisd,
    _XMMI2Cvtpd2pi,
    _XMMI2Cvtsd2si,
    _XMMI2Cvttpd2pi,
    _XMMI2Cvttsd2si,
    _XMMI2Cvtps2pd,
    _XMMI2Cvtss2sd,
    _XMMI2Cvtpd2ps,
    _XMMI2Cvtsd2ss,
    _XMMI2Cvtdq2ps,
    _XMMI2Cvttps2dq,
    _XMMI2Cvtps2dq,
    _XMMI2Cvttpd2dq,
    _XMMI2Cvtpd2dq,
    _XMMI2Addsubpd,
    _XMMI2Haddpd,
    _XMMI2Hsubpd,
#if defined(__ia64__)
    _FpCodeFma,_FpCodeFmaSingle,_FpCodeFmaDouble,_FpCodeFms,_FpCodeFmsSingle,_FpCodeFmsDouble,_FpCodeFnma,_FpCodeFnmaSingle,_FpCodeFnmaDouble,_FpCodeFamin,_FpCodeFamax
#endif
  } _FP_OPERATION_CODE;

#ifdef _CRTBLD
#define OP_UNSPEC    _FpCodeUnspecified
#define OP_ADD       _FpCodeAdd
#define OP_SUB       _FpCodeSubtract
#define OP_MUL       _FpCodeMultiply
#define OP_DIV       _FpCodeDivide
#define OP_REM       _FpCodeRemainder
#define OP_COMP      _FpCodeCompare
#define OP_CVT       _FpCodeConvert
#define OP_RND       _FpCodeRound
#define OP_TRUNC     _FpCodeTruncate
#define OP_EXP       _FpCodeExp
#define OP_POW       _FpCodePow
#define OP_LOG       _FpCodeLog
#define OP_LOG10     _FpCodeLog10
#define OP_SINH      _FpCodeSinh
#define OP_COSH      _FpCodeCosh
#define OP_TANH      _FpCodeTanh
#define OP_ASIN      _FpCodeAsin
#define OP_ACOS      _FpCodeAcos
#define OP_ATAN      _FpCodeAtan
#define OP_ATAN2     _FpCodeAtan2
#define OP_SQRT      _FpCodeSquareRoot
#define OP_SIN       _FpCodeSin
#define OP_COS       _FpCodeCos
#define OP_TAN       _FpCodeTan
#define OP_CEIL      _FpCodeCeil
#define OP_FLOOR     _FpCodeFloor
#define OP_ABS       _FpCodeFabs
#define OP_MODF      _FpCodeModf
#define OP_LDEXP     _FpCodeLdexp
#define OP_CABS      _FpCodeCabs
#define OP_HYPOT     _FpCodeHypot
#define OP_FMOD      _FpCodeFmod
#define OP_FREXP     _FpCodeFrexp
#define OP_Y0        _FpCodeY0
#define OP_Y1        _FpCodeY1
#define OP_YN        _FpCodeYn
#define OP_LOGB       _FpCodeLogb
#define OP_NEXTAFTER  _FpCodeNextafter
#define OP_ADDPS     _XMMIAddps
#define OP_ADDSS     _XMMIAddss
#define OP_SUBPS     _XMMISubps
#define OP_SUBSS     _XMMISubss
#define OP_MULPS     _XMMIMulps
#define OP_MULSS     _XMMIMulss
#define OP_DIVPS     _XMMIDivps
#define OP_DIVSS     _XMMIDivss
#define OP_SQRTPS    _XMMISqrtps
#define OP_SQRTSS    _XMMISqrtss
#define OP_MAXPS     _XMMIMaxps
#define OP_MAXSS     _XMMIMaxss
#define OP_MINPS     _XMMIMinps
#define OP_MINSS     _XMMIMinss
#define OP_CMPPS     _XMMICmpps
#define OP_CMPSS     _XMMICmpss
#define OP_COMISS    _XMMIComiss
#define OP_UCOMISS   _XMMIUComiss
#define OP_CVTPI2PS  _XMMICvtpi2ps
#define OP_CVTSI2SS  _XMMICvtsi2ss
#define OP_CVTPS2PI  _XMMICvtps2pi
#define OP_CVTSS2SI  _XMMICvtss2si
#define OP_CVTTPS2PI _XMMICvttps2pi
#define OP_CVTTSS2SI _XMMICvttss2si
#define OP_ADDSUBPS  _XMMIAddsubps
#define OP_HADDPS    _XMMIHaddps
#define OP_HSUBPS    _XMMIHsubps
#define OP_ROUNDPS   _XMMIRoundps
#define OP_ROUNDSS   _XMMIRoundss
#define OP_DPPS      _XMMIDpps
#define OP_ADDPD     _XMMI2Addpd
#define OP_ADDSD     _XMMI2Addsd
#define OP_SUBPD     _XMMI2Subpd
#define OP_SUBSD     _XMMI2Subsd
#define OP_MULPD     _XMMI2Mulpd
#define OP_MULSD     _XMMI2Mulsd
#define OP_DIVPD     _XMMI2Divpd
#define OP_DIVSD     _XMMI2Divsd
#define OP_SQRTPD    _XMMI2Sqrtpd
#define OP_SQRTSD    _XMMI2Sqrtsd
#define OP_MAXPD     _XMMI2Maxpd
#define OP_MAXSD     _XMMI2Maxsd
#define OP_MINPD     _XMMI2Minpd
#define OP_MINSD     _XMMI2Minsd
#define OP_CMPPD     _XMMI2Cmppd
#define OP_CMPSD     _XMMI2Cmpsd
#define OP_COMISD    _XMMI2Comisd
#define OP_UCOMISD   _XMMI2UComisd
#define OP_CVTPD2PI  _XMMI2Cvtpd2pi
#define OP_CVTSD2SI  _XMMI2Cvtsd2si
#define OP_CVTTPD2PI _XMMI2Cvttpd2pi
#define OP_CVTTSD2SI _XMMI2Cvttsd2si
#define OP_CVTPS2PD  _XMMI2Cvtps2pd
#define OP_CVTSS2SD  _XMMI2Cvtss2sd
#define OP_CVTPD2PS  _XMMI2Cvtpd2ps
#define OP_CVTSD2SS  _XMMI2Cvtsd2ss
#define OP_CVTDQ2PS  _XMMI2Cvtdq2ps
#define OP_CVTTPS2DQ _XMMI2Cvttps2dq
#define OP_CVTPS2DQ  _XMMI2Cvtps2dq
#define OP_CVTTPD2DQ _XMMI2Cvttpd2dq
#define OP_CVTPD2DQ  _XMMI2Cvtpd2dq
#define OP_ADDSUBPD  _XMMI2Addsubpd
#define OP_HADDPD    _XMMI2Haddpd
#define OP_HSUBPD    _XMMI2Hsubpd
#define OP_ROUNDPD   _XMMI2Roundpd
#define OP_ROUNDSD   _XMMI2Roundsd
#define OP_DPPD      _XMMI2Dppd

#endif /* _CRTBLD */

  typedef enum {
    _FpRoundNearest,_FpRoundMinusInfinity,_FpRoundPlusInfinity,_FpRoundChopped
  } _FPIEEE_ROUNDING_MODE;

  typedef enum {
    _FpPrecisionFull,_FpPrecision53,_FpPrecision24,
#if defined(__ia64__)
    _FpPrecision64,_FpPrecision113
#endif
  } _FPIEEE_PRECISION;

  typedef float _FP32;
  typedef double _FP64;
  typedef short _I16;
  typedef int _I32;
  typedef unsigned short _U16;
  typedef unsigned int _U32;
  __MINGW_EXTENSION typedef __int64 _Q64;

  typedef struct
#if defined(__ia64__)
    _CRT_ALIGN(16)
#endif
  {
    unsigned short W[5];
  } _FP80;

  typedef struct _CRT_ALIGN(16) {
    unsigned long W[4];
  } _FP128;

  typedef struct _CRT_ALIGN(8) {
    unsigned long W[2];
  } _I64;

  typedef struct _CRT_ALIGN(8) {
    unsigned long W[2];
  } _U64;

  typedef struct
#if defined(__ia64__)
    _CRT_ALIGN(16)
#endif
  {
    unsigned short W[5];
  } _BCD80;

  typedef struct _CRT_ALIGN(16) {
    _Q64 W[2];
  } _FPQ64;

  typedef struct {
    union {
      _FP32 Fp32Value;
      _FP64 Fp64Value;
      _FP80 Fp80Value;
      _FP128 Fp128Value;
      _I16 I16Value;
      _I32 I32Value;
      _I64 I64Value;
      _U16 U16Value;
      _U32 U32Value;
      _U64 U64Value;
      _BCD80 Bcd80Value;
      char *StringValue;
      int CompareValue;
      _Q64 Q64Value;
      _FPQ64 Fpq64Value;
    } Value;
    unsigned int OperandValid : 1;
    unsigned int Format : 4;
  } _FPIEEE_VALUE;

  typedef struct {
    unsigned int Inexact : 1;
    unsigned int Underflow : 1;
    unsigned int Overflow : 1;
    unsigned int ZeroDivide : 1;
    unsigned int InvalidOperation : 1;
  } _FPIEEE_EXCEPTION_FLAGS;

  typedef struct {
    unsigned int RoundingMode : 2;
    unsigned int Precision : 3;
    unsigned int Operation :12;
    _FPIEEE_EXCEPTION_FLAGS Cause;
    _FPIEEE_EXCEPTION_FLAGS Enable;
    _FPIEEE_EXCEPTION_FLAGS Status;
    _FPIEEE_VALUE Operand1;
    _FPIEEE_VALUE Operand2;
    _FPIEEE_VALUE Result;
#if defined(__ia64__)
    _FPIEEE_VALUE Operand3;
#endif
  } _FPIEEE_RECORD,*_PFPIEEE_RECORD;

  struct _EXCEPTION_POINTERS;

  _CRTIMP int __cdecl _fpieee_flt(unsigned long _ExceptionCode,struct _EXCEPTION_POINTERS *_PtExceptionPtr,int (__cdecl *_Handler)(_FPIEEE_RECORD *));

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif
