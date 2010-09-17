
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -Wl,-L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,-entry,_WinMainCRTStartup -Wl,--enable-stdcall-fixup -Wl,--enable-auto-image-base -Wl,--kill-at -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds")

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

add_dependencies(calc psdk)
