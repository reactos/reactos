
add_definitions(
    -DUSE_WINE_TODOS
    -D__WINESRC__)

if(MSVC_IDE)
    # msi_winetest.rc: let rc.exe find custom.dll in its subdirectory, i.e. Debug.
    include_directories($<TARGET_FILE_DIR:custom>)
endif()

if(GCC AND GCC_VERSION VERSION_GREATER 7)
    add_compile_flags("-Wno-format-overflow")
endif()

spec2def(custom.dll custom.spec)
add_library(custom MODULE custom.c ${CMAKE_CURRENT_BINARY_DIR}/custom.def)
target_link_libraries(custom uuid)
set_module_type(custom win32dll)
add_importlibs(custom msi ole32 oleaut32 shell32 advapi32 odbccp32 msvcrt kernel32)

spec2def(selfreg.dll selfreg.spec)
add_library(selfreg MODULE selfreg.c ${CMAKE_CURRENT_BINARY_DIR}/selfreg.def)
target_link_libraries(selfreg uuid)
set_module_type(selfreg win32dll)
add_importlibs(selfreg msi ole32 shell32 advapi32 odbccp32 msvcrt kernel32)

generate_idl_iids(typelib.idl)
add_idl_headers(msi_winetest_idlheaders typelib.idl)

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
    ${CMAKE_CURRENT_BINARY_DIR}/typelib_i.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.9.0")
    # CMake 3.9 and higher requires to specify this dependency manually
    # see https://gitlab.kitware.com/cmake/cmake/issues/19933
    set_property(SOURCE msi_winetest.rc PROPERTY OBJECT_DEPENDS custom.dll)
endif()

add_executable(msi_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    msi_winetest.rc
    rsrc.rc)

add_typelib(typelib.idl)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/typelib.tlb)
target_link_libraries(msi_winetest uuid)
set_module_type(msi_winetest win32cui)
add_importlibs(msi_winetest secur32 cabinet msi shell32 ole32 oleaut32 user32 advapi32 version odbccp32 msvcrt kernel32)
add_pch(msi_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msi_winetest)
add_dependencies(msi_winetest custom msi_winetest_idlheaders selfreg)
