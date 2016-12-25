
add_definitions(
    -DUSE_WINE_TODOS
    -D__WINESRC__)

list(APPEND SOURCE
    action.c
    automation.c
    db.c
    format.c
    install.c
    msi.c
    package.c
    patch.c
    record.c
    source.c
    suminfo.c
    testlist.c)

add_executable(msi_winetest ${SOURCE})
target_link_libraries(msi_winetest uuid)
set_module_type(msi_winetest win32cui)
add_importlibs(msi_winetest cabinet msi shell32 ole32 oleaut32 user32 advapi32 version msvcrt kernel32)
add_cd_file(TARGET msi_winetest DESTINATION reactos/bin FOR all)
