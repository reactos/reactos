
set_unicode()

list(APPEND SOURCE
    Hellos.c
	Hellop.c
	hello_s.c)

add_executable(tests ${SOURCE})
add_definitions(-D__USE_CRTIMP)
target_link_libraries(tests ${PSEH_LIB})

set_module_type(tests win32cui)
add_importlibs(tests advapi32 rpcrt4 msvcrt kernel32 ntdll user32 ksuser setupapi)
add_cab_target(tests 1)
