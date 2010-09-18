
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DDISABLE_HTMLHELP_SUPPORT=1)

add_executable(calc WIN32 
    about.c
    convert.c
    function.c
    rpn.c
    utl.c
    winmain.c
    resource.rc)

target_link_libraries(calc mingw_wmain mingw_common
                      -ladvapi32
                      -luser32
                      -lgdi32
                      -lmsvcrt)

set_target_properties(calc PROPERTIES LINK_FLAGS "-Wl,-entry,_WinMainCRTStartup")

add_dependencies(calc psdk)
