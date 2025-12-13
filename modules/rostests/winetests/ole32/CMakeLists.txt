
add_definitions(-DUSE_WINE_TODOS)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(testlib.dll testlib.spec)

add_library(testlib MODULE testlib.c ${CMAKE_CURRENT_BINARY_DIR}/testlib.def)
set_module_type(testlib win32dll)
add_importlibs(testlib msvcrt kernel32 ntdll)
add_rostests_file(TARGET testlib)

# Hack to allow the testlib DLL to be embedded in ole32 winetest resources
if(MSVC_IDE)
    if(DBG)
        set(TESTLIB_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    else()
        set(TESTLIB_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Release)
    endif()
else()
    set(TESTLIB_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

    # hack for rosbe
    add_custom_command(TARGET testlib POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:testlib> ${TESTLIB_OUTPUT_DIR}/CMakeFiles/ole32_winetest.dir/
)
endif()
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ole32_winetest.rc DESTINATION ${TESTLIB_OUTPUT_DIR})

list(APPEND SOURCE
    clipboard.c
    compobj.c
    defaulthandler.c
    dragdrop.c
    errorinfo.c
    hglobalstream.c
    marshal.c
    moniker.c
    ole2.c
    ole_server.c
    propvariant.c
    stg_prop.c
    storage32.c
    usrmarshal.c
    ${TESTLIB_OUTPUT_DIR}/ole32_winetest.rc)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(ole32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(ole32_winetest uuid)
set_module_type(ole32_winetest win32cui)
add_importlibs(ole32_winetest oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(ole32_winetest ntdll)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(ole32_winetest PRIVATE -Wno-format-overflow -Wno-discarded-qualifiers)
endif()

add_pch(ole32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET ole32_winetest)

set_source_files_properties(ole32_winetest.rc OBJECT_DEPENDS testlib)

