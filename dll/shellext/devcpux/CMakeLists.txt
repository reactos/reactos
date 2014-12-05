
spec2def(devcpux.dll devcpux.spec)
add_library(devcpux SHARED processor.c processor.rc ${CMAKE_CURRENT_BINARY_DIR}/devcpux.def)
set_module_type(devcpux win32dll UNICODE)
add_importlibs(devcpux user32 powrprof comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET devcpux DESTINATION reactos/system32 FOR all)
