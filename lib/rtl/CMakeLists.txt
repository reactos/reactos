
include_directories(.)

add_definitions(-D_NTOSKRNL_)
add_definitions(-DNO_RTL_INLINES)
add_definitions(-D_NTSYSTEM_)
add_definitions(-D_NTDLLBUILD_)

file(GLOB_RECURSE ARCH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.S)

get_directory_property(defines COMPILE_DEFINITIONS)

foreach(arg ${defines})
  set(result_defs "${result_defs} -D${arg}")
endforeach(arg ${defines})

set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> ${result_defs} -D__ASM__ <FLAGS> -c <SOURCE>")

file(GLOB SOURCE "*.c")

list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/avlsupp.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/mem.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/memgen.c
    )

add_library(rtl ${ARCH_SOURCE} ${SOURCE})
add_dependencies(rtl psdk)

