
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mlang ${CMAKE_CURRENT_SOURCE_DIR}/mlang.spec ${CMAKE_CURRENT_BINARY_DIR}/mlang.def)

add_library(mlang SHARED mlang.c regsvr.c)

set_entrypoint(mlang 0)

target_link_libraries(mlang
    ${CMAKE_CURRENT_BINARY_DIR}/mlang.def
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

add_dependencies(mlang mlang_def psdk buildno_header)
add_livecd_target(mlang reactos/system32)