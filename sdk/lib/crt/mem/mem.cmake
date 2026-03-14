
list(APPEND LIBCNTPR_MEM_SOURCE
    mem/memccpy.c
    mem/memcmp.c
    mem/memicmp.c
)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_MEM_ASM_SOURCE
        mem/i386/memchr_asm.s
        mem/i386/memmove_asm.s
        mem/i386/memset_asm.s
    )
    #list(APPEND CRT_MEM_ASM_SOURCE
    #    ${LIBCNTPR_MEM_ASM_SOURCE}
    #)
else()
    list(APPEND LIBCNTPR_MEM_SOURCE
        mem/memchr.c
        mem/memcpy.c
        mem/memmove.c
        mem/memset.c
    )
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        # Prevent GCC from optimizing loops in memcpy/memmove/memset back into
        # calls to memcpy/memmove/memset, causing infinite recursion at -O2.
        set_source_files_properties(mem/memcpy.c mem/memmove.c mem/memset.c
            PROPERTIES COMPILE_FLAGS "-fno-tree-loop-distribute-patterns")
    endif()
endif()

#list(APPEND CRT_MEM_SOURCE
#    ${LIBCNTPR_MEM_SOURCE}
#)

# Needed by ext2fs. Should use RtlCompareMemory instead?
add_library(memcmp mem/memcmp.c)
add_dependencies(memcmp psdk)
