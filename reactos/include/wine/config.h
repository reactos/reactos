/* Wine configuration file for ReactOS */

#ifndef __REACTOS_WINE_CONFIG_H
#define __REACTOS_WINE_CONFIG_H

/* Define if using alloca.c.  */
#undef C_ALLOCA

/* Define to empty if the keyword does not work.  */
#undef const

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define if you have alloca, as a function or macro.  */
#undef HAVE_ALLOCA

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#undef HAVE_ALLOCA_H

/* Define as __inline if that's what the C compiler calls it.  */
#undef inline

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

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

/* Define if the X Window System is missing or not being used.  */
#undef X_DISPLAY_MISSING

/* The number of bytes in a long long.  */
#undef SIZEOF_LONG_LONG

/* Define if you have the __libc_fork function.  */
#undef HAVE___LIBC_FORK

/* Define if you have the _lwp_create function.  */
#undef HAVE__LWP_CREATE

/* Define if you have the clone function.  */
#undef HAVE_CLONE

/* Define if you have the connect function.  */
#undef HAVE_CONNECT

/* Define if you have the dlopen function.  */
#undef HAVE_DLOPEN

/* Define if you have the ecvt function.  */
#undef HAVE_ECVT

/* Define if you have the finite function.  */
#undef HAVE_FINITE

/* Define if you have the fpclass function.  */
#undef HAVE_FPCLASS

/* Define if you have the ftruncate64 function.  */
#undef HAVE_FTRUNCATE64

/* Define if you have the getbkgd function.  */
#undef HAVE_GETBKGD

/* Define if you have the gethostbyname function.  */
#undef HAVE_GETHOSTBYNAME

/* Define if you have the getnetbyaddr function.  */
#undef HAVE_GETNETBYADDR

/* Define if you have the getnetbyname function.  */
#undef HAVE_GETNETBYNAME

/* Define if you have the getpagesize function.  */
#undef HAVE_GETPAGESIZE

/* Define if you have the getprotobyname function.  */
#undef HAVE_GETPROTOBYNAME

/* Define if you have the getprotobynumber function.  */
#undef HAVE_GETPROTOBYNUMBER

/* Define if you have the getrlimit function.  */
#undef HAVE_GETRLIMIT

/* Define if you have the getservbyport function.  */
#undef HAVE_GETSERVBYPORT

/* Define if you have the getsockopt function.  */
#undef HAVE_GETSOCKOPT

/* Define if you have the inet_network function.  */
#undef HAVE_INET_NETWORK

/* Define if you have the iswalnum function.  */
#undef HAVE_ISWALNUM

/* Define if you have the lseek64 function.  */
#undef HAVE_LSEEK64

/* Define if you have the lstat function.  */
#undef HAVE_LSTAT

/* Define if you have the memmove function.  */
#undef HAVE_MEMMOVE

/* Define if you have the mmap function.  */
#undef HAVE_MMAP

/* Define if you have the openpty function.  */
#undef HAVE_OPENPTY

/* Define if you have the resizeterm function.  */
#undef HAVE_RESIZETERM

/* Define if you have the rfork function.  */
#undef HAVE_RFORK

/* Define if you have the select function.  */
#undef HAVE_SELECT

/* Define if you have the sendmsg function.  */
#undef HAVE_SENDMSG

/* Define if you have the settimeofday function.  */
#undef HAVE_SETTIMEOFDAY

/* Define if you have the sigaltstack function.  */
#undef HAVE_SIGALTSTACK

/* Define if you have the statfs function.  */
#undef HAVE_STATFS

/* Define if you have the strcasecmp function.  */
#undef HAVE_STRCASECMP

/* Define if you have the strerror function.  */
#define HAVE_STRERROR

/* Define if you have the strncasecmp function.  */
#undef HAVE_STRNCASECMP

/* Define if you have the tcgetattr function.  */
#undef HAVE_TCGETATTR

/* Define if you have the timegm function.  */
#undef HAVE_TIMEGM

/* Define if you have the usleep function.  */
#undef HAVE_USLEEP

/* Define if you have the vfscanf function.  */
#undef HAVE_VFSCANF

/* Define if you have the wait4 function.  */
#undef HAVE_WAIT4

/* Define if you have the waitpid function.  */
#undef HAVE_WAITPID

/* Define if you have the <GL/gl.h> header file.  */
#undef HAVE_GL_GL_H

