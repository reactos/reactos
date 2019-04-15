
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

    # Disable warning " unary minus operator applied to unsigned type, result still unsigned"
    add_compile_flags("/wd4146")

    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    add_compile_flags("/wd4244")

endif()

if(CMAKE_CROSSCOMPILING)
    add_definitions(
        -D_DLL -D__USE_CRTIMP
        -D__NO_ISOCEXT
        -Dstrtoull=_strtoui64
        -Dstrtoll=_strtoi64
        -Dopen=_open
        -Dclose=_close)

    include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
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
