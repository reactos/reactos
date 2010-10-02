
add_executable(dbgprint dbgprint.c)

set_module_type(dbgprint win32cui)
add_importlibs(dbgprint msvcrt)