/* Define if you have the <GL/glext.h> header file.  */
#undef HAVE_GL_GLEXT_H

/* Define if you have the <GL/glx.h> header file.  */
#undef HAVE_GL_GLX_H

/* Define if you have the <X11/XKBlib.h> header file.  */
#undef HAVE_X11_XKBLIB_H

/* Define if you have the <X11/Xlib.h> header file.  */
#undef HAVE_X11_XLIB_H

/* Define if you have the <X11/extensions/XShm.h> header file.  */
#undef HAVE_X11_EXTENSIONS_XSHM_H

/* Define if you have the <X11/extensions/Xrender.h> header file.  */
#undef HAVE_X11_EXTENSIONS_XRENDER_H

/* Define if you have the <X11/extensions/Xvlib.h> header file.  */
#undef HAVE_X11_EXTENSIONS_XVLIB_H

/* Define if you have the <X11/extensions/shape.h> header file.  */
#undef HAVE_X11_EXTENSIONS_SHAPE_H

/* Define if you have the <X11/extensions/xf86dga.h> header file.  */
#undef HAVE_X11_EXTENSIONS_XF86DGA_H

/* Define if you have the <X11/extensions/xf86vmode.h> header file.  */
#undef HAVE_X11_EXTENSIONS_XF86VMODE_H

/* Define if you have the <X11/xpm.h> header file.  */
#undef HAVE_X11_XPM_H

/* Define if you have the <arpa/inet.h> header file.  */
#undef HAVE_ARPA_INET_H

/* Define if you have the <arpa/nameser.h> header file.  */
#undef HAVE_ARPA_NAMESER_H

/* Define if you have the <curses.h> header file.  */
#undef HAVE_CURSES_H

/* Define if you have the <dlfcn.h> header file.  */
#undef HAVE_DLFCN_H

/* Define if you have the <elf.h> header file.  */
#undef HAVE_ELF_H

/* Define if you have the <float.h> header file.  */
#define HAVE_FLOAT_H

/* Define if you have the <freetype/freetype.h> header file.  */
#undef HAVE_FREETYPE_FREETYPE_H

/* Define if you have the <freetype/ftglyph.h> header file.  */
#undef HAVE_FREETYPE_FTGLYPH_H

/* Define if you have the <freetype/ftnames.h> header file.  */
#undef HAVE_FREETYPE_FTNAMES_H

/* Define if you have the <freetype/ftoutln.h> header file.  */
#undef HAVE_FREETYPE_FTOUTLN_H

/* Define if you have the <freetype/ftsnames.h> header file.  */
#undef HAVE_FREETYPE_FTSNAMES_H

/* Define if you have the <freetype/ttnameid.h> header file.  */
#undef HAVE_FREETYPE_TTNAMEID_H

/* Define if you have the <freetype/tttables.h> header file.  */
#undef HAVE_FREETYPE_TTTABLES_H

/* Define if you have the <ieeefp.h> header file.  */
#undef HAVE_IEEEFP_H

/* Define if you have the <jpeglib.h> header file.  */
#undef HAVE_JPEGLIB_H

/* Define if you have the <libio.h> header file.  */
#undef HAVE_LIBIO_H

/* Define if you have the <libutil.h> header file.  */
#undef HAVE_LIBUTIL_H

/* Define if you have the <link.h> header file.  */
#undef HAVE_LINK_H

/* Define if you have the <linux/cdrom.h> header file.  */
#undef HAVE_LINUX_CDROM_H

/* Define if you have the <linux/input.h> header file.  */
#undef HAVE_LINUX_INPUT_H

/* Define if you have the <linux/joystick.h> header file.  */
#undef HAVE_LINUX_JOYSTICK_H

/* Define if you have the <linux/ucdrom.h> header file.  */
#undef HAVE_LINUX_UCDROM_H

/* Define if you have the <machine/soundcard.h> header file.  */
#undef HAVE_MACHINE_SOUNDCARD_H

/* Define if you have the <ncurses.h> header file.  */
#undef HAVE_NCURSES_H

/* Define if you have the <net/if.h> header file.  */
#undef HAVE_NET_IF_H

/* Define if you have the <netdb.h> header file.  */
#undef HAVE_NETDB_H

/* Define if you have the <netinet/in.h> header file.  */
#undef HAVE_NETINET_IN_H

