
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE hid.c stubs.c hid.rc)

spec2def(hid ${CMAKE_CURRENT_SOURCE_DIR}/hid.spec ${CMAKE_CURRENT_BINARY_DIR}/hid.def)

add_library(hid SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/hid_precomp.h.gch)

set_target_properties(hid PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(hid
    ${CMAKE_CURRENT_BINARY_DIR}/hid.def
    -lkernel32
    -lntdll)

add_pch(hid ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(hid hid_def psdk)
