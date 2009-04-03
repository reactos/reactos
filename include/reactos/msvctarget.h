#ifndef _MSC_VER
#ifndef __GNUC__
#error Unsupported compiler
#endif

/* translate GCC target defines to MS equivalents. */
#if defined(__i686__) && !defined(_M_IX86)
#define _M_IX86 600
#undef __i686__
#elif defined(__i586__) && !defined(_M_IX86)
#define _M_IX86 500
#undef __i586__
#elif defined(__i486__) && !defined(_M_IX86)
#define _M_IX86 400
#undef __i486__
#elif defined(__i386__) && !defined(_M_IX86)
#define _M_IX86 300
#endif
#endif
