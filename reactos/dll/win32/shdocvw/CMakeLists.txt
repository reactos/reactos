
add_definitions(
    -D__WINESRC__
    -D_SHDOCVW_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(shdocvw.dll shdocvw.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    shdocvw_main.c
    shlinstobj.c
    shdocvw.h
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw_stubs.c)

add_typelib(shdocvw_v1.idl)
set_source_files_properties(shdocvw.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shdocvw_v1.tlb)

add_library(shdocvw SHARED
    ${SOURCE}
    shdocvw.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shdocvw.def)

set_module_type(shdocvw win32dll)
target_link_libraries(shdocvw uuid wine)
add_delay_importlibs(shdocvw ole32 oleaut32 ieframe)
add_importlibs(shdocvw shlwapi advapi32 msvcrt kernel32 ntdll)
add_dependencies(shdocvw stdole2)
add_pch(shdocvw shdocvw.h SOURCE)
add_cd_file(TARGET shdocvw DESTINATION reactos/system32 FOR all)
