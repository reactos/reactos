/***
*new.h - declarations and definitions for C++ memory allocation functions
*
*   Copyright (c) 1990-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   Contains the function declarations for C++ memory allocation functions.
*
****/

#ifndef _INC_NEW

#ifdef __cplusplus


/* constants for based heap routines */

#define _NULLSEG    ((__segment)0)
#define _NULLOFF    ((void __based(void) *)0xffff)

/* types and structures */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

typedef int (__cdecl * _PNH)( size_t );
typedef int (__cdecl * _PNHH)( unsigned long, size_t );
typedef int (__cdecl * _PNHB)( __segment, size_t );

/* function prototypes */

_PNH __cdecl _set_new_handler( _PNH );
_PNH __cdecl _set_nnew_handler( _PNH );
_PNH __cdecl _set_fnew_handler( _PNH );
_PNHH __cdecl _set_hnew_handler( _PNHH );
_PNHB __cdecl _set_bnew_handler( _PNHB );

#else 

/* handler functions only supported in C++, emit appropriate error */
#error Functions declared in new.h can only be used in C++ source

#endif 

#define _INC_NEW
#endif 
