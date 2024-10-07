
spec2def(custom.dll custom.spec)
add_library(msi_custom MODULE custom.c ${CMAKE_CURRENT_BINARY_DIR}/custom.def)
set_target_properties(msi_custom PROPERTIES OUTPUT_NAME "custom")
target_link_libraries(msi_custom uuid)
set_module_type(msi_custom win32dll)
add_importlibs(msi_custom msi ole32 oleaut32 odbccp32 shell32 advapi32 msvcrt kernel32)

spec2def(selfreg.dll selfreg.spec)
add_library(msi_selfreg MODULE selfreg.c ${CMAKE_CURRENT_BINARY_DIR}/selfreg.def)
set_target_properties(msi_selfreg PROPERTIES OUTPUT_NAME "selfreg")
set_module_type(msi_selfreg win32dll)
add_importlibs(msi_selfreg advapi32 msvcrt kernel32)

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
    typelib.idl)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/typelib_i.c
    testlist.c)

# CMake 3.9 and higher requires to specify this dependency manually
# see https://gitlab.kitware.com/cmake/cmake/issues/19933
set_property(SOURCE msi_winetest.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/custom.dll  ${CMAKE_CURRENT_BINARY_DIR}/selfreg.dll ${CMAKE_CURRENT_BINARY_DIR}/typelib.tlb)

add_executable(msi_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    msi_winetest.rc)

add_idl_headers(msi_winetest_idlheader typelib.idl)
generate_idl_iids(typelib.idl)
add_typelib(typelib.idl)

target_compile_definitions(msi_winetest PRIVATE __WINESRC__ USE_WINE_TODOS strcasecmp=_stricmp)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(msi_winetest PRIVATE -Wno-format-overflow -Wno-format)
    target_compile_options(msi_custom PRIVATE -Wno-format)
endif()

# msi_winetest.rc: let rc.exe find custom.dll in its subdirectory, i.e. Debug.
if (MSVC_IDE)
    target_include_directories(msi_winetest PRIVATE $<$<COMPILE_LANGUAGE:RC>:$<TARGET_FILE_DIR:msi_custom>>)
endif()

target_link_libraries(msi_winetest uuid)
set_module_type(msi_winetest win32cui)
add_importlibs(msi_winetest secur32 cabinet msi shell32 ole32 oleaut32 odbccp32 user32 advapi32 version msvcrt kernel32)
add_pch(msi_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msi_winetest)
add_dependencies(msi_winetest msi_custom msi_selfreg msi_winetest_idlheader stdole2)
