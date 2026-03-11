#!/bin/sh

if [ "x$ROS_ARCH" = "x" ]; then
	echo "Could not detect RosBE."
	exit 1
fi

BUILD_ENVIRONMENT=MinGW
ARCH=$ROS_ARCH
REACTOS_SOURCE_DIR=$(cd `dirname $0` && pwd)
TOOLCHAIN_FILE=toolchain-gcc.cmake

usage() {
	echo "Invalid parameter given."
	echo "Usage: configure.sh [options]"
	echo "  clang       - Use Clang/LLVM compiler (default: GCC/MinGW)"
	echo "  makefiles   - Use Unix Makefiles generator (default: Ninja)"
	echo "  -D<var>=<val> - Pass option to CMake"
	exit 1
}

CMAKE_GENERATOR="Ninja"
while [ $# -gt 0 ]; do
	case $1 in
		-D)
			shift
			if echo "x$1" | grep 'x?*=*' > /dev/null; then
				ROS_CMAKEOPTS=$ROS_CMAKEOPTS" -D $1"
			else
				usage
			fi
		;;

		-D?*=*|-D?*)
			ROS_CMAKEOPTS=$ROS_CMAKEOPTS" $1"
		;;
		makefiles|Makefiles)
			CMAKE_GENERATOR="Unix Makefiles"
		;;
		clang|Clang)
			BUILD_ENVIRONMENT=Clang
			TOOLCHAIN_FILE=sdk/cmake/clang.cmake
		;;
		*)
			usage
	esac

	shift
done

REACTOS_OUTPUT_PATH=output-$BUILD_ENVIRONMENT-$ARCH

echo "Configuring a new ReactOS build on:"
echo $(uname -srvpio); echo

if [ "$REACTOS_SOURCE_DIR" = "$PWD" ]; then
	echo "Creating directories in $REACTOS_OUTPUT_PATH"
	mkdir -p "$REACTOS_OUTPUT_PATH"
	cd "$REACTOS_OUTPUT_PATH"
fi

rm -f CMakeCache.txt host-tools/CMakeCache.txt

cmake -G "$CMAKE_GENERATOR" -DENABLE_CCACHE:BOOL=0 -DCMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN_FILE -DARCH:STRING=$ARCH $EXTRA_ARGS $ROS_CMAKEOPTS "$REACTOS_SOURCE_DIR"
if [ $? -ne 0 ]; then
    echo "An error occurred while configuring ReactOS"
    exit 1
fi

echo "Configure script complete! Execute appropriate build commands (e.g. ninja, make, makex, etc.) from $REACTOS_OUTPUT_PATH"
