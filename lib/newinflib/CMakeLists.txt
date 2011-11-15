
list(APPEND GLOBAL_FILES
    infcore.c
    infget.c
    infput.c)

if(CMAKE_CROSSCOMPILING)
    list(APPEND SOURCE
        infrosgen.c
        infrosget.c
        infrosput.c)

    add_library(inflib ${GLOBAL_FILES} ${SOURCE})
    add_pch(inflib inflib.h)
    add_dependencies(inflib psdk)
else()
    list(APPEND SOURCE
        infhostgen.c
        infhostget.c
        infhostput.c
        infhostrtl.c)

    add_definitions(-D__NO_CTYPE_INLINES -DINFLIB_HOST)
    if(NOT MSVC)
        add_compile_flags("-Wpointer-arith -Wwrite-strings")
    endif()
    add_library(inflibhost ${GLOBAL_FILES} ${SOURCE})
    target_link_libraries(inflibhost unicode)
endif()
