
add_definitions(
    -D__WINESRC__
    -DWIN32)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libmpg123)

spec2def(winemp3.acm winemp3.acm.spec)

add_library(winemp3.acm MODULE
    mpegl3.c
    ${CMAKE_CURRENT_BINARY_DIR}/winemp3.def)

set_module_type(winemp3.acm win32dll)
set_target_properties(winemp3.acm PROPERTIES SUFFIX "")
target_link_libraries(winemp3.acm wine libmpg123 oldnames)
add_importlibs(winemp3.acm shlwapi winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET winemp3.acm DESTINATION reactos/system32 FOR all)
