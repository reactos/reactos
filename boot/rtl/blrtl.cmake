
add_definitions(
    -DNO_RTL_INLINES
    -D_BLDR_
    -D_NTSYSTEM_)

set(NTOS_RTL_SOURCE_DIR "${REACTOS_SOURCE_DIR}/sdk/lib/rtl")
include_directories(${NTOS_RTL_SOURCE_DIR})

if (GCC)
    # Enable this again. CORE-17637
    add_compile_options(-Wunused-result)
endif()

list(APPEND SOURCE
    # ${NTOS_RTL_SOURCE_DIR}/assert.c   ## Requires a local implementation.
    ${NTOS_RTL_SOURCE_DIR}/bitmap.c
    # ${NTOS_RTL_SOURCE_DIR}/bootdata.c ## Requires a local implementation.
    ${NTOS_RTL_SOURCE_DIR}/compress.c
    ${NTOS_RTL_SOURCE_DIR}/crc32.c
    # ${NTOS_RTL_SOURCE_DIR}/debug.c    ## Requires a local implementation.
    ${NTOS_RTL_SOURCE_DIR}/image.c
    ${NTOS_RTL_SOURCE_DIR}/largeint.c
    ## message.c
    # ${NTOS_RTL_SOURCE_DIR}/nls.c      ## Requires a local implementation.
    nlsboot.c
    ${NTOS_RTL_SOURCE_DIR}/random.c
    ## res.c    ## Optional? Needs SEH
    # ${NTOS_RTL_SOURCE_DIR}/time.c     ## Optional
    ${NTOS_RTL_SOURCE_DIR}/unicode.c
    ${NTOS_RTL_SOURCE_DIR}/rtl.h)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        ${NTOS_RTL_SOURCE_DIR}/i386/debug_asm.S
        ${NTOS_RTL_SOURCE_DIR}/i386/rtlmem.s
        ${NTOS_RTL_SOURCE_DIR}/i386/rtlswap.S
        ## ${NTOS_RTL_SOURCE_DIR}/i386/res_asm.s
        )
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        ${NTOS_RTL_SOURCE_DIR}/amd64/debug_asm.S
        ## ${NTOS_RTL_SOURCE_DIR}/amd64/rtlmem.S
        )
    list(APPEND SOURCE
        ${NTOS_RTL_SOURCE_DIR}/bitmap64.c
        ${NTOS_RTL_SOURCE_DIR}/byteswap.c
        ${NTOS_RTL_SOURCE_DIR}/mem.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        ${NTOS_RTL_SOURCE_DIR}/arm/debug_asm.S)
    list(APPEND SOURCE
        ${NTOS_RTL_SOURCE_DIR}/byteswap.c
        ${NTOS_RTL_SOURCE_DIR}/mem.c)
endif()

add_asm_files(blrtl_asm ${ASM_SOURCE})
add_library(blrtl ${SOURCE} ${blrtl_asm})
add_pch(blrtl ${NTOS_RTL_SOURCE_DIR}/rtl.h SOURCE)
add_dependencies(blrtl psdk asm)
