#!/usr/bin/env bash

# compile ldns for windows
cdir="$(echo ldns.win.$$)"
tmpdir=$(pwd)
mkdir "$cdir"
cd "$cdir"
#configure="mingw32-configure"
#strip="i686-w64-mingw32-strip"
#warch="i686"
configure="mingw64-configure"
strip="x86_64-w64-mingw32-strip"
warch="x86_64"
WINSSL="$HOME/Downloads/openssl-1.1.0h.tar.gz"
cross_flag=""
cross_flag_nonstatic=""
RC="no"
SNAPSHOT="no"
CHECKOUT=""
# the destination is a zipfile in the start directory ldns-a.b.c.zip
# the start directory is a git repository, and it is copied to build from.

info () {
	echo "info: $1"
}

error_cleanup () {
	echo "$1"
	cd "$tmpdir"
	rm -rf "$cdir"
	exit 1
}

replace_text () {
    (cp "$1" "$1".orig && \
        sed -e "s/$2/$3/g" < "$1".orig > "$1" && \
        rm "$1".orig) || error_cleanup "Replacement for $1 failed."
}

# Parse command line arguments
while [ "$1" ]; do
    case "$1" in
	"-h")
		echo "Compile a zip file with static executables, and"
		echo "dynamic library, static library, include dir and"
		echo "manual pages."
		echo ""
		echo "	-h		This usage information."
		echo "	-s		snapshot, current date appended to version"
		echo "	-rc <nr>	release candidate, the number is added to version"
		echo "			ldns-<version>rc<nr>."
		echo "	-c <tag/br>	Checkout this tag or branch, (defaults to current"
		echo "			branch)."
		echo "	-wssl <file>	Pass openssl.tar.gz file, use absolute path."
		echo ""
		exit 1
		;;
	"-c")
		CHECKOUT="$2"
		shift
		;;
	"-s")
		SNAPSHOT="yes"
		;;
	"-rc")
		RC="$2"
		shift
		;;
	"-wssl")
		WINSSL="$2"
		shift
		;;
	*)
		error_cleanup "Unrecognized argument -- $1"
		;;
    esac
    shift
done
if [ -z "$CHECKOUT" ]
then
        if [ "$RC" = "no" ]
        then
                CHECKOUT=$( (git status | head -n 1 | awk '{print$3}') || echo master)
        else
                CHECKOUT=$( (git status | head -n 1 | awk '{print$3}') || echo develop)
        fi
fi

# this script creates a temp directory $cdir.
# this directory contains subdirectories:
# ldns/ : ldns source compiled
# openssl-a.b.c/ : the openSSL source compiled
# ldnsinstall/ : install of ldns here.
# sslinstall/ : install of ssl here.
# file/ : directory to gather the components of the zipfile distribution
# ldns-nonstatic/ : ldns source compiled nonstatic
# ldnsinstall-nonstatic/ : install of ldns nonstatic compile
# openssl-nonstatic/ : nonstatic openssl source compiled
# sslinstall-nonstatic/ : install of nonstatic openssl compile

info "exporting source into $cdir/ldns"
git clone git://git.nlnetlabs.nl/ldns/ ldns || error_cleanup "git command failed"
(cd ldns; git checkout "$CHECKOUT") || error_cleanup "Could not checkout $CHECKOUT"
#svn export . $cdir/ldns
info "exporting source into $cdir/ldns-nonstatic"
git clone git://git.nlnetlabs.nl/ldns/ ldns-nonstatic || error_cleanup "git command failed"
(cd ldns-nonstatic; git checkout "$CHECKOUT") || error_cleanup "Could not checkout $CHECKOUT"
#svn export . $cdir/ldns-nonstatic

# Fix up the version number if necessary
(cd ldns; if test ! -f install-sh -a -f ../../install-sh; then cp ../../install-sh . ; fi; libtoolize -ci; autoreconf -fi)
version=$(./ldns/configure --version | head -1 | awk '{ print $3 }') || \
    error_cleanup "Cannot determine version number."
