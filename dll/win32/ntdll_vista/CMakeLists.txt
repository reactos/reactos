
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600
    -D__NTDLL__
    -D_NTOSKRNL_
    -D_NTSYSTEM_)

spec2def(ntdll_vista_addon.dll ntdll_vista_addon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    condvar.c
    srw.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_vista_addon.def)

add_library(ntdll_vista_static ${SOURCE})
add_library(ntdll_vista_addon MODULE DllMain.c)
set_module_type(ntdll_vista_addon win32dll ENTRYPOINT DllMain 12)
target_link_libraries(ntdll_vista_addon ntdll_vista_static)
if(ARCH STREQUAL "arm")
    target_link_libraries(ntdll_vista_addon chkstk)
endif()
add_importlibs(ntdll_vista_addon ntdll)
add_dependencies(ntdll_vista_addon psdk)
