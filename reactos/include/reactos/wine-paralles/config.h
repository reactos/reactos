//
// hand-tuned for native win32 build with MSVC
//

#define __WINE_CONFIG_H

/* avoid fine grained includes which cause problems with MS SDK */
#define WINE_NATIVEWIN32 0

/* use MSVC native exceptions */
#define USE_COMPILER_EXCEPTIONS 1

#ifdef NDEBUG
# define WINE_NO_DEBUG_MSGS 1
# define WINE_NO_TRACE_MSGS 1
#endif


#define HAVE_MODE_T 1
#define HAVE_SSIZE_T 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define if OpenGL is present on the system */
#define HAVE_OPENGL 1

/* Define to 1 if you have the <GL/glext.h> header file. */
#define HAVE_GL_GLEXT_H 1

/* Define to 1 if you have the <GL/gl.h> header file. */
#define HAVE_GL_GL_H 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if the system has the type `size_t'. */
#define HAVE_SIZE_T 1

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `vsnprintf' function. */
#undef HAVE_VSNPRINTF

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `_stricmp' function. */
#define HAVE__STRICMP 1

/* Define to 1 if you have the `_strnicmp' function. */
#define HAVE__STRNICMP 1

/* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF 1

/* Define to a macro to generate an assembly function directive */
#undef __ASM_FUNC

/* Define to a macro to generate an assembly name from a C symbol */
#undef __ASM_NAME

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline
#endif

/* MSVC expects function pointer attributes within braces
   GCC is happy with WINAPI (x) */
#define WINE_WINAPI(x) (WINAPI x)
#define WINE_APIENTRY(x) (APIENTRY x)

/* these are defined in WINE winnt.h which we don't use */
#define DUMMYSTRUCTNAME s
#define DUMMYSTRUCTNAME1 s1
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#define DUMMYSTRUCTNAME4 s4
#define DUMMYSTRUCTNAME5 s5
#define DUMMYUNIONNAME u
#define DUMMYUNIONNAME1 u1
#define DUMMYUNIONNAME2 u2
#define DUMMYUNIONNAME3 u3
#define DUMMYUNIONNAME4 u4
#define DUMMYUNIONNAME5 u5
/* for native headers */
#define DUMMYUNIONNAMEN(n) u##n
