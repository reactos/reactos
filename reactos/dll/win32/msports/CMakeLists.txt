
set_unicode()

spec2def(msports.dll msports.spec)

list(APPEND SOURCE
    classinst.c
    msports.c
    parallel.c
    serial.c
    msports.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msports_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msports.def)

add_library(msports SHARED ${SOURCE})

set_module_type(msports win32dll)

target_link_libraries(msports wine)

add_importlibs(msports comctl32 setupapi advapi32 user32 kernel32 ntdll)

add_cd_file(TARGET msports DESTINATION reactos/system32 FOR all)
add_importlib_target(msports.spec)
