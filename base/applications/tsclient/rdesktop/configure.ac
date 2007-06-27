AC_INIT(rdesktop, 1.4.1)

AC_CONFIG_SRCDIR([rdesktop.c])

AC_PROG_CC
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall"
fi

AC_PROG_INSTALL
AC_LANG_C
AC_HEADER_STDC
AC_C_BIGENDIAN([AC_DEFINE(B_ENDIAN)], [AC_DEFINE(L_ENDIAN)])
AC_PATH_XTRA

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(inet_aton, resolv)

AC_CHECK_HEADER(sys/select.h, AC_DEFINE(HAVE_SYS_SELECT_H))
AC_CHECK_HEADER(sys/modem.h, AC_DEFINE(HAVE_SYS_MODEM_H))
AC_CHECK_HEADER(sys/filio.h, AC_DEFINE(HAVE_SYS_FILIO_H))
AC_CHECK_HEADER(sys/strtio.h, AC_DEFINE(HAVE_SYS_STRTIO_H))
AC_CHECK_HEADER(locale.h, AC_DEFINE(HAVE_LOCALE_H))
AC_CHECK_HEADER(langinfo.h, AC_DEFINE(HAVE_LANGINFO_H))

AC_CHECK_TOOL(STRIP, strip, :)

rpath=""

#
# OpenSSL detection borrowed from stunnel
#
checkssldir() { :
    if test -f "$1/include/openssl/ssl.h"; then
        ssldir="$1"
        return 0
    fi
    return 1
}
AC_MSG_CHECKING([for OpenSSL directory])
AC_ARG_WITH(openssl,
    [  --with-openssl=DIR          look for OpenSSL at DIR/include, DIR/lib],
    [
        dnl Check the specified location only
        checkssldir "$withval"
    ],
    [
        dnl Search default locations of OpenSSL library
        for maindir in /usr/local /usr/lib /usr/pkg /usr /var/ssl /opt; do
            for dir in $maindir $maindir/openssl $maindir/ssl; do
                checkssldir $dir && break 2
            done
        done
    ]
)
if test -z "$ssldir"; then
    AC_MSG_RESULT([Not found])
    echo
    echo "Couldn't find your OpenSSL library installation dir"
    echo "Use --with-openssl option to fix this problem"
    echo
    exit 1
fi
AC_MSG_RESULT([$ssldir])
AC_SUBST(ssldir)
AC_DEFINE_UNQUOTED(ssldir, "$ssldir")

dnl Add OpenSSL includes and libraries
CFLAGS="$CFLAGS -I$ssldir/include"
AC_ARG_ENABLE(static-openssl, 
             [  --enable-static-openssl link OpenSSL statically],
             [ 
LIBS="$LIBS $ssldir/lib/libcrypto.a" 
             ],
             [ 
LIBS="$LIBS -L$ssldir/lib -lcrypto" 
rpath="$rpath:$ssldir/lib"
             ])


