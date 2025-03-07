
PROJECT(setuplib_unittest)

include_directories(
    ${REACTOS_SOURCE_DIR}/modules/rostests/apitests/include
    ${REACTOS_SOURCE_DIR}/base/setup/lib)

list(APPEND SOURCE
    IsValidInstallDirectory.c
    testlist.c
    precomp.h)

add_executable(setuplib_unittest ${SOURCE})
set_module_type(setuplib_unittest win32cui)
add_importlibs(setuplib_unittest setuplib msvcrt kernel32)
#add_pch(setuplib_unittest precomp.h SOURCE)
add_rostests_file(TARGET setuplib_unittest)
