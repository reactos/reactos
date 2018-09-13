
#if !defined(_X86_)
#error this is x86 only
#endif

//-----------------------------------------------------------------------------
//  ldouble.c
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//      define stuff needed for long double in win32
//
//  Revision History:
//
//  []      06-Apr-1993 Dans    Created
//
//-----------------------------------------------------------------------------

#include "debexpr.h"
#include "ldouble.h"
//
// imported stuff from the CRT for 10-byte real support
//
#define cbMantissaMax   22
#define cdigitPrecision LDBL_DIG

#if defined(_DLL)
#define CRT_IMPORT  __declspec(dllimport)
#else
#define CRT_IMPORT
#endif

typedef struct FOS {    /* Floating point Output Structure */
    short   exp;
    char    sign;
    char    cbMantissa;
    char    rgchMantissa[ cbMantissaMax ];
    } FOS, *PFOS;

#ifdef __cplusplus
extern "C"
{
#endif

CRT_IMPORT unsigned __cdecl
__STRINGTOLD ( 
    FLOAT10 *, 
    const char **   ppchEnd, 
    const char *    szFloat,
    int             fMultIn12
    );

CRT_IMPORT int __cdecl
$I10_OUTPUT (
    FLOAT10,
    int         cDigits,
    unsigned    outputFlags,
    PFOS        pfos
    );

#ifdef __cplusplus
}
#endif

// SzFromLd
//  return a string rep of the long double passed in
//
char *  SzFromLd ( char * pchBuf, size_t cb, FLOAT10 f10 ) {
    FOS fos;

    if ( $I10_OUTPUT ( f10, cdigitPrecision, 1, &fos ) ) {
        if ( fos.cbMantissa == 1 && fos.rgchMantissa[0] == '0' ) {
            fos.exp++;
            }
        memset ( &fos.rgchMantissa[ fos.cbMantissa ], '0', cbMantissaMax - fos.cbMantissa );

        fos.rgchMantissa[ cdigitPrecision ] = 0;
        if ( fos.sign == 0x20 )
            fos.sign = '+';
        _snprintf (
            pchBuf,
            cb,
            "%c%c.%se%+05hd",
            fos.sign,
            fos.rgchMantissa[0], 
            &fos.rgchMantissa[1],
            fos.exp - 1
            );
        }
    else {
        // signifies that it is a special number
        _snprintf ( pchBuf, cb, fos.rgchMantissa );
        }
    return pchBuf;
    }

// LdFromSz
//  generate a FLOAT10 from a string rep
//
FLOAT10 LdFromSz ( char * szFloat, char ** ppchEnd ) {
    
    FLOAT10 f10ret = {0};

    __STRINGTOLD ( &f10ret, (const char**)ppchEnd, szFloat, TRUE );
    return f10ret;
    }