info "LDNS version: $version"
if [ "$RC" != "no" ]; then
	info "Building LDNS release candidate $RC."
	version2="${version}-rc$RC"
	info "Version number: $version2"
	replace_text "ldns/configure.ac" "AC_INIT(ldns, $version" "AC_INIT(ldns, $version2"
	replace_text "ldns-nonstatic/configure.ac" "AC_INIT(ldns, $version" "AC_INIT(ldns, $version2"
	version="$version2"
fi
if [ "$SNAPSHOT" = "yes" ]; then
	info "Building LDNS snapshot."
	version2="${version}_$(date +%Y%m%d)"
	info "Snapshot version number: $version2"
	replace_text "ldns/configure.ac" "AC_INIT(ldns, $version" "AC_INIT(ldns, $version2"
	replace_text "ldns-nonstatic/configure.ac" "AC_INIT(ldns, $version" "AC_INIT(ldns, $version2"
	version="$version2"
fi

# Build OpenSSL
gzip -cd "$WINSSL" | tar xf - || error_cleanup "tar unpack of $WINSSL failed"
sslinstall="$(pwd)/sslinstall"
cd openssl-* || error_cleanup "no openssl-X dir in tarball"
if test $configure = "mingw64-configure"; then
	sslflags="no-shared no-asm -DOPENSSL_NO_CAPIENG mingw64"
else
	sslflags="no-shared no-asm -DOPENSSL_NO_CAPIENG mingw"
fi
info "winssl: Configure $sslflags"
CC="${warch}-w64-mingw32-gcc" AR="${warch}-w64-mingw32-ar" RANLIB="${warch}-w64-mingw32-ranlib" WINDRES="${warch}-w64-mingw32-windres" ./Configure --prefix="$sslinstall" "$sslflags" || error_cleanup "OpenSSL Configure failed"
info "winssl: make"
make || error_cleanup "make failed for $WINSSL"
info "winssl: make install_sw"
make install_sw || error_cleanup "OpenSSL install failed"
cross_flag="$cross_flag --with-ssl=$sslinstall"
cd ..

# Build ldns
ldnsinstall="$(pwd)/ldnsinstall"
cd ldns
info "ldns: autoconf"
# cp install-sh because one at ../.. means libtoolize won't install it for us.
if test ! -f install-sh -a -f ../../install-sh; then cp ../../install-sh . ; fi
libtoolize -ci
autoreconf -fi
ldns_flag="--with-examples --with-drill"
info "ldns: Configure $cross_flag $ldns_flag"
$configure "$cross_flag" "$ldns_flag" || error_cleanup "ldns configure failed"
info "ldns: make"
make || error_cleanup "ldns make failed"
# do not strip debug symbols, could be useful for stack traces
# $strip lib/*.dll || error_cleanup "cannot strip ldns dll"
make doc || error_cleanup "ldns make doc failed"
DESTDIR=$ldnsinstall make install || error_cleanup "ldns make install failed"
cd ..

# Build OpenSSL nonstatic
sslinstallnonstatic="$(pwd)/sslinstallnonstatic"
mkdir openssl-nonstatic
cd openssl-nonstatic
# remove openssl-a.b.c/ and put in openssl-nonstatic directory
gzip -cd "$WINSSL" | tar xf - --strip-components=1 || error_cleanup "tar unpack of $WINSSL failed"
if test "$configure" = "mingw64-configure"; then
	sslflags_nonstatic="shared no-asm -DOPENSSL_NO_CAPIENG mingw64"
else
	sslflags_nonstatic="shared no-asm -DOPENSSL_NO_CAPIENG mingw"
