# This file is part of the FreeType project.
#
# Process this file with autoconf to produce a configure script.
#
# Copyright 2001, 2002, 2003, 2004, 2005 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

AC_INIT
AC_CONFIG_SRCDIR([ftconfig.in])


# Don't forget to update docs/VERSION.DLL!

version_info='9:8:3'
AC_SUBST([version_info])
ft_version=`echo $version_info | tr : .`
AC_SUBST([ft_version])


# checks for system type

AC_CANONICAL_TARGET


# checks for programs

AC_PROG_CC
AC_PROG_CPP


# get compiler flags right

if test "x$CC" = xgcc; then
  XX_CFLAGS="-Wall"
  XX_ANSIFLAGS="-pedantic -ansi"
else
  case "$host" in
  *-dec-osf*)
    CFLAGS=
    XX_CFLAGS="-std1 -g3"
    XX_ANSIFLAGS=
    ;;
  *)
    XX_CFLAGS=
    XX_ANSIFLAGS=
    ;;
  esac
fi
AC_SUBST([XX_CFLAGS])
AC_SUBST([XX_ANSIFLAGS])


# auxiliary programs

AC_CHECK_PROG([RMF], [rm], [rm -f])
AC_CHECK_PROG([RMDIR], [rmdir], [rmdir])


# Since this file will be finally moved to another directory we make
# the path of the install script absolute.  This small code snippet has
# been taken from automake's `ylwrap' script.

AC_PROG_INSTALL
case "$INSTALL" in
/*)
  ;;
*/*)
  INSTALL="`pwd`/$INSTALL" ;;
esac


# checks for header files

AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h unistd.h])


# checks for typedefs, structures, and compiler characteristics

AC_C_CONST
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])


# checks for library functions

# Here we check whether we can use our mmap file component.

AC_FUNC_MMAP
if test "$ac_cv_func_mmap_fixed_mapped" != yes; then
  FTSYS_SRC='$(BASE_DIR)/ftsystem.c'
else
  FTSYS_SRC='$(BUILD_DIR)/ftsystem.c'

  AC_CHECK_DECLS([munmap],
    [],
    [],
    [

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/mman.h>

    ])

  FT_MUNMAP_PARAM
fi
AC_SUBST([FTSYS_SRC])

AC_CHECK_FUNCS([memcpy memmove])


# Check for system zlib

AC_ARG_WITH([zlib],
  dnl don't quote AS_HELP_STRING!
  AS_HELP_STRING([--without-zlib],
                 [use internal zlib instead of system-wide]))
if test x$with_zlib != xno && test -z "$LIBZ"; then
  AC_CHECK_LIB([z], [gzsetparams], [AC_CHECK_HEADER([zlib.h], [LIBZ='-lz'])])
fi
if test x$with_zlib != xno && test -n "$LIBZ"; then
  CFLAGS="$CFLAGS -DFT_CONFIG_OPTION_SYSTEM_ZLIB"
  LDFLAGS="$LDFLAGS $LIBZ"
  SYSTEM_ZLIB=yes
fi


# Whether to use Mac OS resource-based fonts or not

AC_ARG_WITH([old-mac-fonts],
  dnl don't quote AS_HELP_STRING!
  AS_HELP_STRING([--with-old-mac-fonts],
                 [allow Mac resource-based fonts to be used]))
if test x$with_old_mac_fonts = xyes; then
  LDFLAGS="$LDFLAGS -Xlinker -framework -Xlinker CoreServices \
                    -Xlinker -framework -Xlinker ApplicationServices"
else
  CFLAGS="$CFLAGS -DDARWIN_NO_CARBON"
fi


AC_SUBST([LIBZ])
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([SYSTEM_ZLIB])


AC_PROG_LIBTOOL
# urgh -- these are internal libtool variables...
AC_SUBST([enable_shared])
AC_SUBST([hardcode_libdir_flag_spec])
AC_SUBST([wl])


# configuration file -- stay in 8.3 limit
#
# since #undef doesn't survive in configuration header files we replace
# `/undef' with `#undef' after creating the output file

AC_CONFIG_HEADERS([ftconfig.h:ftconfig.in],
  [mv ftconfig.h ftconfig.tmp
   sed 's|/undef|#undef|' < ftconfig.tmp > ftconfig.h
   rm ftconfig.tmp])

# create the Unix-specific sub-Makefiles `builds/unix/unix-def.mk'
# and 'builds/unix/unix-cc.mk' that will be used by the build system
#
AC_CONFIG_FILES([unix-cc.mk:unix-cc.in
                 unix-def.mk:unix-def.in
                 freetype-config
                 freetype2.pc:freetype2.in])

# re-generate the Jamfile to use libtool now
#
# AC_CONFIG_FILES([../../Jamfile:../../Jamfile.in])

AC_OUTPUT

# end of configure.ac
