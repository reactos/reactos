
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(oledlg ${CMAKE_CURRENT_SOURCE_DIR}/oledlg.spec ${CMAKE_CURRENT_BINARY_DIR}/oledlg.def)

add_library(oledlg SHARED
    insobjdlg.c
    oledlg_main.c
    pastespl.c
    rsrc.rc)

set_target_properties(oledlg PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(oledlg
    ${CMAKE_CURRENT_BINARY_DIR}/oledlg.def
    wine
    -lole32
    -lcomdlg32
    -luser32
    -ladvapi32
    mingw_common)

add_dependencies(oledlg oledlg_def psdk)
