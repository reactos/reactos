/* Header file for libgcc2.c.  */
/* Copyright (C) 2000, 2001
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef GCC_LIBGCC2_H
#define GCC_LIBGCC2_H

extern int __gcc_bcmp (const unsigned char *, const unsigned char *, size_t);
extern void __clear_cache (char *, char *);
extern void __eprintf (const char *, const char *, unsigned int, const char *)
  __attribute__ ((__noreturn__));

struct bb;
extern void __bb_exit_func (void);
extern void __bb_init_func (struct bb *);
extern void __bb_fork_func (void);
extern void __bb_trace_func (void);
extern void __bb_trace_ret (void);
extern void __bb_init_trace_func (struct bb *, unsigned long);

struct exception_descriptor;
extern short int __get_eh_table_language (struct exception_descriptor *);
extern short int __get_eh_table_version (struct exception_descriptor *);

/* Permit the tm.h file to select the endianness to use just for this
   file.  This is used when the endianness is determined when the
   compiler is run.  */

#ifndef LIBGCC2_WORDS_BIG_ENDIAN
#define LIBGCC2_WORDS_BIG_ENDIAN WORDS_BIG_ENDIAN
#endif

#ifndef LIBGCC2_LONG_DOUBLE_TYPE_SIZE
#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE LONG_DOUBLE_TYPE_SIZE
#endif

#ifndef MIN_UNITS_PER_WORD
#define MIN_UNITS_PER_WORD UNITS_PER_WORD
#endif

/* In the first part of this file, we are interfacing to calls generated
   by the compiler itself.  These calls pass values into these routines
   which have very specific modes (rather than very specific types), and
   these compiler-generated calls also expect any return values to have
   very specific modes (rather than very specific types).  Thus, we need
   to avoid using regular C language type names in this part of the file
   because the sizes for those types can be configured to be anything.
   Instead we use the following special type names.  */

typedef		 int QItype	__attribute__ ((mode (QI)));
typedef unsigned int UQItype	__attribute__ ((mode (QI)));
typedef		 int HItype	__attribute__ ((mode (HI)));
typedef unsigned int UHItype	__attribute__ ((mode (HI)));
#if MIN_UNITS_PER_WORD > 1
/* These typedefs are usually forbidden on dsp's with UNITS_PER_WORD 1 */
typedef 	 int SItype	__attribute__ ((mode (SI)));
typedef unsigned int USItype	__attribute__ ((mode (SI)));
#if LONG_LONG_TYPE_SIZE > 32
/* These typedefs are usually forbidden on archs with UNITS_PER_WORD 2 */
typedef		 int DItype	__attribute__ ((mode (DI)));
typedef unsigned int UDItype	__attribute__ ((mode (DI)));
#if MIN_UNITS_PER_WORD > 4
/* These typedefs are usually forbidden on archs with UNITS_PER_WORD 4 */
typedef		 int TItype	__attribute__ ((mode (TI)));
typedef unsigned int UTItype	__attribute__ ((mode (TI)));
#endif
#endif
#endif

#if BITS_PER_UNIT == 8

typedef 	float SFtype	__attribute__ ((mode (SF)));
typedef		float DFtype	__attribute__ ((mode (DF)));

#if LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96
typedef		float XFtype	__attribute__ ((mode (XF)));
#endif
#if LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128
typedef		float TFtype	__attribute__ ((mode (TF)));
#endif

#else /* BITS_PER_UNIT != 8 */

/* On dsp's there are usually qf/hf/tqf modes used instead of the above.
   For now we don't support them in libgcc2.c.  */

#undef L_fixdfdi
#undef L_fixsfdi
#undef L_fixtfdi
#undef L_fixunsdfdi
#undef L_fixunsdfsi
#undef L_fixunssfdi
#undef L_fixunssfsi
#undef L_fixunstfdi
#undef L_fixunsxfdi
#undef L_fixunsxfsi
#undef L_fixxfdi
#undef L_floatdidf
#undef L_floatdisf
#undef L_floatditf
#undef L_floatdixf

