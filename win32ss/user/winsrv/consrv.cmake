
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(consrv)

list(APPEND CONSRV_SOURCE
    consrv/alias.c
    consrv/coninput.c
    consrv/conoutput.c
    consrv/console.c
    consrv/frontendctl.c
    consrv/handle.c
    consrv/init.c
    consrv/lineinput.c
    consrv/settings.c
    consrv/condrv/coninput.c
    consrv/condrv/conoutput.c
    consrv/condrv/console.c
    consrv/condrv/dummyfrontend.c
    consrv/condrv/graphics.c
    consrv/condrv/text.c
    consrv/frontends/input.c
    consrv/frontends/gui/guiterm.c
    consrv/frontends/gui/guisettings.c
    consrv/frontends/gui/graphics.c
    consrv/frontends/gui/text.c
    consrv/frontends/tui/tuiterm.c
    # consrv/consrv.rc
    )

#
# Explicitely enable MS extensions to be able to use unnamed (anonymous) nested structs.
#
# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    ## NOTE: No need to specify it as we use MSVC :)
    ##add_target_compile_flags(consrv "/Ze")
    #set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_FLAGS "/Ze")
else()
    #add_target_compile_flags(consrv "-fms-extensions")
    set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_FLAGS "-fms-extensions")
endif()

add_library(consrv ${CONSRV_SOURCE})
#add_object_library(consrv ${CONSRV_SOURCE})

add_importlibs(consrv psapi)         # And the default ones from winsrv
add_delay_importlibs(consrv ole32)   # And the default ones from winsrv
target_link_libraries(consrv uuid)   # And the default ones from winsrv

set_module_type(consrv module UNICODE)
