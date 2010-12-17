
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(loadperf.dll loadperf.spec)

add_library(loadperf SHARED
            loadperf_main.c
            stubs.c
            ${CMAKE_CURRENT_BINARY_DIR}/loadperf.def)

set_module_type(loadperf win32dll)
target_link_libraries(loadperf wine)
add_importlibs(loadperf msvcrt kernel32 ntdll)
add_cab_target(loadperf 1)
add_importlib_target(loadperf.spec)
