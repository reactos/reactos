
add_definitions(-D_NTOSKRNL_)
add_definitions(-DNO_RTL_INLINES)
add_definitions(-D_NTSYSTEM_)
add_definitions(-D_NTDLLBUILD_)

file(GLOB_RECURSE ARCH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.S)
file(GLOB SOURCE "*.c")

list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/avlsupp.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/mem.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/memgen.c)

add_library(rtl ${ARCH_SOURCE} ${SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/rtl.h.gch)
add_pch(rtl ${CMAKE_CURRENT_SOURCE_DIR}/rtl.h ${SOURCE})
add_dependencies(rtl psdk)

