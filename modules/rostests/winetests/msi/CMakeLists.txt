
add_definitions(
    -DUSE_WINE_TODOS
    -D__WINESRC__)

if(MSVC_IDE)
    # msi_winetest.rc: let rc.exe find custom.dll in its subdirectory, i.e. Debug.
    include_directories($<TARGET_FILE_DIR:custom>)
endif()

spec2def(custom.dll custom.spec)
add_library(custom MODULE custom.c ${CMAKE_CURRENT_BINARY_DIR}/custom.def)
target_link_libraries(custom uuid)
set_module_type(custom win32dll)
add_importlibs(custom msi ole32 msvcrt kernel32)

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
    precomp.h)

add_executable(msi_winetest ${SOURCE} testlist.c msi_winetest.rc)
target_link_libraries(msi_winetest uuid)
set_module_type(msi_winetest win32cui)
add_importlibs(msi_winetest cabinet msi shell32 ole32 oleaut32 user32 advapi32 version msvcrt kernel32)
add_pch(msi_winetest precomp.h SOURCE)
add_rostests_file(TARGET msi_winetest)
add_dependencies(msi_winetest custom)
