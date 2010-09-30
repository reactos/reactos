add_executable(xcopy
    xcopy.c
    rsrc.rc)

set_module_type(xcopy win32cui)

set_unicode(xcopy yes)

add_importlibs(xcopy shell32 user32 msvcrt ntdll)

target_link_libraries(xcopy wine)