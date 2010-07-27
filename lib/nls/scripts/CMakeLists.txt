
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/icu4ros/icu/source/common)

# Special C++ flags
add_definitions(-fno-exceptions -fno-rtti)

# Re-definition of WINVER
remove_definitions(-DWINVER=0x502) # This removes a top-level definition: The compiler never gets it on the command-line
add_definitions(-DWINVER=0x600) # And now we add our own

file(GLOB_RECURSE SOURCE "*.cpp")

add_library(scripts ${SOURCE})