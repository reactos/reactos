
list(APPEND SOURCE
    wsf.c
    testlist.c)

add_executable(wscript_apitest ${SOURCE})
target_link_libraries(wscript_apitest wine ${PSEH_LIB})
set_module_type(wscript_apitest win32cui)
add_importlibs(wscript_apitest shlwapi msvcrt user32 kernel32)
add_rostests_file(TARGET wscript_apitest)
