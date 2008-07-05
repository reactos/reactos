AC_DEFUN(wi_ARG_ENABLE_DEBUG, [
# if DEBUGBUILD is yes, other macros try to set up a compilation environment
# with debugging symbols enabled.  Example macros which are affected are
# wi_CFLAGS and wi_SFLAG.
#
DEBUGBUILD=no
DEBUGCONFIGUREFLAG=""
AC_ARG_ENABLE(debug,
[  --enable-debug          enable debugging symbols],
[
	DEBUGBUILD=yes
	DEBUGCONFIGUREFLAG="--enable-debug"
],[
	DEBUGBUILD=no
	DEBUGCONFIGUREFLAG=""
])
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_EXTRA_IDIR, [
incdir="$1"
if test -r $incdir ; then
	case "$CPPFLAGS" in
		*${incdir}*)
			# echo "   + already had $incdir" 1>&6
			;;
		*)
			if test "$CPPFLAGS" = "" ; then
				CPPFLAGS="-I$incdir"
			else
				CPPFLAGS="$CPPFLAGS -I$incdir"
			fi
			echo "   + found $incdir" 1>&6
			;;
	esac
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_PROG_TAR, [
TAR=""
AC_PATH_PROG(TAR, "tar")
if test -x "$TAR" ; then
	AC_DEFINE_UNQUOTED(TAR, "$TAR")
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_EXTRA_LDIR, [
libdir="$1"
if test -r $libdir ; then
	case "$LDFLAGS" in
		*${libdir}*)
			# echo "   + already had $libdir" 1>&6
			;;
		*)
			if test "$LDFLAGS" = "" ; then
				LDFLAGS="-L$libdir"
			else
				LDFLAGS="$LDFLAGS -L$libdir"
			fi
			echo "   + found $libdir" 1>&6
			;;
	esac
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_GNU_LD, [
AC_MSG_CHECKING([for GNU ld])
wi_cv_prog_ld="ld"
result="no"
x=`ld --version 2>/dev/null | fgrep GNU`
if test "$x" != "" ; then
	wi_cv_prog_ld="gld"
	result="yes"
fi
AC_MSG_RESULT($result)
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_LD_READONLY_TEXT, [
if test "$SYS$wi_cv_prog_ld" = "linuxgld" ; then
	LDFLAGS="$LDFLAGS -Xlinker -n"
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_EXTRA_SYSV_SUNOS_DIRS, [
AC_MSG_CHECKING([for System V compatibility directories])
AC_MSG_RESULT([])
wi_EXTRA_IDIR("/usr/5include")
wi_EXTRA_LDIR("/usr/5lib")
])
dnl
dnl
dnl If you want to also look for include and lib subdirectories in the
dnl $HOME tree, you supply "yes" as the first argument to this macro.
dnl
dnl If you want to look for subdirectories in include/lib directories,
dnl you pass the names in argument 3, otherwise pass a dash.
dnl
AC_DEFUN(wi_EXTRA_DIRS, [
AC_MSG_CHECKING([for extra include and lib directories])
AC_MSG_RESULT([])
ifelse([$1], yes, [dnl
b1=`cd .. ; pwd`
b2=`cd ../.. ; pwd`
exdirs="$HOME $j $b1 $b2 $prefix $2"
if test -x "$HOME/bin/OS" ; then
	b3=`$HOME/bin/OS`
	b3="$HOME/$b3"
	if test -d "$b3" ; then
		exdirs="$b3 $exdirs"
	fi
fi
],[dnl
exdirs="$prefix $2"
])
subexdirs="$3"
if test "$subexdirs" = "" ; then
	subexdirs="-"
fi
for subexdir in $subexdirs ; do
if test "$subexdir" = "-" ; then
	subexdir=""
else
	subexdir="/$subexdir"
fi
for exdir in $exdirs ; do
	case "$exdir" in
		"/usr"|"/"|"//")
			if test "$exdir" = "//" ; then exdir="/" ; fi
			if test "$subexdir" != ""; then
				incdir="${exdir}/include${subexdir}"
				wi_EXTRA_IDIR($incdir)

				libdir="${exdir}/lib${subexdir}"
				wi_EXTRA_LDIR($libdir)
			fi
			;;
		*)
			if test "$subexdir" = ""; then
				incdir="${exdir}/include${subexdir}"
				wi_EXTRA_IDIR($incdir)

				libdir="${exdir}/lib${subexdir}"
				wi_EXTRA_LDIR($libdir)
			fi
			;;
	esac
done
done
])
dnl
dnl
dnl
AC_DEFUN(wi_HPUX_CFLAGS,
[AC_MSG_CHECKING(if HP-UX ansi C compiler flags are needed)
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([wi_OS_VAR])
ac_cv_hpux_flags=no
if test "$os" = hp-ux ; then
	if test "$ac_cv_prog_gcc" = yes ; then
		if test "$CFLAGS" != "" ; then
			# Shouldn't be in there.
changequote(<<, >>)dnl
			CFLAGS=`echo "$CFLAGS" | sed 's/-A[ae]//g'`
changequote([, ])dnl
			case "$CFLAGS" in
				*_HPUX_SOURCE*)
					;;
				*)
					# This is required for the extended
					# namespace.
					#
					CFLAGS="-D_HPUX_SOURCE $CFLAGS"
					;;
			esac
		fi
	else
		# If you're not using gcc, then you better have a cc/c89
		# that is usable.  If you have the barebones compiler, it
		# won't work.  The good compiler uses -Aa for the ANSI
		# compatible stuff.
changequote(<<, >>)dnl
		x=`echo "$CFLAGS" | grep 'A[ae]' 2>/dev/null`
changequote([, ])dnl
		if test "$x" = "" ; then
			CFLAGS="$CFLAGS -Ae"
		fi
	fi
	ac_cv_hpux_flags=yes
fi
AC_MSG_RESULT($ac_cv_hpux_flags)
])
dnl
dnl
dnl
AC_DEFUN(wi_SFLAG, [AC_REQUIRE([AC_PROG_CC])
STRIP="strip"
if test "$SFLAG" = "" ; then
	SFLAG="-s"
	case "$OS" in
		macosx*)
			SFLAG='-Wl,-x'
			;;
	esac
fi
#
# Was it ./configure --enable-debug ?
#
if test "$DEBUGBUILD" = yes ; then
	SFLAG=""
	STRIP=":"
fi
case "$CFLAGS" in
	"-g"|"-g "*|*" -g"|*" -g "*|*"-g"[0-9]*)
		# SFLAG="# $SFLAG"
		SFLAG=""
		STRIP=":"
		;;
esac
STRIPFLAG="$SFLAG"
])
dnl
dnl
dnl
AC_DEFUN(wi_REQUEST_NO_Y2K_WARNINGS, [
	wi_cv_request_no_y2k=yes
])
dnl
dnl
dnl
AC_DEFUN(wi_CFLAGS_NO_Y2K_WARNINGS, [AC_REQUIRE([AC_PROG_CC])
if test "$ac_cv_prog_gcc" = yes ; then
	case "$CFLAGS" in
		*-Wno-format-y2k*)
			;;
		*)
			oldCFLAGS="$CFLAGS"
			CFLAGS="$CFLAGS -Wno-format-y2k"
			#
			# Now check if this version of GCC
			# accepts this flag...
			#
