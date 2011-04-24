
add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(subst subst.c subst.rc)

set_module_type(subst win32cui)
add_importlibs(subst msvcrt kernel32 ntdll)
add_cab_target(subst 1)
