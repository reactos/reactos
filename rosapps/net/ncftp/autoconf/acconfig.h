/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

#undef CAN_USE_SYS_SELECT_H

/* Define if using alloca.c.  */
#undef C_ALLOCA

/* Define if type char is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
#undef __CHAR_UNSIGNED__
#endif

/* Define if the closedir function returns void instead of int.  */
#undef CLOSEDIR_VOID

/* Define to empty if the keyword does not work.  */
#undef const

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define for DGUX with <sys/dg_sys_info.h>.  */
#undef DGUX

/* Define if you have <dirent.h>.  */
#undef DIRENT

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#undef GETGROUPS_T

/* Define if the `getloadavg' function needs to be run setuid or setgid.  */
#undef GETLOADAVG_PRIVILEGED

/* Define if the `getpgrp' function takes no argument.  */
#undef GETPGRP_VOID

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef gid_t

/* Define if you have alloca, as a function or macro.  */
#undef HAVE_ALLOCA

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#undef HAVE_ALLOCA_H

/* Define if your curses library has this functionality. */
#undef HAVE_BEEP

#undef HAVE_CURSES_H

/* Define if you don't have vprintf but do have _doprnt.  */
#undef HAVE_DOPRNT

/* Define if your system has a working fnmatch function.  */
#undef HAVE_FNMATCH

/* Define if your curses library has this functionality. */
#undef HAVE_GETBEGX

/* Define if your system has its own `getloadavg' function.  */
#undef HAVE_GETLOADAVG

/* Define if your curses library has this functionality. */
#undef HAVE_GETMAXX

/* Define if your curses library has this functionality. */
#undef HAVE_GETMAXYX

/* Define if you have the getmntent function.  */
#undef HAVE_GETMNTENT

/* Define if you have <hpsecurity.h>.  */
#undef HAVE_HPSECURITY_H

/* Define if you have the curses library. */
#undef HAVE_LIBCURSES

/* Define if you want to use the Hpwd library, and you also have it's corresponding database library (such as *dbm). */
#undef HAVE_LIBHPWD

/* Define if you have the ncurses library. */
#undef HAVE_LIBNCURSES

/* Define if you have the readline library, version 2.0 or higher. */
#undef HAVE_LIBREADLINE

/* Define if the `long double' type works.  */
#undef HAVE_LONG_DOUBLE

/* Define if you support file names longer than 14 characters.  */
#undef HAVE_LONG_FILE_NAMES

/* Define if your compiler supports the "long long" integral type. */
#undef HAVE_LONG_LONG

/* Most system's curses library uses a _maxx field instead of maxx. */
#undef HAVE__MAXX

/* Define if you have a working `mmap' system call.  */
#undef HAVE_MMAP

#undef HAVE_MSGHDR_ACCRIGHTS

#undef HAVE_MSGHDR_CONTROL

#undef HAVE_PR_PASSWD_FG_OLDCRYPT

/* Define if you have a _res global variable used by resolve routines. */
#undef HAVE__RES_DEFDNAME

/* Define if system calls automatically restart after interruption
   by a signal.  */
#undef HAVE_RESTARTABLE_SYSCALLS

/* Define if you have sigsetjmp and siglongjmp. */
#undef HAVE_SIGSETJMP

#undef HAVE_SOCKADDR_UN_SUN_LEN

#undef HAVE_STATFS_F_BAVAIL

/* Define if your struct stat has st_blksize.  */
#undef HAVE_ST_BLKSIZE

/* Define if your struct stat has st_blocks.  */
#undef HAVE_ST_BLOCKS

/* Define if you have the strcoll function and it is properly defined.  */
#undef HAVE_STRCOLL

/* Define if your struct stat has st_rdev.  */
#undef HAVE_ST_RDEV

/* Define if you have the strftime function.  */
#undef HAVE_STRFTIME

/* Define if you have the ANSI # stringizing operator in cpp. */
#undef HAVE_STRINGIZE

#undef HAVE_STRUCT_CMSGDHR

#undef HAVE_STRUCT_STAT64

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#undef HAVE_SYS_WAIT_H

/* Define if your curses library has this functionality. */
#undef HAVE_TOUCHWIN

/* Define if your struct tm has tm_zone.  */
#undef HAVE_TM_ZONE

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#undef HAVE_TZNAME

/* Define if you have <unistd.h>.  */
#undef HAVE_UNISTD_H

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
#undef HAVE_UTIME_NULL

/* Define if you have a ut_host field in your struct utmp. */
#undef HAVE_UTMP_UT_HOST

#undef HAVE_UTMP_UT_NAME

