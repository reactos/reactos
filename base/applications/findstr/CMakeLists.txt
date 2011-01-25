
add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(findstr
    findstr.c
    findstr.rc)

set_module_type(findstr win32cui)
add_importlibs(findstr user32 msvcrt kernel32)
add_cab_target(findstr 1)