AC_TRY_COMPILE([],[int junk;],[],[CFLAGS="$oldCFLAGS"])
			unset oldCFLAGS
			;;
	esac
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_CFLAGS, [AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE_CPP()
wi_HPUX_CFLAGS
	if test "$CFLAGS" = "" ; then
		AC_MSG_WARN([Your CFLAGS environment variable was not set.  A default of \"-g\" will be used.])
		CFLAGS="-g"
	fi
	if test "x$wi_cv_request_no_y2k" = xyes ; then
		wi_CFLAGS_NO_Y2K_WARNINGS
	fi
	if test "$NOOPTCFLAGS" = "" ; then
changequote(<<, >>)dnl
		NOOPTCFLAGS=`echo "$CFLAGS" | sed 's/[-+]O[0-9A-Za-z]*//g;s/-xO[0-9]//g;s/-Wc,-O3//g;s/-IPA//g;s/\ \ */ /g;s/^\ *//;s/\ *$//;'`
changequote([, ])dnl
	fi
	if test "$DEBUGCFLAGS" = "" ; then
		DEBUGCFLAGS="-g $NOOPTCFLAGS"
	fi
	#
	# Was it ./configure --enable-debug ?
	#
	AC_MSG_CHECKING([if this is a debug build])
	if test "$DEBUGBUILD" = yes ; then
		AC_MSG_RESULT(yes)
		CFLAGS="$DEBUGCFLAGS"
		SFLAG=""
		STRIPFLAG=""
		STRIP=":"
	else
		AC_MSG_RESULT(no)
	fi
	AC_MSG_CHECKING([NOOPTCFLAGS])
	AC_MSG_RESULT($NOOPTCFLAGS)
	AC_MSG_CHECKING([DEBUGCFLAGS])
	AC_MSG_RESULT($DEBUGCFLAGS)
	AC_MSG_CHECKING([CFLAGS])
	AC_MSG_RESULT($CFLAGS)
])
dnl
dnl
dnl
AC_DEFUN(wi_CFLAGS_LFS64, [AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([wi_OS_VAR])
wi_CFLAGS
if test "os_${os}_gcc_${ac_cv_prog_gcc}" = os_hp-ux_gcc_yes ; then
	case "$CFLAGS" in
		*__STDC_EXT__*)
			;;
		*)
			# This is required for the extended
			# namespace symbols for Large Files.
			#
			CFLAGS="-D__STDC_EXT__ $CFLAGS"
			;;
	esac
fi
case "$CFLAGS" in
	*-D_LARGEFILE64_SOURCE*)
		;;
	*)
		CFLAGS="-D_LARGEFILE64_SOURCE $CFLAGS"
		;;
esac
AC_MSG_CHECKING([additional CFLAGS for LFS64 support])
AC_MSG_RESULT($CFLAGS)
])
dnl
dnl
dnl
AC_DEFUN(wi_PROTOTYPES, [
AC_MSG_CHECKING(if the compiler supports function prototypes)
AC_TRY_COMPILE(,[extern void exit(int status);],[wi_cv_prototypes=yes
AC_DEFINE(PROTOTYPES)],wi_cv_prototypes=no)
AC_MSG_RESULT($wi_cv_prototypes)
])
dnl
dnl
dnl
AC_DEFUN(wi_INSECURE_CHOWN, [
AC_MSG_CHECKING(if chown can be used to subvert security)
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
 
main()
{
	int result;
	char fn[64];
	FILE *fp;
	struct stat st;

	setuid(1);	/* if you're root, try set to someone else. */
	sprintf(fn, "/tmp/fu%06ld", (long) getpid());
	unlink(fn);
	fp = fopen(fn, "w");
	if (fp == NULL)
		exit(1);	/* assume the worst */
	fprintf(fp, "%s\n", "hello world");
	fclose(fp);

	result = chown(fn, 0, 0);
	if (stat(fn, &st) < 0) {
		unlink(fn);
		exit((result == 0) ? 0 : 1);
	}
	unlink(fn);

	/* exit(0) if the insecure chown to uid 0 succeeded. */
	exit((st.st_uid == 0) ? 0 : 1);
}],[
	# action if true
	wi_cv_insecure_chown=yes
	AC_DEFINE(INSECURE_CHOWN)
],[
	# action if false
	wi_cv_insecure_chown=no
],[
	# action if cross-compiling, guess
	wi_cv_insecure_chown=no
])

AC_MSG_RESULT($wi_cv_insecure_chown)
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_SNPRINTF, [
if test "$ac_cv_func_snprintf" = "no" ; then
	AC_CHECK_LIB(snprintf,snprintf)
	if test "$ac_cv_lib_snprintf_snprintf" = yes ; then
		unset ac_cv_func_snprintf
		AC_CHECK_FUNCS(snprintf)
	fi
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_SNPRINTF_TERMINATES, [
if test "$ac_cv_func_snprintf" != "no" ; then
AC_MSG_CHECKING(if snprintf always NUL terminates)
	if test "$ac_cv_func_snprintf" = "no" ; then
		AC_CHECK_LIB(snprintf,snprintf)
	fi
AC_TRY_RUN([
	/* program */
#include <stdio.h>
#include <string.h>
 
main()
{
	char s[10];
	int i, result;

	for (i=0; i<(int)(sizeof(s)/sizeof(char)); i++)
		s[i] = 'x';
	result = (int) snprintf(s, sizeof(s), "%s %s!", "hello", "world");
	if (s[sizeof(s) - 1] == '\0')
		exit(0);
	exit(1);

}
],[
	# action if true
	wi_cv_snprintf_terminates=no
	AC_DEFINE(SNPRINTF_TERMINATES)
	x="yes";
],[
	# action if false
  	wi_cv_snprintf_terminates=yes
	x="no";
],[
	# action if cross compiling
	wi_cv_snprintf_terminates=no
	x="unknown";
])
AC_MSG_RESULT($x)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_SNPRINTF, [
wi_SPRINTF_RETVAL
dnl Uncache these -- config.cache doesn't cache it right for this case.
unset ac_cv_func_snprintf
unset ac_cv_func_vsnprintf

AC_CHECK_FUNCS(snprintf vsnprintf)
wi_SNPRINTF_TERMINATES

AC_CHECK_HEADERS(snprintf.h)
wi_LIB_SNPRINTF
])
dnl
dnl
dnl
AC_DEFUN(wi_HEADER_HPSECURITY_H, [
AC_MSG_CHECKING(for hpsecurity.h)
wi_cv_header_hpsecurity_h=no
if test -f /usr/include/hpsecurity.h ; then
	wi_cv_header_hpsecurity_h=yes
	AC_DEFINE(HAVE_HPSECURITY_H)
fi
AC_MSG_RESULT($wi_cv_header_hpsecurity_h)
])
dnl
dnl
dnl
AC_DEFUN(wi_HEADER_SYS_SELECT_H, [
# See if <sys/select.h> is includable after <sys/time.h>
if test "$ac_cv_header_sys_time_h" = no ; then
AC_CHECK_HEADERS(sys/time.h sys/select.h)
else
AC_CHECK_HEADERS(sys/select.h)
fi
if test "$ac_cv_header_sys_select_h" = yes ; then
	AC_MSG_CHECKING([if <sys/select.h> is compatible with <sys/time.h>])
	selecth=yes
	if test "$ac_cv_header_sys_time_h" = yes ; then
		AC_TRY_COMPILE([
#if defined(_AIX) || defined(__aix) || defined(__AIX)
#	define _ALL_SOURCE 1
#endif
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>],[
		fd_set a;
		struct timeval tmval;

		tmval.tv_sec = 0;],selecth=yes,selecth=no)
	fi
	if test "$selecth" = yes ; then
		AC_DEFINE(CAN_USE_SYS_SELECT_H)
	fi
	AC_MSG_RESULT($selecth)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_44BSD, [
AC_CHECK_FUNC(strerror,[a=yes],[a=no])
if test "$a" = no ; then
	# Not in libc, try lib44bsd.
	AC_CHECK_LIB(44bsd,strerror)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_NSL, [
dnl Note: Check for socket lib first, then nsl.

case "$OS" in
	hpux1[123456789]*)
		# HP-UX 11 uses NSL for YP services
		AC_CHECK_LIB(nsl,getpwent)
		;;

	*)
		AC_CHECK_FUNC(gethostbyname,[a=yes],[a=no])
		if test "$a" = no ; then
			# Not in libc, try libnsl.
			AC_CHECK_LIB(nsl,gethostbyname)
		fi
		;;
esac

])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_SOCKET, [
AC_CHECK_FUNC(socket,[a=yes],[a=no])
if test "$a" = no ; then
	# Not in libc, try libsocket.
	AC_CHECK_LIB(socket,socket)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_RESOLV, [
# See if we could access two well-known sites without help of any special
# libraries, like resolv.
dnl
AC_MSG_WARN([the following check may take several minutes if networking is not up.  You may want to bring it up now and restart configure, otherwise please be patient.])
dnl
AC_MSG_CHECKING([if we need to look for -lresolv])
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
 
main()
{
	struct hostent *hp1, *hp2;
	int result;

	hp1 = gethostbyname("gatekeeper.dec.com");
	hp2 = gethostbyname("ftp.ncsa.uiuc.edu");
	result = ((hp1 != (struct hostent *) 0) && (hp2 != (struct hostent *) 0));
	exit(! result);
}],look_for_resolv=no,look_for_resolv=yes,look_for_resolv=yes)

AC_MSG_RESULT($look_for_resolv)
if test "$look_for_resolv" = yes ; then
AC_CHECK_LIB(resolv,main)
else
	ac_cv_lib_resolv=no
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_TCP_WRAPPERS, [
AC_MSG_CHECKING([for tcp wrappers library (libwrap)])

AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>

/* These are needed if libwrap.a was compiled with
 * PROCESS_OPTIONS defined.
 */
int allow_severity = 1;	/* arbitrary for this test */
int deny_severity = 2;	/* arbitrary for this test */

],[
	/* function-body */
	exit((allow_severity == deny_severity) ? 1 : 0);
],[
dnl	...Don't bother defining this symbol...
dnl	...Check for tcpd.h instead...
dnl	AC_DEFINE(HAVE_LIBWRAP)
dnl
dnl	...Don't modify LIBS, instead set WRAPLIB...
dnl	LIBS="-lwrap  $LIBS"
dnl
	WRAPLIB="-lwrap"
	wi_cv_lib_wrap_hosts_access=yes
],[
	WRAPLIB=""
	wi_cv_lib_wrap_hosts_access=no
])
AC_MSG_RESULT($wi_cv_lib_wrap_hosts_access)
])
dnl
dnl
dnl
AC_DEFUN(wi_NET_LIBS, [
# Mostly for SunOS 4 -- needs to come first because other libs depend on it
wi_LIB_44BSD

wi_LIB_SOCKET

if test "$SYS" = unixware ; then
	# So far, only UnixWare needs this.
	AC_CHECK_LIB(gen,syslog)

	case "$OS" in
		unixware2*)
			if test -f /usr/ucblib/libucb.a ; then
				LDFLAGS="$LDFLAGS -L/usr/ucblib"
				LIBS="$LIBS -lucb"
			fi
			if test -f /usr/include/unistd.h ; then
				ac_cv_header_unistd_h=yes
			fi

			# UnixWare 2 needs both lsocket and lnsl, and configure
			# script won't detect this condition properly because 
			# the libraries are interdependent.
			#
			LIBS="$LIBS -lsocket -lnsl"

			# Now look for socket()
			#
			# AC_CHECK_FUNC(socket,[a=yes],[a=no])
			#
			AC_CHECK_FUNC(socket,[a=yes],[a=no])
			;;
		*)
			;;
	esac
