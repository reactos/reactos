#!/usr/bin/env bash

# This step should install tools needed for all packages - OpenSSL and LDNS
echo "Updating tools"
sudo apt-get -qq update
sudo apt-get -qq install --no-install-recommends curl wget tar zip unzip patch perl openjdk-8-jdk autoconf automake libtool pkg-config

# Android builds run config.guess early to determine BUILD and HOST. We need to add config.guess
# and config.sub now. Later, bootstrap_ldns.sh will handle the complete bootstrap of LDNS.

echo "Adding config.guess"
if ! wget -q -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'; then
    echo "Failed to download config.guess"
fi

echo "Adding config.sub"
if ! wget -q -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'; then
    echo "Failed to download config.sub"
fi

echo "Fixing config permissions"
chmod a+x config.guess config.sub
if [ -n "$(command -v xattr 2>/dev/null)" ]; then
    xattr -d com.apple.quarantine config.guess 2>/dev/null
    xattr -d com.apple.quarantine config.sub 2>/dev/null
fi
