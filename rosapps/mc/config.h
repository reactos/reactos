/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
/* This is the configuration file for the Midnight Commander. It was generated
   by autoconf's configure.
   
   Configure for Midnight Commander
   Copyright (C) 1994, 1995 Janne Kukonlehto
   Copyright (C) 1994, 1995 Miguel de Icaza
   Copyright (C) 1995 Jakub Jelinek
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
   
#include <VERSION>



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

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if you have the getmntent function.  */
#define HAVE_GETMNTENT 1

/* Define if you have a working `mmap' system call.  */
/* #undef HAVE_MMAP */

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if your struct stat has st_rdev.  */
#define HAVE_ST_RDEV 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
/* #undef MAJOR_IN_MKDEV */

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
#define MAJOR_IN_SYSMACROS 1

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

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

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if the X Window System is missing or not being used.  */
/* #undef X_DISPLAY_MISSING */

#define PACKAGE "mc"

/* Always defined */
#define D_INO_IN_DIRENT 1
/* #undef IS_AIX */
/* #undef MOUNTED_FREAD */
/* #undef MOUNTED_FREAD_FSTYP */
/* #undef MOUNTED_GETFSSTAT */
/* #undef MOUNTED_GETMNT */
#define MOUNTED_GETMNTENT1 1
/* #undef MOUNTED_GETMNTENT2 */
/* #undef MOUNTED_GETMNTINFO */
/* #undef MOUNTED_VMOUNT */
#define STAT_STATFS2_BSIZE 1
/* #undef STAT_STATFS2_FSIZE */
/* #undef STAT_STATFS2_FS_DATA */
/* #undef STAT_STATFS3_OSF1 */
/* #undef STAT_STATFS4 */
/* #undef STAT_STATVFS */

/* Define umode_t if your system does not provide it */
#define umode_t int

/* Define nlink_t if your system does not provide it */
/* #undef nlink_t */

/* Does the file command accepts the -L option */
#define FILE_L 1

/* Does the file command work well with - option for stdin? */
#define FILE_STDIN 1

/* Does the grep command work well with - option for stdin? */
#define GREP_STDIN 1

/* Is the program using the GPM library? */
/* #undef HAVE_LIBGPM */

/* Is the program using the distributed slang library? */
#define HAVE_SLANG 1

/* Is the program using a system-installed slang library? */
/* #undef HAVE_SYSTEM_SLANG */

/* Define if the slang.h header file is inside a directory slang
** in the standard directories
*/
/* #undef SLANG_H_INSIDE_SLANG_DIR */

/* Does the program have subshell support? */
#define HAVE_SUBSHELL_SUPPORT 1

/* If you don't have gcc, define this */
/* #undef OLD_TOOLS */ 

/* Is the subshell the default or optional? */
/* #undef SUBSHELL_OPTIONAL */

/* Use SunOS SysV curses? */
/* #undef SUNOS_CURSES */

/* Use SystemV curses? */
/* #undef USE_SYSV_CURSES */

/* Use Ncurses? */
/* #undef USE_NCURSES */

/* If you Curses does not have color define this one */
/* #undef NO_COLOR_SUPPORT */

/* Support the Midnight Commander Virtual File System? */
#define USE_VFS 1

/* Support for the Memory Allocation Debugger */
/* #undef HAVE_MAD */

/* Extra Debugging */
/* #undef MCDEBUG */

/* If the Slang library will be using it's own terminfo instead of termcap */
#define SLANG_TERMINFO 1

/* If Slang library should use termcap */
/* #undef USE_TERMCAP */

/* If you have socket and the rest of the net functions use this */
#define USE_NETCODE 1

/* If defined, use .netrc for FTP connections */
/* #undef USE_NETRC */ 

/* If your operating system does not have enough space for a file name
 * in a struct dirent, then define this
 */
/* #undef NEED_EXTRA_DIRENT_BUFFER */

/* Define if you want the du -s summary */
#define HAVE_DUSUM 1

/* Define if your du does handle -b correctly */
#define DUSUM_USEB 1