fi

dnl AC_CHECK_LIB(inet,main)

wi_LIB_NSL
wi_LIB_RESOLV

if test "$SYS" = dynixptx ; then
	LIBS="$LIBS -lsocket -lnsl"
fi

])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_DEFINE_UNAME, [
# Get first 127 chars of all uname information.  Some folks have
# way too much stuff there, so grab only the first 127.
unam=`uname -a 2>/dev/null | cut -c1-127`
if test "$unam" != "" ; then
	AC_DEFINE_UNQUOTED(UNAME, "$unam")
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_FUNC_SIGSETJMP, [
AC_MSG_CHECKING([for sigsetjmp and siglongjmp])

AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
],[
	/* function-body */
	sigjmp_buf sjb;

	if (sigsetjmp(sjb, 1) != 0)
		siglongjmp(sjb, 1);	/* bogus code, of course. */
	exit(0);
],[
	AC_DEFINE(HAVE_SIGSETJMP)
	wi_cv_func_sigsetjmp=yes
],[
	wi_cv_func_sigsetjmp=no
])
AC_MSG_RESULT($wi_cv_func_sigsetjmp)
])
dnl
dnl
dnl
AC_DEFUN(wi_UTMP_UT_NAME, [
AC_MSG_CHECKING([for ut_name field in struct utmp])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
],[
struct utmp u;

u.ut_name[0] = '\0';
exit(((int) &u.ut_name) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmp_ut_name=yes
	AC_DEFINE(HAVE_UTMP_UT_NAME)
],[
	wi_cv_utmp_ut_name=no
])
AC_MSG_RESULT($wi_cv_utmp_ut_name)
])
dnl
dnl
dnl
AC_DEFUN(wi_UTMPX_UT_SYSLEN, [
AC_MSG_CHECKING([for ut_syslen field in struct utmpx])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmpx.h>
],[
struct utmpx u;

u.ut_syslen = 0;
exit(((int) &u.ut_syslen) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmpx_ut_syslen=yes
	AC_DEFINE(HAVE_UTMPX_UT_SYSLEN)
],[
	wi_cv_utmpx_ut_syslen=no
])
AC_MSG_RESULT($wi_cv_utmpx_ut_syslen)
])
dnl
dnl
dnl
AC_DEFUN(wi_UTMP_UT_USER, [
AC_MSG_CHECKING([for ut_user field in struct utmp])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
],[
struct utmp u;

u.ut_user[0] = '\0';
exit(((int) &u.ut_user) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmp_ut_user=yes
	AC_DEFINE(HAVE_UTMP_UT_USER)
],[
	wi_cv_utmp_ut_user=no
])
AC_MSG_RESULT($wi_cv_utmp_ut_user)
])
dnl
dnl
dnl
AC_DEFUN(wi_UTMP_UT_PID, [
AC_MSG_CHECKING([for ut_pid field in struct utmp])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
],[
struct utmp u;

u.ut_pid = 1;
exit(((int) &u.ut_pid) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmp_ut_pid=yes
	AC_DEFINE(HAVE_UTMP_UT_PID)
],[
	wi_cv_utmp_ut_pid=no
])
AC_MSG_RESULT($wi_cv_utmp_ut_pid)
])

