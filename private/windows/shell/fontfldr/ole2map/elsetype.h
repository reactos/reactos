/***************************************************************************
 * ELSETYPE.H - Public ElseWare include file.
 *
 * $keywords: elsetype.h 1.13  5-Oct-93 5:46:20 PM$
 *
 * Used to design public interfaces to mapper, mai, TMR, 318 etc.)
 *
 * Most ElseWare types contain an "EW_" to avoid confusion with other host
 * defined types.  All source files must include this file.  This file
 * should be included **AFTER** other standard environment headers such
 * as windows.h, c include files.
 *
 * Other include files:
 *
 *    ELSEPRIV.H     Used to contain other useful info that need not be
 *                   in public interfaces.
 *
 *    <proj>.h       Public interface to a project, e.g. TMR.H
 *
 *    <proj>PRIV.H   Private interfaces for a project, e.g. TMRPRIV.H
 *
 *    <comp>.H       Component interfaces within a project, e.g. DM.H
 *
 *    <comp>PRIV.H   Component private interfaces.
 *
 * Copyright (C) 1992-93 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/

#ifndef __ELSETYPE_H__
#define __ELSETYPE_H__

/***************************************************************************
 * Keyword definitions.
 ***************************************************************************/


#if defined(__STDC__) || defined(WIN32)

/* Standard C, i.e. ANSI C
 */

#ifdef EW_FAR
#undef EW_FAR
#endif
#define EW_FAR

#ifdef EW_NEAR
#undef EW_NEAR
#endif
#define EW_NEAR

#ifdef EW_HUGE
#undef EW_HUGE
#endif
#define EW_HUGE

#ifdef EW_PASCAL
#undef EW_PASCAL
#endif
#ifdef WIN32
#define EW_PASCAL __stdcall
#else
#define EW_PASCAL
#endif /* WIN32 */

#ifdef EW_CDECL
#undef EW_CDECL
#endif
#define EW_CDECL

#else /* __STDC__ */

#ifndef EW_FAR
#define EW_FAR       __far
#endif

#ifndef EW_NEAR
#define EW_NEAR      __near
#endif

#ifndef EW_HUGE
#define EW_HUGE      __huge
#endif

#ifndef EW_PASCAL
#define EW_PASCAL    __pascal
#endif

#ifndef EW_CDECL
#define EW_CDECL     __cdecl
#endif

#endif /* __STDC__ */

#ifndef EXPORT
#define EXPORT       __export
#endif

#ifndef EW_VOID
#define EW_VOID      void
#endif

#ifndef GLOBAL
#define GLOBAL       extern
#endif

#ifndef LOCAL
#define LOCAL        static
#endif

#ifndef NULL
#define NULL         0L
#endif

#ifndef EW_NULL
#define EW_NULL      0L
#endif

#ifndef MAXLONG
#define MAXLONG      (0x7FFFFFFF)
#endif

#ifndef TRUE
#define TRUE         1
#endif

#ifndef FALSE
#define FALSE        0
#endif

#ifndef MAX
#define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifdef _NATIVE_IS_MOTOROLA /* native byte order matches Motorola 68000 */
   #ifndef SWAPL
      #define SWAPL(a)        (a)
   #endif
   #ifndef SWAPW
      #define SWAPW(a)        (a)
   #endif
   #ifndef SWAPWINC
      #define SWAPWINC(a)     (*(a)++)
   #endif
#else
 /* Portable code to extract a short or a long from a 2- or 4-byte buffer */
 /* which was encoded using Motorola 68000 (TrueType "native") byte order. */
   #define FS_2BYTE(p)  ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
   #define FS_4BYTE(p)  ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )

   #ifndef SWAPW
      #define SWAPW(a)        ((short) FS_2BYTE( (unsigned char EW_FAR*)(&(a)) ))
   #endif

   #ifndef SWAPL
      #define SWAPL(a)        ((long) FS_4BYTE( (unsigned char EW_FAR*)(&(a)) ))
   #endif

   #ifndef SWAPWINC
      #define SWAPWINC(a)     SWAPW(*(a)); a++  /* Do NOT parenthesize! */
   #endif


#endif

#ifndef FLIPW
   #define FLIPW(a)           ((short) FS_2BYTE( (unsigned char EW_FAR*)(&(a)) ))
#endif

#ifndef FLIPL
   #define FLIPL(a)           ((long) FS_4BYTE( (unsigned char EW_FAR*)(&(a)) ))
#endif

#ifndef ASW
   #define ASW(a,b)        (a = SWAPW((b)))
#endif

