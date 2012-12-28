
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(inetcpl.cpl inetcpl.spec)

list(APPEND SOURCE
    inetcpl.c
    content.c
    general.c
    security.c
    inetcpl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/inetcpl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetcpl.def)

add_library(inetcpl SHARED ${SOURCE})
set_module_type(inetcpl cpl UNICODE)
target_link_libraries(inetcpl wine)
add_importlibs(inetcpl advapi32 comctl32 user32 gdi32 shlwapi msvcrt kernel32 ntdll)
add_delay_importlibs(inetcpl cryptui wininet ole32 urlmon)
add_cd_file(TARGET inetcpl DESTINATION reactos/system32 FOR all)
