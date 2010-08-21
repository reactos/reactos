
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)
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
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/gdi32/libgdi32.a
    -lcomctl32
    -lole32
    -loleaut32
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
    -luxtheme
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(aclui ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(aclui aclui_def psdk)
