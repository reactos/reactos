
list(APPEND SOURCE
    CachedGetUserFromSid.c)

add_executable(utildll_apitest
    ${SOURCE}
    testlist.c)

set_module_type(utildll_apitest win32cui)
add_importlibs(utildll_apitest msvcrt kernel32)
add_rostests_file(TARGET utildll_apitest)
