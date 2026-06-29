
spec2def(wlntfytests.dll wlntfytests.spec)

add_library(wlntfytests MODULE
    wlntfytests.c
    wlntfytests.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wlntfytests.def)

set_module_type(wlntfytests win32dll UNICODE ENTRYPOINT DllMain 12)
target_link_libraries(wlntfytests wine)
add_importlibs(wlntfytests advapi32 user32 kernel32 ntdll)
add_rostests_file(TARGET wlntfytests SUBDIR suppl)
