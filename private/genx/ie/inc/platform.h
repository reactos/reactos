//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       platform.h
//
//  Contents:   Macros for portable access to platform dependent values.
//
//
// This file contains macros for easy cross platform developing.
// There are macros for compiler differences and platform/layer differences.
//
//----------------------------------------------------------------------------


#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#ifdef __cplusplus
   // +++ Unaligned memory access templates/macros
#  include <unaligned.hpp>
#endif

////////////////////////////////////////////////////////////////////
//
// Global defines (should be moved to NT public headers)
//

#define VER_PLATFORM_WIN32_UNIX 9

////////////////////////////////////////////////////////////////////
//
// Compiler differences
//

#if !defined(_MSC_VER) && !defined(__APOGEE__)
    // For compilers lacking VC++ extensions

#   define __cdecl
#   define INLINEOP      /* Inline operators not available IEUNIX */

#   ifdef UNIX
        // Unix specific compiler problems
#       define UNSIZED_ARRAY 1
#   else
#       define UNSIZED_ARRAY
#   endif // UNIX

#else // !_MSC_VER

    // VC++ compilers

#   define INLINEOP inline
#   define UNSIZED_ARRAY

#endif

#define EMPTY_SIZE UNSIZED_ARRAY

////////////////////////////////////////////////////////////////////
//
// Platform / Layer dependent sections.
//
// NOTE! #ifdefing based on WIN32 is invalid as some layers define
//       WIN32 for compatability.
//

#define ENDEXCEPT  __endexcept
#define ENDFINALLY __endfinally

#if !defined( UNIX )
#define __endexcept
#define __endfinally
#endif // UNIX

// +++ File Separators

#if defined( UNIX )
// UNIX

#  ifndef FILENAME_SEPARATOR
#  define FILENAME_SEPARATOR       '/'
#  endif

#  ifndef FILENAME_SEPARATOR_W
#  define FILENAME_SEPARATOR_W     L'/'
#  endif

#  ifndef FILENAME_SEPARATOR_STR
#  define FILENAME_SEPARATOR_STR   "/"
#  endif

#  ifndef FILENAME_SEPARATOR_STR_W
#  define FILENAME_SEPARATOR_STR_W L"/"
#  endif

#  ifndef PATH_SEPARATOR
#  define PATH_SEPARATOR           ':'
#  endif

#  ifndef PATH_SEPARATOR_W
#  define PATH_SEPARATOR_W         L':'
#  endif

#  ifndef PATH_SEPARATOR_STR
#  define PATH_SEPARATOR_STR       ":"
#  endif

#  ifndef PATH_SEPARATOR_STR_W
#  define PATH_SEPARATOR_STR_W     L":"
#  endif

#  ifndef LINE_SEPARATOR_STR
#  define LINE_SEPARATOR_STR       "\n"
#  endif

#  ifndef LINE_SEPARATOR_STR_W
#  define LINE_SEPARATOR_STR_W     L"\n"
#  endif

#else // UNIX

// Windows / MAC

#  ifndef FILENAME_SEPARATOR
#  define FILENAME_SEPARATOR       '\\'
#  endif

#  ifndef FILENAME_SEPARATOR_W
#  define FILENAME_SEPARATOR_W     L'\\'
#  endif

#  ifndef FILENAME_SEPARATOR_STR
#  define FILENAME_SEPARATOR_STR   "\\"
#  endif

#  ifndef FILENAME_SEPARATOR_STR_W
#  define FILENAME_SEPARATOR_STR_W L"\\"
#  endif

#  ifndef PATH_SEPARATOR
#  define PATH_SEPARATOR           ';'
#  endif

#  ifndef PATH_SEPARATOR_W
#  define PATH_SEPARATOR_W         L';'
#  endif

#  ifndef PATH_SEPARATOR_STR
#  define PATH_SEPARATOR_STR       ";"
#  endif

#  ifndef PATH_SEPARATOR_STR_W
#  define PATH_SEPARATOR_STR_W     L";"
#  endif

#  ifndef LINE_SEPARATOR_STR
#  define LINE_SEPARATOR_STR       "\r\n"
#  endif

#  ifndef LINE_SEPARATOR_STR_W
#  define LINE_SEPARATOR_STR_W     L"\r\n"
#  endif

#endif // Windows / MAC



#ifdef UNIX


#  define PLATFORM_ACCEL_KEY ALT
#  define PLATFORM_ACCEL_STR "Alt"  // --  Look in rc.sed files
#  define FACCELKEY FALT

#define VK_OEM_SLASH 0xBF

#else   /* UNIX  */

#define INTERFACE_PROLOGUE(a)
#define INTERFACE_EPILOGUE(a)
#define INTERFACE_PROLOGUE_(a,b)
#define INTERFACE_EPILOGUE_(a,b)


#  define PLATFORM_ACCEL_KEY CONTROL
#  define PLATFORM_ACCEL_STR "Ctrl"  // --  Look in rc.sed files
#  define FACCELKEY FCONTROL

#define VK_OEM_SLASH '/'

#endif  /* UNIX */

#define MAKELONGLONG(low,high) ((LONGLONG)(((DWORD)(low)) | ((LONGLONG)((DWORD)(high))) << 32))

#ifdef BIG_ENDIAN
#define MAKE_LI(low,high) { high, low }
#define PALETTE_ENTRY( r, g, b, f )  { f, b, g, r }
#else
#define MAKE_LI(low,high) { low, high }
#define PALETTE_ENTRY( r, g, b, f )  { r, g, b, f }
#endif

#endif // __PLATFORM_H_
