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


@TOP@

#undef PACKAGE

/* Always defined */
#undef D_INO_IN_DIRENT
#undef IS_AIX
#undef MOUNTED_FREAD
#undef MOUNTED_FREAD_FSTYP
#undef MOUNTED_GETFSSTAT
#undef MOUNTED_GETMNT
#undef MOUNTED_GETMNTENT1
#undef MOUNTED_GETMNTENT2
#undef MOUNTED_GETMNTINFO
#undef MOUNTED_VMOUNT
#undef STAT_STATFS2_BSIZE
#undef STAT_STATFS2_FSIZE
#undef STAT_STATFS2_FS_DATA
#undef STAT_STATFS3_OSF1
#undef STAT_STATFS4
#undef STAT_STATVFS

/* Define umode_t if your system does not provide it */
#undef umode_t

/* Define nlink_t if your system does not provide it */
#undef nlink_t

/* Does the file command accepts the -L option */
#undef FILE_L

/* Does the file command work well with - option for stdin? */
#undef FILE_STDIN

/* Does the grep command work well with - option for stdin? */
#undef GREP_STDIN

/* Is the program using the GPM library? */
#undef HAVE_LIBGPM

/* Is the program using the distributed slang library? */
#undef HAVE_SLANG

/* Is the program using a system-installed slang library? */
#undef HAVE_SYSTEM_SLANG

/* Define if the slang.h header file is inside a directory slang
** in the standard directories
*/
#undef SLANG_H_INSIDE_SLANG_DIR

/* Does the program have subshell support? */
#undef HAVE_SUBSHELL_SUPPORT

/* If you don't have gcc, define this */
#undef OLD_TOOLS 

/* Are you using other type of curses? */
#undef OTHER_CURSES

/* Is the subshell the default or optional? */
#undef SUBSHELL_OPTIONAL

/* Use SunOS SysV curses? */
#undef SUNOS_CURSES

/* Use old BSD curses? */
#undef USE_BSD_CURSES

/* Use SystemV curses? */
#undef USE_SYSV_CURSES

/* Use Ncurses? */
#undef USE_NCURSES

/* If you Curses does not have color define this one */
#undef NO_COLOR_SUPPORT

/* Support the Midnight Commander Virtual File System? */
#undef USE_VFS

/* Support for the Memory Allocation Debugger */
#undef HAVE_MAD

/* Extra Debugging */
#undef MCDEBUG

/* If the Slang library will be using it's own terminfo instead of termcap */
#undef SLANG_TERMINFO

/* If Slang library should use termcap */
#undef USE_TERMCAP

/* If you have socket and the rest of the net functions use this */
#undef USE_NETCODE

/* If defined, use .netrc for FTP connections */
#undef USE_NETRC 

/* If your operating system does not have enough space for a file name
 * in a struct dirent, then define this
 */
#undef NEED_EXTRA_DIRENT_BUFFER

/* Define if you want the du -s summary */
#undef HAVE_DUSUM

/* Define if your du does handle -b correctly */
#undef DUSUM_USEB

/* Define to size of chunks du is displaying its information.
 * If DUSUM_USEB is defined, this should be 1
 */
#define DUSUM_FACTOR 512

/* Define this one if you want termnet support */
#undef USE_TERMNET

/* Defined if you have the file command */
#undef HAVE_FILECMD

/* Defined if you have libXpm, <X11/xpm.h>, libXext, <X11/extensions/shape.h> */
#undef HAVE_XPM_SHAPE

/* Defined if you have shadow passwords on Linux */
#undef LINUX_SHADOW

/* Defined if you have the crypt prototype in neither unistd.h nor crypt.h */
#undef NEED_CRYPT_PROTOTYPE

/* Defined if your CPP understands ## macro token pasting method */
#undef HAVE_PORTABLE_TOKEN_PASTING

/* Define if you want to turn on SCO-specific code */
#undef SCO_FLAVOR

/* Define if your system has struct linger */
#undef HAVE_STRUCT_LINGER

/* Define if your curses has this one (AIX, OSF/1) */
#undef USE_SETUPTERM

/* Link in ext2fs code for delfs experimental file system */
#undef USE_EXT2FSLIB

/* Define if you have putenv routine */
#undef HAVE_PUTENV

/* Define if you have isascii */
#undef HAVE_ISASCII

/* Define if you want to use the HSC firewall */
#undef HSC_PROXY

/* Define if your system uses PAM for auth stuff */
#undef HAVE_PAM

/* Define if you have the pmap_getmaps function */
#undef HAVE_PMAP_GETMAPS

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the get_process_stats function and have to use that instead of gettimeofday  */
#undef HAVE_GET_PROCESS_STATS

/* Define if you want to call the internal routine edit() for the editor */
#undef USE_INTERNAL_EDIT

/* Define if your system has socketpair */
#undef HAVE_SOCKETPAIR

/* Do we have posix signals? */
#undef HAVE_SIGACTION
#undef HAVE_SIGPROCMASK
#undef HAVE_SIGEMPTYSET
#undef HAVE_SIGADDSET

/* Version of ncurses */
#undef NCURSES_970530

#undef HAVE_STPCPY

#undef ENABLE_NLS
#undef HAVE_CATGETS
#undef HAVE_GETTEXT
#undef HAVE_LC_MESSAGES

@BOTTOM@

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

