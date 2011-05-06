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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#define _GNU_SOURCE  /* for pread/pwrite */
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

#ifndef HAVE_MODE_T
typedef int mode_t;
#endif
#ifndef HAVE_OFF_T
typedef long off_t;
#endif
#ifndef HAVE_PID_T
typedef int pid_t;
#endif
#ifndef HAVE_SIZE_T
typedef unsigned int size_t;
#endif
#ifndef HAVE_SSIZE_T
typedef int ssize_t;
#endif
#ifndef HAVE_FSBLKCNT_T
typedef unsigned long fsblkcnt_t;
#endif
#ifndef HAVE_FSFILCNT_T
typedef unsigned long fsfilcnt_t;
#endif

#ifndef HAVE_STRUCT_STATVFS_F_BLOCKS
struct statvfs
{
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t    f_blocks;
    fsblkcnt_t    f_bfree;
    fsblkcnt_t    f_bavail;
    fsfilcnt_t    f_files;
    fsfilcnt_t    f_ffree;
    fsfilcnt_t    f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;
};
#endif /* HAVE_STRUCT_STATVFS_F_BLOCKS */


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
#endif

#ifndef S_ISSOCK
# define S_ISSOCK(mod) (0)
#endif

#ifndef S_ISDIR
# define S_ISDIR(mod) (((mod) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_ISCHR
# define S_ISCHR(mod) (((mod) & _S_IFMT) == _S_IFCHR)
#endif

#ifndef S_ISFIFO
# define S_ISFIFO(mod) (((mod) & _S_IFMT) == _S_IFIFO)
#endif

#ifndef S_ISREG
# define S_ISREG(mod) (((mod) & _S_IFMT) == _S_IFREG)
#endif

#ifndef S_IWUSR
# define S_IWUSR 0
#endif

/* So we open files in 64 bit access mode on Linux */
#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#if !defined(S_IXUSR) && defined(S_IEXEC)
# define S_IXUSR S_IEXEC
#endif
#if !defined(S_IXGRP) && defined(S_IEXEC)
# define S_IXGRP S_IEXEC
#endif
#if !defined(S_IXOTH) && defined(S_IEXEC)
# define S_IXOTH S_IEXEC
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


/* Macros to define assembler functions somewhat portably */

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(__APPLE__)
# define __ASM_GLOBAL_FUNC(name,code) \
      __asm__( ".text\n\t" \
               ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code \
               "\n\t.previous" );
#else  /* defined(__GNUC__) && !defined(__MINGW32__) && !defined(__APPLE__)  */
# define __ASM_GLOBAL_FUNC(name,code) \
      void __asm_dummy_##name(void) { \
          asm( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code ); \
      }
#endif  /* __GNUC__ */


/* Constructor functions */

#ifdef __GNUC__
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

#ifndef HAVE_FSTATVFS
int fstatvfs( int fd, struct statvfs *buf );
#endif

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

#ifndef HAVE_FFS
int ffs( int x );
#endif

#ifndef HAVE_FUTIMES
struct timeval;
int futimes(int fd, const struct timeval tv[2]);
#endif

#ifndef HAVE_GETPAGESIZE
size_t getpagesize(void);
#endif  /* HAVE_GETPAGESIZE */

#ifndef HAVE_GETTID
pid_t gettid(void);
#endif /* HAVE_GETTID */

#ifndef HAVE_LSTAT
int lstat(const char *file_name, struct stat *buf);
#endif /* HAVE_LSTAT */

#ifndef HAVE_MEMMOVE
void *memmove(void *dest, const void *src, size_t len);
#endif /* !defined(HAVE_MEMMOVE) */

#ifndef HAVE_PREAD
ssize_t pread( int fd, void *buf, size_t count, off_t offset );
#endif /* HAVE_PREAD */

#ifndef HAVE_PWRITE
ssize_t pwrite( int fd, const void *buf, size_t count, off_t offset );
#endif /* HAVE_PWRITE */

#ifndef HAVE_READLINK
int readlink( const char *path, char *buf, size_t size );
#endif /* HAVE_READLINK */

