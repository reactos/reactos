/* softx86cfg.h
 *
 * Configuration #defines for Softx86 emulation library.
 *
 * (C) 2003, 2004 Jonathan Campbell.
 *
 * If ENDIAN.H is present, this header file determines the native byte
 * order from that. If WIN32 is defined, this header assumes little
 * Endian since the Win32 environment exists only on the x86 platform.
 * If neither happens, a set of #defines will be used that must be modified
 * manually to fit your system.
 *  */

/* If we're being compiled with GCC and NOT MINGW assume Linux environment
   and headers */
#ifdef __GNUC__
#ifndef __MINGW32__
#include <endian.h>	/*   /usr/include/endian.h   */
#endif
#endif //__GNUC__

/* AUTODETECT USING ENDIAN.H UNDER LINUX */
#ifdef _ENDIAN_H

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SX86_BYTE_ORDER			LE
#elif __BYTE_ORDER == __BIG_ENDIAN
#define SX86_BYTE_ORDER			BE
#else
#error endian.h does not provide byte order!
// #define it here
#endif /* __BYTE_ORDER */

/* 32-bit Windows */
#elif WIN32

/* comment this out if compiling with Microsoft Visual C++ 6.0
   and you have installed Service Pack 5. otherwise, you need
   this so Softx86 can incorporate workarounds for error C2520:
   "conversion from unsigned __int64 to double not implemented" */
#define MSVC_CANT_CONVERT_UI64_TO_DOUBLE

#define SX86_BYTE_ORDER			LE

/* 32-bit Windows, using MINGW */
#elif __MINGW32__

#define SX86_BYTE_ORDER			LE

#else
/* WE HAVE NO IDEA, SO WHOEVER IS COMPILING THE LIBRARY AND DEPENDENT
 * CODE NEEDS TO MODIFY THESE MANUALLY TO FIT THEIR PLATFORM */

/* comment this out when you have modified the #defines below */
/* obviously when you are done this needs to be removed :) */
#error Unable to auto-detect your platform! You must modify softx86cfg.h manually!

/* modify these to your platform. acceptable values are LE and BE */
#define SX86_BYTE_ORDER			LE

#endif

#define SX86_INSIDER
