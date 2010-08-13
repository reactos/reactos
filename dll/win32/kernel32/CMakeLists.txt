set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-D_KERNEL32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

file(GLOB_RECURSE SOURCE "*.c")

file(GLOB_RECURSE ARCH_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/thread/${ARCH}/*.s")

add_library(kernel32 SHARED
            ${SOURCE}
            ${ARCH_SOURCE}
            ${CMAKE_CURRENT_SOURCE_DIR}/kernel32.rc
            ${CMAKE_CURRENT_BINARY_DIR}/kernel32_k32.h.gch)

set_target_properties(kernel32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(kernel32 ${CMAKE_CURRENT_SOURCE_DIR}/kernel32.def
                      pseh
                      ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(kernel32 ${CMAKE_CURRENT_SOURCE_DIR}/k32.h ${SOURCE})
add_dependencies(kernel32 errcodes version)
