add_executable(hackssign_client client.c hackssign.rc)
set_module_type(hackssign_client win32cui UNICODE)
add_importlibs(hackssign_client msvcrt kernel32 advapi32)
add_cd_file(TARGET hackssign_client DESTINATION reactos/bin FOR all)

add_library(hackssign_driver SHARED driver.c hackssign.rc)
set_module_type(hackssign_driver kernelmodedriver)
add_importlibs(hackssign_driver ntoskrnl hal)
add_cd_file(TARGET hackssign_driver DESTINATION reactos/bin FOR all)
