/* $Id: unistd.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * unistd.h
 *
 * standard symbolic constants and types. Conforming to the Single UNIX(r)
 * Specification Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __UNISTD_H_INCLUDED__
#define __UNISTD_H_INCLUDED__

/* INCLUDES */
#include <sys/types.h>
#include <stdio.h>
#include <inttypes.h>

/* OBJECTS */
extern char   *optarg;
extern int    optind, opterr, optopt;

/* TYPES */

/* CONSTANTS */
/* FIXME: set these constants appropriately */
/* Integer value indicating version of the ISO POSIX-1 standard (C
   language binding). */
#define _POSIX_VERSION    (0)

/* Integer value indicating version of the ISO POSIX-2 standard
   (Commands). */
#define _POSIX2_VERSION   (0)

/* Integer value indicating version of the ISO POSIX-2 standard (C
   language binding). */
#define _POSIX2_C_VERSION (0)

/* Integer value indicating version of the X/Open Portability Guide to
   which the implementation conforms. */
#define _XOPEN_VERSION     (500)

/* The version of the XCU specification to which the implementation
   conforms */
/* TODO: set to an appropriate value when commands and utilities will
   be available */
#define _XOPEN_XCU_VERSION (-1)

#if _XOPEN_XCU_VERSION != -1
#error TODO: define these constants
#define _POSIX2_C_BIND
#define _POSIX2_C_VERSION
#define _POSIX2_CHAR_TERM
#define _POSIX2_LOCALEDEF
#define _POSIX2_UPE
#define _POSIX2_VERSION
#endif

#if 0
/* TODO: check for conformance to the following specs */
#define _XOPEN_XPG2
#define _XOPEN_XPG3
#define _XOPEN_XPG4
#define _XOPEN_UNIX
#endif

#if 0
/* TODO: don't forget these features */
/* The use of chown() is restricted to a process with appropriate
   privileges, and to changing the group ID of a file only to the
   effective group ID of the process or to one of its supplementary
   group IDs. */
#define _POSIX_CHOWN_RESTRICTED

/* Terminal special characters defined in <termios.h> can be disabled
   using this character value. */
#define _POSIX_VDISABLE

/* Each process has a saved set-user-ID and a saved set-group-ID. */
#define _POSIX_SAVED_IDS

/* Implementation supports job control. */
#define _POSIX_JOB_CONTROL

#endif

/* Pathname components longer than {NAME_MAX} generate an error. */
#define _POSIX_NO_TRUNC (1)

/* The implementation supports the threads option. */
#define _POSIX_THREADS               (1)

/* FIXME: none of the following is strictly true yet */
/* The implementation supports the thread stack address attribute
   option. */ /* FIXME: not currently implemented. Should be trivial */
#define _POSIX_THREAD_ATTR_STACKADDR (1)

/* The implementation supports the thread stack size attribute
   option. */ /* FIXME: not currently implemented. Should be trivial */
#define _POSIX_THREAD_ATTR_STACKSIZE (1)

/* The implementation supports the process-shared synchronisation
   option. */ /* FIXME? not sure */
#define _POSIX_THREAD_PROCESS_SHARED (1)

/* The implementation supports the thread-safe functions option. */
/* FIXME: fix errno (currently not thread-safe) */
#define _POSIX_THREAD_SAFE_FUNCTIONS (1)

/*
   Constants for Options and Feature Groups
 */

/* Implementation supports the C Language Binding option. This will
   always have a value other than -1. */
#define _POSIX2_C_BIND (1)

/* Implementation supports the C Language Development Utilities
   option. */ /* FIXME: please change this when C compiler and
   utilities are ported */
#define _POSIX2_C_DEV (-1)

/* Implementation supports at least one terminal type. */ /* FIXME:
   please change this when terminal emulation is complete */
#define _POSIX2_CHAR_TERM (-1)

/* Implementation supports the FORTRAN Development Utilities option. */
/* FIXME: please change this when Fortran compiler and utilities are
   ported */
#define _POSIX2_FORT_DEV (-1)

/* Implementation supports the FORTRAN Run-time Utilities option. */
/* FIXME: please change this when Fortran runtimes are ported */
#define _POSIX2_FORT_RUN (-1)

/* Implementation supports the creation of locales by the localedef
   utility. */ /* FIXME: please change this when locales are ready */
#define _POSIX2_LOCALEDEF (-1)

/* Implementation supports the Software Development Utilities option. */
/* FIXME? */
#define _POSIX2_SW_DEV (-1)

/* The implementation supports the User Portability Utilities option. */
/* FIXME? */
#define _POSIX2_UPE (-1)

