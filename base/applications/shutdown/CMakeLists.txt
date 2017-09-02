
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

list(APPEND SOURCE
    gui.c
    misc.c
    shutdown.c
    precomp.h)

add_executable(shutdown ${SOURCE} shutdown.rc)
set_module_type(shutdown win32cui UNICODE)
target_link_libraries(shutdown conutils ${PSEH_LIB})
add_importlibs(shutdown advapi32 user32 powrprof msvcrt kernel32)
add_pch(shutdown precomp.h SOURCE)
add_cd_file(TARGET shutdown DESTINATION reactos/system32 FOR all)
