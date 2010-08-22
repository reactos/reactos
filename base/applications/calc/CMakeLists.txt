
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

target_link_libraries(calc mingw_wmain
                      ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
                      ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
                      ${REACTOS_SOURCE_DIR}/dll/win32/gdi32/libgdi32.a
                      mingw_common
                      -lmsvcrt)

add_dependencies(calc psdk)