#undef HAVE_UTMP_UT_USER

#undef HAVE_UTMP_UT_PID

#undef HAVE_UTMP_UT_TIME

#undef HAVE_UTMPX_UT_SYSLEN

/* Define if you have <vfork.h>.  */
#undef HAVE_VFORK_H

/* Define if you have the vprintf function.  */
#undef HAVE_VPRINTF

/* Define if you have the wait3 system call.  */
#undef HAVE_WAIT3

/* Define as __inline if that's what the C compiler calls it.  */
#undef inline

/* Define if chown is promiscuous (regular user can give away ownership) */
#undef INSECURE_CHOWN

/* Define if int is 16 bits instead of 32.  */
#undef INT_16_BITS

/* Define if long int is 64 bits.  */
#undef LONG_64_BITS

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
#undef MAJOR_IN_MKDEV

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
#undef MAJOR_IN_SYSMACROS

/* Define if on MINIX.  */
#undef _MINIX

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef mode_t

/* Define if you don't have <dirent.h>, but have <ndir.h>.  */
#undef NDIR

/* Define if you have <memory.h>, and <string.h> doesn't declare the
   mem* functions.  */
#undef NEED_MEMORY_H

/* Define if your struct nlist has an n_un member.  */
#undef NLIST_NAME_UNION

/* Define if you have <nlist.h>.  */
#undef NLIST_STRUCT

/* Define if your C compiler doesn't accept -c and -o together.  */
#undef NO_MINUS_C_MINUS_O

/* Define if your Fortran 77 compiler doesn't accept -c and -o together. */
#undef F77_NO_MINUS_C_MINUS_O

/* Define to `long' if <sys/types.h> doesn't define.  */
#undef off_t

#undef OS

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef pid_t

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
#undef _POSIX_1_SOURCE

/* Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* Format string for the printf() family for 64 bit integers. */
#undef PRINTF_LONG_LONG

/* Define if printing a "long long" with "%lld" works . */
#undef PRINTF_LONG_LONG_LLD

/* Define if printing a "long long" with "%qd" works . */
#undef PRINTF_LONG_LONG_QD

/* Define if your C compiler supports ANSI C function prototyping. */
#undef PROTOTYPES

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* Format string for the scanf() family for 64 bit integers. */
#undef SCANF_LONG_LONG

/* Define if scanning a "long long" with "%lld" works. */
#undef SCANF_LONG_LONG_LLD

/* Define if scanning a "long long" with "%qd" works. */
#undef SCANF_LONG_LONG_QD
  
/* Define to the type of arg1 for select(). */
#undef SELECT_TYPE_ARG1

/* Define to the type of args 2, 3 and 4 for select(). */
#undef SELECT_TYPE_ARG234

/* Define to the type of arg5 for select(). */
#undef SELECT_TYPE_ARG5

/* Define if the `setpgrp' function takes no argument.  */
#undef SETPGRP_VOID

/* Define if the setvbuf function takes the buffering type as its second
   argument and the buffer pointer as the third, as on System V
   before release 3.  */
#undef SETVBUF_REVERSED

/* Define to `int' if <sys/signal.h> doesn't define.  */
#undef sig_atomic_t

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

#undef SNPRINTF_TERMINATES

#undef SPRINTF_RETURNS_PTR

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
#undef STACK_DIRECTION

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
#undef STAT_MACROS_BROKEN

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define on System V Release 4.  */
#undef SVR4

/* Define if you don't have <dirent.h>, but have <sys/dir.h>.  */
#undef SYSDIR

/* Define if you don't have <dirent.h>, but have <sys/ndir.h>.  */
#undef SYSNDIR

/* Define if `sys_siglist' is declared by <signal.h>.  */
#undef SYS_SIGLIST_DECLARED

/* Define to the full path of the Tar program, if you have it. */
#undef TAR

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if your <sys/time.h> declares struct tm.  */
#undef TM_IN_SYS_TIME

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef uid_t

/* Define for Encore UMAX.  */
#undef UMAX

/* Result of "uname -a" */
#undef UNAME

/* Define for Encore UMAX 4.3 that has <inq_status/cpustats.h>
   instead of <sys/cpustats.h>.  */
#undef UMAX4_3

/* Define if you do not have <strings.h>, index, bzero, etc..  */
#undef USG

/* Define vfork as fork if vfork does not work.  */
#undef vfork

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#undef WORDS_BIGENDIAN

/* Define if the X Window System is missing or not being used.  */
#undef X_DISPLAY_MISSING

/* Define if lex declares yytext as a char * by default, not a char[].  */
#undef YYTEXT_POINTER


/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
