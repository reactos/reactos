add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(find find.c find.rc)

set_module_type(find win32cui)
add_importlibs(find user32 msvcrt kernel32)

add_cab_target(find 1)