
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_executable(uiautomationcore_winetest uiautomation.c testlist.c)
set_module_type(uiautomationcore_winetest win32cui)
target_link_libraries(uiautomationcore_winetest wine uuid)
add_importlibs(uiautomationcore_winetest ole32 oleacc oleaut32 propsys user32 uiautomationcore msvcrt kernel32_vista kernel32)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(uiautomationcore_winetest PRIVATE -Wno-format-overflow -Wno-discarded-qualifiers)
endif()

add_rostests_file(TARGET uiautomationcore_winetest)