#
# Alignment
#
AC_MSG_CHECKING([if architecture needs alignment])
AC_TRY_RUN([
#include <stdlib.h>
#include <signal.h>
int main(int argc, char **argv)
{
	unsigned char test[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
	signal(SIGBUS, exit);
	signal(SIGABRT, exit);
	signal(SIGSEGV, exit);
	if (*((unsigned int *)(test + 1)) != 0x55443322 && *((unsigned int *)(test + 1)) != 0x22334455) {
		return 1;
	}
	return 0;
}], 
 [AC_MSG_RESULT(no)], 
 [AC_MSG_RESULT(yes) 
  AC_DEFINE(NEED_ALIGN)], 
 [AC_MSG_RESULT(assuming yes) 
  AC_DEFINE(NEED_ALIGN)])


#
# EGD
#
AC_ARG_WITH(egd-socket,
    [  --with-egd-socket=PATH  look for Entropy Gathering Daemon socket at PATH],
    [EGD_SOCKET="$withval"],
    [EGD_SOCKET="/var/run/egd-pool"]
)
AC_DEFINE_UNQUOTED(EGD_SOCKET, "$EGD_SOCKET")


#
# rdp2vnc
#
vncserverconfig=libvncserver-config
AC_ARG_WITH(libvncserver-config,
    [  --with-libvncserver-config=CMD  use CMD as libvncserver-config],
    [vncserverconfig="$withval"]
)
AC_ARG_WITH(libvncserver,
    [  --with-libvncserver     make rdp2vnc],
    [ 
    VNCINC=`$vncserverconfig --cflags`
    AC_SUBST(VNCINC)
    LDVNC=`$vncserverconfig --libs`
    AC_SUBST(LDVNC)
    VNCLINK=`$vncserverconfig --link`
    AC_SUBST(VNCLINK)
    RDP2VNCTARGET="rdp2vnc"
    AC_SUBST(RDP2VNCTARGET)
    ]
)

#
# sound
#
AC_ARG_WITH(libao,
    [  --with-libao=DIR        look for libao at DIR/include, DIR/lib],
    [
    CFLAGS="$CFLAGS -I$withval/include"
    CPPFLAGS="$CPPFLAGS -I$withval/include"
    LIBS="$LIBS -L$withval/lib"
    rpath="$rpath:$withval/lib"
    ]
)

sound="yes"
AC_ARG_WITH(sound,
    [  --with-sound            select sound system ("oss", "sgi", "sun" or "libao") ],
    [ 
    sound="$withval" 
    ])
if test "$sound" = yes; then
    AC_CHECK_HEADER(ao/ao.h, [sound=libao])
    AC_CHECK_HEADER(sys/soundcard.h, [sound=oss])
    AC_CHECK_HEADER(dmedia/audio.h, [sound=sgi])
    AC_CHECK_HEADER(sys/audioio.h, [sound=sun])
fi
if test "$sound" = no; then
    break
elif test "$sound" = oss; then
    SOUNDOBJ="rdpsnd.o rdpsnd_oss.o"
    AC_DEFINE(WITH_RDPSND)
elif test "$sound" = sgi; then
    SOUNDOBJ="rdpsnd.o rdpsnd_sgi.o"
    LDFLAGS="$LDFLAGS -laudio"
    AC_DEFINE(WITH_RDPSND)
elif test "$sound" = sun; then
    SOUNDOBJ="rdpsnd.o rdpsnd_sun.o"
    AC_DEFINE(WITH_RDPSND)
elif test "$sound" = libao; then
    SOUNDOBJ="rdpsnd.o rdpsnd_libao.o"
    LDFLAGS="$LDFLAGS -lao"
    AC_DEFINE(WITH_RDPSND)
else
   AC_MSG_WARN([sound support disabled])
   AC_MSG_WARN([Currently supported systems are Open Sound System (oss), SGI AL (sgi), Sun/BSD (sun) and libao])
fi
AC_SUBST(SOUNDOBJ)

#
# dirfd
#
dnl Find out how to get the file descriptor associated with an open DIR*.
dnl From Jim Meyering

AC_DEFUN([UTILS_FUNC_DIRFD],
[

  AC_HEADER_DIRENT
  dirfd_headers='
#if HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# if HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
'
  AC_CHECK_FUNCS(dirfd)
  AC_CHECK_DECLS([dirfd], , , $dirfd_headers)

  AC_CACHE_CHECK([whether dirfd is a macro],
    jm_cv_func_dirfd_macro,
    [AC_EGREP_CPP([dirent_header_defines_dirfd], [$dirfd_headers
#ifdef dirfd
 dirent_header_defines_dirfd
#endif],
       jm_cv_func_dirfd_macro=yes,
       jm_cv_func_dirfd_macro=no)])

  # Use the replacement only if we have no function, macro,
  # or declaration with that name.
  if test $ac_cv_func_dirfd,$ac_cv_have_decl_dirfd,$jm_cv_func_dirfd_macro \
      = no,no,no; then
    AC_REPLACE_FUNCS([dirfd])
    AC_CACHE_CHECK(
	      [how to get the file descriptor associated with an open DIR*],
		   gl_cv_sys_dir_fd_member_name,
      [
        dirfd_save_CFLAGS=$CFLAGS
	for ac_expr in d_fd dd_fd; do

	  CFLAGS="$CFLAGS -DDIR_FD_MEMBER_NAME=$ac_expr"
	  AC_TRY_COMPILE(
	    [$dirfd_headers
	    ],
	    [DIR *dir_p = opendir("."); (void) dir_p->DIR_FD_MEMBER_NAME;],
	    dir_fd_found=yes
	  )
	  CFLAGS=$dirfd_save_CFLAGS
	  test "$dir_fd_found" = yes && break
	done
	test "$dir_fd_found" = yes || ac_expr=no_such_member

	gl_cv_sys_dir_fd_member_name=$ac_expr
      ]
    )
    if test $gl_cv_sys_dir_fd_member_name != no_such_member; then
      AC_DEFINE_UNQUOTED(DIR_FD_MEMBER_NAME,
	$gl_cv_sys_dir_fd_member_name,
	[the name of the file descriptor member of DIR])
    fi
    AH_VERBATIM(DIR_TO_FD,
		[#ifdef DIR_FD_MEMBER_NAME
# define DIR_TO_FD(Dir_p) ((Dir_p)->DIR_FD_MEMBER_NAME)
#else
# define DIR_TO_FD(Dir_p) -1
#endif
]
    )
  fi
])

UTILS_FUNC_DIRFD

#
# iconv
#

dnl This macros shamelessly stolen from
dnl http://gcc.gnu.org/ml/gcc-bugs/2001-06/msg01398.html.
dnl Written by Bruno Haible.

AC_DEFUN([UTILS_FUNC_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])
  AC_CHECK_HEADER(iconv.h, AC_DEFINE(HAVE_ICONV_H))

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])

