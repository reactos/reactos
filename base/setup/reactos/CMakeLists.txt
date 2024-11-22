
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
#    ${REACTOS_SOURCE_DIR}/base/setup/lib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    spapisup/fileqsup.c
    spapisup/infsupp.c
    drivepage.c
    reactos.c
    treelist.c
    reactos.h)

file(GLOB reactos_rc_deps res/*.*)
add_rc_deps(reactos.rc ${reactos_rc_deps})
add_executable(reactos ${SOURCE} reactos.rc)

add_pch(reactos reactos.h SOURCE)
set_module_type(reactos win32gui UNICODE)
target_link_libraries(reactos uuid)
target_link_libraries(reactos zlib_solo) ## We use USETUP's cabinet implementation
add_importlibs(reactos advapi32 gdi32 user32 comctl32 shlwapi setupapi setuplib msvcrt kernel32 ntdll)
add_cd_file(TARGET reactos DESTINATION reactos NO_CAB FOR bootcd)
