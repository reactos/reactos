
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(msvcrt20.dll msvcrt20.spec)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_
    -D_MSVCRT_LIB_
    -D_MT
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -DCRTDLL
    -D__MINGW_IMPORT="")

list(APPEND SOURCE
    msvcrt20.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

add_library(msvcrt20 MODULE ${SOURCE})
set_module_type(msvcrt20 win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt20 psdk)
target_link_libraries(msvcrt20 crt wine ${PSEH_LIB})

if(MSVC)
    # export of deleting destructor "name"
    target_link_options(msvcrt20 PRIVATE /ignore:4102)
endif()

add_importlibs(msvcrt20 kernel32 ntdll)
add_cd_file(TARGET msvcrt20 DESTINATION reactos/system32 FOR all)
