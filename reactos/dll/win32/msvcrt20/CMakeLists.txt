
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

spec2def(msvcrt20.dll msvcrt20.spec)

list(APPEND SOURCE
    msvcrt20.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

add_library(msvcrt20 SHARED ${SOURCE})
set_entrypoint(msvcrt20 DllMain@12)
set_image_base(msvcrt20 ${baseaddress_msvcrt20})

target_link_libraries(msvcrt20 crt wine)

if(NOT MSVC)
  target_link_libraries(msvcrt20 pseh)
endif()

add_importlibs(msvcrt20 kernel32 ntdll)
add_dependencies(msvcrt20 psdk)
add_cd_file(TARGET msvcrt20 DESTINATION reactos/system32 FOR all)
