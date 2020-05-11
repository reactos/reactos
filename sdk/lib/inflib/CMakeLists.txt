
list(APPEND SOURCE
    infcore.c
    infget.c
    infput.c)

if(CMAKE_CROSSCOMPILING)
    list(APPEND SOURCE
        infrosgen.c
        infrosget.c
        infrosput.c
        inflib.h)

    add_library(inflib ${SOURCE})
    add_pch(inflib inflib.h SOURCE)
    add_dependencies(inflib psdk)
else()
    list(APPEND SOURCE
        infhostgen.c
        infhostget.c
        infhostput.c
        infhostrtl.c)

    add_definitions(-D__NO_CTYPE_INLINES -DINFLIB_HOST -D_CRT_SECURE_NO_WARNINGS)
    add_library(inflibhost ${SOURCE})
    target_include_directories(inflibhost INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    if(NOT MSVC)
        target_compile_options(inflibhost PRIVATE -fshort-wchar -Wpointer-arith -Wwrite-strings)
    endif()

    target_link_libraries(inflibhost PRIVATE host_includes)
endif()
