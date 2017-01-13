
add_executable(sdk_apitest delayimp.cpp testlist.c)
set_module_type(sdk_apitest win32cui)
target_link_libraries(sdk_apitest ${PSEH_LIB})
add_importlibs(sdk_apitest msvcrt kernel32 ntdll)
add_delay_importlibs(sdk_apitest winmm version dbghelp shlwapi sfc_os imagehlp)
add_rostests_file(TARGET sdk_apitest)
