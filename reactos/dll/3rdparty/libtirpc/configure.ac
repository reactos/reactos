AC_INIT(libtirpc, 0.2.1)
AM_INIT_AUTOMAKE(libtirpc, 0.2.1)
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/auth_des.c])

AC_ARG_ENABLE(gss,[  --enable-gss            Turn on gss api], [case "${enableval}" in
        yes) gss=true ; AC_CHECK_LIB([gssapi],[gss_init_sec_context]) ;;
        no)  gss=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-gss) ;;
      esac],[gss=false])
AM_CONDITIONAL(GSS, test x$gss = xtrue)
if test x$gss = xtrue; then
	AC_DEFINE(HAVE_LIBGSSAPI, 1, [])
	PKG_CHECK_MODULES(GSSGLUE, libgssglue, [],
	AC_MSG_ERROR([Unable to locate information required to use libgssglue.]))
fi

AC_PROG_CC
AM_CONFIG_HEADER(config.h)
AC_PROG_LIBTOOL
##AC_PROG_RANLIB
AC_HEADER_DIRENT
AC_PREFIX_DEFAULT(/usr)
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h limits.h locale.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])
AC_CHECK_LIB([pthread], [pthread_create])


AC_CONFIG_FILES([Makefile src/Makefile man/Makefile doc/Makefile])
AC_OUTPUT(libtirpc.pc)
