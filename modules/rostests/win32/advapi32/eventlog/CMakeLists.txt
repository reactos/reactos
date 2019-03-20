## Original MSVC commands:
## mc -U MyEventProvider.mc
## rc /fo $(IntDir)MyEventProvider.res MyEventProvider.rc
## link /dll /noentry /SUBSYSTEM:CONSOLE",5.0" /out:$(SolutionDir)$(Configuration)\MyEventProvider.dll $(IntDir)MyEventProvider.res

add_message_headers(UNICODE MyEventProvider.mc)
add_library(MyEventProvider_dll MODULE ${CMAKE_CURRENT_BINARY_DIR}/MyEventProvider.rc)
add_dependencies(MyEventProvider_dll MyEventProvider)
set_module_type(MyEventProvider_dll module UNICODE)
set_target_properties(MyEventProvider_dll PROPERTIES OUTPUT_NAME "MyEventProvider")
add_rostests_file(TARGET MyEventProvider_dll SUBDIR suppl)

add_executable(evtlogtest EvtLogTest.c)
set_module_type(evtlogtest win32cui UNICODE)
add_dependencies(evtlogtest MyEventProvider MyEventProvider_dll)
add_importlibs(evtlogtest advapi32 msvcrt kernel32)
add_rostests_file(TARGET evtlogtest SUBDIR suppl)
