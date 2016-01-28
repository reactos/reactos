
spec2def(testvdd.dll testvdd.spec)

list(APPEND SOURCE
    testvdd.c
    testvdd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/testvdd.def)

add_library(testvdd SHARED ${SOURCE})
#set_module_type(testvdd win32dll UNICODE ENTRYPOINT VDDInitialize)
set_module_type(testvdd win32dll UNICODE)

target_link_libraries(testvdd ${PSEH_LIB})

add_importlibs(testvdd ntvdm user32 msvcrt kernel32 ntdll)
#add_cd_file(TARGET testvdd DESTINATION reactos/system32 FOR all)
