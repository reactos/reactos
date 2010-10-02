
set_unicode()

add_definitions(-DSUPPORT_UXTHEME)

file(GLOB_RECURSE SOURCE *.c)

spec2def(aclui ${CMAKE_CURRENT_SOURCE_DIR}/aclui.spec ${CMAKE_CURRENT_BINARY_DIR}/aclui.def)

add_library(aclui SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/aclui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/aclui_precomp.h.gch)

set_target_properties(aclui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(aclui
    ${CMAKE_CURRENT_BINARY_DIR}/aclui.def
    -luser32
    -lgdi32
    -lcomctl32
    -lole32
    -loleaut32
    -ladvapi32
    -luxtheme
    -lkernel32
    -lntdll)

add_pch(aclui ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(aclui aclui_def psdk buildno_header)
