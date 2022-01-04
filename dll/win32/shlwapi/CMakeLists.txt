
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
    wsprintf.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_stubs.c)

add_library(shlwapi MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    $<TARGET_OBJECTS:shlwapi_autocomp>
    shlwapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def)

# our C++ atlbase.h conflicts with the one from wine, so only use wine includes for C
# Unfortunately, we can't use different includes for C & C++ in VS generator, so use an object library to achieve this
target_include_directories(shlwapi BEFORE PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(shlwapi_autocomp OBJECT autocomp.cpp)
target_link_libraries(shlwapi_autocomp PRIVATE atl_classes)
target_compile_definitions(shlwapi_autocomp PRIVATE $<TARGET_PROPERTY:shlwapi,COMPILE_DEFINITIONS>)
target_compile_options(shlwapi_autocomp PRIVATE $<TARGET_PROPERTY:shlwapi,COMPILE_OPTIONS>)
add_dependencies(shlwapi_autocomp psdk)

set_module_type(shlwapi win32dll UNICODE)
target_link_libraries(shlwapi uuid wine cpprt)
add_delay_importlibs(shlwapi userenv oleaut32 ole32 comdlg32 mpr mlang urlmon shell32 winmm version)
add_importlibs(shlwapi user32 gdi32 advapi32 wininet msvcrt kernel32 ntdll)
add_pch(shlwapi precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET shlwapi DESTINATION reactos/system32 FOR all)
