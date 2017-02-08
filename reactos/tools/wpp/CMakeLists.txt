
if(MSVC)
    if(MSVC_VERSION LESS 1900)
        add_definitions(
            -Dsnprintf=_snprintf
            -Dstrtoull=_strtoui64
            -Dstrtoll=_strtoi64)

        # Add this definition for WDK only, VS 9 doesn't like that
        if(DEFINED ENV{DDKBUILDENV})
            add_definitions(-Dvsnprintf=_vsnprintf)
        endif()
    endif()
endif()

if(CMAKE_CROSSCOMPILING)
    add_definitions(
        -D_DLL -D__USE_CRTIMP
        -D__NO_ISOCEXT
        -Dstrtoull=_strtoui64
        -Dstrtoll=_strtoi64
        -Dopen=_open
        -Dclose=_close)

    include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
endif()

list(APPEND SOURCE
    preproc.c
    wpp.c
    ppl.yy.c
    ppy.tab.c)

if(CMAKE_CROSSCOMPILING)
    add_library(wpp ${SOURCE})
else()
    add_library(wpphost ${SOURCE})
endif()
