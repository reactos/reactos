
# Replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

include_directories(inc)

if(DBG)
    add_compile_definitions(_DEBUG) # TODO: define this globally
endif()

# Silence GCC/Clang warnings
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wno-builtin-declaration-mismatch
        -Wno-unused-function
    )
endif()

add_compile_definitions(
    _CORECRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    _NTSYSTEM_
)

if(${ARCH} STREQUAL "i386")
    list(APPEND VCSTARTUP_ASM_SOURCES
        i386/__security_check_cookie.s
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND VCSTARTUP_ASM_SOURCES
        amd64/__security_check_cookie.s
    )
endif()
add_asm_files(vcstartup_asm ${VCSTARTUP_ASM_SOURCES})

list(APPEND VCSTARTUP_SOURCES
    ${vcstartup_asm}
    __acrt_initialize_stub.cpp
    __report_gsfailure.c
    __report_rangecheckfailure.c
    __scrt_uninitialize_crt.cpp
    __security_init_cookie.c
    __vcrt_init_stubs.c
    _fltused.c
    _onexit.c
    atexit.c
    initializers.cpp
    isa_available.cpp
    mainCRTStartup.cpp
    tlssup.c
    wmainCRTStartup.cpp
)

# Startup library (linked into executables, linking to ucrtbase)
add_library(vcstartup ${VCSTARTUP_SOURCES})
target_link_libraries(vcstartup ${PSEH_LIB} libkernel32)
target_include_directories(vcstartup PRIVATE  $<TARGET_PROPERTY:vcruntime,INTERFACE_INCLUDE_DIRECTORIES>)
