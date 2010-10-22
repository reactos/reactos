
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(winspool ${CMAKE_CURRENT_SOURCE_DIR}/winspool.spec ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

add_library(winspool SHARED
    info.c
    stubs.c
    winspool.rc)

set_target_properties(winspool PROPERTIES SUFFIX ".drv")
set_module_type(winspool win32dll)

target_link_libraries(winspool
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def
    wine)

add_importlibs(winspool advapi32 shlwapi kernel32 ntdll)
add_dependencies(winspool winspool_def)
#add_livecd_target(winspool reactos/system32 winspool.drv)
