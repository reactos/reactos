
add_definitions(-DDEBUG_NT4)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound)

list(APPEND SOURCE
    sndblst.c)

add_library(sndblst MODULE ${SOURCE})
spec2def(sndblst sndblst.dll sndblst.spec)

set_module_type(sndblst win32dll UNICODE)
target_link_libraries(sndblst mment4 mmebuddy)
add_importlibs(sndblst user32 winmm advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET sndblst DESTINATION reactos/system32 FOR all)
