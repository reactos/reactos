include_directories(
    include)

set_rc_compiler()

add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND KMTEST_SOURCE
    kmtest/kmtest.c
    kmtest/service.c
    kmtest/kmtest.rc)

add_executable(kmtest ${KMTEST_SOURCE})
set_module_type(kmtest win32cui)
add_importlibs(kmtest advapi32 msvcrt kernel32)

add_cd_file(TARGET kmtest DESTINATION reactos/bin FOR all)
