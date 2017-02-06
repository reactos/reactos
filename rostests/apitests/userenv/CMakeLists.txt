
list(APPEND SOURCE
    GetProfileDirs.c
    LoadUserProfile.c
    testlist.c)

add_executable(userenv_apitest ${SOURCE})
target_link_libraries(userenv_apitest wine ${PSEH_LIB})
set_module_type(userenv_apitest win32cui)
add_importlibs(userenv_apitest userenv advapi32 msvcrt kernel32)
add_rostests_file(TARGET userenv_apitest)
