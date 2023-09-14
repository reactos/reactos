
add_definitions(
    -D__WINESRC__
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
)

remove_definitions(-D__ROS_LONG64__)
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

spec2def(coinst.dll coinst.spec)
add_library(setupapi_coinst MODULE coinst.c ${CMAKE_CURRENT_BINARY_DIR}/coinst.def)
set_target_properties(setupapi_coinst PROPERTIES OUTPUT_NAME "coinst")
set_module_type(setupapi_coinst win32dll)
add_importlibs(setupapi_coinst msvcrt kernel32)

spec2def(selfreg.dll selfreg.spec)
add_library(setupapi_selfreg MODULE selfreg.c ${CMAKE_CURRENT_BINARY_DIR}/selfreg.def)
set_target_properties(setupapi_selfreg PROPERTIES OUTPUT_NAME "selfreg")
set_module_type(setupapi_selfreg win32dll)
target_link_libraries(setupapi_selfreg uuid)
add_importlibs(setupapi_selfreg advapi32 ole32 msvcrt kernel32)

list(APPEND SOURCE
    devinst.c
    dialog.c
    diskspace.c
    install.c
    misc.c
    parser.c
    query.c
    setupcab.c
    stringtable.c
    testlist.c)

set_property(SOURCE setupapi.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/coinst.dll ${CMAKE_CURRENT_BINARY_DIR}/selfreg.dll)

add_executable(setupapi_winetest ${SOURCE} setupapi.rc)
set_module_type(setupapi_winetest win32cui)
target_link_libraries(setupapi_winetest uuid oldnames)
add_importlibs(setupapi_winetest advapi32 cabinet ole32 setupapi user32 shell32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET setupapi_winetest)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(setupapi_winetest PRIVATE -Wno-format-overflow)
endif()
