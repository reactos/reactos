
list(APPEND SOURCE
    DisplayModes.c
    precomp.h)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(videoprt_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(videoprt_apitest wine ${PSEH_LIB})
set_module_type(videoprt_apitest win32cui)
add_importlibs(videoprt_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_pch(videoprt_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET videoprt_apitest)
