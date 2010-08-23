
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciwave ${CMAKE_CURRENT_SOURCE_DIR}/mciwave.spec ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def)

add_library(mciwave SHARED mciwave.c)

set_target_properties(mciwave PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(mciwave
    ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def
    wine
    -lwinmm
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a)

add_dependencies(mciwave mciwave_def psdk)
