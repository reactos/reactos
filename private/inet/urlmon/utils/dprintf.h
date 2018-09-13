//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       dprintf.h
//
//  Contents:   Debugging output routine function prototypes
//
//  Functions:  w4printf
//              w4vprintf
//              w4dprintf
//              w4vdprintf
//              
//  History:    18-Oct-91   vich        Created
//      
//----------------------------------------------------------------------------

#include <stdarg.h>
#ifdef unix
#include <windows.h>
#endif /* unix */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
int _cdecl w4printf(const char *format, ...);
int _cdecl w4vprintf(const char *format, va_list arglist);
#endif

int _cdecl w4dprintf(const char *format, ...);
int _cdecl w4vdprintf(const char *format, va_list arglist);

#ifdef __cplusplus
}
#endif
 

