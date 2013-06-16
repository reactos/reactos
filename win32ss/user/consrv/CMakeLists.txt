
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
    graphics.c
    text.c
    console.c
    handle.c
    init.c
    lineinput.c
    settings.c
    frontends/gui/guiterm.c
    frontends/gui/guisettings.c
    frontends/gui/graphics.c
    frontends/gui/text.c
    frontends/tui/tuiterm.c
    ${CMAKE_CURRENT_BINARY_DIR}/consrv.def)

add_library(consrv SHARED
    ${SOURCE}
    consrv.rc)

#
# Explicitely enable MS extensions to be able to use unnamed (anonymous) nested structs.
#
# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    ## NOTE: No need to specify it as we use MSVC :)
    ##add_target_compile_flags(consrv "/Ze")
    #set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/Ze")
else()
    #add_target_compile_flags(consrv "-fms-extensions")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fms-extensions")
endif()

target_link_libraries(consrv win32ksys ${PSEH_LIB} uuid) # win32ksys because of NtUser...()

set_module_type(consrv win32dll UNICODE)

add_importlibs(consrv psapi msvcrt kernel32 ntdll csrsrv)
add_delay_importlibs(consrv user32 gdi32 advapi32 ole32)

add_dependencies(consrv bugcodes)
add_cd_file(TARGET consrv DESTINATION reactos/system32 FOR all)