dnl
dnl
dnl
AC_DEFUN(wi_UTMP_UT_TIME, [
AC_MSG_CHECKING([for ut_time field in struct utmp])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
],[
struct utmp u;

u.ut_time = 1;
exit(((int) &u.ut_time) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmp_ut_time=yes
	AC_DEFINE(HAVE_UTMP_UT_TIME)
],[
	wi_cv_utmp_ut_time=no
])
AC_MSG_RESULT($wi_cv_utmp_ut_time)
])
dnl
dnl
dnl
AC_DEFUN(wi_UTMP_UT_HOST, [
AC_MSG_CHECKING([for ut_host field in struct utmp])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <utmp.h>
],[
struct utmp u;

u.ut_host[0] = '\0';
exit(((int) &u.ut_host) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_utmp_ut_host=yes
	AC_DEFINE(HAVE_UTMP_UT_HOST)
],[
	wi_cv_utmp_ut_host=no
])
AC_MSG_RESULT($wi_cv_utmp_ut_host)
])
dnl
dnl
dnl
AC_DEFUN(wi_STRUCT_STAT64, [
AC_MSG_CHECKING([for struct stat64])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
],[
struct stat64 st;

st.st_size = 0;
exit(((int) &st.st_size) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_struct_stat64=yes
	AC_DEFINE(HAVE_STRUCT_STAT64)
],[
	wi_cv_struct_stat64=no
])
AC_MSG_RESULT($wi_cv_struct_stat64)
])
dnl
dnl
dnl
AC_DEFUN(wi_STRUCT_CMSGHDR, [
AC_MSG_CHECKING([for struct cmsghdr])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
],[
struct cmsghdr cm;

cm.cmsg_len = 0;
cm.cmsg_level = 0;
cm.cmsg_type = 0;
exit(((int) &cm.cmsg_type) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_struct_cmsghdr=yes
	AC_DEFINE(HAVE_STRUCT_CMSGDHR)
],[
	wi_cv_struct_cmsghdr=no
])
AC_MSG_RESULT($wi_cv_struct_cmsghdr)
])
dnl
dnl
dnl
AC_DEFUN(wi_MSGHDR_CONTROL, [
AC_MSG_CHECKING([for msg_control field in struct msghdr])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
],[
struct msghdr m;

m.msg_control = &m;
m.msg_controllen = sizeof(m);
exit(((int) &m.msg_control) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_msghdr_control=yes
	AC_DEFINE(HAVE_MSGHDR_CONTROL)
],[
	wi_cv_msghdr_control=no
])
AC_MSG_RESULT($wi_cv_msghdr_control)
])
dnl
dnl
dnl
AC_DEFUN(wi_MSGHDR_ACCRIGHTS, [
AC_MSG_CHECKING([for msg_accrights field in struct msghdr])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
],[
struct msghdr m;

m.msg_accrights = &m;
m.msg_accrightslen = sizeof(m);
exit(((int) &m.msg_accrights) & 0xff);	/* bogus code, of course. */
],[
	wi_cv_msghdr_accrights=yes
	AC_DEFINE(HAVE_MSGHDR_ACCRIGHTS)
],[
	wi_cv_msghdr_accrights=no
])
AC_MSG_RESULT($wi_cv_msghdr_accrights)
])
dnl
dnl
dnl
AC_DEFUN(wi_PR_PASSWD_FG_OLDCRYPT, [
AC_MSG_CHECKING([for fg_oldcrypt field in struct pr_passwd])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#if defined(HAVE_USERPW_H) && defined(HAVE_GETUSERPW)	/* AIX */
#	include <userpw.h>
#elif defined(HAVE_PWDADJ_H) && defined(HAVE_GETPWANAM)	/* SunOS */
#	include <sys/label.h>
#	ifdef HAVE_SYS_AUDIT_H
#		include <sys/audit.h>
#	endif
#	include <pwdadj.h>
#elif defined(HAVE_GETESPWNAM) /* Digital UNIX 4 */
#	ifdef HAVE_SYS_SECDEFINES_H
#		include <sys/secdefines.h>
#	endif
#	ifdef HAVE_SYS_SECURITY_H
#		include <sys/security.h>
#	endif
#	ifdef HAVE_SYS_AUDIT_H
#		include <sys/audit.h>
#	endif
#	ifdef HAVE_KRB_H
#		include <krb.h>
#	endif
#	ifdef HAVE_PROT_H
#		include <prot.h>
#	endif
#elif defined(HAVE_GETPRPWNAM) /* SCO Open Server V, Digital UNIX 3, HP-UX 10 */
#	ifdef HAVE_SYS_SECDEFINES_H
#		include <sys/secdefines.h>
#	endif
#	ifdef HAVE_SYS_SECURITY_H
#		include <sys/security.h>
#	endif
#	ifdef HAVE_SYS_AUDIT_H
#		include <sys/audit.h>
#	endif
#	ifdef HAVE_HPSECURITY_H
#		include <hpsecurity.h>
#	endif
#	ifdef HAVE_KRB_H
#		include <krb.h>
#	endif
#	ifdef HAVE_PROT_H
#		include <prot.h>
#	endif
#endif
],[
	struct pr_passwd xu;
	memset(&xu, 0, sizeof(xu));
	if (xu.uflg.fg_oldcrypt != 0)
		xu.uflg.fg_oldcrypt++;	/* bogus code, of course */
	exit(0);
],[
	wi_cv_pr_passwd_fg_oldcrypt=yes
	AC_DEFINE(HAVE_PR_PASSWD_FG_OLDCRYPT)
],[
	wi_cv_pr_passwd_fg_oldcrypt=no
])
AC_MSG_RESULT($wi_cv_pr_passwd_fg_oldcrypt)
])
dnl
dnl
dnl
AC_DEFUN(wi_SOCKADDR_UN_SUN_LEN, [
AC_MSG_CHECKING([for sun_len field in struct sockaddr_un])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
],[
struct sockaddr_un uaddr;

uaddr.sun_len = strlen("/tmp/test.sock");
exit(((int) uaddr.sun_len);	/* bogus code, of course. */
],[
	wi_cv_sockaddr_un_sun_len=yes
	AC_DEFINE(HAVE_SOCKADDR_UN_SUN_LEN)
],[
	wi_cv_sockaddr_un_sun_len=no
])
AC_MSG_RESULT($wi_cv_sockaddr_un_sun_len)
])
dnl
dnl
dnl
AC_DEFUN(wi_STATFS_F_BAVAIL, [
AC_MSG_CHECKING([for f_bavail field in struct statfs])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STATFS_H
#	include <sys/statfs.h>
#elif defined(HAVE_SYS_VFS_H)
#	include <sys/vfs.h>
#endif
],[
struct statfs st;

st.f_bavail = 1;
exit((int) st.f_bavail);	/* bogus code, of course. */
],[
	wi_cv_statfs_f_bavail=yes
	AC_DEFINE(HAVE_STATFS_F_BAVAIL)
],[
	wi_cv_statfs_f_bavail=no
])
AC_MSG_RESULT($wi_cv_statfs_f_bavail)
])
dnl
dnl
dnl
AC_DEFUN(wi_SPRINTF_RETVAL, [
AC_MSG_CHECKING([what sprintf() returns])
AC_TRY_RUN([
	/* program */
#include <stdio.h>
#include <string.h>
 
main()
{
	int result;
	char s[8];

	result = (int) sprintf(s, "%d", 22);
	if (result == 2)
		exit(0);
	exit(1);

}
],[
	# action if true
	wi_cv_sprintf_returns_ptr=no
	x="length of data written";
],[
	# action if false
  	wi_cv_sprintf_returns_ptr=yes
	AC_DEFINE(SPRINTF_RETURNS_PTR)
	x="pointer to data";
],[
	# action if cross compiling
	wi_cv_sprintf_returns_ptr=no
	x="unknown";
])
AC_MSG_RESULT($x)
])
dnl
dnl
dnl
AC_DEFUN(wi_LIB_CRYPT, [
AC_MSG_CHECKING([which library has usable crypt() function])
ac_save_LIBS="$LIBS"
crypt_lib=NONE

for lib in "c" "crypt" "descrypt" "des"
do

if test "$lib" = "c" ; then
	LIBS="$ac_save_LIBS"
else
	LIBS="$ac_save_LIBS -l${lib}"
fi

AC_TRY_RUN([
	/* program */
#include <stdio.h>
#include <string.h>

extern char *crypt(const char *key, const char *salt);

main()
{
	char cleartext[256];
	char *cp;

	memset(cleartext, 0, sizeof(cleartext));
	strcpy(cleartext, "password");

	cp = crypt(cleartext, "xx");
	if ((cp != NULL) && (strcmp(cp, "xxj31ZMTZzkVA") == 0)) {
		/* printf("PASS\n"); */
		exit(0);
	}
	/* printf("FAIL\n"); */
	exit(1);
}
],[
	# action if true
	crypt_lib="$lib"
],[
	# action if false
	:
],[
	# action if cross compiling
	:
])


if test "$crypt_lib" != NONE ; then
	break
fi

done


LIBS="$ac_save_LIBS"

if test "$crypt_lib" = NONE ; then
	crypt_lib=c
	AC_MSG_RESULT([none?])
else
	AC_MSG_RESULT([lib${crypt_lib}])
fi
if test "$crypt_lib" != c ; then
	AC_CHECK_LIB(${lib},crypt)
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_TEST, [
changequote(<^, ^>)dnl
changequote([, ])dnl
])
dnl
dnl
dnl
AC_DEFUN(wi__RES_DEFDNAME, [
AC_MSG_CHECKING([for useable _res global variable])
AC_TRY_LINK([
	/* includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifdef HAVE_ARPA_NAMESER_H
#	include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
#	include <resolv.h>
#endif
],[
	/* function-body */
	int len;

	res_init();
	len = (int) strlen(_res.defdname);
],[
	wi_cv__res_defdname=yes
	AC_DEFINE(HAVE__RES_DEFDNAME)
],[
	wi_cv__res_defdname=no
])
AC_MSG_RESULT($wi_cv__res_defdname)
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_TYPE_SIG_ATOMIC_T, [
AC_MSG_CHECKING([for sig_atomic_t])
AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/signal.h>
#include <signal.h>	/* MG: for IRIX */
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
],[
	/* function-body */
	sig_atomic_t sample;

	sample = (sig_atomic_t) getpid();	/* bogus code, of course */
	exit((sample > 0) ? 0 : 1);
],[
	ac_cv_type_sig_atomic_t=yes
],[
	ac_cv_type_sig_atomic_t=no
])
AC_MSG_RESULT($ac_cv_type_sig_atomic_t)
if test $ac_cv_type_sig_atomic_t = no ; then
	AC_DEFINE(sig_atomic_t, int)
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_LIB_READLINE, [
AC_MSG_CHECKING([for GNU Readline library, version 2.0 or newer])

wi_cv_lib_readline=no
wi_cv_lib_readline_result=no
ac_save_LIBS="$LIBS"
# Note: $LIBCURSES is permitted to be empty.
for LIBREADLINE in "-lreadline" "-lreadline $LIBCURSES" "-lreadline -ltermcap" "-lreadline -lncurses" "-lreadline -lcurses"
do
	LIBS="$ac_save_LIBS $LIBREADLINE"
	AC_TRY_RUN([
	/* program */
#include <stdio.h>
#include <stdlib.h>
 
main(int argc, char **argv)
{
	/* Note:  don't actually call readline, since it may block;
	 * We just want to see if it (dynamic) linked in okay.
	 */
	if (argc == 0)	/* never true */
		readline(0);
	exit(0);
}
],[
	# action if true
	wi_cv_lib_readline=yes
],[
	# action if false
	wi_cv_lib_readline=no
],[
	# action if cross compiling
	wi_cv_lib_readline=no
])

	if test "$wi_cv_lib_readline" = yes ; then break ; fi
done

# Now try it again, to be sure it is recent enough.
# rl_function_of_keyseq appeared in version 2.0
#
dnl AC_CHECK_FUNC(rl_function_of_keyseq, [wi_cv_lib_readline=yes],[
dnl 	wi_cv_lib_readline=no;wi_cv_lib_readline_result="no (it is present but too old to use)"
dnl ])
	AC_TRY_LINK([
		/* includes */
	],[
		/* function-body */
		readline(0);
		rl_function_of_keyseq(0);
	],[
		wi_cv_lib_readline=yes
	],[
		wi_cv_lib_readline=no
		wi_cv_lib_readline_result="no (it is present but too old to use)"
	])

if test "$wi_cv_lib_readline" = no ; then
	LIBREADLINE=""
	# restore LIBS
	LIBS="$ac_save_LIBS"
else
	/bin/rm -f readline.ver
	touch readline.ver

	AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

extern char *rl_library_version;

main()
{
	FILE *fp;
	double d;

	sscanf(rl_library_version, "%lf", &d);
	fp = fopen("readline.ver", "w");
	if (fp == NULL) exit(1);
	if (fprintf(fp, "%s\n", rl_library_version) < 0) exit(1);
	if (fprintf(fp, "%03d\n", (int) (d * 100.0)) < 0) exit(1);
	if (fclose(fp) < 0) exit(1);
	exit(0);
}
	],[
		# action if true
		rl_library_version=`sed -n 1,1p readline.ver 2>/dev/null`
		rlver=`sed -n 2,2p readline.ver 2>/dev/null`
		/bin/rm -f readline.ver
	],[
		# action if false
		rl_library_version=''
		rlver=''
		/bin/rm -f readline.ver
	],[
		# action if cross compiling
		rl_library_version=''
		rlver=''
		/bin/rm -f readline.ver
	])

	case "$rlver" in
		???)
			wi_cv_lib_readline_result="yes, installed version is $rl_library_version"
			;;
		*)
			# Test using current LIBS.
			AC_TRY_LINK([
				/* includes */
				extern int rl_completion_append_character;
			],[
				/* function-body */
				readline(0);
				rl_completion_append_character = 0;
			],[
				rlver="210"
			],[
				rlver="200"
			])

			if test "$rlver" = "210" ; then
				wi_cv_lib_readline_result="yes, version 2.1 or higher"
			else
				wi_cv_lib_readline_result="yes, version 2.0"
			fi
			;;
	esac

	wi_cv_lib_readline=yes
	# restore LIBS
	LIBS="$ac_save_LIBS"
