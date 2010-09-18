


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
