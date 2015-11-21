
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__ -DMSIRUNMODE=MSIRUNMODE_T)
spec2def(msi.dll msi.spec ADD_IMPORTLIB)
generate_idl_iids(msiserver.idl)

list(APPEND SOURCE
    action.c
    alter.c
    appsearch.c
    assembly.c
    automation.c
    classes.c
    cond.tab.c
    create.c
    custom.c
    database.c
    delete.c
    dialog.c
    distinct.c
    drop.c
    files.c
    font.c
    format.c
    handle.c
    insert.c
    install.c
    media.c
    msi.c
    msi_main.c
    msiquery.c
    package.c
    patch.c
    record.c
    registry.c
    script.c
    select.c
    source.c
    sql.tab.c
    storages.c
    streams.c
    string.c
    suminfo.c
    table.c
    tokenize.c
    update.c
    upgrade.c
    where.c
    msipriv.h)

add_library(msi SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/msiserver_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/msi_stubs.c
    msi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msi.def)

add_idl_headers(msi_idlheader msiserver.idl)
add_typelib(msiserver.idl)
set_source_files_properties(msi.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msiserver.tlb)
set_module_type(msi win32dll)
target_link_libraries(msi uuid ${PSEH_LIB} wine)
add_dependencies(msi msi_idlheader)
add_delay_importlibs(msi odbccp32 crypt32 wintrust)
add_importlibs(msi advapi32 advapi32_vista cabinet comctl32 gdi32 ole32 oleaut32 shell32 shlwapi urlmon user32 version wininet mspatcha
    #FIXME : should be in delayed imports
    imagehlp
    msvcrt
    kernel32_vista
    kernel32
    ntdll)

add_pch(msi msipriv.h SOURCE)
add_cd_file(TARGET msi DESTINATION reactos/system32 FOR all)
