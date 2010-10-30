
set_unicode()

add_definitions(-DDEBUG_NT4)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

spec2def(sndblst.dll sndblst.spec)

add_library(sndblst SHARED sndblst.c ${CMAKE_CURRENT_BINARY_DIR}/sndblst.def)

set_module_type(sndblst win32dll)

target_link_libraries(sndblst
    mment4
    mmebuddy)

add_importlibs(sndblst user32 winmm advapi32 kernel32 ntdll)

add_cab_target(sndblst 1)
