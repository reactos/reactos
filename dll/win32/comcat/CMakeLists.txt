
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(comcat ${CMAKE_CURRENT_SOURCE_DIR}/comcat.spec ${CMAKE_CURRENT_BINARY_DIR}/comcat.def)

add_library(comcat SHARED comcat_main.c version.rc)

set_entrypoint(comcat 0)

target_link_libraries(comcat
    ${CMAKE_CURRENT_BINARY_DIR}/comcat.def
    uuid
    wine)

add_importlibs(comcat
    ole32
    user32
    advapi32
    ntdll)

add_dependencies(comcat comcat_def psdk buildno_header)
add_livecd_target(comcat reactos/system32)
