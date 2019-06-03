
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

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
    stubs.c)

add_library(msvcrt40 MODULE ${SOURCE})
spec2def(msvcrt40 msvcrt40.dll msvcrt40.spec)

set_module_type(msvcrt40 win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt40 psdk)
target_link_libraries(msvcrt40 crt wine ${PSEH_LIB})

if(MSVC)
    # export of deleting destructor "name"
    add_target_link_flags(msvcrt40 "/ignore:4102")
endif()

add_importlibs(msvcrt40 kernel32 ntdll)
add_cd_file(TARGET msvcrt40 DESTINATION reactos/system32 FOR all)
