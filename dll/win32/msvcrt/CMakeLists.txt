
spec2def(msvcrt.dll msvcrt.spec)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_
    -D_MSVCRT_LIB_
    -D_MT
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -DCRTDLL
    -D__MINGW_IMPORT="")

include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

set_rc_compiler()

list(APPEND SOURCE
    dllmain.c
    stubs.c
    msvcrt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.def)

add_library(msvcrt SHARED ${SOURCE})

if (NOT MSVC)
set_target_properties(msvcrt PROPERTIES LINK_FLAGS "-u __seh_longjmp_unwind@4")
endif()

add_dependencies(msvcrt psdk)
set_entrypoint(msvcrt DllMain@12)
set_image_base(msvcrt ${baseaddress_msvcrt})

target_link_libraries(msvcrt
    crt
    wine
    ${PSEH_LIB})

add_pch(msvcrt precomp.h)
add_importlibs(msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrt DESTINATION reactos/system32 FOR all)
add_importlib_target(msvcrt.spec)

