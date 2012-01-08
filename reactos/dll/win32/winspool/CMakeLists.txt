

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(winspool.drv winspool.spec)

list(APPEND SOURCE
    info.c
    stubs.c
    winspool.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

add_library(winspool SHARED ${SOURCE})

set_target_properties(winspool PROPERTIES SUFFIX ".drv")

set_module_type(winspool win32dll UNICODE)
target_link_libraries(winspool wine)
add_importlibs(winspool advapi32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET winspool DESTINATION reactos/system32 FOR all)
add_importlib_target(winspool.spec)
