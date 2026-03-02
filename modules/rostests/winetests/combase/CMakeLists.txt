
add_definitions(-D__ROS_LONG64__)
add_definitions(-DUSE_WINE_TODOS)
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(wine.combase.test.dll wine.combase.test.spec)
list(APPEND SOURCE
    roapi.c
    string.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(combase_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    combase.rc)

target_link_libraries(combase_winetest uuid)
set_module_type(combase_winetest win32cui)
add_importlibs(combase_winetest oleaut32 ole32 combase user32 gdi32 advapi32 msvcrt kernel32)
add_dependencies(combase_winetest wine.combase.test)

if(MSVC)
    add_importlibs(combase_winetest ntdll)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(combase_winetest PRIVATE -Wno-format-overflow -Wno-discarded-qualifiers)
endif()

#add_pch(combase_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET combase_winetest)
add_library(wine.combase.test MODULE wine.combase.test.c ${CMAKE_CURRENT_BINARY_DIR}/wine.combase.test.def)
target_link_libraries(wine.combase.test wine)
set_module_type(wine.combase.test win32dll)
add_importlibs(wine.combase.test combase msvcrt kernel32 ntdll)

# combase_winetest.rc: let rc.exe find wine.combase.test.dll in its subdirectory, i.e. Debug.
if (MSVC_IDE)
    target_include_directories(combase_winetest PRIVATE $<$<COMPILE_LANGUAGE:RC>:$<TARGET_FILE_DIR:wine.combase.test>>)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(wine.combase.test PRIVATE -Wno-discarded-qualifiers)
endif()

set_source_files_properties(combase.rc OBJECT_DEPENDS wine.combase.test)

add_rostests_file(TARGET wine.combase.test)
