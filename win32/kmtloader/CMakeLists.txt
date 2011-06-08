add_executable(kmtloader kmtloader.c)
set_module_type(kmtloader win32cui)
add_importlibs(kmtloader msvcrt kernel32 advapi32)
add_cd_file(TARGET kmtloader DESTINATION reactos/bin FOR all)
