
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(wbemprox.dll wbemprox.spec)

list(APPEND SOURCE
    builtin.c
    class.c
    main.c
    process.c
    qualifier.c
    query.c
    reg.c
    service.c
    services.c
    table.c
    wbemlocator.c
    wql.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/wbemprox.def)

add_library(wbemprox SHARED ${SOURCE} wbemprox.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(wbemprox)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_source_files_properties(wbemprox.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wbemprox.rgs)
set_module_type(wbemprox win32dll)
target_link_libraries(wbemprox wine)
add_importlibs(wbemprox iphlpapi dxgi oleaut32 ole32 advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_dependencies(wbemprox d3d_idl_headers)
add_cd_file(TARGET wbemprox DESTINATION reactos/system32/wbem FOR all)
