
add_executable(wininet_apitest InternetOpen.c testlist.c)
target_link_libraries(wininet_apitest wine)
set_module_type(wininet_apitest win32cui)
#add_delay_importlibs(wininet_apitest wininet)
add_importlibs(wininet_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET wininet_apitest)
