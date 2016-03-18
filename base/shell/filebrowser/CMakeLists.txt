
PROJECT(SHELL)

list(APPEND SOURCE
    filebrowser.c
    filebrowser.rc)

add_executable(filebrowser ${SOURCE})
set_module_type(filebrowser win32gui UNICODE)
add_importlibs(filebrowser shell32 browseui msvcrt kernel32)

add_cd_file(TARGET filebrowser DESTINATION reactos FOR all)
