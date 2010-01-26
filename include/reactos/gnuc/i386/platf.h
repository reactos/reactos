#pragma once

/* platf.h gnuc i32
platform dependent basic types and macros
(c) Jose M. Catena Gomez, diwaves.com
*/

/* this must produce a compiler error */
#define _ERR_UNSUPPORTED #error

/* attribues */
#define __declspec(x) __attribute__((x))
#define _CDECL __declspec(cdecl)
#define _THISCALL
#define _STDCALL __declspec(stdcall)
#define _FASTCALL __declspec(regparm(3))
#define _CLRCALL
#define _INLINE __inline__
#define _INLINEF __inline__ __declspec(always_inline)
#define _NAKED
#define _INTERRUPT _ERR_UNSUPPORTED
#define _SAVEREGS _ERR_UNSUPPORTED
#define _LOADDS _ERR_UNSUPPORTED
#define _DLLEXPORT __declspec(dllexport)
#define _DLLIMPORT __declspec(dllimport)
#define _ALIGN(x) __declspec(aligned(x))

#define _PACK(x) __pragma(pack(push, x))
#define _PACK_POP __pragma(pack(pop))
#define _RESTRICT __restrict__
#define _ONCE
#define _OPTIMIZE_DFT
#define _OPTIMIZE_OFF_ALL
#define _OPTIMIZE_OFF_GLOBAL
#define _OPTIMIZE_OFF_STKF
#define _INTRINSIC(x, subst) \
#define x subst
#define _NOINTRINSIC(x) \
#undef x
#define _NOWARN_PUSH
#define _NOWARN_POP
#define _NOWARN_MSC(x)
#define _NOWARN_GNUC(x)

#define _SECTION(x) __declspec((section(sectn)))
#define _SECTION_FN(sectn, fn) __declspec((section(sectn)))
#define _SECTION_FND(sectn) __declspec((section(sectn)))
#define _SECTION_CODE(x)
#define _SECTION_CODE_END
#define _SECTION_DATA(x)
#define _SECTION_DATA_END
#define _SECTION_BSS(x)
#define _SECTION_BSS_END
#define _SECTION_CONST(x)
#define _SECTION_CONST_END

#define _ASM_BEGIN __asm__ {
#define _ASM_END }
#define _ASM __asm__

#define IN
#define OUT

/* ptr size */
#define PTRSIZ 4
#define PTRSIZMIN 4
#define PTRSIZMAX 4
#define _NEAR
#define _FAR
#define _FARH
#define _BASED
#define _BASED1 __based
#define _SEGMENT _UNSUPPORTED
#define _SEGNAME _UNSUPPORTED

/*************************************************************************
basic types
*************************************************************************/
typedef void *pvoid, _NEAR *npvoid, _FAR *lpvoid, _FARH *hpvoid;

/* chars */
#define CHRSIZMIN	1
#define CHRSIZMAX	2
#define CHRSIZ		1
#define CHRWSIZ		2
#define CHRXSIZ		2
#define CHRSIGN		1
#define CHRWSIGN	0

typedef char chr8, *pchr8p, _NEAR *npchr8, _FAR *lpchr8, _FARH *hpchr8;
typedef signed char chr8s, *pchr8s, _NEAR *npchr8s, _FAR *lpchr8s, _FARH *hpchr8s;
typedef unsigned char chr8u, *pchr8u, _NEAR *npchr8u, _FAR *lpchr8u, _FARH *hpchr8u;

/* chars unicode 16 bits */
#ifdef _NATIVE_WCHAR_T_DEFINED
typedef wchar_t chr16;
typedef signed short chr16s;
typedef unsigned short chr16u;
#else
typedef unsigned short chr16;
typedef signed short chr16s;
typedef unsigned short chr16u;
#endif

typedef chr16 *pchr16, _NEAR *npchr16, _FAR *lpchr16, _FARH *hpchr16;
typedef chr16s *pchr16s, _NEAR *npchr16s, _FAR *lpchr16s, _FARH *hpchr16s;
typedef chr16u *pchr16u, _NEAR *npchr16u, _FAR *lpchr16u, _FARH *hpchr16u;

#ifdef _UNICODE
typedef chr16 chr;
typedef chr16s chrs;
typedef chr16u chru;
#else
typedef chr8 chr;
typedef chr8s chrs;
typedef chr8u chru;
#endif

typedef chr *pchr, _NEAR *npchr, _FAR *lpchr, _FARH hpchr;
typedef chrs *pchrs, _NEAR *npchrs, _FAR *lpchrs, _FARH hpchrs;
typedef chru *pchru, _NEAR *npchru, _FAR *lpchru, _FARH hpchru;