#ifndef ASL
   #define ASL(a,b)        (a = SWAPL((b)))
#endif

#ifndef ASWT
   extern short __nASWT;
   #define ASWT(a,b)       {__nASWT = (b); a = SWAPW(__nASWT);}
#endif

#ifndef ASLT
   extern long __lASLT;
   #define ASLT(a,b)       {__lASLT = (b); a = SWAPL(__lASLT);}
#endif


/***************************************************************************
 * ElseWare specific types.  These are unlikely to cause conflict.
 ***************************************************************************/

typedef int             EW_RC, EW_FAR* EW_LPRC;

/***************************************************************************
 * Common types.  Prepend with an "EW_" to avoid conflict.
 *
 * m means MOTOROLA order for multiple byte types
 *
 * Note that the types in, WORD, DWORD have been avoided.
 ***************************************************************************/

typedef EW_VOID                        EW_FAR* EW_LPVOID;    /*  v, lpv    */

typedef signed short     EW_SHORT,     EW_FAR* EW_LPSHORT;   /*  n,  lpn   */
typedef signed short     EW_mSHORT,    EW_FAR* EW_LPmSHORT;  /* mn,  lpmn  */

typedef unsigned short   EW_USHORT,    EW_FAR* EW_LPUSHORT;  /*  un, lpun  */
typedef unsigned short   EW_mUSHORT,   EW_FAR* EW_LPmUSHORT; /* mun, lpmun */

typedef signed long      EW_LONG,      EW_FAR* EW_LPLONG;    /*  l,  lpl   */
typedef signed long      EW_mLONG,     EW_FAR* EW_LPmLONG;   /* ul,  lpul  */

typedef unsigned long    EW_ULONG,     EW_FAR* EW_LPULONG;   /*  ul, lpul  */
typedef unsigned long    EW_mULONG,    EW_FAR* EW_LPmULONG;  /* mul, lpmul */

typedef signed char      EW_CHAR,      EW_FAR* EW_LPCHAR;    /* c,   lpc   */
typedef signed char                    EW_FAR* EW_LPSTR;     /*      lpsz  */
typedef unsigned char    EW_BYTE,      EW_FAR* EW_LPBYTE;    /* j,   lpj   */
typedef signed short     EW_BOOL,      EW_FAR* EW_LPBOOL;    /* b,   lpb   */

typedef signed short                  EW_HUGE* EW_HPSHORT;   /*      hpn   */
typedef signed short                  EW_HUGE* EW_HPmSHORT;  /*      hpmn  */
typedef unsigned short                EW_HUGE* EW_HPUSHORT;  /*      hpun  */
typedef unsigned short                EW_HUGE* EW_HPmUSHORT; /*      hpmun */
typedef signed long                   EW_HUGE* EW_HPLONG;    /*      hpl   */
typedef signed long                   EW_HUGE* EW_HPmLONG;   /*      hpml  */
typedef unsigned long                 EW_HUGE* EW_HPULONG;   /*      hpul  */
typedef unsigned long                 EW_HUGE* EW_HPmULONG;  /*      hpmul */
typedef signed char                   EW_HUGE* EW_HPCHAR;    /*      hpc   */
typedef signed char                   EW_HUGE* EW_HPSTR;     /*      hpsz  */
typedef unsigned char                 EW_HUGE* EW_HPBYTE;    /*      hpj   */

/*
 * other hungarian:
 *    sz    for zero terminated strings
 *    sp    for pascal strings
 *    a...  for arrays, except for arrays of char which are strings
 */

#ifdef __EW_TT_TYPES__
/*
 * Note: these typedefs do not follow the convention of all CAPS
 *       so that they more closely match the format used in the
 *       TrueType Spec 1.0.
 */
typedef unsigned long    EW_Fixed,     EW_FAR* EW_LPFixed;    /*  fi,  lpnfi  */
typedef unsigned long    EW_mFixed,    EW_FAR* EW_LPmFixed;   /* mfi,  lpmfi  */
typedef signed short     EW_FWord,     EW_FAR* EW_LPFWord;    /*  fw,  lpfw   */
typedef signed short     EW_mFWord,    EW_FAR* EW_LPmFWord;   /* mfw,  lpmfw  */
typedef unsigned short   EW_UFWord,    EW_FAR* EW_LPUFWord;   /* ufw,  lpufw  */
typedef unsigned short   EW_mUFWord,   EW_FAR* EW_LPmUFword;  /* mufw, lpmufw */

