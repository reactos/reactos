
#pragma once

/* translate GCC target defines to MS equivalents. */
#if defined(__i386__)
 #if !defined(_X86_)
  #define _X86_ 1
 #endif
 #if !defined(_M_IX86)
  #define _M_IX86 1
 #endif
#elif defined(__x86_64__) || defined(__x86_64)
 #if !defined(_AMD64_)
  #define _AMD64_ 1
 #endif
 #if !defined(_M_AMD64)
  #define _M_AMD64 1
 #endif
 #if !defined(_M_X64)
  #define _M_X64 1
 #endif
#elif defined(__arm__)
 #if !defined(_ARM_)
  #define _ARM_ 1
 #endif
 #if !defined(_M_ARM)
  #define _M_ARM 1
 #endif
#elif defined(__arm64__)
 #if !defined(_ARM64_)
  #define _ARM64_ 1
 #endif
 #if !defined(_M_ARM64)
  #define _M_ARM64 1
 #endif
#elif defined(__ia64__)
 #if !defined(_IA64_)
  #define _IA64_ 1
 #endif
 #if !defined(_M_IA64)
  #define _M_IA64 1
 #endif
#elif defined(__alpha__)
 #if !defined(_ALPHA_)
  #define _ALPHA_ 1
 #endif
 #if !defined(_M_ALPHA)
  #define _M_ALPHA 1
 #endif
#elif defined(__powerpc__)
 #if !defined(_PPC_)
  #define _PPC_ 1
 #endif
 #if !defined(_M_PPC)
  #define _M_PPC 1
 #endif
#else
#error Unknown architecture
#endif
