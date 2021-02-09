
list(APPEND SOURCE
    cmds.c
    cmdtab.c
    domacro.c
    fake.c
    ftp.c
    main.c
    ruserpass.c
    precomp.h)

add_executable(ftp ${SOURCE} ftp.rc)
target_compile_definitions(ftp PRIVATE lint)

set_module_type(ftp win32cui)
add_importlibs(ftp ws2_32 iphlpapi msvcrt kernel32)
target_link_libraries(ftp oldnames)
add_pch(ftp precomp.h SOURCE)

if(MSVC)
    add_importlibs(ftp ntdll)
endif()

add_cd_file(TARGET ftp DESTINATION reactos/system32 FOR all)
