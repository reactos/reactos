#
# Resolve how to compile Posix thread programs.
#

# serial 1

AC_DEFUN(AM_POSIX_THREADS,
[
#
# First, check the easily recognizable compilers.
#
AC_CANONICAL_HOST

# pgcc, pthreads gcc by Chris Provenzano
AC_CHECK_PROG(PGCC_BY_PROVENZANO, pgcc, yes, no)

if test "$PGCC_BY_PROVENZANO" = "yes"; then
  CC='pgcc'
  CFLAGS="$CFLAGS -g -Wall -Wno-unused"
  LDFLAGS="$LDFLAGS -g"
fi

if test "X$CC" = "X"; then
  # xlc_r, thread safe C compiler for AIX
  AC_CHECK_PROG(XLC_R_AIX, xlc_r, yes, no)

  if test "$XLC_R_AIX" = "yes"; then
    CC='xlc_r'
    CFLAGS='-I/usr/local/include -g'
    LDFLAGS="$LDFLAGS -L/usr/local/lib -g"
  fi
fi

AC_ARG_WITH(cc,
[  --with-cc(=CC)	use system's native compiler or compiler CC],
  if test "X$withval" != "Xno"; then
    if test "X$withval" = "Xyes"; then
      CC='cc'
    else
      CC=$withval
    fi
    CFLAGS='-I/usr/local/include'
    LDFLAGS="$LDFLAGS -L/usr/local/lib"
    echo "using compiler CC=$CC"
  fi
)

if test "X$CC" = "X"; then
  # Ok, not an easy compiler, let's try our knowledge-base
  AC_PROG_CC
  case "$host" in
    *solaris2.5* )
      if test -n "$GCC"; then
	CFLAGS="$CFLAGS -D_REENTRANT"
      else
	CFLAGS="$CFLAGS -threads"
      fi
      ;;
    *osf* )
      if test -n "$GCC"; then
        AC_WARN(don't know how to make gcc thread-aware, using cc)
	CC='cc'
	CFLAGS="$CFLAGS -threads"
      else
	CFLAGS=="$CFLAGS -threads"
      fi
    ;;
    *aix4* )
      if test -n "$GCC"; then
	CFLAGS="$CFLAGS -D_REENTRANT"
	LDFLAGS="$LDFLAGS -L/usr/lib/threads -nostartfiles /lib/crt0_r.o -lc_r -lpthreads"
      else
	AC_WARN(don't know how to make $CC thread-aware)
      fi
    ;;
    *-*-linux-gnu )
      CFLAGS="$CFLAGS -D_REENTRANT"
      LDFLAGS="-lpthread"
    ;;
    * )
      AC_WARN(don't know how to make compiler thread-aware)
      ;;
  esac
fi
# Check pthread functions.
AC_CHECK_FUNCS(pthread_attr_setscope \
	pthread_attr_setstacksize \
	pthread_mutexattr_create \
	pthread_condattr_create \
	pthread_condattr_init \
	pthread_attr_create)

# Check if pthread_mutex_init() takes attributes by value.
AC_MSG_CHECKING([if pthread_mutex_init() takes attributes by value])
AC_TRY_COMPILE([
#include <stdio.h>
#include <pthread.h>
], [
  pthread_mutex_t m;

  pthread_mutex_init (&m, NULL);
], [
  AC_MSG_RESULT(no)
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(MUTEXATTR_BY_VALUE)
])

# Check if pthread_cond_init() takes attributes by value.
AC_MSG_CHECKING([if pthread_cond_init() takes attributes by value])
AC_TRY_COMPILE([
#include <stdio.h>
#include <pthread.h>
], [
  pthread_cond_t c;

  pthread_cond_init (&c, NULL);
], [
  AC_MSG_RESULT(no)
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(CONDATTR_BY_VALUE)
])

# Check if pthread_create() takes attributes by value.
AC_MSG_CHECKING([if pthread_create() takes attributes by value])
AC_TRY_COMPILE([
#include <stdio.h>
#include <pthread.h>
], [
  pthread_t th;
  pthread_attr_t th_attr;

  pthread_create (&th, &th_attr, NULL, NULL);
], [
  AC_MSG_RESULT(no)
], [
  AC_MSG_RESULT(yes)
  AC_DEFINE(THREADATTR_BY_VALUE)
])

# Check if asctime_r() takes three arguments.
AC_MSG_CHECKING([if asctime_r() takes three arguments])
AC_TRY_COMPILE([
#include <stdio.h>
#include <time.h>
], [
  struct tm tm;
  char buf[256];

  asctime_r (&tm, buf, sizeof (buf));
], [
  AC_DEFINE(ASCTIME_R_WITH_THREE_ARGS)
  AC_MSG_RESULT(yes)
], [
  AC_MSG_RESULT(no)
])

# Check if drand48_r() is available and it works with DRAND48D data.
AC_MSG_CHECKING([if drand48_r() works with DRAND48D])
AC_TRY_COMPILE([
#include <stdlib.h>
], [
  DRAND48D data;
  double r;

  srand48_r (42, &data);
  drand48_r (&data, &r);
], [
  AC_DEFINE(DRAND48_R_WITH_DRAND48D)
  AC_MSG_RESULT(yes)
], [
  AC_MSG_RESULT(no)
  AC_MSG_WARN(assuming that the drand48() function is thread-safe)
])


# End of AM_POSIX_THREADS macro.
])
