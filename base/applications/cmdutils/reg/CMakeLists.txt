add_definitions(-D_WIN32_WINNT=0x600)

add_executable(reg
    reg.c
    rsrc.rc)

set_module_type(reg win32cui)

set_unicode(reg yes)

add_importlibs(reg advapi32 user32 msvcrt)