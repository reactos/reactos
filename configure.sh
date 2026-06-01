#!/bin/sh

REACTOS_SOURCE_DIR=$(cd `dirname $0` && pwd)

BUILD_ENVIRONMENT=MinGW
TOOLCHAIN_FILE=toolchain-gcc.cmake
CMAKE_GENERATOR="Ninja"
ARCH=$ROS_ARCH

help() {
	cat <<EOF
Help for configure script
Syntax: configure.sh [script-options] [CMake-options]

Script options:
  -h, --help            Show this help and exit
  --clang               Use MinGW Clang (toolchain-clang.cmake)
  --gcc                 Use MinGW GCC (toolchain-gcc.cmake, default)
  -a, --arch <arch>     Target architecture: i386, amd64, arm, arm64
                        (defaults to \$ROS_ARCH from RosBE)
  makefiles             Use the "Unix Makefiles" generator (default: Ninja)

CMake options:
  -D<var>=<val>         Forwarded to CMake
  -D <var>=<val>        Same, with a space
EOF
}

usage() {
	echo "Invalid parameter given. Run 'configure.sh --help' for usage." >&2
	exit 1
}

while [ $# -gt 0 ]; do
	case $1 in
		-h|--help|help)
			help
			exit 0
		;;
		--clang)
			BUILD_ENVIRONMENT=Clang
			TOOLCHAIN_FILE=toolchain-clang.cmake
		;;
		--gcc)
			BUILD_ENVIRONMENT=MinGW
			TOOLCHAIN_FILE=toolchain-gcc.cmake
		;;
		-a|--arch)
			shift
			[ $# -gt 0 ] || usage
			ARCH=$1
		;;
		--arch=*)
			ARCH=${1#--arch=}
		;;
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
		*)
			usage
	esac

	shift
done

if [ "x$ARCH" = "x" ]; then
	echo "Could not detect RosBE and no -a/--arch given." >&2
	exit 1
fi

REACTOS_OUTPUT_PATH=output-$BUILD_ENVIRONMENT-$ARCH

echo "Configuring a new ReactOS build on:"
uname -a; echo

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
