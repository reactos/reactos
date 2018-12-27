
list(APPEND SOURCE
    mspatcha.c
    testlist.c)

# File name does not contain 'patch' on purpose!
# This triggers automatic elevation in Nt 6+

add_executable(msptcha_apitest ${SOURCE})
set_module_type(msptcha_apitest win32cui)
add_importlibs(msptcha_apitest msvcrt kernel32 ntdll)
add_delay_importlibs(msptcha_apitest mspatcha)
add_rostests_file(TARGET msptcha_apitest)