typedef signed short     EW_F2DOT14,   EW_FAR* EW_LPF2DOT14;  /* f2,   lpf2   */
typedef signed short     EW_mF2DOT14,  EW_FAR* EW_LPmF2DOT14; /* mf2,  lpmf2  */
typedef signed long      EW_F26DOT6,   EW_FAR* EW_LPF26DOT6;  /* f26,  lpf26  */
typedef signed long      EW_mF26DOT6,  EW_FAR* EW_LPmF26DOT6; /* mf26, lpmf26 */
typedef signed short     EW_F10DOT6,   EW_FAR* EW_LPF10DOT6;  /* f10,  lpf10  */
typedef signed short     EW_mF10DOT6,  EW_FAR* EW_LPmF10DOT6; /* mf10, lpmf10 */

typedef                  EW_Fixed      EW_HUGE* EW_HPFixed;   /*  fi,  lpnfi  */
typedef                  EW_mFixed     EW_HUGE* EW_HPmFixed;  /* mfi,  lpmfi  */
typedef                  EW_FWord      EW_HUGE* EW_HPFWord;   /*  fw,  lpfw   */
typedef                  EW_mFWord     EW_HUGE* EW_HPmFWord;  /* mfw,  lpmfw  */
typedef                  EW_UFWord     EW_HUGE* EW_HPUFWord;  /* ufw,  lpufw  */
typedef                  EW_mUFWord    EW_HUGE* EW_HPmUFword; /* mufw, lpmufw */

typedef                  EW_F2DOT14    EW_HUGE* EW_HPF2DOT14;  /* f2,   lpf2   */
typedef                  EW_mF2DOT14   EW_HUGE* EW_HPmF2DOT14; /* mf2,  lpmf2  */
typedef                  EW_F26DOT6    EW_HUGE* EW_HPF26DOT6;  /* f26,  lpf26  */
typedef                  EW_mF26DOT6   EW_HUGE* EW_HPmF26DOT6; /* mf26, lpmf26 */
typedef                  EW_F10DOT6    EW_HUGE* EW_HPF10DOT6;  /* f10,  lpf10  */
typedef                  EW_mF10DOT6   EW_HUGE* EW_HPmF10DOT6; /* mf10, lpmf10 */
#endif /* __EW_TT_TYPES__ */



typedef EW_LPBYTE (EW_NEAR EW_PASCAL *EW_NEAROPPTR)();
typedef EW_LPBYTE (EW_FAR  EW_PASCAL *EW_FAROPPTR )();
typedef EW_RC     (EW_FAR            *EW_FARFUNC)();
typedef EW_LPBYTE (EW_FAR            *EW_LPPTR);

/***************************************************************************
 * EW_PANOSE
 ***************************************************************************/

typedef struct tagEW_PANOSE
{
    EW_BYTE    jFamilyType;
    EW_BYTE    jSerifStyle;
    EW_BYTE    jWeight;
    EW_BYTE    jProportion;
    EW_BYTE    jContrast;
    EW_BYTE    jStrokeVariation;
    EW_BYTE    jArmStyle;
    EW_BYTE    jLetterform;
    EW_BYTE    jMidline;
    EW_BYTE    jXHeight;
} EW_PANOSE, EW_FAR* EW_LPPANOSE;


/***************************************************************************
 * EW_lseWare specific macros and constants.
 ***************************************************************************/


#endif /* __ELSETYPE_H__ */

/***************************************************************************
 * Revision log:
 ***************************************************************************/
/*
 * $lgb$
 * 1.0    22-Dec-92    cdm This is the official version.
 * 1.1    22-Dec-92    cdm Added LPBOOL.
 * 1.2    24-Dec-92    cdm Draft 2.
 * 1.3     6-Jan-93    cdm Fixed typo in EW_BOOL decl.
 * 1.4    27-Jan-93    emr Added EW_LPVOID
 * 1.5    30-Jan-93    msd Added EW_LPSTR.
 * 1.6    10-Feb-93    msd Added EW_ prefix to FAR, NEAR, HUGE, PASCAL, and CDECL.
 * 1.7    18-Feb-93    msd Bumped copyright notice.
 * 1.8    26-Feb-93    msd Added EW_PANOSE struct.
 * 1.9    21-Apr-93    emr Added EW_LPPTR. It's used by 318 and Pecos.
 * 1.10   21-Apr-93    emr Fixed MIN() macro.
 * 1.11   30-Apr-93    emr Some of the types in the TrueType section where being multiply defined. This was an error  on the Think C Macintosh compiler.
 * 1.12   19-Jul-93    pmh Latest engine5 elsetype.h.
 * 1.13    5-Oct-93   paul signed vars; FLIP macros
 * $lge$
 */
