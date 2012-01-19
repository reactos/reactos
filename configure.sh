#!/bin/bash
if [ "x$ROS_ARCH" == "x" ]
then
  echo Could not detect RosBE.
  exit 1
fi

BUILD_ENVIRONMENT=MinGW
ARCH=$ROS_ARCH
REACTOS_SOURCE_DIR=$(cd `dirname $0` && pwd)
REACTOS_OUTPUT_PATH=output-$BUILD_ENVIRONMENT-$ARCH

if [ "$REACTOS_SOURCE_DIR" == "$PWD" ]
then
  echo Creating directories in $REACTOS_OUTPUT_PATH
  mkdir -p $REACTOS_OUTPUT_PATH
  cd "$REACTOS_OUTPUT_PATH"
fi

mkdir -p host-tools
mkdir -p reactos

echo Preparing host tools...
cd host-tools
if [ -f CMakeCache.txt ]
then
  rm -f CMakeCache.txt
fi

REACTOS_BUILD_TOOLS_DIR="$PWD"
cmake -G "Unix Makefiles" -DARCH=$ARCH "$REACTOS_SOURCE_DIR"

echo Preparing reactos...
cd ../reactos
if [ -f CMakeCache.txt ]
then
  rm -f CMakeCache.txt
fi

cmake -G "Unix Makefiles" -DENABLE_CCACHE=0 -DPCH=0 -DCMAKE_TOOLCHAIN_FILE=toolchain-gcc.cmake -DARCH=$ARCH -DREACTOS_BUILD_TOOLS_DIR="$REACTOS_BUILD_TOOLS_DIR" "$REACTOS_SOURCE_DIR"

echo Configure script complete! Enter directories and execute appropriate build commands\(ex: make, makex, etc...\).

