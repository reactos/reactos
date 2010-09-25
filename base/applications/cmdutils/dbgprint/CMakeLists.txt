add_executable(dbgprint dbgprint.c)

set_module_type(dbgprint win32cui)

set_unicode(dbgprint no)

add_importlibs(dbgprint msvcrt)