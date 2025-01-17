
set(CMAKE_CXX_STANDARD 17)

# Replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

if(MSVC)
    # Disable warning C4083: expected ')'; found identifier '<warning identifier>'
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4083>)

    # Disable warning C4189: 'cvt': local variable is initialized but not referenced
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4189>)
endif()

# Internal includes
include_directories(BEFORE inc)

if(${ARCH} STREQUAL "i386")
    include_directories(inc/i386)
endif()

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_compile_definitions(
    WINVER=0x600
    _WIN32_WINNT=0x600
    _UCRT
    _CORECRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    _GCC_NO_SAL_ATTRIIBUTES
    CRTDLL
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Silence GCC/Clang warnings
    add_compile_options(
        -Wno-unknown-warning-option
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-builtin-declaration-mismatch
        -Wno-parentheses
        -Wno-unused-variable
        -Wno-sign-compare
        -Wno-enum-compare
        -Wno-switch
        -Wno-write-strings
        -Wno-comment
        -Wno-narrowing
        -Wno-misleading-indentation
        -Wno-missing-braces
        -Wno-unused-value
        -Wno-unused-local-typedef
        -Wno-unused-function
        -Wno-writable-strings
        -Wno-microsoft-template
        -Wno-switch
        -Wno-ignored-pragmas
        -Wno-empty-body
        -Wno-tautological-constant-out-of-range-compare
        -Wno-ignored-attributes
        -Wno-uninitialized
    )
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_definitions(
        _lrotl=___lrotl
        _rotl=___rotl
        _rotl64=___rotl64
        _lrotr=___lrotr
        _rotr=___rotr
        _rotr64=___rotr64
    )
endif()

# Hack until we support globally defining _DEBUG
if(DBG)
    add_compile_definitions(_DEBUG)
endif()

include(conio/conio.cmake)
include(convert/convert.cmake)
include(dll/dll.cmake)
include(env/env.cmake)
include(exec/exec.cmake)
include(filesystem/filesystem.cmake)
include(float/float.cmake)
include(heap/heap.cmake)
include(initializers/initializers.cmake)
include(internal/internal.cmake)
include(locale/locale.cmake)
include(lowio/lowio.cmake)
include(math/math.cmake)
include(mbstring/mbstring.cmake)
include(misc/misc.cmake)
include(startup/startup.cmake)
include(stdio/stdio.cmake)
include(stdlib/stdlib.cmake)
include(string/string.cmake)
include(time/time.cmake)

add_library(ucrt OBJECT
    ${UCRT_CONIO_SOURCES}
    ${UCRT_CONVERT_SOURCES}
    ${UCRT_DLL_SOURCES}
    ${UCRT_ENV_SOURCES}
    ${UCRT_EXEC_SOURCES}
    ${UCRT_FILESYSTEM_SOURCES}
    ${UCRT_FLOAT_SOURCES}
    ${UCRT_HEAP_SOURCES}
    ${UCRT_INITIALIZERS_SOURCES}
    ${UCRT_INTERNAL_SOURCES}
    ${UCRT_LOCALE_SOURCES}
    ${UCRT_LOWIO_SOURCES}
    ${UCRT_MATH_SOURCES}
    ${UCRT_MBSTRING_SOURCES}
    ${UCRT_MISC_SOURCES}
    ${UCRT_STARTUP_SOURCES}
    ${UCRT_STDIO_SOURCES}
    ${UCRT_STDLIB_SOURCES}
    ${UCRT_STRING_SOURCES}
    ${UCRT_TIME_SOURCES}
    ${UCRT_VCRUNTIME_SOURCES}
)

target_link_libraries(ucrt vcruntime)
add_dependencies(ucrt psdk asm)
