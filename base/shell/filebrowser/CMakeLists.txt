
PROJECT(SHELL)

list(APPEND SOURCE
    filebrowser.c
    filebrowser.rc)

add_executable(filebrowser ${SOURCE})
set_module_type(filebrowser win32gui UNICODE)
add_importlibs(filebrowser
    shell32
    msvcrt
    kernel32)

add_cd_file(TARGET explorer DESTINATION reactos FOR all)add_cd_file(TARGET explorer DESTINATION reactos FOR all)