/* The implementation supports the X/Open Encryption Feature Group. */
/* FIXME: please change this when encryption is ready */
#define _XOPEN_CRYPT (-1)

/* The implementation supports the Issue 4, Version 2 Enhanced
   Internationalisation Feature Group. This is always set to a value
   other than -1. */ /* TODO: high priority. Support for this feature is
   needed for a conforming implementation */
#define _XOPEN_ENH_I18N (-1)

/* The implementation supports the Legacy Feature Group. */
#define _XOPEN_LEGACY (1)

/* The implementation supports the X/Open Realtime Feature Group. */
/* FIXME? unlikely to be ever supported */
#define _XOPEN_REALTIME (-1)

/* The implementation supports the X/Open Realtime Threads Feature
   Group. */ /* FIXME? really unlikely to be ever supported */
#define _XOPEN_REALTIME_THREADS (-1)

/* The implementation supports the Issue 4, Version 2 Shared Memory
   Feature Group. This is always set to a value other than -1. */ /* TODO:
   high priority. Support for this feature is needed for a conforming
   implementation */
#define _XOPEN_SHM (-1)

/* Implementation provides a C-language compilation environment with
   32-bit int, long, pointer and off_t types. */
#define _XBS5_ILP32_OFF32 (1)

/* Implementation provides a C-language compilation environment with 
   32-bit int, long and pointer types and an off_t type using at
   least 64 bits. */ /* FIXME? check the off_t type */
#define _XBS5_ILP32_OFFBIG (1)

/* Implementation provides a C-language compilation environment with
   32-bit int and 64-bit long, pointer and off_t types. */ /* FIXME: on
   some architectures this may be true */
#define _XBS5_LP64_OFF64 (-1)

/* Implementation provides a C-language compilation environment with
   an int type using at least 32 bits and long, pointer and off_t
   types using at least 64 bits. */ /* FIXME: on some architectures
   this may be true */
#define _XBS5_LPBIG_OFFBIG (-1)

/* Implementation supports the File Synchronisation option. */
/* TODO: high priority. Implement this */
#define _POSIX_FSYNC

/* Implementation supports the Memory Mapped Files option. */
/* TODO: high priority. Implement this */
#define _POSIX_MAPPED_FILES

/* Implementation supports the Memory Protection option. */
/* TODO: high priority. Implement this */
#define _POSIX_MEMORY_PROTECTION

#if 0
/* Implementation supports the Prioritized Input and Output option. */
/* FIXME? unlikely to be ever supported */
#define _POSIX_PRIORITIZED_IO
#endif

/* FIXME: these should be implemented */
/* Asynchronous input or output operations may be performed for the
   associated file. */
#define _POSIX_ASYNC_IO (-1)

/* Prioritized input or output operations may be performed for the
   associated file. */
#define _POSIX_PRIO_IO (-1)

/* Synchronised input or output operations may be performed for the
   associated file. */
#define _POSIX_SYNC_IO (-1)

/*
   null pointer
 */
#ifndef NULL
/* NULL seems to be defined pretty much everywhere - we prevent
   redefinition */
#define NULL ((void *)(0))
#endif

/*
   constants for the access() function
 */

/* Test for read permission. */
#define R_OK (0x00000001)
/*  Test for write permission. */
#define W_OK (0x00000002)
/* Test for execute (search) permission. */
#define X_OK (0x00000004)
/* Test for existence of file. */
#define F_OK (0)

/*
  constants for the confstr() function
 */
#define _CS_PATH                        (1)
#define _CS_XBS5_ILP32_OFF32_CFLAGS     (2)
#define _CS_XBS5_ILP32_OFF32_LDFLAGS    (3)
#define _CS_XBS5_ILP32_OFF32_LIBS       (4)
#define _CS_XBS5_ILP32_OFF32_LINTFLAGS  (5)
#define _CS_XBS5_ILP32_OFFBIG_CFLAGS    (6)
#define _CS_XBS5_ILP32_OFFBIG_LDFLAGS   (7)
#define _CS_XBS5_ILP32_OFFBIG_LIBS      (8)
#define _CS_XBS5_ILP32_OFFBIG_LINTFLAGS (9)
#define _CS_XBS5_LP64_OFF64_CFLAGS      (10)
#define _CS_XBS5_LP64_OFF64_LDFLAGS     (11)
#define _CS_XBS5_LP64_OFF64_LIBS        (12)
#define _CS_XBS5_LP64_OFF64_LINTFLAGS   (13)
#define _CS_XBS5_LPBIG_OFFBIG_CFLAGS    (14)
#define _CS_XBS5_LPBIG_OFFBIG_LDFLAGS   (15)
#define _CS_XBS5_LPBIG_OFFBIG_LIBS      (16)
#define _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS (17)

