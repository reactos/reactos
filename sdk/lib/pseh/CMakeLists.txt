
if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        i386/seh.s
        i386/seh_prolog.s)
endif()

if(NOT MSVC)

    if(USE_PSEH3)
        include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/pseh)
        list(APPEND SOURCE
            i386/pseh3.c
            i386/pseh3_i386.S)
    elseif(USE_DUMMY_PSEH)
        list(APPEND SOURCE dummy.c)
    elseif(ARCH STREQUAL "i386")
        list(APPEND SOURCE
            i386/framebased.S
            i386/framebased-gcchack.c
            i386/framebased-gcchack-asm.S)
    elseif(ARCH STREQUAL "amd64")
        list(APPEND SOURCE amd64/framebased.S)
    elseif(ARCH STREQUAL "powerpc")
        list(APPEND SOURCE powerpc/framebased.S)
    endif()

    add_library(pseh ${SOURCE} ${ASM_SOURCE})
    target_link_libraries(pseh chkstk)
    add_dependencies(pseh psdk)

else()

    if(ARCH STREQUAL "amd64")
        list(APPEND ASM_SOURCE
            amd64/seh.s
            amd64/seh_prolog.s)
    elseif(ARCH STREQUAL "arm")
        list(APPEND ASM_SOURCE
            arm/seh_prolog.s)
    endif()

    list(APPEND SOURCE dummy.c)
    add_asm_files(pseh_asm ${ASM_SOURCE})
    add_library(pseh ${SOURCE} ${pseh_asm})
    add_dependencies(pseh asm)

endif()
