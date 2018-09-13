/***
*setjmp.h - definitions/declarations for setjmp/longjmp routines
*
*       Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the machine-dependent buffer used by
*       setjmp/longjmp to save and restore the program state, and
*       declarations for those routines.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_SETJMP
#define _INC_SETJMP

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/*
 * Definitions specific to particular setjmp implementations.
 */

#if     defined(_M_IX86)

/*
 * MS compiler for x86
 */

#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif

#define _JBLEN  16
#define _JBTYPE int

/*
 * Define jump buffer layout for x86 setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;



#elif   defined(_M_MRX000)

#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif

/*
 * All MIPS implementations need _JBLEN of 16
 */
#define _JBLEN  16
#define _JBTYPE double

/*
 * Define jump buffer layout for MIPS setjmp/longjmp.
 */
typedef struct __JUMP_BUFFER {
    unsigned long FltF20;
    unsigned long FltF21;
    unsigned long FltF22;
    unsigned long FltF23;
    unsigned long FltF24;
    unsigned long FltF25;
    unsigned long FltF26;
    unsigned long FltF27;
    unsigned long FltF28;
    unsigned long FltF29;
    unsigned long FltF30;
    unsigned long FltF31;
    unsigned long IntS0;
    unsigned long IntS1;
    unsigned long IntS2;
    unsigned long IntS3;
    unsigned long IntS4;
    unsigned long IntS5;
    unsigned long IntS6;
    unsigned long IntS7;
    unsigned long IntS8;
    unsigned long IntSp;
    unsigned long Type;
    unsigned long Fir;
} _JUMP_BUFFER;


#elif   defined(_M_ALPHA)

/*
 * The Alpha C8/GEM C compiler uses an intrinsic _setjmp.
 * The Alpha acc compiler implements setjmp as a function.
 */
#ifdef  _MSC_VER
#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif
#endif

/*
 * Alpha implementations use a _JBLEN of 24 quadwords.
 * A double is used only to obtain quadword size and alignment.
 */
#define _JBLEN  24
#define _JBTYPE double

/*
 * Define jump buffer layout for Alpha setjmp/longjmp.
 * A double is used only to obtain quadword size and alignment.
 */
typedef struct __JUMP_BUFFER {
    unsigned long Fp;
    unsigned long Pc;
    unsigned long Seb;
    unsigned long Type;
    double FltF2;
    double FltF3;
    double FltF4;
    double FltF5;
    double FltF6;
    double FltF7;
    double FltF8;
    double FltF9;
    double IntS0;
    double IntS1;
    double IntS2;
    double IntS3;
    double IntS4;
    double IntS5;
    double IntS6;
    double IntSp;
    double Fir;
    double Fill[5];
} _JUMP_BUFFER;

#elif defined(_M_PPC)
/*
 * The Microsoft VC++ V4.0 compiler uses an intrinsic _setjmp.
 * The Motorola C8.5 compiler implements setjmp as a function.
 */

#if _MSC_VER > 850
#ifndef _INC_SETJMPEX
#undef _setjmp
#define setjmp  _setjmp
#endif
#endif

/*
 * Min length is 240 bytes; round to 256 bytes.
 * Since this is allocated as an array of "double", the
 * number of entries required is 32.
 *
 * All PPC implementations need _JBLEN of 32
 */

#define _JBLEN  32
#define _JBTYPE double

/*
 * Define jump buffer layout for PowerPC setjmp/longjmp.
 */

typedef struct __JUMP_BUFFER {
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    unsigned long Gpr1;
    unsigned long Gpr2;
    unsigned long Gpr13;
    unsigned long Gpr14;
    unsigned long Gpr15;
    unsigned long Gpr16;
    unsigned long Gpr17;
    unsigned long Gpr18;
    unsigned long Gpr19;
    unsigned long Gpr20;
    unsigned long Gpr21;
    unsigned long Gpr22;
    unsigned long Gpr23;
    unsigned long Gpr24;
    unsigned long Gpr25;
    unsigned long Gpr26;
    unsigned long Gpr27;
    unsigned long Gpr28;
    unsigned long Gpr29;
    unsigned long Gpr30;
    unsigned long Gpr31;
    unsigned long Cr;
    unsigned long Iar;
    unsigned long Type;
} _JUMP_BUFFER;

#elif   defined(_M_M68K)

#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif

/* internal only for version checking for compatibility */
#define _SETJMP_VER 1.0

/* internal only, jump buffer layout definition */
typedef struct __JUMP_BUFFER {   
        unsigned long d1;       /* 0x00, saved */
        unsigned long d2;       /* 0x04, saved */
        unsigned long d3;       /* 0x08, saved */
        unsigned long d4;       /* 0x0c, saved */
        unsigned long d5;       /* 0x10, saved */
        unsigned long d6;       /* 0x14, saved */
        unsigned long d7;       /* 0x18, saved */
        unsigned long a1;       /* 0x1c, non-swappable version: return address, 
                                         swappable version: swapper lib routine pointer for building return address */
        unsigned long a2;       /* 0x20, saved */
        unsigned long a3;       /* 0x24, saved */
        unsigned long a4;       /* 0x28, saved */
        unsigned long a6;       /* 0x2c, saved */
        unsigned long a7;       /* 0x30, saved */
        unsigned long SwapChain; /* 0x34, without swapper: unused. with swapper: link swappable frames */
        unsigned short reserved; /* 0x38, unused */
        unsigned short SN;      /* 0x3a, swappable version: segment number */
        unsigned long BPC;      /* 0x3c, swappable version: offset within segment */
} _JUMP_BUFFER; 

#define _JBTYPE int
#define _JBLEN  (sizeof(_JUMP_BUFFER)/sizeof(_JBTYPE))

#elif   defined(_M_MPPC)

#ifndef _INC_SETJMPEX
#define setjmp  _setjmp
#endif

#define _JBTYPE int

/* define the buffer type for building the state information */

#define _JBLEN 64   /* return address, R1, R2, R13-R31, FPR14-FPR31, CR 2-5 */
#endif


/* Define the buffer type for holding the state information */

#ifndef _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif


/* Function prototypes */

int __cdecl setjmp(jmp_buf);
_CRTIMP void __cdecl longjmp(jmp_buf, int);

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_SETJMP */
