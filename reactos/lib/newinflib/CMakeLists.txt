
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

    add_definitions(-D__NO_CTYPE_INLINES -DINFLIB_HOST -D_CRT_NON_CONFORMING_SWPRINTFS)
    add_library(inflibhost ${GLOBAL_FILES} ${SOURCE})
    if(NOT MSVC)
        add_target_compile_flags(inflibhost "-Wpointer-arith -Wwrite-strings")
    endif()
    target_link_libraries(inflibhost unicode)
endif()