/* Define to size of chunks du is displaying its information.
 * If DUSUM_USEB is defined, this should be 1
 */
#define DUSUM_FACTOR 1

/* Define this one if you want termnet support */
/* #undef USE_TERMNET */

/* Defined if you have libXpm, <X11/xpm.h>, libXext, <X11/extensions/shape.h> */
/* #undef HAVE_XPM_SHAPE */

/* Defined if you have shadow passwords on Linux */
/* #undef LINUX_SHADOW */

/* Defined if you have the crypt prototype in neither unistd.h nor crypt.h */
#define NEED_CRYPT_PROTOTYPE 1

/* Define if you want to turn on SCO-specific code */
/* #undef SCO_FLAVOR */

/* Define if your system has struct linger */
#define HAVE_STRUCT_LINGER 1

/* Define if your curses has this one (AIX, OSF/1) */
/* #undef USE_SETUPTERM */

/* Link in ext2fs code for delfs experimental file system */
/* #undef USE_EXT2FSLIB */

/* Define if you want to use the HSC firewall */
/* #undef HSC_PROXY */

/* Define if your system uses PAM for auth stuff */
/* #undef HAVE_PAM */

/* Define if you have the get_process_stats function and have to use that instead of gettimeofday  */
/* #undef HAVE_GET_PROCESS_STATS */

/* Define if you want to call the internal routine edit() for the editor */
#define USE_INTERNAL_EDIT 1

/* Define if your system has socketpair */
#define HAVE_SOCKETPAIR 1

/* Version of ncurses */
/* #undef NCURSES_970530 */

/* #undef HAVE_STPCPY */

#define ENABLE_NLS 1
/* #undef HAVE_CATGETS */
/* #undef HAVE_GETTEXT */
#define HAVE_LC_MESSAGES 1

/* Define if you have the __argz_count function.  */
/* #undef HAVE___ARGZ_COUNT */

/* Define if you have the __argz_next function.  */
/* #undef HAVE___ARGZ_NEXT */

/* Define if you have the __argz_stringify function.  */
/* #undef HAVE___ARGZ_STRINGIFY */

/* Define if you have the cfgetospeed function.  */
#define HAVE_CFGETOSPEED 1

/* Define if you have the crypt function.  */
/* #undef HAVE_CRYPT */

/* Define if you have the dcgettext function.  */
/* #undef HAVE_DCGETTEXT */

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getmntinfo function.  */
/* #undef HAVE_GETMNTINFO */

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the getwd function.  */
#define HAVE_GETWD 1

/* Define if you have the grantpt function.  */
#define HAVE_GRANTPT 1

/* Define if you have the initgroups function.  */
#define HAVE_INITGROUPS 1

/* Define if you have the keyok function.  */
/* #undef HAVE_KEYOK */

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the munmap function.  */
#define HAVE_MUNMAP 1

/* Define if you have the pmap_getmaps function.  */
/* #undef HAVE_PMAP_GETMAPS */

/* Define if you have the pmap_getport function.  */
/* #undef HAVE_PMAP_GETPORT */

/* Define if you have the pmap_set function.  */
/* #undef HAVE_PMAP_SET */

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the pwdauth function.  */
/* #undef HAVE_PWDAUTH */

/* Define if you have the resizeterm function.  */
/* #undef HAVE_RESIZETERM */

/* Define if you have the rresvport function.  */
#define HAVE_RRESVPORT 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the sigaction function.  */
#define HAVE_SIGACTION 1

/* Define if you have the sigaddset function.  */
#define HAVE_SIGADDSET 1

/* Define if you have the sigemptyset function.  */
#define HAVE_SIGEMPTYSET 1

/* Define if you have the sigprocmask function.  */
#define HAVE_SIGPROCMASK 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the socketpair function.  */
#define HAVE_SOCKETPAIR 1

/* Define if you have the statfs function.  */
#define HAVE_STATFS 1

/* Define if you have the statlstat function.  */
/* #undef HAVE_STATLSTAT */

