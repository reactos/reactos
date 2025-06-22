
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

spec2def(ddrawex.dll ddrawex.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ddraw.c
    main.c
    surface.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddrawex_stubs.c)

if(MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #target_compile_options(ddrawex PRIVATE "/FIwine/typeof.h")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/FIwine/typeof.h")
endif()

add_library(ddrawex MODULE
    ${SOURCE}
    ddrawex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ddrawex.def)

set_module_type(ddrawex win32dll)
target_link_libraries(ddrawex wine_dll_register wine uuid dxguid ${PSEH_LIB})
add_importlibs(ddrawex ddraw advapi32 gdi32 user32 d3dwine msvcrt kernel32 ntdll)
add_dependencies(ddrawex wineheaders)
add_cd_file(TARGET ddrawex DESTINATION reactos/system32 FOR all)
