srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(
cd $srcdir
cat macros/gnome.m4 mc-aclocal.m4 gettext.m4 > aclocal.m4
autoheader
autoconf
)

$srcdir/configure $*
