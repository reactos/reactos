add_definitions(-D__NTDLL__)
add_definitions(-D_NTOSKRNL_)
add_definitions(-DCRTDLL)

SET(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE 1)

include_directories(BEFORE ./include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

file(GLOB_RECURSE SOURCE *.c)
list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ldr/elf.c ${CMAKE_CURRENT_SOURCE_DIR}/dispatch/dispatch.c)
list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/dispatch/amd64/stubs.c)

file(GLOB_RECURSE ARCH_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/dispatch/${ARCH}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/dispatch/${ARCH}/*.s")

add_library(ntdll SHARED ${ARCH_SOURCE} ${SOURCE})

target_link_libraries(ntdll rtl ntdllsys libcntpr pseh)
add_dependencies(ntdll ntstatus version)