
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(consrv_new)

list(APPEND CONSRV_SOURCE
    consrv_new/alias.c
    consrv_new/coninput.c
    consrv_new/conoutput.c
    consrv_new/console.c
    consrv_new/frontendctl.c
    consrv_new/handle.c
    consrv_new/init.c
    consrv_new/lineinput.c
    consrv_new/settings.c
    consrv_new/condrv/coninput.c
    consrv_new/condrv/conoutput.c
    consrv_new/condrv/console.c
    consrv_new/condrv/dummyfrontend.c
    consrv_new/condrv/graphics.c
    consrv_new/condrv/text.c
    consrv_new/frontends/input.c
    consrv_new/frontends/gui/guiterm.c
    consrv_new/frontends/gui/guisettings.c
    consrv_new/frontends/gui/graphics.c
    consrv_new/frontends/gui/text.c
    consrv_new/frontends/tui/tuiterm.c
    # consrv_new/consrv.rc
    )

#
# Explicitely enable MS extensions to be able to use unnamed (anonymous) nested structs.
#
# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    ## NOTE: No need to specify it as we use MSVC :)
    ##add_target_compile_flags(consrv_new "/Ze")
    #set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_FLAGS "/Ze")
else()
    #add_target_compile_flags(consrv_new "-fms-extensions")
    set_source_files_properties(${CONSRV_SOURCE} PROPERTIES COMPILE_FLAGS "-fms-extensions")
endif()

add_library(consrv_new ${CONSRV_SOURCE})
#add_object_library(consrv_new ${CONSRV_SOURCE})
add_importlibs(consrv_new psapi)         # And the default ones from winsrv
add_delay_importlibs(consrv_new ole32)   # And the default ones from winsrv
target_link_libraries(consrv_new uuid)   # And the default ones from winsrv
set_module_type(consrv_new module UNICODE)
