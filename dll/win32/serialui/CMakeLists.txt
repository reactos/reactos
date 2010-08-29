
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

spec2def(serialui ${CMAKE_CURRENT_SOURCE_DIR}/serialui.spec ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

add_library(serialui SHARED serialui.c serialui.rc)

set_target_properties(serialui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(serialui
    ${CMAKE_CURRENT_BINARY_DIR}/serialui.def
    -luser32
    -lshlwapi
    -lkernel32)

add_dependencies(serialui serialui_def psdk)
