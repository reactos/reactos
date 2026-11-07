
list(APPEND UCRT_STDLIB_SOURCES
    stdlib/abs.cpp
    stdlib/bsearch.cpp
    stdlib/bsearch_s.cpp
    stdlib/byteswap.cpp
    stdlib/div.cpp
    stdlib/imaxabs.cpp
    stdlib/imaxdiv.cpp
    stdlib/labs.cpp
    stdlib/ldiv.cpp
    stdlib/lfind.cpp
    stdlib/lfind_s.cpp
    stdlib/llabs.cpp
    stdlib/lldiv.cpp
    stdlib/lsearch.cpp
    stdlib/lsearch_s.cpp
    stdlib/qsort.cpp
    stdlib/qsort_s.cpp
    stdlib/rand.cpp
    stdlib/rand_s.cpp
    stdlib/rotl.cpp
    stdlib/rotr.cpp
)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if(ARCH STREQUAL "arm64")
        add_asm_files(UCRT_STDLIB_ASM stdlib/arm64/clang-hacks.s)
    else()
        add_asm_files(UCRT_STDLIB_ASM stdlib/clang-hacks.s)
    endif()
    list(APPEND UCRT_STDLIB_SOURCES
        ${UCRT_STDLIB_ASM}
    )
endif()