fi
AC_MSG_RESULT($wi_cv_lib_readline_result)
AC_SUBST(LIBREADLINE)

if test "$wi_cv_lib_readline" = yes ; then
	# Now verify that all the headers are installed.
	#
	AC_REQUIRE_CPP()
	unset ac_cv_header_readline_chardefs_h
	unset ac_cv_header_readline_history_h
	unset ac_cv_header_readline_keymaps_h
	unset ac_cv_header_readline_readline_h
	unset ac_cv_header_readline_tilde_h
	AC_CHECK_HEADERS([readline/chardefs.h readline/history.h readline/keymaps.h readline/readline.h readline/tilde.h])

	for xxwi in \
		"$ac_cv_header_readline_chardefs_h" \
		"$ac_cv_header_readline_history_h" \
		"$ac_cv_header_readline_keymaps_h" \
		"$ac_cv_header_readline_readline_h" \
		"$ac_cv_header_readline_tilde_h" 
	do
		if test "$xxwi" = no ; then
			break
		fi
	done

	if test "$xxwi" = no ; then
		AC_MSG_WARN([GNU Readline headers are not installed or could not be found -- GNU Readline will not be used.])
		wi_cv_lib_readline=no
		wi_cv_lib_readline_result="no (headers not installed)"
	else
		AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE, $rlver)
	fi
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_USE_LONG_LONG, [
AC_MSG_CHECKING([for 64-bit integral type: long long])
LONGEST_INT="long"
AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

long long hugeNumvar = 1;

main()
{
	long long hugeNumtoo = 2;

	if (hugeNumtoo > hugeNumvar)
		hugeNumvar++;
	if (sizeof(hugeNumvar) < 8)
		exit(1);
	exit(0);
}

],[
	# action if true
	wi_cv_type_long_long=yes
	LONGEST_INT="long long"
],[
	# action if false
  	wi_cv_type_long_long=no
],[
	# action if cross compiling
	wi_cv_type_long_long=no
])
AC_MSG_RESULT($wi_cv_type_long_long)

if test "$wi_cv_type_long_long" = yes ; then
	
AC_MSG_CHECKING([how to print a 64-bit integral type])
wi_cv_printf_long_long=fail

AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main()
{
	char s[80];
	long long hugeNum;

	hugeNum = (long long) 1000000000;
	hugeNum = hugeNum * (long long) 99;
	hugeNum = hugeNum + (long long) 1;

	(void) sprintf(s, "%lld", hugeNum);
	exit((strcmp(s, "99000000001") == 0) ? 0 : 1);
}
],[
	# action if true
	wi_cv_printf_long_long="%lld"
],[
	# action if false
	:
],[
	# action if cross compiling
	:
])


if test "$wi_cv_printf_long_long" = fail ; then

AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main()
{
	char s[80];
	long long hugeNum;

	hugeNum = (long long) 1000000000;
	hugeNum = hugeNum * (long long) 99;
	hugeNum = hugeNum + (long long) 1;

	(void) sprintf(s, "%qd", hugeNum);
	exit((strcmp(s, "99000000001") == 0) ? 0 : 1);
}
],[
	# action if true
	wi_cv_printf_long_long="%qd"
],[
	# action if false
	:
],[
	# action if cross compiling
	:
])
fi

