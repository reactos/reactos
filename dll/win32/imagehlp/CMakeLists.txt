
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_IMAGEHLP_SOURCE_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(imagehlp ${CMAKE_CURRENT_SOURCE_DIR}/imagehlp.spec ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def)

list(APPEND SOURCE
    access.c
    imagehlp_main.c
    integrity.c
    modify.c
    imagehlp.rc)

add_library(imagehlp SHARED 
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp_precomp.h.gch
    ${SOURCE})

set_target_properties(imagehlp PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(imagehlp
    ${CMAKE_CURRENT_BINARY_DIR}/imagehlp.def
    wine
    ${REACTOS_SOURCE_DIR}/dll/win32/dbghelp/libdbghelp.a
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a)

add_pch(imagehlp ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(imagehlp imagehlp_def psdk)
