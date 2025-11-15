
add_definitions(-DUSE_WINE_TODOS)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(ole32_testlib.dll testlib.spec)

add_library(ole32_testlib MODULE testlib.c ${CMAKE_CURRENT_BINARY_DIR}/ole32_testlib.def)
set_module_type(ole32_testlib win32dll)
add_importlibs(ole32_testlib msvcrt kernel32)

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
    ole32_winetest.rc)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(ole32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

# CMake 3.9 and higher requires to specify this dependency manually
# see https://gitlab.kitware.com/cmake/cmake/issues/19933
set_property(SOURCE ole32_winetest.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ole32_testlib.dll)

# ole32_winetest.rc: let rc.exe find ole32_testlib.dll in its subdirectory, i.e. Debug.
if (MSVC_IDE)
    target_include_directories(ole32_winetest PRIVATE $<$<COMPILE_LANGUAGE:RC>:$<TARGET_FILE_DIR:ole32_testlib>>)
endif()

target_link_libraries(ole32_winetest uuid)
set_module_type(ole32_winetest win32cui)
add_importlibs(ole32_winetest oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(ole32_winetest ole32_testlib)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(ole32_winetest PRIVATE -Wno-format-overflow -Wno-discarded-qualifiers)
endif()

add_pch(ole32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET ole32_winetest)
