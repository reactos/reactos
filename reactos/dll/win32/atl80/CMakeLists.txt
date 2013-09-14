
add_definitions(
    -D__WINESRC__
    -D_ATL_VER=_ATL_VER_80)

spec2def(atl80.dll atl80.spec)

list(APPEND SOURCE
    atl80.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl80_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/atl80.def)

add_library(atl80 SHARED ${SOURCE})
set_module_type(atl80 win32dll)
target_link_libraries(atl80 wine)
add_importlibs(atl80 atl100 oleaut32 user32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET atl80 DESTINATION reactos/system32 FOR all)
