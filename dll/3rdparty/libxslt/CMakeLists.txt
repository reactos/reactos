
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(
    -DHAVE_CONFIG_H
    -DWIN32
    -D_WINDOWS
    -D_MBCS
    -DHAVE_STAT
    -DHAVE_WIN32_THREADS
    -D_REENTRANT
    -D_WINSOCKAPI_
    -DLIBXML_STATIC)

add_library(libxslt SHARED
    attributes.c
    attrvt.c
    documents.c
    extensions.c
    extra.c
    functions.c
    imports.c
    keys.c
    namespaces.c
    numbers.c
    pattern.c
    preproc.c
    security.c
    templates.c
    transform.c
    variables.c
    xslt.c
    xsltlocale.c
    xsltutils.c)

set_module_type(libxslt win32dll ENTRYPOINT 0)
target_link_libraries(libxslt libxml2)
add_importlibs(libxslt msvcrt ws2_32 kernel32)

if(MSVC)
    add_importlibs(libxslt ntdll)
endif()

add_dependencies(libxslt psdk)
add_pch(libxslt libxslt.h)
add_cd_file(TARGET libxslt DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_target_compile_flags(libxslt "-Wno-pointer-sign")
    allow_warnings(libxslt)
endif()
