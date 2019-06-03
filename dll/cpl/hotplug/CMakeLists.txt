
list(APPEND SOURCE
    hotplug.c
    enum.c)

add_library(hotplug MODULE
    ${SOURCE}
    hotplug.rc)
spec2def(hotplug hotplug.dll hotplug.spec)

set_module_type(hotplug win32dll UNICODE)
add_importlibs(hotplug user32 gdi32 advapi32 setupapi comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET hotplug DESTINATION reactos/system32 FOR all)
