/*
 * Wine porting definitions
 *
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE  /* for pread/pwrite, isfinite */
#endif
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_DIRECT_H
# include <direct.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_PROCESS_H
# include <process.h>
#endif
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/****************************************************************
 * Type definitions
 */

#if !defined(_MSC_VER) && !defined(__int64)
#  if defined(__x86_64__) || defined(_WIN64)
#    define __int64 long
#  else
#    define __int64 long long
#  endif
#endif

#if !defined(HAVE_MODE_T) && !defined(_MODE_T)
typedef int mode_t;
#endif
#if !defined(HAVE_OFF_T) && !defined(_OFF_T)
typedef long off_t;
#endif
#if !defined(HAVE_PID_T) && !defined(_PID_T)
typedef int pid_t;
#endif
#if !defined(HAVE_SIZE_T) && !defined(_SIZE_T)
typedef unsigned int size_t;
#endif
#if !defined(HAVE_SSIZE_T) && !defined(_SSIZE_T)
typedef int ssize_t;
#endif
//#ifndef HAVE_SOCKLEN_T
//typedef unsigned int socklen_t;
//#endif

#ifndef HAVE_STATFS
# ifdef __BEOS__
#  define HAVE_STRUCT_STATFS_F_BFREE
struct statfs {
  long   f_bsize;  /* block_size */
  long   f_blocks; /* total_blocks */
  long   f_bfree;  /* free_blocks */
};
# else /* defined(__BEOS__) */
struct statfs;
# endif /* defined(__BEOS__) */
#endif /* !defined(HAVE_STATFS) */

struct stat;

/****************************************************************
 * Macro definitions
 */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#define RTLD_LAZY    0x001
#define RTLD_NOW     0x002
#define RTLD_GLOBAL  0x100
#endif

#if !defined(HAVE_FTRUNCATE) && defined(HAVE_CHSIZE)
#define ftruncate chsize
#endif

#if !defined(HAVE_POPEN) && defined(HAVE__POPEN)
#define popen _popen
#endif

#if !defined(HAVE_PCLOSE) && defined(HAVE__PCLOSE)
#define pclose _pclose
#endif

#if !defined(HAVE_SNPRINTF) && defined(HAVE__SNPRINTF)
#define snprintf _snprintf
#endif

#if !defined(HAVE_VSNPRINTF) && defined(HAVE__VSNPRINTF)
#define vsnprintf _vsnprintf
#endif

#ifndef S_ISLNK
# define S_ISLNK(mod) (0)
#endif /* S_ISLNK */

/* So we open files in 64 bit access mode on Linux */
#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

/****************************************************************
 * Constants
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.570796326794896619
#endif

#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif

#ifndef INFINITY
static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()
#endif

#ifndef NAN
static inline float __port_nan(void)
{
    static const unsigned __nan_bytes = 0x7fc00000;
    return *(const float *)&__nan_bytes;
}
#define NAN __port_nan()
#endif

/* Constructor functions */

#ifdef _MSC_VER
# define DECL_GLOBAL_CONSTRUCTOR(func) /* nothing */
#elif defined(__GNUC__)
# define DECL_GLOBAL_CONSTRUCTOR(func) \
    static void func(void) __attribute__((constructor)); \
    static void func(void)
#elif defined(__i386__)
# define DECL_GLOBAL_CONSTRUCTOR(func) \
    static void __dummy_init_##func(void) { \
        asm(".section .init,\"ax\"\n\t" \
            "call " #func "\n\t" \
            ".previous"); } \
    static void func(void)
#elif defined(__sparc__)
# define DECL_GLOBAL_CONSTRUCTOR(func) \
    static void __dummy_init_##func(void) { \
        asm("\t.section \".init\",#alloc,#execinstr\n" \
            "\tcall " #func "\n" \
            "\tnop\n" \
            "\t.section \".text\",#alloc,#execinstr\n" ); } \
    static void func(void)
#elif defined(_M_AMD64)
#pragma message("You must define the DECL_GLOBAL_CONSTRUCTOR macro for amd64")
#else
# error You must define the DECL_GLOBAL_CONSTRUCTOR macro for your platform
#endif


/* Register functions */

