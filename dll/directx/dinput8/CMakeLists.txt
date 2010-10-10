
set_unicode()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dinput8 ${CMAKE_CURRENT_SOURCE_DIR}/dinput8.spec ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def)

add_library(dinput8 SHARED
    dinput8_main.c
    regsvr.c
    version.rc)

set_entrypoint(dinput8 0)

target_link_libraries(dinput8
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def
    dxguid
    uuid
    wine)

add_importlibs(dinput8
    user32
    advapi32
    ole32
    winmm
    dinput
    kernel32
    ntdll)

add_dependencies(dinput8 dinput8_def psdk buildno_header)
