#! /bin/sh

#--------------------------------------------------------------------------
# autoconf 2.52 or newer
#
ac_version="`${AUTOCONF:-autoconf} --version 2> /dev/null | head -1 | sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`"
if test -z "$ac_version"; then
  echo "ERROR: autoconf not found."
  echo "       You need autoconf version 2.52 or newer installed."
  exit 1
fi
IFS=.; set $ac_version; IFS=' '
if test "$1" = "2" -a "$2" -lt "52" || test "$1" -lt "2"; then
  echo "ERROR: autoconf version $ac_version found."
  echo "       You need autoconf version 2.52 or newer installed."
  exit 1
fi

echo "found: autoconf version $ac_version (ok)"

#--------------------------------------------------------------------------
# libtool 1.4 or newer
#

#
# find libtoolize, or glibtoolize on MacOS X
#
libtoolize=`conftools/PrintPath glibtoolize libtoolize`
if [ "x$libtoolize" = "x" ]; then
  echo "ERROR: libtoolize not found."
  echo "       You need libtool version 1.4 or newer installed"
  exit 1
fi

lt_pversion="`$libtoolize --version 2> /dev/null | sed -e 's/^[^0-9]*//'`"

# convert something like 1.4p1 to 1.4.p1
lt_version="`echo $lt_pversion | sed -e 's/\([a-z]*\)$/.\1/'`"

IFS=.; set $lt_version; IFS=' '
if test "$1" = "1" -a "$2" -lt "4"; then
  echo "ERROR: libtool version $lt_pversion found."
  echo "       You need libtool version 1.4 or newer installed"
  exit 1
fi

echo "found: libtool version $lt_pversion (ok)"

#--------------------------------------------------------------------------

# Remove any libtool files so one can switch between libtool 1.3
# and libtool 1.4 by simply rerunning the buildconf script.
(cd conftools/; rm -f ltmain.sh ltconfig)

#
# Create the libtool helper files
#
echo "Copying libtool helper files ..."

#
# Note: we don't use --force (any more) since we have a special
# config.guess/config.sub that we want to ensure is used.
#
# --copy to avoid symlinks; we want originals for the distro
# --automake to make it shut up about "things to do"
#
$libtoolize --copy --automake

#
# Find the libtool.m4 file. The developer/packager can set the LIBTOOL_M4
# environment variable to specify its location. If that variable is not
# set, then we'll assume a "standard" libtool installation and try to
# derive its location.
#
ltpath=`dirname $libtoolize`
ltfile=${LIBTOOL_M4-`cd $ltpath/../share/aclocal ; pwd`/libtool.m4}
cp $ltfile conftools/libtool.m4

echo "Using libtool.m4 from ${ltfile}."

#--------------------------------------------------------------------------

### for a little while... remove stray aclocal.m4 files from
### developers' working copies. we no longer use it. (nothing else
### will remove it, and leaving it creates big problems)
rm -f aclocal.m4

#
# Generate the autoconf header template (expat_config.h.in) and ./configure
#
echo "Creating expat_config.h.in ..."
${AUTOHEADER:-autoheader}

echo "Creating configure ..."
### do some work to toss config.cache?
${AUTOCONF:-autoconf}

# toss this; it gets created by autoconf on some systems
rm -rf autom4te*.cache

# exit with the right value, so any calling script can continue
exit 0
