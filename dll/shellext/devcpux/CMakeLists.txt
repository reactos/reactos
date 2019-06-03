
add_library(devcpux MODULE processor.c processor.rc)
spec2def(devcpux devcpux.dll devcpux.spec)

set_module_type(devcpux win32dll UNICODE)
add_importlibs(devcpux user32 powrprof comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET devcpux DESTINATION reactos/system32 FOR all)
