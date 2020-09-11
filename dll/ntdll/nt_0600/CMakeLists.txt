
spec2def(ntdll_vista.dll ntdll_vista.spec ADD_IMPORTLIB)

add_definitions(
    -D__NTDLL__
    -D_NTOSKRNL_
    -DCRTDLL)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

list(APPEND SOURCE
    DllMain.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_vista.def)

add_library(ntdll_vista MODULE ${SOURCE})
set_module_type(ntdll_vista win32dll ENTRYPOINT DllMain 12)
target_link_libraries(ntdll_vista rtl_vista)
if(ARCH STREQUAL "arm")
    target_link_libraries(ntdll_vista chkstk)
endif()
add_importlibs(ntdll_vista ntdll)
add_dependencies(ntdll_vista psdk)
add_cd_file(TARGET ntdll_vista DESTINATION reactos/system32 FOR all)