/* Define if you have the <netinet/in_systm.h> header file.  */
#undef HAVE_NETINET_IN_SYSTM_H

/* Define if you have the <netinet/ip.h> header file.  */
#undef HAVE_NETINET_IP_H

/* Define if you have the <netinet/tcp.h> header file.  */
#undef HAVE_NETINET_TCP_H

/* Define if you have the <pty.h> header file.  */
#undef HAVE_PTY_H

/* Define if you have the <resolv.h> header file.  */
#undef HAVE_RESOLV_H

/* Define if you have the <sched.h> header file.  */
#undef HAVE_SCHED_H

/* Define if you have the <socket.h> header file.  */
#undef HAVE_SOCKET_H

/* Define if you have the <soundcard.h> header file.  */
#undef HAVE_SOUNDCARD_H

/* Define if you have the <strings.h> header file.  */
#undef HAVE_STRINGS_H

/* Define if you have the <sys/cdio.h> header file.  */
#undef HAVE_SYS_CDIO_H

/* Define if you have the <sys/errno.h> header file.  */
#undef HAVE_SYS_ERRNO_H

/* Define if you have the <sys/file.h> header file.  */
#undef HAVE_SYS_FILE_H

/* Define if you have the <sys/filio.h> header file.  */
#undef HAVE_SYS_FILIO_H

/* Define if you have the <sys/ipc.h> header file.  */
#undef HAVE_SYS_IPC_H

/* Define if you have the <sys/link.h> header file.  */
#undef HAVE_SYS_LINK_H

/* Define if you have the <sys/lwp.h> header file.  */
#undef HAVE_SYS_LWP_H

/* Define if you have the <sys/mman.h> header file.  */
#undef HAVE_SYS_MMAN_H

/* Define if you have the <sys/modem.h> header file.  */
#undef HAVE_SYS_MODEM_H

/* Define if you have the <sys/mount.h> header file.  */
#undef HAVE_SYS_MOUNT_H

/* Define if you have the <sys/msg.h> header file.  */
#undef HAVE_SYS_MSG_H

/* Define if you have the <sys/param.h> header file.  */
#undef HAVE_SYS_PARAM_H

/* Define if you have the <sys/ptrace.h> header file.  */
#undef HAVE_SYS_PTRACE_H

/* Define if you have the <sys/reg.h> header file.  */
#undef HAVE_SYS_REG_H

/* Define if you have the <sys/shm.h> header file.  */
#undef HAVE_SYS_SHM_H

/* Define if you have the <sys/signal.h> header file.  */
#undef HAVE_SYS_SIGNAL_H

/* Define if you have the <sys/socket.h> header file.  */
#undef HAVE_SYS_SOCKET_H

/* Define if you have the <sys/sockio.h> header file.  */
#undef HAVE_SYS_SOCKIO_H

/* Define if you have the <sys/soundcard.h> header file.  */
#undef HAVE_SYS_SOUNDCARD_H

/* Define if you have the <sys/statfs.h> header file.  */
#undef HAVE_SYS_STATFS_H

/* Define if you have the <sys/strtio.h> header file.  */
#undef HAVE_SYS_STRTIO_H

/* Define if you have the <sys/syscall.h> header file.  */
#undef HAVE_SYS_SYSCALL_H

/* Define if you have the <sys/user.h> header file.  */
#undef HAVE_SYS_USER_H

/* Define if you have the <sys/v86.h> header file.  */
#undef HAVE_SYS_V86_H

/* Define if you have the <sys/v86intr.h> header file.  */
#undef HAVE_SYS_V86INTR_H

/* Define if you have the <sys/vfs.h> header file.  */
#undef HAVE_SYS_VFS_H

/* Define if you have the <sys/vm86.h> header file.  */
#undef HAVE_SYS_VM86_H

/* Define if you have the <sys/wait.h> header file.  */
#undef HAVE_SYS_WAIT_H

/* Define if you have the <syscall.h> header file.  */
#undef HAVE_SYSCALL_H

/* Define if you have the <ucontext.h> header file.  */
#undef HAVE_UCONTEXT_H

/* Define if you have the curses library (-lcurses).  */
#undef HAVE_LIBCURSES

/* Define if you have the i386 library (-li386).  */
#undef HAVE_LIBI386

/* Define if you have the m library (-lm).  */
#undef HAVE_LIBM

/* Define if you have the mmap library (-lmmap).  */
#undef HAVE_LIBMMAP

