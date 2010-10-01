
add_definitions(-DUNICODE -D_UNICODE)

add_library(wsock32 SHARED stubs.c wsock32.rc)

set_module_type(wsock32 win32dll)

target_link_libraries(wsock32 ${CMAKE_CURRENT_SOURCE_DIR}/wsock32.def)
add_importlibs(wsock32 ntdll ws2_32)
add_dependencies(wsock32 psdk)
