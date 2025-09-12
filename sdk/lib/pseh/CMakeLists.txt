
if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        i386/seh.s
        i386/seh_prolog.s)
endif()

if(MSVC OR ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (ARCH STREQUAL "amd64")))
    list(APPEND SOURCE dummy.c)
    add_asm_files(pseh_asm ${ASM_SOURCE})
    add_library(pseh ${SOURCE} ${pseh_asm})
    add_dependencies(pseh asm)
elseif((CMAKE_C_COMPILER_ID STREQUAL "GNU") AND (ARCH STREQUAL "amd64"))
    # MinGW-w64 GCC has native SEH support for Win64
    # SEH is automatic, no special flags needed
    add_library(pseh INTERFACE)
else()

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
    endif()

    add_library(pseh ${SOURCE} ${ASM_SOURCE})
    target_link_libraries(pseh chkstk)
    add_dependencies(pseh psdk)

    target_include_directories(pseh PRIVATE include/pseh)
endif()

target_include_directories(pseh INTERFACE include)

# Make it clear that we are using PSEH2
if ((CMAKE_C_COMPILER_ID STREQUAL "GNU") OR
    ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (NOT (ARCH STREQUAL "amd64"))))
endif()
