#!/bin/sh

# Copyright 2005 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

run ()
{
  echo "running \`$*'"
  eval $*

  if test $? != 0 ; then
    echo "error while running \`$*'"
    exit 1
  fi
}

if test ! -f ./builds/unix/configure.ac; then
  echo "You must be in the same directory as \`autogen.sh'."
  echo "Bootstrapping doesn't work if srcdir != builddir."
  exit 1
fi

cd builds/unix

run aclocal -I . --force
run libtoolize --force --copy
run autoconf --force

chmod +x mkinstalldirs
chmod +x install-sh

cd ../..

chmod +x ./configure

# EOF
