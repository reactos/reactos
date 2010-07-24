dnl aclocal.m4 for ICU
dnl Copyright (c) 1999-2007, International Business Machines Corporation and
dnl others. All Rights Reserved.
dnl Stephen F. Booth

dnl @TOP@

dnl ICU_CHECK_MH_FRAG
AC_DEFUN(ICU_CHECK_MH_FRAG, [
	AC_CACHE_CHECK(
		[which Makefile fragment to use],
		[icu_cv_host_frag],
		[
case "${host}" in
*-*-solaris*)
	if test "$GCC" = yes; then	
		icu_cv_host_frag=mh-solaris-gcc
	else
		icu_cv_host_frag=mh-solaris
	fi ;;
alpha*-*-linux-gnu)
	if test "$GCC" = yes; then
		icu_cv_host_frag=mh-alpha-linux-gcc
	else
		icu_cv_host_frag=mh-alpha-linux-cc
	fi ;;
powerpc*-*-linux*)
	if test "$GCC" = yes; then
		icu_cv_host_frag=mh-linux
	else
		icu_cv_host_frag=mh-linux-va
	fi ;;
*-*-linux*|*-pc-gnu) icu_cv_host_frag=mh-linux ;;
*-*-cygwin|*-*-mingw32)
	if test "$GCC" = yes; then
		AC_TRY_COMPILE([
#ifndef __MINGW32__
#error This is not MinGW
#endif], [], icu_cv_host_frag=mh-mingw, icu_cv_host_frag=mh-cygwin)
	else
		icu_cv_host_frag=mh-cygwin-msvc
	fi ;;
*-*-*bsd*|*-*-dragonfly*) 	icu_cv_host_frag=mh-bsd-gcc ;;
*-*-aix*)
	if test "$GCC" = yes; then
		icu_cv_host_frag=mh-aix-gcc
	else
		icu_cv_host_frag=mh-aix-va
	fi ;;
*-*-hpux*)
	if test "$GCC" = yes; then
		icu_cv_host_frag=mh-hpux-gcc
	else
		case "$CXX" in
		*aCC)    icu_cv_host_frag=mh-hpux-acc ;;
		esac
	fi ;;
*-*ibm-openedition*|*-*-os390*)	icu_cv_host_frag=mh-os390 ;;
*-*-os400*)	icu_cv_host_frag=mh-os400 ;;
*-apple-rhapsody*)	icu_cv_host_frag=mh-darwin ;;
*-apple-darwin*)	icu_cv_host_frag=mh-darwin ;;
*-*-beos)	icu_cv_host_frag=mh-beos ;;
*-*-irix*)	icu_cv_host_frag=mh-irix ;;
*-dec-osf*) icu_cv_host_frag=mh-alpha-osf ;;
*-*-nto*)	icu_cv_host_frag=mh-qnx ;;
*-ncr-*)	icu_cv_host_frag=mh-mpras ;;
*) 		icu_cv_host_frag=mh-unknown ;;
esac
		]
	)
])

dnl ICU_CONDITIONAL - similar example taken from Automake 1.4
AC_DEFUN(ICU_CONDITIONAL,
[AC_SUBST($1_TRUE)
if $2; then
  $1_TRUE=
else
  $1_TRUE='#'
fi])

dnl ICU_PROG_LINK - Make sure that the linker is usable
AC_DEFUN(ICU_PROG_LINK,
[
case "${host}" in
    *-*-cygwin*|*-*-mingw*)
        if test "$GCC" != yes && test -n "`link --version 2>&1 | grep 'GNU coreutils'`"; then
            AC_MSG_ERROR([link.exe is not a valid linker. Your PATH is incorrect.
                  Please follow the directions in ICU's readme.])
        fi;;
    *);;
esac])

dnl AC_SEARCH_LIBS_FIRST(FUNCTION, SEARCH-LIBS [, ACTION-IF-FOUND
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])
dnl Search for a library defining FUNC, then see if it's not already available.

AC_DEFUN(AC_SEARCH_LIBS_FIRST,
[AC_PREREQ([2.13])
AC_CACHE_CHECK([for library containing $1], [ac_cv_search_$1],
[ac_func_search_save_LIBS="$LIBS"
ac_cv_search_$1="no"
for i in $2; do
LIBS="-l$i $5 $ac_func_search_save_LIBS"
AC_TRY_LINK_FUNC([$1],
[ac_cv_search_$1="-l$i"
break])
done
if test "$ac_cv_search_$1" = "no"; then
AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="none required"])
fi
LIBS="$ac_func_search_save_LIBS"])
if test "$ac_cv_search_$1" != "no"; then
  test "$ac_cv_search_$1" = "none required" || LIBS="$ac_cv_search_$1 $LIBS"
  $3
else :
  $4
fi])

