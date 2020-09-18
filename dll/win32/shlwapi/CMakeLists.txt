
project(SHELL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_SHLWAPI_)

spec2def(shlwapi.dll shlwapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    assoc.c
    clist.c
    istream.c
    msgbox.c
    ordinal.c
    path.c
    reg.c
    regstream.c
    rosordinal.c
    shlwapi_main.c
    stopwatch.c
    string.c
    thread.c
    url.c)

list(APPEND PCH_SKIP_SOURCE
    autocomp.cpp
    wsprintf.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_stubs.c)

add_library(shlwapi MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    shlwapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def)

# our C++ atlbase.h conflicts with the one from wine, so only use wine includes for C
target_include_directories(shlwapi BEFORE PRIVATE $<$<COMPILE_LANGUAGE:C>:${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine>)

set_module_type(shlwapi win32dll UNICODE)
target_link_libraries(shlwapi uuid wine cpprt atl_classes)
add_delay_importlibs(shlwapi userenv oleaut32 ole32 comdlg32 mpr mlang urlmon shell32 winmm version)
add_importlibs(shlwapi user32 gdi32 advapi32 wininet msvcrt kernel32 ntdll)
add_pch(shlwapi precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET shlwapi DESTINATION reactos/system32 FOR all)
