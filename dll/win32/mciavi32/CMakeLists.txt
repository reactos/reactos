
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciavi32 ${CMAKE_CURRENT_SOURCE_DIR}/mciavi32.spec ${CMAKE_CURRENT_BINARY_DIR}/mciavi32.def)

add_library(mciavi32 SHARED
    info.c
    mciavi.c
    mmoutput.c
    wnd.c
    mciavi_res.rc)

set_target_properties(mciavi32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mciavi32
    ${CMAKE_CURRENT_BINARY_DIR}/mciavi32.def
    wine
    -lmsvfw32
    -lwinmm
    mingw_common
    -luser32
    -lgdi32)

add_dependencies(mciavi32 mciavi32_def psdk)
