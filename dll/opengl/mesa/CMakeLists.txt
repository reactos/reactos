
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

# From Sconstruct
include_directories(BEFORE
    include)

include_directories(
    src/gallium/include
    src/gallium/auxiliary
    src/gallium/drivers
    src/gallium/winsys)

if(NOT MSVC)
    add_compile_flags_language("-std=gnu99" "C")
    add_compile_flags("-Wno-deprecated-declarations")
    if(GCC_VERSION VERSION_EQUAL 4.7 OR GCC_VERSION VERSION_GREATER 4.7)
        add_compile_flags("-Wno-error=unused-local-typedefs")
    endif()
    # optimizations (builtin memcmp is slow on some gcc versions) used in SConsript files
    add_compile_flags("-ffast-math -fno-builtin-memcmp")
else()
    include_directories(include/c99)
    add_compile_flags("/wd4996")
endif()

add_definitions(
    -DWIN32
    -D_WINDOWS
    -DPIPE_SUBSYSTEM_WINDOWS_USER
    -D_DLL
#    -DDEBUG
)

# our DBG definitions conflict with mesa source code
remove_definitions(-DDBG=1 -DDBG=0)

add_subdirectory(src)
