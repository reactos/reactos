#!/bin/sh

if [ -f rcmd.c ] ; then
	cd ..
fi
wd=`pwd`
for f in libncftp sio Strn doc ; do
	if [ ! -f "$f" ] && [ ! -d "$f" ] ; then
		echo "Missing directory $f ?" 1>&2
		exit 1
	fi
done

( cd libncftp ; make clean )

TMPDIR=/tmp
if [ "$#" -lt 2 ] ; then
	TARDIR="libncftp"
	STGZFILE="$TARDIR.tar.gz"
else
	TARDIR="$1"
	STGZFILE="$2"
fi

rm -rf $TMPDIR/TAR
mkdir -p -m755 $TMPDIR/TAR/$TARDIR 2>/dev/null

chmod 755 configure sh/* install-sh 2>/dev/null

find . -depth -follow -type f | sed '
/sio\/configure$/d
/Strn\/configure$/d
/\.o$/d
/\.so$/d
/\.a$/d
/\.lib$/d
/\.ncb$/d
/\.pdb$/d
/\.idb$/d
/\.pch$/d
/\.ilk$/d
/\.res$/d
/\.aps$/d
/\.opt$/d
/\.plg$/d
/\.obj$/d
/\.exe$/d
/\.zip$/d
/\.gz$/d
/\.tgz$/d
/\.tar$/d
/\.swp$/d
/\.orig$/d
/\.rej$/d
/\/Makefile\.bin$/p
/\.bin$/d
/\/bin/d
/\/core$/d
/\/^[Rr]elease$/d
/\/^[Dd]ebug$/d
/\/sio\/.*\//d
/shit/d
/\/upload/d
/\/config\.h\.in$/p
/\/config\./d
/\/Makefile$/d
/\/OLD/d
/\/old/d' | cut -c3- | tee "$wd/doc/manifest.txt" | cpio -Lpdm $TMPDIR/TAR/$TARDIR

( cd "$TMPDIR/TAR/$TARDIR" ; ln -s doc/README.txt README )

x=`tar --help 2>&1 | sed -n 's/.*owner=NAME.*/owner=NAME/g;/owner=NAME/p'`
case "$x" in
	*owner=NAME*)
		TARFLAGS="-c --owner=bin --group=bin --verbose -f"
		TAR=tar
		;;
	*)
		TARFLAGS="cvf"
		TAR=tar
		x2=`gtar --help 2>&1 | sed -n 's/.*owner=NAME.*/owner=NAME/g;/owner=NAME/p'`
		case "$x2" in
			*owner=NAME*)
				TARFLAGS="-c --owner=bin --group=bin --verbose -f"
				TAR=gtar
				;;
		esac
		;;
esac

( cd $TMPDIR/TAR ; $TAR $TARFLAGS - $TARDIR | gzip -c > $STGZFILE )
cp $TMPDIR/TAR/$STGZFILE .
chmod 644 $STGZFILE
rm -rf $TMPDIR/TAR
ls -l $STGZFILE 2>/dev/null
mv $TGZFILE newbin/ 2>/dev/null
exit 0
