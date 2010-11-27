
set_unicode()

include_directories(${REACTOS_BINARY_DIR}/include/reactos)

spec2def(netevent.dll netevent.spec)

add_library(netevent SHARED netevent.c netevt.rc ${CMAKE_CURRENT_BINARY_DIR}/netevent.def)

set_module_type(netevent win32dll)

add_importlibs(netevent msvcrt kernel32)

add_dependencies(netevent neteventmsg)
add_cab_target(netevent 1)