/* Define if you have the statvfs function.  */
/* #undef HAVE_STATVFS */

/* Define if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strncasecmp function.  */
#define HAVE_STRNCASECMP 1

/* Define if you have the sysconf function.  */
#define HAVE_SYSCONF 1

/* Define if you have the tcgetattr function.  */
#define HAVE_TCGETATTR 1

/* Define if you have the tcsetattr function.  */
#define HAVE_TCSETATTR 1

/* Define if you have the truncate function.  */
#define HAVE_TRUNCATE 1

/* Define if you have the <argz.h> header file.  */
/* #undef HAVE_ARGZ_H */

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <ext2fs/ext2fs.h> header file.  */
/* #undef HAVE_EXT2FS_EXT2FS_H */

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <grp.h> header file.  */
#define HAVE_GRP_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <linux/ext2_fs.h> header file.  */
/* #undef HAVE_LINUX_EXT2_FS_H */

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <mntent.h> header file.  */
#define HAVE_MNTENT_H 1

/* Define if you have the <mnttab.h> header file.  */
/* #undef HAVE_MNTTAB_H */

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <nl_types.h> header file.  */
/* #undef HAVE_NL_TYPES_H */

/* Define if you have the <rpc/pmap_clnt.h> header file.  */
/* #undef HAVE_RPC_PMAP_CLNT_H */

/* Define if you have the <shadow.h> header file.  */
/* #undef HAVE_SHADOW_H */

/* Define if you have the <shadow/shadow.h> header file.  */
/* #undef HAVE_SHADOW_SHADOW_H */

/* Define if you have the <slang.h> header file.  */
/* #undef HAVE_SLANG_H */

/* Define if you have the <slang/slang.h> header file.  */
/* #undef HAVE_SLANG_SLANG_H */

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/dustat.h> header file.  */
/* #undef HAVE_SYS_DUSTAT_H */

/* Define if you have the <sys/filsys.h> header file.  */
/* #undef HAVE_SYS_FILSYS_H */

/* Define if you have the <sys/fs_types.h> header file.  */
/* #undef HAVE_SYS_FS_TYPES_H */

/* Define if you have the <sys/fstyp.h> header file.  */
/* #undef HAVE_SYS_FSTYP_H */

/* Define if you have the <sys/mount.h> header file.  */
#define HAVE_SYS_MOUNT_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/statfs.h> header file.  */
/* #undef HAVE_SYS_STATFS_H */

/* Define if you have the <sys/statvfs.h> header file.  */
/* #undef HAVE_SYS_STATVFS_H */

/* Define if you have the <sys/vfs.h> header file.  */
#define HAVE_SYS_VFS_H 1

/* Define if you have the <termios.h> header file.  */
#define HAVE_TERMIOS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <utime.h> header file.  */
#define HAVE_UTIME_H 1

/* Define if you have the <values.h> header file.  */
/* #undef HAVE_VALUES_H */

/* Define if you have the i library (-li).  */
/* #undef HAVE_LIBI */

/* Define if you have the intl library (-lintl).  */
#define HAVE_LIBINTL 1

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the pt library (-lpt).  */
/* #undef HAVE_LIBPT */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

#ifdef HAVE_LIBPT
#    define HAVE_GRANTPT
#endif

#if defined(HAVE_LIBCRYPT) || defined(HAVE_LIBCRYPT_I)
#    define HAVE_CRYPT
#endif

#ifdef HAVE_XVIEW
#    include <xvmain.h>
#endif

#if defined(HAVE_SIGADDSET) && defined(HAVE_SIGEMPTYSET)
# if defined(HAVE_SIGACTION) && defined(HAVE_SIGPROCMASK)
#  define SLANG_POSIX_SIGNALS
# endif
#endif

#ifdef __os2__
#    define OS2_NT 1
#    define S_ISFIFO(x) 0
#endif

#ifdef _OS_NT
#    define OS2_NT 1
#endif

#ifndef OS2_NT
/* some Unices do not define this, and slang requires it: */
#ifndef unix
#    define unix
#endif
#endif

