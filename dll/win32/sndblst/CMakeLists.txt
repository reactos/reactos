
set_unicode()

add_definitions(-DDEBUG_NT4)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(sndblst SHARED sndblst.c sndblst.def)

if(NOT MSVC)
    set_source_files_properties(sndblst.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(sndblst win32dll)

target_link_libraries(sndblst
    mment4
    mmebuddy)

add_importlibs(sndblst user32 winmm advapi32 msvcrt kernel32 ntdll)

add_cab_target(sndblst 1)