// string literal macros 
// double definitions here are necessary because preprocessor behavior
#define _T16(x) L ## x
#define T16(x) _T16(x)
#define _T8(x) x
#define T8(x) _T8(x)
#ifdef _UNICODE
#define TX(x) _T16(x)
#else
#define TX(x) _T8(x)
#endif
// stringfy
#define _sfy(x) #x
#define sfy(x) _sfy(x)

/* integers */
#define INTSIZMIN   1
#define INTSIZMAX   8
#define INTSIZ	    4
#define INTSMIN	    -2147483648
#define INTSMAX	    0x7FFFFFFF

typedef int *pint, _NEAR *npint, _FAR *lpint, _FARH *hpint;
typedef signed int ints, *pints, _NEAR *npints, _FAR *lpints, _FARH *hpints;
typedef unsigned int intu, *pintu, _NEAR *npintu, _FAR *lpintu, _FARH *hpintu;

typedef __int8 i8, *pi8, _NEAR *npi8, _FAR *lpi8, _FARH *hpi8;
typedef signed __int8 i8s, *pi8s, _NEAR *npi8s, _FAR *lpi8s, _FARH *hpi8s;
typedef unsigned __int8 i8u, *pi8u, _NEAR *npi8u, _FAR *lpi8u, _FARH *hpi8u;

typedef short i16, *pi16, _NEAR *npi16, _FAR *lpi16, _FARH *hpi16;
typedef signed short i16s, *pi16s, _NEAR *npi16s, _FAR *lpi16s, _FARH *hpi16s;
typedef unsigned short i16u, *pi16u, _NEAR *npi16u, _FAR *lpi16u, _FARH *hpi16u;

typedef long i32, *pi32, _NEAR *npi32, _FAR *lpi32, _FARH *hpi32;
typedef signed long i32s, *pi32s, _NEAR *npi32s, _FAR *lpi32s, _FARH *hpi32s;
typedef unsigned long i32u, *pi32u, _NEAR *npi32u, _FAR *lpi32u, _FARH *hpi32u;		// = unsigned long DWORD

typedef __int64 i64, *i64p, _NEAR *npi64, _FAR *lpi64, _FARH *hpi64;
typedef signed __int64 i64s, *pi64s, _NEAR *npi64s, _FAR *lpi64s, _FARH *hpi64s;
typedef unsigned __int64 i64u, *pi64u, _NEAR *npi64u, _FAR *lpi64u, _FARH *hpi64u;

typedef union {i64 x; struct {i32u l; i32 h;}} i64x, *pi64x, _NEAR *npi64x, _FAR *lpi64x, _FARH *hpi64x;
typedef union {i64s x; struct {i32u l; i32s h;}} i64xs, *pi64xs, _NEAR *npi64xs, _FAR *lpi64xs, _FARH *hpi64xs;
typedef union {i64u x; struct {i32u l; i32u h;}} i64xu, *pi64xu, _NEAR *npi64xu, _FAR *lpi64xu, _FARH *hpi64xu;

typedef struct {i64u l; i64 h;} i128, *pi128, _NEAR *npi128, _FAR *lpi128, _FARH *hpi128;
typedef struct {i64u l; i64s h;} i128s, *pi128s, _NEAR *npi128s, _FAR *lpi128s, _FARH *hpi128s;
typedef struct {i64u l; i64u h;} i128u, *pi128u, _NEAR *npi128u, _FAR *lpi128u, _FARH *hpi128u;

// this int must be the same size of a pointer
typedef long iptr, *piptr, _NEAR *npiptr, _FAR *lpiptr, _FARH *hpiptr;
typedef signed long iptrs, *piptrs, _NEAR *npiptrs, _FAR *lpiptrs, _FARH *hpiptrs;
typedef unsigned long iptru, *piptru, _NEAR *npiptru, _FAR *lpiptru, _FARH *hpiptru;

/* floats */
typedef float f32, *pf32, _NEAR *npf32, _FAR *lpf32, _FARH *hpf32;
typedef double f64, *pf64p, _NEAR *npf64, _FAR *lpf64, _FARH *hpf64;
typedef struct {i64u l; i64u h;} f128, *pf128, _NEAR *npf128, _FAR *lpf128, _FARH *hpf128;

/* size */
#define SIZTSIZ 4
#define SIZTMAX 0xFFFFFFFF
typedef i32u sizt, *psizt, _NEAR *npsizt, _FAR *npsizt, _FARH *hpsizt;

/*************************************************************************

*************************************************************************/
// debug 
#if _DEBUG
#define dbgref TX("(") TX(__FILE__) TX(":") TX(sfy(__LINE__)) TX(":") TX(__FUNCTION__) TX(")")
#else
#define dbgref 0
#endif

#define _ReturnAddress() (__builtin_return_address(0))
#define ReturnAddress _ReturnAddress
#define _ReturnAddressn(x) (__builtin_return_address(x))
#define ReturnAddressn(x) _ReturnAddressn(x)
