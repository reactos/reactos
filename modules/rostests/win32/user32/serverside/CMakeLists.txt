
list(APPEND SOURCE
    serverside.c)

add_executable(serverside ${SOURCE})
target_link_libraries(serverside ${PSEH_LIB})
set_module_type(serverside win32gui UNICODE)
add_importlibs(serverside gdi32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET serverside SUBDIR suppl)
