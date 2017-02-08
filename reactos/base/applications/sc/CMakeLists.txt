
add_definitions(-DDEFINE_GUID)

list(APPEND SOURCE
    control.c
    create.c
    delete.c
    print.c
    query.c
    sc.c
    start.c
    usage.c
    sc.h)

add_executable(sc ${SOURCE} sc.rc)
set_module_type(sc win32cui UNICODE)
add_importlibs(sc advapi32 msvcrt kernel32)
add_pch(sc sc.h SOURCE)
add_cd_file(TARGET sc DESTINATION reactos/system32 FOR all)
