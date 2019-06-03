
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    loadperf_main.c
    stubs.c)

add_library(loadperf MODULE ${SOURCE})
spec2def(loadperf loadperf.dll loadperf.spec ADD_IMPORTLIB)

set_module_type(loadperf win32dll)
target_link_libraries(loadperf wine)
add_importlibs(loadperf msvcrt kernel32 ntdll)
add_cd_file(TARGET loadperf DESTINATION reactos/system32 FOR all)
