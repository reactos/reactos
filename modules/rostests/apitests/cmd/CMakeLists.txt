
list(APPEND SOURCE
    cmd.c
    precomp.h)

add_executable(cmd_apitest ${SOURCE} testlist.c)
target_link_libraries(cmd_apitest wine ${PSEH_LIB})
set_module_type(cmd_apitest win32cui)
add_importlibs(cmd_apitest msvcrt kernel32)
add_pch(cmd_apitest precomp.h SOURCE)
add_rostests_file(TARGET cmd_apitest)