#ifdef __i386__
#define DEFINE_REGS_ENTRYPOINT( name, fn, args, pop_args ) \
    __ASM_GLOBAL_FUNC( name, \
                       "call " __ASM_NAME("__wine_call_from_32_regs") "\n\t" \
                       ".long " __ASM_NAME(#fn) "\n\t" \
                       ".byte " #args "," #pop_args )
/* FIXME: add support for other CPUs */
#endif  /* __i386__ */


/****************************************************************
 * Function definitions (only when using libwine_port)
 */

#ifndef NO_LIBWINE_PORT

#ifndef HAVE_GETOPT_LONG
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;
struct option;

#ifndef HAVE_STRUCT_OPTION_NAME
struct option
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
};
#endif

extern int getopt_long (int ___argc, char *const *___argv,
                        const char *__shortopts,
                        const struct option *__longopts, int *__longind);
extern int getopt_long_only (int ___argc, char *const *___argv,
                             const char *__shortopts,
                             const struct option *__longopts, int *__longind);
#endif  /* HAVE_GETOPT_LONG */

#ifndef HAVE_GETPAGESIZE
size_t getpagesize(void);
#endif  /* HAVE_GETPAGESIZE */

#if !defined(HAVE_ISFINITE) && !defined(isfinite)
int isfinite(double x);
#endif

#if !defined(HAVE_ISINF) && !defined(isinf)
int isinf(double x);
#endif

#if !defined(HAVE_ISNAN) && !defined(isnan)
int isnan(double x);
#endif

#ifndef HAVE_LSTAT
int lstat(const char *file_name, struct stat *buf);
#endif /* HAVE_LSTAT */

#ifndef HAVE_MEMMOVE
void *memmove(void *dest, const void *src, size_t len);
#endif /* !defined(HAVE_MEMMOVE) */

#ifndef __REACTOS__
#ifndef HAVE_PREAD
ssize_t pread( int fd, void *buf, size_t count, off_t offset );
#endif /* HAVE_PREAD */

#ifndef HAVE_PWRITE
ssize_t pwrite( int fd, const void *buf, size_t count, off_t offset );
#endif /* HAVE_PWRITE */
#endif /* __REACTOS__ */

#ifdef _WIN32
#ifndef HAVE_SIGSETJMP
# include <setjmp.h>
typedef jmp_buf sigjmp_buf;
int sigsetjmp( sigjmp_buf buf, int savesigs );
void siglongjmp( sigjmp_buf buf, int val );
#endif /* HAVE_SIGSETJMP */
#endif

#ifndef HAVE_STATFS
int statfs(const char *name, struct statfs *info);
#endif /* !defined(HAVE_STATFS) */

#ifndef HAVE_STRNCASECMP
# ifndef HAVE__STRNICMP
int strncasecmp(const char *str1, const char *str2, size_t n);
# else
# define strncasecmp _strnicmp
# endif
#endif /* !defined(HAVE_STRNCASECMP) */

#ifndef HAVE_STRERROR
const char *strerror(int err);
#endif /* !defined(HAVE_STRERROR) */

#ifndef HAVE_STRCASECMP
# ifndef HAVE__STRICMP
int strcasecmp(const char *str1, const char *str2);
# else
# define strcasecmp _stricmp
# endif
#endif /* !defined(HAVE_STRCASECMP) */

#if !defined(HAVE_USLEEP) && !defined(__CYGWIN__)
int usleep (unsigned int useconds);
#endif /* !defined(HAVE_USLEEP) */

#ifdef __i386__
static inline void *memcpy_unaligned( void *dst, const void *src, size_t size )
{
    return memcpy( dst, src, size );
}
#else
extern void *memcpy_unaligned( void *dst, const void *src, size_t size );
#endif /* __i386__ */

extern int mkstemps(char *template, int suffix_len);

/* Process creation flags */
#ifndef _P_WAIT
# define _P_WAIT    0
# define _P_NOWAIT  1
# define _P_OVERLAY 2
# define _P_NOWAITO 3
# define _P_DETACH  4
#endif
#ifndef HAVE_SPAWNVP
extern int spawnvp(int mode, const char *cmdname, const char * const argv[]);
#endif

/* Interlocked functions */

#define interlocked_cmpxchg InterlockedCompareExchange
#define interlocked_cmpxchg_ptr InterlockedCompareExchangePointer
#define interlocked_xchg InterlockedExchange
#define interlocked_xchg_ptr InterlockedExchangePointer
#define interlocked_xchg_add InterlockedExchangeAdd

#if defined(_MSC_VER) && !defined(__clang__)
__forceinline
int
ffs(int mask)
{
    long index;
    if (_BitScanForward(&index, mask) == 0) return 0;
    return index;
}
#else
#define ffs __builtin_ffs
#endif

#else /* NO_LIBWINE_PORT */

#define __WINE_NOT_PORTABLE(func) func##_is_not_portable func##_is_not_portable

#define getopt_long             __WINE_NOT_PORTABLE(getopt_long)
#define getopt_long_only        __WINE_NOT_PORTABLE(getopt_long_only)
#define getpagesize             __WINE_NOT_PORTABLE(getpagesize)
#define lstat                   __WINE_NOT_PORTABLE(lstat)
#define memcpy_unaligned        __WINE_NOT_PORTABLE(memcpy_unaligned)
#define memmove                 __WINE_NOT_PORTABLE(memmove)
#define pread                   __WINE_NOT_PORTABLE(pread)
#define pwrite                  __WINE_NOT_PORTABLE(pwrite)
#define spawnvp                 __WINE_NOT_PORTABLE(spawnvp)
#define statfs                  __WINE_NOT_PORTABLE(statfs)
#define strcasecmp              __WINE_NOT_PORTABLE(strcasecmp)
#define strerror                __WINE_NOT_PORTABLE(strerror)
#define strncasecmp             __WINE_NOT_PORTABLE(strncasecmp)
#define usleep                  __WINE_NOT_PORTABLE(usleep)

#endif /* NO_LIBWINE_PORT */

#endif /* !defined(__WINE_WINE_PORT_H) */
