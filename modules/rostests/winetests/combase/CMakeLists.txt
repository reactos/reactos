
add_definitions(-D__ROS_LONG64__)
add_definitions(-DUSE_WINE_TODOS)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(wine.combase.test.dll wine.combase.test.spec)

if(DLL_EXPORT_VERSION GREATER_EQUAL 0xA00)
    set(COMBASE_NAME "combase")
else()
    set(COMBASE_NAME "combase_win10")
endif()

list(APPEND SOURCE
    roapi.c
    string.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(combase_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(combase_winetest uuid)
set_module_type(combase_winetest win32cui)
add_importlibs(combase_winetest oleaut32 ole32 ${COMBASE_NAME} user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(combase_winetest ntdll)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(combase_winetest PRIVATE -Wno-format-overflow)
endif()

#add_pch(combase_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET combase_winetest)

add_library(wine.combase.test MODULE wine.combase.test.c ${CMAKE_CURRENT_BINARY_DIR}/wine.combase.test.def)
target_link_libraries(wine.combase.test wine)
set_module_type(wine.combase.test win32dll)
add_importlibs(wine.combase.test ${COMBASE_NAME} msvcrt kernel32 ntdll)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(wine.combase.test PRIVATE -Werror=discarded-qualifiers)
endif()

add_rostests_file(TARGET wine.combase.test)