if test "$wi_cv_printf_long_long" = fail ; then
	wi_cv_printf_long_long_msg_result='cannot print'
else
	wi_cv_printf_long_long_msg_result="$wi_cv_printf_long_long"
fi

AC_MSG_RESULT($wi_cv_printf_long_long_msg_result)

	
AC_MSG_CHECKING([how to scan a 64-bit integral type])
wi_cv_scanf_long_long=fail

AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main()
{
	long long hugeNum, justAsHugeNum;

	hugeNum = (long long) 1000000000;
	hugeNum = hugeNum * (long long) 99;
	hugeNum = hugeNum + (long long) 1;

	justAsHugeNum = (long long) 0;
	--justAsHugeNum;
	sscanf("99000000001", "%lld", &justAsHugeNum);
	if (memcmp(&hugeNum, &justAsHugeNum, sizeof(hugeNum)) == 0)
		exit(0);
	exit(1);
}
],[
	# action if true
	wi_cv_scanf_long_long="%lld"
],[
	# action if false
	:
],[
	# action if cross compiling
	:
])


if test "$wi_cv_scanf_long_long" = fail ; then

AC_TRY_RUN([
	/* program */
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main()
{
	long long hugeNum, justAsHugeNum;

	hugeNum = (long long) 1000000000;
	hugeNum = hugeNum * (long long) 99;
	hugeNum = hugeNum + (long long) 1;

	justAsHugeNum = (long long) 0;
	--justAsHugeNum;
	sscanf("99000000001", "%qd", &justAsHugeNum);
	if (memcmp(&hugeNum, &justAsHugeNum, sizeof(hugeNum)) == 0)
		exit(0);
	exit(1);
}
],[
	# action if true
	wi_cv_scanf_long_long="%qd"
],[
	# action if false
	:
],[
	# action if cross compiling
	:
])
fi

if test "$wi_cv_scanf_long_long" = fail ; then
	wi_cv_scanf_long_long_msg_result='cannot scan'
else
	wi_cv_scanf_long_long_msg_result="$wi_cv_scanf_long_long"
fi

AC_MSG_RESULT($wi_cv_scanf_long_long_msg_result)

fi

AC_MSG_CHECKING([if everything was available to use the 64-bit integral type])

if test "$wi_cv_type_long_long" = no ; then
	wi_cv_use_long_long_msg_result="no (long long type not available)"
	wi_cv_use_long_long="no"
	wi_cv_scanf_long_long="fail"
	wi_cv_prihtf_long_long="fail"
	LONGEST_INT="long"
elif test "$wi_cv_printf_long_long" = fail ; then
	wi_cv_use_long_long_msg_result="no (libc printf() does not support them)"
	wi_cv_use_long_long="no"
	wi_cv_scanf_long_long="fail"
	wi_cv_prihtf_long_long="fail"
	LONGEST_INT="long"
elif test "$wi_cv_scanf_long_long" = fail ; then
	wi_cv_use_long_long_msg_result="no (libc scanf() does not support them)"
	wi_cv_use_long_long="no"
	wi_cv_scanf_long_long="fail"
	wi_cv_prihtf_long_long="fail"
	LONGEST_INT="long"
else
	AC_DEFINE(HAVE_LONG_LONG)
	if test "$wi_cv_printf_long_long$wi_cv_scanf_long_long" = "%lld%qd" ; then
		# FreeBSD 3.2 has %lld and %qd, but we want to
		# run on 3.1 and 3.0.
		#
		wi_cv_printf_long_long="%qd"
	fi
	AC_DEFINE_UNQUOTED(PRINTF_LONG_LONG, "$wi_cv_printf_long_long")
	AC_DEFINE_UNQUOTED(SCANF_LONG_LONG , "$wi_cv_scanf_long_long")
	if test "$wi_cv_printf_long_long" = "%qd" ; then
		AC_DEFINE(PRINTF_LONG_LONG_QD)
	else
		AC_DEFINE(PRINTF_LONG_LONG_LLD)
	fi
	if test "$wi_cv_scanf_long_long" = "%qd" ; then
		AC_DEFINE(SCANF_LONG_LONG_QD)
	else
		AC_DEFINE(SCANF_LONG_LONG_LLD)
	fi
	wi_cv_use_long_long="yes"
	wi_cv_use_long_long_msg_result="yes"
fi
AC_MSG_RESULT($wi_cv_use_long_long_msg_result)
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_CREATE_TAR_FILES, [
AC_MSG_CHECKING([how to create TAR files])
changequote(<<, >>)dnl
TAR=/usr/bin/tar
if [ ! -f /usr/bin/tar ] && [ -f /bin/tar ] ; then
	TAR=/bin/tar
fi
x=""
if [ -x /usr/bin/what ] ; then
	x=`/usr/bin/what "$TAR" 2>&1 | sed -n 's/.*pax.*/pax/g;/pax/p'`
elif [ -x /bin/what ] ; then
	x=`/bin/what "$TAR" 2>&1 | sed -n 's/.*pax.*/pax/g;/pax/p'`
fi
if [ "x$x" != "xpax" ] ; then
	# The junk above is because Mac OS X Server's tar freaks out
	# and does not exit if you do "tar --help".
	#
	x=`$TAR --help 2>&1 | sed -n 's/.*owner=NAME.*/owner=NAME/g;/owner=NAME/p'`
fi
case "$x" in
	*owner=NAME*)
		TARFLAGS="-c --owner=root --group=bin --verbose -f"
		;;
	*)
		TARFLAGS="cvf"
		x2=`gtar --help 2>&1 | sed -n 's/.*owner=NAME.*/owner=NAME/g;/owner=NAME/p'`
		case "$x2" in
			*owner=NAME*)
				TARFLAGS="-c --owner=root --group=bin --verbose -f"
				TAR=gtar
				;;
		esac
		;;
esac
changequote([, ])dnl
AC_SUBST(TARFLAGS)
AC_SUBST(TAR)
AC_MSG_RESULT([$TAR $TARFLAGS])
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_HEADER_CURSES, [
AC_MSG_CHECKING([for curses library headers])
if test "$nc_cv_ncurses" != "no" ; then
	AC_CHECK_HEADERS(ncurses.h curses.h)
else
	AC_CHECK_HEADERS(curses.h)
fi
dnl needed for Solaris 7
if test "$ac_cv_header_curses_h" = no ; then
	if test -f /usr/include/curses.h ; then
		AC_DEFINE(HAVE_CURSES_H)
		ac_cv_header_curses_h=yes
	fi
fi
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_LIB_CURSES, [
wi_HEADER_CURSES
AC_MSG_CHECKING([for curses library])

wi_cv_lib_curses=no
wi_cv_lib_curses_result=no
ac_save_LIBS="$LIBS"
for LIBCURSES in "-lncurses" "-lcurses" "-lcurses -ltermcap" "-ltermcap -lcurses"
do
	if test "x$LIBCURSES-$nc_cv_ncurses" = "x-lncurses-no" ; then
		# This should never work
		LIBCURSES="-lkdfjkdjfs"
	fi
	LIBS="$ac_save_LIBS $LIBCURSES"
	AC_TRY_RUN([
	/* program */
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif

 
main(int argc, char **argv)
{
	/* Note:  don't actually call curses, since it may block;
	 * We just want to see if it (dynamic) linked in okay.
	 */
	if (argc == 4)
		initscr();
	exit(0);
}
],[
	# action if true
	wi_cv_lib_curses=yes
	wi_cv_lib_curses_result="yes"
],[
	# action if false
	wi_cv_lib_curses=no
],[
	# action if cross compiling
	wi_cv_lib_curses=no
])

	if test "$wi_cv_lib_curses" = yes ; then break ; fi
done

# restore LIBS
LIBS="$ac_save_LIBS"

if test "$wi_cv_lib_curses_result" != "no" ; then
	case "$LIBCURSES" in
		"-lncurses")
			AC_DEFINE(HAVE_LIBNCURSES)
			;;
		"-lcurses")
			AC_DEFINE(HAVE_LIBCURSES)
			;;
		"-lcurses -ltermcap")
			AC_DEFINE(HAVE_LIBCURSES)
			;;
		"-ltermcap -lcurses")
			AC_DEFINE(HAVE_LIBCURSES)
			;;
	esac