/*
 constants for the lseek() and fcntl() functions
 */

#define SEEK_SET (1) /* Set file offset to offset. */
#define SEEK_CUR (2) /* Set file offset to current plus offset. */
#define SEEK_END (3) /* Set file offset to EOF plus offset. */

/*
  constants for sysconf()
 */
#define _SC_2_C_BIND                     (1)
#define _SC_2_C_DEV                      (2)
#define _SC_2_C_VERSION                  (3)
#define _SC_2_FORT_DEV                   (4)
#define _SC_2_FORT_RUN                   (5)
#define _SC_2_LOCALEDEF                  (6)
#define _SC_2_SW_DEV                     (7)
#define _SC_2_UPE                        (8)
#define _SC_2_VERSION                    (9)
#define _SC_ARG_MAX                      (10)
#define _SC_AIO_LISTIO_MAX               (11)
#define _SC_AIO_MAX                      (12)
#define _SC_AIO_PRIO_DELTA_MAX           (13)
#define _SC_ASYNCHRONOUS_IO              (14)
#define _SC_ATEXIT_MAX                   (15)
#define _SC_BC_BASE_MAX                  (16)
#define _SC_BC_DIM_MAX                   (17)
#define _SC_BC_SCALE_MAX                 (18)
#define _SC_BC_STRING_MAX                (19)
#define _SC_CHILD_MAX                    (20)
#define _SC_CLK_TCK                      (21)
#define _SC_COLL_WEIGHTS_MAX             (22)
#define _SC_DELAYTIMER_MAX               (23)
#define _SC_EXPR_NEST_MAX                (24)
#define _SC_FSYNC                        (25)
#define _SC_GETGR_R_SIZE_MAX             (26)
#define _SC_GETPW_R_SIZE_MAX             (27)
#define _SC_IOV_MAX                      (28)
#define _SC_JOB_CONTROL                  (29)
#define _SC_LINE_MAX                     (30)
#define _SC_LOGIN_NAME_MAX               (31)
#define _SC_MAPPED_FILES                 (32)
#define _SC_MEMLOCK                      (33)
#define _SC_MEMLOCK_RANGE                (34)
#define _SC_MEMORY_PROTECTION            (35)
#define _SC_MESSAGE_PASSING              (36)
#define _SC_MQ_OPEN_MAX                  (37)
#define _SC_MQ_PRIO_MAX                  (38)
#define _SC_NGROUPS_MAX                  (39)
#define _SC_OPEN_MAX                     (40)
#define _SC_PAGE_SIZE                    (41)
#define _SC_PASS_MAX                     (42) /* LEGACY */
#define _SC_PRIORITIZED_IO               (43)
#define _SC_PRIORITY_SCHEDULING          (44)
#define _SC_RE_DUP_MAX                   (45)
#define _SC_REALTIME_SIGNALS             (46)
#define _SC_RTSIG_MAX                    (47)
#define _SC_SAVED_IDS                    (48)
#define _SC_SEMAPHORES                   (49)
#define _SC_SEM_NSEMS_MAX                (50)
#define _SC_SEM_VALUE_MAX                (51)
#define _SC_SHARED_MEMORY_OBJECTS        (52)
#define _SC_SIGQUEUE_MAX                 (53)
#define _SC_STREAM_MAX                   (54)
#define _SC_SYNCHRONIZED_IO              (55)
#define _SC_THREADS                      (56)
#define _SC_THREAD_ATTR_STACKADDR        (57)
#define _SC_THREAD_ATTR_STACKSIZE        (58)
#define _SC_THREAD_DESTRUCTOR_ITERATIONS (59)
#define _SC_THREAD_KEYS_MAX              (60)
#define _SC_THREAD_PRIORITY_SCHEDULING   (61)
#define _SC_THREAD_PRIO_INHERIT          (62)
#define _SC_THREAD_PRIO_PROTECT          (63)
#define _SC_THREAD_PROCESS_SHARED        (64)
#define _SC_THREAD_SAFE_FUNCTIONS        (65)
#define _SC_THREAD_STACK_MIN             (66)
#define _SC_THREAD_THREADS_MAX           (67)
#define _SC_TIMERS                       (68)
#define _SC_TIMER_MAX                    (69)
#define _SC_TTY_NAME_MAX                 (70)
#define _SC_TZNAME_MAX                   (71)
#define _SC_VERSION                      (72)
#define _SC_XOPEN_VERSION                (73)
#define _SC_XOPEN_CRYPT                  (74)
#define _SC_XOPEN_ENH_I18N               (75)
#define _SC_XOPEN_SHM                    (76)
#define _SC_XOPEN_UNIX                   (77)
#define _SC_XOPEN_XCU_VERSION            (78)
#define _SC_XOPEN_LEGACY                 (79)
#define _SC_XOPEN_REALTIME               (80)
#define _SC_XOPEN_REALTIME_THREADS       (81)
#define _SC_XBS5_ILP32_OFF32             (82)
#define _SC_XBS5_ILP32_OFFBIG            (83)
#define _SC_XBS5_LP64_OFF64              (84)
#define _SC_XBS5_LPBIG_OFFBIG            (85)

