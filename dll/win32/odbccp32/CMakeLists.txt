
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(odbccp32 ${CMAKE_CURRENT_SOURCE_DIR}/odbccp32.spec ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def)

add_library(odbccp32 SHARED odbccp32.c)

set_entrypoint(odbccp32 0)

target_link_libraries(odbccp32
    ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def
    uuid
    wine)

add_importlibs(odbccp32
    msvcrt
    advapi32
    kernel32
    ntdll)

add_dependencies(odbccp32 odbccp32_def)
add_cab_target(odbccp32 1)
