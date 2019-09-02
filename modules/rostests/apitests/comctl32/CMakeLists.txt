
add_executable(comctl32_apitest button.c propsheet.c toolbar.c testlist.c ../include/msgtrace.c comctl32_apitest.rc)
target_link_libraries(comctl32_apitest wine)
set_module_type(comctl32_apitest win32cui)
add_importlibs(comctl32_apitest uxtheme comctl32 user32 gdi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET comctl32_apitest)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/comctl32v5.manifest")
