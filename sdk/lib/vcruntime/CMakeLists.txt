
# Replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

include_directories(inc)

# Silence GCC/Clang warnings
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wno-builtin-declaration-mismatch
        -Wno-unused-function
    )
endif()

list(APPEND VCRT_COMMON_SOURCES
    __report_gsfailure.c
    __report_rangecheckfailure.c
    __security_init_cookie.c
    _fltused.c
    initializers.cpp
    isa_available.cpp
    tlssup.c
)

list(APPEND VCRT_RUNTIME_SOURCES
    __std_terminate.c
    __vcrt_init.c
)

list(APPEND VCRT_STARTUP_SOURCES
    __acrt_initialize_stub.cpp
    __scrt_uninitialize_crt.cpp
    __vcrt_init_stubs.c
    mainCRTStartup.cpp
    wmainCRTStartup.cpp
)

if(${ARCH} STREQUAL "i386")
    list(APPEND VCRT_COMMON_SOURCES
        i386/__security_check_cookie.s
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND VCRT_COMMON_SOURCES
        amd64/__security_check_cookie.s
    )
endif()

# Common between vcstartup and vcruntime
add_library(vcrt_common ${VCRT_COMMON_SOURCES})
target_compile_definitions(vcrt_common PRIVATE
    _CORECRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
)
target_link_libraries(vcrt_common ${PSEH_LIB})
add_dependencies(vcrt_common psdk)

# Runtime library (linked into ucrtbase)
add_library(vcruntime ${VCRT_RUNTIME_SOURCES} $<TARGET_OBJECTS:vcrt_common>)
target_compile_definitions(vcruntime PRIVATE
    _CORECRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
)
target_link_libraries(vcruntime ${PSEH_LIB})
target_include_directories(vcruntime INTERFACE inc)

# Startup library (inked into executables, linking to ucrtbase)
add_library(vcstartup ${VCRT_STARTUP_SOURCES} $<TARGET_OBJECTS:vcrt_common>)
target_compile_definitions(vcstartup PRIVATE
    _CORECRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
)
target_link_libraries(vcstartup ${PSEH_LIB} libucrtbase libkernel32)
