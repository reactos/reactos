
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

spec2def(msvcrt40.dll msvcrt40.spec)

list(APPEND SOURCE
    msvcrt40.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

add_library(msvcrt40 SHARED ${SOURCE})
set_image_base(msvcrt40 ${baseaddress_msvcrt40})
set_entrypoint(msvcrt40 DllMain@12)

target_link_libraries(msvcrt40 crt wine)

if(NOT MSVC)
    target_link_libraries(msvcrt40 pseh)
endif()

add_importlibs(msvcrt40 kernel32 ntdll)

add_cd_file(TARGET msvcrt40 DESTINATION reactos/system32 FOR all)
