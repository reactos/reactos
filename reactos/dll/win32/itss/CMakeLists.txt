
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(itss.dll itss.spec)

list(APPEND SOURCE
    chm_lib.c
    lzx.c
    itss.c
    moniker.c
    protocol.c
    storage.c
    #${REACTOS_BINARY_DIR}/include/reactos/wine/itss_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/itss_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/itss.def)

add_library(itss SHARED
    ${SOURCE}
    rsrc.rc)

set_module_type(itss win32dll)
target_link_libraries(itss uuid wine)
if(MSVC)
    target_link_libraries(itss itss_guid)
else()
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(itss)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

add_importlibs(itss
    urlmon
    shlwapi
    ole32
    msvcrt
    kernel32
    ntdll)

add_dependencies(itss wineheaders)
add_cd_file(TARGET itss DESTINATION reactos/system32 FOR all)
