#!/bin/sh

# Check if ROS_ARCH environment variable is set (used to determine target architecture)
if [ "x$ROS_ARCH" = "x" ]; then
	echo "Could not detect RosBE (ReactOS Build Environment)."
	exit 1
fi

# Set the build environment and architecture variables
BUILD_ENVIRONMENT=MinGW
ARCH=$ROS_ARCH

# Determine the absolute path of the ReactOS source directory (script directory)
REACTOS_SOURCE_DIR=$(cd `dirname $0` && pwd)

# Define the output directory based on build environment and architecture
REACTOS_OUTPUT_PATH=output-$BUILD_ENVIRONMENT-$ARCH

# Function to display usage info and exit on invalid parameters
usage() {
	echo "Invalid parameter given."
	exit 1
}

# Default CMake generator
CMAKE_GENERATOR="Ninja"

# Parse command line arguments
while [ $# -gt 0 ]; do
	case $1 in
		-D)
			shift
			# Check if the next parameter is a valid CMake option of the form -D<VAR>=<VALUE>
			if echo "x$1" | grep 'x?*=*' > /dev/null; then
				ROS_CMAKEOPTS=$ROS_CMAKEOPTS" -D $1"
			else
				usage
			fi
		;;
		-D?*=*|-D?*)
			# Accept options like -DVAR=VALUE directly
			ROS_CMAKEOPTS=$ROS_CMAKEOPTS" $1"
		;;
		makefiles|Makefiles)
			# Switch to Unix Makefiles generator if requested
			CMAKE_GENERATOR="Unix Makefiles"
		;;
		*)
			usage
	esac

	shift
done

# Display the system info where ReactOS will be built
echo "Configuring a new ReactOS build on:"
echo $(uname -srvpio); echo

# If running from the source directory, create and move into output directory
if [ "$REACTOS_SOURCE_DIR" = "$PWD" ]; then
	echo "Creating directories in $REACTOS_OUTPUT_PATH"
	mkdir -p "$REACTOS_OUTPUT_PATH"
	cd "$REACTOS_OUTPUT_PATH"
fi

# Remove any previous CMake cache to ensure a clean build configuration
rm -f CMakeCache.txt host-tools/CMakeCache.txt

# Run CMake with the chosen generator, toolchain, architecture, and any extra options
cmake -G "$CMAKE_GENERATOR" -DENABLE_CCACHE:BOOL=0 -DCMAKE_TOOLCHAIN_FILE:FILEPATH=toolchain-gcc.cmake -DARCH:STRING=$ARCH $EXTRA_ARGS $ROS_CMAKEOPTS "$REACTOS_SOURCE_DIR"

# Check if cmake command succeeded
if [ $? -ne 0 ]; then
    echo "An error occurred while configuring ReactOS"
    exit 1
fi

echo "Configure script complete! Execute appropriate build commands (e.g. ninja, make, makex, etc.) from $REACTOS_OUTPUT_PATH"