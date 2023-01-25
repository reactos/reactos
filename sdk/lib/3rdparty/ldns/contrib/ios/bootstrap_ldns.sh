#!/usr/bin/env bash

if ! git submodule update --init; then
    echo "Failed to init submodule"
    exit 1
fi

echo "AUTOTOOLS_BUILD: $AUTOTOOLS_BUILD"
echo "AUTOTOOLS_HOST:  $AUTOTOOLS_HOST"

# libtool complains about our updated config.guess and config.sub.
# Remove them to get through bootstrap. Re-add them after libtoolize.

echo "Running libtoolize"
if [ -n "$(command -v glibtoolize)" ]; then
    rm -f config.guess config.sub
    if ! glibtoolize -ci ; then
        echo "Failed to libtoolize (glibtoolize)"
        exit 1
    fi
elif [ -n "$(command -v libtoolize)" ]; then
    rm -f config.guess config.sub
    if ! libtoolize -ci ; then
        echo "Failed to libtoolize (libtoolize)"
        exit 1
    fi
else
    echo "Failed to find a libtool"
    exit 1
fi

echo "Updating config.guess"
if ! wget -q -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'; then
    echo "Failed to download config.guess"
fi

echo "Updating config.sub"
if ! wget -q -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'; then
    echo "Failed to download config.sub"
fi

echo "Fixing config permissions"
chmod a+x config.guess config.sub
if [ -n "$(command -v xattr 2>/dev/null)" ]; then
    xattr -d com.apple.quarantine config.guess 2>/dev/null
    xattr -d com.apple.quarantine config.sub 2>/dev/null
fi

echo "Running autoreconf"
if ! autoreconf -fi ; then
    echo "Failed to autoreconf"
    exit 1
fi

exit 0
