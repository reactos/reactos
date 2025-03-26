
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    concfg
    consrv)

list(APPEND CONSRV_SOURCE
    consrv/alias.c
    consrv/coninput.c
    consrv/conoutput.c
    consrv/console.c
    consrv/frontendctl.c
    consrv/handle.c
    consrv/history.c
    consrv/init.c
    consrv/lineinput.c
    consrv/popup.c
    consrv/settings.c
    consrv/shutdown.c
    consrv/subsysreg.c
    consrv/condrv/coninput.c
    consrv/condrv/conoutput.c
    consrv/condrv/console.c
    consrv/condrv/dummyterm.c
    consrv/condrv/graphics.c
    consrv/condrv/text.c
    consrv/frontends/input.c
    consrv/frontends/terminal.c
    consrv/frontends/wcwidth.c
    consrv/frontends/gui/conwnd.c
    consrv/frontends/gui/fullscreen.c
    consrv/frontends/gui/guiterm.c
    consrv/frontends/gui/guisettings.c
    consrv/frontends/gui/graphics.c
    consrv/frontends/gui/text.c
    consrv/frontends/tui/tuiterm.c
    # consrv/consrv.rc
    consrv/consrv.h)

#
# Explicitly enable MS extensions to be able to use unnamed (anonymous) nested structs.
#
# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    ## NOTE: No need to specify it as we use MSVC :)
    ##target_compile_options(consrv PRIVATE "/Ze")
    #set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_OPTIONS "/Ze")
else()
    #target_compile_options(consrv PRIVATE "-fms-extensions")
    set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_OPTIONS "-fms-extensions")
endif()

add_library(consrv ${CONSRV_SOURCE})
target_link_libraries(consrv pseh)
add_dependencies(consrv psdk)
add_pch(consrv consrv/consrv.h CONSRV_SOURCE)
#add_object_library(consrv ${CONSRV_SOURCE})
#list(APPEND CONSRV_IMPORT_LIBS)
list(APPEND CONSRV_DELAY_IMPORT_LIBS shell32 ole32 psapi)
list(APPEND CONSRV_TARGET_LINK_LIBS concfg uuid)
set_module_type(consrv module UNICODE)
