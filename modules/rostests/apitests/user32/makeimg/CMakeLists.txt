PROJECT(MAKEIMG)

add_executable(showimg showimg.c)

set_module_type(showimg win32gui UNICODE)
add_importlibs(showimg gdi32 user32 shell32 msvcrt kernel32 ntdll)

add_library(image MODULE image.rc)
set_module_type(image win32dll ENTRYPOINT 0)


add_rostests_file(TARGET showimg)
add_rostests_file(TARGET image)
