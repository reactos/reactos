
project(HAL)
if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
    add_subdirectory(halx86)
elseif((ARCH STREQUAL "arm") OR (ARCH STREQUAL "arm64"))
    add_subdirectory(halarm)
endif()
