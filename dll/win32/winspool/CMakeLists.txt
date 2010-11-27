
spec2def(winspool.drv winspool.spec)

set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(winspool SHARED
    info.c
    stubs.c
    winspool_winspool.stubs.c
    winspool.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

set_target_properties(winspool PROPERTIES SUFFIX ".drv")
set_module_type(winspool win32dll)

target_link_libraries(winspool wine)

add_importlibs(winspool advapi32 shlwapi kernel32 ntdll)

add_cab_target(winspool 1)
add_importlib_target(winspool.spec)
