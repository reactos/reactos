
list(APPEND SOURCE
    common.c
    stacktrace.c
    loadconfig.h)

add_executable(loadconfig_apitest ${SOURCE} testlist.c)
target_link_libraries(loadconfig_apitest wine ${PSEH_LIB})
set_module_type(loadconfig_apitest win32cui)
add_importlibs(loadconfig_apitest msvcrt kernel32 ntdll)
add_pch(loadconfig_apitest loadconfig.h SOURCE)
add_rostests_file(TARGET loadconfig_apitest)