fi
info "winsslnonstatic: Configure $sslflags_nonstatic"
CC="${warch}-w64-mingw32-gcc" AR="${warch}-w64-mingw32-ar" RANLIB="${warch}-w64-mingw32-ranlib" WINDRES="${warch}-w64-mingw32-windres" ./Configure --prefix="$sslinstallnonstatic" "$sslflags_nonstatic" || error_cleanup "OpenSSL Configure failed"
info "winsslnonstatic: make"
make || error_cleanup "make failed for $WINSSL"
info "winsslnonstatic: make install_sw"
make install_sw || error_cleanup "OpenSSL install failed"
cross_flag_nonstatic="$cross_flag_nonstatic --with-ssl=$sslinstallnonstatic"
cd ..

# Build ldns nonstatic
ldnsinstallnonstatic="$(pwd)/ldnsinstall-nonstatic"
cd ldns-nonstatic
info "ldnsnonstatic: autoconf"
# cp install-sh because one at ../.. means libtoolize won't install it for us.
if test ! -f install-sh -a -f ../../install-sh; then cp ../../install-sh . ; fi
libtoolize -ci
autoreconf -fi
ldns_flag_nonstatic="--with-examples --with-drill"
info "ldnsnonstatic: Configure $cross_flag_nonstatic $ldns_flag_nonstatic"
$configure "$cross_flag_nonstatic" "$ldns_flag_nonstatic" || error_cleanup "ldns configure failed"
info "ldnsnonstatic: make"
make || error_cleanup "ldns make failed"
# do not strip debug symbols, could be useful for stack traces
# $strip lib/*.dll || error_cleanup "cannot strip ldns dll"
make doc || error_cleanup "ldns make doc failed"
DESTDIR=$ldnsinstallnonstatic make install || error_cleanup "ldns make install failed"
cd ..

# create zipfile
file="ldns-$version.zip"
rm -f "$file"
info "Creating $file"
mkdir file
cd file
installplace="$ldnsinstall/usr/$warch-w64-mingw32/sys-root/mingw"
installplacenonstatic="$ldnsinstallnonstatic/usr/$warch-w64-mingw32/sys-root/mingw"
cp "$installplace"/lib/libldns.a .
cp "$installplacenonstatic"/lib/libldns.dll.a .
cp "$installplacenonstatic"/bin/*.dll .
cp "$sslinstallnonstatic"/lib/*.dll.a .
cp "$sslinstallnonstatic"/bin/*.dll .
cp "$sslinstallnonstatic"/lib/engines-*/*.dll .
cp ../ldns/LICENSE .
cp ../ldns/README .
cp ../ldns/Changelog .
info "copy static exe"
for x in "$installplace"/bin/* ; do
	cp "$x" "$(basename "$x").exe"
done
# but the shell script stays a script file
mv ldns-config.exe ldns-config
info "copy include"
mkdir include
mkdir include/ldns
cp "$installplace"/include/ldns/*.h include/ldns/.
info "copy man1"
mkdir man1
cp "$installplace"/share/man/man1/* man1/.
info "copy man3"
mkdir man3
cp "$installplace"/share/man/man3/* man3/.
info "create cat1"
mkdir cat1
for x in man1/*.1; do groff -man -Tascii -Z "$x" | grotty -cbu > cat1/"$(basename "$x" .1).txt"; done
info "create cat3"
mkdir cat3
for x in man3/*.3; do groff -man -Tascii -Z "$x" | grotty -cbu > cat3/"$(basename "$x" .3).txt"; done
rm -f "../../$file"
info "$file contents"
# show contents of directory we are zipping up.
du -s ./*
# zip it
info "zip $file"
zip -r ../../"$file" LICENSE README libldns.a *.dll *.dll.a Changelog *.exe include man1 man3 cat1 cat3
info "Testing $file"
(cd ../.. ; zip -T "$file" ) || error_cleanup "errors in zipfile $file"
cd ..

# cleanup before exit
cd "$tmpdir"
rm -rf "$cdir"
echo "done"
# display
ls -lG "$file"
