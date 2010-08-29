
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(shlwapi ${CMAKE_CURRENT_SOURCE_DIR}/shlwapi.spec ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def)

add_library(shlwapi SHARED
    assoc.c
    clist.c
    istream.c
    msgbox.c
    ordinal.c
    path.c
    reg.c
    regstream.c
    shlwapi_main.c
    stopwatch.c
    string.c
    thread.c
    url.c
    wsprintf.c
    shlwapi.rc)

set_target_properties(shlwapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(shlwapi
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def
    wine
    uuid
    -luser32
    -lgdi32
    -ladvapi32
    -loleaut32
    -lole32
    -lcomctl32
    -lcomdlg32
    ${REACTOS_SOURCE_DIR}/dll/win32/mpr/libmpr.a
    ${REACTOS_SOURCE_DIR}/dll/win32/mlang/libmlang.a
    ${REACTOS_SOURCE_DIR}/dll/win32/urlmon/liburlmon.a
    ${REACTOS_SOURCE_DIR}/dll/win32/shell32/libshell32.a
    -lwinmm
    -lversion
    mingw_common)

add_dependencies(shlwapi shlwapi_def psdk)