dnl Check if we can build and use 64-bit libraries
AC_DEFUN(AC_CHECK_64BIT_LIBS,
[
    AC_ARG_ENABLE(64bit-libs,
        [  --enable-64bit-libs     build 64-bit libraries [default=yes]],
        [ENABLE_64BIT_LIBS=${enableval}],
        [ENABLE_64BIT_LIBS=yes]
    )
    dnl These results can't be cached because is sets compiler flags.
    AC_MSG_CHECKING([for 64-bit executable support])
    if test "$ENABLE_64BIT_LIBS" != no; then
        if test "$GCC" = yes; then
            dnl First we check that gcc already compiles as 64-bit
            if test -n "`$CXX -dumpspecs 2>&1 && $CC -dumpspecs 2>&1 | grep -v __LP64__`"; then
                ENABLE_64BIT_LIBS=yes
            else
                dnl Now we check a little more forcefully.
                dnl Maybe the compiler builds as 32-bit on a 64-bit machine.
                OLD_CFLAGS="${CFLAGS}"
                OLD_CXXFLAGS="${CXXFLAGS}"
                CFLAGS="${CFLAGS} -m64"
                CXXFLAGS="${CXXFLAGS} -m64"
                AC_TRY_RUN(int main(void) {return 0;},
                   ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=no)
                if test "$ENABLE_64BIT_LIBS" = no; then
                    # Nope. We're on a 32-bit machine with a 32-bit compiler.
                    CFLAGS="${OLD_CFLAGS}"
                    CXXFLAGS="${OLD_CXXFLAGS}"
                fi
            fi
        else
            case "${host}" in
            sparc*-*-solaris*)
                SPARCV9=`isainfo -n 2>&1 | grep sparcv9`
                SOL64=`$CXX -xarch=v9 2>&1 && $CC -xarch=v9 2>&1 | grep -v usage:`
                if test -z "$SOL64" && test -n "$SPARCV9"; then
                    CFLAGS="${CFLAGS} -xtarget=ultra -xarch=v9"
                    CXXFLAGS="${CXXFLAGS} -xtarget=ultra -xarch=v9"
                    LDFLAGS="${LDFLAGS} -xtarget=ultra -xarch=v9"
                    ENABLE_64BIT_LIBS=yes
                else
                    ENABLE_64BIT_LIBS=no
                fi
                ;;
            i386-*-solaris*)
                AMD64=`isainfo -n 2>&1 | grep amd64`
                # The new compiler option
                SOL64=`$CXX -m64 2>&1 && $CC -m64 2>&1 | grep -v usage:`
                if test -z "$SOL64" && test -n "$AMD64"; then
                    CFLAGS="${CFLAGS} -m64"
                    CXXFLAGS="${CXXFLAGS} -m64"
                    ENABLE_64BIT_LIBS=yes
                else
                    # The older compiler option
                    SOL64=`$CXX -xtarget=generic64 2>&1 && $CC -xtarget=generic64 2>&1 | grep -v usage:`
                    if test -z "$SOL64" && test -n "$AMD64"; then
                        CFLAGS="${CFLAGS} -xtarget=generic64"
                        CXXFLAGS="${CXXFLAGS} -xtarget=generic64"
                        ENABLE_64BIT_LIBS=yes
                    else
                        ENABLE_64BIT_LIBS=no
                    fi
                fi
                ;;
            ia64-*-linux*)
                # check for ecc/ecpc compiler support
                if test -n "`$CXX --help 2>&1 && $CC --help 2>&1 | grep -v Intel`"; then
                    if test -n "`$CXX --help 2>&1 && $CC --help 2>&1 | grep -v Itanium`"; then
                        ENABLE_64BIT_LIBS=yes
                    else
                        ENABLE_64BIT_LIBS=no
                    fi
                else
                    # unknown
                    ENABLE_64BIT_LIBS=no
                fi
                ;;
            *-*-cygwin)
                dnl vcvarsamd64.bat should have been used to enable 64-bit builds.
                dnl We only do this check to display the correct answer.
                if test -n "`$CXX -help 2>&1 | grep 'for x64'`"; then
                    ENABLE_64BIT_LIBS=yes
                else
                    # unknown
                    ENABLE_64BIT_LIBS=no
                fi
                ;;
            *-*-aix*|powerpc64-*-linux*)
                OLD_CFLAGS="${CFLAGS}"
                OLD_CXXFLAGS="${CXXFLAGS}"
                OLD_LDFLAGS="${LDFLAGS}"
                CFLAGS="${CFLAGS} -q64"
                CXXFLAGS="${CXXFLAGS} -q64"
                LDFLAGS="${LDFLAGS} -q64"
                AC_TRY_RUN(int main(void) {return 0;},
                   ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=no)
                if test "$ENABLE_64BIT_LIBS" = no; then
                    CFLAGS="${OLD_CFLAGS}"
                    CXXFLAGS="${OLD_CXXFLAGS}"
                    LDFLAGS="${OLD_LDFLAGS}"
                else
                    case "${host}" in
                    *-*-aix*)
                        ARFLAGS="${ARFLAGS} -X64"
                    esac
                fi
                ;;
            *-*-hpux*)
                dnl First we try the newer +DD64, if that doesn't work,
                dnl try other options.

                OLD_CFLAGS="${CFLAGS}"
                OLD_CXXFLAGS="${CXXFLAGS}"
                CFLAGS="${CFLAGS} +DD64"
                CXXFLAGS="${CXXFLAGS} +DD64"
                AC_TRY_RUN(int main(void) {return 0;},
                    ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=no)
                if test "$ENABLE_64BIT_LIBS" = no; then
                    CFLAGS="${OLD_CFLAGS}"
                    CXXFLAGS="${OLD_CXXFLAGS}"
                    CFLAGS="${CFLAGS} +DA2.0W"
                    CXXFLAGS="${CXXFLAGS} +DA2.0W"
                    AC_TRY_RUN(int main(void) {return 0;},
                        ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=no)
                    if test "$ENABLE_64BIT_LIBS" = no; then
                        CFLAGS="${OLD_CFLAGS}"
                        CXXFLAGS="${OLD_CXXFLAGS}"
                    fi
                fi
                ;;
            *-*ibm-openedition*|*-*-os390*)
                OLD_CFLAGS="${CFLAGS}"
                OLD_CXXFLAGS="${CXXFLAGS}"
                OLD_LDFLAGS="${LDFLAGS}"
                CFLAGS="${CFLAGS} -Wc,lp64"
                CXXFLAGS="${CXXFLAGS} -Wc,lp64"
                LDFLAGS="${LDFLAGS} -Wl,lp64"
                AC_TRY_RUN(int main(void) {return 0;},
                   ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=no)
                if test "$ENABLE_64BIT_LIBS" = no; then
                    CFLAGS="${OLD_CFLAGS}"
                    CXXFLAGS="${OLD_CXXFLAGS}"
                    LDFLAGS="${OLD_LDFLAGS}"
                fi
                ;;
            *)
                ENABLE_64BIT_LIBS=no
                ;;
            esac
        fi
    else
        if test "$GCC" = yes; then
            OLD_CFLAGS="${CFLAGS}"
            OLD_CXXFLAGS="${CXXFLAGS}"
            CFLAGS="${CFLAGS} -m32"
            CXXFLAGS="${CXXFLAGS} -m32"
            AC_TRY_RUN(int main(void) {return 0;},
               ENABLE_64BIT_LIBS=no, ENABLE_64BIT_LIBS=yes, ENABLE_64BIT_LIBS=yes)
            if test "$ENABLE_64BIT_LIBS" = yes; then
                CFLAGS="${OLD_CFLAGS}"
                CXXFLAGS="${OLD_CXXFLAGS}"
            fi
        fi
    fi
    dnl Individual tests that fail should reset their own flags.
    AC_MSG_RESULT($ENABLE_64BIT_LIBS)
])

