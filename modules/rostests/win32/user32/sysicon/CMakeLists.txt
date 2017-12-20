
list(APPEND SOURCE
    sysicon.c
    sysicon.rc)

add_executable(sysicon ${SOURCE})
target_link_libraries(sysicon ${PSEH_LIB})
set_module_type(sysicon win32gui UNICODE)
add_importlibs(sysicon gdi32 user32 msvcrt kernel32)
add_rostests_file(TARGET sysicon SUBDIR suppl)
