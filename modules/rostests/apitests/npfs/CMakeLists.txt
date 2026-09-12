list(APPEND SOURCE
    GetConnectionAttributes.c
    testlist.c)

add_executable(npfs_apitest ${SOURCE})
set_module_type(npfs_apitest win32cui)
target_link_libraries(npfs_apitest winetestlib)
add_importlibs(npfs_apitest msvcrt kernel32 ntdll)

add_rostests_file(TARGET npfs_apitest)
