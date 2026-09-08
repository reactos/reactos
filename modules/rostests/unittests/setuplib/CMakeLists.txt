
PROJECT(setuplib_unittest)

include_directories(${REACTOS_SOURCE_DIR}/modules/rostests/apitests/include)

list(APPEND SOURCE
    IsValidInstallDirectory.c
    testlist.c
    precomp.h)

add_executable(setuplib_unittest ${SOURCE})
#add_pch(setuplib_unittest precomp.h SOURCE)
target_link_libraries(setuplib_unittest setuplib_static ${PSEH_LIB})
set_module_type(setuplib_unittest win32cui)
add_importlibs(setuplib_unittest msvcrt kernel32 ntdll)
add_rostests_file(TARGET setuplib_unittest)
