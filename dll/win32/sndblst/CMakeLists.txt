
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DDEBUG_NT4)

spec2def(sndblst ${CMAKE_CURRENT_SOURCE_DIR}/sndblst.spec ${CMAKE_CURRENT_BINARY_DIR}/sndblst.def)

add_library(sndblst SHARED sndblst.c)

set_target_properties(sndblst PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(sndblst
    ${CMAKE_CURRENT_BINARY_DIR}/sndblst.def
    mment4
    mmebuddy
    -luser32
    -lwinmm
    -ladvapi32
    -lkernel32  
    -lntdll)

add_dependencies(sndblst sndblst_def psdk)