/* Define if you have the ncurses library (-lncurses).  */
#undef HAVE_LIBNCURSES

/* Define if you have the ossaudio library (-lossaudio).  */
#undef HAVE_LIBOSSAUDIO

/* Define if you have the xpg4 library (-lxpg4).  */
#undef HAVE_LIBXPG4

/* Define if all debug messages are to be compiled out */
#undef NO_DEBUG_MSGS

/* Define if TRACE messages are to be compiled out */
#undef NO_TRACE_MSGS

/* Define if you have libjpeg including devel headers */
#undef HAVE_LIBJPEG

/* Define if you have the Xpm library */
#undef HAVE_LIBXXPM

/* Define if you have the XKB extension */
#undef HAVE_XKB

/* Define if you have the X Shm extension */
#undef HAVE_LIBXXSHM

/* Define if you have the X Shape extension */
#undef HAVE_LIBXSHAPE

/* Define if you have the Xxf86dga library version 2 */
#undef HAVE_LIBXXF86DGA2

/* Define if you have the Xxf86dga library version 1 */
#undef HAVE_LIBXXF86DGA

/* Define if you have the Xxf86vm library */
#undef HAVE_LIBXXF86VM

/* Define if the X libraries support XVideo */
#undef HAVE_XVIDEO

/* Define if you have the XRender extension library */
#undef HAVE_LIBXRENDER

/* Define if OpenGL is present on the system */
#undef HAVE_OPENGL

/* Define if the OpenGL library supports the glXGetProcAddressARB call */
#undef HAVE_GLX_GETPROCADDRESS

/* Define if the OpenGL headers define extension typedefs */
#undef HAVE_GLEXT_PROTOTYPES

/* Define if we have CUPS */
#undef HAVE_CUPS

/* Define if FreeType 2 is installed */
#undef HAVE_FREETYPE

/* Define if we can use ppdev.h for parallel port access */
#undef HAVE_PPDEV

/* Define if IPX should use netipx/ipx.h from libc */
#undef HAVE_IPX_GNU

/* Define if IPX includes are taken from Linux kernel */
#undef HAVE_IPX_LINUX

/* Define if you have the Open Sound system */
#undef HAVE_OSS

/* Define if you have the Open Sound system (MIDI interface) */
#undef HAVE_OSS_MIDI

/* Set this to 64 to enable 64-bit file support on Linux */
#undef _FILE_OFFSET_BITS

/* Define if .type asm directive must be inside a .def directive */
#undef NEED_TYPE_IN_DEF

/* Define if symbols declared in assembly code need an underscore prefix */
#undef NEED_UNDERSCORE_PREFIX

/* Define to use .string instead of .ascii */
#undef HAVE_ASM_STRING

/* Define to the name of the function returning errno for reentrant libc */
#undef ERRNO_LOCATION

/* Define if X libraries are not reentrant (compiled without -D_REENTRANT) */
#undef NO_REENTRANT_X11

/* Define if we have linux/input.h AND it contains the INPUT event API */
#undef HAVE_CORRECT_LINUXINPUT_H

/* Define if Linux-style gethostbyname_r and gethostbyaddr_r are available */
#undef HAVE_LINUX_GETHOSTBYNAME_R_6

/* Define if <linux/joystick.h> defines the Linux 2.2 joystick API */
#undef HAVE_LINUX_22_JOYSTICK_API

/* Define if the struct statfs is defined by <sys/vfs.h> */
#undef STATFS_DEFINED_BY_SYS_VFS

/* Define if the struct statfs is defined by <sys/statfs.h> */
#undef STATFS_DEFINED_BY_SYS_STATFS

/* Define if the struct statfs is defined by <sys/mount.h> */
#undef STATFS_DEFINED_BY_SYS_MOUNT

/* Define if the struct statfs has the member bfree */
#undef STATFS_HAS_BFREE

/* Define if the struct statfs has the member bavail */
#undef STATFS_HAS_BAVAIL

/* Define if struct msghdr contains msg_accrights */
#undef HAVE_MSGHDR_ACCRIGHTS

/* Define if struct sockaddr contains sa_len */
#undef HAVE_SOCKADDR_SA_LEN

/* Define if struct sockaddr_un contains sun_len */
#undef HAVE_SOCKADDR_SUN_LEN

#endif  /* __REACTOS_WINE_CONFIG_H */
