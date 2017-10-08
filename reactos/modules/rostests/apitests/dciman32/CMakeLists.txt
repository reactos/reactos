
add_definitions(-D_DLL -D__USE_CRTIMP)
add_executable(dciman32_apitest DCICreatePrimary.c testlist.c)
target_link_libraries(dciman32_apitest wine)
set_module_type(dciman32_apitest win32cui)
add_importlibs(dciman32_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET dciman32_apitest)
