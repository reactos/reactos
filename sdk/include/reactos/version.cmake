option(NO_REACTOS_BUILDNO "If true, disables the generation of buildno.h and version.h for each configure" OFF)

if (NOT NO_REACTOS_BUILDNO)
    string(TIMESTAMP KERNEL_VERSION_BUILD %Y%m%d-%H%M UTC)
else()
    set(KERNEL_VERSION_BUILD "custom")
endif()

set(KERNEL_VERSION_MAJOR "6")
set(KERNEL_VERSION_MINOR "0")
set(KERNEL_VERSION_PATCH_LEVEL "6000")
set(KERNEL_VERSION_REVISION "51")
set(COPYRIGHT_YEAR "2025")

# KERNEL_VERSION_BUILD_TYPE is "dev" for Git builds
# or "RC1", "RC2", "" for releases.
set(KERNEL_VERSION_BUILD_TYPE "sea")

set(KERNEL_VERSION "${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_VERSION_PATCH_LEVEL}.${KERNEL_VERSION_REVISION}-${WINARCH}")
if(NOT KERNEL_VERSION_BUILD_TYPE STREQUAL "")
    set(KERNEL_VERSION "${KERNEL_VERSION}-${KERNEL_VERSION_BUILD_TYPE}")
endif()

math(EXPR REACTOS_DLL_VERSION_MAJOR "${KERNEL_VERSION_MAJOR}+42")
set(DLL_VERSION_STR "${REACTOS_DLL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_VERSION_PATCH_LEVEL}.${KERNEL_VERSION_REVISION}")
if(NOT KERNEL_VERSION_BUILD_TYPE STREQUAL "")
    set(DLL_VERSION_STR "${DLL_VERSION_STR}-${KERNEL_VERSION_BUILD_TYPE}")
endif()

# Get Git revision through "git describe"
set(COMMIT_HASH "")
set(REVISION "")

configure_file(sdk/include/reactos/version.h.cmake ${REACTOS_BINARY_DIR}/sdk/include/reactos/version.h)
configure_file(sdk/include/reactos/buildno.h.cmake ${REACTOS_BINARY_DIR}/sdk/include/reactos/buildno.h)
