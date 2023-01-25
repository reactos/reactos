#!/usr/bin/env bash

echo "Downloading OpenSSL"
if ! curl -L -k -s -o openssl-1.1.1d.tar.gz https://www.openssl.org/source/openssl-1.1.1d.tar.gz;
then
    echo "Failed to download OpenSSL"
    exit 1
fi

echo "Unpacking OpenSSL"
rm -rf ./openssl-1.1.1d
if ! tar -xf openssl-1.1.1d.tar.gz;
then
    echo "Failed to unpack OpenSSL"
    exit 1
fi

cd openssl-1.1.1d || exit 1

if ! cp ../contrib/ios/15-ios.conf Configurations/; then
    echo "Failed to copy OpenSSL ios config"
    exit 1
fi

# ocsp.c:947:23: error: 'fork' is unavailable: not available on tvOS
# ocsp.c:978:23: error: 'fork' is unavailable: not available on watchOS
# Also see https://github.com/openssl/openssl/issues/7607.
if ! patch -u -p0 < ../contrib/ios/openssl.patch; then
    echo "Failed to patch OpenSSL"
    exit 1
fi

echo "Configuring OpenSSL"
if ! ./Configure "$OPENSSL_HOST" -DNO_FORK no-comp no-asm no-hw no-engine no-tests no-unit-test \
       --prefix="$IOS_PREFIX" --openssldir="$IOS_PREFIX"; then
    echo "Failed to configure OpenSSL"
    exit 1
fi

echo "Building OpenSSL"
if ! make; then
    echo "Failed to build OpenSSL"
    exit 1
fi

echo "Installing OpenSSL"
if ! make install_sw; then
    echo "Failed to install OpenSSL"
    exit 1
fi

exit 0
