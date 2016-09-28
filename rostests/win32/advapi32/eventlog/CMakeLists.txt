## Original MSVC commands:
## mc -U MyEventProvider.mc
## rc /fo $(IntDir)MyEventProvider.res MyEventProvider.rc
## link /dll /noentry /SUBSYSTEM:CONSOLE",5.0" /out:$(SolutionDir)$(Configuration)\MyEventProvider.dll $(IntDir)MyEventProvider.res

## add_message_headers(UNICODE MyEventProvider.mc)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/MyEventProvider.rc ${CMAKE_CURRENT_BINARY_DIR}/MyEventProvider.h
    COMMAND ${CMAKE_MC_COMPILER} -U ${CMAKE_CURRENT_SOURCE_DIR}/MyEventProvider.mc -r ${CMAKE_CURRENT_BINARY_DIR}/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MyEventProvider.mc)

add_library(MyEventProvider SHARED ${CMAKE_CURRENT_BINARY_DIR}/MyEventProvider.rc)
set_module_type(MyEventProvider module UNICODE)
add_cd_file(TARGET MyEventProvider DESTINATION reactos/bin FOR all)

add_executable(evtlogtest EvtLogTest.c)
add_dependencies(evtlogtest MyEventProvider)
set_module_type(evtlogtest win32cui UNICODE)
add_importlibs(evtlogtest advapi32 msvcrt kernel32)
add_cd_file(TARGET evtlogtest DESTINATION reactos/bin FOR all)
