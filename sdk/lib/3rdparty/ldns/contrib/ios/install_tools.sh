#!/usr/bin/env bash

# This step should install tools needed for all packages - OpenSSL and LDNS
# When running on Travis, Homebrew fails in unusual ways, hence '|| true'.
# https://travis-ci.community/t/homebrew-fails-because-an-automake-update-is-an-error/7831/3
echo "Updating tools"
brew update 1>/dev/null || true
echo "Installing tools"
brew install autoconf automake libtool pkg-config curl perl 1>/dev/null || true
