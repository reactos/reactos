
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(msi.dll msi.spec ADD_IMPORTLIB)
generate_idl_iids(msiserver.idl)

set(OLD_IDL_FLAGS ${IDL_FLAGS})
set(IDL_FLAGS ${IDL_FLAGS} --prefix-server=s_)
add_idl_headers(msi_idlheaders msiserver.idl winemsi.idl)
add_rpc_files(client winemsi.idl)
add_rpc_files(server winemsi.idl)
set(IDL_FLAGS ${OLD_IDL_FLAGS})

if(MSVC)
    #  error C4133: 'function': incompatible types - from 'UINT *' to 'MSIINSTALLCONTEXT *'
    replace_compile_flags("/we4133" " ")

    # error C4312: 'type cast': conversion from 'unsigned int' to 'HANDLE' of greater size
    replace_compile_flags("/we4312" " ")
endif()

list(APPEND SOURCE
    action.c
    alter.c
    appsearch.c
    assembly.c
    automation.c
    classes.c
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
    storages.c
    streams.c
    string.c
    suminfo.c
    table.c
    tokenize.c
    update.c
    upgrade.c
    where.c
    precomp.h)

# cond.tab.c/sql.tab.c have been generated with relative file paths...
set_source_files_properties(cond.tab.c sql.tab.c PROPERTIES COMPILE_FLAGS "-UREACTOS_SOURCE_DIR -DREACTOS_SOURCE_DIR=\"\\\".\\\"\"")

add_library(msi MODULE
    ${SOURCE}
    cond.tab.c
    sql.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/msiserver_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/msi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winemsi_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/winemsi_s.c
    msi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msi.def)

add_typelib(msiserver.idl)
set_source_files_properties(msi.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msiserver.tlb)
set_module_type(msi win32dll)
target_link_libraries(msi uuid ${PSEH_LIB} wine)
add_dependencies(msi msi_idlheaders)
add_delay_importlibs(msi odbccp32 wintrust crypt32 mspatcha)
add_importlibs(msi urlmon wininet comctl32 shell32 shlwapi cabinet oleaut32 ole32 version user32 gdi32 advapi32 advapi32_vista rpcrt4 sxs
    #FIXME : should be in delayed imports
    imagehlp
    msvcrt
    kernel32_vista
    kernel32
    ntdll)

add_pch(msi precomp.h SOURCE)
add_cd_file(TARGET msi DESTINATION reactos/system32 FOR all)