#endif /* BITS_PER_UNIT != 8 */

typedef int word_type __attribute__ ((mode (__word__)));

/* Make sure that we don't accidentally use any normal C language built-in
   type names in the first part of this file.  Instead we want to use *only*
   the type names defined above.  The following macro definitions insure
   that if we *do* accidentally use some normal C language built-in type name,
   we will get a syntax error.  */

#define char bogus_type
#define short bogus_type
#define int bogus_type
#define long bogus_type
#define unsigned bogus_type
#define float bogus_type
#define double bogus_type

#if MIN_UNITS_PER_WORD > 4
#define W_TYPE_SIZE (8 * BITS_PER_UNIT)
#define Wtype	DItype
#define UWtype	UDItype
#define HWtype	DItype
#define UHWtype	UDItype
#define DWtype	TItype
#define UDWtype	UTItype
#define __NW(a,b)	__ ## a ## di ## b
#define __NDW(a,b)	__ ## a ## ti ## b
#elif MIN_UNITS_PER_WORD > 2 \
      || (MIN_UNITS_PER_WORD > 1 && LONG_LONG_TYPE_SIZE > 32)
#define W_TYPE_SIZE (4 * BITS_PER_UNIT)
#define Wtype	SItype
#define UWtype	USItype
#define HWtype	SItype
#define UHWtype	USItype
#define DWtype	DItype
#define UDWtype	UDItype
#define __NW(a,b)	__ ## a ## si ## b
#define __NDW(a,b)	__ ## a ## di ## b
#elif MIN_UNITS_PER_WORD > 1
#define W_TYPE_SIZE (2 * BITS_PER_UNIT)
#define Wtype	HItype
#define UWtype	UHItype
#define HWtype	HItype
#define UHWtype	UHItype
#define DWtype	SItype
#define UDWtype	USItype
#define __NW(a,b)	__ ## a ## hi ## b
#define __NDW(a,b)	__ ## a ## si ## b
#else
#define W_TYPE_SIZE BITS_PER_UNIT
#define Wtype	QItype
#define UWtype  UQItype
#define HWtype	QItype
#define UHWtype	UQItype
#define DWtype	HItype
#define UDWtype	UHItype
#define __NW(a,b)	__ ## a ## qi ## b
#define __NDW(a,b)	__ ## a ## hi ## b
#endif

#define Wtype_MAX ((Wtype)(((UWtype)1 << (W_TYPE_SIZE - 1)) - 1))
#define Wtype_MIN (- Wtype_MAX - 1)

#define __muldi3	__NDW(mul,3)
#define __divdi3	__NDW(div,3)
#define __udivdi3	__NDW(udiv,3)
#define __moddi3	__NDW(mod,3)
#define __umoddi3	__NDW(umod,3)
#define __negdi2	__NDW(neg,2)
#define __lshrdi3	__NDW(lshr,3)
#define __ashldi3	__NDW(ashl,3)
#define __ashrdi3	__NDW(ashr,3)
#define __ffsdi2	__NDW(ffs,2)
#define __cmpdi2	__NDW(cmp,2)
#define __ucmpdi2	__NDW(ucmp,2)
#define __udivmoddi4	__NDW(udivmod,4)
#define __fixunstfDI	__NDW(fixunstf,)
#define __fixtfdi	__NDW(fixtf,)
#define __fixunsxfDI	__NDW(fixunsxf,)
#define __fixxfdi	__NDW(fixxf,)
#define __fixunsdfDI	__NDW(fixunsdf,)
#define __fixdfdi	__NDW(fixdf,)
#define __fixunssfDI	__NDW(fixunssf,)
#define __fixsfdi	__NDW(fixsf,)
#define __floatdixf	__NDW(float,xf)
#define __floatditf	__NDW(float,tf)
#define __floatdidf	__NDW(float,df)
#define __floatdisf	__NDW(float,sf)
#define __fixunsxfSI	__NW(fixunsxf,)
#define __fixunstfSI	__NW(fixunstf,)
#define __fixunsdfSI	__NW(fixunsdf,)
#define __fixunssfSI	__NW(fixunssf,)