UTILS_FUNC_ICONV
LIBS="$LIBS $LIBICONV"

#
# socklen_t
# from curl

dnl Check for socklen_t: historically on BSD it is an int, and in
dnl POSIX 1g it is a type of its own, but some platforms use different
dnl types for the argument to getsockopt, getpeername, etc.  So we
dnl have to test to find something that will work.
AC_DEFUN([TYPE_SOCKLEN_T],
[
   AC_CHECK_TYPE([socklen_t], ,[
      AC_MSG_CHECKING([for socklen_t equivalent])
      AC_CACHE_VAL([socklen_t_equiv],
      [
         # Systems have either "struct sockaddr *" or
         # "void *" as the second argument to getpeername
         socklen_t_equiv=
         for arg2 in "struct sockaddr" void; do
            for t in int size_t unsigned long "unsigned long"; do
               AC_TRY_COMPILE([
                  #include <sys/types.h>
                  #include <sys/socket.h>

                  int getpeername (int, $arg2 *, $t *);
               ],[
                  $t len;
                  getpeername(0,0,&len);
               ],[
                  socklen_t_equiv="$t"
                  break
               ])
            done
         done

         if test "x$socklen_t_equiv" = x; then
            AC_MSG_ERROR([Cannot find a type to use in place of socklen_t])
         fi
      ])
      AC_MSG_RESULT($socklen_t_equiv)
      AC_DEFINE_UNQUOTED(socklen_t, $socklen_t_equiv,
			[type to use in place of socklen_t if not defined])],
      [#include <sys/types.h>
#include <sys/socket.h>])
])

TYPE_SOCKLEN_T

#
# statfs stuff
#
AC_CHECK_HEADERS(sys/vfs.h)
AC_CHECK_HEADERS(sys/statvfs.h)
AC_CHECK_HEADERS(sys/statfs.h)
AC_CHECK_HEADERS(sys/param.h)

mount_includes="\
  $ac_includes_default
  #if HAVE_SYS_PARAM_H
  # include <sys/param.h>
  #endif
  "

AC_CHECK_HEADERS(sys/mount.h,,,[$mount_includes])

#################################################
# these tests are taken from the GNU fileutils package
AC_CHECKING(how to get filesystem space usage)
space=no

# Test for statvfs64.
if test $space = no; then
  # SVR4
  AC_CACHE_CHECK([statvfs64 function (SVR4)], fu_cv_sys_stat_statvfs64,
  [AC_TRY_RUN([
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/statvfs.h>
  main ()
  {
    struct statvfs64 fsd;
    exit (statvfs64 (".", &fsd));
  }],
  fu_cv_sys_stat_statvfs64=yes,
  fu_cv_sys_stat_statvfs64=no,
  fu_cv_sys_stat_statvfs64=cross)])
  if test $fu_cv_sys_stat_statvfs64 = yes; then
    space=yes
    AC_DEFINE(STAT_STATVFS64,1,[Whether statvfs64() is available])
  fi
fi

# Perform only the link test since it seems there are no variants of the
# statvfs function.  This check is more than just AC_CHECK_FUNCS(statvfs)
# because that got a false positive on SCO OSR5.  Adding the declaration
# of a `struct statvfs' causes this test to fail (as it should) on such
# systems.  That system is reported to work fine with STAT_STATFS4 which
# is what it gets when this test fails.
if test $space = no; then
  # SVR4
  AC_CACHE_CHECK([statvfs function (SVR4)], fu_cv_sys_stat_statvfs,
                 [AC_TRY_LINK([#include <sys/types.h>
#include <sys/statvfs.h>],
                              [struct statvfs fsd; statvfs (0, &fsd);],
                              fu_cv_sys_stat_statvfs=yes,
                              fu_cv_sys_stat_statvfs=no)])
  if test $fu_cv_sys_stat_statvfs = yes; then
    space=yes
    AC_DEFINE(STAT_STATVFS,1,[Whether statvfs() is available])
  fi
fi

if test $space = no; then
  # DEC Alpha running OSF/1
  AC_MSG_CHECKING([for 3-argument statfs function (DEC OSF/1)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs3_osf1,
  [AC_TRY_RUN([
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
  main ()
  {
    struct statfs fsd;
    fsd.f_fsize = 0;
    exit (statfs (".", &fsd, sizeof (struct statfs)));
  }],
  fu_cv_sys_stat_statfs3_osf1=yes,
  fu_cv_sys_stat_statfs3_osf1=no,
  fu_cv_sys_stat_statfs3_osf1=no)])


#C_MSG_RESULT($fu_cv_sys_stat_statfs3_osf1)
  if test $fu_cv_sys_stat_statfs3_osf1 = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS3_OSF1,1,[Whether statfs requires 3 arguments])
  fi
fi

if test $space = no; then
# AIX
  AC_MSG_CHECKING([for two-argument statfs with statfs.bsize dnl
member (AIX, 4.3BSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_bsize,
  [AC_TRY_RUN([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_bsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_bsize=yes,
  fu_cv_sys_stat_statfs2_bsize=no,
  fu_cv_sys_stat_statfs2_bsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_bsize)
  if test $fu_cv_sys_stat_statfs2_bsize = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS2_BSIZE,1,[Whether statfs requires two arguments and struct statfs has bsize property])
  fi
fi

if test $space = no; then
# SVR3
  AC_MSG_CHECKING([for four-argument statfs (AIX-3.2.5, SVR3)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs4,
  [AC_TRY_RUN([#include <sys/types.h>
#include <sys/statfs.h>
  main ()
  {
  struct statfs fsd;
  exit (statfs (".", &fsd, sizeof fsd, 0));
  }],
    fu_cv_sys_stat_statfs4=yes,
    fu_cv_sys_stat_statfs4=no,
    fu_cv_sys_stat_statfs4=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs4)
  if test $fu_cv_sys_stat_statfs4 = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS4,1,[Whether statfs requires 4 arguments])
  fi
fi

if test $space = no; then
# 4.4BSD and NetBSD
  AC_MSG_CHECKING([for two-argument statfs with statfs.fsize dnl
member (4.4BSD and NetBSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_fsize,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_fsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_fsize=yes,
  fu_cv_sys_stat_statfs2_fsize=no,
  fu_cv_sys_stat_statfs2_fsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_fsize)
  if test $fu_cv_sys_stat_statfs2_fsize = yes; then
    space=yes
        AC_DEFINE(STAT_STATFS2_FSIZE,1,[Whether statfs requires 2 arguments and struct statfs has fsize])
  fi
fi

if test $space = no; then
  # Ultrix
  AC_MSG_CHECKING([for two-argument statfs with struct fs_data (Ultrix)])
  AC_CACHE_VAL(fu_cv_sys_stat_fs_data,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_FS_TYPES_H
#include <sys/fs_types.h>
#endif
  main ()
  {
  struct fs_data fsd;
  /* Ultrix's statfs returns 1 for success,
     0 for not mounted, -1 for failure.  */
  exit (statfs (".", &fsd) != 1);
  }],
  fu_cv_sys_stat_fs_data=yes,
  fu_cv_sys_stat_fs_data=no,
  fu_cv_sys_stat_fs_data=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_fs_data)
  if test $fu_cv_sys_stat_fs_data = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS2_FS_DATA,1,[Whether statfs requires 2 arguments and struct fs_data is available])
  fi
fi

  statxfs_includes="\
$ac_includes_default
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#if !HAVE_SYS_STATVFS_H && !HAVE_SYS_VFS_H
# if HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
#  include <sys/param.h>
#  include <sys/mount.h>
# elif HAVE_NETINET_IN_H && HAVE_NFS_NFS_CLNT_H && HAVE_NFS_VFS_H
/* Ultrix 4.4 needs these for the declaration of struct statfs.  */
#  include <netinet/in.h>
#  include <nfs/nfs_clnt.h>
#  include <nfs/vfs.h>
# endif
#endif
"

AC_CHECK_MEMBERS([struct statfs.f_namemax],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statvfs.f_namemax],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statfs.f_namelen],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statvfs.f_namelen],,,[$statxfs_includes])

#
# Large file support
#
AC_SYS_LARGEFILE

#
# mntent
#
AC_CHECK_HEADER(mntent.h, AC_DEFINE(HAVE_MNTENT_H))
AC_CHECK_FUNCS(setmntent)

#
# IPv6
#
AC_ARG_WITH(ipv6,
    [  --with-ipv6             enable IPv6-support],
    [ 
        if test $withval != "no";
        then
	    AC_DEFINE(IPv6,1)
	fi
    ])


#
# debugging
#
AC_ARG_WITH(debug,
    [  --with-debug            enable protocol debugging output],
    [
        if test $withval != "no";
        then 
            AC_DEFINE(WITH_DEBUG,1)
	fi
    ])

AC_ARG_WITH(debug-kbd,
    [  --with-debug-kbd        enable debugging of keyboard handling],
    [
        if test $withval != "no";
        then 
            AC_DEFINE(WITH_DEBUG_KBD,1)
	fi
    ])

AC_ARG_WITH(debug-rdp5,
    [  --with-debug-rdp5       enable debugging of RDP5 code],
    [
        if test $withval != "no";
        then 
            AC_DEFINE(WITH_DEBUG_RDP5,1)
	fi
    ])

AC_ARG_WITH(debug-clipboard,
    [  --with-debug-clipboard  enable debugging of clipboard code],
    [
        if test $withval != "no";
        then 
            AC_DEFINE(WITH_DEBUG_CLIPBOARD,1)
	fi
    ])

AC_ARG_WITH(debug-channel,
    [  --with-debug-channel  enable debugging of virtual channel code],
    [
        if test $withval != "no";
        then 
            AC_DEFINE(WITH_DEBUG_CHANNEL,1)
	fi
    ])


#
# target-specific stuff
#
# strip leading colon from rpath
rpath=`echo $rpath |sed 's/^://'`
AC_CANONICAL_HOST
case "$host" in
*-*-solaris*)
    LDFLAGS="$LDFLAGS -R$rpath"
    ;;
*-dec-osf*)
    LDFLAGS="$LDFLAGS -Wl,-rpath,$rpath"
    ;;
*-*-hpux*)
    CFLAGS="$CFLAGS -D_XOPEN_SOURCE_EXTENDED"
    ;;
*-*-irix6.5*)
    LIBS="$LIBS -L$ssldir/lib32 -lcrypto"
    CFLAGS="$CFLAGS -D__SGI_IRIX__"
    ;;
esac


AC_OUTPUT(Makefile)

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoconf"
dnl End:
