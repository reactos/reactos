
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mlang.dll mlang.spec)

add_library(mlang SHARED mlang.c regsvr.c
    ${CMAKE_CURRENT_BINARY_DIR}/mlang.def)

set_entrypoint(mlang 0)

target_link_libraries(mlang
    uuid
    wine)

add_importlibs(mlang
    ole32
    user32
    gdi32
    advapi32
    oleaut32
    kernel32
    ntdll)

add_dependencies(mlang psdk buildno_header)
add_cab_target(mlang 1)