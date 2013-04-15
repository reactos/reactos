
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(consrv.dll consrv.spec)

list(APPEND SOURCE
    alias.c
    coninput.c
    conoutput.c
    console.c
    handle.c
    init.c
    lineinput.c
    settings.c
    consrv.rc
    frontends/gui/guiterm.c
    frontends/gui/guisettings.c
    frontends/tui/tuiterm.c
    ${CMAKE_CURRENT_BINARY_DIR}/consrv.def)

add_library(consrv SHARED ${SOURCE})

target_link_libraries(consrv win32ksys ${PSEH_LIB} uuid) # win32ksys because of NtUser...()

set_module_type(consrv win32dll UNICODE)

add_importlibs(consrv psapi msvcrt kernel32 ntdll csrsrv)
add_delay_importlibs(consrv user32 gdi32 advapi32 ole32)

add_dependencies(consrv bugcodes)
add_cd_file(TARGET consrv DESTINATION reactos/system32 FOR all)
