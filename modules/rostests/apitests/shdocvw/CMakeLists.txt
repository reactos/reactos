
list(APPEND SOURCE
    MRUList.cpp
    WinList.cpp
    testlist.c)

add_executable(shdocvw_apitest ${SOURCE})
set_module_type(shdocvw_apitest win32cui)
target_link_libraries(shdocvw_apitest ${PSEH_LIB} uuid)
add_importlibs(shdocvw_apitest shlwapi shell32 oleaut32 ole32 user32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET shdocvw_apitest)