else
	LIBCURSES=''
fi

AC_SUBST(LIBCURSES)
AC_MSG_RESULT([$wi_cv_lib_curses_result])
])
dnl
dnl
dnl
dnl
AC_DEFUN(wi_CURSES_FEATURES, [
if test "$wi_cv_lib_curses" = "yes" ; then
	# Then $LIBCURSES is a list of curses and support libraries.
	ac_save_LIBS="$LIBS";
	LIBS="$LIBS $LIBCURSES";


	# maxx or _maxx
	AC_MSG_CHECKING([whether curses structure has maxx or _maxx field])
	AC_TRY_COMPILE([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif
],[
		WINDOW *w;
	
		w = newwin(10, 10, 1, 1);
		w->maxx = 0;
],[
AC_MSG_RESULT([maxx])
],[
AC_DEFINE(HAVE__MAXX)
AC_MSG_RESULT([_maxx])
])

	AC_CHECK_FUNCS(__getmaxx __getmaxy __getbegx __getbegy)

	# getbegx
	AC_MSG_CHECKING([for getbegx() functionality in curses library])
	AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif

],[
	/* function-body */
	WINDOW *junk = 0;
	int mx = 0;

	mx = getbegx(junk);
	exit(0);
],[
	AC_DEFINE(HAVE_GETBEGX)
	AC_MSG_RESULT([yes])
],[
	AC_MSG_RESULT([no])
])


	# getmaxx
	AC_MSG_CHECKING([for getmaxx() functionality in curses library])
	AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif
],[
	/* function-body */
	WINDOW *junk = 0;
	int mx = 0;

	mx = getmaxx(junk);
	exit(0);
],[
	AC_DEFINE(HAVE_GETMAXX)
	AC_MSG_RESULT([yes])
],[
	AC_MSG_RESULT([no])
])

	# getmaxyx
	AC_MSG_CHECKING([for getmaxyx() functionality in curses library])
	AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif
],[
	/* function-body */
	WINDOW *junk = 0;
	int mx = 0, my = 0;

	getmaxyx(junk, my, mx);
	exit(my < 0 ? my : 0);
],[
	AC_DEFINE(HAVE_GETMAXYX)
	AC_MSG_RESULT([yes])
],[
	AC_MSG_RESULT([no])
])

	# touchwin
	AC_MSG_CHECKING([for touchwin() functionality in curses library])
	AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif
],[
	/* function-body */
	WINDOW *junk = 0;
	touchwin(junk);
	exit(0);
],[
	AC_DEFINE(HAVE_TOUCHWIN)
	AC_MSG_RESULT([yes])
],[
	AC_MSG_RESULT([no])
])

	# beep
	AC_MSG_CHECKING([for beep() functionality in curses library])
	AC_TRY_LINK([
	/* includes */
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#else
#	include <curses.h>
#endif
],[
	/* function-body */
	beep();
	exit(getpid() & 1);
],[
	AC_DEFINE(HAVE_BEEP)
	AC_MSG_RESULT([yes])
],[
	AC_MSG_RESULT([no])
])

	AC_CHECK_FUNCS(keypad nodelay curs_set doupdate wnoutrefresh)

	LIBS="$ac_save_LIBS";
fi
])
dnl
dnl
dnl
AC_DEFUN(wi_SHADOW_FUNCS, [
AC_CHECK_FUNCS(md5_crypt md5crypt bcrypt getspnam)

# UnixWare 7
if test "$ac_cv_func_getspnam" = no ; then
	unset ac_cv_func_getspnam
	AC_CHECK_LIB(gen,getspnam)
	if test "$ac_cv_lib_gen_getspnam" = yes ; then
		AC_CHECK_FUNCS(getspnam)
	fi
fi

# AIX
#
case "$SYS" in
	"aix"|"")
		AC_CHECK_FUNCS(getuserpw)
		;;
	*)
		;;
esac

# C2: SCO Open Server 5; Digital UNIX
AC_CHECK_FUNCS(set_auth_parameters bigcrypt)

# C2: Digital UNIX 3.2, 4.0; SCO Open Server 5; HP-UX 11
AC_CHECK_FUNCS(getprpwnam)

# Digital UNIX 4.0
AC_CHECK_FUNCS(getespwnam get_num_crypts get_crypt_name)

# Digital Unix 4.0
AC_CHECK_FUNCS(dispcrypt)

# SunOS
AC_CHECK_FUNCS(getpwanam)
])
dnl
dnl
dnl
AC_DEFUN(wi_SHADOW_HEADERS, [
AC_CHECK_HEADERS(shadow.h crypt.h)

# AIX
AC_CHECK_HEADERS(userpw.h)

# SunOS
AC_CHECK_HEADERS(pwdadj.h)

# HP-UX
#
# Bug in header on these version 10 which cause is it not
# to get detected.
#
wi_HEADER_HPSECURITY_H

# SCO Open Server, Digital UNIX
AC_CHECK_HEADERS(sys/security.h sys/audit.h krb.h prot.h)

# Digital UNIX
AC_CHECK_HEADERS(sys/secdefines.h)

# Digital UNIX
wi_PR_PASSWD_FG_OLDCRYPT
])
dnl
dnl
dnl
AC_DEFUN(wi_SHADOW_LIBS, [
check_for_libcrypt=yes

# AIX security library is libs.a
AC_CHECK_LIB(s,getuserpw)
if test "$ac_cv_lib_s" = yes ; then
	check_for_libcrypt=no
elif test "$ac_cv_lib_s_getuserpw" = yes ; then
	check_for_libcrypt=no
fi

# SCO OpenServer 5 stuff for shadow password
AC_CHECK_LIB(x,nap)
AC_CHECK_LIB(prot,getprpwnam)

# Digital UNIX
AC_CHECK_LIB(security,endprpwent)

# HP-UX
AC_CHECK_LIB(sec,getprpwnam)

if test "$ac_cv_lib_sec_getprpwnam" = no ; then
	# DYNIX/ptx
	AC_CHECK_LIB(sec,getspnam)
fi

if test "$check_for_libcrypt" = yes ; then
	wi_LIB_CRYPT
fi
AC_CHECK_FUNCS(crypt)
])
dnl
dnl
dnl
AC_DEFUN(wi_OS_VAR, [
changequote(!@, @!)dnl
if [ -x "$HOME/bin/OS" ] ; then
	HOME_OS=`$HOME/bin/OS`
	HOME_OS="$HOME/$HOME_OS"
fi
host=`uname -n 2>/dev/null | tr '[A-Z]' '[a-z]'`
os=`uname -s 2>/dev/null | tr '[A-Z]' '[a-z]'`
dnl work around inability to use $1
os_v=`uname -v 2>/dev/null | sed 's/^[^0-9.]*//;s/[^0-9.]*$//;s/pre.*//;s/test.*//' | awk '-F[-/: ]' '{n = 1; print $n; }'`
os_r=`uname -r 2>/dev/null | sed 's/^[^0-9.]*//;s/[^0-9.]*$//;s/pre.*//;s/test.*//' | awk '-F[-/: ]' '{n = 1; print $n; }'`
os_r1=`echo "${os_r}" | cut -c1`
arch=`uname -m 2>/dev/null | tr '[A-Z]' '[a-z]'`
archp=`uname -p 2>/dev/null | tr '[A-Z]' '[a-z]'`
OS=''
SYS=''
NDEFS=''

# Special case a few systems where if your CFLAGS appear
# to want to generate for 32 bit, use that mode rather
# than 64 bit.
#
case "$os,$CFLAGS" in
	irix64,*-n32*)
		os=irix
		# then go to regular "irix" instead of "irix64" below.
		;;
esac

case "$os" in
	osf1)
		case "$os_r" in
			3*|4*)
				OS="digitalunix${os_r}-$arch"
				SYS=digitalunix
				;;
			*)
				OS="tru64unix${os_r}-$arch"
				SYS=tru64unix
				;;
		esac
		NDEFS="$NDEFS -DDIGITAL_UNIX=$os_r1"
		;;
	aix)
		OS="aix${os_v}.${os_r}"
		SYS=aix
		NDEFS="$NDEFS -DAIX=${os_v}"
		;;
	irix)
		OS="irix${os_r}"
		SYS=irix
		NDEFS="$NDEFS -DIRIX=$os_r1"
		;;
	irix64)
		OS="irix64_${os_r}"
		SYS=irix64
		NDEFS="$NDEFS -DIRIX=$os_r1 -DIRIX64=$os_r1"
		;;
	hp-ux)
		os_r=`echo "${os_r}" | cut -d. -f2-`
		os_r1=`echo "$os_r" | cut -d. -f1`
		os_r2=`echo "${os_r}" | cut -d. -f2`
		os_int=`expr "$os_r1" '*' 100 + "$os_r2"`
		OS="hpux${os_r}"
		SYS=hpux
		NDEFS="$NDEFS -DHPUX=$os_int"
		;;
	freebsd)
		OS="freebsd${os_r}-$arch"
		os_r1=`echo "$os_r" | cut -d. -f1`
		os_r2=`echo "$os_r" | cut -d. -f2`
		os_r3=`echo "$os_r" | cut -d. -f3`
		if [ "$os_r3" = "" ] ; then os_r3=0 ; fi
		os_int=`expr "$os_r1" '*' 100 + "$os_r2" '*' 10 + "$os_r3"`
		SYS=freebsd
		NDEFS="$NDEFS -DFREEBSD=$os_int"
		;;
	netbsd)
		OS="netbsd${os_r}-$arch"
		NDEFS="$NDEFS -DNETBSD=$os_r1"
		SYS=netbsd
		;;
	openbsd)
		OS="openbsd${os_r}-$arch"
		SYS=openbsd
		NDEFS="$NDEFS -DOPENBSD=$os_r1"
		;;
	sco*)
		OS=scosv
		SYS=sco
		NDEFS="$NDEFS -DSCO=$os_r1"
		;;
	dynix*)
		OS="dynixptx${os_v}"
		SYS=dynixptx
		os_v1=`echo "$os_v" | cut -d. -f1`
		os_v2=`echo "$os_v" | cut -d. -f2`
		os_v3=`echo "$os_v" | cut -d. -f3`
		if [ "$os_v3" = "" ] ; then os_v3=0 ; fi
		os_int=`expr "$os_v1" '*' 100 + "$os_v2" '*' 10 + "$os_v3"`
		NDEFS="$NDEFS -DDYNIX=$os_int"
		;;
	linux)
		case "$arch" in
			*86)
				arch=x86
				;;
		esac

		libc=""
		os_r1=`echo "$os_r" | cut -d. -f1`
		os_r2=`echo "$os_r" | cut -d. -f2`
		os_r3=`echo "$os_r" | cut -d- -f1 | cut -d. -f3`
		os_int=`expr "$os_r1" '*' 10000 + "$os_r2" '*' 1000 + "$os_r3"`
		NDEFS="$NDEFS -DLINUX=$os_int"

		vertest="./vertest.$$"
		/bin/rm -f "$vertest" "$vertest.c"
		cat <<EOF > "$vertest.c"
