
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wldap32 ${CMAKE_CURRENT_SOURCE_DIR}/wldap32.spec ${CMAKE_CURRENT_BINARY_DIR}/wldap32.def)

add_library(wldap32 SHARED
    add.c
    ber.c
    bind.c
    compare.c
    control.c
    delete.c
    dn.c
    error.c
    extended.c
    init.c
    main.c
    misc.c
    modify.c
    modrdn.c
    option.c
    page.c
    parse.c
    rename.c
    search.c
    value.c
    wldap32.rc)

set_module_type(wldap32 win32dll)

target_link_libraries(wldap32
     ${CMAKE_CURRENT_BINARY_DIR}/wldap32.def
     wine)

add_importlibs(wldap32 user32 kernel32 ntdll)
add_dependencies(wldap32 wldap32_def)
add_livecd_target(wldap32 reactos/system32)
