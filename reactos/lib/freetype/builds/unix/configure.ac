dnl This file is part of the FreeType project.
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT
AC_CONFIG_SRCDIR([ftconfig.in])

dnl configuration file -- stay in 8.3 limit
AC_CONFIG_HEADER(ftconfig.h:ftconfig.in)

dnl Don't forget to update VERSION.DLL!
version_info='9:2:3'
AC_SUBST(version_info)
ft_version=`echo $version_info | tr : .`
AC_SUBST(ft_version)

dnl checks for system type
AC_CANONICAL_TARGET([])

dnl checks for programs
AC_PROG_CC
AC_PROG_CPP

dnl get Compiler flags right.
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
AC_SUBST(XX_CFLAGS)
AC_SUBST(XX_ANSIFLAGS)

AC_CHECK_PROG(RMF, rm, rm -f)
AC_CHECK_PROG(RMDIR, rmdir, rmdir)

dnl Since this file will be finally moved to another directory we make
dnl the path of the install script absolute.  This small code snippet has
dnl been taken from automake's `ylwrap' script.
AC_PROG_INSTALL
case "$INSTALL" in
  /*)
    ;;
  */*)
    INSTALL="`pwd`/$INSTALL" ;;
esac

dnl checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h)

dnl checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)

dnl checks for library functions

dnl Here we check whether we can use our mmap file component.
AC_FUNC_MMAP
if test "$ac_cv_func_mmap_fixed_mapped" != yes; then
  FTSYS_SRC='$(BASE_)ftsystem.c'
else
  FTSYS_SRC='$(BUILD)/ftsystem.c'

  FT_MUNMAP_DECL
  FT_MUNMAP_PARAM
fi
AC_SUBST(FTSYS_SRC)

AC_CHECK_FUNCS(memcpy memmove)

AC_PROG_LIBTOOL

dnl create the Unix-specific sub-Makefiles `builds/unix/unix-def.mk'
dnl and 'builds/unix/unix-cc.mk' that will be used by the build system
dnl
AC_CONFIG_FILES([unix-cc.mk:unix-cc.in unix-def.mk:unix-def.in freetype-config])

dnl re-generate the Jamfile to use libtool now
dnl
dnl AC_CONFIG_FILES([../../Jamfile:../../Jamfile.in])

AC_OUTPUT

dnl end of configure.ac
