
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(pdh.dll pdh.spec)

list(APPEND SOURCE
    pdh_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/pdh.def)

add_library(pdh SHARED ${SOURCE})
set_entrypoint(pdh 0)

target_link_libraries(pdh wine)

add_importlibs(pdh kernel32 ntdll)
add_importlib_target(pdh.spec)

add_cab_target(pdh 1)