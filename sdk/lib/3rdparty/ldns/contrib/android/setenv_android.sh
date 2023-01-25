#!/usr/bin/env bash

# ====================================================================
# Sets the cross compile environment for Android
#
# Based upon OpenSSL's setenv-android.sh by TH, JW, and SM.
# Heavily modified by JWW for Crypto++.
# Updated by Skycoder42 for current recommendations for Android.
# Modified by JWW for LDNS.
# ====================================================================

#########################################
#####        Some validation        #####
#########################################

if [ -z "$ANDROID_API" ]; then
    echo "ANDROID_API is not set. Please set it"
    [[ "$0" = "${BASH_SOURCE[0]}" ]] && exit 1 || return 1
fi

if [ -z "$ANDROID_CPU" ]; then
    echo "ANDROID_CPU is not set. Please set it"
    [[ "$0" = "${BASH_SOURCE[0]}" ]] && exit 1 || return 1
fi

if [ ! -d "$ANDROID_NDK_ROOT" ]; then
    echo "ERROR: ANDROID_NDK_ROOT is not a valid path. Please set it."
    echo "NDK root is $ANDROID_NDK_ROOT"
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# cryptest-android.sh may run this script without sourcing.
if [ "$0" = "${BASH_SOURCE[0]}" ]; then
    echo "setenv-android.sh is usually sourced, but not this time."
fi

#####################################################################

# Need to set THIS_HOST to darwin-x86_64, linux-x86_64,
# windows, or windows-x86_64

if [[ "$(uname -s | grep -i -c darwin)" -ne 0 ]]; then
    THIS_HOST=darwin-x86_64
elif [[ "$(uname -s | grep -i -c linux)" -ne 0 ]]; then
    THIS_HOST=linux-x86_64
else
    echo "ERROR: Unknown host"
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

ANDROID_TOOLCHAIN="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/$THIS_HOST/bin"
ANDROID_SYSROOT="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/$THIS_HOST/sysroot"

# Error checking
if [ ! -d "$ANDROID_TOOLCHAIN" ]; then
    echo "ERROR: ANDROID_TOOLCHAIN is not a valid path. Please set it."
    echo "Path is $ANDROID_TOOLCHAIN"
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -d "$ANDROID_SYSROOT" ]; then
    echo "ERROR: ANDROID_SYSROOT is not a valid path. Please set it."
    echo "Path is $ANDROID_SYSROOT"
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

#####################################################################

THE_ARCH=$(tr '[:upper:]' '[:lower:]' <<< "$ANDROID_CPU")

# https://developer.android.com/ndk/guides/abis.html
case "$THE_ARCH" in
  armv7*|armeabi*)
    CC="armv7a-linux-androideabi$ANDROID_API-clang"
    CXX="armv7a-linux-androideabi$ANDROID_API-clang++"
    LD="arm-linux-androideabi-ld"
    AS="arm-linux-androideabi-as"
    AR="arm-linux-androideabi-ar"
    RANLIB="arm-linux-androideabi-ranlib"
    STRIP="arm-linux-androideabi-strip"

    CFLAGS="-march=armv7-a -mthumb -mfloat-abi=softfp -funwind-tables -fexceptions"
    CXXFLAGS="-march=armv7-a -mthumb -mfloat-abi=softfp -funwind-tables -fexceptions -frtti"
    ;;

  armv8*|aarch64|arm64)
    CC="aarch64-linux-android$ANDROID_API-clang"
    CXX="aarch64-linux-android$ANDROID_API-clang++"
    LD="aarch64-linux-android-ld"
    AS="aarch64-linux-android-as"
    AR="aarch64-linux-android-ar"
    RANLIB="aarch64-linux-android-ranlib"
    STRIP="aarch64-linux-android-strip"

    CFLAGS="-funwind-tables -fexceptions"
    CXXFLAGS="-funwind-tables -fexceptions -frtti"
    ;;

  x86)
    CC="i686-linux-android$ANDROID_API-clang"
    CXX="i686-linux-android$ANDROID_API-clang++"
    LD="i686-linux-android-ld"
    AS="i686-linux-android-as"
    AR="i686-linux-android-ar"
    RANLIB="i686-linux-android-ranlib"
    STRIP="i686-linux-android-strip"

    CFLAGS="-mtune=intel -mssse3 -mfpmath=sse -funwind-tables -fexceptions"
    CXXFLAGS="-mtune=intel -mssse3 -mfpmath=sse -funwind-tables -fexceptions -frtti"
    ;;

  x86_64|x64)
    CC="x86_64-linux-android$ANDROID_API-clang"
    CXX="x86_64-linux-android$ANDROID_API-clang++"
    LD="x86_64-linux-android-ld"
    AS="x86_64-linux-android-as"
    AR="x86_64-linux-android-ar"
    RANLIB="x86_64-linux-android-ranlib"
    STRIP="x86_64-linux-android-strip"

    CFLAGS="-march=x86-64 -msse4.2 -mpopcnt -mtune=intel -funwind-tables -fexceptions"
    CXXFLAGS="-march=x86-64 -msse4.2 -mpopcnt -mtune=intel -funwind-tables -fexceptions -frtti"
    ;;

  *)
    echo "ERROR: Unknown architecture $ANDROID_CPU"
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
    ;;
esac

#####################################################################

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$CC" ]; then
    echo "ERROR: Failed to find Android clang. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$CXX" ]; then
    echo "ERROR: Failed to find Android clang++. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$RANLIB" ]; then
    echo "ERROR: Failed to find Android ranlib. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$AR" ]; then
    echo "ERROR: Failed to find Android ar. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$AS" ]; then
    echo "ERROR: Failed to find Android as. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

# Error checking
if [ ! -e "$ANDROID_TOOLCHAIN/$LD" ]; then
    echo "ERROR: Failed to find Android ld. Please edit this script."
    [ "$0" = "${BASH_SOURCE[0]}" ] && exit 1 || return 1
fi

#####################################################################

LENGTH=${#ANDROID_TOOLCHAIN}
SUBSTR=${PATH:0:$LENGTH}
if [ "$SUBSTR" != "$ANDROID_TOOLCHAIN" ]; then
    export PATH="$ANDROID_TOOLCHAIN:$PATH"
fi

#####################################################################

export CPP CC CXX LD AS AR RANLIB STRIP
export ANDROID_SYSROOT="$AOSP_SYSROOT"
export CPPFLAGS="-D__ANDROID_API__=$ANDROID_API"
export CFLAGS="$CFLAGS --sysroot=$AOSP_SYSROOT"
export CXXFLAGS="$CXXFLAGS -stdlib=libc++ --sysroot=$AOSP_SYSROOT"

#####################################################################

echo "ANDROID_TOOLCHAIN: $ANDROID_TOOLCHAIN"

echo "CPP: $(command -v "$CPP")"
echo "CC: $(command -v "$CC")"
echo "CXX: $(command -v "$CXX")"
echo "LD: $(command -v "$LD")"
echo "AS: $(command -v "$AS")"
echo "AR: $(command -v "$AR")"

echo "ANDROID_SYSROOT: $ANDROID_SYSROOT"

echo "CPPFLAGS: $CPPFLAGS"
echo "CFLAGS: $CFLAGS"
echo "CXXFLAGS: $CXXFLAGS"

[ "$0" = "${BASH_SOURCE[0]}" ] && exit 0 || return 0
