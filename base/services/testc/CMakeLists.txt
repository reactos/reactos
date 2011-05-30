
set_unicode()

list(APPEND SOURCE
    Helloc.c
	hello_c.c)

add_executable(testc ${SOURCE})
add_definitions(-D__USE_CRTIMP)
target_link_libraries(testc ${PSEH_LIB})
set_module_type(testc win32cui)
add_importlibs(testc advapi32 rpcrt4 msvcrt kernel32 ntdll user32 ksuser setupapi)
add_cab_target(testc 1)
