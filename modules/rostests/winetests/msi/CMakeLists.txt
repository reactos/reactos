
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
    suminfo.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

# CMake 3.9 and higher requires to specify this dependency manually
# see https://gitlab.kitware.com/cmake/cmake/issues/19933
set_property(SOURCE msi_winetest.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/custom.dll)

add_executable(msi_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    msi_winetest.rc)

target_compile_definitions(msi_winetest PRIVATE __WINESRC__ USE_WINE_TODOS)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(msi_winetest PRIVATE -Wno-format-overflow)
endif()

# msi_winetest.rc: let rc.exe find custom.dll in its subdirectory, i.e. Debug.
if (MSVC_IDE)
    target_include_directories(msi_winetest PRIVATE $<$<COMPILE_LANGUAGE:RC>:$<TARGET_FILE_DIR:custom>>)
endif()

target_link_libraries(msi_winetest uuid)
set_module_type(msi_winetest win32cui)
add_importlibs(msi_winetest cabinet msi shell32 ole32 oleaut32 user32 advapi32 version msvcrt kernel32)
add_pch(msi_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msi_winetest)
add_dependencies(msi_winetest custom)
