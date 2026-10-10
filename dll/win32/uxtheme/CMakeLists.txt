
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)

spec2def(uxtheme_ros.dll uxtheme_vista.spec ADD_IMPORTLIB)
if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    spec2def(uxtheme.dll uxtheme_vista.spec ADD_IMPORTLIB)
else()
    spec2def(uxtheme.dll uxtheme.spec ADD_IMPORTLIB)
endif()

list(APPEND SOURCE
    buffer.c
    draw.c
    errinfo.c
    main.c
    metric.c
    msstyles.c
    ncscrollbar.c
    nonclient.c
    property.c
    resource.h
    stylemap.c
    system.c
    themehooks.c
    uxini.c
    uxthemep.h)

if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    add_definitions(-DENABLE_PNG_SUPPORT)

    list(APPEND SOURCE
        pngsup.cpp)
endif()

file(GLOB uxtheme_rc_deps ${CMAKE_CURRENT_SOURCE_DIR}/lang/*.rc)
add_rc_deps(uxtheme.rc ${uxtheme_rc_deps})

add_library(uxtheme MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_stubs.c
    uxtheme.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

set_module_type(uxtheme win32dll)
target_link_libraries(uxtheme wine)
add_delay_importlibs(uxtheme msimg32)
if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    add_importlibs(uxtheme user32 advapi32 gdiplus gdi32 shlwapi msvcrt kernel32 ntdll)
else()
    add_importlibs(uxtheme user32 advapi32 gdi32 msvcrt kernel32 ntdll)
endif()
add_pch(uxtheme uxthemep.h SOURCE)
add_cd_file(TARGET uxtheme DESTINATION reactos/system32 FOR all)


add_library(uxtheme_ros MODULE
    buffer.c
    vista.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_ros_stubs.c
    uxtheme.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_ros.def)

set_module_type(uxtheme_ros win32dll)
target_link_libraries(uxtheme_ros wine)
add_delay_importlibs(uxtheme_ros msimg32)
add_importlibs(uxtheme_ros uxtheme user32 advapi32 gdiplus gdi32 shlwapi msvcrt kernel32 ntdll)
add_pch(uxtheme_ros uxthemep.h SOURCE)
add_cd_file(TARGET uxtheme_ros DESTINATION reactos/system32 FOR all)