
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmscript.dll dmscript.spec)

list(APPEND SOURCE
    dmscript_main.c
    script.c
    scripttrack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c)

add_library(dmscript MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmscript.def)

add_idl_reg_scripts(dmscript registry dmscript.idl)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/dmscript.rgs")

set_module_type(dmscript win32dll)
target_link_libraries(dmscript wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmscript ole32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(dmscript wineheaders)
add_cd_file(TARGET dmscript DESTINATION reactos/system32 FOR all)
set_wine_module(dmscript)
