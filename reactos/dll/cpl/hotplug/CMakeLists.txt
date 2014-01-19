

spec2def(hotplug.dll hotplug.spec)

list(APPEND SOURCE
    hotplug.c
    enum.c
    hotplug.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hotplug.def)

add_library(hotplug SHARED ${SOURCE})

set_module_type(hotplug win32dll UNICODE)

add_importlibs(hotplug
    msvcrt
    user32
    gdi32
    advapi32
    ntdll
    setupapi
    comctl32
    kernel32)

add_cd_file(TARGET hotplug DESTINATION reactos/system32 FOR all)
