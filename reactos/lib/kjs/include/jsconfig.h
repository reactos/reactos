/* jsconfig.h.  Generated automatically by configure.  */
/* jsconfig.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/*
 * Define the PACKAGE and VERSION only if they are undefined.  This means
 * that we do not redefine them, when the library is used in another
 * GNU like package that defines PACKAGE and VERSION.
 */

/* Package name. */
#ifndef PACKAGE
#define PACKAGE "js"
#endif /* no PACKAGE */

/* Version number. */
#ifndef VERSION
#define VERSION "0.2.5"
#endif /* no VERSION */

/* Are C prototypes supported. */
#define PROTOTYPES 1

/* Canonical host name and its parts. */
#define CANONICAL_HOST "i686-pc-linux-gnu"
#define CANONICAL_HOST_CPU "i686"
#define CANONICAL_HOST_VENDOR "pc"
#define CANONICAL_HOST_OS "linux-gnu"

/* Do we want to build all instruction dispatchers? */
/* #undef ALL_DISPATCHERS */

/* Do we want to profile byte-code operands. */
/* #undef PROFILING */

/* Do we want the byte-code operand hooks. */
/* #undef BC_OPERAND_HOOKS */

/*
 * Unconditionall disable the jumps byte-code instruction dispatch
 * method.
 */
/* #undef DISABLE_JUMPS */

/* Does the struct stat has st_blksize member? */
#define HAVE_STAT_ST_ST_BLKSIZE 1

/* Does the struct stat has st_blocks member? */
#define HAVE_STAT_ST_ST_BLOCKS 1

/* Does the asctime_r() function take three arguments. */
/* #undef ASCTIME_R_WITH_THREE_ARGS */

/* Does the drand48_r() work with DRAND48D data. */
/* #undef DRAND48_R_WITH_DRAND48D */

/* How the attribute structures are passed to the init functions. */
/* #undef CONDATTR_BY_VALUE */
/* #undef MUTEXATTR_BY_VALUE */
/* #undef THREADATTR_BY_VALUE */

/* JS */
#define WITH_JS 1

/* Curses. */
/* #undef WITH_CURSES */
/* #undef HAVE_CURSES_H */
/* #undef HAVE_NCURSES_H */

/* MD5 */
#define WITH_MD5 1

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* Define if you have the drand48 function.  */
#define HAVE_DRAND48 1

/* Define if you have the lstat function.  */
#define HAVE_LSTAT 1

/* Define if you have the pthread_attr_create function.  */
/* #undef HAVE_PTHREAD_ATTR_CREATE */

/* Define if you have the pthread_attr_setscope function.  */
/* #undef HAVE_PTHREAD_ATTR_SETSCOPE */

/* Define if you have the pthread_attr_setstacksize function.  */
/* #undef HAVE_PTHREAD_ATTR_SETSTACKSIZE */

/* Define if you have the pthread_condattr_create function.  */
/* #undef HAVE_PTHREAD_CONDATTR_CREATE */

/* Define if you have the pthread_condattr_init function.  */
/* #undef HAVE_PTHREAD_CONDATTR_INIT */

/* Define if you have the pthread_mutexattr_create function.  */
/* #undef HAVE_PTHREAD_MUTEXATTR_CREATE */

/* Define if you have the sleep function.  */
#define HAVE_SLEEP 1

/* Define if you have the srand48 function.  */
#define HAVE_SRAND48 1

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the <dlfcn.h> header file.  */
#define HAVE_DLFCN_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <float.h> header file.  */
#define HAVE_FLOAT_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

#include "ddk/ntddk.h"
#include "ntos/rtl.h"
/* #include "assert.h" */
#ifndef __TYPE_UINT64
typedef unsigned __int64 __uint64;
#define __TYPE_UINT64
#endif/*__TYPE_UINT64*/

extern void __kernel_abort();
#define abort(x) __kernel_abort()
#define HUGE_VAL (__uint64)(-1LL)

#ifndef SETJMP_DEF
#define SETJMP_DEF
typedef struct _jmpbuf_kernel {
    int words[8];
} jmp_buf[1];
#endif/*SETJMP_DEF*/

int setjmp( jmp_buf j );
void longjmp( jmp_buf env, int val );
