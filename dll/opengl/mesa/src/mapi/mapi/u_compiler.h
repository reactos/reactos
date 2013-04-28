#ifndef _U_COMPILER_H_
#define _U_COMPILER_H_

/* Function inlining */
#ifndef inline
#  ifdef __cplusplus
     /* C++ supports inline keyword */
#  elif defined(__GNUC__)
#    define inline __inline__
#  elif defined(_MSC_VER)
#    define inline __inline
#  elif defined(__ICL)
#    define inline __inline
#  elif defined(__INTEL_COMPILER)
     /* Intel compiler supports inline keyword */
#  elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#    define inline __inline
#  elif defined(__SUNPRO_C) && defined(__C99FEATURES__)
     /* C99 supports inline keyword */
#  elif (__STDC_VERSION__ >= 199901L)
     /* C99 supports inline keyword */
#  else
#    define inline
#  endif
#endif
#ifndef INLINE
#  define INLINE inline
#endif

/* Function visibility */
#ifndef PUBLIC
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PUBLIC __attribute__((visibility("default")))
#  elif defined(_MSC_VER)
#    define PUBLIC __declspec(dllexport)
#  else
#    define PUBLIC
#  endif
#endif

#ifndef likely
#  if defined(__GNUC__)
#    define likely(x)   __builtin_expect(!!(x), 1)
#    define unlikely(x) __builtin_expect(!!(x), 0)
#  else
#    define likely(x)   (x)
#    define unlikely(x) (x)
#  endif
#endif

#endif /* _U_COMPILER_H_ */
