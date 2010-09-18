


add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCE *.c)

spec2def(authz ${CMAKE_CURRENT_SOURCE_DIR}/authz.spec ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

add_library(authz SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/authz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/authz_precomp.h.gch)

set_target_properties(authz PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(authz
    ${CMAKE_CURRENT_BINARY_DIR}/authz.def
    -ladvapi32
    -lkernel32
    -lntdll)

add_pch(authz ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(authz authz_def psdk)