#ifndef HAVE_SIGSETJMP
# include <setjmp.h>
typedef jmp_buf sigjmp_buf;
int sigsetjmp( sigjmp_buf buf, int savesigs );
void siglongjmp( sigjmp_buf buf, int val );
#endif /* HAVE_SIGSETJMP */

#ifndef HAVE_STATVFS
int statvfs( const char *path, struct statvfs *buf );
#endif

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

#ifndef HAVE_USLEEP
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

#if defined(__i386__) && defined(__GNUC__)

extern inline long interlocked_cmpxchg( long *dest, long xchg, long compare );
extern inline void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare );
extern inline long interlocked_xchg( long *dest, long val );
extern inline void *interlocked_xchg_ptr( void **dest, void *val );
extern inline long interlocked_xchg_add( long *dest, long incr );

extern inline long interlocked_cmpxchg( long *dest, long xchg, long compare )
{
    long ret;
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (dest), "r" (xchg), "0" (compare) : "memory" );
    return ret;
}

extern inline void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    void *ret;
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (dest), "r" (xchg), "0" (compare) : "memory" );
    return ret;
}

extern inline long interlocked_xchg( long *dest, long val )
{
    long ret;
    __asm__ __volatile__( "lock; xchgl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (val) : "memory" );
    return ret;
}

extern inline void *interlocked_xchg_ptr( void **dest, void *val )
{
    void *ret;
    __asm__ __volatile__( "lock; xchgl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (val) : "memory" );
    return ret;
}

extern inline long interlocked_xchg_add( long *dest, long incr )
{
    long ret;
    __asm__ __volatile__( "lock; xaddl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (incr) : "memory" );
    return ret;
}

#else  /* __i386___ && __GNUC__ */

extern long interlocked_cmpxchg( long *dest, long xchg, long compare );
extern void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare );
extern long interlocked_xchg( long *dest, long val );
extern void *interlocked_xchg_ptr( void **dest, void *val );
extern long interlocked_xchg_add( long *dest, long incr );

#endif  /* __i386___ && __GNUC__ */

#else /* NO_LIBWINE_PORT */

#define __WINE_NOT_PORTABLE(func) func##_is_not_portable func##_is_not_portable

#define ffs                     __WINE_NOT_PORTABLE(ffs)
#define fstatvfs                __WINE_NOT_PORTABLE(fstatvfs)
#define futimes                 __WINE_NOT_PORTABLE(futimes)
#define getopt_long             __WINE_NOT_PORTABLE(getopt_long)
#define getopt_long_only        __WINE_NOT_PORTABLE(getopt_long_only)
#define getpagesize             __WINE_NOT_PORTABLE(getpagesize)
#define interlocked_cmpxchg     __WINE_NOT_PORTABLE(interlocked_cmpxchg)
#define interlocked_cmpxchg_ptr __WINE_NOT_PORTABLE(interlocked_cmpxchg_ptr)
#define interlocked_xchg        __WINE_NOT_PORTABLE(interlocked_xchg)
#define interlocked_xchg_ptr    __WINE_NOT_PORTABLE(interlocked_xchg_ptr)
#define interlocked_xchg_add    __WINE_NOT_PORTABLE(interlocked_xchg_add)
#define lstat                   __WINE_NOT_PORTABLE(lstat)
#define memcpy_unaligned        __WINE_NOT_PORTABLE(memcpy_unaligned)
#define memmove                 __WINE_NOT_PORTABLE(memmove)
#define pread                   __WINE_NOT_PORTABLE(pread)
#define pwrite                  __WINE_NOT_PORTABLE(pwrite)
#define spawnvp                 __WINE_NOT_PORTABLE(spawnvp)
#define statvfs                 __WINE_NOT_PORTABLE(statvfs)
#define strcasecmp              __WINE_NOT_PORTABLE(strcasecmp)
#define strerror                __WINE_NOT_PORTABLE(strerror)
#define strncasecmp             __WINE_NOT_PORTABLE(strncasecmp)
#define usleep                  __WINE_NOT_PORTABLE(usleep)

#endif /* NO_LIBWINE_PORT */

#endif /* !defined(__WINE_WINE_PORT_H) */