#define _SC_PAGESIZE _SC_PAGE_SIZE

/* possible values for the function argument to the lockf() function */
/* Lock a section for exclusive use. */
#define F_LOCK  (1)
/* Unlock locked sections. */
#define F_ULOCK (2)
/* Test section for locks by other processes. */
#define F_TEST  (3)
/* Test and lock a section for exclusive use. */
#define F_TLOCK (4)

/* File number of stdin. It is 0. */
#define STDIN_FILENO  (0)
/* File number of stdout. It is 1. */
#define STDOUT_FILENO (1)
/* File number of stderr. It is 2. */
#define STDERR_FILENO (2)

/* PROTOTYPES */
int          access(const char *, int);
unsigned int alarm(unsigned int);
int          brk(void *);
int          chdir(const char *);
int          chroot(const char *); /* LEGACY */
int          chown(const char *, uid_t, gid_t);
int          close(int);
size_t       confstr(int, char *, size_t);
char        *crypt(const char *, const char *);
char        *ctermid(char *);
char        *cuserid(char *s); /* LEGACY */
int          dup(int);
int          dup2(int, int);
void         encrypt(char[64], int);
int          execl(const char *, const char *, ...);
int          execle(const char *, const char *, ...);
int          execlp(const char *, const char *, ...);
int          execv(const char *, char *const []);
int          execve(const char *, char *const [], char *const []);
int          execvp(const char *, char *const []);
void        _exit(int);
int          fchown(int, uid_t, gid_t);
int          fchdir(int);
int          fdatasync(int);
pid_t        fork(void);
long int     fpathconf(int, int);
int          fsync(int);
int          ftruncate(int, off_t);
char        *getcwd(char *, size_t);
int          getdtablesize(void); /* LEGACY */
gid_t        getegid(void);
uid_t        geteuid(void);
gid_t        getgid(void);
int          getgroups(int, gid_t []);
long         gethostid(void);
char        *getlogin(void);
int          getlogin_r(char *, size_t);
int          getopt(int, char * const [], const char *);
int          getpagesize(void); /* LEGACY */
char        *getpass(const char *); /* LEGACY */
pid_t        getpgid(pid_t);
pid_t        getpgrp(void);
pid_t        getpid(void);
pid_t        getppid(void);
pid_t        getsid(pid_t);
uid_t        getuid(void);
char        *getwd(char *);
int          isatty(int);
int          lchown(const char *, uid_t, gid_t);
int          link(const char *, const char *);
int          lockf(int, int, off_t);
off_t        lseek(int, off_t, int);
int          nice(int);
long int     pathconf(const char *, int);
int          pause(void);
int          pipe(int [2]);
ssize_t      pread(int, void *, size_t, off_t);
int          pthread_atfork(void (*)(void), void (*)(void),
                 void(*)(void));
ssize_t      pwrite(int, const void *, size_t, off_t);
ssize_t      read(int, void *, size_t);
int          readlink(const char *, char *, size_t);
int          rmdir(const char *);
void        *sbrk(intptr_t);
int          setgid(gid_t);
int          setpgid(pid_t, pid_t);
pid_t        setpgrp(void);
int          setregid(gid_t, gid_t);
int          setreuid(uid_t, uid_t);
pid_t        setsid(void);
int          setuid(uid_t);
unsigned int sleep(unsigned int);
void         swab(const void *, void *, ssize_t);
int          symlink(const char *, const char *);
void         sync(void);
long int     sysconf(int);
pid_t        tcgetpgrp(int);
int          tcsetpgrp(int, pid_t);
int          truncate(const char *, off_t);
char        *ttyname(int);
int          ttyname_r(int, char *, size_t);
useconds_t   ualarm(useconds_t, useconds_t);
int          unlink(const char *);
int          usleep(useconds_t);
pid_t        vfork(void);
ssize_t      write(int, const void *, size_t);

/* MACROS */

#endif /* __UNISTD_H_INCLUDED__ */

/* EOF */