extern DWtype __muldi3 (DWtype, DWtype);
extern DWtype __divdi3 (DWtype, DWtype);
extern UDWtype __udivdi3 (UDWtype, UDWtype);
extern UDWtype __umoddi3 (UDWtype, UDWtype);
extern DWtype __moddi3 (DWtype, DWtype);

/* __udivmoddi4 is static inline when building other libgcc2 portions.  */
#if (!defined (L_udivdi3) && !defined (L_divdi3) && \
     !defined (L_umoddi3) && !defined (L_moddi3))
extern UDWtype __udivmoddi4 (UDWtype, UDWtype, UDWtype *);
#endif

/* __negdi2 is static inline when building other libgcc2 portions.  */
#if !defined(L_divdi3) && !defined(L_moddi3)
extern DWtype __negdi2 (DWtype);
#endif

extern DWtype __lshrdi3 (DWtype, word_type);
extern DWtype __ashldi3 (DWtype, word_type);
extern DWtype __ashrdi3 (DWtype, word_type);
extern DWtype __ffsdi2 (DWtype);

/* __udiv_w_sdiv is static inline when building other libgcc2 portions.  */
#if (!defined(L_udivdi3) && !defined(L_divdi3) && \
     !defined(L_umoddi3) && !defined(L_moddi3))
extern UWtype __udiv_w_sdiv (UWtype *, UWtype, UWtype, UWtype);
#endif

extern word_type __cmpdi2 (DWtype, DWtype);
extern word_type __ucmpdi2 (DWtype, DWtype);

extern Wtype __absvsi2 (Wtype);
extern DWtype __absvdi2 (DWtype);
extern Wtype __addvsi3 (Wtype, Wtype);
extern DWtype __addvdi3 (DWtype, DWtype);
extern Wtype __subvsi3 (Wtype, Wtype);
extern DWtype __subvdi3 (DWtype, DWtype);
extern Wtype __mulvsi3 (Wtype, Wtype);
extern DWtype __mulvdi3 (DWtype, DWtype);
extern Wtype __negvsi2 (Wtype);
extern DWtype __negvdi2 (DWtype);

#if BITS_PER_UNIT == 8
extern DWtype __fixdfdi (DFtype);
extern DWtype __fixsfdi (SFtype);
extern DFtype __floatdidf (DWtype);
extern SFtype __floatdisf (DWtype);
extern UWtype __fixunsdfSI (DFtype);
extern UWtype __fixunssfSI (SFtype);
extern DWtype __fixunsdfDI (DFtype);
extern DWtype __fixunssfDI (SFtype);

#if LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96
extern DWtype __fixxfdi (XFtype);
extern DWtype __fixunsxfDI (XFtype);
extern XFtype __floatdixf (DWtype);
extern UWtype __fixunsxfSI (XFtype);
#endif

#if LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128
extern DWtype __fixunstfDI (TFtype);
extern DWtype __fixtfdi (TFtype);
extern TFtype __floatditf (DWtype);
#endif
#endif /* BITS_PER_UNIT == 8 */

/* DWstructs are pairs of Wtype values in the order determined by
   LIBGCC2_WORDS_BIG_ENDIAN.  */

#if LIBGCC2_WORDS_BIG_ENDIAN
  struct DWstruct {Wtype high, low;};
#else
  struct DWstruct {Wtype low, high;};
#endif

/* We need this union to unpack/pack DImode values, since we don't have
   any arithmetic yet.  Incoming DImode parameters are stored into the
   `ll' field, and the unpacked result is read from the struct `s'.  */

typedef union
{
  struct DWstruct s;
  DWtype ll;
} DWunion;

#include "longlong.h"

#endif /* ! GCC_LIBGCC2_H */