#include <stdio.h>
#include <gnu/libc-version.h>

main()
{
	const char *ver = gnu_get_libc_version();
	const char *rel = gnu_get_libc_release();

	fprintf(stdout, "glibc%s\n", ver);
	exit(0);
}
EOF
		echo $ac_n "checking version of C library""... $ac_c" 1>&6
		echo "configure:: checking version of C library" >&5
		${CC-cc} $DEFS $CPPFLAGS $CFLAGS "$vertest.c" -o "$vertest" >/dev/null 2>&1
		if [ -x "$vertest" ] ; then libc=`$vertest` ; fi
		/bin/rm -f "$vertest" "$vertest.c"

		case "$libc" in
			glibc*)
				echo "$libc" 1>&6
				glibc_r=`echo "$libc" | sed 's/glibc//'`
				glibc_r1=`echo "$glibc_r" | cut -d. -f1`
				glibc_r2=`echo "$glibc_r" | cut -d. -f2`
				glibc_r3=`echo "$glibc_r" | cut -d- -f1 | cut -d. -f3`
				glibc_int=`expr "$glibc_r1" '*' 10000 + "$glibc_r2" '*' 1000 + "$glibc_r3"`
				NDEFS="$NDEFS -DLINUX_GLIBC=$glibc_int"
				libc="glibc${glibc_r1}.${glibc_r2}"
				OS="linux-$arch"
				;;
			*)
				if test -f /lib/libc-2.2.2.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=22002"
					libc="glibc2.2"
					OS="linux-$arch"
				elif test -f /lib/libc-2.2.1.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=22001"
					libc="glibc2.2"
					OS="linux-$arch"
				elif test -f /lib/libc-2.2.0.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=22000"
					libc="glibc2.1"
					OS="linux-$arch"
				elif test -f /lib/libc-2.1.3.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=21003"
					libc="glibc2.1"
					OS="linux-$arch"
				elif test -f /lib/libc-2.1.2.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=21002"
					libc="glibc2.1"
					OS="linux-$arch"
				elif test -f /lib/libc-2.1.1.so ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=21001"
					libc="glibc2.1"
					OS="linux-$arch"
				elif test -f /lib/libc.so.6 ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=20000"
					libc="glibc2.0"
					OS="linux-$arch"
				elif test -f /lib/libc.so.6.1 ; then
					NDEFS="$NDEFS -DLINUX_GLIBC=20001"
					libc="glibc2.0"
					OS="linux-$arch"
				else
					NDEFS="$NDEFS -DLINUX_LIBC=5"
					libc="libc5"
					OS="linux-$arch"
				fi
				echo "$libc" 1>&6
				;;
		esac
		SYS=linux
		;;
	bsd/os)
		OS="bsdos${os_r}"
		SYS=bsdos
		NDEFS="$NDEFS -DBSDOS=$os_r1"
		;;
	ultrix)
		OS="ultrix-$arch"
		SYS=ultrix
		;;
	unixware|eeyore)
		OS="unixware${os_v}"
		SYS=unixware
		;;
	macos*|darwin|rhapsody)
		OS="macosx"
		SYS="macosx"
		;;
	sunos)
		if [ "$arch" = "" ] ; then arch="sparc" ; fi
		if [ "$archp" = "" ] ; then archp="$arch" ; fi
		case "$os_r" in
			5.[789]*)
				os_r=`echo "$os_r" | cut -c3-`
				OS="solaris${os_r}-$archp"
				NDEFS="$NDEFS -DSOLARIS=\\\"$os_r\\\""
				SYS=solaris
				;;
			5.[0123456]*)
				maj=`echo "$os_r" | cut -c1-1`
				maj=`expr "$maj" - 3`
				os_r=`echo "$os_r" | cut -c2-`
				os_r="${maj}${os_r}"
				OS="solaris${os_r}-$archp"
				NDEFS="$NDEFS -DSOLARIS=\\\"$os_r\\\""
				SYS=solaris
				;;
			4.*)
				OS="sunos${os_r}-sparc"
				NDEFS="$NDEFS -DSUNOS=\\\"$os_r\\\""
				SYS=sunos
				;;
			*)
				OS="solaris${os_r}-$archp"
				NDEFS="$NDEFS -DSOLARIS=\\\"$os_r\\\""
				SYS=solaris
				;;
		esac
		;;
	*)
		OS="$os"
		SYS="$os"

		if grep Novell /usr/include/sys/types.h ; then
			OS="unixware${os_v}"
			SYS="unixware"
		fi
		;;
esac

changequote([, ])

AC_SUBST(NDEFS)
AC_SUBST(OS)
AC_SUBST(host)
AC_SUBST(SYS)
AC_SUBST(HOME_OS)
])
