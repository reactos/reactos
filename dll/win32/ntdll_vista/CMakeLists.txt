
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600
    -D__NTDLL__
    -D_NTOSKRNL_
    -D_NTSYSTEM_)

spec2def(ntdll_vista.dll ntdll_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    condvar.c
    srw.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_vista.def)

add_library(ntdll_vista MODULE ${SOURCE})
set_module_type(ntdll_vista win32dll ENTRYPOINT DllMain 12)
if(ARCH STREQUAL "arm")
    target_link_libraries(ntdll_vista chkstk)
endif()
add_importlibs(ntdll_vista ntdll)
add_dependencies(ntdll_vista psdk)
add_cd_file(TARGET ntdll_vista DESTINATION reactos/system32 FOR all)
