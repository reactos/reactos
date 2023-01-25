#!/usr/bin/env bash

if [ -z "$ANDROID_SDK_ROOT" ]; then
    echo "ERROR: ANDROID_SDK_ROOT is not set. Please set it."
    echo "SDK root is $ANDROID_SDK_ROOT"
    exit 1
fi

if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "ERROR: ANDROID_NDK_ROOT is not set. Please set it."
    echo "NDK root is $ANDROID_NDK_ROOT"
    exit 1
fi

echo "Using ANDROID_SDK_ROOT: $ANDROID_SDK_ROOT"
echo "Using ANDROID_NDK_ROOT: $ANDROID_NDK_ROOT"

echo "Downloading SDK"
if ! curl -L -k -s -o "$HOME/android-sdk.zip" https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip;
then
    echo "Failed to download SDK"
    exit 1
fi

echo "Downloading NDK"
if ! curl -L -k -s -o "$HOME/android-ndk.zip" https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip;
then
    echo "Failed to download NDK"
    exit 1
fi

echo "Unpacking SDK to $ANDROID_SDK_ROOT"
if ! unzip -qq "$HOME/android-sdk.zip" -d "$ANDROID_SDK_ROOT";
then
    echo "Failed to unpack SDK"
    exit 1
fi

echo "Unpacking NDK to $ANDROID_NDK_ROOT"
if ! unzip -qq "$HOME/android-ndk.zip" -d "$HOME";
then
    echo "Failed to unpack NDK"
    exit 1
fi

if ! mv "$HOME/android-ndk-r20b" "$ANDROID_NDK_ROOT";
then
    echo "Failed to move $HOME/android-ndk-r20b to $ANDROID_NDK_ROOT"
    exit 1
fi

rm -f "$HOME/android-sdk.zip"
rm -f "$HOME/android-ndk.zip"

# https://stackoverflow.com/a/47028911/608639
touch "$ANDROID_SDK_ROOT/repositories.cfg"

echo "Finished installing SDK and NDK"

exit 0