dnl Strict compilation options.
AC_DEFUN(AC_CHECK_STRICT_COMPILE,
[
    AC_MSG_CHECKING([whether strict compiling is on])
    AC_ARG_ENABLE(strict,[  --enable-strict         compile with strict compiler options [default=no]], [
        if test "$enableval" = no
        then
            ac_use_strict_options=no
        else
            ac_use_strict_options=yes
        fi
      ], [ac_use_strict_options=no])
    AC_MSG_RESULT($ac_use_strict_options)

    if test "$ac_use_strict_options" = yes
    then
        if test "$GCC" = yes
        then
            CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings -Wno-long-long"
            case "${host}" in
            *-*-solaris*)
                CFLAGS="$CFLAGS -D__STDC__=0";;
            esac
        else
            case "${host}" in
            *-*-cygwin)
                if test "`$CC /help 2>&1 | head -c9`" = "Microsoft"
                then
                    CFLAGS="$CFLAGS /W4"
                fi
            esac
        fi
        if test "$GXX" = yes
        then
            CXXFLAGS="$CXXFLAGS -W -Wall -ansi -pedantic -Wpointer-arith -Wwrite-strings -Wno-long-long"
            case "${host}" in
            *-*-solaris*)
                CXXFLAGS="$CXXFLAGS -D__STDC__=0";;
            esac
        else
            case "${host}" in
            *-*-cygwin)
                if test "`$CXX /help 2>&1 | head -c9`" = "Microsoft"
                then
                    CXXFLAGS="$CXXFLAGS /W4"
                fi
            esac
        fi
    fi
])


