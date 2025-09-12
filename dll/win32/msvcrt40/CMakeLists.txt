
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(msvcrt40.dll msvcrt40.spec)

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
    msvcrt40.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

add_library(msvcrt40 MODULE ${SOURCE})
set_module_type(msvcrt40 win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt40 psdk)
target_link_libraries(msvcrt40 crt wine ${PSEH_LIB})

if(MSVC)
    # export of deleting destructor "name"
    target_link_options(msvcrt40 PRIVATE /ignore:4102)
endif()

add_importlibs(msvcrt40 kernel32 ntdll)
add_cd_file(TARGET msvcrt40 DESTINATION reactos/system32 FOR all)
