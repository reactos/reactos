
set_unicode()

include_directories(${REACTOS_BINARY_DIR}/include/reactos)

spec2def(netevent ${CMAKE_CURRENT_SOURCE_DIR}/netevent.spec ${CMAKE_CURRENT_BINARY_DIR}/netevent.def)

add_library(netevent SHARED netevent.c netevt.rc)

set_module_type(netevent win32dll)

target_link_libraries(netevent
    ${CMAKE_CURRENT_BINARY_DIR}/netevent.def)

add_importlibs(netevent kernel32)

add_dependencies(netevent netevent_def neteventmsg)
add_cab_target(netevent 1)
