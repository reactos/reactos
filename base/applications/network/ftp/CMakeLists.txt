
add_definitions(
    -D_DLL -D__USE_CRTIMP
    -Dlint)

add_executable(ftp
    cmds.c
    cmdtab.c
    domacro.c
    fake.c
    ftp.c
    main.c
    ruserpass.c
    ftp.rc)

set_module_type(ftp win32cui)
add_importlibs(ftp ws2_32 iphlpapi msvcrt kernel32)

if(MSVC)
    target_link_libraries(ftp oldnames)
    add_importlibs(ftp ntdll)
endif()

add_cab_target(ftp 1)
