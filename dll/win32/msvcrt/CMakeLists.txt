
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

list(APPEND SOURCE
     dllmain.c
     msvcrt_stubs.c
     msvcrt.rc
     ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.def)

add_library(msvcrt SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_precomp.h.gch
    ${SOURCE})

if (NOT MSVC)
set_target_properties(msvcrt PROPERTIES LINK_FLAGS "-u __seh_longjmp_unwind@4")
endif()

set_module_type(msvcrt win32dll)

set_entrypoint(msvcrt DllMain@12)

target_link_libraries(msvcrt
    crt
    wine
    ${PSEH_LIB})

add_pch(msvcrt ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_importlibs(msvcrt kernel32 ntdll)
add_cab_target(msvcrt 1)
add_importlib_target(msvcrt.spec)

